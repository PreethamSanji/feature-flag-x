#include "controllers/admin_controller.h"
#include "services/service_registry.h"
#include "utils/json_helpers.h"

namespace ffx {

void AdminController::get_audit_log(const drogon::HttpRequestPtr& req,
                                    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto flag_key = req->getParameter("flag_key");
    if (flag_key.empty()) {
        callback(json_error(drogon::k400BadRequest, "Missing flag_key parameter"));
        return;
    }

    int limit = 50;
    auto limit_str = req->getParameter("limit");
    if (!limit_str.empty()) {
        try { limit = std::stoi(limit_str); } catch (...) {}
    }

    auto& audit = ServiceRegistry::instance().audit_repo();
    auto result = audit.find_by_flag_key(flag_key, limit);

    if (!result.has_value()) {
        callback(json_error(drogon::k500InternalServerError, result.error()));
        return;
    }

    nlohmann::json j = result.value();
    callback(json_success(j));
}

} // namespace ffx
