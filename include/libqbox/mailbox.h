#pragma once

#include <systemc>
#include "tlm.h"
#include "tlm_utils/simple_target_socket.h"

template <unsigned int N_TARGETS>
class Mailbox : public sc_core::sc_module {
public:
    sc_core::sc_vector<tlm_utils::simple_target_socket_tagged<Mailbox>> socket;

    sc_core::sc_vector<sc_core::sc_signal<bool, sc_core::SC_MANY_WRITERS>> reset;
    sc_core::sc_vector<sc_core::sc_signal<bool, sc_core::SC_MANY_WRITERS>> irq;

    sc_core::sc_event update_event;

    SC_HAS_PROCESS(Mailbox);
    Mailbox(const sc_core::sc_module_name &name)
        : sc_module(name)
        , socket("socket", N_TARGETS)
        , reset("reset", N_TARGETS)
        , irq("irq", N_TARGETS)
    {
        for (unsigned int i = 0; i < N_TARGETS; i++) {
            socket[i].register_b_transport(this, &Mailbox::b_transport, i);
            reset[i] = 1;
        }

        SC_THREAD(update);
    }

private:
    uint32_t mbox = 0xffffffff;

    uint32_t m_delay_ns[N_TARGETS];

    uint32_t m_reset_values[N_TARGETS];
    uint32_t m_irq_values[N_TARGETS];

    uint32_t m_counter[N_TARGETS];

    void update()
    {
        for (;;) {
            wait(update_event);

            for (unsigned int i = 0; i < N_TARGETS; i++) {
                if (m_reset_values[i]) {
                    reset[i] = 0;
                    wait(0, sc_core::SC_NS);
                    reset[i] = 1;
                    m_reset_values[i] = 0;
                }

                irq[i] = m_irq_values[i];
            }
        }
    }

    void b_transport(int id, tlm::tlm_generic_payload& trans, sc_core::sc_time& delay)
    {
        unsigned char *ptr = trans.get_data_ptr();
        enum tlm::tlm_command cmd = trans.get_command();
        uint64_t addr = trans.get_address();

        if (cmd == tlm::TLM_WRITE_COMMAND) {
            uint32_t val32 = *(uint32_t *)ptr;
            switch (addr) {
            case 0x000: /* mailbox */
                printf("%s: cpu %d writing %d\n", name(), id, *(uint32_t *)ptr);
                fflush(stdout);
                mbox = val32;
                break;
            case 0x004: /* set delay in ns */
                m_delay_ns[id] = *(uint32_t *)ptr;
                break;
            case 0x008: /* reset */
                {
                    uint8_t target = (val32 >> 8) & 0xff;
                    m_reset_values[target] = 1;
                    update_event.notify(m_delay_ns[id], sc_core::SC_NS);
                }
                break;
            case 0x00C: /* irq */
                {
                    uint8_t target = (val32 >> 8) & 0xff;
                    uint8_t value = val32 & 0xff;
                    m_irq_values[target] = value;
                    if (m_delay_ns[id]) {
                        update_event.notify(m_delay_ns[id], sc_core::SC_NS);
                    }
                    else {
                        update_event.notify();
                        sc_core::wait(0, sc_core::SC_NS);
                    }
                }
                break;
            case 0xffc: /* stop simulation */
                sc_core::sc_stop();
                break;
            }
        }
        else if (cmd == tlm::TLM_READ_COMMAND) {
            switch (addr) {
            case 0x000: /* mailbox */
                *(uint32_t *)ptr = mbox;
                break;
            case 0xff8: /* auto increment counter */
                *ptr = m_counter[id]++;
                break;
            case 0xffc: /* read cpu index */
                *ptr = id;
                break;
            }
        }

        trans.set_dmi_allowed(false);
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    }
};
