#include "verbs.hpp"

#include <infiniband/verbs.h>

#include <boost/program_options.hpp>
#include <boost/asio.hpp>

#include <cstdint>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <utility>
#include <iterator>
#include <algorithm>

namespace po = boost::program_options;

auto main(int _argc, char* _argv[]) -> int
{
    try {
        po::options_description desc{"Options"};
        desc.add_options()
            ("server,s", po::bool_switch(), "Launches server.")
            ("host,h", po::value<std::string>(), "The host to connect to. Ignored if -s is used.")
            ("port,p", po::value<std::string>()->default_value("9900"), "The port to connect to.")
            ("gid-index,g", po::value<int>()->default_value(0), "The index of the GID to use.")
            ("skip-rdma,x", po::bool_switch(), "Skips RDMA message passing steps.")
            ("help", po::bool_switch(), "Show this message.");

        po::variables_map vm;
        po::store(po::parse_command_line(_argc, _argv, desc), vm);
        po::notify(vm);

        if (vm["help"].as<bool>()) {
            std::cout << desc << '\n';
            return 0;
        }

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

        rdma::queue_pair qp{pd, qp_init_attrs, cq};

        // Exchange QP information.
        const auto sq_psn = rdma::generate_random_int();
        const auto port_info = context.port_info(port_number);
        const auto gid_index = vm["gid-index"].as<int>();

        rdma::queue_pair_info qp_info{};
        qp_info.qp_num = qp.queue_pair_number();
        qp_info.rq_psn = 0;//sq_psn;
        qp_info.lid = port_info.lid;
        qp_info.gid = context.gid(port_number, gid_index);

        constexpr auto local_info = true;
        rdma::print_queue_pair_info(qp_info, local_info);
        std::cout << '\n';

        const auto run_server = vm["server"].as<bool>();
        const auto host = run_server ? "" : vm["host"].as<std::string>();
        const auto port = vm["port"].as<std::string>();

        // Exchange the required QP information between the client and server.
        // This is required so that the QPs can be transistioned through the proper
        // states and connected for data communication.
        rdma::exchange_queue_pair_info(host, port, qp_info, run_server);

        rdma::print_queue_pair_info(qp_info, !local_info);
        std::cout << '\n';

        // Change the QP's state to RTS.
        constexpr auto access_flags = 0;//IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;
        rdma::change_queue_pair_state_to_init(qp, port_number, pkey_index, access_flags);

        const auto grh_required = (port_info.flags & IBV_QPF_GRH_REQUIRED) == IBV_QPF_GRH_REQUIRED;
        rdma::change_queue_pair_state_to_rtr(qp, qp_info, port_number, gid_index, grh_required);

        rdma::change_queue_pair_state_to_rts(qp, sq_psn);

        std::cout << '\n';
        const auto [qp_attrs, q_attrs] = qp.query_attribute(IBV_QP_RQ_PSN | IBV_QP_AV);
        rdma::print_queue_pair_attributes(qp_attrs, q_attrs);
        std::cout << '\n';

        // Memory Regions can be registered at any time. However, doing this in the
        // data path could negatively affect performance.
        std::vector<std::uint8_t> buffer(128);
        rdma::memory_region mr{pd, buffer, access_flags};

        if (!vm["skip-rdma"].as<bool>()) {
            if (run_server) {
                std::cout << "Posting receive request ... ";
                qp.post_receive(buffer, mr);
                std::cout << "done!\n";

                std::cout << "Waiting for completion ... ";
                const auto wc = qp.wait_for_completion();
                std::cout << "done!\n";

                std::cout << "WC Status: " << ibv_wc_status_str(wc.status) << ", Code: " << wc.status << '\n';

                if (wc.status == IBV_WC_SUCCESS) {
                    std::cout << "Message received: ";
                    std::cout.write((char*) buffer.data(), buffer.size());
                    std::cout << '\n';
                }
                else {
                    std::cout << "Error receiving message.\n";
                }
            }
            else {
                const char msg[] = "This was sent from the client!";
                std::copy(msg, msg + strlen(msg), buffer.data() + (grh_required ? 40 : 0));

                std::cout << "Posting send request ... ";
                qp.post_send(buffer, mr);
                std::cout << "done!\n";

                std::cout << "Waiting for completion ... ";
                const auto wc = qp.wait_for_completion();
                std::cout << "done!\n";

                std::cout << "WC Status: " << ibv_wc_status_str(wc.status) << ", Code: " << wc.status << '\n';

                if (wc.status == IBV_WC_SUCCESS)
                    std::cout << "Message sent!\n";
                else
                    std::cout << "Could not send message.\n";
            }
        }

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
    }

    return 1;
}

