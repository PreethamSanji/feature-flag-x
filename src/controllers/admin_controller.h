#pragma once

#include <drogon/HttpController.h>

namespace ffx {

/// REST controller for admin operations (audit logs).
class AdminController : public drogon::HttpController<AdminController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AdminController::get_audit_log, "/api/v1/audit", drogon::Get, "ffx::JwtFilter", "ffx::AdminFilter");
    METHOD_LIST_END

    void get_audit_log(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};

} // namespace ffx
