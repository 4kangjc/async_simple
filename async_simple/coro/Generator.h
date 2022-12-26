/*
 * Copyright (c) 2022, Alibaba Group Holding Limited;
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef ASYNC_SIMPLE_CORO_GENERATOR_H
#define ASYNC_SIMPLE_CORO_GENERATOR_H

#include "async_simple/coro/Lazy.h"

namespace async_simple::coro {

template <typename T>
class Generator;

namespace detail {

template <typename T>
class GeneratorPromise : public LazyPromiseBase {
public:
    // Unlike `Lazy`, Generator will run directly when it is created until it is
    // suspended for the first time
    std::suspend_never initial_suspend() noexcept { return {}; }

    template <std::convertible_to<T> From>
    FinalAwaiter yield_value(From&& from) {
        _value.emplace(std::forward<From>(from));
        return {};
    }
    void return_void() noexcept {}
    void unhandled_exception() noexcept {
        _value.setException(std::current_exception());
    }

    Generator<T> get_return_object() noexcept;
public:
    T& result() & {
        return _value.value();
    }
    T&& result() && {
        return std::move(_value).value();
    }

    Try<T> tryResult() noexcept {
        return std::move(_value);
    }

    void checkHasError() const {
        if (_value.hasError()) {
            std::rethrow_exception(_value.getException());
        }
    }
protected:
    Try<T> _value;
};

template <typename T>
struct GeneratorAwaiterBase {
    using Handle = CoroHandle<GeneratorPromise<T>>;
    Handle _handle;
    Try<T> _preValue;

    GeneratorAwaiterBase(GeneratorAwaiterBase& other) = delete;
    GeneratorAwaiterBase& operator=(GeneratorAwaiterBase& other) = delete;

    GeneratorAwaiterBase(Handle coro) : _handle(coro) {}

    bool await_ready() noexcept {
        _preValue = std::move(_handle.promise()).tryResult();
        return false; 
    }
    auto awaitResume() {
        _handle.promise().checkHasError();
        if constexpr (std::is_void_v<T>) {
            _preValue.value();
        } else {
            return std::move(_preValue).value();
        }
    }

    Try<T> awaitResumeTry() noexcept {
        return std::move(_preValue);
    }
};

template <typename T, bool reschedule>
class GeneratorBase {
public:
    using promise_type = GeneratorPromise<T>;
    using Handle = CoroHandle<promise_type>;

    // It is used in syncAwait. See syncAwait.h
    using ValueType = std::vector<T>;

    struct AwaiterBase : public GeneratorAwaiterBase<T> {
        using Base = GeneratorAwaiterBase<T>;
        AwaiterBase(Handle coro) : Base(coro) {}

        AS_INLINE auto await_suspend(
            std::coroutine_handle<> continuation) noexcept(!reschedule) {
            // current coro started, caller becomes my continuation
            this->_handle.promise()._continuation = continuation;

            return awaitSuspendImpl();
        }

    private:
        auto awaitSuspendImpl() noexcept(!reschedule) {
            if constexpr (reschedule) {
                // executor schedule performed
                auto& pr = this->_handle.promise();
                logicAssert(pr._executor, "RescheduleLazy need executor");
                pr._executor->schedule(
                    [h = this->_handle]() mutable { h.resume(); });
            } else {
                return this->_handle;
            }
        }
    };

    struct TryAwaiter : public AwaiterBase {
        TryAwaiter(Handle coro) : AwaiterBase(coro) {}
        AS_INLINE Try<T> await_resume() noexcept {
            return AwaiterBase::awaitResumeTry();
        };
    };

    struct ValueAwaiter : public AwaiterBase {
        ValueAwaiter(Handle coro) : AwaiterBase(coro) {}
        AS_INLINE T await_resume() { return AwaiterBase::awaitResume(); }
    };

    ~GeneratorBase() {
        if (_coro) {
            assert(_coro.done());
            _coro.destroy();
            _coro = nullptr;
        }
    }
    explicit GeneratorBase(Handle coro) : _coro(coro) {}
    GeneratorBase(GeneratorBase&& other) : _coro(std::move(other._coro)) {
        other._coro = nullptr;
    }

    GeneratorBase(const GeneratorBase&) = delete;
    GeneratorBase& operator=(const GeneratorBase&) = delete;

    Executor* getExecutor() { return _coro.promise()._executor; }

    template <class Container = std::vector<T>, class F>
    void start(F&& callback) requires(std::is_invocable_v<F, Try<Container>>) {
        Try<Container> try_result;
        try {
            Container yield_value;
            while (*this) {
                yield_value.emplace_back(this->operator()());
            }
            try_result.emplace(std::move(yield_value));
        } catch(...) {
            try_result.setException(std::current_exception());
        }
        callback(std::move(try_result));
    }

    bool isReady() const { return !_coro || _coro.done(); }
    explicit operator bool() const {
        if (_coro) {
            _coro.promise().checkHasError();
            return !_coro.done();
        } else {
            return false;
        }
    }
    T operator()() const {
        if (_coro) {
            auto res = std::move(_coro.promise().result());
            _coro();
            
            return res;
        } else {
            assert(false);
        }
    }

    auto coAwaitTry() { return TryAwaiter(_coro); }
protected:
    Handle _coro;
};

} // namespace detail


template <typename T>
class [[nodiscard]] Generator : public detail::GeneratorBase<T, false> {
    using Base = detail::GeneratorBase<T, false>;
public:
    using Base::Base;

    auto coAwait(Executor* ex) {
        this->_coro.promise()._executor = ex;
        return typename Base::ValueAwaiter(this->_coro);
    }

};

template <typename T>
inline Generator<T> detail::GeneratorPromise<T>::get_return_object() noexcept {
    return Generator<T>(Generator<T>::Handle::from_promise(*this));
}

} // namespace async_simple::coro

#endif
