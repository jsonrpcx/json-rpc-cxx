#include "examples/inmemoryconnector.hpp"
#include "warehouseapp.hpp"

#include <jsonrpccxx/server.hpp>
#include <jsonrpccxx/client.hpp>
#include <iostream>

using namespace jsonrpccxx;
using namespace std;

int main() {
  JsonRpc2Server rpcServer;
  InMemoryConnector connector(rpcServer);
  JsonRpcClient client(connector, version::v2);

  //Bindings
  WarehouseServer app;
  rpcServer.Add("GetProduct", GetHandle(&WarehouseServer::GetProduct, app), {"id"});
  rpcServer.Add("AddProduct", GetHandle(&WarehouseServer::AddProduct, app), {"product"});

  Product p = {"0xff", 22.4, "Product 1", category::cash_carry};
  cout << "Adding product: " << client.CallMethod<bool>(1, "AddProduct", {p}) << "\n";

  Product p2 = client.CallMethod<Product>(1, "GetProduct", {"0xff"});
  cout << "Found product: " << p2.name << "\n";
  try {
    Product p3 = client.CallMethod<Product>(1, "GetProduct", {"0xff2"});
  } catch (JsonRpcException &e) {
    cerr << "Error finding product: " << e.what() << "\n";
    return 1;
  }
  return 0;
}