
#include "integrationtest.hpp"
#include "../examples/warehouse/warehouseapp.hpp"

#define TEST_MODULE "[integration]"

TEST_CASE_METHOD(IntegrationTest, "warehouse_test", TEST_MODULE) {
  WarehouseServer app;
  rpcServer.Add("GetProduct", GetHandle(&WarehouseServer::GetProduct, app));
  rpcServer.Add("AddProduct", GetHandle(&WarehouseServer::AddProduct, app));

  Product p = {"0xff", 22.4, "Product 1", category::cash_carry};
  CHECK(client.CallMethod<bool>(1, "AddProduct", {p}));

  Product p2 = client.CallMethod<Product>(1, "GetProduct", {"0xff"});
  CHECK((p2.id == p.id && p2.name == p.name && p2.price == p.price && p2.cat == p.cat));
}