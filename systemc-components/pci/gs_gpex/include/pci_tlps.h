/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: jhieb@micron.com 2025
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef PCI_TLPS_H
#define PCI_TLPS_H


typedef union _TLPHeader3DW {
    uint8_t data[12];  // 3 DWORDs = 12 bytes
    uint32_t dwords[3]; // Access as DWORDs for byte swapping
    struct {
        // DW0 (Byte 0-3)
        struct {
            uint32_t length     : 10; // Bits 0-9: Length in DWs
            uint32_t at         : 2;  // Bits 10-11: Address Type (00 = untranslated)
            uint32_t attr       : 2;  // Bits 12-13: Attributes (Relaxed Ordering, No Snoop)
            uint32_t ep         : 1;  // Bit 14: Poisoned TLP
            uint32_t td         : 1;  // Bit 15: TLP Digest present
            uint32_t th         : 1;  // Bit 16: TLP Processing Hints
            uint32_t ln         : 1;  // Bit 17: Lightweight Notification
            uint32_t reserved1  : 2;  // Bits 18-19: Reserved
            uint32_t tc         : 3;  // Bits 20-22: Traffic Class
            uint32_t reserved2  : 1;  // Bit 23: Reserved
            uint32_t type       : 5;  // Bits 24-28: Type (00100b = CfgRd0, 00101b = CfgRd1)
            uint32_t fmt        : 2;  // Bits 29-30: Format (00b = 3DW, no data)
            uint32_t reserved3  : 1;  // Bit 31: Reserved
        } dword0;
        
        // DW1 (Byte 4-7)
        struct {
            uint32_t first_be   : 4;  // Bits 0-3: First DW Byte Enable
            uint32_t last_be    : 4;  // Bits 4-7: Last DW Byte Enable
            uint32_t tag        : 8;  // Bits 8-15: Tag
            uint32_t requester_id : 16; // Bits 16-31: Requester ID (Bus:Dev:Func)
        } dword1;
        
        // DW2 (Byte 8-11)
        struct {
            uint32_t reg_num    : 2;  // Bits 0-1: Register Number (must be 00b)
            uint32_t reg_offset : 10; // Bits 2-11: Extended Register Number (DWORD aligned)
            uint32_t reserved   : 4;  // Bits 12-15: Reserved
            uint32_t function   : 3;  // Bits 16-18: Function Number
            uint32_t device     : 5;  // Bits 19-23: Device Number
            uint32_t bus        : 8;  // Bits 24-31: Bus Number
        } dword2;
    };

    _TLPHeader3DW() { memset(data, 0, sizeof(data)); }

    // Helper to byte-swap to big-endian (network byte order)
    void to_big_endian() {
        dwords[0] = __builtin_bswap32(dwords[0]);
        dwords[1] = __builtin_bswap32(dwords[1]);
        dwords[2] = __builtin_bswap32(dwords[2]);
    }

} TLPHeader3DW;

typedef union _TLPCompletion3DW {
    uint8_t data[16];   // 3 DW header + 1 DW data = 16 bytes
    uint32_t dwords[4]; // Access as DWORDs for byte swapping
    struct {
        // DW0 (Byte 0-3)
        struct {
            uint32_t length     : 10; // Bits 0-9: Length in DWs (1 for single DWORD)
            uint32_t at         : 2;  // Bits 10-11: Address Type
            uint32_t attr       : 2;  // Bits 12-13: Attributes
            uint32_t ep         : 1;  // Bit 14: Poisoned completion
            uint32_t td         : 1;  // Bit 15: TLP Digest present
            uint32_t th         : 1;  // Bit 16: TLP Processing Hints
            uint32_t ln         : 1;  // Bit 17: Lightweight Notification
            uint32_t reserved1  : 2;  // Bits 18-19: Reserved
            uint32_t tc         : 3;  // Bits 20-22: Traffic Class
            uint32_t reserved2  : 1;  // Bit 23: Reserved
            uint32_t type       : 5;  // Bits 24-28: Type (01010b = Completion with Data)
            uint32_t fmt        : 2;  // Bits 29-30: Format (10b = 3DW header with data)
            uint32_t reserved3  : 1;  // Bit 31: Reserved
        } dword0;
        
        // DW1 (Byte 4-7)
        struct {
            uint32_t byte_count : 12; // Bits 0-11: Byte Count (remaining bytes, typically 4)
            uint32_t bcm        : 1;  // Bit 12: Byte Count Modified
            uint32_t status     : 3;  // Bits 13-15: Completion Status (000b = SC - Successful)
            uint32_t completer_id : 16; // Bits 16-31: Completer ID (Bus:Dev:Func)
        } dword1;
        
        // DW2 (Byte 8-11)
        struct {
            uint32_t lower_addr : 7;  // Bits 0-6: Lower Address (byte address within DWORD)
            uint32_t reserved   : 1;  // Bit 7: Reserved
            uint32_t tag        : 8;  // Bits 8-15: Tag (matches request tag)
            uint32_t requester_id : 16; // Bits 16-31: Requester ID (original requester)
        } dword2;
        
        // DW3 (Byte 12-15) - Data payload
        uint32_t data_payload;
    };

    _TLPCompletion3DW() { memset(data, 0, sizeof(data)); }

    // Helper to byte-swap from big-endian (network byte order)
    void from_big_endian() {
        dwords[0] = __builtin_bswap32(dwords[0]);
        dwords[1] = __builtin_bswap32(dwords[1]);
        dwords[2] = __builtin_bswap32(dwords[2]);
        // Not swapping payload - it's already in little-endian (PCI config data)
        // dwords[3] = __builtin_bswap32(dwords[3]);
    }

    // Helper to byte-swap to big-endian (network byte order)
    void to_big_endian() {
        dwords[0] = __builtin_bswap32(dwords[0]);
        dwords[1] = __builtin_bswap32(dwords[1]);
        dwords[2] = __builtin_bswap32(dwords[2]);
        // Not swapping payload - it's already in little-endian (PCI config data)
        // dwords[3] = __builtin_bswap32(dwords[3]);
    }

    // Helper to get PCI config data in correct byte order
    uint32_t get_pci_config_dword() const {
        // Swap the two 16-bit words in the payload
        return ((data_payload & 0xFFFF) << 16) | ((data_payload >> 16) & 0xFFFF);
    }

    // Completion Status values
    enum CompletionStatus {
        SC  = 0b000,  // Successful Completion
        UR  = 0b001,  // Unsupported Request
        CRS = 0b010,  // Configuration Request Retry Status
        CA  = 0b100   // Completer Abort
    };

} TLPCompletion3DW;

typedef union _TLPHeader4DW {
    uint8_t data[16];   // 4 DWORDs = 16 bytes (header + data)
    uint32_t dwords[4]; // Access as DWORDs for byte swapping
    struct {
        // DW0 (Byte 0-3)
        struct {
            uint32_t length     : 10; // Bits 0-9: Length in DWs
            uint32_t at         : 2;  // Bits 10-11: Address Type (00 = untranslated)
            uint32_t attr       : 2;  // Bits 12-13: Attributes (Relaxed Ordering, No Snoop)
            uint32_t ep         : 1;  // Bit 14: Poisoned TLP
            uint32_t td         : 1;  // Bit 15: TLP Digest present
            uint32_t th         : 1;  // Bit 16: TLP Processing Hints
            uint32_t ln         : 1;  // Bit 17: Lightweight Notification
            uint32_t reserved1  : 2;  // Bits 18-19: Reserved
            uint32_t tc         : 3;  // Bits 20-22: Traffic Class
            uint32_t reserved2  : 1;  // Bit 23: Reserved
            uint32_t type       : 5;  // Bits 24-28: Type (00100b = CfgWr0)
            uint32_t fmt        : 2;  // Bits 29-30: Format (10b = 3DW header with data)
            uint32_t reserved3  : 1;  // Bit 31: Reserved
        } dword0;
        
        // DW1 (Byte 4-7)
        struct {
            uint32_t first_be   : 4;  // Bits 0-3: First DW Byte Enable
            uint32_t last_be    : 4;  // Bits 4-7: Last DW Byte Enable
            uint32_t tag        : 8;  // Bits 8-15: Tag
            uint32_t requester_id : 16; // Bits 16-31: Requester ID (Bus:Dev:Func)
        } dword1;
        
        // DW2 (Byte 8-11)
        struct {
            uint32_t reg_num    : 2;  // Bits 0-1: Register Number (must be 00b)
            uint32_t reg_offset : 10; // Bits 2-11: Extended Register Number (DWORD aligned)
            uint32_t reserved   : 4;  // Bits 12-15: Reserved
            uint32_t function   : 3;  // Bits 16-18: Function Number
            uint32_t device     : 5;  // Bits 19-23: Device Number
            uint32_t bus        : 8;  // Bits 24-31: Bus Number
        } dword2;
        
        // DW3 (Byte 12-15) - Data payload
        uint32_t data_payload;
    };

    _TLPHeader4DW() { memset(data, 0, sizeof(data)); }

    // Helper to byte-swap to big-endian (network byte order)
    void to_big_endian() {
        dwords[0] = __builtin_bswap32(dwords[0]);
        dwords[1] = __builtin_bswap32(dwords[1]);
        dwords[2] = __builtin_bswap32(dwords[2]);
        // Not swapping payload - it's already in little-endian (PCI config data)
        // dwords[3] = __builtin_bswap32(dwords[3]); // Swap data payload too
    }

} TLPHeader4DW;

// 3DW Memory Write TLP (for 32-bit addresses with data)
typedef union _TLPMemoryWrite3DW {
    uint8_t data[12 + 4096];  // 3DW header + max payload
    uint32_t dwords[3 + 1024];
    struct {
        // DW0 (Byte 0-3)
        struct {
            uint32_t length     : 10; // Bits 0-9: Length in DWs
            uint32_t at         : 2;  // Bits 10-11: Address Type
            uint32_t attr       : 2;  // Bits 12-13: Attributes
            uint32_t ep         : 1;  // Bit 14: Poisoned TLP
            uint32_t td         : 1;  // Bit 15: TLP Digest present
            uint32_t th         : 1;  // Bit 16: TLP Processing Hints
            uint32_t ln         : 1;  // Bit 17: Lightweight Notification
            uint32_t reserved1  : 2;  // Bits 18-19: Reserved
            uint32_t tc         : 3;  // Bits 20-22: Traffic Class
            uint32_t reserved2  : 1;  // Bit 23: Reserved
            uint32_t type       : 5;  // Bits 24-28: Type (00000b = Memory Write)
            uint32_t fmt        : 2;  // Bits 29-30: Format (10b = 3DW with data)
            uint32_t reserved3  : 1;  // Bit 31: Reserved
        } dword0;
        
        // DW1 (Byte 4-7)
        struct {
            uint32_t first_be   : 4;  // Bits 0-3: First DW Byte Enable
            uint32_t last_be    : 4;  // Bits 4-7: Last DW Byte Enable
            uint32_t tag        : 8;  // Bits 8-15: Tag
            uint32_t requester_id : 16; // Bits 16-31: Requester ID
        } dword1;
        
        // DW2 (Byte 8-11)
        struct {
            uint32_t addr       : 32;  // 32-bit address (bits [31:2] valid, [1:0] = 00)
        } dword2;
        
        // Data payload (up to 4096 bytes)
        uint8_t data_payload[4096];
    };
    
    _TLPMemoryWrite3DW() { memset(data, 0, sizeof(data)); }
    
    void to_big_endian() {
        for (int i = 0; i < 3; i++) {
            dwords[i] = __builtin_bswap32(dwords[i]);
        }
        // Data payload stays in original byte order
    }
    void from_big_endian() {
        for (int i = 0; i < 3; i++) dwords[i] = __builtin_bswap32(dwords[i]);
        // Data payload stays in original byte order
    }
} TLPMemoryWrite3DW;

// 4DW Memory Write TLP (for 64-bit addresses with data)
typedef union _TLPMemoryWrite4DW {
    uint8_t data[16 + 4096];  // 4DW header + max payload
    uint32_t dwords[4 + 1024];
    struct {
        // DW0 (Byte 0-3)
        struct {
            uint32_t length     : 10; // Bits 0-9: Length in DWs
            uint32_t at         : 2;  // Bits 10-11: Address Type
            uint32_t attr       : 2;  // Bits 12-13: Attributes
            uint32_t ep         : 1;  // Bit 14: Poisoned TLP
            uint32_t td         : 1;  // Bit 15: TLP Digest present
            uint32_t th         : 1;  // Bit 16: TLP Processing Hints
            uint32_t ln         : 1;  // Bit 17: Lightweight Notification
            uint32_t reserved1  : 2;  // Bits 18-19: Reserved
            uint32_t tc         : 3;  // Bits 20-22: Traffic Class
            uint32_t reserved2  : 1;  // Bit 23: Reserved
            uint32_t type       : 5;  // Bits 24-28: Type (00000b = Memory Write)
            uint32_t fmt        : 2;  // Bits 29-30: Format (11b = 4DW with data)
            uint32_t reserved3  : 1;  // Bit 31: Reserved
        } dword0;
        
        // DW1 (Byte 4-7)
        struct {
            uint32_t first_be   : 4;  // Bits 0-3: First DW Byte Enable
            uint32_t last_be    : 4;  // Bits 4-7: Last DW Byte Enable
            uint32_t tag        : 8;  // Bits 8-15: Tag
            uint32_t requester_id : 16; // Bits 16-31: Requester ID
        } dword1;
        
        // DW2 (Byte 8-11)
        struct {
            uint32_t addr_high  : 32;  // Upper 32 bits of 64-bit address
        } dword2;
        
        // DW3 (Byte 12-15)
        struct {
            uint32_t addr_low   : 32;  // Lower 32 bits (bits [31:2] valid, [1:0] = 00)
        } dword3;
        
        // Data payload (up to 4096 bytes)
        uint8_t data_payload[4096];
    };
    
    _TLPMemoryWrite4DW() { memset(data, 0, sizeof(data)); }
    
    void to_big_endian() {
        for (int i = 0; i < 4; i++) {
            dwords[i] = __builtin_bswap32(dwords[i]);
        }
        // Data payload stays in original byte order
    }

    void from_big_endian() {
        for (int i = 0; i < 4; i++) dwords[i] = __builtin_bswap32(dwords[i]);
        // Data payload stays in original byte order
    }
} TLPMemoryWrite4DW;

// 3DW Memory Read TLP
typedef union _TLPMemoryRead3DW {
    uint8_t data[12];
    uint32_t dwords[3];
    struct {
        struct {
            uint32_t length     : 10;
            uint32_t at         : 2;
            uint32_t attr       : 2;
            uint32_t ep         : 1;
            uint32_t td         : 1;
            uint32_t th         : 1;
            uint32_t reserved1  : 3;
            uint32_t tc         : 3;
            uint32_t reserved2  : 1;
            uint32_t type       : 5;  // 00000b = Memory Read
            uint32_t fmt        : 2;  // 00b = 3DW, no data
            uint32_t reserved3  : 1;
        } dword0;
        
        struct {
            uint32_t first_be   : 4;
            uint32_t last_be    : 4;
            uint32_t tag        : 8;
            uint32_t requester_id : 16;
        } dword1;
        
        struct {
            uint32_t addr       : 32;  // Bits [31:2] valid, [1:0] = 00
        } dword2;
    };
    
    _TLPMemoryRead3DW() { memset(data, 0, sizeof(data)); }
    void to_big_endian() {
        for (int i = 0; i < 3; i++) dwords[i] = __builtin_bswap32(dwords[i]);
    }
    void from_big_endian() {
        for (int i = 0; i < 3; i++) dwords[i] = __builtin_bswap32(dwords[i]);
    }
} TLPMemoryRead3DW;

// 4DW Memory Read TLP (for 64-bit addresses)
typedef union _TLPMemoryRead4DW {
    uint8_t data[16];
    uint32_t dwords[4];
    struct {
        struct {
            uint32_t length     : 10;
            uint32_t at         : 2;
            uint32_t attr       : 2;
            uint32_t ep         : 1;
            uint32_t td         : 1;
            uint32_t th         : 1;
            uint32_t reserved1  : 3;
            uint32_t tc         : 3;
            uint32_t reserved2  : 1;
            uint32_t type       : 5;  // 00000b = Memory Read
            uint32_t fmt        : 2;  // 01b = 4DW, no data
            uint32_t reserved3  : 1;
        } dword0;
        
        struct {
            uint32_t first_be   : 4;
            uint32_t last_be    : 4;
            uint32_t tag        : 8;
            uint32_t requester_id : 16;
        } dword1;
        
        struct {
            uint32_t addr_high  : 32;  // Upper 32 bits of address
        } dword2;
        
        struct {
            uint32_t addr_low   : 32;  // Lower 32 bits, [1:0] = 00
        } dword3;
    };
    
    _TLPMemoryRead4DW() { memset(data, 0, sizeof(data)); }
    void to_big_endian() {
        for (int i = 0; i < 4; i++) dwords[i] = __builtin_bswap32(dwords[i]);
    }
    void from_big_endian() {
        for (int i = 0; i < 4; i++) dwords[i] = __builtin_bswap32(dwords[i]);
    }
} TLPMemoryRead4DW;

// Completion with Data TLP
typedef union _TLPCompletionWithData {
    uint8_t data[12 + 4096];  // Header + max payload
    uint32_t dwords[3 + 1024];
    struct {
        struct {
            uint32_t length     : 10;
            uint32_t at         : 2;
            uint32_t attr       : 2;
            uint32_t ep         : 1;
            uint32_t td         : 1;
            uint32_t th         : 1;
            uint32_t reserved1  : 3;
            uint32_t tc         : 3;
            uint32_t reserved2  : 1;
            uint32_t type       : 5;  // 01010b = Completion with Data
            uint32_t fmt        : 2;  // 10b = 3DW with data
            uint32_t reserved3  : 1;
        } dword0;
        
        struct {
            uint32_t byte_count : 12;
            uint32_t bcm        : 1;
            uint32_t status     : 3;
            uint32_t completer_id : 16;
        } dword1;
        
        struct {
            uint32_t lower_addr : 7;
            uint32_t reserved   : 1;
            uint32_t tag        : 8;
            uint32_t requester_id : 16;
        } dword2;
        
        uint8_t data_payload[4096];
    };
    
    enum CompletionStatus {
        SC = 0,  // Successful Completion
        UR = 1,  // Unsupported Request
        CRS = 2, // Configuration Request Retry Status
        CA = 4   // Completer Abort
    };
    
    _TLPCompletionWithData() { memset(data, 0, sizeof(data)); }
    void from_big_endian() {
        for (int i = 0; i < 3; i++) dwords[i] = __builtin_bswap32(dwords[i]);
        // Data payload stays in original byte order
    }
    void to_big_endian() {
        for (int i = 0; i < 3; i++) dwords[i] = __builtin_bswap32(dwords[i]);
        // Data payload stays in original byte order
    }
} TLPCompletionWithData;

#endif