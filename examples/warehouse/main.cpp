#include "inmemoryconnector.hpp"
#include "warehouseapp.hpp"

#include <iostream>
#include <jsonrpccxx/client.hpp>
#include <jsonrpccxx/server.hpp>

using namespace jsonrpccxx;
using namespace std;

class WareHouseClient {
public:
  explicit WareHouseClient(JsonRpcClient &client) : client(client) {}
  bool AddProduct(const Product &p) { return client.CallMethod<bool>(1, "AddProduct", {p}); }
  Product GetProduct(const std::string &id) { return client.CallMethod<Product>(1, "GetProduct", {id}); }

private:
  JsonRpcClient &client;
};

int main() {
  JsonRpc2Server rpcServer;
  InMemoryConnector connector(rpcServer);
  JsonRpcClient client(connector, version::v2);

  // Bindings
  WareHouseClient appClient(client);
  WarehouseServer app;
  rpcServer.Add("GetProduct", GetHandle(&WarehouseServer::GetProduct, app), {"id"});
  rpcServer.Add("AddProduct", GetHandle(&WarehouseServer::AddProduct, app), {"product"});

  Product p = {"0xff", 22.4, "Product 1", category::cash_carry};
  cout << "Adding product: " << std::boolalpha << appClient.AddProduct(p) << "\n";

  Product p2 = appClient.GetProduct("0xff");
  cout << "Found product: " << p2.name << "\n";
  try {
    appClient.GetProduct("0xff2");
  } catch (JsonRpcException &e) {
    cerr << "Error finding product: " << e.what() << "\n";
    return 1;
  }
  return 0;
}