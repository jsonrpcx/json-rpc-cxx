#include "client.hpp"
#include "catch/catch.hpp"
#include "common.hpp"
#include <iostream>

#define TEST_MODULE "[client]"

using namespace std;
using namespace jsonrpccxx;
using namespace Catch::Matchers;

bool has_key(const json &v, const std::string &key) { return v.find(key) != v.end(); }
bool has_key_type(const json &v, const std::string &key, json::value_t type) { return has_key(v, key) && v.at(key).type() == type; }

class TestClientConnector : public IClientConnector {
public:
  json request;
  string raw_response;

  std::string Send(const std::string &r) override {
    this->request = json::parse(r);
    return raw_response;
  }

  void SetResult(const json &result) {
    json response = {{"jsonrpc", "2.0"}, {"id", "1"}, {"result", result}};
    raw_response = response.dump();
  }

  void SetError(const JsonRpcException &e) {
    json response = {{"jsonrpc", "2.0"}, {"id", "1"}};
    if (!e.Data().empty()) {
      response["error"] = {{"code", e.Code()}, {"message", e.Message()}, {"data", e.Data()}};
    } else {
      response["error"] = {{"code", e.Code()}, {"message", e.Message()}};
    }
    raw_response = response.dump();
  }

  void VerifyMethodRequest(version version, const string &name, json id) {
    CHECK(request["method"] == name);
    CHECK(request["id"] == id);
    if (version == version::v2) {
      CHECK(request["jsonrpc"] == "2.0");
    } else {
      CHECK(!has_key(request, "jsonrpc"));
      CHECK(has_key(request, "params"));
    }
  }

  void VerifyNotificationRequest(version version, const string &name) {
    CHECK(request["method"] == name);
    if (version == version::v2) {
      CHECK(request["jsonrpc"] == "2.0");
      CHECK(!has_key(request, "id"));
    } else {
      CHECK(!has_key(request, "jsonrpc"));
      CHECK(request["id"].is_null());
      CHECK(has_key(request, "params"));
    }
  }
};

struct F {
  TestClientConnector c;
  JsonRpcClient clientV1;
  JsonRpcClient clientV2;
  F() : clientV1(c, version::v1), clientV2(c, version::v2) {}
};

TEST_CASE_METHOD(F, "v2_method_noparams", TEST_MODULE) {
  c.SetResult(true);
  clientV2.CallMethod("000-000-000", "some.method_1", {});
  c.VerifyMethodRequest(version::v2, "some.method_1", "000-000-000");
  CHECK(!has_key(c.request, "params"));

  c.SetResult(true);
  clientV2.CallMethod("000-000-000", "some.method_1");
  c.VerifyMethodRequest(version::v2, "some.method_1", "000-000-000");
  CHECK(!has_key(c.request, "params"));
}

TEST_CASE_METHOD(F, "v1_method_noparams", TEST_MODULE) {
  c.SetResult(true);
  clientV1.CallMethod(37, "some.method_1", {});
  c.VerifyMethodRequest(version::v1, "some.method_1", 37);
  CHECK(has_key_type(c.request, "params", json::value_t::null));

  c.SetResult(true);
  clientV1.CallMethod(37, "some.method_1");
  c.VerifyMethodRequest(version::v1, "some.method_1", 37);
  CHECK(has_key_type(c.request, "params", json::value_t::null));
}

TEST_CASE_METHOD(F, "v2_method_call_params_byname", TEST_MODULE) {
  c.SetResult(true);
  clientV2.CallMethodNamed("1", "some.method_1", {{"a", "hello"}, {"b", 77}, {"c", true}});
  c.VerifyMethodRequest(version::v2, "some.method_1", "1");
  CHECK(c.request["params"]["a"] == "hello");
  CHECK(c.request["params"]["b"] == 77);
  CHECK(c.request["params"]["c"] == true);
}

TEST_CASE_METHOD(F, "v1_method_call_params_byname", TEST_MODULE) {
  c.SetResult(true);
  clientV1.CallMethodNamed("1", "some.method_1", {{"a", "hello"}, {"b", 77}, {"c", true}});
  c.VerifyMethodRequest(version::v1, "some.method_1", "1");
  CHECK(c.request["params"]["a"] == "hello");
  CHECK(c.request["params"]["b"] == 77);
  CHECK(c.request["params"]["c"] == true);
}

TEST_CASE_METHOD(F, "v2_method_call_params_byposition", TEST_MODULE) {
  c.SetResult(true);
  clientV2.CallMethod("1", "some.method_1", {"hello", 77, true});
  c.VerifyMethodRequest(version::v2, "some.method_1", "1");
  CHECK(c.request["params"][0] == "hello");
  CHECK(c.request["params"][1] == 77);
  CHECK(c.request["params"][2] == true);
}

TEST_CASE_METHOD(F, "v1_method_call_params_byposition", TEST_MODULE) {
  c.SetResult(true);
  clientV1.CallMethod("1", "some.method_1", {"hello", 77, true});
  c.VerifyMethodRequest(version::v1, "some.method_1", "1");
  CHECK(c.request["params"][0] == "hello");
  CHECK(c.request["params"][1] == 77);
  CHECK(c.request["params"][2] == true);
}

TEST_CASE_METHOD(F, "v2_method_result_simple", TEST_MODULE) {
  c.SetResult(23);
  JsonRpcResponse r = clientV2.CallMethod("1", "some.method_1", {});
  c.VerifyMethodRequest(version::v2, "some.method_1", "1");
  CHECK(23 == r.result);
}

TEST_CASE_METHOD(F, "v1_method_result_simple", TEST_MODULE) {
  c.SetResult(23);
  JsonRpcResponse r = clientV1.CallMethod("1", "some.method_1", {});
  c.VerifyMethodRequest(version::v1, "some.method_1", "1");
  CHECK(23 == r.result);
}

TEST_CASE_METHOD(F, "v2_method_result_object", TEST_MODULE) {
  c.SetResult({{"a", 3}, {"b", 4}});
  JsonRpcResponse r = clientV2.CallMethod("1", "some.method_1", {});
  c.VerifyMethodRequest(version::v2, "some.method_1", "1");
  CHECK(r.result["a"] == 3);
  CHECK(r.result["b"] == 4);
}

TEST_CASE_METHOD(F, "v1_method_result_object", TEST_MODULE) {
  c.SetResult({{"a", 3}, {"b", 4}});
  JsonRpcResponse r = clientV1.CallMethod("1", "some.method_1", {});
  c.VerifyMethodRequest(version::v1, "some.method_1", "1");
  CHECK(r.result["a"] == 3);
  CHECK(r.result["b"] == 4);
}

TEST_CASE_METHOD(F, "v2_method_result_array", TEST_MODULE) {
  c.SetResult({2, 3, 4});
  JsonRpcResponse r = clientV2.CallMethod("1", "some.method_1", {});
  c.VerifyMethodRequest(version::v2, "some.method_1", "1");
  CHECK(r.result[0] == 2);
  CHECK(r.result[1] == 3);
  CHECK(r.result[2] == 4);
}

TEST_CASE_METHOD(F, "v1_method_result_array", TEST_MODULE) {
  c.SetResult({2, 3, 4});
  JsonRpcResponse r = clientV1.CallMethod("1", "some.method_1", {});
  c.VerifyMethodRequest(version::v1, "some.method_1", "1");
  CHECK(r.result[0] == 2);
  CHECK(r.result[1] == 3);
  CHECK(r.result[2] == 4);
}

TEST_CASE_METHOD(F, "v2_method_result_empty", TEST_MODULE) {
  c.raw_response = "{}";
  REQUIRE_THROWS_WITH(clientV2.CallMethod("1", "some.method_1", {}),
                      Contains("result") && Contains("or") && Contains("error") && Contains("invalid server response"));
  c.VerifyMethodRequest(version::v2, "some.method_1", "1");
  c.raw_response = "[]";
  REQUIRE_THROWS_WITH(clientV2.CallMethod("1", "some.method_1", {}),
                      Contains("result") && Contains("or") && Contains("error") && Contains("invalid server response"));
  c.VerifyMethodRequest(version::v2, "some.method_1", "1");
}

TEST_CASE_METHOD(F, "v1_method_result_empty", TEST_MODULE) {
  c.raw_response = "{}";
  REQUIRE_THROWS_WITH(clientV1.CallMethod("1", "some.method_1", {}),
                      Contains("result") && Contains("or") && Contains("error") && Contains("invalid server response"));
  c.VerifyMethodRequest(version::v1, "some.method_1", "1");
  c.raw_response = "[]";
  REQUIRE_THROWS_WITH(clientV1.CallMethod("1", "some.method_1", {}),
                      Contains("result") && Contains("or") && Contains("error") && Contains("invalid server response"));
  c.VerifyMethodRequest(version::v1, "some.method_1", "1");
}

TEST_CASE_METHOD(F, "v2_method_error", TEST_MODULE) {
  c.SetError(JsonRpcException{-32602, "invalid method name"});
  REQUIRE_THROWS_WITH(clientV2.CallMethod("1", "some.method_1", {}), Contains("-32602") && Contains("invalid method name") && !Contains("data"));
  c.VerifyMethodRequest(version::v2, "some.method_1", "1");
}

TEST_CASE_METHOD(F, "v2_method_error_with_data", TEST_MODULE) {
  c.SetError(JsonRpcException{-32602, "invalid method name", {1, 2}});
  REQUIRE_THROWS_WITH(clientV2.CallMethod("1", "some.method_1", {}),
                      Contains("-32602") && Contains("invalid method name") && Contains("data") && Contains("[1,2]"));
  c.VerifyMethodRequest(version::v2, "some.method_1", "1");
}

TEST_CASE_METHOD(F, "v1_method_error", TEST_MODULE) {
  c.SetError(JsonRpcException{-32602, "invalid method name"});
  REQUIRE_THROWS_WITH(clientV1.CallMethod("1", "some.method_1", {}), Contains("-32602") && Contains("invalid method name") && !Contains("data"));
  c.VerifyMethodRequest(version::v1, "some.method_1", "1");
}

TEST_CASE_METHOD(F, "v1_method_error_with_data", TEST_MODULE) {
  c.SetError(JsonRpcException{-32602, "invalid method name", {1, 2}});
  REQUIRE_THROWS_WITH(clientV1.CallMethod("1", "some.method_1", {}),
                      Contains("-32602") && Contains("invalid method name") && Contains("data") && Contains("[1,2]"));
  c.VerifyMethodRequest(version::v1, "some.method_1", "1");
}

TEST_CASE_METHOD(F, "v2_method_error_invalid_json", TEST_MODULE) {
  c.raw_response = "{asdfasdf,[}";
  REQUIRE_THROWS_WITH(clientV2.CallMethod("1", "some.method_1", {}), Contains("-32700") && Contains("invalid") && Contains("JSON") && Contains("server"));
  c.VerifyMethodRequest(version::v2, "some.method_1", "1");
  c.raw_response = " ";
  REQUIRE_THROWS_WITH(clientV2.CallMethod("1", "some.method_1", {}), Contains("-32700") && Contains("invalid") && Contains("JSON") && Contains("server"));
  c.VerifyMethodRequest(version::v2, "some.method_1", "1");
  c.raw_response = "";
  REQUIRE_THROWS_WITH(clientV2.CallMethod("1", "some.method_1", {}), Contains("-32700") && Contains("invalid") && Contains("JSON") && Contains("server"));
  c.VerifyMethodRequest(version::v2, "some.method_1", "1");
}

TEST_CASE_METHOD(F, "v1_method_error_invalid_json", TEST_MODULE) {
  c.raw_response = "{asdfasdf,[}";
  REQUIRE_THROWS_WITH(clientV1.CallMethod("1", "some.method_1", {}), Contains("-32700") && Contains("invalid") && Contains("JSON") && Contains("server"));
  c.VerifyMethodRequest(version::v1, "some.method_1", "1");
  c.raw_response = " ";
  REQUIRE_THROWS_WITH(clientV1.CallMethod("1", "some.method_1", {}), Contains("-32700") && Contains("invalid") && Contains("JSON") && Contains("server"));
  c.VerifyMethodRequest(version::v1, "some.method_1", "1");
  c.raw_response = "";
  REQUIRE_THROWS_WITH(clientV1.CallMethod("1", "some.method_1", {}), Contains("-32700") && Contains("invalid") && Contains("JSON") && Contains("server"));
  c.VerifyMethodRequest(version::v1, "some.method_1", "1");
}

TEST_CASE_METHOD(F, "v2_notification_call_no_params", TEST_MODULE) {
  c.raw_response = "";
  clientV2.CallNotification("some.notification_1", {});
  c.VerifyNotificationRequest(version::v2, "some.notification_1");
  CHECK(!has_key(c.request, "params"));

  c.raw_response = "";
  clientV2.CallNotification("some.notification_1");
  c.VerifyNotificationRequest(version::v2, "some.notification_1");
  CHECK(!has_key(c.request, "params"));
}

TEST_CASE_METHOD(F, "v1_notification_call_no_params", TEST_MODULE) {
  c.raw_response = "";
  clientV1.CallNotification("some.notification_1", {});
  c.VerifyNotificationRequest(version::v1, "some.notification_1");
  CHECK(has_key_type(c.request, "params", json::value_t::null));

  c.raw_response = "";
  clientV1.CallNotification("some.notification_1");
  c.VerifyNotificationRequest(version::v1, "some.notification_1");
  CHECK(has_key_type(c.request, "params", json::value_t::null));
}

TEST_CASE_METHOD(F, "v2_notification_call_params_byname", TEST_MODULE) {
  c.raw_response = "";
  clientV2.CallNotificationNamed("some.notification_1", {{"a", "hello"}, {"b", 77}, {"c", true}});
  c.VerifyNotificationRequest(version::v2, "some.notification_1");
  CHECK(c.request["params"]["a"] == "hello");
  CHECK(c.request["params"]["b"] == 77);
  CHECK(c.request["params"]["c"] == true);
}

TEST_CASE_METHOD(F, "v1_notification_call_params_byname", TEST_MODULE) {
  c.raw_response = "";
  clientV1.CallNotificationNamed("some.notification_1", {{"a", "hello"}, {"b", 77}, {"c", true}});
  c.VerifyNotificationRequest(version::v1, "some.notification_1");
  CHECK(c.request["params"]["a"] == "hello");
  CHECK(c.request["params"]["b"] == 77);
  CHECK(c.request["params"]["c"] == true);
}

TEST_CASE_METHOD(F, "v2_notification_call_params_byposition", TEST_MODULE) {
  c.raw_response = "";
  clientV2.CallNotification("some.notification_1", {"hello", 77, true});
  c.VerifyNotificationRequest(version::v2, "some.notification_1");
  CHECK(c.request["params"][0] == "hello");
  CHECK(c.request["params"][1] == 77);
  CHECK(c.request["params"][2] == true);
}

TEST_CASE_METHOD(F, "v1_notification_call_params_byposition", TEST_MODULE) {
  c.raw_response = "";
  clientV1.CallNotification("some.notification_1", {"hello", 77, true});
  c.VerifyNotificationRequest(version::v1, "some.notification_1");
  CHECK(c.request["params"][0] == "hello");
  CHECK(c.request["params"][1] == 77);
  CHECK(c.request["params"][2] == true);
}

enum class category2 { order, cash_carry };
struct product2 {
  int id;
  double price;
  string name;
  category2 cat;
};

NLOHMANN_JSON_SERIALIZE_ENUM(category2, {{category2::order, "order"}, {category2::cash_carry, "cc"}});
void to_json(json &j, const product2 &p) { j = json{{"id", p.id}, {"price", p.price}, {"name", p.name}, {"category", p.cat}}; }
void from_json(const json &j, product2 &p) {
  j.at("name").get_to(p.name);
  j.at("id").get_to(p.id);
  j.at("price").get_to(p.price);
  j.at("category").get_to(p.cat);
}

// TODO: test cases with return type mapping and param mapping for v1/v2 method and notification