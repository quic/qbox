/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef NVIC_H
#define NVIC_H

void nvic_enable_irq(unsigned char irq);
void nvic_disable_irq(unsigned char irq);
void _enable_irq();
void _disable_irq();

#endif /* NVIC_H */
