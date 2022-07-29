/*
 * Register Definition API: field macros
 *
 * Copyright (c) 2016 Xilinx Inc.
 * Copyright (c) 2013 Peter Crosthwaite <peter.crosthwaite@xilinx.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef REGISTERFIELDS_H
#define REGISTERFIELDS_H

#include <cstdint>
#include <fstream>
#include "bitops.h"

/* Define constants for a 32 bit register */

/* This macro will define A_FOO, for the byte address of a register
 * as well as R_FOO for the uint32_t[] register number (A_FOO / 4).
 */
#define REG32(reg, addr)                                                  \
    enum { A_ ## reg = (addr) };                                          \
    enum { R_ ## reg = (addr) / 4 };

#define REG8(reg, addr)                                                   \
    enum { A_ ## reg = (addr) };                                          \
    enum { R_ ## reg = (addr) };

#define REG16(reg, addr)                                                  \
    enum { A_ ## reg = (addr) };                                          \
    enum { R_ ## reg = (addr) / 2 };

#define REG64(reg, addr)                                                  \
    enum { A_ ## reg = (addr) };                                          \
    enum { R_ ## reg = (addr) / 8 };

/* Define SHIFT, LENGTH and MASK constants for a field within a register */

/* This macro will define R_FOO_BAR_MASK, R_FOO_BAR_SHIFT and R_FOO_BAR_LENGTH
 * constants for field BAR in register FOO.
 */
#define FIELD(reg, field, shift, length)                                  \
    enum { R_ ## reg ## _ ## field ## _SHIFT = (shift)};                  \
    enum { R_ ## reg ## _ ## field ## _LENGTH = (length)};                \
    enum { R_ ## reg ## _ ## field ## _MASK =                             \
                                        MAKE_64BIT_MASK(shift, length)};

/* Extract a field from a register */
#define FIELD_EX8(storage, reg, field)                                    \
    extract8((storage), R_ ## reg ## _ ## field ## _SHIFT,                \
              R_ ## reg ## _ ## field ## _LENGTH)
#define FIELD_EX16(storage, reg, field)                                   \
    extract16((storage), R_ ## reg ## _ ## field ## _SHIFT,               \
              R_ ## reg ## _ ## field ## _LENGTH)
#define FIELD_EX32(storage, reg, field)                                   \
    extract32((storage), R_ ## reg ## _ ## field ## _SHIFT,               \
              R_ ## reg ## _ ## field ## _LENGTH)
#define FIELD_EX64(storage, reg, field)                                   \
    extract64((storage), R_ ## reg ## _ ## field ## _SHIFT,               \
              R_ ## reg ## _ ## field ## _LENGTH)

/* Extract a field from an array of registers */
#define ARRAY_FIELD_EX32(regs, reg, field)                                \
    FIELD_EX32((regs)[R_ ## reg], reg, field)
#define ARRAY_FIELD_EX64(regs, reg, field)                                \
    FIELD_EX64((regs)[R_ ## reg], reg, field)

/* Deposit a register field.
 * Assigning values larger then the target field will result in
 * compilation warnings.
 */
#define FIELD_DP8(storage, reg, field, val) ({                            \
    struct {                                                              \
        unsigned int v:R_ ## reg ## _ ## field ## _LENGTH;                \
    } _v = { .v = val };                                                  \
    uint8_t _d;                                                           \
    _d = deposit32((storage), R_ ## reg ## _ ## field ## _SHIFT,          \
                  R_ ## reg ## _ ## field ## _LENGTH, _v.v);              \
    _d; })
#define FIELD_DP16(storage, reg, field, val) ({                           \
    struct {                                                              \
        unsigned int v:R_ ## reg ## _ ## field ## _LENGTH;                \
    } _v = { .v = val };                                                  \
    uint16_t _d;                                                          \
    _d = deposit32((storage), R_ ## reg ## _ ## field ## _SHIFT,          \
                  R_ ## reg ## _ ## field ## _LENGTH, _v.v);              \
    _d; })
#define FIELD_DP32(storage, reg, field, val) ({                           \
    struct {                                                              \
        unsigned int v:R_ ## reg ## _ ## field ## _LENGTH;                \
    } _v = { .v = val };                                                  \
    uint32_t _d;                                                          \
    _d = deposit32((storage), R_ ## reg ## _ ## field ## _SHIFT,          \
                  R_ ## reg ## _ ## field ## _LENGTH, _v.v);              \
    _d; })
#define FIELD_DP64(storage, reg, field, val) ({                           \
    struct {                                                              \
        uint64_t v:R_ ## reg ## _ ## field ## _LENGTH;                \
    } _v = { .v = val };                                                  \
    uint64_t _d;                                                          \
    _d = deposit64((storage), R_ ## reg ## _ ## field ## _SHIFT,          \
                  R_ ## reg ## _ ## field ## _LENGTH, _v.v);              \
    _d; })

/* Deposit a field to array of registers.  */
#define ARRAY_FIELD_DP32(regs, reg, field, val)                           \
    (regs)[R_ ## reg] = FIELD_DP32((regs)[R_ ## reg], reg, field, val);
#define ARRAY_FIELD_DP64(regs, reg, field, val)                           \
    (regs)[R_ ## reg] = FIELD_DP64((regs)[R_ ## reg], reg, field, val);

#endif

/**
 * Access description for a register that is part of guest accessible device
 * state.
 *
 * @name: String name of the register
 * @ro: whether or not the bit is read-only
 * @w1c: bits with the common write 1 to clear semantic.
 * @reset: reset value.
 * @cor: Bits that are clear on read
 * @rsvd: Bits that are reserved and should not be changed
 *
 * @pre_write: Pre write callback. Passed the value that's to be written,
 * immediately before the actual write. The returned value is what is written,
 * giving the handler a chance to modify the written value.
 * @post_write: Post write callback. Passed the written value. Most write side
 * effects should be implemented here. This is called during device reset.
 *
 * @post_read: Post read callback. Passes the value that is about to be returned
 * for a read. The return value from this function is what is ultimately read,
 * allowing this function to modify the value before return to the client.
 */

struct RegisterAccessInfo {
    const char *name;
    uint64_t addr;
    uint64_t w1c;
    uint64_t reset;
    uint64_t ro;
    uint64_t rsvd;
    uint64_t unimp;


    std::function<void(tlm::tlm_generic_payload& txn, sc_core::sc_time& delay)> pre_write;
    std::function<void(tlm::tlm_generic_payload& txn, sc_core::sc_time& delay)> post_write;
    std::function<void(tlm::tlm_generic_payload& txn, sc_core::sc_time& delay)> post_read;
//    void (*pre_write)(tlm::tlm_generic_payload& txn, sc_core::sc_time& delay);
//    void (*post_write)(tlm::tlm_generic_payload& txn, sc_core::sc_time& delay);
//    void (*post_read)(tlm::tlm_generic_payload& txn, sc_core::sc_time& delay);

};

