// This example is implemented using the following as a reference:
//
//   https://github.com/linux-rdma/rdma-core/blob/master/librdmacm/examples/rdma_xclient.c
//

#include <rdma/rdma_cma.h>
#include <rdma/rdma_verbs.h>

#include <iostream>
#include <string>
#include <stdexcept>

namespace rdma
{
    class event_channel
    {
    public:
        event_channel()
            : evt_ch_{rdma_create_event_channel()}
        {
            if (!evt_ch_)
                throw std::runtime_error{"event_channel construction error."};
        }

        ~event_channel()
        {
            if (evt_ch_)
                rdma_destroy_event_channel(evt_ch_);
        }

        operator rdma_event_channel*() const noexcept
        {
            return evt_ch_;
        }

    private:
        struct rdma_event_channel* evt_ch_;
    }; // class event_channel

    class address_info
    {
    public:
        address_info(const char* _host, const char* _port)
            : addr_info_{}
        {
            rdma_addrinfo hints{.ai_port_space = RDMA_PS_TCP};

            const auto ec = rdma_getaddrinfo(const_cast<char*>(_host),
                                             const_cast<char*>(_port),
                                             &hints,
                                             &addr_info_);

            if (ec)
                throw std::runtime_error{"rdma_address_info construction error."};
        }

        ~address_info()
        {
            rdma_freeaddrinfo(addr_info_);
        }

        operator rdma_addrinfo*() const noexcept
        {
            return addr_info_;
        }

    private:
        rdma_addrinfo* addr_info_;
    }; // class address_info

    class communication_identifier
    {
    public:
        explicit communication_identifier(const address_info& _addr_info)
            : id_{}
        {
            ibv_qp_init_attr attrs{};
            attrs.cap.max_send_wr = attrs.cap.max_recv_wr = 1;
            attrs.cap.max_send_sge = attrs.cap.max_recv_sge = 1;
            attrs.sq_sig_all = 1;

            const auto ec = rdma_create_ep(&id_, _addr_info, nullptr, &attrs);
            
            if (ec)
                throw std::runtime_error{"communication_identifier construction error."};
        }

        ~communication_identifier()
        {
            if (id_)
                rdma_destroy_ep(id_);
        }

        operator rdma_cm_id*() const noexcept
        {
            return id_;
        }

    private:
        rdma_cm_id* id_;
    }; // class communication_identifier

    class memory_region
    {
    public:
        memory_region(const communication_identifier& _comm_id,
                      const std::uint8_t* _buffer,
                      std::uint64_t _buffer_size)
        {
            mr_ = rdma_reg_msgs(_comm_id, const_cast<std::uint8_t*>(_buffer), _buffer_size);

            if (!mr_)
                throw std::runtime_error{"memory_region construction error."};
        }

        ~memory_region()
        {
            if (mr_)
                rdma_dereg_mr(mr_);
        }

    private:
        ibv_mr* mr_;
    }; // class memory_region
} // namespace rdma

auto main(int _argc, char* _argv[]) -> int
{
    const char* host = "localhost";
    const char* port = "7571";

    rdma_cm_id* id{};
    ibv_mr* mr{};
    ibv_mr* send_mr{};

    ibv_wc wc{};

    rdma::event_channel evt_channel;
    rdma::address_info addr_info{host, port};
    rdma::communication_identifier comm_id{addr_info};

    const std::string msg = "Hello, this was sent using RDMA!";
    const auto* data = reinterpret_cast<const std::uint8_t*>(msg.data());
    rdma::memory_region mem_region{comm_id, data, msg.size() + 1};

    return 0;
}

