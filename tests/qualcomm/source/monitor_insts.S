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

    .text
    .type test_set_prio, @function
    .global test_set_prio

test_set_prio:
    r0 = #3
    r1 = #1
    p0 = cmp.eq(r0,r1)
    setprio(p0, r0)
    jumpr lr

    .size test_set_prio, . - test_set_prio
