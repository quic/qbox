/*
 * This models the QDMA.
 *
 * Copyright (c) 2022 Xilinx Inc.
 * Written by Fred Konrad
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef __PCI_QDMA_H__
#define __PCI_QDMA_H__

/* Generate the different structures for the contexts.  */
#define QDMA_CPM4
#include "qdma-ctx.inc"
#undef QDMA_CPM4
#include "qdma-ctx.inc"

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "def.h"
#include "queue.h"
#include "namespace.h"
#include "systemc.h"
#include "tlm.h"
#include "pci-device-base.h"

#define NR_QDMA_IRQ      16

/*
 * IP configuration:
 *
 * QDMA_SOFT_IP: The IP is a soft IP.
 */
//#define QDMA_SOFT_IP

#ifdef QDMA_SOFT_IP
#define QDMA_DEVICE_ID 0
/*
 * 0: QDMA
 * 1: EQDMA
 */
#define QDMA_VERSAL_IP 0
/*
 * For QDMA:
 *   0: 2018_3
 *   1: 2019_1
 *   2: 2019_2
 *
 * For EQDMA:
 *   0: 2020_1
 *   1: 2020_2
 */
#define QDMA_VIVADO_REL 1
#define QDMA_HAS_MAILBOX ((QDMA_VERSAL_IP != 0) || (QDMA_VIVADO_REL != 0))
#else
/*
 * DEVICE_ID:
 *   0: Soft Device.
 *   1: Versal CPM4.
 *   2: Versal CPM5.
 */
#define QDMA_SOFT_DEVICE_ID 0
#define QDMA_CPM4_DEVICE_ID 1
#define QDMA_CPM5_DEVICE_ID 2

/*
 * QDMA_VERSAL_IP:
 *   0: Versal Hard IP.
 *   1: Versal Soft IP.
 */
#define QDMA_VERSAL_IP 0

/*
 * For CPM4:
 *   0: 2019_2
 * For CPM5:
 *   0: 2021_1
 *   1: 2022_1
 */
#define QDMA_VIVADO_REL 1

/* No mailboxes for the CPM4 IP. */
#define QDMA_HAS_MAILBOX (DEVICE_ID != 1)
#endif

/*
 * Config of the QDMA_GLBL2_MISC_CAP register @0x134.
 * Device ID:   31 .. 28
 * Vivado Rel:  27 .. 24
 * Versal IP:   23 .. 20
 * RTL Version: 19 .. 16
 */
#define QDMA_VERSION_FIELD(devid, vivado, versalip) ((devid << 28 )     \
						     + (vivado << 24)	\
						     + (versalip << 20))

/* Number of queue, as described in the QDMA_GLBL2_CHANNEL_QDMA_CAP register
   @0x120.  */
#define QDMA_QUEUE_COUNT 2

#define QDMA_INTR_RING_ENTRY_ADDR(x) (x << 12)
#define QDMA_INTR_RING_ENTRY_SZ 8

/* Context implementation: actually there are less than 15 contexts since the
   fifth one is reserved.  */
#define QDMA_MAX_CONTEXT_SELECTOR 15
#define QDMA_U32_PER_CONTEXT 8

/* This is the bar ID for the user bar.  */
#define QDMA_USER_BAR_ID 2

/* Max size for descriptors in bytes.  */
#define QDMA_DESC_MAX_SIZE 64

/* PCIE Physical Function register offsets.  */
#define R_CONFIG_BLOCK_IDENT       (0x0 >> 2)
#define R_GLBL2_PF_BARLITE_INT     (0x104 >> 2)
#define R_GLBL2_PF_BARLITE_EXT     (0x10C >> 2)
#define R_GLBL2_CHANNEL_MDMA       (0x118 >> 2)
#define R_GLBL2_CHANNEL_QDMA_CAP   (0x120 >> 2)
#define R_GLBL2_CHANNEL_FUNC_RET   (0x12C >> 2)
#define R_GLBL2_MISC_CAP           (0x134 >> 2)
#define R_GLBL_INTR_CFG            (0x2c4 >> 2)
#define R_GLBL_INTR_CFG_INT_PEND   (1UL << 1)
#define R_GLBL_INTR_CFG_INT_EN     (1UL << 0)
#define R_IND_CTXT_DATA            (0x804 >> 2)
/* Those registers are not at the same place for CPM4.  */
#define R_IND_CTXT_CMD(v)             ((v ? 0x824 : 0x844) >> 2)
#define R_DMAP_SEL_INT_CIDX(v, n)     (((v ? 0x6400 : 0x18000) \
					+ (0x10 * n)) >> 2)
#define R_DMAP_SEL_H2C_DSC_PIDX(v, n) (((v ? 0x6404 : 0x18004) \
					+ (0x10 * n)) >> 2)
#define R_DMAP_SEL_C2H_DSC_PIDX(v, n) (((v ? 0x6408 : 0x18008) \
					+ (0x10 * n)) >> 2)
#define R_DMAP_SEL_CMPT_CIDX(v, n)    (((v ? 0x640C : 0x1800C) \
					+ (0x10 * n)) >> 2)

/* About MSIX Vector mapping:
 *
 * For Versal HARD IP:
 * 0 - User Vector (1 but # might be configurable)
 * 1 - Error Vector
 * 2 - Data Vector
 *
 * For SOFT IP:
 * 0 - MailBox Vector (for IP >= 2019_1 only)
 * 1 - User Vector (1 but # might be configurable)
 * 2 - Error Vector
 * 3 - Data Vector
 */
#if QDMA_HAS_MAILBOX
#define QDMA_PF0_FIRST_DATA_MSIX_VECTOR (3)
#else
#define QDMA_PF0_FIRST_DATA_MSIX_VECTOR (2)
#endif

// NVMe registers
typedef union _RegisterTable {
  uint8_t data[64];
  struct {
    uint64_t capabilities;
    uint32_t version;
    uint32_t interruptMaskSet;
    uint32_t interruptMaskClear;
    uint32_t configuration;
    uint32_t reserved;
    uint32_t status; // TODO(jhieb) do i need this?
    uint32_t subsystemReset;
    uint32_t adminQueueAttributes;
    uint64_t adminSQueueBaseAddress;
    uint64_t adminCQueueBaseAddress;
    uint32_t memoryBufferLocation;
    uint32_t memoryBufferSize;
  };

  _RegisterTable(){
  	// Optional: zero-initialize the union
  	memset(data, 0, sizeof(data));
  }
} RegisterTable;

typedef struct {
  uint64_t nextTime;
  uint32_t requestCount;
  bool valid;
  bool pending;
} AggregationInfo;

const uint32_t nLBAFormat = 4;
const uint32_t lbaFormat[nLBAFormat] = {
    0x02090000,  // 512B + 0, Good performance
    0x020A0000,  // 1KB + 0, Good performance
    0x010B0000,  // 2KB + 0, Better performance
    0x000C0000,  // 4KB + 0, Best performance
};
const uint32_t lbaSize[nLBAFormat] = {
    512,   // 512B
    1024,  // 1KB
    2048,  // 2KB
    4096,  // 4KB
};

typedef struct {
  uint32_t channel;          //!< Total # channels
  uint32_t package;          //!< # packages / channel
  uint32_t die;              //!< # dies / package
  uint32_t plane;            //!< # planes / die
  uint32_t block;            //!< # blocks / plane
  uint32_t page;             //!< # pages / block
  uint32_t superBlock;       //!< Total super blocks
  uint32_t pageSize;         //!< Size of page in bytes
  uint32_t superPageSize;    //!< Size of super page in bytes
  uint32_t pageInSuperPage;  //!< # pages in one superpage
} NANDParameter;

typedef struct {
  uint64_t totalPhysicalBlocks;  //!< (PAL::Parameter::superBlock)
  uint64_t totalLogicalBlocks;
  uint64_t pagesInBlock;  //!< (PAL::Parameter::page)
  uint32_t pageSize;      //!< Mapping unit (PAL::Parameter::superPageSize)
  uint32_t ioUnitInPage;  //!< # smallest I/O unit in one page
  uint32_t pageCountToMaxPerf;  //!< # pages to fully utilize internal parallism
} FTLParameter;

template<typename SW_CTX, typename INTR_CTX, typename INTR_RING_ENTRY>
class qdma : public sc_module
{
private:

	// Manual config items for now.
  	uint32_t cqsize = 16;
  	uint32_t sqsize = 16;
	uint16_t nNamespaces = 1;
	uint32_t lba = 512;
	NANDParameter nandParameters;
	FTLParameter ftlParameter;
	uint64_t totalLogicalPages;
	uint32_t logicalPageSize;
	double overProvisioningRatio = 0.25;
	
	// TODO(jhieb) stuff to move out eventually.
	/* Nvme variables */
	RegisterTable registers;   //!< Table for NVMe Controller Registers
  	uint64_t sqstride;         //!< Calculated SQ Stride
  	uint64_t cqstride;         //!< Calculated CQ stride
  	uint8_t adminQueueInited;  //!< Flag for initialization of Admin CQ/SQ
  	uint16_t arbitration;      //!< Selected Arbitration Mechanism
  	uint32_t interruptMask;    //!< Variable to store current interrupt mask
	uint64_t commandCount;
	ConfigData cfgdata;
	bool shutdownReserved;
	uint64_t registerTableBaseAddress;
	int registerTableSize;
	uint64_t aggregationTime;
  	uint32_t aggregationThreshold;
	std::unordered_map<uint16_t, AggregationInfo> aggregationMap;
	uint32_t queueAllocated;
	uint64_t allocatedLogicalPages;

  	CQueue **ppCQueue;  //!< Completion Queue array
  	SQueue **ppSQueue;  //!< Submission Queue array
  	std::list<SQEntryWrapper> lSQFIFO;  //!< Internal FIFO queue for submission
  	std::list<CQEntryWrapper> lCQFIFO;  //!< Internal FIFO queue for completion
	std::list<Namespace *> lNamespaces;

	/* Context handling.  */

	/* The per queue HW contexts.  */
	struct __attribute__((__packed__)) hw_ctx {
		/* CIDX of the last fetched descriptor.	 */
		uint16_t hw_cidx;
		/* Credit consumed.  */
		uint16_t credit_used;
		uint8_t rsvd;
		/* CIDX != PIDX.  */
		uint8_t desc_pending : 1;
		uint8_t invalid_desc : 1;
		uint8_t event_pending : 1;
		uint8_t desc_fetch_pending : 4;
		uint8_t rsvd2 : 1;
	};

	/* Register area for the contexts described above.  */
	struct {
		uint32_t data[QDMA_U32_PER_CONTEXT];
	} queue_contexts[QDMA_QUEUE_COUNT][QDMA_MAX_CONTEXT_SELECTOR];

	/* MSI-X handling.  */
	enum msix_status { QDMA_MSIX_LOW = 0, QDMA_MSIX_HIGH }
		msix_status[NR_QDMA_IRQ];
	sc_event msix_trig[NR_QDMA_IRQ];

	void msix_strobe(unsigned int msix_id)
	{
		sc_event *msix_trig;

		/* Sanity check on the number of queue.	 */
		if (!(msix_id < NR_QDMA_IRQ)) {
			SC_REPORT_ERROR("qdma", "invalid MSIX ID");
		}

		/* Each queue has it's own event to be triggered.  */
		msix_trig = &this->msix_trig[msix_id];
		while (1) {
			/* Waiting for an MSIX to be triggered.	 */
			wait(*msix_trig);
			this->irq[msix_id].write(true);
			wait(10, SC_NS);
			this->irq[msix_id].write(false);
		}
	}

	/* Context commands.  */
	enum {
		QDMA_CTXT_CMD_CLR = 0,
		QDMA_CTXT_CMD_WR = 1,
		QDMA_CTXT_CMD_RD = 2,
		QDMA_CTXT_CMD_INV = 3
	};

	/* The contexts are indirectly accessed by the driver, also they are
	 * slightly version dependent (CPM4 vs CPM5).  */
	enum {
		QDMA_CTXT_SELC_DEC_SW_C2H = 0,
		QDMA_CTXT_SELC_DEC_SW_H2C = 1,
		QDMA_CTXT_SELC_DEC_HW_C2H = 2,
		QDMA_CTXT_SELC_DEC_HW_H2C = 3,
		QDMA_CTXT_SELC_DEC_CR_C2H = 4,
		QDMA_CTXT_SELC_DEC_CR_H2C = 5,
		/* Write Back, also called completion queue.  */
		QDMA_CTXT_SELC_WRB = 6,
		QDMA_CTXT_SELC_PFTCH = 7,
		/* Interrupt context.  */
		QDMA_CTXT_SELC_INT_COAL = 8,
		QDMA_CTXT_SELC_HOST_PROFILE = 0xA,
		QDMA_CTXT_SELC_TIMER = 0xB,
		QDMA_CTXT_SELC_FMAP_QID2VEC = 0xC,
		QDMA_CTXT_SELC_FNC_STS = 0xD,
	};

	void handle_ctxt_cmd(uint32_t reg) {
		uint32_t qid = (reg >> 7) & 0x1FFF;
		uint32_t cmd = (reg >> 5) & 0x3;
		uint32_t sel = (reg >> 1) & 0xF;
		uint32_t *data;

		if (sel == QDMA_CTXT_SELC_INT_COAL) {
			/* This one requires some special treatment.  */
			this->handle_irq_ctxt_cmd(qid, cmd);
			return;
		}

		/* Find the context data.  */
		assert(qid < QDMA_QUEUE_COUNT);
		switch (sel) {
			case QDMA_CTXT_SELC_FMAP_QID2VEC:
			case QDMA_CTXT_SELC_PFTCH:
			case QDMA_CTXT_SELC_WRB:
			case QDMA_CTXT_SELC_DEC_CR_H2C:
			case QDMA_CTXT_SELC_DEC_CR_C2H:
			case QDMA_CTXT_SELC_DEC_HW_H2C:
			case QDMA_CTXT_SELC_DEC_HW_C2H:
			case QDMA_CTXT_SELC_DEC_SW_H2C:
			case QDMA_CTXT_SELC_DEC_SW_C2H:
				data = this->queue_contexts[qid][sel].data;
				break;
			default:
				SC_REPORT_ERROR("qdma", "Unsupported selector");
				return;
			case QDMA_CTXT_SELC_INT_COAL:
				/* Handled elsewere.  */
				abort();
				return;
		}

		switch (cmd) {
			case QDMA_CTXT_CMD_CLR:
				memset(data, 0, QDMA_U32_PER_CONTEXT * 4);
				break;
			case QDMA_CTXT_CMD_WR:
				memcpy(data, &this->regs.u32[R_IND_CTXT_DATA],
						QDMA_U32_PER_CONTEXT * 4);
				break;
			case QDMA_CTXT_CMD_RD:
				memcpy(&this->regs.u32[R_IND_CTXT_DATA], data,
						QDMA_U32_PER_CONTEXT * 4);
				break;
			case QDMA_CTXT_CMD_INV:
				break;
			default:
				SC_REPORT_ERROR("qdma", "Unsupported command");
				break;
		}
	}

	/* This one deserves a special treatment, because it has some side
	 * effects.  */
	void handle_irq_ctxt_cmd(uint32_t ring_idx, uint32_t cmd) {
		INTR_CTX *intr_ctx =
			(INTR_CTX *)this->queue_contexts[ring_idx]
				[QDMA_CTXT_SELC_INT_COAL].data;

		switch (cmd) {
			case QDMA_CTXT_CMD_CLR:
				memset(intr_ctx, 0, QDMA_U32_PER_CONTEXT * 4);
				break;
			case QDMA_CTXT_CMD_RD:
				memcpy(&this->regs.u32[R_IND_CTXT_DATA],
						intr_ctx,
						QDMA_U32_PER_CONTEXT * 4);
				break;
			case QDMA_CTXT_CMD_WR:
				{
					bool valid = intr_ctx->valid;

					memcpy(intr_ctx,
						&this->regs.u32[R_IND_CTXT_DATA],
						QDMA_U32_PER_CONTEXT * 4);
					if (intr_ctx->valid && !valid) {
						/* Interrupt context validated,
						 * reset the ring index.  */
						this->irq_ring_entry_idx
						  [ring_idx] = 0;
					}
				}
				break;
			case QDMA_CTXT_CMD_INV:
				/* Drop the valid bit.	*/
				intr_ctx->valid = 0;
				break;
			default:
				break;
		}
	}

	/* Descriptors: for h2c and c2h memory mapped transfer.	 */
	struct x2c_mm_descriptor {
		uint64_t src_address : 64;
		uint64_t byte_count : 28;
		uint64_t rsvd0 : 36;
		uint64_t dst_address : 64;
		uint64_t rsvd1 : 64;
	};

	/* Status descriptor, written by the DMA at the end of the transfer.  */
	struct x2c_wb_descriptor {
		/* 0 No errors, 1: DMA error, 2: Descriptor fetch error.  */
		uint16_t err : 2;
		uint16_t rsvd0 : 14;
		uint16_t cidx : 16;
		uint16_t pidx : 16;
		uint16_t rsvd1 : 16;
	};

	/* Transfer data from the Host 2 the Card (h2c = true),
	   Card 2 Host (h2c = false).  */
	int do_mm_dma(uint64_t src_addr, uint64_t dst_addr, uint64_t size,
			bool h2c)
	{
		uint64_t i;
		sc_time delay(SC_ZERO_TIME);
		struct {
			tlm::tlm_generic_payload trans;
			const char *name;
		} trans_ext[2];
		uint32_t data;

		trans_ext[0].trans.set_command(tlm::TLM_READ_COMMAND);
		trans_ext[0].trans.set_data_ptr((unsigned char *)&data);
		trans_ext[0].trans.set_streaming_width(4);
		trans_ext[0].trans.set_data_length(4);

		trans_ext[1].trans.set_command(tlm::TLM_WRITE_COMMAND);
		trans_ext[1].trans.set_data_ptr((unsigned char *)&data);
		trans_ext[1].trans.set_streaming_width(4);
		trans_ext[1].trans.set_data_length(4);

		for (i = 0; i < size; i+=4) {
			trans_ext[0].trans.set_address(src_addr);
			trans_ext[1].trans.set_address(dst_addr);
			src_addr += 4;
			dst_addr += 4;

			if (h2c) {
				this->dma->b_transport(
					trans_ext[0].trans, delay);
			} else {
				this->card_bus->b_transport(
					trans_ext[0].trans, delay);
			}

			if (trans_ext[0].trans.get_response_status() !=
				tlm::TLM_OK_RESPONSE) {
				SC_REPORT_ERROR("qdma",
					"error while fetching the data");
				return -1;
			}

			if (h2c) {
				this->card_bus->b_transport(
					trans_ext[1].trans, delay);
			} else {
				this->dma->b_transport(
					trans_ext[1].trans, delay);
			}

			if (trans_ext[1].trans.get_response_status() !=
				tlm::TLM_OK_RESPONSE) {
				SC_REPORT_ERROR("qdma",
					"error while pushing the data");
				return -1;
			}
		}

		return 0;
	}

	/* The driver wrote the @pidx in the update register of the given qid.
	   Handle the request.	*/
	void run_mm_dma(int16_t qid, bool h2c)
	{
		SW_CTX *sw_ctx;
		struct hw_ctx *hw_ctx;
		uint16_t pidx;
		uint8_t desc[QDMA_DESC_MAX_SIZE];
		int desc_size;
		uint32_t ring_sizes[16] = {
			2048, 64, 128, 192, 256, 384, 512, 768, 1024,
			1536, 3072, 4096, 6144, 8192, 12288, 16384 };
		uint32_t ring_size;
		struct x2c_mm_descriptor *pdesc =
				(struct x2c_mm_descriptor *)desc;
		struct x2c_wb_descriptor *pstatus =
				(struct x2c_wb_descriptor *) desc;

		if (qid > QDMA_QUEUE_COUNT) {
			SC_REPORT_ERROR("qdma", "invalid queue ID");
			return;
		}

		/* Compute some useful information from the context.  */
		sw_ctx = this->get_software_context(qid, h2c);
		hw_ctx =
			(struct hw_ctx *)this->queue_contexts[qid]
			[h2c ? QDMA_CTXT_SELC_DEC_HW_H2C :
				QDMA_CTXT_SELC_DEC_HW_C2H].data;
		pidx = this->regs.u32
		  [h2c ? R_DMAP_SEL_H2C_DSC_PIDX(this->is_cpm4(), qid) :
		   R_DMAP_SEL_C2H_DSC_PIDX(this->is_cpm4(), qid)] & 0xffff;
		desc_size = 8 << sw_ctx->desc_size;
		ring_size = ring_sizes[sw_ctx->ring_size];

		sw_ctx->pidx = pidx;

		/* Check that the producer index is in the descriptor ring, and
		   isn't pointing to the status descriptor.  */
		if (sw_ctx->pidx >= ring_size) {
			SC_REPORT_ERROR("qdma", "Producer index outside the "
					"descriptor ring.");
		}

		/* Running through the remaining descriptors from CIDX to
		 * PIDX.  Wrap around if needed */
		while (sw_ctx->pidx != hw_ctx->hw_cidx) {
			this->fetch_descriptor(
					((uint64_t)sw_ctx->desc_base_high << 32)
					+ sw_ctx->desc_base_low
					+ desc_size * hw_ctx->hw_cidx,
					desc_size, desc);

			this->do_mm_dma(pdesc->src_address, pdesc->dst_address,
					pdesc->byte_count, h2c);

			/* Descriptor is processed, go to the next one.  This
			   might warp around the descriptor ring, also skip the
			   last descriptor which is the status descriptor.  */
			hw_ctx->hw_cidx = hw_ctx->hw_cidx == ring_size - 1 ?
				0 : hw_ctx->hw_cidx + 1;

			/* Sending MSIX and / or writing back status descriptor
			   doesn't make sense at this point since the simulator
			   won't notice.  Do it once for all when the queue
			   finishes its work to gain performance.  */
			if (sw_ctx->pidx != hw_ctx->hw_cidx) {
				continue;
			}

			/* Update the status, and write the descriptor back. */
			if (sw_ctx->writeback_en) {
				/* Fetch the last descriptor, put the status in
				 * it, and write it back.  */
				this->fetch_descriptor(
					((uint64_t)sw_ctx->desc_base_high << 32)
					+ sw_ctx->desc_base_low
					+ desc_size * ring_size,
					desc_size,
					desc);

				pstatus->err = 0;
				pstatus->cidx = hw_ctx->hw_cidx;
				pstatus->pidx = pidx;
				this->descriptor_writeback(
					((uint64_t)sw_ctx->desc_base_high << 32)
					+ sw_ctx->desc_base_low
					+ desc_size * ring_size,
					desc_size,
					desc);
			}

			/* Trigger an IRQ?  */
			if ((!sw_ctx->irq_arm) || (!sw_ctx->irq_enabled)) {
				/* The software is polling for the completion.
				 * Just get out. */
				continue;
			}

			if (this->irq_aggregation_enabled(qid, h2c)) {
				INTR_CTX *intr_ctx;
				INTR_RING_ENTRY entry;
				int ring_idx = this->get_vec(qid, h2c);

				/* Each queue has a programmable irq ring
				 * associated to it.  */
				intr_ctx =
				  (INTR_CTX *)this->queue_contexts
				      [ring_idx]
				      [QDMA_CTXT_SELC_INT_COAL].data;

				/* Update the PIDX in the Interrupt Context
				 * Structure.  */
				intr_ctx->pidx = pidx;

				if (!intr_ctx->valid) {
					SC_REPORT_ERROR("qdma",
						"invalid interrupt context");
					return;
				}

				/* Now the controller needs to populate the IRQ
				 * ring.  */
				entry.qid = qid;
				entry.interrupt_type = h2c ? 0 : 1;
				entry.coal_color = intr_ctx->color;
				entry.error = 0;
				entry.interrupt_state = 0;
				entry.color = entry.coal_color;
				entry.cidx = hw_ctx->hw_cidx;
				entry.pidx = intr_ctx->pidx;

				/* Write it to the buffer.  */
				this->write_irq_ring_entry(ring_idx, &entry);

				/* Send the MSI-X associated to the ring.  */
				this->msix_trig[intr_ctx->vector].notify();
			} else {
				/* Direct interrupt: legacy or MSI-X.  */
				/* Pends an IRQ for the driver.	 */
#ifdef QDMA_SOFT_IP
				this->regs.u32[R_GLBL_INTR_CFG] |=
					R_GLBL_INTR_CFG_INT_PEND;
				this->update_legacy_irq();
#endif
				/* Send the MSI-X.  */
				this->msix_trig[get_vec(qid, h2c)].notify();
			}
		}
	}

	/* Update the IRQ.  */
	void update_legacy_irq(void)
	{
#ifndef QDMA_SOFT_IP
		return;
#endif

		bool irq_on;

		if (this->regs.u32[R_GLBL_INTR_CFG] & R_GLBL_INTR_CFG_INT_EN) {
			/* Yes, so consider sending a legacy IRQ not an MSI-X.
			 */
			irq_on = this->regs.u32[R_GLBL_INTR_CFG] &
					R_GLBL_INTR_CFG_INT_PEND;

			this->irq[0].write(!!irq_on);
		}
	}

	/* Descriptors.	 */
	void fetch_descriptor(uint64_t addr, uint8_t size, uint8_t *data) {
		sc_time delay(SC_ZERO_TIME);
		tlm::tlm_generic_payload trans;

		/* Do only 4bytes transactions.	 */
		trans.set_command(tlm::TLM_READ_COMMAND);
		trans.set_data_length(4);
		trans.set_streaming_width(4);

		for (int i = 0; i < size; i += 4) {
			trans.set_address(addr + i);
			trans.set_data_ptr(data + i);
			this->dma->b_transport(trans, delay);
			if (trans.get_response_status() !=
				tlm::TLM_OK_RESPONSE) {
				goto err;
			}
		}

		return;
err:
		SC_REPORT_ERROR("qdma", "error fetching the descriptor");
	}

	void descriptor_writeback(uint64_t addr, uint8_t size, uint8_t *data) {
		sc_time delay(SC_ZERO_TIME);
		tlm::tlm_generic_payload trans;

		trans.set_command(tlm::TLM_WRITE_COMMAND);
		trans.set_address(addr);
		trans.set_data_ptr(data);
		trans.set_data_length(size);
		trans.set_streaming_width(size);

		this->dma->b_transport(trans, delay);

		if (trans.get_response_status() != tlm::TLM_OK_RESPONSE) {
			SC_REPORT_ERROR("qdma",
					"error writing back the descriptor");
		}
	}

	/* Write the IRQ ring entry and increment the ring pointer and the
	 * color in case of a warp arround.  NOTE: The IRQ context is not queue
	 * specific, but rather each queue has a ring index which is selecting
	 * the interrupt context.  In the CPM4 flavour it's defined in the
	 * qid2vec table.  */
	void write_irq_ring_entry(uint32_t ring_idx,
				  const INTR_RING_ENTRY *entry) {
		sc_time delay(SC_ZERO_TIME);
		tlm::tlm_generic_payload trans;
		uint64_t addr;
		INTR_CTX *intr_ctx =
			(INTR_CTX *)this->queue_contexts[ring_idx]
			[QDMA_CTXT_SELC_INT_COAL].data;

		/* Compute the address of the entry.  */
		addr = QDMA_INTR_RING_ENTRY_ADDR(intr_ctx->baddr);
		addr += QDMA_INTR_RING_ENTRY_SZ
		  * this->irq_ring_entry_idx[ring_idx];

		trans.set_command(tlm::TLM_WRITE_COMMAND);
		trans.set_address(addr);
		trans.set_data_ptr((unsigned char *)entry);
		trans.set_data_length(QDMA_INTR_RING_ENTRY_SZ);
		trans.set_streaming_width(QDMA_INTR_RING_ENTRY_SZ);

		this->dma->b_transport(trans, delay);

		if (trans.get_response_status() != tlm::TLM_OK_RESPONSE) {
			SC_REPORT_ERROR("qdma",
					"error writing to the IRQ ring");
		}

		/* Now that the entry is written increment the counter, if
		 * there is a wrap around, invert the color, so the driver
		 * doesn't risk to miss / overwrite data.  */
		this->irq_ring_entry_idx[ring_idx]++;
		if (this->irq_ring_entry_idx[ring_idx] * QDMA_INTR_RING_ENTRY_SZ
		    == (1 + intr_ctx->page_size) * 4096) {
			this->irq_ring_entry_idx[ring_idx] = 0;
			intr_ctx->color = intr_ctx->color ? 0 : 1;
		}
	}

	void axi_master_light_bar_b_transport(tlm::tlm_generic_payload &trans,
					      sc_time &delay) {
		tlm::tlm_command cmd = trans.get_command();
		sc_dt::uint64 addr = trans.get_address();
		unsigned char *data = trans.get_data_ptr();
		unsigned int len = trans.get_data_length();
		unsigned char *byte_en = trans.get_byte_enable_ptr();
		unsigned int s_width = trans.get_streaming_width();
		uint32_t v = 0;

		if (byte_en || len > 4 || s_width < len) {
			goto err;
		}

		if (cmd == tlm::TLM_READ_COMMAND) {
			switch (addr >> 2) {
				default:
					v = this->axi_regs.u32[addr >> 2];
					break;
			}
			memcpy(data, &v, len);
		} else if (cmd == tlm::TLM_WRITE_COMMAND) {
			memcpy(&v, data, len);
			switch (addr >> 2) {
				default:
					this->axi_regs.u32[addr >> 2] = v;
					break;
			}
		} else {
			goto err;
		}

		trans.set_response_status(tlm::TLM_OK_RESPONSE);
		return;

err:
		SC_REPORT_WARNING("qdma",
				"unsupported read / write on the axi bar");
		trans.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
	}

	void ringCQHeadDoorbell(uint16_t qid, uint16_t head){
		CQueue *pQueue = ppCQueue[qid];

		if (pQueue) {
			//uint16_t oldhead = pQueue->getHead();
			//uint32_t oldcount = pQueue->getItemCount();

			pQueue->setHead(head);

			// TODO(Jhieb) explore interrupts.
			/*if (pQueue->interruptEnabled()) {
				clearInterrupt(pQueue->getInterruptVector());
			}*/
		}
	}

	void ringSQTailDoorbell(uint16_t qid, uint16_t tail){
		SQueue *pQueue = ppSQueue[qid];

		if (pQueue) {
			//uint16_t oldtail = pQueue->getTail();
			//uint32_t oldcount = pQueue->getItemCount();

			pQueue->setTail(tail);
		}
	}

	void controller_identify(uint8_t *data){
		uint16_t vid, ssvid;
		uint64_t totalSize;
		uint64_t unallocated;

		// TODO(jhieb) support?
		//pParent->getVendorID(vid, ssvid);

		totalSize = totalLogicalPages * logicalPageSize;
		// TODO(jhieb) isn't this inverted wrong?
  		unallocated = allocatedLogicalPages * logicalPageSize;
		//pSubsystem->getNVMCapacity(totalSize, unallocated);

		SC_REPORT_WARNING("qdma", "CONTROLLER IDFY");

		unallocated = totalSize - unallocated;

		// TODO(jhieb) move all of this into a struct?
		/** Controller Capabilities and Features **/
		{
			// PCI Vendor ID
			memcpy(data + 0x0000, &vid, 2);

			// PCI Subsystem Vendor ID
			memcpy(data + 0x0002, &ssvid, 2);

			// Serial Number
			memcpy(data + 0x0004, "00000000000000000000", 0x14);

			// Model Number
			memcpy(data + 0x0018, "SimpleSSD NVMe Controller by CAMELab    ", 0x28);

			// Firmware Revision
			memcpy(data + 0x0040, "02.01.03", 0x08);

			// Recommended Arbitration Burst
			data[0x0048] = 0x00;

			// IEEE OUI Identifier
			{
			data[0x0049] = 0x00;
			data[0x004A] = 0x00;
			data[0x004B] = 0x00;
			}

			// Controller Multi-Path I/O and Namespace Sharing Capabilities
			// [Bits ] Description
			// [07:04] Reserved
			// [03:03] 1 for Asymmetric Namespace Access Reporting
			// [02:02] 1 for SR-IOV Virtual Function, 0 for PCI (Physical) Function
			// [01:01] 1 for more than one host may connected to NVM subsystem
			// [00:00] 1 for NVM subsystem may has more than one NVM subsystem port
			data[0x004C] = 0x00;

			// Maximum Data Transfer Size
			data[0x004D] = 0x00;  // No limit

			// Controller ID
			{
			data[0x004E] = 0x00;
			data[0x004F] = 0x00;
			}

			// Version
			{
			data[0x0050] = 0x01;
			data[0x0051] = 0x04;
			data[0x0052] = 0x00;
			data[0x0053] = 0x00;
			}  // NVM Express 1.4 Compliant Controller

			// RTD3 Resume Latency
			{
			data[0x0054] = 0x00;
			data[0x0055] = 0x00;
			data[0x0056] = 0x00;
			data[0x0057] = 0x00;
			}  // Not reported

			// RTD3 Enter Latency
			{
			data[0x0058] = 0x00;
			data[0x0059] = 0x00;
			data[0x005A] = 0x00;
			data[0x005B] = 0x00;
			}  // Not repotred

			// Optional Asynchronous Events Supported
			{
			// [Bits ] Description
			// [31:15] Reserved
			// [14:14] 1 for Support Endurance Group Event Aggregate Log Page Change
			//         Notice
			// [13:13] 1 for Support LBA Status Information Notice
			// [12:12] 1 for Support Predictable Latency Event Aggregate Log Change
			//         Notice
			// [11:11] 1 for Support Asymmetric Namespace Access Change Notice
			// [10:10] Reserved
			// [09:09] 1 for Support Firmware Activation Notice
			// [08:08] 1 for Support Namespace Attributes Notice
			// [07:00] Reserved
			data[0x005C] = 0x00;
			data[0x005D] = 0x00;
			data[0x005E] = 0x00;
			data[0x005F] = 0x00;
			}

			// Controller Attributes
			{
			// [Bits ] Description
			// [31:01] Reserved
			// [09:09] 1 for Support UUID List
			// [08:08] 1 for Support SQ Associations
			// [07:07] 1 for Support Namespace Granularity
			// [06:06] 1 for Traffic Based Keep Alive Support
			// [05:05] 1 for Support Predictable Latency Mode
			// [04:04] 1 for Support Endurance Group
			// [03:03] 1 for Support Read Recovery Levels
			// [02:02] 1 for Support NVM Sets
			// [01:01] 1 for Support Non-Operational Power State Permissive Mode
			// [00:00] 1 for Support 128-bit Host Identifier
			data[0x0060] = 0x00;
			data[0x0061] = 0x00;
			data[0x0062] = 0x00;
			data[0x0063] = 0x00;
			}

			// Read Recovery Levels Supported
			{
			// [Bits ] Description
			// [15:15] 1 for Read Recovery Level 15 - Fast Fail
			// ...
			// [04:04] 1 for Read Recovery Level 4 - Default
			// ...
			// [00:00] 1 for Read Recovery Level 0
			data[0x0064] = 0x00;
			data[0x0065] = 0x00;
			}

			memset(data + 0x0066, 0, 9);  // Reserved

			// Controller Type
			// [Value] Description
			// [   0h] Reserved (Controller Type not reported)
			// [   1h] I/O Controller
			// [   2h] Discovery Controller
			// [   3h] Administrative Controller
			// [4h to FFh] Reserved
			data[0x006F] = 0x01;

			// FRU Globally Unique Identifier
			memset(data + 0x0070, 0, 16);

			// Command Retry Delay Time 1
			{
			data[0x0080] = 0x00;
			data[0x0081] = 0x00;
			}

			// Command Retry Delay Time 2
			{
			data[0x0082] = 0x00;
			data[0x0083] = 0x00;
			}

			// Command Retry Delay Time 3
			{
			data[0x0084] = 0x00;
			data[0x0085] = 0x00;
			}

			memset(data + 0x0086, 0, 106);  // Reserved
			memset(data + 0x00F0, 0, 16);   // See NVMe-MI Specification
		}

		/** Admin Command Set Attributes & Optional Controller Capabilities **/
		{
			// Optional Admin Command Support
			{
			// [Bits ] Description
			// [15:10] Reserved
			// [09:09] 1 for SupportGet LBA Status capability
			// [08:08] 1 for Support Doorbell Buffer Config command
			// [07:07] 1 for Support Virtualization Management command
			// [06:06] 1 for Support NVMe-MI Send and NVMe-MI Receive commands
			// [05:05] 1 for Support Directives
			// [04:04] 1 for Support Device Self-Test command
			// [03:03] 1 for Support Namespace Management and Namespace Attachment
			//         commands
			// [02:02] 1 for Support Firmware Commit and Firmware Image Download
			//         commands
			// [01:01] 1 for Support Format NVM command
			// [00:00] 1 for Support Security Send and Security Receive commands
			data[0x0100] = 0x0A;
			data[0x0101] = 0x00;
			}

			// Abort Command Limit
			data[0x0102] = 0x03;  // Recommanded value is 4 (3 + 1)

			// Asynchronous Event Request Limit
			data[0x0103] = 0x03;  // Recommanded value is 4 (3 + 1))

			// Firmware Updates
			// [Bits ] Description
			// [07:05] Reserved
			// [04:04] 1 for Support firmware activation without a reset
			// [03:01] The number of firmware slot
			// [00:00] 1 for First firmware slot is read only, 0 for read/write
			data[0x0104] = 0x00;

			// Log Page Attributes
			// [Bits ] Description
			// [07:05] Reserved
			// [04:04] 1 for Support Persisten Event log
			// [03:03] 1 for Support Telemetry Host-Initiated and Telemetry Controller-
			//         Initiated log pages and Telemetry Log Notices
			// [02:02] 1 for Support extended data for Get Log Page command
			// [01:01] 1 for Support Command Effects log page
			// [00:00] 1 for Support S.M.A.R.T. / Health information log page per
			//         namespace basis
			data[0x0105] = 0x01;

			// Error Log Page Entries, 0's based value
			data[0x0106] = 0x63;  // 64 entries

			// Number of Power States Support, 0's based value
			data[0x0107] = 0x00;  // 1 states

			// Admin Vendor Specific Command Configuration
			// [Bits ] Description
			// [07:01] Reserved
			// [00:00] 1 for all vendor specific commands use the format at Figure 12.
			//         0 for format is vendor specific
			data[0x0108] = 0x00;

			// Autonomous Power State Transition Attributes
			// [Bits ] Description
			// [07:01] Reserved
			// [00:00] 1 for Support autonomous power state transitions
			data[0x0109] = 0x00;

			// Warning Composite Temperature Threshold
			{
			data[0x010A] = 0x00;
			data[0x010B] = 0x00;
			}

			// Critical Composite Temperature Threshold
			{
			data[0x010C] = 0x00;
			data[0x010D] = 0x00;
			}

			// Maximum Time for Firmware Activation
			{
			data[0x010E] = 0x00;
			data[0x010F] = 0x00;
			}

			// Host Memory Buffer Preferred Size
			{
			data[0x0110] = 0x00;
			data[0x0111] = 0x00;
			data[0x0112] = 0x00;
			data[0x0113] = 0x00;
			}

			// Host Memory Buffer Minimum Size
			{
			data[0x0114] = 0x00;
			data[0x0115] = 0x00;
			data[0x0116] = 0x00;
			data[0x0117] = 0x00;
			}

			// Total NVM Capacity
			{
			memcpy(data + 0x118, &totalSize, 8);
			memset(data + 0x120, 0, 8);
			}

			// Unallocated NVM Capacity
			{
			memcpy(data + 0x118, &unallocated, 8);
			memset(data + 0x120, 0, 8);
			}

			// Replay Protected Memory Block Support
			{
			// [Bits ] Description
			// [31:24] Access Size
			// [23:16] Total Size
			// [15:06] Reserved
			// [05:03] Authentication Method
			// [02:00] Number of RPMB Units
			data[0x0138] = 0x00;
			data[0x0139] = 0x00;
			data[0x013A] = 0x00;
			data[0x013B] = 0x00;
			}

			// Extended Device Self-Test Time
			{
			data[0x013C] = 0x00;
			data[0x013D] = 0x00;
			}

			// Device Self-Test Options
			// [Bits ] Description
			// [07:01] Reserved
			// [00:00] 1 for Support only one device self-test operation in process at
			//         a time
			data[0x013E] = 0x00;

			// Firmware Update Granularity
			data[0x013F] = 0x00;

			// Keep Alive Support
			{
			data[0x0140] = 0x00;
			data[0x0141] = 0x00;
			}

			// Host Controlled Thermal Management Attributes
			{
			// [Bits ] Description
			// [15:01] Reserved
			// [00:00] 1 for Support host controlled thermal management
			data[0x0142] = 0x00;
			data[0x0143] = 0x00;
			}

			// Minimum Thernam Management Temperature
			{
			data[0x0144] = 0x00;
			data[0x0145] = 0x00;
			}

			// Maximum Thernam Management Temperature
			{
			data[0x0146] = 0x00;
			data[0x0147] = 0x00;
			}

			// Sanitize Capabilities
			{
			// [Bits ] Description
			// [31:30] No-Deallocate Modifies Media After Sanitize
			// [29:29] No-Deallocate Inhibited
			// [28:03] Reserved
			// [02:02] 1 for Support Overwrite
			// [01:01] 1 for Support Block Erase
			// [00:00] 1 for Support Crypto Erase
			data[0x0148] = 0x00;
			data[0x0149] = 0x00;
			data[0x014A] = 0x00;
			data[0x014B] = 0x00;
			}

			// Host Memory Buffer Minimum Descriptor Entry Size
			{
			data[0x014C] = 0x00;
			data[0x014D] = 0x00;
			data[0x014E] = 0x00;
			data[0x014F] = 0x00;
			}

			// Host Memory Maximum Descriptors Entries
			{
			data[0x0150] = 0x00;
			data[0x0151] = 0x00;
			}

			// NVM Set Identifier Maximum
			{
			data[0x0152] = 0x00;
			data[0x0153] = 0x00;
			}

			// Endurance Group Identifier Maximum
			{
			data[0x0154] = 0x00;
			data[0x0155] = 0x00;
			}

			// ANA Transition Time
			data[0x0156] = 0x00;

			// Asymmetric Namespace Access Capabilities
			// [Bits ] Description
			// [07:07] 1 for Support non-zero ANAGRPID
			// [06:06] 1 for ANAGRPID does not change while namespace is attached
			// [05:05] Reserved
			// [04:04] 1 for Support ANA Change state
			// [03:03] 1 for Support ANA Persistent Loss state
			// [02:02] 1 for Support ANA Inaccessible state
			// [01:01] 1 for Support ANA Non-Optimized state
			// [00:00] 1 for Support ANA Optimized state
			data[0x157] = 0x00;

			// ANA Group Identifier Maximum
			{
			data[0x0158] = 0x00;
			data[0x0159] = 0x00;
			data[0x015A] = 0x00;
			data[0x015B] = 0x00;
			}

			// Number of ANA AGroup Identifiers
			{
			data[0x015C] = 0x00;
			data[0x015D] = 0x00;
			data[0x015E] = 0x00;
			data[0x015F] = 0x00;
			}

			// Persistent Event Log Size
			{
			data[0x0160] = 0x00;
			data[0x0161] = 0x00;
			data[0x0162] = 0x00;
			data[0x0163] = 0x00;
			}

			// Reserved
			memset(data + 0x0164, 0, 156);
		}

		/** NVM Command Set Attributes **/
		{
			// Submission Queue Entry Size
			// [Bits ] Description
			// [07:04] Maximum Submission Queue Entry Size
			// [03:00] Minimum Submission Queue Entry Size
			data[0x0200] = 0x66;  // 64Bytes, 64Bytes

			// Completion Queue Entry Size
			// [Bits ] Description
			// [07:04] Maximum Completion Queue Entry Size
			// [03:00] Minimum Completion Queue Entry Size
			data[0x0201] = 0x44;  // 16Bytes, 16Bytes

			// Maximum  Outstanding Commands
			{
			data[0x0202] = 0x00;
			data[0x0203] = 0x00;
			}

			// Number of Namespaces
			// SimpleSSD supports infinite number of namespaces (0xFFFFFFFD)
			// But kernel's DIV_ROUND_UP has problem when number is too big
			// #define _KERNEL_DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))
			// This wrong macro introduces DIV_ROUND_UP(0xFFFFFFFD, 1024) to zero
			// So we use 1024 here, for only one IDENTIFY NSLIST command
			// TODO(jhieb) is this necessary??
			//*(uint32_t *)(data + 0x0204) = 1024;
			*(uint32_t *)(data + 0x0204) = 1;

			// Optional NVM Command Support
			{
			// [Bits ] Description
			// [15:08] Reserved
			// [07:07] 1 for Support Verify command
			// [06:06] 1 for Support Timestamp features
			// [05:05] 1 for Support reservations
			// [04:04] 1 for Support Save field in Set Features command and Select
			//         field in Get Features command
			// [03:03] 1 for Support Write Zeros command
			// [02:02] 1 for Support Dataset Management command
			// [01:01] 1 for Support Write Uncorrectable command
			// [00:00] 1 for Support Compare command
			data[0x0208] = 0x05;
			data[0x0209] = 0x00;
			}

			// Fused Operation Support
			{
			// [Bits ] Description
			// [15:01] Reserved
			// [00:00] 1 for Support Compare and Write fused operation
			data[0x020A] = 0x00;
			data[0x020B] = 0x00;
			}

			// Format NVM Attributes
			// [Bits ] Description
			// [07:03] Reserved
			// [02:02] 1 for Support cryptographic erase
			// [01:01] 1 for Support cryptographic erase performed on all namespaces,
			//         0 for namespace basis
			// [00:00] 1 for Format on specific namespace results on format on all
			//         namespaces, 0 for namespace basis
			data[0x020C] = 0x00;

			// Volatile Write Cache
			// [Bits ] Description
			// [07:03] Reserved
			// [02:01] Indicated Flush comand behavior if the NSID is 0xFFFFFFFF
			// [00:00] 1 for volatile write cache is present
			data[0x020D] = 1;
			data[0x020D] |= 0x06;

			// Atomic Write Unit Normal
			{
			data[0x020E] = 0x00;
			data[0x020F] = 0x00;
			}

			// Atomic Write Unit Power Fail
			{
			data[0x0210] = 0x00;
			data[0x0211] = 0x00;
			}

			// NVM Vendor Specific Command Configuration
			// [Bits ] Description
			// [07:01] Reserved
			// [00:00] 1 for all vendor specific commands use the format at Figure 12.
			//         0 for format is vendor specific
			data[0x0212] = 0x00;

			// Namespace Write Protection Capabilities
			// [Bits ] Description
			// [07:03] Reserved
			// [02:02] 1 for Support Permenant Write Protect state
			// [01:01] 1 for Support Write Protect Until Power Cycle state
			// [00:00] 1 for Support No Write Protect and Write Protect state
			data[0x0213] = 0x00;

			// Atomic Compare & Write Unit
			{
			data[0x0214] = 0x00;
			data[0x0215] = 0x00;
			}

			// Reserved
			memset(data + 0x0216, 0, 2);

			// SGL Support
			{
			// [Bits ] Description
			// [31:22] Reserved
			// [21:21] 1 for Support Ransport SGL Data Block
			// [20:20] 1 for Support Address field in SGL Data Block
			// [19:19] 1 for Support MPTR containing SGL descriptor
			// [18:18] 1 for Support MPTR/DPTR containing SGL with larger than amount
			//         of data to be trasferred
			// [17:17] 1 for Support byte aligned contiguous physical buffer of
			//         metadata is supported
			// [16:16] 1 for Support SGL Bit Bucket descriptor
			// [15:03] Reserved
			// [02:02] 1 for Support Keyed SGL Data Block descriptor
			// [01:01] Reserved
			// [00:00] 1 for Support SGLs in NVM Command Set
			data[0x0218] = 0x01;
			data[0x0219] = 0x00;
			data[0x021A] = 0x17;
			data[0x021B] = 0x00;
			}

			// Maximun Number of Allowed Namespaces
			*(uint32_t *)(data + 0x021C) = 0;

			// Reserved
			memset(data + 0x0220, 0, 224);

			// NVM Subsystem NVMe Qualified Name
			{
			memset(data + 0x300, 0, 0x100);
			strncpy((char *)data + 0x0300,
					"nqn.2014-08.org.nvmexpress:uuid:270a1c70-962c-4116-6f1e340b9321",
					0x44);
			}

			// Reserved
			memset(data + 0x0400, 0, 768);

			// NVMe over Fabric
			memset(data + 0x0700, 0, 256);
		}

		/** Power State Descriptors **/
		// Power State 0
		/// Descriptor
		{
			// Maximum Power
			{
			data[0x0800] = 0xC4;
			data[0x0801] = 0x09;
			}

			// Reserved
			data[0x0802] = 0x00;

			// [Bits ] Description
			// [31:26] Reserved
			// [25:25] Non-Operational State
			// [24:24] Max Power Scale
			data[0x0803] = 0x00;

			// Entry Latency
			{
			data[0x0804] = 0x00;
			data[0x0805] = 0x00;
			data[0x0806] = 0x00;
			data[0x0807] = 0x00;
			}

			// Exit Latency
			{
			data[0x0808] = 0x00;
			data[0x0809] = 0x00;
			data[0x080A] = 0x00;
			data[0x080B] = 0x00;
			}

			// [Bits   ] Description
			// [103:101] Reserved
			// [100:096] Relative Read Throughput
			data[0x080C] = 0x00;

			// [Bits   ] Description
			// [111:109] Reserved
			// [108:104] Relative Read Latency
			data[0x080D] = 0x00;

			// [Bits   ] Description
			// [119:117] Reserved
			// [116:112] Relative Write Throughput
			data[0x080E] = 0x00;

			// [Bits   ] Description
			// [127:125] Reserved
			// [124:120] Relative Write Latency
			data[0x080E] = 0x00;

			// Idle Power
			{
			data[0x080F] = 0x00;
			data[0x0810] = 0x00;
			}

			// [Bits   ] Description
			// [151:150] Idle Power Scale
			// [149:144] Reserved
			data[0x0811] = 0x00;

			// Reserved
			data[0x0812] = 0x00;

			// Active Power
			{
			data[0x0813] = 0x00;
			data[0x0814] = 0x00;
			}

			// [Bits   ] Description
			// [183:182] Active Power Scale
			// [181:179] Reserved
			// [178:176] Active Power Workload
			data[0x0815] = 0x00;

			// Reserved
			memset(data + 0x0816, 0, 9);
		}

		// PSD1 ~ PSD31
		memset(data + 0x0820, 0, 992);

		// Vendor specific area
		memset(data + 0x0C00, 0, 1024);

	}

	void read_dma_host(uint64_t size, uint64_t addr, uint8_t *buffer){
		// TODO(jhieb) this has problems with the memory being outside of the card memory space need to figure it out.
		// Currently hacking by using the descriptor writeback in chunks.
		//this->do_mm_dma(reinterpret_cast<uint64_t>(buffer), req.entry.data1, 0x1000, false);
		// Write the identify buffer to host memory piece by piece
		const size_t chunk_size = 64;  // Write 64 bytes at a time, max desc size.
		const size_t total_size = size;
		for (size_t offset = 0; offset < total_size; offset += chunk_size) {
			size_t remaining = total_size - offset;
			size_t current_chunk = (remaining < chunk_size) ? remaining : chunk_size;
			this->fetch_descriptor(
				addr + offset,    // Host PRP address + offset
				current_chunk,               // Current chunk size
				buffer + offset              // Buffer + offset
			);
		}
	}

	void write_dma_prp_list_host(PRPList* prpList, uint8_t *buffer){
		std::vector<PRP> prpVec = prpList->getPRPList();
		for(size_t i=0;i < prpVec.size();i++){
			write_dma_host(prpVec[i].size, prpVec[i].addr, buffer);
			// move buffer ptr forward by size of prp that we just wrote for the next one.
			buffer += prpVec[i].size / 8;
		}
	}

	void write_dma_sgl_list_host(SGL* sgl, uint8_t *buffer){
		std::vector<Chunk> chunkVec = sgl->getChunkList();
		for(size_t i=0;i < chunkVec.size();i++){
			write_dma_host(chunkVec[i].length, chunkVec[i].addr, buffer);
			// move buffer ptr forward by size of chunk that we just wrote for the next one.
			buffer += chunkVec[i].length / 8;
		}
	}

	void write_dma_host(uint64_t size, uint64_t addr, uint8_t *buffer){
		// TODO(jhieb) this has problems with the memory being outside of the card memory space need to figure it out.
		// Currently hacking by using the descriptor writeback in chunks.
		//this->do_mm_dma(reinterpret_cast<uint64_t>(buffer), req.entry.data1, 0x1000, false);
		// Write the identify buffer to host memory piece by piece
		const size_t chunk_size = 64;  // Write 64 bytes at a time, max desc size.
		const size_t total_size = size;
		for (size_t offset = 0; offset < total_size; offset += chunk_size) {
			size_t remaining = total_size - offset;
			size_t current_chunk = (remaining < chunk_size) ? remaining : chunk_size;
			
			this->descriptor_writeback(
				addr + offset,    // Host PRP address + offset
				current_chunk,               // Current chunk size
				buffer + offset              // Buffer + offset
			);
		}
	}

	void fillIdentifyNamespace(uint8_t *buffer,
										Namespace::Information *info) {
		// Namespace Size
		memcpy(buffer + 0, &info->size, 8);

		// Namespace Capacity
		memcpy(buffer + 8, &info->capacity, 8);

		// Namespace Utilization
		// This function also called from OpenChannelSSD subsystem
		// TODO(jhieb) support?
		/*if (pHIL) {
			info->utilization =
				pHIL->getUsedPageCount(info->range.slpn,
									info->range.slpn + info->range.nlp) *
				logicalPageSize / info->lbaSize;
		}*/

		memcpy(buffer + 16, &info->utilization, 8);

		// Namespace Features
		// [Bits ] Description
		// [07:05] Reserved
		// [04:04] NVM Set capabilities
		// [03:03] Reuse of NGUID field
		// [02:02] 1 for Support Deallocated or Unwritten Logical Block error
		// [01:01] 1 for NAWUN, NAWUPF, NACWU are defined
		// [00:00] 1 for Support Thin Provisioning
		buffer[24] = 0x04;  // Trim supported

		// Number of LBA Formats
		buffer[25] = nLBAFormat - 1;  // 0's based

		// Formatted LBA Size
		buffer[26] = info->lbaFormatIndex;

		// End-to-end Data Protection Capabilities
		buffer[28] = info->dataProtectionSettings;

		// Namespace Multi-path I/O and Namespace Sharing Capabilities
		buffer[30] = info->namespaceSharingCapabilities;

		// NVM capacity
		memcpy(buffer + 48, &info->sizeInByteL, 8);
		memcpy(buffer + 56, &info->sizeInByteH, 8);

		// LBA Formats
		for (uint32_t i = 0; i < nLBAFormat; i++) {
			memcpy(buffer + 128 + i * 4, lbaFormat + i, 4);
		}

		// OpenChannel SSD
		/*if (!pHIL) {
			buffer[384] = 0x01;
		}*/
	}

	bool identify(SQEntryWrapper &req) {
		bool err = false;

		SC_REPORT_WARNING("qdma", "IDENTIFY");
		CQEntryWrapper resp(req);
		uint8_t cns = req.entry.dword10 & 0xFF;
		//uint16_t cntid = (req.entry.dword10 & 0xFFFF0000) >> 16;
		bool ret = true;

		//RequestContext *pContext = new RequestContext(func, resp);
		//uint16_t idx = 0;

		//uint8_t* buffer = (uint8_t *)calloc(0x1000, sizeof(uint8_t));
		uint8_t buffer[0x1000]; //local storage.
		memset(buffer, 0, sizeof(buffer));
		//pContext->buffer = (uint8_t *)calloc(0x1000, sizeof(uint8_t));
		//debugprint(LOG_HIL_NVME, "ADMIN   | Identify | CNS %d | CNTID %d | NSID %d",
		//			cns, cntid, req.entry.namespaceID);
		std::ostringstream msg;
		switch (cns) {
			case CNS_IDENTIFY_NAMESPACE:
				msg << "ctrl idfy nsid " << req.entry.namespaceID;
				SC_REPORT_INFO("qdma", msg.str().c_str());
			
				if (req.entry.namespaceID == NSID_ALL) {
					// FIXME: Not called by current(4.9.32) NVMe driver
				}
				else {
					for (auto &iter : lNamespaces) {
						if (iter->isAttached() && iter->getNSID() == req.entry.namespaceID) {
							// TODO(jhieb) does this work with multiple namespaces with the buffer pointer?
							fillIdentifyNamespace(buffer, iter->getInfo());
						}
					}
				}

			break;
			case CNS_IDENTIFY_CONTROLLER:
				controller_identify(buffer);
			break;
			case CNS_ACTIVE_NAMESPACE_LIST:
				SC_REPORT_ERROR("qdma", "unsupported idfy");
				/*
				if (req.entry.namespaceID >= NSID_ALL - 1) {
					err = true;
					resp.makeStatus(true, false, TYPE_GENERIC_COMMAND_STATUS,
									STATUS_ABORT_INVALID_NAMESPACE);
				}
				else {
					for (auto &iter : lNamespaces) {
					if (iter->isAttached() && iter->getNSID() > req.entry.namespaceID) {
						((uint32_t *)pContext->buffer)[idx++] = iter->getNSID();
					}
					}
				}
				*/
				break;
			case CNS_ALLOCATED_NAMESPACE_LIST:
				SC_REPORT_ERROR("qdma", "unsupported idfy");
				/*
				if (req.entry.namespaceID >= NSID_ALL - 1) {
					err = true;
					resp.makeStatus(true, false, TYPE_GENERIC_COMMAND_STATUS,
									STATUS_ABORT_INVALID_NAMESPACE);
				}
				else {
					for (auto &iter : lNamespaces) {
					if (iter->getNSID() > req.entry.namespaceID) {
						((uint32_t *)pContext->buffer)[idx++] = iter->getNSID();
					}
					}
				}*/

				break;
			case CNS_IDENTIFY_ALLOCATED_NAMESPACE:
				SC_REPORT_ERROR("qdma", "unsupported idfy");
				/*
				for (auto &iter : lNamespaces) {
					if (iter->getNSID() == req.entry.namespaceID) {
					fillIdentifyNamespace(pContext->buffer, iter->getInfo());
					}
				}*/

				break;
			case CNS_ATTACHED_CONTROLLER_LIST:
				SC_REPORT_ERROR("qdma", "unsupported idfy");
				/*
				// Only one controller
				if (cntid == 0) {
					((uint16_t *)pContext->buffer)[idx++] = 1;
				}*/

				break;
			case CNS_CONTROLLER_LIST:
				SC_REPORT_ERROR("qdma", "unsupported idfy");
				/*
				// Only one controller
				if (cntid == 0) {
					((uint16_t *)pContext->buffer)[idx++] = 1;
				}*/

				break;
		}
		
		if (ret && !err) {
			write_dma_host(4096, req.entry.data1, buffer);
			SC_REPORT_WARNING("qdma", "CTRL IDFY CMPL STS");
			resp.makeStatus(false, false, TYPE_GENERIC_COMMAND_STATUS, STATUS_SUCCESS);
			write_cq_entry(resp);
			// TODO(jhieb) support SGL.
			/*if (req.useSGL) {
			pContext->dma =
				new SGL(cfgdata, doWrite, pContext, req.entry.data1, req.entry.data2);
			}
			else {
			pContext->dma = new PRPList(cfgdata, doWrite, pContext, req.entry.data1,
										req.entry.data2, (uint64_t)0x1000);
			}*/
		}
		else {
			//func(resp);
			//free(buffer);
			//delete pContext;
		}

		return ret;
	}


	bool setFeatures(SQEntryWrapper &req) {
		bool err = false;

		CQEntryWrapper resp(req);
		uint16_t fid = req.entry.dword10 & 0x00FF;
		bool save = req.entry.dword10 & 0x80000000;

		if (save) {
			err = true;
			resp.makeStatus(false, false, TYPE_COMMAND_SPECIFIC_STATUS,
							STATUS_FEATURE_ID_NOT_SAVEABLE);
		}

		if (!err) {
			switch (fid) {
				case FEATURE_NUMBER_OF_QUEUES:
					if ((req.entry.dword11 & 0xFFFF) == 0xFFFF ||
						(req.entry.dword11 & 0xFFFF0000) == 0xFFFF0000) {
						resp.makeStatus(false, false, TYPE_GENERIC_COMMAND_STATUS, STATUS_INVALID_FIELD);
					}
					else {
						if ((req.entry.dword11 & 0xFFFF) >= sqsize) {
							req.entry.dword11 = (req.entry.dword11 & 0xFFFF0000) | (sqsize - 1);
						}
						if (((req.entry.dword11 & 0xFFFF0000) >> 16) >= cqsize) {
							req.entry.dword11 =
								(req.entry.dword11 & 0xFFFF) | ((uint32_t)(cqsize - 1) << 16);
						}

						resp.entry.dword0 = req.entry.dword11;
						queueAllocated = resp.entry.dword0;
						resp.makeStatus(false, false, TYPE_GENERIC_COMMAND_STATUS, STATUS_SUCCESS);
					}

					break;
				case FEATURE_INTERRUPT_COALESCING:
					setCoalescingParameter((req.entry.dword11 >> 8) & 0xFF,
													req.entry.dword11 & 0xFF);
					resp.makeStatus(false, false, TYPE_GENERIC_COMMAND_STATUS, STATUS_SUCCESS);
					break;
				case FEATURE_INTERRUPT_VECTOR_CONFIGURATION:
					setCoalescing(req.entry.dword11 & 0xFFFF,
										req.entry.dword11 & 0x10000);
					resp.makeStatus(false, false, TYPE_GENERIC_COMMAND_STATUS, STATUS_SUCCESS);
					break;
				default:
					resp.makeStatus(true, false, TYPE_GENERIC_COMMAND_STATUS,
									STATUS_INVALID_FIELD);
					break;
			}
		}
		SC_REPORT_WARNING("qdma", "set features CMPL STS");
		write_cq_entry(resp);

		return true;
	}
	
	void setCoalescingParameter(uint8_t time, uint8_t thres) {
		aggregationTime = time * 100000000;
		aggregationThreshold = thres;
	}

	void setCoalescing(uint16_t iv, bool enable) {
		auto iter = aggregationMap.find(iv);

		if (iter != aggregationMap.end()) {
			iter->second.valid = enable;
			iter->second.nextTime = 0;
			iter->second.requestCount = 0;
			iter->second.pending = false;
		}
	}

	int createCQueueCtrl(uint16_t cqid, uint16_t size, uint16_t iv,
                             bool ien, bool pc, uint64_t prp1) {
		int ret = 1;  // Invalid Queue ID
		static DMAFunction empty = [](uint64_t, void *) {};
		if (ppCQueue[cqid] == NULL) {
			ppCQueue[cqid] = new CQueue(iv, ien, cqid, size);
			ppCQueue[cqid]->setBase(new PRPList(cfgdata, empty, nullptr, prp1, size * cqstride, pc), cqstride, prp1);
			ret = 0;
			// Interrupt coalescing config
			auto iter = aggregationMap.find(iv);
			AggregationInfo info;

			info.valid = false;
			info.nextTime = 0;
			info.requestCount = 0;

			if (iter == aggregationMap.end()) {
				aggregationMap.insert({iv, info});
			}
			else {
				iter->second = info;
			}
		}

		return ret;
	}

	bool createCQueue(SQEntryWrapper &req) {
		CQEntryWrapper resp(req);
		bool err = false;

		uint16_t cqid = req.entry.dword10 & 0xFFFF;
		uint16_t entrySize = ((req.entry.dword10 & 0xFFFF0000) >> 16) + 1;
		uint16_t iv = (req.entry.dword11 & 0xFFFF0000) >> 16;
		bool ien = req.entry.dword11 & 0x02;
		bool pc = req.entry.dword11 & 0x01;

		if (entrySize > cfgdata.maxQueueEntry) {
			err = true;
			resp.makeStatus(false, false, TYPE_COMMAND_SPECIFIC_STATUS,
							STATUS_INVALID_QUEUE_SIZE);
		}

		if (!err) {
			int ret = createCQueueCtrl(cqid, entrySize, iv, ien, pc,
											req.entry.data1);

			if (ret == 1) {
				resp.makeStatus(false, false, TYPE_COMMAND_SPECIFIC_STATUS,
								STATUS_INVALID_QUEUE_ID);
			} else {
				resp.makeStatus(false, false, TYPE_GENERIC_COMMAND_STATUS, STATUS_SUCCESS);
			}
		}
		SC_REPORT_WARNING("qdma", "create Cqueue CMPL STS");
		write_cq_entry(resp);

		return true;
	}

	int createSQueueCtrl(uint16_t sqid, uint16_t cqid, uint16_t size,
								uint8_t priority, bool pc, uint64_t prp1) {
		int ret = 1;  // Invalid Queue ID
		static DMAFunction empty = [](uint64_t, void *) {};
		if (ppSQueue[sqid] == NULL) {
			if (ppCQueue[cqid] != NULL) {
				ppSQueue[sqid] = new SQueue(cqid, priority, sqid, size);
				ppSQueue[sqid]->setBase(new PRPList(cfgdata, empty, nullptr, prp1, size * sqstride, pc), sqstride, prp1);
				ret = 0;
			}
			else {
			ret = 2;  // Invalid CQueue
			}
		}

		return ret;
	}
	

	bool createSQueue(SQEntryWrapper &req) {
		CQEntryWrapper resp(req);
		bool err = false;

		uint16_t sqid = req.entry.dword10 & 0xFFFF;
		uint16_t entrySize = ((req.entry.dword10 & 0xFFFF0000) >> 16) + 1;
		uint16_t cqid = (req.entry.dword11 & 0xFFFF0000) >> 16;
		uint8_t priority = (req.entry.dword11 & 0x06) >> 1;
		bool pc = req.entry.dword11 & 0x01;

		if (entrySize > cfgdata.maxQueueEntry) {
			err = true;
			resp.makeStatus(false, false, TYPE_COMMAND_SPECIFIC_STATUS,
							STATUS_INVALID_QUEUE_SIZE);
		}

		if (!err) {
			int ret = createSQueueCtrl(sqid, cqid, entrySize, priority, pc,
											req.entry.data1);

			if (ret == 1) {
				resp.makeStatus(false, false, TYPE_COMMAND_SPECIFIC_STATUS,
								STATUS_INVALID_QUEUE_ID);
			}
			else if (ret == 2) {
				resp.makeStatus(false, false, TYPE_COMMAND_SPECIFIC_STATUS,
								STATUS_INVALID_COMPLETION_QUEUE);
			} else {
				resp.makeStatus(false, false, TYPE_GENERIC_COMMAND_STATUS, STATUS_SUCCESS);
			}
		}
		SC_REPORT_WARNING("qdma", "create Squeue CMPL STS");
		write_cq_entry(resp);

		return true;
	}


	bool getLogPage(SQEntryWrapper &req) {
		CQEntryWrapper resp(req);
		uint16_t numdl = (req.entry.dword10 & 0xFFFF0000) >> 16;
		uint16_t lid = req.entry.dword10 & 0xFFFF;
		uint16_t numdu = req.entry.dword11 & 0xFFFF;
		uint32_t lopl = req.entry.dword12;
		uint32_t lopu = req.entry.dword13;
		bool ret = false;
		bool submit = true;

		uint32_t req_size = (((uint32_t)numdu << 16 | numdl) + 1) * 4;
		uint64_t offset = ((uint64_t)lopu << 32) | lopl;

		/*
		DMAFunction smartInfo = [offset](uint64_t, void *context) {
			RequestContext *pContext = (RequestContext *)context;

			pContext->dma->write(offset, 512, pContext->buffer, dmaDone, context);
		};*/

		switch (lid) {
			case LOG_ERROR_INFORMATION:
				SC_REPORT_WARNING("qdma", "LOG ERROR INFORMATION");
				ret = true;
				break;
			case LOG_SMART_HEALTH_INFORMATION: {
				SC_REPORT_WARNING("qdma", "LOG SMART HEALTH");
				if (req.entry.namespaceID == NSID_ALL) {
					ret = true;
					submit = false;

					//pContext->buffer = globalHealth.data;

					/*if (req.useSGL) {
						pContext->dma = new SGL(cfgdata, smartInfo, pContext, req.entry.data1,
												req.entry.data2);
					}
					else {
						pContext->dma =
							new PRPList(cfgdata, smartInfo, pContext, req.entry.data1,
										req.entry.data2, (uint64_t)req_size);
					}*/
				}
				} break;
			case LOG_FIRMWARE_SLOT_INFORMATION:
				SC_REPORT_WARNING("qdma", "LOG FIRMWARE SLOT");
				ret = true;
				break;
			default:
				ret = true;
				resp.makeStatus(true, false, TYPE_COMMAND_SPECIFIC_STATUS,
								STATUS_INVALID_LOG_PAGE);
			break;
		}

		if (ret) {
			//write_dma_host(4096, req.entry.data1, buffer);
			SC_REPORT_WARNING("qdma", "GET LOG PAGE CMPL");
			resp.makeStatus(false, false, TYPE_GENERIC_COMMAND_STATUS, STATUS_SUCCESS);
			write_cq_entry(resp);
			// TODO(jhieb) support SGL.
			/*if (req.useSGL) {
			pContext->dma =
				new SGL(cfgdata, doWrite, pContext, req.entry.data1, req.entry.data2);
			}
			else {
			pContext->dma = new PRPList(cfgdata, doWrite, pContext, req.entry.data1,
										req.entry.data2, (uint64_t)0x1000);
			}*/
		}
		else {
			//func(resp);
			//free(buffer);
			//delete pContext;
		}

		return ret;
	}

	void submitCommand(SQEntryWrapper &req) {
		CQEntryWrapper resp(req);
		bool processed = false;

		commandCount++;

		// Admin command
		if (req.sqID == 0) {
			switch (req.entry.dword0.opcode) {
				case OPCODE_DELETE_IO_SQUEUE:
					//processed = deleteSQueue(req, func);
					SC_REPORT_ERROR("qdma", "unsupported opcode DELETE IO SQUEUE");
					break;
				case OPCODE_CREATE_IO_SQUEUE:
					processed = createSQueue(req);
					break;
				case OPCODE_GET_LOG_PAGE:
					processed = getLogPage(req);
					break;
				case OPCODE_DELETE_IO_CQUEUE:
					//processed = deleteCQueue(req, func);
					SC_REPORT_ERROR("qdma", "unsupported opcode DELETE IO CQUEUE");
					break;
				case OPCODE_CREATE_IO_CQUEUE:
					processed = createCQueue(req);
					break;
				case OPCODE_IDENTIFY:
					processed = identify(req);
					break;
				case OPCODE_ABORT:
					//processed = abort(req, func);
					SC_REPORT_ERROR("qdma", "unsupported opcode ABORT");
					break;
				case OPCODE_SET_FEATURES:
					processed = setFeatures(req);
					break;
				case OPCODE_GET_FEATURES:
					//processed = getFeatures(req, func);
					SC_REPORT_ERROR("qdma", "unsupported opcode GET FEATURES");
					break;
				case OPCODE_ASYNC_EVENT_REQ:
					break;
				case OPCODE_NAMESPACE_MANAGEMENT:
					//processed = namespaceManagement(req, func);
					SC_REPORT_ERROR("qdma", "unsupported opcode");
					break;
				case OPCODE_NAMESPACE_ATTACHMENT:
					//processed = namespaceAttachment(req, func);
					SC_REPORT_ERROR("qdma", "unsupported opcode");
					break;
				case OPCODE_FORMAT_NVM:
					//processed = formatNVM(req, func);
					SC_REPORT_ERROR("qdma", "unsupported opcode");
					break;
				default:
					resp.makeStatus(true, false, TYPE_GENERIC_COMMAND_STATUS,
									STATUS_INVALID_OPCODE);
					break;
			}
		} else {
			if (req.entry.namespaceID < NSID_ALL) {
				for (auto &iter : lNamespaces) {
					if (iter->getNSID() == req.entry.namespaceID) {
						uint8_t *buffer = nullptr;
						uint64_t slba;
						uint16_t nlb;
						uint32_t lbaSize;
						uint64_t totalBlockSize;
						std::ostringstream msg;
						PRPList* list;
						SGL* sgl;
						static DMAFunction empty = [](uint64_t, void *) {};
						CQEntryWrapper resp = iter->submitCommand(req, buffer);
						// TODO(jhieb) how do i know if the command needs to write the buffer back?  Pass reference to QDMA to namespace so it can do the write?
						//				option two pass back a counter of how much buffer needs to be written and to where??
						switch (req.entry.dword0.opcode){
							case OPCODE_READ:
							  	slba = ((uint64_t)req.entry.dword11 << 32) | req.entry.dword10;
  								nlb = (req.entry.dword12 & 0xFFFF) + 1;
								lbaSize = iter->getLBASize();
								totalBlockSize = lbaSize * nlb;

								if(req.useSGL){
									sgl = new SGL(cfgdata, empty, nullptr, req.entry.data1, req.entry.data2);
									while(sgl->getReqSGLDMA()){
										read_dma_host(sgl->length, sgl->address, sgl->buffer);
										sgl->parseSGLSegment();
									}
									write_dma_sgl_list_host(sgl, buffer);
								} else {
									list = new PRPList(cfgdata, empty, nullptr, req.entry.data1, req.entry.data2, totalBlockSize);
									// We need to DMA read the PRPList so that we can finish building our PRPList.
									if(list->getPRP2ReqDMA()){
										read_dma_host(list->prpListSize, list->prpListAddr, list->buffer);
										list->processPRPListBuffer();
									}
									write_dma_prp_list_host(list, buffer);
									delete list;
								}

      							msg << "read slba " << slba << " nlb " << nlb;
      							SC_REPORT_INFO("qdma", msg.str().c_str());
								// delete list;
								free(buffer);
								buffer = NULL;
								break;
							default:
								break;
						}
						write_cq_entry(resp);
						processed = true;
						break;
					}
				}
			} else {
				// TODO(jhieb) when does this make sense?
				if (lNamespaces.size() > 0) {
					uint8_t *buffer = nullptr;
					CQEntryWrapper resp = lNamespaces.front()->submitCommand(req, buffer);
					processed = true;
				}
			}
		}

		// failed to do work.  Invalid namespace status for now?
		if (!processed) {
			resp.makeStatus(false, false, TYPE_GENERIC_COMMAND_STATUS,
							STATUS_ABORT_INVALID_NAMESPACE);
			SC_REPORT_ERROR("qdma", "FAILED TO SUBMIT COMMAND");
			//func(resp);
		}
	}

	void handle_request(){
		// Check SQFIFO
		if (lSQFIFO.size() > 0) {
			SQEntryWrapper *front = new SQEntryWrapper(lSQFIFO.front());
			lSQFIFO.pop_front();
			submitCommand(*front);
		}
	}

	void write_cq_entry(CQEntryWrapper resp){
		CQueue *pQueue = ppCQueue[resp.cqID];
		uint64_t addr = pQueue->getBaseAddr() + pQueue->getTail() * cqstride;
		pQueue->setData(&(resp.entry));
		this->descriptor_writeback(addr, cqstride, resp.entry.data);
		SC_REPORT_WARNING("qdma", "CQ Entry Push MSIX");
		this->msix_trig[resp.cqID].notify();
	}

	SQEntryWrapper read_sq_entry(int qid){
		uint8_t local_buffer[QDMA_DESC_MAX_SIZE]; //local storage.
		SQueue *pQueue = ppSQueue[qid];
		uint64_t addr = pQueue->getBaseAddr() + pQueue->getHead() * sqstride;
		this->fetch_descriptor(addr, sqstride, local_buffer);
		SQEntry entry;
		memcpy(&entry, local_buffer, sizeof(SQEntry));
		// increment the head.
		pQueue->getData(&entry);
		// TODO(jhieb) squid do i need it?
		SQEntryWrapper new_cmd(entry, qid, pQueue->getCQID(), pQueue->getHead(), 0); // entry, sqid, cqid, sqhead, squid;
		return new_cmd;
	}

	void nvme_config_write(sc_dt::uint64 addr, uint32_t data) {
		// Admin queue doorbells.
		if(addr >= REG_DOORBELL_BEGIN){
			const int dstrd = 4; // doorbell stride
			int offset = addr - REG_DOORBELL_BEGIN;
			
			// Calculate queue type and queue ID from offset
			uint32_t uiTemp, uiMask, sqid;
			uiTemp = offset / dstrd;
			uiMask = (uiTemp & 0x00000001); // 0 for submission queue tail doorbell
											// 1 for completion queue head doorbell
			sqid = (uiTemp >> 1);         // queue ID

			if(uiMask){
				ringCQHeadDoorbell(sqid, data);
				SC_REPORT_WARNING("qdma", "RING CQ TAIL DOORBELL");
			} else {
				ringSQTailDoorbell(sqid, data);
				SC_REPORT_WARNING("qdma", "RING SQ TAIL DOORBELL");
				lSQFIFO.push_back(read_sq_entry(sqid));
				handle_request();
			}
		} else {
			// non admin queue commands.  Primarily nvme config registers at the start of the BAR.
			nvme_config_register_write(addr, data);
		}
	}

	void nvme_config_register_write(sc_dt::uint64 addr, uint32_t data) {
		static DMAFunction empty = [](uint64_t, void *) {};
		
		switch(addr){
			case REG_CONTROLLER_CONFIG:
				
				registers.configuration &= 0xFF00000E;
        		registers.configuration |= (data & 0x00FFFFF1);

				// Update entry size
        		sqstride = (int)powf(2.f, (registers.configuration & 0x000F0000) >> 16);
        		cqstride = (int)powf(2.f, (registers.configuration & 0x00F00000) >> 20);
				// TODO(jhieb) should assert if these don't match the sizeof(sqentry) and sizeof(cqentry)

				// Update Memory Page Size
				cfgdata.memoryPageSizeOrder =
					((registers.configuration & 0x780) >> 7) + 11;  // CC.MPS + 12 - 1
				cfgdata.memoryPageSize =
					(int)powf(2.f, cfgdata.memoryPageSizeOrder + 1);

				// Update Arbitration Mechanism
				arbitration = (registers.configuration & 0x00003800) >> 11;

				// create the admin completion and submission queues.
				// Apply to admin queue
				if (ppCQueue[0]) {
					ppCQueue[0]->setBase(
						new PRPList(cfgdata, empty, nullptr,
									registers.adminCQueueBaseAddress,
									ppCQueue[0]->getSize() * cqstride, true),
						cqstride, registers.adminCQueueBaseAddress);
				}
				if (ppSQueue[0]) {
					ppSQueue[0]->setBase(
						new PRPList(cfgdata, empty, nullptr,
									registers.adminSQueueBaseAddress,
									ppSQueue[0]->getSize() * sqstride, true),
						sqstride, registers.adminSQueueBaseAddress);
				}
				// Shutdown notification
				if (registers.configuration & 0x0000C000) {
					registers.status &= 0xFFFFFFF2;  // RDY = 1
					registers.status |= 0x00000005;  // Shutdown processing occurring
					shutdownReserved = true;
				} else if(data & 0x00000001 ){
					// If CFG.EN = 1, Set CSTS.RDY = 1
					this->regs.u32[REG_CONTROLLER_STATUS >> 2] |= 0x00000001;
					// TODO(Jhieb) disabling CSTS for testing.
					//this->regs.u32[REG_CONTROLLER_STATUS >> 2] &= 0xFFFFFFFE;
				} else {
					// If CFG.EN = 0, Set CSTS.RDY = 0
					this->regs.u32[REG_CONTROLLER_STATUS >> 2] &= 0xFFFFFFFE;
				}
				break;
			case REG_ADMIN_QUEUE_ATTRIBUTE:
				registers.adminQueueAttributes &= 0xF000F000;
				registers.adminQueueAttributes |= (data & 0x0FFF0FFF);
			    break;
			case REG_ADMIN_CQUEUE_BASE_ADDR:
				registers.adminCQueueBaseAddress = data;
				adminQueueInited++;
				break;
			case REG_ADMIN_CQUEUE_BASE_ADDR + 4:
				// upper 32 bytes
				registers.adminCQueueBaseAddress = (static_cast<uint64_t>(data) << 32) | registers.adminCQueueBaseAddress;
				adminQueueInited++;
				break;
			case REG_ADMIN_SQUEUE_BASE_ADDR:
				registers.adminSQueueBaseAddress = data;
				adminQueueInited++;
				break;
			case REG_ADMIN_SQUEUE_BASE_ADDR + 4:
			    // upper 32 bytes
				registers.adminSQueueBaseAddress = (static_cast<uint64_t>(data) << 32) | registers.adminSQueueBaseAddress;
                adminQueueInited++;
                break;
			default:
			    SC_REPORT_WARNING("qdma", "unsupported register write");
				break;
		}


		// If we got all the admin queue details then create the initial queues.
		if (adminQueueInited == 4) {
			uint16_t entrySize = 0;
			adminQueueInited = 0;
			entrySize = ((registers.adminQueueAttributes & 0x0FFF0000) >> 16) + 1;
			ppCQueue[0] = new CQueue(0, true, 0, entrySize);
			entrySize = (registers.adminQueueAttributes & 0x0FFF) + 1;
			ppSQueue[0] = new SQueue(0, 0, 0, entrySize);
		}
	}

	void config_bar_b_transport(tlm::tlm_generic_payload &trans,
			sc_time &delay) {
		tlm::tlm_command cmd = trans.get_command();
		sc_dt::uint64 addr = trans.get_address();
		unsigned char *data = trans.get_data_ptr();
		unsigned int len = trans.get_data_length();
		unsigned char *byte_en = trans.get_byte_enable_ptr();
		unsigned int s_width = trans.get_streaming_width();
		uint32_t v = 0;

		if (byte_en || len > 4 || s_width < len) {
			goto err;
		}
		if (cmd == tlm::TLM_READ_COMMAND) {
			switch (addr >> 2) {
				case R_GLBL2_MISC_CAP:
					v = this->qdma_get_version();
					break;
				default:
					v = this->regs.u32[addr >> 2];
					break;
			}
			memcpy(data, &v, len);
		} else if (cmd == tlm::TLM_WRITE_COMMAND) {
			bool done = true;
			memcpy(&v, data, len);

			// TODO(jhieb) this shouldn't live in the qdma module but ok for now.
			nvme_config_write(addr, v);

			/* There is some differences in the register set for
			 * the cpm4 flavour, handle those register appart.  */
			if (this->is_cpm4()) {
			  switch (addr >> 2) {
			  case R_DMAP_SEL_INT_CIDX(1, 0) ...
			    R_DMAP_SEL_CMPT_CIDX(1, QDMA_QUEUE_COUNT):
				{
					int qid = (addr
					  - (R_DMAP_SEL_INT_CIDX(1, 0) << 2))
					  / 0x10;

					this->regs.u32[addr >> 2] = v;

					switch (addr % 0x10) {
						case 0x4:
							/* R_DMAP_SEL_H2C_DSC_PIDX(n) */
							this->run_mm_dma(qid,
									true);
							break;
						case 0x8:
							/* R_DMAP_SEL_C2H_DSC_PIDX(n) */
							this->run_mm_dma(qid,
									false);
							break;
						default:
							break;
					}
					break;
				}
			  case R_IND_CTXT_CMD(1):
					this->handle_ctxt_cmd(v);
					/* Drop the busy bit.  */
					this->regs.u32[addr >> 2] =
							v & 0xFFFFFFFE;
					break;
				default:
					done = false;
					break;
			  }
			} else {
			  switch (addr >> 2) {
			  case R_DMAP_SEL_INT_CIDX(0, 0) ...
			    R_DMAP_SEL_CMPT_CIDX(0, QDMA_QUEUE_COUNT):
				{
					int qid = (addr
					  - (R_DMAP_SEL_INT_CIDX(0, 0) << 2))
					  / 0x10;

					this->regs.u32[addr >> 2] = v;

					switch (addr % 0x10) {
						case 0x4:
							/* R_DMAP_SEL_H2C_DSC_PIDX(n) */
							this->run_mm_dma(qid,
									true);
							break;
						case 0x8:
							/* R_DMAP_SEL_C2H_DSC_PIDX(n) */
							this->run_mm_dma(qid,
									false);
							break;
						default:
							break;
					}
					break;
				}
			  case R_IND_CTXT_CMD(0):
					this->handle_ctxt_cmd(v);
					/* Drop the busy bit.  */
					this->regs.u32[addr >> 2] =
							v & 0xFFFFFFFE;
					break;
				default:
					done = false;
					break;
			  }
			}

			if (!done) {
				switch (addr >> 2) {
					case R_CONFIG_BLOCK_IDENT:
					case R_GLBL2_MISC_CAP:
						/* Read Only register.	*/
						break;
					case R_GLBL_INTR_CFG:
						/* W1C */
						if (v
						   & R_GLBL_INTR_CFG_INT_PEND) {
							v &=
						      ~R_GLBL_INTR_CFG_INT_PEND;
						}
						this->regs.u32[addr >> 2] = v;
						this->update_legacy_irq();
						break;
					default:
						this->regs.u32[addr >> 2] = v;
						break;
				}
			}
		} else {
			goto err;
		}

		trans.set_response_status(tlm::TLM_OK_RESPONSE);
		return;
err:
		SC_REPORT_WARNING("qdma",
				"unsupported read / write on the config bar");
		trans.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
		return;
	}

	union {
		struct {
			uint32_t config_block_ident;
		};
		uint32_t u32[0x18000];
	} regs;

	struct {
		uint32_t u32[0xA8];
	} axi_regs;

	/* Reset the IP.  */
	void reset(void) {
		this->regs.config_block_ident = 0x1FD30001;
		/* One bar mapped for PF0.  */
		this->regs.u32[R_GLBL2_PF_BARLITE_INT] = 0x01;
		this->regs.u32[R_GLBL2_CHANNEL_QDMA_CAP] =
			QDMA_QUEUE_COUNT;
		this->regs.u32[R_GLBL2_CHANNEL_MDMA] = 0x00030f0f;
		this->regs.u32[R_GLBL2_CHANNEL_FUNC_RET] = 0;
		this->regs.u32[R_GLBL2_PF_BARLITE_EXT] = 1
			<< QDMA_USER_BAR_ID;
	}

	void init_msix()
	{
		for (int i = 0; i < NR_QDMA_IRQ; i++) {
			sc_spawn(sc_bind(&qdma::msix_strobe,
				 this,
				 i));
		}
	}

	bool createNamespace(uint32_t nsid, Namespace::Information *info) {
		std::list<LPNRange> allocated;
		std::list<LPNRange> unallocated;

		// Allocate LPN
		uint64_t requestedLogicalPages =
			info->size / logicalPageSize * lbaSize[info->lbaFormatIndex];
		uint64_t unallocatedLogicalPages = totalLogicalPages - allocatedLogicalPages;

		if (requestedLogicalPages > unallocatedLogicalPages) {
			return false;
		}

		// Collect allocated slots
		for (auto &iter : lNamespaces) {
			allocated.push_back(iter->getInfo()->range);
		}

		// Sort
		allocated.sort([](const LPNRange &a, const LPNRange &b) -> bool {
			return a.slpn < b.slpn;
		});

		// Merge
		auto iter = allocated.begin();

		if (iter != allocated.end()) {
			auto next = iter;

			while (true) {
			next++;

			if (iter != allocated.end() || next == allocated.end()) {
				break;
			}

			if (iter->slpn + iter->nlp == next->slpn) {
				iter = allocated.erase(iter);
				next = iter;
			}
			else {
				iter++;
			}
			}
		}

		// Invert
		unallocated.push_back(LPNRange(0, totalLogicalPages));

		for (auto &iter : allocated) {
			// Split last item
			auto &last = unallocated.back();

			if (last.slpn <= iter.slpn &&
				last.slpn + last.nlp >= iter.slpn + iter.nlp) {
				unallocated.push_back(LPNRange(
					iter.slpn + iter.nlp, last.slpn + last.nlp - iter.slpn - iter.nlp));
				last.nlp = iter.slpn - last.slpn;
			}
			else {
				SC_REPORT_WARNING("qdma", "create namespace bug");
			}
		}

		// Allocated unallocated area to namespace
		for (auto iter = unallocated.begin(); iter != unallocated.end(); iter++) {
			if (iter->nlp >= requestedLogicalPages) {
			info->range = *iter;
			info->range.nlp = requestedLogicalPages;

			break;
			}
		}

		if (info->range.nlp == 0) {
			return false;
		}

		allocatedLogicalPages += requestedLogicalPages;

		// Fill Information
		info->sizeInByteL = requestedLogicalPages * logicalPageSize;
		info->sizeInByteH = 0;

		// Create namespace
		Namespace *pNS = new Namespace(cfgdata);
		pNS->setData(nsid, info);

		lNamespaces.push_back(pNS);
		/*
		debugprint(LOG_HIL_NVME,
					"NS %-5d| CREATE | LBA size %" PRIu32 " | Capacity %" PRIu64, nsid,
					info->lbaSize, info->size);
		*/

		lNamespaces.sort([](Namespace *lhs, Namespace *rhs) -> bool {
			return lhs->getNSID() < rhs->getNSID();
		});

		return true;
	}

	virtual bool is_cpm4() = 0;
	virtual uint32_t qdma_get_version() = 0;
	virtual bool irq_aggregation_enabled(int qid, bool h2c) = 0;
	/* Either get the MSIX Vector (in direct interrupt mode), or the IRQ
	 * Ring Index (in indirect interrupt mode).  */
	virtual int get_vec(int qid, bool h2c) = 0;
protected:
	SW_CTX *get_software_context(int qid, bool h2c)
	{
		return (SW_CTX *)this->queue_contexts[qid]
			[h2c ? QDMA_CTXT_SELC_DEC_SW_H2C
			     : QDMA_CTXT_SELC_DEC_SW_C2H].data;
	}

	qid2vec_ctx_cpm4 *get_qid2vec_context(int qid)
	{
		return (qid2vec_ctx_cpm4 *)(this->queue_contexts[qid]
			[QDMA_CTXT_SELC_FMAP_QID2VEC].data);
	}

	/* Current entry in a given interrupt ring.  */
	int irq_ring_entry_idx[QDMA_QUEUE_COUNT];
public:
	SC_HAS_PROCESS(qdma);
	sc_in<bool> rst;
	tlm_utils::simple_initiator_socket<qdma> card_bus;

	/* Interface to toward PCIE.  */
	tlm_utils::simple_target_socket<qdma> config_bar;
	tlm_utils::simple_target_socket<qdma> user_bar;
	tlm_utils::simple_initiator_socket<qdma> dma;
	sc_vector<sc_out<bool> > irq;

	qdma(sc_core::sc_module_name name) :
		rst("rst"),
		card_bus("card_initiator_socket"),
		config_bar("config_bar"),
		user_bar("user_bar"),
		dma("dma"),
		irq("irq", NR_QDMA_IRQ)
	{
		memset(&regs, 0, sizeof regs);

		SC_METHOD(reset);
		dont_initialize();
		sensitive << rst;

		config_bar.register_b_transport(this,
						&qdma::config_bar_b_transport);
		user_bar.register_b_transport(this,
			&qdma::axi_master_light_bar_b_transport);
		this->init_msix();

		// Nvme setup stuff.
		// Allocate array for Command Queues
		// MaxIOCQueue = 16
		// MaxIOSQueue = 16
		adminQueueInited = 0;
		ppCQueue = (CQueue **)calloc(cqsize+1, sizeof(CQueue *));
		ppSQueue = (SQueue **)calloc(sqsize+1, sizeof(SQueue *));

		// [Bits ] Name  : Description                     : Current Setting
		// [63:56] Reserved
		// [55:52] MPSMZX: Memory Page Size Maximum        : 2^14 Bytes
		// [51:48] MPSMIN: Memory Page Size Minimum        : 2^12 Bytes
		// [47:45] Reserved
		// [44:37] CSS   : Command Sets Supported          : NVM command set
		// [36:36] NSSRS : NVM Subsystem Reset Supported   : No
		// [35:32] DSTRD : Doorbell Stride                 : 0 (4 bytes)
		// [31:24] TO    : Timeout                         : 40 * 500ms
		// [23:19] Reserved
		// [18:17] AMS   : Arbitration Mechanism Supported : Weighted Round Robin
		// [16:16] CQR   : Contiguous Queues Required      : Yes
		// [15:00] MQES  : Maximum Queue Entries Supported : 4096 Entries
		registers.capabilities = 0x0020002028010FFF;
		registers.version = 0x00010201;  // NVMe 1.2.1

		cfgdata.maxQueueEntry = (registers.capabilities & 0xFFFF) + 1;

		allocatedLogicalPages = 0;

		// TODO(jhieb) hard coded config for now.
		nandParameters.channel = 8;
		nandParameters.package = 4;
		nandParameters.die = 2;
		nandParameters.plane = 2;
		nandParameters.block = 512;
		nandParameters.page = 512;
		nandParameters.pageSize = 16384;

		nandParameters.superBlock = nandParameters.block * nandParameters.channel * nandParameters.package * nandParameters.die * nandParameters.plane;
		nandParameters.pageInSuperPage = 1 * nandParameters.channel * nandParameters.package * nandParameters.die * nandParameters.plane;
		// nandParameters.superPageSize = nandParameters.superPageSize; // ?

		ftlParameter.totalPhysicalBlocks = nandParameters.superBlock;
		ftlParameter.totalLogicalBlocks = nandParameters.superBlock * (1 - overProvisioningRatio);
		ftlParameter.pagesInBlock = nandParameters.page;
		ftlParameter.pageSize = nandParameters.pageSize;
		// TODO(jhieb) what is this?
		ftlParameter.pageCountToMaxPerf = nandParameters.superBlock / nandParameters.block;
		
		totalLogicalPages = ftlParameter.totalLogicalBlocks * ftlParameter.pagesInBlock;
		logicalPageSize = ftlParameter.pageSize;

		if (nNamespaces > 0) {
			Namespace::Information info;
			uint64_t totalSize;

			for (info.lbaFormatIndex = 0; info.lbaFormatIndex < nLBAFormat;info.lbaFormatIndex++) {
				if (lbaSize[info.lbaFormatIndex] == lba) {
					info.lbaSize = lbaSize[info.lbaFormatIndex];
					break;
				}
			}

			if (info.lbaFormatIndex == nLBAFormat) {
				SC_REPORT_WARNING("qdma", "Failed to setting LBA size (512B ~ 4KB)");
			}

			// Fill Namespace information
			totalSize = totalLogicalPages * logicalPageSize / lba;
			info.dataProtectionSettings = 0x00;
			info.namespaceSharingCapabilities = 0x00;

			for (uint16_t i = 0; i < nNamespaces; i++) {
				info.size = totalSize / nNamespaces;
				info.capacity = info.size;

				if (createNamespace(NSID_LOWEST + i, &info)) {
					lNamespaces.back()->attach(true);
				}
				else {
					SC_REPORT_WARNING("qdma", "Failed to create namespace");
				}
			}
		}
	}
};

class qdma_cpm5:
  public qdma<struct sw_ctx_cpm5,
	      struct intr_ctx_cpm5,
	      struct intr_ring_entry_cpm5>
{
private:
	bool is_cpm4()
	{
		return false;
	}

	bool irq_aggregation_enabled(int qid, bool h2c)
	{
		struct sw_ctx_cpm5 *sw_ctx = this->get_software_context(qid,
									h2c);
		return (sw_ctx->int_aggr != 0);
	}

	int get_vec(int qid, bool h2c)
	{
		struct sw_ctx_cpm5 *sw_ctx
			= this->get_software_context(qid, h2c);
		return sw_ctx->vec;
	}

	uint32_t qdma_get_version()
	{
		return QDMA_VERSION_FIELD(QDMA_CPM5_DEVICE_ID, 1, 0);
	}
public:
	qdma_cpm5(sc_core::sc_module_name name):
	  qdma(name)
	{
	}
};

class qdma_cpm4:
  public qdma<struct sw_ctx_cpm4,
	      struct intr_ctx_cpm4,
	      struct intr_ring_entry_cpm4>
{
private:
	bool is_cpm4()
	{
		return true;
	}

	bool irq_aggregation_enabled(int qid, bool h2c)
	{
		struct qid2vec_ctx_cpm4 *qid2vec_ctx =
			this->get_qid2vec_context(qid);

		return h2c ? qid2vec_ctx->h2c_en_coal
			   : qid2vec_ctx->c2h_en_coal;
	}

	int get_vec(int qid, bool h2c)
	{
		struct qid2vec_ctx_cpm4 *qid2vec_ctx
			= this->get_qid2vec_context(qid);

		return h2c ? qid2vec_ctx->h2c_vector : qid2vec_ctx->c2h_vector;
	}

	uint32_t qdma_get_version()
	{
		return QDMA_VERSION_FIELD(QDMA_CPM4_DEVICE_ID, 0, 0);
	}
public:
	qdma_cpm4(sc_core::sc_module_name name):
	  qdma(name)
	{
	}
};

#endif /* __PCI_QDMA_H__ */
