#ifndef KDD_RDMA_MEMORY_REGION_HPP
#define KDD_RDMA_MEMORY_REGION_HPP

#include "context.hpp"
#include "protection_domain.hpp"

#include <infiniband/verbs.h>

#include <stdio.h>
#include <errno.h>

#include <vector>
#include <stdexcept>

namespace rdma
{
    class memory_region
    {
    public:
        memory_region(const protection_domain& _pd,
                      const std::vector<std::uint8_t>& _buffer,
                      ibv_access_flags _access)
            : mr_{ibv_reg_mr(&_pd.handle(), const_cast<std::uint8_t*>(_buffer.data()), _buffer.size(), _access)}
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
} // namespace rdma

#endif // KDD_RDMA_MEMORY_REGION_HPP
