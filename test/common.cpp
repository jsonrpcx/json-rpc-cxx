#include "catch/catch.hpp"
#include <iostream>
#include <jsonrpccxx/common.hpp>

#define TEST_MODULE "[common]"

using namespace std;
using namespace jsonrpccxx;
using namespace Catch::Matchers;

TEST_CASE("exception error type", TEST_MODULE) {
  CHECK(JsonRpcException(-32700, "").Type() == parse_error);
  CHECK(JsonRpcException(-32600, "").Type() == invalid_request);
  CHECK(JsonRpcException(-32601, "").Type() == method_not_found);
  CHECK(JsonRpcException(-32602, "").Type() == invalid_params);
  CHECK(JsonRpcException(-32603, "").Type() == internal_error);

  for(int c = -32000; c >= -32099; c--)
    CHECK(JsonRpcException(c, "").Type() == server_error);

  CHECK(JsonRpcException(0, "").Type() == invalid);
  CHECK(JsonRpcException(32700, "").Type() == invalid);
  CHECK(JsonRpcException(33000, "").Type() == invalid);
}