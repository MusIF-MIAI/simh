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

#define MIKASA_HWRPB_PA             0x00002000
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
#define MIKASA_PFN_BITMAP_PA        (MIKASA_HWRPB_PA + 0x2000)
#define MIKASA_PMR_SIZE             0x38
#define MIKASA_BOOT_IMAGE_PA        0x00200000
#define MIKASA_BOOT_IMAGE_VA        0x20000000
#define MIKASA_BOOT_STACK_TOP       0x200CE000
#define MIKASA_BOOT_RESERVED_SIZE   0x00100000
#define MIKASA_PT_SPACE_VA          0x40000000
#define MIKASA_PT_SPACE_PA          0x00400000
#define MIKASA_PT_SPACE_SIZE        0x00800000
#define MIKASA_L1_PT_PA             MIKASA_PT_SPACE_PA
#define MIKASA_L3_REGION0_OFF       0x00040000
#define MIKASA_L3_REGION1_OFF       0x00080000
#define MIKASA_L3_PTSPACE_OFF       0x000C0000
#define MIKASA_L2_PT_OFF            0x00100000
#define MIKASA_L3_REGION0_PA        (MIKASA_PT_SPACE_PA + MIKASA_L3_REGION0_OFF)
#define MIKASA_L3_REGION1_PA        (MIKASA_PT_SPACE_PA + MIKASA_L3_REGION1_OFF)
#define MIKASA_L3_PTSPACE_PA        (MIKASA_PT_SPACE_PA + MIKASA_L3_PTSPACE_OFF)
#define MIKASA_L2_PT_PA             (MIKASA_PT_SPACE_PA + MIKASA_L2_PT_OFF)
#define MIKASA_TLB_SPAN             0x00400000
#define MIKASA_BOOT_PT_RESERVED_END (MIKASA_L2_PT_PA + MIKASA_PAGE_SIZE)

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
extern t_uint64 R[32];
extern uint32 pal_mode;
extern uint32 pal_type;
extern uint32 dmapen;
extern uint32 fpen;
extern uint32 pcc_l;
extern UNIT dka_unit[];

uint32 mikasa_irq_mask = 0;
uint32 mikasa_irq_summary = 0;
t_uint64 mikasa_hwrpb_pa = MIKASA_HWRPB_PA;
t_uint64 mikasa_last_osflags = 0;
t_uint64 mikasa_last_boot_bytes = 0;
t_uint64 mikasa_callback_count = 0;
t_uint64 mikasa_getenv_count = 0;
t_uint64 mikasa_ioread_count = 0;
t_uint64 mikasa_iowrite_count = 0;
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
t_stat mikasa_pal_proc_inst (uint32 fnc);

static void mikasa_zero_phys (t_uint64 pa, uint32 size);
static t_uint64 mikasa_sum_qwords (t_uint64 pa, uint32 size);
static t_uint64 mikasa_fill_bitmap (t_uint64 bitmap_pa, t_uint64 bits);
static void mikasa_write_mem_cluster (t_uint64 mddt_pa, uint32 cluster,
    t_uint64 start_pfn, t_uint64 pfn_count, uint32 usage,
    t_uint64 *bitmap_pa);
static t_uint64 mikasa_hwrpb_va_for_pa (t_uint64 pa);
static t_uint64 mikasa_pte (t_uint64 pa, uint32 flags);
static void mikasa_map_range (t_uint64 l3_pa, t_uint64 va_base,
    t_uint64 pa_base, t_uint64 bytes);
static void mikasa_build_boot_pt (void);
static void mikasa_load_tlb_range (t_uint64 va, t_uint64 pa, t_uint64 bytes,
    uint32 flags);
static void mikasa_load_boot_tlb (void);
static void mikasa_write_console_crb (t_uint64 hwrpb_pa);
static void mikasa_set_boot_env (CONST char *bootdev, uint32 osflags);
static t_uint64 mikasa_callback_status (t_uint64 status, t_uint64 count);
static const char *mikasa_get_env_value (uint32 env);
static void mikasa_copy_env_value (const char *value);
static uint32 mikasa_boot_target (CONST char *bootdev);
static void mikasa_clear_io_channels (void);
static int32 mikasa_parse_io_unit (const char *devstr);
static void mikasa_read_callback_string (t_uint64 va, t_uint64 len,
    char *buf, uint32 bufsize);
static void mikasa_open_io (void);
static void mikasa_close_io (void);
static void mikasa_read_io (void);
static void mikasa_write_io (void);
static t_stat mikasa_console_callback (void);

static char mikasa_auto_action[] = "BOOT";
static char mikasa_booted_dev[64] = "SCSI 0 6 0 0 0 0 0";
static char mikasa_booted_osflags[32] = "0";
static char mikasa_boot_file[] = "";
static char mikasa_boot_reset[] = "OFF";
static char mikasa_enable_audit[] = "ON";
static char mikasa_language[] = "36";
static char mikasa_tty_dev[] = "0";
static uint32 mikasa_io_channel[MIKASA_DKA_UNITS] = { 0 };

UNIT mikasa_unit = { UDATA (NULL, 0, 0) };

REG mikasa_reg[] = {
    { HRDATA (HWRPB, mikasa_hwrpb_pa, 64), REG_RO },
    { HRDATA (OSFLAGS, mikasa_last_osflags, 64), REG_RO },
    { HRDATA (BOOTBYTES, mikasa_last_boot_bytes, 64), REG_RO },
    { HRDATA (CALLBACKS, mikasa_callback_count, 64), REG_RO },
    { HRDATA (GETENVS, mikasa_getenv_count, 64), REG_RO },
    { HRDATA (IOREADS, mikasa_ioread_count, 64), REG_RO },
    { HRDATA (IOWRITES, mikasa_iowrite_count, 64), REG_RO },
    { HRDATA (IRQSMM, mikasa_irq_summary, 16) },
    { HRDATA (IRQMASK, mikasa_irq_mask, 16) },
    { HRDATA (PALIPL, mikasa_pal_ipl, 5) },
    { HRDATA (PALPCBB, mikasa_pal_pcbb, 64) },
    { HRDATA (PALSCBB, mikasa_pal_scbb, 64) },
    { NULL }
    };

DEVICE mikasa_dev = {
    "MIKASA", &mikasa_unit, mikasa_reg, NULL,
    1, 16, 32, 1, 16, 8,
    NULL, NULL, &mikasa_reset,
    NULL, NULL, NULL, NULL,
    0, 0, NULL
    };

static void mikasa_zero_phys (t_uint64 pa, uint32 size)
{
uint32 i;

for (i = 0; i < size; i = i + 8)
    WritePQ (pa + i, 0);
return;
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
WritePQ (MIKASA_L1_PT_PA, mikasa_pte (MIKASA_L2_PT_PA, PTE_KRE | PTE_KWE));
WritePQ (MIKASA_L2_PT_PA + (l2_region0 << 3),
    mikasa_pte (MIKASA_L3_REGION0_PA, PTE_KRE | PTE_KWE));
WritePQ (MIKASA_L2_PT_PA + (l2_region1 << 3),
    mikasa_pte (MIKASA_L3_REGION1_PA, PTE_KRE | PTE_KWE));
WritePQ (MIKASA_L2_PT_PA + (l2_ptspace << 3),
    mikasa_pte (MIKASA_L3_PTSPACE_PA, PTE_KRE | PTE_KWE));
mikasa_map_range (MIKASA_L3_REGION0_PA, MIKASA_HWRPB_VA, MIKASA_HWRPB_PA,
    MIKASA_HWRPB_SIZE);
mikasa_map_range (MIKASA_L3_REGION1_PA, MIKASA_BOOT_IMAGE_VA,
    MIKASA_BOOT_IMAGE_PA, MIKASA_BOOT_RESERVED_SIZE);
mikasa_map_range (MIKASA_L3_PTSPACE_PA, MIKASA_PT_SPACE_VA,
    MIKASA_PT_SPACE_PA, MIKASA_PT_SPACE_SIZE);
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
mikasa_load_tlb_range (MIKASA_PT_SPACE_VA, MIKASA_PT_SPACE_PA,
    MIKASA_PT_SPACE_SIZE, PTE_KRE | PTE_KWE);
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
#define MPAL_RD_PS      0x90
#define MPAL_REI        0x91
#define MPAL_SWASTEN    0x9B
#define MPAL_WR_PS_SW   0x9C
#define MPAL_RSCC       0x9D
#define MPAL_RD_UNQ     0x9E
#define MPAL_WR_UNQ     0x9F
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

    case MPAL_RD_PS:
        R[0] = ((t_uint64) mikasa_pal_ipl << 8) |
            (mikasa_pal_ps & MPAL_PS_SW_MASK);
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
            ((pcc_l - mikasa_pal_last_pcc) & M32);
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
return SCPE_OK;
}
