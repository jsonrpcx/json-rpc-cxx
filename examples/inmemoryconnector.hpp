#pragma once
#include <jsonrpccxx/iclientconnector.hpp>
#include <jsonrpccxx/server.hpp>

class InMemoryConnector : public jsonrpccxx::IClientConnector{
public:
    InMemoryConnector(jsonrpccxx::JsonRpcServer& server) : server(server) {}

    virtual std::string Send(const std::string &request) {
        return server.HandleRequest(request);
    }

private:
    jsonrpccxx::JsonRpcServer& server;
};