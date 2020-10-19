#include "verbspp.hpp"

#include <infiniband/verbs.h>

#include <boost/asio.hpp>

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include <utility>

auto print_device_info(const rdma::context& _c) -> void
{
    const auto info = _c.device_info();

    std::cout << "Device Information\n";
    std::cout << "------------------\n";
    std::cout << "firmware version   : " << info.fw_ver << '\n';
    std::cout << "node guid          : " << info.node_guid << '\n';      // In network byte order
    std::cout << "system image guid  : " << info.sys_image_guid << '\n'; // In network byte order
    std::cout << "max mr size        : " << info.max_mr_size << '\n';
    std::cout << "vendor id          : " << info.vendor_id << '\n';
    std::cout << "vendor part id     : " << info.vendor_part_id << '\n';
    std::cout << "hardware version   : " << info.hw_ver << '\n';
    std::cout << "physical port count: " << info.phys_port_cnt << '\n';
}

auto print_port_info(const rdma::context& _c, std::uint8_t _port_number) -> void
{
    const auto info = _c.port_info();

    std::cout << "Port Information\n";
    std::cout << "----------------\n";
    std::cout << "state     : " << rdma::to_string(info.state) << '\n';
    std::cout << "max mtu   : " << rdma::to_string(info.max_mtu) << '\n';
    std::cout << "active mtu: " << rdma::to_string(info.active_mtu) << '\n';
    std::cout << "lid       : " << info.lid << '\n';
}

auto main(int _argc, char* _argv[]) -> int
{
    try {
        rdma::device_list devices;
        std::cout << "Number of RDMA devices: " << devices.size() << '\n';

        for (int i = 0; i < devices.size(); ++i) {
            auto d = devices[i];
            std::cout << i << ". Device - Name: " << d.name() << ", GUID: " << d.guid() << '\n';
        }

        rdma::context context{devices[0]};
        print_device_info(context);

        constexpr auto port_number = 1;
        print_port_info(context, port_number);

        constexpr auto pkey_index = 0;
        std::cout << "\npkey: " << context.pkey(port_number, pkey_index) << '\n';

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

        constexpr auto access_flags = 0;
        rdma::change_queue_pair_state_to_init(qp, port_number, pkey_index, access_flags);

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

        queue_pair_info qp_info{
            .qpn = "",
            .lid = "",
            .rq_psn = "",
            .gid = ""
        };

        constexpr auto is_server = true;
        rdma::connect_queue_pairs(host, port, qp_info, is_server);

        // This is not necessary for initialization. Memory Regions can be registered at
        // any time following initialization.
        std::vector<std::uint8_t> buffer(128);
        rdma::memory_region mr{pd, buffer, IBV_ACCESS_LOCAL_WRITE};

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
    }

    return 1;
}

