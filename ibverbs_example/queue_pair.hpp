#ifndef KDD_RDMA_QUEUE_PAIR_HPP
#define KDD_RDMA_QUEUE_PAIR_HPP

#include "context.hpp"
#include "protection_domain.hpp"
#include "memory_region.hpp"

#include <infiniband/verbs.h>

#include <stdio.h>
#include <errno.h>

#include <tuple>
#include <vector>
#include <stdexcept>

namespace rdma
{
    class queue_pair
    {
    public:
        queue_pair(const protection_domain& _pd, const ibv_qp_init_attr& _attrs)
            : qp_{ibv_create_qp(&_pd.handle(), &_attrs)}
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

        auto modify_attribute(const ibv_qp_attr& _attr, ibv_qp_attr_mask _mask) const -> void
        {
            if (ibv_modify_qp(qp_, &_attr, _mask)) {
                perror("ibv_modify_qp");
                throw std::invalid_argument{"ibv_modify_qp error"};
            }
        }

        auto query_attribute(ibv_qp_attr_mask _mask) const -> std::tuple<ibv_qp_attr, ibv_qp_init_attr>
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

        auto post_send(const std::vector<std::uint8_t>& _buffer,
                       const memory_region& _mr) -> void
        {
            // TODO The incoming buffer could be registered for the duration
            // of this function call. This would simplify usage of the library
            // by removing the need for users to create memory regions outside
            // of the queue pair. This also applies to post_receive().

            ibv_sge sg_list{};
            sg_list.addr = reinterpret_cast<std::uintptr_t>(_buffer.data());
            sg_list.length = _buffer.size();
            sg_list.lkey = _mr.local_key();

            ibv_send_wr wr{};
            wr.opcode = IBV_WR_SEND;
            wr.sg_list = &sg_list;
            wr.num_sge = 1;

            ibv_send_wr* bad_wr;

            if (ibv_post_send(qp_, &wr, &bad_wr)) {
                perror("ibv_post_send");
                throw std::runtime_error{"ibv_post_send error"};
            }

            // TODO Poll completion queue and wait for send work completion.
        }

        auto post_receive(std::vector<std::uint8_t>& _buffer,
                          const memory_region& _mr) -> void
        {
            // TODO The incoming buffer could be registered for the duration
            // of this function call. This would simplify usage of the library
            // by removing the need for users to create memory regions outside
            // of the queue pair. This also applies to post_receive().

            ibv_sge sg_list{};
            sg_list.addr = reinterpret_cast<std::uintptr_t>(_buffer.data());
            sg_list.length = _buffer.size();
            sg_list.lkey = _mr.local_key();

            ibv_recv_wr wr{};
            wr.sg_list = &sg_list;
            wr.num_sge = 1;

            ibv_send_wr* bad_wr;

            if (ibv_post_recv(qp_, &wr, &bad_wr)) {
                perror("ibv_post_recv");
                throw std::runtime_error{"ibv_post_recv error"};
            }

            // TODO Poll completion queue and wait for receive work completion.
        }

    private:
        ibv_qp* qp_;
    }; // class queue_pair
} // namespace rdma

#endif // KDD_RDMA_QUEUE_PAIR_HPP
