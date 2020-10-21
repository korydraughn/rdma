#ifndef KDD_RDMA_UTILITY_HPP
#define KDD_RDMA_UTILITY_HPP

#include "verbs.hpp"

#include <infiniband/verbs.h>

#include <endian.h>
#include <byteswap.h>

#include <boost/asio.hpp>

#include <cstdint>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <random>

#if __BYTE_ORDER == __LITTLE_ENDIAN
    inline std::uint64_t htonll(std::uint64_t _x) { return bswap_64(_x); }
    inline std::uint64_t ntohll(std::uint64_t _x) { return bswap_64(_x); }
#elif __BYTE_ORDER == __BIG_ENDIAN
    inline std::uint64_t htonll(std::uint64_t _x) { return _x; }
    inline std::uint64_t ntohll(std::uint64_t _x) { return _x; }
#else
    #error __BYTE_ORDER is neither __LITTLE_ENDIAN nor __BIG_ENDIAN
#endif

namespace rdma
{
    inline
    auto change_queue_pair_state_to_init(queue_pair& _qp,
                                         std::uint8_t _port_number,
                                         int _pkey_index,
                                         int _access_flags) -> void
    {
        std::cout << "Changing QP state to INIT\n";
        ibv_qp_attr attrs{};

        attrs.qp_state = IBV_QPS_INIT;
        attrs.pkey_index = _pkey_index;
        attrs.port_num = _port_number;
        attrs.qp_access_flags = _access_flags;

        const auto props = (IBV_QP_STATE |
                            IBV_QP_PKEY_INDEX |
                            IBV_QP_PORT |
                            IBV_QP_ACCESS_FLAGS);

        _qp.modify_attribute(attrs, props);
        std::cout << "QP state changed successfully!\n";
    }

    inline
    auto change_queue_pair_state_to_rtr(queue_pair& _qp,
                                        const queue_pair_info& _remote_info,
                                        std::uint8_t _port_number,
                                        std::uint8_t _gid_index,
                                        bool _grh_required) -> void
    {
        std::cout << "Changing QP state to RTR\n";
        ibv_qp_attr attrs{};

        attrs.qp_state = IBV_QPS_RTR;
        attrs.path_mtu = IBV_MTU_512;
        attrs.dest_qp_num = _remote_info.qp_num;
        attrs.rq_psn = _remote_info.rq_psn;
        attrs.max_dest_rd_atomic = 1;
        attrs.min_rnr_timer = 12;
        attrs.ah_attr.dlid = _remote_info.lid;
        attrs.ah_attr.sl = 0;
        attrs.ah_attr.src_path_bits = 0;
        attrs.ah_attr.port_num = _port_number;

        if (_grh_required) {
            attrs.ah_attr.is_global = 1;
            attrs.ah_attr.grh.dgid = _remote_info.gid;
            attrs.ah_attr.grh.flow_label = 0;
            attrs.ah_attr.grh.hop_limit = 1;
            attrs.ah_attr.grh.sgid_index = 0;//_gid_index;
            attrs.ah_attr.grh.traffic_class = 0;
        }

        const auto props = (IBV_QP_STATE |
                            IBV_QP_AV |
                            IBV_QP_PATH_MTU |
                            IBV_QP_DEST_QPN |
                            IBV_QP_RQ_PSN |
                            IBV_QP_MAX_DEST_RD_ATOMIC |
                            IBV_QP_MIN_RNR_TIMER);

        _qp.modify_attribute(attrs, props);
        std::cout << "QP state changed successfully!\n";
    }

    inline
    auto change_queue_pair_state_to_rts(queue_pair& _qp, std::uint32_t _sq_psn) -> void
    {
        std::cout << "Changing QP state to RTS\n";
        ibv_qp_attr attrs{};

        attrs.qp_state = IBV_QPS_RTS;
        attrs.sq_psn = _sq_psn;
        attrs.timeout = 14;
        attrs.retry_cnt = 0; //7; // Set to zero for development.
        attrs.rnr_retry = 0; //7; // Set to zero for development.
        attrs.max_rd_atomic = 1;

        const auto props = (IBV_QP_STATE |
                            IBV_QP_SQ_PSN |
                            IBV_QP_TIMEOUT |
                            IBV_QP_RETRY_CNT |
                            IBV_QP_RNR_RETRY |
                            IBV_QP_MAX_QP_RD_ATOMIC);

        _qp.modify_attribute(attrs, props);
        std::cout << "QP state changed successfully!\n";
    }

    struct queue_pair_info
    {
        std::uint32_t qp_num;
        std::uint32_t rq_psn;
        std::uint16_t lid;
        ibv_gid gid;
    };

    inline
    auto exchange_queue_pair_info(const std::string& _host,
                                  const std::string& _port,
                                  queue_pair_info& _qp_info,
                                  bool _is_server) -> void
    {
        using tcp = boost::asio::ip::tcp;

        _qp_info.qp_num = htonl(_qp_info.qp_num);
        _qp_info.lid = htons(_qp_info.lid);

        if (_is_server) {
            boost::asio::io_service io_service;

            tcp::endpoint endpoint(tcp::v4(), std::stoi(_port));
            tcp::acceptor acceptor{io_service, endpoint};

            std::cout << "Waiting for client to connect ... ";
            tcp::iostream stream;
            boost::system::error_code ec;
            acceptor.accept(*stream.rdbuf(), ec);

            if (ec) {
                throw std::runtime_error{"connect_queue_pairs server error"};
            }

            std::cout << "connected!\n";
            std::cout << "Exchanging QP information with client ... ";

            // Capture the client's queue pair information.
            queue_pair_info client_qp_info{};
            stream.read((char*) &client_qp_info, sizeof(queue_pair_info));

            // Send the server's queue pair information.
            stream.write((char*) &_qp_info, sizeof(queue_pair_info));

            // Copy the client's info into the out parameter.
            _qp_info = client_qp_info;
        }
        else {
            std::cout << "Connecting to server ... ";
            tcp::iostream stream{_host, _port};

            if (!stream)
                throw std::runtime_error{"connect_queue_pairs client error"};

            std::cout << "connected!\n";
            std::cout << "Exchanging QP information with server ... ";

            // Send the client's queue pair information to the server.
            stream.write((char*) &_qp_info, sizeof(queue_pair_info));

            // Capture the server's queue pair information.
            stream.read((char*) &_qp_info, sizeof(queue_pair_info));
        }

        std::cout << "done!\n";

        _qp_info.qp_num = ntohl(_qp_info.qp_num);
        _qp_info.lid = ntohs(_qp_info.lid);
    }

    inline
    constexpr auto to_string(ibv_port_state _pstate) noexcept -> const char*
    {
        switch (_pstate) {
            case IBV_PORT_NOP:          return "IBV_PORT_NOP";
            case IBV_PORT_DOWN:         return "IBV_PORT_DOWN";
            case IBV_PORT_INIT:         return "IBV_PORT_INIT";
            case IBV_PORT_ARMED:        return "IBV_PORT_ARMED";
            case IBV_PORT_ACTIVE:       return "IBV_PORT_ACTIVE";
            case IBV_PORT_ACTIVE_DEFER: return "IBV_PORT_ACTIVE_DEFER";
            default:                    return "?";
        }
    }

    inline
    constexpr auto to_string(ibv_mtu _mtu) noexcept -> const char*
    {
        switch (_mtu) {
            case IBV_MTU_256:  return "IBV_MTU_256";
            case IBV_MTU_512:  return "IBV_MTU_512";
            case IBV_MTU_1024: return "IBV_MTU_1024";
            case IBV_MTU_2048: return "IBV_MTU_2048";
            case IBV_MTU_4096: return "IBV_MTU_4096";
            default:           return "?";
        }
    }

    auto generate_random_int() -> std::uint32_t
    {
        std::random_device rd;
        std::mt19937 gen{rd()};
        std::uniform_int_distribution<std::uint32_t> distrib{1, 1 << 16};
        return distrib(gen);
    }

    auto print_device_info(const context& _c) -> void
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
        std::cout << "physical port count: " << (int) info.phys_port_cnt << '\n';
    }

    auto print_port_info(const context& _c, std::uint8_t _port_number) -> void
    {
        const auto info = _c.port_info(_port_number);

        std::cout << "Port Information\n";
        std::cout << "----------------\n";
        std::cout << "state       : " << to_string(info.state) << '\n';
        std::cout << "max mtu     : " << to_string(info.max_mtu) << '\n';
        std::cout << "active mtu  : " << to_string(info.active_mtu) << '\n';
        std::cout << "gid tbl ln  : " << info.gid_tbl_len << '\n';
        std::cout << "lid         : " << info.lid << '\n';
        std::cout << "sm lid      : " << info.sm_lid << '\n';
        std::cout << "sm sl       : " << (int) info.sm_sl << '\n';

        const auto grh_required = (info.flags & IBV_QPF_GRH_REQUIRED) == IBV_QPF_GRH_REQUIRED;
        std::cout << "grh required: " << std::boolalpha << grh_required << '\n';
        std::cout.unsetf(std::ios::boolalpha);
    }

    auto print_queue_pair_attributes(const ibv_qp_attr& _qp_attrs, const ibv_qp_init_attr& _qp_init_attrs) -> void
    {
        std::cout << "Queue Pair Attributes\n";
        std::cout << "---------------------\n";
        std::cout << "pkey index        : " << _qp_attrs.pkey_index << '\n';
        std::cout << "qp port           : " << (int) _qp_attrs.port_num << '\n';
        std::cout << "q key             : " << _qp_attrs.qkey << '\n';
        std::cout << "retry cnt         : " << (int) _qp_attrs.retry_cnt << '\n';
        std::cout << "min rnr timer     : " << (int) _qp_attrs.min_rnr_timer << '\n';
        std::cout << "max rd atomic     : " << (int) _qp_attrs.max_rd_atomic << '\n';
        std::cout << "max qp rd atomic  : " << (int) _qp_attrs.max_rd_atomic << '\n';
        std::cout << "max dest rd atomic: " << (int) _qp_attrs.max_dest_rd_atomic << '\n';
        std::cout << "max send wr       : " << _qp_init_attrs.cap.max_send_wr << '\n';
        std::cout << "max recv wr       : " << _qp_init_attrs.cap.max_recv_wr << '\n';
        std::cout << "max send sge      : " << _qp_init_attrs.cap.max_send_sge << '\n';
        std::cout << "max recv sge      : " << _qp_init_attrs.cap.max_recv_sge << '\n';
        std::cout << "max inline data   : " << _qp_init_attrs.cap.max_inline_data << '\n';
    }

    auto print_queue_pair_info(const queue_pair_info& _qpi, bool _local_info) -> void
    {
        if (_local_info) {
            std::cout << "Local Queue Pair Information\n";
            std::cout << "----------------------------\n";
        }
        else {
            std::cout << "Remote Queue Pair Information\n";
            std::cout << "-----------------------------\n";
        }

        std::cout << "qp_num: " << _qpi.qp_num << '\n';
        std::cout << "rq_psn: " << _qpi.rq_psn << '\n';
        std::cout << "lid   : " << _qpi.lid << '\n';

        std::ostringstream ss;
        for (int i = 0; i < 8; ++i) {
            for (int j = 0; j < 2; ++j)
                ss << std::hex << std::setw(2) << std::setfill('0') << std::right
                   << static_cast<int>(_qpi.gid.raw[i * 2 + j]);

            if (i < 7)
                ss << ':';
        }
        std::cout << "gid   : " << ss.str() << '\n';
    }
} // namespace rdma

#endif // KDD_RDMA_UTILITY_HPP
