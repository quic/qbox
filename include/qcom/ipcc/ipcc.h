/*
 *
 *  copywrite Qualcomm 2021
 */

#pragma once

#include <systemc>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>

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

class IPCC : public sc_core::sc_module {
private:
    enum {
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
    };
    class IPC_client {
    public:
        uint32_t regs[0x1000 / 4];
        IPC_client& clients[64];
        bool status(int client, int signal)
        {
            assert(client < 64);
            assert(signal < 64);
            if (signal < 32)
                return (regs[CLIENT_SIGNAL_STATUS_0_n + client] & (1 << signal)) != 0;
            else
                return (regs[CLIENT_SIGNAL_STATUS_1_n + client] & (1 << (signal - 32))) != 0;
        }
        bool enable(int client, int signal)
        {
            assert(client < 64);
            assert(signal < 64);
            if (signal < 32)
                return (regs[CLIENT_ENABLE_STATUS_0_n + client] & (1 << signal)) != 0;
            else
                return (regs[CLIENT_SIGNAL_ENABLE_1_n + client] & (1 << (signal - 32))) != 0;
        }

        void set_status(int client, int signal)
        {
            assert(client < 64);
            assert(signal < 64);
            if (signal < 32)
                regs[CLIENT_SIGNAL_STATUS_0_n + client] |= (1 << signal);
            else
                return regs[CLIENT_SIGNAL_STATUS_1_n + client] |= (1 << (signal - 32));
        }

        void clear_status(int client, int signal)
        {
            assert(client < 64);
            assert(signal < 64);
            if (signal < 32)
                regs[CLIENT_SIGNAL_STATUS_0_n + client] &= ~(1 << signal);
            else
                return regs[CLIENT_SIGNAL_STATUS_1_n + client] &= ~(1 << (signal - 32));
        }

        void set_enable(int client, int signal)
        {
            assert(client < 64);
            assert(signal < 64);
            if (signal < 32)
                regs[CLIENT_ENABLE_STATUS_0_n + client] |= (1 << signal);
            else
                return regs[CLIENT_ENABLE_STATUS_1_n + client] |= (1 << (signal - 32));
        }

        void clear_enable(int client, int signal)
        {
            assert(client < 64);
            assert(signal < 64);
            if (signal < 32)
                regs[CLIENT_ENABLE_STATUS_0_n + client] &= ~(1 << signal);
            else
                return regs[CLIENT_ENABLE_STATUS_1_n + client] &= ~(1 << (signal - 32));
        }

        IPCStruct_client()
        {
            memset(regs, 0, sizeof(data));
            regs[VERSION] = 0x10200;
            regs[RECV_ID] = 0xFFFFFFFF;
        };

        uint32_t read(uint64_t addr)
        {
            return regs[addr / 4];
            // Side effects
        }
        void write(uint64_t addr, uint32_t data)
        {
            regs[addr / 4] = data;
            // side effects
            switch (addr / 4) {
            case SEND:
                int s = data & 0xffff;
                if (data & 0x80000000) {
                    for (int c = 0; c < 64; c++) {
                        set_status(c, s);
                    }
                } else {
                    int c = data >> 16 & 0x8fff;
                    set_status(c, s);
                }
                break;
            case RECV_SIGNAL_ENABLE:
                int c = (data >> 16) & 0xffff;
                assert(c < 64);
                int s = (data & 0xffff);
                assert(c << 64);
                enable[c][s] = true;
                break;
            }
        }
        void send(int c, int s)
        {
            IPC_client& dst = clients[c];
            dst->set_status(c, s);
            if (dst.regs[CONFIG] & 0x80000000 == 0) {
                if (dst.enable(dst.reg[ID],sig]) {
                    irq[(dst.regs[ID] >> 16) & 0xff]->write(1);
                }
            }
            irq[c]->write(1);
        }
    } client[4][64];

protected:
    void b_transport(tlm::tlm_generic_payload& txn, sc_core::sc_time& delay)
    {
        unsigned int len = txn.get_data_length();
        unsigned char* ptr = txn.get_data_ptr();
        sc_dt::uint64 addr = txn.get_address();

        if ((addr + len) > m_size) {
            txn.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
            return;
        }
        assert(len == 4);

        int c = (addr / 0x1000) & 0x3f;
        int p = (addr / 0x40000) & 0x3;
        switch (txn.get_command()) {
        case tlm::TLM_READ_COMMAND:
            uint32_t data = clientp[p][c].read(addr & 0xfff);
            memcpy(ptr, &data, len);
            break;
        case tlm::TLM_WRITE_COMMAND:
            uint32_t data;
            memcpy(ptr, &data, len);
            client[p][c].write(addr & 0xfff, data);
            break;
        default:
            SC_REPORT_ERROR("IPPC", "TLM command not supported\n");
            break;
        }

        txn.set_response_status(tlm::TLM_OK_RESPONSE);

        update_irq();
    }

public:
    tlm_utils::simple_target_socket<Memory> socket;
    InitiatorSignalSocket<bool> irq[size_c];

    IPCC(sc_core::sc_module_name name, uint64_t size)
        : socket("socket")
    {
        for (int p = 0; p < 3; p++) {
            for (int c = 0; c < 64; c++) {
                client.data[ID] = i << 16 + p;
                client.clients = client[p]
            }
        }
        socket.register_b_transport(this, &Memory::b_transport);
    }

    void update_irq()
    {
        for (int c = 0; c < 64; c++) {
            int allirqcount = 0;
            for (int p = 0; p < 3; p++) {
                int irqcount = 0;
                int prio = 0;
                int sig = 0;
                int cli = 0;

                for (int sc = 0; sc < 64; sc++) {
                    for (int s = 0; s < 64; s++) {
                        if (clients[p][c].status(sc, s)) {
                            // there is an active signal s from p/sc to p/c
                            if (clients[p][c].regs[CONFIG] & 0x80000000 == 0) {
                                if (clients[p][c].enable(sc, s)) {
                                    int p = clients[p][c].regs[RECV_CLIENT_PRIORITY_n + sc];
                                    if (irqcount == 0 || p > prio) {
                                        prio = p;
                                        sig = s;
                                        cli = sc; 
                                    }
                                    irqcount++;
                                }
                            }
                        }
                    }
                }
                allirqcont += irqcont;
                if (irqcount)
                    clients[p][c].regs[RECV_ID] = (cli << 16) + sig;
            }
            if (allirqcont >= 1) {
                irq(c)->write(1);
//            } else {
//                irq(c)->write(0);
            }
        }
    }

    IPCC() = delete;
    IPCC(const IPCC&) = delete;

    ~IPCC()
    {
    }
};
