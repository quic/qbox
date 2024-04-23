#
# Copyright (c) 2022 Qualcomm Innovation Center, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

"""
This is a proof of concept python-systemc i2c slave model which uses 
systemc features exposed by the pybind11 baesd PythonBinder C++ model to
react on the coming systemc transactions and initiate the proper transactions
from/to the i2c systemc C++ model.

this backend is being used by "qupv3_qupv3_se_wrapper_se0_i2c-test.cc".
"""
from tlm_generic_payload import tlm_command, tlm_generic_payload
from sc_core import sc_time, sc_spawn, sc_spawn_options, sc_time_unit, wait, sc_event
from gs import async_event
import tlm_do_b_transport
import cpp_shared_vars
import numpy as np
import numpy.typing as npt
import argparse
import shlex
from typing import Optional, Callable, List, Any
from types import FrameType
from queue import Queue
import sys
import dataclasses


DELTA_CHANGE = 0x0
ABSOLUTE_VALUE = 0x1
INFINITE = 0x2

q = Queue()


@dataclasses.dataclass
class ctrl:
    cmd: np.uint32
    can_send: np.uint32

    def serialize(self) -> npt.NDArray[np.uint8]:
        cmd_arr = np.frombuffer(self.cmd.to_bytes(4, "little"), dtype=np.uint8)
        can_send_arr = np.frombuffer(
            self.can_send.to_bytes(4, "little"), dtype=np.uint8
        )
        return np.concatenate([cmd_arr, can_send_arr])

    def deserialize(self, byte_data: np.ndarray[np.uint8]) -> None:
        cmd_arr = byte_data[:4]
        can_send_arr = byte_data[4:]
        self.cmd = int.from_bytes(cmd_arr.tobytes(), byteorder="little")
        i = int.from_bytes(can_send_arr.tobytes(), byteorder="little")
        if self.cmd == INFINITE:
            self.can_send = sys.maxint
        if self.cmd == ABSOLUTE_VALUE:
            self.can_send = i
        if self.cmd == DELTA_CHANGE:
            self.can_send = self.can_send + i

    def sent(self, i):
        if self.cmd != INFINITE:
            self.can_send = self.can_send - i
            assert self.can_send >= 0


m_ctrl = ctrl(can_send=0, cmd=0)


def log(msg: str) -> None:
    if args.debug:
        print(f"[python] {msg}")


def parse_args(args_str: str) -> argparse.Namespace:
    """parse module arguments"""
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-a",
        "--address",
        type=int,
        default=1,
        help="i2c slave address",
    )
    parser.add_argument(
        "-d",
        "--debug",
        help="enable debug messages",
        action="store_true",
    )
    return parser.parse_args(shlex.split(args_str))


args = parse_args(cpp_shared_vars.module_args)


def i2c_read(id: int, trans: tlm_generic_payload, delay: sc_time) -> None:
    log("i2c_read -> Master requesting data from slave")
    # write data to master
    try:
        log(f"trans.get_data_length(): {trans.get_data_length()}")
        log(f"q.qsize(): {q.qsize()}")
        if trans.get_data_length() > q.qsize():
            raise RuntimeError(
                f"command: {trans.get_data_length()} requested data size is bigger than the slave-queue size, please adjust the requested data size "
            )
        txn = tlm_generic_payload()
        txn.set_address(
            args.address
        )  # set to slave address to notify the master where the txn is comming from
        i = 0
        data = []
        while i < trans.get_data_length():
            data.append((q.get()))
            i = i + 1
        log(f"i2c_slvae, requested data: {data}")
        log(f"i2c_slvae, data_len: {len(data)}")
        txn.set_data_ptr(data)
        txn.set_data_length(len(data))
        txn.set_streaming_width(len(data))
        txn.set_command(tlm_command.TLM_WRITE_COMMAND)
        delay = sc_time(0, sc_time_unit.SC_NS)
        tlm_do_b_transport.do_b_transport(0, txn, delay)
        m_ctrl.sent(len(data))
    except SystemExit:
        return
    except Exception as e:
        raise Exception(f"sendall sc method error: {e}")


def i2c_write(id: int, trans: tlm_generic_payload, delay: sc_time) -> None:
    log("i2c_write -> Master sending data to slave")
    # systemc wait
    # read data from master and write it to slave queue
    data = trans.get_data()
    data_len = trans.get_streaming_width()
    log(f"i2c_slvae, recived data: {data}")
    log(f"i2c_slvae, data_len: {data_len}")
    i = 0
    while i < data_len:
        q.put(data[i])
        i = i + 1


def set_send_limit_to_inf() -> None:
    """send the ctrl struct with cmd set to INFINITE to i2c qup"""
    try:
        trans = tlm_generic_payload()
        trans.set_data_length(8)
        trans.set_streaming_width(8)
        # biflow_socket ctrl struct
        ctrl_lst = ctrl(cmd=INFINITE, can_send=0x0)
        data = ctrl_lst.serialize()
        trans.set_data_ptr(data)
        trans.set_command(tlm_command.TLM_IGNORE_COMMAND)
        delay = sc_time()  # control trans, no need to set delay
        tlm_do_b_transport.do_b_transport(1, trans, delay)
    except SystemExit:
        return
    except Exception as e:
        raise Exception(f"set_send_limit_to_inf error: {e}")


def before_end_of_elaboration() -> None:
    """called by the C++ systemc kernel before end of elaboration"""
    try:
        set_send_limit_to_inf()
    except SystemExit:
        return
    except Exception as e:
        raise Exception(f"before_end_of_elaboration error: {e}")


# Entry point of the script, this function will be called from the PythonBinder module.
def b_transport(id: int, trans: tlm_generic_payload, delay: sc_time) -> None:
    log(f"transaction: {trans}")
    log(f"transaction id: {id}")
    """Make sure the trans and slave addresses are identical"""
    try:
        if id == 0:
            assert trans.get_address() == args.address
            if trans.get_command() == tlm_command.TLM_READ_COMMAND:
                i2c_read(id, trans, delay)
            elif trans.get_command() == tlm_command.TLM_WRITE_COMMAND:
                i2c_write(id, trans, delay)
            else:
                raise RuntimeError(
                    f"command: {trans.get_command()} is not supported: command has to be read/write"
                )
        if id == 1:
            m_ctrl.deserialize(trans.get_data())
    except SystemExit:
        return
    except Exception as e:
        raise Exception(f"b_transport error: {e}")
