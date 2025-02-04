/*
 * This file is part of libqbox
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2021
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#undef SYSTEMMODE
#include "reset-test-base.h"

int sc_main(int argc, char* argv[]) { return run_testbench<CpuArmCortexA53SimpleResetBase>(argc, argv); }
