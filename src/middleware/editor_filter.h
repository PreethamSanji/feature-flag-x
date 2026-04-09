#pragma once

#include <drogon/HttpFilter.h>

namespace ffx {

/// Drogon HTTP filter that requires editor or admin role.
class EditorFilter : public drogon::HttpFilter<EditorFilter> {
public:
    void doFilter(const drogon::HttpRequestPtr& req,
                  drogon::FilterCallback&& cb,
                  drogon::FilterChainCallback&& ccb) override;
};

} // namespace ffx
