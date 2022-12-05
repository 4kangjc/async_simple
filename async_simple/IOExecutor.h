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
#ifndef ASYNC_SIMPLE_IO_EXECUTOR_H
#define ASYNC_SIMPLE_IO_EXECUTOR_H

#include <functional>
#include <string>

namespace async_simple {

struct iovec_t {
    void* iov_base;
    size_t iov_len;
};

// IO type
using IOOPCode = unsigned int;

// [Require] User implements an IOCallback
struct IOCallback;

// The IOExecutor would accept IO read/write requests.
// After the user implements an IOExecutor, he should associate
// the IOExecutor with the corresponding Executor implementation.
class IOExecutor {
public:
    using Func = std::function<void()>;

    IOExecutor() {}
    virtual ~IOExecutor() {}

    IOExecutor(const IOExecutor&) = delete;
    IOExecutor& operator=(const IOExecutor&) = delete;

public:
    virtual void submitIO(int fd, IOOPCode cmd, void* buffer, size_t length,
                          off_t offset, IOCallback cbfn) = 0;
    virtual void submitIOV(int fd, IOOPCode cmd, const iovec_t* iov,
                           size_t count, off_t offset, IOCallback cbfn) = 0;
};

}  // namespace async_simple

#endif
