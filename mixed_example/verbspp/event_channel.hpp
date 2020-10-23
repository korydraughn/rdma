#ifndef VERBSPP_EVENT_CHANNEL_HPP
#define VERBSPP_EVENT_CHANNEL_HPP

#include <rdma/rdma_cma.h>

#include <stdexcept>

namespace verbs
{
    class event_channel
    {
    public:
        event_channel()
            : ch_{rdma_create_event_channel()}
        {
            if (!ch_)
                throw std::runtime_error{""};
        }

        event_channel(const event_channel&) = delete;
        event_channel operator=(const event_channel&) = delete;

        ~event_channel()
        {
            if (ch_)
                rdma_destroy_event_channel(ch_);
        }

    private:
        rdma_event_channel* ch_;
    };
} // namespace verbs

#endif // VERBSPP_EVENT_CHANNEL_HPP
