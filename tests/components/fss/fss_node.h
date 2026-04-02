/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _GS_FSS_NODE_
#define _GS_FSS_NODE_

#include <iostream>
#include <iomanip>
#include <memory>
#include <algorithm>
#include <unordered_map>
#include <vector>
#include <tuple>
#include <cstring>
#include <sstream>
#include <rapidjson/include/rapidjson/document.h>
#include <rapidjson/include/rapidjson/writer.h>
#include <rapidjson/include/rapidjson/stringbuffer.h>
#include "fss_utils.h"
#include "fss_interfaces.h"

class fss_node
{
public:
    fss_node(fssString _nm, uint64_t baud_rate): NodeName(_nm), baudrate(baud_rate) {}
    ~fss_node() {}
    static fssBindIFHandle init_node(std::shared_ptr<fss_node> node);
    static fssRetStatus bind_events_channel(fssBindIFHandle _self, fssString _events_channel_name,
                                            fssEventsIfHandle _events_channel_handle);
    static fssRetStatus bind_time_sync_channel(fssBindIFHandle _self, fssString _time_sync_channel_name,
                                               fssTimeSyncIfHandle _time_sync_channel_handle);
    static fssRetStatus bind_data_channel(fssBindIFHandle _self, fssString _data_channel_name,
                                          fssBusIfHandle _data_channel_handle);
    static fssRetStatus bind_control_channel(fssBindIFHandle _self, fssString _control_channel_name,
                                             fssControlIfHanlde _control_channel_handle);
    static fssEventsIfHandle get_events_handle(fssBindIFHandle _self, fssString _events_channel_name);
    static fssTimeSyncIfHandle get_time_sync_handle(fssBindIFHandle _self, fssString _time_sync_channel_name);
    static fssBusIfHandle get_data_bus_handle(fssBindIFHandle _self, fssString _data_channel_name);
    static fssControlIfHanlde get_control_handle(fssBindIFHandle _self, fssString _control_channel_name);
    static void handle_events(fssEventsIfHandle _events_handle, fssUint64 _event);
    static void handle_events_aux(fssEventsIfHandle _events_handle, fssUint64 _event);
    static void update_time(fssTimeSyncIfHandle time_sync_handle, TimeWindow window);
    static fssBusIfItemSize get_number(fssBusIfHandle _self);
    static fssString get_name(fssBusIfHandle _self, fssBusIfItemIndex _index);
    static fssBusIfItemSize get_size(fssBusIfHandle _self, fssBusIfItemIndex _index);
    static fssBusIfItemSize get_index(fssBusIfHandle _self, fssString _name);
    static fssBusIfItemIndex add_item(fssBusIfHandle _self, fssString _name, fssBusIfItemSize _size);
    static fssBusIfItemData get_item(fssBusIfHandle _self, fssBusIfItemIndex _index);
    static void set_item(fssBusIfHandle _self, fssBusIfItemIndex _index, fssBusIfItemData _data);
    static void transmit(fssBusIfHandle _self);
    static fssString do_command(fssControlIfHanlde _self, fssString _cmd);

    void write_data(fssBusIfHandle handle, uint64_t address, uint64_t size, uint8_t* data);
    void read_data(fssBusIfHandle handle, uint64_t address, uint64_t size, uint8_t* data);

    void init_registers();
    fssString get_name() const { return NodeName; }

private:
    void handle_events_internal(fssUint64 event);
    void handle_events_internal_aux(fssUint64 event);

    void update_time_internal(const TimeWindow& window);

public:
    static constexpr uint64_t Version = (1ULL << 32); // 0x100000000;
private:
    fssString NodeName;
    fssBindIf BindHandle;
    fssBusIf DataBusHandle;
    fssEventsIf EventsHandle;
    fssEventsIf AuxEventsHandle;
    fssTimeSyncIf TimeSyncHandle;
    fssControlIf ControlHandle;
    uint64_t baudrate;
    fssString other_events_channel_name;
    fssString aux_events_channel_name;
    fssString other_time_sync_channel_name;
    fssEventsIfHandle other_events_channel_handle;
    fssEventsIfHandle aux_events_channel_handle;
    fssTimeSyncIfHandle other_time_sync_channel_handle;
    fssString other_control_channel_name;
    fssControlIfHanlde other_control_channel_handle;
    TimeWindow current_time;
    fssBusIfHandle other_data_bus_handle;
    fssString other_data_bus_name;
    std::tuple<uint64_t, uint64_t, bool, std::vector<uint8_t>> DataBus; // <address, size, is_write, data>
    std::string inspection_str;
};

#endif