#pragma once

#include "common.hpp"
#include "typemapper.hpp"
#include <map>
#include <string>
#include <vector>

namespace jsonrpccxx {

  typedef std::vector<std::string> NamedParamMapping;
  static NamedParamMapping NAMED_PARAM_MAPPING;

  class Dispatcher {
  public:
    Dispatcher() :
      methods(),
      notifications(),
      mapping() {}

    bool Add(const std::string &name, MethodHandle callback, const NamedParamMapping &mapping = NAMED_PARAM_MAPPING) {
      if (Contains(name))
        return false;
      methods[name] = std::move(callback);
      if (!mapping.empty()) {
        this->mapping[name] = mapping;
      }
      return true;
    }

    bool Add(const std::string &name, NotificationHandle callback, const NamedParamMapping &mapping = NAMED_PARAM_MAPPING) {
      if (Contains(name))
        return false;
      notifications[name] = std::move(callback);
      if (!mapping.empty()) {
        this->mapping[name] = mapping;
      }
      return true;
    }

    inline bool ContainsMethod(const std::string &name) const { return (methods.find(name) != methods.end()); }

    inline bool ContainsNotification(const std::string &name) const { return (notifications.find(name) != notifications.end()); }

    inline bool Contains(const std::string &name) const { return (ContainsMethod(name) || ContainsNotification(name)); }

    bool Remove(const std::string &name) {
      if (!Contains(name))
        return false;
      methods.erase(name);
      notifications.erase(name);
      mapping.erase(name);
      return true;
    }

    std::vector<std::string> MethodNames() const {
      std::vector<std::string> names;
      for(const auto &mapPair: methods) {
        names.push_back(mapPair.first);
      }
      return names;
    }

    std::vector<std::string> NotificationNames() const {
      std::vector<std::string> names;
      for(const auto &mapPair: notifications) {
        names.push_back(mapPair.first);
      }
      return names;
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
        throw JsonRpcException(method_not_found, "method not found: " + name);
      }
      try {
        return method->second(normalize_parameter(name, params));
      } catch (json::type_error &e) {
        throw JsonRpcException(invalid_params, "invalid parameter: " + std::string(e.what()));
      } catch (JsonRpcException &e) {
        throw process_type_error(name, e);
      }
    }

    void InvokeNotification(const std::string &name, const json &params) {
      auto notification = notifications.find(name);
      if (notification == notifications.end()) {
        throw JsonRpcException(method_not_found, "notification not found: " + name);
      }
      try {
        notification->second(normalize_parameter(name, params));
      } catch (json::type_error &e) {
        throw JsonRpcException(invalid_params, "invalid parameter: " + std::string(e.what()));
      } catch (JsonRpcException &e) {
        throw process_type_error(name, e);
      }
    }

  private:
    std::map<std::string, MethodHandle> methods;
    std::map<std::string, NotificationHandle> notifications;
    std::map<std::string, NamedParamMapping> mapping;

    inline json normalize_parameter(const std::string &name, const json &params) {
      if (params.type() == json::value_t::array) {
        return params;
      } else if (params.type() == json::value_t::object) {
        if (mapping.find(name) == mapping.end()) {
          throw JsonRpcException(invalid_params, "invalid parameter: procedure doesn't support named parameter");
        }
        json result;
        for (auto const &p : mapping[name]) {
          if (params.find(p) == params.end()) {
            throw JsonRpcException(invalid_params, "invalid parameter: missing named parameter \"" + p + "\"");
          }
          result.push_back(params[p]);
        }
        return result;
      }
      throw JsonRpcException(invalid_request, "invalid request: params field must be an array, object");
    }
  };
} // namespace jsonrpccxx
