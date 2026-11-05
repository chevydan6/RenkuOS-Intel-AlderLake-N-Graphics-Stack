// SPDX-License-Identifier: MIT
// Minimal MCP stdio server. Run as an authenticated SSH subprocess.
#include "json11/json11.hpp"
#include <sys/utsname.h>
#include <cstdio>
#include <cstring>
#include <string>

using json11::Json;
using Object = Json::object;
using Array = Json::array;

static void send(const Json& value) {
    const std::string line = value.dump();
    std::fwrite(line.data(), 1, line.size(), stdout);
    std::fputc('\n', stdout);
    std::fflush(stdout);
}
static Json error(const Json& id, int code, const char* message) {
    return Object{{"jsonrpc", "2.0"}, {"id", id},
        {"error", Object{{"code", code}, {"message", message}}}};
}
static Json result(const Json& id, const Json& value) {
    return Object{{"jsonrpc", "2.0"}, {"id", id}, {"result", value}};
}
int main() {
    static char line[1024 * 1024 + 2];
    bool initialized = false;
    bool ready = false;
    while (std::fgets(line, sizeof(line), stdin)) {
        size_t length = std::strlen(line);
        if (length == sizeof(line) - 1 && line[length - 1] != '\n') {
            int ch; while ((ch = std::getchar()) != '\n' && ch != EOF) {}
            send(error(Json(), -32700, "Message too large"));
            continue;
        }
        std::string parseError;
        Json request = Json::parse(std::string(line, length), parseError);
        if (!parseError.empty()) { send(error(Json(), -32700, "Parse error")); continue; }
        if (!request.is_object() || request["jsonrpc"].string_value() != "2.0"
            || !request["method"].is_string()) {
            send(error(Json(), -32600, "Invalid request")); continue;
        }
        const auto& object = request.object_items();
        bool hasId = object.find("id") != object.end();
        Json id = request["id"];
        if (hasId && !id.is_string() && !id.is_number()) {
            send(error(Json(), -32600, "Invalid request ID")); continue;
        }
        const std::string method = request["method"].string_value();
        if (!hasId) {
            if (method == "notifications/initialized" && initialized) ready = true;
            continue;
        }
        if (method == "initialize") {
            if (initialized || !request["params"].is_object()
                || !request["params"]["protocolVersion"].is_string()) {
                send(error(id, -32600, "Invalid initialization")); continue;
            }
            initialized = true;
            send(result(id, Object{{"protocolVersion", "2025-06-18"},
                {"capabilities", Object{{"tools", Object{}}}},
                {"serverInfo", Object{{"name", "renku-mcp"}, {"version", "0.1.0"}}}}));
        } else if (method == "ping") {
            send(result(id, Object{}));
        } else if (!ready) {
            send(error(id, -32002, "Server not initialized"));
        } else if (method == "tools/list") {
            Json schema = Object{{"type", "object"}, {"properties", Object{}},
                {"additionalProperties", false}};
            Json tool = Object{{"name", "system_info"},
                {"description", "Read OS and machine information"}, {"inputSchema", schema},
                {"annotations", Object{{"readOnlyHint", true}}}};
            send(result(id, Object{{"tools", Array{tool}}}));
        } else if (method == "tools/call") {
            Json params = request["params"];
            if (!params.is_object() || params["name"].string_value() != "system_info"
                || (!params["arguments"].is_null()
                    && (!params["arguments"].is_object() || !params["arguments"].object_items().empty()))) {
                send(error(id, -32602, "Unknown tool or invalid arguments")); continue;
            }
            struct utsname info;
            if (uname(&info) != 0) {
                send(result(id, Object{{"content", Array{Object{{"type", "text"},
                    {"text", "Unable to read system information"}}}}, {"isError", true}}));
                continue;
            }
            Json data = Object{{"sysname", info.sysname}, {"release", info.release},
                {"version", info.version}, {"machine", info.machine}, {"nodename", info.nodename}};
            send(result(id, Object{{"content", Array{Object{{"type", "text"}, {"text", data.dump()}}}}}));
        } else {
            send(error(id, -32601, "Method not found"));
        }
    }
    return std::ferror(stdin) ? 1 : 0;
}
