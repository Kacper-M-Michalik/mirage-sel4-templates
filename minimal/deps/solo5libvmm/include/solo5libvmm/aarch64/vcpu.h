#pragma once

#include <stdint.h>
#include <stddef.h>

#define __AC(X,Y)               (X##Y)
#define _AC(X,Y)                __AC(X,Y)
#define _AT(T,X)                ((T)(X))
#define _BITUL(x)               (_AC(1,UL) << (x))
#define _BITULL(x)              (_AC(1,ULL) << (x))
#define ATTRINDX(t)     (_AC(t, UL) << 2)
#define GENMASK32(h, l) (((~0U) << (l)) & (~0U >> (31 - (h))))
#define GENMASK64(h, l) (((~0UL) << (l)) & (~0UL >> (63 - (h))))

// HVT DEFS
#define AARCH64_PGD_PGT_BASE     _AC(0x1000, UL)
#define AARCH64_PGD_PGT_SIZE     _AC(0x1000, UL)
#define AARCH64_PUD_PGT_BASE     _AC(0x2000, UL)
#define AARCH64_PUD_PGT_SIZE     _AC(0x1000, UL)
#define AARCH64_PMD_PGT_BASE     _AC(0x3000, UL)
#define AARCH64_PMD_PGT_SIZE     _AC(0x4000, UL)
#define AARCH64_PTE_PGT_BASE     _AC(0x7000, UL)
#define AARCH64_PTE_PGT_SIZE     _AC(0x1000, UL)
#define AARCH64_BOOT_INFO        _AC(0x10000, UL)
#define AARCH64_GUEST_MIN_BASE   _AC(0x100000, UL)
#define AARCH64_MMIO_BASE        _AC(0x100000000, UL)
#define AARCH64_MMIO_SZ          _AC(0x40000000, UL)
#define AARCH64_GUEST_BLOCK_SIZE _AC(0x200000, UL)
#define AARCH64_PGT_MAP_START	 AARCH64_BOOT_INFO

/*
 * Hardware page table definitions.
 */
#define PAGE_SHIFT  12
#define PAGE_SIZE   (1 << (PAGE_SHIFT))

#define PGT_DESC_TYPE_TABLE (_AC(3, UL) << 0)
#define PGT_DESC_TYPE_SECT  (_AC(1, UL) << 0)
#define PGT_DESC_TYPE_PAGE  (_AC(3, UL) << 0)

#define SECT_VALID      (_AC(1, UL) << 0)
#define SECT_USER       (_AC(1, UL) << 6)     /* AP[1] */
#define SECT_RDONLY     (_AC(1, UL) << 7)     /* AP[2] */
#define SECT_S          (_AC(3, UL) << 8)
#define SECT_AF         (_AC(1, UL) << 10)
#define SECT_NG         (_AC(1, UL) << 11)
#define SECT_CONT       (_AC(1, UL) << 52)
#define SECT_PXN        (_AC(1, UL) << 53)
#define SECT_UXN        (_AC(1, UL) << 54)

#define MT_DEVICE_nGnRnE    0
#define MT_DEVICE_nGnRE     1
#define MT_DEVICE_GRE       2
#define MT_NORMAL_NC        3
#define MT_NORMAL           4
#define MT_NORMAL_WT        5

#define PROT_SECT_DEFAULT       	(PGT_DESC_TYPE_SECT | SECT_AF | SECT_S)
#define PROT_SECT_NORMAL        	(PROT_SECT_DEFAULT | SECT_PXN | SECT_UXN | ATTRINDX(MT_NORMAL))
#define PROT_SECT_NORMAL_EXEC   	(PROT_SECT_DEFAULT | SECT_UXN | ATTRINDX(MT_NORMAL))
#define PROT_SECT_DEVICE_nGnRE  	(PROT_SECT_DEFAULT | SECT_PXN | SECT_UXN | ATTRINDX(MT_DEVICE_nGnRE))

#define PROT_PAGE_DEFAULT       	(PGT_DESC_TYPE_PAGE | SECT_AF | SECT_S)
#define PROT_PAGE_DEFAULT_NORMAL  	(PROT_PAGE_DEFAULT | ATTRINDX(MT_NORMAL))
#define PROT_PAGE_DEFAULT_DEVICE   	(PROT_PAGE_DEFAULT | ATTRINDX(MT_DEVICE_nGnRE))
#define PROT_PAGE_NORMAL    		(PROT_PAGE_DEFAULT_NORMAL | SECT_PXN | SECT_UXN)
#define PROT_PAGE_NORMAL_RO    		(PROT_PAGE_DEFAULT_NORMAL | SECT_PXN | SECT_UXN | SECT_RDONLY)
#define PROT_PAGE_NORMAL_EXEC   	(PROT_PAGE_DEFAULT_NORMAL | SECT_UXN)
#define PROT_PAGE_NORMAL_EXEC_RO    (PROT_PAGE_DEFAULT_NORMAL | SECT_UXN | SECT_RDONLY)
#define PROT_PAGE_DEVICE_nGnRE  	(PROT_PAGE_DEFAULT_DEVICE | SECT_PXN | SECT_UXN)

/*
 * Define the MMU transfer block size:
 * PGD entry size: 512GB -- Translation Level 0
 * PUD entry size: 1GB   -- Translation Level 1
 * PMD entry size: 2MB   -- Translation Level 2
 * PTE entry size: 4KB   -- Translation Level 3
 */
#define PGD_SHIFT	39
#define PGD_SIZE	(_AC(1, UL) << PGD_SHIFT)
#define PGD_MASK	(~(PGD_SIZE-1))
#define PUD_SHIFT	30
#define PUD_SIZE	(_AC(1, UL) << PUD_SHIFT)
#define PUD_MASK	(~(PUD_SIZE-1))
#define PMD_SHIFT	21
#define PMD_SIZE	(_AC(1, UL) << PMD_SHIFT)
#define PMD_MASK	(~(PMD_SIZE-1))

/* AArch64 SPSR bits */
#define PSR_F_BIT	0x00000040
#define PSR_I_BIT	0x00000080
#define PSR_A_BIT	0x00000100
#define PSR_D_BIT	0x00000200
#define PSR_MODE_EL1h	0x00000005

#define AARCH64_PSTATE_INIT (PSR_D_BIT | PSR_A_BIT | PSR_I_BIT | PSR_F_BIT | PSR_MODE_EL1h)

#define _FPEN_NOTRAP        0x3
#define _FPEN_SHIFT         20
#define _FPEN_MASK          GENMASK32(21, 20)

#define MAIR(attr, mt)      (_AC(attr, UL) << ((mt) * 8))

#define MAIR_EL1_INIT       \
        MAIR(0x00, MT_DEVICE_nGnRnE) | MAIR(0x04, MT_DEVICE_nGnRE) | \
        MAIR(0x0C, MT_DEVICE_GRE) | MAIR(0x44, MT_NORMAL_NC) | \
        MAIR(0xFF, MT_NORMAL) | MAIR(0xBB, MT_NORMAL_WT)

#define TCR_T0SZ_OFFSET     0
#define TCR_T1SZ_OFFSET     16
#define TCR_T0SZ(x)         ((_AC(64, UL) - (x)) << TCR_T0SZ_OFFSET)
#define TCR_T1SZ(x)         ((_AC(64, UL) - (x)) << TCR_T1SZ_OFFSET)
#define TCR_TxSZ(x)         (TCR_T0SZ(x) | TCR_T1SZ(x))

#define TCR_IRGN0_SHIFT     8
#define TCR_IRGN0_WBWA      (_AC(1, UL) << TCR_IRGN0_SHIFT)
#define TCR_IRGN1_SHIFT     24
#define TCR_IRGN1_WBWA      (_AC(1, UL) << TCR_IRGN1_SHIFT)
#define TCR_IRGN_WBWA       (TCR_IRGN0_WBWA | TCR_IRGN1_WBWA)

#define TCR_ORGN0_SHIFT     10
#define TCR_ORGN0_WBWA      (_AC(1, UL) << TCR_ORGN0_SHIFT)
#define TCR_ORGN1_SHIFT     26
#define TCR_ORGN1_WBWA      (_AC(1, UL) << TCR_ORGN1_SHIFT)
#define TCR_ORGN_WBWA       (TCR_ORGN0_WBWA | TCR_ORGN1_WBWA)

#define TCR_SH0_SHIFT       12
#define TCR_SH0_INNER       (_AC(3, UL) << TCR_SH0_SHIFT)
#define TCR_SH1_SHIFT       28
#define TCR_SH1_INNER       (_AC(3, UL) << TCR_SH1_SHIFT)
#define TCR_SHARED          (TCR_SH0_INNER | TCR_SH1_INNER)

#define TCR_TG0_SHIFT       14
#define TCR_TG0_4K          (_AC(0, UL) << TCR_TG0_SHIFT)
#define TCR_TG1_SHIFT       30
#define TCR_TG1_4K          (_AC(2, UL) << TCR_TG1_SHIFT)

#define TCR_ASID16          (_AC(1, UL) << 36)
#define TCR_TBI0            (_AC(1, UL) << 37)
#define TCR_IPS_1TB         (_AC(2, UL) << 32)

#define TCR_TG_FLAGS        TCR_TG0_4K | TCR_TG1_4K
#define TCR_CACHE_FLAGS     TCR_IRGN_WBWA | TCR_ORGN_WBWA

#define VA_BITS     40
#define VA_SIZE     (_AC(1, UL) << VA_BITS)
#define PA_SIZE     (_AC(1, UL) << VA_BITS)

#define TCR_EL1_INIT        \
            TCR_TxSZ(VA_BITS) | TCR_CACHE_FLAGS | TCR_SHARED | \
            TCR_TG_FLAGS | TCR_ASID16 | TCR_TBI0 | TCR_IPS_1TB

#define _SCTLR_M            _BITUL(0)
#define _SCTLR_C            _BITUL(2)
#define _SCTLR_I            _BITUL(12)

uint64_t aarch64_get_counter_frequency(void);

void setup_memory_mapping(uint8_t* mem, uint64_t mem_size);
void setup_system_registers(size_t vcpu_id, uint64_t sp);
void setup_tcb_registers(size_t vcpu_id, uint64_t p_entry, uint64_t boot_info_addr);

void vcpu_print_tcb_regs(size_t vcpu_id);
void vcpu_print_sys_regs(size_t vcpu_id);
void vcpu_reset_regs(size_t vcpu_id);