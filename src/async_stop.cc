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

#include <signal.h>

#include "utils/async_stop.h"

async_stop::async_stop_channel *async_stop::singleton = NULL;

void async_stop::signal_handler(int signo)
{
    std::cerr << "Async stop due to signal " << signo << " caught." << std::endl;
    trigger();
}

void async_stop::trigger(void)
{
    if (!singleton) {
        std::cerr << "Error: async_stop should be registered before use." << std::endl;
        return;
    }
    singleton->trigger();
}

void async_stop::create(void)
{
    if (!singleton) {
        singleton = new async_stop_channel;
    }
}

void async_stop::register_on_signal(int signo)
{
    create();
    struct sigaction sigact;
    sigact.sa_handler = signal_handler;
    sigaction(signo, &sigact, NULL);
}
