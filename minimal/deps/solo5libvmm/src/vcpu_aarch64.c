#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <microkit.h>
#include <solo5libvmm/vcpu_aarch64.h>
#include <util.h>
#include <hvt_abi.h>

uint64_t aarch64_get_counter_frequency(void)
{
    uint64_t frq;

    __asm__ __volatile__("mrs %0, cntfrq_el0" : "=r" (frq):: "memory");

    return frq;
}

void enable_guest_float(size_t vcpu_id)
{
    seL4_Word data;

    LOG_VMM("Enabling the floating-point and Advanced SIMD registers\n");
    data = microkit_vcpu_arm_read_reg(vcpu_id, seL4_VCPUReg_CPACR);
    LOG_VMM("data: %ld\n", data);
    data &= ~(_FPEN_MASK);
    data |= (_FPEN_NOTRAP << _FPEN_SHIFT);    
    LOG_VMM("final data: %ld\n", data);
    microkit_vcpu_arm_write_reg(vcpu_id, seL4_VCPUReg_CPACR, data);
}

void enable_guest_mmu(size_t vcpu_id)
{
    LOG_VMM("Setting up Memory Attribute Indirection Register\n");
    microkit_vcpu_arm_write_reg(vcpu_id, seL4_VCPUReg_MAIR, MAIR_EL1_INIT);   

    LOG_VMM("Setting up Translation Control Register\n");
    microkit_vcpu_arm_write_reg(vcpu_id, seL4_VCPUReg_TCR, TCR_EL1_INIT);

    /*
     * Setup Translation Table Base Register 0 EL1. The translation range
     * doesn't exceed the 0 ~ 1^64. So the TTBR0_EL1 is enough.
     */
    LOG_VMM("Setting up Translation Table Base Register 0 EL1\n");
    microkit_vcpu_arm_write_reg(vcpu_id, seL4_VCPUReg_TTBR0, AARCH64_PGD_PGT_BASE);

    /* Enable MMU and I/D Cache for EL1 */
    LOG_VMM("Setting up System Control Register EL1\n");
    microkit_vcpu_arm_write_reg(vcpu_id, seL4_VCPUReg_SCTLR, (_SCTLR_M | _SCTLR_C | _SCTLR_I));
}

void setup_core_registers(size_t vcpu_id, size_t mem_size, uint64_t p_entry)
{
    /* Set default PSTATE flags to SPSR_EL1 */
    LOG_VMM("Initializing spsr[EL1]\n");
    microkit_vcpu_arm_write_reg(vcpu_id, seL4_VCPUReg_SPSR_EL1, AARCH64_PSTATE_INIT);    

    /*
     * Set Stack Pointer for Guest. ARM64 require stack be 16-bytes alignment by default.
     * TODO: CHECK IF -16 IS CORRECT
     */
    LOG_VMM("Initializing sp[EL1]\n");
    microkit_vcpu_arm_write_reg(vcpu_id, seL4_VCPUReg_SP_EL1, mem_size - 16);        

    //TODO: get rid of constants
    seL4_Error err;
    seL4_UserContext context = {0};
    context.pc = (seL4_Word)p_entry;
    context.x0 = 0x10000;
    
    err = seL4_TCB_WriteRegisters(
        BASE_VM_TCB_CAP + vcpu_id,
        seL4_False,
        0,
        4, // Due to ordering of UserContext, need to do 4
        &context
    );
    assert(err == seL4_NoError);
}

void setup_memory_mapping(uint8_t *mem, uint64_t mem_size)
{
    uint64_t paddr, pmd_paddr;
    uint64_t *pgd = (uint64_t *)(mem + AARCH64_PGD_PGT_BASE);
    uint64_t *pud = (uint64_t *)(mem + AARCH64_PUD_PGT_BASE);
    uint64_t *pmd = (uint64_t *)(mem + AARCH64_PMD_PGT_BASE);
    uint64_t *pte = (uint64_t *)(mem + AARCH64_PTE_PGT_BASE);

    /*
     * In order to keep consistency with x86_64, we limit hvt_hypercall only
     * to support sending 32-bit pointers. So we limit the guest to support
     * only 4GB memory. This will avoid using additional code to guarantee the
     * hypercall parameters are using the memory below 4GB.
     *
     * Address above 4GB is using for MMIO space now. This would be changed
     * easily if the design of hvt_hypercall would be changed in the future.
     */
    assert((mem_size & (AARCH64_GUEST_BLOCK_SIZE -1)) == 0);
    assert(mem_size <= AARCH64_MMIO_BASE);
    assert(mem_size >= AARCH64_GUEST_BLOCK_SIZE);

    /* Zero all page tables */
    memset(pgd, 0, AARCH64_PGD_PGT_SIZE);
    memset(pud, 0, AARCH64_PUD_PGT_SIZE);
    memset(pmd, 0, AARCH64_PMD_PGT_SIZE);
    memset(pte, 0, AARCH64_PTE_PGT_SIZE);

    /* Map first 2MB block in pte table */
    for (paddr = 0; paddr < AARCH64_GUEST_BLOCK_SIZE;
         paddr += PAGE_SIZE, pte++) {
        /*
         * Leave all pages below AARCH64_PGT_MAP_START unmapped in the guest.
         * This includes the zero page and the guest's page tables.
         */
        if (paddr < AARCH64_PGT_MAP_START)
            continue;

        /*
         * Map the remainder of the pages below AARCH64_GUEST_MIN_BASE
         * as read-only; these are used for input from hvt to the guest
         * only, with the rest reserved for future use.
         */
        if (paddr < AARCH64_GUEST_MIN_BASE)
            *pte = paddr | PROT_PAGE_NORMAL_RO;
        else
            *pte = paddr | PROT_PAGE_NORMAL_EXEC;
    }
    assert(paddr == AARCH64_GUEST_BLOCK_SIZE);

    /* Link pte table to pmd[0] */
    *pmd++ = AARCH64_PTE_PGT_BASE | PGT_DESC_TYPE_TABLE;

    /* Mapping left memory by 2MB block in pmd table */
    for (; paddr < mem_size; paddr += PMD_SIZE, pmd++)
        *pmd = paddr | PROT_SECT_NORMAL_EXEC;

    /* Link pmd tables (PMD0, PMD1, PMD2, PMD3) to pud[0] ~ pud[3] */
    pmd_paddr = AARCH64_PMD_PGT_BASE;
    for (paddr = 0; paddr < mem_size;
         paddr += PUD_SIZE, pud++, pmd_paddr += PAGE_SIZE)
        *pud = pmd_paddr | PGT_DESC_TYPE_TABLE;

    /* RAM address should not exceed MMIO_BASE */
    assert(paddr <= AARCH64_MMIO_BASE);
    
    /* Mapping MMIO */
    pud += ((AARCH64_MMIO_BASE - paddr) >> PUD_SHIFT);
    for (paddr = AARCH64_MMIO_BASE;
         paddr < AARCH64_MMIO_BASE + AARCH64_MMIO_SZ;
         paddr += PUD_SIZE, pud++)
        *pud = paddr | PROT_SECT_DEVICE_nGnRE;

    /* Link pud table to pgd[0] */
    *pgd = AARCH64_PUD_PGT_BASE | PGT_DESC_TYPE_TABLE;
}