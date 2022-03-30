/*
*  This file is part of libgsutils
* Copyright (c) 2022 GreenSocs
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version, or under the
* Apache License, Version 2.0 (the "License”) at your discretion.
*
* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
* You may obtain a copy of the Apache License at
* http://www.apache.org/licenses/LICENSE-2.0
*/


#ifndef _LIBGSUTILS_PORTS_TARGET_SIGNAL_SOCKET_H
#define _LIBGSUTILS_PORTS_TARGET_SIGNAL_SOCKET_H

#include <functional>
#include <cassert>

#include <systemc>

template <class T>
class TargetSignalSocket;

template <class T>
class TargetSignalSocketProxy : public sc_core::sc_signal_inout_if<T> {
public:
    using Iface = sc_core::sc_signal_inout_if<T>;
    using ValueChangedCallback = std::function<void (const T&)>;

protected:
    TargetSignalSocket<T> &m_parent;

    T m_val;
    ValueChangedCallback m_cb;
    sc_core::sc_event m_ev;

public:
    TargetSignalSocketProxy(TargetSignalSocket<T> &parent)
        : m_parent(parent)
    {}

    void register_value_changed_cb(const ValueChangedCallback &cb)
    {
        m_cb = cb;
    }

    TargetSignalSocket<T> & get_parent()
    {
        return m_parent;
    }

    void notify()
    {
        m_ev.notify();
    }

    /* SystemC interfaces */

    /* sc_core::sc_signal_in_if<T> */
    virtual const sc_core::sc_event& default_event() const
    {
        return m_ev;
    }

    virtual const sc_core::sc_event& value_changed_event() const
    {
        return m_ev;
    }

    virtual const T& read() const
    {
        return m_val;
    }

    virtual const T& get_data_ref() const
    {
        return m_val;
    }

    virtual bool event() const
    {
        /* Not implemented */
        assert(false);
        return false;
    }

    /* sc_core::sc_signal_write_if<T> */
    virtual void write(const T& val)
    {
        bool changed = (val != m_val);

        m_val = val;

        if (m_cb) {
            m_cb(val);
        }

        if (changed) {
            m_ev.notify();
        }
    }
};

template <>
class TargetSignalSocketProxy<bool> : public sc_core::sc_signal_inout_if<bool> {
public:
    using Iface = sc_core::sc_signal_inout_if<bool>;
    using ValueChangedCallback = std::function<void (const bool&)>;

protected:
    TargetSignalSocket<bool> &m_parent;

    bool m_val;
    ValueChangedCallback m_cb;
    sc_core::sc_event m_ev;
    sc_core::sc_event m_posedge_ev;
    sc_core::sc_event m_negedge_ev;

public:
    TargetSignalSocketProxy<bool>(TargetSignalSocket<bool> &parent)
        : m_parent(parent)
    {}

    void register_value_changed_cb(const ValueChangedCallback &cb)
    {
        m_cb = cb;
    }

    TargetSignalSocket<bool> & get_parent()
    {
        return m_parent;
    }

    void notify()
    {
        m_ev.notify();
    }

    /* SystemC interfaces */

    /* sc_core::sc_signal_in_if<bool> */
    virtual const sc_core::sc_event& default_event() const
    {
        return m_ev;
    }

    virtual const sc_core::sc_event& value_changed_event() const
    {
        return m_ev;
    }

    virtual const sc_core::sc_event& posedge_event() const
    {
        return m_posedge_ev;
    }

    virtual const sc_core::sc_event& negedge_event() const
    {
        return m_negedge_ev;
    }

    virtual const bool& read() const
    {
        return m_val;
    }

    virtual const bool& get_data_ref() const
    {
        return m_val;
    }

    virtual bool event() const
    {
        /* Not implemented */
        assert(false);
        return false;
    }

    virtual bool posedge() const
    {
        /* Not implemented */
        assert(false);
        return false;
    }

    virtual bool negedge() const
    {
        /* Not implemented */
        assert(false);
        return false;
    }

    /* sc_core::sc_signal_write_if<bool> */
    virtual void write(const bool& val)
    {
        bool changed = (val != m_val);
        m_val = val;

        if (m_cb) {
            m_cb(val);
        }

        if (changed) {
            m_ev.notify();
            val ? m_posedge_ev.notify() : m_negedge_ev.notify();
        }
    }
};

/* TODO: TargetSignalSocketProxy specialization for sc_dt::sc_logic type */

template <class T>
class TargetSignalSocket
    : public sc_core::sc_export< sc_core::sc_signal_inout_if<T> > {
public:
    using Iface = typename TargetSignalSocketProxy<T>::Iface;
    using Parent = sc_core::sc_export<Iface>;
    using ValueChangedCallback = typename TargetSignalSocketProxy<T>::ValueChangedCallback;

protected:
    TargetSignalSocketProxy<T> m_proxy;

public:
    TargetSignalSocket(const char *name)
        : Parent(name)
        , m_proxy(*this)
    {
        Parent::bind(m_proxy);
    }

    void register_value_changed_cb(const ValueChangedCallback &cb)
    {
        m_proxy.register_value_changed_cb(cb);
    }

    const T& read() const
    {
        return m_proxy.read();
    }
};

#endif
