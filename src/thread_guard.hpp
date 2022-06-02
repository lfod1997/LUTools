// Created: 2022-05-30

#ifndef _THREAD_GUARD_HPP_
#define _THREAD_GUARD_HPP_

#include <type_traits>
#include <stdexcept>

template <typename ThreadTy>
class ThreadGuard {
    ThreadTy _th;

public:
#pragma region Move-construct-only

    ThreadGuard(const ThreadGuard&) = delete;

    ThreadGuard& operator=(const ThreadGuard&) = delete;

    ThreadGuard(ThreadGuard&& src) noexcept:
        _th(std::move(src._th)) {}

    ThreadGuard& operator=(ThreadGuard&&) = delete; // No discard of own thread!

#pragma endregion

    template <typename ...ValTy>
    explicit ThreadGuard(ValTy&& ...args):
        _th(ThreadTy { std::forward<ValTy>(args)... }) {}

    explicit ThreadGuard(ThreadTy&& th):
        _th(std::move(th)) {
        if (!_th.joinable()) {
            throw std::logic_error { "no valid thread" };
        }
    }

    ~ThreadGuard() noexcept {
        _th.join();
    }
};

#endif // _THREAD_GUARD_HPP_
