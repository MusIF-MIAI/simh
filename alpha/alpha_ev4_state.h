#ifndef ALPHA_EV4_STATE_H
#define ALPHA_EV4_STATE_H 1

#include "alpha_defs.h"

typedef struct {
    uint32 ps_sw;
    uint32 hier;
    uint32 hier_cre;
    uint32 hier_pc0;
    uint32 hier_pc1;
    uint32 hier_sle;
    uint32 hirr_crr;
    uint32 hirr_pc0;
    uint32 hirr_pc1;
    uint32 hirr_slr;
    uint32 sirr;
    uint32 aster;
    uint32 sier;
    t_uint64 sl_rcv;
    t_uint64 sl_clr;
    t_uint64 sl_xmit;
    t_uint64 exc_addr;
    t_uint64 pal_base;
    t_uint64 pc;
    uint32 pc_align;
    uint32 pal_mode;
    uint32 itlb_cm;
    uint32 dtlb_cm;
    uint32 ipl;
    uint32 fpen;
    uint32 pcc_h;
    uint32 pcc_l;
    uint32 lock_flag;
    uint32 shadow_active;
    t_uint64 ibox_ipr_raw[32];
    t_uint64 abox_ipr_raw[32];
    t_uint64 paltemp_raw[32];
} EV4_SHARED_STATE;

extern EV4_SHARED_STATE ev4_state;

void ev4_sync_from_simh (void);
void ev4_sync_to_simh (void);
t_stat ev4_rei (void);

#endif /* ALPHA_EV4_STATE_H */
