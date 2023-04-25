/*
 *
 *  copywrite Qualcomm 2021
 */

#pragma once

#include <greensocs/gsutils/ports/initiator-signal-socket.h>
#include <systemc>
#include <cci_configuration>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>
#include <greensocs/gsutils/module_factory_registery.h>

#include <scp/report.h>

/* registers as extracted

IPC_PROTOCOLp_CLIENTc_VERSION, 0x400000, Read, Yes, 0x10200
IPC_PROTOCOLp_CLIENTc_ID, 0x400004, Read, Yes, 0x0
IPC_PROTOCOLp_CLIENTc_CONFIG, 0x400008, Read/Write, Yes, 0x0
IPC_PROTOCOLp_CLIENTc_SEND, 0x40000C, Write, Yes, 0x0
IPC_PROTOCOLp_CLIENTc_RECV_ID, 0x400010, Read, Yes, 0xFFFFFFFF
IPC_PROTOCOLp_CLIENTc_RECV_SIGNAL_ENABLE, 0x400014, Write, Yes, 0x0
IPC_PROTOCOLp_CLIENTc_RECV_SIGNAL_DISABLE, 0x400018, Write, Yes, 0x0
IPC_PROTOCOLp_CLIENTc_RECV_SIGNAL_CLEAR, 0x40001C, Write, Yes, 0x0
IPC_PROTOCOLp_CLIENTc_GLOBAL_SIGNAL_STATUS_0, 0x400020, Read, Yes, 0x0
IPC_PROTOCOLp_CLIENTc_GLOBAL_SIGNAL_STATUS_1, 0x400024, Read, Yes, 0x0
IPC_PROTOCOLp_CLIENTc_GLOBAL_SIGNAL_CLEAR_0, 0x400028, Write, Yes, 0x0
IPC_PROTOCOLp_CLIENTc_GLOBAL_SIGNAL_CLEAR_1, 0x40002C, Write, Yes, 0x0
IPC_PROTOCOLp_CLIENTc_GLOBAL_SIGNAL_MASK_0, 0x400030, Read/Write, Yes, 0x0
IPC_PROTOCOLp_CLIENTc_GLOBAL_SIGNAL_MASK_1, 0x400034, Read/Write, Yes, 0x0
IPC_PROTOCOLp_CLIENTc_CLIENT_CLEAR, 0x400038, Write, Yes, 0x0
IPC_PROTOCOLp_CLIENTc_GLOBAL_QUAL_SIGNAL_STATUS_0, 0x40003C, Read, Yes, 0x0
IPC_PROTOCOLp_CLIENTc_GLOBAL_QUAL_SIGNAL_STATUS_1, 0x400040, Read, Yes, 0x0
IPC_PROTOCOLp_CLIENTc_SEND16, 0x400044, Write, Yes, 0x0
IPC_PROTOCOLp_CLIENTc_RECV_CLIENT_PRIORITY_n, 0x400100, Read/Write, Yes, 0x0
IPC_PROTOCOLp_CLIENTc_RECV_CLIENT_ENABLE_0_n, 0x400200, Write, Yes, 0x0
IPC_PROTOCOLp_CLIENTc_RECV_CLIENT_ENABLE_1_n, 0x400300, Write, Yes, 0x0
IPC_PROTOCOLp_CLIENTc_RECV_CLIENT_DISABLE_0_n, 0x400400, Write, Yes, 0x0
IPC_PROTOCOLp_CLIENTc_RECV_CLIENT_DISABLE_1_n, 0x400500, Write, Yes, 0x0
IPC_PROTOCOLp_CLIENTc_CLIENT_SIGNAL_STATUS_0_n, 0x400600, Read, Yes, 0x0
IPC_PROTOCOLp_CLIENTc_CLIENT_SIGNAL_STATUS_1_n, 0x400700, Read, Yes, 0x0
IPC_PROTOCOLp_CLIENTc_CLIENT_ENABLE_STATUS_0_n, 0x400800, Read, Yes, 0x0
IPC_PROTOCOLp_CLIENTc_CLIENT_ENABLE_STATUS_1_n, 0x400900, Read, Yes, 0x0

*/

/**
 * @class IPCC
 *
 * @brief An IPCC implementation
 *
 */

class IPCC : public sc_core::sc_module
{
private:
    enum IPCC_Regs {
        VERSION,
        ID,
        CONFIG,
        SEND,
        RECV_ID,
        RECV_SIGNAL_ENABLE,
        RECV_SIGNAL_DISABLE,
        RECV_SIGNAL_CLEAR,
        GLOBAL_SIGNAL_STATUS_0,
        GLOBAL_SIGNAL_STATUS_1,
        GLOBAL_SIGNAL_CLEAR_0,
        GLOBAL_SIGNAL_CLEAR_1,
        GLOBAL_SIGNAL_MASK_0,
        GLOBAL_SIGNAL_MASK_1,
        CLIENT_CLEAR,
        GLOBAL_QUAL_SIGNAL_STATUS_0,
        GLOBAL_QUAL_SIGNAL_STATUS_1,
        SEND16,
        RECV_CLIENT_PRIORITY_n = 0x100 / 4,
        RECV_CLIENT_ENABLE_0_n = 0x200 / 4,
        RECV_CLIENT_ENABLE_1_n = 0x300 / 4,
        RECV_CLIENT_DISABLE_0_n = 0x400 / 4,
        RECV_CLIENT_DISABLE_1_n = 0x500 / 4,
        // I think these means "Is a signal coming from client N active on this client"
        CLIENT_SIGNAL_STATUS_0_n = 0x600 / 4,
        CLIENT_SIGNAL_STATUS_1_n = 0x700 / 4,
        CLIENT_ENABLE_STATUS_0_n = 0x800 / 4,
        CLIENT_ENABLE_STATUS_1_n = 0x900 / 4,
        MAX_CLIENT_SIZE = 64
    };
    class IPC_client
    {
    public:
        uint32_t regs[0x1000 / 4];
        // These could be parameters, but then each IPC_client needs a unique name and would need to
        // be constructed during elaboration
        uint32_t p_client_size;
        uint32_t p_version;
        bool status(int client, int signal) {
            assert(client < p_client_size);
            assert(signal < p_client_size);
            if (signal < 32)
                return (regs[CLIENT_SIGNAL_STATUS_0_n + client] & (1 << signal)) != 0;
            else
                return (regs[CLIENT_SIGNAL_STATUS_1_n + client] & (1 << (signal - 32))) != 0;
        }
        bool enable(int client, int signal) {
            assert(client < p_client_size);
            assert(signal < p_client_size);
            if (signal < 32)
                return (regs[CLIENT_ENABLE_STATUS_0_n + client] & (1 << signal)) != 0;
            else
                return (regs[CLIENT_ENABLE_STATUS_1_n + client] & (1 << (signal - 32))) != 0;
        }

        void set_status(int client, int signal) {
            assert(client < p_client_size);
            assert(signal < p_client_size);
            if (signal < 32)
                regs[CLIENT_SIGNAL_STATUS_0_n + client] |= (1 << signal);
            else
                regs[CLIENT_SIGNAL_STATUS_1_n + client] |= (1 << (signal - 32));
        }

        void clear_status(int client, int signal) {
            assert(client < p_client_size);
            assert(signal < p_client_size);
            if (signal < 32)
                regs[CLIENT_SIGNAL_STATUS_0_n + client] &= ~(1 << signal);
            else
                regs[CLIENT_SIGNAL_STATUS_1_n + client] &= ~(1 << (signal - 32));
        }

        void set_enable(int client, int signal) {
            assert(client < p_client_size);
            assert(signal < p_client_size);
            if (signal < 32)
                regs[CLIENT_ENABLE_STATUS_0_n + client] |= (1 << signal);
            else
                regs[CLIENT_ENABLE_STATUS_1_n + client] |= (1 << (signal - 32));
        }

        void clear_enable(int client, int signal) {
            assert(client < p_client_size);
            assert(signal < p_client_size);
            if (signal < 32)
                regs[CLIENT_ENABLE_STATUS_0_n + client] &= ~(1 << signal);
            else
                regs[CLIENT_ENABLE_STATUS_1_n + client] &= ~(1 << (signal - 32));
        }

        IPC_client(): p_client_size(MAX_CLIENT_SIZE), p_version(0x10200) {
            memset(regs, 0, sizeof(regs));
            regs[VERSION] = p_version;
            regs[RECV_ID] = 0xFFFFFFFF;
        };
    };
    IPC_client client[4][MAX_CLIENT_SIZE];
    sc_core::sc_event update_irq_ev;

protected:
    cci::cci_param<unsigned int> p_num_irq;

    uint32_t read(IPC_client& src, uint64_t addr) {
        uint32_t ret = src.regs[addr / 4];
        // Side effects
        switch (addr / 4) {
        case RECV_ID:
            if ((src.regs[CONFIG] & 0x1) && (ret != 0xffffffff)) {
                int sc = (src.regs[ID] >> 16) & 0x3f;
                src.clear_status(ret >> 16, ret & 0xffff);
                if (irq_status[sc]) {
                    irq_status[sc] = false;
                    irq[sc]->write(0);
                    SCP_INFO(SCMOD) << "CLEAR IRQ (on read) " << sc;
                }
            }
            break;
        }

        return ret;
    }
    void write(IPC_client& src, uint64_t addr, uint32_t data) {
        src.regs[addr / 4] = data;
        // side effects
        switch (addr / 4) {
        case SEND: {
            int s = data & 0xffff;
            if (data & 0x80000000) {
                for (int c = 0; c < src.p_client_size; c++) {
                    send(src, c, s);
                }
            } else {
                int c = (data >> 16) & 0x8fff;
                send(src, c, s);
            }
            break;
        }
        case RECV_SIGNAL_ENABLE: {
            int c = (data >> 16) & 0xffff;
            int s = (data & 0xffff);
            src.set_enable(c, s);
            break;
        }
        case RECV_SIGNAL_DISABLE: {
            int c = (data >> 16) & 0xffff;
            int s = (data & 0xffff);
            src.clear_enable(c, s);
            break;
        }
        case RECV_SIGNAL_CLEAR: {
            int c = (data >> 16) & 0xffff;
            int s = (data & 0xffff);
            src.clear_status(c, s);
            int sc = (src.regs[ID] >> 16) & 0x3f;
            if (irq_status[sc]) {
                irq[sc]->write(0);
                irq_status[sc] = false;
                SCP_INFO(SCMOD) << "CLEAR IRQ " << sc;
            }
            break;
        }
        case CLIENT_CLEAR: {
            uint32_t id = src.regs[ID];
            if (data & 0x1) {
                memset(src.regs, 0, sizeof(src.regs));
                src.regs[VERSION] = 0x10200;
                src.regs[RECV_ID] = 0xFFFFFFFF;
                src.regs[ID] = id;
            }
            break;
        }
        }
    }
    void send(IPC_client& src, int c, int s) {
        int p = src.regs[ID] & 0x3f;
        int sc = (src.regs[ID] >> 16) & 0x3f;
        IPC_client& dst = client[p][c];
        dst.set_status(sc, s);
    }
    void update_irq() {
        int client_size = client[0][0].p_client_size;
        for (int c = 0; c < client_size; c++) {
            int allirqcount = 0;
            for (int p = 0; p < 3; p++) {
                int irqcount = 0;
                int prio = 0;
                int sig = 0;
                int cli = 0;

                for (int sc = 0; sc < client_size; sc++) {
                    for (int s = 0; s < client_size; s++) {
                        if (client[p][c].status(sc, s)) {
                            // there is an active signal s from p/sc to p/c
                            if ((client[p][c].regs[CONFIG] & 0x80000000) == 0) {
                                if (client[p][c].enable(sc, s)) {
                                    int pr = client[p][c].regs[RECV_CLIENT_PRIORITY_n + sc];
                                    if (irqcount == 0 || pr > prio) {
                                        prio = pr;
                                        sig = s;
                                        cli = sc;
                                    }
                                    irqcount++;
                                }
                            }
                        }
                    }
                }
                allirqcount += irqcount;
                if (irqcount) {
                    client[p][c].regs[RECV_ID] = (cli << 16) + sig;
                } else {
                    client[p][c].regs[RECV_ID] = 0xffffffff;
                }
            }
            if (allirqcount >= 1) {
                if (!irq_status[c]) {
                    SCP_INFO(SCMOD) << "SEND IRQ " << c;
                    irq_status[c] = true;
                    irq[c]->write(1);
                }
            } else {
                if (irq_status[c]) {
                    SCP_INFO(SCMOD) << "CLEAR IRQ " << c;
                    irq_status[c] = false;
                    irq[c]->write(0);
                }
            }
        }
    }
    void b_transport(tlm::tlm_generic_payload& txn, sc_core::sc_time& delay) {
        unsigned int len = txn.get_data_length();
        unsigned char* ptr = txn.get_data_ptr();
        sc_dt::uint64 addr = txn.get_address();
        if (len > sizeof(client)) {
            txn.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
            return;
        }
        assert(len == 4);

        int c = (addr / 0x1000) & 0x3f;
        int p = (addr / 0x40000) & 0x3;
        switch (txn.get_command()) {
        case tlm::TLM_READ_COMMAND: {
            uint32_t data = read(client[p][c], addr & 0xfff);
            memcpy(ptr, &data, len);
            SCP_INFO(SCMOD) << "b_transport read " << std::hex << addr << " " << data;
            break;
        }
        case tlm::TLM_WRITE_COMMAND: {
            uint32_t data;
            memcpy(&data, ptr, len);
            SCP_INFO(SCMOD) << "b_transport write " << std::hex << addr << " " << data;
            write(client[p][c], addr & 0xfff, data);
            break;
        }
        default:
            SCP_ERR(SCMOD) << "TLM command not supported";
            break;
        }

        txn.set_response_status(tlm::TLM_OK_RESPONSE);

        update_irq_ev.notify();
    }

public:
    tlm_utils::simple_target_socket<IPCC> socket;
    // InitiatorSignalSocket<bool> irq[MAX_CLIENT_SIZE];
    sc_core::sc_vector<InitiatorSignalSocket<bool>> irq;
    bool irq_status[MAX_CLIENT_SIZE] = { false };

    SC_HAS_PROCESS(IPCC);

    IPCC(sc_core::sc_module_name name)
    : p_num_irq ("num_irq", MAX_CLIENT_SIZE, "Number of irq")
    , socket("target_socket")
    , irq ("irq", p_num_irq)
    {
        int client_size = client[0][0].p_client_size;
        for (int p = 0; p < 3; p++) {
            for (int c = 0; c < client_size; c++) {
                client[p][c].regs[ID] = (c << 16) + p;
            }
        }
        socket.register_b_transport(this, &IPCC::b_transport);

        SC_METHOD(update_irq);
        sensitive << update_irq_ev;
    }

    IPCC() = delete;
    IPCC(const IPCC&) = delete;

    ~IPCC() {}
};

GSC_MODULE_REGISTER(IPCC);
