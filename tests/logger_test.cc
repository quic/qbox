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

#include <systemc>
#include <greensocs/libgsutils.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <scp/report.h>

using testing::AnyOf;
using testing::Eq;

SC_MODULE(testA)
{
    void testA_method() {
        std::cout << "(cout) test A" << std::endl;
        GS_LOG("(GS_LOG) test A");
        SC_REPORT_INFO("testA","(SC_REPORT_INFO) test A");
        SCP_INFO() << "(SCP_INFO) test A";
    }
    SC_CTOR(testA)
    {
        GS_LOG("(GS_LOG) Construct TestA");
        SC_METHOD(testA_method);
    }
};

int sc_main(int argc, char **argv){
  scp::init_logging(
      scp::LogConfig()
          .logLevel(scp::log::DBGTRACE) // set log level to DBGTRACE = TRACEALL
          .msgTypeFieldWidth(10)); // make the msg type column a bit tighter

  std::stringstream buffer;
  std::streambuf *old = std::cout.rdbuf(buffer.rdbuf());

  std::cout << "(cout) Logger test" << std::endl;
  GS_LOG("(GS_LOG) Logging test");
  SC_REPORT_INFO("sc_main", "(SC_REPORT_INFO) Logging test");

  GS_LOG("(GS_LOG) instance testA");
  testA atest("TestA_Instance");
  GS_LOG("(GS_LOG) testA instanced");

  GS_LOG("(GS_LOG) running SC_START");
  sc_core::sc_start();
  GS_LOG("(GS_LOG) SC_START finished");

  std::cout.rdbuf(old);

  std::string text = buffer.str(); // text will now contain "Bla\n"
  std::cout << "Text found: " << std::endl << text << std::endl;

  return EXIT_SUCCESS;
}