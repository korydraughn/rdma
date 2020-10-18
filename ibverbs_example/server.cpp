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
    // TODO Print device context properties.

    rdma::protection_domain pd{context};

    std::vector<std::uint8_t> buffer(128);
    rdma::memory_region mr{pd, buffer, IBV_ACCESS_LOCAL_WRITE};

    constexpr auto cqe_size = 1;
    rdma::completion_queue cq{cqe_size, context};

    ibv_qp_init_attr qp_init_attrs{};
    qp_init_attrs.qp_type = IBV_QPT_RC;
    qp_init_attrs.sq_sig_all = 1; // Each SR will produce a WC.
    qp_init_attrs.send_cq = &cq.handle();
    qp_init_attrs.recv_cq = &cq.handle();
    qp_init_attrs.cap.max_send_wr = 1;
    qp_init_attrs.cap.max_recv_wr = 1;
    qp_init_attrs.cap.max_send_sge = 1;
    qp_init_attrs.cap.max_recv_sge = 1;

    rdma::queue_pair qp{pd, qp_init_attrs};

    // TODO Exchange queue pair information.
    //
    // Use the Communication Manager (rdma_cm) to do this. The following information
    // needs to be exchanged when connecting queue pairs:
    // - QP number
    // - LID number
    // - RQ Packet Serial Number (PSN)
    // - GID (if GRH is used)
    
    // Next steps:
    //
    // csm.ornl.gov/workshops/openshmem2014/documents/presentations_and_tutorials/Tutorials/Verbs%20programming%20tutorial-final.pdf
    //   slide 78

    return 0;
}

