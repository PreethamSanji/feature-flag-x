#include "middleware/api_key_filter.h"
#include "services/service_registry.h"
#include <drogon/HttpResponse.h>

namespace ffx {

void ApiKeyFilter::doFilter(const drogon::HttpRequestPtr& req,
                            drogon::FilterCallback&& cb,
                            drogon::FilterChainCallback&& ccb) {
    auto api_key = req->getHeader("X-API-Key");
    if (api_key.empty()) {
        auto resp = drogon::HttpResponse::newHttpJsonResponse(
            nlohmann::json{{"error", "Missing X-API-Key header"}});
        resp->setStatusCode(drogon::k401Unauthorized);
        cb(resp);
        return;
    }

    auto& auth = ServiceRegistry::instance().auth_service();
    auto result = auth.validate_api_key(api_key);

    if (!result.has_value()) {
        auto resp = drogon::HttpResponse::newHttpJsonResponse(
            nlohmann::json{{"error", result.error()}});
        resp->setStatusCode(drogon::k401Unauthorized);
        cb(resp);
        return;
    }

    auto& user = result.value();
    req->attributes()->insert("user_id", user.id);
    req->attributes()->insert("user_email", user.email);
    req->attributes()->insert("user_role", role_to_string(user.role));

    ccb();
}

} // namespace ffx
