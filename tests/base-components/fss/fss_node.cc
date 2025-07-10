/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fss_node.h"

std::unordered_map<fssBindIFHandle, std::shared_ptr<fss_node>> NodeInstances;

void die_null_ptr(const std::string& fn_name, const std::string& ptr_name)
{
    std::cerr << "Error: fss_node.cc -> Function: " << fn_name << " => pointer: " << ptr_name << "is null!"
              << std::endl;
    exit(EXIT_FAILURE);
}

void die_msg(const std::string& fn_name, const std::string& msg)
{
    std::cerr << "Error: fss_node.cc -> Function: " << fn_name << " => " << msg << std::endl;
    exit(EXIT_FAILURE);
}

fssRetStatus fss_node::bind_events_channel(fssBindIFHandle _self, fssString _events_channel_name,
                                           fssEventsIfHandle _events_channel_handle)
{
    if (!_self) {
        die_null_ptr("bind_events_channel", "_self");
    }

    if (!_events_channel_name) {
        die_null_ptr("bind_events_channel", "_events_channel_name");
    }

    if (!_events_channel_handle) {
        die_null_ptr("bind_events_channel", "_events_channel_handle");
    }

    if (_events_channel_handle->version != Version) {
        die_msg("bind_events_channel", "_events_channel_handle->version != Version");
    }
    auto node = NodeInstances.at(_self);
    if (!node) {
        die_null_ptr("bind_events_channel", "node");
    }
    if (std::string(_events_channel_name).find("aux") != std::string::npos) {
        node->aux_events_channel_name = _events_channel_name;
        node->aux_events_channel_handle = _events_channel_handle;
    } else {
        node->other_events_channel_name = _events_channel_name;
        node->other_events_channel_handle = _events_channel_handle;
    }
    return fssStatusOk;
}

fssRetStatus fss_node::bind_time_sync_channel(fssBindIFHandle _self, fssString _time_sync_channel_name,
                                              fssTimeSyncIfHandle _time_sync_channel_handle)
{
    if (!_self) {
        die_null_ptr("bind_time_sync_channel", "_self");
    }
    if (!_time_sync_channel_name) {
        die_null_ptr("bind_time_sync_channel", "_time_sync_channel_name");
    }
    if (!_time_sync_channel_handle) {
        die_null_ptr("bind_time_sync_channel", "_time_sync_channel_handle");
    }
    if (_time_sync_channel_handle->version != Version) {
        die_msg("bind_time_sync_channel", "_time_sync_channel_handle->version != Version");
    }
    auto node = NodeInstances.at(_self);
    if (!node) {
        die_null_ptr("bind_time_sync_channel", "node");
    }
    node->other_time_sync_channel_name = _time_sync_channel_name;
    node->other_time_sync_channel_handle = _time_sync_channel_handle;
    return fssStatusOk;
}

fssRetStatus fss_node::bind_data_channel(fssBindIFHandle _self, fssString _data_channel_name,
                                         fssBusIfHandle _data_channel_handle)
{
    if (!_self) {
        die_null_ptr("bind_data_channel", "_self");
    }
    if (!_data_channel_name) {
        die_null_ptr("bind_data_channel", "_data_channel_name");
    }
    if (!_data_channel_handle) {
        die_null_ptr("bind_data_channel", "_data_channel_handle");
    }
    if (_data_channel_handle->version != Version) {
        die_msg("bind_data_channel", "_data_channel_handle->version != Version");
    }
    auto node = NodeInstances.at(_self);
    if (!node) {
        die_null_ptr("bind_data_channel", "node");
    }
    node->other_data_bus_name = _data_channel_name;
    node->other_data_bus_handle = _data_channel_handle;
    return fssStatusOk;
}

fssRetStatus fss_node::bind_control_channel(fssBindIFHandle _self, fssString _control_channel_name,
                                            fssControlIfHanlde _control_channel_handle)
{
    if (!_self) {
        die_null_ptr("bind_control_channel", "_self");
    }
    if (!_control_channel_name) {
        die_null_ptr("bind_control_channel", "_control_channel_name");
    }
    if (!_control_channel_handle) {
        die_null_ptr("bind_control_channel", "_control_channel_handle");
    }
    if (_control_channel_handle->version != Version) {
        die_msg("bind_control_channel", "_control_channel_handle->version != Version");
    }
    auto node = NodeInstances.at(_self);
    if (!node) {
        die_null_ptr("bind_control_channel", "node");
    }
    node->other_control_channel_name = _control_channel_name;
    node->other_control_channel_handle = _control_channel_handle;
    return fssStatusOk;
}

fssEventsIfHandle fss_node::get_events_handle(fssBindIFHandle _self, fssString _events_channel_name)
{
    if (!_self) {
        die_null_ptr("get_events_handle", "_self");
    }
    if (!_events_channel_name) {
        die_null_ptr("get_events_handle", "_events_channel_name");
    }
    auto node = NodeInstances.at(_self);
    if (!node) {
        die_null_ptr("get_events_handle", "node");
    }
    if (std::string(_events_channel_name).find("aux") != std::string::npos) {
        return &node->AuxEventsHandle;
    }
    return &node->EventsHandle;
}

fssTimeSyncIfHandle fss_node::get_time_sync_handle(fssBindIFHandle _self, fssString _time_sync_channel_name)
{
    if (!_self) {
        die_null_ptr("get_time_sync_handle", "_self");
    }
    if (!_time_sync_channel_name) {
        die_null_ptr("get_time_sync_handle", "_time_sync_channel_name");
    }
    auto node = NodeInstances.at(_self);
    if (!node) {
        die_null_ptr("get_time_sync_handle", "node");
    }
    return &node->TimeSyncHandle;
}

fssBusIfHandle fss_node::get_data_bus_handle(fssBindIFHandle _self, fssString _data_channel_name)
{
    if (!_self) {
        die_null_ptr("get_data_bus_handle", "_self");
    }
    if (!_data_channel_name) {
        die_null_ptr("get_data_bus_handle", "_data_channel_name");
    }
    auto node = NodeInstances.at(_self);
    if (!node) {
        die_null_ptr("get_data_bus_handle", "node");
    }
    return &node->DataBusHandle;
}

fssControlIfHanlde fss_node::get_control_handle(fssBindIFHandle _self, fssString _control_channel_name)
{
    if (!_self) {
        die_null_ptr("get_control_handle", "_self");
    }
    if (!_control_channel_name) {
        die_null_ptr("get_control_handle", "_control_channel_name");
    }
    auto node = NodeInstances.at(_self);
    if (!node) {
        die_null_ptr("get_control_handle", "node");
    }
    return &node->ControlHandle;
}

void fss_node::handle_events_internal(fssUint64 event)
{
    switch (event) {
    case 2:
        std::cout << "Node: " << NodeName << " received SC_BEFORE_END_OF_ELABORATION event" << std::endl;
        init_registers();
        break;
    case 4:
        std::cout << "Node: " << NodeName << " received SC_END_OF_ELABORATION event" << std::endl;
        break;
    case 8: {
        std::cout << "Node: " << NodeName << " received SC_START_OF_SIMULATION event" << std::endl;
        // set enable register to default value: 1
        uint32_t data = 0x1;
        write_data(&DataBusHandle, 0, 4, reinterpret_cast<uint8_t*>(&data));
        data = 0;
        read_data(&DataBusHandle, 0, 4, reinterpret_cast<uint8_t*>(&data));
        if (data != 1) {
            die_msg("handle_events_internal", "data != 1");
        }
        break;
    }
    default:
        /*Ignore*/
        break;
    }
}

void fss_node::handle_events_internal_aux(fssUint64 event)
{
    switch (event) {
    case 2:
        std::cout << "Node: " << NodeName << " received SC_BEFORE_END_OF_ELABORATION Aux event" << std::endl;
        init_registers();
        break;
    case 4:
        std::cout << "Node: " << NodeName << " received SC_END_OF_ELABORATION Aux event" << std::endl;
        break;
    case 8:
        std::cout << "Node: " << NodeName << " received SC_START_OF_SIMULATION Aux event" << std::endl;
    default:
        /*Ignore*/
        break;
    }
}

void fss_node::init_registers()
{
    // initialize 4 registers each of size 32 (4 bytes) bit to 0
    std::cout << "Node: " << NodeName << " init registers to 0" << std::endl;
    for (uint32_t i = 0; i < 16; i++) {
        std::get<3>(DataBus).push_back(0);
    }
}

void fss_node::update_time_internal(const TimeWindow& window)
{
    std::cout << "Node: " << NodeName << " received time window: [" << window.from << " s - " << window.to << " s]"
              << std::endl;
    current_time = window;
    /**
     *  send the current updated time to the time centeral controller again to confirm time was updated, it will advance
     * the time and then sync again
     */
    other_time_sync_channel_handle->updateTimeWindow(other_time_sync_channel_handle, current_time);
}

void fss_node::handle_events(fssEventsIfHandle _events_handle, fssUint64 _event)
{
    if (!_events_handle) {
        die_null_ptr("handle_events", "_events_handle");
    }
    fss_node* node = gs::container_of<fss_node, fssEventsIf>(_events_handle, &fss_node::EventsHandle);
    node->handle_events_internal(_event);
}

void fss_node::handle_events_aux(fssEventsIfHandle _events_handle, fssUint64 _event)
{
    if (!_events_handle) {
        die_null_ptr("handle_events_aux", "_events_handle");
    }
    fss_node* node = gs::container_of<fss_node, fssEventsIf>(_events_handle, &fss_node::AuxEventsHandle);
    node->handle_events_internal_aux(_event);
}

void fss_node::update_time(fssTimeSyncIfHandle time_sync_handle, TimeWindow window)
{
    if (!time_sync_handle) {
        die_null_ptr("update_time", "time_sync_handle");
    }
    fss_node* node = gs::container_of<fss_node, fssTimeSyncIf>(time_sync_handle, &fss_node::TimeSyncHandle);
    node->update_time_internal(window);
}

fssString fss_node::do_command(fssControlIfHanlde _self, fssString _cmd)
{
    if (!_self) {
        die_null_ptr("do_command", "_self");
    }
    if (!_cmd) {
        die_null_ptr("do_command", "_cmd");
    }
    fss_node* node = gs::container_of<fss_node, fssControlIf>(_self, &fss_node::ControlHandle);
    rapidjson::Document document, ret;
    ret.SetObject();
    document.Parse(_cmd);
    if (!document.IsObject()) {
        die_msg("do_command", "!document.IsObject()");
    }
    uint64_t address = 0;
    uint32_t value = 0;
    if (document.HasMember("address")) {
        address = document["address"].GetUint64();
    } else {
        return nullptr;
    }
    if (document.HasMember("command")) {
        if (std::string(document["command"].GetString()) == std::string("get_reg")) {
            node->read_data(&node->DataBusHandle, address, 4, reinterpret_cast<uint8_t*>(&value));
        } else if (std::string(document["command"].GetString()) == std::string("set_reg")) {
            if (document.HasMember("value")) {
                value = document["value"].GetUint();
                node->write_data(&node->DataBusHandle, address, 4, reinterpret_cast<uint8_t*>(&value));
            } else {
                return nullptr;
            }
        } else {
            return nullptr;
        }
        ret.AddMember("value", rapidjson::Value(value), ret.GetAllocator());
        rapidjson::StringBuffer str;
        rapidjson::Writer<rapidjson::StringBuffer> writer(str);
        ret.Accept(writer);
        node->inspection_str = str.GetString();
        return node->inspection_str.c_str();
    }
    return nullptr;
}

// how many instances of the item you need to access
fssBusIfItemSize fss_node::get_number(fssBusIfHandle _self)
{
    if (!_self) {
        die_null_ptr("get_number", "_self");
    }
    return 1;
} // you can only add one unique item
fssString fss_node::get_name(fssBusIfHandle _self, fssBusIfItemIndex _index)
{
    if (!_self) {
        die_null_ptr("get_name", "_self");
    }
    switch (_index) {
    case 0:
        return "address";
    case 1:
        return "size";
    case 2:
        return "is_write";
    case 3:
        return "data";
    default:
        std::cerr << "fss_node -> fssBusIf -> get_name() Error: index: " << _index << " is no supported!" << std::endl;
        exit(EXIT_FAILURE);
    }
}
fssBusIfItemSize fss_node::get_size(fssBusIfHandle _self, fssBusIfItemIndex _index)
{
    if (!_self) {
        die_null_ptr("get_size", "_self");
    }
    fss_node* node = gs::container_of<fss_node, fssBusIf>(_self, &fss_node::DataBusHandle);
    switch (_index) {
    case 0:
    case 1:
        return sizeof(uint64_t);
    case 2:
        return sizeof(bool);
    case 3:
        return std::get<3>(node->DataBus).size();
    default:
        std::cerr << "fss_node -> fssBusIf -> get_size() Error: index: " << _index << " is no supported!" << std::endl;
        exit(EXIT_FAILURE);
    }
}
fssBusIfItemSize fss_node::get_index(fssBusIfHandle _self, fssString _name)
{
    if (!_self) {
        die_null_ptr("get_index", "_self");
    }
    if (!_name) {
        die_null_ptr("get_index", "_name");
    }
    if (!std::strcmp(_name, "address")) {
        return 0;
    } else if (!std::strcmp(_name, "size")) {
        return 1;
    } else if (!std::strcmp(_name, "is_write")) {
        return 2;
    } else if (!std::strcmp(_name, "data")) {
        return 3;
    } else {
        std::cerr << "fss_node -> fssBusIf -> get_index() Error: name: " << _name << " is no supported!" << std::endl;
        exit(EXIT_FAILURE);
    }
}
fssBusIfItemIndex fss_node::add_item(fssBusIfHandle _self, fssString _name, fssBusIfItemSize _size)
{
    if (!_self) {
        die_null_ptr("add_item", "_self");
    }
    if (!_name) {
        die_null_ptr("add_item", "_name");
    }
    // Can't add any items to this bus
    return _self->getIndex(_self, _name);
}
fssBusIfItemData fss_node::get_item(fssBusIfHandle _self, fssBusIfItemIndex _index)
{
    if (!_self) {
        die_null_ptr("get_item", "_self");
    }
    fss_node* node = gs::container_of<fss_node, fssBusIf>(_self, &fss_node::DataBusHandle);
    switch (_index) {
    case 0:
        std::cout << "Node: " << node->NodeName << " read address, value: 0x" << std::hex << std::get<0>(node->DataBus)
                  << std::endl;
        return reinterpret_cast<void*>(&std::get<0>(node->DataBus));
    case 1:
        std::cout << "Node: " << node->NodeName << " read size, value: 0x" << std::hex << std::get<1>(node->DataBus)
                  << std::endl;
        return reinterpret_cast<void*>(&std::get<1>(node->DataBus));
    case 2:
        std::cout << "Node: " << node->NodeName << " read is_write, value: " << std::boolalpha
                  << std::get<2>(node->DataBus) << std::endl;
        return reinterpret_cast<void*>(&std::get<2>(node->DataBus));
    case 3:
        if (!std::get<3>(node->DataBus).size()) {
            die_msg("get_item", "!std::get<3>(node->DataBus).size()");
        }
        std::cout << "Node: " << node->NodeName << " read data, value: ";
        for (uint64_t i = std::get<0>(node->DataBus);
             i < /*size + address*/ (std::get<0>(node->DataBus) + std::get<1>(node->DataBus)); i++) {
            std::cout << "0x" << std::hex << static_cast<uint64_t>(std::get<3>(node->DataBus)[i]) << " ";
        }
        std::cout << std::endl;
        return reinterpret_cast<void*>(
            reinterpret_cast<uint8_t*>(std::get<3>(node->DataBus).data() + std::get<0>(node->DataBus)));
    default:
        std::cerr << "fss_node -> fssBusIf -> get_item() Error: index: " << _index << " is no supported!" << std::endl;
        exit(EXIT_FAILURE);
    }
}
void fss_node::set_item(fssBusIfHandle _self, fssBusIfItemIndex _index, fssBusIfItemData _data)
{
    if (!_self) {
        die_null_ptr("set_item", "_self");
    }
    if (!_data) {
        die_null_ptr("set_item", "_data");
    }
    fss_node* node = gs::container_of<fss_node, fssBusIf>(_self, &fss_node::DataBusHandle);
    if (!node) {
        die_null_ptr("set_item", "node");
    }
    switch (_index) {
    case 0:
        std::cout << "Node: " << node->NodeName << " write address, value: 0x" << std::hex
                  << *(reinterpret_cast<uint64_t*>(_data)) << std::endl;
        std::get<0>(node->DataBus) = *(reinterpret_cast<uint64_t*>(_data));
        break;
    case 1:
        std::cout << "Node: " << node->NodeName << " write size, value: 0x" << std::hex
                  << *(reinterpret_cast<uint64_t*>(_data)) << std::endl;
        std::get<1>(node->DataBus) = *(reinterpret_cast<uint64_t*>(_data));
        break;
    case 2:
        std::cout << "Node: " << node->NodeName << " write is_write, value: " << std::boolalpha
                  << *(reinterpret_cast<bool*>(_data)) << std::endl;
        std::get<2>(node->DataBus) = *(reinterpret_cast<bool*>(_data));
        break;
    case 3: {
        if (std::get<1>(node->DataBus) < 0) {
            die_msg("set_item", "std::get<1>(node->DataBus) < 0");
        }
        if ((std::get<0>(node->DataBus) + std::get<1>(node->DataBus)) > std::get<3>(node->DataBus).size()) {
            std::get<3>(node->DataBus).resize(std::get<0>(node->DataBus) + std::get<1>(node->DataBus));
        }
        std::cout << "Node: " << node->NodeName << " write data, value: ";
        for (uint64_t i = 0; i < /*size*/ std::get<1>(node->DataBus); i++) {
            std::cout << "0x" << std::hex << static_cast<uint64_t>(*(reinterpret_cast<uint8_t*>(_data) + i)) << " ";
        }
        std::cout << std::endl;
        std::copy(
            reinterpret_cast<uint8_t*>(_data), reinterpret_cast<uint8_t*>(_data) + /*size*/ std::get<1>(node->DataBus),
            reinterpret_cast<uint8_t*>(std::get<3>(node->DataBus).data()) + /*address*/ std::get<0>(node->DataBus));
        break;
    }
    default:
        std::cerr << "fss_node -> fssBusIf -> get_item() Error: index: " << _index << " is no supported!" << std::endl;
        exit(EXIT_FAILURE);
    }
}

void fss_node::write_data(fssBusIfHandle handle, uint64_t address, uint64_t size, uint8_t* data)
{
    bool is_write = true;
    fss_node::set_item(handle, 0, &address);
    fss_node::set_item(handle, 1, &size);
    fss_node::set_item(handle, 2, &is_write);
    fss_node::set_item(handle, 3, data);
}

void fss_node::read_data(fssBusIfHandle handle, uint64_t address, uint64_t size, uint8_t* data)
{
    if (size > std::get<3>(DataBus).size()) {
        die_msg("read_data", "size > std::get<3>(DataBus).size()");
    }
    bool is_write = false;
    fss_node::set_item(handle, 0, &address);
    fss_node::set_item(handle, 1, &size);
    fss_node::set_item(handle, 2, &is_write);
    uint8_t* bus_data = reinterpret_cast<uint8_t*>(fss_node::get_item(handle, 3));
    std::copy(bus_data, bus_data + size, data);
}

void fss_node::transmit(fssBusIfHandle _self)
{
    if (!_self) {
        die_null_ptr("transmit", "_self");
    }
    fss_node* node = gs::container_of<fss_node, fssBusIf>(_self, &fss_node::DataBusHandle);
    if (!node) {
        die_null_ptr("transmit", "node");
    }
    switch (std::get<0>(node->DataBus)) { // address
    case 0x0:
        if (std::get<2>(node->DataBus)) { // write
            std::cout << node->NodeName << ": Register 0x0 write, data: 0x" << std::hex
                      << *reinterpret_cast<uint32_t*>(std::get<3>(node->DataBus).data()) << std::endl;
        } else { // read
            std::cout << node->NodeName << ": Register 0x0 read, data: 0x" << std::hex
                      << *reinterpret_cast<uint32_t*>(std::get<3>(node->DataBus).data()) << std::endl;
        }
        break;
    case 0x4:
        if (std::get<2>(node->DataBus)) {
            std::cout << node->NodeName << ": Register 0x4 write, data: 0x" << std::hex
                      << *reinterpret_cast<uint32_t*>(std::get<3>(node->DataBus).data() + 4) << std::endl;
        } else {
            std::cout << node->NodeName << ": Register 0x4 read, data: 0x" << std::hex
                      << *reinterpret_cast<uint32_t*>(std::get<3>(node->DataBus).data() + 4) << std::endl;
        }
        break;
    case 0x8:
        if (std::get<2>(node->DataBus)) {
            std::cout << node->NodeName << ": Register 0x8 write, data: 0x" << std::hex
                      << *reinterpret_cast<uint32_t*>(std::get<3>(node->DataBus).data() + 8) << std::endl;
        } else {
            std::cout << node->NodeName << ": Register 0x8 read, data: 0x" << std::hex
                      << *reinterpret_cast<uint32_t*>(std::get<3>(node->DataBus).data() + 8) << std::endl;
        }
        break;
    default:
        std::cerr << "fss_node -> fssBusIf -> transmit() Error: access to register address "
                  << std::get<0>(node->DataBus) << " which is not existing!" << std::endl;
        exit(EXIT_FAILURE);
    }
}

fssBindIFHandle fss_node::init_node(std::shared_ptr<fss_node> node)
{
    if (!node) {
        return nullptr;
    }
    std::cout << "Create Node: " << node->NodeName << std::endl;
    node->BindHandle.version = Version;
    node->BindHandle.bindEventsIf = &fss_node::bind_events_channel;
    node->BindHandle.getEventsIfHandle = &fss_node::get_events_handle;
    node->BindHandle.bindTimeSyncIf = &fss_node::bind_time_sync_channel;
    node->BindHandle.getTimeSyncIfHandle = &fss_node::get_time_sync_handle;
    node->BindHandle.bindBusIf = &fss_node::bind_data_channel;
    node->BindHandle.getBusIfHandle = &fss_node::get_data_bus_handle;
    node->BindHandle.bindControlIf = &fss_node::bind_control_channel;
    node->BindHandle.getControlIfHandle = &fss_node::get_control_handle;
    node->EventsHandle.version = Version;
    node->EventsHandle.handleEvents = &fss_node::handle_events;
    node->AuxEventsHandle.version = Version;
    node->AuxEventsHandle.handleEvents = &fss_node::handle_events_aux;
    node->TimeSyncHandle.version = Version;
    node->TimeSyncHandle.updateTimeWindow = &fss_node::update_time;
    node->DataBusHandle.version = Version;
    node->DataBusHandle.getNumber = &fss_node::get_number;
    node->DataBusHandle.getName = &fss_node::get_name;
    node->DataBusHandle.getSize = &fss_node::get_size;
    node->DataBusHandle.getIndex = &fss_node::get_index;
    node->DataBusHandle.addItem = &fss_node::add_item;
    node->DataBusHandle.getItem = &fss_node::get_item;
    node->DataBusHandle.setItem = &fss_node::set_item;
    node->DataBusHandle.transmit = &fss_node::transmit;
    node->ControlHandle.version = Version;
    node->ControlHandle.doCommand = &fss_node::do_command;
    return &node->BindHandle;
}

fssBindIFHandle fssCreateNode(fssString _node_name, fssConfigIfHandle _config_if_handle)
{
    if (!_node_name) {
        die_null_ptr("fssCreateNode", "_node_name");
    }
    if (!_config_if_handle) {
        die_null_ptr("fssCreateNode", "_config_if_handle");
    }
    if (_config_if_handle->version != fss_node::Version) {
        die_msg("fssCreateNode", "_config_if_handle->version != fss_node::Version");
    }
    std::string configs = _config_if_handle->getConfigs(_config_if_handle, "baud_rate");
    auto separator_idx = configs.find_last_of(';');
    std::string baud_rate_str = "baud_rate=";
    auto baud_rate_str_len = baud_rate_str.size();
    auto baud_rate_str_idx = configs.find(baud_rate_str);
    if (separator_idx == std::string::npos) {
        die_msg("fssCreateNode", "configurations doesn't use ';' separator!");
    }
    if (baud_rate_str_idx == std::string::npos) {
        die_msg("fssCreateNode", "configurations doesn't include baud_rate parameter!");
    }
    uint64_t baud_rate = 0;
    std::stringstream ss(configs.substr(baud_rate_str_len, separator_idx));
    ss >> baud_rate;
    std::cout << "Node: " << _node_name << " has Baudrate: " << baud_rate << std::endl;
    auto node = std::make_shared<fss_node>(_node_name, baud_rate);
    auto bind_handle = fss_node::init_node(node);
    if (!bind_handle) {
        die_null_ptr("fssCreateNode", "bind_handle");
    }
    NodeInstances.emplace(std::make_pair(bind_handle, node));
    return bind_handle;
}

fssRetStatus fssDestroyNode(fssBindIFHandle _node_handle)
{
    if (!_node_handle) {
        std::cerr << "fss_node -> fssDestroyNode() Error: fssBindIFHandle is nullptr!" << std::endl;
        return fssStatusError;
    }
    if (!NodeInstances.empty() || (NodeInstances.find(_node_handle) != NodeInstances.end())) {
        std::cout << "Destroy node: " << NodeInstances.at(_node_handle)->get_name() << std::endl;
        NodeInstances.erase(_node_handle);
    }
    return fssStatusOk;
}