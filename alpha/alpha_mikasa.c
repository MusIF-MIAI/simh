/* alpha_mikasa.c: AlphaServer 1000 Mikasa system scaffolding

   Copyright (c) 2026, Mancausoft

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "alpha_defs.h"
#include "alpha_ev5_defs.h"
#include "sim_fio.h"

#include <time.h>

#define MIKASA_HWRPB_PA             0x00380000
#define MIKASA_HWRPB_VA             0x10000000
#define MIKASA_HWRPB_SIZE           0x8000
#define MIKASA_MDDT_OFF             0x800
#define MIKASA_PROCESSOR_OFF        0x1000
#define MIKASA_PROCESSOR_SIZE       0x278
#define MIKASA_CTB_OFF              0x1800
#define MIKASA_CTB_SIZE             0x90
#define MIKASA_CTB_TERM_SIZE        0x70
#define MIKASA_CRB_OFF              0x1900
#define MIKASA_CONSOLE_VA_PD_OFF    0x1A00
#define MIKASA_CONSOLE_PA_PD_OFF    0x1A20
#define MIKASA_CONSOLE_CODE_OFF     0x1A40
#define MIKASA_SWRPB_OFF            0x1B00
#define MIKASA_CONSOLE_VA_PD_PA     (MIKASA_HWRPB_PA + MIKASA_CONSOLE_VA_PD_OFF)
#define MIKASA_CONSOLE_VA_PD_VA     (MIKASA_HWRPB_VA + MIKASA_CONSOLE_VA_PD_OFF)
#define MIKASA_CONSOLE_PA_PD_PA     (MIKASA_HWRPB_PA + MIKASA_CONSOLE_PA_PD_OFF)
#define MIKASA_CONSOLE_PA_PD_VA     (MIKASA_HWRPB_VA + MIKASA_CONSOLE_PA_PD_OFF)
#define MIKASA_CONSOLE_CODE_PA      (MIKASA_HWRPB_PA + MIKASA_CONSOLE_CODE_OFF)
#define MIKASA_CONSOLE_CODE_VA      (MIKASA_HWRPB_VA + MIKASA_CONSOLE_CODE_OFF)
#define MIKASA_SWRPB_VA             (MIKASA_HWRPB_VA + MIKASA_SWRPB_OFF)
#define MIKASA_PAGE_SIZE            8192
#define MIKASA_DKA_UNITS            4
#define MIKASA_DKA_BLOCK_SIZE       512
#define MIKASA_IO_BUFSIZE           4096
#define MIKASA_SCSI_SLOT            6
#define MIKASA_APB_IO_CMD_OFF       0x120
#define MIKASA_APB_IO_LBN_OFF       0x128
#define MIKASA_APB_IO_ADDR_OFF      0x130
#define MIKASA_APB_IO_CMD_READ      2
#define MIKASA_APB_IO_CMD_WRITE     4
#define MIKASA_APB_IO_ERR           0x10
#define MIKASA_VMS_SYSVA_BASE       0x80000000
#define MIKASA_VMS_SYSVA_LIMIT      0xC0000000
#define MIKASA_VMS_SYSVA_MASK       0x1FFFFFFF
#define MIKASA_PFN_BITMAP_PA        (MIKASA_HWRPB_PA + 0x2000)
#define MIKASA_PMR_SIZE             0x38
#define MIKASA_BOOT_IMAGE_PA        0x00200000
#define MIKASA_BOOT_IMAGE_VA        0x20000000
#define MIKASA_BOOT_STACK_TOP       0x200CE000
#define MIKASA_BOOT_RESERVED_SIZE   0x00100000
#define MIKASA_PT_SPACE_VA          0x40000000
#define MIKASA_PT_SPACE_PA          0x00400000
#define MIKASA_PT_SPACE_VA_SIZE     (((t_uint64) 1) << (VA_N_VPN + 3))
#define MIKASA_L1_PT_PA             MIKASA_PT_SPACE_PA
#define MIKASA_L2_PT_OFF            0x00002000
#define MIKASA_L3_REGION0_OFF       0x00040000
#define MIKASA_L3_REGION1_OFF       0x00080000
#define MIKASA_L3_BOOTPTS_OFF       0x00100000
#define MIKASA_L3_BOOTPTS_PAGES     (VA_M_LVL + 1)
#define MIKASA_L3_REGION0_PA        (MIKASA_PT_SPACE_PA + MIKASA_L3_REGION0_OFF)
#define MIKASA_L3_REGION1_PA        (MIKASA_PT_SPACE_PA + MIKASA_L3_REGION1_OFF)
#define MIKASA_L3_BOOTPTS_PA        (MIKASA_PT_SPACE_PA + MIKASA_L3_BOOTPTS_OFF)
#define MIKASA_L2_PT_PA             (MIKASA_PT_SPACE_PA + MIKASA_L2_PT_OFF)
#define MIKASA_PT_SPACE_SIZE        (MIKASA_L3_BOOTPTS_OFF + \
                                      (MIKASA_L3_BOOTPTS_PAGES * \
                                      MIKASA_PAGE_SIZE))
#define MIKASA_TLB_SPAN             0x00400000
#define MIKASA_BOOT_PT_RESERVED_END (MIKASA_PT_SPACE_PA + \
                                      MIKASA_PT_SPACE_SIZE)
#define MIKASA_LFU_HEADER_SIZE      0x240
#define MIKASA_LFU_MAGIC_OFF        4
#define MIKASA_LFU_MAGIC_SIZE       8
#define MIKASA_LFU_VERSION_OFF      0x208
#define MIKASA_LFU_VERSION_SIZE     12
#define MIKASA_LFU_VENDOR_OFF       0x214
#define MIKASA_LFU_VENDOR_SIZE      4
#define MIKASA_LFU_PLATFORM_OFF     0x218
#define MIKASA_LFU_PLATFORM_SIZE    8
#define MIKASA_LFU_ARCH_OFF         0x220
#define MIKASA_LFU_ARCH_SIZE        8
#define MIKASA_LFU_IMAGE_SIZE_OFF   0x22C
#define MIKASA_LFU_KIND_OFF         0x230
#define MIKASA_LFU_KIND_SIZE        8
#define MIKASA_LFU_TRAILER_OFF      0x23C
#define MIKASA_ROM_LOAD_PA          0x00900000
#define MIKASA_ROM_WORKSPACE_SIZE   0x01000000
#define MIKASA_AXPBOX_ROM_MEMSIZE   0x00200000
#define MIKASA_AXPBOX_ROM_SIZE      (16 + MIKASA_AXPBOX_ROM_MEMSIZE)
#define MIKASA_EPIC_BASE            0x1A0000000ULL
#define MIKASA_EPIC_SIZE            0x00001000
#define MIKASA_APECS_PCI_IACK       0x1B0000000ULL
#define MIKASA_APECS_PCI_SIO        0x1C0000000ULL
#define MIKASA_APECS_PCI_SIO_SIZE   0x02000000
#define MIKASA_APECS_PCI_CONF       0x1E0000000ULL
#define MIKASA_APECS_PCI_CONF_SIZE  0x02000000
#define MIKASA_APECS_PCI_SPARSE     0x200000000ULL
#define MIKASA_APECS_PCI_SPARSE_SIZE 0x100000000ULL
#define MIKASA_APECS_PCI_DENSE      0x300000000ULL
#define MIKASA_APECS_PCI_DENSE_SIZE 0x100000000ULL
/* On-board devices use raw APECS IDSEL slots; SRM prints virtual slots. */
#define MIKASA_PCI_SLOT_SCSI        6
/* SRM reports the EISA bridge as virtual slot 2; config cycles use IDSEL 7. */
#define MIKASA_PCI_SLOT_PCEB        7
#define MIKASA_PCI_CFG_DWORDS       64
#define MIKASA_NCR_REG_SIZE         0x100
#define MIKASA_NCR_BAR_SIZE         0x100
#define MIKASA_NCR_BAR_MASK         0xFFFFFF00u
#define MIKASA_NCR_REG_SCNTL0       0x00
#define MIKASA_NCR_REG_DSTAT        0x0C
#define MIKASA_NCR_REG_ISTAT        0x14
#define MIKASA_NCR_REG_SIST0        0x42
#define MIKASA_NCR_REG_SIST1        0x43
#define MIKASA_NCR_REG_CTEST1       0x19
#define MIKASA_NCR_REG_CTEST2       0x1A
#define MIKASA_NCR_REG_CTEST3       0x1B
#define MIKASA_NCR_REG_MACNTL       0x46
#define MIKASA_NCR_REG_GPCNTL       0x47
#define MIKASA_NCR_REG_STEST0       0x4C
#define MIKASA_NCR_SCNTL0_ARB       0xC0
#define MIKASA_NCR_DSTAT_DFE        0x80
#define MIKASA_NCR_ISTAT_SRST       0x40
#define MIKASA_NCR_CTEST1_FMT       0xF0
#define MIKASA_NCR_CTEST2_DACK      0x01
#define MIKASA_NCR_MACNTL_810       0x40
#define MIKASA_ISA_DMA1             0x000
#define MIKASA_ISA_PIC1             0x020
#define MIKASA_ISA_CFG_INDEX        0x022
#define MIKASA_ISA_CFG_DATA         0x023
#define MIKASA_ISA_PIT              0x040
#define MIKASA_ISA_PORTB            0x061
#define MIKASA_ISA_RTC_INDEX        0x070
#define MIKASA_ISA_RTC_DATA         0x071
#define MIKASA_ISA_PIC2             0x0A0
#define MIKASA_ISA_COM2             0x2F8
#define MIKASA_ISA_SIO_INDEX        0x398
#define MIKASA_ISA_SIO_DATA         0x399
#define MIKASA_ISA_COM1             0x3F8
#define MIKASA_ISA_NMI_CTRL         0x461
#define MIKASA_ISA_OCP              0x530
#define MIKASA_ISA_ICU_IMR          0x536
#define MIKASA_ISA_ICU_PRESENT      0x8000
#define MIKASA_EISA_CRAM            0x0800
#define MIKASA_EISA_CRAM_SIZE       0x0100
#define MIKASA_EISA_CRAM_PAGES      32
#define MIKASA_EISA_CRAM_PAGE_REG   0x0C00
#define MIKASA_EISA_SLOT_ID         0x0C80
#define MIKASA_OCP_READY            0x80

#define MIKASA_UART_LSR_DR          0x01
#define MIKASA_UART_LSR_THRE        0x20
#define MIKASA_UART_LSR_TEMT        0x40
#define MIKASA_UART_IIR_NOPEND      0x01
#define MIKASA_UART_LCR_DLAB        0x80
#define MIKASA_UART_MSR_DCTS        0x01
#define MIKASA_UART_MSR_CTS         0x10
#define MIKASA_UART_MSR_DSR         0x20
#define MIKASA_UART_MSR_DCD         0x80

#define MIKASA_RTC_STATUSA          0x0A
#define MIKASA_RTC_STATUSB          0x0B
#define MIKASA_RTC_INTR            0x0C
#define MIKASA_RTC_STATUSD          0x0D
#define MIKASA_RTC_CENTURY          0x32
#define MIKASA_RTC_BINARY           0x04
#define MIKASA_RTC_24HR             0x02
#define MIKASA_RTC_POWER_OK         0x80

#define MIKASA_DBG_IO               0x0001
#define MIKASA_DBG_UART             0x0002
#define MIKASA_DBG_EISA             0x0004
#define MIKASA_DBG_RTC              0x0008
#define MIKASA_DBG_PCI              0x0010

#define HWRPB_ID                    0x0000004250525748ULL
#define HWRPB_CHECKSUM_OFF          0x120
#define HWRPB_ST_DEC_1000_COMPAT    5
#define HWRPB_EV45_CPU              3
#define HWRPB_SLOT_STATE_BOOT       0x1ED
#define HWRPB_CPU_VAR_EV45          0x7
#define HWRPB_HWPCB_PT_VA_OFF       0x50
#define HWRPB_CTB_TYPE_TERMINAL     2
#define HWRPB_PMR_USAGE_CONSOLE     1

#define HWRPB_CRB_K_GETC            1
#define HWRPB_CRB_K_PUTS            2
#define HWRPB_CRB_K_RESET_TERM      3
#define HWRPB_CRB_K_SET_TERM_INTR   4
#define HWRPB_CRB_K_SET_TERM_CTL    5
#define HWRPB_CRB_K_PROCESS_KEYCODE 6
#define HWRPB_CRB_K_OPEN            16
#define HWRPB_CRB_K_CLOSE           17
#define HWRPB_CRB_K_IOCTRL          18
#define HWRPB_CRB_K_READ            19
#define HWRPB_CRB_K_WRITE           20
#define HWRPB_CRB_K_SET_ENV         32
#define HWRPB_CRB_K_RESET_ENV       33
#define HWRPB_CRB_K_GET_ENV         34
#define HWRPB_CRB_K_SAVE_ENV        35
#define HWRPB_CRB_K_AUTO_ACTION     1
#define HWRPB_CRB_K_BOOT_DEV        2
#define HWRPB_CRB_K_BOOTCMD_DEV     3
#define HWRPB_CRB_K_BOOTED_DEV      4
#define HWRPB_CRB_K_BOOT_FILE       5
#define HWRPB_CRB_K_BOOTED_FILE     6
#define HWRPB_CRB_K_BOOT_OSFLAGS    7
#define HWRPB_CRB_K_BOOTED_OSFLAGS  8
#define HWRPB_CRB_K_BOOT_RESET      9
#define HWRPB_CRB_K_DUMP_DEV        10
#define HWRPB_CRB_K_ENABLE_AUDIT    11
#define HWRPB_CRB_K_LICENSE         12
#define HWRPB_CRB_K_CHAR_SET        13
#define HWRPB_CRB_K_LANGUAGE        14
#define HWRPB_CRB_K_TTY_DEV         15
#define HWRPB_CRB_K_PSWITCH         48
#define HWRPB_CRB_STS_SUCCESS       0
#define HWRPB_CRB_STS_TRUNCATED     1
#define HWRPB_CRB_STS_READONLY      4
#define HWRPB_CRB_STS_UNRECOGNIZED  6
#define HWRPB_CRB_STS_TOOLONG       7
#define HWRPB_CRB_STS_SHIFT         61
#define HWRPB_CRB_OPEN_STS_NXDEV    (2ULL << 62)
#define HWRPB_CRB_OPEN_STS_ERROR    (3ULL << 62)
#define HWRPB_CRB_IO_STS_FAIL       (1ULL << 63)
#define HWRPB_CRB_IO_STS_EOD        (1ULL << 62)
#define HWRPB_CRB_IO_STS_ILL        (1ULL << 61)

#define PDSC_FLAGS_NULL_NATIVE      0x3008

#define MPAL_MIKASA_CCALL           0xBF

#define ALPHA_INSN_CALL_PAL(fnc)    ((uint32) (fnc))
#define ALPHA_INSN_RET              0x6BFA8001

extern UNIT cpu_unit;
extern jmp_buf save_env;
extern t_uint64 *M;
extern t_uint64 R[32];
extern t_uint64 PC;
extern t_uint64 p1;
extern uint32 ir;
extern uint32 lock_flag;
extern uint32 pal_mode;
extern uint32 pal_type;
extern uint32 dmapen;
extern uint32 fpen;
extern uint32 cm_macc;
extern uint32 cm_wacc;
extern uint32 pcc_l;
extern uint32 pc_align;
extern t_uint64 ev5_palbase;
extern UNIT dka_unit[];

uint32 mikasa_irq_mask = 0;
uint32 mikasa_irq_summary = 0;
t_uint64 mikasa_hwrpb_pa = MIKASA_HWRPB_PA;
t_uint64 mikasa_last_osflags = 0;
t_uint64 mikasa_last_boot_bytes = 0;
t_uint64 mikasa_next_l3_pt_pa = MIKASA_L3_BOOTPTS_PA;
t_uint64 mikasa_callback_count = 0;
t_uint64 mikasa_getenv_count = 0;
t_uint64 mikasa_ioread_count = 0;
t_uint64 mikasa_iowrite_count = 0;
t_uint64 mikasa_apb_ioread_count = 0;
t_uint64 mikasa_apb_iowrite_count = 0;
t_uint64 mikasa_apb_last_cmd = 0;
t_uint64 mikasa_apb_last_lbn = 0;
t_uint64 mikasa_apb_last_addr = 0;
t_uint64 mikasa_apb_last_pa = 0;
static t_bool mikasa_apb_dma_valid = FALSE;
static t_uint64 mikasa_apb_dma_cpu_addr = 0;
static t_uint64 mikasa_apb_dma_cpu_len = 0;
static t_uint64 mikasa_apb_dma_bus_addr = 0;
static t_uint64 mikasa_apb_dma_bus_len = 0;
t_uint64 mikasa_align_count = 0;
t_uint64 mikasa_align_last_va = 0;
t_uint64 mikasa_align_last_pc = 0;
t_uint64 mikasa_pal_pcbb = 0;
t_uint64 mikasa_pal_prbr = 0;
t_uint64 mikasa_pal_ptbr = 0;
t_uint64 mikasa_pal_scbb = 0;
t_uint64 mikasa_pal_vtbr = 0;
t_uint64 mikasa_pal_virbnd = M64;
t_uint64 mikasa_pal_sysptbr = 0;
t_uint64 mikasa_pal_thread = 0;
t_uint64 mikasa_pal_scc = 0;
uint32 mikasa_pal_ipl = 0;
uint32 mikasa_pal_sisr = 0;
uint32 mikasa_pal_asten = 0;
uint32 mikasa_pal_astsr = 0;
uint32 mikasa_pal_mces = 0;
uint32 mikasa_pal_ps = 0;
uint32 mikasa_pal_last_pcc = 0;

t_stat mikasa_reset (DEVICE *dptr);
t_stat mikasa_boot_prepare (CONST char *bootdev, uint32 osflags,
    t_uint64 image_bytes);
t_stat mikasa_set_rom (UNIT *uptr, int32 val, CONST char *cptr, void *desc);
t_stat mikasa_set_rom_payload (UNIT *uptr, int32 val, CONST char *cptr,
    void *desc);
t_stat mikasa_save_rom (UNIT *uptr, int32 val, CONST char *cptr, void *desc);
t_stat mikasa_boot_rom (int32 unitno);
void mikasa_mem_write (t_uint64 pa, t_uint64 dat, uint32 lnt);
t_stat mikasa_pal_proc_excp (uint32 abval);
t_stat mikasa_pal_proc_inst (uint32 fnc);
static t_bool mikasa_io_rd (t_uint64 pa, t_uint64 *val, uint32 lnt);
static t_bool mikasa_io_wr (t_uint64 pa, t_uint64 val, uint32 lnt);

static void mikasa_zero_phys (t_uint64 pa, uint32 size);
static t_stat mikasa_ensure_mem (void);
static void mikasa_copy_cstr (char *dst, uint32 dst_size, const uint8 *src,
    uint32 src_size);
static uint32 mikasa_get_le32 (const uint8 *buf);
static t_uint64 mikasa_get_le64 (const uint8 *buf);
static t_bool mikasa_lfu_header_valid (const uint8 *header);
static t_stat mikasa_load_bytes (FILE *fileref, t_uint64 pa, t_uint64 bytes);
static void mikasa_prepare_rom_cpu (t_uint64 pc, t_uint64 palbase);
static void mikasa_note_rom (const char *path, t_uint64 pc,
    t_uint64 palbase);
static t_stat mikasa_load_lfu_rom (FILE *fileref, t_offset fsize,
    const char *path, const uint8 *header);
static t_stat mikasa_load_axpbox_rom (FILE *fileref, const char *path,
    const uint8 *header);
static t_stat mikasa_load_raw_rom_payload (FILE *fileref, t_offset fsize,
    const char *path);
static t_stat mikasa_write_le64 (FILE *fileref, t_uint64 val);
static t_uint64 mikasa_sum_qwords (t_uint64 pa, uint32 size);
static t_uint64 mikasa_fill_bitmap (t_uint64 bitmap_pa, t_uint64 bits);
static void mikasa_write_mem_cluster (t_uint64 mddt_pa, uint32 cluster,
    t_uint64 start_pfn, t_uint64 pfn_count, uint32 usage,
    t_uint64 *bitmap_pa);
static t_uint64 mikasa_hwrpb_va_for_pa (t_uint64 pa);
static t_uint64 mikasa_pte (t_uint64 pa, uint32 flags);
static t_bool mikasa_pte_to_pa (t_uint64 pte, t_uint64 page_off,
    t_uint64 *pa);
static t_bool mikasa_boot_pt_walk_pte (t_uint64 va, t_uint64 *pte);
static t_bool mikasa_boot_pt_va_to_pa (t_uint64 va, t_uint64 *pa);
static t_bool mikasa_boot_pt_va_to_pte (t_uint64 va, t_uint64 *pte);
static t_bool mikasa_boot_va_to_pte (t_uint64 va, t_uint64 *pte);
static t_bool mikasa_boot_va_to_pa (t_uint64 va, t_uint64 *pa);
static t_uint64 mikasa_alloc_l3_pt (void);
static void mikasa_set_l2_entry (uint32 l2_index, t_uint64 l3_pa);
static void mikasa_map_range (t_uint64 l3_pa, t_uint64 va_base,
    t_uint64 pa_base, t_uint64 bytes);
static void mikasa_build_boot_pt (void);
static void mikasa_load_tlb_range (t_uint64 va, t_uint64 pa, t_uint64 bytes,
    uint32 flags);
static void mikasa_load_itlb_page (t_uint64 va, t_uint64 pa, uint32 flags);
static void mikasa_load_dtlb_page (t_uint64 va, t_uint64 pa, uint32 flags);
static void mikasa_load_boot_tlb (void);
static uint32 mikasa_pal_probe (uint32 acc);
static t_int64 mikasa_pal_insqhil (void);
static t_int64 mikasa_pal_insqtil (void);
static t_int64 mikasa_pal_insqhiq (void);
static t_int64 mikasa_pal_insqtiq (void);
static t_int64 mikasa_pal_insquel (uint32 defer);
static t_int64 mikasa_pal_insqueq (uint32 defer);
static t_int64 mikasa_pal_remqhil (void);
static t_int64 mikasa_pal_remqtil (void);
static t_int64 mikasa_pal_remqhiq (void);
static t_int64 mikasa_pal_remqtiq (void);
static t_int64 mikasa_pal_remquel (uint32 defer);
static t_int64 mikasa_pal_remqueq (uint32 defer);
static t_bool mikasa_pal_proc_align (void);
static t_uint64 mikasa_pal_read_una (t_uint64 va, uint32 size);
static void mikasa_pal_write_una (t_uint64 va, t_uint64 val, uint32 size);
static t_uint64 mikasa_pal_read_una_l (t_uint64 va);
static void mikasa_pal_write_una_l (t_uint64 va, t_uint64 val);
static void mikasa_write_console_crb (t_uint64 hwrpb_pa);
static void mikasa_set_boot_env (CONST char *bootdev, uint32 osflags);
static t_uint64 mikasa_callback_status (t_uint64 status, t_uint64 count);
static const char *mikasa_get_env_value (uint32 env);
static void mikasa_copy_env_value (const char *value);
static uint32 mikasa_boot_target (CONST char *bootdev);
static void mikasa_clear_io_channels (void);
static int32 mikasa_parse_io_unit (const char *devstr);
static t_bool mikasa_apb_dma_window_pa (t_uint64 addr, t_uint64 cpu_addr,
    t_uint64 cpu_len, t_uint64 bus_addr, t_uint64 bus_len, t_uint64 *pa);
static t_bool mikasa_apb_io_descriptor_pa (t_uint64 addr, t_uint64 *pa);
static t_bool mikasa_apb_io_target_pa (t_uint64 addr, t_uint64 *pa);
static void mikasa_write_phys_long (t_uint64 pa, t_uint64 dat);
static void mikasa_write_phys_byte (t_uint64 pa, uint8 dat);
static uint8 mikasa_read_phys_byte (t_uint64 pa);
static t_bool mikasa_apb_iobox_read (uint32 unit, t_uint64 lbn,
    t_uint64 addr);
static t_bool mikasa_apb_iobox_write (uint32 unit, t_uint64 lbn,
    t_uint64 addr);
static void mikasa_apb_iobox_command (t_uint64 pa, t_uint64 cmd);
static void mikasa_read_callback_string (t_uint64 va, t_uint64 len,
    char *buf, uint32 bufsize);
static void mikasa_open_io (void);
static void mikasa_close_io (void);
static void mikasa_read_io (void);
static void mikasa_write_io (void);
static t_stat mikasa_console_callback (void);
static void mikasa_uart_poll (void);
static uint8 mikasa_uart_read (uint32 port);
static void mikasa_uart_write (uint32 port, uint8 val);
static uint8 mikasa_isa_read (uint32 port);
static void mikasa_isa_write (uint32 port, uint8 val);
static t_uint64 mikasa_sparse_read (t_uint64 pa, uint32 lnt);
static void mikasa_sparse_write (t_uint64 pa, t_uint64 val, uint32 lnt);

static char mikasa_auto_action[] = "BOOT";
static char mikasa_booted_dev[64] = "SCSI 0 6 0 0 0 0 0";
static char mikasa_booted_osflags[32] = "0";
static char mikasa_boot_file[] = "";
static char mikasa_boot_reset[] = "OFF";
static char mikasa_enable_audit[] = "ON";
static char mikasa_language[] = "36";
static char mikasa_tty_dev[] = "0";
static uint32 mikasa_io_channel[MIKASA_DKA_UNITS] = { 0 };
static t_bool mikasa_rom_loaded = FALSE;
static t_uint64 mikasa_rom_pc = 0;
static t_uint64 mikasa_rom_palbase = 0;
static char mikasa_rom_path[CBUFSIZE] = "";
static uint8 mikasa_uart_lcr = 0;
static uint8 mikasa_uart_mcr = 0;
static uint8 mikasa_uart_ier = 0;
static uint8 mikasa_uart_scr = 0;
static uint8 mikasa_uart_dll = 0;
static uint8 mikasa_uart_dlm = 0;
static uint8 mikasa_uart_rbr = 0;
static t_bool mikasa_uart_rbr_valid = FALSE;
static uint8 mikasa_com2_lcr = 0;
static uint8 mikasa_com2_mcr = 0;
static uint8 mikasa_com2_ier = 0;
static uint8 mikasa_com2_scr = 0;
static uint8 mikasa_com2_dll = 0;
static uint8 mikasa_com2_dlm = 0;
static uint8 mikasa_pic_cmd[2] = { 0 };
static uint8 mikasa_pic_imr[2] = { 0xFF, 0xFF };
static uint8 mikasa_cfg_index = 0;
static uint8 mikasa_cfg_reg[256] = { 0 };
static uint8 mikasa_pit_mode = 0;
static uint8 mikasa_pit_reg[3] = { 0 };
static uint8 mikasa_portb = 0;
static uint8 mikasa_rtc_index = 0;
static uint8 mikasa_rtc_reg[128] = { 0 };
static uint8 mikasa_sio_index = 0;
static uint8 mikasa_sio_reg[256] = { 0 };
static uint8 mikasa_nmi_ctrl = 0;
static uint8 mikasa_eisa_cram_page = 0;
static uint8 mikasa_eisa_cram[MIKASA_EISA_CRAM_PAGES][MIKASA_EISA_CRAM_SIZE];
static uint32 mikasa_epic_reg[MIKASA_EPIC_SIZE >> 5] = { 0 };
static uint32 mikasa_ncr_cfg[MIKASA_PCI_CFG_DWORDS] = { 0 };
static uint8 mikasa_ncr_reg[MIKASA_NCR_REG_SIZE] = { 0 };
static uint32 mikasa_pceb_cfg[MIKASA_PCI_CFG_DWORDS] = { 0 };
static uint8 mikasa_ocp_reg[4] = { 0 };
static uint32 mikasa_scc_scale = 1;

UNIT mikasa_unit = { UDATA (NULL, 0, 0) };

DIB mikasa_dib = {
    MIKASA_EPIC_BASE, MIKASA_APECS_PCI_DENSE + MIKASA_APECS_PCI_DENSE_SIZE,
    &mikasa_io_rd, &mikasa_io_wr, 0
    };

DEBTAB mikasa_debug[] = {
    { "IO", MIKASA_DBG_IO },
    { "UART", MIKASA_DBG_UART },
    { "EISA", MIKASA_DBG_EISA },
    { "RTC", MIKASA_DBG_RTC },
    { "PCI", MIKASA_DBG_PCI },
    { NULL, 0 }
    };

REG mikasa_reg[] = {
    { HRDATA (HWRPB, mikasa_hwrpb_pa, 64), REG_RO },
    { HRDATA (OSFLAGS, mikasa_last_osflags, 64), REG_RO },
    { HRDATA (BOOTBYTES, mikasa_last_boot_bytes, 64), REG_RO },
    { HRDATA (CALLBACKS, mikasa_callback_count, 64), REG_RO },
    { HRDATA (GETENVS, mikasa_getenv_count, 64), REG_RO },
    { HRDATA (IOREADS, mikasa_ioread_count, 64), REG_RO },
    { HRDATA (IOWRITES, mikasa_iowrite_count, 64), REG_RO },
    { HRDATA (APBIOREADS, mikasa_apb_ioread_count, 64), REG_RO },
    { HRDATA (APBIOWRITES, mikasa_apb_iowrite_count, 64), REG_RO },
    { HRDATA (APBIOCMD, mikasa_apb_last_cmd, 64), REG_RO },
    { HRDATA (APBIOLBN, mikasa_apb_last_lbn, 64), REG_RO },
    { HRDATA (APBIOADDR, mikasa_apb_last_addr, 64), REG_RO },
    { HRDATA (APBIOPA, mikasa_apb_last_pa, 64), REG_RO },
    { HRDATA (ALIGNS, mikasa_align_count, 64), REG_RO },
    { HRDATA (ALIGNVA, mikasa_align_last_va, 64), REG_RO },
    { HRDATA (ALIGNPC, mikasa_align_last_pc, 64), REG_RO },
    { HRDATA (IRQSMM, mikasa_irq_summary, 16) },
    { HRDATA (IRQMASK, mikasa_irq_mask, 16) },
    { HRDATA (PALIPL, mikasa_pal_ipl, 5) },
    { HRDATA (PALPCBB, mikasa_pal_pcbb, 64) },
    { HRDATA (PALSCBB, mikasa_pal_scbb, 64) },
    { DRDATA (SCCSCALE, mikasa_scc_scale, 32) },
    { NULL }
    };

MTAB mikasa_mod[] = {
    { MTAB_XTD|MTAB_VDV|MTAB_VALR|MTAB_NC, 0, "ROM", "ROM",
      &mikasa_set_rom, NULL, NULL,
      "Load a Mikasa SRM LFU ROM or AXPbox decompressed ROM image" },
    { MTAB_XTD|MTAB_VDV|MTAB_VALR|MTAB_NC, 0, "ROMPAYLOAD", "ROMPAYLOAD",
      &mikasa_set_rom_payload, NULL, NULL,
      "Load an extracted Mikasa SRM LFU payload image" },
    { MTAB_XTD|MTAB_VDV|MTAB_VALR|MTAB_NC, 0, "ROMSAVE", "ROMSAVE",
      &mikasa_save_rom, NULL, NULL,
      "Save the current decompressed Mikasa SRM ROM image" },
    { 0 }
    };

DEVICE mikasa_dev = {
    "MIKASA", &mikasa_unit, mikasa_reg, mikasa_mod,
    1, 16, 32, 1, 16, 8,
    NULL, NULL, &mikasa_reset,
    NULL, NULL, NULL, &mikasa_dib,
    DEV_DIB|DEV_DEBUG, 0, mikasa_debug
    };

static void mikasa_uart_poll (void)
{
t_stat c;

if (mikasa_uart_rbr_valid)
    return;
c = sim_poll_kbd ();
if (c >= SCPE_KFLAG) {
    mikasa_uart_rbr = (uint8) (c & M8);
    mikasa_uart_rbr_valid = TRUE;
    }
return;
}

static uint8 mikasa_uart_read (uint32 port)
{
uint32 reg = port - MIKASA_ISA_COM1;

switch (reg) {

    case 0:
        if (mikasa_uart_lcr & MIKASA_UART_LCR_DLAB)
            return mikasa_uart_dll;
        mikasa_uart_poll ();
        if (mikasa_uart_rbr_valid) {
            mikasa_uart_rbr_valid = FALSE;
            return mikasa_uart_rbr;
            }
        return 0;

    case 1:
        if (mikasa_uart_lcr & MIKASA_UART_LCR_DLAB)
            return mikasa_uart_dlm;
        return mikasa_uart_ier;

    case 2:
        return MIKASA_UART_IIR_NOPEND;

    case 3:
        return mikasa_uart_lcr;

    case 4:
        return mikasa_uart_mcr;

    case 5:
        mikasa_uart_poll ();
        return MIKASA_UART_LSR_THRE | MIKASA_UART_LSR_TEMT |
            (mikasa_uart_rbr_valid? MIKASA_UART_LSR_DR: 0);

    case 6:
        return MIKASA_UART_MSR_CTS | MIKASA_UART_MSR_DSR |
            MIKASA_UART_MSR_DCD | MIKASA_UART_MSR_DCTS;

    case 7:
        return mikasa_uart_scr;

    default:
        return 0;
        }
}

static void mikasa_uart_write (uint32 port, uint8 val)
{
uint32 reg = port - MIKASA_ISA_COM1;
int32 c;

switch (reg) {

    case 0:
        if (mikasa_uart_lcr & MIKASA_UART_LCR_DLAB)
            mikasa_uart_dll = val;
        else {
            c = sim_tt_outcvt (val, TT_MODE_8B);
            sim_debug (MIKASA_DBG_UART, &mikasa_dev,
                "COM1 transmit %02X\n", val);
            if (c >= 0)
                (void) sim_putchar_s (c);
            }
        break;

    case 1:
        if (mikasa_uart_lcr & MIKASA_UART_LCR_DLAB)
            mikasa_uart_dlm = val;
        else
            mikasa_uart_ier = val;
        break;

    case 3:
        mikasa_uart_lcr = val;
        break;

    case 4:
        mikasa_uart_mcr = val;
        break;

    case 7:
        mikasa_uart_scr = val;
        break;

    default:
        break;
        }
return;
}

static uint8 mikasa_com2_read (uint32 port)
{
uint32 reg = port - MIKASA_ISA_COM2;

switch (reg) {
    case 0:
        return (mikasa_com2_lcr & MIKASA_UART_LCR_DLAB) ?
            mikasa_com2_dll : 0;

    case 1:
        return (mikasa_com2_lcr & MIKASA_UART_LCR_DLAB) ?
            mikasa_com2_dlm : mikasa_com2_ier;

    case 2:
        return MIKASA_UART_IIR_NOPEND;

    case 3:
        return mikasa_com2_lcr;

    case 4:
        return mikasa_com2_mcr;

    case 5:
        return MIKASA_UART_LSR_THRE | MIKASA_UART_LSR_TEMT;

    case 6:
        return MIKASA_UART_MSR_CTS |
            MIKASA_UART_MSR_DSR |
            MIKASA_UART_MSR_DCD;

    case 7:
        return mikasa_com2_scr;

    default:
        return 0;
        }
}

static void mikasa_com2_write (uint32 port, uint8 val)
{
uint32 reg = port - MIKASA_ISA_COM2;
int32 c;

switch (reg) {
    case 0:
        if (mikasa_com2_lcr & MIKASA_UART_LCR_DLAB)
            mikasa_com2_dll = val;
        else {
            c = sim_tt_outcvt (val, TT_MODE_8B);
            sim_debug (MIKASA_DBG_UART, &mikasa_dev,
                "COM2 transmit %02X\n", val);
            if (c >= 0)
                (void) sim_putchar_s (c);
            }
        break;

    case 1:
        if (mikasa_com2_lcr & MIKASA_UART_LCR_DLAB)
            mikasa_com2_dlm = val;
        else
            mikasa_com2_ier = val;
        break;

    case 3:
        mikasa_com2_lcr = val;
        break;

    case 4:
        mikasa_com2_mcr = val;
        break;

    case 7:
        mikasa_com2_scr = val;
        break;

    default:
        break;
        }
return;
}

static void mikasa_pic_update_mask (void)
{
mikasa_irq_mask = ((uint32) mikasa_pic_imr[0]) |
    (((uint32) mikasa_pic_imr[1]) << 8);
return;
}

static uint8 mikasa_pic_read (uint32 port)
{
uint32 which = (port >= MIKASA_ISA_PIC2) ? 1 : 0;

if (port & 1)
    return mikasa_pic_imr[which];
return 0;
}

static void mikasa_pic_write (uint32 port, uint8 val)
{
uint32 which = (port >= MIKASA_ISA_PIC2) ? 1 : 0;

if (port & 1) {
    mikasa_pic_imr[which] = val;
    mikasa_pic_update_mask ();
    }
else
    mikasa_pic_cmd[which] = val;
return;
}

static uint8 mikasa_rtc_encode (uint32 val)
{
if (mikasa_rtc_reg[MIKASA_RTC_STATUSB] & MIKASA_RTC_BINARY)
    return (uint8) val;
return (uint8) (((val / 10) << 4) | (val % 10));
}

static uint8 mikasa_rtc_read_data (void)
{
uint8 index = mikasa_rtc_index & 0x7F;
uint8 val;
time_t now;
struct tm *tm;

now = time (NULL);
tm = localtime (&now);
if (tm == NULL)
    return mikasa_rtc_reg[index];

switch (index) {
    case 0x00:
        val = mikasa_rtc_encode ((uint32) tm->tm_sec);
        break;

    case 0x02:
        val = mikasa_rtc_encode ((uint32) tm->tm_min);
        break;

    case 0x04:
        val = mikasa_rtc_encode ((uint32) tm->tm_hour);
        break;

    case 0x06:
        val = mikasa_rtc_encode ((uint32) (tm->tm_wday + 1));
        break;

    case 0x07:
        val = mikasa_rtc_encode ((uint32) tm->tm_mday);
        break;

    case 0x08:
        val = mikasa_rtc_encode ((uint32) (tm->tm_mon + 1));
        break;

    case 0x09:
        val = mikasa_rtc_encode ((uint32) (tm->tm_year % 100));
        break;

    case MIKASA_RTC_STATUSA:
        val = (mikasa_rtc_reg[index] & 0x7F) | 0x20;
        break;

    case MIKASA_RTC_INTR:
        mikasa_rtc_reg[index] = 0;
        val = 0;
        break;

    case MIKASA_RTC_STATUSD:
        val = MIKASA_RTC_POWER_OK;
        break;

    case MIKASA_RTC_CENTURY:
        val = mikasa_rtc_encode ((uint32) ((tm->tm_year + 1900) / 100));
        break;

    default:
        val = mikasa_rtc_reg[index];
        break;
        }
sim_debug (MIKASA_DBG_RTC, &mikasa_dev, "RTC read %02X=%02X\n",
    index, val);
return val;
}

static void mikasa_rtc_write_data (uint8 val)
{
uint8 index = mikasa_rtc_index & 0x7F;

if ((index != MIKASA_RTC_INTR) && (index != MIKASA_RTC_STATUSD))
    mikasa_rtc_reg[index] = val;
sim_debug (MIKASA_DBG_RTC, &mikasa_dev, "RTC write %02X=%02X\n",
    index, val);
return;
}

static uint8 mikasa_pit_read (uint32 port)
{
if (port == (MIKASA_ISA_PIT + 3))
    return mikasa_pit_mode;
return mikasa_pit_reg[port - MIKASA_ISA_PIT];
}

static void mikasa_pit_write (uint32 port, uint8 val)
{
if (port == (MIKASA_ISA_PIT + 3))
    mikasa_pit_mode = val;
else
    mikasa_pit_reg[port - MIKASA_ISA_PIT] = val;
return;
}

static t_bool mikasa_eisa_slot_id_port (uint32 port)
{
return ((port & 0x0FFF) >= MIKASA_EISA_SLOT_ID) &&
    ((port & 0x0FFF) < (MIKASA_EISA_SLOT_ID + 4));
}

static uint8 mikasa_eisa_read (uint32 port)
{
if ((port >= MIKASA_EISA_CRAM) &&
    (port < (MIKASA_EISA_CRAM + MIKASA_EISA_CRAM_SIZE))) {
    uint8 val = mikasa_eisa_cram[mikasa_eisa_cram_page &
        (MIKASA_EISA_CRAM_PAGES - 1)][port - MIKASA_EISA_CRAM];
    sim_debug (MIKASA_DBG_EISA, &mikasa_dev,
        "EISA CRAM read page %02X offset %02X=%02X\n",
        mikasa_eisa_cram_page, port - MIKASA_EISA_CRAM, val);
    return val;
    }
if (port == MIKASA_EISA_CRAM_PAGE_REG) {
    sim_debug (MIKASA_DBG_EISA, &mikasa_dev,
        "EISA CRAM page read %02X\n", mikasa_eisa_cram_page);
    return mikasa_eisa_cram_page;
    }
if (mikasa_eisa_slot_id_port (port)) {
    sim_debug (MIKASA_DBG_EISA, &mikasa_dev,
        "EISA slot id read %04X=FF\n", port);
    return 0xFF;
    }
return 0;
}

static t_bool mikasa_eisa_write (uint32 port, uint8 val)
{
if ((port >= MIKASA_EISA_CRAM) &&
    (port < (MIKASA_EISA_CRAM + MIKASA_EISA_CRAM_SIZE))) {
    mikasa_eisa_cram[mikasa_eisa_cram_page &
        (MIKASA_EISA_CRAM_PAGES - 1)][port - MIKASA_EISA_CRAM] = val;
    sim_debug (MIKASA_DBG_EISA, &mikasa_dev,
        "EISA CRAM write page %02X offset %02X=%02X\n",
        mikasa_eisa_cram_page, port - MIKASA_EISA_CRAM, val);
    return TRUE;
    }
if (port == MIKASA_EISA_CRAM_PAGE_REG) {
    mikasa_eisa_cram_page = val & (MIKASA_EISA_CRAM_PAGES - 1);
    sim_debug (MIKASA_DBG_EISA, &mikasa_dev,
        "EISA CRAM page write %02X\n", mikasa_eisa_cram_page);
    return TRUE;
    }
if (mikasa_eisa_slot_id_port (port)) {
    sim_debug (MIKASA_DBG_EISA, &mikasa_dev,
        "EISA slot id write %04X=%02X\n", port, val);
    return TRUE;
    }
return FALSE;
}

static t_uint64 mikasa_sparse_pack (uint32 off, uint32 val, uint32 lnt,
    uint32 mode)
{
uint32 port = off >> 5;

if (mode == 1)
    return ((t_uint64) (val & 0xFFFF)) << (8 * (port & 2));
if (mode == 3)
    return val;
if (lnt == L_BYTE)
    return val & 0xFF;
return ((t_uint64) (val & 0xFF)) << (8 * (port & 3));
}

static t_uint64 mikasa_epic_read (t_uint64 pa, uint32 lnt)
{
uint32 off = (uint32) (pa - MIKASA_EPIC_BASE);
uint32 index = (off & (MIKASA_EPIC_SIZE - 1)) >> 5;
uint32 val = mikasa_epic_reg[index];

sim_debug (MIKASA_DBG_PCI, &mikasa_dev, "EPIC read %03X=%08X\n",
    off, val);
return val;
}

static void mikasa_epic_write (t_uint64 pa, t_uint64 val, uint32 lnt)
{
uint32 off = (uint32) (pa - MIKASA_EPIC_BASE);
uint32 index = (off & (MIKASA_EPIC_SIZE - 1)) >> 5;

mikasa_epic_reg[index] = (uint32) val;
sim_debug (MIKASA_DBG_PCI, &mikasa_dev, "EPIC write %03X=%08X\n",
    off, (uint32) val);
return;
}

static t_bool mikasa_pceb_bar_reg (uint32 reg)
{
return ((reg >= 0x10) && (reg <= 0x24)) || (reg == 0x30);
}

static uint32 mikasa_masked_update (uint32 old, uint32 val, uint32 mask)
{
return (old & ~mask) | (val & mask);
}

static uint32 mikasa_ncr_conf_mask (uint32 reg)
{
switch (reg) {
    case 0x04:
        return 0x00000157u;

    case 0x0C:
        return 0x0000FFFFu;

    case 0x10:
    case 0x14:
        return MIKASA_NCR_BAR_MASK;

    case 0x3C:
        return 0x000000FFu;

    default:
        return 0;
        }
}

static uint8 mikasa_ncr_read_b (uint32 reg)
{
uint8 val = mikasa_ncr_reg[reg & (MIKASA_NCR_REG_SIZE - 1)];

if ((reg & (MIKASA_NCR_REG_SIZE - 1)) == MIKASA_NCR_REG_DSTAT)
    mikasa_ncr_reg[MIKASA_NCR_REG_DSTAT] &= MIKASA_NCR_DSTAT_DFE;
else if (((reg & (MIKASA_NCR_REG_SIZE - 1)) == MIKASA_NCR_REG_SIST0) ||
    ((reg & (MIKASA_NCR_REG_SIZE - 1)) == MIKASA_NCR_REG_SIST1))
    mikasa_ncr_reg[reg & (MIKASA_NCR_REG_SIZE - 1)] = 0;
sim_debug (MIKASA_DBG_PCI, &mikasa_dev, "NCR read %02X=%02X\n",
    reg, val);
return val;
}

static uint32 mikasa_ncr_read_l (uint32 reg)
{
uint32 val;

reg = reg & (MIKASA_NCR_REG_SIZE - 1);
val = ((uint32) mikasa_ncr_read_b (reg)) |
    (((uint32) mikasa_ncr_read_b (reg + 1)) << 8) |
    (((uint32) mikasa_ncr_read_b (reg + 2)) << 16) |
    (((uint32) mikasa_ncr_read_b (reg + 3)) << 24);
return val;
}

static t_uint64 mikasa_ncr_read_len (uint32 reg, uint32 lnt)
{
if (lnt == L_BYTE)
    return mikasa_ncr_read_b (reg);
if (lnt == L_WORD)
    return ((uint32) mikasa_ncr_read_b (reg)) |
        (((uint32) mikasa_ncr_read_b (reg + 1)) << 8);
return mikasa_ncr_read_l (reg);
}

static void mikasa_ncr_init_regs (void)
{
memset (mikasa_ncr_reg, 0, sizeof (mikasa_ncr_reg));
mikasa_ncr_reg[MIKASA_NCR_REG_SCNTL0] = MIKASA_NCR_SCNTL0_ARB;
mikasa_ncr_reg[MIKASA_NCR_REG_DSTAT] = MIKASA_NCR_DSTAT_DFE;
mikasa_ncr_reg[MIKASA_NCR_REG_CTEST1] = MIKASA_NCR_CTEST1_FMT;
mikasa_ncr_reg[MIKASA_NCR_REG_CTEST2] = MIKASA_NCR_CTEST2_DACK;
mikasa_ncr_reg[MIKASA_NCR_REG_CTEST3] = 0x10;
mikasa_ncr_reg[MIKASA_NCR_REG_MACNTL] = MIKASA_NCR_MACNTL_810;
mikasa_ncr_reg[MIKASA_NCR_REG_GPCNTL] = 0x0F;
mikasa_ncr_reg[MIKASA_NCR_REG_STEST0] = 0x03;
return;
}

static void mikasa_ncr_write_b (uint32 reg, uint8 val)
{
reg = reg & (MIKASA_NCR_REG_SIZE - 1);
sim_debug (MIKASA_DBG_PCI, &mikasa_dev, "NCR write %02X=%02X\n",
    reg, val);
mikasa_ncr_reg[reg] = val;
if ((reg == MIKASA_NCR_REG_ISTAT) && (val & MIKASA_NCR_ISTAT_SRST))
    mikasa_ncr_init_regs ();
return;
}

static void mikasa_ncr_write_l (uint32 reg, uint32 val)
{
mikasa_ncr_write_b (reg, (uint8) val);
mikasa_ncr_write_b (reg + 1, (uint8) (val >> 8));
mikasa_ncr_write_b (reg + 2, (uint8) (val >> 16));
mikasa_ncr_write_b (reg + 3, (uint8) (val >> 24));
return;
}

static void mikasa_ncr_write_len (uint32 reg, t_uint64 val, uint32 lnt)
{
if (lnt == L_BYTE)
    mikasa_ncr_write_b (reg, (uint8) val);
else if (lnt == L_WORD) {
    mikasa_ncr_write_b (reg, (uint8) val);
    mikasa_ncr_write_b (reg + 1, (uint8) (val >> 8));
    }
else
    mikasa_ncr_write_l (reg, (uint32) val);
return;
}

static t_bool mikasa_ncr_bar_reg (uint32 addr, uint32 bar, uint32 mask,
    uint32 *reg)
{
uint32 base;

base = mikasa_ncr_cfg[bar >> 2] & ~0xFFu;
base = base & mask;
if (base == 0)
    return FALSE;
if ((addr < base) || (addr >= (base + MIKASA_NCR_BAR_SIZE)))
    return FALSE;
*reg = addr - base;
return TRUE;
}

static t_bool mikasa_ncr_io_read (uint32 port, uint8 *val)
{
uint32 reg;

if (!mikasa_ncr_bar_reg (port, 0x10, 0xFFFFFFFFu, &reg))
    return FALSE;
*val = mikasa_ncr_read_b (reg);
return TRUE;
}

static t_bool mikasa_ncr_io_write (uint32 port, uint8 val)
{
uint32 reg;

if (!mikasa_ncr_bar_reg (port, 0x10, 0xFFFFFFFFu, &reg))
    return FALSE;
mikasa_ncr_write_b (reg, val);
return TRUE;
}

static uint32 mikasa_ncr_conf_read_l (uint32 func, uint32 reg)
{
if (func != 0)
    return 0xFFFFFFFFu;
if (reg >= 0x80)
    return mikasa_ncr_read_l (reg - 0x80);
return mikasa_ncr_cfg[reg >> 2];
}

static void mikasa_ncr_conf_write_l (uint32 func, uint32 reg, uint32 val)
{
uint32 mask;

if (func != 0)
    return;
if (reg >= 0x80) {
    mikasa_ncr_write_l (reg - 0x80, val);
    return;
    }
mask = mikasa_ncr_conf_mask (reg);
if (mask != 0)
    mikasa_ncr_cfg[reg >> 2] =
        mikasa_masked_update (mikasa_ncr_cfg[reg >> 2], val, mask);
return;
}

static uint32 mikasa_pci_conf_read_l (uint32 cfg)
{
uint32 bus = (cfg >> 16) & 0xFF;
uint32 slot = (cfg >> 11) & 0x1F;
uint32 func = (cfg >> 8) & 7;
uint32 reg = cfg & 0xFC;
uint32 val = 0xFFFFFFFFu;

if ((bus == 0) && (slot == MIKASA_PCI_SLOT_PCEB)) {
    val = (func == 0) ? mikasa_pceb_cfg[reg >> 2] : 0xFFFFFFFFu;
    sim_debug (MIKASA_DBG_PCI, &mikasa_dev,
        "PCI config read bus %u slot %u func %u reg %02X=%08X\n",
        bus, slot, func, reg, val);
    return val;
    }
if ((bus == 0) && (slot == MIKASA_PCI_SLOT_SCSI)) {
    val = mikasa_ncr_conf_read_l (func, reg);
    sim_debug (MIKASA_DBG_PCI, &mikasa_dev,
        "PCI config read bus %u slot %u func %u reg %02X=%08X\n",
        bus, slot, func, reg, val);
    return val;
    }
sim_debug (MIKASA_DBG_PCI, &mikasa_dev,
    "PCI config read bus %u slot %u func %u reg %02X=FFFFFFFF\n",
    bus, slot, func, reg);
return val;
}

static void mikasa_pci_conf_write_l (uint32 cfg, uint32 val)
{
uint32 bus = (cfg >> 16) & 0xFF;
uint32 slot = (cfg >> 11) & 0x1F;
uint32 func = (cfg >> 8) & 7;
uint32 reg = cfg & 0xFC;

if ((bus == 0) && (slot == MIKASA_PCI_SLOT_PCEB) && (func == 0)) {
    if ((reg != 0x00) && (reg != 0x08) && !mikasa_pceb_bar_reg (reg))
        mikasa_pceb_cfg[reg >> 2] = val;
    sim_debug (MIKASA_DBG_PCI, &mikasa_dev,
        "PCI config write bus %u slot %u func %u reg %02X=%08X\n",
        bus, slot, func, reg, val);
    return;
    }
if ((bus == 0) && (slot == MIKASA_PCI_SLOT_SCSI)) {
    mikasa_ncr_conf_write_l (func, reg, val);
    sim_debug (MIKASA_DBG_PCI, &mikasa_dev,
        "PCI config write bus %u slot %u func %u reg %02X=%08X\n",
        bus, slot, func, reg, val);
    return;
    }
sim_debug (MIKASA_DBG_PCI, &mikasa_dev,
    "PCI config write bus %u slot %u func %u reg %02X=%08X\n",
    bus, slot, func, reg, val);
return;
}

static t_uint64 mikasa_pci_conf_read (t_uint64 pa, uint32 lnt)
{
uint32 off = (uint32) (pa - MIKASA_APECS_PCI_CONF);
uint32 mode = (off >> 3) & 3;
uint32 cfg = off >> 5;
uint32 val;

val = mikasa_pci_conf_read_l (cfg & ~3u);
return mikasa_sparse_pack (off, val >> (8 * (cfg & 3)), lnt, mode);
}

static void mikasa_pci_conf_write (t_uint64 pa, t_uint64 val, uint32 lnt)
{
uint32 off = (uint32) (pa - MIKASA_APECS_PCI_CONF);
uint32 mode = (off >> 3) & 3;
uint32 cfg = off >> 5;
uint32 data;

if (mode == 3)
    data = (uint32) val;
else if (mode == 1)
    data = (uint32) (val >> (8 * (cfg & 2)));
else if (lnt == L_BYTE)
    data = (uint32) val;
else
    data = (uint32) (val >> (8 * (cfg & 3)));
mikasa_pci_conf_write_l (cfg & ~3u, data);
return;
}

static uint8 mikasa_isa_read (uint32 port)
{
uint32 icu = mikasa_irq_mask | MIKASA_ISA_ICU_PRESENT;
uint8 val;

if (mikasa_ncr_io_read (port, &val))
    return val;
if ((port >= MIKASA_ISA_DMA1) && (port < (MIKASA_ISA_DMA1 + 0x10)))
    return 0;
if ((port == MIKASA_ISA_PIC1) || (port == (MIKASA_ISA_PIC1 + 1)) ||
    (port == MIKASA_ISA_PIC2) || (port == (MIKASA_ISA_PIC2 + 1)))
    return mikasa_pic_read (port);
if (port == MIKASA_ISA_CFG_INDEX)
    return mikasa_cfg_index;
if (port == MIKASA_ISA_CFG_DATA)
    return mikasa_cfg_reg[mikasa_cfg_index];
if ((port >= MIKASA_ISA_PIT) && (port < (MIKASA_ISA_PIT + 4)))
    return mikasa_pit_read (port);
if (port == MIKASA_ISA_PORTB)
    return mikasa_portb;
if (port == MIKASA_ISA_RTC_INDEX)
    return mikasa_rtc_index;
if (port == MIKASA_ISA_RTC_DATA)
    return mikasa_rtc_read_data ();
if ((port >= MIKASA_ISA_COM2) && (port < (MIKASA_ISA_COM2 + 8)))
    return mikasa_com2_read (port);
if (port == MIKASA_ISA_SIO_INDEX)
    return mikasa_sio_index;
if (port == MIKASA_ISA_SIO_DATA)
    return mikasa_sio_reg[mikasa_sio_index];
if ((port >= MIKASA_ISA_COM1) && (port < (MIKASA_ISA_COM1 + 8)))
    return mikasa_uart_read (port);
if (port == MIKASA_ISA_NMI_CTRL)
    return mikasa_nmi_ctrl;
if ((port >= MIKASA_ISA_OCP) && (port < (MIKASA_ISA_OCP + 4))) {
    uint32 reg = port - MIKASA_ISA_OCP;

    if (reg == 1)
        return mikasa_ocp_reg[reg] | MIKASA_OCP_READY;
    return mikasa_ocp_reg[reg];
    }
if ((port == MIKASA_ISA_ICU_IMR) || (port == (MIKASA_ISA_ICU_IMR + 1)))
    return (uint8) (icu >> ((port - MIKASA_ISA_ICU_IMR) << 3));
if ((port == MIKASA_EISA_CRAM_PAGE_REG) ||
    ((port >= MIKASA_EISA_CRAM) &&
     (port < (MIKASA_EISA_CRAM + MIKASA_EISA_CRAM_SIZE))) ||
    mikasa_eisa_slot_id_port (port))
    return mikasa_eisa_read (port);
sim_debug (MIKASA_DBG_IO, &mikasa_dev, "unhandled ISA read %03X\n", port);
return 0;
}

static void mikasa_isa_write (uint32 port, uint8 val)
{
if (mikasa_ncr_io_write (port, val))
    return;
if ((port >= MIKASA_ISA_DMA1) && (port < (MIKASA_ISA_DMA1 + 0x10)))
    return;
if ((port == MIKASA_ISA_PIC1) || (port == (MIKASA_ISA_PIC1 + 1)) ||
    (port == MIKASA_ISA_PIC2) || (port == (MIKASA_ISA_PIC2 + 1))) {
    mikasa_pic_write (port, val);
    return;
    }
if (port == MIKASA_ISA_CFG_INDEX) {
    mikasa_cfg_index = val;
    return;
    }
if (port == MIKASA_ISA_CFG_DATA) {
    mikasa_cfg_reg[mikasa_cfg_index] = val;
    return;
    }
if ((port >= MIKASA_ISA_PIT) && (port < (MIKASA_ISA_PIT + 4))) {
    mikasa_pit_write (port, val);
    return;
    }
if (port == MIKASA_ISA_PORTB) {
    mikasa_portb = val;
    return;
    }
if (port == MIKASA_ISA_RTC_INDEX) {
    mikasa_rtc_index = val;
    return;
    }
if (port == MIKASA_ISA_RTC_DATA) {
    mikasa_rtc_write_data (val);
    return;
    }
if ((port >= MIKASA_ISA_COM2) && (port < (MIKASA_ISA_COM2 + 8))) {
    mikasa_com2_write (port, val);
    return;
    }
if (port == MIKASA_ISA_SIO_INDEX) {
    mikasa_sio_index = val;
    return;
    }
if (port == MIKASA_ISA_SIO_DATA) {
    mikasa_sio_reg[mikasa_sio_index] = val;
    return;
    }
if ((port >= MIKASA_ISA_COM1) && (port < (MIKASA_ISA_COM1 + 8))) {
    mikasa_uart_write (port, val);
    return;
    }
if (port == MIKASA_ISA_NMI_CTRL) {
    mikasa_nmi_ctrl = val;
    return;
    }
if ((port >= MIKASA_ISA_OCP) && (port < (MIKASA_ISA_OCP + 4))) {
    mikasa_ocp_reg[port - MIKASA_ISA_OCP] = val;
    return;
    }
if (port == MIKASA_ISA_ICU_IMR) {
    mikasa_pic_imr[0] = val;
    mikasa_pic_update_mask ();
    }
else if (port == (MIKASA_ISA_ICU_IMR + 1)) {
    mikasa_pic_imr[1] = val;
    mikasa_pic_update_mask ();
    }
else if (mikasa_eisa_write (port, val))
    return;
else
    sim_debug (MIKASA_DBG_IO, &mikasa_dev,
        "unhandled ISA write %03X=%02X\n", port, val);
return;
}

static t_uint64 mikasa_sparse_read (t_uint64 pa, uint32 lnt)
{
t_uint64 off = pa - MIKASA_APECS_PCI_SIO;
uint32 port = (uint32) (off >> 5);
uint32 mode = (uint32) ((off >> 3) & 3);
uint32 val = 0;

if (mode == 1) {
    val = ((uint32) mikasa_isa_read (port & ~1u)) |
        (((uint32) mikasa_isa_read ((port & ~1u) + 1)) << 8);
    return ((t_uint64) val) << (8 * (port & 2));
    }
if (mode == 3) {
    val = ((uint32) mikasa_isa_read (port & ~3u)) |
        (((uint32) mikasa_isa_read ((port & ~3u) + 1)) << 8) |
        (((uint32) mikasa_isa_read ((port & ~3u) + 2)) << 16) |
        (((uint32) mikasa_isa_read ((port & ~3u) + 3)) << 24);
    return val;
    }
val = mikasa_isa_read (port);
if (lnt == L_BYTE)
    return val;
return ((t_uint64) val) << (8 * (port & 3));
}

static void mikasa_sparse_write (t_uint64 pa, t_uint64 val, uint32 lnt)
{
t_uint64 off = pa - MIKASA_APECS_PCI_SIO;
uint32 port = (uint32) (off >> 5);
uint32 mode = (uint32) ((off >> 3) & 3);

if (mode == 1) {
    uint32 data = (uint32) (val >> (8 * (port & 2)));

    mikasa_isa_write (port & ~1u, (uint8) data);
    mikasa_isa_write ((port & ~1u) + 1, (uint8) (data >> 8));
    return;
    }
if (mode == 3) {
    uint32 data = (uint32) val;

    mikasa_isa_write (port & ~3u, (uint8) data);
    mikasa_isa_write ((port & ~3u) + 1, (uint8) (data >> 8));
    mikasa_isa_write ((port & ~3u) + 2, (uint8) (data >> 16));
    mikasa_isa_write ((port & ~3u) + 3, (uint8) (data >> 24));
    return;
    }
if (lnt == L_BYTE)
    mikasa_isa_write (port, (uint8) val);
else
    mikasa_isa_write (port, (uint8) (val >> (8 * (port & 3))));
return;
}

static t_uint64 mikasa_pci_sparse_mem_read (t_uint64 pa, uint32 lnt)
{
t_uint64 off = pa - MIKASA_APECS_PCI_SPARSE;
uint32 addr = (uint32) (off >> 5);
uint32 mode = (uint32) ((off >> 3) & 3);
uint32 reg;
uint32 val;

if (mikasa_ncr_bar_reg (addr, 0x14, 0x07FFFFFFu, &reg)) {
    if (mode == 3)
        return mikasa_ncr_read_l (reg & ~3u);
    if (mode == 1) {
        val = (uint32) mikasa_ncr_read_len (reg & ~1u, L_WORD);
        return ((t_uint64) val) << (8 * (addr & 2));
        }
    val = (uint32) mikasa_ncr_read_b (reg);
    if (lnt == L_BYTE)
        return val;
    return ((t_uint64) val) << (8 * (addr & 3));
    }
sim_debug (MIKASA_DBG_IO, &mikasa_dev,
    "absent APECS sparse memory read %llX\n",
    (unsigned long long) pa);
return M32;
}

static void mikasa_pci_sparse_mem_write (t_uint64 pa, t_uint64 val,
    uint32 lnt)
{
t_uint64 off = pa - MIKASA_APECS_PCI_SPARSE;
uint32 addr = (uint32) (off >> 5);
uint32 mode = (uint32) ((off >> 3) & 3);
uint32 reg;
uint32 data;

if (mikasa_ncr_bar_reg (addr, 0x14, 0x07FFFFFFu, &reg)) {
    if (mode == 3)
        mikasa_ncr_write_l (reg & ~3u, (uint32) val);
    else if (mode == 1) {
        data = (uint32) (val >> (8 * (addr & 2)));
        mikasa_ncr_write_len (reg & ~1u, data, L_WORD);
        }
    else if (lnt == L_BYTE)
        mikasa_ncr_write_b (reg, (uint8) val);
    else {
        data = (uint32) (val >> (8 * (addr & 3)));
        mikasa_ncr_write_b (reg, (uint8) data);
        }
    return;
    }
sim_debug (MIKASA_DBG_IO, &mikasa_dev,
    "unhandled APECS sparse memory write %llX=%llX\n",
    (unsigned long long) pa, (unsigned long long) val);
return;
}

static t_uint64 mikasa_pci_dense_mem_read (t_uint64 pa, uint32 lnt)
{
uint32 addr = (uint32) (pa - MIKASA_APECS_PCI_DENSE);
uint32 reg;

if (mikasa_ncr_bar_reg (addr, 0x14, 0xFFFFFFFFu, &reg))
    return mikasa_ncr_read_len (reg, lnt);
sim_debug (MIKASA_DBG_IO, &mikasa_dev,
    "absent APECS dense memory read %llX\n",
    (unsigned long long) pa);
return M32;
}

static void mikasa_pci_dense_mem_write (t_uint64 pa, t_uint64 val,
    uint32 lnt)
{
uint32 addr = (uint32) (pa - MIKASA_APECS_PCI_DENSE);
uint32 reg;

if (mikasa_ncr_bar_reg (addr, 0x14, 0xFFFFFFFFu, &reg)) {
    mikasa_ncr_write_len (reg, val, lnt);
    return;
    }
sim_debug (MIKASA_DBG_IO, &mikasa_dev,
    "unhandled APECS dense memory write %llX=%llX\n",
    (unsigned long long) pa, (unsigned long long) val);
return;
}

static t_bool mikasa_io_rd (t_uint64 pa, t_uint64 *val, uint32 lnt)
{
if ((pa >= MIKASA_EPIC_BASE) && (pa < (MIKASA_EPIC_BASE + MIKASA_EPIC_SIZE)))
    *val = mikasa_epic_read (pa, lnt);
else if (pa == MIKASA_APECS_PCI_IACK)
    *val = 0;
else if ((pa >= MIKASA_APECS_PCI_SIO) &&
    (pa < (MIKASA_APECS_PCI_SIO + MIKASA_APECS_PCI_SIO_SIZE)))
    *val = mikasa_sparse_read (pa, lnt);
else if ((pa >= MIKASA_APECS_PCI_CONF) &&
    (pa < (MIKASA_APECS_PCI_CONF + MIKASA_APECS_PCI_CONF_SIZE)))
    *val = mikasa_pci_conf_read (pa, lnt);
else if ((pa >= MIKASA_APECS_PCI_SPARSE) &&
    (pa < (MIKASA_APECS_PCI_SPARSE + MIKASA_APECS_PCI_SPARSE_SIZE)))
    *val = mikasa_pci_sparse_mem_read (pa, lnt);
else if ((pa >= MIKASA_APECS_PCI_DENSE) &&
    (pa < (MIKASA_APECS_PCI_DENSE + MIKASA_APECS_PCI_DENSE_SIZE)))
    *val = mikasa_pci_dense_mem_read (pa, lnt);
else {
    sim_debug (MIKASA_DBG_IO, &mikasa_dev,
        "unhandled APECS read %llX\n", (unsigned long long) pa);
    *val = M32;
    }
return TRUE;
}

static t_bool mikasa_io_wr (t_uint64 pa, t_uint64 val, uint32 lnt)
{
if ((pa >= MIKASA_EPIC_BASE) && (pa < (MIKASA_EPIC_BASE + MIKASA_EPIC_SIZE)))
    mikasa_epic_write (pa, val, lnt);
else if ((pa >= MIKASA_APECS_PCI_SIO) &&
    (pa < (MIKASA_APECS_PCI_SIO + MIKASA_APECS_PCI_SIO_SIZE)))
    mikasa_sparse_write (pa, val, lnt);
else if ((pa >= MIKASA_APECS_PCI_CONF) &&
    (pa < (MIKASA_APECS_PCI_CONF + MIKASA_APECS_PCI_CONF_SIZE)))
    mikasa_pci_conf_write (pa, val, lnt);
else if ((pa >= MIKASA_APECS_PCI_SPARSE) &&
    (pa < (MIKASA_APECS_PCI_SPARSE + MIKASA_APECS_PCI_SPARSE_SIZE)))
    mikasa_pci_sparse_mem_write (pa, val, lnt);
else if ((pa >= MIKASA_APECS_PCI_DENSE) &&
    (pa < (MIKASA_APECS_PCI_DENSE + MIKASA_APECS_PCI_DENSE_SIZE)))
    mikasa_pci_dense_mem_write (pa, val, lnt);
else
    sim_debug (MIKASA_DBG_IO, &mikasa_dev,
        "unhandled APECS write %llX=%llX\n",
        (unsigned long long) pa, (unsigned long long) val);
return TRUE;
}

static void mikasa_zero_phys (t_uint64 pa, uint32 size)
{
uint32 i;

for (i = 0; i < size; i = i + 8)
    WritePQ (pa + i, 0);
return;
}

static t_stat mikasa_ensure_mem (void)
{
if (M == NULL) M = (t_uint64 *) calloc (((uint32) MEMSIZE) >> 3,
    sizeof (t_uint64));
if (M == NULL) return SCPE_MEM;
return SCPE_OK;
}

static void mikasa_copy_cstr (char *dst, uint32 dst_size, const uint8 *src,
    uint32 src_size)
{
uint32 i;

if (dst_size == 0)
    return;
for (i = 0; (i < (dst_size - 1)) && (i < src_size) && (src[i] != 0); i++)
    dst[i] = (char) src[i];
dst[i] = 0;
return;
}

static uint32 mikasa_get_le32 (const uint8 *buf)
{
return ((uint32) buf[0]) |
    (((uint32) buf[1]) << 8) |
    (((uint32) buf[2]) << 16) |
    (((uint32) buf[3]) << 24);
}

static t_uint64 mikasa_get_le64 (const uint8 *buf)
{
return ((t_uint64) buf[0]) |
    (((t_uint64) buf[1]) << 8) |
    (((t_uint64) buf[2]) << 16) |
    (((t_uint64) buf[3]) << 24) |
    (((t_uint64) buf[4]) << 32) |
    (((t_uint64) buf[5]) << 40) |
    (((t_uint64) buf[6]) << 48) |
    (((t_uint64) buf[7]) << 56);
}

static t_bool mikasa_lfu_header_valid (const uint8 *header)
{
static const uint8 magic[MIKASA_LFU_MAGIC_SIZE] =
    { 'L', 'F', 'U', ' ', 'A', 'P', 'U', 0 };
static const uint8 trailer[4] = { 0x44, 0x33, 0x22, 0x11 };

return (memcmp (header + MIKASA_LFU_MAGIC_OFF, magic, sizeof (magic)) == 0) &&
    (memcmp (header + MIKASA_LFU_TRAILER_OFF, trailer, sizeof (trailer)) == 0);
}

static t_stat mikasa_load_bytes (FILE *fileref, t_uint64 pa, t_uint64 bytes)
{
uint8 buf[4096];
t_uint64 done = 0;

if ((bytes != 0) && !ADDR_IS_MEM (pa + bytes - 1))
    return SCPE_NXM;

while (done < bytes) {
    size_t cnt = sizeof (buf);
    size_t i;

    if ((bytes - done) < cnt)
        cnt = (size_t) (bytes - done);
    if (sim_fread (buf, 1, cnt, fileref) != cnt)
        return SCPE_IOERR;
    for (i = 0; i < cnt; i++)
        WritePB (pa + done + i, buf[i]);
    done = done + cnt;
    }

return SCPE_OK;
}

static void mikasa_prepare_rom_cpu (t_uint64 pc, t_uint64 palbase)
{
uint32 i;

for (i = 0; i < 32; i++)
    R[i] = 0;
R[31] = 0;
PC = pc & ~((t_uint64) 3);
pc_align = (uint32) (pc & 3);
ev5_palbase = palbase;
pal_mode = 1;
dmapen = 0;
fpen = 1;
pal_type = PAL_VMS;
lock_flag = 0;
return;
}

static void mikasa_note_rom (const char *path, t_uint64 pc,
    t_uint64 palbase)
{
mikasa_rom_pc = pc;
mikasa_rom_palbase = palbase;
if (path != NULL) {
    strncpy (mikasa_rom_path, path, sizeof (mikasa_rom_path) - 1);
    mikasa_rom_path[sizeof (mikasa_rom_path) - 1] = 0;
    }
mikasa_rom_loaded = TRUE;
mikasa_prepare_rom_cpu (pc, palbase);
return;
}

static t_stat mikasa_load_lfu_rom (FILE *fileref, t_offset fsize,
    const char *path, const uint8 *header)
{
t_stat r;
t_uint64 bytes;
t_uint64 available;
t_uint64 zero_size;
char version[MIKASA_LFU_VERSION_SIZE + 1];
char vendor[MIKASA_LFU_VENDOR_SIZE + 1];
char platform[MIKASA_LFU_PLATFORM_SIZE + 1];
char arch[MIKASA_LFU_ARCH_SIZE + 1];
char kind[MIKASA_LFU_KIND_SIZE + 1];

if (fsize <= MIKASA_LFU_HEADER_SIZE)
    return SCPE_FMT;
available = (t_uint64) (fsize - MIKASA_LFU_HEADER_SIZE);
bytes = mikasa_get_le32 (header + MIKASA_LFU_IMAGE_SIZE_OFF);
if ((bytes == 0) || (bytes > available))
    return SCPE_FMT;
if (!ADDR_IS_MEM (MIKASA_ROM_LOAD_PA + bytes - 1))
    return SCPE_NXM;

zero_size = MEMSIZE < MIKASA_ROM_WORKSPACE_SIZE ? MEMSIZE :
    MIKASA_ROM_WORKSPACE_SIZE;
mikasa_zero_phys (0, (uint32) zero_size);

if (sim_fseeko (fileref, MIKASA_LFU_HEADER_SIZE, SEEK_SET))
    return SCPE_IOERR;
r = mikasa_load_bytes (fileref, MIKASA_ROM_LOAD_PA, bytes);
if (r != SCPE_OK)
    return r;

mikasa_copy_cstr (version, sizeof (version),
    header + MIKASA_LFU_VERSION_OFF, MIKASA_LFU_VERSION_SIZE);
mikasa_copy_cstr (vendor, sizeof (vendor),
    header + MIKASA_LFU_VENDOR_OFF, MIKASA_LFU_VENDOR_SIZE);
mikasa_copy_cstr (platform, sizeof (platform),
    header + MIKASA_LFU_PLATFORM_OFF, MIKASA_LFU_PLATFORM_SIZE);
mikasa_copy_cstr (arch, sizeof (arch),
    header + MIKASA_LFU_ARCH_OFF, MIKASA_LFU_ARCH_SIZE);
mikasa_copy_cstr (kind, sizeof (kind),
    header + MIKASA_LFU_KIND_OFF, MIKASA_LFU_KIND_SIZE);

mikasa_note_rom (path, MIKASA_ROM_LOAD_PA | 1, MIKASA_ROM_LOAD_PA);
sim_printf ("Loaded Mikasa SRM LFU ROM from %s: %s/%s/%s %s %s, "
    "%llu bytes at %08llX, PC=%08llX PALBASE=%08llX\n",
    path, vendor, platform, arch, kind, version,
    (unsigned long long) bytes,
    (unsigned long long) MIKASA_ROM_LOAD_PA,
    (unsigned long long) PC,
    (unsigned long long) ev5_palbase);
return SCPE_OK;
}

static t_stat mikasa_load_axpbox_rom (FILE *fileref, const char *path,
    const uint8 *header)
{
t_stat r;
t_uint64 pc = mikasa_get_le64 (header);
t_uint64 palbase = mikasa_get_le64 (header + 8);

if (!ADDR_IS_MEM (MIKASA_AXPBOX_ROM_MEMSIZE - 1))
    return SCPE_NXM;
mikasa_zero_phys (0, MIKASA_AXPBOX_ROM_MEMSIZE);
if (sim_fseeko (fileref, 16, SEEK_SET))
    return SCPE_IOERR;
r = mikasa_load_bytes (fileref, 0, MIKASA_AXPBOX_ROM_MEMSIZE);
    if (r != SCPE_OK)
        return r;

mikasa_note_rom (path, pc, palbase);
sim_printf ("Loaded AXPbox decompressed SRM ROM from %s: "
    "PC=%08llX PALBASE=%08llX, %u bytes at 00000000\n",
    path,
    (unsigned long long) PC,
    (unsigned long long) ev5_palbase,
    MIKASA_AXPBOX_ROM_MEMSIZE);
return SCPE_OK;
}

static t_stat mikasa_load_raw_rom_payload (FILE *fileref, t_offset fsize,
    const char *path)
{
t_stat r;
t_uint64 zero_size;

if (fsize <= 0)
    return SCPE_FMT;
if (!ADDR_IS_MEM (MIKASA_ROM_LOAD_PA + ((t_uint64) fsize) - 1))
    return SCPE_NXM;

zero_size = MEMSIZE < MIKASA_ROM_WORKSPACE_SIZE ? MEMSIZE :
    MIKASA_ROM_WORKSPACE_SIZE;
mikasa_zero_phys (0, (uint32) zero_size);

if (sim_fseeko (fileref, 0, SEEK_SET))
    return SCPE_IOERR;
r = mikasa_load_bytes (fileref, MIKASA_ROM_LOAD_PA, (t_uint64) fsize);
if (r != SCPE_OK)
    return r;

mikasa_note_rom (path, MIKASA_ROM_LOAD_PA | 1, MIKASA_ROM_LOAD_PA);
sim_printf ("Loaded Mikasa SRM ROM payload from %s: %llu bytes at %08llX, "
    "PC=%08llX PALBASE=%08llX\n",
    path,
    (unsigned long long) fsize,
    (unsigned long long) MIKASA_ROM_LOAD_PA,
    (unsigned long long) PC,
    (unsigned long long) ev5_palbase);
return SCPE_OK;
}

t_stat mikasa_set_rom (UNIT *uptr, int32 val, CONST char *cptr, void *desc)
{
FILE *fileref;
t_offset fsize;
uint8 header[MIKASA_LFU_HEADER_SIZE];
t_stat r;

if ((cptr == NULL) || (*cptr == 0))
    return SCPE_ARG;
r = mikasa_ensure_mem ();
if (r != SCPE_OK)
    return r;

fileref = sim_fopen (cptr, "rb");
if (fileref == NULL)
    return sim_messagef (SCPE_OPENERR,
        "MIKASA: cannot open ROM image %s\n", cptr);
fsize = sim_fsize_ex (fileref);
if (fsize < 16) {
    fclose (fileref);
    return SCPE_FMT;
    }
memset (header, 0, sizeof (header));
if (sim_fread (header, 1, 16, fileref) != 16) {
    fclose (fileref);
    return SCPE_IOERR;
    }

if (fsize == MIKASA_AXPBOX_ROM_SIZE) {
    r = mikasa_load_axpbox_rom (fileref, cptr, header);
    fclose (fileref);
    return r;
    }

if (fsize < MIKASA_LFU_HEADER_SIZE) {
    fclose (fileref);
    return SCPE_FMT;
    }
if (sim_fseeko (fileref, 0, SEEK_SET)) {
    fclose (fileref);
    return SCPE_IOERR;
    }
if (sim_fread (header, 1, sizeof (header), fileref) != sizeof (header)) {
    fclose (fileref);
    return SCPE_IOERR;
    }
if (!mikasa_lfu_header_valid (header)) {
    fclose (fileref);
    return SCPE_FMT;
    }
r = mikasa_load_lfu_rom (fileref, fsize, cptr, header);
fclose (fileref);
return r;
}

t_stat mikasa_set_rom_payload (UNIT *uptr, int32 val, CONST char *cptr,
    void *desc)
{
FILE *fileref;
t_offset fsize;
t_stat r;

if ((cptr == NULL) || (*cptr == 0))
    return SCPE_ARG;
r = mikasa_ensure_mem ();
if (r != SCPE_OK)
    return r;

fileref = sim_fopen (cptr, "rb");
if (fileref == NULL)
    return sim_messagef (SCPE_OPENERR,
        "MIKASA: cannot open ROM payload image %s\n", cptr);
fsize = sim_fsize_ex (fileref);
r = mikasa_load_raw_rom_payload (fileref, fsize, cptr);
fclose (fileref);
return r;
}

static t_stat mikasa_write_le64 (FILE *fileref, t_uint64 val)
{
uint8 buf[8];

buf[0] = (uint8) val;
buf[1] = (uint8) (val >> 8);
buf[2] = (uint8) (val >> 16);
buf[3] = (uint8) (val >> 24);
buf[4] = (uint8) (val >> 32);
buf[5] = (uint8) (val >> 40);
buf[6] = (uint8) (val >> 48);
buf[7] = (uint8) (val >> 56);
if (sim_fwrite (buf, 1, sizeof (buf), fileref) != sizeof (buf))
    return SCPE_IOERR;
return SCPE_OK;
}

t_stat mikasa_save_rom (UNIT *uptr, int32 val, CONST char *cptr, void *desc)
{
FILE *fileref;
t_uint64 pc;
t_uint64 pa;
t_stat r;

if ((cptr == NULL) || (*cptr == 0))
    return SCPE_ARG;
if (!ADDR_IS_MEM (MIKASA_AXPBOX_ROM_MEMSIZE - 1))
    return SCPE_NXM;

fileref = sim_fopen (cptr, "wb");
if (fileref == NULL)
    return sim_messagef (SCPE_OPENERR,
        "MIKASA: cannot open ROM output image %s\n", cptr);

pc = (PC & ~((t_uint64) 3)) | (pc_align & 3);
r = mikasa_write_le64 (fileref, pc);
if (r == SCPE_OK)
    r = mikasa_write_le64 (fileref, ev5_palbase);
for (pa = 0; (r == SCPE_OK) && (pa < MIKASA_AXPBOX_ROM_MEMSIZE); pa++) {
    uint8 byte = ReadPB (pa);

    if (sim_fwrite (&byte, 1, 1, fileref) != 1)
        r = SCPE_IOERR;
    }
fclose (fileref);
if (r != SCPE_OK)
    return r;

sim_printf ("Saved Mikasa decompressed SRM ROM to %s: PC=%08llX "
    "PALBASE=%08llX, %u bytes at 00000000\n",
    cptr,
    (unsigned long long) pc,
    (unsigned long long) ev5_palbase,
    MIKASA_AXPBOX_ROM_MEMSIZE);
return SCPE_OK;
}

t_stat mikasa_boot_rom (int32 unitno)
{
if (unitno != 0)
    return SCPE_ARG;
if (!mikasa_rom_loaded)
    return sim_messagef (SCPE_UNATT,
        "MIKASA: ROM not loaded; use SET MIKASA ROM=<path>\n");
mikasa_prepare_rom_cpu (mikasa_rom_pc, mikasa_rom_palbase);
sim_printf ("Starting Mikasa SRM ROM from %s: PC=%08llX PALBASE=%08llX\n",
    mikasa_rom_path[0] ? mikasa_rom_path : "loaded image",
    (unsigned long long) PC,
    (unsigned long long) ev5_palbase);
return SCPE_OK;
}

static t_uint64 mikasa_sum_qwords (t_uint64 pa, uint32 size)
{
t_uint64 sum = 0;
uint32 i;

for (i = 0; i < size; i = i + 8)
    sum = sum + ReadPQ (pa + i);
return sum;
}

static t_uint64 mikasa_fill_bitmap (t_uint64 bitmap_pa, t_uint64 bits)
{
t_uint64 qwords = (bits + 63) >> 6;
t_uint64 sum = 0;
t_uint64 i;

for (i = 0; i < qwords; i++) {
    t_uint64 q = M64;

    if ((i == (qwords - 1)) && (bits & 63))
        q = (((t_uint64) 1) << (bits & 63)) - 1;
    WritePQ (bitmap_pa + (i << 3), q);
    sum = sum + q;
    }
return sum;
}

static t_uint64 mikasa_hwrpb_va_for_pa (t_uint64 pa)
{
if ((pa >= MIKASA_HWRPB_PA) &&
    (pa < (MIKASA_HWRPB_PA + MIKASA_HWRPB_SIZE)))
    return MIKASA_HWRPB_VA + (pa - MIKASA_HWRPB_PA);
return pa;
}

static void mikasa_write_mem_cluster (t_uint64 mddt_pa, uint32 cluster,
    t_uint64 start_pfn, t_uint64 pfn_count, uint32 usage,
    t_uint64 *bitmap_pa)
{
t_uint64 cluster_pa = mddt_pa + 0x18 + (((t_uint64) cluster) *
    MIKASA_PMR_SIZE);
t_uint64 bitmap_sum = mikasa_fill_bitmap (*bitmap_pa, pfn_count);
t_uint64 bitmap_bytes = (pfn_count + 7) >> 3;
t_uint64 bitmap_va = mikasa_hwrpb_va_for_pa (*bitmap_pa);

WritePQ (cluster_pa + 0x000, start_pfn);
WritePQ (cluster_pa + 0x008, pfn_count);
WritePQ (cluster_pa + 0x010, pfn_count);
WritePQ (cluster_pa + 0x018, bitmap_va);
WritePQ (cluster_pa + 0x020, *bitmap_pa);
WritePQ (cluster_pa + 0x028, bitmap_sum);
WritePQ (cluster_pa + 0x030, usage);
*bitmap_pa = (*bitmap_pa + bitmap_bytes + 7) & ~((t_uint64) 7);
return;
}

static t_uint64 mikasa_pte (t_uint64 pa, uint32 flags)
{
return (((pa >> VA_N_OFF) & PFN_MASK) << PTE_V_PFN) | flags | PTE_V;
}

static t_bool mikasa_pte_to_pa (t_uint64 pte, t_uint64 page_off, t_uint64 *pa)
{
if ((pte & PTE_V) == 0)
    return FALSE;
*pa = (((pte >> PTE_V_PFN) & PFN_MASK) << VA_N_OFF) | page_off;
return ADDR_IS_MEM (*pa);
}

static t_bool mikasa_boot_pt_walk_pte (t_uint64 va, t_uint64 *pte)
{
uint32 vpn = VA_GETVPN (va);
t_uint64 l1pte, l2pte, l3pte;
t_uint64 l2_pa, l3_pa;

l1pte = ReadPQ (MIKASA_L1_PT_PA + VPN_GETLVL1 (vpn));
if (!mikasa_pte_to_pa (l1pte, 0, &l2_pa))
    return FALSE;
l2pte = ReadPQ (l2_pa + VPN_GETLVL2 (vpn));
if (!mikasa_pte_to_pa (l2pte, 0, &l3_pa))
    return FALSE;
l3pte = ReadPQ (l3_pa + VPN_GETLVL3 (vpn));
if ((l3pte & PTE_V) == 0)
    return FALSE;
*pte = l3pte;
return TRUE;
}

static t_bool mikasa_boot_pt_walk (t_uint64 va, t_uint64 *pa)
{
t_uint64 l3pte;

if (!mikasa_boot_pt_walk_pte (va, &l3pte))
    return FALSE;
return mikasa_pte_to_pa (l3pte, va & VA_M_OFF, pa);
}

static t_bool mikasa_boot_pt_va_to_pa (t_uint64 va, t_uint64 *pa)
{
t_uint64 pteva = va - MIKASA_PT_SPACE_VA;
uint32 pte_vpn;
t_uint64 l2_ptea;
t_uint64 l1pte, l2pte;
t_uint64 l2_pa, l3_pa;

if (pteva >= (((t_uint64) 1) << (VA_N_VPN + 3)))
    return FALSE;
pte_vpn = (uint32) (pteva >> 3);
l1pte = ReadPQ (MIKASA_L1_PT_PA + VPN_GETLVL1 (pte_vpn));
if (!mikasa_pte_to_pa (l1pte, 0, &l2_pa))
    return FALSE;
l2_ptea = l2_pa + VPN_GETLVL2 (pte_vpn);
l2pte = ReadPQ (l2_ptea);
if (!mikasa_pte_to_pa (l2pte, 0, &l3_pa)) {
    l3_pa = mikasa_alloc_l3_pt ();
    if (l3_pa == 0)
        return FALSE;
    WritePQ (l2_ptea, mikasa_pte (l3_pa, PTE_KRE | PTE_KWE));
    }
*pa = l3_pa + VPN_GETLVL3 (pte_vpn) + (va & 7);
return ADDR_IS_MEM (*pa);
}

static t_bool mikasa_boot_pt_va_to_pte (t_uint64 va, t_uint64 *pte)
{
t_uint64 pa;

if (!mikasa_boot_pt_va_to_pa (va, &pa))
    return FALSE;
*pte = mikasa_pte (pa & ~((t_uint64) VA_M_OFF), PTE_KRE | PTE_KWE);
return TRUE;
}

static t_bool mikasa_boot_va_to_pte (t_uint64 va, t_uint64 *pte)
{
t_uint64 pa;

if (va < MIKASA_BOOT_PT_RESERVED_END) {
    *pte = mikasa_pte (va, PTE_KRE | PTE_KWE);
    return TRUE;
    }
if ((va >= MIKASA_HWRPB_VA) &&
    (va < (MIKASA_HWRPB_VA + MIKASA_HWRPB_SIZE))) {
    pa = MIKASA_HWRPB_PA + (va - MIKASA_HWRPB_VA);
    *pte = mikasa_pte (pa, PTE_KRE | PTE_KWE);
    return TRUE;
    }
if ((va >= MIKASA_BOOT_IMAGE_VA) &&
    (va < (MIKASA_BOOT_IMAGE_VA + MIKASA_BOOT_RESERVED_SIZE))) {
    pa = MIKASA_BOOT_IMAGE_PA + (va - MIKASA_BOOT_IMAGE_VA);
    *pte = mikasa_pte (pa, PTE_KRE | PTE_KWE);
    return TRUE;
    }
if ((va >= MIKASA_PT_SPACE_VA) &&
    ((va - MIKASA_PT_SPACE_VA) < MIKASA_PT_SPACE_VA_SIZE) &&
    mikasa_boot_pt_va_to_pte (va, pte))
    return TRUE;
return mikasa_boot_pt_walk_pte (va, pte);
}

static t_bool mikasa_boot_va_to_pa (t_uint64 va, t_uint64 *pa)
{
if (va < MIKASA_BOOT_PT_RESERVED_END) {
    *pa = va;
    return TRUE;
    }
if ((va >= MIKASA_HWRPB_VA) &&
    (va < (MIKASA_HWRPB_VA + MIKASA_HWRPB_SIZE))) {
    *pa = MIKASA_HWRPB_PA + (va - MIKASA_HWRPB_VA);
    return TRUE;
    }
if ((va >= MIKASA_BOOT_IMAGE_VA) &&
    (va < (MIKASA_BOOT_IMAGE_VA + MIKASA_BOOT_RESERVED_SIZE))) {
    *pa = MIKASA_BOOT_IMAGE_PA + (va - MIKASA_BOOT_IMAGE_VA);
    return TRUE;
    }
if ((va >= MIKASA_PT_SPACE_VA) &&
    ((va - MIKASA_PT_SPACE_VA) < MIKASA_PT_SPACE_VA_SIZE) &&
    mikasa_boot_pt_va_to_pa (va, pa))
    return TRUE;
if (mikasa_boot_pt_walk (va, pa))
    return TRUE;
return FALSE;
}

static t_uint64 mikasa_alloc_l3_pt (void)
{
t_uint64 pa = mikasa_next_l3_pt_pa;
t_uint64 end_pa = MIKASA_L3_BOOTPTS_PA +
    (((t_uint64) MIKASA_L3_BOOTPTS_PAGES) << VA_N_OFF);

if ((pa + MIKASA_PAGE_SIZE) > end_pa)
    return 0;
mikasa_next_l3_pt_pa = pa + MIKASA_PAGE_SIZE;
return pa;
}

static void mikasa_set_l2_entry (uint32 l2_index, t_uint64 l3_pa)
{
t_uint64 pte = mikasa_pte (l3_pa, PTE_KRE | PTE_KWE);

WritePQ (MIKASA_L2_PT_PA + (((t_uint64) l2_index) << 3), pte);
return;
}

static void mikasa_map_range (t_uint64 l3_pa, t_uint64 va_base,
    t_uint64 pa_base, t_uint64 bytes)
{
t_uint64 pages = (bytes + MIKASA_PAGE_SIZE - 1) / MIKASA_PAGE_SIZE;
t_uint64 page;
uint32 l3_index = (uint32) ((va_base >> VA_N_OFF) & VA_M_LVL);

for (page = 0; page < pages; page++)
    WritePQ (l3_pa + ((l3_index + page) << 3),
        mikasa_pte (pa_base + (page << VA_N_OFF), PTE_KRE | PTE_KWE));
return;
}

static void mikasa_build_boot_pt (void)
{
uint32 l2_region0 = (MIKASA_HWRPB_VA >> (VA_N_OFF + VA_N_LVL)) & VA_M_LVL;
uint32 l2_region1 = (MIKASA_BOOT_IMAGE_VA >> (VA_N_OFF + VA_N_LVL)) &
    VA_M_LVL;
uint32 l2_ptspace = (MIKASA_PT_SPACE_VA >> (VA_N_OFF + VA_N_LVL)) & VA_M_LVL;

mikasa_zero_phys (MIKASA_PT_SPACE_PA, MIKASA_PT_SPACE_SIZE);
mikasa_next_l3_pt_pa = MIKASA_L3_BOOTPTS_PA;
WritePQ (MIKASA_L1_PT_PA, mikasa_pte (MIKASA_L2_PT_PA, PTE_KRE | PTE_KWE));
mikasa_set_l2_entry (l2_region0, MIKASA_L3_REGION0_PA);
mikasa_set_l2_entry (l2_region1, MIKASA_L3_REGION1_PA);
mikasa_set_l2_entry (l2_ptspace, MIKASA_L2_PT_PA);
mikasa_map_range (MIKASA_L3_REGION0_PA, MIKASA_HWRPB_VA, MIKASA_HWRPB_PA,
    MIKASA_HWRPB_SIZE);
mikasa_map_range (MIKASA_L3_REGION1_PA, MIKASA_BOOT_IMAGE_VA,
    MIKASA_BOOT_IMAGE_PA, MIKASA_BOOT_RESERVED_SIZE);
return;
}

static void mikasa_load_tlb_range (t_uint64 va, t_uint64 pa, t_uint64 bytes,
    uint32 flags)
{
t_uint64 offset;
t_uint64 span = MIKASA_TLB_SPAN;
t_uint64 pte_flags = flags | (3u << PTE_V_GH);

for (offset = 0; offset < bytes; offset = offset + span) {
    t_uint64 pte = mikasa_pte (pa + offset, (uint32) pte_flags);
    itlb_load (VA_GETVPN (va + offset), pte);
    dtlb_load (VA_GETVPN (va + offset), pte);
    }
return;
}

static void mikasa_load_itlb_page (t_uint64 va, t_uint64 pa, uint32 flags)
{
itlb_load (VA_GETVPN (va), mikasa_pte (pa, flags));
return;
}

static void mikasa_load_dtlb_page (t_uint64 va, t_uint64 pa, uint32 flags)
{
dtlb_load (VA_GETVPN (va), mikasa_pte (pa, flags));
return;
}

static void mikasa_load_boot_tlb (void)
{
tlb_reset (NULL);
tlb_set_cm (MODE_K);
mikasa_load_tlb_range (MIKASA_HWRPB_VA, MIKASA_HWRPB_PA, MIKASA_TLB_SPAN,
    PTE_KRE | PTE_KWE);
mikasa_load_tlb_range (MIKASA_HWRPB_PA, MIKASA_HWRPB_PA, MIKASA_TLB_SPAN,
    PTE_KRE | PTE_KWE);
mikasa_load_tlb_range (MIKASA_BOOT_IMAGE_VA, MIKASA_BOOT_IMAGE_PA,
    MIKASA_BOOT_RESERVED_SIZE, PTE_KRE | PTE_KWE);
return;
}

static void mikasa_write_console_crb (t_uint64 hwrpb_pa)
{
t_uint64 crb_pa = hwrpb_pa + MIKASA_CRB_OFF;
t_uint64 va_pd_pa = hwrpb_pa + MIKASA_CONSOLE_VA_PD_OFF;
t_uint64 pa_pd_pa = hwrpb_pa + MIKASA_CONSOLE_PA_PD_OFF;
t_uint64 code_pa = hwrpb_pa + MIKASA_CONSOLE_CODE_OFF;
t_uint64 hwrpb_pages = (MIKASA_HWRPB_SIZE + MIKASA_PAGE_SIZE - 1) /
    MIKASA_PAGE_SIZE;

WritePQ (crb_pa + 0x000, MIKASA_CONSOLE_VA_PD_VA);      /* VA dispatch PD */
WritePQ (crb_pa + 0x008, pa_pd_pa);                    /* PA dispatch PD */
WritePQ (crb_pa + 0x010, 0);                           /* VA fixup PD */
WritePQ (crb_pa + 0x018, 0);                           /* PA fixup PD */
WritePQ (crb_pa + 0x020, 1);                           /* VA/PA maps */
WritePQ (crb_pa + 0x028, hwrpb_pages);
WritePQ (crb_pa + 0x030, MIKASA_HWRPB_VA);             /* console VA */
WritePQ (crb_pa + 0x038, hwrpb_pa);                    /* console PA */
WritePQ (crb_pa + 0x040, hwrpb_pages);

WritePQ (va_pd_pa + 0x000, PDSC_FLAGS_NULL_NATIVE);    /* null-frame PD */
WritePQ (va_pd_pa + 0x008, MIKASA_CONSOLE_CODE_VA);
WritePQ (va_pd_pa + 0x010, 0);

WritePQ (pa_pd_pa + 0x000, PDSC_FLAGS_NULL_NATIVE);    /* null-frame PD */
WritePQ (pa_pd_pa + 0x008, MIKASA_CONSOLE_CODE_PA);
WritePQ (pa_pd_pa + 0x010, 0);

WritePL (code_pa + 0x000, ALPHA_INSN_CALL_PAL (MPAL_MIKASA_CCALL));
WritePL (code_pa + 0x004, ALPHA_INSN_RET);
return;
}

static void mikasa_set_boot_env (CONST char *bootdev, uint32 osflags)
{
uint32 target = mikasa_boot_target (bootdev);

snprintf (mikasa_booted_dev, sizeof (mikasa_booted_dev),
    "SCSI 0 %u 0 0 %u 0 0", MIKASA_SCSI_SLOT, target);
snprintf (mikasa_booted_osflags, sizeof (mikasa_booted_osflags), "0,%X",
    osflags);
return;
}

static t_uint64 mikasa_callback_status (t_uint64 status, t_uint64 count)
{
return (status << HWRPB_CRB_STS_SHIFT) | (count & 0xFFFFFFFFu);
}

static const char *mikasa_get_env_value (uint32 env)
{
switch (env) {

    case HWRPB_CRB_K_AUTO_ACTION:
        return mikasa_auto_action;

    case HWRPB_CRB_K_BOOT_DEV:
    case HWRPB_CRB_K_BOOTCMD_DEV:
    case HWRPB_CRB_K_BOOTED_DEV:
    case HWRPB_CRB_K_DUMP_DEV:
        return mikasa_booted_dev;

    case HWRPB_CRB_K_BOOT_FILE:
    case HWRPB_CRB_K_BOOTED_FILE:
        return mikasa_boot_file;

    case HWRPB_CRB_K_BOOT_OSFLAGS:
    case HWRPB_CRB_K_BOOTED_OSFLAGS:
        return mikasa_booted_osflags;

    case HWRPB_CRB_K_BOOT_RESET:
        return mikasa_boot_reset;

    case HWRPB_CRB_K_ENABLE_AUDIT:
        return mikasa_enable_audit;

    case HWRPB_CRB_K_CHAR_SET:
    case HWRPB_CRB_K_LANGUAGE:
        return mikasa_language;

    case HWRPB_CRB_K_TTY_DEV:
        return mikasa_tty_dev;

    default:
        return NULL;
        }
}

static void mikasa_copy_env_value (const char *value)
{
t_uint64 i;
t_uint64 len;
t_uint64 max;

if (value == NULL) {
    R[0] = mikasa_callback_status (HWRPB_CRB_STS_UNRECOGNIZED, 0);
    return;
    }

len = (t_uint64) strlen (value);
max = R[19];
if (len > max) {
    for (i = 0; i < max; i++)
        WriteB (R[18] + i, value[i]);
    R[0] = mikasa_callback_status (HWRPB_CRB_STS_TRUNCATED, max);
    return;
    }

for (i = 0; i < len; i++)
    WriteB (R[18] + i, value[i]);
R[0] = mikasa_callback_status (HWRPB_CRB_STS_SUCCESS, len);
return;
}

static uint32 mikasa_boot_target (CONST char *bootdev)
{
uint32 unit;

if ((bootdev == NULL) || (strncmp (bootdev, "DKA", 3) != 0))
    return 0;
unit = (uint32) strtoul (bootdev + 3, NULL, 10);
if (unit >= 100)
    unit = unit / 100;
if (unit >= MIKASA_DKA_UNITS)
    unit = 0;
return unit;
}

static void mikasa_clear_io_channels (void)
{
uint32 i;

for (i = 0; i < MIKASA_DKA_UNITS; i++)
    mikasa_io_channel[i] = 0;
return;
}

static int32 mikasa_parse_io_unit (const char *devstr)
{
const char *p = devstr;
uint32 values[8];
uint32 count = 0;

if (strncmp (devstr, "DKA", 3) == 0)
    return (int32) mikasa_boot_target (devstr);

if (strncmp (devstr, "SCSI", 4) != 0)
    return -1;
p = p + 4;
while ((*p != 0) && (count < 8)) {
    char *endp;
    t_uint64 value;

    while (*p == ' ')
        p++;
    if (*p == 0)
        break;
    value = (t_uint64) strtoull (p, &endp, 10);
    if (endp == p)
        return -1;
    if (value > 0xFFFFFFFFu)
        return -1;
    values[count++] = (uint32) value;
    p = endp;
    }

if ((count >= 5) && (values[4] < MIKASA_DKA_UNITS))
    return (int32) values[4];
return -1;
}

static t_bool mikasa_apb_io_target_pa (t_uint64 addr, t_uint64 *pa)
{
t_uint64 low_addr = addr & M32;

if (mikasa_apb_io_descriptor_pa (addr, pa))
    return TRUE;
if (mikasa_boot_va_to_pa (addr, pa))
    return TRUE;
if ((addr & L_SIGN) && mikasa_boot_va_to_pa (SEXT_L_Q (addr), pa))
    return TRUE;
if ((low_addr >= MIKASA_VMS_SYSVA_BASE) &&
    (low_addr < MIKASA_VMS_SYSVA_LIMIT)) {
    *pa = low_addr & MIKASA_VMS_SYSVA_MASK;
    return ADDR_IS_MEM (*pa);
    }
if (ADDR_IS_MEM (addr)) {
    *pa = addr;
    return TRUE;
    }
if ((addr >= MIKASA_BOOT_IMAGE_VA) &&
    (addr < (MIKASA_BOOT_IMAGE_VA + MIKASA_BOOT_RESERVED_SIZE))) {
    *pa = MIKASA_BOOT_IMAGE_PA + (addr - MIKASA_BOOT_IMAGE_VA);
    return ADDR_IS_MEM (*pa);
    }
return FALSE;
}

static t_bool mikasa_apb_io_descriptor_pa (t_uint64 addr, t_uint64 *pa)
{
t_uint64 desc_pa;
t_uint64 cpu_addr;
t_uint64 cpu_len;
t_uint64 bus_addr;
t_uint64 bus_len;

/* Multi-block APB reads pass the DMA descriptor only on the first block. */
if ((R[17] < MIKASA_BOOT_IMAGE_VA) ||
    (R[17] >= (MIKASA_BOOT_IMAGE_VA + MIKASA_BOOT_RESERVED_SIZE)))
    return mikasa_apb_dma_valid && mikasa_apb_dma_window_pa (addr,
        mikasa_apb_dma_cpu_addr, mikasa_apb_dma_cpu_len,
        mikasa_apb_dma_bus_addr, mikasa_apb_dma_bus_len, pa);
if (!mikasa_boot_va_to_pa (R[17], &desc_pa))
    return FALSE;
if (!ADDR_IS_MEM (desc_pa + 15))
    return FALSE;
cpu_addr = ReadPL (desc_pa);
cpu_len = ReadPL (desc_pa + 4);
bus_addr = ReadPL (desc_pa + 8);
bus_len = ReadPL (desc_pa + 12);
if (mikasa_apb_dma_window_pa (addr, cpu_addr, cpu_len, bus_addr,
    bus_len, pa)) {
    mikasa_apb_dma_valid = TRUE;
    mikasa_apb_dma_cpu_addr = cpu_addr;
    mikasa_apb_dma_cpu_len = cpu_len;
    mikasa_apb_dma_bus_addr = bus_addr;
    mikasa_apb_dma_bus_len = bus_len;
    return TRUE;
    }
return FALSE;
}

static t_bool mikasa_apb_dma_window_pa (t_uint64 addr, t_uint64 cpu_addr,
    t_uint64 cpu_len, t_uint64 bus_addr, t_uint64 bus_len, t_uint64 *pa)
{
t_uint64 offset;

if ((cpu_len == 0) || (bus_len == 0) || ((addr & M32) < bus_addr))
    return FALSE;
offset = (addr & M32) - bus_addr;
if ((offset >= cpu_len) || (offset >= bus_len))
    return FALSE;
return mikasa_boot_va_to_pa ((cpu_addr + offset) & M64, pa);
}

static void mikasa_write_phys_long (t_uint64 pa, t_uint64 dat)
{
dat = dat & M32;
if (pa & 4) M[pa >> 3] = (M[pa >> 3] & M32) | (dat << 32);
else M[pa >> 3] = (M[pa >> 3] & ~((t_uint64) M32)) | dat;
return;
}

static void mikasa_write_phys_byte (t_uint64 pa, uint8 dat)
{
uint32 bo = ((uint32) pa) & 7;

M[pa >> 3] = (M[pa >> 3] & ~(((t_uint64) M8) << (bo << 3))) |
    (((t_uint64) dat) << (bo << 3));
return;
}

static uint8 mikasa_read_phys_byte (t_uint64 pa)
{
uint32 bo = ((uint32) pa) & 7;

return (uint8) ((M[pa >> 3] >> (bo << 3)) & M8);
}

static t_bool mikasa_apb_iobox_read (uint32 unit, t_uint64 lbn,
    t_uint64 addr)
{
uint8 buf[MIKASA_DKA_BLOCK_SIZE];
t_uint64 pa;
uint32 i;
UNIT *uptr;

if ((unit >= MIKASA_DKA_UNITS) || !mikasa_apb_io_target_pa (addr, &pa))
    return FALSE;
if (!ADDR_IS_MEM (pa + MIKASA_DKA_BLOCK_SIZE - 1))
    return FALSE;
mikasa_apb_last_pa = pa;
uptr = &dka_unit[unit];
if ((uptr->flags & UNIT_ATT) == 0)
    return FALSE;
if ((uptr->capac != 0) && (lbn >= uptr->capac))
    return FALSE;
if (sim_fseeko (uptr->fileref,
    ((t_offset) lbn) * MIKASA_DKA_BLOCK_SIZE, SEEK_SET))
    return FALSE;
if (sim_fread (buf, 1, sizeof (buf), uptr->fileref) != sizeof (buf))
    return FALSE;
for (i = 0; i < sizeof (buf); i++)
    mikasa_write_phys_byte (pa + i, buf[i]);
mikasa_apb_ioread_count++;
return TRUE;
}

static t_bool mikasa_apb_iobox_write (uint32 unit, t_uint64 lbn,
    t_uint64 addr)
{
uint8 buf[MIKASA_DKA_BLOCK_SIZE];
t_uint64 pa;
uint32 i;
UNIT *uptr;

if ((unit >= MIKASA_DKA_UNITS) || !mikasa_apb_io_target_pa (addr, &pa))
    return FALSE;
if (!ADDR_IS_MEM (pa + MIKASA_DKA_BLOCK_SIZE - 1))
    return FALSE;
mikasa_apb_last_pa = pa;
uptr = &dka_unit[unit];
if (((uptr->flags & UNIT_ATT) == 0) || (uptr->flags & UNIT_RO))
    return FALSE;
if ((uptr->capac != 0) && (lbn >= uptr->capac))
    return FALSE;
for (i = 0; i < sizeof (buf); i++)
    buf[i] = mikasa_read_phys_byte (pa + i);
if (sim_fseeko (uptr->fileref,
    ((t_offset) lbn) * MIKASA_DKA_BLOCK_SIZE, SEEK_SET))
    return FALSE;
if (sim_fwrite (buf, 1, sizeof (buf), uptr->fileref) != sizeof (buf))
    return FALSE;
mikasa_apb_iowrite_count++;
return TRUE;
}

static void mikasa_apb_iobox_command (t_uint64 pa, t_uint64 cmd)
{
t_uint64 base_pa;
t_uint64 lbn;
t_uint64 addr;
t_bool ok = FALSE;

if (pa < MIKASA_APB_IO_CMD_OFF)
    return;
if ((cmd != MIKASA_APB_IO_CMD_READ) && (cmd != MIKASA_APB_IO_CMD_WRITE))
    return;
base_pa = pa - MIKASA_APB_IO_CMD_OFF;
if (!ADDR_IS_MEM (base_pa + MIKASA_APB_IO_ADDR_OFF + 3))
    return;
lbn = ReadPL (base_pa + MIKASA_APB_IO_LBN_OFF);
addr = ReadPL (base_pa + MIKASA_APB_IO_ADDR_OFF);
mikasa_apb_last_cmd = cmd;
mikasa_apb_last_lbn = lbn;
mikasa_apb_last_addr = addr;
if (cmd == MIKASA_APB_IO_CMD_READ)
    ok = mikasa_apb_iobox_read (0, lbn, addr);
else
    ok = mikasa_apb_iobox_write (0, lbn, addr);
if (!ok)
    mikasa_write_phys_long (pa, cmd | MIKASA_APB_IO_ERR);
return;
}

void mikasa_mem_write (t_uint64 pa, t_uint64 dat, uint32 lnt)
{
if ((pa < MIKASA_BOOT_IMAGE_PA) ||
    (pa >= (MIKASA_BOOT_IMAGE_PA + MIKASA_BOOT_RESERVED_SIZE)))
    return;
if ((lnt == L_LONG) && ((pa & 0x1FF) == MIKASA_APB_IO_CMD_OFF))
    mikasa_apb_iobox_command (pa, dat & M32);
return;
}

static void mikasa_read_callback_string (t_uint64 va, t_uint64 len,
    char *buf, uint32 bufsize)
{
uint32 i;
uint32 max;

if (bufsize == 0)
    return;
max = (len < (bufsize - 1)) ? (uint32) len : (bufsize - 1);
for (i = 0; i < max; i++)
    buf[i] = (char) ReadB (va + i);
buf[max] = 0;
return;
}

static void mikasa_open_io (void)
{
char devstr[128];
int32 unit;

mikasa_read_callback_string (R[17], R[18], devstr, sizeof (devstr));
unit = mikasa_parse_io_unit (devstr);
if ((unit < 0) || (unit >= MIKASA_DKA_UNITS) ||
    ((dka_unit[unit].flags & UNIT_ATT) == 0)) {
    R[0] = HWRPB_CRB_OPEN_STS_NXDEV;
    return;
    }

if (mikasa_io_channel[unit] != 0) {
    R[0] = HWRPB_CRB_OPEN_STS_ERROR;
    return;
    }

mikasa_io_channel[unit] = unit + 1;
R[0] = mikasa_io_channel[unit];
return;
}

static void mikasa_close_io (void)
{
uint32 channel = (uint32) R[17];
uint32 unit;

if ((channel == 0) || (channel > MIKASA_DKA_UNITS)) {
    R[0] = HWRPB_CRB_IO_STS_FAIL;
    return;
    }
unit = channel - 1;
if (mikasa_io_channel[unit] != channel) {
    R[0] = HWRPB_CRB_IO_STS_FAIL;
    return;
    }
mikasa_io_channel[unit] = 0;
R[0] = 0;
return;
}

static void mikasa_read_io (void)
{
uint8 buf[MIKASA_IO_BUFSIZE];
UNIT *uptr;
t_uint64 count = R[18];
t_uint64 addr = R[19];
t_uint64 lbn = R[20];
t_uint64 bytes_left;
t_uint64 done = 0;
t_uint64 status = 0;
uint32 channel = (uint32) R[17];
uint32 unit;

mikasa_ioread_count++;
if ((channel == 0) || (channel > MIKASA_DKA_UNITS)) {
    R[0] = HWRPB_CRB_IO_STS_FAIL;
    return;
    }
unit = channel - 1;
if (mikasa_io_channel[unit] != channel) {
    R[0] = HWRPB_CRB_IO_STS_FAIL;
    return;
    }
uptr = &dka_unit[unit];
if ((uptr->flags & UNIT_ATT) == 0) {
    R[0] = HWRPB_CRB_IO_STS_FAIL;
    return;
    }

if (count & (MIKASA_DKA_BLOCK_SIZE - 1)) {
    status |= HWRPB_CRB_IO_STS_ILL;
    count &= ~((t_uint64) MIKASA_DKA_BLOCK_SIZE - 1);
    }
if (count == 0) {
    R[0] = HWRPB_CRB_IO_STS_FAIL | status;
    return;
    }
if ((uptr->capac != 0) && (lbn >= uptr->capac)) {
    R[0] = HWRPB_CRB_IO_STS_EOD;
    return;
    }
if ((uptr->capac != 0) &&
    ((lbn + (count / MIKASA_DKA_BLOCK_SIZE)) > uptr->capac)) {
    count = (uptr->capac - lbn) * MIKASA_DKA_BLOCK_SIZE;
    status |= HWRPB_CRB_IO_STS_EOD;
    }

if (sim_fseeko (uptr->fileref,
    ((t_offset) lbn) * MIKASA_DKA_BLOCK_SIZE, SEEK_SET)) {
    R[0] = HWRPB_CRB_IO_STS_FAIL | status | done;
    return;
    }

bytes_left = count;
while (bytes_left != 0) {
    uint32 chunk = (bytes_left > sizeof (buf)) ? sizeof (buf) :
        (uint32) bytes_left;
    uint32 i;

    if (sim_fread (buf, 1, chunk, uptr->fileref) != chunk) {
        R[0] = HWRPB_CRB_IO_STS_FAIL | status | done;
        return;
        }
    for (i = 0; i < chunk; i++)
        WriteB (addr + done + i, buf[i]);
    done = done + chunk;
    bytes_left = bytes_left - chunk;
    }

R[0] = status | (done & 0xFFFFFFFFu);
return;
}

static void mikasa_write_io (void)
{
uint8 buf[MIKASA_IO_BUFSIZE];
UNIT *uptr;
t_uint64 count = R[18];
t_uint64 addr = R[19];
t_uint64 lbn = R[20];
t_uint64 bytes_left;
t_uint64 done = 0;
t_uint64 status = 0;
uint32 channel = (uint32) R[17];
uint32 unit;

mikasa_iowrite_count++;
if ((channel == 0) || (channel > MIKASA_DKA_UNITS)) {
    R[0] = HWRPB_CRB_IO_STS_FAIL;
    return;
    }
unit = channel - 1;
if ((mikasa_io_channel[unit] != channel) ||
    ((dka_unit[unit].flags & UNIT_ATT) == 0) ||
    (dka_unit[unit].flags & UNIT_RO)) {
    R[0] = HWRPB_CRB_IO_STS_FAIL;
    return;
    }
uptr = &dka_unit[unit];

if (count & (MIKASA_DKA_BLOCK_SIZE - 1)) {
    status |= HWRPB_CRB_IO_STS_ILL;
    count &= ~((t_uint64) MIKASA_DKA_BLOCK_SIZE - 1);
    }
if (count == 0) {
    R[0] = HWRPB_CRB_IO_STS_FAIL | status;
    return;
    }
if ((uptr->capac != 0) && (lbn >= uptr->capac)) {
    R[0] = HWRPB_CRB_IO_STS_EOD;
    return;
    }
if ((uptr->capac != 0) &&
    ((lbn + (count / MIKASA_DKA_BLOCK_SIZE)) > uptr->capac)) {
    count = (uptr->capac - lbn) * MIKASA_DKA_BLOCK_SIZE;
    status |= HWRPB_CRB_IO_STS_EOD;
    }

if (sim_fseeko (uptr->fileref,
    ((t_offset) lbn) * MIKASA_DKA_BLOCK_SIZE, SEEK_SET)) {
    R[0] = HWRPB_CRB_IO_STS_FAIL | status | done;
    return;
    }

bytes_left = count;
while (bytes_left != 0) {
    uint32 chunk = (bytes_left > sizeof (buf)) ? sizeof (buf) :
        (uint32) bytes_left;
    uint32 i;

    for (i = 0; i < chunk; i++)
        buf[i] = (uint8) ReadB (addr + done + i);
    if (sim_fwrite (buf, 1, chunk, uptr->fileref) != chunk) {
        R[0] = HWRPB_CRB_IO_STS_FAIL | status | done;
        return;
        }
    done = done + chunk;
    bytes_left = bytes_left - chunk;
    }

R[0] = status | (done & 0xFFFFFFFFu);
return;
}

static t_stat mikasa_console_callback (void)
{
uint32 fnc = (uint32) R[16];
t_stat c;
t_uint64 i, len;

mikasa_callback_count++;
switch (fnc) {

    case HWRPB_CRB_K_GETC:
        c = sim_poll_kbd ();
        if ((c & SCPE_KFLAG) || ((c > 0) && (c < 256)))
            R[0] = (((t_uint64) c & 0xFF) << 8) | 1;
        else
            R[0] = 0;
        break;

    case HWRPB_CRB_K_PUTS:
        len = R[19];
        if (len > 4096)
            len = 4096;
        for (i = 0; i < len; i++) {
            t_stat r = sim_putchar (ReadB (R[18] + i) & 0xFF);
            if (r != SCPE_OK)
                return r;
            }
        R[0] = 1;
        break;

    case HWRPB_CRB_K_RESET_TERM:
    case HWRPB_CRB_K_SET_TERM_INTR:
    case HWRPB_CRB_K_SET_TERM_CTL:
    case HWRPB_CRB_K_PROCESS_KEYCODE:
    case HWRPB_CRB_K_PSWITCH:
        R[0] = 1;
        break;

    case HWRPB_CRB_K_GET_ENV:
        mikasa_getenv_count++;
        mikasa_copy_env_value (mikasa_get_env_value ((uint32) R[17]));
        break;

    case HWRPB_CRB_K_SET_ENV:
    case HWRPB_CRB_K_RESET_ENV:
        if (mikasa_get_env_value ((uint32) R[17]) == NULL)
            R[0] = mikasa_callback_status (HWRPB_CRB_STS_UNRECOGNIZED, 0);
        else
            R[0] = mikasa_callback_status (HWRPB_CRB_STS_READONLY,
                sizeof (mikasa_booted_dev));
        break;

    case HWRPB_CRB_K_SAVE_ENV:
        R[0] = mikasa_callback_status (HWRPB_CRB_STS_SUCCESS, 0);
        break;

    case HWRPB_CRB_K_OPEN:
        mikasa_open_io ();
        break;

    case HWRPB_CRB_K_CLOSE:
        mikasa_close_io ();
        break;

    case HWRPB_CRB_K_IOCTRL:
        R[0] = 0;
        break;

    case HWRPB_CRB_K_READ:
        mikasa_read_io ();
        break;

    case HWRPB_CRB_K_WRITE:
        mikasa_write_io ();
        break;

    default:
        R[0] = 0;
        break;
        }

return SCPE_OK;
}

t_stat mikasa_pal_proc_excp (uint32 abval)
{
t_uint64 va = p1;
t_uint64 pa;

switch (abval) {

    case EXC_ALIGN:
        if (mikasa_pal_proc_align ())
            return SCPE_OK;
        break;

    case EXC_TBM + EXC_E:
        if (!mikasa_boot_va_to_pa (va, &pa))
            return SCPE_NOFNC;
        mikasa_load_itlb_page (va & ~((t_uint64) VA_M_OFF),
            pa & ~((t_uint64) VA_M_OFF), PTE_KRE | PTE_KWE);
        return SCPE_OK;

    case EXC_TBM + EXC_R:
    case EXC_TBM + EXC_W:
        if (!mikasa_boot_va_to_pa (va, &pa))
            return SCPE_NOFNC;
        mikasa_load_dtlb_page (va & ~((t_uint64) VA_M_OFF),
            pa & ~((t_uint64) VA_M_OFF), PTE_KRE | PTE_KWE);
        PC = (PC - 4) & M64;
        return SCPE_OK;
        }

return SCPE_NOFNC;
}

static t_bool mikasa_pal_proc_align (void)
{
uint32 op = I_GETOP (ir);
uint32 ra = I_GETRA (ir);
t_uint64 val;

mikasa_align_count++;
mikasa_align_last_va = p1;
mikasa_align_last_pc = (PC - 4) & M64;
switch (op) {

    case OP_LDWU:
        if (ra != 31)
            R[ra] = mikasa_pal_read_una (p1, 2);
        return TRUE;

    case OP_LDL:
        if (ra != 31) {
            val = mikasa_pal_read_una (p1, 4);
            R[ra] = SEXT_L_Q (val);
            }
        return TRUE;

    case OP_LDQ:
        if (ra != 31)
            R[ra] = mikasa_pal_read_una (p1, 8);
        return TRUE;

    case OP_LDL_L:
        if (ra != 31) {
            val = mikasa_pal_read_una (p1, 4);
            R[ra] = SEXT_L_Q (val);
            lock_flag = 1;
            }
        return TRUE;

    case OP_LDQ_L:
        if (ra != 31) {
            R[ra] = mikasa_pal_read_una (p1, 8);
            lock_flag = 1;
            }
        return TRUE;

    case OP_STW:
        mikasa_pal_write_una (p1, R[ra], 2);
        return TRUE;

    case OP_STL:
        mikasa_pal_write_una (p1, R[ra], 4);
        return TRUE;

    case OP_STQ:
        mikasa_pal_write_una (p1, R[ra], 8);
        return TRUE;

    case OP_STL_C:
        if (lock_flag) {
            mikasa_pal_write_una (p1, R[ra], 4);
            if (ra != 31)
                R[ra] = 1;
            }
        else R[ra] = 0;
        lock_flag = 0;
        return TRUE;

    case OP_STQ_C:
        if (lock_flag) {
            mikasa_pal_write_una (p1, R[ra], 8);
            if (ra != 31)
                R[ra] = 1;
            }
        else R[ra] = 0;
        lock_flag = 0;
        return TRUE;
        }

return FALSE;
}

static uint32 mikasa_pal_probe (uint32 acc)
{
t_uint64 pte;
uint32 pm = ((uint32) R[18]) & 3;
uint32 req;
t_uint64 end_va = (R[16] + R[17]) & M64;

if (pm <= MODE_K)
    pm = MODE_K;
req = (acc << pm) | PTE_V;
if (!mikasa_boot_va_to_pte (R[16], &pte))
    return 0;
if (req & ~((uint32) pte))
    return 0;
if (!mikasa_boot_va_to_pte (end_va, &pte))
    return 0;
if (req & ~((uint32) pte))
    return 0;
return 1;
}

static t_int64 mikasa_pal_insqhil (void)
{
t_uint64 h = R[16];
t_uint64 d = R[17];
t_uint64 ar, a;

if ((h == d) || ((h | d) & 07) ||
    ((SEXT_L_Q (h) & M64) != h) ||
    ((SEXT_L_Q (d) & M64) != d))
    ABORT (EXC_RSVO);
ReadAccQ (d, cm_wacc);
ar = ReadQ (h);
if (ar & 06)
    ABORT (EXC_RSVO);
if (ar & 01)
    return -1;
WriteQ (h, ar | 1);
a = (SEXT_L_Q (ar + h)) & M64;
ReadAccQ (a, cm_wacc);
WriteL (a + 4, (uint32) (d - a));
WriteL (d, (uint32) (a - d));
WriteL (d + 4, (uint32) (h - d));
WriteL (h, (uint32) (d - h));
return ((ar & M32) == 0) ? 0 : +1;
}

static t_int64 mikasa_pal_insqtil (void)
{
t_uint64 h = R[16];
t_uint64 d = R[17];
t_uint64 ar, c;

if ((h == d) || ((h | d) & 07) ||
    ((SEXT_L_Q (h) & M64) != h) ||
    ((SEXT_L_Q (d) & M64) != d))
    ABORT (EXC_RSVO);
ReadAccQ (d, cm_wacc);
ar = ReadQ (h);
if ((ar & M32) == 0)
    return mikasa_pal_insqhil ();
if (ar & 06)
    ABORT (EXC_RSVO);
if (ar & 01)
    return -1;
WriteQ (h, ar | 1);
c = ar >> 32;
c = (SEXT_L_Q (c + h)) & M64;
if (c & 07) {
    WriteQ (h, ar);
    ABORT (EXC_RSVO);
    }
ReadAccQ (c, cm_wacc);
WriteL (c, (uint32) (d - c));
WriteL (d, (uint32) (h - d));
WriteL (d + 4, (uint32) (c - d));
WriteL (h + 4, (uint32) (d - h));
WriteL (h, (uint32) ar);
return 0;
}

static t_int64 mikasa_pal_insqhiq (void)
{
t_uint64 h = R[16];
t_uint64 d = R[17];
t_uint64 ar, a;

if ((h == d) || ((h | d) & 0xF))
    ABORT (EXC_RSVO);
ReadAccQ (d, cm_wacc);
ar = ReadQ (h);
if (ar & 0xE)
    ABORT (EXC_RSVO);
if (ar & 01)
    return -1;
WriteQ (h, ar | 1);
a = (ar + h) & M64;
ReadAccQ (a, cm_wacc);
WriteQ (a + 8, (d - a) & M64);
WriteQ (d, (a - d) & M64);
WriteQ (d + 8, (h - d) & M64);
WriteQ (h, (d - h) & M64);
return (ar == 0) ? 0 : +1;
}

static t_int64 mikasa_pal_insqtiq (void)
{
t_uint64 h = R[16];
t_uint64 d = R[17];
t_uint64 ar, c;

if ((h == d) || ((h | d) & 0xF))
    ABORT (EXC_RSVO);
ReadAccQ (d, cm_wacc);
ar = ReadQ (h);
if (ar == 0)
    return mikasa_pal_insqhiq ();
if (ar & 0xE)
    ABORT (EXC_RSVO);
if (ar & 01)
    return -1;
WriteQ (h, ar | 1);
c = ReadQ (h + 8);
c = (c + h) & M64;
if (c & 0xF) {
    WriteQ (h, ar);
    ABORT (EXC_RSVO);
    }
ReadAccQ (c, cm_wacc);
WriteQ (c, (d - c) & M64);
WriteQ (d, (h - d) & M64);
WriteQ (d + 8, (c - d) & M64);
WriteQ (h + 8, (d - h) & M64);
WriteQ (h, ar);
return 0;
}

static t_int64 mikasa_pal_remqhil (void)
{
t_uint64 h = R[16];
t_uint64 ar, a, b;

if ((h & 07) || ((SEXT_L_Q (h) & M64) != h))
    ABORT (EXC_RSVO);
ar = ReadQ (h);
if (ar & 06)
    ABORT (EXC_RSVO);
if (ar & 01)
    return -1;
if ((ar & M32) == 0)
    return 0;
WriteQ (h, ar | 1);
a = (SEXT_L_Q (ar + h)) & M64;
ReadAccQ (a, cm_wacc);
b = ReadL (a);
b = (SEXT_L_Q (b + a)) & M64;
if (b & 07) {
    WriteQ (h, ar);
    ABORT (EXC_RSVO);
    }
ReadAccQ (b, cm_wacc);
WriteL (b + 4, (uint32) (h - b));
WriteL (h, (uint32) (b - h));
R[1] = a;
return ((b & M32) == (h & M32)) ? +2 : +1;
}

static t_int64 mikasa_pal_remqtil (void)
{
t_uint64 h = R[16];
t_uint64 ar, b, c;

if ((h & 07) || ((SEXT_L_Q (h) & M64) != h))
    ABORT (EXC_RSVO);
ar = ReadQ (h);
if (ar & 06)
    ABORT (EXC_RSVO);
if (ar & 01)
    return -1;
if ((ar & M32) == 0)
    return 0;
WriteQ (h, ar | 1);
c = ar >> 32;
if (c & 07) {
    WriteQ (h, ar);
    ABORT (EXC_RSVO);
    }
if ((ar & M32) == (c & M32)) {
    WriteQ (h, ar);
    return mikasa_pal_remqhil ();
    }
c = (SEXT_L_Q (c + h)) & M64;
ReadL (c + 4);
b = ReadL (c + 4);
b = (SEXT_L_Q (b + c)) & M64;
if (b & 07) {
    WriteQ (h, ar);
    ABORT (EXC_RSVO);
    }
ReadAccQ (b, cm_wacc);
WriteL (b, (uint32) (h - b));
WriteL (h + 4, (uint32) (b - h));
WriteL (h, (uint32) ar);
R[1] = c;
return +1;
}

static t_int64 mikasa_pal_remqhiq (void)
{
t_uint64 h = R[16];
t_uint64 ar, a, b;

if (h & 0xF)
    ABORT (EXC_RSVO);
ar = ReadQ (h);
if (ar & 0xE)
    ABORT (EXC_RSVO);
if (ar & 01)
    return -1;
if (ar == 0)
    return 0;
WriteQ (h, ar | 1);
a = (ar + h) & M64;
ReadAccQ (a, cm_wacc);
b = ReadQ (a);
b = (b + a) & M64;
if (b & 0xF) {
    WriteQ (h, ar);
    ABORT (EXC_RSVO);
    }
ReadAccQ (b, cm_wacc);
WriteQ (b + 8, (h - b) & M64);
WriteQ (h, (b - h) & M64);
R[1] = a;
return (b == h) ? +2 : +1;
}

static t_int64 mikasa_pal_remqtiq (void)
{
t_uint64 h = R[16];
t_uint64 ar, b, c;

if (h & 0xF)
    ABORT (EXC_RSVO);
ar = ReadQ (h);
if (ar & 0xE)
    ABORT (EXC_RSVO);
if (ar & 01)
    return -1;
if (ar == 0)
    return 0;
WriteQ (h, ar | 1);
c = ReadQ (h + 8);
if (c & 0xF) {
    WriteQ (h, ar);
    ABORT (EXC_RSVO);
    }
if (ar == c) {
    WriteQ (h, ar);
    return mikasa_pal_remqhiq ();
    }
c = (c + h) & M64;
ReadAccQ (c + 8, cm_wacc);
b = ReadQ (c + 8);
b = (b + c) & M64;
if (b & 0xF) {
    WriteQ (h, ar);
    ABORT (EXC_RSVO);
    }
ReadAccQ (b, cm_wacc);
WriteQ (b, (h - b) & M64);
WriteQ (h + 8, (b - h) & M64);
WriteQ (h, ar);
R[1] = c;
return +1;
}

static t_int64 mikasa_pal_insquel (uint32 defer)
{
t_uint64 p = SEXT_L_Q (R[16]) & M64;
t_uint64 e = SEXT_L_Q (R[17]) & M64;
t_uint64 s;

if (defer) {
    p = mikasa_pal_read_una_l (p);
    p = SEXT_L_Q (p) & M64;
    }
s = mikasa_pal_read_una_l (p);
s = SEXT_L_Q (s) & M64;
(void) mikasa_pal_read_una_l ((s + 4) & M64);
(void) mikasa_pal_read_una_l ((e + 4) & M64);
mikasa_pal_write_una_l (e, s);
mikasa_pal_write_una_l ((e + 4) & M64, p);
mikasa_pal_write_una_l ((s + 4) & M64, e);
mikasa_pal_write_una_l (p, e);
return (((s & M32) == (p & M32)) ? +1 : 0);
}

static t_int64 mikasa_pal_insqueq (uint32 defer)
{
t_uint64 p = R[16];
t_uint64 e = R[17];
t_uint64 s;

if (defer) {
    if (p & 07)
        ABORT (EXC_RSVO);
    p = ReadQ (p);
    }
if ((e | p) & 0xF)
    ABORT (EXC_RSVO);
s = ReadAccQ (p, cm_macc);
if (s & 0xF)
    ABORT (EXC_RSVO);
ReadAccQ (s + 8, cm_wacc);
ReadAccQ (e + 8, cm_wacc);
WriteQ (e, s);
WriteQ (e + 8, p);
WriteQ (s + 8, e);
WriteQ (p, e);
return ((s == p) ? +1 : 0);
}

static t_int64 mikasa_pal_remquel (uint32 defer)
{
t_uint64 e = SEXT_L_Q (R[16]) & M64;
t_uint64 s, p;

if (defer) {
    e = mikasa_pal_read_una_l (e);
    e = SEXT_L_Q (e) & M64;
    }
s = mikasa_pal_read_una_l (e);
p = mikasa_pal_read_una_l ((e + 4) & M64);
s = SEXT_L_Q (s) & M64;
p = SEXT_L_Q (p) & M64;
if (e == p)
    return -1;
(void) mikasa_pal_read_una_l ((s + 4) & M64);
mikasa_pal_write_una_l (p, s);
mikasa_pal_write_una_l ((s + 4) & M64, p);
return ((s == p) ? 0 : +1);
}

static t_int64 mikasa_pal_remqueq (uint32 defer)
{
t_uint64 e = R[16];
t_uint64 s, p;

if (defer) {
    if (e & 07)
        ABORT (EXC_RSVO);
    e = ReadQ (e);
    }
if (e & 0xF)
    ABORT (EXC_RSVO);
s = ReadQ (e);
p = ReadQ (e + 8);
if ((s | p) & 0xF)
    ABORT (EXC_RSVO);
if (e == p)
    return -1;
ReadAccQ (s + 8, cm_wacc);
WriteQ (p, s);
WriteQ (s + 8, p);
return ((s == p) ? 0 : +1);
}

static t_uint64 mikasa_pal_read_una (t_uint64 va, uint32 size)
{
t_uint64 val = 0;
uint32 i;

for (i = 0; i < size; i++)
    val = val | ((t_uint64) ReadB ((va + i) & M64) << (i << 3));
return val;
}

static void mikasa_pal_write_una (t_uint64 va, t_uint64 val, uint32 size)
{
uint32 i;

for (i = 0; i < size; i++)
    WriteB ((va + i) & M64, val >> (i << 3));
return;
}

static t_uint64 mikasa_pal_read_una_l (t_uint64 va)
{
t_uint64 val;

val = ReadB (va);
val |= ReadB ((va + 1) & M64) << 8;
val |= ReadB ((va + 2) & M64) << 16;
val |= ReadB ((va + 3) & M64) << 24;
return val & M32;
}

static void mikasa_pal_write_una_l (t_uint64 va, t_uint64 val)
{
WriteB (va, val);
WriteB ((va + 1) & M64, val >> 8);
WriteB ((va + 2) & M64, val >> 16);
WriteB ((va + 3) & M64, val >> 24);
return;
}

/* Minimal native VMS PAL dispatcher for bootstrap bring-up. */

#define MPAL_HALT       0x00
#define MPAL_DRAINA     0x01
#define MPAL_CFLUSH     0x02
#define MPAL_LDQP       0x03
#define MPAL_STQP       0x04
#define MPAL_SWPCTX     0x05
#define MPAL_MF_ASN     0x06
#define MPAL_MT_ASTEN   0x07
#define MPAL_MT_ASTSR   0x08
#define MPAL_CSERVE     0x09
#define MPAL_SWPPAL     0x0A
#define MPAL_MF_FEN     0x0B
#define MPAL_MT_FEN     0x0C
#define MPAL_MT_IPIR    0x0D
#define MPAL_MF_IPL     0x0E
#define MPAL_MT_IPL     0x0F
#define MPAL_MF_MCES    0x10
#define MPAL_MT_MCES    0x11
#define MPAL_MF_PCBB    0x12
#define MPAL_MF_PRBR    0x13
#define MPAL_MT_PRBR    0x14
#define MPAL_MF_PTBR    0x15
#define MPAL_MF_SCBB    0x16
#define MPAL_MT_SCBB    0x17
#define MPAL_MT_SIRR    0x18
#define MPAL_MF_SISR    0x19
#define MPAL_MF_TBCHK   0x1A
#define MPAL_MT_TBIA    0x1B
#define MPAL_MT_TBIAP   0x1C
#define MPAL_MT_TBIS    0x1D
#define MPAL_MF_ESP     0x1E
#define MPAL_MT_ESP     0x1F
#define MPAL_MF_SSP     0x20
#define MPAL_MT_SSP     0x21
#define MPAL_MF_USP     0x22
#define MPAL_MT_USP     0x23
#define MPAL_MT_TBISD   0x24
#define MPAL_MT_TBISI   0x25
#define MPAL_MF_ASTEN   0x26
#define MPAL_MF_ASTSR   0x27
#define MPAL_MF_VTBR    0x29
#define MPAL_MT_VTBR    0x2A
#define MPAL_MT_PERFMON 0x2B
#define MPAL_MT_DATFX   0x2E
#define MPAL_MF_VIRBND  0x30
#define MPAL_MT_VIRBND  0x31
#define MPAL_MF_SYSPTBR 0x32
#define MPAL_MT_SYSPTBR 0x33
#define MPAL_WTINT      0x3E
#define MPAL_MF_WHAMI   0x3F
#define MPAL_BPT        0x80
#define MPAL_BUGCHK     0x81
#define MPAL_IMB        0x86
#define MPAL_INSQHIL    0x87
#define MPAL_INSQTIL    0x88
#define MPAL_INSQHIQ    0x89
#define MPAL_INSQTIQ    0x8A
#define MPAL_INSQUEL    0x8B
#define MPAL_INSQUEQ    0x8C
#define MPAL_INSQUELD   0x8D
#define MPAL_INSQUEQD   0x8E
#define MPAL_PROBER     0x8F
#define MPAL_PROBEW     0x90
#define MPAL_RD_PS      0x91
#define MPAL_REI        0x92
#define MPAL_REMQHIL    0x93
#define MPAL_REMQTIL    0x94
#define MPAL_REMQHIQ    0x95
#define MPAL_REMQTIQ    0x96
#define MPAL_REMQUEL    0x97
#define MPAL_REMQUEQ    0x98
#define MPAL_REMQUELD   0x99
#define MPAL_REMQUEQD   0x9A
#define MPAL_SWASTEN    0x9B
#define MPAL_WR_PS_SW   0x9C
#define MPAL_RSCC       0x9D
#define MPAL_RD_UNQ     0x9E
#define MPAL_WR_UNQ     0x9F
#define MPAL_AMOVRR     0xA0
#define MPAL_AMOVRM     0xA1
#define MPAL_INSQHILR   0xA2
#define MPAL_INSQTILR   0xA3
#define MPAL_INSQHIQR   0xA4
#define MPAL_INSQTIQR   0xA5
#define MPAL_REMQHILR   0xA6
#define MPAL_REMQTILR   0xA7
#define MPAL_REMQHIQR   0xA8
#define MPAL_REMQTIQR   0xA9
#define MPAL_GENTRAP    0xAA
#define MPAL_CLRFEN     0xAE

#define MPAL_IPL_MASK   0x1F
#define MPAL_AST_MASK   0x0F
#define MPAL_SISR_MASK  0xFFFE
#define MPAL_PS_SW_MASK 0x03

t_stat mikasa_pal_proc_inst (uint32 fnc)
{
uint32 arg32 = (uint32) R[16];

switch (fnc) {

    case MPAL_HALT:
        return STOP_HALT;

    case MPAL_DRAINA:
    case MPAL_CFLUSH:
    case MPAL_MT_IPIR:
    case MPAL_MT_TBIA:
    case MPAL_MT_TBIAP:
    case MPAL_MT_TBIS:
    case MPAL_MT_TBISD:
    case MPAL_MT_TBISI:
    case MPAL_MT_PERFMON:
    case MPAL_IMB:
        break;

    case MPAL_LDQP:
        R[0] = ReadPQ (R[16]);
        break;

    case MPAL_STQP:
        WritePQ (R[16], R[17]);
        break;

    case MPAL_SWPCTX:
        mikasa_pal_pcbb = R[16];
        break;

    case MPAL_MF_ASN:
        R[0] = 0;
        break;

    case MPAL_MT_ASTEN:
        R[0] = mikasa_pal_asten & MPAL_AST_MASK;
        mikasa_pal_asten = ((mikasa_pal_asten & arg32) | (arg32 >> 4)) &
            MPAL_AST_MASK;
        break;

    case MPAL_MT_ASTSR:
        R[0] = mikasa_pal_astsr & MPAL_AST_MASK;
        mikasa_pal_astsr = ((mikasa_pal_astsr & arg32) | (arg32 >> 4)) &
            MPAL_AST_MASK;
        break;

    case MPAL_CSERVE:
        R[0] = 0;
        break;

    case MPAL_SWPPAL:
        pal_type = PAL_VMS;
        R[0] = 0;
        break;

    case MPAL_MF_FEN:
        R[0] = fpen & 1;
        break;

    case MPAL_MT_FEN:
        fpen = arg32 & 1;
        break;

    case MPAL_MF_IPL:
        R[0] = mikasa_pal_ipl & MPAL_IPL_MASK;
        break;

    case MPAL_MT_IPL:
        R[0] = mikasa_pal_ipl & MPAL_IPL_MASK;
        mikasa_pal_ipl = arg32 & MPAL_IPL_MASK;
        break;

    case MPAL_MF_MCES:
        R[0] = mikasa_pal_mces;
        break;

    case MPAL_MT_MCES:
        mikasa_pal_mces = arg32;
        break;

    case MPAL_MF_PCBB:
        R[0] = mikasa_pal_pcbb;
        break;

    case MPAL_MF_PRBR:
        R[0] = mikasa_pal_prbr;
        break;

    case MPAL_MT_PRBR:
        mikasa_pal_prbr = R[16];
        break;

    case MPAL_MF_PTBR:
        R[0] = mikasa_pal_ptbr >> 13;
        break;

    case MPAL_MF_SCBB:
        R[0] = mikasa_pal_scbb;
        break;

    case MPAL_MT_SCBB:
        mikasa_pal_scbb = R[16];
        break;

    case MPAL_MT_SIRR:
        mikasa_pal_sisr = (mikasa_pal_sisr | (1u << (arg32 & 0xF))) &
            MPAL_SISR_MASK;
        break;

    case MPAL_MF_SISR:
        R[0] = mikasa_pal_sisr & MPAL_SISR_MASK;
        break;

    case MPAL_MF_TBCHK:
        R[0] = Q_SIGN;
        break;

    case MPAL_MF_ESP:
    case MPAL_MF_SSP:
    case MPAL_MF_USP:
        R[0] = 0;
        break;

    case MPAL_MT_ESP:
    case MPAL_MT_SSP:
    case MPAL_MT_USP:
        break;

    case MPAL_MF_ASTEN:
        R[0] = mikasa_pal_asten & MPAL_AST_MASK;
        break;

    case MPAL_MF_ASTSR:
        R[0] = mikasa_pal_astsr & MPAL_AST_MASK;
        break;

    case MPAL_MF_VTBR:
        R[0] = mikasa_pal_vtbr;
        break;

    case MPAL_MT_VTBR:
        mikasa_pal_vtbr = R[16];
        break;

    case MPAL_MT_DATFX:
        break;

    case MPAL_MF_VIRBND:
        R[0] = mikasa_pal_virbnd;
        break;

    case MPAL_MT_VIRBND:
        mikasa_pal_virbnd = R[16];
        break;

    case MPAL_MF_SYSPTBR:
        R[0] = mikasa_pal_sysptbr;
        break;

    case MPAL_MT_SYSPTBR:
        mikasa_pal_sysptbr = R[16];
        break;

    case MPAL_WTINT:
    case MPAL_MF_WHAMI:
        R[0] = 0;
        break;

    case MPAL_BPT:
        return STOP_IBKPT;

    case MPAL_BUGCHK:
        return STOP_BUGCHK;

    case MPAL_GENTRAP:
    case MPAL_REI:
        return STOP_NSPAL;

    case MPAL_INSQHIL:
    case MPAL_INSQHILR:
        R[0] = mikasa_pal_insqhil ();
        break;

    case MPAL_INSQTIL:
    case MPAL_INSQTILR:
        R[0] = mikasa_pal_insqtil ();
        break;

    case MPAL_INSQHIQ:
    case MPAL_INSQHIQR:
        R[0] = mikasa_pal_insqhiq ();
        break;

    case MPAL_INSQTIQ:
    case MPAL_INSQTIQR:
        R[0] = mikasa_pal_insqtiq ();
        break;

    case MPAL_INSQUEL:
        R[0] = mikasa_pal_insquel (0);
        break;

    case MPAL_INSQUEQ:
        R[0] = mikasa_pal_insqueq (0);
        break;

    case MPAL_INSQUELD:
        R[0] = mikasa_pal_insquel (1);
        break;

    case MPAL_INSQUEQD:
        R[0] = mikasa_pal_insqueq (1);
        break;

    case MPAL_PROBER:
        R[0] = mikasa_pal_probe (PTE_KRE);
        break;

    case MPAL_PROBEW:
        R[0] = mikasa_pal_probe (PTE_KRE | PTE_KWE);
        break;

    case MPAL_REMQHIL:
    case MPAL_REMQHILR:
        R[0] = mikasa_pal_remqhil ();
        break;

    case MPAL_REMQTIL:
    case MPAL_REMQTILR:
        R[0] = mikasa_pal_remqtil ();
        break;

    case MPAL_REMQHIQ:
    case MPAL_REMQHIQR:
        R[0] = mikasa_pal_remqhiq ();
        break;

    case MPAL_REMQTIQ:
    case MPAL_REMQTIQR:
        R[0] = mikasa_pal_remqtiq ();
        break;

    case MPAL_REMQUEL:
        R[0] = mikasa_pal_remquel (0);
        break;

    case MPAL_REMQUEQ:
        R[0] = mikasa_pal_remqueq (0);
        break;

    case MPAL_REMQUELD:
        R[0] = mikasa_pal_remquel (1);
        break;

    case MPAL_REMQUEQD:
        R[0] = mikasa_pal_remqueq (1);
        break;

    case MPAL_RD_PS:
        R[0] = ((t_uint64) mikasa_pal_ipl << 8) |
            (mikasa_pal_ps & MPAL_PS_SW_MASK);
        break;

    case MPAL_AMOVRR:
    case MPAL_AMOVRM:
        R[0] = 0;
        break;

    case MPAL_SWASTEN:
        R[0] = mikasa_pal_asten & 1;
        mikasa_pal_asten = (mikasa_pal_asten & ~1u) | (arg32 & 1);
        break;

    case MPAL_WR_PS_SW:
        mikasa_pal_ps = (mikasa_pal_ps & ~MPAL_PS_SW_MASK) |
            (arg32 & MPAL_PS_SW_MASK);
        break;

    case MPAL_RSCC:
        mikasa_pal_scc = mikasa_pal_scc +
            ((t_uint64) ((pcc_l - mikasa_pal_last_pcc) & M32) *
             (t_uint64) (mikasa_scc_scale? mikasa_scc_scale: 1));
        mikasa_pal_last_pcc = pcc_l;
        R[0] = mikasa_pal_scc;
        break;

    case MPAL_RD_UNQ:
        R[0] = mikasa_pal_thread;
        break;

    case MPAL_WR_UNQ:
        mikasa_pal_thread = R[16];
        break;

    case MPAL_CLRFEN:
        fpen = 0;
        break;

    case MPAL_MIKASA_CCALL:
        return mikasa_console_callback ();

    default:
        return STOP_NSPAL;
        }

return SCPE_OK;
}

t_stat mikasa_boot_prepare (CONST char *bootdev, uint32 osflags,
    t_uint64 image_bytes)
{
t_uint64 mem_pages;
t_uint64 bitmap_bytes;
t_uint64 bitmap_pages;
t_uint64 console_pages;
t_uint64 hwrpb_pa = MIKASA_HWRPB_PA;
t_uint64 mddt_pa = hwrpb_pa + MIKASA_MDDT_OFF;
t_uint64 proc_pa = hwrpb_pa + MIKASA_PROCESSOR_OFF;
t_uint64 ctb_pa = hwrpb_pa + MIKASA_CTB_OFF;
t_uint64 bitmap_pa = MIKASA_PFN_BITMAP_PA;
t_uint64 bitmap_cursor = bitmap_pa;
uint32 cluster = 0;

if (!ADDR_IS_MEM (hwrpb_pa + MIKASA_HWRPB_SIZE - 1))
    return SCPE_NXM;

mikasa_zero_phys (hwrpb_pa, MIKASA_HWRPB_SIZE);
mem_pages = ((t_uint64) MEMSIZE) / MIKASA_PAGE_SIZE;
bitmap_bytes = (mem_pages + 7) >> 3;
bitmap_pages = (bitmap_bytes + MIKASA_PAGE_SIZE - 1) / MIKASA_PAGE_SIZE;
if (!ADDR_IS_MEM (bitmap_pa + (bitmap_pages * MIKASA_PAGE_SIZE) - 1))
    return SCPE_NXM;
if ((bitmap_pa + bitmap_bytes) > (hwrpb_pa + MIKASA_HWRPB_SIZE))
    return SCPE_NXM;

console_pages = (MIKASA_BOOT_PT_RESERVED_END +
    MIKASA_PAGE_SIZE - 1) / MIKASA_PAGE_SIZE;
if (mem_pages <= console_pages)
    return SCPE_NXM;
mikasa_zero_phys (bitmap_pa, (uint32) (bitmap_pages * MIKASA_PAGE_SIZE));
mikasa_apb_dma_valid = FALSE;
mikasa_set_boot_env (bootdev, osflags);
mikasa_clear_io_channels ();
mikasa_build_boot_pt ();
mikasa_load_boot_tlb ();

WritePQ (hwrpb_pa + 0x000, hwrpb_pa);                  /* physical address */
WritePQ (hwrpb_pa + 0x008, HWRPB_ID);                  /* "HWRPB" */
WritePQ (hwrpb_pa + 0x010, 6);                         /* revision */
WritePQ (hwrpb_pa + 0x018, MIKASA_HWRPB_SIZE);
WritePQ (hwrpb_pa + 0x020, 0);                         /* CPU id */
WritePQ (hwrpb_pa + 0x028, MIKASA_PAGE_SIZE);
WritePQ (hwrpb_pa + 0x030, 34);                        /* PA bits */
WritePQ (hwrpb_pa + 0x038, 127);                       /* max ASN */
WritePQ (hwrpb_pa + 0x050, HWRPB_ST_DEC_1000_COMPAT);  /* AlphaServer 1000 */
WritePQ (hwrpb_pa + 0x058, 0);                         /* variation */
WritePQ (hwrpb_pa + 0x060, 0);                         /* revision */
WritePQ (hwrpb_pa + 0x068, 1024 * 4096);               /* interval freq */
WritePQ (hwrpb_pa + 0x070, 266000000);                 /* cycle counter */
WritePQ (hwrpb_pa + 0x078, MIKASA_PT_SPACE_VA);        /* VPTB */
WritePQ (hwrpb_pa + 0x090, 1);                         /* processors */
WritePQ (hwrpb_pa + 0x098, MIKASA_PROCESSOR_SIZE);
WritePQ (hwrpb_pa + 0x0A0, MIKASA_PROCESSOR_OFF);
WritePQ (hwrpb_pa + 0x0A8, 1);                         /* CTB count */
WritePQ (hwrpb_pa + 0x0B0, MIKASA_CTB_SIZE);
WritePQ (hwrpb_pa + 0x0B8, MIKASA_CTB_OFF);
WritePQ (hwrpb_pa + 0x0C0, MIKASA_CRB_OFF);
WritePQ (hwrpb_pa + 0x0C8, MIKASA_MDDT_OFF);
WritePQ (hwrpb_pa + 0x110, MIKASA_SWRPB_VA);

WritePQ (proc_pa + 0x000, MIKASA_BOOT_STACK_TOP);       /* initial KSP */
WritePQ (proc_pa + 0x020, MIKASA_L1_PT_PA >> VA_N_OFF); /* initial PTBR */
WritePQ (proc_pa + HWRPB_HWPCB_PT_VA_OFF, MIKASA_PT_SPACE_VA);
WritePQ (proc_pa + 0x080, HWRPB_SLOT_STATE_BOOT);
WritePQ (proc_pa + 0x0A8, 0);                          /* PAL revision */
WritePQ (proc_pa + 0x0B0, HWRPB_EV45_CPU);
WritePQ (proc_pa + 0x0B8, HWRPB_CPU_VAR_EV45);         /* variation */
WritePQ (proc_pa + 0x0C0, 0);                          /* revision */
WritePQ (proc_pa + 0x0E8, proc_pa);                    /* halt PCBB */
WritePQ (proc_pa + 0x118, 0);                          /* bootstrap halt code */

WritePQ (mddt_pa + 0x008, 0);                          /* optional PA */
mikasa_write_mem_cluster (mddt_pa, cluster++, 0, console_pages,
    HWRPB_PMR_USAGE_CONSOLE, &bitmap_cursor);
mikasa_write_mem_cluster (mddt_pa, cluster++, console_pages,
    mem_pages - console_pages, 0, &bitmap_cursor);
WritePQ (mddt_pa + 0x010, cluster);                     /* clusters */
WritePQ (mddt_pa + 0x000, mikasa_sum_qwords (mddt_pa + 8,
    0x10 + (cluster * MIKASA_PMR_SIZE)));

WritePQ (ctb_pa + 0x000, HWRPB_CTB_TYPE_TERMINAL);
WritePQ (ctb_pa + 0x008, 0);                           /* OPA0 unit */
WritePQ (ctb_pa + 0x018, MIKASA_CTB_TERM_SIZE);
WritePQ (ctb_pa + 0x038, 9600);                        /* baud rate */

mikasa_write_console_crb (hwrpb_pa);

WritePQ (hwrpb_pa + HWRPB_CHECKSUM_OFF,
    mikasa_sum_qwords (hwrpb_pa, HWRPB_CHECKSUM_OFF));

R[5] = osflags;
R[16] = hwrpb_pa;
R[17] = 0;
R[18] = 0;
R[30] = MIKASA_BOOT_STACK_TOP;
pal_mode = 0;
dmapen = 1;
fpen = 1;
pal_type = PAL_VMS;
mikasa_hwrpb_pa = hwrpb_pa;
mikasa_last_osflags = osflags;
mikasa_last_boot_bytes = image_bytes;
mikasa_pal_pcbb = proc_pa;
mikasa_pal_ptbr = MIKASA_L1_PT_PA;
mikasa_pal_ipl = 0x1F;
mikasa_pal_mces = 8;
mikasa_pal_last_pcc = pcc_l;

return SCPE_OK;
}

t_stat mikasa_reset (DEVICE *dptr)
{
mikasa_irq_mask = 0;
mikasa_irq_summary = 0;
mikasa_uart_lcr = 0;
mikasa_uart_mcr = 0;
mikasa_uart_ier = 0;
mikasa_uart_scr = 0;
mikasa_uart_dll = 0;
mikasa_uart_dlm = 0;
mikasa_uart_rbr = 0;
mikasa_uart_rbr_valid = FALSE;
mikasa_com2_lcr = 0;
mikasa_com2_mcr = 0;
mikasa_com2_ier = 0;
mikasa_com2_scr = 0;
mikasa_com2_dll = 0;
mikasa_com2_dlm = 0;
mikasa_pic_cmd[0] = 0;
mikasa_pic_cmd[1] = 0;
mikasa_pic_imr[0] = 0;
mikasa_pic_imr[1] = 0;
mikasa_cfg_index = 0;
memset (mikasa_cfg_reg, 0, sizeof (mikasa_cfg_reg));
mikasa_pit_mode = 0;
memset (mikasa_pit_reg, 0, sizeof (mikasa_pit_reg));
mikasa_portb = 0;
mikasa_rtc_index = 0;
memset (mikasa_rtc_reg, 0, sizeof (mikasa_rtc_reg));
mikasa_rtc_reg[MIKASA_RTC_STATUSA] = 0x26;
mikasa_rtc_reg[MIKASA_RTC_STATUSB] = MIKASA_RTC_24HR;
mikasa_sio_index = 0;
memset (mikasa_sio_reg, 0, sizeof (mikasa_sio_reg));
mikasa_nmi_ctrl = 0;
mikasa_eisa_cram_page = 0;
memset (mikasa_eisa_cram, 0, sizeof (mikasa_eisa_cram));
memset (mikasa_epic_reg, 0, sizeof (mikasa_epic_reg));
memset (mikasa_ncr_cfg, 0, sizeof (mikasa_ncr_cfg));
mikasa_ncr_cfg[0x00 >> 2] = 0x00011000u;
mikasa_ncr_cfg[0x04 >> 2] = 0x02000001u;
mikasa_ncr_cfg[0x08 >> 2] = 0x01000001u;
mikasa_ncr_cfg[0x10 >> 2] = 0x00000001u;
mikasa_ncr_cfg[0x3C >> 2] = 0x401101FFu;
mikasa_ncr_init_regs ();
memset (mikasa_pceb_cfg, 0, sizeof (mikasa_pceb_cfg));
mikasa_pceb_cfg[0x00 >> 2] = 0x04828086u;
mikasa_pceb_cfg[0x04 >> 2] = 0x02000007u;
mikasa_pceb_cfg[0x08 >> 2] = 0x06020003u;
mikasa_pceb_cfg[0x40 >> 2] = 0x80800020u;
memset (mikasa_ocp_reg, 0, sizeof (mikasa_ocp_reg));
return SCPE_OK;
}
