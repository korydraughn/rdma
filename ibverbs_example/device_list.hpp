#ifndef KDD_RDMA_DEVICE_LIST_HPP
#define KDD_RDMA_DEVICE_LIST_HPP

#include <infiniband/verbs.h>

#include <stdio.h>
#include <errno.h>

#include <stdexcept>

namespace rdma
{
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
                throw std::runtime_error{"ibv_get_device_list error"};
            }
        }

        device_list(const device_list&) = delete;
        auto operator=(const device_list&) -> device_list& = delete;

        ~device_list()
        {
            if (devices_)
                ibv_free_device_list(devices_);
        }

        auto operator[](int _index) const -> device
        {
            if (_index < 0 || _index >= num_devices_)
                throw std::out_of_range{"device index out of range"};

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

#endif // KDD_RDMA_DEVICE_LIST_HPP
