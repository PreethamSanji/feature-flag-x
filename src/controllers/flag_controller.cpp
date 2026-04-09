#include "controllers/flag_controller.h"
#include "services/service_registry.h"
#include "utils/json_helpers.h"
#include <format>

namespace ffx {

void FlagController::list_flags(const drogon::HttpRequestPtr& req,
                                std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto env = req->getParameter("environment");
    auto& svc = ServiceRegistry::instance().flag_service();

    auto result = svc.list_flags(env);
    if (!result.has_value()) {
        callback(json_error(drogon::k500InternalServerError, result.error()));
        return;
    }

    nlohmann::json flags_json = result.value();
    callback(json_success(flags_json));
}

void FlagController::get_flag(const drogon::HttpRequestPtr& req,
                              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                              const std::string& key) {
    auto env = req->getParameter("environment");
    if (env.empty()) env = "production";

    // Try cache first
    auto& cache = ServiceRegistry::instance().cache_service();
    auto cached = cache.get_flag(env, key);
    if (cached.has_value()) {
        nlohmann::json j = cached.value();
        auto resp = json_success(j);
        resp->addHeader("ETag", std::format("\"{}\"", cached->version));
        resp->addHeader("X-Cache", "HIT");

        auto if_none_match = req->getHeader("If-None-Match");
        if (if_none_match == std::format("\"{}\"", cached->version)) {
            auto not_modified = drogon::HttpResponse::newHttpResponse();
            not_modified->setStatusCode(drogon::k304NotModified);
            callback(not_modified);
            return;
        }

        callback(resp);
        return;
    }

    auto& svc = ServiceRegistry::instance().flag_service();
    auto result = svc.get_flag(key, env);
    if (!result.has_value()) {
        callback(json_error(drogon::k500InternalServerError, result.error()));
        return;
    }
    if (!result->has_value()) {
        callback(json_error(drogon::k404NotFound, "Flag not found"));
        return;
    }

    auto& flag = result->value();
    cache.set_flag(flag);

    nlohmann::json j = flag;
    auto resp = json_success(j);
    resp->addHeader("ETag", std::format("\"{}\"", flag.version));
    resp->addHeader("X-Cache", "MISS");
    callback(resp);
}

void FlagController::create_flag(const drogon::HttpRequestPtr& req,
                                 std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto body = parse_json_body(req);
    if (!body.has_value()) {
        callback(json_error(drogon::k400BadRequest, "Invalid JSON body"));
        return;
    }

    try {
        Flag flag = body->get<Flag>();
        auto user_id = req->attributes()->get<std::string>("user_id");
        auto& svc = ServiceRegistry::instance().flag_service();

        auto result = svc.create_flag(flag, user_id);
        if (!result.has_value()) {
            callback(json_error(drogon::k400BadRequest, result.error()));
            return;
        }

        nlohmann::json j = result.value();
        callback(json_success(j, drogon::k201Created));
    } catch (const std::exception& e) {
        callback(json_error(drogon::k400BadRequest, e.what()));
    }
}

void FlagController::update_flag(const drogon::HttpRequestPtr& req,
                                 std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                 const std::string& key) {
    auto body = parse_json_body(req);
    if (!body.has_value()) {
        callback(json_error(drogon::k400BadRequest, "Invalid JSON body"));
        return;
    }

    try {
        Flag flag = body->get<Flag>();
        auto user_id = req->attributes()->get<std::string>("user_id");
        auto env = req->getParameter("environment");
        if (env.empty()) env = "production";

        auto& svc = ServiceRegistry::instance().flag_service();
        auto result = svc.update_flag(key, env, flag, user_id);
        if (!result.has_value()) {
            callback(json_error(drogon::k400BadRequest, result.error()));
            return;
        }

        // Invalidate cache
        ServiceRegistry::instance().cache_service().invalidate(env, key);

        nlohmann::json j = result.value();
        callback(json_success(j));
    } catch (const std::exception& e) {
        callback(json_error(drogon::k400BadRequest, e.what()));
    }
}

void FlagController::delete_flag(const drogon::HttpRequestPtr& req,
                                 std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                 const std::string& key) {
    auto user_id = req->attributes()->get<std::string>("user_id");
    auto env = req->getParameter("environment");
    if (env.empty()) env = "production";

    auto& svc = ServiceRegistry::instance().flag_service();
    auto result = svc.delete_flag(key, env, user_id);
    if (!result.has_value()) {
        callback(json_error(drogon::k404NotFound, result.error()));
        return;
    }

    ServiceRegistry::instance().cache_service().invalidate(env, key);
    callback(json_success(nlohmann::json{{"message", "Flag deleted"}}, drogon::k200OK));
}

void FlagController::toggle_flag(const drogon::HttpRequestPtr& req,
                                 std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                 const std::string& key) {
    auto user_id = req->attributes()->get<std::string>("user_id");
    auto env = req->getParameter("environment");
    if (env.empty()) env = "production";

    auto& svc = ServiceRegistry::instance().flag_service();
    auto result = svc.toggle_flag(key, env, user_id);
    if (!result.has_value()) {
        callback(json_error(drogon::k404NotFound, result.error()));
        return;
    }

    ServiceRegistry::instance().cache_service().invalidate(env, key);

    nlohmann::json j = result.value();
    callback(json_success(j));
}

void FlagController::evaluate(const drogon::HttpRequestPtr& req,
                              std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto body = parse_json_body(req);
    if (!body.has_value()) {
        callback(json_error(drogon::k400BadRequest, "Invalid JSON body"));
        return;
    }

    try {
        auto flag_key = body->at("flag_key").get<std::string>();
        auto env = body->value("environment", std::string("production"));

        std::unordered_map<std::string, std::string> context;
        if (body->contains("context")) {
            for (auto& [k, v] : body->at("context").items()) {
                context[k] = v.is_string() ? v.get<std::string>() : v.dump();
            }
        }

        // Try cache first for evaluation
        auto& cache = ServiceRegistry::instance().cache_service();
        auto cached_flag = cache.get_flag(env, flag_key);

        auto& svc = ServiceRegistry::instance().flag_service();
        auto result = svc.evaluate(flag_key, context, env);
        if (!result.has_value()) {
            callback(json_error(drogon::k404NotFound, result.error()));
            return;
        }

        nlohmann::json j = result.value();
        callback(json_success(j));
    } catch (const std::exception& e) {
        callback(json_error(drogon::k400BadRequest, e.what()));
    }
}

void FlagController::evaluate_bulk(const drogon::HttpRequestPtr& req,
                                   std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto body = parse_json_body(req);
    if (!body.has_value()) {
        callback(json_error(drogon::k400BadRequest, "Invalid JSON body"));
        return;
    }

    try {
        auto flag_keys = body->at("flag_keys").get<std::vector<std::string>>();
        auto env = body->value("environment", std::string("production"));

        std::unordered_map<std::string, std::string> context;
        if (body->contains("context")) {
            for (auto& [k, v] : body->at("context").items()) {
                context[k] = v.is_string() ? v.get<std::string>() : v.dump();
            }
        }

        auto& svc = ServiceRegistry::instance().flag_service();
        auto result = svc.evaluate_bulk(flag_keys, context, env);
        if (!result.has_value()) {
            callback(json_error(drogon::k400BadRequest, result.error()));
            return;
        }

        nlohmann::json j = result.value();
        callback(json_success(j));
    } catch (const std::exception& e) {
        callback(json_error(drogon::k400BadRequest, e.what()));
    }
}

} // namespace ffx
