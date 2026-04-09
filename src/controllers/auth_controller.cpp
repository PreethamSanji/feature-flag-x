#include "controllers/auth_controller.h"
#include "services/service_registry.h"
#include "utils/json_helpers.h"

namespace ffx {

void AuthController::login(const drogon::HttpRequestPtr& req,
                           std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto body = parse_json_body(req);
    if (!body.has_value()) {
        callback(json_error(drogon::k400BadRequest, "Invalid JSON body"));
        return;
    }

    try {
        auto email = body->at("email").get<std::string>();
        auto password = body->at("password").get<std::string>();

        auto& auth = ServiceRegistry::instance().auth_service();
        auto result = auth.login(email, password);

        if (!result.has_value()) {
            callback(json_error(drogon::k401Unauthorized, result.error()));
            return;
        }

        callback(json_success(nlohmann::json{{"token", result.value()}}));
    } catch (const std::exception& e) {
        callback(json_error(drogon::k400BadRequest, e.what()));
    }
}

void AuthController::register_user(const drogon::HttpRequestPtr& req,
                                   std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto body = parse_json_body(req);
    if (!body.has_value()) {
        callback(json_error(drogon::k400BadRequest, "Invalid JSON body"));
        return;
    }

    try {
        auto email = body->at("email").get<std::string>();
        auto password = body->at("password").get<std::string>();
        auto role_str = body->value("role", std::string("viewer"));
        auto role = role_from_string(role_str);

        auto& auth = ServiceRegistry::instance().auth_service();
        auto result = auth.register_user(email, password, role);

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

} // namespace ffx
