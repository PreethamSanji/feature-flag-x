#pragma once

#include <drogon/HttpFilter.h>

namespace ffx {

/// Drogon HTTP filter that requires admin role.
class AdminFilter : public drogon::HttpFilter<AdminFilter> {
public:
    void doFilter(const drogon::HttpRequestPtr& req,
                  drogon::FilterCallback&& cb,
                  drogon::FilterChainCallback&& ccb) override;
};

} // namespace ffx
