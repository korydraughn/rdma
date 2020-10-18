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

        // TODO Add functions that allow querying the device properties
        // (e.g. device attributes, port properties, pkey, port GID).

    private:
        ibv_context* ctx_;
    }; // class context

    class protection_domain
    {
    public:
        explicit protection_domain(const context& _c)
            : pd_{ibv_alloc_pd(&_c.handle())}
        {
        }

        protection_domain(const protection_domain&) = delete;
        auto operator=(const protection_domain&) -> protection_domain& = delete;

        ~protection_domain()
        {
            if (pd_)
                ibv_dealloc_pd(pd_);
        }

        auto handle() const noexcept -> ibv_pd&
        {
            return *pd_;
        }

    private:
        ibv_pd* pd_;
    }; // class protection_domain

    class memory_region
    {
    public:
        memory_region(const protection_domain& _pd,
                      const std::vector<std::uint8_t>& _buffer,
                      ibv_access_flags _access)
            : mr_{ibv_reg_mr(&_pd.handle(), _buffer.data(), _buffer.size(), _access)}
        {
            if (!mr_) {
                perror("ibv_reg_mr");
                throw std::runtime_error{"ibv_reg_mr error"};
            }
        }

        memory_region(const memory_region&) = delete;
        auto operator=(const memory_region&) -> memory_region& = delete;

        ~memory_region()
        {
            if (mr_)
                ibv_dereg_mr(mr_);
        }

        auto local_key() const
        {
            return mr_->lkey;
        }

        auto remote_key() const
        {
            return mr_->rkey;
        }

        auto memory_address() const
        {
            return mr_->addr;
        }

        auto memory_size() const
        {
            return mr_->length;
        }

    private:
        ibv_mr* mr_;
    }; // class memory_region

    class completion_queue
    {
    public:
        completion_queue(int _comp_queue_size, const context& _ctx)
            : cq_{ibv_create_cq(&_ctx.handle(), _comp_queue_size, nullptr, nullptr, 0)}
        {
            if (!cq) {
                perror("ibv_create_cq");
                throw std::runtime_error{"ibv_create_cq error"};
            }
        }

        completion_queue(int _comp_queue_size, const completion_event_channel& _evt_ch)
            : cq_{ibv_create_cq(_evt_ch.ctx_, _comp_queue_size, nullptr, _evt_ch_.evt_ch_, 0)}
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

    class device
    {
    public:
        explicit device(ibv_device& _device)
            : device_{_device}
        {
        }

        device(const device&) = delete;
        auto operator=(const device&) -> device& = delete;

        auto handle() const noexcept -> ibv_device&
        {
            return *device_;
        }

        auto guid() const noexcept -> std::uint64_t
        {
            return ibv_get_device_guid(device_);
        }

        auto name() const noexcept -> const char*
        {
            return ibv_get_device_name(device_);
        }

    private:
        ibv_device* device_;
    }; // class device

    class device_list
    {
    public:
        device_list()
            , devices_{}
            : num_devices_{}
        {
            devices_ = ibv_get_device_list(&num_devices_);

            if (!devices_) {
                perror("ibv_get_device_list");
                throw std::runtime_error{"ibv_get_device_list error."};
            }
        }

        device_list(const device_list&) = delete;
        auto operator=(const device_list&) -> device_list& = delete;

        ~device_list()
        {
            if (devices_)
                ibv_free_device_list(devices_);
        }

        auto operator[](int _index) -> device
        {
            if (_index < 0 || _index >= num_devices_)
                throw std::out_of_range{"device index out of range."};

            return device{*devices_[_index]};
        }

        auto size() const noexcept -> int
        {
            return num_devices_;
        }

        auto empty() const noexcept -> bool
        {
            return 0 == num_devices_;
        }

    private:
        ibv_device** devices_;
        int num_devices_;
    }; // class device_list
} // namespace rdma

