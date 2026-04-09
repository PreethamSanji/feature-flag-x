#pragma once

#include <nlohmann/json.hpp>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <optional>
#include <string>

namespace ffx {

/// Create an error JSON response.
inline drogon::HttpResponsePtr json_error(drogon::HttpStatusCode code, const std::string& message) {
    nlohmann::json body = {{"error", message}};
    auto resp = drogon::HttpResponse::newHttpJsonResponse(body);
    resp->setStatusCode(code);
    return resp;
}

/// Create a success JSON response.
inline drogon::HttpResponsePtr json_success(const nlohmann::json& data,
                                             drogon::HttpStatusCode code = drogon::k200OK) {
    auto resp = drogon::HttpResponse::newHttpJsonResponse(data);
    resp->setStatusCode(code);
    return resp;
}

/// Parse JSON body from a Drogon request.
inline std::optional<nlohmann::json> parse_json_body(const drogon::HttpRequestPtr& req) {
    try {
        auto body = std::string(req->body());
        if (body.empty()) return std::nullopt;
        return nlohmann::json::parse(body);
    } catch (...) {
        return std::nullopt;
    }
}

} // namespace ffx
