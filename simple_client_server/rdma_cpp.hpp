// This example is implemented using the following as a reference:
//
//   https://github.com/linux-rdma/rdma-core/blob/master/librdmacm/examples/rdma_xclient.c
//

#include <rdma/rdma_cma.h>
#include <rdma/rdma_verbs.h>

#include <stdexcept>

namespace rdma
{
    enum class app_type
    {
        client,
        server
    };

    class address_info
    {
    public:
        address_info(const char* _host, const char* _port, app_type _app_type)
            : addr_info_{}
        {
            rdma_addrinfo hints{
                .ai_port_space = RDMA_PS_TCP,
                .ai_qp_type = IBV_QPT_RC
            };

            if (app_type::server == _app_type)
                hints.ai_flags = RAI_PASSIVE;

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

    class queue_pair
    {
    public:
        explicit queue_pair(rdma_cm_id& _comm_id)
            : comm_id_{_comm_id}
        {
        }

        auto post_send(std::uint8_t* _buffer, std::uint32_t _buffer_size) -> void
        {
            ibv_sge sge{
                .addr = (std::uint64_t) (std::uintptr_t) _buffer,
                .length = _buffer_size,
                .lkey = 0
            };

            ibv_send_wr wr{
                .wr_id = (std::uintptr_t) nullptr,
                .next = nullptr,

                .sg_list = &sge,
                .num_sge = 1,

                .opcode = IBV_WR_SEND
            };

            ibv_send_wr* bad_wr{};

            auto ec = ibv_post_send(comm_id_.qp, &wr, &bad_wr);

            if (ec)
                throw std::runtime_error{"queue_pair::post_send error."};

            // Wait for send to complete.
            ibv_wc wc{};

            // This function requires that separate rdma_cm_id's must be used for
            // sends and receive completions. This function may be bad for implementing
            // an iRODS transport. How expensive is it to have multiple rdma_cm_id's?
            ec = rdma_get_send_comp(&comm_id_, &wc);

            if (ec <= 0)
                throw std::runtime_error{"queue_pair::post_send completion error."};
        }

        auto post_receive() -> void
        {

        }

    private:
        rdma_cm_id& comm_id_;
    }; // class queue_pair

    class communication_manager
    {
    public:
        explicit communication_manager(const address_info& _addr_info)
            : id_{}
        {
            ibv_qp_init_attr attrs{};
            attrs.cap.max_send_wr = attrs.cap.max_recv_wr = 1;
            attrs.cap.max_send_sge = attrs.cap.max_recv_sge = 1;
            attrs.sq_sig_all = 1;

            const auto ec = rdma_create_ep(&id_, _addr_info, nullptr, &attrs);
            
            if (ec)
                throw std::runtime_error{"communication_manager construction error."};
        }

        explicit communication_manager(rdma_cm_id& _comm_id)
            : id_{&_comm_id}
        {
        }

        ~communication_manager()
        {
            if (id_)
                rdma_destroy_ep(id_);
        }

        operator rdma_cm_id*() const noexcept
        {
            return id_;
        }

        auto connect() const -> queue_pair
        {
            const auto ec = rdma_connect(id_, nullptr);

            if (ec)
                throw std::runtime_error{"communication_manager::connect error."};

            return queue_pair{*id_};
        }

    private:
        rdma_cm_id* id_;
    }; // class communication_manager

    class memory_region
    {
    public:
        memory_region(const communication_manager& _comm_id,
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

        operator ibv_mr*() const noexcept
        {
            return mr_;
        }

    private:
        ibv_mr* mr_;
    }; // class memory_region
} // namespace rdma

