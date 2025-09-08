#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <microkit.h>
#include <solo5libvmm/aarch64/vcpu.h>
#include <solo5libvmm/util.h>

uint64_t aarch64_get_counter_frequency(void)
{
    uint64_t frq;

    __asm__ __volatile__("mrs %0, cntfrq_el0" : "=r" (frq):: "memory");

    return frq;
}

void setup_system_registers(size_t vcpu_id, uint64_t sp)
{
    // Enable Float and SIMD
    LOG_VMM("Enabling the floating-point and Advanced SIMD registers\n");
    //data &= ~(_FPEN_MASK); // HVT KVM reads first then does this, why? By default is 0
    seL4_Word data = (_FPEN_NOTRAP << _FPEN_SHIFT);    
    LOG_VMM("Should be: %x\n", data);
    microkit_vcpu_arm_write_reg(vcpu_id, seL4_VCPUReg_CPACR, data);


    // Enable and setup MMU
    LOG_VMM("Setting up Memory Attribute Indirection Register\n");
    LOG_VMM("Should be: %x\n", MAIR_EL1_INIT);
    microkit_vcpu_arm_write_reg(vcpu_id, seL4_VCPUReg_MAIR, MAIR_EL1_INIT);   

    LOG_VMM("Setting up Translation Control Register\n");
    LOG_VMM("Should be: %x\n", TCR_EL1_INIT);
    microkit_vcpu_arm_write_reg(vcpu_id, seL4_VCPUReg_TCR, TCR_EL1_INIT);

    // Setup Translation Table Base Register 0 EL1. The translation range doesn't exceed the 0 ~ 1^64. So the TTBR0_EL1 is enough
    LOG_VMM("Setting up Translation Table Base Register 0 EL1\n");
    LOG_VMM("Should be: %x\n", AARCH64_PGD_PGT_BASE);
    microkit_vcpu_arm_write_reg(vcpu_id, seL4_VCPUReg_TTBR0, AARCH64_PGD_PGT_BASE);

    // Enable MMU and I/D Cache for EL1
    LOG_VMM("Setting up System Control Register EL1\n");
    LOG_VMM("Should be: %x\n", (_SCTLR_M | _SCTLR_C | _SCTLR_I));
    microkit_vcpu_arm_write_reg(vcpu_id, seL4_VCPUReg_SCTLR, (_SCTLR_M | _SCTLR_C | _SCTLR_I));


    // Setup virtualised sp and spsr    
    LOG_VMM("Initializing spsr[EL1]\n");
    LOG_VMM("Should be: %x\n", AARCH64_PSTATE_INIT);
    microkit_vcpu_arm_write_reg(vcpu_id, seL4_VCPUReg_SPSR_EL1, AARCH64_PSTATE_INIT);    

    // ARM64 requires 16 byte alignment for sp, also sp takes 16 bytes up from pointed address, so we do -16 to make stack point to valid 16 bytes
    assert(sp % 16 == 0);
    LOG_VMM("Initializing sp[EL1]\n");
    LOG_VMM("Should be: %x\n", sp - 16);
    microkit_vcpu_arm_write_reg(vcpu_id, seL4_VCPUReg_SP_EL1, sp - 16);       
}

void setup_tcb_registers(size_t vcpu_id, uint64_t p_entry, uint64_t boot_info_addr)
{
    seL4_Error err;
    seL4_UserContext context = {0};
    context.pc = (seL4_Word)p_entry;
    context.x0 = (seL4_Word)boot_info_addr;
    context.spsr = 0b0101; // Set to EL1h, this spsr is for sel4, not the virtualised spsr
    
    err = seL4_TCB_WriteRegisters(
        BASE_VM_TCB_CAP + vcpu_id,
        seL4_False,
        0,
        4, // Due to ordering of UserContext, need to set 4 fields
        &context
    );
    assert(err == seL4_NoError);
}

void setup_memory_mapping(uint8_t* mem, uint64_t mem_size)
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
    for (paddr = 0; paddr < mem_size; paddr += PUD_SIZE, pud++, pmd_paddr += PAGE_SIZE)
        *pud = pmd_paddr | PGT_DESC_TYPE_TABLE;

    /* RAM address should not exceed MMIO_BASE */
    assert(paddr <= AARCH64_MMIO_BASE);
    
    /* Mapping MMIO */
    pud += ((AARCH64_MMIO_BASE - paddr) >> PUD_SHIFT);
    for (paddr = AARCH64_MMIO_BASE; paddr < AARCH64_MMIO_BASE + AARCH64_MMIO_SZ; paddr += PUD_SIZE, pud++)
        *pud = paddr | PROT_SECT_DEVICE_nGnRE;

    /* Link pud table to pgd[0] */
    *pgd = AARCH64_PUD_PGT_BASE | PGT_DESC_TYPE_TABLE;
}

void vcpu_print_tcb_regs(size_t vcpu_id) 
{
    seL4_UserContext regs;
    seL4_Error err = seL4_TCB_ReadRegisters(BASE_VM_TCB_CAP + vcpu_id, seL4_False, 0, sizeof(seL4_UserContext) / sizeof(seL4_Word), &regs);
    assert(err == seL4_NoError);
    if (err != seL4_NoError) 
    {
        LOG_VMM("Failed to read TCB registers\n");
        return;
    }

    LOG_VMM("Dumping VCPU (ID 0x%lx) TCB registers:\n", vcpu_id);
    
    printf("    pc: 0x%016lx\n", regs.pc);
    printf("    sp: 0x%016lx\n", regs.sp);
    printf("    spsr: 0x%016lx\n", regs.spsr); 
    printf("    tpidr_el0: 0x%016lx\n", regs.tpidr_el0);
    printf("    tpidrro_el0: 0x%016lx\n", regs.tpidrro_el0);
    printf("    x0: 0x%016lx\n", regs.x0);
    printf("    x1: 0x%016lx\n", regs.x1);
    printf("    x2: 0x%016lx\n", regs.x2);
    printf("    x3: 0x%016lx\n", regs.x3);
    printf("    x4: 0x%016lx\n", regs.x4);
    printf("    x5: 0x%016lx\n", regs.x5);
    printf("    x6: 0x%016lx\n", regs.x6);
    printf("    x7: 0x%016lx\n", regs.x7);
    printf("    x8: 0x%016lx\n", regs.x8); 
    printf("    x9: 0x%016lx\n", regs.x9);
    printf("    x10: 0x%016lx\n", regs.x10);
    printf("    x11: 0x%016lx\n", regs.x11);
    printf("    x12: 0x%016lx\n", regs.x12);
    printf("    x13: 0x%016lx\n", regs.x13);
    printf("    x14: 0x%016lx\n", regs.x14);
    printf("    x15: 0x%016lx\n", regs.x15);
    printf("    x16: 0x%016lx\n", regs.x16);
    printf("    x17: 0x%016lx\n", regs.x17);
    printf("    x18: 0x%016lx\n", regs.x18);
    printf("    x19: 0x%016lx\n", regs.x19);
    printf("    x20: 0x%016lx\n", regs.x20);
    printf("    x21: 0x%016lx\n", regs.x21);
    printf("    x22: 0x%016lx\n", regs.x22);
    printf("    x23: 0x%016lx\n", regs.x23);
    printf("    x24: 0x%016lx\n", regs.x24);
    printf("    x25: 0x%016lx\n", regs.x25);
    printf("    x26: 0x%016lx\n", regs.x26);
    printf("    x27: 0x%016lx\n", regs.x27);
    printf("    x28: 0x%016lx\n", regs.x28);
    printf("    x29: 0x%016lx\n", regs.x29);
    printf("    x30: 0x%016lx\n", regs.x30);
}

void vcpu_print_sys_regs(size_t vcpu_id) 
{
    LOG_VMM("Dumping VCPU (ID 0x%lx) system registers:\n", vcpu_id);

    printf("    sctlr: 0x%016lx\n", microkit_vcpu_arm_read_reg(vcpu_id, seL4_VCPUReg_SCTLR));
    printf("    ttbr0: 0x%016lx\n", microkit_vcpu_arm_read_reg(vcpu_id, seL4_VCPUReg_TTBR0));
    printf("    ttbr1: 0x%016lx\n", microkit_vcpu_arm_read_reg(vcpu_id, seL4_VCPUReg_TTBR1));
    printf("    tcr: 0x%016lx\n", microkit_vcpu_arm_read_reg(vcpu_id, seL4_VCPUReg_TCR));
    printf("    mair: 0x%016lx\n", microkit_vcpu_arm_read_reg(vcpu_id, seL4_VCPUReg_MAIR));
    printf("    amair: 0x%016lx\n", microkit_vcpu_arm_read_reg(vcpu_id, seL4_VCPUReg_AMAIR));
    printf("    cidr: 0x%016lx\n", microkit_vcpu_arm_read_reg(vcpu_id, seL4_VCPUReg_CIDR));
    printf("    actlr: 0x%016lx\n", microkit_vcpu_arm_read_reg(vcpu_id, seL4_VCPUReg_ACTLR));
    printf("    cpacr: 0x%016lx\n", microkit_vcpu_arm_read_reg(vcpu_id, seL4_VCPUReg_CPACR));
    printf("    afsr0: 0x%016lx\n", microkit_vcpu_arm_read_reg(vcpu_id, seL4_VCPUReg_AFSR0));
    printf("    afsr1: 0x%016lx\n", microkit_vcpu_arm_read_reg(vcpu_id, seL4_VCPUReg_AFSR1));
    printf("    esr: 0x%016lx\n", microkit_vcpu_arm_read_reg(vcpu_id, seL4_VCPUReg_ESR));
    printf("    far: 0x%016lx\n", microkit_vcpu_arm_read_reg(vcpu_id, seL4_VCPUReg_FAR));
    printf("    isr: 0x%016lx\n", microkit_vcpu_arm_read_reg(vcpu_id, seL4_VCPUReg_ISR));
    printf("    vbar: 0x%016lx\n", microkit_vcpu_arm_read_reg(vcpu_id, seL4_VCPUReg_VBAR));
    printf("    tpidr_el1: 0x%016lx\n", microkit_vcpu_arm_read_reg(vcpu_id, seL4_VCPUReg_TPIDR_EL1));
    printf("    vmpidr_el2: 0x%016lx\n", microkit_vcpu_arm_read_reg(vcpu_id, seL4_VCPUReg_VMPIDR_EL2));
    printf("    sp_el1: 0x%016lx\n", microkit_vcpu_arm_read_reg(vcpu_id, seL4_VCPUReg_SP_EL1));
    printf("    elr_el1: 0x%016lx\n", microkit_vcpu_arm_read_reg(vcpu_id, seL4_VCPUReg_ELR_EL1));
    printf("    spsr_el1: 0x%016lx\n", microkit_vcpu_arm_read_reg(vcpu_id, seL4_VCPUReg_SPSR_EL1));
    printf("    cntv_ctl: 0x%016lx\n", microkit_vcpu_arm_read_reg(vcpu_id, seL4_VCPUReg_CNTV_CTL));
    printf("    cntv_cval: 0x%016lx\n", microkit_vcpu_arm_read_reg(vcpu_id, seL4_VCPUReg_CNTV_CVAL));
    printf("    cntvoff: 0x%016lx\n", microkit_vcpu_arm_read_reg(vcpu_id, seL4_VCPUReg_CNTVOFF));
    printf("    cntkctl_el1: 0x%016lx\n", microkit_vcpu_arm_read_reg(vcpu_id, seL4_VCPUReg_CNTKCTL_EL1));
}

void vcpu_reset_regs(size_t vcpu_id)
{
    // Reset TCB registers
    seL4_UserContext regs = {0};
    seL4_Error err = seL4_TCB_WriteRegisters(BASE_VM_TCB_CAP + vcpu_id, seL4_False, 0, sizeof(seL4_UserContext)/sizeof(seL4_Word), &regs);
    assert(err == seL4_NoError);

    // Reset system registers
    microkit_vcpu_arm_write_reg(vcpu_id, seL4_VCPUReg_SCTLR, 0);
    microkit_vcpu_arm_write_reg(vcpu_id, seL4_VCPUReg_TTBR0, 0);
    microkit_vcpu_arm_write_reg(vcpu_id, seL4_VCPUReg_TTBR1, 0);
    microkit_vcpu_arm_write_reg(vcpu_id, seL4_VCPUReg_TCR, 0);
    microkit_vcpu_arm_write_reg(vcpu_id, seL4_VCPUReg_MAIR, 0);
    microkit_vcpu_arm_write_reg(vcpu_id, seL4_VCPUReg_AMAIR, 0);
    microkit_vcpu_arm_write_reg(vcpu_id, seL4_VCPUReg_CIDR, 0);
    microkit_vcpu_arm_write_reg(vcpu_id, seL4_VCPUReg_ACTLR, 0);
    microkit_vcpu_arm_write_reg(vcpu_id, seL4_VCPUReg_CPACR, 0);
    microkit_vcpu_arm_write_reg(vcpu_id, seL4_VCPUReg_AFSR0, 0);
    microkit_vcpu_arm_write_reg(vcpu_id, seL4_VCPUReg_AFSR1, 0);
    microkit_vcpu_arm_write_reg(vcpu_id, seL4_VCPUReg_ESR, 0);
    microkit_vcpu_arm_write_reg(vcpu_id, seL4_VCPUReg_FAR, 0);
    microkit_vcpu_arm_write_reg(vcpu_id, seL4_VCPUReg_ISR, 0);
    microkit_vcpu_arm_write_reg(vcpu_id, seL4_VCPUReg_VBAR, 0);
    microkit_vcpu_arm_write_reg(vcpu_id, seL4_VCPUReg_TPIDR_EL1, 0);
    microkit_vcpu_arm_write_reg(vcpu_id, seL4_VCPUReg_VMPIDR_EL2, 0);
    microkit_vcpu_arm_write_reg(vcpu_id, seL4_VCPUReg_SP_EL1, 0);
    microkit_vcpu_arm_write_reg(vcpu_id, seL4_VCPUReg_ELR_EL1, 0);
    microkit_vcpu_arm_write_reg(vcpu_id, seL4_VCPUReg_SPSR_EL1, 0);     
    microkit_vcpu_arm_write_reg(vcpu_id, seL4_VCPUReg_CNTV_CTL, 0);
    microkit_vcpu_arm_write_reg(vcpu_id, seL4_VCPUReg_CNTV_CVAL, 0);
    microkit_vcpu_arm_write_reg(vcpu_id, seL4_VCPUReg_CNTVOFF, 0);
    microkit_vcpu_arm_write_reg(vcpu_id, seL4_VCPUReg_CNTKCTL_EL1, 0);
}