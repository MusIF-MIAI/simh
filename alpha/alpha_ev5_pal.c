/* alpha_ev5_pal.c - Alpha EV5 PAL mode simulator

   Copyright (c) 2003-2006, Robert M Supnik

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
   ROBERT M SUPNIK BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

   Except as contained in this notice, the name of Robert M Supnik shall not be
   used in advertising or otherwise to promote the sale, use or other dealings
   in this Software without prior written authorization from Robert M Supnik.

   EV5 was the second generation Alpha CPU.  It was a four-way, in order issue
   CPU with onchip primary instruction and data caches, an onchip second level
   cache, and support for an offchip third level cache.  EV56 was a shrink, with
   added support for byte and word operations.  PCA56 was a version of EV56
   without the onchip second level cache.  PCA57 was a shrink of PCA56.

   EV5 includes the usual five PALcode instructions:

        HW_LD           PALcode load
        HW_ST           PALcode store
        HW_MTPR         PALcode move to internal processor register
        HW_MFPR         PALcode move from internal processor register
        HW_REI          PALcode return

   PALcode instructions can only be issued in PALmode, or in kernel mode
   if the appropriate bit is set in ICSR.

   EV5 implements 8 "PAL shadow" registers, which replace R8-R14, R25 in
   PALmode without save/restore; and 24 "PAL temporary" registers.

   Internal registers fall into three groups: IBox IPRs, MBox IPRs, and
   PAL temporaries.
*/

#include "alpha_defs.h"
#include "alpha_ev5_defs.h"
#include "alpha_ev4_ipr.h"
#include "alpha_ev4_state.h"

#define EV4_PALO_INTR   0x00E0

t_uint64 ev5_palshad[PALSHAD_SIZE] = { 0 };             /* PAL shadow reg */
t_uint64 ev5_palsave[PALSHAD_SIZE] = { 0 };             /* PAL save main */
t_uint64 ev5_paltemp[PALTEMP_SIZE] = { 0 };             /* PAL temps */
t_uint64 ev5_palbase = 0;                               /* PALcode base */
t_uint64 ev5_excaddr = 0;                               /* exception address */
t_uint64 ev5_isr = 0;                                   /* intr summary */
t_uint64 ev5_icsr = 0;                                  /* IBox control */
t_uint64 ev5_itb_pte = 0;                               /* ITLB pte */
t_uint64 ev5_itb_pte_temp = 0;                          /* ITLB readout */
t_uint64 ev5_ivptbr = 0;                                /* IBox virt ptbl */
t_uint64 ev5_iva_form = 0;                              /* Ibox fmt'd VA */
t_uint64 ev5_va = 0;                                    /* Mbox VA */
t_uint64 ev5_mvptbr = 0;                                /* Mbox virt ptbl */
t_uint64 ev5_va_form = 0;                               /* Mbox fmt'd VA */
t_uint64 ev5_dtb_pte = 0;                               /* DTLB pte */
t_uint64 ev5_dtb_pte_temp = 0;                          /* DTLB readout */
t_uint64 ev5_dc_test_tag = 0;                           /* Dcache test tag */
t_uint64 ev5_dc_test_tag_temp = 0;                      /* Dcache tag readout */
uint32 ev5_itb_tag = 0;                                 /* ITLB tag (vpn) */
uint32 ev5_dtb_tag = 0;                                 /* DTLB tag (vpn) */
uint32 ev5_icperr = 0;                                  /* Icache par err */
uint32 ev5_mm_stat = 0;                                 /* MBox fault code */
uint32 ev5_mcsr = 0;                                    /* MBox control */
uint32 ev5_alt_mode = 0;                                /* MBox alt mode */
uint32 ev5_dc_mode = 0;                                 /* Dcache mode */
uint32 ev5_dcperr = 0;                                  /* Dcache par err */
uint32 ev5_dc_test_ctl = 0;                             /* Dcache test ctrl */
uint32 ev5_maf_mode = 0;                                /* MAF mode */
uint32 ev5_va_lock = 0;                                 /* VA lock flag */
uint32 ev5_mchk = 0;                                    /* machine check pin */
uint32 ev5_sli = 0;                                     /* serial line intr */
uint32 ev5_crd = 0;                                     /* corr read data pin */
uint32 ev5_pwrfl = 0;                                   /* power fail pin */
uint32 ev5_ipl = 0;                                     /* ipl */
uint32 ev5_sirr = 0;                                    /* software int req */
uint32 ev5_astrr = 0;                                   /* AST requests */
uint32 ev5_asten = 0;                                   /* AST enables */
uint32 ev4_hier = 0;                                    /* hardware int enables */
uint32 ev4_ps_sw = 0;                                   /* EV4 PS software bits */
const uint32 ast_map[4] = { 0x1, 0x3, 0x7, 0xF };

t_stat ev5_palent (t_uint64 fpc, uint32 off);
t_stat ev5_palent_d (t_uint64 fpc, uint32 off, uint32 sta);
t_stat pal_proc_reset_hwre (DEVICE *dptr);
t_stat pal_proc_intr_ev5 (uint32 lvl);
uint32 pal_eval_intr_ev5 (uint32 flag);
t_stat ev4_pal_19 (uint32 ir);
t_stat ev4_pal_1d (uint32 ir);
static t_bool pal_hwre_enabled (void);
static t_bool ev4_ipr_is_mikasa (void);
t_bool ev4_icsr_hwe (void);
t_uint64 ev4_hirr_bits (uint32 pins);
t_uint64 ev4_read_icsr (void);
t_uint64 ev4_read_ibox_ipr (uint32 idx);
t_uint64 ev4_read_abox_ipr (uint32 idx);
void ev4_write_icsr (t_uint64 val);
void ev4_write_ibox_ipr (uint32 idx, t_uint64 val);
void ev4_write_abox_ipr (uint32 idx, t_uint64 val);

#define EV4_ICCSR_W_V_ASN       47
#define EV4_ICCSR_W_M_ASN       0x3F
#define EV4_ICCSR_W_V_PCE       44
#define EV4_ICCSR_W_M_PCE       0x3
#define EV4_ICCSR_W_RES46       (((t_uint64) 1u) << 46)
#define EV4_ICCSR_W_RES43       (((t_uint64) 1u) << 43)
#define EV4_ICCSR_W_FPE         (((t_uint64) 1u) << 42)
#define EV4_ICCSR_W_MAP         (((t_uint64) 1u) << 41)
#define EV4_ICCSR_W_HWE         (((t_uint64) 1u) << 40)
#define EV4_ICCSR_W_DI          (((t_uint64) 1u) << 39)
#define EV4_ICCSR_W_BHE         (((t_uint64) 1u) << 38)
#define EV4_ICCSR_W_JSE         (((t_uint64) 1u) << 37)
#define EV4_ICCSR_W_BPE         (((t_uint64) 1u) << 36)
#define EV4_ICCSR_W_PIPE        (((t_uint64) 1u) << 35)
#define EV4_ICCSR_W_V_PCMUX1    32
#define EV4_ICCSR_W_M_PCMUX1    0x7
#define EV4_ICCSR_W_V_PCMUX0    8
#define EV4_ICCSR_W_M_PCMUX0    0xF
#define EV4_ICCSR_W_RES4        (((t_uint64) 1u) << 4)
#define EV4_ICCSR_W_PC0         (((t_uint64) 1u) << 3)
#define EV4_ICCSR_W_PC1         (((t_uint64) 1u) << 0)
#define EV4_ICCSR_W_RW          (((t_uint64) EV4_ICCSR_W_M_ASN << EV4_ICCSR_W_V_ASN) | \
                                 ((t_uint64) EV4_ICCSR_W_M_PCE << EV4_ICCSR_W_V_PCE) | \
                                 EV4_ICCSR_W_RES46 | EV4_ICCSR_W_RES43 | \
                                 EV4_ICCSR_W_FPE | EV4_ICCSR_W_MAP | EV4_ICCSR_W_HWE | \
                                 EV4_ICCSR_W_DI | EV4_ICCSR_W_BHE | EV4_ICCSR_W_JSE | \
                                 EV4_ICCSR_W_BPE | EV4_ICCSR_W_PIPE | \
                                 ((t_uint64) EV4_ICCSR_W_M_PCMUX1 << EV4_ICCSR_W_V_PCMUX1) | \
                                 ((t_uint64) EV4_ICCSR_W_M_PCMUX0 << EV4_ICCSR_W_V_PCMUX0) | \
                                 EV4_ICCSR_W_RES4 | EV4_ICCSR_W_PC0 | EV4_ICCSR_W_PC1)

#define EV4_ICCSR_R_V_ASN       28
#define EV4_ICCSR_R_V_PCE       44
#define EV4_ICCSR_R_RES34       (((t_uint64) 1u) << 34)
#define EV4_ICCSR_R_V_RES       24
#define EV4_ICCSR_R_FPE         (((t_uint64) 1u) << 23)
#define EV4_ICCSR_R_MAP         (((t_uint64) 1u) << 22)
#define EV4_ICCSR_R_HWE         (((t_uint64) 1u) << 21)
#define EV4_ICCSR_R_DI          (((t_uint64) 1u) << 20)
#define EV4_ICCSR_R_BHE         (((t_uint64) 1u) << 19)
#define EV4_ICCSR_R_JSE         (((t_uint64) 1u) << 18)
#define EV4_ICCSR_R_BPE         (((t_uint64) 1u) << 17)
#define EV4_ICCSR_R_PIPE        (((t_uint64) 1u) << 16)
#define EV4_ICCSR_R_V_PCMUX1    13
#define EV4_ICCSR_R_V_PCMUX0    9
#define EV4_ICCSR_R_PC1         (((t_uint64) 1u) << 2)
#define EV4_ICCSR_R_PC0         (((t_uint64) 1u) << 1)

extern t_uint64 R[32];
extern t_uint64 PC;
extern t_uint64 trap_mask;
extern t_uint64 p1;
extern uint32 ir;
extern uint32 vax_flag, lock_flag;
extern uint32 fpen;
extern uint32 pcc_h, pcc_l, pcc_enb;
extern uint32 trap_summ;
extern uint32 arch_mask;
extern uint32 pal_mode, pal_type;
extern uint32 cpu_model;
extern uint32 int_req[IPL_HLVL];
extern uint32 itlb_cm, dtlb_cm;
extern uint32 itlb_asn, dtlb_asn;
extern uint32 itlb_spage, dtlb_spage;
extern jmp_buf save_env;
extern uint32 pal_type;
extern t_uint64 pcq[PCQ_SIZE];                          /* PC queue */
extern int32 pcq_p;                                     /* PC queue ptr */

extern int32 parse_reg (const char *cptr);
extern t_bool mikasa_pal_uses_simh_ipl (void);
/* EV5PAL data structures

   ev5pal_dev   device descriptor
   ev5pal_unit  unit
   ev5pal_reg   register list
*/

UNIT ev5pal_unit = { UDATA (NULL, 0, 0) };

REG ev5pal_reg[] = {
    { BRDATA (PALSHAD, ev5_palshad, 16, 64, PALSHAD_SIZE) },
    { BRDATA (PALSAVE, ev5_palsave, 16, 64, PALSHAD_SIZE) },
    { BRDATA (PALTEMP, ev5_paltemp, 16, 64, PALTEMP_SIZE) },
    { HRDATA (PALBASE, ev5_palbase, 64) },
    { HRDATA (EXCADDR, ev5_excaddr, 64) },
    { HRDATA (IPL, ev5_ipl, 5) },
    { HRDATA (SIRR, ev5_sirr, 15) },
    { HRDATA (ASTRR, ev5_astrr, 4) },
    { HRDATA (ASTEN, ev5_asten, 4) },
    { HRDATA (ISR, ev5_isr, 35) },
    { HRDATA (ICSR, ev5_icsr, 40) },
    { HRDATA (ITB_TAG, ev5_itb_tag, 32) },
    { HRDATA (ITB_PTE, ev5_itb_pte, 64) },
    { HRDATA (ITB_PTE_TEMP, ev5_itb_pte_temp, 64) },
    { HRDATA (IVA_FORM, ev5_iva_form, 64) },
    { HRDATA (IVPTBR, ev5_ivptbr, 64) },
    { HRDATA (ICPERR_STAT, ev5_icperr, 14) },
    { HRDATA (VA, ev5_va, 64) },
    { HRDATA (VA_FORM, ev5_va_form, 64) },
    { HRDATA (MVPTBR, ev5_mvptbr, 64) },
    { HRDATA (MM_STAT, ev5_mm_stat, 17) },
    { HRDATA (MCSR, ev5_mcsr, 6) },
    { HRDATA (DTB_TAG, ev5_dtb_tag, 32) },
    { HRDATA (DTB_PTE, ev5_dtb_pte, 64) },
    { HRDATA (DTB_PTE_TEMP, ev5_dtb_pte_temp, 64) },
    { HRDATA (DC_MODE, ev5_dc_mode, 4) },
    { HRDATA (DC_PERR_STAT, ev5_dcperr, 6) },
    { HRDATA (DC_TEST_CTL, ev5_dc_test_ctl, 13) },
    { HRDATA (DC_TEST_TAG, ev5_dc_test_tag, 39) },
    { HRDATA (DC_TEST_TAG_TEMP, ev5_dc_test_tag_temp, 39) },
    { HRDATA (MAF_MODE, ev5_maf_mode, 8) },
    { FLDATA (VA_LOCK, ev5_va_lock, 0) },
    { FLDATA (MCHK, ev5_mchk, 0) },
    { FLDATA (CRD, ev5_crd, 0) },
    { FLDATA (PWRFL, ev5_pwrfl, 0) },
    { FLDATA (SLI, ev5_sli, 0) },
    { NULL }
    };

DEVICE ev5pal_dev = {
    "EV5PAL", &ev5pal_unit, ev5pal_reg, NULL,
    1, 16, 1, 1, 16, 8,
    NULL, NULL, &pal_proc_reset_hwre,
    NULL, NULL, NULL,
    NULL, DEV_DIS
    };

/* EV5 interrupt dispatch - reached from top of instruction loop -
   dispatch to PALcode */

t_stat ev5_pal_proc_intr (uint32 lvl)
{
return ev5_palent (PC, PALO_INTR);
}

/* EV5 trap dispatch - reached from bottom of instruction loop -
   trap_mask and trap_summ are set up correctly - dispatch to PALcode */

t_stat ev5_pal_proc_trap (uint32 summ)
{
return ev5_palent (PC, PALO_TRAP);
}

/* EV5 exception dispatch - reached from ABORT handler -
   set up any exception-specific registers - dispatch to PALcode */

t_stat ev5_pal_proc_excp (uint32 abval)
{
switch (abval) {

    case EXC_RSVI:                                      /* reserved instruction */
        return ev5_palent (PC, PALO_RSVI);

    case EXC_ALIGN:                                     /* unaligned */
        return ev5_palent (PC, PALO_ALGN);

    case EXC_FPDIS:                                     /* fp disabled */
        return ev5_palent (PC, PALO_FDIS);

    case EXC_FOX+EXC_R:                                 /* FOR */
        return ev5_palent_d (PC, PALO_DFLT, MM_STAT_FOR);

    case EXC_FOX+EXC_W:                                 /* FOW */
        return ev5_palent_d (PC, PALO_DFLT, MM_STAT_FOR|MM_STAT_WR);

    case EXC_BVA+EXC_E:                                 /* instr bad VA */
    case EXC_ACV+EXC_E:                                 /* instr ACV */
        ev5_itb_tag = VA_GETVPN (PC);                   /* fault VPN */
        if (ev5_icsr & ICSR_NT)                         /* formatted addr */
            ev5_iva_form = ev5_ivptbr | FMT_IVA_NT (PC);
        else ev5_iva_form = ev5_ivptbr | FMT_IVA_VMS (PC);
        return ev5_palent (PC, PALO_IACV);

    case EXC_ACV+EXC_R:                                 /* data read ACV */
        return ev5_palent_d (PC, PALO_DFLT, MM_STAT_ACV);

    case EXC_ACV+EXC_W:                                 /* data write ACV */
        return ev5_palent_d (PC, PALO_DFLT, MM_STAT_ACV|MM_STAT_WR);

    case EXC_BVA+EXC_R:                                 /* data read bad addr */
        return ev5_palent_d (PC, PALO_DFLT, MM_STAT_BVA);

    case EXC_BVA+EXC_W:                                 /* data write bad addr */
        return ev5_palent_d (PC, PALO_DFLT, MM_STAT_BVA|MM_STAT_WR);

    case EXC_TBM + EXC_E:                               /* TLB miss */
        ev5_itb_tag = VA_GETVPN (PC);                   /* fault VPN */
        if (ev5_icsr & ICSR_NT)                         /* formatted addr */
            ev5_iva_form = ev5_ivptbr | FMT_IVA_NT (PC);
        else ev5_iva_form = ev5_ivptbr | FMT_IVA_VMS (PC);
        return ev5_palent (PC, PALO_ITBM);

    case EXC_TBM + EXC_R:                               /* data TB miss read */
        if ((I_GETOP (ir) == HW_LD) && (ir & HW_LD_PTE))
            return ev5_palent_d (PC, PALO_DTBM_D, MM_STAT_TBM);
        return ev5_palent_d (PC, PALO_DTBM, MM_STAT_TBM);

    case EXC_TBM + EXC_W:                               /* data TB miss write */
        if ((I_GETOP (ir) == HW_LD) && (ir & HW_LD_PTE))
            return ev5_palent_d (PC, PALO_DTBM_D, MM_STAT_TBM|MM_STAT_WR);
        return ev5_palent_d (PC, PALO_DTBM, MM_STAT_TBM|MM_STAT_WR);

    case EXC_RSVO:                                      /* reserved operand */
    case EXC_TNV+EXC_E:                                 /* instr TNV */
    case EXC_TNV+EXC_R:                                 /* data read TNV */
    case EXC_TNV+EXC_W:                                 /* data write TNV */
    case EXC_FOX+EXC_E:                                 /* FOE */
        return SCPE_IERR;                               /* should never get here */

    default:
        return STOP_INVABO;
        }

return SCPE_OK;
}

/* EV5 call PAL - reached from instruction decoder -
   compute offset from function code - dispatch to PALcode */

t_stat ev5_pal_proc_inst (uint32 fnc)
{
uint32 off = (fnc & 0x3F) << 6;

if (fnc & 0x80) return ev5_palent (PC, PALO_CALLUNPR + off);
if (itlb_cm != MODE_K) ABORT (EXC_RSVI);
return ev5_palent (PC, PALO_CALLPR + off);
}

/* EV5 evaluate interrupts - returns highest outstanding
   interrupt level about target ipl - plus nonmaskable flags
   
   flag = 1: evaluate for real interrupt capability
   flag = 0: evaluate as though IPL = 0, normal mode */

uint32 pal_eval_intr (uint32 flag)
{
uint32 i, req = 0;
uint32 lvl = (flag && (!ev4_ipr_is_mikasa () ||
    mikasa_pal_uses_simh_ipl ()))? ev5_ipl: 0;
uint32 mikasa_sirr = ev5_sirr;
uint32 mikasa_asten = ev5_asten;
uint32 mikasa_astrr = ev5_astrr;

if (flag && pal_mode) return 0;
if (ev5_mchk) req = IPL_1F;
else if (!ev4_ipr_is_mikasa () && ev5_crd && (ev5_icsr & ICSR_CRDE))
    req = IPL_CRD;
else if (ev5_pwrfl) req = IPL_PWRFL;
else if (ev4_ipr_is_mikasa ()) {
    mikasa_sirr = ev4_state.sirr;
    mikasa_asten = ev4_state.aster;
    uint32 hwre = 0;

    if (ev5_crd && ev4_state.hier_cre)
        req = IPL_CRD;
    else if ((ev4_state.hirr_pc0 && ev4_state.hier_pc0) ||
        (ev4_state.hirr_pc1 && ev4_state.hier_pc1))
        req = IPL_HMIN;
    else {
        for (i = 0; i < IPL_HLVL; i++) {
            if (int_req[i])
                hwre |= 1u << i;
            }
        hwre &= ev4_state.hier;
        for (i = IPL_HLVL; i > 0; i--) {
            if ((hwre >> (i - 1)) & 1) {
                req = IPL_HMIN + i - 1;
                break;
                }
            }
        }
    if ((req == 0) && mikasa_sirr) {
        for (i = IPL_SMAX; i > 0; i--) {
            if ((mikasa_sirr >> (i - 1)) & 1) {
                req = i;
                break;
                }
            }
        }
    }
else if (int_req[3] && !(ev5_icsr & ICSR_MSK3)) req = IPL_HMIN + 3;
else if (int_req[2] && !(ev5_icsr & ICSR_MSK2)) req = IPL_HMIN + 2;
else if (int_req[1] && !(ev5_icsr & ICSR_MSK1)) req = IPL_HMIN + 1;
else if (int_req[0] && !(ev5_icsr & ICSR_MSK0)) req = IPL_HMIN + 0;
else if (ev5_sirr) {
    for (i = IPL_SMAX; i > 0; i--) {                    /* check swre int */
        if ((ev5_sirr >> (i - 1)) & 1) {                /* req != 0? int */
            req = i;
            break;
            }
        }
    }
if ((req < IPL_AST) && (mikasa_astrr & mikasa_asten & ast_map[itlb_cm]))
    req = IPL_AST;
if (req <= lvl) req = 0;
if (ev5_sli && (ev5_icsr & ICSR_SLE)) req = req | IPL_SLI;
if (ev5_isr & ISR_HALT) req = req | IPL_HALT;
return req;
}

/* EV5 enter PAL, data TLB miss/memory management flows -
   set Mbox registers - dispatch to PALcode  */

t_stat ev5_palent_d (t_uint64 fpc, uint32 off, uint32 sta)
{
if (!ev5_va_lock) {                                     /* not locked? */
    ev5_mm_stat = sta |                                 /* merge IR<31:21> */
        ((ir >> (I_V_RA - MM_STAT_V_RA)) & MM_STAT_IMASK);
    ev5_va = p1;                                        /* fault address */
    if (ev5_mcsr & MCSR_NT)                             /* formatted VA */
        ev5_va_form = ev5_mvptbr | FMT_MVA_NT (p1);
    else ev5_va_form = ev5_mvptbr | FMT_MVA_VMS (p1);
    ev5_va_lock = 1;                                    /* lock registers */
    }
return ev5_palent (fpc, off);
}

/* EV5 enter PAL */

t_stat ev5_palent (t_uint64 fpc, uint32 off)
{
ev5_excaddr = fpc | pal_mode;                           /* save exc addr */
PCQ_ENTRY;                                              /* save PC */
PC = ev5_palbase + off;                                 /* new PC */
if (!pal_mode && (ev5_icsr & ICSR_SDE)) {               /* entering PALmode? */
    PAL_USE_SHADOW;                                     /* swap in shadows */
    }
pal_mode = 1;                                           /* in PAL mode */
return SCPE_OK;
}

/* PAL instructions */

/* 1B: HW_LD */

t_stat pal_1b (uint32 ir)
{
t_uint64 dsp, ea, res;
uint32 ra, rb, acc, mode;

if (!pal_mode && (!(itlb_cm == MODE_K) ||               /* pal mode, or kernel */
    !pal_hwre_enabled ())) ABORT (EXC_RSVI);            /* and enabled? */
ra = I_GETRA (ir);                                      /* get ra */
rb = I_GETRB (ir);                                      /* get rb */
dsp = HW_LD_GETDSP (ir);                                /* get displacement */
ea = (R[rb] + (SEXT_HW_LD_DSP (dsp))) & M64;            /* eff address */
if (ir & HW_LD_V) {                                     /* virtual? */
    mode = (ir & HW_LD_ALT)? ev5_alt_mode: dtlb_cm;     /* access mode */
    acc = (ir & HW_LD_WCH)? ACC_W (mode): ACC_R (mode);
    if (ir & HW_LD_Q) res = ReadAccQ (ea, acc);         /* quad? */
    else {                                              /* long, sext */
        res = ReadAccL (ea, acc);
        res = SEXT_L_Q (res);
        }
    }
else if (ir & HW_LD_Q) res = ReadPQ (ea);               /* physical, quad? */
else {
    res = ReadPL (ea);                                  /* long, sext */
    res = SEXT_L_Q (res);
    }
if (ir & HW_LD_LCK) lock_flag = 1;                      /* lock? set flag */
if (ra != 31) R[ra] = res;                              /* if not R31, store */
return SCPE_OK;
}

/* 1F: HW_ST */

t_stat pal_1f (uint32 ir)
{
t_uint64 dsp, ea;
uint32 ra, rb, acc, mode;

if (!pal_mode && (!(itlb_cm == MODE_K) ||               /* pal mode, or kernel */
    !pal_hwre_enabled ())) ABORT (EXC_RSVI);            /* and enabled? */
ra = I_GETRA (ir);                                      /* get ra */
rb = I_GETRB (ir);                                      /* get rb */
dsp = HW_LD_GETDSP (ir);                                /* get displacement */
ea = (R[rb] + (SEXT_HW_LD_DSP (dsp))) & M64;            /* eff address */
if ((ir & HW_LD_LCK) && !lock_flag) R[ra] = 0;          /* lock fail? */
else {
    if (ir & HW_LD_V) {                                 /* virtual? */
        mode = (ir & HW_LD_ALT)? ev5_alt_mode: dtlb_cm; /* access mode */
        acc = ACC_W (mode);
        if (ir & HW_LD_Q) WriteAccQ (ea, R[ra], acc);   /* quad? */
        else WriteAccL (ea, R[ra], acc);                /* long */
        }
    else if (ir & HW_LD_Q) WritePQ (ea, R[ra]);         /* physical, quad? */
    else WritePL (ea,  R[ra]);                          /* long */
    if (ir & HW_LD_LCK) lock_flag = 0;                  /* unlock? clr flag */
    }
return SCPE_OK;
}

/* 1E: HW_REI */

t_stat pal_1e (uint32 ir)
{
uint32 new_pal = ((uint32) ev5_excaddr) & 1;

if (!pal_mode && (!(itlb_cm == MODE_K) ||               /* pal mode, or kernel */
    !pal_hwre_enabled ())) ABORT (EXC_RSVI);            /* and enabled? */
if (ev4_ipr_is_mikasa ())
    return ev4_rei ();
PCQ_ENTRY;
PC = ev5_excaddr;
if (pal_mode && !new_pal && (ev5_icsr & ICSR_SDE)) {    /* leaving PAL mode? */
    PAL_USE_MAIN;                                       /* swap out shadows */
    }
pal_mode = new_pal;
return SCPE_OK;
}

/* PAL move from processor registers */

t_stat pal_19 (uint32 ir)
{
t_uint64 res;
uint32 fnc, ra;
static const uint32 itbr_map_gh[4] = {
    ITBR_PTE_GH0, ITBR_PTE_GH1, ITBR_PTE_GH2, ITBR_PTE_GH3 };

fnc = I_GETMDSP (ir);
ra = I_GETRA (ir);
if (ev4_ipr_is_mikasa ())
    return ev4_pal_mfpr (ir);
if (!pal_mode && (!(itlb_cm == MODE_K) ||               /* pal mode, or kernel */
    !pal_hwre_enabled ())) ABORT (EXC_RSVI);            /* and enabled? */
switch (fnc) {

    case ISR:                                           /* intr summary */
        res = ev5_isr | ((ev5_astrr & ev5_asten) << ISR_V_AST) |
            ((ev5_sirr & SIRR_M_SIRR) << ISR_V_SIRR) |
            (int_req[0] && !(ev5_icsr & ICSR_MSK0)? ISR_IRQ0: 0) |
            (int_req[1] && !(ev5_icsr & ICSR_MSK1)? ISR_IRQ1: 0) |
            (int_req[2] && !(ev5_icsr & ICSR_MSK2)? ISR_IRQ2: 0) |
            (int_req[3] && !(ev5_icsr & ICSR_MSK3)? ISR_IRQ3: 0);
        if (ev5_astrr & ev5_asten & ast_map[itlb_cm]) res = res | ISR_ATR;
        break;

    case ITB_PTE:
        res = itlb_read ();
        ev5_itb_pte_temp = (res & PFN_MASK) |
            ((res & PTE_ASM)? ITBR_PTE_ASM: 0) |
            ((res & (PTE_KRE|PTE_ERE|PTE_SRE|PTE_URE)) << 
                (ITBR_PTE_V_KRE - PTE_V_KRE)) |
            itbr_map_gh[PTE_GETGH (res)];
        res = 0;
        break;

    case ITB_ASN:
        res = (itlb_asn & ITB_ASN_M_ASN) << ITB_ASN_V_ASN;
        break;

    case ITB_PTE_TEMP:
        res = ev5_itb_pte_temp;
        break;

    case SIRR:
        res = (ev5_sirr & SIRR_M_SIRR) << SIRR_V_SIRR;
        break;

    case ASTRR:
        res = ev5_astrr & AST_MASK;
        break;

    case ASTEN:
        res = ev5_asten & AST_MASK;
        break;

    case EXC_ADDR:
        res = ev5_excaddr;
        break;

    case EXC_SUMM:
        res = trap_summ & TRAP_SUMM_RW;
        break;

    case EXC_MASK:
        res = trap_mask;
        break;

    case PAL_BASE:
        res = ev5_palbase & PAL_BASE_RW;
        break;

    case ICM:
        res = (itlb_cm & ICM_M_CM) << ICM_V_CM;
        break;

    case IPLR:
        res = (ev5_ipl & IPLR_M_IPL) << IPLR_V_IPL;
        break;

    case INTID:
        res = pal_eval_intr (0) & INTID_MASK;
        break;

    case IFAULT_VA_FORM:
        res = ev5_iva_form;
        break;

    case IVPTBR:
        res = ev5_ivptbr;
        break;

    case ICSR:
        res = (ev5_icsr & ICSR_RW) | ICSR_MBO |
            ((itlb_spage & ICSR_M_SPE) << ICSR_V_SPE) |
            ((fpen & 1) << ICSR_V_FPE) |
            ((arch_mask & AMASK_BWX)? ICSR_BSE: 0);
        break;

    case PALTEMP+0x00:     case PALTEMP+0x01:     case PALTEMP+0x02:     case PALTEMP+0x03:
    case PALTEMP+0x04:     case PALTEMP+0x05:     case PALTEMP+0x06:     case PALTEMP+0x07:
    case PALTEMP+0x08:     case PALTEMP+0x09:     case PALTEMP+0x0A:     case PALTEMP+0x0B:
    case PALTEMP+0x0C:     case PALTEMP+0x0D:     case PALTEMP+0x0E:     case PALTEMP+0x0F:
    case PALTEMP+0x10:     case PALTEMP+0x11:     case PALTEMP+0x12:     case PALTEMP+0x13:
    case PALTEMP+0x14:     case PALTEMP+0x15:     case PALTEMP+0x16:     case PALTEMP+0x17:
        res = ev5_paltemp[fnc - PALTEMP];
        break;

    case DTB_PTE:
        ev5_dtb_pte_temp = dtlb_read ();
        res = 0;
        break;

    case DTB_PTE_TEMP:
        res = ev5_dtb_pte_temp;
        break;

    case MM_STAT:
        res = ev5_mm_stat;
        break;

    case VA:
        res = ev5_va;
        ev5_va_lock = 0;
        break;

    case VA_FORM:
        res = ev5_va_form;
        break;

    case DC_PERR_STAT:
        res = ev5_dcperr;
        break;

    case MCSR:
        res = (ev5_mcsr & MCSR_RW) | ((dtlb_spage & MCSR_M_SPE) << MCSR_V_SPE);
        break;

    case DC_MODE:
        res = ev5_dc_mode & DC_MODE_RW;
        break;

    case MAF_MODE:
        res = ev5_maf_mode & MAF_MODE_RW;
        break;

    case CC:
        res = (((t_uint64) pcc_h) << 32) | ((t_uint64) pcc_l);
        break;

    case DC_TEST_CTL:
        res = ev5_dc_test_ctl & DC_TEST_CTL_RW;
        break;

    case DC_TEST_TAG:
        // to be determined
        res = 0;
        break;

    case DC_TEST_TAG_TEMP:
        res = ev5_dc_test_tag_temp & DC_TEST_TAG_RW;
        break;

    default:
        res = 0;
        break;
        }

if (ra != 31) R[ra] = res & M64;
return SCPE_OK;
}

/* PAL move to processor registers */

t_stat pal_1d (uint32 ir)
{
uint32 fnc = I_GETMDSP (ir);
uint32 ra = I_GETRA (ir);
t_uint64 val = R[ra];

if (ev4_ipr_is_mikasa ())
    return ev4_pal_mtpr (ir);
if (!pal_mode && (!(itlb_cm == MODE_K) ||               /* pal mode, or kernel */
    !pal_hwre_enabled ())) ABORT (EXC_RSVI);            /* and enabled? */
switch (fnc) {

    case ITB_TAG:
        ev5_itb_tag = VA_GETVPN (val);
        break;

    case ITB_PTE:
        ev5_itb_pte = (val | PTE_V) & (PFN_MASK | ((t_uint64) (PTE_ASM | PTE_GH |
                PTE_KRE | PTE_ERE | PTE_SRE | PTE_URE)));           
        itlb_load (ev5_itb_tag, ev5_itb_pte);
        break;

    case ITB_ASN:
        itlb_set_asn ((((uint32) val) >> ITB_ASN_V_ASN) & ITB_ASN_M_ASN);
        break;

    case ITB_IA:
        tlb_ia (TLB_CI | TLB_CA);
        break;

    case ITB_IAP:
        tlb_ia (TLB_CI);
        break;

    case ITB_IS:
        tlb_is (val, TLB_CI);
        break;

    case SIRR:
        ev5_sirr = (((uint32) val) >> SIRR_V_SIRR) & SIRR_M_SIRR;
        break;

    case ASTRR:
        ev5_astrr = ((uint32) val) & AST_MASK;
        break;

    case ASTEN:
        ev5_asten = ((uint32) val) & AST_MASK;
        break;

    case EXC_ADDR:
        ev5_excaddr = val;
        break;

    case EXC_SUMM:
        trap_summ = 0;
        trap_mask = 0;
        break;

    case PAL_BASE:
        ev5_palbase = val & PAL_BASE_RW;
        if (ev4_ipr_is_mikasa ())
            mikasa_palbase_changed (ev5_palbase);
        break;

    case ICM:
        itlb_set_cm ((((uint32) val) >> ICM_V_CM) & ICM_M_CM);
        break;

    case IPLR:
        ev5_ipl = (((uint32) val) >> IPLR_V_IPL) & IPLR_M_IPL;
        break;

    case IVPTBR:
        if (ev5_icsr & ICSR_NT) ev5_ivptbr = val & IVPTBR_NT;
        else ev5_ivptbr = val & IVPTBR_VMS;
        break;

    case HWINT_CLR:
        ev5_isr = ev5_isr & ~(val & HWINT_CLR_W1C);
        break;

    case ICSR:
        if (pal_mode && ((val ^ ev5_icsr) & ICSR_SDE)) {
            if (val & ICSR_SDE) { PAL_USE_SHADOW; }
            else { PAL_USE_MAIN; }
            }
        ev5_icsr = val & ICSR_RW;
        itlb_set_spage ((((uint32) val) >> ICSR_V_SPE) & ICSR_M_SPE);
        fpen = (((uint32) val) >> ICSR_V_FPE) & 1;
        if (val & ICSR_BSE) arch_mask = arch_mask | AMASK_BWX;
        else arch_mask = arch_mask & ~AMASK_BWX;
        break;

    case ICPERR_STAT:
        ev5_icperr = ev5_icperr & ~(((uint32) val) & ICPERR_W1C);
        break;

    case PALTEMP+0x00:     case PALTEMP+0x01:     case PALTEMP+0x02:     case PALTEMP+0x03:
    case PALTEMP+0x04:     case PALTEMP+0x05:     case PALTEMP+0x06:     case PALTEMP+0x07:
    case PALTEMP+0x08:     case PALTEMP+0x09:     case PALTEMP+0x0A:     case PALTEMP+0x0B:
    case PALTEMP+0x0C:     case PALTEMP+0x0D:     case PALTEMP+0x0E:     case PALTEMP+0x0F:
    case PALTEMP+0x10:     case PALTEMP+0x11:     case PALTEMP+0x12:     case PALTEMP+0x13:
    case PALTEMP+0x14:     case PALTEMP+0x15:     case PALTEMP+0x16:     case PALTEMP+0x17:
        ev5_paltemp[fnc - PALTEMP] = val;
        break;

    case DTB_ASN:
        dtlb_set_asn (((uint32) (val >> DTB_ASN_V_ASN)) & DTB_ASN_M_ASN);
        break;

    case DTB_CM:
        dtlb_set_cm (((uint32) (val >> ICM_V_CM)) & ICM_M_CM);
        break;

    case DTB_TAG:
        ev5_dtb_tag = VA_GETVPN (val);
        val = (val | PTE_V) & (PFN_MASK | ((t_uint64) (PTE_MASK & ~PTE_FOE)));
        dtlb_load (ev5_dtb_tag, val);
        break;

    case DTB_PTE:
        ev5_dtb_pte = val;
        break;

    case MVPTBR:
        ev5_mvptbr = val & ~MVPTBR_MBZ;
        break;

    case DC_PERR_STAT:
        ev5_dcperr = ev5_dcperr & ~(((uint32) val) & DC_PERR_W1C);
        if ((ev5_dcperr & DC_PERR_W1C) == 0) ev5_dcperr = 0;
        break;

    case DTB_IA:
        tlb_ia (TLB_CD | TLB_CA);
        break;

    case DTB_IAP:
        tlb_ia (TLB_CD);
        break;

    case DTB_IS:
        tlb_is (val, TLB_CD);
        break;

    case MCSR:
        ev5_mcsr = ((uint32) val) & MCSR_RW;
        dtlb_set_spage ((((uint32) val) >> MCSR_V_SPE) & MCSR_M_SPE);
        if (ev5_mcsr & MCSR_NT) pal_type = PAL_NT;
        break;

    case DC_MODE:
        ev5_dc_mode = ((uint32) val) & DC_MODE_RW;
        break;

    case MAF_MODE:
        ev5_maf_mode = ((uint32) val) & MAF_MODE_RW;
        break;

    case CC:
        pcc_h = (uint32) ((val >> 32) & M32);
        break;

    case CC_CTL:
        pcc_l = ((uint32) val) & (M32 & ~CC_CTL_MBZ);
        if (val & CC_CTL_ENB) pcc_enb = 1;
        else pcc_enb = 0;
        break;

    case DC_TEST_CTL:
        ev5_dc_test_ctl = ((uint32) val) & DC_TEST_CTL_RW;
        break;

    case DC_TEST_TAG:
        ev5_dc_test_tag = val & DC_TEST_TAG_RW;
        break;

    default:
        break;
        }

return SCPE_OK;
}

static t_bool ev4_ipr_is_mikasa (void)
{
return cpu_model == ALPHA_MODEL_MIKASA_4_266;
}

static t_bool pal_hwre_enabled (void)
{
if (ev4_ipr_is_mikasa ())
    return ev4_icsr_hwe ();
return (ev5_icsr & ICSR_HWE) != 0;
}

t_bool ev4_icsr_hwe (void)
{
return (ev5_icsr & EV4_ICCSR_W_HWE) != 0;
}

t_uint64 ev4_read_icsr (void)
{
t_uint64 res = 0;

res |= ((ev5_icsr >> EV4_ICCSR_W_V_PCE) & EV4_ICCSR_W_M_PCE) <<
    EV4_ICCSR_R_V_PCE;
res |= ((ev5_icsr >> EV4_ICCSR_W_V_ASN) & EV4_ICCSR_W_M_ASN) <<
    EV4_ICCSR_R_V_ASN;
if (fpen)
    res |= EV4_ICCSR_R_FPE;
if (ev5_icsr & EV4_ICCSR_W_MAP)
    res |= EV4_ICCSR_R_MAP;
if (ev5_icsr & EV4_ICCSR_W_HWE)
    res |= EV4_ICCSR_R_HWE;
if (ev5_icsr & EV4_ICCSR_W_DI)
    res |= EV4_ICCSR_R_DI;
if (ev5_icsr & EV4_ICCSR_W_BHE)
    res |= EV4_ICCSR_R_BHE;
if (ev5_icsr & EV4_ICCSR_W_JSE)
    res |= EV4_ICCSR_R_JSE;
if (ev5_icsr & EV4_ICCSR_W_BPE)
    res |= EV4_ICCSR_R_BPE;
if (ev5_icsr & EV4_ICCSR_W_PIPE)
    res |= EV4_ICCSR_R_PIPE;
res |= ((ev5_icsr >> EV4_ICCSR_W_V_PCMUX1) & EV4_ICCSR_W_M_PCMUX1) <<
    EV4_ICCSR_R_V_PCMUX1;
res |= ((ev5_icsr >> EV4_ICCSR_W_V_PCMUX0) & EV4_ICCSR_W_M_PCMUX0) <<
    EV4_ICCSR_R_V_PCMUX0;
if (ev5_icsr & EV4_ICCSR_W_PC1)
    res |= EV4_ICCSR_R_PC1;
if (ev5_icsr & EV4_ICCSR_W_PC0)
    res |= EV4_ICCSR_R_PC0;
return res;
}

void ev4_write_icsr (t_uint64 val)
{
ev5_icsr = val & EV4_ICCSR_W_RW;
itlb_set_spage ((val & EV4_ICCSR_W_MAP)? SPEN_43: 0);
fpen = (val & EV4_ICCSR_W_FPE)? 1: 0;
}

t_uint64 ev4_ps_read (void)
{
return (ev4_ps_sw & 0x7) |
    (((t_uint64) (itlb_cm & 3)) << 3) |
    (((t_uint64) (ev5_ipl & 0x1F)) << 8);
}

t_uint64 ev4_hirr_bits (uint32 pins)
{
return (((t_uint64) (pins & 0x38)) << 2) |
    (((t_uint64) (pins & 0x07)) << 10);
}

uint32 ev4_ps_write_mode (t_uint64 val)
{
return ((uint32) (val >> 3)) & 3;
}

t_uint64 ev4_read_ibox_ipr (uint32 idx)
{
t_uint64 res;
static const uint32 itbr_map_gh[4] = {
    ITBR_PTE_GH0, ITBR_PTE_GH1, ITBR_PTE_GH2, ITBR_PTE_GH3 };

switch (idx) {

    case 1:                                             /* ITB_PTE */
        res = itlb_read ();
        ev5_itb_pte_temp = (res & PFN_MASK) |
            ((res & PTE_ASM)? ITBR_PTE_ASM: 0) |
            ((res & (PTE_KRE|PTE_ERE|PTE_SRE|PTE_URE)) <<
                (ITBR_PTE_V_KRE - PTE_V_KRE)) |
            itbr_map_gh[PTE_GETGH (res)];
        return 0;

    case 2:                                             /* ICCSR */
        return ev4_read_icsr ();

    case 3:                                             /* ITB_PTE_TEMP */
        return ev5_itb_pte_temp;

    case 4:                                             /* EXC_ADDR */
        return ev5_excaddr;

    case 5:                                             /* SL_RCV */
        return 0;

    case 9:                                             /* PS */
        return ev4_ps_read ();

    case 10:                                            /* EXC_SUM */
        return trap_summ & TRAP_SUMM_RW;

    case 11:                                            /* PAL_BASE */
        return ev5_palbase & 0x00000003FFFFC000;

    case 12: {                                          /* HIRR */
        uint32 pins = 0;
        uint32 i;
        t_uint64 res;

        for (i = 0; i < IPL_HLVL; i++) {
            if (int_req[i])
                pins |= 1u << i;
        }
        res = ev4_hirr_bits (pins) | ((pins & ev4_hier) ? 0x02 : 0);
        return res;
        }

    case 13:                                            /* SIRR */
        return ((t_uint64) (ev5_sirr & SIRR_M_SIRR)) << SIRR_V_SIRR;

    case 14:                                            /* ASTRR */
        return ev5_astrr & AST_MASK;

    case 16: {                                          /* HIER */
        t_uint64 res = ev4_hirr_bits (ev4_hier);

        return res;
        }

    case 17:                                            /* SIER */
        return 0;

    case 18:                                            /* ASTER */
        return ev5_asten & AST_MASK;

    default:
        return 0;
        }
}

t_uint64 ev4_read_abox_ipr (uint32 idx)
{
switch (idx) {

    case 2:                                             /* DTB_PTE */
        ev5_dtb_pte_temp = dtlb_read ();
        return 0;

    case 3:                                             /* DTB_PTE_TEMP */
        return ev5_dtb_pte_temp;

    case 4:                                             /* MM_CSR */
        return ev5_mm_stat;

    case 5:                                             /* VA */
        ev5_va_lock = 0;
        return ev5_va;

    case 12:                                            /* DC_STAT/C_STAT */
        return 0;

    case 16:                                            /* CC */
        return (((t_uint64) pcc_h) << 32) | ((t_uint64) pcc_l);

    default:
        return 0;
        }
}

void ev4_write_ibox_ipr (uint32 idx, t_uint64 val)
{
switch (idx) {

    case 0:                                             /* TB_TAG */
        ev5_itb_tag = VA_GETVPN (val);
        ev5_dtb_tag = VA_GETVPN (val);
        break;

    case 1:                                             /* ITB_PTE */
        ev5_itb_pte = (val | PTE_V) & (PFN_MASK | ((t_uint64) (PTE_ASM | PTE_GH |
            PTE_KRE | PTE_ERE | PTE_SRE | PTE_URE)));
        itlb_load (ev5_itb_tag, ev5_itb_pte);
        break;

    case 2:                                             /* ICCSR */
        ev4_write_icsr (val);
        break;

    case 4:                                             /* EXC_ADDR */
        ev5_excaddr = val;
        break;

    case 6:                                             /* ITBZAP */
    case 7:                                             /* ITBASM */
    case 8:                                             /* ITBIS */
        tlb_ia (TLB_CI);
        break;

    case 9:                                             /* PS */
        ev4_ps_sw = ((uint32) val) & 0x7;
        itlb_set_cm (ev4_ps_write_mode (val));
        ev5_ipl = (((uint32) val) >> 8) & 0x1F;
        break;

    case 10:                                            /* EXC_SUM */
        trap_summ = 0;
        trap_mask = 0;
        break;

    case 11:                                            /* PAL_BASE */
        ev5_palbase = val & 0x00000003FFFFC000;
        break;

    case 13:                                            /* SIRR */
        ev5_sirr = (((uint32) val) >> SIRR_V_SIRR) & SIRR_M_SIRR;
        break;

    case 14:                                            /* ASTRR */
        ev5_astrr = ((uint32) val) & AST_MASK;
        break;

    case 16:                                            /* HIER */
        ev4_hier = (((uint32) val) >> 9) & 0x3F;
        break;

    case 18:                                            /* ASTER */
        ev5_asten = ((uint32) val) & AST_MASK;
        break;

    case 19:                                            /* SL_CLR */
    case 22:                                            /* SL_XMIT */
    default:
        break;
        }
}

void ev4_write_abox_ipr (uint32 idx, t_uint64 val)
{
switch (idx) {

    case 2:                                             /* DTB_PTE */
        ev5_dtb_pte = (val | PTE_V) & (PFN_MASK |
            ((t_uint64) (PTE_MASK & ~PTE_FOE)));
        dtlb_load (ev5_dtb_tag, ev5_dtb_pte);
        break;

    case 6:                                             /* DTBZAP */
    case 7:                                             /* DTBASM */
    case 8:                                             /* DTBIS */
        tlb_ia (TLB_CD);
        break;

    case 15:                                            /* ALT_MODE */
        ev5_alt_mode = ev4_ps_write_mode (val);
        break;

    case 16:                                            /* CC */
        pcc_h = (uint32) ((val >> 32) & M32);
        break;

    case 17:                                            /* CC_CTL */
        pcc_l = ((uint32) val) & (M32 & ~CC_CTL_MBZ);
        pcc_enb = (val & CC_CTL_ENB)? 1: 0;
        break;

    case 21:                                            /* FLUSH_IC */
    case 23:                                            /* FLUSH_IC_ASM */
    default:
        break;
        }
}

t_stat ev4_pal_19 (uint32 ir)
{
uint32 fnc = I_GETMDSP (ir);
uint32 ra = I_GETRA (ir);
uint32 idx = fnc & 0x1F;
uint32 is_paltemp = fnc & 0x80;
uint32 is_abox = fnc & 0x40;
uint32 is_ibox = fnc & 0x20;
t_uint64 res = 0;

if (!pal_mode && (!(itlb_cm == MODE_K) ||               /* pal mode, or kernel */
    !pal_hwre_enabled ())) ABORT (EXC_RSVI);            /* and enabled? */
if (is_paltemp)
    res = ev5_paltemp[idx];
else if (is_ibox)
    res = ev4_read_ibox_ipr (idx);
else if (is_abox)
    res = ev4_read_abox_ipr (idx);
else
    return SCPE_OK;
if (ra != 31)
    R[ra] = res & M64;
return SCPE_OK;
}

t_stat ev4_pal_1d (uint32 ir)
{
uint32 fnc = I_GETMDSP (ir);
uint32 ra = I_GETRA (ir);
uint32 idx = fnc & 0x1F;
uint32 is_paltemp = fnc & 0x80;
uint32 is_abox = fnc & 0x40;
uint32 is_ibox = fnc & 0x20;
t_uint64 val = R[ra];

if (!pal_mode && (!(itlb_cm == MODE_K) ||               /* pal mode, or kernel */
    !pal_hwre_enabled ())) ABORT (EXC_RSVI);            /* and enabled? */
if (is_paltemp)
    ev5_paltemp[idx] = val;
if (is_ibox)
    ev4_write_ibox_ipr (idx, val);
if (is_abox)
    ev4_write_abox_ipr (idx, val);
return SCPE_OK;
}

/* EV5 PALcode reset */

t_stat pal_proc_reset_hwre (DEVICE *dptr)
{
ev5_palbase = 0;
ev5_mchk = 0;
ev5_pwrfl = 0;
ev5_crd = 0;
ev5_sli = 0;
ev4_hier = 0;
ev4_ps_sw = 0;
itlb_set_cm (MODE_K);
itlb_set_asn (0);
itlb_set_spage (0);
dtlb_set_cm (MODE_K);
dtlb_set_asn (0);
dtlb_set_spage (0);
return SCPE_OK;
}

/* EV5 PAL instruction print and parse routines */

static const char *pal_inam[] = {
    "HW_MFPR", "HW_LD", "HW_MTPR", "HW_REI", "HW_ST", NULL
    };

static const uint32 pal_ival[] = {
    0x64000000, 0x6C000000, 0x74000000, 0x7BFF8000, 0x7C000000
    };

struct pal_opt {
    uint32      mask;                                   /* bit mask */
    char        let;                                    /* matching letter */
    };

static struct pal_opt ld_st_opt[] = {
    { HW_LD_V,      'V' },
    { HW_LD_ALT,    'A' },
    { HW_LD_WCH,    'W' },
    { HW_LD_Q,      'Q' },
    { HW_LD_PTE,    'P' },
    { HW_LD_LCK,    'L' },
    { 0 }
    };

static struct pal_opt rei_opt[] = {
    { HW_REI_S, 'S' },
    { 0 }
    };

/* Print options for hardware PAL instruction */

void fprint_opt_ev5 (FILE *of, uint32 inst, struct pal_opt opt[])
{
uint32 i;

for (i = 0; opt[i].mask != 0; i++) {
    if (inst & opt[i].mask) {
        fprintf (of, "/%c", opt[i].let);
        inst = inst & ~opt[i].mask;
        }
    }
return;
}

/* Parse options for hardware PAL instruction */

CONST char *parse_opt_ev5 (CONST char *cptr, uint32 *val, struct pal_opt opt[])
{
uint32 i;
char *tptr, gbuf[CBUFSIZE];

if (*(cptr - 1) != '/') return cptr;
cptr = get_glyph (cptr - 1, tptr = gbuf, 0);
while (*tptr == '/') {
    tptr++;
    for (i = 0; opt[i].mask != 0; i++) {
        if (*tptr == opt[i].let) {
            *val = *val | opt[i].mask;
            break;
            }
        }
    if (opt[i].mask == 0) return NULL;
    tptr++;
    }
if (*tptr != 0) return NULL;
return cptr;
}

/* Print PAL hardware opcode symbolically */

t_stat fprint_pal_hwre (FILE *of, uint32 inst)
{
uint32 op, ra, rb;

op = I_GETOP (inst);
ra = I_GETRA (inst);
rb = I_GETRB (inst);
switch (op) {

    case OP_PAL19:                                              /* HW_MFPR */
    case OP_PAL1D:                                              /* HW_MTPR */
        fputs ((op == OP_PAL19)? "HW_MFPR": "HW_MTPR", of);
        fprintf (of, " R%d,%X", ra, inst & M16);
        break;

    case OP_PAL1B:                                              /* HW_LD */
    case OP_PAL1F:                                              /* HW_ST */
        fputs ((op == OP_PAL1B)? "HW_LD": "HW_ST", of);
        fprint_opt_ev5 (of, inst, ld_st_opt);
        fprintf (of, " R%d,%X", ra, inst & HW_LD_DSP);
        if (rb != 31) fprintf (of, "(R%d)", rb);
        break;
 
   case OP_PAL1E:                                               /* HW_REI */
        fputs ("HW_REI", of);
        fprint_opt_ev5 (of, inst, rei_opt);
        break;

    default:
        return SCPE_ARG;
        }

return -3;
}

/* Parse PAL hardware opcode symbolically */

t_stat parse_pal_hwre (CONST char *cptr, t_value *inst)
{
uint32 i, d, val = 0;
int32 reg;
CONST char *tptr;
char gbuf[CBUFSIZE];
t_stat r;

cptr = get_glyph (cptr, gbuf, '/');
for (i = 0; pal_inam[i] != NULL; i++) {
    if (strcmp (gbuf, pal_inam[i]) == 0) val = pal_ival[i];
    }
if (val == 0) return SCPE_ARG;
switch (I_GETOP (val)) {

    case OP_PAL19:                                              /* HW_MFPR */
    case OP_PAL1D:                                              /* HW_MTPR */
        if (*(cptr - 1) == '/') return SCPE_ARG;
        cptr = get_glyph (cptr, gbuf, ',');             /* get reg */
        if ((reg = parse_reg (gbuf)) < 0) return SCPE_ARG;
        val = val | (reg << I_V_RA) | (reg << I_V_RB);
        cptr = get_glyph (cptr, gbuf, 0);               /* get ipr */
        d = (uint32) get_uint (gbuf, 16, M16, &r);
        if (r != SCPE_OK) return r;
        val = val | d;
        break;

    case OP_PAL1B:                                              /* HW_LD */
    case OP_PAL1F:                                              /* HW_ST */
        cptr = parse_opt_ev5 (cptr, &val, ld_st_opt);
        if (cptr == NULL) return SCPE_ARG;
        cptr = get_glyph (cptr, gbuf, ',');             /* get reg */
        if ((reg = parse_reg (gbuf)) < 0) return SCPE_ARG;
        val = val | (reg << I_V_RA);
        cptr = get_glyph (cptr, gbuf, 0);
        d = (uint32) strtotv (gbuf, &tptr, 16);
        if ((gbuf == tptr) || (d > HW_LD_DSP)) return SCPE_ARG;
        val = val | d;
        if (*tptr == '(') {
            tptr = get_glyph (tptr + 1, gbuf, ')');
            if ((reg = parse_reg (gbuf)) < 0) return SCPE_ARG;
            val = val | (reg << I_V_RB);
            }
        else val = val | (31 << I_V_RB);
        break;

    case OP_PAL1E:                                              /* HW_REI */
        cptr = parse_opt_ev5 (cptr, &val, rei_opt);
        if (cptr == NULL) return SCPE_ARG;
        break;

    default:
        return SCPE_ARG;
        }

*inst = val;
if (*cptr != 0) return SCPE_ARG;
return -3;
}
