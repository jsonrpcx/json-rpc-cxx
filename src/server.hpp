#pragma once

#include "common.hpp"
#include "dispatcher.hpp"
#include <string>

namespace jsonrpccpp {
  class JsonRpcServer {
  public:
    virtual ~JsonRpcServer() = default;
    virtual std::string HandleRequest(const std::string &request) = 0;

    bool Add(const std::string &name, MethodHandle callback, const NamedParamMapping &mapping = NAMED_PARAM_MAPPING) {
      if (name.rfind("rpc.", 0) == 0)
        return false;
      return dispatcher.Add(name, callback, mapping);
    }
    bool Add(const std::string &name, NotificationHandle callback, const NamedParamMapping &mapping = NAMED_PARAM_MAPPING) {
      if (name.rfind("rpc.", 0) == 0)
        return false;
      return dispatcher.Add(name, callback, mapping);
    }

  protected:
    Dispatcher dispatcher;
    static inline bool has_key(const nlohmann::json &json, const std::string &name) { return (json.find(name) != json.end()); }
    static inline bool has_key_type(const json &v, const std::string &key, json::value_t type) { return has_key(v, key) && v.at(key).type() == type; }
  };

  class JsonRpc2Server : public JsonRpcServer {
  public:
    JsonRpc2Server() = default;
    ~JsonRpc2Server() override = default;

    std::string HandleRequest(const std::string &requestString) override {
      try {
        json request = json::parse(requestString);
        if (request.is_array()) {
          json result = json::array();
          for (json &r : request) {
            json res = this->HandleSingleRequest(r);
            if (!res.is_null()) {
              result.push_back(std::move(res));
            }
          }
          return result.dump();
        } else if (request.is_object()) {
          json res = HandleSingleRequest(request);
          if (!res.is_null()) {
            return res.dump();
          } else {
            return "";
          }
        } else {
          return json{{"id", nullptr}, {"error", {{"code", -32600}, {"message", "invalid request: expected array or object"}}}, {"jsonrpc", "2.0"}}.dump();
        }
      } catch (json::parse_error &e) {
        return json{{"id", nullptr}, {"error", {{"code", -32700}, {"message", std::string("parse error: ") + e.what()}}}, {"jsonrpc", "2.0"}}.dump();
      }
    }

  private:
    static inline bool valid_id(const json &request) {
      return has_key(request, "id") && (request["id"].is_number() || request["id"].is_string() || request["id"].is_null());
    }

    json HandleSingleRequest(json &request) {
      json id = nullptr;
      if (valid_id(request)) {
        id = request["id"];
      }
      try {
        return ProcessSingleRequest(request);
      } catch (JsonRpcException &e) {
        json error = {{"code", e.Code()}, {"message", e.Message()}};
        if (!e.Data().is_null()) {
          error["data"] = e.Data();
        }
        return json{{"id", id}, {"error", error}, {"jsonrpc", "2.0"}};
      } catch (std::exception &e) {
        return json{{"id", id}, {"error", {{"code", -32603}, {"message", std::string("internal server error: ") + e.what()}}}, {"jsonrpc", "2.0"}};
      } catch (...) {
        return json{{"id", id}, {"error", {{"code", -32603}, {"message", std::string("internal server error")}}}, {"jsonrpc", "2.0"}};
      }
    }

    json ProcessSingleRequest(json &request) {
      if (!has_key_type(request, "jsonrpc", json::value_t::string) || request["jsonrpc"] != "2.0") {
        throw JsonRpcException(-32600, R"(invalid request: missing jsonrpc field set to "2.0")");
      }
      if (!has_key_type(request, "method", json::value_t::string)) {
        throw JsonRpcException(-32600, "invalid request: method field must be a string");
      }
      if (has_key(request, "id") && !valid_id(request)) {
        throw JsonRpcException(-32600, "invalid request: id field must be a number, string or null");
      }
      if (has_key(request, "params") && !(request["params"].is_array() || request["params"].is_object() || request["params"].is_null())) {
        throw JsonRpcException(-32600, "invalid request: params field must be an array, object or null");
      }
      if (!has_key(request, "params") || has_key_type(request, "params", json::value_t::null)) {
        request["params"] = json::array();
      }
      if (!has_key(request, "id")) {
        try {
          dispatcher.InvokeNotification(request["method"], request["params"]);
          return json();
        } catch (std::exception &e) {
          return json();
        }
      } else {
        return {{"jsonrpc", "2.0"}, {"id", request["id"]}, {"result", dispatcher.InvokeMethod(request["method"], request["params"])}};
      }
    }
  };
}