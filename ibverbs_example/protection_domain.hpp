#ifndef KDD_RDMA_PROTECTION_DOMAIN_HPP
#define KDD_RDMA_PROTECTION_DOMAIN_HPP

#include "context.hpp"

#include <infiniband/verbs.h>

#include <stdio.h>
#include <errno.h>

#include <stdexcept>

namespace rdma
{
    class protection_domain
    {
    public:
        explicit protection_domain(const context& _c)
            : pd_{ibv_alloc_pd(&_c.handle())}
        {
            if (!pd_) {
                perror("ibv_alloc_pd");
                throw std::runtime_error{"ibv_alloc_pd error"};
            }
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
} // namespace rdma

#endif // KDD_RDMA_PROTECTION_DOMAIN_HPP
