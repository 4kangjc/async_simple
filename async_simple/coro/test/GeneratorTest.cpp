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

#include "async_simple/coro/Generator.h"
#include <iostream>

namespace async_simple::coro {



} // namespace async_simple::coro

using namespace async_simple::coro;
using namespace async_simple;


Generator<uint64_t>
fibonacci_sequence(unsigned n) {
    if (n == 0)
        co_return;
 
    if (n > 94)
        throw std::runtime_error("Too big Fibonacci sequence. Elements would overflow.");
 
    co_yield 0;
 
    if (n == 1)
        co_return;
 
    co_yield 1;
 
    if (n == 2)
        co_return;
 
    uint64_t a = 0;
    uint64_t b = 1;
 
    for (unsigned i = 2; i < n; i++)
    {
        uint64_t s = a + b;
        co_yield s;
        a = b;
        b = s;
        if (i == 11) {
            // throw std::runtime_error("test");
        }
    }
}

Lazy<> Main(int n) {
    try
    {
        auto gen = fibonacci_sequence(n); // max 94 before uint64_t overflows
 
        for (int j = 0; gen; j++) {
            auto i = co_await gen;
            // auto i = gen();
            std::cout << "fib(" << j << ")=" << i << '\n';
        }
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Exception: " << ex.what() << '\n';
    }
    catch (...)
    {
        std::cerr << "Unknown exception.\n";
    }
    co_return;
}

void Test(int n) {
    try
    {
        auto gen = fibonacci_sequence(n); // max 94 before uint64_t overflows
        
        for (int j = 0; gen; j++) {
            auto i = gen();
            std::cout << "fib(" << j << ")=" << i << '\n';
        }
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Exception: " << ex.what() << '\n';
    }
    catch (...)
    {
        std::cerr << "Unknown exception.\n";
    }
}

// int main() {
//     // Test(13);
//     Main(13).start([](auto&&){

//     });
//     //
//     fibonacci_sequence(10).start([](async_simple::Try<std::vector<uint64_t>>
//     t) {
//     //     if (t.hasError()) {
//     //         return;
//     //     }
//     //     for (auto i : t.value()) {
//     //         std::cout << i << std::endl;
//     //     }
//     // });
// }
