#ifndef KDD_RDMA_UTILITY_HPP
#define KDD_RDMA_UTILITY_HPP

#include "queue_pair.hpp"

#include <infiniband/verbs.h>

#include <boost/asio.hpp>

#include <cstdint>
#include <string>

namespace rdma
{
    inline
    auto change_queue_pair_state_to_init(queue_pair& _qp,
                                         std::uint8_t _port_number,
                                         int _pkey_index,
                                         int _access_flags) -> void
    {
        ibv_qp_attr attrs{};

        attrs.qp_state = IBV_QPS_INIT;
        attrs.pkey_index = 0;
        attrs.port_num = _port_number;
        attrs.qp_access_flags = 0;

        const auto props = (IBV_QP_STATE |
                            IBV_QP_PKEY_INDEX |
                            IBV_QP_PORT |
                            IBV_QP_ACCESS_FLAGS);

        qp.modify_attribute(attrs, props);
    }

    inline
    auto change_queue_pair_state_to_rtr(queue_pair& _qp,
                                        std::uint32_t _dest_qp_num,
                                        std::uint32_t _rq_psn,
                                        std::uint8_t _lid,
                                        std::uint8_t _gid,
                                        std::uint8_t _port_number) -> void
    {
        ibv_qp_attr attrs{};

        attrs.qp_state = IBV_QPS_RTR;
        attrs.path_mtu = IBV_MTU_256;
        attrs.dest_qp_num = _dest_qp_num;
        attrs.rq_psn = _rq_psn;
        attrs.max_dest_rd_atomic = 1;
        attrs.min_rnr_timer = 12;
        attrs.ah_attr.is_global = 0;
        attrs.ah_attr.dlid = _lid;
        attrs.ah_attr.sl = 0;
        attrs.ah_attr.src_path_bits = 0;
        attrs.ah_attr.port_num = _port_number;

        const auto props = (IBV_QP_STATE |
                            IBV_QP_AV |
                            IBV_QP_PATH_MTU |
                            IBV_QP_DEST_QPN |
                            IBV_QP_RQ_PSN);

        qp.modify_attribute(attrs, props);
    }

    inline
    auto change_queue_pair_state_to_rts(queue_pair& _qp) -> void
    {
        ibv_qp_attr attrs{};

        attrs.qp_state = IBV_QPS_RTS;
        attrs.sq_psn = 0;
        attrs.timeout = 14;
        attrs.retry_cnt = 7;
        attrs.rnr_retry = 7; // infiinite
        attrs.max_rd_atomic = 1;

        const auto props = (IBV_QP_STATE |
                            IBV_QP_SQ_PSN |
                            IBV_QP_MAX_QP_RD_ATOMIC |
                            IBV_QP_RETRY_CNT |
                            IBV_QP_RNR_RETRY |
                            IBV_QP_TIMEOUT);

        qp.modify_attribute(attrs, props);
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
                                  int _port,
                                  queue_pair_info& _qp_info,
                                  bool _is_server) -> void
    {
        using tcp = boost::asio::ip::tcp;

        if (_is_server) {
            boost::asio::io_context io_context;

            tcp::endpoint endpoint{tcp::v4(), _port};
            tcp::acceptor acceptor{io_context, endpoint};

            tcp::iostream stream;
            boost::system::error_code ec;
            acceptor.accept(stream.socket(), ec);

            if (ec) {
                throw std::runtime_error{"connect_queue_pairs server error"};
            }

            // Capture the client's queue pair information.
            queue_pair_info client_qp_info{};
            stream.read((char*) &client_qp_info, sizeof(queue_pair_info));

            // Send the server's queue pair information.
            stream.write((char*) &_qp_info, sizeof(queue_pair_info));
            _qp_info = client_qp_info;

            return;
        }

        tcp::iostream stream{_host, _port};

        if (!stream)
            throw std::runtime_error{"connect_queue_pairs client error"};

        // Send the client's queue pair information to the server.
        stream.write((char*) &_qp_info, sizeof(queue_pair_info));

        // Capture the server's queue pair information.
        stream.read((char*) &_qp_info, sizeof(queue_pair_info));
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
            default:                    return "???";
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
            default:           return "???";
        }
    }
} // namespace rdma

#endif // KDD_RDMA_UTILITY_HPP
