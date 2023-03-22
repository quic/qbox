/*
 *  Copyright(c) 2020 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

/*
 *
 *	Qtimer Example
 *
 * This example initializes two timers that cause interrupts at different
 * intervals.  The thread 0 will sit in wait mode till the interrupt is
 * serviced then return to wait mode.
 *
 */
#include "qtimer.h"
int main()
{
    int i;
    exit_flag = 0;
    printf("\nCSR base=0x%x; L2VIC base=0x%x\n", CSR_BASE, L2VIC_BASE);
    printf("QTimer1 will go off 20 times (once every 1/%d sec).\n",
           (QTMR_FREQ) / (ticks_per_qtimer1));
    printf("QTimer2 will go off 2 times (once every 1/%d sec).\n\n",
           (QTMR_FREQ) / (ticks_per_qtimer2));

    add_translation((void *)CSR_BASE, (void *)CSR_BASE, 4);

    enable_core_interrupt();

    init_l2vic();
    // initialize qtimers 1 and 2
    init_qtimers(3);

    while (qtimer2_cnt < 2) {
        /* Thread 0 waits for interrupts */
        /* Wait disabled, spin instead: qemu timer bug when
         *  all threads are waiting.
         */
        asm_wait();
	printf ("qtimer_cnt1 = %d, qtimer_cnt2 = %d\n", qtimer1_cnt, qtimer2_cnt);
    }
    printf("PASS\n");
    return 0;
}
