#include "verbspp.hpp"

#include <infiniband/verbs.h>

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include <utility>

auto main(int _argc, char* _argv[]) -> int
{
    rdma::device_list devices;
    std::cout << "Number of RDMA devices: " << devices.size() << '\n';

    for (int i = 0; i < devices.size(); ++i) {
        auto d = devices[i];
        std::cout << i << ". Device - Name: " << d.name() << ", GUID: " << d.guid() << '\n';
    }

    rdma::context context{devices[0]};
    // TODO Print device contect properties.

    rdma::protection_domain pd{context};

    std::vector<std::uint8_t> buffer(128);
    rdma::memory_region mr{pd, buffer, IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE};

    rdma::completion_event_channel comp_ch{context};
    rdma::completion_queue cq{1, comp_ch};

    return 0;
}

