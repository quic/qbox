#
# Copyright (c) 2022 Qualcomm Innovation Center, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

"""
This is a proof of concept python-systemc uart backend model which uses 
systemc features exposed by the pybind11 baesd PythonBinder C++ model to
react on the coming systemc transactions and initiate the proper transactions
from/to the pl011 uart systemc C++ model.

to connect the model to QQVP please configure it like that in your conf.lua:

python_backend_stdio_0 = {
        moduletype = "PythonBinder";
        py_module_name = "py-uart";
        py_module_dir = "/path/to/py-models";
        py_module_args = "--enable_input --baudrate 115200";
        tlm_initiator_ports_num = 2;
        tlm_target_ports_num = 2;
        initiator_socket_0 = {bind = "&pl011_uart_0.backend_socket.backend_socket_target_socket"};
        initiator_socket_1 = {bind = "&pl011_uart_0.backend_socket.backend_socket_initiator_socket_control"};
        target_socket_0 = {bind = "&pl011_uart_0.backend_socket.backend_socket_initiator_socket"};
        target_socket_1 = {bind = "&pl011_uart_0.backend_socket.backend_socket_target_socket_control"};
    };

   pl011_uart_0 = {
       moduletype = "Pl011",
       -- args = {"&platform.charbackend_stdio_0"};
       target_socket = {address= UART0,
                                 size=0x1000, 
                                 bind = "&router.initiator_socket"},
       irq = {bind = "&gic_0.spi_in_379"},
   };
"""

from tlm_generic_payload import tlm_command, tlm_generic_payload
from sc_core import sc_time, sc_spawn, sc_spawn_options, sc_time_unit, wait
from gs import async_event
import tlm_do_b_transport
import cpp_shared_vars
import numpy as np
import numpy.typing as npt
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

DELTA_CHANGE = 0x0
ABSOLUTE_VALUE = 0x1
INFINITE = 0x2
EXIT_SUCCESS = 0x0
EXIT_FAILURE = 0x1


def log(msg: str) -> None:
    print(f"[python] {msg}", file=sys.stderr)


log(f"module args: {cpp_shared_vars.module_args}")


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


args = parse_args(cpp_shared_vars.module_args)


@dataclasses.dataclass
class ctrl:
    cmd: np.uint64
    can_send: np.uint64

    def serialize(self) -> npt.NDArray[np.uint8]:
        cmd_arr = np.frombuffer(self.cmd.to_bytes(8, "little"), dtype=np.uint8)
        can_send_arr = np.frombuffer(
            self.can_send.to_bytes(8, "little"), dtype=np.uint8
        )
        return np.concatenate([cmd_arr, can_send_arr])


@dataclasses.dataclass
class stdin_receiver:
    send_event: async_event
    queue: Queue
    timeout: int  # ms
    ifd: int

    def __post_init__(self):
        self._old_flags: List[Any] = termios.tcgetattr(self.ifd)
        self._new_flags: List[Any] = termios.tcgetattr(self.ifd)

    def set_termios_attr(self) -> None:
        # slef.new_flags[3] = lflags
        self._new_flags[3] = self._new_flags[3] & ~(
            termios.ECHO | termios.ECHONL | termios.ICANON | termios.IEXTEN
        )
        termios.tcsetattr(self.ifd, termios.TCSANOW, self._new_flags)

    def reset_termios_attr(self) -> None:
        termios.tcsetattr(self.ifd, termios.TCSANOW, self._old_flags)


receiver = stdin_receiver(
    send_event=async_event(True), queue=Queue(), timeout=300, ifd=sys.stdin.fileno()
)


def sigint_handler(signno: int, frame: Optional[FrameType]) -> None:
    """handler for SIGINT to make the guest OS aware of CTRL+C"""
    ch = b"\x03"
    receiver.queue.put(ch)
    receiver.send_event.notify(sc_time(0, sc_time_unit.SC_NS))


signal.signal(signal.SIGINT, sigint_handler)


def notifier() -> None:
    """systemc thread to check for input and notifies the sendall method whenever a stdin byte is available, called by systemc simulation kernel"""
    while True:
        try:
            if receiver.ifd in select.select([receiver.ifd], [], [], 0)[0]:
                ch = os.read(receiver.ifd, 1)
                receiver.queue.put(ch)
                # let's do the processing in the sendall systemc method by notifying a systemc event
                receiver.send_event.notify(sc_time(0, sc_time_unit.SC_NS))
            wait(sc_time(1 / args.baudrate, sc_time_unit.SC_US))
        except SystemExit as syse:
            break
        except Exception as e:
            log(f"notifier sc thread error: {e}")


def sendall() -> None:
    """systemc method to send all bytes read from stdin to pl011, called by systemc simulation kernel"""
    try:
        txn = tlm_generic_payload()
        data = np.frombuffer(receiver.queue.get(), dtype=np.uint8)
        txn.set_data_length(data.size)
        txn.set_data_ptr(data)
        txn.set_command(tlm_command.TLM_IGNORE_COMMAND)
        delay = sc_time(1 / args.baudrate, sc_time_unit.SC_US)
        tlm_do_b_transport.do_b_transport(0, txn, delay)
    except SystemExit as syse:
        return
    except Exception as e:
        log(f"sendall sc method error: {e}")


def spawn_sc_method(method: Callable, method_name: str, event: async_event) -> None:
    """spawn new systemc method, sc_spawn and options methods calls C++ sc_core::[sc_spawn() & sc_spawn_options]"""
    try:
        options = sc_spawn_options()
        options.spawn_method()
        options.dont_initialize()
        options.set_sensitivity(event)
        sc_spawn(method, method_name, options)
    except SystemExit as syse:
        return
    except Exception as e:
        log(f"spawn_sc_method error: {e}")


def spawn_sc_thread(thread: Callable, thread_name: str) -> None:
    """spawn new systemc thread, sc_spawn is calling C++ sc_core::sc_spawn()"""
    try:
        options = sc_spawn_options()
        sc_spawn(thread, thread_name, options)
    except SystemExit as syse:
        return
    except Exception as e:
        log(f"spawn_sc_thread error: {e}")


def set_send_limit_to_inf() -> None:
    """send the ctrl struct with cmd set to INFINITE to pl011"""
    try:
        trans = tlm_generic_payload()
        trans.set_data_length(16)
        trans.set_streaming_width(16)
        # biflow_socket ctrl struct
        ctrl_lst = ctrl(cmd=INFINITE, can_send=0x0)
        data = ctrl_lst.serialize()
        trans.set_data_ptr(data)
        trans.set_command(tlm_command.TLM_IGNORE_COMMAND)
        delay = sc_time()  # control trans, no need to set delay
        tlm_do_b_transport.do_b_transport(1, trans, delay)
    except SystemExit as syse:
        return
    except Exception as e:
        log(f"set_send_limit_to_inf error: {e}")


def before_end_of_elaboration() -> None:
    """called by the C++ systemc kernel before end of elaboration"""
    try:
        receiver.set_termios_attr()
        receiver.send_event.async_detach_suspending()
        if args.enable_input:
            spawn_sc_method(
                method=sendall, method_name="sendall", event=receiver.send_event
            )
            spawn_sc_thread(thread=notifier, thread_name="notifier")
        set_send_limit_to_inf()
    except SystemExit as syse:
        return
    except Exception as e:
        log(f"before_end_of_elaboration error: {e}")


def b_transport(id: int, trans: tlm_generic_payload, delay: sc_time) -> None:
    """called from C++ side (PythonBinder model) when a txn happens on initiator port id"""
    try:
        if id == 0:
            data = trans.get_data()
            data_len = trans.get_data_length()
            for i in range(0, data_len):
                os.write(sys.stdout.fileno(), data[i])
            sys.stdout.flush()
    except SystemExit as syse:
        return
    except Exception as e:
        log(f"b_transport error: {e}")


@atexit.register
def terminate() -> None:
    """cleanup before exiting"""
    receiver.reset_termios_attr()
    log("py-uart script terminated")
