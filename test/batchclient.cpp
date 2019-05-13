#include "batchclient.hpp"
#include "catch/catch.hpp"
#include "common.hpp"
#include <iostream>

#define TEST_MODULE "[batch]"

using namespace std;
using namespace jsonrpccxx;
using namespace Catch::Matchers;

TEST_CASE("batchresponse_parsing", TEST_MODULE) {
  BatchResponse br({{{"jsonrpc", "2.0"}, {"id", "1"}, {"result", "someresultstring"}},
                    {{"jsonrpc", "2.0"}, {"id", "2"}, {"result", 33}},
                    {{"jsonrpc", "2.0"}, {"id", "3"}, {"error", {{"code", -111}, {"message", "the error message"}}}},
                    {{"jsonrpc", "2.0"}, {"id", nullptr}, {"error", {{"code", -112}, {"message", "the error message"}}}},
                    3});

  CHECK(br.HasErrors());
  CHECK(br.Get<string>("1") == "someresultstring");
  REQUIRE_THROWS_WITH(br.Get<string>(1), Contains("no result found for id 1"));
  CHECK(br.Get<int>("2") == 33);
  REQUIRE_THROWS_WITH(br.Get<int>("1"), Contains("type must be number, but is string"));
  REQUIRE_THROWS_WITH(br.Get<string>("3"), Contains("-111: the error message"));
  REQUIRE_THROWS_WITH(br.Get<string>(nullptr), Contains("no result found for id null"));

  CHECK(br.GetInvalidIndexes().size() == 2);
  CHECK(br.GetAt(br.GetInvalidIndexes()[0])["error"]["code"] == -112);
  CHECK(br.GetAt(br.GetInvalidIndexes()[1]) == 3);
}