# Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
# SPDX-License-Identifier: BSD-3-Clause

from tlm_generic_payload import tlm_command, tlm_response_status, tlm_generic_payload
import initiator_signal_socket
from sc_core import (
    sc_time,
    sc_time_unit,
    sc_spawn,
    sc_spawn_options,
    wait,
    sc_event,
    sc_time_stamp,
)
import tlm_do_b_transport
import cpp_shared_vars
import functools
import numpy as np
from typing import List, Callable
import argparse
import shlex
import inspect

signal_write_values: List[bool] = []
signal_writer_event = sc_event()
dummy_event = sc_event()


def parse_args(args_str: str) -> argparse.Namespace:
    """parse module arguments"""
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-d",
        "--debug",
        help="enable debug messages",
        action="store_true",
    )
    return parser.parse_args(shlex.split(args_str))


args = parse_args(cpp_shared_vars.module_args)


def log(msg: str) -> None:
    if args.debug:
        print(f"[python] {msg}")


def sc_thread(func):
    """decorator to convert a function to systemc thread"""

    def wrapper(*args, **kwargs):
        if not inspect.isgeneratorfunction(func):
            return func()
        if not hasattr(wrapper, "g"):
            wrapper.g = func()
        return next(wrapper.g)

    return functools.update_wrapper(wrapper, func)


def to_uint8_nparray(func):
    """decorator to convert list of numbers to uint8 np array"""

    def wrapper(*args, **kwargs):
        lst: List[int]
        lst = func(*args, **kwargs)
        return np.array(lst, dtype=np.uint8)

    return functools.update_wrapper(wrapper, func)


@to_uint8_nparray
def generate_test_data1() -> List[int]:
    return [0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7]


@to_uint8_nparray
def generate_test_data2() -> List[int]:
    return [0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80]


@to_uint8_nparray
def generate_test_data3() -> List[int]:
    return [
        0x00,
        0x11,
        0x22,
        0x33,
        0x44,
        0x55,
        0x66,
        0x77,
        0x88,
        0x99,
        0xAA,
        0xBB,
        0xCC,
        0xDD,
        0xEE,
        0xFF,
    ]


def start_of_simulation() -> None:
    trans = tlm_generic_payload()
    trans.set_address(0x1008)
    trans.set_data_length(8)
    trans.set_streaming_width(8)
    data = generate_test_data3()[:8]
    trans.set_data_ptr(data)
    trans.set_command(tlm_command.TLM_WRITE_COMMAND)
    delay = sc_time(0, sc_time_unit.SC_NS)
    tlm_do_b_transport.do_b_transport(0, trans, delay)
    signal_writer_event.notify(sc_time(0, sc_time_unit.SC_NS))


@sc_thread
def signal_writer():
    dummy_event.notify(sc_time(10, sc_time_unit.SC_NS))
    for _ in range(0, 2):
        initiator_signal_socket.write(0, False)
        yield sc_time(10, sc_time_unit.SC_NS)
        initiator_signal_socket.write(0, True)
        yield sc_time(10, sc_time_unit.SC_NS)


@sc_thread
def dummy_thread1():
    yield (sc_time(10, sc_time_unit.SC_NS), dummy_event)
    log(f"after execute systemc wait event, simulation time: {sc_time_stamp()}")
    yield sc_time(100, sc_time_unit.SC_MS)
    log(f"after execute systemc wait time 100 ms, simulation time: {sc_time_stamp()}")
    yield sc_time(100, sc_time_unit.SC_MS)
    log(f"after execute systemc wait time 100 ms, simulation time: {sc_time_stamp()}")
    yield sc_time(100, sc_time_unit.SC_MS)
    log(f"after execute systemc wait time 100 ms, simulation time: {sc_time_stamp()}")


@sc_thread
def dummy_thread2():
    log(f"log a msg and exit thread, simulation time: {sc_time_stamp()}")


@sc_thread
def dummy_thread3():
    yield sc_time(200, sc_time_unit.SC_MS)
    log(f"after execute systemc wait time 200 ms, simulation time: {sc_time_stamp()}")
    yield sc_time(200, sc_time_unit.SC_MS)
    log(f"after execute systemc wait time 200 ms, simulation time: {sc_time_stamp()}")
    yield sc_time(200, sc_time_unit.SC_MS)
    log(f"after execute systemc wait time 200 ms, simulation time: {sc_time_stamp()}")
    log(f"Simulation time: {sc_time_stamp()}")
    assert sc_time_stamp() == sc_time(600, sc_time_unit.SC_MS)


def spawn_sc_thread(
    thread: Callable, thread_name: str, event: sc_event = None, donot_init: bool = False
) -> None:
    try:
        options = sc_spawn_options()
        if event:
            options.set_sensitivity(event)
        if donot_init:
            options.dont_initialize()
        sc_spawn(thread, thread_name, options)
    except Exception as e:
        log(f"spawn_sc_thread error: {e}")


def end_of_elaboration() -> None:
    spawn_sc_thread(
        thread=signal_writer,
        thread_name="signal_writer",
        event=signal_writer_event,
        donot_init=True,
    )
    spawn_sc_thread(thread=dummy_thread1, thread_name="dummy_thread1")
    spawn_sc_thread(thread=dummy_thread2, thread_name="dummy_thread2")
    spawn_sc_thread(thread=dummy_thread3, thread_name="dummy_thread3")


def target_signal_cb(id: int, value: bool) -> None:
    log(f"target signal socket {id} received value: {value}")
    signal_write_values.append(value)


def end_of_simulation() -> None:
    test_signal_write_values = [True, False, True, False]
    assert test_signal_write_values == signal_write_values


def test1(id: int, trans: tlm_generic_payload, delay: sc_time) -> None:
    log("test1 -> testing important transation attributes getting")
    # systemc wait
    wait(sc_time(1000, sc_time_unit.SC_NS))
    assert trans.get_data_length() == 8
    assert trans.get_command() == tlm_command.TLM_WRITE_COMMAND
    assert (trans.get_data() == generate_test_data1()).all()
    assert trans.get_response_status() == tlm_response_status.TLM_INCOMPLETE_RESPONSE
    assert delay == sc_time(100, sc_time_unit.SC_NS)
    log(f"delay: {delay}")
    delay += sc_time(100, sc_time_unit.SC_NS)
    delay *= float(2)
    log("adding 100 SC_NS to delay and multiply by 2")
    log(f"delay: {delay}")
    assert delay == sc_time(400, sc_time_unit.SC_NS)
    log("sc_time operator overloading is working ok")
    trans.set_response_status(tlm_response_status.TLM_OK_RESPONSE)


def test2(id: int, trans: tlm_generic_payload, delay: sc_time) -> None:
    log(
        "test2 -> testing important transation attributes setting and transaction initiation"
    )
    # systemc wait
    wait(sc_time(1000, sc_time_unit.SC_NS))
    # write data to memory model
    trans.set_address(0x1000)
    trans.set_data_length(8)
    trans.set_streaming_width(8)
    data = generate_test_data2()
    trans.set_data(data)
    trans.set_command(tlm_command.TLM_WRITE_COMMAND)
    tlm_do_b_transport.do_b_transport(0, trans, delay)


def test3(id: int, trans: tlm_generic_payload, delay: sc_time) -> None:
    log("test3 -> testing a read transaction")
    # systemc wait
    wait(sc_time(1000, sc_time_unit.SC_NS))
    data = generate_test_data3()
    assert trans.get_data_length() == 16
    assert trans.get_command() == tlm_command.TLM_READ_COMMAND
    trans.set_data(data)
    trans.set_response_status(tlm_response_status.TLM_OK_RESPONSE)


# Entry point of the script, this function will be called from the PythonBinder module.
def b_transport(id: int, trans: tlm_generic_payload, delay: sc_time) -> None:
    log(f"transaction: {trans}")
    if trans.get_address() == 0x2000:
        test1(id, trans, delay)
    elif trans.get_address() == 0x2100:
        test2(id, trans, delay)
    elif trans.get_address() == 0x2200:
        test3(id, trans, delay)
    else:
        raise RuntimeError(
            f"address: 0x{trans.get_address():x} is not supported in test"
        )
