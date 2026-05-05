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

#define MIKASA_HWRPB_PA             0x10000000
#define MIKASA_HWRPB_SIZE           0x2000
#define MIKASA_MDDT_OFF             0x800
#define MIKASA_PROCESSOR_OFF        0x1000
#define MIKASA_PROCESSOR_SIZE       0x278
#define MIKASA_PAGE_SIZE            8192

#define HWRPB_ID                    0x0000004250525748ULL
#define HWRPB_CHECKSUM_OFF          0x120
#define HWRPB_ST_DEC_1000           5
#define HWRPB_EV45_CPU              3

extern UNIT cpu_unit;
extern t_uint64 R[32];
extern uint32 pal_mode;
extern uint32 pal_type;
extern uint32 dmapen;
extern uint32 fpen;
extern uint32 pcc_l;

uint32 mikasa_irq_mask = 0;
uint32 mikasa_irq_summary = 0;
t_uint64 mikasa_hwrpb_pa = MIKASA_HWRPB_PA;
t_uint64 mikasa_last_osflags = 0;
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
t_stat mikasa_boot_prepare (CONST char *bootdev, uint32 osflags);
t_stat mikasa_pal_proc_inst (uint32 fnc);

static void mikasa_zero_phys (t_uint64 pa, uint32 size);
static t_uint64 mikasa_sum_qwords (t_uint64 pa, uint32 size);

UNIT mikasa_unit = { UDATA (NULL, 0, 0) };

REG mikasa_reg[] = {
    { HRDATA (HWRPB, mikasa_hwrpb_pa, 64), REG_RO },
    { HRDATA (OSFLAGS, mikasa_last_osflags, 64), REG_RO },
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

    default:
        return STOP_NSPAL;
        }

return SCPE_OK;
}

t_stat mikasa_boot_prepare (CONST char *bootdev, uint32 osflags)
{
t_uint64 mem_pages;
t_uint64 hwrpb_pa = MIKASA_HWRPB_PA;
t_uint64 mddt_pa = hwrpb_pa + MIKASA_MDDT_OFF;
t_uint64 proc_pa = hwrpb_pa + MIKASA_PROCESSOR_OFF;

(void) bootdev;

if (!ADDR_IS_MEM (hwrpb_pa + MIKASA_HWRPB_SIZE - 1))
    return SCPE_NXM;

mikasa_zero_phys (hwrpb_pa, MIKASA_HWRPB_SIZE);
mem_pages = ((t_uint64) MEMSIZE) / MIKASA_PAGE_SIZE;

WritePQ (hwrpb_pa + 0x000, hwrpb_pa);                  /* physical address */
WritePQ (hwrpb_pa + 0x008, HWRPB_ID);                  /* "HWRPB" */
WritePQ (hwrpb_pa + 0x010, 6);                         /* revision */
WritePQ (hwrpb_pa + 0x018, MIKASA_HWRPB_SIZE);
WritePQ (hwrpb_pa + 0x020, 0);                         /* CPU id */
WritePQ (hwrpb_pa + 0x028, MIKASA_PAGE_SIZE);
WritePQ (hwrpb_pa + 0x030, 34);                        /* PA bits */
WritePQ (hwrpb_pa + 0x038, 127);                       /* max ASN */
WritePQ (hwrpb_pa + 0x050, HWRPB_ST_DEC_1000);         /* AlphaServer 1000 */
WritePQ (hwrpb_pa + 0x058, 0);                         /* variation */
WritePQ (hwrpb_pa + 0x060, 0);                         /* revision */
WritePQ (hwrpb_pa + 0x068, 1024 * 4096);               /* interval freq */
WritePQ (hwrpb_pa + 0x070, 266000000);                 /* cycle counter */
WritePQ (hwrpb_pa + 0x090, 1);                         /* processors */
WritePQ (hwrpb_pa + 0x098, MIKASA_PROCESSOR_SIZE);
WritePQ (hwrpb_pa + 0x0A0, MIKASA_PROCESSOR_OFF);
WritePQ (hwrpb_pa + 0x0C8, MIKASA_MDDT_OFF);

WritePQ (proc_pa + 0x0A8, 0);                          /* PAL revision */
WritePQ (proc_pa + 0x0B0, HWRPB_EV45_CPU);
WritePQ (proc_pa + 0x0B8, 0);                          /* variation */
WritePQ (proc_pa + 0x0C0, 0);                          /* revision */

WritePQ (mddt_pa + 0x008, 0);                          /* optional PA */
WritePQ (mddt_pa + 0x010, 1);                          /* clusters */
WritePQ (mddt_pa + 0x018, 0);                          /* start PFN */
WritePQ (mddt_pa + 0x020, mem_pages);
WritePQ (mddt_pa + 0x028, mem_pages);
WritePQ (mddt_pa + 0x030, 0);                          /* bitmap VA */
WritePQ (mddt_pa + 0x038, 0);                          /* bitmap PA */
WritePQ (mddt_pa + 0x040, 0);                          /* bitmap checksum */
WritePQ (mddt_pa + 0x048, 0);                          /* usage */
WritePQ (mddt_pa + 0x000, mikasa_sum_qwords (mddt_pa + 8, 0x48));

WritePQ (hwrpb_pa + HWRPB_CHECKSUM_OFF,
    mikasa_sum_qwords (hwrpb_pa, HWRPB_CHECKSUM_OFF));

R[5] = osflags;
R[16] = hwrpb_pa;
R[17] = 0;
R[18] = 0;
pal_mode = 1;
dmapen = 0;
fpen = 1;
pal_type = PAL_VMS;
mikasa_hwrpb_pa = hwrpb_pa;
mikasa_last_osflags = osflags;
mikasa_pal_last_pcc = pcc_l;

return SCPE_OK;
}

t_stat mikasa_reset (DEVICE *dptr)
{
mikasa_irq_mask = 0;
mikasa_irq_summary = 0;
return SCPE_OK;
}
