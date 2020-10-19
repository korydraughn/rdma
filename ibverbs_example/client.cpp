#include "verbs.hpp"

#include <infiniband/verbs.h>

#include <boost/asio.hpp>

#include <cstdint>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <utility>

auto main(int _argc, char* _argv[]) -> int
{
    if (_argc != 3) {
        std::cout << "USAGE: client <host> <port>\n";
        return 1;
    }

    try {
        rdma::device_list devices;
        std::cout << "Number of RDMA devices: " << devices.size() << '\n';
        std::cout << '\n';

        for (int i = 0; i < devices.size(); ++i) {
            auto d = devices[i];
            std::cout << i << ". Device - Name: " << d.name() << ", GUID: " << d.guid() << '\n';
        }
        std::cout << '\n';

        rdma::context context{devices[0]};
        rdma::print_device_info(context);
        std::cout << '\n';

        constexpr auto port_number = 1;
        rdma::print_port_info(context, port_number);
        std::cout << '\n';

        constexpr auto pkey_index = 0;
        std::cout << "pkey: " << context.pkey(port_number, pkey_index) << '\n';
        std::cout << '\n';

        rdma::protection_domain pd{context};

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

        const auto [qp_attrs, q_attrs] = qp.query_attribute(IBV_QP_RQ_PSN | IBV_QP_AV);
        static_cast<void>(q_attrs);

        rdma::queue_pair_info qp_info{};
        qp_info.qp_num = qp.queue_pair_number();
        qp_info.rq_psn = qp_attrs.rq_psn;
        qp_info.lid = qp_attrs.ah_attr.dlid;
        qp_info.gid = qp_attrs.ah_attr.grh.dgid;

        std::cout << "Client Queue Pair Info\n";
        std::cout << "----------------------\n";
        rdma::print_queue_pair_info(qp_info);
        std::cout << '\n';

        constexpr auto is_server = false;
        const auto* host = _argv[1];
        const auto* port = _argv[2];
        rdma::exchange_queue_pair_info(host, port, qp_info, is_server);

        std::cout << "Server Queue Pair Info\n";
        std::cout << "----------------------\n";
        rdma::print_queue_pair_info(qp_info);
        std::cout << '\n';

        constexpr auto access_flags = 0;
        rdma::change_queue_pair_state_to_init(qp, port_number, pkey_index, access_flags);
        rdma::change_queue_pair_state_to_rtr(qp, qp_info.qp_num, qp_info.rq_psn, qp_info.lid, qp_info.gid, port_number);
        rdma::change_queue_pair_state_to_rts(qp);

        // This is not necessary for initialization. Memory Regions can be registered at
        // any time following initialization.
        //std::vector<std::uint8_t> buffer(128);
        //rdma::memory_region mr{pd, buffer, IBV_ACCESS_LOCAL_WRITE};

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
    }

    return 1;
}

