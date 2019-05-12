#pragma once
#include "common.hpp"
#include "nlohmann/json.hpp"
#include <exception>
#include <string>

namespace jsonrpccpp {
  typedef nlohmann::json json;
  class JsonRpcException : public std::exception {
  public:
    JsonRpcException(int code, const std::string &message) noexcept : code(code), message(message), data(nullptr) {
      err = std::to_string(code) + ": " + message;
    }
    JsonRpcException(int code, const std::string &message, const json &data) noexcept : code(code), message(message), data(data) {
      err = std::to_string(code) + ": " + message + ", data: " + data.dump();
    }

    // TODO: user uppercase
    int Code() const { return code; }
    const std::string &Message() const { return message; }
    const json &Data() const { return data; }

    const char *what() const noexcept { return err.c_str(); }

  private:
    int code;
    std::string message;
    json data;
    std::string err;
  };
} // namespace jsonrpccpp