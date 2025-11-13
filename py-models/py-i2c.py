# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
# SPDX-License-Identifier: BSD-3-Clause

"""
This is a proof of concept python-systemc i2c slave model which uses 
systemc features exposed by the pybind11 baesd PythonBinder C++ model to
react on the coming systemc transactions and initiate the proper transactions
from/to the i2c systemc C++ model.

this backend is being used by "qupv3_qupv3_se_wrapper_se0_i2c-test.cc".
CCI parameter current_mod_id_prefix should be set like that: current_mod_id_prefix = "i2c_"; 
"""
from tlm_generic_payload import tlm_command, tlm_generic_payload
import i2c_biflow_socket
from sc_core import sc_time, sc_time_unit
import i2c_cpp_shared_vars
import argparse
import shlex
from queue import Queue
import array

q = Queue()


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


args = parse_args(i2c_cpp_shared_vars.module_args)


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
        requested_len = trans.get_data_length()
        txn = tlm_generic_payload()
        # set to slave address to notify the master where the txn is comming from
        txn.set_address(args.address)
        txn.set_data_length(requested_len)
        txn.set_command(tlm_command.TLM_WRITE_COMMAND)
        i2c_biflow_socket.set_default_txn(txn)
        i = 0
        while i < trans.get_data_length():
            i2c_biflow_socket.enqueue(q.get())
            i = i + 1
    except SystemExit:
        return
    except Exception as e:
        raise Exception(f"sendall sc method error: {e}")


def i2c_write(id: int, trans: tlm_generic_payload, delay: sc_time) -> None:
    log("i2c_write -> Master sending data to slave")
    # read data from master and write it to slave queue
    data = array.array('B', memoryview(trans.get_data()))
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
        i2c_biflow_socket.can_receive_any()
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
def bf_b_transport(trans: tlm_generic_payload, delay: sc_time) -> None:
    log(f"transaction: {trans}")
    log(f"transaction id: {id}")
    """Make sure the trans and slave addresses are identical"""
    try:
        assert trans.get_address() == args.address
        if trans.get_command() == tlm_command.TLM_READ_COMMAND:
            i2c_read(id, trans, delay)
        elif trans.get_command() == tlm_command.TLM_WRITE_COMMAND:
            i2c_write(id, trans, delay)
        else:
            raise RuntimeError(
                f"command: {trans.get_command()} is not supported: command has to be read/write"
            )
    except SystemExit:
        return
    except Exception as e:
        raise Exception(f"b_transport error: {e}")
