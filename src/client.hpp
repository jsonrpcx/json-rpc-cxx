#pragma once
#include "common.hpp"
#include "iclientconnector.hpp"
#include <exception>
#include <nlohmann/json.hpp>
#include <string>
#include <variant>

namespace jsonrpccxx {
  enum class version { v1, v2 };

  typedef std::vector<json> positional_parameter;
  typedef std::map<std::string, json> named_parameter;
  typedef std::variant<int, std::string> id_type;

  struct JsonRpcResponse {
    std::string id;
    json result;
  };

  class JsonRpcClient {
  public:
    JsonRpcClient(IClientConnector &connector, version v) : connector(connector), v(v) {}
    virtual ~JsonRpcClient() = default;

    JsonRpcResponse CallMethod(const id_type &id, const std::string &name, const positional_parameter &params = {}) { return call_method(id, name, params); }
    JsonRpcResponse CallMethodNamed(const id_type &id, const std::string &name, const named_parameter &params = {}) { return call_method(id, name, params); }

    void CallNotification(const std::string &name, const positional_parameter &params = {}) { call_notification(name, params); }
    void CallNotificationNamed(const std::string &name, const named_parameter &params = {}) { call_notification(name, params); }

  protected:
    IClientConnector &connector;

  private:
    version v;
    static inline bool has_key(const json &v, const std::string &key) { return v.find(key) != v.end(); }
    static inline bool has_key_type(const json &v, const std::string &key, json::value_t type) { return has_key(v, key) && v.at(key).type() == type; }

    static inline JsonRpcException get_error(const json &value) {
      bool has_code = has_key_type(value, "code", json::value_t::number_integer);
      bool has_message = has_key_type(value, "message", json::value_t::string);
      bool has_data = has_key(value, "data");
      if (has_code && has_message) {
        if (has_data) {
          return JsonRpcException(value["code"], value["message"], value["data"].get<json>());
        } else {
          return JsonRpcException(value["code"], value["message"]);
        }
      }
      return JsonRpcException(-32603, R"(invalid error response: "code" (negative number) and "message" (string) are required)");
    }

    JsonRpcResponse call_method(const id_type &id, const std::string &name, const json &params) {
      json j = {{"method", name}};
      if (std::get_if<int>(&id) != nullptr) {
        j["id"] = std::get<int>(id);
      } else {
        j["id"] = std::get<std::string>(id);
      }
      if (v == version::v2) {
        j["jsonrpc"] = "2.0";
      }
      if (!params.empty() && !params.is_null()) {
        j["params"] = params;
      } else if (v == version::v1) {
        j["params"] = nullptr;
      }
      try {
        json response = json::parse(connector.Send(j.dump()));
        if (has_key_type(response, "error", json::value_t::object)) {
          throw get_error(response["error"]);
        }
        if (has_key(response, "result") && has_key(response, "id")) {
          return JsonRpcResponse{response["id"].get<std::string>(), response["result"].get<json>()};
        }
        throw JsonRpcException(-32603, R"(invalid server response: neither "result" nor "error" fields found)");
      } catch (json::parse_error &e) {
        throw JsonRpcException(-32700, std::string("invalid JSON response from server: ") + e.what());
      }
    }

    void call_notification(const std::string &name, const nlohmann::json &params) {
      nlohmann::json j = {{"method", name}};
      if (v == version::v2) {
        j["jsonrpc"] = "2.0";
      } else {
        j["id"] = nullptr;
      }
      if (!params.empty() && !params.is_null()) {
        j["params"] = params;
      } else if (v == version::v1) {
        j["params"] = nullptr;
      }
      connector.Send(j.dump());
    }
  };

  class BatchRequest {
  public:
    BatchRequest() : call(json::array()) {}

    BatchRequest &AddMethodCall(const id_type &id, const std::string &name, const positional_parameter &params = {}) {
      json request = {{"method", name}, {"params", params}, {"jsonrpc", "2.0"}};
      if (std::get_if<int>(&id) != nullptr) {
        request["id"] = std::get<int>(id);
      } else {
        request["id"] = std::get<std::string>(id);
      }
      call.push_back(request);
      return *this;
    }

    BatchRequest &AddNamedMethodCall(const id_type &id, const std::string &name, const named_parameter &params = {}) {
      json request = {{"method", name}, {"params", params}, {"jsonrpc", "2.0"}};
      if (std::get_if<int>(&id) != nullptr) {
        request["id"] = std::get<int>(id);
      } else {
        request["id"] = std::get<std::string>(id);
      }
      call.push_back(request);
      return *this;
    }

    BatchRequest &AddNotificationCall(const std::string &name, const positional_parameter &params = {}) {
      call.push_back({{"method", name}, {"params", params}, {"jsonrpc", "2.0"}});
      return *this;
    }

    BatchRequest &AddNamedNotificationCall(const std::string &name, const named_parameter &params = {}) {
      call.push_back({{"method", name}, {"params", params}, {"jsonrpc", "2.0"}});
      return *this;
    }

    const json &Build() const { return call; }

  private:
    json call;
  };

  class BatchResponse {
  public:
    BatchResponse(std::map<json, json> &&results, std::map<json, JsonRpcException> &&errors) {
      this->errors = errors;
      this->results = results;
    }

    template <typename T>
    T GetResult(const json &id) {
      if (results.find(id) != results.end()) {
        // TOOD: catch type conversion error
        return results[id].get<T>();
      } else if (errors.find(id) != errors.end()) {
        throw errors[id];
      }
      throw JsonRpcException(-32700, std::string("no result found for id ") + id.dump());
    }

  private:
    std::map<json, json> results;
    std::map<json, JsonRpcException> errors;
  };

  class JsonRpc2Client : public JsonRpcClient {
  public:
    JsonRpc2Client(IClientConnector &connector) : JsonRpcClient(connector, version::v2) {}

    BatchResponse BatchCall(const BatchRequest &request) {
      try {
        json response = json::parse(connector.Send(request.Build().dump()));
        if (!response.is_array()) {
          throw JsonRpcException(-32700, std::string("invalid JSON response from server: expected array"));
        }
        std::map<json, json> results;
        std::map<json, JsonRpcException> errors;

        return BatchResponse(std::move(results), std::move(errors));
      } catch (json::parse_error &e) {
        throw JsonRpcException(-32700, std::string("invalid JSON response from server: ") + e.what());
      }
    }
  };
} // namespace jsonrpccxx