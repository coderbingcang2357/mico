/**
 *  Libevent.cpp
 *
 *  Test program to check AMQP functionality based on Libevent
 *
 *  @author Brent Dimmig <brentdimmig@gmail.com>
 */

/**
 *  Dependencies
 */

#pragma once

#include <iostream>
#include <amqpcpp/exchangetype.h>
#include <amqpcpp/flags.h>
#include <amqpcpp/login.h>
#include <event2/event.h>
#include <amqpcpp.h>
#include <amqpcpp/libevent.h>
#include <string>
#include <vector>
#include <list>
#include <thread>
#include <mutex>
#include <functional>

class RabbitMq {

    event_base *evbase;
    struct event *queuetimer;
    struct event *heartbeatimer;

    std::string url;
    std::string exchange;
    std::string queue;
    std::string routkey;

    AMQP::LibEventHandler *handler;
    AMQP::TcpConnection *connection;
    AMQP::TcpChannel *channel;

    std::function<void(const std::string &msg)> consumcb;

    std::list<std::string> msgqueue;
    std::mutex lockqueue;
    //event
    //
    void processQueue()
    {
        std::list<std::string> mq;
        {
            std::lock_guard<std::mutex> g(lockqueue);
            mq.swap(msgqueue);
        }

        for (auto &m : mq)
        {
            channel->publish(this->exchange, this->routkey, m).onSuccess([](){
            }).onError([&m](const char *msg){
                printf("public msg %s failed: %s\n", m.c_str(), msg);
            });
        }
    }

    void heartbeat()
    {
        connection->heartbeat();
    }

public:
    bool publish(const std::string &m)
    {
        std::lock_guard<std::mutex> g(lockqueue);
        msgqueue.push_back(m);
        return true;
    }

    bool consume(std::function<void(const std::string &msg)> cb)
    {
        consumcb = cb;
        channel->consume(this->queue).onReceived([this](const AMQP::Message &message,
                    uint64_t deliveryTag,
                    bool redelivered){
                consumcb(std::string(message.body(), message.bodySize()));
                channel->ack(deliveryTag);
                }).onError([](const char *msg){
                printf("consume failed: %s\n", msg);
        });
        return true;
    }

    static void heartbeatCallback(evutil_socket_t fd, short what, void *arg)
    {
        RabbitMq *rm = (RabbitMq *)arg;
        rm->heartbeat();
    }

    static void timerCallback(evutil_socket_t fd, short what, void *arg)
    {
        RabbitMq *rm = (RabbitMq *)arg;
        rm->processQueue();
    }


/**
 *  Main program
 *  @return int
 */
    RabbitMq(std::string url,
            const std::string &exchange,
            const std::string &queue,
            const std::string &routkey)
    {
        this->url = url;
        this->exchange = exchange;
        this->queue = queue;
        this->routkey = routkey;
    }

    ~RabbitMq()
    {
        event_free(queuetimer);
        event_free(heartbeatimer);
        event_base_free(evbase);
        delete channel;
        delete connection;
        delete handler;
    }

    bool init()
    {
        // access to the event loop
        evbase = event_base_new();

        // queuetimer
        struct timeval one_sec = { 0, 10 * 1000 }; // 10ms
        //struct timeval one_sec = { 0, 10 * 1000 }; // 10ms
        queuetimer = event_new(evbase, -1, EV_PERSIST, timerCallback, this);
        event_add(queuetimer, &one_sec);

        // heartbeattimer
        struct timeval sec30 = { 30, 10 * 1000 }; // 10ms
        heartbeatimer = event_new(evbase, -1, EV_PERSIST, heartbeatCallback, this);
        event_add(heartbeatimer, &sec30);

        // handler for libevent
        handler = new AMQP::LibEventHandler(evbase);

        AMQP::Address addr(url);
        // make a connection
        connection = new AMQP::TcpConnection(handler, addr);

        // we need a channel too
        channel = new AMQP::TcpChannel(connection);
        channel->onError([](const char *msg){
                printf("channel error: %s", msg);
                });

        channel->declareExchange(this->exchange, AMQP::direct, AMQP::durable).onSuccess([](){
            printf("declareExchange success.\n");
        }).onError([] (const char *msg){
            printf("error: %s.\n", msg);
        });

        // create a temporary queue
        channel->declareQueue(this->queue, AMQP::durable).onSuccess([](const std::string &name, uint32_t messagecount, uint32_t consumercount) {
            // report the name of the temporary queue
            std::cout << "declared queue " << name << std::endl;
            // now we can close the connection
            // connection.close();
        });

        channel->bindQueue(this->exchange, this->queue, this->routkey).onSuccess([](){
                printf("bind success.\n");
            }).onError([](const char *msg){
                printf("bind error: %s.\n", msg);
            });


        // done
        return 0;
    }

    bool run()
    {
        // run the loop
        event_base_dispatch(evbase);

        return true;
    }

    bool quit()
    {
        printf("break\n");
        event_del(queuetimer);
        event_del(heartbeatimer);
        event_base_loopbreak(evbase);
        printf("after break\n");
        return true;
    }
};

