#pragma once
#include "common.hpp"

namespace jsonrpccxx {
    class IClientJsonConnector {
    public:
        virtual ~IClientJsonConnector() = default;

        /***
         * @throws json::parse_error In case repsonse can not be parsed.
         */
        virtual json Send(const json &request) = 0;
    };
}
