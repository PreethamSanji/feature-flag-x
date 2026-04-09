#include "middleware/jwt_filter.h"
#include "services/service_registry.h"
#include <drogon/HttpResponse.h>

namespace ffx {

void JwtFilter::doFilter(const drogon::HttpRequestPtr& req,
                         drogon::FilterCallback&& cb,
                         drogon::FilterChainCallback&& ccb) {
    auto auth_header = req->getHeader("Authorization");
    if (auth_header.empty() || auth_header.substr(0, 7) != "Bearer ") {
        auto resp = drogon::HttpResponse::newHttpJsonResponse(
            nlohmann::json{{"error", "Missing or invalid Authorization header"}});
        resp->setStatusCode(drogon::k401Unauthorized);
        cb(resp);
        return;
    }

    auto token = auth_header.substr(7);
    auto& auth = ServiceRegistry::instance().auth_service();
    auto result = auth.validate_token(token);

    if (!result.has_value()) {
        auto resp = drogon::HttpResponse::newHttpJsonResponse(
            nlohmann::json{{"error", result.error()}});
        resp->setStatusCode(drogon::k401Unauthorized);
        cb(resp);
        return;
    }

    // Store user info in request attributes for downstream handlers
    auto& user = result.value();
    req->attributes()->insert("user_id", user.id);
    req->attributes()->insert("user_email", user.email);
    req->attributes()->insert("user_role", role_to_string(user.role));

    ccb();
}

} // namespace ffx
