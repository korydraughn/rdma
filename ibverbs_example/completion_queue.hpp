#ifndef KDD_RDMA_COMPLETION_QUEUE_HPP
#define KDD_RDMA_COMPLETION_QUEUE_HPP

#include "context.hpp"

#include <infiniband/verbs.h>

#include <stdio.h>
#include <errno.h>

#include <stdexcept>

namespace rdma
{
    class completion_queue;

    class completion_event_channel
    {
    public:
        explicit completion_event_channel(const context& _c)
            : ctx_{&_c.handle()}
            , evt_ch_{ibv_create_comp_channel(ctx_)}
        {
            if (!evt_ch_) {
                perror("ibv_create_comp_channel");
                throw std::runtime_error{"ibv_create_comp_channel error"};
            }
        }

        completion_event_channel(const completion_event_channel&) = delete;
        auto operator=(const completion_event_channel&) -> completion_event_channel& = delete;

        ~completion_event_channel()
        {
            if (evt_ch_)
                ibv_destroy_comp_channel(evt_ch_);
        }

        auto handle() const noexcept -> ibv_comp_channel&
        {
            return *evt_ch_;
        }

        friend class completion_queue;

    private:
        ibv_context* ctx_;
        ibv_comp_channel* evt_ch_;
    }; // completion_event_channel

    class completion_queue
    {
    public:
        completion_queue(int _cpe_size, const context& _ctx)
            : cq_{ibv_create_cq(&_ctx.handle(), _cpe_size, nullptr, nullptr, 0)}
        {
            if (!cq) {
                perror("ibv_create_cq");
                throw std::runtime_error{"ibv_create_cq error"};
            }
        }

        completion_queue(int _cpe_size, const completion_event_channel& _evt_ch)
            : cq_{ibv_create_cq(_evt_ch.ctx_, _cpe_size, nullptr, _evt_ch_.evt_ch_, 0)}
        {
            if (!cq) {
                perror("ibv_create_cq");
                throw std::runtime_error{"ibv_create_cq error"};
            }
        }

        completion_queue(const completion_queue&) = delete;
        auto operator=(const completion_queue&) -> completion_queue& = delete;

        ~completion_queue()
        {
            if (cq_)
                ibv_destroy_cq(cq_);
        }

        auto resize(int _new_size) const -> void
        {
            if (_new_size < 1)
                throw std::invalid_argument{"completion queue size must be greater than 0."};

            if (ibv_resize_cq(cq_, _new_size)) {
                perror("ibv_resize_cq");
                throw std::runtime_error{"ibv_resize_cq error"};
            }
        }

    private:
        ibv_cq* cq_;
    }; // class completion_queue
} // namespace rdma

#endif // KDD_RDMA_COMPLETION_QUEUE_HPP
