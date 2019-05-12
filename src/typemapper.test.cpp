#include "catch/catch.hpp"
#include "typemapper.hpp"
#include <iostream>
#include <limits>

using namespace jsonrpccpp;
using namespace std;
using namespace Catch::Matchers;

#define TEST_MODULE "[typemapper]"

static string notifyResult = "";

int add(int a, int b) { return a + b; }
void notify(const std::string &hello) { notifyResult = string("Hello world: ") + hello; }

class SomeClass {
public:
  int add(int a, int b) { return a + b; }
  void notify(const std::string &hello) { notifyResult = string("Hello world: ") + hello; }
};

TEST_CASE("test function binding", TEST_MODULE) {
  MethodHandle mh = GetHandle(&add);
  CHECK(mh(R"([3, 4])"_json) == 7);

  notifyResult = "";
  NotificationHandle mh2 = GetHandle(&notify);
  CHECK(notifyResult.empty());
  mh2(R"(["someone"])"_json);
  CHECK(notifyResult == "Hello world: someone");
}

TEST_CASE("test class member binding", TEST_MODULE) {
  SomeClass instance;
  MethodHandle mh = GetHandle(&SomeClass::add, instance);
  CHECK(mh(R"([3, 4])"_json) == 7);

  notifyResult = "";
  NotificationHandle mh2 = GetHandle(&SomeClass::notify, instance);
  CHECK(notifyResult.empty());
  mh2(R"(["someone"])"_json);
  CHECK(notifyResult == "Hello world: someone");
}

TEST_CASE("test incorrect params", TEST_MODULE) {
  SomeClass instance;
  MethodHandle mh = GetHandle(&SomeClass::add, instance);
  REQUIRE_THROWS_WITH(mh(R"(["3", "4"])"_json), Contains("must be integer, but is string"));
  REQUIRE_THROWS_WITH(mh(R"([true, true])"_json), Contains("must be integer, but is boolean"));
  REQUIRE_THROWS_WITH(mh(R"([null, 3])"_json), Contains("must be integer, but is null"));
  REQUIRE_THROWS_WITH(mh(R"([{"a": true}, 3])"_json), Contains("must be integer, but is object"));
  REQUIRE_THROWS_WITH(mh(R"([[2,3], 3])"_json), Contains("must be integer, but is array"));
  REQUIRE_THROWS_WITH(mh(R"([3.4, 3])"_json), Contains("must be integer, but is float"));
  REQUIRE_THROWS_WITH(mh(R"([4])"_json), Contains("expected 2 argument(s), but found 1"));
  REQUIRE_THROWS_WITH(mh(R"([5, 6, 5])"_json), Contains("expected 2 argument(s), but found 3"));

  notifyResult = "";
  NotificationHandle mh2 = GetHandle(&SomeClass::notify, instance);
  REQUIRE_THROWS_WITH(mh2(R"([33])"_json), Contains("must be string, but is unsigned integer"));
  REQUIRE_THROWS_WITH(mh2(R"([-33])"_json), Contains("must be string, but is integer"));
  REQUIRE_THROWS_WITH(mh2(R"(["someone", "anotherone"])"_json), Contains("expected 1 argument(s), but found 2"));
  REQUIRE_THROWS_WITH(mh2(R"([])"_json), Contains("expected 1 argument(s), but found 0"));
  REQUIRE_THROWS_WITH(mh2(R"([true])"_json), Contains("must be string, but is boolean"));
  REQUIRE_THROWS_WITH(mh2(R"([null])"_json), Contains("must be string, but is null"));
  REQUIRE_THROWS_WITH(mh2(R"([3.4])"_json), Contains("must be string, but is float"));
  REQUIRE_THROWS_WITH(mh2(R"([{"a": true}])"_json), Contains("must be string, but is object"));
  REQUIRE_THROWS_WITH(mh2(R"([["some string"]])"_json), Contains("must be string, but is array"));

  CHECK(notifyResult.empty());
}

enum class category { order, cash_carry };

struct product {
  int id;
  double price;
  string name;
  category cat;
};

NLOHMANN_JSON_SERIALIZE_ENUM(category, {{category::order, "order"}, {category::cash_carry, "cc"}});

void to_json(json &j, const product &p) { j = json{{"id", p.id}, {"price", p.price}, {"name", p.name}, {"category", p.cat}}; }

product get_product(int id) {
  if (id == 1)
    return product{1, 22.50, "some product", category::order};
  else if (id == 2)
    return product{2, 55.50, "some product 2", category::cash_carry};
  throw JsonRpcException(-50000, "product not found");
}

TEST_CASE("test with custom struct return", TEST_MODULE) {
  MethodHandle mh = GetHandle(&get_product);
  json j = mh(R"([1])"_json);
  CHECK(j["id"] == 1);
  CHECK(j["name"] == "some product");
  CHECK(j["price"] == 22.5);
  CHECK(j["category"] == category::order);

  j = mh(R"([2])"_json);
  CHECK(j["id"] == 2);
  CHECK(j["name"] == "some product 2");
  CHECK(j["price"] == 55.5);
  CHECK(j["category"] == category::cash_carry);

  REQUIRE_THROWS_WITH(mh(R"([444])"_json), Contains("product not found"));
}

void from_json(const json &j, product &p) {
  j.at("name").get_to(p.name);
  j.at("id").get_to(p.id);
  j.at("price").get_to(p.price);
  j.at("category").get_to(p.cat);
}

static vector<product> catalog;
bool add_products(const vector<product> &products) {
  if (products.empty())
    return false;
  std::copy(products.begin(), products.end(), std::back_inserter(catalog));
  return true;
};

TEST_CASE("test with custom params", TEST_MODULE) {
  MethodHandle mh = GetHandle(&add_products);
  catalog.clear();
  json params =
      R"([[{"id": 1, "price": 22.50, "name": "some product", "category": "order"}, {"id": 2, "price": 55.50, "name": "some product 2", "category": "cc"}]])"_json;

  CHECK(mh(params) == true);
  REQUIRE(catalog.size() == 2);

  CHECK(catalog[0].id == 1);
  CHECK(catalog[0].name == "some product");
  CHECK(catalog[0].price == 22.5);
  CHECK(catalog[0].cat == category::order);
  CHECK(catalog[1].id == 2);
  CHECK(catalog[1].name == "some product 2");
  CHECK(catalog[1].price == 55.5);
  CHECK(catalog[1].cat == category::cash_carry);

  REQUIRE_THROWS_WITH(mh(R"([[{"id": 1, "price": 22.50}]])"_json), Contains("key 'name' not found"));
  REQUIRE_THROWS_WITH(mh(R"([{"id": 1, "price": 22.50}])"_json), Contains("must be array, but is object"));
}

unsigned long unsigned_add(unsigned int a, int b) { return a + b; }

unsigned long unsigned_add2(unsigned short a, short b) { return a + b; }

TEST_CASE("test number range checking", TEST_MODULE) {
  MethodHandle mh = GetHandle(&unsigned_add);

  REQUIRE_THROWS_WITH(mh(R"([-3,3])"_json), Contains("must be unsigned integer, but is integer"));
  REQUIRE_THROWS_WITH(mh(R"([null,3])"_json), Contains("must be unsigned integer, but is null"));

  unsigned int max_us = numeric_limits<unsigned int>::max();
  unsigned int max_s = numeric_limits<int>::max();
  CHECK(mh({max_us, max_s}) == max_us + max_s);
  REQUIRE_THROWS_WITH(mh({max_us, max_us}), Contains("invalid parameter: exceeds value range of integer"));

  MethodHandle mh2 = GetHandle(&unsigned_add2);
  unsigned short max_su = numeric_limits<unsigned short>::max();
  unsigned short max_ss = numeric_limits<short>::max();
  CHECK(mh2({max_su, max_ss}) == max_su + max_ss);
  REQUIRE_THROWS_WITH(mh2({max_su, max_su}), Contains("invalid parameter: exceeds value range of integer"));
}