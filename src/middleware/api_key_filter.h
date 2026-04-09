#pragma once

#include <drogon/HttpFilter.h>

namespace ffx {

/// Drogon HTTP filter that validates X-API-Key headers.
class ApiKeyFilter : public drogon::HttpFilter<ApiKeyFilter> {
public:
    void doFilter(const drogon::HttpRequestPtr& req,
                  drogon::FilterCallback&& cb,
                  drogon::FilterChainCallback&& ccb) override;
};

} // namespace ffx
