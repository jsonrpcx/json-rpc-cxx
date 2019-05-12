#pragma once

#include "common.hpp"
#include "typemapper.hpp"
#include <map>
#include <string>

namespace jsonrpccpp {

  typedef std::vector<std::string> NamedParamMapping;
  static NamedParamMapping NAMED_PARAM_MAPPING;

  class Dispatcher {
  public:
    bool Add(const std::string &name, MethodHandle callback, const NamedParamMapping &mapping = NAMED_PARAM_MAPPING) {
      if (contains(name))
        return false;
      methods[name] = std::move(callback);
      if (!mapping.empty()) {
        this->mapping[name] = mapping;
      }
      return true;
    }

    bool Add(const std::string &name, NotificationHandle callback, const NamedParamMapping &mapping = NAMED_PARAM_MAPPING) {
      if (contains(name))
        return false;
      notifications[name] = std::move(callback);
      if (!mapping.empty()) {
        this->mapping[name] = mapping;
      }
      return true;
    }

    JsonRpcException process_type_error(const std::string &name, JsonRpcException &e) {
      if (e.Code() == -32602 && !e.Data().empty()) {
        std::string message = e.Message() + " for parameter ";
        if (this->mapping.find(name) != this->mapping.end()) {
          message += "\"" + mapping[name][e.Data().get<unsigned int>()] + "\"";
        } else {
          message += std::to_string(e.Data().get<unsigned int>());
        }
        return JsonRpcException(e.Code(), message);
      }
      return e;
    }

    json InvokeMethod(const std::string &name, const json &params) {
      auto method = methods.find(name);
      if (method == methods.end()) {
        throw JsonRpcException(-32601, "method not found: " + name);
      }
      try {
        return method->second(normalizeParameter(name, params));
      } catch (json::type_error &e) {
        throw JsonRpcException(-32602, "invalid parameter: " + std::string(e.what()));
      } catch (JsonRpcException &e) {
        throw process_type_error(name, e);
      }
    }

    void InvokeNotification(const std::string &name, const json &params) {
      auto notification = notifications.find(name);
      if (notification == notifications.end()) {
        throw JsonRpcException(-32601, "notification not found: " + name);
      }
      try {
        notification->second(normalizeParameter(name, params));
      } catch (json::type_error &e) {
        throw JsonRpcException(-32602, "invalid parameter: " + std::string(e.what()));
      } catch (JsonRpcException &e) {
        throw process_type_error(name, e);
      }
    }

    inline json normalizeParameter(const std::string &name, const json &params) {
      if (params.type() == json::value_t::array) {
        return params;
      } else if (params.type() == json::value_t::object) {
        if (mapping.find(name) == mapping.end()) {
          throw JsonRpcException(-32602, "invalid parameter: procedure doesn't support named parameter");
        }
        json result;
        for (auto const &p : mapping[name]) {
          if (params.find(p) == params.end()) {
            throw JsonRpcException(-32602, "invalid parameter: missing named parameter \"" + p + "\"");
          }
          result.push_back(params[p]);
        }
        return result;
      }
      throw JsonRpcException(-32600, "invalid request: params field must be an array, object");
    }

  private:
    std::map<std::string, MethodHandle> methods;
    std::map<std::string, NotificationHandle> notifications;
    std::map<std::string, NamedParamMapping> mapping;
    inline bool contains(const std::string &name) { return (methods.find(name) != methods.end() || notifications.find(name) != notifications.end()); }
  };
} // namespace jsonrpccpp