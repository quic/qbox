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
from sc_core import sc_time, sc_spawn, sc_spawn_options, sc_time_unit
from gs import async_event
import uart_cpp_shared_vars
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
import copy
import threading


def handle_SIGINT():
    """handler for SIGINT to make the guest OS aware of CTRL+C"""
    ch = b"\x03"
    uart_biflow_socket.enqueue(int.from_bytes(ch))


def module_init() -> None:
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
sim_end_flag = threading.Event()


@dataclasses.dataclass
class stdin_receiver:
    send_event: async_event
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


receiver = stdin_receiver(
    send_event=async_event(True), queue=Queue(), ifd=sys.stdin.fileno()
)


def notifier():
    """systemc thread to check for input and notifies the sendall method whenever a stdin byte is available, called by systemc simulation kernel"""
    while not sim_end_flag.is_set():
        read_fds = select.select([receiver.ifd], [], [], 0.1)[0]
        if receiver.ifd in read_fds:
            ch = os.read(receiver.ifd, 1)
            uart_biflow_socket.enqueue(int.from_bytes(ch))


notifier_th = threading.Thread(target=notifier)


def set_send_limit_to_inf() -> None:
    """send the ctrl struct with cmd set to INFINITE to pl011"""
    try:
        uart_biflow_socket.can_receive_any()
    except SystemExit:
        return
    except Exception as e:
        raise Exception(f"set_send_limit_to_inf error: {e}")


def end_of_elaboration() -> None:
    """called by the C++ systemc kernel before end of elaboration"""
    module_init()
    try:
        receiver.set_termios_attr()
        if args.enable_input:
            notifier_th.start()
        set_send_limit_to_inf()
    except SystemExit:
        return
    except Exception as e:
        raise Exception(f"before_end_of_elaboration error: {e}")


def bf_b_transport(trans: tlm_generic_payload, delay: sc_time) -> None:
    """called from C++ side (PythonBinder model)"""
    try:
        data = memoryview(trans.get_data())
        data_len = trans.get_streaming_width()
        for i in range(0, data_len):
            os.write(sys.stdout.fileno(), bytes([data[i]]))
            sys.stdout.flush()
    except SystemExit:
        return
    except Exception as e:
        raise Exception(f"b_transport error: {e}")


def end_of_simulation() -> None:
    if sim_end_flag.is_set():
        return
    sim_end_flag.set()
    receiver.reset_termios_attr()
    if notifier_th.ident is not None and notifier_th.is_alive():
        notifier_th.join()


@atexit.register
def terminate() -> None:
    """cleanup before exiting"""
    log("py-uart script terminated")
    end_of_simulation()
    # make sure the termios is reset again
    receiver.reset_termios_attr()
