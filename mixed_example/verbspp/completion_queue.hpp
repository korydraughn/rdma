#ifndef VERBSPP_COMPLETION_QUEUE_HPP
#define VERBSPP_COMPLETION_QUEUE_HPP

namespace verbs
{
    class completion_queue
    {
    public:
        completion_queue(const communication_identifier& _cm_id)
            : cq_{ibv_create_cq(&_cm_id.context(), )}
        {
        }

        completion_queue(const completion_queue&) = delete;
        completion_queue& operator=(const completion_queue&) = delete;

        ~completion_queue()
        {
            if (cq_)
                ibv_destroy_cq(cq_);
        }

    private:
        ibv_cq* cq_;
    };
} // namespace verbs

#endif // VERBSPP_COMPLETION_QUEUE_HPP
