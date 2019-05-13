#include "catch/catch.hpp"
#include "server.hpp"
#include <iostream>

using namespace jsonrpccxx;
using namespace std;
using namespace Catch::Matchers;

#define TEST_MODULE "[dispatcher]"

static string procCache;
unsigned int add_function(unsigned int a, unsigned int b) { return a + b; }
void some_procedure(const string &param) { procCache = param; }

TEST_CASE("add and invoke positional", TEST_MODULE) {
  Dispatcher d;
  CHECK(d.Add("some method", GetHandle(&add_function)));
  CHECK(!d.Add("some method", GetHandle(&add_function)));
  CHECK(d.InvokeMethod("some method", {11, 22}) == 33);

  procCache = "";
  CHECK(d.Add("some notification", GetHandle(&some_procedure)));
  d.InvokeNotification("some notification", {"some string"});
  CHECK(procCache == "some string");
}

TEST_CASE("invoking supported named parameter", TEST_MODULE) {
  Dispatcher d;
  CHECK(d.Add("some method", GetHandle(&add_function), {"a", "b"}));
  REQUIRE(d.InvokeMethod("some method", {{"a", 11}, {"b", 22}}) == 33);

  procCache = "";
  CHECK(d.Add("some notification", GetHandle(&some_procedure), {"param"}));
  json p = {{"param", "some string"}};
  d.InvokeNotification("some notification", p);
  CHECK(procCache == "some string");
}

TEST_CASE("invoking missing named parameter", TEST_MODULE) {
  Dispatcher d;
  CHECK(d.Add("some method", GetHandle(&add_function), {"a", "b"}));
  REQUIRE_THROWS_WITH(d.InvokeMethod("some method", {{"a", 11}, {"xx", 22}}), Contains("-32602: invalid parameter: missing named parameter \"b\""));

  procCache = "";
  CHECK(d.Add("some notification", GetHandle(&some_procedure), {"param"}));
  json p = {{"param2", "some string"}};
  REQUIRE_THROWS_WITH(d.InvokeNotification("some notification", p), Contains("-32602: invalid parameter: missing named parameter \"param\""));
  CHECK(procCache.empty());
}

TEST_CASE("invoking wrong type namedparameter", TEST_MODULE) {
  Dispatcher d;
  CHECK(d.Add("some method", GetHandle(&add_function), {"a", "b"}));
  REQUIRE_THROWS_WITH(d.InvokeMethod("some method", {{"a", "asdfasdf"}, {"b", -7}}), Contains("-32602") && Contains("must be unsigned integer, but is string"));
  REQUIRE_THROWS_WITH(d.InvokeMethod("some method", {{"a", -10}, {"b", -7}}), Contains("-32602") && Contains("must be unsigned integer, but is integer"));
}

TEST_CASE("error on invoking unsupported named parameter", TEST_MODULE) {
  Dispatcher d;
  CHECK(d.Add("some method", GetHandle(&add_function)));
  REQUIRE_THROWS_WITH(d.InvokeMethod("some method", {{"a", 11}, {"b", 22}}), Contains("invalid parameter: procedure doesn't support named parameter"));

  CHECK(d.Add("some notification", GetHandle(&some_procedure)));
  json p = {{"param", "some string"}};
  REQUIRE_THROWS_WITH(d.InvokeNotification("some notification", p), Contains("invalid parameter: procedure doesn't support named parameter"));
}

// TODO: avoid signed, unsigned bool invocations