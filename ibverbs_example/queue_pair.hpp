#ifndef KDD_RDMA_QUEUE_PAIR_HPP
#define KDD_RDMA_QUEUE_PAIR_HPP

#include "context.hpp"
#include "protection_domain.hpp"
#include "completion_queue.hpp"
#include "memory_region.hpp"

#include <infiniband/verbs.h>

#include <stdio.h>
#include <errno.h>

#include <iostream>
#include <tuple>
#include <vector>
#include <stdexcept>

namespace rdma
{
    class queue_pair
    {
    public:
        queue_pair(const protection_domain& _pd,
                   ibv_qp_init_attr& _attrs,
                   const completion_queue& _cq)
            : qp_{ibv_create_qp(&_pd.handle(), &_attrs)}
            , cq_{&_cq.handle()}
        {
            if (!qp_) {
                perror("ibv_create_qp");
                throw std::runtime_error{"ibv_create_qp error"};
            }
        }

        ~queue_pair()
        {
            if (qp_)
                ibv_destroy_qp(qp_);
        }

        auto queue_pair_number() const noexcept -> std::uint32_t
        {
            return qp_->qp_num;
        }

        auto modify_attribute(const ibv_qp_attr& _attr, int _mask) const -> void
        {
            if (ibv_modify_qp(qp_, const_cast<ibv_qp_attr*>(&_attr), _mask)) {
                perror("ibv_modify_qp");
                throw std::invalid_argument{"ibv_modify_qp error"};
            }
        }

        auto query_attribute(int _mask) const -> std::tuple<ibv_qp_attr, ibv_qp_init_attr>
        {
            ibv_qp_attr attrs{};
            ibv_qp_init_attr init_attrs{};

            const auto ec = ibv_query_qp(qp_, &attrs, _mask, &init_attrs);

            if (ec) {
                perror("ibv_query_qp");
                throw std::invalid_argument{"ibv_query_qp error"};
            }

            return {attrs, init_attrs};
        }

        auto post_send(const std::vector<std::uint8_t>& _buffer, const memory_region& _mr) -> void
        {
            // TODO The incoming buffer could be registered for the duration
            // of this function call. This would simplify usage of the library
            // by removing the need for users to create memory regions outside
            // of the queue pair. This also applies to post_receive().

            ibv_sge sg_list{};
            sg_list.addr = (std::uintptr_t) _buffer.data();
            sg_list.length = _buffer.size();
            sg_list.lkey = _mr.local_key();

            ibv_send_wr wr{};
            wr.opcode = IBV_WR_SEND;
            wr.send_flags = IBV_SEND_SIGNALED;
            wr.sg_list = &sg_list;
            wr.num_sge = 1;

            ibv_send_wr* bad_wr{};

            if (ibv_post_send(qp_, &wr, &bad_wr)) {
                perror("ibv_post_send");
                throw std::runtime_error{"ibv_post_send error"};
            }
        }

        auto post_receive(std::vector<std::uint8_t>& _buffer, const memory_region& _mr) -> void
        {
            // TODO The incoming buffer could be registered for the duration
            // of this function call. This would simplify usage of the library
            // by removing the need for users to create memory regions outside
            // of the queue pair. This also applies to post_receive().

            ibv_sge sg_list{};
            sg_list.addr = (std::uintptr_t) _buffer.data();
            sg_list.length = _buffer.size();
            sg_list.lkey = _mr.local_key();

            ibv_recv_wr wr{};
            wr.sg_list = &sg_list;
            wr.num_sge = 1;

            ibv_recv_wr* bad_wr{};

            if (ibv_post_recv(qp_, &wr, &bad_wr)) {
                perror("ibv_post_recv");
                throw std::runtime_error{"ibv_post_recv error"};
            }
        }

        auto wait_for_work_completion() -> ibv_wc
        {
            int n_comp = 0;
            ibv_wc wc{};

            do {
                n_comp = ibv_poll_cq(cq_, 1, &wc);

                if (n_comp < 0) {
                    perror("ibv_poll_cq");
                    throw std::runtime_error{"ibv_poll_cq receive error"};
                }
            }
            while (n_comp == 0);

            return wc;
        }

    private:
        ibv_qp* qp_;
        ibv_cq* cq_;
    }; // class queue_pair
} // namespace rdma

#endif // KDD_RDMA_QUEUE_PAIR_HPP
