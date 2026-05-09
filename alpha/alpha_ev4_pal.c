/* alpha_ev4_pal.c - Alpha EV4/Mikasa PAL dispatch shim

   Copyright (c) 1993-2008, Robert M Supnik
   Copyright (c) 2023-2026, The SIMH contributors

   Authoritative model decision for Mikasa:
   - Primary strategy: run original PAL behavior through Mikasa handlers
     in alpha_mikasa.c (and progressively move logic into EV4 backend files).
   - During SRM execution, CALL_PAL functions not intercepted by the Mikasa
     platform handler are vectored into the active SRM PAL image.
   - After OS handoff, native PAL services are implemented in simulator code
     paths and progressively moved into EV4 backend files.
   - EV5 behavior remains isolated in alpha_ev5_pal.c for non-Mikasa models.
   - Non-Mikasa models remain on EV5 backend behavior.
*/

#include "alpha_defs.h"
#include "alpha_ev5_defs.h"
#include "alpha_ev4_pal_defs.h"
#include "alpha_ev4_ipr.h"
#include "alpha_ev4_state.h"

#define STATIC_ASSERT_EQ(name, expr) typedef char static_assert_##name[(expr) ? 1 : -1]
STATIC_ASSERT_EQ(ev4_vec_intr, EV4_PAL_OFF_INTR == 0x00E0);
STATIC_ASSERT_EQ(ev4_ps_layout, (EV4_OSF_PS_M_IPL == 0x7) && (EV4_OSF_PS_M_CM == 0x8));
STATIC_ASSERT_EQ(ev4_cserve_putc, EV4_CSERVE_K_PUTC == 0x0F);

extern uint32 cpu_model;
extern t_uint64 PC_Global;
extern t_uint64 PC;
extern t_uint64 R[32];
extern t_uint64 ev5_palshad[PALSHAD_SIZE], ev5_palsave[PALSHAD_SIZE];
extern t_uint64 pcq[PCQ_SIZE];
extern int32 pcq_p;
extern uint32 pc_align, pal_mode, itlb_cm, dtlb_cm;
extern uint32 fpen, pcc_h, pcc_l, lock_flag;
extern uint32 ev5_ipl;
extern t_uint64 ev5_icsr;
extern t_uint64 ev5_excaddr, ev5_palbase;
extern t_stat ev5_palent (t_uint64 oldpc, uint32 paloff);
extern t_stat ev5_pal_proc_intr (uint32 lvl);
extern t_stat ev5_pal_proc_trap (uint32 summ);
extern t_stat ev5_pal_proc_excp (uint32 abval);
extern t_stat ev5_pal_proc_inst (uint32 fnc);

extern t_stat mikasa_pal_proc_intr (uint32 lvl);
extern t_stat mikasa_pal_proc_trap (uint32 summ);
extern t_stat mikasa_pal_proc_excp (uint32 abval);
extern t_stat mikasa_pal_proc_inst (uint32 fnc);
extern DEVICE mikasa_dev;
extern jmp_buf save_env;

#define MIKASA_DBG_PAL 0x0040

typedef struct {
    t_uint64 iccsr, hirr, hier, sirr, astrr, aster, sier;
    t_uint64 sl_rcv, sl_clr, sl_xmit;
    t_uint64 exc_addr, pal_base, ps, mmcsr, va;
    t_uint64 abox_ctl, biu_ctl, biu_stat, dc_stat, fill_addr, fill_syndrome, bc_tag;
    t_uint64 pt_ent_int, pt_ent_arith, pt_ent_mm, pt_ent_una, pt_ent_sys, pt_ent_if;
    t_uint64 pt_impure, pt_usp, pt_ksp, pt_kgp, pt_int_mask, pt_sys_val;
    t_uint64 pt_mces, pt_whami, pt_ptbr, pt_vpt_ptr, pt_prev_pal, pt_prcb;
} EV4_PAL_STATE;

static EV4_PAL_STATE ev4_pal_state;
EV4_SHARED_STATE ev4_state = { 0 };

static void ev4_use_shadow (void)
{
if (!ev4_state.shadow_active) {
    PAL_USE_SHADOW;
    ev4_state.shadow_active = 1;
    }
}

static void ev4_use_main (void)
{
if (ev4_state.shadow_active) {
    PAL_USE_MAIN;
    ev4_state.shadow_active = 0;
    }
}

static t_stat ev4_unhandled_event (const char *kind, uint32 code)
{
sim_debug (MIKASA_DBG_PAL, &mikasa_dev,
    "EV4PAL unhandled %s code=%u pc=%016llX\n",
    kind, code, (unsigned long long) PC);
return SCPE_IERR;
}

static t_stat ev4_palent (t_uint64 fpc, uint32 off)
{
ev4_state.exc_addr = fpc | pal_mode;
ev4_state.pal_base = ev5_palbase;
ev5_excaddr = ev4_state.exc_addr;
PCQ_ENTRY;
PC = ev4_state.pal_base + off;
if (!pal_mode)
    ev4_use_shadow ();
pal_mode = 1;
ev4_state.pal_mode = 1;
return SCPE_OK;
}

static t_stat ev4_call_palent (t_uint64 fpc, uint32 fnc)
{
uint32 off = ((fnc & 0x3F) << 6) |
    ((fnc & 0x80) ? EV4_PAL_OFF_CALLUNPR : EV4_PAL_OFF_CALLPR);

if (!(fnc & 0x80) && (itlb_cm != MODE_K))
    ABORT (EXC_RSVI);
return ev4_palent (fpc, off);
}

t_stat ev4_rei (void)
{
uint32 new_pal = ((uint32) ev4_state.exc_addr) & 1;

PCQ_ENTRY;
PC = ev4_state.exc_addr;
if (pal_mode && !new_pal)
    ev4_use_main ();
pal_mode = new_pal;
ev4_state.pal_mode = new_pal;
return SCPE_OK;
}

void ev4_sync_from_simh (void)
{
ev4_state.pc = PC;
ev4_state.pc_align = pc_align;
ev4_state.pal_mode = pal_mode;
ev4_state.itlb_cm = itlb_cm;
ev4_state.dtlb_cm = dtlb_cm;
ev4_state.ipl = ev5_ipl;
ev4_state.fpen = fpen;
ev4_state.pcc_h = pcc_h;
ev4_state.pcc_l = pcc_l;
ev4_state.lock_flag = lock_flag;
}

void ev4_sync_to_simh (void)
{
PC = ev4_state.pc;
pc_align = ev4_state.pc_align;
pal_mode = ev4_state.pal_mode;
itlb_cm = ev4_state.itlb_cm;
dtlb_cm = ev4_state.dtlb_cm;
ev5_ipl = ev4_state.ipl;
fpen = ev4_state.fpen;
pcc_h = ev4_state.pcc_h;
pcc_l = ev4_state.pcc_l;
lock_flag = ev4_state.lock_flag;
}

t_stat pal_proc_intr (uint32 lvl)
{
if (cpu_model == ALPHA_MODEL_MIKASA_4_266) {
    ev4_sync_from_simh ();
    sim_debug (MIKASA_DBG_PAL, &mikasa_dev,
        "EV4PAL dispatch intr lvl=%u pc=%016llX\n",
        lvl, (unsigned long long) PC);
    ev4_pal_state.hirr = lvl;
    ev4_state.hier = (uint32) ev4_pal_state.hier;
    t_stat r = mikasa_pal_proc_intr (lvl);

    if (r != SCPE_NOFNC)
        return r;
    return ev4_palent (PC, EV4_PAL_OFF_INTR);
    }
return ev5_pal_proc_intr (lvl);
}

t_stat pal_proc_trap (uint32 summ)
{
if (cpu_model == ALPHA_MODEL_MIKASA_4_266) {
    ev4_sync_from_simh ();
    sim_debug (MIKASA_DBG_PAL, &mikasa_dev,
        "EV4PAL dispatch trap summ=%u pc=%016llX\n",
        summ, (unsigned long long) PC);
    t_stat r = mikasa_pal_proc_trap (summ);

    if (r != SCPE_NOFNC)
        return r;
    return ev4_unhandled_event ("trap", summ);
    }
return ev5_pal_proc_trap (summ);
}

t_stat pal_proc_excp (uint32 abval)
{
if (cpu_model == ALPHA_MODEL_MIKASA_4_266) {
    ev4_sync_from_simh ();
    sim_debug (MIKASA_DBG_PAL, &mikasa_dev,
        "EV4PAL dispatch excp abval=%u pc=%016llX\n",
        abval, (unsigned long long) PC);
    t_stat r = mikasa_pal_proc_excp (abval);

    if (r != SCPE_NOFNC)
        return r;
    return ev4_unhandled_event ("excp", abval);
    }
return ev5_pal_proc_excp (abval);
}

t_stat pal_proc_inst (uint32 fnc)
{
if (cpu_model == ALPHA_MODEL_MIKASA_4_266) {
    ev4_sync_from_simh ();
    sim_debug (MIKASA_DBG_PAL, &mikasa_dev,
        "EV4PAL dispatch inst fnc=%u pc=%016llX\n",
        fnc, (unsigned long long) PC);
    t_stat r = mikasa_pal_proc_inst (fnc);

    if (r != SCPE_NOFNC)
        return r;
    return ev4_call_palent (PC, fnc);
    }
return ev5_pal_proc_inst (fnc);
}

t_stat ev4_pal_proc_mfpr (uint32 ir)
{
return ev4_pal_mfpr (ir);
}

t_stat ev4_pal_proc_mtpr (uint32 ir)
{
return ev4_pal_mtpr (ir);
}
