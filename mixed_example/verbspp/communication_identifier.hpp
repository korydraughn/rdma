#ifndef VERBPP_CONTEXT_HPP
#define VERBPP_CONTEXT_HPP

#include "event_channel.hpp"

#include <rdma/rdma_cma.h>

#include <thread>
#include <stdexcept>

namespace verbs
{
    class communication_identifier
    {
    public:
        communication_identifier(rdma_port_space _ps,
                                 const event_channel& _evt_ch,
                                 const event_handler& _evt_handler,
                                 void* _user_data = nullptr)
            : id_{}
            , evt_th_{}
        {
            if (const auto ec = rdma_create_id(_evt_ch.handle(), &id_, _user_data, _ps); ec)
                throw std::runtime_error{"rdma_create_id failed"};

            start_event_processing_thread(*_evt_ch.handle());
        }

        communication_identifier(rdma_port_space _ps, void* _user_data = nullptr)
            : id_{}
            , evt_th_{}
        {
            if (const auto ec = rdma_create_id(nullptr, &id_, _user_data, _ps); ec)
                throw std::runtime_error{"rdma_create_id failed"};
        }

        communication_identifier(const communication_identifier&) = delete;
        communication_identifier operator=(const communication_identifier&) = delete;

        rdma_cm_id* handle()
        {
            return id_;
        }

        void listen(const char* _host, int _port, int _backlog)
        {
            sockaddr sa{};

            sa.sin_family = AF_INET;
            sa.sin_port = htons(_port);
            sa.sin_addr.s_addr = INADDR_ANY;

            if (const auto ec = rdma_bind_addr(id_, &sa); ec)
                throw std::runtime_error{"rdma_bind_addr failed"};

            if (const auto ec = rdma_listen(id_, _backlog); ec)
                throw std::runtime_error{"rdma_listen failed"};
        }

        ~communication_identifier()
        {
            if (id_)
                rdma_destroy_id(id_);

            if (evt_th_.joinable())
                evt_th_.join();
        }

    private:
        void start_event_processing_thread(rdma_event_channel& _ch)
        {
            evt_th_ = std::thread{[this, &_ch] {
                rdma_cm_event *e = nullptr;

                while (true) {
                    if (const auto ec = rdma_get_cm_event(&_ch, &e); ec != 0)
                        throw std::runtime_error{"rdma_get_cm_event failed"};

                    process_event(*e);
                    rdma_ack_cm_event(e);
                }
            }};
        }

        void process_event(const rdma_cm_event& _evt)
        {
            switch (_evt.event) {
                case RDMA_CM_EVENT_ADDR_RESOLVED:
                    break;

                case RDMA_CM_EVENT_ADDR_ERROR:
                    break;

                case RDMA_CM_EVENT_ROUTE_RESOLVED:
                    break;

                case RDMA_CM_EVENT_ROUTE_ERROR:
                    break;

                case RDMA_CM_EVENT_CONNECT_REQUEST:
                    ibv_qp_init_attr qp_attrs{};

                    qp_attrs.

                    if (const auto ec = rdma_create_qp(id_, nullptr, qp_attrs); ec)
                        std::cerr << "Error: could not create QP.\n";
                    break;

                case RDMA_CM_EVENT_CONNECT_RESPONSE:
                    break;

                case RDMA_CM_EVENT_CONNECT_ERROR:
                    break;

                case RDMA_CM_EVENT_UNREACHABLE:
                    break;

                case RDMA_CM_EVENT_REJECTED:
                    break;

                case RDMA_CM_EVENT_ESTABLISHED:
                    break;

                case RDMA_CM_EVENT_DISCONNECTED:
                    break;

                case RDMA_CM_EVENT_DEVICE_REMOVAL:
                    break;

                case RDMA_CM_EVENT_MULTICAST_JOIN:
                    break;

                case RDMA_CM_EVENT_MULTICAST_ERROR:
                    break;

                case RDMA_CM_EVENT_ADDR_CHANGE:
                    break;

                case RDMA_CM_EVENT_TIMEWAIT_EXIT:
                    break;

                default:
                    break;
            }
        }

        rdma_cm_id* id_;
        std::thread evt_th_;
    };
} // namespace verbs

#endif // VERBPP_CONTEXT_HPP
