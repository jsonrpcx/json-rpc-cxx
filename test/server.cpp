#include "catch/catch.hpp"
#include <iostream>
#include <jsonrpccxx/server.hpp>

using namespace jsonrpccxx;
using namespace std;
using namespace Catch::Matchers;

#define TEST_MODULE "[server]"

class TestServerConnector {
public:
  TestServerConnector(JsonRpcServer &handler) : handler(handler) {}

  void SendRawRequest(const string &request) { this->raw_response = handler.HandleRequest(request); }

  void SendRequest(const json &request) { SendRawRequest(request.dump()); }

  json BuildMethodCall(json id, const string &name, const json &params) { return {{"id", id}, {"method", name}, {"params", params}, {"jsonrpc", "2.0"}}; }

  void CallMethod(json id, const string &name, const json &params) { SendRequest(BuildMethodCall(id, name, params)); }

  json BuildNotificationCall(const string &name, const json &params) { return {{"method", name}, {"params", params}, {"jsonrpc", "2.0"}}; }

  void CallNotification(const string &name, const json &params) { SendRequest(BuildNotificationCall(name, params)); }

  json VerifyMethodResult(json id) {
    json result = json::parse(this->raw_response);
    return VerifyMethodResult(id, result);
  }

  static json VerifyMethodResult(json id, json &result) {
    REQUIRE(!has_key(result, "error"));
    REQUIRE(result["jsonrpc"] == "2.0");
    REQUIRE(result["id"] == id);
    REQUIRE(has_key(result, "result"));
    return result["result"];
  }

  json VerifyBatchResponse() {
    json result = json::parse(raw_response);
    REQUIRE(result.is_array());
    return result;
  }

  void VerifyNotificationResult() { VerifyNotificationResult(this->raw_response); }

  static void VerifyNotificationResult(string &raw_response) { REQUIRE(raw_response == ""); }

  json VerifyMethodError(int code, string message, json id) {
    json error = json::parse(this->raw_response);
    return VerifyMethodError(code, message, id, error);
  }

  static json VerifyMethodError(int code, string message, json id, json &result) {
    REQUIRE(!has_key(result, "result"));
    REQUIRE(result["jsonrpc"] == "2.0");
    REQUIRE(result["id"] == id);
    REQUIRE(has_key_type(result, "error", json::value_t::object));
    REQUIRE(has_key_type(result["error"], "code", json::value_t::number_integer));
    REQUIRE(result["error"]["code"] == code);
    REQUIRE(has_key_type(result["error"], "message", json::value_t::string));
    REQUIRE_THAT(result["error"]["message"], Contains(message));
    return result["error"];
  }

private:
  JsonRpcServer &handler;
  string raw_response;
};

struct Server2 {
  JsonRpc2Server server;
  TestServerConnector connector;

  Server2() : connector(server) {}
};

TEST_CASE_METHOD(Server2, "v2_method_not_found", TEST_MODULE) {
  connector.CallMethod(1, "some_invalid_method", nullptr);
  connector.VerifyMethodError(-32601, "method not found: some_invalid_method", 1);
}

TEST_CASE_METHOD(Server2, "v2_malformed_requests", TEST_MODULE) {
  string name = "some_method";
  json params = nullptr;

  connector.SendRawRequest("dfasdf");
  connector.VerifyMethodError(-32700, "parse error", nullptr);

  connector.SendRequest({{"id", true}, {"method", name}, {"params", params}, {"jsonrpc", "2.0"}});
  connector.VerifyMethodError(-32600, "invalid request: id field must be a number, string or null", nullptr);
  connector.SendRequest({{"id", {3}}, {"method", name}, {"params", params}, {"jsonrpc", "2.0"}});
  connector.VerifyMethodError(-32600, "invalid request: id field must be a number, string or null", nullptr);
  connector.SendRequest({{"id", {{"a", "b"}}}, {"method", name}, {"params", params}, {"jsonrpc", "2.0"}});
  connector.VerifyMethodError(-32600, "invalid request: id field must be a number, string or null", nullptr);
  connector.SendRequest({{"id", nullptr}, {"method", name}, {"params", params}, {"jsonrpc", "2.0"}});
  connector.VerifyMethodError(-32601, "method not found: some_method", nullptr);

  connector.SendRequest({{"id", 1}, {"params", params}, {"jsonrpc", "2.0"}});
  connector.VerifyMethodError(-32600, "invalid request: method field must be a string", 1);
  connector.SendRequest({{"id", 1}, {"method", 33}, {"params", params}, {"jsonrpc", "2.0"}});
  connector.VerifyMethodError(-32600, "invalid request: method field must be a string", 1);
  connector.SendRequest({{"id", 1}, {"method", true}, {"params", params}, {"jsonrpc", "2.0"}});
  connector.VerifyMethodError(-32600, "invalid request: method field must be a string", 1);

  connector.SendRequest({{"id", 1}, {"method", name}, {"params", params}, {"jsonrpc", "3.0"}});
  connector.VerifyMethodError(-32600, R"(invalid request: missing jsonrpc field set to "2.0")", 1);
  connector.SendRequest({{"id", 1}, {"method", name}, {"params", params}, {"jsonrpc", nullptr}});
  connector.VerifyMethodError(-32600, R"(invalid request: missing jsonrpc field set to "2.0")", 1);
  connector.SendRequest({{"id", 1}, {"method", name}, {"params", params}});
  connector.VerifyMethodError(-32600, R"(invalid request: missing jsonrpc field set to "2.0")", 1);

  connector.SendRequest({{"id", 1}, {"method", name}, {"jsonrpc", "2.0"}});
  connector.VerifyMethodError(-32601, "method not found: some_method", 1);
  connector.SendRequest({{"id", 1}, {"method", name}, {"params", true}, {"jsonrpc", "2.0"}});
  connector.VerifyMethodError(-32600, "invalid request: params field must be an array, object or null", 1);
}

enum class category { ord, cc };

NLOHMANN_JSON_SERIALIZE_ENUM(category, {{category::ord, "order"}, {category::cc, "cc"}});

struct product {
  int id;
  double price;
  string name;
  category cat;
};

void to_json(json &j, const product &p);
void from_json(const json &j, product &p);

class TestServer {
public:
  unsigned int add_function(unsigned int a, unsigned int b) {
    this->param_a = a;
    this->param_b = b;
    return a + b;
  }
  unsigned int div_function(unsigned int a, unsigned int b) {
    this->param_a = a;
    this->param_b = b;
    if (b != 0)
      return a / b;
    else
      throw JsonRpcException(-32602, "b must not be 0");
  }
  void some_procedure(const string &param) { param_proc = param; }
  bool add_products(const vector<product> &products) {
    if (products.empty())
      return false;
    std::copy(products.begin(), products.end(), std::back_inserter(catalog));
    return true;
  };

  void dirty_notification() { throw std::exception(); }
  int dirty_method(int a, int b) { throw std::exception(); }
  int dirty_method2(int a, int b) { throw - 7; }

  string param_proc;
  int param_a;
  int param_b;
  vector<product> catalog;
};

TEST_CASE_METHOD(Server2, "v2_invocations", TEST_MODULE) {
  TestServer t;
  REQUIRE(server.Add("add_function", GetHandle(&TestServer::add_function, t), {"a", "b"}));
  REQUIRE(server.Add("div_function", GetHandle(&TestServer::div_function, t), {"a", "b"}));
  REQUIRE(server.Add("some_procedure", GetHandle(&TestServer::some_procedure, t), {"param"}));
  REQUIRE(server.Add("add_products", GetHandle(&TestServer::add_products, t), {"products"}));
  REQUIRE(server.Add("dirty_notification", GetHandle(&TestServer::dirty_notification, t), {"products"}));
  REQUIRE(server.Add("dirty_method", GetHandle(&TestServer::dirty_method, t), {"a", "b"}));
  REQUIRE(server.Add("dirty_method2", GetHandle(&TestServer::dirty_method2, t), {"a", "b"}));

  REQUIRE(!server.Add("dirty_method2", GetHandle(&TestServer::dirty_method2, t), {"a", "b"}));
  REQUIRE(!server.Add("rpc.something", GetHandle(&TestServer::dirty_method2, t), {"a", "b"}));
  REQUIRE(!server.Add("rpc.", GetHandle(&TestServer::dirty_method2, t), {"a", "b"}));
  REQUIRE(server.Add("rpc", GetHandle(&TestServer::dirty_method2, t), {"a", "b"}));

  connector.CallMethod(1, "add_function", {{"a", 3}, {"b", 4}});
  CHECK(connector.VerifyMethodResult(1) == 7);
  CHECK(t.param_a == 3);
  CHECK(t.param_b == 4);

  connector.CallNotification("some_procedure", {{"param", "something set"}});
  connector.VerifyNotificationResult();
  CHECK(t.param_proc == "something set");

  json params = json::parse(
      R"({"products": [{"id": 1, "price": 23.3, "name": "some product", "category": "cc"},{"id": 2, "price": 23.4, "name": "some product 2", "category": "order"}]})");

  connector.CallMethod(1, "add_products", params);
  CHECK(connector.VerifyMethodResult(1) == true);
  CHECK(t.catalog.size() == 2);
  CHECK(t.catalog[0].id == 1);
  CHECK(t.catalog[0].name == "some product");
  CHECK(t.catalog[0].price == 23.3);
  CHECK(t.catalog[0].cat == category::cc);
  CHECK(t.catalog[1].id == 2);
  CHECK(t.catalog[1].name == "some product 2");
  CHECK(t.catalog[1].price == 23.4);
  CHECK(t.catalog[1].cat == category::ord);

  connector.CallNotification("dirty_notification", nullptr);
  connector.VerifyNotificationResult();
  connector.CallMethod(1, "dirty_method", {{"a", 3}, {"b", 0}});
  connector.VerifyMethodError(-32603, "internal server error", 1);
  connector.CallMethod(1, "div_function", {{"a", 3}, {"b", 0}});
  connector.VerifyMethodError(-32602, "b must not be 0", 1);
  connector.CallMethod(1, "dirty_method2", {{"a", 3}, {"b", 0}});
  connector.VerifyMethodError(-32603, "internal server error", 1);
}

TEST_CASE_METHOD(Server2, "v2_batch") {
  TestServer t;
  REQUIRE(server.Add("add_function", GetHandle(&TestServer::add_function, t), {"a", "b"}));

  json batchcall;

  batchcall.push_back(connector.BuildMethodCall(1, "add_function", {{"a", 3}, {"b", 4}}));
  batchcall.push_back(connector.BuildMethodCall(2, "add_function", {{"a", 300}, {"b", 4}}));
  batchcall.push_back(connector.BuildMethodCall(3, "add_function", {{"a", 300}}));
  batchcall.push_back("");

  connector.SendRequest(batchcall);
  json batchresponse = connector.VerifyBatchResponse();
  REQUIRE(batchresponse.size() == 4);

  REQUIRE(TestServerConnector::VerifyMethodResult(1, batchresponse[0]) == 7);
  REQUIRE(TestServerConnector::VerifyMethodResult(2, batchresponse[1]) == 304);
  TestServerConnector::VerifyMethodError(-32602, R"(missing named parameter "b")", 3, batchresponse[2]);
  TestServerConnector::VerifyMethodError(-32600, R"(invalid request)", nullptr, batchresponse[3]);

  connector.SendRawRequest("[]");
  batchresponse = connector.VerifyBatchResponse();
  REQUIRE(batchresponse.size() == 0);
}
