#include "alpha_ev4_ipr.h"
#include "alpha_ev5_defs.h"
#include "alpha_ev4_state.h"

/* Transitional EV4 IPR bridge.
   Current implementation stays delegated to existing Mikasa PAL handlers.
   This file establishes the dedicated EV4 IPR backend surface so EV4
   HW_MFPR/HW_MTPR logic can be moved here incrementally. */

extern t_stat ev4_pal_19 (uint32 ir);
extern t_stat ev4_pal_1d (uint32 ir);
extern t_uint64 R[32];
extern t_uint64 ev5_excaddr, ev5_palbase;
extern t_uint64 ev5_itb_pte, ev5_itb_pte_temp;
extern t_uint64 ev5_dtb_pte, ev5_dtb_pte_temp;
extern t_uint64 ev5_dtb_pte_temp;
extern t_uint64 ev5_va;
extern uint32 ev5_sirr, ev5_astrr, ev5_asten;
extern uint32 ev5_crd;
extern uint32 ev5_itb_tag, ev5_dtb_tag;
extern uint32 ev5_alt_mode;
extern uint32 ev5_mm_stat, ev5_va_lock;
extern uint32 ev5_mcsr;
extern t_uint64 ev5_icsr;
extern uint32 ev5_dc_test_tag, ev5_dc_test_tag_temp;
extern uint32 pcc_h, pcc_l, pcc_enb;
extern uint32 int_req[IPL_HLVL];
extern uint32 mikasa_irq_mask;
extern uint32 cpu_model;
extern t_uint64 ev4_ps_read (void);
extern uint32 ev4_ps_write_mode (t_uint64 val);
extern t_uint64 ev4_read_icsr (void);
extern t_uint64 ev4_read_abox_ipr (uint32 idx);
extern t_uint64 ev4_hirr_bits (uint32 pins);
extern void ev4_write_icsr (t_uint64 val);
extern void ev4_write_ibox_ipr (uint32 idx, t_uint64 val);
extern void ev4_write_abox_ipr (uint32 idx, t_uint64 val);
extern t_uint64 ev5_paltemp[PALTEMP_SIZE];
extern t_uint64 itlb_read (void);
extern t_uint64 dtlb_read (void);
extern void tlb_ia (uint32 sw);
extern void tlb_is (t_uint64 va, uint32 flags);
extern uint32 itlb_cm, ev5_ipl;
extern uint32 trap_summ, trap_mask;
extern void itlb_set_cm (uint32 cm);
extern void dtlb_set_cm (uint32 cm);
extern void mikasa_refresh_cpu_info (void);

static void ev4_flush_ic (void)
{
tlb_ia (TLB_CI | TLB_CA);
}

static void ev4_flush_ic_asm (void)
{
tlb_ia (TLB_CI);
}

static void ev4_itb_zap (void)
{
tlb_ia (TLB_CI | TLB_CA);
}

static void ev4_itb_asm (void)
{
tlb_ia (TLB_CI);
}

static void ev4_itb_is (t_uint64 va)
{
tlb_is (va, TLB_CI);
}

static void ev4_dtb_zap (void)
{
tlb_ia (TLB_CD | TLB_CA);
}

static void ev4_dtb_asm (void)
{
tlb_ia (TLB_CD);
}

static void ev4_dtb_is (t_uint64 va)
{
tlb_is (va, TLB_CD);
}

static void ev4_xtb_zap (void)
{
tlb_ia (TLB_CI | TLB_CD | TLB_CA);
}

static void ev4_xtb_asm (void)
{
tlb_ia (TLB_CI | TLB_CD);
}

static t_uint64 ev4_ps_pack (void)
{
    return ((t_uint64) (ev4_state.ps_sw & 0x7)) |
        (((t_uint64) (itlb_cm & 0x3)) << 3) |
        (((t_uint64) (ev5_ipl & 0x1F)) << 8);
}

static void ev4_ps_unpack (t_uint64 val)
{
    uint32 ipl = (uint32) ((val >> 8) & 0x1F);
    uint32 cm = (((uint32) val) >> 3) & 0x3;

    ev4_state.ps_sw = ((uint32) val) & 0x7;
    ev4_state.ipl = ipl;
    ev5_ipl = ev4_state.ipl;
    itlb_set_cm (cm);
    dtlb_set_cm (cm);
    ev4_state.itlb_cm = cm;
    ev4_state.dtlb_cm = cm;
}

static void ev4_pt9_unpack_raw (t_uint64 val)
{
    uint32 ipl = (uint32) ((val >> 8) & 0x1F);
    uint32 cm = (((uint32) val) >> 3) & 0x3;

    ev4_state.ps_sw = (uint32) (val & 0x7);
    ev4_state.ipl = ipl;
    ev5_ipl = ipl;
    itlb_set_cm (cm);
    dtlb_set_cm (cm);
    ev4_state.itlb_cm = cm;
    ev4_state.dtlb_cm = cm;
}

static void ev4_apply_ptintmask (void)
{
    uint32 ipl = ev4_state.ipl & 0x7;
    uint32 mask = (uint32) ((ev4_state.paltemp_raw[22] >> (ipl * 8)) & 0xFF);

    ev4_state.hier = (mask >> 1) & 0x3F;
    ev4_state.hier_pc0 = mask & 0x01;
    ev4_state.hier_pc1 = (mask >> 7) & 0x01;
}

static t_stat ev4_ipr_mfpr_subset (uint32 ir, t_bool *handled)
{
uint32 fnc = I_GETMDSP (ir);
uint32 ra = I_GETRA (ir);
uint32 idx = fnc & 0x1F;
uint32 is_paltemp = fnc & 0x80;
uint32 is_abox = fnc & 0x40;
uint32 is_ibox = fnc & 0x20;
t_uint64 res = 0;

*handled = FALSE;
if (is_paltemp)
    switch (idx) {
        case 27:                                        /* pt27/WHAMI CPU id */
            mikasa_refresh_cpu_info ();
            res = ev4_state.paltemp_raw[idx];
            if (ra != 31)
                R[ra] = res & M64;
            *handled = TRUE;
            return SCPE_OK;
        case 2:                                         /* pt2_iccsr alias */
            res = ev4_state.paltemp_raw[2];
            if (ra != 31)
                R[ra] = res & M64;
            *handled = TRUE;
            return SCPE_OK;
        case 9:                                         /* pt9_ps alias */
            res = ev4_state.paltemp_raw[9];
            if (ra != 31)
                R[ra] = res & M64;
            *handled = TRUE;
            return SCPE_OK;
        case 22:                                        /* ptIntMask */
            res = ev4_state.paltemp_raw[22];
            if (ra != 31)
                R[ra] = res & M64;
            *handled = TRUE;
            return SCPE_OK;
        default:
            res = ev4_state.paltemp_raw[idx];
            if (ra != 31)
                R[ra] = res & M64;
            *handled = TRUE;
            return SCPE_OK;
        }
if (is_ibox) switch (idx) {
    case 9:                                             /* PS */
        res = ev4_ps_pack ();
        break;
    case 13:                                            /* SIRR */
        res = ((t_uint64) (ev4_state.sirr & SIRR_M_SIRR)) << SIRR_V_SIRR;
        break;
    case 12: {                                          /* HIRR */
        uint32 pins = 0, i;
        for (i = 0; i < IPL_HLVL; i++) {
            if (int_req[i])
                pins |= 1u << i;
            }
        res = ev4_hirr_bits (pins) |
            (pins ? 0x02 : 0) |
            ((ev5_crd || ev4_state.hirr_crr) ? (1u << 4) : 0) |
            (ev4_state.hirr_pc0 ? (1u << 9) : 0) |
            (ev4_state.hirr_pc1 ? (1u << 8) : 0) |
            (ev4_state.hirr_slr ? (1u << 13) : 0);
        break;
        }
    case 10:                                            /* EXC_SUM */
        res = trap_summ & TRAP_SUMM_RW;
        break;
    case 4:                                             /* EXC_ADDR */
        res = ev4_state.exc_addr;
        break;
    case 2:                                             /* ICCSR */
        res = ev4_read_icsr ();
        break;
    case 3:                                             /* ITB_PTE_TEMP */
        res = ev5_itb_pte_temp;
        break;
    case 1: {                                           /* ITB_PTE */
        static const uint32 itbr_map_gh[4] = {
            ITBR_PTE_GH0, ITBR_PTE_GH1, ITBR_PTE_GH2, ITBR_PTE_GH3 };
        t_uint64 pte = itlb_read ();
        ev5_itb_pte_temp = (pte & PFN_MASK) |
            ((pte & PTE_ASM)? ITBR_PTE_ASM: 0) |
            ((pte & (PTE_KRE|PTE_ERE|PTE_SRE|PTE_URE)) <<
                (ITBR_PTE_V_KRE - PTE_V_KRE)) |
            itbr_map_gh[PTE_GETGH (pte)];
        res = 0;
        break;
        }
    case 11:                                            /* PAL_BASE */
        res = ev4_state.pal_base & 0x00000003FFFFC000;
        break;
    case 16:                                            /* HIER */
        res = ev4_hirr_bits (ev4_state.hier) |
            (((t_uint64) (ev4_state.hier_cre & 1)) << 4) |
            (((t_uint64) (ev4_state.hier_pc0 & 1)) << 9) |
            (((t_uint64) (ev4_state.hier_pc1 & 1)) << 8) |
            (((t_uint64) (ev4_state.hier_sle & 1)) << 13);
        break;
    case 18:                                            /* ASTER */
        res = ev4_state.aster & AST_MASK;
        break;
    case 5:                                             /* SL_RCV */
        res = ev4_state.sl_rcv;
        break;
    case 17:                                            /* SIER */
        res = ev4_state.sier;
        break;
    case 14:                                            /* ASTRR */
        res = ev5_astrr & AST_MASK;
        break;
    default:
        res = ev4_state.ibox_ipr_raw[idx];
        break;
    }
if (is_abox) switch (idx) {
    case 0:                                             /* TB_CTL/DTB_CTL */
        res = ev4_state.abox_ipr_raw[idx];
        break;
    case 2:                                             /* DTB_PTE */
        ev5_dtb_pte_temp = dtlb_read ();
        res = 0;
        break;
    case 3:                                             /* DTB_PTE_TEMP */
        res = ev5_dtb_pte_temp;
        break;
    case 4:                                             /* MM_CSR */
        res = ev5_mm_stat;
        break;
    case 5:                                             /* VA */
        ev5_va_lock = 0;
        res = ev5_va;
        break;
    case 9:                                             /* BIU_ADDR */
    case 10:                                            /* BIU_STAT */
    case 11:                                            /* DC_ADDR */
    case 12:                                            /* DC_STAT */
    case 13:                                            /* FILL_ADDR */
    case 14:                                            /* ABOX_CTL */
    case 18:                                            /* BIU_CTL */
    case 19:                                            /* FILL_SYNDROME */
    case 20:                                            /* BC_TAG */
        res = ev4_read_abox_ipr (idx);
        break;
    case 16:                                            /* CC */
        res = (((t_uint64) pcc_h) << 32) | ((t_uint64) pcc_l);
        break;
    default:
        res = ev4_state.abox_ipr_raw[idx];
        break;
    }
if (!is_ibox && !is_abox)
    return SCPE_OK;
if (ra != 31)
    R[ra] = res & M64;
*handled = TRUE;
return SCPE_OK;
}

static t_stat ev4_ipr_mtpr_subset (uint32 ir, t_bool *handled)
{
uint32 fnc = I_GETMDSP (ir);
uint32 ra = I_GETRA (ir);
uint32 idx = fnc & 0x1F;
uint32 is_paltemp = fnc & 0x80;
uint32 is_abox = fnc & 0x40;
uint32 is_ibox = fnc & 0x20;
t_uint64 val = R[ra];

*handled = FALSE;
if (is_paltemp)
    switch (idx) {
        case 2:                                         /* pt2_iccsr alias */
            ev4_write_icsr (val);
            ev4_state.paltemp_raw[2] = ev5_icsr;
            ev5_paltemp[idx] = ev4_state.paltemp_raw[2];
            *handled = TRUE;
            return SCPE_OK;
        case 9:                                         /* pt9_ps alias */
            {
            ev4_pt9_unpack_raw (val);
            ev4_apply_ptintmask ();
            ev4_state.paltemp_raw[9] = val;
            ev5_paltemp[idx] = val;
            *handled = TRUE;
            return SCPE_OK;
            }
        case 22:                                        /* ptIntMask */
            ev4_state.paltemp_raw[22] = val;
            ev5_paltemp[22] = val;
            ev4_apply_ptintmask ();
            *handled = TRUE;
            return SCPE_OK;
        default:
            ev4_state.paltemp_raw[idx] = val;
            ev5_paltemp[idx] = val;
            *handled = TRUE;
            return SCPE_OK;
        }
if (is_ibox) switch (idx) {
    case 9:                                             /* PS */
        {
        ev4_ps_unpack (val);
        ev4_apply_ptintmask ();
        break;
        }
    case 13:                                            /* SIRR */
        ev4_state.sirr = (((uint32) val) >> SIRR_V_SIRR) & SIRR_M_SIRR;
        break;
    case 12:                                            /* HIRR */
        /* W1C/W0C platform-dependent; preserve programmable shadow for now. */
        ev4_state.ibox_ipr_raw[idx] = val;
        break;
    case 14:                                            /* ASTRR */
        ev5_astrr = ((uint32) val) & AST_MASK;
        break;
    case 10:                                            /* EXC_SUM */
        trap_summ = 0;
        trap_mask = 0;
        break;
    case 4:                                             /* EXC_ADDR */
        ev4_state.exc_addr = val;
        ev5_excaddr = val;
        break;
    case 2:                                             /* ICCSR */
        ev4_write_icsr (val);
        break;
    case 11:                                            /* PAL_BASE */
        ev4_state.pal_base = val & 0x00000003FFFFC000;
        ev5_palbase = ev4_state.pal_base;
        if (cpu_model == ALPHA_MODEL_MIKASA_4_266)
            mikasa_palbase_changed (ev5_palbase);
        break;
    case 0:                                             /* TB_TAG */
        ev5_itb_tag = VA_GETVPN (val);
        ev5_dtb_tag = VA_GETVPN (val);
        break;
    case 1:                                             /* ITB_PTE */
        ev5_itb_pte = (val | PTE_V) & (PFN_MASK | ((t_uint64) (PTE_ASM | PTE_GH |
            PTE_KRE | PTE_ERE | PTE_SRE | PTE_URE)));
        itlb_load (ev5_itb_tag, ev5_itb_pte);
        break;
    case 6:                                             /* ITBZAP */
        ev4_itb_zap ();
        break;
    case 7:                                             /* ITBASM */
        ev4_itb_asm ();
        break;
    case 8:                                             /* ITBIS */
        ev4_itb_is (val);
        break;
    case 19:                                            /* SL_CLR */
        ev4_state.sl_clr = val;
        if ((val & (((t_uint64) 1) << 2)) == 0) {
            ev5_crd = 0;
            ev4_state.hirr_crr = 0;
            }
        if ((val & (((t_uint64) 1) << 8)) == 0)
            ev4_state.hirr_pc0 = 0;
        if ((val & (((t_uint64) 1) << 15)) == 0)
            ev4_state.hirr_pc1 = 0;
        if ((val & (((t_uint64) 1) << 32)) == 0)
            ev4_state.hirr_slr = 0;
        break;
    case 22:                                            /* SL_XMIT */
        ev4_state.sl_xmit = val;
        break;
    case 17:                                            /* SIER */
        ev4_state.sier = ((uint32) val) & 0xFFFF;
        break;
    case 16:                                            /* HIER */
        ev4_state.hier = (((uint32) val) >> 9) & 0x3F;
        ev4_state.hier_cre = (((uint32) val) >> 2) & 0x1;
        ev4_state.hier_pc0 = (((uint32) val) >> 8) & 0x1;
        ev4_state.hier_pc1 = (((uint32) val) >> 15) & 0x1;
        ev4_state.hier_sle = (uint32) ((val >> 32) & 0x1);
        break;
    case 18:                                            /* ASTER */
        ev4_state.aster = ((uint32) val) & AST_MASK;
        break;
    default:
        ev4_state.ibox_ipr_raw[idx] = val;
        break;
    }
if (is_abox) switch (idx) {
    case 0:                                             /* TB_CTL/DTB_CTL */
        ev4_state.abox_ipr_raw[idx] = val;
        break;
    case 2:                                             /* DTB_PTE */
        ev5_dtb_pte = (val | PTE_V) & (PFN_MASK |
            ((t_uint64) (PTE_MASK & ~PTE_FOE)));
        dtlb_load (ev5_dtb_tag, ev5_dtb_pte);
        break;
    case 6:                                             /* DTBZAP */
        ev4_dtb_zap ();
        break;
    case 7:                                             /* DTBASM */
        ev4_dtb_asm ();
        break;
    case 8:                                             /* DTBIS */
        ev4_dtb_is (val);
        break;
    case 15:                                            /* ALT_MODE */
        ev5_alt_mode = ev4_ps_write_mode (val);
        break;
    case 14:                                            /* ABOX_CTL */
        ev5_mcsr = ((uint32) val) & MCSR_RW;
        ev4_write_abox_ipr (idx, val);
        break;
    case 18:                                            /* BIU_CTL */
        ev4_write_abox_ipr (idx, val);
        break;
    case 20:                                            /* BC_TAG */
        ev5_dc_test_tag = val & DC_TEST_TAG_RW;
        ev4_state.abox_ipr_raw[idx] = val & DC_TEST_TAG_RW;
        break;
    case 21:                                            /* FLUSH_IC */
        ev4_flush_ic ();
        break;
    case 23:                                            /* FLUSH_IC_ASM */
        ev4_flush_ic_asm ();
        break;
    case 16:                                            /* CC */
        pcc_h = (uint32) ((val >> 32) & M32);
        break;
    case 17:                                            /* CC_CTL */
        pcc_l = ((uint32) val) & (M32 & ~CC_CTL_MBZ);
        pcc_enb = (val & CC_CTL_ENB)? 1: 0;
        break;
    default:
        ev4_state.abox_ipr_raw[idx] = val;
        break;
    }
if (!is_ibox && !is_abox)
    return SCPE_OK;
*handled = TRUE;
return SCPE_OK;
}

t_stat ev4_pal_mfpr (uint32 ir)
{
t_bool handled = FALSE;
t_stat r = ev4_ipr_mfpr_subset (ir, &handled);

ev4_sync_from_simh ();
if ((r != SCPE_OK) || handled)
    return r;
ev5_sirr = ev4_state.sirr;
ev5_asten = ev4_state.aster;
ev4_sync_to_simh ();
return ev4_pal_19 (ir);
}

t_stat ev4_pal_mtpr (uint32 ir)
{
t_bool handled = FALSE;
t_stat r = ev4_ipr_mtpr_subset (ir, &handled);

ev4_sync_from_simh ();
if ((r != SCPE_OK) || handled)
    return r;
ev5_sirr = ev4_state.sirr;
ev5_asten = ev4_state.aster;
ev4_sync_to_simh ();
return ev4_pal_1d (ir);
}
