# Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
# SPDX-License-Identifier: BSD-3-Clause

"""
This is a proof of concept python-systemc uart backend model which uses 
systemc features exposed by the pybind11 baesd PythonBinder C++ model to
react on the coming systemc transactions and initiate the proper transactions
from/to the pl011 uart systemc C++ model.

to connect the model to QQVP please configure it like that in your conf.lua:

python_backend_stdio_0 = {
        moduletype = "python_binder";
        py_module_name = "py-uart";
        py_module_dir = "/path/to/py-models";
        py_module_args = "--enable_input --baudrate 115200";
        current_mod_id_prefix = "uart_";
        biflow_socket_num = 1;
    };

    pl011_uart_0 = {
        moduletype = "Pl011",
        dylib_path = "uart-pl011",
        target_socket = {address= UART0,
                                    size=0x1000,
                                    bind = "&router.initiator_socket"},
        irq = {bind = "&gic_0.spi_in_379"},
        backend_socket = {bind = "&python_backend_stdio_0.biflow_socket_0"},
    };
"""

from tlm_generic_payload import tlm_generic_payload
import uart_biflow_socket
from sc_core import sc_time, sc_spawn, sc_spawn_options, sc_time_unit, sc_event
import uart_cpp_shared_vars
import numpy as np
import argparse
import shlex
from typing import Optional, Callable, List, Any
from types import FrameType
import sys
from queue import Queue
import dataclasses
import os
import termios
import select
import atexit
import signal
import functools
import inspect
import copy


def sigint_handler(signno: int, frame: Optional[FrameType]) -> None:
    """handler for SIGINT to make the guest OS aware of CTRL+C"""
    ch = b"\x03"
    receiver.queue.put(ch)
    receiver.send_event.notify(sc_time(0, sc_time_unit.SC_NS))


def sc_thread(func):
    """decorator to convert a function to systemc thread"""

    def wrapper(*args, **kwargs):
        if not inspect.isgeneratorfunction(func):
            return func()
        if not hasattr(wrapper, "g"):
            wrapper.g = func()
        return next(wrapper.g)

    return functools.update_wrapper(wrapper, func)


def module_init() -> None:
    signal.signal(signal.SIGINT, sigint_handler)
    log(f"loaded module: {__name__}")
    log(f"module args: {uart_cpp_shared_vars.module_args}")


def log(msg: str) -> None:
    print(f"[python] {msg}", file=sys.stderr)


def parse_args(args_str: str) -> argparse.Namespace:
    """parse module arguments"""
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-b",
        "--baudrate",
        type=int,
        default=19200,
        help="uart baudrate",
    )
    parser.add_argument(
        "-i",
        "--enable_input",
        help="enable console input",
        action="store_true",
    )

    return parser.parse_args(shlex.split(args_str))


args = parse_args(uart_cpp_shared_vars.module_args)


@dataclasses.dataclass
class stdin_receiver:
    send_event: sc_event
    queue: Queue
    ifd: int

    def __post_init__(self):
        self._old_flags: List[Any] = copy.deepcopy(termios.tcgetattr(self.ifd))
        self._new_flags: List[Any] = copy.deepcopy(termios.tcgetattr(self.ifd))

    def set_termios_attr(self) -> None:
        # slef.new_flags[3] = lflags
        self._old_flags[3] = self._new_flags[3] | (
            termios.ECHO | termios.ECHONL | termios.ICANON | termios.IEXTEN
        )
        self._new_flags[3] = self._new_flags[3] & ~(
            termios.ECHO | termios.ECHONL | termios.ICANON | termios.IEXTEN
        )
        termios.tcsetattr(self.ifd, termios.TCSANOW, self._new_flags)

    def reset_termios_attr(self) -> None:
        termios.tcsetattr(self.ifd, termios.TCSANOW, self._old_flags)


receiver = stdin_receiver(send_event=sc_event(), queue=Queue(), ifd=sys.stdin.fileno())


@sc_thread
def notifier():
    """systemc thread to check for input and notifies the sendall method whenever a stdin byte is available, called by systemc simulation kernel"""
    while True:
        if receiver.ifd in select.select([receiver.ifd], [], [], 0)[0]:
            ch = os.read(receiver.ifd, 1)
            receiver.queue.put(ch)
            # let's do the processing in the sendall systemc method by notifying a systemc event
            receiver.send_event.notify(sc_time(0, sc_time_unit.SC_NS))
        yield sc_time(10, sc_time_unit.SC_NS)


def sendall() -> None:
    """systemc method to send all bytes read from stdin to pl011, called by systemc simulation kernel"""
    if not receiver.queue.empty():
        data = np.frombuffer(receiver.queue.get(), dtype=np.uint8)
        uart_biflow_socket.enqueue(data)
    if not receiver.queue.empty():
        receiver.send_event.notify(sc_time(1 / args.baudrate, sc_time_unit.SC_SEC))


def spawn_sc_method(method: Callable, method_name: str, event: sc_event) -> None:
    """spawn new systemc method, sc_spawn and options methods calls C++ sc_core::[sc_spawn() & sc_spawn_options]"""
    try:
        options = sc_spawn_options()
        options.spawn_method()
        options.dont_initialize()
        options.set_sensitivity(event)
        sc_spawn(method, method_name, options)
    except SystemExit:
        return
    except Exception as e:
        raise Exception(f"spawn_sc_method error: {e}")


def spawn_sc_thread(thread: Callable, thread_name: str) -> None:
    """spawn new systemc thread, sc_spawn is calling C++ sc_core::sc_spawn()"""
    try:
        options = sc_spawn_options()
        sc_spawn(thread, thread_name, options)
    except SystemExit:
        return
    except Exception as e:
        raise Exception(f"spawn_sc_thread error: {e}")


def set_send_limit_to_inf() -> None:
    """send the ctrl struct with cmd set to INFINITE to pl011"""
    try:
        uart_biflow_socket.can_receive_any()
    except SystemExit:
        return
    except Exception as e:
        raise Exception(f"set_send_limit_to_inf error: {e}")


def before_end_of_elaboration() -> None:
    """called by the C++ systemc kernel before end of elaboration"""
    module_init()
    try:
        receiver.set_termios_attr()
        if args.enable_input:
            spawn_sc_method(
                method=sendall, method_name="sendall", event=receiver.send_event
            )
            spawn_sc_thread(thread=notifier, thread_name="notifier")
        set_send_limit_to_inf()
    except SystemExit:
        return
    except Exception as e:
        raise Exception(f"before_end_of_elaboration error: {e}")


def bf_b_transport(trans: tlm_generic_payload, delay: sc_time) -> None:
    """called from C++ side (PythonBinder model)"""
    try:
        data = trans.get_data()
        data_len = trans.get_streaming_width()
        for i in range(0, data_len):
            os.write(sys.stdout.fileno(), data[i])
            sys.stdout.flush()
    except SystemExit:
        return
    except Exception as e:
        raise Exception(f"b_transport error: {e}")


def end_of_simulation() -> None:
    receiver.reset_termios_attr()


@atexit.register
def terminate() -> None:
    """cleanup before exiting"""
    log("py-uart script terminated")
