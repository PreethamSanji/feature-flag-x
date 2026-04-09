#pragma once

#include <drogon/HttpController.h>

namespace ffx {

/// REST controller for feature flag operations.
class FlagController : public drogon::HttpController<FlagController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(FlagController::list_flags, "/api/v1/flags", drogon::Get, "ffx::JwtFilter");
    ADD_METHOD_TO(FlagController::get_flag, "/api/v1/flags/{key}", drogon::Get, "ffx::JwtFilter");
    ADD_METHOD_TO(FlagController::create_flag, "/api/v1/flags", drogon::Post, "ffx::JwtFilter", "ffx::EditorFilter");
    ADD_METHOD_TO(FlagController::update_flag, "/api/v1/flags/{key}", drogon::Put, "ffx::JwtFilter", "ffx::EditorFilter");
    ADD_METHOD_TO(FlagController::delete_flag, "/api/v1/flags/{key}", drogon::Delete, "ffx::JwtFilter", "ffx::AdminFilter");
    ADD_METHOD_TO(FlagController::toggle_flag, "/api/v1/flags/{key}/toggle", drogon::Post, "ffx::JwtFilter", "ffx::EditorFilter");
    ADD_METHOD_TO(FlagController::evaluate, "/api/v1/evaluate", drogon::Post, "ffx::ApiKeyFilter");
    ADD_METHOD_TO(FlagController::evaluate_bulk, "/api/v1/evaluate/bulk", drogon::Post, "ffx::ApiKeyFilter");
    METHOD_LIST_END

    void list_flags(const drogon::HttpRequestPtr& req,
                    std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void get_flag(const drogon::HttpRequestPtr& req,
                  std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                  const std::string& key);
    void create_flag(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void update_flag(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                     const std::string& key);
    void delete_flag(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                     const std::string& key);
    void toggle_flag(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                     const std::string& key);
    void evaluate(const drogon::HttpRequestPtr& req,
                  std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void evaluate_bulk(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};

} // namespace ffx
