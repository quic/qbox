/*
* Copyright (c) 2022 GreenSocs
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version, or under the
* Apache License, Version 2.0 (the "License”) at your discretion.
*
* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
* You may obtain a copy of the Apache License at
* http://www.apache.org/licenses/LICENSE-2.0
*/

#ifndef SEMAPHORE_H
#define SEMAPHORE_H

namespace gs {

#include <mutex>
#include <condition_variable>

    class semaphore {
    private:
        std::mutex mutex_;
        std::condition_variable condition_;
        unsigned long count_ = 0; // Initialized as locked.

    public:
        void notify() {
            std::lock_guard<decltype(mutex_)> lock(mutex_);
            ++count_;
            condition_.notify_one();
        }

        void wait() {
            std::unique_lock<decltype(mutex_)> lock(mutex_);
            while (!count_) // Handle spurious wake-ups.
                condition_.wait(lock);
            --count_;
        }

        bool try_wait() {
            std::lock_guard<decltype(mutex_)> lock(mutex_);
            if (count_) {
                --count_;
                return true;
            }
            return false;
        }

        semaphore() = default;
        ~semaphore() {
            // don't block on destruction
            notify();
        }
    };
}

#endif // SEMAPHORE_H
