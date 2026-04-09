#include "middleware/editor_filter.h"
#include "models/user.h"
#include <drogon/HttpResponse.h>

namespace ffx {

void EditorFilter::doFilter(const drogon::HttpRequestPtr& req,
                            drogon::FilterCallback&& cb,
                            drogon::FilterChainCallback&& ccb) {
    auto role_str = req->attributes()->get<std::string>("user_role");
    auto role = role_from_string(role_str);

    if (!role_has_permission(role, UserRole::editor)) {
        auto resp = drogon::HttpResponse::newHttpJsonResponse(
            nlohmann::json{{"error", "Editor access required"}});
        resp->setStatusCode(drogon::k403Forbidden);
        cb(resp);
        return;
    }

    ccb();
}

} // namespace ffx
