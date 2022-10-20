/*
 *  This file is part of libgsutils
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA. You may obtain a copy of the Apache License at
 * http://www.apache.org/licenses/LICENSE-2.0
 */

/*
 * GTest helper to workaround SystemC limitations regarding its non-resettable
 nature.
 *
 * You can use the TEST_BENCH macro as you would use the TEST_F GTest macro.
 * Your fixture class must inherit from the TestBench class.
 *
 * Before each test, the test program is forked so that it starts from a fresh
 * SystemC environment.
 *
 *
 * Using GDB
 * =========
 *
 * Note that debugging with GDB is a little more involved, because of the forks.
 * You can do something like this:
 *
 * (gdb) set detach-on-fork off
 * (gdb) set non-stop on
 * (gdb) b somewhere
 * (gdb) r
 * Thread 2.1 "xxx" hit Breakpoint 1, somewhere() (...)
    at ...

 * (gdb) thread 2.1
 * (gdb) ... you can now debug as normal in the child
 *
 * Note that in non-stop mode, other inferiors are still running. To stop one,
 * select it with the `thread' command and issue the `interrupt' command.
 *
 * Example
 * =======
 *
 * class MyTestBench : public TestBench {
 * private:
 *     SomeModule m_module_under_test;
 *     AnotherModule m_mock_module;
 *
 * public:
 *     MyTestBench(const sc_core::sc_module_name &n)
 *         : TestBench(n)
 *         , m_module_under_test("module-under-test")
 *         , m_mock_module("mock-module")
 *     {
 * m_mock_module.target_socket.bind(m_module_under_test.initiator_socket);
 *     }
 * };
 *
 * TEST_BENCH(MyTestBench, Test0)
 * {
 *     m_module_under_test.do_something();
 *     ASSERT_TRUE(m_mock_module.expected_result());
 * }
 *
 * int sc_main(int argc, char *argv[])
 * {
 *     ::testing::InitGoogleTest(&argc, argv);
 *     return RUN_ALL_TESTS();
 * }
 *
 */

#ifndef _GREENSOCS_GSUTILS_TESTS_TEST_BENCH_H
#define _GREENSOCS_GSUTILS_TESTS_TEST_BENCH_H

#include <systemc>

#include <gtest/gtest.h>

class TestBench : public sc_core::sc_module {
protected:
  virtual void test_bench_body() = 0;

public:
  SC_HAS_PROCESS(TestBench);
  TestBench(const sc_core::sc_module_name &n) : sc_core::sc_module(n) {
    SC_THREAD(test_bench_body);
  }
};
template <class T> class TestBenchEnv : public ::testing::Environment {
  // Would be nice to be a smart pointer, but sc_bind takes a pointer.
  T *instance;

public:
  void SetUp() override { instance = new T(); }
  void TearDown() override { delete instance; }
  T *get() { return instance; }
};

#ifndef _WIN32
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

template <class T> static inline void run_test_bench(T *instance) {
  sc_spawn(sc_bind(&T::test_bench_body, instance));
  sc_core::sc_start();
}

#else /* _WIN32 */
#include <iostream>

static inline void run_test_bench() {
  std::cerr << "Running tests on Windows is not supported.\n";
  ASSERT_TRUE(false);
}

#endif

#define TEST_BENCH_NAME(name) TestBench__##name

#define TEST_BENCH(test_bench, name)                                           \
  class TEST_BENCH_NAME(name) : public test_bench {                            \
  protected:                                                                   \
    void test_bench_body() override;                                           \
                                                                               \
  public:                                                                      \
    TEST_BENCH_NAME(name)() : test_bench(#name) {}                             \
  };                                                                           \
  testing::Environment *const test_bench_env##name =                           \
      testing::AddGlobalTestEnvironment(                                       \
          new TestBenchEnv<TEST_BENCH_NAME(name)>());                          \
  TEST(TEST_BENCH_NAME(name), name) {                                          \
    run_test_bench<TEST_BENCH_NAME(name)>(                                     \
        static_cast<TestBenchEnv<TEST_BENCH_NAME(name)> *>(                    \
            test_bench_env##name)                                              \
            ->get());                                                          \
  }                                                                            \
  void TEST_BENCH_NAME(name)::test_bench_body()

#endif
