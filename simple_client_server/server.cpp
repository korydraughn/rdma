// This example is implemented using the following as a reference:
//
//   https://github.com/linux-rdma/rdma-core/blob/master/librdmacm/examples/rdma_xserver.c
//

#include "rdma_cpp.hpp"

#include <cstring>
#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>

auto main(int _argc, char* _argv[]) -> int
{
    if (_argc != 2) {
        std::cerr << "Invalid argument count.\n"
		     "Usage: rdma_server <port>\n";
	return 1;
    }

    const char* host = "0.0.0.0"; //nullptr;
    const char* port = _argv[1];

    try {
        rdma::address_info addr_info{host, port, rdma::app_type::server};
        rdma::communication_manager listen_mgr{addr_info};
	std::cout << "Listening for new connection requests ...\n";

        auto ec = rdma_listen(listen_mgr, 0);
        if (ec)
            throw std::runtime_error{"rdma_listen error."};
	std::cout << "Waiting for request ...\n";

        rdma_cm_id* client_comm_id{};
        ec = rdma_get_request(listen_mgr, &client_comm_id);
        if (ec)
            throw std::runtime_error{"rdma_get_request error."};
        rdma::communication_manager client_comm_mgr{*client_comm_id};
	std::cout << "Created new communication manager for client.\n";

        std::vector<std::uint8_t> buffer(std::strlen("Kory Draughn") + 1);
        rdma::memory_region mem_region{client_comm_mgr, buffer.data(), buffer.size()};
	std::cout << "Created and registered memory region (buffer size: " << buffer.size() << ").\n";

        // 1. Call rdma_post_recv to post receive request.
        ec = rdma_post_recv(client_comm_mgr, nullptr, buffer.data(), buffer.size(), mem_region);
        if (ec)
            throw std::runtime_error{"rdma_post_recv error."};
	std::cout << "Posted receive request.\n";

        // 2. Call rdma_accept to accept a connection request.
        ec = rdma_accept(client_comm_mgr, nullptr);
        if (ec)
            throw std::runtime_error{"rdma_accept error."};
	std::cout << "Accepted new connection.\n";
        
        // 3. Call rdma_get_recv_comp to signal to the rdma_cm_id that the request has been handled.
        ibv_wc wc{};
#if 1
        while ((ec = rdma_get_recv_comp(client_comm_mgr, &wc)) == 0)
	    std::cout << "rdma_get_recv_comp::ec = " << ec << '\n';

        if (ec < 0)
            throw std::runtime_error{"rdma_get_recv_comp error."};
#else
	ec = rdma_get_recv_comp(client_comm_mgr, &wc);
	std::cout << "rdma_get_recv_comp::ec = " << ec << '\n';

	if (ec <= 0)
            throw std::runtime_error{"rdma_get_recv_comp error."};
#endif
        std::cout << "Received Message: " << (char*) buffer.data() << '\n';
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}

