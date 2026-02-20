#include "runtime/EngineRuntime.hpp"
#include <pthread.h>
#include <sched.h>
#include <chrono>

namespace chimera {

EngineRuntime::EngineRuntime(InstitutionalEngine& engine)
    : engine_(engine)
{}

EngineRuntime::~EngineRuntime() {
    stop();
}

void EngineRuntime::pin_thread_to_core(std::thread& t,
                                       int core_id)
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);

    pthread_setaffinity_np(
        t.native_handle(),
        sizeof(cpu_set_t),
        &cpuset);
}

void EngineRuntime::start()
{
    running_ = true;

    market_thread_ =
        std::thread(&EngineRuntime::market_loop, this);

    execution_thread_ =
        std::thread(&EngineRuntime::execution_loop, this);

    pin_thread_to_core(market_thread_, 0);
    pin_thread_to_core(execution_thread_, 1);
}

void EngineRuntime::stop()
{
    running_ = false;

    if (market_thread_.joinable())
        market_thread_.join();

    if (execution_thread_.joinable())
        execution_thread_.join();
}

void EngineRuntime::post_market_event(
    const std::string&,
    std::function<void()> fn)
{
    while (!market_ring_.push(fn))
        std::this_thread::yield();
}

void EngineRuntime::post_execution_event(
    const std::string&,
    std::function<void()> fn)
{
    while (!exec_ring_.push(fn))
        std::this_thread::yield();
}

void EngineRuntime::market_loop()
{
    while (running_)
    {
        std::function<void()> task;

        if (market_ring_.pop(task))
        {
            if (task)
                task();
        }
        else
        {
            std::this_thread::yield();
        }
    }
}

void EngineRuntime::execution_loop()
{
    while (running_)
    {
        std::function<void()> task;

        if (exec_ring_.pop(task))
        {
            if (task)
                task();
        }
        else
        {
            std::this_thread::yield();
        }
    }
}

}
