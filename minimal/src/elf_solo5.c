#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <elf.h>
#include <elf_solo5.h>
#include <util.h>

/*
 * Define EM_TARGET, EM_PAGE_SIZE and EI_DATA_TARGET for the architecture we
 * are compiling on.
 */
#if defined(__x86_64__)
#define EM_TARGET EM_X86_64
#define EM_PAGE_SIZE 0x1000
#elif defined(__aarch64__)
#define EM_TARGET EM_AARCH64
#define EM_PAGE_SIZE 0x1000
#elif defined(__powerpc64__)
#define EM_TARGET EM_PPC64
#define EM_PAGE_SIZE 0x10000
#else
#error Unsupported target
#endif

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define EI_DATA_TARGET ELFDATA2LSB
#else
#define EI_DATA_TARGET ELFDATA2MSB
#endif

/*
 * Solo5-owned ELF notes are identified by an n_name of "Solo5".
 */
#define SOLO5_NOTE_NAME "Solo5"

/*
 * Defines an Elf64_Nhdr with n_name filled in and padded to a 4-byte boundary,
 * i.e. the common part of a Solo5-owned Nhdr.
 */
struct solo5_nhdr {
    Elf64_Nhdr h;
    char n_name[(sizeof(SOLO5_NOTE_NAME) + 3) & -4];
    /*
     * Note content ("descriptor" in ELF terms) follows in the file here,
     * possibly with some internal alignment before the first struct member
     * (see below).
     */
};

_Static_assert((sizeof(struct solo5_nhdr)) == (sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t) + 8),
        "struct solo5_nhdr alignment issue");

static bool ehdr_is_valid(const Elf64_Ehdr* hdr)
{
    /*
     * 1. Validate that this is an ELF64 header we support.
     *
     * Note: e_ident[EI_OSABI] and e_ident[EI_ABIVERSION] are deliberately NOT
     * checked as compilers do not provide a way to override this without
     * building the entire toolchain from scratch.
     */
    if (!(hdr->e_ident[EI_MAG0] == ELFMAG0
            && hdr->e_ident[EI_MAG1] == ELFMAG1
            && hdr->e_ident[EI_MAG2] == ELFMAG2
            && hdr->e_ident[EI_MAG3] == ELFMAG3
            && hdr->e_ident[EI_CLASS] == ELFCLASS64
            && hdr->e_ident[EI_DATA] == EI_DATA_TARGET
            && hdr->e_version == EV_CURRENT))
        return false;
    /*
     * 2. Validate ELF64 header internal sizes match what we expect, and that
     * at least one program header entry is present.
     */
    if (hdr->e_ehsize != sizeof (Elf64_Ehdr))
        return false;
    if (hdr->e_phnum < 1)
        return false;
    if (hdr->e_phentsize != sizeof (Elf64_Phdr))
        return false;
    /*
     * 3. Validate that this is an executable for our target architecture.
     */
    if (hdr->e_type != ET_EXEC)
        return false;
    if (hdr->e_machine != EM_TARGET)
        return false;

    return true;
}

/*
 * Align (addr) down to (align) boundary. Returns 1 if (align) is not a
 * non-zero power of 2.
 */
static int align_down(Elf64_Addr addr, Elf64_Xword align, Elf64_Addr* out_result)
{
    if (align > 0 && (align & (align - 1)) == 0) 
    {
        *out_result = addr & -align;
        return 0;
    }
    else return 1;
}

/*
 * Align (addr) up to (align) boundary. Returns 1 if an overflow would occur or
 * (align) is not a non-zero power of 2, otherwise result in (*out_result) and
 * 0.
 */
static int align_up(Elf64_Addr addr, Elf64_Xword align, Elf64_Addr* out_result)
{
    Elf64_Addr result;

    if (align > 0 && (align & (align - 1)) == 0) {
        if (add_overflow(addr, (align - 1), result))
            return 1;
        result = result & -align;
        *out_result = result;
        return 0;
    }
    else
        return 1;
}

bool elf_load(uint8_t* elf_ptr, size_t elf_size, uint8_t* mem, size_t mem_size, uint64_t p_min_loadaddr, uint64_t* p_entry, uint64_t* p_end)
{
    Elf64_Phdr phdr;
    Elf64_Ehdr ehdr;
    /* Program entry point */
    Elf64_Addr e_entry;     
    /* Highest memory address occupied */            
    Elf64_Addr e_end;                   

    /* Copy into structs to guarentee struct required alignment */
    memcpy(&ehdr, elf_ptr, sizeof(Elf64_Ehdr));
    if (!ehdr_is_valid(&ehdr)) return false;

    /*
     * e_entry must be non-zero and within range of our memory allocation.
     */
    if (ehdr.e_entry < p_min_loadaddr || ehdr.e_entry >= mem_size) return false;

    e_entry = ehdr.e_entry;

    size_t ph_size = ehdr.e_phnum * ehdr.e_phentsize;    

    /*
     * Load all program segments with the PT_LOAD directive.
     */
    uint8_t* next_phdr_address = elf_ptr + ehdr.e_phoff;
    e_end = 0;
    Elf64_Addr plast_vaddr = 0;

    for (Elf64_Half ph_i = 0; ph_i < ehdr.e_phnum; ph_i++) 
    {
        memcpy(&phdr, next_phdr_address, sizeof(Elf64_Phdr));
        next_phdr_address += sizeof(Elf64_Phdr);
        //TODO: should add check that we stay under elf_size

        Elf64_Addr p_vaddr = phdr.p_vaddr;
        Elf64_Xword p_filesz = phdr.p_filesz;
        Elf64_Xword p_memsz = phdr.p_memsz;
        Elf64_Xword p_align = phdr.p_align;
        Elf64_Addr temp, p_vaddr_start, p_vaddr_end;

        /*
         * consider only non empty PT_LOAD
         */
        if (phdr.p_filesz == 0 || phdr.p_type != PT_LOAD) continue;

        /* Verify segment is at or above minimum text address */
        if (p_vaddr < p_min_loadaddr) return false;

        /*
         * The ELF specification mandates that program headers are sorted on
         * p_vaddr in ascending order. Enforce this, at the same time avoiding
         * any surprises later.
         */
        if (p_vaddr < plast_vaddr) return false;
        else plast_vaddr = p_vaddr;

        /*
         * Compute p_vaddr_start = p_vaddr, aligned down to requested alignment
         * and verify result is within range.
         */
        if (align_down(p_vaddr, p_align, &p_vaddr_start)) return false;
        if (p_vaddr_start < p_min_loadaddr) return false;

        /*
         * Disallow overlapping segments. This may be overkill, but in practice
         * the Solo5 toolchains do not produce such executables.
         */
        if (p_vaddr_start < e_end) return false;

        /*
         * Verify p_vaddr + p_filesz is within range.
         */
        if (p_vaddr >= mem_size) return false;
        if (add_overflow(p_vaddr, p_filesz, temp)) return false;
        if (temp > mem_size) return false;

        /*
         * Compute p_vaddr_end = p_vaddr + p_memsz, aligned up to requested
         * alignment and verify result is within range.
         */
        if (p_memsz < p_filesz) return false;
        if (add_overflow(p_vaddr, p_memsz, p_vaddr_end)) return false;
        if (align_up(p_vaddr_end, p_align, &p_vaddr_end)) return false;
        if (p_vaddr_end > mem_size) return false;

        /*
         * Keep track of the highest byte of memory occupied by the program.
         */
        if (p_vaddr_end > e_end) 
        {
            e_end = p_vaddr_end;
            /*
             * Double check result for host (caller) address space overflow.
             */
            assert((mem + e_end) >= (mem + p_min_loadaddr));
        }

        /*
         * Load the segment (p_vaddr ... p_vaddr + p_filesz) into host memory space at
         * host_vaddr (where mem is where we mapped guest memory in host space) and ensure 
         * any BSS (p_memsz - p_filesz) is initialised to zero.
         */
        uint8_t* host_vaddr = mem + p_vaddr;
        /* Double check result for host (caller) address space overflow. */
        assert(host_vaddr >= (mem + p_min_loadaddr));

        uint8_t* segment_data = elf_ptr + phdr.p_offset;
        memcpy(host_vaddr, segment_data, p_filesz);
        memset(host_vaddr + p_filesz, 0, p_memsz - p_filesz);

        /* The Microkit system description will ensure that VMM can R/W guest memory but not execute, as for
         * guest side protection, I don't know if its currently possible to change (EPT2) memory permissions after bootup,
         * all of guest memory is RWX from guest perspective.
         * In the future we could modify the VCpu initialisation, which currently creates guest owned translation tables (EPT1)
         * to mark lower addresses as read-only (per HVT spec), to also propagate elf permissions.
         */
        /*
         * Memory protection flags should be applied to the aligned address
         * range (p_vaddr_start .. p_vaddr_end). Before we apply them, also
         * verify that the address range is aligned to the architectural page
         * size.
         */        
        /*
        if (p_vaddr_start & (EM_PAGE_SIZE - 1))
            goto out_invalid;
        if (p_vaddr_end & (EM_PAGE_SIZE - 1))
            goto out_invalid;
        int prot = PROT_NONE;
        if (phdr.p_flags & PF_R)
            prot |= PROT_READ;
        if (phdr.p_flags & PF_W)
            prot |= PROT_WRITE;
        if (phdr.p_flags & PF_X)
            prot |= PROT_EXEC;
        if (prot & PROT_WRITE && prot & PROT_EXEC) {
            printf("Error: phdr[%u] requests WRITE and EXEC permissions", ph_i);
            goto out_invalid;
        }
        assert(t_guest_mprotect != NULL);
        if (t_guest_mprotect(t_guest_mprotect_arg, p_vaddr_start, p_vaddr_end,
                    prot) == -1)
            goto out_error;
        */
    }

    *p_entry = e_entry;
    *p_end = e_end;
    return true;
}