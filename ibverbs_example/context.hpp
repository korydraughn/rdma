#ifndef KDD_RDMA_CONTEXT_HPP
#define KDD_RDMA_CONTEXT_HPP

#include "device_list.hpp"

#include <infiniband/verbs.h>

#include <stdio.h>
#include <errno.h>

#include <stdexcept>

namespace rdma
{
    class context
    {
    public:
        explicit context(const device& _device)
            : ctx_{ibv_open_device(&_device.handle())}
        {
            if (!ctx_) {
                perror("ibv_open_device");
                throw std::runtime_error{"ibv_open_device error"};
            }
        }

        context(const context&) = delete;
        auto operator=(const context&) -> context& = delete;

        ~context()
        {
            if (ctx_)
                ibv_close_device(ctx_);
        }

        auto handle() const noexcept -> ibv_context&
        {
            return *ctx_;
        }

        auto device_info() const -> ibv_device_attr
        {
            ibv_device_attr attrs{};

            if (ibv_query_device(ctx_, &attrs)) {
                perror("ibv_query_device");
                throw std::runtime_error{"ibv_query_device error"};
            }

            return attrs;
        }

        auto port_info(std::uint8_t _port_number) const -> ibv_port_attr
        {
            ibv_port_attr attrs{};

            if (ibv_query_port(ctx_, _port_number, &attrs)) {
                perror("ibv_query_port");
                throw std::runtime_error{"ibv_query_port error"};
            }

            return attrs;
        }

        auto pkey(std::uint8_t _port_number, int _pkey_index) const -> std::uint16_t
        {
            std::uint16_t pkey;

            if (ibv_query_pkey(ctx_, _port_number, _pkey_index, &pkey)) {
                perror("ibv_query_pkey");
                throw std::runtime_error{"ibv_query_pkey error"};
            }

            return pkey;
        }

        auto gid(std::uint8_t _port_number, int _pkey_index) const -> ibv_gid
        {
            ibv_gid gid;

            if (ibv_query_gid(ctx_, _port_number, _pkey_index, &gid)) {
                perror("ibv_query_gid");
                throw std::runtime_error{"ibv_query_gid error"};
            }

            return gid;
        }

    private:
        ibv_context* ctx_;
    }; // class context
} // namespace rdma

#endif // KDD_RDMA_CONTEXT_HPP
