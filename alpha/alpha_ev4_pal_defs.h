/* alpha_ev4_pal_defs.h - EV4/Mikasa PAL constants

   Source-aligned constants derived from:
   - mikasa/dc21064.h
   - mikasa/osf.h
   - mikasa/cserve.h
*/

#ifndef ALPHA_EV4_PAL_DEFS_H
#define ALPHA_EV4_PAL_DEFS_H 1

/* EV4 PAL vector offsets */
#define EV4_PAL_OFF_RESET      0x0000
#define EV4_PAL_OFF_MCHK       0x0020
#define EV4_PAL_OFF_ARITH      0x0060
#define EV4_PAL_OFF_INTR       0x00E0
#define EV4_PAL_OFF_DFAULT     0x01E0
#define EV4_PAL_OFF_ITBM       0x03E0
#define EV4_PAL_OFF_IACV       0x07E0
#define EV4_PAL_OFF_DTBM       0x08E0
#define EV4_PAL_OFF_DTBM_PAL   0x09E0
#define EV4_PAL_OFF_UNALIGN    0x11E0
#define EV4_PAL_OFF_OPCDEC     0x13E0
#define EV4_PAL_OFF_FEN        0x17E0
#define EV4_PAL_OFF_CALLPR     0x2000
#define EV4_PAL_OFF_CALLUNPR   0x3000

/* IPR group masks/index shape */
#define EV4_IPR_V_PAL          7
#define EV4_IPR_M_PAL          (1u << EV4_IPR_V_PAL)
#define EV4_IPR_V_ABX          6
#define EV4_IPR_M_ABX          (1u << EV4_IPR_V_ABX)
#define EV4_IPR_V_IBX          5
#define EV4_IPR_M_IBX          (1u << EV4_IPR_V_IBX)
#define EV4_IPR_V_INDEX        0
#define EV4_IPR_M_INDEX        (0x1Fu << EV4_IPR_V_INDEX)

/* Key EV4 Ibox IPRs */
#define EV4_IPR_ITB_TAG        (EV4_IPR_M_IBX + 0x0)
#define EV4_IPR_ITB_PTE        (EV4_IPR_M_IBX + 0x1)
#define EV4_IPR_ICCSR          (EV4_IPR_M_IBX + 0x2)
#define EV4_IPR_ITB_PTE_TEMP   (EV4_IPR_M_IBX + 0x3)
#define EV4_IPR_EXC_ADDR       (EV4_IPR_M_IBX + 0x4)
#define EV4_IPR_PS             (EV4_IPR_M_IBX + 0x9)
#define EV4_IPR_EXC_SUM        (EV4_IPR_M_IBX + 0xA)
#define EV4_IPR_PAL_BASE       (EV4_IPR_M_IBX + 0xB)
#define EV4_IPR_HIRR           (EV4_IPR_M_IBX + 0xC)
#define EV4_IPR_SIRR           (EV4_IPR_M_IBX + 0xD)
#define EV4_IPR_ASTRR          (EV4_IPR_M_IBX + 0xE)
#define EV4_IPR_HIER           (EV4_IPR_M_IBX + 0x10)
#define EV4_IPR_SIER           (EV4_IPR_M_IBX + 0x11)
#define EV4_IPR_ASTER          (EV4_IPR_M_IBX + 0x12)

/* OSF/1 PS layout */
#define EV4_OSF_PS_V_CM        3
#define EV4_OSF_PS_M_CM        (1u << EV4_OSF_PS_V_CM)
#define EV4_OSF_PS_V_IPL       0
#define EV4_OSF_PS_M_IPL       (7u << EV4_OSF_PS_V_IPL)

/* OSF/1 entInt constants */
#define EV4_OSF_INT_K_IP       0x0
#define EV4_OSF_INT_K_CLK      0x1
#define EV4_OSF_INT_K_MCHK     0x2
#define EV4_OSF_INT_K_DEV      0x3
#define EV4_OSF_INT_K_PERF     0x4

/* OSF/1 PTE format */
#define EV4_OSF_PTE_V_PFN      32
#define EV4_OSF_PTE_V_SW       16
#define EV4_OSF_PTE_V_UWE      13
#define EV4_OSF_PTE_M_UWE      (1u << EV4_OSF_PTE_V_UWE)
#define EV4_OSF_PTE_V_KWE      12
#define EV4_OSF_PTE_M_KWE      (1u << EV4_OSF_PTE_V_KWE)
#define EV4_OSF_PTE_V_URE      9
#define EV4_OSF_PTE_M_URE      (1u << EV4_OSF_PTE_V_URE)
#define EV4_OSF_PTE_V_KRE      8
#define EV4_OSF_PTE_M_KRE      (1u << EV4_OSF_PTE_V_KRE)
#define EV4_OSF_PTE_V_GH       5
#define EV4_OSF_PTE_M_GH       (3u << EV4_OSF_PTE_V_GH)
#define EV4_OSF_PTE_V_ASM      4
#define EV4_OSF_PTE_M_ASM      (1u << EV4_OSF_PTE_V_ASM)
#define EV4_OSF_PTE_V_FOE      3
#define EV4_OSF_PTE_M_FOE      (1u << EV4_OSF_PTE_V_FOE)
#define EV4_OSF_PTE_V_FOW      2
#define EV4_OSF_PTE_M_FOW      (1u << EV4_OSF_PTE_V_FOW)
#define EV4_OSF_PTE_V_FOR      1
#define EV4_OSF_PTE_M_FOR      (1u << EV4_OSF_PTE_V_FOR)
#define EV4_OSF_PTE_V_VALID    0
#define EV4_OSF_PTE_M_VALID    (1u << EV4_OSF_PTE_V_VALID)

/* OSF/1 PCB offsets */
#define EV4_OSF_PCB_Q_KSP      0x0000
#define EV4_OSF_PCB_Q_USP      0x0008
#define EV4_OSF_PCB_Q_PTBR     0x0010
#define EV4_OSF_PCB_L_PCC      0x0018
#define EV4_OSF_PCB_L_ASN      0x001C
#define EV4_OSF_PCB_Q_UNIQUE   0x0020
#define EV4_OSF_PCB_Q_FEN      0x0028

/* OSF/1 frame offsets */
#define EV4_OSF_FRM_Q_PS       0x0000
#define EV4_OSF_FRM_Q_PC       0x0008
#define EV4_OSF_FRM_Q_GP       0x0010
#define EV4_OSF_FRM_Q_A0       0x0018
#define EV4_OSF_FRM_Q_A1       0x0020
#define EV4_OSF_FRM_Q_A2       0x0028
#define EV4_OSF_FRM_K_SIZE     48

/* MCES */
#define EV4_OSF_MCES_V_MIP     0
#define EV4_OSF_MCES_M_MIP     (1u << EV4_OSF_MCES_V_MIP)
#define EV4_OSF_MCES_V_SCE     1
#define EV4_OSF_MCES_M_SCE     (1u << EV4_OSF_MCES_V_SCE)
#define EV4_OSF_MCES_V_PCE     2
#define EV4_OSF_MCES_M_PCE     (1u << EV4_OSF_MCES_V_PCE)
#define EV4_OSF_MCES_V_DPC     3
#define EV4_OSF_MCES_M_DPC     (1u << EV4_OSF_MCES_V_DPC)
#define EV4_OSF_MCES_V_DSC     4
#define EV4_OSF_MCES_M_DSC     (1u << EV4_OSF_MCES_V_DSC)

/* CALL_PAL entries (unprivileged) */
#define EV4_OSF_PAL_BPT_ENTRY       0x80
#define EV4_OSF_PAL_BUGCHK_ENTRY    0x81
#define EV4_OSF_PAL_CALLSYS_ENTRY   0x83
#define EV4_OSF_PAL_IMB_ENTRY       0x86
#define EV4_OSF_PAL_RDUNIQUE_ENTRY  0x9E
#define EV4_OSF_PAL_WRUNIQUE_ENTRY  0x9F
#define EV4_OSF_PAL_GENTRAP_ENTRY   0xAA

/* CALL_PAL entries (privileged) */
#define EV4_OSF_PAL_HALT_ENTRY      0x0000
#define EV4_OSF_PAL_CFLUSH_ENTRY    0x0001
#define EV4_OSF_PAL_DRAINA_ENTRY    0x0002
#define EV4_OSF_PAL_CSERVE_ENTRY    0x0009
#define EV4_OSF_PAL_SWPPAL_ENTRY    0x000A
#define EV4_OSF_PAL_WRIPIR_ENTRY    0x000D
#define EV4_OSF_PAL_RDMCES_ENTRY    0x0010
#define EV4_OSF_PAL_WRMCES_ENTRY    0x0011
#define EV4_OSF_PAL_WRFEN_ENTRY     0x002B
#define EV4_OSF_PAL_WRVPTPTR_ENTRY  0x002D
#define EV4_OSF_PAL_SWPCTX_ENTRY    0x0030
#define EV4_OSF_PAL_WRVAL_ENTRY     0x0031
#define EV4_OSF_PAL_RDVAL_ENTRY     0x0032
#define EV4_OSF_PAL_TBI_ENTRY       0x0033
#define EV4_OSF_PAL_WRENT_ENTRY     0x0034
#define EV4_OSF_PAL_SWPIPL_ENTRY    0x0035
#define EV4_OSF_PAL_RDPS_ENTRY      0x0036
#define EV4_OSF_PAL_WRKGP_ENTRY     0x0037
#define EV4_OSF_PAL_WRUSP_ENTRY     0x0038
#define EV4_OSF_PAL_WRPERFMON_ENTRY 0x0039
#define EV4_OSF_PAL_RDUSP_ENTRY     0x003A
#define EV4_OSF_PAL_WHAMI_ENTRY     0x003C
#define EV4_OSF_PAL_RETSYS_ENTRY    0x003D
#define EV4_OSF_PAL_RTI_ENTRY       0x003F

/* CSERVE subfunction codes */
#define EV4_CSERVE_K_LDQP      0x01
#define EV4_CSERVE_K_STQP      0x02
#define EV4_CSERVE_K_RD_ABOX   0x03
#define EV4_CSERVE_K_RD_BIU    0x04
#define EV4_CSERVE_K_RD_ICCSR  0x05
#define EV4_CSERVE_K_WR_ABOX   0x06
#define EV4_CSERVE_K_WR_BIU    0x07
#define EV4_CSERVE_K_WR_ICCSR  0x08
#define EV4_CSERVE_K_JTOPAL    0x09
#define EV4_CSERVE_K_WR_INT    0x0A
#define EV4_CSERVE_K_RD_IMPURE 0x0B
#define EV4_CSERVE_K_PUTC      0x0F

#endif /* ALPHA_EV4_PAL_DEFS_H */
