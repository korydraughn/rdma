// This example is implemented using the following as a reference:
//
//   https://github.com/linux-rdma/rdma-core/blob/master/librdmacm/examples/rdma_xclient.c
//

#include "rdma_cpp.hpp"

#include <cstring>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <sstream>
#include <exception>
#include <iterator>

constexpr auto header_length = 10;

auto encode_message(const std::string& _msg) -> std::string
{
    std::ostringstream ss;
    ss << std::left << std::setfill(' ') << std::setw(header_length) << _msg.size();
    return ss.str() + _msg;
}

auto main(int _argc, char* _argv[]) -> int
{
    if (_argc != 4) {
        std::cerr << "Invalid argument count.\n"
		     "Usage: rdma_client <host> <port> <message>\n"
		     "  <message> must be less than 100 bytes in size.\n";
	return 1;
    }

    const char* host = _argv[1];
    const char* port = _argv[2];
    const std::string msg  = _argv[3];

    try {
        rdma::address_info addr_info{host, port, rdma::app_type::client};
        rdma::communication_manager comm_mgr{addr_info};

	// We can avoid this copy with a little extra work.
        std::vector<std::uint8_t> buffer(header_length + msg.size());
	const auto m = encode_message(msg);
	std::copy(std::begin(m), std::end(m), std::begin(buffer));
        rdma::memory_region mem_region{comm_mgr, buffer.data(), buffer.size()};

        auto qp = comm_mgr.connect();
        qp.post_send(buffer.data(), buffer.size(), mem_region);
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}

