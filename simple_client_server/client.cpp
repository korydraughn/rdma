// This example is implemented using the following as a reference:
//
//   https://github.com/linux-rdma/rdma-core/blob/master/librdmacm/examples/rdma_xclient.c
//

#include "rdma_cpp.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <exception>

auto main(int _argc, char* _argv[]) -> int
{
    if (_argc != 3) {
        std::cerr << "Invalid argument count.\n"
		     "Usage: rdma_client <host> <port>\n";
	return 1;
    }

    const char* host = _argv[1];
    const char* port = _argv[2];

    try {
        rdma::address_info addr_info{host, port, rdma::app_type::client};
        rdma::communication_manager comm_mgr{addr_info};

        std::vector<std::uint8_t> buffer{'K', 'o', 'r', 'y', ' ', 'D', 'r', 'a', 'u', 'g', 'h', 'n', '\0'};
        rdma::memory_region mem_region{comm_mgr, buffer.data(), buffer.size()};

        auto qp = comm_mgr.connect();
        qp.post_send(buffer.data(), buffer.size());
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}

