/*
 *  This file is part of libqbox
 *  Copyright (C) 2020 Greensocs
 *
 *  Author: Damien Hedde
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef ASYNC_STOP_
#define ASYNC_STOP_

#include <systemc>

class async_stop
{
    class async_stop_channel : sc_core::sc_prim_channel
    {
        void update(void)
        {
            sc_core::sc_stop();
        }
    public:
        async_stop_channel() : sc_core::sc_prim_channel("async_stopper") {}
        void trigger(void)
        {
            async_request_update();
        }
    };
    static async_stop_channel *singleton;

    static void signal_handler(int signo);

public:
    /*
     * Create the async_stop channel if it does not exist. Must be done during sc elaboration.
     */
    static void create(void);

    /*
     * Trigger an asycnhronous sc_stop(). Thread safe method.
     */
    static void trigger(void);

    /*
     * Create the async_stop channel and register its trigger on a given signal.
     * It overrides previous registered sigaction.
     */
    static void register_on_signal(int signo);
};

#endif
