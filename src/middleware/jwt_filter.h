#pragma once

#include <drogon/HttpFilter.h>

namespace ffx {

/// Drogon HTTP filter that validates JWT Bearer tokens.
class JwtFilter : public drogon::HttpFilter<JwtFilter> {
public:
    void doFilter(const drogon::HttpRequestPtr& req,
                  drogon::FilterCallback&& cb,
                  drogon::FilterChainCallback&& ccb) override;
};

} // namespace ffx
