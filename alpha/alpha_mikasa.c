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
#include "alpha_ev4_pal_defs.h"
#include "alpha_ev4_state.h"
#include "sim_fio.h"
#include "sim_scsi.h"
#include "sim_timer.h"
#include "mikasa/dc21064.h"
#include "mikasa/impure.h"
#include "mikasa/platform.h"

/*
 * The DECchip header uses many legacy identifiers as macros that collide with
 * local identifiers (for example `va`, `ps`, `hier`, and interrupt state names).
 * They are intentionally undefined here so EV4 state fields and local variables can
 * keep readable names.
 */
#ifdef va
#undef va
#endif
#ifdef ps
#undef ps
#endif
#ifdef hier
#undef hier
#endif
#ifdef tbTag
#undef tbTag
#endif
#ifdef itbPte
#undef itbPte
#endif
#ifdef itbPteTemp
#undef itbPteTemp
#endif
#ifdef excAddr
#undef excAddr
#endif
#ifdef slRcv
#undef slRcv
#endif
#ifdef itbZap
#undef itbZap
#endif
#ifdef itbAsm
#undef itbAsm
#endif
#ifdef itbIs
#undef itbIs
#endif
#ifdef excSum
#undef excSum
#endif
#ifdef palBase
#undef palBase
#endif
#ifdef hirr
#undef hirr
#endif
#ifdef sirr
#undef sirr
#endif
#ifdef astrr
#undef astrr
#endif
#ifdef sier
#undef sier
#endif
#ifdef aster
#undef aster
#endif
#ifdef slClr
#undef slClr
#endif
#ifdef slXmit
#undef slXmit
#endif
#ifdef dtbCtl
#undef dtbCtl
#endif
#ifdef tbCtl
#undef tbCtl
#endif
#ifdef dtbPte
#undef dtbPte
#endif
#ifdef dtbPteTemp
#undef dtbPteTemp
#endif
#ifdef mmcsr
#undef mmcsr
#endif
#ifdef dtbZap
#undef dtbZap
#endif
#ifdef dtbAsm
#undef dtbAsm
#endif
#ifdef dtbIs
#undef dtbIs
#endif
#ifdef biuAddr
#undef biuAddr
#endif
#ifdef biuStat
#undef biuStat
#endif
#ifdef dcAddr
#undef dcAddr
#endif
#ifdef dcStat
#undef dcStat
#endif
#ifdef fillAddr
#undef fillAddr
#endif
#ifdef aboxCtl
#undef aboxCtl
#endif
#ifdef altMode
#undef altMode
#endif
#ifdef cc
#undef cc
#endif
#ifdef ccCtl
#undef ccCtl
#endif
#ifdef biuCtl
#undef biuCtl
#endif
#ifdef fillSyndrome
#undef fillSyndrome
#endif
#ifdef bcTag
#undef bcTag
#endif
#ifdef flushIc
#undef flushIc
#endif
#ifdef flushIcAsm
#undef flushIcAsm
#endif
#ifdef xtbZap
#undef xtbZap
#endif
#ifdef xtbAsm
#undef xtbAsm
#endif

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
#define MIKASA_IMPURE_PA            (MIKASA_HWRPB_PA + 0x7000)
#define MIKASA_PAGE_SIZE            8192
#define MIKASA_DKA_UNITS            4
#define MIKASA_DKA_BLOCK_SIZE       512
#define MIKASA_IO_BUFSIZE           4096
#define MIKASA_NCR_SCSI_MAXFR       (1u << 16)
#define MIKASA_NCR_SCSI_BUFSIZE     256
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
#define MIKASA_CPU_INFO_QW_PA       0x00004080
#define MIKASA_CPU_TYPE             2
#define MIKASA_CPU_VERSION          3
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
#define MIKASA_PAL_STACK_FRAME      EV4_OSF_FRM_K_SIZE
#define MIKASA_PCB_KSP              0x00
#define MIKASA_PCB_USP              0x08
#define MIKASA_PCB_PTBR             0x10
#define MIKASA_PCB_PCC              0x18
#define MIKASA_PCB_ASN              0x1C
#define MIKASA_PCB_UNIQUE           0x20
#define MIKASA_PCB_FEN              0x28
#define MIKASA_PCBV_FLAGS           MIKASA_PCB_FEN
#define MIKASA_PCB_THREAD           MIKASA_PCB_UNIQUE
#define MIKASA_PCB_MIN_SIZE         0x40
#define MIKASA_PROC_HALT_PCBB       0x0E8
#define MIKASA_PROC_HALT_CODE       0x118
#define MIKASA_CFLUSH_LINES_PER_ITER 8
#define MIKASA_CFLUSH_INDEX_MASK    ((((t_uint64) 1u) << (BC_V_SIZE + 1)) - 1)
#define MIKASA_CNS_R0               0x010
#define MIKASA_CNS_R30              0x100
#define MIKASA_CNS_PRBR             0x230
#define MIKASA_CNS_SCBB             0x258
#define MIKASA_CNS_PCBB             0x260
#define MIKASA_SRM_BOOT_PTBR        0x2F8
#define MIKASA_SRM_BOOT_PC          0x318
#define MIKASA_SRM_BOOT_PALBASE     0x320
#define MIKASA_SCB_FDIS             0x010
#define MIKASA_SCB_ACV              0x080
#define MIKASA_SCB_TNV              0x090
#define MIKASA_SCB_FOR              0x0A0
#define MIKASA_SCB_FOW              0x0B0
#define MIKASA_SCB_FOE              0x0C0
#define MIKASA_SCB_ARITH            0x200
#define MIKASA_SCB_KAST             0x240
#define MIKASA_SCB_ALIGN            0x280
#define MIKASA_SCB_BPT              0x400
#define MIKASA_SCB_BUG              0x410
#define MIKASA_SCB_RSVI             0x420
#define MIKASA_SCB_RSVO             0x430
#define MIKASA_SCB_GENTRAP          0x440
#define MIKASA_SCB_CHMK             0x480
#define MIKASA_SCB_CHME             0x490
#define MIKASA_SCB_CHMS             0x4A0
#define MIKASA_SCB_CHMU             0x4B0
#define MIKASA_SCB_SISR0            0x500
#define MIKASA_SCB_TIMER            0x600
#define MIKASA_SCB_PASVR            0x6F0
#define MIKASA_SCB_IO               0x800
#define MIKASA_SCB_MCHK             MIKASA_SCB_PASVR
#define MIKASA_VMS_MME_E            0x0000000000000001
#define MIKASA_VMS_MME_R            0x0000000000000000
#define MIKASA_VMS_MME_W            0x8000000000000000
#define MIKASA_OSF_MM_K_TNV         0
#define MIKASA_OSF_MM_K_ACV         1
#define MIKASA_OSF_MM_K_FOR         2
#define MIKASA_OSF_MM_K_FOE         3
#define MIKASA_OSF_MM_K_FOW         4
#define MIKASA_OSF_IF_K_BPT         0
#define MIKASA_OSF_IF_K_BUGCHK      1
#define MIKASA_OSF_IF_K_GENTRAP     2
#define MIKASA_OSF_IF_K_FEN         3
#define MIKASA_OSF_IF_K_OPCDEC      4
#define HLT_K_SW_HALT               0x0005
#define HLT_K_KSP_INVAL             0x0002
#define HLT_K_DBL_MCHK              0x0006
#define MIKASA_INT_K_IP             0x0
#define MIKASA_INT_K_CLK            0x1
#define MIKASA_INT_K_MCHK           0x2
#define MIKASA_INT_K_DEV            0x3
#define MIKASA_INT_K_PERF           0x4
#define MIKASA_IPL_K_DEV            0x3
#define MIKASA_IPL_K_CLK            0x5
#define MIKASA_IPL_K_PERF           0x6
#define MIKASA_IPL_K_MCHK           0x7
#define MIKASA_SCB_PERF             0x0650
#define MIKASA_SCB_Q_SYSERR         0x0620
#define MIKASA_SCB_Q_PROCERR        0x0630
#define MIKASA_SCB_Q_SYSMCHK        0x0660
#define MIKASA_SCB_Q_PROCMCHK       0x0670
#define MIKASA_MCHK_K_UNKNOWN       0x008A
#define MIKASA_MCHK_K_BUGCHECK      0x008E
#define MIKASA_MCHK_K_OS_BUGCHECK   0x0090
#define MIKASA_MCHK_K_ICPERR        0x0094
#define MIKASA_MCHK_K_REV           0x0001
#define MIKASA_MCHK_K_ECC_C         0x0086
#define MIKASA_MCHK_K_CACKSOFT      0x008C
#define MIKASA_MCHK_K_SYSTEM        0x0200
#define MIKASA_MCHK_K_SIO_SERR      0x0204
#define MIKASA_MCHK_K_SIO_IOCHK     0x0206
#define MIKASA_MCHK_K_DCSR          0x0208
#define MIKASA_KSEG_BASE            0xFFFFFC0000000000ULL
#define MIKASA_PAL_AST_MASK         0x0F
#define MIKASA_PAL_SISR_MASK        0xFFFE
#define MIKASA_PAL_PTBR_PHYS        0x1
#define MIKASA_PAL_PHYS_PTE         (PTE_KWE | PTE_EWE | PTE_KRE | \
                                      PTE_ERE | PTE_GH | PTE_ASM | PTE_V)
#define MIKASA_PSV_V_SPA            56
#define MIKASA_PSV_M_SPA            0x3F
#define MIKASA_PSV_V_IPL            8
#define MIKASA_PSV_M_IPL            0x1F
#define MIKASA_PSV_V_VMM            7
#define MIKASA_PSV_V_CM             3
#define MIKASA_PSV_M_CM             0x3
#define MIKASA_PSV_V_IP             2
#define MIKASA_PSV_V_SW             0
#define MIKASA_PSV_M_SW             0x3
#define MIKASA_PSV_VMM              (1u << MIKASA_PSV_V_VMM)
#define MIKASA_PSV_IP               (1u << MIKASA_PSV_V_IP)
#define MIKASA_PSV_MASK             (MIKASA_PSV_VMM | MIKASA_PSV_IP | \
                                      MIKASA_PSV_M_SW)
#define MIKASA_PSV_MBZ              0xC0FFFFFFFFFFE0E4ULL
#define MIKASA_OSF_PSV_M_IPL        0x7
#define MIKASA_OSF_PSV_M_CM         (1u << MIKASA_PSV_V_CM)
#define MIKASA_WHAMI_V_SWAP         32
#define MIKASA_WHAMI_M_ID           0xFFULL
#define MIKASA_WHAMI_K_SWAP         0x53574150ULL
#define MIKASA_COMANCHE_BASE        0x180000000ULL
#define MIKASA_COMANCHE_SIZE        0x00001000
#define MIKASA_COMANCHE_GCR         0x000
#define MIKASA_COMANCHE_ED          0x040
#define MIKASA_COMANCHE_TAGENB      0x060
#define MIKASA_COMANCHE_ERR_LO      0x080
#define MIKASA_COMANCHE_ERR_HI      0x0A0
#define MIKASA_COMANCHE_LCK_LO      0x0C0
#define MIKASA_COMANCHE_LCK_HI      0x0E0
#define MIKASA_COMANCHE_GTIM        0x200
#define MIKASA_COMANCHE_RTIM        0x220
#define MIKASA_COMANCHE_VFP         0x240
#define MIKASA_COMANCHE_PD_LO       0x260
#define MIKASA_COMANCHE_PD_HI       0x280
#define MIKASA_COMANCHE_B0_BAR      0x800
#define MIKASA_COMANCHE_B0_CR       0xA00
#define MIKASA_COMANCHE_B0_TRA      0xC00
#define MIKASA_COMANCHE_B0_TRB      0xE00
#define MIKASA_COMANCHE_BANKS       9
#define MIKASA_COMANCHE_ED_ERR      0x000001FFu
#define MIKASA_COMANCHE_ED_PASS2    0x00002000u
#define MIKASA_COMANCHE_ED_STICKY   (MIKASA_COMANCHE_ED_ERR | \
                                      MIKASA_COMANCHE_ED_PASS2)
#define MIKASA_COMANCHE_ADDR_HI     0x00001FFFu
#define MIKASA_COMANCHE_GCR_WIDEMEM 0x00000010u
#define MIKASA_COMANCHE_GCR_MASK    0x00003FF6u
#define MIKASA_COMANCHE_TAGENB_MASK 0x0000FFFEu
#define MIKASA_COMANCHE_VFP_MASK    0x00007FFFu
#define MIKASA_COMANCHE_BAR_MASK    0x0000FFE0u
#define MIKASA_COMANCHE_CR_S0_MASK  0x000001FFu
#define MIKASA_COMANCHE_CR_S8_MASK  0x000003FFu
#define MIKASA_COMANCHE_TRA_MASK    0x00007FFFu
#define MIKASA_COMANCHE_TRB_MASK    0x00003FFFu
#define MIKASA_COMANCHE_CR_VALID    0x00000001u
#define MIKASA_COMANCHE_CR_SIZE_1G  0x0000001Eu
#define MIKASA_EPIC_BASE            0x1A0000000ULL
#define MIKASA_EPIC_SIZE            0x00001000
#define MIKASA_APECS_PCI_IACK       0x1B0000000ULL
#define MIKASA_APECS_PCI_SIO        0x1C0000000ULL
#define MIKASA_APECS_PCI_SIO_SIZE   0x02000000
#define MIKASA_APECS_PCI_CONF       0x1E0000000ULL
#define MIKASA_APECS_PCI_CONF_SIZE  0x20000000
#define MIKASA_APECS_PCI_SPARSE     0x200000000ULL
#define MIKASA_APECS_PCI_SPARSE_SIZE 0x100000000ULL
#define MIKASA_APECS_PCI_DENSE      0x300000000ULL
#define MIKASA_APECS_PCI_DENSE_SIZE 0x100000000ULL
#define MIKASA_EPIC_DCSR            0x000
#define MIKASA_EPIC_PEAR            0x020
#define MIKASA_EPIC_SEAR            0x040
#define MIKASA_EPIC_DCSR_TENB       0x00000001u
#define MIKASA_EPIC_DCSR_PENB       0x00000004u
#define MIKASA_EPIC_DCSR_DCEI       0x00000008u
#define MIKASA_EPIC_DCSR_DPEC       0x00000010u
#define MIKASA_EPIC_DCSR_IORT       0x00000020u
#define MIKASA_EPIC_DCSR_LOST       0x00000040u
#define MIKASA_EPIC_DCSR_DDPE       0x00000100u
#define MIKASA_EPIC_DCSR_IOPE       0x00000200u
#define MIKASA_EPIC_DCSR_TABT       0x00000400u
#define MIKASA_EPIC_DCSR_NDEV       0x00000800u
#define MIKASA_EPIC_DCSR_CMRD       0x00001000u
#define MIKASA_EPIC_DCSR_UMRD       0x00002000u
#define MIKASA_EPIC_DCSR_IPTL       0x00004000u
#define MIKASA_EPIC_DCSR_MERR       0x00008000u
#define MIKASA_EPIC_DCSR_DBYP       0x00030000u
#define MIKASA_EPIC_DCSR_PCMD       0x003C0000u
#define MIKASA_EPIC_DCSR_CTL        (MIKASA_EPIC_DCSR_TENB | \
                                      MIKASA_EPIC_DCSR_PENB | \
                                      MIKASA_EPIC_DCSR_DCEI | \
                                      MIKASA_EPIC_DCSR_IORT | \
                                      MIKASA_EPIC_DCSR_DBYP | \
                                      MIKASA_EPIC_DCSR_PCMD)
#define MIKASA_EPIC_DCSR_ERR        (MIKASA_EPIC_DCSR_DPEC | \
                                      MIKASA_EPIC_DCSR_LOST | \
                                      MIKASA_EPIC_DCSR_DDPE | \
                                      MIKASA_EPIC_DCSR_IOPE | \
                                      MIKASA_EPIC_DCSR_TABT | \
                                      MIKASA_EPIC_DCSR_NDEV | \
                                      MIKASA_EPIC_DCSR_CMRD | \
                                      MIKASA_EPIC_DCSR_UMRD | \
                                      MIKASA_EPIC_DCSR_IPTL | \
                                      MIKASA_EPIC_DCSR_MERR)
#define MIKASA_EPIC_SEAR_ERR        0xFFFFFFF0u
#define MIKASA_EPIC_TBASE_1         0x0C0
#define MIKASA_EPIC_TBASE_2         0x0E0
#define MIKASA_EPIC_TBASE_MASK      0xFFFFFE00u
#define MIKASA_EPIC_TBASE_SHIFT     1
#define MIKASA_EPIC_PCI_BASE_1      0x100
#define MIKASA_EPIC_PCI_BASE_2      0x120
#define MIKASA_EPIC_PCI_BASE_SGEN   0x00040000u
#define MIKASA_EPIC_PCI_BASE_WENB   0x00080000u
#define MIKASA_EPIC_PCI_BASE_MASK   0xFFF00000u
#define MIKASA_EPIC_PCI_MASK_1      0x140
#define MIKASA_EPIC_PCI_MASK_2      0x160
#define MIKASA_EPIC_PCI_MASK_MASK   0xFFF00000u
#define MIKASA_EPIC_HAXR0           0x180
#define MIKASA_EPIC_HAXR1           0x1A0
#define MIKASA_EPIC_HAXR1_EADDR     0xF8000000u
#define MIKASA_EPIC_HAXR2           0x1C0
#define MIKASA_EPIC_HAXR2_CONF_TYPE 0x00000003u
#define MIKASA_EPIC_HAXR2_EADDR     0xFF000000u
#define MIKASA_EPIC_PMLT            0x1E0
#define MIKASA_EPIC_PMLT_PMLC       0x000000FFu
#define MIKASA_EPIC_TLB_TAG_0       0x200
#define MIKASA_EPIC_TLB_DATA_0      0x300
#define MIKASA_EPIC_TLB_ENTRIES     8
#define MIKASA_EPIC_TLB_TAG_EVAL    0x00001000u
#define MIKASA_EPIC_TLB_TAG_PAGE    0xFFFFE000u
#define MIKASA_EPIC_TLB_TAG_MASK    (MIKASA_EPIC_TLB_TAG_EVAL | \
                                      MIKASA_EPIC_TLB_TAG_PAGE)
#define MIKASA_EPIC_TLB_DATA_PAGE   0x001FFFFEu
#define MIKASA_EPIC_TBIA            0x400
#define MIKASA_EPIC_SGMAP_EVAL      0x0000000000000001ULL
#define MIKASA_EPIC_SGMAP_PFN       0x00000000001FFFFEULL
#define MIKASA_EPIC_DCSR_PASS2      0x80000000u
/* On-board devices use raw APECS IDSEL slots; SRM prints virtual slots. */
#define MIKASA_PCI_SLOT_VGA         2
#define MIKASA_PCI_SLOT_SCSI        6
/* SRM reports the EISA bridge as virtual slot 2; config cycles use IDSEL 7. */
#define MIKASA_PCI_SLOT_PCEB        7
#define MIKASA_PCI_SLOT_TULIP       11
#define MIKASA_PCI_CFG_DWORDS       64
#define MIKASA_VGA_LEGACY_BASE      0x000A0000u
#define MIKASA_VGA_LEGACY_SIZE      0x00020000u
#define MIKASA_VGA_ROM_BASE         0x000C0000u
#define MIKASA_VGA_ROM_SIZE         0x00010000u
#define MIKASA_VGA_FB_MASK          0xFC000000u
#define MIKASA_VGA_ROM_MASK         0xFFFF0000u
#define MIKASA_APECS_HAE_REG1       0x01000000u
#define MIKASA_TULIP_REG_SIZE       0x80
#define MIKASA_TULIP_BAR_SIZE       0x80
#define MIKASA_TULIP_BAR_MASK       0xFFFFFF80u
#define MIKASA_TULIP_CSR_BUSMODE    0x00
#define MIKASA_TULIP_CSR_TXPOLL     0x08
#define MIKASA_TULIP_CSR_RXPOLL     0x10
#define MIKASA_TULIP_CSR_RXLIST     0x18
#define MIKASA_TULIP_CSR_TXLIST     0x20
#define MIKASA_TULIP_CSR_STATUS     0x28
#define MIKASA_TULIP_CSR_CMD        0x30
#define MIKASA_TULIP_CSR_INTR       0x38
#define MIKASA_TULIP_CSR_MISSED     0x40
#define MIKASA_TULIP_CSR_SIO        0x48
#define MIKASA_TULIP_CSR_SIA_STATUS 0x60
#define MIKASA_TULIP_CSR_SIA_CONN   0x68
#define MIKASA_TULIP_CSR_SIA_TXRX   0x70
#define MIKASA_TULIP_CSR_SIA_GEN    0x78
#define MIKASA_TULIP_BUSMODE_SWR    0x00000001u
#define MIKASA_TULIP_SIO_EE_CS      0x00000001u
#define MIKASA_TULIP_SIO_EE_CLK     0x00000002u
#define MIKASA_TULIP_SIO_EE_DATAIN  0x00000004u
#define MIKASA_TULIP_SIO_EE_DATAOUT 0x00000008u
#define MIKASA_TULIP_SIO_EESEL      0x00000800u
#define MIKASA_TULIP_SIO_ROM_READ   0x00004000u
#define MIKASA_TULIP_SROM_BYTES     128
#define MIKASA_TULIP_SROM_READ      6
#define MIKASA_TULIP_SROM_ADDR_BITS 6
#define MIKASA_TULIP_ENETROM_BYTES  32
#define MIKASA_TULIP_DESC_OWNER     0x80000000u
#define MIKASA_TULIP_DFLAG_ENDRING  0x0008u
#define MIKASA_TULIP_DFLAG_CHAIN    0x0004u
#define MIKASA_TULIP_DFLAG_TXINTR   0x0200u
#define MIKASA_TULIP_STS_TXS_MASK   0x00700000u
#define MIKASA_TULIP_STS_TXS_STOP   0x00000000u
#define MIKASA_TULIP_STS_TXS_SUSP   0x00600000u
#define MIKASA_TULIP_STS_RXS_MASK   0x000E0000u
#define MIKASA_TULIP_STS_RXS_STOP   0x00000000u
#define MIKASA_TULIP_STS_RXS_WAIT   0x00060000u
#define MIKASA_TULIP_STS_RXS_SUSP   0x00080000u
#define MIKASA_TULIP_STS_NORMAL     0x00010000u
#define MIKASA_TULIP_STS_ABNORMAL   0x00008000u
#define MIKASA_TULIP_STS_RXNOBUF    0x00000080u
#define MIKASA_TULIP_STS_TXINTR     0x00000001u
#define MIKASA_TULIP_STS_TXNOBUF    0x00000004u
#define MIKASA_TULIP_STS_W1C        0x0001FFFFu
#define MIKASA_TULIP_STS_IRQS       (MIKASA_TULIP_STS_NORMAL | \
                                      MIKASA_TULIP_STS_ABNORMAL | \
                                      MIKASA_TULIP_STS_RXNOBUF | \
                                      MIKASA_TULIP_STS_TXINTR | \
                                      MIKASA_TULIP_STS_TXNOBUF)
#define MIKASA_TULIP_CMD_TXRUN      0x00002000u
#define MIKASA_TULIP_CMD_RXRUN      0x00000002u
#define MIKASA_TULIP_IRQ            11
#define MIKASA_TULIP_INTBIT         (1u << MIKASA_TULIP_IRQ)
#define MIKASA_NCR_REG_SIZE         0x100
#define MIKASA_NCR_BAR_SIZE         0x100
#define MIKASA_NCR_BAR_MASK         0xFFFFFF00u
#define MIKASA_NCR_REG_SCNTL0       0x00
#define MIKASA_NCR_REG_SCNTL1       0x01
#define MIKASA_NCR_REG_SCNTL2       0x02
#define MIKASA_NCR_REG_SCNTL3       0x03
#define MIKASA_NCR_REG_SCID         0x04
#define MIKASA_NCR_REG_SXFER        0x05
#define MIKASA_NCR_REG_SDID         0x06
#define MIKASA_NCR_REG_GPREG        0x07
#define MIKASA_NCR_REG_SFBR         0x08
#define MIKASA_NCR_REG_SOCL         0x09
#define MIKASA_NCR_REG_SSID         0x0A
#define MIKASA_NCR_REG_SBCL         0x0B
#define MIKASA_NCR_REG_DSTAT        0x0C
#define MIKASA_NCR_REG_SSTAT0       0x0D
#define MIKASA_NCR_REG_SSTAT1       0x0E
#define MIKASA_NCR_REG_SSTAT2       0x0F
#define MIKASA_NCR_REG_DSA          0x10
#define MIKASA_NCR_REG_ISTAT        0x14
#define MIKASA_NCR_REG_DBC          0x24
#define MIKASA_NCR_REG_DNAD         0x28
#define MIKASA_NCR_REG_DSP          0x2C
#define MIKASA_NCR_REG_DSPS         0x30
#define MIKASA_NCR_REG_DMODE        0x38
#define MIKASA_NCR_REG_DIEN         0x39
#define MIKASA_NCR_REG_DCNTL        0x3B
#define MIKASA_NCR_REG_ADDER        0x3C
#define MIKASA_NCR_REG_SIEN0        0x40
#define MIKASA_NCR_REG_SIEN1        0x41
#define MIKASA_NCR_REG_SIST0        0x42
#define MIKASA_NCR_REG_SIST1        0x43
#define MIKASA_NCR_REG_CTEST1       0x19
#define MIKASA_NCR_REG_CTEST2       0x1A
#define MIKASA_NCR_REG_CTEST3       0x1B
#define MIKASA_NCR_REG_TEMP         0x1C
#define MIKASA_NCR_REG_DFIFO        0x20
#define MIKASA_NCR_REG_CTEST4       0x21
#define MIKASA_NCR_REG_CTEST5       0x22
#define MIKASA_NCR_REG_CTEST6       0x23
#define MIKASA_NCR_REG_SCRATCHA     0x34
#define MIKASA_NCR_REG_MACNTL       0x46
#define MIKASA_NCR_REG_GPCNTL       0x47
#define MIKASA_NCR_REG_STIME0       0x48
#define MIKASA_NCR_REG_STIME1       0x49
#define MIKASA_NCR_REG_RESPID       0x4A
#define MIKASA_NCR_REG_STEST0       0x4C
#define MIKASA_NCR_REG_STEST1       0x4D
#define MIKASA_NCR_REG_STEST2       0x4E
#define MIKASA_NCR_REG_STEST3       0x4F
#define MIKASA_NCR_REG_SIDL         0x50
#define MIKASA_NCR_REG_STEST4       0x52
#define MIKASA_NCR_REG_SODL         0x54
#define MIKASA_NCR_REG_SBDL         0x58
#define MIKASA_NCR_REG_SCRATCHB     0x5C
#define MIKASA_NCR_SCNTL0_ARB       0xC0
#define MIKASA_NCR_SCNTL0_START     0x20
#define MIKASA_NCR_SCNTL0_WATN      0x10
#define MIKASA_NCR_SCNTL0_TRG       0x01
#define MIKASA_NCR_SCNTL0_WRITABLE  0xFB
#define MIKASA_NCR_SCNTL1_ISCON     0x10
#define MIKASA_NCR_SCNTL1_RST       0x08
#define MIKASA_NCR_SCNTL2_SDU       0x80
#define MIKASA_NCR_SCNTL2_W1C       0x09
#define MIKASA_NCR_SCNTL2_WRITABLE  0xF2
#define MIKASA_NCR_SCNTL3_WRITABLE  0x77
#define MIKASA_NCR_SCID_WRITABLE    0x67
#define MIKASA_NCR_SDID_WRITABLE    0x07
#define MIKASA_NCR_GPREG_WRITABLE   0x03
#define MIKASA_NCR_SSID_VAL         0x80
#define MIKASA_NCR_BUS_REQ          0x80
#define MIKASA_NCR_BUS_ACK          0x40
#define MIKASA_NCR_BUS_BSY          0x20
#define MIKASA_NCR_BUS_SEL          0x10
#define MIKASA_NCR_BUS_ATN          0x08
#define MIKASA_NCR_SOCL_ACK         0x40
#define MIKASA_NCR_SOCL_ATN         0x08
#define MIKASA_NCR_DSTAT_DFE        0x80
#define MIKASA_NCR_DSTAT_BF         0x20
#define MIKASA_NCR_DSTAT_SIR        0x04
#define MIKASA_NCR_DSTAT_SSI        0x08
#define MIKASA_NCR_DSTAT_ABRT       0x10
#define MIKASA_NCR_DSTAT_IID        0x01
#define MIKASA_NCR_DSTAT_IRQS       0x7Du
#define MIKASA_NCR_SSTAT0_ILF       0x80
#define MIKASA_NCR_SSTAT0_ORF       0x40
#define MIKASA_NCR_SSTAT0_OLF       0x20
#define MIKASA_NCR_SSTAT0_WOA       0x04
#define MIKASA_NCR_SSTAT0_RST       0x02
#define MIKASA_NCR_SSTAT1_FIFO      0xF0
#define MIKASA_NCR_DMODE_MAN        0x01
#define MIKASA_NCR_DMODE_WRITABLE   0xCF
#define MIKASA_NCR_DCNTL_PFF        0x40
#define MIKASA_NCR_DCNTL_SSM        0x10
#define MIKASA_NCR_DCNTL_STD        0x04
#define MIKASA_NCR_DCNTL_IRQD       0x02
#define MIKASA_NCR_DCNTL_WRITABLE   0xFB
#define MIKASA_NCR_ISTAT_SRST       0x40
#define MIKASA_NCR_ISTAT_SIGP       0x20
#define MIKASA_NCR_ISTAT_ABRT       0x80
#define MIKASA_NCR_ISTAT_SEM        0x10
#define MIKASA_NCR_ISTAT_CON        0x08
#define MIKASA_NCR_ISTAT_INTF       0x04
#define MIKASA_NCR_ISTAT_SIP        0x02
#define MIKASA_NCR_ISTAT_DIP        0x01
#define MIKASA_NCR_SIST0_MIA        0x80
#define MIKASA_NCR_SIST0_CMP        0x40
#define MIKASA_NCR_SIST0_SEL        0x20
#define MIKASA_NCR_SIST0_RSL        0x10
#define MIKASA_NCR_SIST0_SGE        0x08
#define MIKASA_NCR_SIST0_UDC        0x04
#define MIKASA_NCR_SIST0_RST        0x02
#define MIKASA_NCR_SIST0_PAR        0x01
#define MIKASA_NCR_SIST1_STO        0x04
#define MIKASA_NCR_SIEN1_WRITABLE   0x07
#define MIKASA_NCR_SSTAT2_ILF1      0x80
#define MIKASA_NCR_SSTAT2_ORF1      0x40
#define MIKASA_NCR_SSTAT2_OLF1      0x20
#define MIKASA_NCR_SSTAT2_LDSC      0x02
#define MIKASA_NCR_CTEST1_FMT       0xF0
#define MIKASA_NCR_CTEST2_WRITABLE  0x08
#define MIKASA_NCR_CTEST2_SIGP      0x40
#define MIKASA_NCR_CTEST2_DACK      0x01
#define MIKASA_NCR_CTEST3_REV       0x10
#define MIKASA_NCR_CTEST3_FLF       0x08
#define MIKASA_NCR_CTEST3_CLF       0x04
#define MIKASA_NCR_CTEST3_WRITABLE  0x0B
#define MIKASA_NCR_CTEST5_ADCK      0x80
#define MIKASA_NCR_CTEST5_BBCK      0x40
#define MIKASA_NCR_CTEST5_WRITABLE  0x3F
#define MIKASA_NCR_MACNTL_810       0x40
#define MIKASA_NCR_MACNTL_WRITABLE  0x0F
#define MIKASA_NCR_GPCNTL_WRITABLE  0xC3
#define MIKASA_NCR_STIME1_WRITABLE  0x0F
#define MIKASA_NCR_STEST1_WRITABLE  0xCC
#define MIKASA_NCR_STEST2_WRITABLE  0xBF
#define MIKASA_NCR_STEST3_CSF       0x02
#define MIKASA_NCR_STEST3_WRITABLE  0xF7
#define MIKASA_NCR_IRQ              12
#define MIKASA_NCR_INTBIT           (1u << MIKASA_NCR_IRQ)
#define MIKASA_RTC_IRQ              8
#define MIKASA_RTC_INTBIT           (1u << MIKASA_RTC_IRQ)
#define MIKASA_PIT_IRQ              0
#define MIKASA_ICU_HWRE_LEVEL       2
#define MIKASA_PIC_HWRE_LEVEL       1
#define MIKASA_ELCR1_WRITABLE       0xF8
#define MIKASA_ELCR2_WRITABLE       0xDE
#define MIKASA_CLOCK_TMR            0
#define MIKASA_CLOCK_HZ             100
#define MIKASA_CLOCK_DELAY          10000
#define MIKASA_PIT_HZ               1193182u
#define MIKASA_PIT_CHANNELS         3
#define MIKASA_PIT_ACCESS_LATCH     0
#define MIKASA_PIT_ACCESS_LO        1
#define MIKASA_PIT_ACCESS_HI        2
#define MIKASA_PIT_ACCESS_LOHI      3
#define MIKASA_PIT_MODE_TERMINAL    0
#define MIKASA_PIT_MODE_ONESHOT     1
#define MIKASA_PIT_MODE_RATE        2
#define MIKASA_PIT_MODE_SQUARE      3
#define MIKASA_PIT_MODE_STROBE      4
#define MIKASA_PIT_MODE_HWSTROBE    5
#define MIKASA_PIT_STATUS_OUT       0x80
#define MIKASA_PIT_STATUS_NULL      0x40
#define MIKASA_PORTB_GATE2          0x01
#define MIKASA_PORTB_SPKR           0x02
#define MIKASA_PORTB_OUT2           0x20
#define MIKASA_NCR_DSPS_OK          0
#define MIKASA_NCR_DSPS_DAT_OUT     8
#define MIKASA_NCR_DSPS_DAT_IN      9
#define MIKASA_NCR_TRACE_INSNS      64
#define MIKASA_NCR_TRACE_LIMIT      8
#define MIKASA_NCR_SCRIPT_SCAN_INSNS 128
#define MIKASA_NCR_SCRIPT_STACK     8
#define MIKASA_NCR_DMA_SEGS         32
#define MIKASA_NCR_BM_TYPE_MASK     0xC0000000u
#define MIKASA_NCR_BM_TYPE          0x00000000u
#define MIKASA_NCR_BM_INDIRECT      0x20000000u
#define MIKASA_NCR_BM_TABLE         0x10000000u
#define MIKASA_NCR_BM_PHASE_MASK    0x07000000u
#define MIKASA_NCR_BM_PHASE_SHIFT   24
#define MIKASA_NCR_BM_COUNT_MASK    0x00FFFFFFu
#define MIKASA_NCR_PHASE_DAT_OUT    0
#define MIKASA_NCR_PHASE_DAT_IN     1
#define MIKASA_NCR_PHASE_CMD        2
#define MIKASA_NCR_PHASE_STS        3
#define MIKASA_NCR_PHASE_MSG_OUT    6
#define MIKASA_NCR_PHASE_MSG_IN     7
#define MIKASA_NCR_SCR_SEL_ABS      0x40000000u
#define MIKASA_NCR_SCR_SEL_ABS_ATN  0x41000000u
#define MIKASA_NCR_SCR_SEL_TBL      0x42000000u
#define MIKASA_NCR_SCR_SEL_TBL_ATN  0x43000000u
#define MIKASA_NCR_SCR_REL          0x04000000u
#define MIKASA_NCR_SCR_WAIT_DISC    0x48000000u
#define MIKASA_NCR_SCR_WAIT_RESEL   0x50000000u
#define MIKASA_NCR_SCR_JUMP         0x80080000u
#define MIKASA_NCR_SCR_JUMPR        0x80880000u
#define MIKASA_NCR_SCR_CALL         0x88080000u
#define MIKASA_NCR_SCR_CALLR        0x88880000u
#define MIKASA_NCR_SCR_RETURN       0x90080000u
#define MIKASA_NCR_SCR_SET          0x58000000u
#define MIKASA_NCR_SCR_CLR          0x60000000u
#define MIKASA_NCR_SCR_CARRY        0x00000400u
#define MIKASA_NCR_SCR_TARGET       0x00000200u
#define MIKASA_NCR_SCR_ACK          0x00000040u
#define MIKASA_NCR_SCR_ATN          0x00000008u
#define MIKASA_NCR_MM_GROUP_MASK    0xE0000000u
#define MIKASA_NCR_MM_GROUP         0xC0000000u
#define MIKASA_NCR_TC_GROUP_MASK    0xF8000000u
#define MIKASA_NCR_TC_JUMP          0x80000000u
#define MIKASA_NCR_TC_CALL          0x88000000u
#define MIKASA_NCR_TC_RETURN        0x90000000u
#define MIKASA_NCR_TC_INT           0x98000000u
#define MIKASA_NCR_TC_REL           0x00800000u
#define MIKASA_NCR_TC_CARRY         0x00200000u
#define MIKASA_NCR_TC_IFTRUE        0x00080000u
#define MIKASA_NCR_TC_DATA          0x00040000u
#define MIKASA_NCR_TC_PHASE         0x00020000u
#define MIKASA_NCR_TC_INTFLY        0x00100000u
#define MIKASA_NCR_REGOP_GROUP_MASK 0xF8000000u
#define MIKASA_NCR_SFBR_REG         0x68000000u
#define MIKASA_NCR_REG_SFBR_OP      0x70000000u
#define MIKASA_NCR_REG_REG          0x78000000u
#define MIKASA_NCR_LS_GROUP_MASK    0xE0000000u
#define MIKASA_NCR_LS_GROUP         0xE0000000u
#define MIKASA_NCR_LS_LOAD          0x01000000u
#define MIKASA_NCR_LS_DSA_REL       0x10000000u
#define MIKASA_NCR_ALU_OP_MASK      0x07000000u
#define MIKASA_NCR_ALU_LOAD         0x00000000u
#define MIKASA_NCR_ALU_SHL          0x01000000u
#define MIKASA_NCR_ALU_OR           0x02000000u
#define MIKASA_NCR_ALU_XOR          0x03000000u
#define MIKASA_NCR_ALU_AND          0x04000000u
#define MIKASA_NCR_ALU_SHR          0x05000000u
#define MIKASA_NCR_ALU_ADD          0x06000000u
#define MIKASA_NCR_ALU_ADDC         0x07000000u
#define MIKASA_NCR_SCRIPT_NO_PHASE  8
#define MIKASA_ISA_DMA1             0x000
#define MIKASA_ISA_PIC1             0x020
#define MIKASA_ISA_CFG_INDEX        0x022
#define MIKASA_ISA_CFG_DATA         0x023
#define MIKASA_ISA_PIT              0x040
#define MIKASA_ISA_KBD_DATA         0x060
#define MIKASA_ISA_PORTB            0x061
#define MIKASA_ISA_KBD_CMD          0x064
#define MIKASA_ISA_RTC_INDEX        0x070
#define MIKASA_ISA_RTC_DATA         0x071
#define MIKASA_ISA_DMA_PAGE         0x080
#define MIKASA_ISA_PIC2             0x0A0
#define MIKASA_ISA_DMA2             0x0C0
#define MIKASA_ISA_COM2             0x2F8
#define MIKASA_ISA_SIO_INDEX        0x398
#define MIKASA_ISA_SIO_DATA         0x399
#define MIKASA_ISA_FDC              0x3F0
#define MIKASA_ISA_COM1             0x3F8
#define MIKASA_ISA_NMI_CTRL         0x461
#define MIKASA_NMISC_M_SERR         0x80
#define MIKASA_NMISC_M_IOCHK        0x40
#define MIKASA_NMISC_M_IOCHK_EN     0x08
#define MIKASA_NMISC_M_SERR_EN      0x04
#define MIKASA_ISA_ELCR1            0x4D0
#define MIKASA_ISA_ELCR2            0x4D1
#define MIKASA_ISA_OCP              0x530
#define MIKASA_ISA_ICU_IRR          0x534
#define MIKASA_ISA_ICU_IMR          0x536
#define MIKASA_ISA_ICU_PRESENT      0x8000
#define MIKASA_EISA_CRAM            0x0800
#define MIKASA_EISA_CRAM_SIZE       0x0100
#define MIKASA_EISA_CRAM_PAGES      32
#define MIKASA_EISA_CRAM_PAGE_REG   0x0C00
#define MIKASA_EISA_SLOT_ID         0x0C80
#define MIKASA_OCP_BUSY             0x80
#define MIKASA_OCP_HALT             0x40
#define MIKASA_OCP_ADDR_MASK        0x7F
#define MIKASA_OCP_DDRAM_SIZE       0x80

#define MIKASA_UART_IRQ             4
#define MIKASA_UART_CTRL_P          0x10
#define MIKASA_UART_IER_RDA         0x01
#define MIKASA_UART_IER_THRE        0x02
#define MIKASA_UART_IER_RLS         0x04
#define MIKASA_UART_IER_MS          0x08
#define MIKASA_UART_IIR_MS          0x00
#define MIKASA_UART_IIR_THRE        0x02
#define MIKASA_UART_IIR_RDA         0x04
#define MIKASA_UART_IIR_RLS         0x06
#define MIKASA_UART_LSR_DR          0x01
#define MIKASA_UART_LSR_THRE        0x20
#define MIKASA_UART_LSR_TEMT        0x40
#define MIKASA_UART_IIR_NOPEND      0x01
#define MIKASA_UART_LCR_DLAB        0x80
#define MIKASA_UART_MSR_DCTS        0x01
#define MIKASA_UART_MSR_CTS         0x10
#define MIKASA_UART_MSR_DSR         0x20
#define MIKASA_UART_MSR_DCD         0x80

#define MIKASA_KBD_IRQ              1
#define MIKASA_MOUSE_IRQ            12
#define MIKASA_KBC_RESP_SIZE        8
#define MIKASA_KBC_STATUS_OBF       0x01
#define MIKASA_KBC_STATUS_SYS       0x04
#define MIKASA_KBC_STATUS_AUX       0x20
#define MIKASA_KBC_CMD_READ_BYTE    0x20
#define MIKASA_KBC_CMD_WRITE_BYTE   0x60
#define MIKASA_KBC_CMD_DISABLE_AUX  0xA7
#define MIKASA_KBC_CMD_ENABLE_AUX   0xA8
#define MIKASA_KBC_CMD_TEST_AUX     0xA9
#define MIKASA_KBC_CMD_DISABLE_KBD  0xAD
#define MIKASA_KBC_CMD_ENABLE_KBD   0xAE
#define MIKASA_KBC_CMD_TEST_KBD     0xAB
#define MIKASA_KBC_CMD_SELF_TEST    0xAA
#define MIKASA_KBC_CMD_READ_OUT     0xD0
#define MIKASA_KBC_CMD_WRITE_OUT    0xD1
#define MIKASA_KBC_CMD_WRITE_AUX    0xD4
#define MIKASA_KBC_CCB_KBD_INT      0x01
#define MIKASA_KBC_CCB_AUX_INT      0x02
#define MIKASA_KBC_CCB_KBD_DISABLE  0x10
#define MIKASA_KBC_CCB_AUX_DISABLE  0x20
#define MIKASA_KBC_OUT_NORMAL       0x03
#define MIKASA_KBD_ACK              0xFA
#define MIKASA_KBD_BAT_OK           0xAA
#define MIKASA_KBD_ID1              0xAB
#define MIKASA_KBD_ID2              0x83

#define MIKASA_FDC_MSR_RQM          0x80

#define MIKASA_RTC_STATUSA          0x0A
#define MIKASA_RTC_STATUSB          0x0B
#define MIKASA_RTC_INTR            0x0C
#define MIKASA_RTC_STATUSD          0x0D
#define MIKASA_RTC_CENTURY          0x32
#define MIKASA_RTC_UIP              0x80
#define MIKASA_RTC_UIE              0x10
#define MIKASA_RTC_AIE              0x20
#define MIKASA_RTC_PIE              0x40
#define MIKASA_RTC_BINARY           0x04
#define MIKASA_RTC_24HR             0x02
#define MIKASA_RTC_UF               0x10
#define MIKASA_RTC_AF               0x20
#define MIKASA_RTC_PF               0x40
#define MIKASA_RTC_IRQF             0x80
#define MIKASA_RTC_POWER_OK         0x80
#define MIKASA_RTC_NVRAM_SIZE       128

#define MIKASA_DBG_IO               0x0001
#define MIKASA_DBG_UART             0x0002
#define MIKASA_DBG_EISA             0x0004
#define MIKASA_DBG_RTC              0x0008
#define MIKASA_DBG_PCI              0x0010
#define MIKASA_DBG_NCRDISC          0x0020
#define MIKASA_DBG_PAL              0x0040

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
extern t_uint64 pcq[PCQ_SIZE];
extern t_uint64 p1;
extern int32 pcq_p;
extern uint32 ir;
extern uint32 lock_flag;
extern uint32 pal_mode;
extern uint32 pal_type;
extern uint32 dmapen;
extern uint32 fpen;
extern uint32 cm_macc;
extern uint32 cm_racc;
extern uint32 cm_wacc;
extern uint32 itlb_cm;
extern uint32 itlb_asn;
extern uint32 dtlb_cm;
extern uint32 pcc_l;
extern uint32 pcc_h;
extern uint32 pcc_enb;
extern uint32 ev5_crd;
extern t_uint64 ev5_icsr;
extern t_uint64 ev4_read_icsr (void);
extern void ev4_write_icsr (t_uint64 val);
extern t_uint64 ev4_read_abox_ipr (uint32 idx);
extern void ev4_write_abox_ipr (uint32 idx, t_uint64 val);
extern t_uint64 ev4_hirr_bits (uint32 pins);
extern t_uint64 ev5_paltemp[];
extern uint32 pc_align;
extern t_uint64 trap_mask;
extern uint32 int_req[IPL_HLVL];
/*
 * Mikasa owns the EV4/PAL state in this file and in ev4_state.  The ev5_*
 * globals below are compatibility mirrors for the shared Alpha simulator core
 * and for non-Mikasa code paths; they must not be the authoritative Mikasa PAL
 * representation.
 */
extern uint32 ev5_ipl;
extern uint32 ev5_sirr;
extern uint32 ev5_astrr;
extern uint32 ev5_asten;
extern t_uint64 ev5_palbase;
extern t_uint64 ev5_ivptbr;
extern t_uint64 ev5_mvptbr;
extern UNIT dka_unit[];
extern SCSI_DEV dka_scsi_dev;

typedef struct {
    uint8 sfbr;
    uint32 phase;
    t_bool side_effects;
    t_bool carry;
    } MIKASA_NCR_SCRIPT_STATE;

typedef struct {
    uint32 count;
    uint32 addr;
    uint32 op;
    } MIKASA_NCR_DMA_SEG;

typedef struct {
    MIKASA_NCR_DMA_SEG seg[MIKASA_NCR_DMA_SEGS];
    uint32 count;
    uint32 bytes;
    } MIKASA_NCR_DMA_LIST;

typedef struct {
    MIKASA_NCR_DMA_LIST moves;
    t_bool selected;
    uint32 target;
    t_bool direct_valid;
    uint32 direct_count;
    uint32 direct_addr;
    t_bool table_valid;
    uint32 table_off;
    t_bool phase_int_valid;
    uint32 phase_int_dsps;
    t_bool any_int_valid;
    uint32 any_int_dsps;
    t_bool phase_next_valid;
    uint32 phase_next_dsp;
    } MIKASA_NCR_SCRIPT_SCAN;

typedef struct {
    t_bool active;
    t_bool status_pending;
    uint32 dsp;
    uint32 dsa;
    uint32 target;
    uint32 lun;
    t_bool tag_valid;
    uint8 tag_msg;
    uint8 tag;
    uint32 data_phase;
    uint8 status;
    } MIKASA_NCR_TRANSACTION;

typedef struct {
    uint32 reload;
    uint32 counter;
    uint32 latch;
    uint32 write_latch;
    uint8 access;
    uint8 mode;
    uint8 write_state;
    uint8 read_state;
    uint8 status;
    t_bool bcd;
    t_bool running;
    t_bool gate;
    t_bool out;
    t_bool latched;
    t_bool status_latched;
    t_bool null_count;
    } MIKASA_PIT_CHANNEL;

uint32 mikasa_irq_mask = 0;
uint32 mikasa_irq_summary = 0;
t_uint64 mikasa_hwrpb_pa = MIKASA_HWRPB_PA;
t_uint64 mikasa_boot_l1_pt_pa = MIKASA_L1_PT_PA;
t_uint64 mikasa_boot_vptb_va = MIKASA_PT_SPACE_VA;
t_uint64 mikasa_boot_image_pa = MIKASA_BOOT_IMAGE_PA;
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
static uint32 mikasa_apb_trace_count = 0;
static void mikasa_write_phys_byte (t_uint64 pa, uint8 dat);
static uint8 mikasa_read_phys_byte (t_uint64 pa);

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
t_uint64 mikasa_pal_sysval = 0;
t_uint64 mikasa_pal_thread = 0;
t_uint64 mikasa_pal_whami_reg = 0;
t_uint64 mikasa_pal_esp = 0;
t_uint64 mikasa_pal_ssp = 0;
t_uint64 mikasa_pal_usp = 0;
t_uint64 mikasa_pal_stkp[4] = { 0 };
t_uint64 mikasa_pal_impure = 0;
t_uint64 mikasa_pal_scc = 0;
uint32 mikasa_pal_ipl = 0;
uint32 mikasa_pal_sisr = 0;
uint32 mikasa_pal_asten = 0;
uint32 mikasa_pal_astsr = 0;
uint32 mikasa_pal_mces = 0;
uint32 mikasa_pal_ps = 0;
uint32 mikasa_pal_datfx = 0;
uint32 mikasa_pal_last_pcc = 0;
static const uint32 mikasa_ast_map[4] = { 0x1, 0x3, 0x7, 0xF };
static const uint32 mikasa_ast_pri[16] = {
    0, MODE_K, MODE_E, MODE_K, MODE_S, MODE_K, MODE_E, MODE_K,
    MODE_U, MODE_K, MODE_E, MODE_K, MODE_S, MODE_K, MODE_E, MODE_K
    };
static t_bool mikasa_srm_bootstrap_pal = FALSE;
static t_bool mikasa_srm_os_handoff = FALSE;

t_stat mikasa_reset (DEVICE *dptr);
t_stat mikasa_boot_prepare (CONST char *bootdev, uint32 osflags,
    t_uint64 image_bytes);
t_stat mikasa_set_rom (UNIT *uptr, int32 val, CONST char *cptr, void *desc);
t_stat mikasa_set_rom_payload (UNIT *uptr, int32 val, CONST char *cptr,
    void *desc);
t_stat mikasa_save_rom (UNIT *uptr, int32 val, CONST char *cptr, void *desc);
t_stat mikasa_set_nvram (UNIT *uptr, int32 val, CONST char *cptr,
    void *desc);
t_stat mikasa_set_vga (UNIT *uptr, int32 val, CONST char *cptr, void *desc);
t_stat mikasa_set_halt (UNIT *uptr, int32 val, CONST char *cptr, void *desc);
t_stat mikasa_boot_rom (int32 unitno);
t_stat mikasa_svc (UNIT *uptr);
void mikasa_mem_write (t_uint64 pa, t_uint64 dat, uint32 lnt);
t_bool mikasa_pal_uses_simh_ipl (void);
t_stat mikasa_pal_proc_intr (uint32 lvl);
t_stat mikasa_pal_proc_trap (uint32 summ);
t_stat mikasa_pal_proc_excp (uint32 abval);
t_stat mikasa_pal_proc_inst (uint32 fnc);
static t_bool mikasa_io_rd (t_uint64 pa, t_uint64 *val, uint32 lnt);
static t_bool mikasa_io_wr (t_uint64 pa, t_uint64 val, uint32 lnt);

static void mikasa_rtc_init_defaults (void);
static t_stat mikasa_nvram_save_file (void);
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
static void mikasa_set_cpu_info (void);
static t_stat mikasa_load_axpbox_rom (FILE *fileref, const char *path,
    const uint8 *header);
static t_stat mikasa_load_raw_rom_payload (FILE *fileref, t_offset fsize,
    const char *path);
static t_uint64 mikasa_debug_frame_ra (void);
static t_uint64 mikasa_debug_parent_frame_ra (void);
static t_stat mikasa_write_le64 (FILE *fileref, t_uint64 val);
static t_uint64 mikasa_sum_qwords (t_uint64 pa, uint32 size);
static t_uint64 mikasa_fill_bitmap (t_uint64 bitmap_pa, t_uint64 bits);
static t_uint64 mikasa_find_hwrpb_pa (void);
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
static uint32 mikasa_pal_find_pte_vms (t_uint64 va, t_uint64 *pte);
static uint32 mikasa_pal_find_pte_vms_mode (t_uint64 va, t_uint64 *pte,
    t_bool physical_mode);
static t_bool mikasa_pal_physical_mode (void);
static t_bool mikasa_pal_ptbr_physical_mode (void);
static t_uint64 mikasa_pal_ptbr_pa (void);
static t_uint64 mikasa_pal_ptbr_pfn (void);
static t_uint64 mikasa_pal_vtbr_va (void);
static t_uint64 mikasa_pal_ptbr_from_pfn (t_uint64 pfn);
static t_uint64 mikasa_pal_pcbb_icsr (t_uint64 icsr, uint32 asn,
    t_uint64 fen);
static void mikasa_pal_set_paltemp_raw (uint32 idx, t_uint64 value);
static t_bool mikasa_pal_cns_quad_pa (uint32 offset, t_uint64 *pa);
static t_uint64 mikasa_pal_read_cns_quad (uint32 offset, t_uint64 fallback);
static void mikasa_pal_write_cns_quad (uint32 offset, t_uint64 value);
static void mikasa_pal_sync_paltemp_icsr (void);
static void mikasa_pal_set_paltemp_whami (t_uint64 val);
static void mikasa_pal_mark_pal_swapped (void);
static void mikasa_pal_set_vptptr (t_uint64 vptptr);
static void mikasa_pal_write_fen (uint32 fen);
static void mikasa_pal_set_ptintmask (t_uint64 mask);
static void mikasa_pal_set_paltemp_ent (uint32 which, t_uint64 addr);
static uint32 mikasa_pal_test_va (t_uint64 va, uint32 acc, t_uint64 *pa);
static uint32 mikasa_pal_tlb_check (t_uint64 va);
static t_bool mikasa_pal_proc_align (void);
static t_bool mikasa_pal_proc_align_store (void);
static t_uint64 mikasa_pal_read_una (t_uint64 va, uint32 size);
static void mikasa_pal_write_una (t_uint64 va, t_uint64 val, uint32 size);
static t_uint64 mikasa_pal_read_una_l (t_uint64 va);
static void mikasa_pal_write_una_l (t_uint64 va, t_uint64 val);
static t_stat mikasa_pal_cserve (void);
static t_stat mikasa_pal_cserve_platform (void);
static t_bool mikasa_pal_valid_phys (t_uint64 pa, uint32 size);
static t_bool mikasa_pal_route_os_events (void);
static t_stat mikasa_pal_interrupt_post (uint32 type, uint32 vec,
    t_uint64 par2, uint32 newipl);
static t_bool mikasa_pal_bootstrap_map (void);
static void mikasa_pal_set_mode (uint32 mode, uint32 ipl);
static void mikasa_pal_set_pc (t_uint64 pc);
static uint32 mikasa_pal_current_mode (void);
static uint32 mikasa_pal_stack_mode (uint32 mode);
static t_stat mikasa_pal_cflush (void);
static t_stat mikasa_pal_draina (void);
static t_bool mikasa_pal_use_pal_dtb_flow (void);
static t_stat mikasa_pal_handle_tbm_bootstrap (uint32 abval, t_uint64 va,
    t_uint64 *pa);
static void mikasa_pal_sync_abox_crd (void);
static t_stat mikasa_pal_handle_tbm_native (uint32 abval, t_uint64 va);
static t_stat mikasa_pal_handle_tbm_pal (uint32 abval, t_uint64 va);
static t_bool mikasa_pal_fetch_ignore (void);
static void mikasa_pal_set_mm_args (t_uint64 va, t_uint64 code, t_uint64 ref);
static uint32 mikasa_pal_ps_ipl (t_uint64 ps);
static uint32 mikasa_pal_ps_mode (t_uint64 ps);
static t_uint64 mikasa_pal_pack_ps (uint32 ipl, uint32 mode);
static t_uint64 mikasa_pal_read_ps (void);
static t_uint64 mikasa_pal_live_hirr (void);
static t_uint64 mikasa_pal_read_hier (void);
static void mikasa_pal_apply_ipl_mask (void);
static void mikasa_pal_init_int_mask (void);
static void mikasa_pal_set_hier_from_ipl (uint32 ipl);
static void mikasa_pal_save_impure_state (void);
static void mikasa_pal_reset_impure (t_uint64 impure);
static t_uint64 mikasa_pal_build_short_logout (t_uint64 code);
static t_uint64 mikasa_pal_build_long_logout (t_uint64 code);
static t_stat mikasa_pal_mchk_logout (t_uint64 code);
static t_stat mikasa_pal_mchk_pal (void);
static t_stat mikasa_pal_mchk_callsys (void);
static t_stat mikasa_pal_mchk_ksp_not_valid (void);
static t_stat mikasa_pal_crd_logout (void);
static void mikasa_pal_write_pcbb_flags (t_uint64 mask, t_uint64 bits);
static t_stat mikasa_pal_load_pcbb (t_uint64 pcbb);
static t_stat mikasa_pal_save_pcbb_state (void);
static t_stat mikasa_pal_swpctx (void);
static t_stat mikasa_pal_halt (void);
static t_stat mikasa_pal_swppal (void);
static t_stat mikasa_pal_retsys (void);
static t_stat mikasa_pal_rti (void);
static t_stat mikasa_pal_intexc (uint32 vec, uint32 newmode, uint32 newipl);
static t_stat mikasa_pal_ent_dispatch_frame_pc (t_uint64 entry,
    uint32 newmode, uint32 newipl, t_uint64 frame_pc);
static t_stat mikasa_pal_ent_dispatch (t_uint64 entry, uint32 newmode,
    uint32 newipl);
static t_stat mikasa_pal_inst_fault (uint32 code);
static t_stat mikasa_pal_callsys (void);
static t_stat mikasa_pal_mm_intexc (t_uint64 va, t_uint64 code,
    t_uint64 ref);
static t_stat mikasa_pal_rei (void);
static t_uint64 mikasa_pal_whami (void);
static t_stat mikasa_pal_cserve_start (void);
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
static t_bool mikasa_pci_dma_addr_to_pa (uint32 addr, t_uint64 *pa);
static t_bool mikasa_pci_dma_read_byte (uint32 addr, uint8 *val);
static t_bool mikasa_pci_dma_write_byte (uint32 addr, uint8 val);
static t_bool mikasa_pci_dma_read_long (uint32 addr, uint32 *val);
static void mikasa_write_phys_long (t_uint64 pa, t_uint64 dat);
static void mikasa_write_phys_byte (t_uint64 pa, uint8 dat);
static uint8 mikasa_read_phys_byte (t_uint64 pa);
static uint32 mikasa_read_phys_long (t_uint64 pa);
static t_uint64 mikasa_read_phys_quad (t_uint64 pa);
static void mikasa_irq_update (void);
static void mikasa_pic_set_irq (uint32 irq, t_bool state);
static uint8 mikasa_pic_iack (void);
static uint32 mikasa_ncr_sext24 (uint32 val);
static uint32 mikasa_ncr_next_script_addr (uint32 dsp, uint32 op);
static uint8 mikasa_ncr_read_b (uint32 reg);
static void mikasa_ncr_write_b (uint32 reg, uint8 val);
static void mikasa_ncr_update_irq (void);
static void mikasa_tulip_update_irq (void);
static t_uint64 mikasa_ncr_frame_ra (void);
static t_uint64 mikasa_ncr_parent_frame_ra (void);
static void mikasa_ncr_debug_state (const char *tag);
static void mikasa_ncr_promote_interrupt_stack (void);
static void mikasa_ncr_clear_transaction (void);
static void mikasa_ncr_clear_wait_reselect (void);
static void mikasa_ncr_set_wait_reselect (uint32 dsp, uint32 jump);
static void mikasa_ncr_clear_scsi_fifo (void);
static void mikasa_ncr_write_ctest3 (uint8 val);
static void mikasa_ncr_write_ctest5 (uint8 val);
static void mikasa_ncr_write_stest3 (uint8 val);
static void mikasa_ncr_disc_log (const char *fmt, ...);
static t_bool mikasa_ncr_table_entry (uint32 dsa, uint32 off, uint32 *count,
    uint32 *addr);
static void mikasa_ncr_disc_track_cmd (uint32 target, uint32 lun,
    const uint8 *cdb);
static void mikasa_ncr_disc_track_timeout (uint32 target);
static void mikasa_ncr_disc_track_completion (uint32 target, uint32 lun,
    const uint8 *cdb, const MIKASA_NCR_DMA_LIST *data, uint8 status,
    uint32 dsps, uint32 resid);
static t_bool mikasa_ncr_bar_reg (uint32 addr, uint32 bar, uint32 mask,
    uint32 *reg);
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
static void mikasa_uart_update_irq (void);
static void mikasa_uart_poll (void);
static uint8 mikasa_uart_read (uint32 port);
static void mikasa_uart_write (uint32 port, uint8 val);
static void mikasa_rtc_update_irq (void);
static t_bool mikasa_rtc_periodic_due (void);
static void mikasa_pit_reset (void);
static uint8 mikasa_pit_read (uint32 port);
static void mikasa_pit_write (uint32 port, uint8 val);
static void mikasa_pit_tick (void);
static void mikasa_pit_set_gate (uint32 chan, t_bool state);
static uint8 mikasa_portb_read (void);
static void mikasa_portb_write (uint8 val);
static uint8 mikasa_ocp_read (uint32 reg);
static void mikasa_ocp_write (uint32 reg, uint8 val);
static uint8 mikasa_isa_read (uint32 port);
static void mikasa_isa_write (uint32 port, uint8 val);
static t_uint64 mikasa_comanche_read (t_uint64 pa, uint32 lnt);
static void mikasa_comanche_write (t_uint64 pa, t_uint64 val, uint32 lnt);
static void mikasa_comanche_init_regs (void);
static t_uint64 mikasa_sparse_read (t_uint64 pa, uint32 lnt);
static void mikasa_sparse_write (t_uint64 pa, t_uint64 val, uint32 lnt);

static char mikasa_auto_action[] = "BOOT";
static char mikasa_booted_dev[64] = "SCSI 0 6 0 0 0 0 0";
static char mikasa_booted_osflags[32] = "0";
static char mikasa_boot_file[] = "SYS0.SYSCOMMON.SYSEXE.SYSBOOT.EXE";
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
static t_bool mikasa_uart_thre_irq = TRUE;
static uint8 mikasa_uart_msr_delta = 0;
static uint8 mikasa_com2_lcr = 0;
static uint8 mikasa_com2_mcr = 0;
static uint8 mikasa_com2_ier = 0;
static uint8 mikasa_com2_scr = 0;
static uint8 mikasa_com2_dll = 0;
static uint8 mikasa_com2_dlm = 0;
static uint8 mikasa_pic_cmd[2] = { 0 };
static uint8 mikasa_pic_imr[2] = { 0xFF, 0xFF };
static uint8 mikasa_pic_irr[2] = { 0 };
static uint8 mikasa_pic_isr[2] = { 0 };
static uint8 mikasa_pic_vec[2] = { 0, 8 };
static uint8 mikasa_pic_init[2] = { 0 };
static t_bool mikasa_pic_read_isr[2] = { FALSE, FALSE };
static t_bool mikasa_pic_need_icw4[2] = { FALSE, FALSE };
static t_bool mikasa_pic_auto_eoi[2] = { FALSE, FALSE };
static uint8 mikasa_pic_level[2] = { 0 };
static uint8 mikasa_elcr[2] = { 0 };
static uint8 mikasa_dma1_reg[0x10] = { 0 };
static uint8 mikasa_dma2_reg[0x20] = { 0 };
static uint8 mikasa_dma_page_reg[0x10] = { 0 };
static uint8 mikasa_cfg_index = 0;
static uint8 mikasa_cfg_reg[256] = { 0 };
static MIKASA_PIT_CHANNEL mikasa_pit[MIKASA_PIT_CHANNELS];
static uint32 mikasa_pit_accum = 0;
static uint8 mikasa_kbc_cmd_byte = 0;
static uint8 mikasa_kbc_out_port = MIKASA_KBC_OUT_NORMAL;
static uint8 mikasa_kbc_pending_cmd = 0;
static uint8 mikasa_kbd_pending_cmd = 0;
static uint8 mikasa_kbc_resp[MIKASA_KBC_RESP_SIZE] = { 0 };
static t_bool mikasa_kbc_resp_aux[MIKASA_KBC_RESP_SIZE] = { FALSE };
static uint32 mikasa_kbc_resp_head = 0;
static uint32 mikasa_kbc_resp_count = 0;
static uint8 mikasa_portb = 0;
static uint8 mikasa_rtc_index = 0;
static uint8 mikasa_rtc_reg[128] = { 0 };
static uint32 mikasa_rtc_periodic_accum = 0;
static time_t mikasa_rtc_last_update = 0;
static time_t mikasa_rtc_last_alarm = 0;
static char mikasa_nvram_path[CBUFSIZE] = "";
static t_bool mikasa_nvram_loaded = FALSE;
static uint8 mikasa_sio_index = 0;
static uint8 mikasa_sio_reg[256] = { 0 };
static uint8 mikasa_fdc_reg[8] = { 0 };
static uint8 mikasa_nmi_ctrl = 0;
static uint8 mikasa_eisa_cram_page = 0;
static uint8 mikasa_eisa_cram[MIKASA_EISA_CRAM_PAGES][MIKASA_EISA_CRAM_SIZE];
static uint32 mikasa_comanche_reg[MIKASA_COMANCHE_SIZE >> 5] = { 0 };
static uint32 mikasa_epic_reg[MIKASA_EPIC_SIZE >> 5] = { 0 };
static uint32 mikasa_ncr_cfg[MIKASA_PCI_CFG_DWORDS] = { 0 };
static uint8 mikasa_ncr_reg[MIKASA_NCR_REG_SIZE] = { 0 };
static uint8 mikasa_ncr_dstat_stack = 0;
static uint8 mikasa_ncr_sist0_stack = 0;
static uint8 mikasa_ncr_sist1_stack = 0;
static t_bool mikasa_ncr_dsps_stack_valid = FALSE;
static uint32 mikasa_ncr_dsps_stack = 0;
static uint32 mikasa_tulip_cfg[MIKASA_PCI_CFG_DWORDS] = { 0 };
static uint8 mikasa_tulip_reg[MIKASA_TULIP_REG_SIZE] = { 0 };
static uint8 mikasa_tulip_srom[MIKASA_TULIP_SROM_BYTES] = { 0 };
static uint8 mikasa_tulip_enetrom[MIKASA_TULIP_ENETROM_BYTES] = { 0 };
static uint32 mikasa_tulip_srom_shift = 0;
static uint32 mikasa_tulip_srom_bits = 0;
static uint32 mikasa_tulip_srom_out = 0;
static uint32 mikasa_tulip_srom_out_bits = 0;
static uint32 mikasa_tulip_enetrom_idx = 0;
static uint32 mikasa_tulip_tx_base = 0;
static uint32 mikasa_tulip_tx_cur = 0;
static uint32 mikasa_tulip_rx_base = 0;
static uint32 mikasa_tulip_rx_cur = 0;
static uint32 mikasa_tulip_desc_skip = 0;
static uint32 mikasa_vga_cfg[MIKASA_PCI_CFG_DWORDS] = { 0 };
static uint8 mikasa_vga_reg[0x20] = { 0 };
static uint8 mikasa_vga_crtc_index = 0;
static uint8 mikasa_vga_seq_index = 0;
static uint8 mikasa_vga_gfx_index = 0;
static uint8 mikasa_vga_attr_index = 0;
static t_bool mikasa_vga_attr_addr = TRUE;
static t_bool mikasa_vga_enabled = TRUE;
static uint32 mikasa_ncr_script_trace_count = 0;
static uint32 mikasa_ncr_disc_pass = 0;
static t_bool mikasa_ncr_disc_pass_armed = FALSE;
static uint32 mikasa_ncr_disc_last_target = M32;
static uint32 mikasa_ncr_disc_last_lun = M32;
static uint32 mikasa_ncr_disc_last_cdb = 0xFF;
static uint32 mikasa_icu_int_req_bits = 0;
static uint32 mikasa_pic_int_req_bits = 0;
static MIKASA_NCR_TRANSACTION mikasa_ncr_transaction = { 0 };
static t_bool mikasa_ncr_wait_reselect = FALSE;
static uint32 mikasa_ncr_wait_reselect_dsp = 0;
static uint32 mikasa_ncr_wait_reselect_jump = 0;
static uint8 mikasa_ncr_sense_key[MIKASA_DKA_UNITS] = { 0 };
static uint8 mikasa_ncr_sense_asc[MIKASA_DKA_UNITS] = { 0 };
static uint8 mikasa_ncr_sense_ascq[MIKASA_DKA_UNITS] = { 0 };
static SCSI_BUS mikasa_ncr_scsi_bus = { 0 };
static t_bool mikasa_direct_apb = FALSE;
static uint32 mikasa_pceb_cfg[MIKASA_PCI_CFG_DWORDS] = { 0 };
static uint8 mikasa_ocp_reg[4] = { 0 };
static uint8 mikasa_ocp_ddram[MIKASA_OCP_DDRAM_SIZE] = { 0 };
static uint8 mikasa_ocp_addr = 0;
static uint8 mikasa_ocp_busy_count = 0;
static t_bool mikasa_halt_switch = FALSE;
static t_bool mikasa_halt_pending = FALSE;
uint32 mikasa_scc_scale = 1;

UNIT mikasa_unit = { UDATA (&mikasa_svc, 0, 0) };

DIB mikasa_dib = {
    MIKASA_COMANCHE_BASE, MIKASA_APECS_PCI_DENSE + MIKASA_APECS_PCI_DENSE_SIZE,
    &mikasa_io_rd, &mikasa_io_wr, 0
    };

DEBTAB mikasa_debug[] = {
    { "IO", MIKASA_DBG_IO },
    { "UART", MIKASA_DBG_UART },
    { "EISA", MIKASA_DBG_EISA },
    { "RTC", MIKASA_DBG_RTC },
    { "PCI", MIKASA_DBG_PCI },
    { "NCRDISC", MIKASA_DBG_NCRDISC },
    { "PAL", MIKASA_DBG_PAL },
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
    { MTAB_XTD|MTAB_VDV|MTAB_VALR|MTAB_NC, 0, "NVRAM", "NVRAM",
      &mikasa_set_nvram, NULL, NULL,
      "Load or create a 128-byte Mikasa RTC/NVRAM image" },
    { MTAB_XTD|MTAB_VDV|MTAB_VALR|MTAB_NC, 0, "VGA", "VGA",
      &mikasa_set_vga, NULL, NULL,
      "Enable or disable the Mikasa PCI VGA probe device" },
    { MTAB_XTD|MTAB_VDV|MTAB_VALR|MTAB_NC, 0, "HALT", "HALT",
      &mikasa_set_halt, NULL, NULL,
      "Set the Mikasa front-panel Halt switch IN or OUT" },
    { 0 }
    };

DEVICE mikasa_dev = {
    "MIKASA", &mikasa_unit, mikasa_reg, mikasa_mod,
    1, 16, 32, 1, 16, 8,
    NULL, NULL, &mikasa_reset,
    NULL, NULL, NULL, &mikasa_dib,
    DEV_DIB|DEV_DEBUG, 0, mikasa_debug
    };

static t_uint64 mikasa_debug_frame_ra (void)
{
t_uint64 addr = R[29] + 8;

if (!ADDR_IS_MEM (addr + 7))
    return 0;
return mikasa_read_phys_quad (addr);
}

static t_uint64 mikasa_debug_parent_frame_ra (void)
{
t_uint64 gp_addr = R[29] + 32;
t_uint64 gp;
t_uint64 addr;

if (!ADDR_IS_MEM (gp_addr + 7))
    return 0;
gp = mikasa_read_phys_quad (gp_addr);
addr = gp + 8;
if (!ADDR_IS_MEM (addr + 7))
    return 0;
return mikasa_read_phys_quad (addr);
}

static void mikasa_uart_update_irq (void)
{
t_bool pending = FALSE;

if ((mikasa_uart_ier & MIKASA_UART_IER_RDA) && mikasa_uart_rbr_valid)
    pending = TRUE;
if ((mikasa_uart_ier & MIKASA_UART_IER_THRE) && mikasa_uart_thre_irq)
    pending = TRUE;
if ((mikasa_uart_ier & MIKASA_UART_IER_MS) && (mikasa_uart_msr_delta != 0))
    pending = TRUE;
mikasa_pic_set_irq (MIKASA_UART_IRQ, pending);
return;
}

static uint8 mikasa_uart_iir (void)
{
uint8 iir = MIKASA_UART_IIR_NOPEND;

mikasa_uart_poll ();
if ((mikasa_uart_ier & MIKASA_UART_IER_RDA) && mikasa_uart_rbr_valid)
    iir = MIKASA_UART_IIR_RDA;
else if ((mikasa_uart_ier & MIKASA_UART_IER_THRE) && mikasa_uart_thre_irq) {
    iir = MIKASA_UART_IIR_THRE;
    mikasa_uart_thre_irq = FALSE;
    }
else if ((mikasa_uart_ier & MIKASA_UART_IER_MS) && (mikasa_uart_msr_delta != 0))
    iir = MIKASA_UART_IIR_MS;
mikasa_uart_update_irq ();
return iir;
}

static void mikasa_uart_poll (void)
{
t_stat c;

if (mikasa_uart_rbr_valid)
    return;
c = sim_poll_kbd ();
if (c >= SCPE_KFLAG) {
    mikasa_uart_rbr = (uint8) (c & M8);
    if (mikasa_uart_rbr == MIKASA_UART_CTRL_P) {
        mikasa_halt_pending = TRUE;
        sim_debug (MIKASA_DBG_UART, &mikasa_dev,
            "COM1 Ctrl-P latched halt request pc=%llX ra=%llX\n",
            (unsigned long long) PC, (unsigned long long) R[26]);
        }
    mikasa_uart_rbr_valid = TRUE;
    sim_debug (MIKASA_DBG_UART, &mikasa_dev,
        "COM1 receive pc=%llX ra=%llX fra=%llX pra=%llX %02X\n",
        (unsigned long long) PC, (unsigned long long) R[26],
        (unsigned long long) mikasa_debug_frame_ra (),
        (unsigned long long) mikasa_debug_parent_frame_ra (),
        mikasa_uart_rbr);
    mikasa_uart_update_irq ();
    }
return;
}

static uint8 mikasa_uart_read (uint32 port)
{
uint32 reg = port - MIKASA_ISA_COM1;
uint8 val;

switch (reg) {

    case 0:
        if (mikasa_uart_lcr & MIKASA_UART_LCR_DLAB)
            return mikasa_uart_dll;
        mikasa_uart_poll ();
        if (mikasa_uart_rbr_valid) {
            mikasa_uart_rbr_valid = FALSE;
            sim_debug (MIKASA_DBG_UART, &mikasa_dev,
                "COM1 read pc=%llX ra=%llX fra=%llX pra=%llX RBR %02X\n",
                (unsigned long long) PC, (unsigned long long) R[26],
                (unsigned long long) mikasa_debug_frame_ra (),
                (unsigned long long) mikasa_debug_parent_frame_ra (),
                mikasa_uart_rbr);
            mikasa_uart_update_irq ();
            return mikasa_uart_rbr;
            }
        return 0;

    case 1:
        if (mikasa_uart_lcr & MIKASA_UART_LCR_DLAB)
            return mikasa_uart_dlm;
        return mikasa_uart_ier;

    case 2:
        return mikasa_uart_iir ();

    case 3:
        return mikasa_uart_lcr;

    case 4:
        return mikasa_uart_mcr;

    case 5:
        mikasa_uart_poll ();
        return MIKASA_UART_LSR_THRE | MIKASA_UART_LSR_TEMT |
            (mikasa_uart_rbr_valid? MIKASA_UART_LSR_DR: 0);

    case 6:
        val = MIKASA_UART_MSR_CTS | MIKASA_UART_MSR_DSR |
            MIKASA_UART_MSR_DCD | mikasa_uart_msr_delta;
        mikasa_uart_msr_delta = 0;
        mikasa_uart_update_irq ();
        return val;

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
                "COM1 transmit pc=%llX ra=%llX fra=%llX pra=%llX %02X\n",
                (unsigned long long) PC, (unsigned long long) R[26],
                (unsigned long long) mikasa_debug_frame_ra (),
                (unsigned long long) mikasa_debug_parent_frame_ra (), val);
            if (c >= 0)
                (void) sim_putchar_s (c);
            mikasa_uart_thre_irq = TRUE;
            mikasa_uart_update_irq ();
            }
        break;

    case 1:
        if (mikasa_uart_lcr & MIKASA_UART_LCR_DLAB)
            mikasa_uart_dlm = val;
        else {
            mikasa_uart_ier = val;
            if (val & MIKASA_UART_IER_THRE)
                mikasa_uart_thre_irq = TRUE;
            sim_debug (MIKASA_DBG_UART, &mikasa_dev,
                "COM1 write pc=%llX ra=%llX fra=%llX pra=%llX IER=%02X\n",
                (unsigned long long) PC, (unsigned long long) R[26],
                (unsigned long long) mikasa_debug_frame_ra (),
                (unsigned long long) mikasa_debug_parent_frame_ra (), val);
            mikasa_uart_update_irq ();
            }
        break;

    case 3:
        mikasa_uart_lcr = val;
        sim_debug (MIKASA_DBG_UART, &mikasa_dev,
            "COM1 write pc=%llX ra=%llX fra=%llX pra=%llX LCR=%02X\n",
            (unsigned long long) PC, (unsigned long long) R[26],
            (unsigned long long) mikasa_debug_frame_ra (),
            (unsigned long long) mikasa_debug_parent_frame_ra (), val);
        break;

    case 4:
        mikasa_uart_mcr = val;
        sim_debug (MIKASA_DBG_UART, &mikasa_dev,
            "COM1 write pc=%llX ra=%llX fra=%llX pra=%llX MCR=%02X\n",
            (unsigned long long) PC, (unsigned long long) R[26],
            (unsigned long long) mikasa_debug_frame_ra (),
            (unsigned long long) mikasa_debug_parent_frame_ra (), val);
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

static uint8 mikasa_pic_read (uint32 port)
{
uint32 which = (port >= MIKASA_ISA_PIC2) ? 1 : 0;
uint8 val;

if (port & 1)
    val = mikasa_pic_imr[which];
else
    val = mikasa_pic_read_isr[which] ? mikasa_pic_isr[which] :
        mikasa_pic_irr[which];
sim_debug (MIKASA_DBG_IO, &mikasa_dev, "PIC%u read %03X=%02X\n",
    which + 1, port, val);
return val;
}

static uint32 mikasa_pic_first (uint8 pending)
{
uint32 i;

for (i = 0; i < 8; i++) {
    if (pending & (1u << i))
        return i;
    }
return 8;
}

static void mikasa_pic_refresh_level (uint32 which, uint8 bit)
{
if ((mikasa_elcr[which] & bit) && (mikasa_pic_level[which] & bit) &&
    ((mikasa_pic_isr[which] & bit) == 0))
    mikasa_pic_irr[which] |= bit;
return;
}

static void mikasa_pic_eoi (uint32 which, uint8 bit)
{
mikasa_pic_isr[which] &= ~bit;
mikasa_pic_refresh_level (which, bit);
return;
}

static void mikasa_pic_write_elcr (uint32 which, uint8 val)
{
uint32 i;
uint8 mask = (which == 0) ? MIKASA_ELCR1_WRITABLE : MIKASA_ELCR2_WRITABLE;

mikasa_elcr[which] = val & mask;
for (i = 0; i < 8; i++)
    mikasa_pic_refresh_level (which, (uint8) (1u << i));
mikasa_irq_update ();
sim_debug (MIKASA_DBG_IO, &mikasa_dev, "ELCR%u=%02X\n",
    which + 1, mikasa_elcr[which]);
return;
}

static void mikasa_pic_write (uint32 port, uint8 val)
{
uint32 which = (port >= MIKASA_ISA_PIC2) ? 1 : 0;

if (port & 1) {
    switch (mikasa_pic_init[which]) {
    case 1:
        mikasa_pic_vec[which] = val & 0xF8;
        mikasa_pic_init[which] = 2;
        break;

    case 2:
        mikasa_pic_init[which] = mikasa_pic_need_icw4[which] ? 3 : 0;
        break;

    case 3:
        mikasa_pic_auto_eoi[which] = (val & 2) ? TRUE : FALSE;
        mikasa_pic_init[which] = 0;
        break;

    default:
        mikasa_pic_imr[which] = val;
        mikasa_irq_update ();
        break;
        }
    }
else {
    mikasa_pic_cmd[which] = val;
    if (val & 0x10) {                                 /* ICW1 */
        mikasa_pic_init[which] = 1;
        mikasa_pic_need_icw4[which] = (val & 1) ? TRUE : FALSE;
        }
    else if ((val & 0x18) == 0x08)                    /* OCW3 */
        mikasa_pic_read_isr[which] = (val & 1) ? TRUE : FALSE;
    else if ((val & 0x60) == 0x60) {                  /* specific EOI */
        uint8 bit = 1u << (val & 7);

        mikasa_pic_eoi (which, bit);
        mikasa_irq_update ();
        }
    else if (val == 0x20) {                           /* non-specific EOI */
        uint32 i = mikasa_pic_first (mikasa_pic_isr[which]);

        if (i < 8)
            mikasa_pic_eoi (which, (uint8) (1u << i));
        mikasa_irq_update ();
        }
    }
sim_debug (MIKASA_DBG_IO, &mikasa_dev, "PIC%u write %03X=%02X\n",
    which + 1, port, val);
return;
}

static uint8 mikasa_rtc_encode (uint32 val)
{
if (mikasa_rtc_reg[MIKASA_RTC_STATUSB] & MIKASA_RTC_BINARY)
    return (uint8) val;
return (uint8) (((val / 10) << 4) | (val % 10));
}

static t_bool mikasa_rtc_alarm_field_match (uint8 alarm, uint8 now)
{
if ((alarm & 0xC0) == 0xC0)
    return TRUE;
return alarm == now;
}

static t_bool mikasa_rtc_alarm_match (struct tm *tm)
{
return mikasa_rtc_alarm_field_match (mikasa_rtc_reg[0x01],
    mikasa_rtc_encode ((uint32) tm->tm_sec)) &&
    mikasa_rtc_alarm_field_match (mikasa_rtc_reg[0x03],
    mikasa_rtc_encode ((uint32) tm->tm_min)) &&
    mikasa_rtc_alarm_field_match (mikasa_rtc_reg[0x05],
    mikasa_rtc_encode ((uint32) tm->tm_hour));
}

static t_bool mikasa_rtc_periodic_due (void)
{
uint32 rate = mikasa_rtc_reg[MIKASA_RTC_STATUSA] & 0x0F;
uint32 hz;

if ((rate < 3) || (rate > 15))
    return FALSE;
hz = 32768u >> (rate - 1);
if (hz >= MIKASA_CLOCK_HZ)
    return TRUE;
mikasa_rtc_periodic_accum += hz;
if (mikasa_rtc_periodic_accum < MIKASA_CLOCK_HZ)
    return FALSE;
mikasa_rtc_periodic_accum -= MIKASA_CLOCK_HZ;
return TRUE;
}

static void mikasa_rtc_update_irq (void)
{
uint8 regc = mikasa_rtc_reg[MIKASA_RTC_INTR];
uint8 regb = mikasa_rtc_reg[MIKASA_RTC_STATUSB];
uint8 enabled = 0;

if ((regc & MIKASA_RTC_PF) && (regb & MIKASA_RTC_PIE))
    enabled |= MIKASA_RTC_PF;
if ((regc & MIKASA_RTC_AF) && (regb & MIKASA_RTC_AIE))
    enabled |= MIKASA_RTC_AF;
if ((regc & MIKASA_RTC_UF) && (regb & MIKASA_RTC_UIE))
    enabled |= MIKASA_RTC_UF;
if (enabled) {
    mikasa_rtc_reg[MIKASA_RTC_INTR] |= MIKASA_RTC_IRQF;
    mikasa_irq_summary |= MIKASA_RTC_INTBIT;
    }
else {
    mikasa_rtc_reg[MIKASA_RTC_INTR] &= ~MIKASA_RTC_IRQF;
    mikasa_irq_summary &= ~MIKASA_RTC_INTBIT;
    }
mikasa_irq_update ();
return;
}

static void mikasa_rtc_init_defaults (void)
{
memset (mikasa_rtc_reg, 0, sizeof (mikasa_rtc_reg));
mikasa_rtc_reg[0x01] = 0xC0;
mikasa_rtc_reg[0x03] = 0xC0;
mikasa_rtc_reg[0x05] = 0xC0;
mikasa_rtc_reg[MIKASA_RTC_STATUSA] = 0x26;
mikasa_rtc_reg[MIKASA_RTC_STATUSB] = MIKASA_RTC_24HR;
mikasa_rtc_reg[MIKASA_RTC_STATUSD] = MIKASA_RTC_POWER_OK;
return;
}

static t_stat mikasa_nvram_save_file (void)
{
FILE *fileref;
t_stat r = SCPE_OK;

if (mikasa_nvram_path[0] == 0)
    return SCPE_OK;
fileref = sim_fopen (mikasa_nvram_path, "wb");
if (fileref == NULL)
    return sim_messagef (SCPE_OPENERR,
        "MIKASA: cannot open NVRAM image %s\n", mikasa_nvram_path);
if (sim_fwrite (mikasa_rtc_reg, 1, MIKASA_RTC_NVRAM_SIZE, fileref) !=
    MIKASA_RTC_NVRAM_SIZE)
    r = SCPE_IOERR;
fclose (fileref);
return r;
}

t_stat mikasa_set_nvram (UNIT *uptr, int32 val, CONST char *cptr, void *desc)
{
FILE *fileref;
size_t bytes;

if ((cptr == NULL) || (*cptr == 0))
    return SCPE_ARG;
strncpy (mikasa_nvram_path, cptr, sizeof (mikasa_nvram_path) - 1);
mikasa_nvram_path[sizeof (mikasa_nvram_path) - 1] = 0;
fileref = sim_fopen (cptr, "rb");
if (fileref == NULL) {
    mikasa_rtc_init_defaults ();
    mikasa_nvram_loaded = TRUE;
    return mikasa_nvram_save_file ();
    }
bytes = sim_fread (mikasa_rtc_reg, 1, MIKASA_RTC_NVRAM_SIZE, fileref);
fclose (fileref);
if (bytes != MIKASA_RTC_NVRAM_SIZE)
    return sim_messagef (SCPE_FMT,
        "MIKASA: NVRAM image %s is not %u bytes\n", cptr,
        MIKASA_RTC_NVRAM_SIZE);
mikasa_nvram_loaded = TRUE;
mikasa_rtc_reg[MIKASA_RTC_STATUSD] = MIKASA_RTC_POWER_OK;
sim_printf ("Loaded Mikasa RTC/NVRAM image from %s\n", cptr);
return SCPE_OK;
}

t_stat mikasa_set_vga (UNIT *uptr, int32 val, CONST char *cptr, void *desc)
{
if ((cptr == NULL) || (*cptr == 0))
    return SCPE_ARG;
if (sim_strcasecmp (cptr, "ENABLED") == 0) {
    mikasa_vga_enabled = TRUE;
    return SCPE_OK;
    }
if (sim_strcasecmp (cptr, "DISABLED") == 0) {
    mikasa_vga_enabled = FALSE;
    return SCPE_OK;
    }
return sim_messagef (SCPE_ARG,
    "MIKASA: VGA must be ENABLED or DISABLED\n");
}

t_stat mikasa_set_halt (UNIT *uptr, int32 val, CONST char *cptr, void *desc)
{
if ((cptr == NULL) || (*cptr == 0))
    return SCPE_ARG;
if (sim_strcasecmp (cptr, "IN") == 0) {
    mikasa_halt_switch = TRUE;
    return SCPE_OK;
    }
if (sim_strcasecmp (cptr, "OUT") == 0) {
    mikasa_halt_switch = FALSE;
    return SCPE_OK;
    }
return sim_messagef (SCPE_ARG,
    "MIKASA: HALT must be IN or OUT\n");
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
        val = (mikasa_rtc_reg[index] & 0x7F) |
            (((sim_os_msec () % 1000) >= 990) ? MIKASA_RTC_UIP : 0);
        break;

    case MIKASA_RTC_INTR:
        val = mikasa_rtc_reg[index];
        mikasa_rtc_reg[index] = 0;
        mikasa_rtc_update_irq ();
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

if ((index != MIKASA_RTC_INTR) && (index != MIKASA_RTC_STATUSD)) {
    mikasa_rtc_reg[index] = val;
    (void) mikasa_nvram_save_file ();
    if (index == MIKASA_RTC_STATUSA)
        mikasa_rtc_periodic_accum = 0;
    if (index == MIKASA_RTC_STATUSB) {
        if ((val & (MIKASA_RTC_UIE | MIKASA_RTC_AIE |
            MIKASA_RTC_PIE)) == 0)
            mikasa_rtc_reg[MIKASA_RTC_INTR] &= ~MIKASA_RTC_IRQF;
        mikasa_rtc_update_irq ();
        }
    }
sim_debug (MIKASA_DBG_RTC, &mikasa_dev, "RTC write %02X=%02X\n",
    index, val);
return;
}

t_stat mikasa_svc (UNIT *uptr)
{
uint8 events = 0;
time_t now;
struct tm *tm;

if (mikasa_rtc_periodic_due ())
    events |= MIKASA_RTC_PF;
now = time (NULL);
if (now != mikasa_rtc_last_update) {
    mikasa_rtc_last_update = now;
    events |= MIKASA_RTC_UF;
    if (now != mikasa_rtc_last_alarm) {
        tm = localtime (&now);
        if ((tm != NULL) && mikasa_rtc_alarm_match (tm)) {
            mikasa_rtc_last_alarm = now;
            events |= MIKASA_RTC_AF;
            }
        }
    }
if (events != 0) {
    mikasa_rtc_reg[MIKASA_RTC_INTR] |= events;
    mikasa_rtc_update_irq ();
    }
mikasa_uart_poll ();
mikasa_pit_tick ();
sim_activate (uptr, sim_rtcn_calb (MIKASA_CLOCK_HZ, MIKASA_CLOCK_TMR));
return SCPE_OK;
}

static void mikasa_pit_reset (void)
{
uint32 i;

memset (mikasa_pit, 0, sizeof (mikasa_pit));
for (i = 0; i < MIKASA_PIT_CHANNELS; i++) {
    mikasa_pit[i].access = MIKASA_PIT_ACCESS_LOHI;
    mikasa_pit[i].mode = MIKASA_PIT_MODE_TERMINAL;
    mikasa_pit[i].null_count = TRUE;
    mikasa_pit[i].gate = (i == 2) ?
        ((mikasa_portb & MIKASA_PORTB_GATE2) != 0) : TRUE;
    mikasa_pit[i].out = TRUE;
    }
mikasa_pit_accum = 0;
mikasa_pic_set_irq (MIKASA_PIT_IRQ, FALSE);
return;
}

static uint32 mikasa_pit_delta (void)
{
uint32 delta = MIKASA_PIT_HZ / MIKASA_CLOCK_HZ;

mikasa_pit_accum += MIKASA_PIT_HZ % MIKASA_CLOCK_HZ;
if (mikasa_pit_accum >= MIKASA_CLOCK_HZ) {
    mikasa_pit_accum -= MIKASA_CLOCK_HZ;
    delta++;
    }
return delta;
}

static uint8 mikasa_pit_status (uint32 chan)
{
MIKASA_PIT_CHANNEL *pit = &mikasa_pit[chan];
uint8 mode = pit->mode;
uint8 status = (pit->out ? MIKASA_PIT_STATUS_OUT : 0) |
    (pit->null_count ? MIKASA_PIT_STATUS_NULL : 0) |
    ((pit->access & 3) << 4) | ((mode & 7) << 1) |
    (pit->bcd ? 1 : 0);

return status;
}

static void mikasa_pit_latch_count (uint32 chan)
{
MIKASA_PIT_CHANNEL *pit = &mikasa_pit[chan];

if (!pit->latched) {
    pit->latch = pit->counter & 0xFFFFu;
    pit->latched = TRUE;
    pit->read_state = 0;
    }
return;
}

static uint8 mikasa_pit_read_counter (MIKASA_PIT_CHANNEL *pit, uint32 val,
    t_bool *done)
{
uint8 ret;

*done = TRUE;
switch (pit->access) {

    case MIKASA_PIT_ACCESS_LO:
        ret = val & 0xFF;
        break;

    case MIKASA_PIT_ACCESS_HI:
        ret = (val >> 8) & 0xFF;
        break;

    case MIKASA_PIT_ACCESS_LOHI:
    default:
        if (pit->read_state == 0) {
            pit->read_state = 1;
            *done = FALSE;
            ret = val & 0xFF;
            }
        else {
            pit->read_state = 0;
            ret = (val >> 8) & 0xFF;
            }
        break;
        }
return ret;
}

static uint8 mikasa_pit_read (uint32 port)
{
uint32 chan = port - MIKASA_ISA_PIT;
MIKASA_PIT_CHANNEL *pit;
uint8 ret;
t_bool done;

if (chan >= MIKASA_PIT_CHANNELS)
    return 0;
pit = &mikasa_pit[chan];
if (pit->status_latched) {
    pit->status_latched = FALSE;
    sim_debug (MIKASA_DBG_IO, &mikasa_dev, "PIT%u status read %02X\n",
        chan, pit->status);
    return pit->status;
    }
ret = mikasa_pit_read_counter (pit,
    pit->latched ? pit->latch : (pit->counter & 0xFFFFu), &done);
if (pit->latched && done)
    pit->latched = FALSE;
sim_debug (MIKASA_DBG_IO, &mikasa_dev,
    "PIT%u read %02X reload=%u counter=%u\n",
    chan, ret, pit->reload, pit->counter);
return ret;
}

static void mikasa_pit_load_counter (uint32 chan, uint32 count)
{
MIKASA_PIT_CHANNEL *pit = &mikasa_pit[chan];

pit->reload = count ? count : 0x10000u;
pit->counter = pit->reload;
pit->running = pit->gate;
pit->out = ((pit->mode == MIKASA_PIT_MODE_TERMINAL) ||
    (pit->mode == MIKASA_PIT_MODE_ONESHOT) ||
    (pit->mode == MIKASA_PIT_MODE_STROBE) ||
    (pit->mode == MIKASA_PIT_MODE_HWSTROBE)) ? FALSE : TRUE;
pit->null_count = FALSE;
pit->read_state = 0;
sim_debug (MIKASA_DBG_IO, &mikasa_dev,
    "PIT%u load reload=%u mode=%u access=%u\n",
    chan, pit->reload, pit->mode, pit->access);
return;
}

static void mikasa_pit_write_counter (uint32 chan, uint8 val)
{
MIKASA_PIT_CHANNEL *pit = &mikasa_pit[chan];

switch (pit->access) {

    case MIKASA_PIT_ACCESS_LO:
        mikasa_pit_load_counter (chan, val);
        break;

    case MIKASA_PIT_ACCESS_HI:
        mikasa_pit_load_counter (chan, ((uint32) val) << 8);
        break;

    case MIKASA_PIT_ACCESS_LOHI:
    default:
        if (pit->write_state == 0) {
            pit->write_latch = val;
            pit->write_state = 1;
            }
        else {
            pit->write_state = 0;
            mikasa_pit_load_counter (chan,
                pit->write_latch | (((uint32) val) << 8));
            }
        break;
        }
return;
}

static void mikasa_pit_latch_status (uint32 chan)
{
MIKASA_PIT_CHANNEL *pit = &mikasa_pit[chan];

if (!pit->status_latched) {
    pit->status = mikasa_pit_status (chan);
    pit->status_latched = TRUE;
    }
return;
}

static void mikasa_pit_control (uint8 val)
{
uint32 chan = (val >> 6) & 3;
uint8 access = (val >> 4) & 3;
uint8 mode = (val >> 1) & 7;

if (chan == 3) {
    uint32 i;

    for (i = 0; i < MIKASA_PIT_CHANNELS; i++) {
        if ((val & (1u << (i + 1))) == 0)
            continue;
        if ((val & 0x20) == 0)
            mikasa_pit_latch_count (i);
        if ((val & 0x10) == 0)
            mikasa_pit_latch_status (i);
        }
    sim_debug (MIKASA_DBG_IO, &mikasa_dev, "PIT readback %02X\n", val);
    return;
    }

if (access == MIKASA_PIT_ACCESS_LATCH) {
    mikasa_pit_latch_count (chan);
    sim_debug (MIKASA_DBG_IO, &mikasa_dev, "PIT%u latch\n", chan);
    return;
    }

if (mode >= 6)
    mode -= 4;
mikasa_pit[chan].access = access;
mikasa_pit[chan].mode = mode;
mikasa_pit[chan].bcd = (val & 1) != 0;
mikasa_pit[chan].write_state = 0;
mikasa_pit[chan].read_state = 0;
mikasa_pit[chan].null_count = TRUE;
mikasa_pit[chan].out = ((mode == MIKASA_PIT_MODE_TERMINAL) ||
    (mode == MIKASA_PIT_MODE_STROBE)) ? FALSE : TRUE;
sim_debug (MIKASA_DBG_IO, &mikasa_dev,
    "PIT%u control access=%u mode=%u bcd=%u\n",
    chan, access, mode, mikasa_pit[chan].bcd ? 1 : 0);
return;
}

static void mikasa_pit_write (uint32 port, uint8 val)
{
uint32 chan = port - MIKASA_ISA_PIT;

if (chan == 3) {
    mikasa_pit_control (val);
    return;
    }
if (chan < MIKASA_PIT_CHANNELS)
    mikasa_pit_write_counter (chan, val);
return;
}

static void mikasa_pit_tick (void)
{
uint32 delta = mikasa_pit_delta ();
uint32 chan;

for (chan = 0; chan < MIKASA_PIT_CHANNELS; chan++) {
    MIKASA_PIT_CHANNEL *pit = &mikasa_pit[chan];
    uint32 remain;

    if (!pit->running || !pit->gate || (pit->reload == 0))
        continue;
    if (delta < pit->counter) {
        pit->counter -= delta;
        continue;
        }
    remain = delta - pit->counter;
    pit->out = TRUE;
    if (chan == 0) {
        sim_debug (MIKASA_DBG_IO, &mikasa_dev,
            "PIT0 IRQ reload=%u remain=%u\n", pit->reload, remain);
        mikasa_pic_set_irq (MIKASA_PIT_IRQ, TRUE);
        mikasa_pic_set_irq (MIKASA_PIT_IRQ, FALSE);
        }
    if ((pit->mode == MIKASA_PIT_MODE_TERMINAL) ||
        (pit->mode == MIKASA_PIT_MODE_ONESHOT) ||
        (pit->mode == MIKASA_PIT_MODE_STROBE) ||
        (pit->mode == MIKASA_PIT_MODE_HWSTROBE)) {
        pit->running = FALSE;
        pit->counter = 0;
        continue;
        }
    if (pit->reload != 0)
        remain = remain % pit->reload;
    pit->counter = pit->reload - remain;
    if (pit->counter == 0)
        pit->counter = pit->reload;
    }
return;
}

static void mikasa_pit_set_gate (uint32 chan, t_bool state)
{
MIKASA_PIT_CHANNEL *pit;
t_bool old;

if (chan >= MIKASA_PIT_CHANNELS)
    return;
pit = &mikasa_pit[chan];
old = pit->gate;
pit->gate = state;
if (!state) {
    pit->running = FALSE;
    return;
    }
if (!old && !pit->null_count) {
    if ((pit->mode == MIKASA_PIT_MODE_ONESHOT) ||
        (pit->mode == MIKASA_PIT_MODE_HWSTROBE)) {
        pit->counter = pit->reload;
        pit->out = FALSE;
        }
    pit->running = TRUE;
    }
return;
}

static uint8 mikasa_portb_read (void)
{
uint8 val = mikasa_portb & ~(uint8) MIKASA_PORTB_OUT2;

if (mikasa_pit[2].out)
    val |= MIKASA_PORTB_OUT2;
sim_debug (MIKASA_DBG_IO, &mikasa_dev, "PORTB read %02X\n", val);
return val;
}

static void mikasa_portb_write (uint8 val)
{
mikasa_portb = val & (MIKASA_PORTB_GATE2 | MIKASA_PORTB_SPKR);
mikasa_pit_set_gate (2, (val & MIKASA_PORTB_GATE2) != 0);
sim_debug (MIKASA_DBG_IO, &mikasa_dev, "PORTB write %02X\n", val);
return;
}

static uint8 mikasa_ocp_read (uint32 reg)
{
uint8 val;

reg = reg & 3;
if (reg == 0) {
    val = mikasa_ocp_ddram[mikasa_ocp_addr & MIKASA_OCP_ADDR_MASK];
    mikasa_ocp_addr = (mikasa_ocp_addr + 1) & MIKASA_OCP_ADDR_MASK;
    }
else if (reg == 1) {
    val = 0;                                                /* busy clear */
    if (mikasa_ocp_busy_count != 0) {
        val |= MIKASA_OCP_BUSY;
        mikasa_ocp_busy_count--;
        }
    if (mikasa_halt_pending)
        val |= MIKASA_OCP_HALT;
    }
else
    val = mikasa_ocp_reg[reg];
sim_debug (MIKASA_DBG_IO, &mikasa_dev,
    "OCP read pc=%llX ra=%llX %u=%02X addr=%02X halt=%c\n",
    (unsigned long long) PC, (unsigned long long) R[26],
    reg, val, mikasa_ocp_addr, mikasa_halt_pending ? 'y' : 'n');
return val;
}

static void mikasa_ocp_write (uint32 reg, uint8 val)
{
reg = reg & 3;
mikasa_ocp_reg[reg] = val;
if (reg == 0) {
    mikasa_ocp_ddram[mikasa_ocp_addr & MIKASA_OCP_ADDR_MASK] = val;
    mikasa_ocp_addr = (mikasa_ocp_addr + 1) & MIKASA_OCP_ADDR_MASK;
    mikasa_ocp_busy_count = 1;
    }
else if (reg == 1) {
    if (val & 0x80)
        mikasa_ocp_addr = val & MIKASA_OCP_ADDR_MASK;
    else if (val & 0x40)
        mikasa_ocp_addr = val & MIKASA_OCP_ADDR_MASK;
    else if (val == 0x01) {
        memset (mikasa_ocp_ddram, ' ', sizeof (mikasa_ocp_ddram));
        mikasa_ocp_addr = 0;
        }
    else if (val == 0x02)
        mikasa_ocp_addr = 0;
    mikasa_ocp_busy_count = 2;
    }
sim_debug (MIKASA_DBG_IO, &mikasa_dev,
    "OCP write pc=%llX ra=%llX %u=%02X addr=%02X\n",
    (unsigned long long) PC, (unsigned long long) R[26],
    reg, val, mikasa_ocp_addr);
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

static void mikasa_epic_tbia (void)
{
uint32 i;

for (i = 0; i < MIKASA_EPIC_TLB_ENTRIES; i++) {
    mikasa_epic_reg[(MIKASA_EPIC_TLB_TAG_0 >> 5) + i] = 0;
    mikasa_epic_reg[(MIKASA_EPIC_TLB_DATA_0 >> 5) + i] = 0;
    }
return;
}

static void mikasa_epic_latch_pci_error (uint32 cause, t_uint64 pa)
{
uint32 dcsr_index = MIKASA_EPIC_DCSR >> 5;
uint32 dcsr = mikasa_epic_reg[dcsr_index];

if ((dcsr & MIKASA_EPIC_DCSR_ERR) != 0)
    cause |= MIKASA_EPIC_DCSR_LOST;
mikasa_epic_reg[dcsr_index] = dcsr | cause | MIKASA_EPIC_DCSR_PASS2;
mikasa_epic_reg[MIKASA_EPIC_PEAR >> 5] = (uint32) pa;
mikasa_epic_reg[MIKASA_EPIC_SEAR >> 5] =
    ((uint32) pa) & MIKASA_EPIC_SEAR_ERR;
sim_debug (MIKASA_DBG_PCI, &mikasa_dev,
    "EPIC latch PCI error cause=%08X pa=%llX\n", cause,
    (unsigned long long) pa);
return;
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
uint32 reg = off & (MIKASA_EPIC_SIZE - 1);
uint32 index = reg >> 5;
uint32 data = (uint32) val;

switch (reg) {
    case MIKASA_EPIC_DCSR:
        data = (data & MIKASA_EPIC_DCSR_CTL) |
            ((mikasa_epic_reg[index] & MIKASA_EPIC_DCSR_ERR) &
            ~data) | MIKASA_EPIC_DCSR_PASS2;
        break;

    case MIKASA_EPIC_SEAR:
        data = data & MIKASA_EPIC_SEAR_ERR;
        break;

    case MIKASA_EPIC_TBASE_1:
    case MIKASA_EPIC_TBASE_2:
        data = data & MIKASA_EPIC_TBASE_MASK;
        break;

    case MIKASA_EPIC_PCI_BASE_1:
    case MIKASA_EPIC_PCI_BASE_2:
        data = data & (MIKASA_EPIC_PCI_BASE_MASK |
            MIKASA_EPIC_PCI_BASE_SGEN | MIKASA_EPIC_PCI_BASE_WENB);
        break;

    case MIKASA_EPIC_PCI_MASK_1:
    case MIKASA_EPIC_PCI_MASK_2:
        data = data & MIKASA_EPIC_PCI_MASK_MASK;
        break;

    case MIKASA_EPIC_HAXR0:
        break;

    case MIKASA_EPIC_HAXR1:
        data = data & MIKASA_EPIC_HAXR1_EADDR;
        break;

    case MIKASA_EPIC_HAXR2:
        data = data & (MIKASA_EPIC_HAXR2_CONF_TYPE |
            MIKASA_EPIC_HAXR2_EADDR);
        break;

    case MIKASA_EPIC_PMLT:
        data = data & MIKASA_EPIC_PMLT_PMLC;
        break;

    case MIKASA_EPIC_TBIA:
        mikasa_epic_tbia ();
        data = 0;
        break;
        }
if ((reg >= MIKASA_EPIC_TLB_TAG_0) &&
    (reg < (MIKASA_EPIC_TLB_TAG_0 +
    (MIKASA_EPIC_TLB_ENTRIES << 5))))
    data = data & MIKASA_EPIC_TLB_TAG_MASK;
else if ((reg >= MIKASA_EPIC_TLB_DATA_0) &&
    (reg < (MIKASA_EPIC_TLB_DATA_0 +
    (MIKASA_EPIC_TLB_ENTRIES << 5))))
    data = data & MIKASA_EPIC_TLB_DATA_PAGE;
mikasa_epic_reg[index] = data;
sim_debug (MIKASA_DBG_PCI, &mikasa_dev, "EPIC write %03X=%08X\n",
    off, data);
return;
}

static t_uint64 mikasa_comanche_read (t_uint64 pa, uint32 lnt)
{
uint32 off = (uint32) (pa - MIKASA_COMANCHE_BASE);
uint32 index = (off & (MIKASA_COMANCHE_SIZE - 1)) >> 5;
uint32 val = mikasa_comanche_reg[index];

sim_debug (MIKASA_DBG_PCI, &mikasa_dev, "COMANCHE read %03X=%08X\n",
    off, val);
return val;
}

static t_bool mikasa_comanche_bank_reg (uint32 reg, uint32 base,
    uint32 *bank)
{
if ((reg < base) || (reg >= (base + (MIKASA_COMANCHE_BANKS << 5))))
    return FALSE;
*bank = (reg - base) >> 5;
return TRUE;
}

static void mikasa_comanche_write (t_uint64 pa, t_uint64 val, uint32 lnt)
{
uint32 off = (uint32) (pa - MIKASA_COMANCHE_BASE);
uint32 reg = off & (MIKASA_COMANCHE_SIZE - 1);
uint32 index = (off & (MIKASA_COMANCHE_SIZE - 1)) >> 5;
uint32 data = (uint32) val;
uint32 bank;

switch (reg) {
    case MIKASA_COMANCHE_GCR:
        data = data & MIKASA_COMANCHE_GCR_MASK;
        break;

    case MIKASA_COMANCHE_ED:
        data = (mikasa_comanche_reg[index] &
            ~(data & MIKASA_COMANCHE_ED_ERR)) |
            MIKASA_COMANCHE_ED_PASS2;
        break;

    case MIKASA_COMANCHE_TAGENB:
        data = data & MIKASA_COMANCHE_TAGENB_MASK;
        break;

    case MIKASA_COMANCHE_ERR_HI:
    case MIKASA_COMANCHE_LCK_HI:
        data = data & MIKASA_COMANCHE_ADDR_HI;
        break;

    case MIKASA_COMANCHE_VFP:
        data = data & MIKASA_COMANCHE_VFP_MASK;
        break;

    case MIKASA_COMANCHE_PD_LO:
    case MIKASA_COMANCHE_PD_HI:
        data = mikasa_comanche_reg[index];
        break;

    default:
        if (mikasa_comanche_bank_reg (reg, MIKASA_COMANCHE_B0_BAR, &bank))
            data = data & MIKASA_COMANCHE_BAR_MASK;
        else if (mikasa_comanche_bank_reg (reg, MIKASA_COMANCHE_B0_CR,
            &bank))
            data = data & ((bank == 8) ? MIKASA_COMANCHE_CR_S8_MASK :
                MIKASA_COMANCHE_CR_S0_MASK);
        else if (mikasa_comanche_bank_reg (reg, MIKASA_COMANCHE_B0_TRA,
            &bank))
            data = data & MIKASA_COMANCHE_TRA_MASK;
        else if (mikasa_comanche_bank_reg (reg, MIKASA_COMANCHE_B0_TRB,
            &bank))
            data = data & MIKASA_COMANCHE_TRB_MASK;
        break;
        }
mikasa_comanche_reg[index] = data;
sim_debug (MIKASA_DBG_PCI, &mikasa_dev, "COMANCHE write %03X=%08X\n",
    off, data);
return;
}

static void mikasa_comanche_init_regs (void)
{
uint32 bank0_size;
uint32 bank;

memset (mikasa_comanche_reg, 0, sizeof (mikasa_comanche_reg));
mikasa_comanche_reg[MIKASA_COMANCHE_GCR >> 5] = 0;
mikasa_comanche_reg[MIKASA_COMANCHE_ED >> 5] = MIKASA_COMANCHE_ED_PASS2;
mikasa_comanche_reg[MIKASA_COMANCHE_TAGENB >> 5] = 0;
mikasa_comanche_reg[MIKASA_COMANCHE_ERR_LO >> 5] = 0;
mikasa_comanche_reg[MIKASA_COMANCHE_ERR_HI >> 5] = 0;
mikasa_comanche_reg[MIKASA_COMANCHE_LCK_LO >> 5] = 0;
mikasa_comanche_reg[MIKASA_COMANCHE_LCK_HI >> 5] = 0;
mikasa_comanche_reg[MIKASA_COMANCHE_GTIM >> 5] = 0;
mikasa_comanche_reg[MIKASA_COMANCHE_RTIM >> 5] = 0;
mikasa_comanche_reg[MIKASA_COMANCHE_VFP >> 5] = 0;
mikasa_comanche_reg[MIKASA_COMANCHE_PD_LO >> 5] = 0;
mikasa_comanche_reg[MIKASA_COMANCHE_PD_HI >> 5] = 0;

for (bank = 0; bank < MIKASA_COMANCHE_BANKS; bank++) {
    mikasa_comanche_reg[(MIKASA_COMANCHE_B0_BAR >> 5) + bank] = 0;
    mikasa_comanche_reg[(MIKASA_COMANCHE_B0_CR >> 5) + bank] = 0;
    mikasa_comanche_reg[(MIKASA_COMANCHE_B0_TRA >> 5) + bank] = 0;
    mikasa_comanche_reg[(MIKASA_COMANCHE_B0_TRB >> 5) + bank] = 0;
    }
bank0_size = (MEMSIZE >= 0x40000000u) ? MIKASA_COMANCHE_CR_SIZE_1G : 0;
mikasa_comanche_reg[MIKASA_COMANCHE_B0_BAR >> 5] = 0;
mikasa_comanche_reg[MIKASA_COMANCHE_B0_CR >> 5] =
    MIKASA_COMANCHE_CR_VALID | bank0_size;
mikasa_comanche_reg[MIKASA_COMANCHE_B0_TRA >> 5] = 0;
mikasa_comanche_reg[MIKASA_COMANCHE_B0_TRB >> 5] = 0;
return;
}

static t_bool mikasa_pceb_bar_reg (uint32 reg)
{
return ((reg >= 0x10) && (reg <= 0x24)) || (reg == 0x30);
}

static uint32 mikasa_pceb_conf_mask (uint32 reg)
{
switch (reg) {
    case 0x04:
        return 0x00000157u;

    case 0x0C:
        return 0x0000FFFFu;

    case 0x3C:
        return 0x000000FFu;

    case 0x40:
        return M32;

    default:
        return ((reg >= 0x44) && (reg < 0x80)) ? M32 : 0;
        }
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

static uint32 mikasa_tulip_conf_mask (uint32 reg)
{
switch (reg) {
    case 0x04:
        return 0x00000157u;

    case 0x0C:
        return 0x0000FFFFu;

    case 0x10:
    case 0x14:
        return MIKASA_TULIP_BAR_MASK;

    case 0x3C:
        return 0x000000FFu;

    default:
        return 0;
        }
}

static t_bool mikasa_vga_bar_reg (uint32 addr, uint32 bar, uint32 mask,
    uint32 *reg)
{
uint32 base = mikasa_vga_cfg[bar >> 2] & mask;

if (base == 0)
    return FALSE;
if ((addr >= base) && (addr < (base + 0x04000000u))) {
    *reg = addr - base;
    return TRUE;
    }
return FALSE;
}

static uint32 mikasa_apecs_sparse_mem_addr (t_uint64 off)
{
uint32 addr = (uint32) ((off >> 5) & 0x07FFFFFFu);

if (addr >= MIKASA_APECS_HAE_REG1)
    addr |= mikasa_epic_reg[MIKASA_EPIC_HAXR1 >> 5] &
        MIKASA_EPIC_HAXR1_EADDR;
return addr;
}

static t_bool mikasa_vga_rom_reg (uint32 addr, uint32 *reg)
{
uint32 base = mikasa_vga_cfg[0x30 >> 2] & MIKASA_VGA_ROM_MASK;

if ((mikasa_vga_cfg[0x30 >> 2] & 1) == 0)
    return FALSE;
if (base == 0)
    return FALSE;
if ((addr >= base) && (addr < (base + MIKASA_VGA_ROM_SIZE))) {
    *reg = addr - base;
    return TRUE;
    }
return FALSE;
}

static uint32 mikasa_vga_conf_mask (uint32 reg)
{
switch (reg) {
    case 0x04:
        return 0x00000157u;

    case 0x0C:
        return 0x0000FFFFu;

    case 0x10:
        return MIKASA_VGA_FB_MASK;

    case 0x30:
        return MIKASA_VGA_ROM_MASK | 1u;

    case 0x3C:
        return 0x000000FFu;

    default:
        return 0;
        }
}

static t_bool mikasa_tulip_bar_reg (uint32 addr, uint32 bar, uint32 mask,
    uint32 *reg)
{
uint32 base;

base = mikasa_tulip_cfg[bar >> 2] & ~0x7Fu;
base = base & mask;
if (base == 0)
    return FALSE;
if ((addr < base) || (addr >= (base + MIKASA_TULIP_BAR_SIZE)))
    return FALSE;
*reg = addr - base;
return TRUE;
}

static uint8 mikasa_tulip_read_b (uint32 reg)
{
uint8 val;

reg = reg & (MIKASA_TULIP_REG_SIZE - 1);
if (reg == MIKASA_TULIP_CSR_SIO) {
    val = mikasa_tulip_enetrom[mikasa_tulip_enetrom_idx &
        (MIKASA_TULIP_ENETROM_BYTES - 1)];
    if (mikasa_tulip_enetrom_idx < (MIKASA_TULIP_ENETROM_BYTES - 1))
        mikasa_tulip_enetrom_idx++;
    }
else
    val = mikasa_tulip_reg[reg];

sim_debug (MIKASA_DBG_PCI, &mikasa_dev, "TULIP read %02X=%02X\n",
    reg, val);
return val;
}

static uint32 mikasa_tulip_read_l (uint32 reg)
{
reg = reg & (MIKASA_TULIP_REG_SIZE - 1);
return ((uint32) mikasa_tulip_read_b (reg)) |
    (((uint32) mikasa_tulip_read_b (reg + 1)) << 8) |
    (((uint32) mikasa_tulip_read_b (reg + 2)) << 16) |
    (((uint32) mikasa_tulip_read_b (reg + 3)) << 24);
}

static uint32 mikasa_tulip_reg_l (uint32 reg)
{
reg = reg & (MIKASA_TULIP_REG_SIZE - 1);
return ((uint32) mikasa_tulip_reg[reg]) |
    (((uint32) mikasa_tulip_reg[(reg + 1) &
    (MIKASA_TULIP_REG_SIZE - 1)]) << 8) |
    (((uint32) mikasa_tulip_reg[(reg + 2) &
    (MIKASA_TULIP_REG_SIZE - 1)]) << 16) |
    (((uint32) mikasa_tulip_reg[(reg + 3) &
    (MIKASA_TULIP_REG_SIZE - 1)]) << 24);
}

static void mikasa_tulip_set_l (uint32 reg, uint32 val)
{
reg = reg & (MIKASA_TULIP_REG_SIZE - 1);
mikasa_tulip_reg[reg] = (uint8) val;
mikasa_tulip_reg[(reg + 1) & (MIKASA_TULIP_REG_SIZE - 1)] =
    (uint8) (val >> 8);
mikasa_tulip_reg[(reg + 2) & (MIKASA_TULIP_REG_SIZE - 1)] =
    (uint8) (val >> 16);
mikasa_tulip_reg[(reg + 3) & (MIKASA_TULIP_REG_SIZE - 1)] =
    (uint8) (val >> 24);
return;
}

static void mikasa_tulip_update_irq (void)
{
uint32 status = mikasa_tulip_reg_l (MIKASA_TULIP_CSR_STATUS);
uint32 intr = mikasa_tulip_reg_l (MIKASA_TULIP_CSR_INTR);

if ((status & intr & MIKASA_TULIP_STS_IRQS) != 0)
    mikasa_irq_summary |= MIKASA_TULIP_INTBIT;
else
    mikasa_irq_summary &= ~MIKASA_TULIP_INTBIT;
mikasa_irq_update ();
return;
}

static void mikasa_tulip_srom_reset_state (void)
{
mikasa_tulip_srom_shift = 0;
mikasa_tulip_srom_bits = 0;
mikasa_tulip_srom_out = 0;
mikasa_tulip_srom_out_bits = 0;
return;
}

static void mikasa_tulip_init_srom (void)
{
static const uint8 mac[6] = { 0x08, 0x00, 0x2B, 0x00, 0x00, 0x01 };
static const uint8 testpat[8] = { 0xFF, 0x00, 0x55, 0xAA,
    0xFF, 0x00, 0x55, 0xAA };
uint32 cksum;

memset (mikasa_tulip_srom, 0xFF, sizeof (mikasa_tulip_srom));
memcpy (&mikasa_tulip_srom[20], mac, sizeof (mac));
memset (mikasa_tulip_enetrom, 0, sizeof (mikasa_tulip_enetrom));
memcpy (&mikasa_tulip_enetrom[0], mac, sizeof (mac));
cksum = ((uint32) mikasa_tulip_enetrom[0]) |
    (((uint32) mikasa_tulip_enetrom[1]) << 8);
cksum = cksum * 2;
if (cksum > 65535)
    cksum = cksum - 65535;
cksum = cksum + (((uint32) mikasa_tulip_enetrom[2]) |
    (((uint32) mikasa_tulip_enetrom[3]) << 8));
if (cksum > 65535)
    cksum = cksum - 65535;
cksum = cksum * 2;
if (cksum > 65535)
    cksum = cksum - 65535;
cksum = cksum + (((uint32) mikasa_tulip_enetrom[4]) |
    (((uint32) mikasa_tulip_enetrom[5]) << 8));
if (cksum >= 65535)
    cksum = cksum - 65535;
mikasa_tulip_enetrom[6] = (uint8) cksum;
mikasa_tulip_enetrom[7] = (uint8) (cksum >> 8);
mikasa_tulip_enetrom[8] = mikasa_tulip_enetrom[7];
mikasa_tulip_enetrom[9] = mikasa_tulip_enetrom[6];
mikasa_tulip_enetrom[10] = mikasa_tulip_enetrom[5];
mikasa_tulip_enetrom[11] = mikasa_tulip_enetrom[4];
mikasa_tulip_enetrom[12] = mikasa_tulip_enetrom[3];
mikasa_tulip_enetrom[13] = mikasa_tulip_enetrom[2];
mikasa_tulip_enetrom[14] = mikasa_tulip_enetrom[1];
mikasa_tulip_enetrom[15] = mikasa_tulip_enetrom[0];
memcpy (&mikasa_tulip_enetrom[24], testpat, sizeof (testpat));
mikasa_tulip_enetrom_idx = 0;
mikasa_tulip_srom_reset_state ();
return;
}

static uint16 mikasa_tulip_srom_word (uint32 addr)
{
uint32 off = (addr << 1) & (MIKASA_TULIP_SROM_BYTES - 1);

return ((uint16) mikasa_tulip_srom[off]) |
    (((uint16) mikasa_tulip_srom[(off + 1) &
    (MIKASA_TULIP_SROM_BYTES - 1)]) << 8);
}

static void mikasa_tulip_write_sio (uint32 val)
{
uint32 old = mikasa_tulip_read_l (MIKASA_TULIP_CSR_SIO);
uint32 sio = (val & ~MIKASA_TULIP_SIO_EE_DATAOUT) |
    (old & MIKASA_TULIP_SIO_EE_DATAOUT);
t_bool enabled = ((sio & (MIKASA_TULIP_SIO_EESEL |
    MIKASA_TULIP_SIO_ROM_READ | MIKASA_TULIP_SIO_EE_CS)) ==
    (MIKASA_TULIP_SIO_EESEL | MIKASA_TULIP_SIO_ROM_READ |
    MIKASA_TULIP_SIO_EE_CS));

if (!enabled) {
    mikasa_tulip_srom_reset_state ();
    mikasa_tulip_set_l (MIKASA_TULIP_CSR_SIO,
        sio & ~MIKASA_TULIP_SIO_EE_DATAOUT);
    return;
    }
if (((old & MIKASA_TULIP_SIO_EE_CLK) == 0) &&
    ((sio & MIKASA_TULIP_SIO_EE_CLK) != 0)) {
    if (mikasa_tulip_srom_out_bits != 0) {
        uint32 bit = (mikasa_tulip_srom_out >>
            (mikasa_tulip_srom_out_bits - 1)) & 1u;

        if (bit)
            sio |= MIKASA_TULIP_SIO_EE_DATAOUT;
        else
            sio &= ~MIKASA_TULIP_SIO_EE_DATAOUT;
        mikasa_tulip_srom_out_bits--;
        }
    else {
        mikasa_tulip_srom_shift =
            (mikasa_tulip_srom_shift << 1) |
            ((sio & MIKASA_TULIP_SIO_EE_DATAIN) ? 1u : 0);
        mikasa_tulip_srom_bits++;
        if (mikasa_tulip_srom_bits ==
            (3 + MIKASA_TULIP_SROM_ADDR_BITS)) {
            uint32 opcode = (mikasa_tulip_srom_shift >>
                MIKASA_TULIP_SROM_ADDR_BITS) & 7u;
            uint32 addr = mikasa_tulip_srom_shift &
                ((1u << MIKASA_TULIP_SROM_ADDR_BITS) - 1);

            mikasa_tulip_srom_out = (opcode == MIKASA_TULIP_SROM_READ) ?
                mikasa_tulip_srom_word (addr) : 0xFFFFu;
            mikasa_tulip_srom_out_bits = 16;
            mikasa_tulip_srom_shift = 0;
            mikasa_tulip_srom_bits = 0;
            }
        }
    }
mikasa_tulip_set_l (MIKASA_TULIP_CSR_SIO, sio);
return;
}

static t_bool mikasa_tulip_dma_write_l (uint32 addr, uint32 val)
{
return mikasa_pci_dma_write_byte (addr, (uint8) val) &&
    mikasa_pci_dma_write_byte (addr + 1, (uint8) (val >> 8)) &&
    mikasa_pci_dma_write_byte (addr + 2, (uint8) (val >> 16)) &&
    mikasa_pci_dma_write_byte (addr + 3, (uint8) (val >> 24));
}

static uint32 mikasa_tulip_next_desc (uint32 desc, uint32 ctl, uint32 addr2,
    uint32 base)
{
uint32 flags = ctl >> 22;

if (flags & MIKASA_TULIP_DFLAG_CHAIN)
    return addr2 & ~3u;
if (flags & MIKASA_TULIP_DFLAG_ENDRING)
    return base;
return desc + 16 + mikasa_tulip_desc_skip;
}

static void mikasa_tulip_process_tx (void)
{
uint32 status = mikasa_tulip_read_l (MIKASA_TULIP_CSR_STATUS);
uint32 desc = mikasa_tulip_tx_cur ? mikasa_tulip_tx_cur : mikasa_tulip_tx_base;
uint32 i;
t_bool completed = FALSE;

if (desc == 0)
    return;
for (i = 0; i < 16; i++) {
    uint32 dstat;
    uint32 ctl;
    uint32 addr2;
    uint32 flags;

    if (!mikasa_pci_dma_read_long (desc, &dstat) ||
        !mikasa_pci_dma_read_long (desc + 4, &ctl) ||
        !mikasa_pci_dma_read_long (desc + 12, &addr2))
        break;
    if ((dstat & MIKASA_TULIP_DESC_OWNER) == 0)
        break;
    flags = ctl >> 22;
    if (!mikasa_tulip_dma_write_l (desc, 0))
        break;
    completed = TRUE;
    sim_debug (MIKASA_DBG_PCI, &mikasa_dev,
        "TULIP TX desc %08X flags=%03X\n", desc, flags);
    desc = mikasa_tulip_next_desc (desc, ctl, addr2, mikasa_tulip_tx_base);
    if (desc == 0)
        break;
    }
mikasa_tulip_tx_cur = desc;
if (completed)
    status |= MIKASA_TULIP_STS_TXINTR | MIKASA_TULIP_STS_NORMAL;
mikasa_tulip_set_l (MIKASA_TULIP_CSR_STATUS, status);
mikasa_tulip_update_irq ();
return;
}

static void mikasa_tulip_update_state (void)
{
uint32 cmd = mikasa_tulip_read_l (MIKASA_TULIP_CSR_CMD);
uint32 status = mikasa_tulip_read_l (MIKASA_TULIP_CSR_STATUS);

status = status & ~(MIKASA_TULIP_STS_TXS_MASK | MIKASA_TULIP_STS_RXS_MASK);
status |= (cmd & MIKASA_TULIP_CMD_TXRUN) ? MIKASA_TULIP_STS_TXS_SUSP :
    MIKASA_TULIP_STS_TXS_STOP;
if (cmd & MIKASA_TULIP_CMD_RXRUN)
    status |= (mikasa_tulip_rx_base != 0) ? MIKASA_TULIP_STS_RXS_WAIT :
        MIKASA_TULIP_STS_RXS_SUSP;
else
    status |= MIKASA_TULIP_STS_RXS_STOP;
mikasa_tulip_set_l (MIKASA_TULIP_CSR_STATUS, status);
mikasa_tulip_update_irq ();
return;
}

static void mikasa_tulip_reset_regs (void)
{
memset (mikasa_tulip_reg, 0, sizeof (mikasa_tulip_reg));
mikasa_tulip_srom_reset_state ();
mikasa_tulip_enetrom_idx = 0;
mikasa_tulip_tx_base = 0;
mikasa_tulip_tx_cur = 0;
mikasa_tulip_rx_base = 0;
mikasa_tulip_rx_cur = 0;
mikasa_tulip_desc_skip = 0;
mikasa_tulip_set_l (MIKASA_TULIP_CSR_STATUS,
    MIKASA_TULIP_STS_TXS_STOP | MIKASA_TULIP_STS_RXS_STOP);
mikasa_tulip_update_irq ();
return;
}

static t_uint64 mikasa_tulip_read_len (uint32 reg, uint32 lnt)
{
if (lnt == L_BYTE)
    return mikasa_tulip_read_b (reg);
if (lnt == L_WORD)
    return ((uint32) mikasa_tulip_read_b (reg)) |
        (((uint32) mikasa_tulip_read_b (reg + 1)) << 8);
return mikasa_tulip_read_l (reg);
}

static void mikasa_tulip_write_b (uint32 reg, uint8 val)
{
reg = reg & (MIKASA_TULIP_REG_SIZE - 1);
mikasa_tulip_reg[reg] = val;
sim_debug (MIKASA_DBG_PCI, &mikasa_dev, "TULIP write %02X=%02X\n",
    reg, val);
return;
}

static void mikasa_tulip_write_l (uint32 reg, uint32 val)
{
reg = reg & (MIKASA_TULIP_REG_SIZE - 1);
sim_debug (MIKASA_DBG_PCI, &mikasa_dev, "TULIP write %02X=%08X\n",
    reg, val);
switch (reg) {
    case MIKASA_TULIP_CSR_BUSMODE:
        if (val & MIKASA_TULIP_BUSMODE_SWR) {
            mikasa_tulip_reset_regs ();
            return;
            }
        mikasa_tulip_desc_skip = ((val >> 2) & 0x1Fu) * 4;
        mikasa_tulip_set_l (reg, val);
        break;

    case MIKASA_TULIP_CSR_STATUS:
        mikasa_tulip_set_l (reg, mikasa_tulip_read_l (reg) &
            ~(val & MIKASA_TULIP_STS_W1C));
        mikasa_tulip_update_irq ();
        break;

    case MIKASA_TULIP_CSR_CMD:
        mikasa_tulip_set_l (reg, val);
        if (val & MIKASA_TULIP_CMD_TXRUN)
            mikasa_tulip_process_tx ();
        mikasa_tulip_update_state ();
        break;

    case MIKASA_TULIP_CSR_INTR:
        mikasa_tulip_set_l (reg, val);
        mikasa_tulip_update_irq ();
        break;

    case MIKASA_TULIP_CSR_SIO:
        mikasa_tulip_enetrom_idx = 0;
        mikasa_tulip_set_l (reg, val & 0x7FFFFFFFu);
        mikasa_tulip_srom_reset_state ();
        break;

    case MIKASA_TULIP_CSR_TXLIST:
        mikasa_tulip_tx_base = val & ~3u;
        mikasa_tulip_tx_cur = mikasa_tulip_tx_base;
        mikasa_tulip_set_l (reg, mikasa_tulip_tx_base);
        break;

    case MIKASA_TULIP_CSR_RXLIST:
        mikasa_tulip_rx_base = val & ~3u;
        mikasa_tulip_rx_cur = mikasa_tulip_rx_base;
        mikasa_tulip_set_l (reg, mikasa_tulip_rx_base);
        mikasa_tulip_update_state ();
        break;

    case MIKASA_TULIP_CSR_TXPOLL:
        mikasa_tulip_process_tx ();
        mikasa_tulip_update_state ();
        break;

    case MIKASA_TULIP_CSR_RXPOLL:
        mikasa_tulip_update_state ();
        break;

    default:
        mikasa_tulip_set_l (reg, val);
        break;
        }
return;
}

static void mikasa_tulip_write_len (uint32 reg, t_uint64 val, uint32 lnt)
{
if (lnt == L_BYTE) {
    uint32 base = reg & ~3u;
    uint32 shift = 8 * (reg & 3u);
    uint32 data = (mikasa_tulip_read_l (base) & ~(0xFFu << shift)) |
        ((((uint32) val) & 0xFFu) << shift);

    mikasa_tulip_write_l (base, data);
    }
else if (lnt == L_WORD) {
    uint32 base = reg & ~3u;
    uint32 shift = 8 * (reg & 2u);
    uint32 data = (mikasa_tulip_read_l (base) & ~(0xFFFFu << shift)) |
        ((((uint32) val) & 0xFFFFu) << shift);

    mikasa_tulip_write_l (base, data);
    }
else
    mikasa_tulip_write_l (reg, (uint32) val);
return;
}

static t_bool mikasa_tulip_io_read (uint32 port, uint8 *val)
{
uint32 reg;

if (!mikasa_tulip_bar_reg (port, 0x10, 0xFFFFFFFFu, &reg))
    return FALSE;
*val = mikasa_tulip_read_b (reg);
return TRUE;
}

static t_bool mikasa_tulip_io_write (uint32 port, uint8 val)
{
uint32 reg;

if (!mikasa_tulip_bar_reg (port, 0x10, 0xFFFFFFFFu, &reg))
    return FALSE;
mikasa_tulip_write_len (reg, val, L_BYTE);
return TRUE;
}

static uint8 mikasa_ncr_read_b (uint32 reg)
{
uint8 val;

reg = reg & 0x7Fu;
val = mikasa_ncr_reg[reg];
if (reg == MIKASA_NCR_REG_DSTAT) {
    mikasa_ncr_reg[MIKASA_NCR_REG_DSTAT] &= MIKASA_NCR_DSTAT_DFE;
    mikasa_ncr_reg[MIKASA_NCR_REG_ISTAT] &= ~MIKASA_NCR_ISTAT_DIP;
    mikasa_ncr_promote_interrupt_stack ();
    mikasa_ncr_update_irq ();
    mikasa_ncr_disc_log ("NCRDISC reg DSTAT=%02X pc=%llX pra=%llX\n", val,
        (unsigned long long) PC,
        (unsigned long long) mikasa_ncr_parent_frame_ra ());
    }
else if ((reg == MIKASA_NCR_REG_SIST0) || (reg == MIKASA_NCR_REG_SIST1)) {
    mikasa_ncr_reg[reg] = 0;
    if ((mikasa_ncr_reg[MIKASA_NCR_REG_SIST0] == 0) &&
        (mikasa_ncr_reg[MIKASA_NCR_REG_SIST1] == 0)) {
        mikasa_ncr_reg[MIKASA_NCR_REG_ISTAT] &= ~MIKASA_NCR_ISTAT_SIP;
        mikasa_ncr_promote_interrupt_stack ();
        }
    mikasa_ncr_update_irq ();
    mikasa_ncr_disc_log ("NCRDISC reg %s=%02X pc=%llX pra=%llX\n",
        (reg == MIKASA_NCR_REG_SIST0) ? "SIST0" : "SIST1", val,
        (unsigned long long) PC,
        (unsigned long long) mikasa_ncr_parent_frame_ra ());
    }
else if (reg == MIKASA_NCR_REG_DFIFO)
    val = mikasa_ncr_reg[MIKASA_NCR_REG_DBC] & 0x7F;
else if (reg == MIKASA_NCR_REG_CTEST2) {
    val = (uint8) ((val & ~MIKASA_NCR_CTEST2_SIGP) |
        ((mikasa_ncr_reg[MIKASA_NCR_REG_ISTAT] & MIKASA_NCR_ISTAT_SIGP) ?
        MIKASA_NCR_CTEST2_SIGP : 0));
    mikasa_ncr_reg[MIKASA_NCR_REG_ISTAT] &= ~MIKASA_NCR_ISTAT_SIGP;
    }
if (sim_deb && (mikasa_dev.dctrl & MIKASA_DBG_PCI))
    sim_debug (MIKASA_DBG_PCI, &mikasa_dev,
        "NCR read pc=%llX ra=%llX fra=%llX pra=%llX %02X=%02X\n",
        (unsigned long long) PC, (unsigned long long) R[26],
        (unsigned long long) mikasa_ncr_frame_ra (),
        (unsigned long long) mikasa_ncr_parent_frame_ra (), reg, val);
if ((reg == MIKASA_NCR_REG_DSPS) || (reg == (MIKASA_NCR_REG_DSPS + 1)) ||
    (reg == (MIKASA_NCR_REG_DSPS + 2)) || (reg == (MIKASA_NCR_REG_DSPS + 3)) ||
    (reg == MIKASA_NCR_REG_SFBR))
    mikasa_ncr_disc_log ("NCRDISC reg %02X=%02X pc=%llX pra=%llX\n", reg, val,
        (unsigned long long) PC,
        (unsigned long long) mikasa_ncr_parent_frame_ra ());
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

static uint32 mikasa_ncr_reg_l (uint32 reg)
{
reg = reg & (MIKASA_NCR_REG_SIZE - 1);
return ((uint32) mikasa_ncr_reg[reg]) |
    (((uint32) mikasa_ncr_reg[(reg + 1) & (MIKASA_NCR_REG_SIZE - 1)]) << 8) |
    (((uint32) mikasa_ncr_reg[(reg + 2) & (MIKASA_NCR_REG_SIZE - 1)]) << 16) |
    (((uint32) mikasa_ncr_reg[(reg + 3) & (MIKASA_NCR_REG_SIZE - 1)]) << 24);
}

static t_uint64 mikasa_ncr_frame_ra (void)
{
return mikasa_debug_frame_ra ();
}

static t_uint64 mikasa_ncr_parent_frame_ra (void)
{
return mikasa_debug_parent_frame_ra ();
}

static void mikasa_ncr_disc_log (const char *fmt, ...)
{
va_list arglist;

if (!sim_deb || ((mikasa_dev.dctrl & MIKASA_DBG_NCRDISC) == 0))
    return;
va_start (arglist, fmt);
_sim_vdebug (MIKASA_DBG_NCRDISC, &mikasa_dev, NULL, fmt, arglist);
va_end (arglist);
return;
}

static void mikasa_ncr_disc_dump_bytes (const char *tag, uint32 addr,
    uint32 len)
{
uint32 i;
uint32 max = (len > 64) ? 64 : len;
char text[3 * 64 + 1];
uint8 val;

if (!sim_deb || ((mikasa_dev.dctrl & MIKASA_DBG_NCRDISC) == 0))
    return;
for (i = 0; i < max; i++) {
    if (!mikasa_pci_dma_read_byte (addr + i, &val)) {
        mikasa_ncr_disc_log ("NCRDISC %s addr=%08X len=%u dma-read-failed "
            "at +%u\n", tag, addr, len, i);
        return;
        }
    (void) snprintf (&text[i * 3], sizeof (text) - (i * 3), "%02X%s",
        val, (i + 1 == max) ? "" : " ");
    }
mikasa_ncr_disc_log ("NCRDISC %s addr=%08X len=%u dump=%s%s\n", tag, addr,
    len, text, (len > max) ? " ..." : "");
return;
}

static void mikasa_ncr_disc_dump_list (const char *tag,
    const MIKASA_NCR_DMA_LIST *list)
{
uint32 i;

if ((list == NULL) || !sim_deb ||
    ((mikasa_dev.dctrl & MIKASA_DBG_NCRDISC) == 0))
    return;
mikasa_ncr_disc_log ("NCRDISC %s list count=%u bytes=%u\n", tag,
    list->count, list->bytes);
for (i = 0; i < list->count; i++) {
    mikasa_ncr_disc_log ("NCRDISC %s seg%u op=%08X count=%u addr=%08X\n",
        tag, i, list->seg[i].op, list->seg[i].count, list->seg[i].addr);
    if (list->seg[i].count != 0)
        mikasa_ncr_disc_dump_bytes (tag, list->seg[i].addr,
            list->seg[i].count);
    }
return;
}

static void mikasa_ncr_disc_dump_dsa (const char *tag)
{
static const uint32 offs[] = { 0x00, 0x08, 0x0C, 0x14, 0x1C, 0x24, 0x2C, 0x34 };
uint32 dsa = mikasa_ncr_reg_l (MIKASA_NCR_REG_DSA);
uint32 i;

if (!sim_deb || ((mikasa_dev.dctrl & MIKASA_DBG_NCRDISC) == 0))
    return;
mikasa_ncr_disc_log ("NCRDISC %s regs pc=%llX ra=%llX fra=%llX pra=%llX "
    "dsp=%08X dsa=%08X dnad=%08X dbc=%08X dsps=%08X istat=%02X "
    "dstat=%02X sist=%02X/%02X sien=%02X/%02X sbcl=%02X sstat=%02X/%02X/%02X "
    "scntl=%02X/%02X/%02X/%02X sfbr=%02X ssid=%02X sdid=%02X sxfer=%02X\n",
    tag, (unsigned long long) PC, (unsigned long long) R[26],
    (unsigned long long) mikasa_ncr_frame_ra (),
    (unsigned long long) mikasa_ncr_parent_frame_ra (),
    mikasa_ncr_reg_l (MIKASA_NCR_REG_DSP),
    dsa,
    mikasa_ncr_reg_l (MIKASA_NCR_REG_DNAD),
    mikasa_ncr_reg_l (MIKASA_NCR_REG_DBC),
    mikasa_ncr_reg_l (MIKASA_NCR_REG_DSPS),
    mikasa_ncr_reg[MIKASA_NCR_REG_ISTAT],
    mikasa_ncr_reg[MIKASA_NCR_REG_DSTAT],
    mikasa_ncr_reg[MIKASA_NCR_REG_SIST0],
    mikasa_ncr_reg[MIKASA_NCR_REG_SIST1],
    mikasa_ncr_reg[MIKASA_NCR_REG_SIEN0],
    mikasa_ncr_reg[MIKASA_NCR_REG_SIEN1],
    mikasa_ncr_reg[MIKASA_NCR_REG_SBCL],
    mikasa_ncr_reg[MIKASA_NCR_REG_SSTAT0],
    mikasa_ncr_reg[MIKASA_NCR_REG_SSTAT1],
    mikasa_ncr_reg[MIKASA_NCR_REG_SSTAT2],
    mikasa_ncr_reg[MIKASA_NCR_REG_SCNTL0],
    mikasa_ncr_reg[MIKASA_NCR_REG_SCNTL1],
    mikasa_ncr_reg[MIKASA_NCR_REG_SCNTL2],
    mikasa_ncr_reg[MIKASA_NCR_REG_SCNTL3],
    mikasa_ncr_reg[MIKASA_NCR_REG_SFBR],
    mikasa_ncr_reg[MIKASA_NCR_REG_SSID],
    mikasa_ncr_reg[MIKASA_NCR_REG_SDID],
    mikasa_ncr_reg[MIKASA_NCR_REG_SXFER]);
for (i = 0; i < sizeof (offs) / sizeof (offs[0]); i++) {
    uint32 count;
    uint32 addr;

    if (mikasa_ncr_table_entry (dsa, offs[i], &count, &addr))
        mikasa_ncr_disc_log ("NCRDISC %s dsa+%02X count=%08X addr=%08X\n",
            tag, offs[i], count, addr);
    else
        mikasa_ncr_disc_log ("NCRDISC %s dsa+%02X unreadable\n", tag,
            offs[i]);
    }
return;
}

static void mikasa_ncr_disc_track_cmd (uint32 target, uint32 lun,
    const uint8 *cdb)
{
uint32 cdb0 = cdb ? cdb[0] : 0xFF;

if ((cdb0 == 0x12) && (target == 0) && (lun == 0)) {
    if (mikasa_ncr_disc_pass_armed)
        mikasa_ncr_disc_pass = mikasa_ncr_disc_pass + 1;
    mikasa_ncr_disc_pass_armed = TRUE;
    mikasa_ncr_disc_log (
        "NCRDISC pass=%u restart pc=%llX ra=%llX fra=%llX pra=%llX\n",
        mikasa_ncr_disc_pass, (unsigned long long) PC,
        (unsigned long long) R[26],
        (unsigned long long) mikasa_ncr_frame_ra (),
        (unsigned long long) mikasa_ncr_parent_frame_ra ());
    if (mikasa_ncr_disc_pass <= 8)
        mikasa_ncr_disc_dump_dsa ("restart");
    }
if ((mikasa_ncr_disc_last_target != target) ||
    (mikasa_ncr_disc_last_lun != lun) ||
    (mikasa_ncr_disc_last_cdb != cdb0)) {
    mikasa_ncr_disc_log ("NCRDISC pass=%u cmd tgt=%u lun=%u cdb=%02X "
        "dsp=%08X dsa=%08X dsps=%08X dbc=%08X\n", mikasa_ncr_disc_pass,
        target, lun, cdb0, mikasa_ncr_reg_l (MIKASA_NCR_REG_DSP),
        mikasa_ncr_reg_l (MIKASA_NCR_REG_DSA),
        mikasa_ncr_reg_l (MIKASA_NCR_REG_DSPS),
        mikasa_ncr_reg_l (MIKASA_NCR_REG_DBC));
    mikasa_ncr_disc_last_target = target;
    mikasa_ncr_disc_last_lun = lun;
    mikasa_ncr_disc_last_cdb = cdb0;
    }
return;
}

static void mikasa_ncr_disc_track_timeout (uint32 target)
{
if (target == 6)
    mikasa_ncr_disc_pass_armed = TRUE;
mikasa_ncr_disc_log ("NCRDISC pass=%u timeout tgt=%u sist1=%02X dstat=%02X "
    "dsps=%08X\n", mikasa_ncr_disc_pass, target,
    mikasa_ncr_reg[MIKASA_NCR_REG_SIST1], mikasa_ncr_reg[MIKASA_NCR_REG_DSTAT],
    mikasa_ncr_reg_l (MIKASA_NCR_REG_DSPS));
return;
}

static void mikasa_ncr_disc_track_completion (uint32 target, uint32 lun,
    const uint8 *cdb, const MIKASA_NCR_DMA_LIST *data, uint8 status,
    uint32 dsps, uint32 resid)
{
mikasa_ncr_disc_log ("NCRDISC pass=%u done tgt=%u lun=%u st=%02X dsps=%u "
    "resid=%u sbcl=%02X sstat2=%02X con=%c\n", mikasa_ncr_disc_pass, target,
    lun, status, dsps, resid, mikasa_ncr_reg[MIKASA_NCR_REG_SBCL],
    mikasa_ncr_reg[MIKASA_NCR_REG_SSTAT2],
    (mikasa_ncr_reg[MIKASA_NCR_REG_ISTAT] & MIKASA_NCR_ISTAT_CON) ? 'y' : 'n');
if ((target == 0) && (lun == 0) && (cdb != NULL) && (cdb[0] == 0x12) &&
    (mikasa_ncr_disc_pass <= 8)) {
    mikasa_ncr_disc_dump_dsa ("inq-complete");
    mikasa_ncr_disc_dump_list ("inq-data", data);
    }
return;
}

static void mikasa_ncr_debug_state (const char *tag)
{
if (!sim_deb || ((mikasa_dev.dctrl & MIKASA_DBG_PCI) == 0))
    return;
sim_debug (MIKASA_DBG_PCI, &mikasa_dev,
    "NCR state %s pc=%llX ra=%llX fra=%llX pra=%llX dsp=%08X dsps=%08X "
    "dsa=%08X dbc=%08X dnad=%08X istat=%02X dstat=%02X sist=%02X/%02X "
    "sien=%02X/%02X sbcl=%02X socl=%02X sstat=%02X/%02X/%02X "
    "scntl=%02X/%02X/%02X trans=%c pend=%c tgt=%u lun=%u wait=%c\n",
    tag ? tag : "", (unsigned long long) PC, (unsigned long long) R[26],
    (unsigned long long) mikasa_ncr_frame_ra (),
    (unsigned long long) mikasa_ncr_parent_frame_ra (),
    mikasa_ncr_reg_l (MIKASA_NCR_REG_DSP),
    mikasa_ncr_reg_l (MIKASA_NCR_REG_DSPS),
    mikasa_ncr_reg_l (MIKASA_NCR_REG_DSA),
    mikasa_ncr_reg_l (MIKASA_NCR_REG_DBC),
    mikasa_ncr_reg_l (MIKASA_NCR_REG_DNAD),
    mikasa_ncr_reg[MIKASA_NCR_REG_ISTAT],
    mikasa_ncr_reg[MIKASA_NCR_REG_DSTAT],
    mikasa_ncr_reg[MIKASA_NCR_REG_SIST0],
    mikasa_ncr_reg[MIKASA_NCR_REG_SIST1],
    mikasa_ncr_reg[MIKASA_NCR_REG_SIEN0],
    mikasa_ncr_reg[MIKASA_NCR_REG_SIEN1],
    mikasa_ncr_reg[MIKASA_NCR_REG_SBCL],
    mikasa_ncr_reg[MIKASA_NCR_REG_SOCL],
    mikasa_ncr_reg[MIKASA_NCR_REG_SSTAT0],
    mikasa_ncr_reg[MIKASA_NCR_REG_SSTAT1],
    mikasa_ncr_reg[MIKASA_NCR_REG_SSTAT2],
    mikasa_ncr_reg[MIKASA_NCR_REG_SCNTL0],
    mikasa_ncr_reg[MIKASA_NCR_REG_SCNTL1],
    mikasa_ncr_reg[MIKASA_NCR_REG_SCNTL2],
    mikasa_ncr_transaction.active ? 'y' : 'n',
    mikasa_ncr_transaction.status_pending ? 'y' : 'n',
    mikasa_ncr_transaction.target, mikasa_ncr_transaction.lun,
    mikasa_ncr_wait_reselect ? 'y' : 'n');
return;
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

static const char *mikasa_ncr_script_name (uint32 op)
{
switch (op & 0xF8F80000u) {
    case 0x80080000u:
        return "JUMP";

    case 0x80880000u:
        return "JUMPR";

    case 0x88080000u:
        return "CALL";

    case 0x88880000u:
        return "CALLR";

    case 0x90080000u:
        return "RETURN";

    case 0x98080000u:
        return "INT";

    case 0x98180000u:
        return "INTF";
        }

switch (op & 0xF8000000u) {
    case 0x80000000u:
        return "JUMP";

    case 0x88000000u:
        return "CALL";

    case 0x90000000u:
        return "RETURN";

    case 0x98000000u:
        return "INT";
        }

switch (op & 0xFF000000u) {
    case 0x40000000u:
        return "SEL_ABS";

    case 0x41000000u:
        return "SEL_ABS_ATN";

    case 0x42000000u:
        return "SEL_TBL";

    case 0x43000000u:
        return "SEL_TBL_ATN";

    case 0x48000000u:
        return "WAIT_DISC";

    case 0x50000000u:
        return "WAIT_RESEL";

    case 0x58000000u:
        return "SET";

    case 0x60000000u:
        return "CLR";

    case 0x68000000u:
        return "SFBR_REG";

    case 0x70000000u:
        return "REG_SFBR";

    case 0x78000000u:
        return "REG_REG";
        }

switch (op & MIKASA_NCR_MM_GROUP_MASK) {
    case 0x00000000u:
        return "MOVE";

    case MIKASA_NCR_MM_GROUP:
        return "COPY";

    case MIKASA_NCR_LS_GROUP:
        return "LOAD_STORE";
        }

return "SCRIPT";
}

static void mikasa_ncr_trace_script (uint32 dsp)
{
t_uint64 pa;
uint32 i;

if (!sim_deb || (mikasa_ncr_script_trace_count >= MIKASA_NCR_TRACE_LIMIT))
    return;
mikasa_ncr_script_trace_count++;
if (!mikasa_pci_dma_addr_to_pa (dsp, &pa)) {
    sim_debug (MIKASA_DBG_PCI, &mikasa_dev,
        "NCR DSP %08X is not in a mapped DMA window\n", dsp);
    return;
    }
if (!ADDR_IS_MEM (pa + ((MIKASA_NCR_TRACE_INSNS * 12) - 1))) {
    sim_debug (MIKASA_DBG_PCI, &mikasa_dev,
        "NCR DSP %08X maps outside RAM at %llX\n", dsp,
        (unsigned long long) pa);
    return;
    }

sim_debug (MIKASA_DBG_PCI, &mikasa_dev,
    "NCR DSP %08X maps to PA %llX, DSA=%08X\n", dsp,
    (unsigned long long) pa, mikasa_ncr_reg_l (MIKASA_NCR_REG_DSA));
{
uint32 dsa = mikasa_ncr_reg_l (MIKASA_NCR_REG_DSA);
t_uint64 dsa_pa;
uint32 off;

if (mikasa_pci_dma_addr_to_pa (dsa, &dsa_pa))
    for (off = 0; off < 0x40; off += 8)
        sim_debug (MIKASA_DBG_PCI, &mikasa_dev,
            "NCR DSA +%02X: %08X %08X\n", off,
            mikasa_read_phys_long (dsa_pa + off),
            mikasa_read_phys_long (dsa_pa + off + 4));
}
for (i = 0; i < MIKASA_NCR_TRACE_INSNS; i++) {
    uint32 op = mikasa_read_phys_long (pa);
    uint32 arg = mikasa_read_phys_long (pa + 4);

    if ((op & MIKASA_NCR_MM_GROUP_MASK) == MIKASA_NCR_MM_GROUP)
        sim_debug (MIKASA_DBG_PCI, &mikasa_dev,
            "NCR SCRIPT %08llX: %-11s %08X %08X %08X\n",
            (unsigned long long) pa, mikasa_ncr_script_name (op), op, arg,
            mikasa_read_phys_long (pa + 8));
    else
        sim_debug (MIKASA_DBG_PCI, &mikasa_dev,
            "NCR SCRIPT %08llX: %-11s %08X %08X\n",
            (unsigned long long) pa, mikasa_ncr_script_name (op), op, arg);
    pa += ((op & MIKASA_NCR_MM_GROUP_MASK) == MIKASA_NCR_MM_GROUP) ? 12 : 8;
    }
return;
}

static void mikasa_ncr_set_reg_l (uint32 reg, uint32 val)
{
reg = reg & (MIKASA_NCR_REG_SIZE - 1);
mikasa_ncr_reg[reg] = (uint8) val;
mikasa_ncr_reg[(reg + 1) & (MIKASA_NCR_REG_SIZE - 1)] = (uint8) (val >> 8);
mikasa_ncr_reg[(reg + 2) & (MIKASA_NCR_REG_SIZE - 1)] = (uint8) (val >> 16);
mikasa_ncr_reg[(reg + 3) & (MIKASA_NCR_REG_SIZE - 1)] = (uint8) (val >> 24);
return;
}

static t_bool mikasa_pic_pending (void)
{
uint8 master = mikasa_pic_irr[0] & ~mikasa_pic_imr[0];
uint8 slave = mikasa_pic_irr[1] & ~mikasa_pic_imr[1];

if (slave && !(mikasa_pic_imr[0] & (1u << 2)))
    master |= 1u << 2;
return master != 0;
}

static uint8 mikasa_pic_iack (void)
{
uint8 master = mikasa_pic_irr[0] & ~mikasa_pic_imr[0];
uint8 slave = mikasa_pic_irr[1] & ~mikasa_pic_imr[1];
uint32 irq;

if (slave && !(mikasa_pic_imr[0] & (1u << 2)))
    master |= 1u << 2;
irq = mikasa_pic_first (master);
if (irq == 2) {
    uint32 sirq = mikasa_pic_first (slave);

    if (sirq < 8) {
        uint8 sbit = (uint8) (1u << sirq);

        mikasa_pic_irr[1] &= ~(1u << sirq);
        if (!mikasa_pic_auto_eoi[1])
            mikasa_pic_isr[1] |= sbit;
        else
            mikasa_pic_refresh_level (1, sbit);
        if (!mikasa_pic_auto_eoi[0])
            mikasa_pic_isr[0] |= 1u << 2;
        if (!(mikasa_pic_irr[1] & ~mikasa_pic_imr[1]))
            mikasa_pic_irr[0] &= ~(1u << 2);
        mikasa_irq_update ();
        sim_debug (MIKASA_DBG_IO, &mikasa_dev,
            "PIC IACK IRQ%u vector=%02X\n", sirq + 8,
            mikasa_pic_vec[1] + sirq);
        return mikasa_pic_vec[1] + sirq;
        }
    }
if (irq < 8) {
    uint8 bit = (uint8) (1u << irq);

    mikasa_pic_irr[0] &= ~(1u << irq);
    if (!mikasa_pic_auto_eoi[0])
        mikasa_pic_isr[0] |= bit;
    else
        mikasa_pic_refresh_level (0, bit);
    mikasa_irq_update ();
    sim_debug (MIKASA_DBG_IO, &mikasa_dev,
        "PIC IACK IRQ%u vector=%02X\n", irq, mikasa_pic_vec[0] + irq);
    return mikasa_pic_vec[0] + irq;
    }
sim_debug (MIKASA_DBG_IO, &mikasa_dev, "PIC IACK spurious\n");
return mikasa_pic_vec[0] + 7;
}

static void mikasa_pic_set_irq (uint32 irq, t_bool state)
{
uint32 which = (irq >= 8) ? 1 : 0;
uint8 bit = 1u << (irq & 7);
t_bool level = (mikasa_elcr[which] & bit) != 0;
t_bool was_asserted = (mikasa_pic_level[which] & bit) != 0;

if (state) {
    mikasa_pic_level[which] |= bit;
    if (level || !was_asserted)
        mikasa_pic_irr[which] |= bit;
    }
else {
    mikasa_pic_level[which] &= ~bit;
    if (level)
        mikasa_pic_irr[which] &= ~bit;
    }
mikasa_irq_update ();
sim_debug (MIKASA_DBG_IO, &mikasa_dev,
    "PIC%u IRQ%u %s irr=%02X isr=%02X imr=%02X elcr=%02X\n",
    which + 1, irq, state ? "set" : "clear", mikasa_pic_irr[which],
    mikasa_pic_isr[which], mikasa_pic_imr[which], mikasa_elcr[which]);
return;
}

static void mikasa_irq_update (void)
{
uint32 active = mikasa_irq_summary & mikasa_irq_mask;
uint32 pic = mikasa_pic_pending () ? 1u : 0;

int_req[MIKASA_ICU_HWRE_LEVEL] =
    (int_req[MIKASA_ICU_HWRE_LEVEL] & ~mikasa_icu_int_req_bits) | active;
mikasa_icu_int_req_bits = active;
int_req[MIKASA_PIC_HWRE_LEVEL] =
    (int_req[MIKASA_PIC_HWRE_LEVEL] & ~mikasa_pic_int_req_bits) | pic;
mikasa_pic_int_req_bits = pic;
sim_debug (MIKASA_DBG_IO, &mikasa_dev,
    "ICU update summary=%04X mask=%04X active=%04X pic=%u icu_hwre=%u pic_hwre=%u\n",
    mikasa_irq_summary, mikasa_irq_mask, active, pic, MIKASA_ICU_HWRE_LEVEL,
    MIKASA_PIC_HWRE_LEVEL);
return;
}

static void mikasa_ncr_assert_irq (void)
{
mikasa_irq_summary |= MIKASA_NCR_INTBIT;
mikasa_irq_update ();
return;
}

static void mikasa_ncr_clear_irq (void)
{
mikasa_irq_summary &= ~MIKASA_NCR_INTBIT;
mikasa_irq_update ();
return;
}

static void mikasa_ncr_update_irq (void)
{
t_bool dma_irq = (mikasa_ncr_reg[MIKASA_NCR_REG_ISTAT] &
    MIKASA_NCR_ISTAT_DIP) &&
    ((mikasa_ncr_reg[MIKASA_NCR_REG_DSTAT] &
    mikasa_ncr_reg[MIKASA_NCR_REG_DIEN] & MIKASA_NCR_DSTAT_IRQS) != 0);
t_bool scsi_irq = (mikasa_ncr_reg[MIKASA_NCR_REG_ISTAT] &
    MIKASA_NCR_ISTAT_SIP) &&
    (((mikasa_ncr_reg[MIKASA_NCR_REG_SIST0] &
    mikasa_ncr_reg[MIKASA_NCR_REG_SIEN0]) != 0) ||
    ((mikasa_ncr_reg[MIKASA_NCR_REG_SIST1] &
    mikasa_ncr_reg[MIKASA_NCR_REG_SIEN1]) != 0));
t_bool fly_irq = (mikasa_ncr_reg[MIKASA_NCR_REG_ISTAT] &
    MIKASA_NCR_ISTAT_INTF) != 0;

if (mikasa_ncr_reg[MIKASA_NCR_REG_DCNTL] & MIKASA_NCR_DCNTL_IRQD) {
    mikasa_ncr_clear_irq ();
    return;
    }
if (dma_irq || scsi_irq || fly_irq)
    mikasa_ncr_assert_irq ();
else
    mikasa_ncr_clear_irq ();
return;
}

static void mikasa_ncr_set_dip (uint8 dstat, uint32 dsps)
{
if (mikasa_ncr_reg[MIKASA_NCR_REG_ISTAT] & MIKASA_NCR_ISTAT_DIP) {
    mikasa_ncr_dstat_stack |= dstat;
    mikasa_ncr_dsps_stack = dsps;
    mikasa_ncr_dsps_stack_valid = TRUE;
    mikasa_ncr_update_irq ();
    mikasa_ncr_disc_log ("NCRDISC irq stack DIP dstat=%02X dsps=%u\n", dstat,
        dsps);
    sim_debug (MIKASA_DBG_PCI, &mikasa_dev,
        "NCR stack DIP dstat=%02X dsps=%u\n", dstat, dsps);
    mikasa_ncr_debug_state ("stack-dip");
    return;
    }
mikasa_ncr_set_reg_l (MIKASA_NCR_REG_DSPS, dsps);
mikasa_ncr_reg[MIKASA_NCR_REG_DSTAT] =
    (mikasa_ncr_reg[MIKASA_NCR_REG_DSTAT] & MIKASA_NCR_DSTAT_IRQS) |
    MIKASA_NCR_DSTAT_DFE | dstat;
mikasa_ncr_reg[MIKASA_NCR_REG_ISTAT] |= MIKASA_NCR_ISTAT_DIP;
mikasa_ncr_update_irq ();
mikasa_ncr_disc_log ("NCRDISC irq signal DIP dstat=%02X dsps=%u\n", dstat,
    dsps);
sim_debug (MIKASA_DBG_PCI, &mikasa_dev,
    "NCR signal DIP dstat=%02X dsps=%u\n", dstat, dsps);
mikasa_ncr_debug_state ("signal-dip");
return;
}

static void mikasa_ncr_set_sip (uint8 sist0, uint8 sist1)
{
if (mikasa_ncr_reg[MIKASA_NCR_REG_ISTAT] & MIKASA_NCR_ISTAT_SIP) {
    mikasa_ncr_sist0_stack |= sist0;
    mikasa_ncr_sist1_stack |= sist1;
    mikasa_ncr_update_irq ();
    mikasa_ncr_disc_log ("NCRDISC irq stack SIP sist0=%02X sist1=%02X\n",
        sist0, sist1);
    mikasa_ncr_debug_state ("stack-sip");
    return;
    }
mikasa_ncr_reg[MIKASA_NCR_REG_SIST0] |= sist0;
mikasa_ncr_reg[MIKASA_NCR_REG_SIST1] |= sist1;
mikasa_ncr_reg[MIKASA_NCR_REG_ISTAT] |= MIKASA_NCR_ISTAT_SIP;
mikasa_ncr_update_irq ();
mikasa_ncr_disc_log ("NCRDISC irq signal SIP sist0=%02X sist1=%02X\n", sist0,
    sist1);
mikasa_ncr_debug_state ("signal-sip");
return;
}

static void mikasa_ncr_promote_interrupt_stack (void)
{
if ((mikasa_ncr_reg[MIKASA_NCR_REG_ISTAT] &
    (MIKASA_NCR_ISTAT_DIP | MIKASA_NCR_ISTAT_SIP)) != 0)
    return;
if ((mikasa_ncr_dstat_stack == 0) && (mikasa_ncr_sist0_stack == 0) &&
    (mikasa_ncr_sist1_stack == 0))
    return;
if (mikasa_ncr_dstat_stack != 0) {
    if (mikasa_ncr_dsps_stack_valid)
        mikasa_ncr_set_reg_l (MIKASA_NCR_REG_DSPS, mikasa_ncr_dsps_stack);
    mikasa_ncr_reg[MIKASA_NCR_REG_DSTAT] =
        (mikasa_ncr_reg[MIKASA_NCR_REG_DSTAT] & MIKASA_NCR_DSTAT_DFE) |
        MIKASA_NCR_DSTAT_DFE | mikasa_ncr_dstat_stack;
    mikasa_ncr_reg[MIKASA_NCR_REG_ISTAT] |= MIKASA_NCR_ISTAT_DIP;
    }
if ((mikasa_ncr_sist0_stack != 0) || (mikasa_ncr_sist1_stack != 0)) {
    mikasa_ncr_reg[MIKASA_NCR_REG_SIST0] |= mikasa_ncr_sist0_stack;
    mikasa_ncr_reg[MIKASA_NCR_REG_SIST1] |= mikasa_ncr_sist1_stack;
    mikasa_ncr_reg[MIKASA_NCR_REG_ISTAT] |= MIKASA_NCR_ISTAT_SIP;
    }
mikasa_ncr_dstat_stack = 0;
mikasa_ncr_sist0_stack = 0;
mikasa_ncr_sist1_stack = 0;
mikasa_ncr_dsps_stack_valid = FALSE;
mikasa_ncr_update_irq ();
return;
}

static t_bool mikasa_ncr_table_entry (uint32 dsa, uint32 off, uint32 *count,
    uint32 *addr)
{
uint32 table = (dsa + off) & ~3u;

return mikasa_pci_dma_read_long (table, count) &&
    mikasa_pci_dma_read_long (table + 4, addr);
}

static t_bool mikasa_ncr_resolve_move (uint32 dsa, uint32 op, uint32 arg,
    uint32 *count, uint32 *addr)
{
if (op & MIKASA_NCR_BM_TABLE)
    return mikasa_ncr_table_entry (dsa, mikasa_ncr_sext24 (arg), count,
        addr);
*count = op & MIKASA_NCR_BM_COUNT_MASK;
if (op & MIKASA_NCR_BM_INDIRECT)
    return mikasa_pci_dma_read_long (arg, addr);
*addr = arg;
return TRUE;
}

static t_bool mikasa_ncr_read_dma_buf (uint32 addr, uint8 *buf, uint32 len)
{
uint32 i;

for (i = 0; i < len; i++)
    if (!mikasa_pci_dma_read_byte (addr + i, &buf[i]))
        return FALSE;
return TRUE;
}

static t_bool mikasa_ncr_write_dma_buf (uint32 addr, const uint8 *buf,
    uint32 len)
{
uint32 i;

for (i = 0; i < len; i++)
    if (!mikasa_pci_dma_write_byte (addr + i, buf[i]))
        return FALSE;
return TRUE;
}

static t_bool mikasa_ncr_write_dma_zero (uint32 addr, uint32 len)
{
uint32 i;

for (i = 0; i < len; i++)
    if (!mikasa_pci_dma_write_byte (addr + i, 0))
        return FALSE;
return TRUE;
}

static void mikasa_ncr_dma_list_init (MIKASA_NCR_DMA_LIST *list)
{
memset (list, 0, sizeof (*list));
return;
}

static uint32 mikasa_ncr_move_op_for_phase (uint32 phase)
{
return (phase << MIKASA_NCR_BM_PHASE_SHIFT) & MIKASA_NCR_BM_PHASE_MASK;
}

static uint32 mikasa_ncr_op_phase (uint32 op)
{
return (op & MIKASA_NCR_BM_PHASE_MASK) >> MIKASA_NCR_BM_PHASE_SHIFT;
}

static void mikasa_ncr_latch_phase (uint32 expected, uint32 sampled)
{
expected = expected & 7u;
sampled = sampled & 7u;
mikasa_ncr_reg[MIKASA_NCR_REG_SOCL] =
    (mikasa_ncr_reg[MIKASA_NCR_REG_SOCL] & ~7u) | expected;
mikasa_ncr_reg[MIKASA_NCR_REG_SBCL] =
    (mikasa_ncr_reg[MIKASA_NCR_REG_SBCL] & ~7u) | sampled;
mikasa_ncr_reg[MIKASA_NCR_REG_SSTAT1] =
    (mikasa_ncr_reg[MIKASA_NCR_REG_SSTAT1] & ~7u) | sampled;
return;
}

static void mikasa_ncr_set_connected (t_bool connected)
{
if (connected) {
    mikasa_ncr_reg[MIKASA_NCR_REG_SCNTL1] |= MIKASA_NCR_SCNTL1_ISCON;
    mikasa_ncr_reg[MIKASA_NCR_REG_ISTAT] |= MIKASA_NCR_ISTAT_CON;
    mikasa_ncr_reg[MIKASA_NCR_REG_SSTAT2] &= ~MIKASA_NCR_SSTAT2_LDSC;
    mikasa_ncr_reg[MIKASA_NCR_REG_SBCL] =
        (mikasa_ncr_reg[MIKASA_NCR_REG_SBCL] | MIKASA_NCR_BUS_BSY) &
        ~MIKASA_NCR_BUS_SEL;
    }
else {
    mikasa_ncr_reg[MIKASA_NCR_REG_SCNTL1] &= ~MIKASA_NCR_SCNTL1_ISCON;
    mikasa_ncr_reg[MIKASA_NCR_REG_ISTAT] &= ~MIKASA_NCR_ISTAT_CON;
    mikasa_ncr_reg[MIKASA_NCR_REG_SBCL] &=
        ~(MIKASA_NCR_BUS_REQ | MIKASA_NCR_BUS_ACK |
        MIKASA_NCR_BUS_BSY | MIKASA_NCR_BUS_SEL | MIKASA_NCR_BUS_ATN);
    }
return;
}

static void mikasa_ncr_write_scntl0 (uint8 val)
{
uint8 old = mikasa_ncr_reg[MIKASA_NCR_REG_SCNTL0];
uint8 newval = val & MIKASA_NCR_SCNTL0_WRITABLE;

mikasa_ncr_reg[MIKASA_NCR_REG_SCNTL0] = newval;
if ((newval & MIKASA_NCR_SCNTL0_START) &&
    ((old & MIKASA_NCR_SCNTL0_START) == 0)) {
    uint32 target = mikasa_ncr_reg[MIKASA_NCR_REG_SDID] &
        MIKASA_NCR_SDID_WRITABLE;

    mikasa_ncr_reg[MIKASA_NCR_REG_SSID] =
        MIKASA_NCR_SSID_VAL | (uint8) target;
    mikasa_ncr_reg[MIKASA_NCR_REG_SSTAT0] |= MIKASA_NCR_SSTAT0_WOA;
    if (newval & MIKASA_NCR_SCNTL0_WATN) {
        mikasa_ncr_reg[MIKASA_NCR_REG_SOCL] |= MIKASA_NCR_SOCL_ATN;
        mikasa_ncr_reg[MIKASA_NCR_REG_SBCL] |= MIKASA_NCR_SOCL_ATN;
        }
    else {
        mikasa_ncr_reg[MIKASA_NCR_REG_SOCL] &= ~MIKASA_NCR_SOCL_ATN;
        mikasa_ncr_reg[MIKASA_NCR_REG_SBCL] &= ~MIKASA_NCR_SOCL_ATN;
        }
    mikasa_ncr_reg[MIKASA_NCR_REG_SCNTL0] &= ~MIKASA_NCR_SCNTL0_START;
    if ((target < MIKASA_DKA_UNITS) &&
        ((dka_unit[target].flags & UNIT_ATT) != 0)) {
        mikasa_ncr_reg[MIKASA_NCR_REG_SCNTL2] |= MIKASA_NCR_SCNTL2_SDU;
        mikasa_ncr_set_connected (TRUE);
        mikasa_ncr_set_sip (MIKASA_NCR_SIST0_CMP, 0);
        }
    else {
        mikasa_ncr_reg[MIKASA_NCR_REG_SCNTL2] &= ~MIKASA_NCR_SCNTL2_SDU;
        mikasa_ncr_set_connected (FALSE);
        mikasa_ncr_set_sip (MIKASA_NCR_SIST0_UDC, MIKASA_NCR_SIST1_STO);
        }
    }
return;
}

static void mikasa_ncr_write_scntl1 (uint8 val)
{
uint8 old = mikasa_ncr_reg[MIKASA_NCR_REG_SCNTL1];
uint8 newval = (val & ~MIKASA_NCR_SCNTL1_ISCON) |
    (old & MIKASA_NCR_SCNTL1_ISCON);

if ((newval & MIKASA_NCR_SCNTL1_RST) &&
    ((old & MIKASA_NCR_SCNTL1_RST) == 0)) {
    mikasa_ncr_clear_transaction ();
    mikasa_ncr_clear_wait_reselect ();
    newval &= ~MIKASA_NCR_SCNTL1_ISCON;
    mikasa_ncr_reg[MIKASA_NCR_REG_ISTAT] &= ~MIKASA_NCR_ISTAT_CON;
    mikasa_ncr_reg[MIKASA_NCR_REG_SBCL] = 0;
    mikasa_ncr_reg[MIKASA_NCR_REG_SIDL] = 0;
    mikasa_ncr_reg[MIKASA_NCR_REG_SODL] = 0;
    mikasa_ncr_reg[MIKASA_NCR_REG_SBDL] = 0;
    mikasa_ncr_reg[MIKASA_NCR_REG_SSTAT0] |= MIKASA_NCR_SSTAT0_RST;
    mikasa_ncr_set_sip (MIKASA_NCR_SIST0_RST, 0);
    }
else if (((newval & MIKASA_NCR_SCNTL1_RST) == 0) &&
    (old & MIKASA_NCR_SCNTL1_RST))
    mikasa_ncr_reg[MIKASA_NCR_REG_SSTAT0] &= ~MIKASA_NCR_SSTAT0_RST;
mikasa_ncr_reg[MIKASA_NCR_REG_SCNTL1] = newval;
return;
}

static void mikasa_ncr_finish_move (uint32 op, uint32 addr, uint32 count,
    uint32 done)
{
uint32 resid = (done < count) ? count - done : 0;
uint32 phase = mikasa_ncr_op_phase (op);

mikasa_ncr_latch_phase (phase, phase);
mikasa_ncr_set_reg_l (MIKASA_NCR_REG_DBC,
    (op & 0xFF000000u) | (resid & MIKASA_NCR_BM_COUNT_MASK));
mikasa_ncr_set_reg_l (MIKASA_NCR_REG_DNAD, addr + done);
return;
}

static void mikasa_ncr_set_sfbr (uint8 val)
{
mikasa_ncr_reg[MIKASA_NCR_REG_SFBR] = val;
mikasa_ncr_reg[MIKASA_NCR_REG_SIDL] = val;
mikasa_ncr_reg[MIKASA_NCR_REG_SBDL] = val;
return;
}

static void mikasa_ncr_set_output_latch (uint8 val)
{
mikasa_ncr_reg[MIKASA_NCR_REG_SODL] = val;
mikasa_ncr_reg[MIKASA_NCR_REG_SBDL] = val;
return;
}

static void mikasa_ncr_signal_phase_mismatch (uint32 expected,
    uint32 sampled)
{
mikasa_ncr_latch_phase (expected, sampled);
mikasa_ncr_set_sip (MIKASA_NCR_SIST0_MIA, 0);
sim_debug (MIKASA_DBG_PCI, &mikasa_dev,
    "NCR phase mismatch expected=%u sampled=%u\n", expected, sampled);
mikasa_ncr_debug_state ("phase-mismatch");
return;
}

static void mikasa_ncr_phase_mismatch (const MIKASA_NCR_DMA_LIST *list,
    uint32 expected, uint32 sampled)
{
if (list->count != 0) {
    mikasa_ncr_finish_move (list->seg[0].op, list->seg[0].addr,
        list->seg[0].count, 0);
    expected = mikasa_ncr_op_phase (list->seg[0].op);
    }
mikasa_ncr_signal_phase_mismatch (expected, sampled);
return;
}

static t_bool mikasa_ncr_last_move_phase (uint32 phase)
{
return mikasa_ncr_op_phase (mikasa_ncr_reg_l (MIKASA_NCR_REG_DBC)) ==
    (phase & 7u);
}

static void mikasa_ncr_finish_dma_list (const MIKASA_NCR_DMA_LIST *list,
    uint32 bytes)
{
uint32 done = 0;
uint32 i;

for (i = 0; i < list->count; i++) {
    uint32 chunk = list->seg[i].count;

    if (bytes <= done)
        chunk = 0;
    else if (chunk > (bytes - done))
        chunk = bytes - done;
    mikasa_ncr_finish_move (list->seg[i].op, list->seg[i].addr,
        list->seg[i].count, chunk);
    done = done + chunk;
    if (done >= bytes)
        break;
    }
return;
}

static t_bool mikasa_ncr_dma_list_append (MIKASA_NCR_DMA_LIST *list,
    uint32 count, uint32 addr, uint32 op)
{
if (count == 0)
    return TRUE;
if (list->count >= MIKASA_NCR_DMA_SEGS)
    return FALSE;
list->seg[list->count].count = count;
list->seg[list->count].addr = addr;
list->seg[list->count].op = op;
list->count++;
list->bytes = ((M32 - list->bytes) < count) ? M32 : list->bytes + count;
return TRUE;
}

static t_bool mikasa_ncr_write_dma_list (const MIKASA_NCR_DMA_LIST *list,
    const uint8 *buf, uint32 len)
{
uint32 done = 0;
uint32 i;

for (i = 0; (i < list->count) && (done < len); i++) {
    uint32 chunk = list->seg[i].count;

    if (chunk > (len - done))
        chunk = len - done;
    if (sim_deb && (mikasa_dev.dctrl & MIKASA_DBG_PCI)) {
        t_uint64 pa;
        uint32 j;

        sim_debug (MIKASA_DBG_PCI, &mikasa_dev,
            "NCR DMA write seg=%u addr=%08X", i, list->seg[i].addr);
        if (mikasa_pci_dma_addr_to_pa (list->seg[i].addr, &pa))
            sim_debug (MIKASA_DBG_PCI, &mikasa_dev, " pa=%llX",
                (unsigned long long) pa);
        else
            sim_debug (MIKASA_DBG_PCI, &mikasa_dev, " pa=unmapped");
        sim_debug (MIKASA_DBG_PCI, &mikasa_dev, " len=%u data", chunk);
        for (j = 0; (j < chunk) && (j < 16); j++)
            sim_debug (MIKASA_DBG_PCI, &mikasa_dev, " %02X",
                buf[done + j]);
        sim_debug (MIKASA_DBG_PCI, &mikasa_dev, "%s\n",
            (chunk > 16) ? " ..." : "");
        }
    if (!mikasa_ncr_write_dma_buf (list->seg[i].addr, buf + done, chunk))
        return FALSE;
    done = done + chunk;
    }
return done == len;
}

static t_bool mikasa_ncr_write_scsi_data (const MIKASA_NCR_DMA_LIST *list,
    const uint8 *buf, uint32 len)
{
if (!mikasa_ncr_write_dma_list (list, buf, len))
    return FALSE;
if (len != 0)
    mikasa_ncr_set_sfbr (buf[len - 1]);
mikasa_ncr_finish_dma_list (list, len);
return TRUE;
}

static t_bool mikasa_ncr_write_dma_list_offset (
    const MIKASA_NCR_DMA_LIST *list, uint32 offset, const uint8 *buf,
    uint32 len)
{
uint32 done = 0;
uint32 i;

for (i = 0; (i < list->count) && (done < len); i++) {
    uint32 count = list->seg[i].count;
    uint32 addr = list->seg[i].addr;
    uint32 chunk;

    if (offset >= count) {
        offset = offset - count;
        continue;
        }
    addr = addr + offset;
    count = count - offset;
    offset = 0;
    chunk = count;
    if (chunk > (len - done))
        chunk = len - done;
    if (sim_deb && (mikasa_dev.dctrl & MIKASA_DBG_PCI)) {
        t_uint64 pa;
        uint32 j;

        sim_debug (MIKASA_DBG_PCI, &mikasa_dev,
            "NCR DMA write-offset seg=%u addr=%08X", i, addr);
        if (mikasa_pci_dma_addr_to_pa (addr, &pa))
            sim_debug (MIKASA_DBG_PCI, &mikasa_dev, " pa=%llX",
                (unsigned long long) pa);
        else
            sim_debug (MIKASA_DBG_PCI, &mikasa_dev, " pa=unmapped");
        sim_debug (MIKASA_DBG_PCI, &mikasa_dev, " len=%u data", chunk);
        for (j = 0; (j < chunk) && (j < 16); j++)
            sim_debug (MIKASA_DBG_PCI, &mikasa_dev, " %02X",
                buf[done + j]);
        sim_debug (MIKASA_DBG_PCI, &mikasa_dev, "%s\n",
            (chunk > 16) ? " ..." : "");
        }
    if (!mikasa_ncr_write_dma_buf (addr, buf + done, chunk))
        return FALSE;
    done = done + chunk;
    }
return done == len;
}

static t_bool mikasa_ncr_write_scsi_data_padded (
    const MIKASA_NCR_DMA_LIST *list, const uint8 *buf, uint32 valid_len,
    uint32 xfer_len)
{
uint8 zero[64];
uint32 done;

if (xfer_len > list->bytes)
    xfer_len = list->bytes;
if (valid_len > xfer_len)
    valid_len = xfer_len;
if (!mikasa_ncr_write_dma_list (list, buf, valid_len))
    return FALSE;
done = valid_len;
memset (zero, 0, sizeof (zero));
while (done < xfer_len) {
    uint32 chunk = xfer_len - done;

    if (chunk > sizeof (zero))
        chunk = sizeof (zero);
    if (!mikasa_ncr_write_dma_list_offset (list, done, zero, chunk))
        return FALSE;
    done = done + chunk;
    }
if (xfer_len != 0)
    mikasa_ncr_set_sfbr ((valid_len == xfer_len) && (valid_len != 0) ?
        buf[valid_len - 1] : 0);
mikasa_ncr_finish_dma_list (list, xfer_len);
return TRUE;
}

static t_bool mikasa_ncr_read_dma_list_offset (
    const MIKASA_NCR_DMA_LIST *list, uint32 offset, uint8 *buf, uint32 len)
{
uint32 done = 0;
uint32 i;

for (i = 0; (i < list->count) && (done < len); i++) {
    uint32 count = list->seg[i].count;
    uint32 addr = list->seg[i].addr;
    uint32 chunk;

    if (offset >= count) {
        offset = offset - count;
        continue;
        }
    addr = addr + offset;
    count = count - offset;
    offset = 0;
    chunk = count;
    if (chunk > (len - done))
        chunk = len - done;
    if (sim_deb && (mikasa_dev.dctrl & MIKASA_DBG_PCI)) {
        t_uint64 pa;

        sim_debug (MIKASA_DBG_PCI, &mikasa_dev,
            "NCR DMA read-offset seg=%u addr=%08X", i, addr);
        if (mikasa_pci_dma_addr_to_pa (addr, &pa))
            sim_debug (MIKASA_DBG_PCI, &mikasa_dev, " pa=%llX",
                (unsigned long long) pa);
        else
            sim_debug (MIKASA_DBG_PCI, &mikasa_dev, " pa=unmapped");
        sim_debug (MIKASA_DBG_PCI, &mikasa_dev, " len=%u\n", chunk);
        }
    if (!mikasa_ncr_read_dma_buf (addr, buf + done, chunk))
        return FALSE;
    done = done + chunk;
    }
return done == len;
}

static t_bool mikasa_ncr_discard_scsi_data (const MIKASA_NCR_DMA_LIST *list)
{
uint8 buf[MIKASA_IO_BUFSIZE];
uint32 done = 0;
t_bool latch = TRUE;

while (done < list->bytes) {
    uint32 chunk = list->bytes - done;

    if (chunk > sizeof (buf))
        chunk = sizeof (buf);
    if (!mikasa_ncr_read_dma_list_offset (list, done, buf, chunk))
        return FALSE;
    if (latch && (chunk != 0)) {
        mikasa_ncr_set_output_latch (buf[0]);
        latch = FALSE;
        }
    done = done + chunk;
    }
mikasa_ncr_finish_dma_list (list, done);
return TRUE;
}

static t_bool mikasa_ncr_write_dma_list_zero (
    const MIKASA_NCR_DMA_LIST *list)
{
uint32 i;

for (i = 0; i < list->count; i++)
    if (!mikasa_ncr_write_dma_zero (list->seg[i].addr, list->seg[i].count))
        return FALSE;
if (list->bytes != 0)
    mikasa_ncr_set_sfbr (0);
mikasa_ncr_finish_dma_list (list, list->bytes);
return TRUE;
}

static void mikasa_ncr_write_be32 (uint8 *buf, uint32 val)
{
buf[0] = (uint8) (val >> 24);
buf[1] = (uint8) (val >> 16);
buf[2] = (uint8) (val >> 8);
buf[3] = (uint8) val;
return;
}

static void mikasa_ncr_write_be64 (uint8 *buf, t_uint64 val)
{
mikasa_ncr_write_be32 (buf, (uint32) (val >> 32));
mikasa_ncr_write_be32 (buf + 4, (uint32) val);
return;
}

static void mikasa_ncr_write_be16 (uint8 *buf, uint32 val)
{
buf[0] = (uint8) (val >> 8);
buf[1] = (uint8) val;
return;
}

static void mikasa_ncr_write_be24 (uint8 *buf, uint32 val)
{
buf[0] = (uint8) (val >> 16);
buf[1] = (uint8) (val >> 8);
buf[2] = (uint8) val;
return;
}

static uint32 mikasa_ncr_min3 (uint32 a, uint32 b, uint32 c)
{
uint32 m = (a < b) ? a : b;

return (m < c) ? m : c;
}

static uint32 mikasa_ncr_alloc_len10 (const uint8 *cdb)
{
return (((uint32) cdb[7]) << 8) | cdb[8];
}

static uint32 mikasa_ncr_cdb_be32 (const uint8 *cdb, uint32 off)
{
return (((uint32) cdb[off]) << 24) | (((uint32) cdb[off + 1]) << 16) |
    (((uint32) cdb[off + 2]) << 8) | cdb[off + 3];
}

static t_uint64 mikasa_ncr_cdb_be64 (const uint8 *cdb, uint32 off)
{
return (((t_uint64) mikasa_ncr_cdb_be32 (cdb, off)) << 32) |
    mikasa_ncr_cdb_be32 (cdb, off + 4);
}

static void mikasa_ncr_clear_sense (uint32 unit)
{
if (unit < MIKASA_DKA_UNITS) {
    mikasa_ncr_sense_key[unit] = 0;
    mikasa_ncr_sense_asc[unit] = 0;
    mikasa_ncr_sense_ascq[unit] = 0;
    }
return;
}

static void mikasa_ncr_set_sense (uint32 unit, uint8 key, uint8 asc,
    uint8 ascq)
{
if (unit < MIKASA_DKA_UNITS) {
    mikasa_ncr_sense_key[unit] = key;
    mikasa_ncr_sense_asc[unit] = asc;
    mikasa_ncr_sense_ascq[unit] = ascq;
    }
return;
}

static uint32 mikasa_ncr_sext24 (uint32 val)
{
val = val & 0x00FFFFFFu;
return (val & 0x00800000u) ? (val | 0xFF000000u) : val;
}

static t_bool mikasa_ncr_fetch_script (uint32 dsp, uint32 *op, uint32 *arg,
    t_bool visible)
{
if (!mikasa_pci_dma_read_long (dsp, op) ||
    !mikasa_pci_dma_read_long (dsp + 4, arg))
    return FALSE;
if (!visible)
    return TRUE;
mikasa_ncr_set_reg_l (MIKASA_NCR_REG_DBC, *op);
mikasa_ncr_set_reg_l (MIKASA_NCR_REG_DSPS, *arg);
mikasa_ncr_set_reg_l (MIKASA_NCR_REG_DSP,
    mikasa_ncr_next_script_addr (dsp, *op));
return TRUE;
}

static t_bool mikasa_ncr_read_script (uint32 dsp, uint32 *op, uint32 *arg)
{
return mikasa_ncr_fetch_script (dsp, op, arg, TRUE);
}

static uint32 mikasa_ncr_next_script_addr (uint32 dsp, uint32 op)
{
return dsp + (((op & MIKASA_NCR_MM_GROUP_MASK) == MIKASA_NCR_MM_GROUP) ?
    12 : 8);
}

static t_bool mikasa_ncr_script_addr_reg (uint32 addr, uint32 *reg)
{
return mikasa_ncr_bar_reg (addr, 0x14, 0xFFFFFFFFu, reg) ||
    mikasa_ncr_bar_reg (addr, 0x10, 0xFFFFFFFFu, reg);
}

static t_bool mikasa_ncr_script_read_byte (uint32 addr, uint8 *val)
{
uint32 reg;

if (mikasa_ncr_script_addr_reg (addr, &reg)) {
    *val = mikasa_ncr_read_b (reg);
    return TRUE;
    }
return mikasa_pci_dma_read_byte (addr, val);
}

static t_bool mikasa_ncr_script_write_byte (uint32 addr, uint8 val)
{
uint32 reg;

if (mikasa_ncr_script_addr_reg (addr, &reg)) {
    mikasa_ncr_write_b (reg, val);
    return TRUE;
    }
return mikasa_pci_dma_write_byte (addr, val);
}

static t_bool mikasa_ncr_script_copy (uint32 src, uint32 dst, uint32 count)
{
uint32 i;

for (i = 0; i < count; i++) {
    uint8 val;

    if (!mikasa_ncr_script_read_byte (src + i, &val) ||
        !mikasa_ncr_script_write_byte (dst + i, val))
        return FALSE;
    }
return TRUE;
}

static uint8 mikasa_ncr_script_alu (uint8 left, uint8 right, uint32 op,
    MIKASA_NCR_SCRIPT_STATE *state)
{
uint32 val;

switch (op & MIKASA_NCR_ALU_OP_MASK) {
    case MIKASA_NCR_ALU_SHL:
        state->carry = (left & 0x80) ? TRUE : FALSE;
        return (uint8) (left << 1);

    case MIKASA_NCR_ALU_OR:
        return left | right;

    case MIKASA_NCR_ALU_XOR:
        return left ^ right;

    case MIKASA_NCR_ALU_AND:
        return left & right;

    case MIKASA_NCR_ALU_SHR:
        state->carry = (left & 1) ? TRUE : FALSE;
        return (uint8) (left >> 1);

    case MIKASA_NCR_ALU_ADD:
        val = ((uint32) left) + right;
        state->carry = (val > 0xFF) ? TRUE : FALSE;
        return (uint8) val;

    case MIKASA_NCR_ALU_ADDC:
        val = ((uint32) left) + right + (state->carry ? 1 : 0);
        state->carry = (val > 0xFF) ? TRUE : FALSE;
        return (uint8) val;

    case MIKASA_NCR_ALU_LOAD:
    default:
        return right;
        }
}

static void mikasa_ncr_script_store_reg (uint32 reg, uint8 val)
{
uint8 old;

reg = reg & (MIKASA_NCR_REG_SIZE - 1);
old = mikasa_ncr_reg[reg];
if ((reg == MIKASA_NCR_REG_DSTAT) ||
    ((reg >= MIKASA_NCR_REG_ADDER) &&
    (reg < (MIKASA_NCR_REG_ADDER + 4))) ||
    (reg == MIKASA_NCR_REG_SSID) ||
    (reg == MIKASA_NCR_REG_SBCL) ||
    (reg == MIKASA_NCR_REG_SSTAT0) ||
    (reg == MIKASA_NCR_REG_SSTAT1) ||
    (reg == MIKASA_NCR_REG_SSTAT2) ||
    (reg == MIKASA_NCR_REG_SIST0) ||
    (reg == MIKASA_NCR_REG_SIST1) ||
    (reg == MIKASA_NCR_REG_SIDL) ||
    (reg == (MIKASA_NCR_REG_SIDL + 1)) ||
    (reg == MIKASA_NCR_REG_SBDL) ||
    (reg == (MIKASA_NCR_REG_SBDL + 1)))
    return;
if (reg == MIKASA_NCR_REG_SODL) {
    mikasa_ncr_set_output_latch (val);
    return;
    }
if (reg == (MIKASA_NCR_REG_SODL + 1)) {
    mikasa_ncr_reg[reg] = val;
    mikasa_ncr_reg[MIKASA_NCR_REG_SBDL + 1] = val;
    return;
    }
if (reg == MIKASA_NCR_REG_SCNTL0) {
    mikasa_ncr_write_scntl0 (val);
    return;
    }
if (reg == MIKASA_NCR_REG_SCNTL1) {
    mikasa_ncr_write_scntl1 (val);
    return;
    }
if (reg == MIKASA_NCR_REG_SCNTL2)
    val = (old & MIKASA_NCR_SCNTL2_W1C &
        ~(val & MIKASA_NCR_SCNTL2_W1C)) |
        (val & MIKASA_NCR_SCNTL2_WRITABLE);
if (reg == MIKASA_NCR_REG_SCID)
    val = val & MIKASA_NCR_SCID_WRITABLE;
if (reg == MIKASA_NCR_REG_SCNTL3)
    val = val & MIKASA_NCR_SCNTL3_WRITABLE;
if (reg == MIKASA_NCR_REG_SDID)
    val = val & MIKASA_NCR_SDID_WRITABLE;
if (reg == MIKASA_NCR_REG_GPREG)
    val = val & MIKASA_NCR_GPREG_WRITABLE;
if (reg == MIKASA_NCR_REG_CTEST3) {
    mikasa_ncr_write_ctest3 (val);
    return;
    }
if (reg == MIKASA_NCR_REG_CTEST2)
    val = val & MIKASA_NCR_CTEST2_WRITABLE;
if (reg == MIKASA_NCR_REG_CTEST5) {
    mikasa_ncr_write_ctest5 (val);
    return;
    }
if (reg == MIKASA_NCR_REG_DMODE)
    val = val & MIKASA_NCR_DMODE_WRITABLE;
if (reg == MIKASA_NCR_REG_DIEN)
    val = val & MIKASA_NCR_DSTAT_IRQS;
if (reg == MIKASA_NCR_REG_DCNTL)
    val = (val & MIKASA_NCR_DCNTL_WRITABLE) & ~MIKASA_NCR_DCNTL_PFF;
if (reg == MIKASA_NCR_REG_SIEN1)
    val = val & MIKASA_NCR_SIEN1_WRITABLE;
if (reg == MIKASA_NCR_REG_MACNTL)
    val = val & MIKASA_NCR_MACNTL_WRITABLE;
if (reg == MIKASA_NCR_REG_GPCNTL)
    val = val & MIKASA_NCR_GPCNTL_WRITABLE;
if (reg == MIKASA_NCR_REG_STIME1)
    val = val & MIKASA_NCR_STIME1_WRITABLE;
if (reg == MIKASA_NCR_REG_STEST1)
    val = val & MIKASA_NCR_STEST1_WRITABLE;
if (reg == MIKASA_NCR_REG_STEST2)
    val = val & MIKASA_NCR_STEST2_WRITABLE;
if (reg == MIKASA_NCR_REG_STEST3) {
    mikasa_ncr_write_stest3 (val);
    return;
    }
mikasa_ncr_reg[reg] = val;
if (reg == MIKASA_NCR_REG_SOCL)
    mikasa_ncr_reg[MIKASA_NCR_REG_SBCL] =
        (mikasa_ncr_reg[MIKASA_NCR_REG_SBCL] &
        ~(MIKASA_NCR_SOCL_ACK | MIKASA_NCR_SOCL_ATN)) |
        (val & (MIKASA_NCR_SOCL_ACK | MIKASA_NCR_SOCL_ATN));
if ((reg == MIKASA_NCR_REG_DIEN) || (reg == MIKASA_NCR_REG_SIEN0) ||
    (reg == MIKASA_NCR_REG_SIEN1))
    mikasa_ncr_update_irq ();
return;
}

static void mikasa_ncr_script_reg_op (uint32 op,
    MIKASA_NCR_SCRIPT_STATE *state)
{
uint32 group = op & MIKASA_NCR_REGOP_GROUP_MASK;
uint32 reg = (op >> 16) & 0x7F;
uint8 data = (uint8) (op >> 8);
uint8 old = mikasa_ncr_reg[reg & (MIKASA_NCR_REG_SIZE - 1)];
uint8 val;

switch (group) {
    case MIKASA_NCR_SFBR_REG:
        val = mikasa_ncr_script_alu (state->sfbr, data, op, state);
        if (state->side_effects)
            mikasa_ncr_script_store_reg (reg, val);
        if ((reg & (MIKASA_NCR_REG_SIZE - 1)) == MIKASA_NCR_REG_SFBR)
            state->sfbr = val;
        break;

    case MIKASA_NCR_REG_SFBR_OP:
        state->sfbr = mikasa_ncr_script_alu (old, data, op, state);
        if (state->side_effects)
            mikasa_ncr_reg[MIKASA_NCR_REG_SFBR] = state->sfbr;
        break;

    case MIKASA_NCR_REG_REG:
        val = mikasa_ncr_script_alu (old, data, op, state);
        if (state->side_effects)
            mikasa_ncr_script_store_reg (reg, val);
        if ((reg & (MIKASA_NCR_REG_SIZE - 1)) == MIKASA_NCR_REG_SFBR)
            state->sfbr = val;
        break;
        }
return;
}

static t_bool mikasa_ncr_script_load_store (uint32 op, uint32 arg,
    MIKASA_NCR_SCRIPT_STATE *state)
{
uint32 reg = (op >> 16) & 0x7F;
uint32 count = op & 7;
uint32 memaddr = (op & MIKASA_NCR_LS_DSA_REL) ?
    mikasa_ncr_reg_l (MIKASA_NCR_REG_DSA) + mikasa_ncr_sext24 (arg) : arg;
uint32 regaddr;
uint32 i;

if (mikasa_ncr_script_addr_reg (memaddr, &regaddr)) {
    if (state->side_effects)
        mikasa_ncr_set_dip (MIKASA_NCR_DSTAT_IID, arg);
    return FALSE;
    }
if ((((memaddr ^ reg) & 3u) != 0) || (count > 4) ||
    (((memaddr & 3u) + count) > 4)) {
    if (state->side_effects)
        mikasa_ncr_set_dip (MIKASA_NCR_DSTAT_IID, arg);
    return FALSE;
    }
for (i = 0; i < count; i++) {
    uint32 r = (reg + i) & (MIKASA_NCR_REG_SIZE - 1);
    uint8 val;

    if (op & MIKASA_NCR_LS_LOAD) {
        if (!mikasa_pci_dma_read_byte (memaddr + i, &val))
            return FALSE;
        if (r == MIKASA_NCR_REG_SFBR)
            continue;
        if (state->side_effects)
            mikasa_ncr_script_store_reg (r, val);
        }
    else {
        val = mikasa_ncr_reg[r];
        if (state->side_effects &&
            !mikasa_pci_dma_write_byte (memaddr + i, val))
            return FALSE;
        }
    }
return TRUE;
}

static void mikasa_ncr_script_set_clear (uint32 op, t_bool set,
    MIKASA_NCR_SCRIPT_STATE *state)
{
uint8 socl = mikasa_ncr_reg[MIKASA_NCR_REG_SOCL];
uint8 scntl0 = mikasa_ncr_reg[MIKASA_NCR_REG_SCNTL0];
uint8 mask = 0;

if (op & MIKASA_NCR_SCR_CARRY)
    state->carry = set;
if (!state->side_effects)
    return;
if (op & MIKASA_NCR_SCR_ACK)
    mask |= MIKASA_NCR_SOCL_ACK;
if (op & MIKASA_NCR_SCR_ATN)
    mask |= MIKASA_NCR_SOCL_ATN;
if (set)
    socl |= mask;
else
    socl &= ~mask;
mikasa_ncr_reg[MIKASA_NCR_REG_SOCL] = socl;
mikasa_ncr_reg[MIKASA_NCR_REG_SBCL] =
    (mikasa_ncr_reg[MIKASA_NCR_REG_SBCL] &
    ~(MIKASA_NCR_SOCL_ACK | MIKASA_NCR_SOCL_ATN)) |
    (socl & (MIKASA_NCR_SOCL_ACK | MIKASA_NCR_SOCL_ATN));
if (op & MIKASA_NCR_SCR_TARGET) {
    if (set)
        scntl0 |= MIKASA_NCR_SCNTL0_TRG;
    else
        scntl0 &= ~MIKASA_NCR_SCNTL0_TRG;
    mikasa_ncr_reg[MIKASA_NCR_REG_SCNTL0] = scntl0;
    }
return;
}

static t_bool mikasa_ncr_script_condition (uint32 op,
    MIKASA_NCR_SCRIPT_STATE *state)
{
t_bool want_true = (op & MIKASA_NCR_TC_IFTRUE) ? TRUE : FALSE;
t_bool do_it;

if (op & MIKASA_NCR_TC_CARRY)
    return state->carry == want_true;
if ((op & (MIKASA_NCR_TC_DATA | MIKASA_NCR_TC_PHASE)) != 0) {
    do_it = TRUE;
    if (op & MIKASA_NCR_TC_DATA) {
        uint8 mask = (uint8) (op >> 8);
        uint8 data = (uint8) op;
        t_bool match = ((state->sfbr & ~mask) == (data & ~mask));

        if (match != want_true)
            do_it = FALSE;
        }
    if (op & MIKASA_NCR_TC_PHASE) {
        t_bool match = (state->phase != MIKASA_NCR_SCRIPT_NO_PHASE) &&
            (state->phase == mikasa_ncr_op_phase (op));

        if (match != want_true)
            do_it = FALSE;
        }
    return do_it;
    }
return TRUE;
}

static t_bool mikasa_ncr_advance_script (uint32 *dsp, uint32 op, uint32 arg,
    uint32 next, uint32 *stack, uint32 *sp, MIKASA_NCR_SCRIPT_STATE *state)
{
uint32 group = op & MIKASA_NCR_TC_GROUP_MASK;

if ((op & MIKASA_NCR_LS_GROUP_MASK) == MIKASA_NCR_LS_GROUP) {
    if (!mikasa_ncr_script_load_store (op, arg, state))
        return FALSE;
    *dsp = next;
    return TRUE;
    }
if ((op & MIKASA_NCR_MM_GROUP_MASK) == MIKASA_NCR_MM_GROUP) {
    uint32 dst;
    uint32 count = op & MIKASA_NCR_BM_COUNT_MASK;

    if ((op & 0x1F000000u) || (count == 0)) {
        if (state->side_effects)
            mikasa_ncr_set_dip (MIKASA_NCR_DSTAT_IID, arg);
        return FALSE;
        }
    if (!mikasa_pci_dma_read_long (next - 4, &dst))
        return FALSE;
    if ((arg & 3u) != (dst & 3u)) {
        if (state->side_effects)
            mikasa_ncr_set_dip (MIKASA_NCR_DSTAT_IID, arg);
        return FALSE;
        }
    if (state->side_effects) {
        mikasa_ncr_set_reg_l (MIKASA_NCR_REG_TEMP, dst);
        if (!mikasa_ncr_script_copy (arg, dst, count))
            return FALSE;
        }
    *dsp = next;
    return TRUE;
    }
if ((op & MIKASA_NCR_BM_TYPE_MASK) == MIKASA_NCR_BM_TYPE) {
    if (((op & MIKASA_NCR_BM_TABLE) == 0) &&
        ((op & MIKASA_NCR_BM_COUNT_MASK) == 0)) {
        if (state->side_effects)
            mikasa_ncr_set_dip (MIKASA_NCR_DSTAT_IID, arg);
        return FALSE;
        }
    if (state->side_effects &&
        (state->phase == mikasa_ncr_op_phase (op)) &&
        ((state->phase == MIKASA_NCR_PHASE_STS) ||
        (state->phase == MIKASA_NCR_PHASE_MSG_IN))) {
        uint32 count;
        uint32 addr;

        if (!mikasa_ncr_resolve_move (mikasa_ncr_reg_l (MIKASA_NCR_REG_DSA),
            op, arg, &count, &addr))
            return FALSE;
        if (count != 0) {
            if (!mikasa_pci_dma_write_byte (addr, state->sfbr))
                return FALSE;
            mikasa_ncr_set_sfbr (state->sfbr);
            mikasa_ncr_finish_move (op, addr, count, 1);
            if (state->phase == MIKASA_NCR_PHASE_STS) {
                state->phase = MIKASA_NCR_PHASE_MSG_IN;
                state->sfbr = 0;
                mikasa_ncr_latch_phase (MIKASA_NCR_PHASE_STS,
                    MIKASA_NCR_PHASE_MSG_IN);
                }
            }
        }
    *dsp = next;
    return TRUE;
    }
if ((op & 0xFF000000u) == MIKASA_NCR_SCR_WAIT_DISC) {
    if (state->side_effects) {
        mikasa_ncr_reg[MIKASA_NCR_REG_SCNTL2] &= ~MIKASA_NCR_SCNTL2_SDU;
        mikasa_ncr_reg[MIKASA_NCR_REG_SSTAT2] |= MIKASA_NCR_SSTAT2_LDSC;
        mikasa_ncr_set_connected (FALSE);
        }
    *dsp = next;
    return TRUE;
    }
if ((op & 0xFF000000u) == MIKASA_NCR_SCR_WAIT_RESEL) {
    uint32 jump = (op & MIKASA_NCR_SCR_REL) ? next + mikasa_ncr_sext24 (arg) :
        arg;

    if (state->side_effects &&
        (mikasa_ncr_reg[MIKASA_NCR_REG_ISTAT] & MIKASA_NCR_ISTAT_SIGP)) {
        *dsp = jump;
        return TRUE;
        }
    if (state->side_effects)
        mikasa_ncr_set_wait_reselect (next, jump);
    return FALSE;
    }
if ((op & 0xFF000000u) == MIKASA_NCR_SCR_SET) {
    mikasa_ncr_script_set_clear (op, TRUE, state);
    *dsp = next;
    return TRUE;
    }
if ((op & 0xFF000000u) == MIKASA_NCR_SCR_CLR) {
    mikasa_ncr_script_set_clear (op, FALSE, state);
    *dsp = next;
    return TRUE;
    }
if ((op & MIKASA_NCR_REGOP_GROUP_MASK) == MIKASA_NCR_SFBR_REG ||
    (op & MIKASA_NCR_REGOP_GROUP_MASK) == MIKASA_NCR_REG_SFBR_OP ||
    (op & MIKASA_NCR_REGOP_GROUP_MASK) == MIKASA_NCR_REG_REG) {
    mikasa_ncr_script_reg_op (op, state);
    *dsp = next;
    return TRUE;
    }
if ((group != MIKASA_NCR_TC_JUMP) && (group != MIKASA_NCR_TC_CALL) &&
    (group != MIKASA_NCR_TC_RETURN) && (group != MIKASA_NCR_TC_INT)) {
    *dsp = next;
    return TRUE;
    }
if (!mikasa_ncr_script_condition (op, state)) {
    *dsp = next;
    return TRUE;
    }

switch (group) {
    case MIKASA_NCR_TC_JUMP:
        *dsp = (op & MIKASA_NCR_TC_REL) ? next + mikasa_ncr_sext24 (arg) :
            arg;
        return TRUE;

    case MIKASA_NCR_TC_CALL:
        if (*sp < MIKASA_NCR_SCRIPT_STACK)
            stack[(*sp)++] = next;
        if (state->side_effects)
            mikasa_ncr_set_reg_l (MIKASA_NCR_REG_TEMP, next);
        *dsp = (op & MIKASA_NCR_TC_REL) ? next + mikasa_ncr_sext24 (arg) :
            arg;
        return TRUE;

    case MIKASA_NCR_TC_RETURN:
        if (*sp != 0) {
            *dsp = stack[--(*sp)];
            return TRUE;
            }
        if (state->side_effects) {
            *dsp = mikasa_ncr_reg_l (MIKASA_NCR_REG_TEMP);
            return TRUE;
            }
        *dsp = next;
        return TRUE;

    case MIKASA_NCR_TC_INT:
        if (op & MIKASA_NCR_TC_INTFLY) {
            if (state->side_effects) {
                mikasa_ncr_reg[MIKASA_NCR_REG_ISTAT] |=
                    MIKASA_NCR_ISTAT_INTF;
                mikasa_ncr_update_irq ();
                }
            *dsp = next;
            return TRUE;
            }
        return FALSE;
        }

*dsp = next;
return TRUE;
}

static void mikasa_ncr_select_latches (uint32 op, uint32 sel, t_bool table)
{
uint8 target = (uint8) ((sel >> 16) & MIKASA_NCR_SDID_WRITABLE);

if (table) {
    mikasa_ncr_reg[MIKASA_NCR_REG_SXFER] = (uint8) (sel >> 8);
    mikasa_ncr_reg[MIKASA_NCR_REG_SCNTL3] = (uint8) (sel >> 24);
    }
mikasa_ncr_reg[MIKASA_NCR_REG_SDID] = target;
mikasa_ncr_reg[MIKASA_NCR_REG_SSID] = MIKASA_NCR_SSID_VAL | target;
mikasa_ncr_reg[MIKASA_NCR_REG_SCNTL2] |= MIKASA_NCR_SCNTL2_SDU;
mikasa_ncr_reg[MIKASA_NCR_REG_SSTAT0] |= MIKASA_NCR_SSTAT0_WOA;
if (op & 0x01000000u) {
    mikasa_ncr_reg[MIKASA_NCR_REG_SOCL] |= MIKASA_NCR_SOCL_ATN;
    mikasa_ncr_reg[MIKASA_NCR_REG_SBCL] |= MIKASA_NCR_SOCL_ATN;
    }
else {
    mikasa_ncr_reg[MIKASA_NCR_REG_SOCL] &= ~MIKASA_NCR_SOCL_ATN;
    mikasa_ncr_reg[MIKASA_NCR_REG_SBCL] &= ~MIKASA_NCR_SOCL_ATN;
    }
return;
}

static t_bool mikasa_ncr_scan_script (uint32 dsp, uint32 dsa, uint32 phase,
    t_bool side_effects, t_bool stop_on_select, MIKASA_NCR_SCRIPT_SCAN *scan)
{
uint32 stack[MIKASA_NCR_SCRIPT_STACK];
MIKASA_NCR_SCRIPT_STATE state;
uint32 sp = 0;
uint32 i;
t_bool seen_move = FALSE;

memset (scan, 0, sizeof (*scan));
mikasa_ncr_dma_list_init (&scan->moves);
memset (&state, 0, sizeof (state));
state.phase = phase;
state.sfbr = mikasa_ncr_reg[MIKASA_NCR_REG_SFBR];
state.side_effects = side_effects;
for (i = 0; i < MIKASA_NCR_SCRIPT_SCAN_INSNS; i++) {
    uint32 op;
    uint32 arg;
    uint32 next;
    uint32 group;

    if (!mikasa_ncr_fetch_script (dsp, &op, &arg, side_effects))
        return FALSE;
    if ((op & MIKASA_NCR_TC_GROUP_MASK) == MIKASA_NCR_SCR_SEL_ABS) {
        uint32 sel = (op >> 16) & 0x0Fu;
        t_bool table = FALSE;

        if (op & 0x02000000u) {
            uint32 table_addr = (dsa + mikasa_ncr_sext24 (op)) & ~3u;

            if (!mikasa_pci_dma_read_long (table_addr, &sel))
                return FALSE;
            table = TRUE;
            }
        if (side_effects)
            mikasa_ncr_select_latches (op, table ? sel : (sel << 16),
                table);
        scan->target = table ? ((sel >> 16) & MIKASA_NCR_SDID_WRITABLE) :
            (sel & MIKASA_NCR_SDID_WRITABLE);
        scan->selected = TRUE;
        if (stop_on_select)
            return TRUE;
        }
    if (((op & MIKASA_NCR_BM_TYPE_MASK) == MIKASA_NCR_BM_TYPE) &&
        (phase != MIKASA_NCR_SCRIPT_NO_PHASE)) {
        uint32 op_phase = mikasa_ncr_op_phase (op);
        uint32 count;
        uint32 addr;

        if (seen_move && (op_phase != phase))
            return TRUE;
        if (op_phase != phase) {
            next = mikasa_ncr_next_script_addr (dsp, op);
            if (!mikasa_ncr_advance_script (&dsp, op, arg, next, stack,
                &sp, &state))
                return TRUE;
            continue;
            }
        if (!mikasa_ncr_resolve_move (dsa, op, arg, &count, &addr))
            return FALSE;
        seen_move = TRUE;
        if (op & MIKASA_NCR_BM_TABLE) {
            if (!scan->table_valid) {
                scan->table_valid = TRUE;
                scan->table_off = mikasa_ncr_sext24 (arg);
                }
            }
        else if (!scan->direct_valid) {
            scan->direct_valid = TRUE;
            scan->direct_count = count;
            scan->direct_addr = addr;
            }
        if (!mikasa_ncr_dma_list_append (&scan->moves, count, addr, op))
            return FALSE;
        scan->phase_next_valid = TRUE;
        scan->phase_next_dsp = mikasa_ncr_next_script_addr (dsp, op);
        }
    group = op & MIKASA_NCR_TC_GROUP_MASK;
    if ((group == MIKASA_NCR_TC_INT) &&
        ((op & MIKASA_NCR_TC_INTFLY) == 0) &&
        mikasa_ncr_script_condition (op, &state)) {
        if (seen_move && !scan->phase_int_valid) {
            scan->phase_int_valid = TRUE;
            scan->phase_int_dsps = arg;
            }
        if (!scan->any_int_valid) {
            scan->any_int_valid = TRUE;
            scan->any_int_dsps = arg;
            }
        return TRUE;
        }
    next = mikasa_ncr_next_script_addr (dsp, op);
    if (!mikasa_ncr_advance_script (&dsp, op, arg, next, stack, &sp,
        &state))
        return TRUE;
    }
return TRUE;
}

static t_bool mikasa_ncr_find_select (uint32 dsp, uint32 dsa, uint32 *target)
{
MIKASA_NCR_SCRIPT_SCAN scan;

if (!mikasa_ncr_scan_script (dsp, dsa, MIKASA_NCR_SCRIPT_NO_PHASE, TRUE,
    TRUE, &scan) || !scan.selected)
    return FALSE;
*target = scan.target;
return TRUE;
}

static t_bool mikasa_ncr_find_direct_move (uint32 dsp, uint32 phase,
    uint32 *count, uint32 *addr)
{
MIKASA_NCR_SCRIPT_SCAN scan;

if (!mikasa_ncr_scan_script (dsp, mikasa_ncr_reg_l (MIKASA_NCR_REG_DSA),
    phase, FALSE, FALSE, &scan) || !scan.direct_valid)
    return FALSE;
*count = scan.direct_count;
*addr = scan.direct_addr;
return TRUE;
}

static t_bool mikasa_ncr_find_table_move (uint32 dsp, uint32 phase,
    uint32 *table_off)
{
MIKASA_NCR_SCRIPT_SCAN scan;

if (!mikasa_ncr_scan_script (dsp, mikasa_ncr_reg_l (MIKASA_NCR_REG_DSA),
    phase, FALSE, FALSE, &scan) || !scan.table_valid)
    return FALSE;
*table_off = scan.table_off;
return TRUE;
}

static t_bool mikasa_ncr_find_linear_move (uint32 dsp, uint32 phase,
    uint32 *table_off, uint32 *next_dsp)
{
uint32 i;

for (i = 0; i < MIKASA_NCR_SCRIPT_SCAN_INSNS; i++) {
    uint32 op;
    uint32 arg;
    uint32 next;

    if (!mikasa_ncr_fetch_script (dsp, &op, &arg, FALSE))
        return FALSE;
    next = mikasa_ncr_next_script_addr (dsp, op);
    if (((op & MIKASA_NCR_BM_TYPE_MASK) == MIKASA_NCR_BM_TYPE) &&
        (mikasa_ncr_op_phase (op) == phase)) {
        if ((op & MIKASA_NCR_BM_TABLE) == 0)
            return FALSE;
        *table_off = mikasa_ncr_sext24 (arg);
        *next_dsp = next;
        return TRUE;
        }
    dsp = next;
    }
return FALSE;
}

static t_bool mikasa_ncr_collect_moves (uint32 dsp, uint32 dsa, uint32 phase,
    MIKASA_NCR_DMA_LIST *list)
{
MIKASA_NCR_SCRIPT_SCAN scan;

if (!mikasa_ncr_scan_script (dsp, dsa, phase, FALSE, FALSE, &scan))
    return FALSE;
*list = scan.moves;
return list->count != 0;
}

static t_bool mikasa_ncr_find_phase_int (uint32 dsp, uint32 phase,
    t_bool require_move, uint32 *dsps)
{
MIKASA_NCR_SCRIPT_SCAN scan;

if (!mikasa_ncr_scan_script (dsp, mikasa_ncr_reg_l (MIKASA_NCR_REG_DSA),
    phase, FALSE, FALSE, &scan))
    return FALSE;
if (require_move) {
    if (!scan.phase_int_valid)
        return FALSE;
    *dsps = scan.phase_int_dsps;
    return TRUE;
    }
if (!scan.any_int_valid)
    return FALSE;
*dsps = scan.any_int_dsps;
return TRUE;
}

static uint32 mikasa_ncr_data_int_dsps (uint32 dsp, uint32 phase)
{
uint32 dsps;

if (mikasa_ncr_find_phase_int (dsp, phase, TRUE, &dsps))
    return dsps;
return (phase == MIKASA_NCR_PHASE_DAT_OUT) ? MIKASA_NCR_DSPS_DAT_OUT :
    MIKASA_NCR_DSPS_DAT_IN;
}

static uint32 mikasa_ncr_status_int_dsps (uint32 dsp)
{
uint32 dsps;

if (mikasa_ncr_find_phase_int (dsp, MIKASA_NCR_PHASE_MSG_IN, TRUE, &dsps))
    return dsps;
if (mikasa_ncr_find_phase_int (dsp, MIKASA_NCR_PHASE_STS, TRUE, &dsps))
    return dsps;
if (mikasa_ncr_find_phase_int (dsp, MIKASA_NCR_SCRIPT_NO_PHASE, FALSE, &dsps))
    return dsps;
return MIKASA_NCR_DSPS_OK;
}

static void mikasa_ncr_clear_transaction (void)
{
memset (&mikasa_ncr_transaction, 0, sizeof (mikasa_ncr_transaction));
return;
}

static void mikasa_ncr_clear_wait_reselect (void)
{
mikasa_ncr_wait_reselect = FALSE;
mikasa_ncr_wait_reselect_dsp = 0;
mikasa_ncr_wait_reselect_jump = 0;
return;
}

static void mikasa_ncr_set_wait_reselect (uint32 dsp, uint32 jump)
{
mikasa_ncr_wait_reselect = TRUE;
mikasa_ncr_wait_reselect_dsp = dsp;
mikasa_ncr_wait_reselect_jump = jump;
mikasa_ncr_set_connected (FALSE);
mikasa_ncr_set_reg_l (MIKASA_NCR_REG_DSP, dsp);
return;
}

static void mikasa_ncr_begin_transaction (uint32 dsp, uint32 dsa,
    uint32 target)
{
mikasa_ncr_clear_transaction ();
mikasa_ncr_clear_wait_reselect ();
mikasa_ncr_transaction.active = TRUE;
mikasa_ncr_transaction.dsp = dsp;
mikasa_ncr_transaction.dsa = dsa;
mikasa_ncr_transaction.target = target;
mikasa_ncr_transaction.lun = 0;
return;
}

static void mikasa_ncr_defer_status_phase (uint32 dsp, uint32 dsa,
    uint32 target, uint32 data_phase, uint8 status)
{
mikasa_ncr_transaction.active = TRUE;
mikasa_ncr_transaction.status_pending = TRUE;
mikasa_ncr_transaction.dsp = dsp;
mikasa_ncr_transaction.dsa = dsa;
mikasa_ncr_transaction.target = target;
mikasa_ncr_transaction.data_phase = data_phase;
mikasa_ncr_transaction.status = status;
return;
}

static t_bool mikasa_ncr_do_msg_out (uint32 dsp, uint32 dsa)
{
MIKASA_NCR_DMA_LIST msg;
uint8 buf[64];
uint32 len;
uint32 off = 0;

if (!mikasa_ncr_collect_moves (dsp, dsa, MIKASA_NCR_PHASE_MSG_OUT, &msg))
    return TRUE;
len = (msg.bytes > sizeof (buf)) ? sizeof (buf) : msg.bytes;
if ((len != 0) && !mikasa_ncr_read_dma_list_offset (&msg, 0, buf, len))
    return FALSE;
while (off < len) {
    uint8 msgb = buf[off];

    if (msgb & 0x80) {                         /* IDENTIFY */
        mikasa_ncr_transaction.lun = msgb & 7u;
        off++;
        continue;
        }
    if ((msgb >= 0x20) && (msgb <= 0x22)) {    /* queue tag */
        if ((off + 1) >= len)
            break;
        mikasa_ncr_transaction.tag_valid = TRUE;
        mikasa_ncr_transaction.tag_msg = msgb;
        mikasa_ncr_transaction.tag = buf[off + 1];
        off = off + 2;
        continue;
        }
    if (msgb == 0x01) {                        /* extended */
        uint32 ext_len;

        if ((off + 1) >= len)
            break;
        ext_len = buf[off + 1];
        if ((off + 2 + ext_len) > len)
            break;
        off = off + 2 + ext_len;
        continue;
        }
    off++;
    }
if (!mikasa_ncr_discard_scsi_data (&msg))
    return FALSE;
if (mikasa_ncr_transaction.tag_valid)
    sim_debug (MIKASA_DBG_PCI, &mikasa_dev,
        "NCR MESSAGE OUT lun=%u tag=%s%u\n", mikasa_ncr_transaction.lun,
        (mikasa_ncr_transaction.tag_msg == 0x20) ? "simple:" :
        (mikasa_ncr_transaction.tag_msg == 0x21) ? "head:" : "ordered:",
        mikasa_ncr_transaction.tag);
else
    sim_debug (MIKASA_DBG_PCI, &mikasa_dev,
        "NCR MESSAGE OUT lun=%u\n", mikasa_ncr_transaction.lun);
mikasa_ncr_reg[MIKASA_NCR_REG_SOCL] &= ~MIKASA_NCR_SOCL_ATN;
mikasa_ncr_reg[MIKASA_NCR_REG_SBCL] &= ~MIKASA_NCR_SOCL_ATN;
return TRUE;
}

static t_bool mikasa_ncr_cmd_data_phase (const uint8 *cdb, uint32 *phase)
{
switch (cdb[0]) {
    case 0x04:                                      /* FORMAT UNIT */
        if (cdb[1] & 0x10) {
            *phase = MIKASA_NCR_PHASE_DAT_OUT;
            return TRUE;
            }
        return FALSE;

    case 0x07:                                      /* REASSIGN BLOCKS */
    case 0x0A:                                      /* WRITE(6) */
    case 0x15:                                      /* MODE SELECT(6) */
    case 0x1D:                                      /* SEND DIAGNOSTIC */
    case 0x2A:                                      /* WRITE(10) */
    case 0x2E:                                      /* WRITE AND VERIFY */
    case 0x3B:                                      /* WRITE BUFFER */
    case 0x3F:                                      /* WRITE LONG */
    case 0x55:                                      /* MODE SELECT(10) */
    case 0x8A:                                      /* WRITE(16) */
    case 0x8E:                                      /* WRITE AND VERIFY(16) */
    case 0xAA:                                      /* WRITE(12) */
    case 0xAE:                                      /* WRITE AND VERIFY(12) */
        *phase = MIKASA_NCR_PHASE_DAT_OUT;
        return TRUE;

    case 0x03:                                      /* REQUEST SENSE */
    case 0x08:                                      /* READ(6) */
    case 0x12:                                      /* INQUIRY */
    case 0x1C:                                      /* RECEIVE DIAGNOSTIC RESULTS */
    case 0x1A:                                      /* MODE SENSE(6) */
    case 0x23:                                      /* READ FORMAT CAPACITIES */
    case 0x25:                                      /* READ CAPACITY(10) */
    case 0x28:                                      /* READ(10) */
    case 0x37:                                      /* READ DEFECT DATA */
    case 0x3C:                                      /* READ BUFFER */
    case 0x3E:                                      /* READ LONG */
    case 0x4D:                                      /* LOG SENSE */
    case 0x5A:                                      /* MODE SENSE(10) */
    case 0x88:                                      /* READ(16) */
    case 0x9E:                                      /* SERVICE ACTION IN(16) */
    case 0xA0:                                      /* REPORT LUNS */
    case 0xA8:                                      /* READ(12) */
    case 0xB7:                                      /* READ DEFECT DATA(12) */
        *phase = MIKASA_NCR_PHASE_DAT_IN;
        return TRUE;

    default:
        return FALSE;
        }
}

static t_bool mikasa_ncr_write_phase_byte (uint32 dsp, uint32 dsa,
    uint32 phase, uint32 def_off, uint8 val)
{
uint32 count;
uint32 addr;
uint32 off;

if (!mikasa_ncr_find_table_move (dsp, phase, &off))
    off = def_off;
if ((off != M32) && mikasa_ncr_table_entry (dsa, off, &count, &addr) &&
    (count != 0)) {
    (void) mikasa_pci_dma_write_byte (addr, val);
    mikasa_ncr_set_sfbr (val);
    mikasa_ncr_finish_move (mikasa_ncr_move_op_for_phase (phase), addr,
        count, 1);
    return TRUE;
    }
if (mikasa_ncr_find_direct_move (dsp, phase, &count, &addr) &&
    (count != 0)) {
    (void) mikasa_pci_dma_write_byte (addr, val);
    mikasa_ncr_set_sfbr (val);
    mikasa_ncr_finish_move (mikasa_ncr_move_op_for_phase (phase), addr,
        count, 1);
    return TRUE;
    }
return FALSE;
}

static void mikasa_ncr_write_status_msg (uint32 dsp, uint32 dsa,
    uint8 status)
{
(void) mikasa_ncr_write_phase_byte (dsp, dsa, MIKASA_NCR_PHASE_STS, 0x24,
    status);
(void) mikasa_ncr_write_phase_byte (dsp, dsa, MIKASA_NCR_PHASE_MSG_IN, M32,
    0);
return;
}

static t_bool mikasa_ncr_scsi_read (uint32 unit, uint32 lbn, uint32 blocks,
    const MIKASA_NCR_DMA_LIST *data)
{
uint8 buf[MIKASA_IO_BUFSIZE];
UNIT *uptr = &dka_unit[unit];
t_uint64 bytes_left = ((t_uint64) blocks) * MIKASA_DKA_BLOCK_SIZE;
uint32 done = 0;

if (bytes_left > data->bytes)
    bytes_left = data->bytes;
if (bytes_left == 0)
    return TRUE;
if ((uptr->flags & UNIT_ATT) == 0)
    return FALSE;
if ((uptr->capac != 0) && (((t_uint64) lbn + blocks) > uptr->capac))
    return FALSE;
if (sim_fseeko (uptr->fileref,
    ((t_offset) lbn) * MIKASA_DKA_BLOCK_SIZE, SEEK_SET))
    return FALSE;

while (bytes_left != 0) {
    uint32 chunk = (bytes_left > sizeof (buf)) ? sizeof (buf) :
        (uint32) bytes_left;

    if (sim_fread (buf, 1, chunk, uptr->fileref) != chunk)
        return FALSE;
    if (chunk != 0)
        mikasa_ncr_set_sfbr (buf[chunk - 1]);
    if (!mikasa_ncr_write_dma_list_offset (data, done, buf, chunk))
        return FALSE;
    done = done + chunk;
    bytes_left = bytes_left - chunk;
    }
mikasa_ncr_finish_dma_list (data, done);
return TRUE;
}

static t_bool mikasa_ncr_scsi_write (uint32 unit, uint32 lbn, uint32 blocks,
    const MIKASA_NCR_DMA_LIST *data, uint8 *status)
{
uint8 buf[MIKASA_IO_BUFSIZE];
UNIT *uptr = &dka_unit[unit];
t_uint64 bytes_left = ((t_uint64) blocks) * MIKASA_DKA_BLOCK_SIZE;
uint32 done = 0;
t_bool latch = TRUE;

if (bytes_left > data->bytes)
    bytes_left = data->bytes;
if (bytes_left == 0)
    return TRUE;
if ((uptr->flags & UNIT_ATT) == 0)
    return FALSE;
if (uptr->flags & UNIT_RO) {
    mikasa_ncr_set_sense (unit, 0x07, 0x27, 0x00);
    *status = 2;
    return TRUE;
    }
if ((uptr->capac != 0) && (((t_uint64) lbn + blocks) > uptr->capac)) {
    mikasa_ncr_set_sense (unit, 0x05, 0x21, 0x00);
    *status = 2;
    return TRUE;
    }
if (sim_fseeko (uptr->fileref,
    ((t_offset) lbn) * MIKASA_DKA_BLOCK_SIZE, SEEK_SET))
    return FALSE;

while (bytes_left != 0) {
    uint32 chunk = (bytes_left > sizeof (buf)) ? sizeof (buf) :
        (uint32) bytes_left;

    if (!mikasa_ncr_read_dma_list_offset (data, done, buf, chunk))
        return FALSE;
    if (latch && (chunk != 0)) {
        mikasa_ncr_set_output_latch (buf[0]);
        latch = FALSE;
        }
    if (sim_fwrite (buf, 1, chunk, uptr->fileref) != chunk)
        return FALSE;
    done = done + chunk;
    bytes_left = bytes_left - chunk;
    }
mikasa_ncr_finish_dma_list (data, done);
mikasa_ncr_clear_sense (unit);
return TRUE;
}

static uint32 mikasa_ncr_mode_page (uint32 unit, uint8 page_code,
    t_bool changeable, uint8 *buf, uint32 max)
{
UNIT *uptr = &dka_unit[unit];
uint32 heads = 16;
uint32 sectors = 45;
uint32 cylinders = 0;

if ((uptr->capac != 0) && (heads * sectors != 0))
    cylinders = ((uint32) uptr->capac) / (heads * sectors);
switch (page_code) {
    case 0x01:                                      /* error recovery */
        if (max < 12)
            return 0;
        buf[0] = 0x01;
        buf[1] = 0x0A;
        if (changeable)
            return 12;
        buf[2] = 0x80;                              /* AWRE */
        buf[3] = 0x0B;                              /* read retry count */
        buf[8] = 0x0B;                              /* write retry count */
        return 12;

    case 0x02:                                      /* disconnect/reconnect */
        if (max < 16)
            return 0;
        buf[0] = 0x02;
        buf[1] = 0x0E;
        return 16;

    case 0x03:                                      /* format device */
        if (max < 24)
            return 0;
        buf[0] = 0x03;
        buf[1] = 0x16;
        if (changeable)
            return 24;
        mikasa_ncr_write_be16 (&buf[10], sectors);
        mikasa_ncr_write_be16 (&buf[12], MIKASA_DKA_BLOCK_SIZE);
        mikasa_ncr_write_be16 (&buf[14], 1);
        return 24;

    case 0x04:                                      /* rigid geometry */
        if (max < 24)
            return 0;
        buf[0] = 0x04;
        buf[1] = 0x16;
        if (changeable)
            return 24;
        mikasa_ncr_write_be24 (&buf[2], cylinders);
        buf[5] = (uint8) heads;
        mikasa_ncr_write_be24 (&buf[6], 0);
        mikasa_ncr_write_be24 (&buf[9], 0);
        mikasa_ncr_write_be16 (&buf[20], (uint32) sectors);
        return 24;

    case 0x08:                                      /* caching */
        if (max < 20)
            return 0;
        buf[0] = 0x08;
        buf[1] = 0x12;
        if (changeable)
            return 20;
        buf[2] = 0x04;                              /* read cache enable */
        return 20;

    case 0x0A:                                      /* control */
        if (max < 12)
            return 0;
        buf[0] = 0x0A;
        buf[1] = 0x0A;
        return 12;

    default:
        return 0;
        }
}

static uint32 mikasa_ncr_mode_pages (uint32 unit, uint8 page_code,
    t_bool changeable, uint8 *buf, uint32 max)
{
uint32 off = 0;

if (page_code == 0x3F) {
    off += mikasa_ncr_mode_page (unit, 0x01, changeable, buf + off,
        max - off);
    off += mikasa_ncr_mode_page (unit, 0x02, changeable, buf + off,
        max - off);
    off += mikasa_ncr_mode_page (unit, 0x03, changeable, buf + off,
        max - off);
    off += mikasa_ncr_mode_page (unit, 0x04, changeable, buf + off,
        max - off);
    off += mikasa_ncr_mode_page (unit, 0x08, changeable, buf + off,
        max - off);
    off += mikasa_ncr_mode_page (unit, 0x0A, changeable, buf + off,
        max - off);
    return off;
    }
return mikasa_ncr_mode_page (unit, page_code, changeable, buf, max);
}

static t_bool mikasa_ncr_scsi_mode_sense (uint32 unit, const uint8 *cdb,
    const MIKASA_NCR_DMA_LIST *data, t_bool ten)
{
uint8 buf[256];
UNIT *uptr = &dka_unit[unit];
uint32 page_code = cdb[2] & 0x3F;
uint32 page_control = (cdb[2] >> 6) & 3;
uint32 alloc = ten ? mikasa_ncr_alloc_len10 (cdb) : cdb[4];
uint32 header = ten ? 8 : 4;
uint32 desc = (cdb[1] & 0x08) ? 0 : 8;
uint32 pages;
uint32 page_off = header + desc;
uint32 total;
uint32 len;
uint32 blocks;

memset (buf, 0, sizeof (buf));
if (uptr->flags & UNIT_RO)
    buf[ten ? 3 : 2] = 0x80;
if (desc != 0) {
    blocks = (uptr->capac > 0xFFFFFFu) ? 0xFFFFFFu : (uint32) uptr->capac;
    if (ten)
        mikasa_ncr_write_be16 (&buf[6], desc);
    else
        buf[3] = (uint8) desc;
    mikasa_ncr_write_be24 (&buf[header + 1], blocks);
    mikasa_ncr_write_be24 (&buf[header + 5], MIKASA_DKA_BLOCK_SIZE);
    }
pages = mikasa_ncr_mode_pages (unit, page_code, page_control == 1,
    buf + page_off, sizeof (buf) - page_off);
if ((pages == 0) && (page_code != 0x00)) {
    mikasa_ncr_set_sense (unit, 0x05, 0x24, 0x00);
    return FALSE;
    }
total = page_off + pages;
if (ten)
    mikasa_ncr_write_be16 (buf, total - 2);
else
    buf[0] = (uint8) (total - 1);
len = mikasa_ncr_min3 (data->bytes, alloc, total);
mikasa_ncr_clear_sense (unit);
return mikasa_ncr_write_scsi_data (data, buf, len);
}

static t_bool mikasa_ncr_scsi_read_format_capacities (uint32 unit,
    const uint8 *cdb, const MIKASA_NCR_DMA_LIST *data)
{
uint8 buf[12];
UNIT *uptr = &dka_unit[unit];
uint32 blocks = (uptr->capac > M32) ? M32 : (uint32) uptr->capac;
uint32 len;

memset (buf, 0, sizeof (buf));
buf[3] = 8;
mikasa_ncr_write_be32 (&buf[4], blocks);
buf[8] = 0x02;                                      /* formatted media */
mikasa_ncr_write_be24 (&buf[9], MIKASA_DKA_BLOCK_SIZE);
len = mikasa_ncr_min3 (data->bytes, mikasa_ncr_alloc_len10 (cdb),
    sizeof (buf));
mikasa_ncr_clear_sense (unit);
return mikasa_ncr_write_scsi_data (data, buf, len);
}

static t_bool mikasa_ncr_scsi_read_defect_data (uint32 unit, const uint8 *cdb,
    const MIKASA_NCR_DMA_LIST *data, t_bool twelve)
{
uint8 buf[8];
uint32 alloc = twelve ? mikasa_ncr_cdb_be32 (cdb, 7) :
    mikasa_ncr_alloc_len10 (cdb);
uint32 total = twelve ? 6 : 4;
uint32 len;

memset (buf, 0, sizeof (buf));
buf[1] = cdb[2] & 0x1F;                             /* requested list/format */
len = mikasa_ncr_min3 (data->bytes, alloc, total);
mikasa_ncr_clear_sense (unit);
return mikasa_ncr_write_scsi_data (data, buf, len);
}

static t_bool mikasa_ncr_scsi_inquiry_vpd (uint32 unit, const uint8 *cdb,
    const MIKASA_NCR_DMA_LIST *data, uint8 *status)
{
uint8 buf[256];
uint32 len;
uint32 page = cdb[2];
uint32 alloc = cdb[4];
const char *id;
char serial[16];

memset (buf, 0, sizeof (buf));
buf[1] = (uint8) page;
(void) snprintf (serial, sizeof (serial), "MIKASA-DKA%u", unit * 100);
switch (page) {
    case 0x00:                                      /* supported pages */
        buf[3] = 3;
        buf[4] = 0x00;
        buf[5] = 0x80;
        buf[6] = 0x83;
        len = 7;
        break;

    case 0x80:                                      /* unit serial */
        len = (uint32) strlen (serial);
        buf[3] = (uint8) len;
        memcpy (&buf[4], serial, len);
        len = len + 4;
        break;

    case 0x83:                                      /* device identification */
        id = serial;
        len = (uint32) strlen (id);
        buf[2] = 0;
        buf[3] = (uint8) (len + 4);
        buf[4] = 0x02;                              /* ASCII designator */
        buf[5] = 0x01;                              /* vendor specific */
        buf[7] = (uint8) len;
        memcpy (&buf[8], id, len);
        len = len + 8;
        break;

    default:
        mikasa_ncr_set_sense (unit, 0x05, 0x24, 0x00);
        *status = 2;
        return TRUE;
        }
len = mikasa_ncr_min3 (data->bytes, alloc, len);
mikasa_ncr_clear_sense (unit);
return mikasa_ncr_write_scsi_data (data, buf, len);
}

static t_stat mikasa_ncr_scsi_init (void)
{
uint32 i;
t_stat r;

mikasa_ncr_scsi_bus.dptr = &mikasa_dev;
r = scsi_init (&mikasa_ncr_scsi_bus, MIKASA_NCR_SCSI_MAXFR);
if (r != SCPE_OK)
    return r;
scsi_reset (&mikasa_ncr_scsi_bus);
for (i = 0; i < MIKASA_DKA_UNITS; i++) {
    scsi_add_unit (&mikasa_ncr_scsi_bus, i, &dka_unit[i]);
    scsi_set_unit (&mikasa_ncr_scsi_bus, &dka_unit[i], &dka_scsi_dev);
    }
return SCPE_OK;
}

static uint32 mikasa_ncr_sim_scsi_cdb_len (const uint8 *cdb)
{
switch (cdb[0]) {
    case 0x25:                                      /* READ CAPACITY(10) */
        return 10;

    default:
        return 6;
        }
}

static t_bool mikasa_ncr_sim_scsi_supported (const uint8 *cdb,
    const MIKASA_NCR_DMA_LIST *data)
{
switch (cdb[0]) {
    case 0x00:                                      /* TEST UNIT READY */
    case 0x16:                                      /* RESERVE UNIT */
    case 0x17:                                      /* RELEASE UNIT */
    case 0x1B:                                      /* START STOP UNIT */
    case 0x1E:                                      /* PREVENT/ALLOW */
        return TRUE;

    default:
        return FALSE;
        }
}

static t_bool mikasa_ncr_scsi_cmd_sim_scsi (uint32 unit, const uint8 *cdb,
    const MIKASA_NCR_DMA_LIST *data, uint8 *status, t_bool *handled)
{
uint8 buf[MIKASA_NCR_SCSI_BUFSIZE];
uint8 msg = 0;
uint32 cmd_len;
uint32 read_len;
uint32 write_len;
t_stat r;

*handled = FALSE;
if (!mikasa_ncr_sim_scsi_supported (cdb, data))
    return TRUE;
*handled = TRUE;

if (mikasa_ncr_scsi_bus.buf == NULL) {
    r = mikasa_ncr_scsi_init ();
    if (r != SCPE_OK)
        return FALSE;
    }

scsi_reset (&mikasa_ncr_scsi_bus);
if (!scsi_arbitrate (&mikasa_ncr_scsi_bus, 7) ||
    !scsi_select (&mikasa_ncr_scsi_bus, unit))
    return FALSE;

mikasa_ncr_scsi_bus.lun = mikasa_ncr_transaction.lun;
cmd_len = mikasa_ncr_sim_scsi_cdb_len (cdb);
if (scsi_write (&mikasa_ncr_scsi_bus, (uint8 *) cdb, cmd_len) != cmd_len) {
    scsi_release (&mikasa_ncr_scsi_bus);
    return FALSE;
    }

if (mikasa_ncr_scsi_bus.phase == SCSI_DATI) {
    read_len = mikasa_ncr_scsi_bus.buf_b - mikasa_ncr_scsi_bus.buf_t;
    if (read_len > sizeof (buf)) {
        scsi_release (&mikasa_ncr_scsi_bus);
        return FALSE;
        }
    read_len = scsi_read (&mikasa_ncr_scsi_bus, buf, read_len);
    write_len = (data->bytes < read_len) ? data->bytes : read_len;
    if (!mikasa_ncr_write_scsi_data (data, buf, write_len)) {
        scsi_release (&mikasa_ncr_scsi_bus);
        return FALSE;
        }
    }

if (mikasa_ncr_scsi_bus.phase != SCSI_STS) {
    scsi_release (&mikasa_ncr_scsi_bus);
    return FALSE;
    }
if (scsi_read (&mikasa_ncr_scsi_bus, status, 1) != 1) {
    scsi_release (&mikasa_ncr_scsi_bus);
    return FALSE;
    }
if (mikasa_ncr_scsi_bus.phase == SCSI_MSGI)
    (void) scsi_read (&mikasa_ncr_scsi_bus, &msg, 1);
scsi_release (&mikasa_ncr_scsi_bus);

if (*status == 0)
    mikasa_ncr_clear_sense (unit);
else
    mikasa_ncr_set_sense (unit, mikasa_ncr_scsi_bus.sense_key & 0x0F,
        mikasa_ncr_scsi_bus.sense_code,
        mikasa_ncr_scsi_bus.sense_qual);
sim_debug (MIKASA_DBG_PCI, &mikasa_dev,
    "NCR target %u CDB %02X handled by sim_scsi status %02X msg %02X\n",
    unit, cdb[0], *status, msg);
return TRUE;
}

static t_bool mikasa_ncr_scsi_cmd (uint32 unit, const uint8 *cdb,
    const MIKASA_NCR_DMA_LIST *data, uint8 *status)
{
uint8 buf[MIKASA_IO_BUFSIZE];
UNIT *uptr = &dka_unit[unit];
uint32 len = 0;
uint32 xfer_len = 0;
uint32 lbn;
uint32 blocks;
t_uint64 lbn64;
t_bool handled;

*status = 0;
memset (buf, 0, sizeof (buf));
if (!mikasa_ncr_scsi_cmd_sim_scsi (unit, cdb, data, status, &handled))
    return FALSE;
if (handled)
    return TRUE;
switch (cdb[0]) {
    case 0x00:                                              /* TEST UNIT READY */
        mikasa_ncr_clear_sense (unit);
        return TRUE;

    case 0x04:                                              /* FORMAT UNIT */
        if (!mikasa_ncr_discard_scsi_data (data))
            return FALSE;
        mikasa_ncr_clear_sense (unit);
        return TRUE;

    case 0x03:                                              /* REQUEST SENSE */
        buf[0] = 0x70;
        buf[2] = mikasa_ncr_sense_key[unit];
        buf[7] = 0x0A;
        buf[12] = mikasa_ncr_sense_asc[unit];
        buf[13] = mikasa_ncr_sense_ascq[unit];
        len = mikasa_ncr_min3 (data->bytes, cdb[4], sizeof (buf));
        if (!mikasa_ncr_write_scsi_data (data, buf, len))
            return FALSE;
        mikasa_ncr_clear_sense (unit);
        return TRUE;

    case 0x08:                                              /* READ(6) */
        lbn = (((uint32) cdb[1] & 0x1F) << 16) |
            (((uint32) cdb[2]) << 8) | cdb[3];
        blocks = cdb[4] ? cdb[4] : 256;
        if (mikasa_ncr_scsi_read (unit, lbn, blocks, data)) {
            mikasa_ncr_clear_sense (unit);
            return TRUE;
            }
        mikasa_ncr_set_sense (unit, 0x05, 0x21, 0x00);
        *status = 2;
        return TRUE;

    case 0x0A:                                              /* WRITE(6) */
        lbn = (((uint32) cdb[1] & 0x1F) << 16) |
            (((uint32) cdb[2]) << 8) | cdb[3];
        blocks = cdb[4] ? cdb[4] : 256;
        return mikasa_ncr_scsi_write (unit, lbn, blocks, data, status);

    case 0x12:                                              /* INQUIRY */
        if (cdb[1] & 1)
            return mikasa_ncr_scsi_inquiry_vpd (unit, cdb, data, status);
        buf[2] = 2;
        buf[3] = 2;
        buf[4] = 31;
        memcpy (&buf[8], "DEC     ", 8);
        memcpy (&buf[16], "RZ58     (C) DEC", 16);
        memcpy (&buf[32], "2000", 4);
        len = mikasa_ncr_min3 (data->bytes, cdb[4],
            5 + (uint32) buf[4]);
        xfer_len = (data->bytes < cdb[4]) ? data->bytes : cdb[4];
        sim_debug (MIKASA_DBG_PCI, &mikasa_dev,
            "NCR INQUIRY target=%u lun=0 alloc=%u bytes=%u xfer=%u pdt=%02X "
            "addlen=%u\n", unit, cdb[4], len, xfer_len, buf[0], buf[4]);
        mikasa_ncr_clear_sense (unit);
        return mikasa_ncr_write_scsi_data_padded (data, buf, len, xfer_len);

    case 0x1D:                                              /* SEND DIAGNOSTIC */
        if (!mikasa_ncr_discard_scsi_data (data))
            return FALSE;
        mikasa_ncr_clear_sense (unit);
        return TRUE;

    case 0x07:                                              /* REASSIGN BLOCKS */
    case 0x15:                                              /* MODE SELECT(6) */
        if (!mikasa_ncr_discard_scsi_data (data))
            return FALSE;
        mikasa_ncr_clear_sense (unit);
        return TRUE;

    case 0x16:                                              /* RESERVE UNIT */
    case 0x17:                                              /* RELEASE UNIT */
    case 0x1B:                                              /* START STOP UNIT */
    case 0x1E:                                              /* PREVENT/ALLOW */
        mikasa_ncr_clear_sense (unit);
        return TRUE;

    case 0x1A:                                              /* MODE SENSE(6) */
        if (mikasa_ncr_scsi_mode_sense (unit, cdb, data, FALSE))
            return TRUE;
        *status = 2;
        return TRUE;

    case 0x1C:                                              /* RECEIVE DIAGNOSTIC RESULTS */
        len = mikasa_ncr_min3 (data->bytes,
            (((uint32) cdb[3]) << 8) | cdb[4], sizeof (buf));
        mikasa_ncr_clear_sense (unit);
        return mikasa_ncr_write_scsi_data (data, buf, len);

    case 0x23:                                              /* READ FORMAT CAPACITIES */
        return mikasa_ncr_scsi_read_format_capacities (unit, cdb, data);

    case 0x25:                                              /* READ CAPACITY(10) */
        if ((uptr->flags & UNIT_ATT) == 0)
            return FALSE;
        mikasa_ncr_write_be32 (&buf[0],
            uptr->capac ? ((uint32) uptr->capac - 1) : 0);
        mikasa_ncr_write_be32 (&buf[4], MIKASA_DKA_BLOCK_SIZE);
        len = (data->bytes < 8) ? data->bytes : 8;
        mikasa_ncr_clear_sense (unit);
        return mikasa_ncr_write_scsi_data (data, buf, len);

    case 0x28:                                              /* READ(10) */
        lbn = (((uint32) cdb[2]) << 24) | (((uint32) cdb[3]) << 16) |
            (((uint32) cdb[4]) << 8) | cdb[5];
        blocks = (((uint32) cdb[7]) << 8) | cdb[8];
        if (mikasa_ncr_scsi_read (unit, lbn, blocks, data)) {
            mikasa_ncr_clear_sense (unit);
            return TRUE;
            }
        mikasa_ncr_set_sense (unit, 0x05, 0x21, 0x00);
        *status = 2;
        return TRUE;

    case 0x2A:                                              /* WRITE(10) */
    case 0x2E:                                              /* WRITE AND VERIFY */
        lbn = (((uint32) cdb[2]) << 24) | (((uint32) cdb[3]) << 16) |
            (((uint32) cdb[4]) << 8) | cdb[5];
        blocks = (((uint32) cdb[7]) << 8) | cdb[8];
        return mikasa_ncr_scsi_write (unit, lbn, blocks, data, status);

    case 0xA8:                                              /* READ(12) */
        lbn = mikasa_ncr_cdb_be32 (cdb, 2);
        blocks = mikasa_ncr_cdb_be32 (cdb, 6);
        if (mikasa_ncr_scsi_read (unit, lbn, blocks, data)) {
            mikasa_ncr_clear_sense (unit);
            return TRUE;
            }
        mikasa_ncr_set_sense (unit, 0x05, 0x21, 0x00);
        *status = 2;
        return TRUE;

    case 0xAA:                                              /* WRITE(12) */
    case 0xAE:                                              /* WRITE AND VERIFY(12) */
        lbn = mikasa_ncr_cdb_be32 (cdb, 2);
        blocks = mikasa_ncr_cdb_be32 (cdb, 6);
        return mikasa_ncr_scsi_write (unit, lbn, blocks, data, status);

    case 0x2B:                                              /* SEEK(10) */
    case 0x2F:                                              /* VERIFY(10) */
    case 0x34:                                              /* PRE-FETCH(10) */
    case 0x35:                                              /* SYNCHRONIZE CACHE */
    case 0x56:                                              /* RESERVE UNIT(10) */
    case 0x57:                                              /* RELEASE UNIT(10) */
    case 0xAF:                                              /* VERIFY(12) */
        mikasa_ncr_clear_sense (unit);
        return TRUE;

    case 0x3B:                                              /* WRITE BUFFER */
    case 0x55:                                              /* MODE SELECT(10) */
        if (!mikasa_ncr_discard_scsi_data (data))
            return FALSE;
        mikasa_ncr_clear_sense (unit);
        return TRUE;

    case 0x37:                                              /* READ DEFECT DATA(10) */
        return mikasa_ncr_scsi_read_defect_data (unit, cdb, data, FALSE);

    case 0x3C:                                              /* READ BUFFER */
        len = mikasa_ncr_min3 (data->bytes, mikasa_ncr_alloc_len10 (cdb),
            sizeof (buf));
        mikasa_ncr_clear_sense (unit);
        return mikasa_ncr_write_scsi_data (data, buf, len);

    case 0x3E:                                              /* READ LONG */
        len = mikasa_ncr_alloc_len10 (cdb);
        if (len == 0)
            len = MIKASA_DKA_BLOCK_SIZE;
        len = mikasa_ncr_min3 (data->bytes, len, sizeof (buf));
        mikasa_ncr_clear_sense (unit);
        return mikasa_ncr_write_scsi_data (data, buf, len);

    case 0x3F:                                              /* WRITE LONG */
        if (uptr->flags & UNIT_RO) {
            mikasa_ncr_set_sense (unit, 0x07, 0x27, 0x00);
            *status = 2;
            return TRUE;
            }
        mikasa_ncr_clear_sense (unit);
        if (!mikasa_ncr_discard_scsi_data (data))
            return FALSE;
        return TRUE;

    case 0x4D:                                              /* LOG SENSE */
        buf[0] = cdb[2] & 0x3F;
        len = mikasa_ncr_min3 (data->bytes, mikasa_ncr_alloc_len10 (cdb),
            sizeof (buf));
        mikasa_ncr_clear_sense (unit);
        return mikasa_ncr_write_scsi_data (data, buf, len);

    case 0x5A:                                              /* MODE SENSE(10) */
        if (mikasa_ncr_scsi_mode_sense (unit, cdb, data, TRUE))
            return TRUE;
        *status = 2;
        return TRUE;

    case 0x88:                                              /* READ(16) */
        lbn64 = mikasa_ncr_cdb_be64 (cdb, 2);
        blocks = mikasa_ncr_cdb_be32 (cdb, 10);
        if (lbn64 > M32) {
            mikasa_ncr_set_sense (unit, 0x05, 0x21, 0x00);
            *status = 2;
            return TRUE;
            }
        if (mikasa_ncr_scsi_read (unit, (uint32) lbn64, blocks, data)) {
            mikasa_ncr_clear_sense (unit);
            return TRUE;
            }
        mikasa_ncr_set_sense (unit, 0x05, 0x21, 0x00);
        *status = 2;
        return TRUE;

    case 0x8A:                                              /* WRITE(16) */
    case 0x8E:                                              /* WRITE AND VERIFY(16) */
        lbn64 = mikasa_ncr_cdb_be64 (cdb, 2);
        blocks = mikasa_ncr_cdb_be32 (cdb, 10);
        if (lbn64 > M32) {
            mikasa_ncr_set_sense (unit, 0x05, 0x21, 0x00);
            *status = 2;
            return TRUE;
            }
        return mikasa_ncr_scsi_write (unit, (uint32) lbn64, blocks, data,
            status);

    case 0x8F:                                              /* VERIFY(16) */
    case 0x91:                                              /* SYNCHRONIZE CACHE(16) */
        mikasa_ncr_clear_sense (unit);
        return TRUE;

    case 0x9E:                                              /* SERVICE ACTION IN(16) */
        if ((cdb[1] & 0x1F) != 0x10) {                      /* READ CAPACITY(16) */
            mikasa_ncr_set_sense (unit, 0x05, 0x20, 0x00);
            *status = 2;
            return TRUE;
            }
        if ((uptr->flags & UNIT_ATT) == 0)
            return FALSE;
        mikasa_ncr_write_be64 (&buf[0],
            uptr->capac ? ((t_uint64) uptr->capac - 1) : 0);
        mikasa_ncr_write_be32 (&buf[8], MIKASA_DKA_BLOCK_SIZE);
        len = mikasa_ncr_min3 (data->bytes, mikasa_ncr_cdb_be32 (cdb, 10),
            32);
        mikasa_ncr_clear_sense (unit);
        return mikasa_ncr_write_scsi_data (data, buf, len);

    case 0xA0:                                              /* REPORT LUNS */
        mikasa_ncr_write_be32 (&buf[0], 8);
        len = mikasa_ncr_min3 (data->bytes, mikasa_ncr_cdb_be32 (cdb, 6),
            16);
        mikasa_ncr_clear_sense (unit);
        return mikasa_ncr_write_scsi_data (data, buf, len);

    case 0xB7:                                              /* READ DEFECT DATA(12) */
        return mikasa_ncr_scsi_read_defect_data (unit, cdb, data, TRUE);

    default:
        sim_debug (MIKASA_DBG_PCI, &mikasa_dev,
            "NCR unsupported SCSI command %02X on target %u\n", cdb[0],
            unit);
        mikasa_ncr_set_sense (unit, 0x05, 0x20, 0x00);
        *status = 2;
        return TRUE;
        }
}

static t_bool mikasa_ncr_scsi_no_lun_cmd (uint32 unit, const uint8 *cdb,
    const MIKASA_NCR_DMA_LIST *data, uint8 *status)
{
uint8 buf[36];
uint32 len;
uint32 xfer_len;

if ((cdb[0] == 0x12) && ((cdb[1] & 1) == 0) && (cdb[4] != 0) &&
    (data->bytes != 0)) {
    memset (buf, 0, sizeof (buf));
    buf[0] = 0x7F;                         /* no logical unit present */
    buf[4] = 31;
    len = mikasa_ncr_min3 (data->bytes, cdb[4], sizeof (buf));
    xfer_len = (data->bytes < cdb[4]) ? data->bytes : cdb[4];
    sim_debug (MIKASA_DBG_PCI, &mikasa_dev,
        "NCR INQUIRY target=%u no-lun alloc=%u bytes=%u xfer=%u pdt=%02X "
        "addlen=%u\n", unit, cdb[4], len, xfer_len, buf[0], buf[4]);
    mikasa_ncr_clear_sense (unit);
    *status = 0;
    return mikasa_ncr_write_scsi_data_padded (data, buf, len, xfer_len);
    }
mikasa_ncr_set_sense (unit, 0x05, 0x25, 0x00);
*status = 2;
return mikasa_ncr_write_dma_list_zero (data);
}

static t_bool mikasa_ncr_run_control_script_phase (uint32 dsp, uint32 phase,
    uint8 sfbr)
{
uint32 stack[MIKASA_NCR_SCRIPT_STACK];
MIKASA_NCR_SCRIPT_STATE state;
uint32 sp = 0;
uint32 i;

memset (stack, 0, sizeof (stack));
memset (&state, 0, sizeof (state));
state.phase = phase;
state.sfbr = sfbr;
state.side_effects = TRUE;
for (i = 0; i < MIKASA_NCR_SCRIPT_SCAN_INSNS; i++) {
    uint32 op;
    uint32 arg;
    uint32 next;
    uint32 group;

    if (!mikasa_ncr_fetch_script (dsp, &op, &arg, TRUE))
        return FALSE;
    group = op & MIKASA_NCR_TC_GROUP_MASK;
    if ((group == MIKASA_NCR_TC_INT) &&
        ((op & MIKASA_NCR_TC_INTFLY) == 0) &&
        mikasa_ncr_script_condition (op, &state)) {
        mikasa_ncr_set_dip (MIKASA_NCR_DSTAT_SIR, arg);
        return TRUE;
        }
    next = mikasa_ncr_next_script_addr (dsp, op);
    if (!mikasa_ncr_advance_script (&dsp, op, arg, next, stack, &sp,
        &state))
        return mikasa_ncr_wait_reselect ? TRUE : FALSE;
    }
return FALSE;
}

static t_bool mikasa_ncr_run_control_script (uint32 dsp)
{
return mikasa_ncr_run_control_script_phase (dsp, MIKASA_NCR_SCRIPT_NO_PHASE,
    mikasa_ncr_reg[MIKASA_NCR_REG_SFBR]);
}

static t_bool mikasa_ncr_run_script (uint32 dsp)
{
uint32 dsa = mikasa_ncr_reg_l (MIKASA_NCR_REG_DSA);
uint32 cmd_count;
uint32 cmd_move_count;
uint32 cmd_addr;
uint32 cmd_off;
uint32 cmd_next_dsp = dsp;
uint32 data_off;
uint32 data_phase = MIKASA_NCR_PHASE_DAT_IN;
uint32 target;
uint32 i;
uint32 data_dsps;
MIKASA_NCR_DMA_LIST data;
MIKASA_NCR_SCRIPT_SCAN cmd_scan;
t_bool has_data;
uint8 cdb[16];
uint8 status;

if (!mikasa_ncr_find_select (dsp, dsa, &target)) {
    mikasa_ncr_clear_transaction ();
    return mikasa_ncr_run_control_script (dsp);
    }
if ((target >= MIKASA_DKA_UNITS) || ((dka_unit[target].flags & UNIT_ATT) == 0)) {
    sim_debug (MIKASA_DBG_PCI, &mikasa_dev,
        "NCR target %u select timeout\n", target);
    mikasa_ncr_disc_track_timeout (target);
    mikasa_ncr_debug_state ("select-timeout");
    mikasa_ncr_clear_transaction ();
    mikasa_ncr_reg[MIKASA_NCR_REG_SCNTL2] &= ~MIKASA_NCR_SCNTL2_SDU;
    mikasa_ncr_set_connected (FALSE);
    mikasa_ncr_set_sip (MIKASA_NCR_SIST0_UDC, MIKASA_NCR_SIST1_STO);
    return TRUE;
    }
mikasa_ncr_begin_transaction (dsp, dsa, target);
mikasa_ncr_set_connected (TRUE);
mikasa_ncr_set_sip (MIKASA_NCR_SIST0_CMP, 0);
if (!mikasa_ncr_do_msg_out (dsp, dsa)) {
    mikasa_ncr_clear_transaction ();
    mikasa_ncr_set_connected (FALSE);
    return FALSE;
    }
if (!mikasa_ncr_scan_script (dsp, dsa, MIKASA_NCR_PHASE_CMD, FALSE, FALSE,
    &cmd_scan)) {
    mikasa_ncr_clear_transaction ();
    mikasa_ncr_set_connected (FALSE);
    return FALSE;
    }
if (cmd_scan.table_valid)
    cmd_off = cmd_scan.table_off;
else if (!mikasa_ncr_find_linear_move (dsp, MIKASA_NCR_PHASE_CMD, &cmd_off,
    &cmd_next_dsp))
    cmd_off = 0x0C;
if (cmd_scan.phase_next_valid)
    cmd_next_dsp = cmd_scan.phase_next_dsp;
if (!mikasa_ncr_table_entry (dsa, cmd_off, &cmd_count, &cmd_addr) ||
    (cmd_count == 0)) {
    mikasa_ncr_clear_transaction ();
    mikasa_ncr_set_connected (FALSE);
    return FALSE;
    }
cmd_move_count = cmd_count;
if (cmd_count > sizeof (cdb))
    cmd_count = sizeof (cdb);
memset (cdb, 0, sizeof (cdb));
if (!mikasa_ncr_read_dma_buf (cmd_addr, cdb, cmd_count)) {
    mikasa_ncr_clear_transaction ();
    mikasa_ncr_set_connected (FALSE);
    return FALSE;
    }
if (cmd_count != 0)
    mikasa_ncr_set_output_latch (cdb[0]);
mikasa_ncr_finish_move (mikasa_ncr_move_op_for_phase (MIKASA_NCR_PHASE_CMD),
    cmd_addr, cmd_move_count, cmd_count);
has_data = mikasa_ncr_cmd_data_phase (cdb, &data_phase);
mikasa_ncr_dma_list_init (&data);
if (has_data && !mikasa_ncr_collect_moves (dsp, dsa, data_phase, &data)) {
    uint32 data_count = 0;
    uint32 data_addr = 0;

    mikasa_ncr_dma_list_init (&data);
    if (!mikasa_ncr_find_table_move (dsp, data_phase, &data_off))
        data_off = (data_phase == MIKASA_NCR_PHASE_DAT_OUT) ? 0x14 : 0x1C;
    if (mikasa_ncr_table_entry (dsa, data_off, &data_count, &data_addr))
        (void) mikasa_ncr_dma_list_append (&data, data_count, data_addr,
            mikasa_ncr_move_op_for_phase (data_phase));
    }

sim_debug (MIKASA_DBG_PCI, &mikasa_dev,
    "NCR target %u lun %u CDB", target, mikasa_ncr_transaction.lun);
for (i = 0; i < cmd_count; i++)
    sim_debug (MIKASA_DBG_PCI, &mikasa_dev, " %02X", cdb[i]);
sim_debug (MIKASA_DBG_PCI, &mikasa_dev, " data_segs=%u/%u\n", data.count,
    data.bytes);
mikasa_ncr_disc_track_cmd (target, mikasa_ncr_transaction.lun, cdb);
mikasa_ncr_debug_state ("scsi-cdb");

if (mikasa_ncr_transaction.lun != 0) {
    if (!mikasa_ncr_scsi_no_lun_cmd (target, cdb, &data, &status)) {
        status = 2;
        (void) mikasa_ncr_write_dma_list_zero (&data);
        }
    }
else if (!mikasa_ncr_scsi_cmd (target, cdb, &data, &status)) {
    status = 2;
    (void) mikasa_ncr_write_dma_list_zero (&data);
    }
sim_debug (MIKASA_DBG_PCI, &mikasa_dev,
    "NCR target %u lun %u status=%02X data_phase=%u resid=%u\n",
    target, mikasa_ncr_transaction.lun, status, data_phase,
    mikasa_ncr_reg_l (MIKASA_NCR_REG_DBC) & MIKASA_NCR_BM_COUNT_MASK);
mikasa_ncr_debug_state ("scsi-status");
if (data.bytes != 0) {
    uint32 data_resid;

    mikasa_ncr_defer_status_phase (dsp, dsa, target, data_phase, status);
    data_resid = mikasa_ncr_reg_l (MIKASA_NCR_REG_DBC) &
        MIKASA_NCR_BM_COUNT_MASK;
    if (data_resid != 0) {
        mikasa_ncr_disc_log ("NCRDISC pass=%u mismatch-resid tgt=%u lun=%u "
            "resid=%u data_phase=%u dsp=%08X dsa=%08X\n",
            mikasa_ncr_disc_pass, target, mikasa_ncr_transaction.lun,
            data_resid, data_phase, dsp, dsa);
        mikasa_ncr_signal_phase_mismatch (data_phase, MIKASA_NCR_PHASE_STS);
        return TRUE;
        }
    if (!mikasa_ncr_last_move_phase (data_phase)) {
        mikasa_ncr_disc_log ("NCRDISC pass=%u mismatch-lastmove tgt=%u lun=%u "
            "data_phase=%u dsp=%08X dsa=%08X\n", mikasa_ncr_disc_pass, target,
            mikasa_ncr_transaction.lun, data_phase, dsp, dsa);
        mikasa_ncr_phase_mismatch (&data, data_phase,
            MIKASA_NCR_PHASE_STS);
        return TRUE;
        }
    data_dsps = mikasa_ncr_data_int_dsps (dsp, data_phase);
    mikasa_ncr_disc_track_completion (target, mikasa_ncr_transaction.lun,
        cdb, &data, status, data_dsps, data_resid);
    mikasa_ncr_set_dip (MIKASA_NCR_DSTAT_SIR, data_dsps);
    }
else {
    uint32 status_dsps = mikasa_ncr_status_int_dsps (cmd_next_dsp);

    mikasa_ncr_latch_phase (MIKASA_NCR_PHASE_STS, MIKASA_NCR_PHASE_STS);
    mikasa_ncr_disc_track_completion (target, mikasa_ncr_transaction.lun,
        cdb, &data, status, status_dsps, 0);
    if (!mikasa_ncr_run_control_script_phase (cmd_next_dsp,
        MIKASA_NCR_PHASE_STS, status)) {
        mikasa_ncr_set_connected (FALSE);
        if (!mikasa_ncr_wait_reselect)
            mikasa_ncr_set_dip (MIKASA_NCR_DSTAT_BF, cmd_next_dsp);
        }
    else
        mikasa_ncr_set_connected (FALSE);
    mikasa_ncr_clear_transaction ();
    }
return TRUE;
}

static t_bool mikasa_ncr_step_script (uint32 dsp)
{
uint32 stack[MIKASA_NCR_SCRIPT_STACK];
MIKASA_NCR_SCRIPT_STATE state;
uint32 sp = 0;
uint32 op;
uint32 arg;
uint32 next;
uint32 group;
uint32 dsps;

memset (stack, 0, sizeof (stack));
memset (&state, 0, sizeof (state));
state.phase = MIKASA_NCR_SCRIPT_NO_PHASE;
state.sfbr = mikasa_ncr_reg[MIKASA_NCR_REG_SFBR];
state.side_effects = TRUE;
if (!mikasa_ncr_fetch_script (dsp, &op, &arg, TRUE))
    return FALSE;
next = mikasa_ncr_next_script_addr (dsp, op);
if ((op & MIKASA_NCR_TC_GROUP_MASK) == MIKASA_NCR_SCR_SEL_ABS) {
    uint32 dsa = mikasa_ncr_reg_l (MIKASA_NCR_REG_DSA);
    uint32 sel = (op >> 16) & 0x0Fu;
    uint32 target;
    t_bool table = FALSE;

    if (op & 0x02000000u) {
        uint32 table_addr = (dsa + mikasa_ncr_sext24 (op)) & ~3u;

        if (!mikasa_pci_dma_read_long (table_addr, &sel))
            return FALSE;
        table = TRUE;
        }
    mikasa_ncr_select_latches (op, table ? sel : (sel << 16), table);
    target = table ? ((sel >> 16) & MIKASA_NCR_SDID_WRITABLE) :
        (sel & MIKASA_NCR_SDID_WRITABLE);
    if ((target < MIKASA_DKA_UNITS) &&
        ((dka_unit[target].flags & UNIT_ATT) != 0)) {
        mikasa_ncr_set_connected (TRUE);
        mikasa_ncr_set_sip (MIKASA_NCR_SIST0_CMP, 0);
        }
    else {
        mikasa_ncr_reg[MIKASA_NCR_REG_SCNTL2] &= ~MIKASA_NCR_SCNTL2_SDU;
        mikasa_ncr_set_sip (MIKASA_NCR_SIST0_UDC, MIKASA_NCR_SIST1_STO);
        }
    mikasa_ncr_set_reg_l (MIKASA_NCR_REG_DSP, next);
    mikasa_ncr_set_dip (MIKASA_NCR_DSTAT_SSI,
        mikasa_ncr_reg_l (MIKASA_NCR_REG_DSPS));
    return TRUE;
    }
group = op & MIKASA_NCR_TC_GROUP_MASK;
if ((group == MIKASA_NCR_TC_INT) &&
    ((op & MIKASA_NCR_TC_INTFLY) == 0) &&
    mikasa_ncr_script_condition (op, &state)) {
    mikasa_ncr_set_dip (MIKASA_NCR_DSTAT_SIR | MIKASA_NCR_DSTAT_SSI, arg);
    return TRUE;
    }
if (!mikasa_ncr_advance_script (&dsp, op, arg, next, stack, &sp, &state))
    mikasa_ncr_set_reg_l (MIKASA_NCR_REG_DSP, next);
else
    mikasa_ncr_set_reg_l (MIKASA_NCR_REG_DSP, dsp);
dsps = mikasa_ncr_reg_l (MIKASA_NCR_REG_DSPS);
mikasa_ncr_set_dip (MIKASA_NCR_DSTAT_SSI, dsps);
return TRUE;
}

static void mikasa_ncr_start_script (uint32 dsp)
{
if (mikasa_ncr_transaction.status_pending) {
    uint32 status_dsp = dsp;
    uint32 status_dsa = mikasa_ncr_transaction.dsa;
    uint8 status = mikasa_ncr_transaction.status;

    mikasa_ncr_transaction.status_pending = FALSE;
    mikasa_ncr_latch_phase (MIKASA_NCR_PHASE_STS,
        MIKASA_NCR_PHASE_STS);
    /*
     * Data-phase commands defer STATUS/MESSAGE completion here; write both
     * bytes through the script table/direct MOVE targets so firmware-side
     * completion structures see the same data path as no-data commands.
     */
    mikasa_ncr_write_status_msg (status_dsp, status_dsa, status);
    mikasa_ncr_trace_script (dsp);
    if (!mikasa_ncr_run_control_script_phase (dsp,
        MIKASA_NCR_PHASE_STS, status)) {
        mikasa_ncr_set_connected (FALSE);
        if (!mikasa_ncr_wait_reselect)
            mikasa_ncr_set_dip (MIKASA_NCR_DSTAT_BF, dsp);
        }
    else
        mikasa_ncr_set_connected (FALSE);
    mikasa_ncr_clear_transaction ();
    }
else {
    mikasa_ncr_trace_script (dsp);
    if (mikasa_ncr_reg[MIKASA_NCR_REG_DCNTL] & MIKASA_NCR_DCNTL_SSM) {
        if (!mikasa_ncr_step_script (dsp)) {
            mikasa_ncr_set_connected (FALSE);
            if (!mikasa_ncr_wait_reselect)
                mikasa_ncr_set_dip (MIKASA_NCR_DSTAT_BF, dsp);
            }
        }
    else if (!mikasa_ncr_run_script (dsp)) {
        mikasa_ncr_set_connected (FALSE);
        if (!mikasa_ncr_wait_reselect)
            mikasa_ncr_set_dip (MIKASA_NCR_DSTAT_BF, dsp);
        }
    }
return;
}

static void mikasa_ncr_init_regs (void)
{
memset (mikasa_ncr_reg, 0, sizeof (mikasa_ncr_reg));
mikasa_ncr_dstat_stack = 0;
mikasa_ncr_sist0_stack = 0;
mikasa_ncr_sist1_stack = 0;
mikasa_ncr_dsps_stack_valid = FALSE;
mikasa_ncr_dsps_stack = 0;
mikasa_ncr_disc_pass = 0;
mikasa_ncr_disc_pass_armed = FALSE;
mikasa_ncr_disc_last_target = M32;
mikasa_ncr_disc_last_lun = M32;
mikasa_ncr_disc_last_cdb = 0xFF;
mikasa_ncr_clear_transaction ();
mikasa_ncr_clear_wait_reselect ();
mikasa_ncr_clear_irq ();
mikasa_ncr_set_connected (FALSE);
mikasa_ncr_reg[MIKASA_NCR_REG_SCNTL0] = MIKASA_NCR_SCNTL0_ARB;
mikasa_ncr_reg[MIKASA_NCR_REG_SCID] = 7;
mikasa_ncr_reg[MIKASA_NCR_REG_RESPID] = 0x80;
mikasa_ncr_reg[MIKASA_NCR_REG_DSTAT] = MIKASA_NCR_DSTAT_DFE;
mikasa_ncr_reg[MIKASA_NCR_REG_CTEST1] = MIKASA_NCR_CTEST1_FMT;
mikasa_ncr_reg[MIKASA_NCR_REG_CTEST2] = MIKASA_NCR_CTEST2_DACK;
mikasa_ncr_reg[MIKASA_NCR_REG_CTEST3] = MIKASA_NCR_CTEST3_REV;
mikasa_ncr_reg[MIKASA_NCR_REG_MACNTL] = MIKASA_NCR_MACNTL_810;
mikasa_ncr_reg[MIKASA_NCR_REG_GPCNTL] = 0x0F;
mikasa_ncr_reg[MIKASA_NCR_REG_STEST0] = 0x03;
return;
}

static void mikasa_ncr_abort_script (void)
{
mikasa_ncr_clear_transaction ();
mikasa_ncr_clear_wait_reselect ();
mikasa_ncr_set_connected (FALSE);
mikasa_ncr_reg[MIKASA_NCR_REG_ISTAT] &= ~MIKASA_NCR_ISTAT_ABRT;
mikasa_ncr_set_dip (MIKASA_NCR_DSTAT_ABRT, 0);
return;
}

static void mikasa_ncr_clear_scsi_fifo (void)
{
mikasa_ncr_reg[MIKASA_NCR_REG_SSTAT0] &=
    ~(MIKASA_NCR_SSTAT0_ILF | MIKASA_NCR_SSTAT0_ORF |
    MIKASA_NCR_SSTAT0_OLF);
mikasa_ncr_reg[MIKASA_NCR_REG_SSTAT1] &= ~MIKASA_NCR_SSTAT1_FIFO;
mikasa_ncr_reg[MIKASA_NCR_REG_SSTAT2] &=
    ~(MIKASA_NCR_SSTAT2_ILF1 | MIKASA_NCR_SSTAT2_ORF1 |
    MIKASA_NCR_SSTAT2_OLF1);
return;
}

static void mikasa_ncr_write_ctest3 (uint8 val)
{
if (val & (MIKASA_NCR_CTEST3_CLF | MIKASA_NCR_CTEST3_FLF))
    mikasa_ncr_reg[MIKASA_NCR_REG_DSTAT] |= MIKASA_NCR_DSTAT_DFE;
if (val & MIKASA_NCR_CTEST3_CLF)
    mikasa_ncr_reg[MIKASA_NCR_REG_DFIFO] = 0;
mikasa_ncr_reg[MIKASA_NCR_REG_CTEST3] =
    MIKASA_NCR_CTEST3_REV | (val & MIKASA_NCR_CTEST3_WRITABLE);
return;
}

static void mikasa_ncr_write_ctest5 (uint8 val)
{
uint32 dnad = mikasa_ncr_reg_l (MIKASA_NCR_REG_DNAD);
uint32 dbc = mikasa_ncr_reg_l (MIKASA_NCR_REG_DBC);

if (val & MIKASA_NCR_CTEST5_ADCK) {
    dnad = dnad + 1;
    mikasa_ncr_set_reg_l (MIKASA_NCR_REG_DNAD, dnad);
    mikasa_ncr_set_reg_l (MIKASA_NCR_REG_ADDER, dnad);
    }
if (val & MIKASA_NCR_CTEST5_BBCK) {
    dbc = (dbc & 0xFF000000u) |
        (((dbc & MIKASA_NCR_BM_COUNT_MASK) - 1) & MIKASA_NCR_BM_COUNT_MASK);
    mikasa_ncr_set_reg_l (MIKASA_NCR_REG_DBC, dbc);
    mikasa_ncr_set_reg_l (MIKASA_NCR_REG_ADDER,
        dnad + (dbc & MIKASA_NCR_BM_COUNT_MASK));
    }
mikasa_ncr_reg[MIKASA_NCR_REG_CTEST5] = val & MIKASA_NCR_CTEST5_WRITABLE;
return;
}

static void mikasa_ncr_write_stest3 (uint8 val)
{
if (val & MIKASA_NCR_STEST3_CSF)
    mikasa_ncr_clear_scsi_fifo ();
mikasa_ncr_reg[MIKASA_NCR_REG_STEST3] =
    (val & MIKASA_NCR_STEST3_WRITABLE) & ~MIKASA_NCR_STEST3_CSF;
return;
}

static void mikasa_ncr_write_b (uint32 reg, uint8 val)
{
reg = reg & 0x7Fu;
if (sim_deb && (mikasa_dev.dctrl & MIKASA_DBG_PCI))
    sim_debug (MIKASA_DBG_PCI, &mikasa_dev,
        "NCR write pc=%llX ra=%llX fra=%llX pra=%llX %02X=%02X\n",
        (unsigned long long) PC, (unsigned long long) R[26],
        (unsigned long long) mikasa_ncr_frame_ra (),
        (unsigned long long) mikasa_ncr_parent_frame_ra (), reg, val);
if (reg == MIKASA_NCR_REG_ISTAT) {
    uint8 istat = mikasa_ncr_reg[MIKASA_NCR_REG_ISTAT];
    t_bool sigp_run = FALSE;
    uint32 sigp_dsp = 0;

    if (val & MIKASA_NCR_ISTAT_INTF)
        istat &= ~MIKASA_NCR_ISTAT_INTF;
    mikasa_ncr_reg[MIKASA_NCR_REG_ISTAT] =
        (istat & (MIKASA_NCR_ISTAT_INTF | MIKASA_NCR_ISTAT_CON |
         MIKASA_NCR_ISTAT_SIP | MIKASA_NCR_ISTAT_DIP)) |
        (val & (MIKASA_NCR_ISTAT_SEM | MIKASA_NCR_ISTAT_SIGP));
    if (val & MIKASA_NCR_ISTAT_SRST)
        mikasa_ncr_init_regs ();
    else if (val & MIKASA_NCR_ISTAT_ABRT)
        mikasa_ncr_abort_script ();
    else if (val & MIKASA_NCR_ISTAT_SIGP) {
        if (mikasa_ncr_wait_reselect) {
            sigp_dsp = mikasa_ncr_wait_reselect_jump;
            mikasa_ncr_clear_wait_reselect ();
            mikasa_ncr_set_reg_l (MIKASA_NCR_REG_DSP, sigp_dsp);
            sigp_run = TRUE;
            }
        }
    else
        mikasa_ncr_reg[MIKASA_NCR_REG_ISTAT] &= ~MIKASA_NCR_ISTAT_SIGP;
    mikasa_ncr_update_irq ();
    if (sigp_run)
        mikasa_ncr_start_script (sigp_dsp);
    }
else if ((reg == MIKASA_NCR_REG_DSTAT) ||
    ((reg >= MIKASA_NCR_REG_ADDER) &&
    (reg < (MIKASA_NCR_REG_ADDER + 4))) ||
    (reg == MIKASA_NCR_REG_SSID) ||
    (reg == MIKASA_NCR_REG_SBCL) ||
    (reg == MIKASA_NCR_REG_SSTAT0) ||
    (reg == MIKASA_NCR_REG_SSTAT1) ||
    (reg == MIKASA_NCR_REG_SSTAT2) ||
    (reg == MIKASA_NCR_REG_SIST0) ||
    (reg == MIKASA_NCR_REG_SIST1) ||
    (reg == MIKASA_NCR_REG_SIDL) ||
    (reg == (MIKASA_NCR_REG_SIDL + 1)) ||
    (reg == MIKASA_NCR_REG_SBDL) ||
    (reg == (MIKASA_NCR_REG_SBDL + 1)))
    return;
else {
    uint8 old = mikasa_ncr_reg[reg];

    if (reg == MIKASA_NCR_REG_SODL) {
        mikasa_ncr_set_output_latch (val);
        return;
        }
    if (reg == (MIKASA_NCR_REG_SODL + 1)) {
        mikasa_ncr_reg[reg] = val;
        mikasa_ncr_reg[MIKASA_NCR_REG_SBDL + 1] = val;
        return;
        }
    if (reg == MIKASA_NCR_REG_SCNTL0) {
        mikasa_ncr_write_scntl0 (val);
        return;
        }
    if (reg == MIKASA_NCR_REG_SCNTL1) {
        mikasa_ncr_write_scntl1 (val);
        return;
        }
    if (reg == MIKASA_NCR_REG_SCNTL2)
        val = (old & MIKASA_NCR_SCNTL2_W1C &
            ~(val & MIKASA_NCR_SCNTL2_W1C)) |
            (val & MIKASA_NCR_SCNTL2_WRITABLE);
    if (reg == MIKASA_NCR_REG_SCID)
        val = val & MIKASA_NCR_SCID_WRITABLE;
    if (reg == MIKASA_NCR_REG_SCNTL3)
        val = val & MIKASA_NCR_SCNTL3_WRITABLE;
    if (reg == MIKASA_NCR_REG_SDID)
        val = val & MIKASA_NCR_SDID_WRITABLE;
    if (reg == MIKASA_NCR_REG_GPREG)
        val = val & MIKASA_NCR_GPREG_WRITABLE;
    if (reg == MIKASA_NCR_REG_CTEST3)
        mikasa_ncr_write_ctest3 (val);
    else if (reg == MIKASA_NCR_REG_CTEST2)
        val = val & MIKASA_NCR_CTEST2_WRITABLE;
    else if (reg == MIKASA_NCR_REG_CTEST5) {
        mikasa_ncr_write_ctest5 (val);
        return;
        }
    else if (reg == MIKASA_NCR_REG_DMODE)
        val = val & MIKASA_NCR_DMODE_WRITABLE;
    else if (reg == MIKASA_NCR_REG_DIEN)
        val = val & MIKASA_NCR_DSTAT_IRQS;
    else if (reg == MIKASA_NCR_REG_DCNTL)
        val = (val & MIKASA_NCR_DCNTL_WRITABLE) & ~MIKASA_NCR_DCNTL_PFF;
    if (reg == MIKASA_NCR_REG_SIEN1)
        val = val & MIKASA_NCR_SIEN1_WRITABLE;
    if (reg == MIKASA_NCR_REG_MACNTL)
        val = val & MIKASA_NCR_MACNTL_WRITABLE;
    if (reg == MIKASA_NCR_REG_GPCNTL)
        val = val & MIKASA_NCR_GPCNTL_WRITABLE;
    if (reg == MIKASA_NCR_REG_STIME1)
        val = val & MIKASA_NCR_STIME1_WRITABLE;
    if (reg == MIKASA_NCR_REG_STEST1)
        val = val & MIKASA_NCR_STEST1_WRITABLE;
    if (reg == MIKASA_NCR_REG_STEST2)
        val = val & MIKASA_NCR_STEST2_WRITABLE;
    if (reg == MIKASA_NCR_REG_STEST3) {
        mikasa_ncr_write_stest3 (val);
        return;
        }
    if (reg != MIKASA_NCR_REG_CTEST3)
        mikasa_ncr_reg[reg] = val;
    if (reg == MIKASA_NCR_REG_SOCL)
        mikasa_ncr_reg[MIKASA_NCR_REG_SBCL] =
            (mikasa_ncr_reg[MIKASA_NCR_REG_SBCL] &
            ~(MIKASA_NCR_SOCL_ACK | MIKASA_NCR_SOCL_ATN)) |
            (val & (MIKASA_NCR_SOCL_ACK | MIKASA_NCR_SOCL_ATN));
    if ((reg == MIKASA_NCR_REG_DIEN) || (reg == MIKASA_NCR_REG_SIEN0) ||
        (reg == MIKASA_NCR_REG_SIEN1))
        mikasa_ncr_update_irq ();
    if ((reg == MIKASA_NCR_REG_DCNTL) && (val & MIKASA_NCR_DCNTL_STD)) {
        mikasa_ncr_reg[reg] = val & ~MIKASA_NCR_DCNTL_STD;
        mikasa_ncr_start_script (mikasa_ncr_reg_l (MIKASA_NCR_REG_DSP));
        }
    if (reg == (MIKASA_NCR_REG_DSP + 3)) {
    uint32 dsp = mikasa_ncr_reg_l (MIKASA_NCR_REG_DSP);

    if (((mikasa_ncr_reg[MIKASA_NCR_REG_DMODE] & MIKASA_NCR_DMODE_MAN) == 0) &&
        ((mikasa_ncr_reg[MIKASA_NCR_REG_DCNTL] & MIKASA_NCR_DCNTL_SSM) == 0))
        mikasa_ncr_start_script (dsp);
    }
    }
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

static uint32 mikasa_tulip_conf_read_l (uint32 func, uint32 reg)
{
if (func != 0)
    return 0xFFFFFFFFu;
return mikasa_tulip_cfg[reg >> 2];
}

static void mikasa_tulip_conf_write_l (uint32 func, uint32 reg, uint32 val)
{
uint32 mask;

if (func != 0)
    return;
mask = mikasa_tulip_conf_mask (reg);
if (mask != 0)
    mikasa_tulip_cfg[reg >> 2] =
        mikasa_masked_update (mikasa_tulip_cfg[reg >> 2], val, mask);
return;
}

static uint32 mikasa_vga_conf_read_l (uint32 func, uint32 reg)
{
if (func != 0)
    return 0xFFFFFFFFu;
return mikasa_vga_cfg[reg >> 2];
}

static void mikasa_vga_conf_write_l (uint32 func, uint32 reg, uint32 val)
{
uint32 mask;

if (func != 0)
    return;
mask = mikasa_vga_conf_mask (reg);
if (mask != 0)
    mikasa_vga_cfg[reg >> 2] =
        mikasa_masked_update (mikasa_vga_cfg[reg >> 2], val, mask);
return;
}

static uint32 mikasa_pci_conf_read_l (uint32 cfg)
{
uint32 bus = (cfg >> 16) & 0xFF;
uint32 slot = (cfg >> 11) & 0x1F;
uint32 func = (cfg >> 8) & 7;
uint32 reg = cfg & 0xFC;
uint32 val = 0xFFFFFFFFu;

if ((bus == 0) && (slot == MIKASA_PCI_SLOT_VGA)) {
    if (!mikasa_vga_enabled)
        return val;
    val = mikasa_vga_conf_read_l (func, reg);
    sim_debug (MIKASA_DBG_PCI, &mikasa_dev,
        "PCI config read bus %u slot %u func %u reg %02X=%08X\n",
        bus, slot, func, reg, val);
    return val;
    }
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
if ((bus == 0) && (slot == MIKASA_PCI_SLOT_TULIP)) {
    val = mikasa_tulip_conf_read_l (func, reg);
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

if ((bus == 0) && (slot == MIKASA_PCI_SLOT_VGA)) {
    if (!mikasa_vga_enabled)
        return;
    mikasa_vga_conf_write_l (func, reg, val);
    sim_debug (MIKASA_DBG_PCI, &mikasa_dev,
        "PCI config write bus %u slot %u func %u reg %02X=%08X\n",
        bus, slot, func, reg, val);
    return;
    }
if ((bus == 0) && (slot == MIKASA_PCI_SLOT_PCEB) && (func == 0)) {
    uint32 mask = mikasa_pceb_conf_mask (reg);

    if ((mask != 0) && !mikasa_pceb_bar_reg (reg))
        mikasa_pceb_cfg[reg >> 2] =
            mikasa_masked_update (mikasa_pceb_cfg[reg >> 2], val, mask);
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
if ((bus == 0) && (slot == MIKASA_PCI_SLOT_TULIP)) {
    mikasa_tulip_conf_write_l (func, reg, val);
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
uint32 reg = cfg & ~3u;
uint32 old;
uint32 data;
uint32 mask;
uint32 shift;

if (mode == 3) {
    data = (uint32) val;
    mask = M32;
    }
else {
    old = mikasa_pci_conf_read_l (reg);
    if (mode == 1) {
        shift = 8 * (cfg & 2);
        data = ((uint32) (val >> shift)) & 0xFFFFu;
        mask = 0xFFFFu << shift;
        }
    else {
        shift = 8 * (cfg & 3);
        data = (lnt == L_BYTE) ? ((uint32) val & 0xFFu) :
            (((uint32) (val >> shift)) & 0xFFu);
        mask = 0xFFu << shift;
        }
    data = (old & ~mask) | ((data << shift) & mask);
    }
mikasa_pci_conf_write_l (reg, data);
return;
}

static void mikasa_kbc_reset (void)
{
mikasa_kbc_cmd_byte = 0;
mikasa_kbc_out_port = MIKASA_KBC_OUT_NORMAL;
mikasa_kbc_pending_cmd = 0;
mikasa_kbd_pending_cmd = 0;
memset (mikasa_kbc_resp, 0, sizeof (mikasa_kbc_resp));
memset (mikasa_kbc_resp_aux, 0, sizeof (mikasa_kbc_resp_aux));
mikasa_kbc_resp_head = 0;
mikasa_kbc_resp_count = 0;
mikasa_pic_set_irq (MIKASA_KBD_IRQ, FALSE);
mikasa_pic_set_irq (MIKASA_MOUSE_IRQ, FALSE);
return;
}

static void mikasa_kbc_update_irq (void)
{
t_bool kbd = FALSE;
t_bool aux = FALSE;
uint32 i;

for (i = 0; i < mikasa_kbc_resp_count; i++) {
    uint32 index = (mikasa_kbc_resp_head + i) % MIKASA_KBC_RESP_SIZE;

    if (mikasa_kbc_resp_aux[index])
        aux = TRUE;
    else
        kbd = TRUE;
    }
mikasa_pic_set_irq (MIKASA_KBD_IRQ,
    kbd && (mikasa_kbc_cmd_byte & MIKASA_KBC_CCB_KBD_INT) &&
    ((mikasa_kbc_cmd_byte & MIKASA_KBC_CCB_KBD_DISABLE) == 0));
mikasa_pic_set_irq (MIKASA_MOUSE_IRQ,
    aux && (mikasa_kbc_cmd_byte & MIKASA_KBC_CCB_AUX_INT) &&
    ((mikasa_kbc_cmd_byte & MIKASA_KBC_CCB_AUX_DISABLE) == 0));
return;
}

static void mikasa_kbc_queue_src (uint8 val, t_bool aux)
{
uint32 tail;

if (mikasa_kbc_resp_count >= MIKASA_KBC_RESP_SIZE)
    return;
tail = (mikasa_kbc_resp_head + mikasa_kbc_resp_count) %
    MIKASA_KBC_RESP_SIZE;
mikasa_kbc_resp[tail] = val;
mikasa_kbc_resp_aux[tail] = aux;
mikasa_kbc_resp_count++;
mikasa_kbc_update_irq ();
return;
}

static void mikasa_kbc_queue (uint8 val)
{
mikasa_kbc_queue_src (val, FALSE);
return;
}

static void mikasa_kbc_queue_aux (uint8 val)
{
mikasa_kbc_queue_src (val, TRUE);
return;
}

static uint8 mikasa_kbc_read_data (void)
{
uint8 val = 0;

if (mikasa_kbc_resp_count != 0) {
    val = mikasa_kbc_resp[mikasa_kbc_resp_head];
    mikasa_kbc_resp_aux[mikasa_kbc_resp_head] = FALSE;
    mikasa_kbc_resp_head = (mikasa_kbc_resp_head + 1) %
        MIKASA_KBC_RESP_SIZE;
    mikasa_kbc_resp_count--;
    }
mikasa_kbc_update_irq ();
sim_debug (MIKASA_DBG_IO, &mikasa_dev, "KBC data read %02X\n", val);
return val;
}

static uint8 mikasa_kbc_read (uint32 port)
{
uint8 status;

if (port == MIKASA_ISA_KBD_DATA)
    return mikasa_kbc_read_data ();
status = MIKASA_KBC_STATUS_SYS |
    ((mikasa_kbc_resp_count != 0) ? MIKASA_KBC_STATUS_OBF : 0) |
    ((mikasa_kbc_resp_count != 0) &&
    mikasa_kbc_resp_aux[mikasa_kbc_resp_head] ? MIKASA_KBC_STATUS_AUX : 0);
sim_debug (MIKASA_DBG_IO, &mikasa_dev, "KBC status read %02X\n", status);
return status;
}

static void mikasa_kbc_write_data (uint8 val)
{
if (mikasa_kbc_pending_cmd == MIKASA_KBC_CMD_WRITE_BYTE) {
    mikasa_kbc_cmd_byte = val;
    mikasa_kbc_pending_cmd = 0;
    mikasa_kbc_update_irq ();
    }
else if (mikasa_kbc_pending_cmd == MIKASA_KBC_CMD_WRITE_OUT) {
    mikasa_kbc_out_port = val;
    mikasa_kbc_pending_cmd = 0;
    }
else if (mikasa_kbc_pending_cmd == MIKASA_KBC_CMD_WRITE_AUX) {
    mikasa_kbc_pending_cmd = 0;
    if ((mikasa_kbc_cmd_byte & MIKASA_KBC_CCB_AUX_DISABLE) == 0)
        mikasa_kbc_queue_aux (MIKASA_KBD_ACK);
    }
else if (mikasa_kbd_pending_cmd != 0) {
    mikasa_kbd_pending_cmd = 0;
    mikasa_kbc_queue (MIKASA_KBD_ACK);
    }
else {
    switch (val) {
        case 0xED:                              /* set LEDs */
        case 0xF0:                              /* select scan set */
        case 0xF3:                              /* typematic rate */
            mikasa_kbd_pending_cmd = val;
            mikasa_kbc_queue (MIKASA_KBD_ACK);
            break;

        case 0xEE:                              /* echo */
            mikasa_kbc_queue (0xEE);
            break;

        case 0xF2:                              /* read keyboard ID */
            mikasa_kbc_queue (MIKASA_KBD_ACK);
            mikasa_kbc_queue (MIKASA_KBD_ID1);
            mikasa_kbc_queue (MIKASA_KBD_ID2);
            break;

        case 0xF4:                              /* enable scanning */
        case 0xF5:                              /* disable scanning */
        case 0xF6:                              /* set defaults */
            mikasa_kbc_queue (MIKASA_KBD_ACK);
            break;

        case 0xFF:                              /* reset keyboard */
            mikasa_kbc_queue (MIKASA_KBD_ACK);
            mikasa_kbc_queue (MIKASA_KBD_BAT_OK);
            break;

        default:
            mikasa_kbc_queue (MIKASA_KBD_ACK);
            break;
            }
    }
sim_debug (MIKASA_DBG_IO, &mikasa_dev, "KBC data write %02X\n", val);
return;
}

static void mikasa_kbc_write (uint32 port, uint8 val)
{
if (port == MIKASA_ISA_KBD_DATA) {
    mikasa_kbc_write_data (val);
    return;
    }
mikasa_kbc_pending_cmd = 0;
switch (val) {
    case MIKASA_KBC_CMD_READ_BYTE:
        mikasa_kbc_queue (mikasa_kbc_cmd_byte);
        break;

    case MIKASA_KBC_CMD_WRITE_BYTE:
    case MIKASA_KBC_CMD_WRITE_OUT:
    case MIKASA_KBC_CMD_WRITE_AUX:
        mikasa_kbc_pending_cmd = val;
        break;

    case MIKASA_KBC_CMD_SELF_TEST:
        mikasa_kbc_queue (0x55);
        break;

    case MIKASA_KBC_CMD_TEST_KBD:
        mikasa_kbc_queue (0);
        break;

    case MIKASA_KBC_CMD_TEST_AUX:
        mikasa_kbc_queue (0);
        break;

    case MIKASA_KBC_CMD_READ_OUT:
        mikasa_kbc_queue (mikasa_kbc_out_port);
        break;

    case MIKASA_KBC_CMD_DISABLE_KBD:
        mikasa_kbc_cmd_byte |= MIKASA_KBC_CCB_KBD_DISABLE;
        mikasa_kbc_update_irq ();
        break;

    case MIKASA_KBC_CMD_ENABLE_KBD:
        mikasa_kbc_cmd_byte &= ~MIKASA_KBC_CCB_KBD_DISABLE;
        mikasa_kbc_update_irq ();
        break;

    case MIKASA_KBC_CMD_DISABLE_AUX:
        mikasa_kbc_cmd_byte |= MIKASA_KBC_CCB_AUX_DISABLE;
        mikasa_kbc_update_irq ();
        break;

    case MIKASA_KBC_CMD_ENABLE_AUX:
        mikasa_kbc_cmd_byte &= ~MIKASA_KBC_CCB_AUX_DISABLE;
        mikasa_kbc_update_irq ();
        break;

    default:
        break;
        }
sim_debug (MIKASA_DBG_IO, &mikasa_dev, "KBC command write %02X\n", val);
return;
}

static t_bool mikasa_vga_io_port (uint32 port)
{
return ((port >= 0x3B4) && (port <= 0x3B5)) ||
    (port == 0x3BA) ||
    ((port >= 0x3C0) && (port <= 0x3CF)) ||
    ((port >= 0x3D4) && (port <= 0x3D5)) ||
    (port == 0x3DA) ||
    (port == 0x500);
}

static uint8 mikasa_vga_io_read (uint32 port)
{
uint8 val = 0xFF;

switch (port) {
    case 0x3B4:
    case 0x3D4:
        val = mikasa_vga_crtc_index;
        break;

    case 0x3B5:
    case 0x3D5:
        val = mikasa_vga_reg[mikasa_vga_crtc_index &
            (sizeof (mikasa_vga_reg) - 1)];
        break;

    case 0x3C4:
        val = mikasa_vga_seq_index;
        break;

    case 0x3C5:
        val = mikasa_vga_reg[mikasa_vga_seq_index &
            (sizeof (mikasa_vga_reg) - 1)];
        break;

    case 0x3CE:
        val = mikasa_vga_gfx_index;
        break;

    case 0x3CF:
        val = mikasa_vga_reg[mikasa_vga_gfx_index &
            (sizeof (mikasa_vga_reg) - 1)];
        break;

    case 0x3C1:
        val = mikasa_vga_reg[mikasa_vga_attr_index &
            (sizeof (mikasa_vga_reg) - 1)];
        break;

    case 0x3BA:
    case 0x3DA:
        mikasa_vga_attr_addr = TRUE;
        val = 0x00;
        break;

    default:
        val = mikasa_vga_reg[port & (sizeof (mikasa_vga_reg) - 1)];
        break;
        }
sim_debug (MIKASA_DBG_IO, &mikasa_dev, "VGA read %03X=%02X\n", port, val);
return val;
}

static void mikasa_vga_io_write (uint32 port, uint8 val)
{
switch (port) {
    case 0x3B4:
    case 0x3D4:
        mikasa_vga_crtc_index = val;
        break;

    case 0x3B5:
    case 0x3D5:
        mikasa_vga_reg[mikasa_vga_crtc_index &
            (sizeof (mikasa_vga_reg) - 1)] = val;
        break;

    case 0x3C0:
        if (mikasa_vga_attr_addr)
            mikasa_vga_attr_index = val & 0x1F;
        else
            mikasa_vga_reg[mikasa_vga_attr_index &
                (sizeof (mikasa_vga_reg) - 1)] = val;
        mikasa_vga_attr_addr = !mikasa_vga_attr_addr;
        break;

    case 0x3C4:
        mikasa_vga_seq_index = val;
        break;

    case 0x3C5:
        mikasa_vga_reg[mikasa_vga_seq_index &
            (sizeof (mikasa_vga_reg) - 1)] = val;
        break;

    case 0x3CE:
        mikasa_vga_gfx_index = val;
        break;

    case 0x3CF:
        mikasa_vga_reg[mikasa_vga_gfx_index &
            (sizeof (mikasa_vga_reg) - 1)] = val;
        break;

    default:
        mikasa_vga_reg[port & (sizeof (mikasa_vga_reg) - 1)] = val;
        break;
        }
sim_debug (MIKASA_DBG_IO, &mikasa_dev, "VGA write %03X=%02X\n", port, val);
return;
}

static uint8 mikasa_isa_read (uint32 port)
{
uint32 irr = (~mikasa_irq_summary) & M16;
uint32 imr = mikasa_irq_mask | MIKASA_ISA_ICU_PRESENT;
uint8 val;

if (mikasa_ncr_io_read (port, &val))
    return val;
if (mikasa_tulip_io_read (port, &val))
    return val;
if (mikasa_vga_enabled && mikasa_vga_io_port (port))
    return mikasa_vga_io_read (port);
if ((port >= MIKASA_ISA_DMA1) && (port < (MIKASA_ISA_DMA1 + 0x10)))
    return mikasa_dma1_reg[port - MIKASA_ISA_DMA1];
if ((port == MIKASA_ISA_PIC1) || (port == (MIKASA_ISA_PIC1 + 1)) ||
    (port == MIKASA_ISA_PIC2) || (port == (MIKASA_ISA_PIC2 + 1)))
    return mikasa_pic_read (port);
if (port == MIKASA_ISA_CFG_INDEX)
    return mikasa_cfg_index;
if (port == MIKASA_ISA_CFG_DATA)
    return mikasa_cfg_reg[mikasa_cfg_index];
if ((port >= MIKASA_ISA_PIT) && (port < (MIKASA_ISA_PIT + 4)))
    return mikasa_pit_read (port);
if ((port == MIKASA_ISA_KBD_DATA) || (port == MIKASA_ISA_KBD_CMD))
    return mikasa_kbc_read (port);
if (port == MIKASA_ISA_PORTB)
    return mikasa_portb_read ();
if (port == MIKASA_ISA_RTC_INDEX)
    return mikasa_rtc_index;
if (port == MIKASA_ISA_RTC_DATA)
    return mikasa_rtc_read_data ();
if ((port >= MIKASA_ISA_DMA_PAGE) && (port < (MIKASA_ISA_DMA_PAGE + 0x10)))
    return mikasa_dma_page_reg[port - MIKASA_ISA_DMA_PAGE];
if ((port >= MIKASA_ISA_DMA2) && (port < (MIKASA_ISA_DMA2 + 0x20)))
    return mikasa_dma2_reg[port - MIKASA_ISA_DMA2];
if ((port >= MIKASA_ISA_COM2) && (port < (MIKASA_ISA_COM2 + 8)))
    return mikasa_com2_read (port);
if (port == MIKASA_ISA_SIO_INDEX)
    return mikasa_sio_index;
if (port == MIKASA_ISA_SIO_DATA)
    return mikasa_sio_reg[mikasa_sio_index];
if ((port >= MIKASA_ISA_FDC) && (port < (MIKASA_ISA_FDC + 8))) {
    uint32 reg = port - MIKASA_ISA_FDC;

    if (reg == 4)
        return mikasa_fdc_reg[reg] | MIKASA_FDC_MSR_RQM;
    return mikasa_fdc_reg[reg];
    }
if ((port >= MIKASA_ISA_COM1) && (port < (MIKASA_ISA_COM1 + 8)))
    return mikasa_uart_read (port);
if (port == MIKASA_ISA_NMI_CTRL)
    return mikasa_nmi_ctrl;
if (port == MIKASA_ISA_ELCR1)
    return mikasa_elcr[0] & MIKASA_ELCR1_WRITABLE;
if (port == MIKASA_ISA_ELCR2)
    return mikasa_elcr[1] & MIKASA_ELCR2_WRITABLE;
if ((port >= MIKASA_ISA_OCP) && (port < (MIKASA_ISA_OCP + 4))) {
    return mikasa_ocp_read (port - MIKASA_ISA_OCP);
    }
if ((port == MIKASA_ISA_ICU_IRR) || (port == (MIKASA_ISA_ICU_IRR + 1)))
    return (uint8) (irr >> ((port - MIKASA_ISA_ICU_IRR) << 3));
if ((port == MIKASA_ISA_ICU_IMR) || (port == (MIKASA_ISA_ICU_IMR + 1)))
    return (uint8) (imr >> ((port - MIKASA_ISA_ICU_IMR) << 3));
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
if (mikasa_tulip_io_write (port, val))
    return;
if (mikasa_vga_enabled && mikasa_vga_io_port (port)) {
    mikasa_vga_io_write (port, val);
    return;
    }
if ((port >= MIKASA_ISA_DMA1) && (port < (MIKASA_ISA_DMA1 + 0x10))) {
    mikasa_dma1_reg[port - MIKASA_ISA_DMA1] = val;
    return;
    }
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
if ((port == MIKASA_ISA_KBD_DATA) || (port == MIKASA_ISA_KBD_CMD)) {
    mikasa_kbc_write (port, val);
    return;
    }
if (port == MIKASA_ISA_PORTB) {
    mikasa_portb_write (val);
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
if ((port >= MIKASA_ISA_DMA_PAGE) && (port < (MIKASA_ISA_DMA_PAGE + 0x10))) {
    mikasa_dma_page_reg[port - MIKASA_ISA_DMA_PAGE] = val;
    return;
    }
if ((port >= MIKASA_ISA_DMA2) && (port < (MIKASA_ISA_DMA2 + 0x20))) {
    mikasa_dma2_reg[port - MIKASA_ISA_DMA2] = val;
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
if ((port >= MIKASA_ISA_FDC) && (port < (MIKASA_ISA_FDC + 8))) {
    mikasa_fdc_reg[port - MIKASA_ISA_FDC] = val;
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
if (port == MIKASA_ISA_ELCR1) {
    mikasa_pic_write_elcr (0, val);
    return;
    }
if (port == MIKASA_ISA_ELCR2) {
    mikasa_pic_write_elcr (1, val);
    return;
    }
if ((port >= MIKASA_ISA_OCP) && (port < (MIKASA_ISA_OCP + 4))) {
    mikasa_ocp_write (port - MIKASA_ISA_OCP, val);
    return;
    }
if (port == MIKASA_ISA_ICU_IMR) {
    mikasa_irq_mask = (mikasa_irq_mask & 0xFF00) | val;
    mikasa_irq_mask &= ~MIKASA_ISA_ICU_PRESENT;
    mikasa_irq_update ();
    sim_debug (MIKASA_DBG_IO, &mikasa_dev, "ICU write %03X=%02X mask=%04X\n",
        port, val, mikasa_irq_mask);
    }
else if (port == (MIKASA_ISA_ICU_IMR + 1)) {
    mikasa_irq_mask = (mikasa_irq_mask & 0x00FF) | (((uint32) val) << 8);
    mikasa_irq_mask &= ~MIKASA_ISA_ICU_PRESENT;
    mikasa_irq_update ();
    sim_debug (MIKASA_DBG_IO, &mikasa_dev, "ICU write %03X=%02X mask=%04X\n",
        port, val, mikasa_irq_mask);
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
uint32 addr = mikasa_apecs_sparse_mem_addr (off);
uint32 mode = (uint32) ((off >> 3) & 3);
uint32 reg;
uint32 val;

if (!mikasa_vga_enabled)
    goto no_vga;
if ((addr >= MIKASA_VGA_LEGACY_BASE) &&
    (addr < (MIKASA_VGA_LEGACY_BASE + MIKASA_VGA_LEGACY_SIZE))) {
    sim_debug (MIKASA_DBG_IO, &mikasa_dev,
        "VGA legacy memory read %08X\n", addr);
    return M32;
    }
if ((addr >= MIKASA_VGA_ROM_BASE) &&
    (addr < (MIKASA_VGA_ROM_BASE + MIKASA_VGA_ROM_SIZE))) {
    sim_debug (MIKASA_DBG_IO, &mikasa_dev,
        "VGA legacy ROM read %08X\n", addr);
    return M32;
    }
if (mikasa_vga_rom_reg (addr, &reg)) {
    sim_debug (MIKASA_DBG_IO, &mikasa_dev,
        "VGA option ROM read %08X\n", addr);
    return M32;
    }
if (mikasa_vga_bar_reg (addr, 0x10, MIKASA_VGA_FB_MASK, &reg)) {
    sim_debug (MIKASA_DBG_IO, &mikasa_dev,
        "VGA framebuffer read %08X\n", addr);
    return M32;
    }
no_vga:
if (mikasa_ncr_bar_reg (addr, 0x14, 0xFFFFFFFFu, &reg)) {
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
if (mikasa_tulip_bar_reg (addr, 0x14, 0xFFFFFFFFu, &reg)) {
    if (mode == 3)
        return mikasa_tulip_read_l (reg & ~3u);
    if (mode == 1) {
        val = (uint32) mikasa_tulip_read_len (reg & ~1u, L_WORD);
        return ((t_uint64) val) << (8 * (addr & 2));
        }
    val = (uint32) mikasa_tulip_read_b (reg);
    if (lnt == L_BYTE)
        return val;
    return ((t_uint64) val) << (8 * (addr & 3));
    }
sim_debug (MIKASA_DBG_IO, &mikasa_dev,
    "absent APECS sparse memory read %llX\n",
    (unsigned long long) pa);
mikasa_epic_latch_pci_error (MIKASA_EPIC_DCSR_NDEV, pa);
return M32;
}

static void mikasa_pci_sparse_mem_write (t_uint64 pa, t_uint64 val,
    uint32 lnt)
{
t_uint64 off = pa - MIKASA_APECS_PCI_SPARSE;
uint32 addr = mikasa_apecs_sparse_mem_addr (off);
uint32 mode = (uint32) ((off >> 3) & 3);
uint32 reg;
uint32 data;

if (!mikasa_vga_enabled)
    goto no_vga;
if ((addr >= MIKASA_VGA_LEGACY_BASE) &&
    (addr < (MIKASA_VGA_LEGACY_BASE + MIKASA_VGA_LEGACY_SIZE))) {
    sim_debug (MIKASA_DBG_IO, &mikasa_dev,
        "VGA legacy memory write %08X=%llX\n",
        addr, (unsigned long long) val);
    return;
    }
if ((addr >= MIKASA_VGA_ROM_BASE) &&
    (addr < (MIKASA_VGA_ROM_BASE + MIKASA_VGA_ROM_SIZE))) {
    sim_debug (MIKASA_DBG_IO, &mikasa_dev,
        "VGA legacy ROM write %08X=%llX\n",
        addr, (unsigned long long) val);
    return;
    }
if (mikasa_vga_rom_reg (addr, &reg)) {
    sim_debug (MIKASA_DBG_IO, &mikasa_dev,
        "VGA option ROM write %08X=%llX\n",
        addr, (unsigned long long) val);
    return;
    }
if (mikasa_vga_bar_reg (addr, 0x10, MIKASA_VGA_FB_MASK, &reg)) {
    sim_debug (MIKASA_DBG_IO, &mikasa_dev,
        "VGA framebuffer write %08X=%llX\n",
        addr, (unsigned long long) val);
    return;
    }
no_vga:
if (mikasa_ncr_bar_reg (addr, 0x14, 0xFFFFFFFFu, &reg)) {
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
if (mikasa_tulip_bar_reg (addr, 0x14, 0xFFFFFFFFu, &reg)) {
    if (mode == 3)
        mikasa_tulip_write_l (reg & ~3u, (uint32) val);
    else if (mode == 1) {
        data = (uint32) (val >> (8 * (addr & 2)));
        mikasa_tulip_write_len (reg & ~1u, data, L_WORD);
        }
    else if (lnt == L_BYTE)
        mikasa_tulip_write_len (reg, val, L_BYTE);
    else {
        data = (uint32) (val >> (8 * (addr & 3)));
        mikasa_tulip_write_len (reg, data, L_BYTE);
        }
    return;
    }
sim_debug (MIKASA_DBG_IO, &mikasa_dev,
    "unhandled APECS sparse memory write %llX=%llX\n",
    (unsigned long long) pa, (unsigned long long) val);
mikasa_epic_latch_pci_error (MIKASA_EPIC_DCSR_NDEV, pa);
return;
}

static t_uint64 mikasa_pci_dense_mem_read (t_uint64 pa, uint32 lnt)
{
uint32 addr = (uint32) (pa - MIKASA_APECS_PCI_DENSE);
uint32 reg;

if (mikasa_ncr_bar_reg (addr, 0x14, 0xFFFFFFFFu, &reg))
    return mikasa_ncr_read_len (reg, lnt);
if (mikasa_tulip_bar_reg (addr, 0x14, 0xFFFFFFFFu, &reg))
    return mikasa_tulip_read_len (reg, lnt);
sim_debug (MIKASA_DBG_IO, &mikasa_dev,
    "absent APECS dense memory read %llX\n",
    (unsigned long long) pa);
mikasa_epic_latch_pci_error (MIKASA_EPIC_DCSR_NDEV, pa);
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
if (mikasa_tulip_bar_reg (addr, 0x14, 0xFFFFFFFFu, &reg)) {
    mikasa_tulip_write_len (reg, val, lnt);
    return;
    }
sim_debug (MIKASA_DBG_IO, &mikasa_dev,
    "unhandled APECS dense memory write %llX=%llX\n",
    (unsigned long long) pa, (unsigned long long) val);
mikasa_epic_latch_pci_error (MIKASA_EPIC_DCSR_NDEV, pa);
return;
}

static t_bool mikasa_io_rd (t_uint64 pa, t_uint64 *val, uint32 lnt)
{
if ((pa >= MIKASA_COMANCHE_BASE) &&
    (pa < (MIKASA_COMANCHE_BASE + MIKASA_COMANCHE_SIZE)))
    *val = mikasa_comanche_read (pa, lnt);
else if ((pa >= MIKASA_EPIC_BASE) &&
    (pa < (MIKASA_EPIC_BASE + MIKASA_EPIC_SIZE)))
    *val = mikasa_epic_read (pa, lnt);
else if (pa == MIKASA_APECS_PCI_IACK)
    *val = mikasa_pic_iack ();
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
if ((pa >= MIKASA_COMANCHE_BASE) &&
    (pa < (MIKASA_COMANCHE_BASE + MIKASA_COMANCHE_SIZE)))
    mikasa_comanche_write (pa, val, lnt);
else if ((pa >= MIKASA_EPIC_BASE) &&
    (pa < (MIKASA_EPIC_BASE + MIKASA_EPIC_SIZE)))
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

static void mikasa_set_cpu_info (void)
{
if (mikasa_pal_valid_phys (MIKASA_CPU_INFO_QW_PA, sizeof (t_uint64))) {
    WritePQ (MIKASA_CPU_INFO_QW_PA,
        (((t_uint64) MIKASA_CPU_TYPE) << 32) | MIKASA_CPU_VERSION);
    }
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
pal_type = PAL_UNIX;
mikasa_srm_bootstrap_pal = FALSE;
mikasa_srm_os_handoff = FALSE;
lock_flag = 0;
mikasa_set_cpu_info ();
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
mikasa_direct_apb = FALSE;
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

static t_uint64 mikasa_find_hwrpb_pa (void)
{
t_uint64 pa;
t_uint64 limit = MEMSIZE;

if (limit > 0x01000000)
    limit = 0x01000000;
for (pa = 0; (pa + HWRPB_CHECKSUM_OFF) < limit; pa = pa + MIKASA_PAGE_SIZE) {
    if ((ReadPQ (pa) == pa) && (ReadPQ (pa + 8) == HWRPB_ID))
        return pa;
    }
return MIKASA_HWRPB_PA;
}

static t_uint64 mikasa_hwrpb_va_for_pa (t_uint64 pa)
{
if ((pa >= mikasa_hwrpb_pa) &&
    (pa < (mikasa_hwrpb_pa + MIKASA_HWRPB_SIZE)))
    return MIKASA_HWRPB_VA + (pa - mikasa_hwrpb_pa);
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

l1pte = ReadPQ (mikasa_boot_l1_pt_pa + VPN_GETLVL1 (vpn));
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
t_uint64 pteva = va - mikasa_boot_vptb_va;
uint32 pte_vpn;
t_uint64 l2_ptea;
t_uint64 l1pte, l2pte;
t_uint64 l2_pa, l3_pa;

if (pteva >= (((t_uint64) 1) << (VA_N_VPN + 3)))
    return FALSE;
pte_vpn = (uint32) (pteva >> 3);
l1pte = ReadPQ (mikasa_boot_l1_pt_pa + VPN_GETLVL1 (pte_vpn));
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
    pa = mikasa_hwrpb_pa + (va - MIKASA_HWRPB_VA);
    *pte = mikasa_pte (pa, PTE_KRE | PTE_KWE);
    return TRUE;
    }
if ((va >= MIKASA_BOOT_IMAGE_VA) &&
    (va < (MIKASA_BOOT_IMAGE_VA + MIKASA_BOOT_RESERVED_SIZE))) {
    pa = mikasa_boot_image_pa + (va - MIKASA_BOOT_IMAGE_VA);
    *pte = mikasa_pte (pa, PTE_KRE | PTE_KWE);
    return TRUE;
    }
if ((va >= mikasa_boot_vptb_va) &&
    ((va - mikasa_boot_vptb_va) < MIKASA_PT_SPACE_VA_SIZE) &&
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
    *pa = mikasa_hwrpb_pa + (va - MIKASA_HWRPB_VA);
    return TRUE;
    }
if ((va >= MIKASA_BOOT_IMAGE_VA) &&
    (va < (MIKASA_BOOT_IMAGE_VA + MIKASA_BOOT_RESERVED_SIZE))) {
    *pa = mikasa_boot_image_pa + (va - MIKASA_BOOT_IMAGE_VA);
    return TRUE;
    }
if ((va >= mikasa_boot_vptb_va) &&
    ((va - mikasa_boot_vptb_va) < MIKASA_PT_SPACE_VA_SIZE) &&
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

mikasa_boot_l1_pt_pa = MIKASA_L1_PT_PA;
mikasa_boot_vptb_va = MIKASA_PT_SPACE_VA;
mikasa_boot_image_pa = MIKASA_BOOT_IMAGE_PA;
mikasa_zero_phys (MIKASA_PT_SPACE_PA, MIKASA_PT_SPACE_SIZE);
mikasa_next_l3_pt_pa = MIKASA_L3_BOOTPTS_PA;
WritePQ (MIKASA_L1_PT_PA, mikasa_pte (MIKASA_L2_PT_PA, PTE_KRE | PTE_KWE));
mikasa_set_l2_entry (l2_region0, MIKASA_L3_REGION0_PA);
mikasa_set_l2_entry (l2_region1, MIKASA_L3_REGION1_PA);
mikasa_set_l2_entry (l2_ptspace, MIKASA_L2_PT_PA);
mikasa_map_range (MIKASA_L3_REGION0_PA, MIKASA_HWRPB_VA, mikasa_hwrpb_pa,
    MIKASA_HWRPB_SIZE);
mikasa_map_range (MIKASA_L3_REGION1_PA, MIKASA_BOOT_IMAGE_VA,
    mikasa_boot_image_pa, MIKASA_BOOT_RESERVED_SIZE);
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
mikasa_load_tlb_range (MIKASA_HWRPB_VA, mikasa_hwrpb_pa, MIKASA_TLB_SPAN,
    PTE_KRE | PTE_KWE);
mikasa_load_tlb_range (mikasa_hwrpb_pa, mikasa_hwrpb_pa, MIKASA_TLB_SPAN,
    PTE_KRE | PTE_KWE);
mikasa_load_tlb_range (MIKASA_BOOT_IMAGE_VA, mikasa_boot_image_pa,
    MIKASA_BOOT_RESERVED_SIZE, PTE_KRE | PTE_KWE);
return;
}

static void mikasa_write_console_crb (t_uint64 hwrpb_pa)
{
t_uint64 crb_pa = hwrpb_pa + MIKASA_CRB_OFF;
t_uint64 va_pd_pa = hwrpb_pa + MIKASA_CONSOLE_VA_PD_OFF;
t_uint64 pa_pd_pa = hwrpb_pa + MIKASA_CONSOLE_PA_PD_OFF;
t_uint64 code_pa = hwrpb_pa + MIKASA_CONSOLE_CODE_OFF;
t_uint64 hwrpb_va = MIKASA_HWRPB_VA;
t_uint64 va_pd_va = hwrpb_va + MIKASA_CONSOLE_VA_PD_OFF;
t_uint64 code_va = hwrpb_va + MIKASA_CONSOLE_CODE_OFF;
t_uint64 hwrpb_pages = (MIKASA_HWRPB_SIZE + MIKASA_PAGE_SIZE - 1) /
    MIKASA_PAGE_SIZE;

WritePQ (crb_pa + 0x000, va_pd_va);                     /* VA dispatch PD */
WritePQ (crb_pa + 0x008, pa_pd_pa);                    /* PA dispatch PD */
WritePQ (crb_pa + 0x010, 0);                           /* VA fixup PD */
WritePQ (crb_pa + 0x018, 0);                           /* PA fixup PD */
WritePQ (crb_pa + 0x020, 1);                           /* VA/PA maps */
WritePQ (crb_pa + 0x028, hwrpb_pages);
WritePQ (crb_pa + 0x030, hwrpb_va);                    /* console VA */
WritePQ (crb_pa + 0x038, hwrpb_pa);                    /* console PA */
WritePQ (crb_pa + 0x040, hwrpb_pages);

WritePQ (va_pd_pa + 0x000, PDSC_FLAGS_NULL_NATIVE);    /* null-frame PD */
WritePQ (va_pd_pa + 0x008, code_va);
WritePQ (va_pd_pa + 0x010, 0);

WritePQ (pa_pd_pa + 0x000, PDSC_FLAGS_NULL_NATIVE);    /* null-frame PD */
WritePQ (pa_pd_pa + 0x008, code_pa);
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

static t_bool mikasa_pci_dma_sg_pa (uint32 window, uint32 addr, uint32 base,
    t_uint64 *pa)
{
uint32 tbase_off = (window == 0) ? MIKASA_EPIC_TBASE_1 :
    MIKASA_EPIC_TBASE_2;
uint32 page = addr & MIKASA_EPIC_TLB_TAG_PAGE;
uint32 page_off = addr & (MIKASA_PAGE_SIZE - 1);
uint32 index = (addr - base) >> VA_N_OFF;
uint32 i;
t_uint64 tbase;
t_uint64 entry_pa;
t_uint64 entry;

/* APECS SGMAP and TLB data entries store Alpha PFN << 1. */
for (i = 0; i < MIKASA_EPIC_TLB_ENTRIES; i++) {
    uint32 tag = mikasa_epic_reg[(MIKASA_EPIC_TLB_TAG_0 >> 5) + i];

    if (((tag & MIKASA_EPIC_TLB_TAG_EVAL) != 0) &&
        ((tag & MIKASA_EPIC_TLB_TAG_PAGE) == page)) {
        uint32 data = mikasa_epic_reg[(MIKASA_EPIC_TLB_DATA_0 >> 5) + i];

        *pa = (((t_uint64) (data & MIKASA_EPIC_TLB_DATA_PAGE)) << 12) |
            page_off;
        return ADDR_IS_MEM (*pa);
        }
    }
tbase = ((t_uint64) (mikasa_epic_reg[tbase_off >> 5] &
    MIKASA_EPIC_TBASE_MASK)) << MIKASA_EPIC_TBASE_SHIFT;
entry_pa = tbase + (((t_uint64) index) << 3);
if (!ADDR_IS_MEM (entry_pa + 7))
    return FALSE;
entry = mikasa_read_phys_quad (entry_pa);
if ((entry & MIKASA_EPIC_SGMAP_EVAL) == 0)
    return FALSE;
*pa = ((entry & MIKASA_EPIC_SGMAP_PFN) << 12) | page_off;
if (!ADDR_IS_MEM (*pa))
    return FALSE;
i = index & (MIKASA_EPIC_TLB_ENTRIES - 1);
mikasa_epic_reg[(MIKASA_EPIC_TLB_TAG_0 >> 5) + i] =
    (page & MIKASA_EPIC_TLB_TAG_PAGE) | MIKASA_EPIC_TLB_TAG_EVAL;
mikasa_epic_reg[(MIKASA_EPIC_TLB_DATA_0 >> 5) + i] =
    (uint32) (entry & MIKASA_EPIC_TLB_DATA_PAGE);
return TRUE;
}

static t_bool mikasa_pci_dma_window_pa (uint32 window, uint32 addr,
    t_uint64 *pa, t_bool *matched)
{
uint32 base_off = (window == 0) ? MIKASA_EPIC_PCI_BASE_1 :
    MIKASA_EPIC_PCI_BASE_2;
uint32 mask_off = (window == 0) ? MIKASA_EPIC_PCI_MASK_1 :
    MIKASA_EPIC_PCI_MASK_2;
uint32 tbase_off = (window == 0) ? MIKASA_EPIC_TBASE_1 :
    MIKASA_EPIC_TBASE_2;
uint32 base_reg = mikasa_epic_reg[base_off >> 5];
uint32 base;
uint32 mask;
t_uint64 size;
t_uint64 target_base;

if ((base_reg & MIKASA_EPIC_PCI_BASE_WENB) == 0)
    return FALSE;
base = base_reg & MIKASA_EPIC_PCI_BASE_MASK;
mask = mikasa_epic_reg[mask_off >> 5] & MIKASA_EPIC_PCI_MASK_MASK;
size = ((t_uint64) mask) + 0x00100000ULL;
if (((t_uint64) addr < base) || ((t_uint64) addr >= (((t_uint64) base) + size)))
    return FALSE;
*matched = TRUE;
if (base_reg & MIKASA_EPIC_PCI_BASE_SGEN)
    return mikasa_pci_dma_sg_pa (window, addr, base, pa);
target_base = ((t_uint64) (mikasa_epic_reg[tbase_off >> 5] &
    MIKASA_EPIC_TBASE_MASK)) << MIKASA_EPIC_TBASE_SHIFT;
*pa = target_base + ((t_uint64) addr - base);
return ADDR_IS_MEM (*pa);
}

static t_bool mikasa_pci_dma_addr_to_pa (uint32 addr, t_uint64 *pa)
{
t_bool matched = FALSE;

if (mikasa_pci_dma_window_pa (0, addr, pa, &matched))
    return TRUE;
if (matched)
    return FALSE;
if (mikasa_pci_dma_window_pa (1, addr, pa, &matched))
    return TRUE;
if (matched)
    return FALSE;
if (ADDR_IS_MEM (addr)) {
    *pa = addr;
    return TRUE;
    }
return FALSE;
}

static t_bool mikasa_pci_dma_read_byte (uint32 addr, uint8 *val)
{
t_uint64 pa;

if (!mikasa_pci_dma_addr_to_pa (addr, &pa))
    return FALSE;
*val = mikasa_read_phys_byte (pa);
return TRUE;
}

static t_bool mikasa_pci_dma_write_byte (uint32 addr, uint8 val)
{
t_uint64 pa;

if (!mikasa_pci_dma_addr_to_pa (addr, &pa))
    return FALSE;
mikasa_write_phys_byte (pa, val);
return TRUE;
}

static t_bool mikasa_pci_dma_read_long (uint32 addr, uint32 *val)
{
t_uint64 pa;

if (!mikasa_pci_dma_addr_to_pa (addr, &pa) || (pa & 3))
    return FALSE;
*val = mikasa_read_phys_long (pa);
return TRUE;
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
    *pa = mikasa_boot_image_pa + (addr - MIKASA_BOOT_IMAGE_VA);
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
if (mikasa_apb_trace_count < 512) {
    sim_printf ("MIKASA APB DESC addr=%llX desc_pa=%llX cpu=%llX len=%llX "
        "bus=%llX blen=%llX\n", (unsigned long long) addr,
        (unsigned long long) desc_pa, (unsigned long long) cpu_addr,
        (unsigned long long) cpu_len, (unsigned long long) bus_addr,
        (unsigned long long) bus_len);
    mikasa_apb_trace_count++;
    }
if (mikasa_apb_dma_window_pa (addr, cpu_addr, cpu_len, bus_addr,
    bus_len, pa)) {
    mikasa_apb_dma_valid = TRUE;
    mikasa_apb_dma_cpu_addr = cpu_addr;
    mikasa_apb_dma_cpu_len = cpu_len;
    mikasa_apb_dma_bus_addr = bus_addr;
    mikasa_apb_dma_bus_len = bus_len;
    if (mikasa_apb_trace_count < 512) {
        sim_printf ("MIKASA APB DESC live-hit addr=%llX -> pa=%llX\n",
            (unsigned long long) addr, (unsigned long long) *pa);
        mikasa_apb_trace_count++;
        }
    return TRUE;
    }
if (mikasa_apb_dma_valid && mikasa_apb_dma_window_pa (addr,
    mikasa_apb_dma_cpu_addr, mikasa_apb_dma_cpu_len,
    mikasa_apb_dma_bus_addr, mikasa_apb_dma_bus_len, pa)) {
    if (mikasa_apb_trace_count < 512) {
        sim_printf ("MIKASA APB DESC cache-hit addr=%llX -> pa=%llX\n",
            (unsigned long long) addr, (unsigned long long) *pa);
        mikasa_apb_trace_count++;
        }
    return TRUE;
    }
if (mikasa_apb_trace_count < 512) {
    sim_printf ("MIKASA APB DESC miss addr=%llX\n",
        (unsigned long long) addr);
    mikasa_apb_trace_count++;
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

static uint32 mikasa_read_phys_long (t_uint64 pa)
{
if (pa & 4)
    return (uint32) ((M[pa >> 3] >> 32) & M32);
return (uint32) (M[pa >> 3] & M32);
}

static t_uint64 mikasa_read_phys_quad (t_uint64 pa)
{
return M[pa >> 3];
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

if (unit >= MIKASA_DKA_UNITS) {
    if (mikasa_apb_trace_count < 256) {
        sim_printf ("MIKASA APB IOBOX READ bad unit=%u\n", unit);
        mikasa_apb_trace_count++;
        }
    return FALSE;
    }
if (!mikasa_apb_io_target_pa (addr, &pa)) {
    if (mikasa_apb_trace_count < 256) {
        sim_printf ("MIKASA APB IOBOX READ map fail addr=%llX\n",
            (unsigned long long) addr);
        mikasa_apb_trace_count++;
        }
    return FALSE;
    }
if (!ADDR_IS_MEM (pa + MIKASA_DKA_BLOCK_SIZE - 1))
    return FALSE;
mikasa_apb_last_pa = pa;
if (mikasa_apb_trace_count < 256) {
    sim_printf ("MIKASA APB IOBOX READ lbn=%llu addr=%llX -> pa=%llX\n",
        (unsigned long long) lbn, (unsigned long long) addr,
        (unsigned long long) pa);
    mikasa_apb_trace_count++;
    }
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
if (mikasa_apb_trace_count < 256) {
    uint32 f110 = ReadPL (base_pa + 0x110);
    uint32 f114 = ReadPL (base_pa + 0x114);
    uint32 f118 = ReadPL (base_pa + 0x118);
    uint32 f11c = ReadPL (base_pa + 0x11C);
    uint32 f124 = ReadPL (base_pa + 0x124);
    uint32 f12c = ReadPL (base_pa + 0x12C);
    uint32 f134 = ReadPL (base_pa + 0x134);

    sim_printf ("MIKASA APB IOBOX CMD=%llu lbn=%llu addr=%llX\n",
        (unsigned long long) cmd, (unsigned long long) lbn,
        (unsigned long long) addr);
    sim_printf ("MIKASA APB IOBOX F110=%08X F114=%08X F118=%08X F11C=%08X "
        "F124=%08X F12C=%08X F134=%08X\n", f110, f114, f118, f11c, f124,
        f12c, f134);
    mikasa_apb_trace_count++;
    }
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
if ((pa < mikasa_boot_image_pa) ||
    (pa >= (mikasa_boot_image_pa + MIKASA_BOOT_RESERVED_SIZE)))
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
if (mikasa_apb_trace_count < 256) {
    sim_printf ("MIKASA APB OPEN dev=\"%s\" unit=%d\n", devstr, unit);
    mikasa_apb_trace_count++;
    }
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
t_uint64 start_pa = 0;
uint32 pa_mode = 0;
uint32 channel = (uint32) R[17];
uint32 unit;

mikasa_ioread_count++;
if (mikasa_apb_trace_count < 256) {
    sim_printf ("MIKASA APB READ ch=%u count=%llu lbn=%llu addr=%llX\n",
        channel, (unsigned long long) count, (unsigned long long) lbn,
        (unsigned long long) addr);
    mikasa_apb_trace_count++;
    }
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

if (mikasa_apb_io_target_pa (addr, &start_pa))
    pa_mode = 1;

if (count & (MIKASA_DKA_BLOCK_SIZE - 1)) {
    if (mikasa_apb_trace_count < 256) {
        sim_printf ("MIKASA APB READ unaligned count=%llu (round down)\n",
            (unsigned long long) count);
        mikasa_apb_trace_count++;
        }
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
    if (pa_mode) {
        for (i = 0; i < chunk; i++)
            mikasa_write_phys_byte (start_pa + done + i, buf[i]);
        }
    else {
        for (i = 0; i < chunk; i++)
            WriteB (addr + done + i, buf[i]);
        }
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
if (mikasa_apb_trace_count < 256) {
    sim_printf ("MIKASA APB WRITE ch=%u count=%llu lbn=%llu addr=%llX\n",
        channel, (unsigned long long) count, (unsigned long long) lbn,
        (unsigned long long) addr);
    mikasa_apb_trace_count++;
    }
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
    if (mikasa_apb_trace_count < 256) {
        sim_printf ("MIKASA APB WRITE unaligned count=%llu (round down)\n",
            (unsigned long long) count);
        mikasa_apb_trace_count++;
        }
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
if (mikasa_apb_trace_count < 1024) {
    sim_printf ("MIKASA CCB fnc=%u R17=%llX R18=%llX R19=%llX R20=%llX\n",
        fnc, (unsigned long long) R[17], (unsigned long long) R[18],
        (unsigned long long) R[19], (unsigned long long) R[20]);
    mikasa_apb_trace_count++;
    }
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
        if (mikasa_apb_trace_count < 512) {
            const char *ev = mikasa_get_env_value ((uint32) R[17]);
            sim_printf ("MIKASA GET_ENV key=%u -> \"%s\"\n", (uint32) R[17],
                ev ? ev : "(null)");
            mikasa_apb_trace_count++;
            }
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

static t_bool mikasa_pal_route_os_events (void)
{
return mikasa_direct_apb || mikasa_srm_os_handoff;
}

static t_stat mikasa_pal_interrupt_post (uint32 type, uint32 vec,
    t_uint64 par2, uint32 newipl)
{
t_stat r;

r = mikasa_pal_ent_dispatch (ev4_state.paltemp_raw[ptEntInt],
    MODE_K, newipl & MIKASA_OSF_PSV_M_IPL);
if (r == SCPE_OK) {
    R[16] = type;
    R[17] = vec;
    R[18] = par2;
    }
return r;
}

static uint32 mikasa_pal_excp_scb (uint32 abval)
{
switch (abval) {

    case EXC_RSVI:
        return MIKASA_SCB_RSVI;

    case EXC_RSVO:
        return MIKASA_SCB_RSVO;

    case EXC_ALIGN:
        return MIKASA_SCB_ALIGN;

    case EXC_FPDIS:
        return MIKASA_SCB_FDIS;

    case EXC_TBM + EXC_E:
    case EXC_BVA + EXC_E:
    case EXC_ACV + EXC_E:
        return MIKASA_SCB_ACV;

    case EXC_TBM + EXC_R:
    case EXC_TBM + EXC_W:
    case EXC_BVA + EXC_R:
    case EXC_BVA + EXC_W:
    case EXC_ACV + EXC_R:
    case EXC_ACV + EXC_W:
    case EXC_TNV + EXC_E:
    case EXC_TNV + EXC_R:
    case EXC_TNV + EXC_W:
        return MIKASA_SCB_TNV;

    case EXC_FOX + EXC_E:
        return MIKASA_SCB_FOE;

    case EXC_FOX + EXC_R:
        return MIKASA_SCB_FOR;

    case EXC_FOX + EXC_W:
        return MIKASA_SCB_FOW;

    default:
        return 0;
        }
}

static t_bool mikasa_pal_bootstrap_map (void)
{
return mikasa_direct_apb || !mikasa_srm_os_handoff;
}

t_bool mikasa_pal_uses_simh_ipl (void)
{
return mikasa_pal_route_os_events ();
}

t_stat mikasa_pal_proc_intr (uint32 lvl)
{
uint32 vec = 0;
uint32 i;
uint32 mode = mikasa_pal_current_mode ();

if (!mikasa_pal_route_os_events ())
    return SCPE_NOFNC;
if (lvl & IPL_HALT)
    return STOP_HALT;
if (ev5_crd && ev4_state.hier_cre &&
    (ev4_read_abox_ipr (0) & ABOX_M_CRD_EN)) {
    return mikasa_pal_crd_logout ();
    }
if (((mikasa_pal_live_hirr () & HIRR_M_PC0) && ev4_state.hier_pc0) ||
    ((mikasa_pal_live_hirr () & HIRR_M_PC1) && ev4_state.hier_pc1)) {
    t_uint64 hirr = mikasa_pal_live_hirr ();
    uint32 pc0_pending = ((hirr & HIRR_M_PC0) != 0) && ev4_state.hier_pc0;
    uint32 pc1 = !pc0_pending && ((hirr & HIRR_M_PC1) != 0) &&
        ev4_state.hier_pc1;

    if (pc1) {
        ev4_state.hirr_pc1 = 0;
        }
    else {
        ev4_state.hirr_pc0 = 0;
        }
    return mikasa_pal_interrupt_post (MIKASA_INT_K_PERF, MIKASA_SCB_PERF,
        pc1, MIKASA_IPL_K_PERF);
    }
lvl = lvl & INTID_MASK;
if ((lvl > IPL_HMAX) || ((lvl > IPL_SMAX) && (lvl < IPL_HMIN)))
    return SCPE_IERR;
if (lvl >= IPL_HMIN) {
    if ((mikasa_pal_live_hirr () & HIRR_M_HWR) == 0)
        return SCPE_OK;
    if (lvl == (IPL_HMIN + MIKASA_PIC_HWRE_LEVEL)) {
        lvl = MIKASA_IPL_K_DEV;
        vec = MIKASA_SCB_IO;
        }
    else if (lvl == (IPL_HMIN + MIKASA_ICU_HWRE_LEVEL)) {
        uint32 active = mikasa_irq_summary & mikasa_irq_mask &
            ev4_state.hier;
        static const uint32 mikasa_irq_order[] = { 4, 5, 2, 1, 0 };

        for (i = 0; i < (sizeof (mikasa_irq_order) / sizeof (mikasa_irq_order[0])); i++) {
            uint32 irq = mikasa_irq_order[i];
            if (active & (1u << irq)) {
                if (irq == 4) {
                    mikasa_rtc_index = MIKASA_RTC_INTR;
                    (void) mikasa_rtc_read_data ();
                    lvl = MIKASA_IPL_K_CLK;
                    vec = MIKASA_SCB_TIMER;
                    break;
                    }
                if ((irq == 1) || (irq == 2)) {
                    lvl = MIKASA_IPL_K_DEV;
                    vec = MIKASA_SCB_IO;
                    break;
                    }
                if (irq == 0) {
                    return mikasa_pal_mchk_logout (MIKASA_MCHK_K_DCSR);
                    }
                if (irq == 5) {
                    uint8 nmisc = mikasa_nmi_ctrl;

                    if ((nmisc & MIKASA_NMISC_M_SERR) &&
                        ((nmisc & MIKASA_NMISC_M_SERR_EN) == 0)) {
                        mikasa_nmi_ctrl =
                            (nmisc | MIKASA_NMISC_M_SERR_EN) & 0x0F;
                        return mikasa_pal_mchk_logout (
                            MIKASA_MCHK_K_SIO_SERR);
                        }
                    if ((nmisc & MIKASA_NMISC_M_IOCHK) &&
                        ((nmisc & MIKASA_NMISC_M_IOCHK_EN) == 0)) {
                        mikasa_nmi_ctrl =
                            (nmisc | MIKASA_NMISC_M_IOCHK_EN) & 0x0F;
                        return mikasa_pal_mchk_logout (
                            MIKASA_MCHK_K_SIO_IOCHK);
                        }
                    return SCPE_OK;
                    }
                vec = MIKASA_SCB_IO + ((irq + 0x10) << 4);
                break;
                }
            }
        if (vec == 0) {
            for (i = 16; i > 0; i--) {
                if (active & (1u << (i - 1))) {
                    vec = MIKASA_SCB_IO + ((i - 1 + 0x10) << 4);
                    break;
                    }
                }
            }
        }
    }
else if (lvl > 0) {
    if ((lvl == IPL_AST) &&
        (mikasa_pal_asten & mikasa_pal_astsr & mikasa_ast_map[mode])) {
        uint32 astm = mikasa_ast_pri[mikasa_pal_astsr & MIKASA_PAL_AST_MASK];

        mikasa_pal_astsr = mikasa_pal_astsr & ~(1u << astm);
        ev5_astrr = mikasa_pal_astsr;
        vec = MIKASA_SCB_KAST + (astm << 4);
        }
    else {
        mikasa_pal_sisr = mikasa_pal_sisr & ~(1u << lvl);
        ev5_sirr = mikasa_pal_sisr & MIKASA_PAL_SISR_MASK;
        vec = MIKASA_SCB_SISR0 + (lvl << 4);
        }
    }
else return SCPE_IERR;
if (vec == 0)
    return SCPE_OK;
if ((lvl == MIKASA_IPL_K_CLK) && (vec == MIKASA_SCB_TIMER))
    return mikasa_pal_interrupt_post (MIKASA_INT_K_CLK, vec, 0, lvl);
if (lvl == MIKASA_IPL_K_DEV)
    return mikasa_pal_interrupt_post (MIKASA_INT_K_DEV, vec, 0, lvl);
return mikasa_pal_interrupt_post (MIKASA_INT_K_IP, vec, 0, lvl);
}

t_stat mikasa_pal_proc_trap (uint32 summ)
{
t_stat r;

if (!mikasa_pal_route_os_events ())
    return SCPE_NOFNC;
if (pal_mode)
    return mikasa_pal_mchk_pal ();
r = mikasa_pal_ent_dispatch_frame_pc (ev4_state.paltemp_raw[ptEntArith],
    MODE_K, mikasa_pal_ipl, (PC - 2) & ~((t_uint64) 2));
if (r == SCPE_OK) {
    R[16] = summ;
    R[17] = trap_mask;
    }
return r;
}

static t_bool mikasa_pal_use_pal_dtb_flow (void)
{
return pal_mode != 0;
}

static t_stat mikasa_pal_handle_tbm_bootstrap (uint32 abval, t_uint64 va,
    t_uint64 *pa)
{
if (abval == (EXC_TBM + EXC_E)) {
    if (!mikasa_boot_va_to_pa (va, pa))
        return SCPE_NOFNC;
    mikasa_load_itlb_page (va & ~((t_uint64) VA_M_OFF),
        *pa & ~((t_uint64) VA_M_OFF), PTE_KRE | PTE_KWE);
    return SCPE_OK;
    }

if ((abval == (EXC_TBM + EXC_R)) || (abval == (EXC_TBM + EXC_W))) {
    if (!mikasa_boot_va_to_pa (va, pa))
        return SCPE_NOFNC;
    mikasa_load_dtlb_page (va & ~((t_uint64) VA_M_OFF),
        *pa & ~((t_uint64) VA_M_OFF), PTE_KRE | PTE_KWE);
    PC = (PC - 4) & M64;
    return SCPE_OK;
    }

return SCPE_NOFNC;
}

static uint32 mikasa_pal_pte_mode_read (void)
{
return PTE_KRE << (mikasa_pal_current_mode () & 3);
}

static uint32 mikasa_pal_pte_mode_write (void)
{
return PTE_KWE << (mikasa_pal_current_mode () & 3);
}

static t_stat mikasa_pal_invalid_pte_fault (t_uint64 va, t_uint64 pte,
    t_uint64 ref)
{
uint32 perm = (ref == 1) ? mikasa_pal_pte_mode_write () :
    mikasa_pal_pte_mode_read ();

return mikasa_pal_mm_intexc (va,
    (pte & perm) ? MIKASA_OSF_MM_K_TNV : MIKASA_OSF_MM_K_ACV, ref);
}

static t_stat mikasa_pal_itb_foe_fault (t_uint64 va, t_uint64 pte)
{
return mikasa_pal_mm_intexc (va,
    (pte & mikasa_pal_pte_mode_read ()) ?
    MIKASA_OSF_MM_K_FOE : MIKASA_OSF_MM_K_ACV, (t_uint64) -1);
}

static t_stat mikasa_pal_handle_tbm_native (uint32 abval, t_uint64 va)
{
t_uint64 pte;
uint32 exc;

if (abval == (EXC_TBM + EXC_E)) {
    exc = mikasa_pal_find_pte_vms (va, &pte);
    if (exc != 0)
        return mikasa_pal_mm_intexc (va,
            (exc == EXC_TNV) ? MIKASA_OSF_MM_K_TNV : MIKASA_OSF_MM_K_ACV,
            (t_uint64) -1);
    if ((pte & PTE_V) == 0)
        return mikasa_pal_invalid_pte_fault (va, pte, (t_uint64) -1);
    if (pte & PTE_FOE)
        return mikasa_pal_itb_foe_fault (va, pte);
    itlb_load (VA_GETVPN (va), pte);
    return SCPE_OK;
    }

if ((abval == (EXC_TBM + EXC_R)) || (abval == (EXC_TBM + EXC_W))) {
    if (mikasa_pal_fetch_ignore ()) {
        return SCPE_OK;
        }
    exc = mikasa_pal_find_pte_vms (va, &pte);
    if (exc != 0) {
        PC = (PC - 4) & M64;
        return mikasa_pal_mm_intexc (va,
            (exc == EXC_TNV) ? MIKASA_OSF_MM_K_TNV : MIKASA_OSF_MM_K_ACV,
            (abval == (EXC_TBM + EXC_W)) ? 1 : 0);
        }
    if ((pte & PTE_V) == 0) {
        PC = (PC - 4) & M64;
        return mikasa_pal_invalid_pte_fault (va, pte,
            (abval == (EXC_TBM + EXC_W)) ? 1 : 0);
        }
    dtlb_load (VA_GETVPN (va), pte);
    PC = (PC - 4) & M64;
    return SCPE_OK;
    }

return SCPE_NOFNC;
}

static t_stat mikasa_pal_handle_tbm_pal (uint32 abval, t_uint64 va)
{
t_uint64 pte;
uint32 exc;

sim_debug (MIKASA_DBG_PAL, &mikasa_dev,
    "PAL DTB miss flow abval=%u va=%llX pc=%llX\n",
    abval, (unsigned long long) va, (unsigned long long) PC);

if (abval == (EXC_TBM + EXC_E)) {
    exc = mikasa_pal_find_pte_vms_mode (va, &pte,
        mikasa_pal_ptbr_physical_mode ());
    if (exc != 0)
        return mikasa_pal_mm_intexc (va,
            (exc == EXC_TNV) ? MIKASA_OSF_MM_K_TNV : MIKASA_OSF_MM_K_ACV,
            (t_uint64) -1);
    if ((pte & PTE_V) == 0)
        return mikasa_pal_invalid_pte_fault (va, pte, (t_uint64) -1);
    if (pte & PTE_FOE)
        return mikasa_pal_itb_foe_fault (va, pte);
    itlb_load (VA_GETVPN (va), pte);
    return SCPE_OK;
    }

if ((abval == (EXC_TBM + EXC_R)) || (abval == (EXC_TBM + EXC_W))) {
    if (mikasa_pal_fetch_ignore ()) {
        return SCPE_OK;
        }
    exc = mikasa_pal_find_pte_vms_mode (va, &pte,
        mikasa_pal_ptbr_physical_mode ());
    if (exc != 0) {
        PC = (PC - 4) & M64;
        return mikasa_pal_mm_intexc (va,
            (exc == EXC_TNV) ? MIKASA_OSF_MM_K_TNV : MIKASA_OSF_MM_K_ACV,
            (abval == (EXC_TBM + EXC_W)) ? 1 : 0);
        }
    if ((pte & PTE_V) == 0) {
        PC = (PC - 4) & M64;
        return mikasa_pal_invalid_pte_fault (va, pte,
            (abval == (EXC_TBM + EXC_W)) ? 1 : 0);
        }
    dtlb_load (VA_GETVPN (va), pte);
    PC = (PC - 4) & M64;
    return SCPE_OK;
    }

	return SCPE_NOFNC;
}

static void mikasa_pal_set_mm_args (t_uint64 va, t_uint64 code, t_uint64 ref)
{
R[16] = va;
R[17] = code;
R[18] = ref;
}

static t_bool mikasa_pal_fetch_ignore (void)
{
    uint32 opn;
    uint32 ifnc;

    if (pal_mode)
        return FALSE;
    opn = I_GETOP (ir);
    if (opn != OP_MISC)
        return FALSE;
    ifnc = I_GETMDSP (ir);
    return (ifnc == 0x8000) || (ifnc == 0xA000);
}

t_stat mikasa_pal_proc_excp (uint32 abval)
{
uint32 vec;
t_uint64 va = ((abval == (EXC_TBM + EXC_E)) ||
    (abval == (EXC_TNV + EXC_E)) ||
    (abval == (EXC_BVA + EXC_E)) ||
    (abval == (EXC_ACV + EXC_E))) ? PC : p1;
t_uint64 pa;
t_stat r;

sim_debug (MIKASA_DBG_PAL, &mikasa_dev,
    "PAL EXCP abval=%u va=%llX pc=%llX pal=%u bootmap=%u routeos=%u\n",
    abval, (unsigned long long) va, (unsigned long long) PC, pal_mode,
    mikasa_pal_bootstrap_map (), mikasa_pal_route_os_events ());

switch (abval) {

    case EXC_ALIGN:
        if (mikasa_pal_bootstrap_map ()) {
            if (mikasa_pal_proc_align ())
                return SCPE_OK;
            return SCPE_NOFNC;
            }
        break;

    case EXC_TBM + EXC_E:
        if (mikasa_pal_bootstrap_map ())
            return mikasa_pal_handle_tbm_bootstrap (abval, va, &pa);
        break;

    case EXC_TBM + EXC_R:
    case EXC_TBM + EXC_W:
        if (mikasa_pal_bootstrap_map ())
            return mikasa_pal_handle_tbm_bootstrap (abval, va, &pa);
        break;
        }

if (!mikasa_pal_route_os_events ())
    return SCPE_NOFNC;
if (pal_mode && ((abval != (EXC_TBM + EXC_E)) &&
    (abval != (EXC_TBM + EXC_R)) && (abval != (EXC_TBM + EXC_W))))
    return mikasa_pal_mchk_pal ();

switch (abval) {

    case EXC_RSVI:
        return mikasa_pal_inst_fault (MIKASA_OSF_IF_K_OPCDEC);

    case EXC_RSVO:
        return mikasa_pal_inst_fault (MIKASA_OSF_IF_K_OPCDEC);

    case EXC_ALIGN:
        if (mikasa_direct_apb || mikasa_pal_datfx) {
            if (!mikasa_pal_proc_align ())
                return mikasa_pal_inst_fault (MIKASA_OSF_IF_K_OPCDEC);
            return SCPE_OK;
            }
        r = mikasa_pal_ent_dispatch (ev4_state.paltemp_raw[ptEntUna],
            MODE_K, mikasa_pal_ipl);
        if (r == SCPE_OK) {
            R[16] = p1;
            R[17] = I_GETOP (ir);
            R[18] = I_GETRA (ir);
            }
        return r;

    case EXC_FPDIS:
        if ((fpen & 1) == 0)
            PC = (PC - 4) & M64;
        return mikasa_pal_inst_fault ((fpen & 1) ?
            MIKASA_OSF_IF_K_OPCDEC : MIKASA_OSF_IF_K_FEN);

    case EXC_TBM + EXC_E:
        if (mikasa_pal_use_pal_dtb_flow ())
            return mikasa_pal_handle_tbm_pal (abval, va);
        return mikasa_pal_handle_tbm_native (abval, va);

    case EXC_TBM + EXC_R:
    case EXC_TBM + EXC_W:
        if (mikasa_pal_use_pal_dtb_flow ())
            return mikasa_pal_handle_tbm_pal (abval, va);
        return mikasa_pal_handle_tbm_native (abval, va);

    case EXC_FOX + EXC_E:
        tlb_is (p1, TLB_CI);
        return mikasa_pal_mm_intexc (va, MIKASA_OSF_MM_K_FOE,
            (t_uint64) -1);

    case EXC_FOX + EXC_R:
        if (mikasa_pal_fetch_ignore ()) {
            return SCPE_OK;
            }
        PC = (PC - 4) & M64;
        return mikasa_pal_mm_intexc (p1, MIKASA_OSF_MM_K_FOR, 0);

    case EXC_FOX + EXC_W:
        if (mikasa_pal_fetch_ignore ()) {
            return SCPE_OK;
            }
        PC = (PC - 4) & M64;
        return mikasa_pal_mm_intexc (p1, MIKASA_OSF_MM_K_FOW, 1);

    case EXC_BVA + EXC_E:
    case EXC_ACV + EXC_E:
        return mikasa_pal_mm_intexc (va, MIKASA_OSF_MM_K_ACV,
            (t_uint64) -1);

    case EXC_BVA + EXC_R:
    case EXC_ACV + EXC_R:
        if (mikasa_pal_fetch_ignore ()) {
            return SCPE_OK;
            }
        PC = (PC - 4) & M64;
        return mikasa_pal_mm_intexc (p1, MIKASA_OSF_MM_K_ACV, 0);

    case EXC_BVA + EXC_W:
    case EXC_ACV + EXC_W:
        if (mikasa_pal_fetch_ignore ()) {
            return SCPE_OK;
            }
        PC = (PC - 4) & M64;
        return mikasa_pal_mm_intexc (p1, MIKASA_OSF_MM_K_ACV, 1);

    case EXC_TNV + EXC_E:
        tlb_is (p1, TLB_CI);
        return mikasa_pal_mm_intexc (va, MIKASA_OSF_MM_K_TNV,
            (t_uint64) -1);

    case EXC_TNV + EXC_R:
        if (mikasa_pal_fetch_ignore ()) {
            return SCPE_OK;
            }
        tlb_is (p1, TLB_CD);
        PC = (PC - 4) & M64;
        return mikasa_pal_mm_intexc (p1, MIKASA_OSF_MM_K_TNV, 0);

    case EXC_TNV + EXC_W:
        if (mikasa_pal_fetch_ignore ()) {
            return SCPE_OK;
            }
        tlb_is (p1, TLB_CD);
        PC = (PC - 4) & M64;
        return mikasa_pal_mm_intexc (p1, MIKASA_OSF_MM_K_TNV, 1);
        }
vec = mikasa_pal_excp_scb (abval);
if (vec != 0)
    return mikasa_pal_intexc (vec, MODE_K, mikasa_pal_ipl);

return STOP_INVABO;
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

static t_bool mikasa_pal_proc_align_store (void)
{
switch (I_GETOP (ir)) {

    case OP_STW:
    case OP_STL:
    case OP_STQ:
    case OP_STL_C:
    case OP_STQ_C:
        return TRUE;
        }

return FALSE;
}

static uint32 mikasa_pal_find_pte_vms_mode (t_uint64 va, t_uint64 *pte,
    t_bool physical_mode)
{
uint32 va_sext = VA_GETSEXT (va);
uint32 vpn = VA_GETVPN (va);
t_uint64 vptea;
t_uint64 l1ptea, l2ptea, l3ptea;
t_uint64 l1pte, l2pte;
TLBENT *tlbp;

if ((va_sext != 0) && (va_sext != VA_M_SEXT))
    return EXC_BVA;
if (physical_mode) {
    *pte = (((va >> VA_N_OFF) & PFN_MASK) << PTE_V_PFN) |
        MIKASA_PAL_PHYS_PTE;
    return 0;
    }
vptea = mikasa_pal_vtbr_va () | (((t_uint64) (vpn & VA_M_VPN)) << 3);
tlbp = dtlb_lookup (VA_GETVPN (vptea));
if ((tlbp != NULL) && (tlbp->tag == VA_GETVPN (vptea)) &&
    ((tlbp->pte & (PTE_KRE | PTE_V)) == (PTE_KRE | PTE_V)))
    l3ptea = PHYS_ADDR (tlbp->pfn, vptea);
else {
    l1ptea = mikasa_pal_ptbr_pa () + VPN_GETLVL1 (vpn);
    if (!mikasa_pal_valid_phys (l1ptea, sizeof (t_uint64)))
        return EXC_ACV;
    l1pte = ReadPQ (l1ptea);
    if ((l1pte & PTE_V) == 0)
        return ((l1pte & PTE_KRE) ? EXC_TNV : EXC_ACV);
    l2ptea = (((l1pte >> PTE_V_PFN) & PFN_MASK) << VA_N_OFF) +
        VPN_GETLVL2 (vpn);
    if (!mikasa_pal_valid_phys (l2ptea, sizeof (t_uint64)))
        return EXC_ACV;
    l2pte = ReadPQ (l2ptea);
    if ((l2pte & PTE_V) == 0)
        return ((l2pte & PTE_KRE) ? EXC_TNV : EXC_ACV);
    l3ptea = (((l2pte >> PTE_V_PFN) & PFN_MASK) << VA_N_OFF) +
        VPN_GETLVL3 (vpn);
    }
if (!mikasa_pal_valid_phys (l3ptea, sizeof (t_uint64)))
    return EXC_ACV;
*pte = ReadPQ (l3ptea);
return 0;
}

static uint32 mikasa_pal_find_pte_vms (t_uint64 va, t_uint64 *pte)
{
return mikasa_pal_find_pte_vms_mode (va, pte, mikasa_pal_physical_mode ());
}

static uint32 mikasa_pal_mm_exc (uint32 not_set)
{
uint32 tacc;

tacc = not_set & ~(PTE_FOR | PTE_FOW | PTE_FOE | PTE_V);
if (tacc != 0)
    return EXC_ACV;
tacc = not_set & (PTE_FOR | PTE_FOW | PTE_FOE);
if (tacc != 0)
    return EXC_FOX;
return EXC_TNV;
}

static uint32 mikasa_pal_test_va (t_uint64 va, uint32 acc, t_uint64 *pa)
{
uint32 va_sext = VA_GETSEXT (va);
uint32 vpn = VA_GETVPN (va);
t_uint64 pte;
uint32 exc;
TLBENT *tlbp;

if (!dmapen) {
    if (pa != NULL)
        *pa = va & EV5_PA_MASK;
    return 0;
    }
if ((va_sext != 0) && (va_sext != VA_M_SEXT))
    return EXC_BVA;
tlbp = dtlb_lookup (vpn);
if (tlbp == NULL) {
    exc = mikasa_pal_find_pte_vms (va, &pte);
    if (exc != 0)
        return exc;
    tlbp = dtlb_load (vpn, pte);
    }
if (acc & ~tlbp->pte)
    return mikasa_pal_mm_exc (acc & ~tlbp->pte);
if (pa != NULL)
    *pa = PHYS_ADDR (tlbp->pfn, va);
return 0;
}

static uint32 mikasa_pal_tlb_check (t_uint64 va)
{
uint32 va_sext = VA_GETSEXT (va);
uint32 vpn = VA_GETVPN (va);
TLBENT *tlbp;

if ((va_sext != 0) && (va_sext != VA_M_SEXT))
    return 0;
tlbp = itlb_lookup (vpn);
if (tlbp != NULL)
    return 1;
tlbp = dtlb_lookup (vpn);
if (tlbp != NULL)
    return 1;
return 0;
}

static uint32 mikasa_pal_probe (uint32 acc)
{
t_uint64 pte;
uint32 pm = ((uint32) R[18]) & 3;
uint32 cm = mikasa_pal_current_mode ();
uint32 req;
t_uint64 end_va = (R[16] + R[17]) & M64;

if (pm <= cm)
    pm = cm;
req = (acc << pm) | PTE_V;
if (mikasa_direct_apb) {
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
if (mikasa_pal_find_pte_vms (R[16], &pte) != 0)
    return 0;
if (req & ~((uint32) pte))
    return 0;
if (mikasa_pal_find_pte_vms (end_va, &pte) != 0)
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
#define MPAL_CFLUSH     0x01
#define MPAL_DRAINA     0x02
#define MPAL_LDQP       0x03
#define MPAL_STQP       0x04
#define MPAL_SWPCTX     0x30
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
#define MPAL_WRFEN      0x2B
#define MPAL_WRVPTPTR   0x2D
#define MPAL_WRVAL      0x31
#define MPAL_RDVAL      0x32
#define MPAL_TBI        0x33
#define MPAL_WRENT      0x34
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
#define MPAL_MT_PERFMON 0x39
#define MPAL_SWPIPL     0x35
#define MPAL_MT_DATFX   0x2E
#define MIKASA_MPAL_MF_VIRBND  0xF0
#define MIKASA_MPAL_MT_VIRBND  0xF1
#define MIKASA_MPAL_MF_SYSPTBR 0xF2
#define MIKASA_MPAL_MT_SYSPTBR 0xF3
#define MPAL_RDPS       0x36
#define MPAL_WRKGP      0x37
#define MPAL_WRUSP      0x38
#define MPAL_RDUSP      0x3A
#define MPAL_RETSYS     0x3D
#define MPAL_WTINT      0x3E
#define MPAL_RTI        0x3F
#define MPAL_MF_WHAMI   0x3C
#define MPAL_BPT        0x80
#define MPAL_BUGCHK     0x81
#define MPAL_CHME       0x82
#define MPAL_CALLSYS    0x83
#define MPAL_CHMS       0x84
#define MPAL_CHMU       0x85
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
#define MIKASA_MPAL_CHMK 0xE3

#define MPAL_IPL_MASK   0x1F
#define MPAL_AST_MASK   0x0F
#define MPAL_SISR_MASK  0xFFFE
#define MPAL_PS_SW_MASK 0x03
#define MIKASA_MCES_DPC 0x08
#define MIKASA_MCES_DSC 0x10
#define MIKASA_INT_K_ZAP_PC \
    ((t_uint64) 0x0000818181818181u)
#define MIKASA_ICCSR_W_PME \
    ((((t_uint64) 1u) << 44) | (((t_uint64) 1u) << 45))
#define MIKASA_ICCSR_W_MUX_PC \
    (((t_uint64) ICCSR_M_MUX0) | ((t_uint64) ICCSR_M_PC0) | \
     ((t_uint64) ICCSR_M_PC1) | (((t_uint64) ICCSR_M_MUX1) << 32))

#define MIKASA_CSERVE_RDIO     0x10
#define MIKASA_CSERVE_WRIO     0x11
#define MIKASA_CSERVE_HALT     0x3E
#define MIKASA_CSERVE_RDWHAMI  0x41
#define MIKASA_CSERVE_START    0x42
#define MIKASA_CSERVE_WRIPIR   0x43
#define MIKASA_CSERVE_JUMP     0x44
#define MIKASA_CSERVE_WRMCES   0x45
#define MIKASA_CSERVE_SWPPAL   0x65
#define MIKASA_CSERVE_IMB      0x66
#define MIKASA_CSERVE_WR_INT   0x0A
#define MIKASA_CSERVE_RD_IMPURE 0x0B
#define MIKASA_CSERVE_PUTC     0x0F
#define MIKASA_CSERVE_LDQP     0x01
#define MIKASA_CSERVE_STQP     0x02
#define MIKASA_CSERVE_RD_ABOX  0x03
#define MIKASA_CSERVE_RD_BIU   0x04
#define MIKASA_CSERVE_RD_ICCSR 0x05
#define MIKASA_CSERVE_WR_ABOX  0x06
#define MIKASA_CSERVE_WR_BIU   0x07
#define MIKASA_CSERVE_WR_ICCSR 0x08

#define MIKASA_PCSERVE_LDQP      0x01
#define MIKASA_PCSERVE_STQP      0x02
#define MIKASA_PCSERVE_JTOPAL    0x09
#define MIKASA_PCSERVE_RD_IMPURE 0x0B

#ifndef MIKASA_CSERVE_JTOPAL
#define MIKASA_CSERVE_JTOPAL MIKASA_PCSERVE_JTOPAL
#endif

#define MIKASA_CSERVE_JTOPAL_SIG 0xDECB
#define MIKASA_ICCSR_W_V_ASN     47
#define MIKASA_ICCSR_W_M_ASN     0x3FULL
#define MIKASA_ICCSR_W_ASN \
    (((t_uint64) MIKASA_ICCSR_W_M_ASN) << MIKASA_ICCSR_W_V_ASN)
#define MIKASA_ICCSR_W_V_FPE     42
#define MIKASA_ICCSR_W_PCBB \
    (((t_uint64) 0x7F9u) << MIKASA_ICCSR_W_V_FPE)
#define MIKASA_ICCSR_W_FPE \
    (((t_uint64) 1u) << 42)
#define MIKASA_ICCSR_W_MAP \
    (((t_uint64) 1u) << 41)
#define MIKASA_ICCSR_W_DI \
    (((t_uint64) 1u) << 39)
#define MIKASA_ICCSR_W_BHE \
    (((t_uint64) 1u) << 38)
#define MIKASA_ICCSR_W_JSE \
    (((t_uint64) 1u) << 37)
#define MIKASA_ICCSR_W_BPE \
    (((t_uint64) 1u) << 36)
#define MIKASA_ICCSR_W_PIPE \
    (((t_uint64) 1u) << 35)
#define MIKASA_ICCSR_K_INIT \
    (MIKASA_ICCSR_W_FPE | MIKASA_ICCSR_W_MAP | MIKASA_ICCSR_W_DI | \
     MIKASA_ICCSR_W_BHE | MIKASA_ICCSR_W_JSE | MIKASA_ICCSR_W_BPE | \
     MIKASA_ICCSR_W_PIPE)
#define MIKASA_ABOX_K_INIT \
    (ABOX_M_DC_ENA | ABOX_M_SPE_2 | ABOX_M_IC_SBUF_EN)
#define MIKASA_BIU_K_CTL         BIU_M_OE
#define MIKASA_PVPTPTR_RESET     (((t_uint64) 2u) << 32)

static t_bool mikasa_pal_valid_phys (t_uint64 pa, uint32 size)
{
if (size == 0)
    return TRUE;
if (pa >= MEMSIZE)
    return FALSE;
return (((t_uint64) size - 1) <= (((t_uint64) MEMSIZE - 1) - pa));
}

static void mikasa_pal_set_pc (t_uint64 pc)
{
PCQ_ENTRY;
PC = pc & M64;
pc_align = (uint32) (pc & 3);
return;
}

static void mikasa_pal_load_jtopal_params (void)
{
t_uint64 base;
t_uint64 val;

if (mikasa_pal_impure == 0)
    return;

base = mikasa_pal_impure + CNS_Q_BASE;
if (!mikasa_pal_valid_phys (base + CNS_Q_SIGNATURE, sizeof (t_uint64)))
    return;

val = ReadPQ (base + CNS_Q_SIGNATURE);
if (((val >> 16) & 0xFFFF) != MIKASA_CSERVE_JTOPAL_SIG)
    return;

R[19] = val;
if (mikasa_pal_valid_phys (base + CNS_Q_ABOX_CTL, sizeof (t_uint64)))
    R[1] = ReadPQ (base + CNS_Q_ABOX_CTL);
if (mikasa_pal_valid_phys (base + CNS_Q_BIU_CTL, sizeof (t_uint64)))
    R[2] = ReadPQ (base + CNS_Q_BIU_CTL);
if (mikasa_pal_valid_phys (base + CNS_Q_PROC_ID, sizeof (t_uint64)))
    R[16] = ReadPQ (base + CNS_Q_PROC_ID);
if (mikasa_pal_valid_phys (base + CNS_Q_MEM_SIZE, sizeof (t_uint64)))
    R[17] = ReadPQ (base + CNS_Q_MEM_SIZE);
if (mikasa_pal_valid_phys (base + CNS_Q_CYCLE_CNT, sizeof (t_uint64)))
    R[18] = ReadPQ (base + CNS_Q_CYCLE_CNT);
if (mikasa_pal_valid_phys (base + CNS_Q_PROC_MASK, sizeof (t_uint64)))
    R[20] = ReadPQ (base + CNS_Q_PROC_MASK);
if (mikasa_pal_valid_phys (base + CNS_Q_SYSCTX, sizeof (t_uint64)))
    R[21] = ReadPQ (base + CNS_Q_SYSCTX);
if (mikasa_pal_valid_phys (base + CNS_Q_SROM_REV, sizeof (t_uint64)))
    R[22] = ReadPQ (base + CNS_Q_SROM_REV);
R[19] = val;
}

static void mikasa_pal_init_paltemp_regs (void)
{
mikasa_pal_set_paltemp_raw (2, MIKASA_ICCSR_K_INIT);             /* pt2_iccsr   */
mikasa_pal_set_paltemp_raw (9, mikasa_pal_pack_ps (ev4_state.ipl & 0x7,
    ev4_state.itlb_cm & 0x1));                                  /* pt9_ps      */
mikasa_pal_set_paltemp_raw (17, mikasa_pal_impure);              /* ptImpure    */
mikasa_pal_set_paltemp_raw (18, mikasa_pal_usp);                 /* ptUsp       */
mikasa_pal_set_paltemp_raw (19, mikasa_pal_stkp[MODE_K]);        /* ptKsp       */
mikasa_pal_set_paltemp_raw (20, 0);                             /* ptKgp       */
mikasa_pal_set_paltemp_raw (24, mikasa_pal_sysval);              /* ptSysVal    */
mikasa_pal_set_paltemp_raw (25, mikasa_pal_mces);               /* ptMces      */
mikasa_pal_set_paltemp_whami (mikasa_pal_whami_reg);             /* ptWhami     */
mikasa_pal_set_paltemp_raw (28, mikasa_pal_ptbr);               /* ptPtbr      */
mikasa_pal_set_paltemp_raw (29, MIKASA_PVPTPTR_RESET);          /* ptVptPtr    */
mikasa_pal_set_paltemp_raw (30, ev5_palbase);                   /* ptPrevPal   */
mikasa_pal_set_paltemp_raw (31, (mikasa_pal_impure ?
    (mikasa_pal_impure + CNS_Q_SCRATCH) : 0));                  /* ptPrcb      */
mikasa_pal_set_ptintmask (INT_K_MASK);                          /* ptIntMask   */

mikasa_pal_ipl = ev4_state.ipl & 0x7;
ev4_state.itlb_cm = (ev4_state.itlb_cm & 1u);
ev4_state.dtlb_cm = (ev4_state.dtlb_cm & 1u);
}

static void mikasa_pal_init_ipr_shadows (void)
{
ev4_write_icsr (MIKASA_ICCSR_K_INIT);
ev4_write_abox_ipr (0, MIKASA_ABOX_K_INIT);   /* aboxCtl */
ev4_write_abox_ipr (18, MIKASA_BIU_K_CTL);    /* biuCtl */
ev4_state.ipl = (ev4_state.ipl & 0x7);
ev4_state.hier_cre = (ev4_state.ipl <= 6) ? 1 : 0;
ev4_state.hirr_crr = 0;
ev4_state.hirr_pc0 = 0;
ev4_state.hirr_pc1 = 0;
ev4_state.hirr_slr = 0;
ev4_state.hier_pc0 = 0;
ev4_state.hier_pc1 = 0;
ev4_state.hier_sle = 0;
ev4_state.sirr = 0;
ev4_state.sier = 0;
ev4_state.aster = 0;
ev4_state.sl_clr = 0;
ev4_state.sl_rcv = 0;
ev4_state.sl_xmit = 0;
ev4_state.ibox_ipr_raw[12] = 0;
ev4_state.abox_ipr_raw[0] = MIKASA_ABOX_K_INIT;
ev4_state.abox_ipr_raw[18] = MIKASA_BIU_K_CTL;
}

static uint32 mikasa_pal_current_mode (void)
{
return dtlb_cm & MIKASA_PSV_M_CM;
}

static uint32 mikasa_pal_stack_mode (uint32 mode)
{
return (mode & 1u) ? MODE_U : MODE_K;
}

static t_uint64 mikasa_pal_read_ps (void)
{
return mikasa_pal_pack_ps (mikasa_pal_ipl, mikasa_pal_current_mode ());
}

static uint32 mikasa_pal_ps_ipl (t_uint64 ps)
{
return (uint32) (ps & MIKASA_OSF_PSV_M_IPL);
}

static uint32 mikasa_pal_ps_mode (t_uint64 ps)
{
return (uint32) ((ps >> MIKASA_PSV_V_CM) & 1u);
}

static t_uint64 mikasa_pal_pack_ps (uint32 ipl, uint32 mode)
{
return ((t_uint64) (ipl & MIKASA_OSF_PSV_M_IPL)) |
    (((t_uint64) (mode & 1u)) << MIKASA_PSV_V_CM);
}

static void mikasa_pal_set_paltemp_whami (t_uint64 val)
{
mikasa_pal_whami_reg = val & M64;
mikasa_pal_set_paltemp_raw (27, mikasa_pal_whami_reg);
}

static void mikasa_pal_mark_pal_swapped (void)
{
t_uint64 whami = mikasa_pal_whami_reg;

whami = (whami & MIKASA_WHAMI_M_ID) |
    ((MIKASA_WHAMI_K_SWAP << MIKASA_WHAMI_V_SWAP) & M64);
mikasa_pal_set_paltemp_whami (whami);
}

static void mikasa_pal_set_ptintmask (t_uint64 mask)
{
mikasa_pal_set_paltemp_raw (22, mask);                    /* ptIntMask */
mikasa_pal_set_hier_from_ipl (ev4_state.ipl);
}

static t_uint64 mikasa_pal_live_hirr (void)
{
uint32 pins = 0;
uint32 i;
t_uint64 hirr;

for (i = 0; i < IPL_HLVL; i++) {
    if (int_req[i])
        pins |= 1u << i;
    }

hirr = ev4_hirr_bits (pins);
if (pins != 0)
    hirr |= HIRR_M_HWR;
if (ev5_crd || ev4_state.hirr_crr)
    hirr |= HIRR_M_CRR;
if (ev4_state.hirr_pc0)
    hirr |= HIRR_M_PC0;
if (ev4_state.hirr_pc1)
    hirr |= HIRR_M_PC1;
if (ev4_state.hirr_slr)
    hirr |= HIRR_M_SLR;
return hirr;
}

static t_uint64 mikasa_pal_read_hier (void)
{
return ev4_hirr_bits (ev4_state.hier) |
    (((t_uint64) (ev4_state.hier_cre & 1)) << 4) |
    (((t_uint64) (ev4_state.hier_pc0 & 1)) << 9) |
    (((t_uint64) (ev4_state.hier_pc1 & 1)) << 8) |
    (((t_uint64) (ev4_state.hier_sle & 1)) << 13);
}

static void mikasa_pal_apply_ipl_mask (void)
{
uint32 ipl = ev4_state.ipl & 0x7;
uint32 mask = (uint32) ((ev4_state.paltemp_raw[22] >> (ipl * 8)) & 0xFF);

ev4_state.hier = (mask >> 1) & 0x3F;
ev4_state.hier_pc0 = mask & 0x01;
ev4_state.hier_pc1 = (mask >> 7) & 0x01;
}

static void mikasa_pal_init_int_mask (void)
{
mikasa_pal_set_paltemp_raw (22, INT_K_MASK);              /* ptIntMask */
mikasa_pal_apply_ipl_mask ();
ev4_state.hier_cre = (ev4_state.ipl <= 6) ? 1 : 0;
}

static void mikasa_pal_set_hier_from_ipl (uint32 ipl)
{
ev4_state.ipl = ipl & 0x7;
mikasa_pal_apply_ipl_mask ();
ev4_state.hier_cre = (ev4_state.ipl <= 6) ? 1 : 0;
}

static void mikasa_pal_set_mode (uint32 mode, uint32 ipl)
{
mikasa_pal_ipl = ipl & MPAL_IPL_MASK;
ev5_ipl = mikasa_pal_ipl;
mikasa_pal_set_hier_from_ipl (mikasa_pal_ipl);
ev4_state.itlb_cm = mode & 0x1;
ev4_state.dtlb_cm = mode & 0x1;
ev4_state.ps_sw = (ev4_state.ipl & 0x7) | ((mode & 0x1) << 3);
mikasa_pal_set_paltemp_raw (9, mikasa_pal_pack_ps (mikasa_pal_ipl, mode)); /* pt9_ps */
tlb_set_cm (mode);
}

static void mikasa_pal_save_impure_state (void)
{
t_uint64 base;
uint32 i;

if ((mikasa_pal_impure == 0) ||
    !mikasa_pal_valid_phys (mikasa_pal_impure + CNS_Q_BASE,
    CNS_K_SIZE))
    return;

base = mikasa_pal_impure + CNS_Q_BASE;
for (i = 0; i < 32; i++)
    WritePQ (base + CNS_Q_GPR + ((t_uint64) i << 3), R[i]);
for (i = 0; i < PALTEMP_SIZE; i++)
    WritePQ (base + CNS_Q_PT + ((t_uint64) i << 3),
        ev4_state.paltemp_raw[i]);

WritePQ (base + CNS_Q_EXC_ADDR, ev4_state.exc_addr);
WritePQ (base + CNS_Q_PS, mikasa_pal_read_ps ());
WritePQ (base + CNS_Q_PAL_BASE, ev4_state.pal_base);
WritePQ (base + CNS_Q_HIER, mikasa_pal_read_hier ());
WritePQ (base + CNS_Q_SIRR, mikasa_pal_sisr);
WritePQ (base + CNS_Q_ICCSR, ev4_read_icsr ());
WritePQ (base + CNS_Q_ABOX_CTL, ev4_read_abox_ipr (0));
WritePQ (base + CNS_Q_BIU_CTL, ev4_read_abox_ipr (18));
}

static void mikasa_pal_reset_impure (t_uint64 impure)
{
if ((impure == 0) || !mikasa_pal_valid_phys (impure, CNS_Q_BASE + CNS_K_SIZE))
    return;

mikasa_pal_impure = impure;
mikasa_zero_phys (impure, CNS_Q_BASE + CNS_K_SIZE);
mikasa_pal_set_paltemp_raw (17, mikasa_pal_impure);       /* ptImpure */
mikasa_pal_set_paltemp_raw (31, mikasa_pal_impure + CNS_Q_SCRATCH);
mikasa_pal_save_impure_state ();
}

static t_uint64 mikasa_pal_build_short_logout (t_uint64 code)
{
t_uint64 base = mikasa_pal_impure + LAS_Q_BASE;
t_uint64 logout_code;
t_uint64 bc_tag;

if ((mikasa_pal_impure == 0) || !mikasa_pal_valid_phys (base, LAS_K_SIZE))
    return 0;
mikasa_zero_phys (base, LAS_K_SIZE);
WritePL (base + LAS_L_FRAME, LAS_K_SIZE);
WritePL (base + LAS_L_FLAG, 1u << 31);
WritePL (base + LAS_L_CPU, LAS_Q_BIU_STAT);
WritePL (base + LAS_L_SYS, (uint32) -1);
logout_code = (((t_uint64) MIKASA_MCHK_K_REV) << 32) |
    ((code != 0) ? code : (t_uint64) MIKASA_MCHK_K_ECC_C);
if (code == MIKASA_MCHK_K_UNKNOWN)
    logout_code = (((t_uint64) MIKASA_MCHK_K_REV) << 32) |
        (t_uint64) MIKASA_MCHK_K_CACKSOFT;
WritePQ (base + LAS_Q_MCHK_CODE, logout_code);
WritePQ (base + LAS_Q_BIU_STAT, ev4_read_abox_ipr (10));
WritePQ (base + LAS_Q_BIU_ADDR, ev4_read_abox_ipr (9));
WritePQ (base + LAS_Q_DC_STAT, ev4_state.abox_ipr_raw[12]);
WritePQ (base + LAS_Q_FILL_SYNDROME, ev4_read_abox_ipr (19));
WritePQ (base + LAS_Q_FILL_ADDR, ev4_read_abox_ipr (13));
bc_tag = ev4_read_abox_ipr (20);
WritePQ (base + LAS_Q_BC_TAG, bc_tag);
return base;
}

static t_uint64 mikasa_pal_build_long_logout (t_uint64 code)
{
t_uint64 base = mikasa_pal_impure + LAF_Q_BASE;
t_uint64 logout_code = code;
uint32 i;

if ((mikasa_pal_impure == 0) || !mikasa_pal_valid_phys (base, LAF_K_SIZE))
    return 0;
mikasa_zero_phys (base, LAF_K_SIZE);
WritePL (base + LAF_L_FRAME, LAF_K_SIZE);
WritePL (base + LAF_L_CPU, LAF_Q_EXC_ADDR);
WritePL (base + LAF_L_SYS, LAF_Q_SYS_BASE);
if ((code & M32) == MIKASA_MCHK_K_ICPERR)
    WritePL (base + LAF_L_FLAG, 1u << 31);
else
    WritePL (base + LAF_L_FLAG, 0);
WritePQ (base + LAF_Q_DC_STAT, code >> 32);
logout_code = (((t_uint64) MIKASA_MCHK_K_REV) << 32) | (code & M32);
for (i = 0; i < PALTEMP_SIZE; i++)
    WritePQ (base + LAF_Q_PT0 + ((t_uint64) i << 3),
        ev4_state.paltemp_raw[i]);
WritePQ (base + LAF_Q_PT0, logout_code);
WritePQ (base + LAF_Q_EXC_ADDR, ev4_state.exc_addr);
WritePQ (base + LAF_Q_PAL_BASE, ev4_state.pal_base);
WritePQ (base + LAF_Q_HIER, mikasa_pal_read_hier ());
WritePQ (base + LAF_Q_HIRR, mikasa_pal_live_hirr ());
WritePQ (base + LAF_Q_MM_CSR, ev4_read_abox_ipr (4));
if ((code >> 32) == 0)
    WritePQ (base + LAF_Q_DC_STAT, ev4_state.abox_ipr_raw[12]);
WritePQ (base + LAF_Q_DC_ADDR, ev4_read_abox_ipr (11));
WritePQ (base + LAF_Q_ABOX_CTL, mikasa_pal_read_cns_quad (CNS_Q_ABOX_CTL,
    ev4_read_abox_ipr (0)));
WritePQ (base + LAF_Q_BIU_STAT, ev4_read_abox_ipr (10));
WritePQ (base + LAF_Q_BIU_ADDR, ev4_read_abox_ipr (9));
WritePQ (base + LAF_Q_BIU_CTL, mikasa_pal_read_cns_quad (CNS_Q_BIU_CTL,
    ev4_read_abox_ipr (18)));
WritePQ (base + LAF_Q_FILL_SYNDROME, ev4_read_abox_ipr (19));
WritePQ (base + LAF_Q_FILL_ADDR, ev4_read_abox_ipr (13));
WritePQ (base + LAF_Q_VA, ev4_read_abox_ipr (5));
WritePQ (base + LAF_Q_BC_TAG, ev4_read_abox_ipr (20));
return base;
}

static t_stat mikasa_pal_mchk_logout (t_uint64 code)
{
t_uint64 frame;
t_stat r;

if (mikasa_pal_mces & MCES_INP) {
    mikasa_pal_set_paltemp_raw (25, mikasa_pal_mces);
    if (mikasa_pal_valid_phys (mikasa_pal_pcbb + MIKASA_PROC_HALT_CODE,
        sizeof (t_uint64)))
        WritePQ (mikasa_pal_pcbb + MIKASA_PROC_HALT_CODE, HLT_K_DBL_MCHK);
    return STOP_HALT;
    }
mikasa_pal_mces |= MCES_INP;
mikasa_pal_set_paltemp_raw (25, mikasa_pal_mces);
mikasa_pal_save_impure_state ();
frame = mikasa_pal_build_long_logout (code);
if (frame == 0)
    return STOP_MME;
ev5_crd = 0;
ev4_state.hirr_crr = 0;
r = mikasa_pal_ent_dispatch (ev4_state.paltemp_raw[ptEntInt],
    MODE_K, MIKASA_IPL_K_MCHK);
if (r == SCPE_OK) {
    R[16] = MIKASA_INT_K_MCHK;
    R[17] = (code >= MIKASA_MCHK_K_SYSTEM) ?
        MIKASA_SCB_Q_SYSMCHK : MIKASA_SCB_Q_PROCMCHK;
    R[18] = MIKASA_KSEG_BASE | frame;
    }
return r;
}

static t_stat mikasa_pal_mchk_pal (void)
{
return mikasa_pal_mchk_logout (MIKASA_MCHK_K_BUGCHECK);
}

static t_stat mikasa_pal_mchk_callsys (void)
{
return mikasa_pal_mchk_logout (MIKASA_MCHK_K_OS_BUGCHECK);
}

static t_stat mikasa_pal_mchk_ksp_not_valid (void)
{
t_stat r = mikasa_pal_save_pcbb_state ();

if (r != SCPE_OK)
    return r;
if (mikasa_pal_valid_phys (mikasa_pal_pcbb + MIKASA_PROC_HALT_CODE,
    sizeof (t_uint64)))
    WritePQ (mikasa_pal_pcbb + MIKASA_PROC_HALT_CODE, HLT_K_KSP_INVAL);
return STOP_HALT;
}

static t_stat mikasa_pal_crd_logout (void)
{
t_uint64 frame;
t_stat r;
t_bool pce;
uint32 disable;
uint32 in_progress;

ev5_crd = 0;
ev4_state.hirr_crr = 0;
pce = (ev4_read_abox_ipr (10) & BIU_STAT_M_FILL_ECC) != 0;
disable = pce ? MIKASA_MCES_DPC : MIKASA_MCES_DSC;
in_progress = MCES_SCRD | MCES_PCRD;
mikasa_pal_save_impure_state ();
frame = 0;
if ((mikasa_pal_mces & in_progress) == 0)
    frame = mikasa_pal_build_short_logout (pce ?
        MIKASA_MCHK_K_ECC_C : MIKASA_MCHK_K_CACKSOFT);
if (((mikasa_pal_mces & in_progress) == 0) && (frame == 0))
    return STOP_MME;
if (mikasa_pal_mces & (disable | in_progress))
    return SCPE_OK;
mikasa_pal_mces |= pce ? MCES_PCRD : MCES_SCRD;
mikasa_pal_set_paltemp_raw (25, mikasa_pal_mces);
r = mikasa_pal_ent_dispatch (ev4_state.paltemp_raw[ptEntInt],
    MODE_K, MIKASA_IPL_K_MCHK);
if (r == SCPE_OK) {
    R[16] = MIKASA_INT_K_MCHK;
    R[17] = pce ? MIKASA_SCB_Q_PROCERR : MIKASA_SCB_Q_SYSERR;
    R[18] = MIKASA_KSEG_BASE | frame;
    }
return r;
}

static t_bool mikasa_pal_physical_mode (void)
{
return (mikasa_pal_vtbr & MIKASA_PAL_PTBR_PHYS) != 0;
}

static t_bool mikasa_pal_ptbr_physical_mode (void)
{
return (mikasa_pal_ptbr & MIKASA_PAL_PTBR_PHYS) != 0;
}

static t_uint64 mikasa_pal_ptbr_pa (void)
{
return mikasa_pal_ptbr & ~((t_uint64) MIKASA_PAL_PTBR_PHYS);
}

static t_uint64 mikasa_pal_ptbr_pfn (void)
{
return (mikasa_pal_ptbr_pa () >> VA_N_OFF) |
    ((mikasa_pal_ptbr & MIKASA_PAL_PTBR_PHYS) ? Q_SIGN : 0);
}

static t_uint64 mikasa_pal_vtbr_va (void)
{
return mikasa_pal_vtbr & ~((t_uint64) MIKASA_PAL_PTBR_PHYS);
}

static t_uint64 mikasa_pal_ptbr_from_pfn (t_uint64 pfn)
{
return ((pfn & ~Q_SIGN) << VA_N_OFF) |
    ((pfn & Q_SIGN) ? MIKASA_PAL_PTBR_PHYS : 0);
}

static t_uint64 mikasa_pal_pcbb_icsr (t_uint64 icsr, uint32 asn, t_uint64 fen)
{
t_uint64 bits;

bits = (((fen >> 59) | (fen & 1)) & ~((t_uint64) 0x16));
bits |= (((t_uint64) asn & MIKASA_ICCSR_W_M_ASN) << 5);
return (icsr & ~MIKASA_ICCSR_W_PCBB) |
    ((bits << MIKASA_ICCSR_W_V_FPE) & MIKASA_ICCSR_W_PCBB);
}

static void mikasa_pal_set_paltemp_raw (uint32 idx, t_uint64 value)
{
ev4_state.paltemp_raw[idx] = value;
ev5_paltemp[idx] = value;
}

static t_bool mikasa_pal_cns_quad_pa (uint32 offset, t_uint64 *pa)
{
if (mikasa_pal_impure == 0)
    return FALSE;
*pa = mikasa_pal_impure + CNS_Q_BASE + offset;
return mikasa_pal_valid_phys (*pa, sizeof (t_uint64));
}

static t_uint64 mikasa_pal_read_cns_quad (uint32 offset, t_uint64 fallback)
{
t_uint64 pa;

if (!mikasa_pal_cns_quad_pa (offset, &pa))
    return fallback;
return ReadPQ (pa);
}

static void mikasa_pal_write_cns_quad (uint32 offset, t_uint64 value)
{
t_uint64 pa;

if (mikasa_pal_cns_quad_pa (offset, &pa))
    WritePQ (pa, value);
}

static void mikasa_pal_sync_paltemp_icsr (void)
{
t_uint64 icsr = ev5_icsr;

if (fpen & 1)
    icsr |= MIKASA_ICCSR_W_FPE;
else
    icsr &= ~MIKASA_ICCSR_W_FPE;
ev4_write_icsr (icsr);
mikasa_pal_set_paltemp_raw (2, ev5_icsr);
}

static void mikasa_pal_set_vptptr (t_uint64 vptptr)
{
mikasa_pal_vtbr = vptptr & M64;
mikasa_pal_set_paltemp_raw (29, mikasa_pal_vtbr);
}

static void mikasa_pal_write_fen (uint32 fen)
{
if (mikasa_pal_valid_phys (mikasa_pal_pcbb + MIKASA_PCB_FEN,
    sizeof (uint32)))
    WritePL (mikasa_pal_pcbb + MIKASA_PCB_FEN, fen & 1);
}

static void mikasa_pal_set_paltemp_ent (uint32 which, t_uint64 addr)
{
#ifdef KDEBUG
if (which > 6)
#else
if (which > 5)
#endif
    return;
switch (which) {
    case 0:
        mikasa_pal_set_paltemp_raw (ptEntInt, addr);
        break;
    case 1:
        mikasa_pal_set_paltemp_raw (ptEntArith, addr);
        break;
    case 2:
        mikasa_pal_set_paltemp_raw (ptEntMM, addr);
        break;
    case 3:
        mikasa_pal_set_paltemp_raw (ptEntIF, addr);
        break;
    case 4:
        mikasa_pal_set_paltemp_raw (ptEntUna, addr);
        break;
    case 5:
        mikasa_pal_set_paltemp_raw (ptEntSys, addr);
        break;
#ifdef KDEBUG
    case 6:
        mikasa_pal_set_paltemp_raw (ptEntDbg, addr);
        break;
#endif
    }
}

static void mikasa_pal_write_pcbb_flags (t_uint64 mask, t_uint64 bits)
{
t_uint64 flags;

if (!mikasa_pal_valid_phys (mikasa_pal_pcbb + MIKASA_PCBV_FLAGS,
    sizeof (t_uint64)))
    return;
flags = ReadPQ (mikasa_pal_pcbb + MIKASA_PCBV_FLAGS);
flags = (flags & ~mask) | (bits & mask);
WritePQ (mikasa_pal_pcbb + MIKASA_PCBV_FLAGS, flags);
return;
}

static t_stat mikasa_pal_load_pcbb (t_uint64 pcbb)
{
t_uint64 val;
uint32 asn;
t_uint64 fen;

if (pcbb & 0x7F)
    ABORT (EXC_RSVO);
if (!mikasa_pal_valid_phys (pcbb, MIKASA_PCB_MIN_SIZE))
    return STOP_MME;

mikasa_pal_pcbb = pcbb;
SP = mikasa_pal_stkp[MODE_K] = ReadPQ (mikasa_pal_pcbb + MIKASA_PCB_KSP);
mikasa_pal_usp = mikasa_pal_stkp[MODE_U] =
    ReadPQ (mikasa_pal_pcbb + MIKASA_PCB_USP);
val = mikasa_pal_ptbr_from_pfn (ReadPQ (mikasa_pal_pcbb +
    MIKASA_PCB_PTBR));
if (val != mikasa_pal_ptbr)
    tlb_ia (TLB_CI | TLB_CD);
mikasa_pal_ptbr = val;
mikasa_pal_vtbr = (mikasa_pal_vtbr_va () |
    (mikasa_pal_vtbr & MIKASA_PAL_PTBR_PHYS) |
    (mikasa_pal_ptbr & MIKASA_PAL_PTBR_PHYS)) & M64;
mikasa_pal_set_paltemp_raw (28, mikasa_pal_ptbr);
mikasa_pal_set_paltemp_raw (29, mikasa_pal_vtbr);
mikasa_pal_set_paltemp_raw (18, mikasa_pal_usp);
mikasa_pal_set_paltemp_raw (19, mikasa_pal_stkp[MODE_K]);
asn = ReadPL (mikasa_pal_pcbb + MIKASA_PCB_ASN) & M16;
itlb_set_asn (asn);
dtlb_set_asn (asn);
fen = ReadPQ (mikasa_pal_pcbb + MIKASA_PCB_FEN);
fpen = (uint32) fen & 1;
ev4_write_icsr (mikasa_pal_pcbb_icsr (ev5_icsr, asn, fen));
mikasa_pal_set_paltemp_raw (2, ev5_icsr);
pcc_h = (ReadPL (mikasa_pal_pcbb + MIKASA_PCB_PCC) - pcc_l) & M32;
mikasa_pal_thread = ReadPQ (mikasa_pal_pcbb + MIKASA_PCB_UNIQUE);
mikasa_pal_set_paltemp_raw (30, ev5_palbase);
mikasa_pal_set_paltemp_raw (9, mikasa_pal_pack_ps (mikasa_pal_ipl,
    mikasa_pal_current_mode ()));
mikasa_pal_set_paltemp_raw (25, mikasa_pal_mces);
mikasa_pal_set_paltemp_raw (24, mikasa_pal_sysval);
mikasa_pal_set_paltemp_raw (17, mikasa_pal_impure);
mikasa_pal_set_paltemp_raw (2, ev5_icsr);
mikasa_pal_set_paltemp_raw (31, mikasa_pal_pcbb);
return SCPE_OK;
}

static t_stat mikasa_pal_save_pcbb_state (void)
{
t_uint64 pcbb = mikasa_pal_pcbb;
uint32 mode = mikasa_pal_stack_mode (mikasa_pal_current_mode ());

if (!mikasa_pal_valid_phys (pcbb, MIKASA_PCB_MIN_SIZE))
    return STOP_MME;

if (mode == MODE_U) {
    mikasa_pal_usp = SP;
    mikasa_pal_stkp[MODE_U] = SP;
    }
else mikasa_pal_stkp[MODE_K] = SP;
WritePQ (pcbb + MIKASA_PCB_KSP, mikasa_pal_stkp[MODE_K]);
WritePQ (pcbb + MIKASA_PCB_USP, mikasa_pal_usp);
WritePL (pcbb + MIKASA_PCB_PCC, (pcc_h + pcc_l) & M32);
return SCPE_OK;
}

static t_stat mikasa_pal_cflush (void)
{
uint32 i;
uint32 j;
t_uint64 victim_pa;

victim_pa = ((R[16] << VA_N_OFF) & MIKASA_CFLUSH_INDEX_MASK) ^
    (((t_uint64) 1u) << BC_V_SIZE);
if (!mikasa_pal_valid_phys (victim_pa, MIKASA_PAGE_SIZE))
    return STOP_NSPAL;

sim_debug (MIKASA_DBG_PAL, &mikasa_dev,
    "PAL CFLUSH pc=%llX pfn=%llX victim_pa=%llX\n",
    (unsigned long long) PC, (unsigned long long) R[16],
    (unsigned long long) victim_pa);

for (i = 0; i < (MIKASA_PAGE_SIZE / (32 * MIKASA_CFLUSH_LINES_PER_ITER)); i++) {
    t_uint64 base = victim_pa + (i * 32 * MIKASA_CFLUSH_LINES_PER_ITER);

    for (j = 0; j < MIKASA_CFLUSH_LINES_PER_ITER; j++) {
        (void) ReadPQ (base + (t_uint64) (j << 5));
        }
    if (mikasa_pal_live_hirr () & HIRR_M_HWR) {
        mikasa_pal_set_pc ((PC - 4) & M64);
        break;
        }
    }

return SCPE_OK;
}

static t_stat mikasa_pal_draina (void)
{
sim_debug (MIKASA_DBG_PAL, &mikasa_dev,
    "PAL DRAINA pc=%llX\n",
    (unsigned long long) PC);
return SCPE_OK;
}

static void mikasa_pal_sync_abox_crd (void)
{
t_uint64 abox_ctl = ev4_read_abox_ipr (0);

abox_ctl = abox_ctl & ~((t_uint64) ABOX_M_CRD_EN);
if ((mikasa_pal_mces & MIKASA_MCES_DPC) == 0)
    abox_ctl |= (t_uint64) ABOX_M_CRD_EN;
ev4_write_abox_ipr (0, abox_ctl);

mikasa_pal_write_cns_quad (CNS_Q_ABOX_CTL, abox_ctl);
}

static t_stat mikasa_pal_swpctx (void)
{
t_uint64 old_pcbb = mikasa_pal_pcbb;

if (R[16] & 0x7F)
    ABORT (EXC_RSVO);
if (!mikasa_pal_valid_phys (R[16], MIKASA_PCB_MIN_SIZE))
    return STOP_MME;
if (mikasa_pal_save_pcbb_state () != SCPE_OK)
    return STOP_MME;
R[0] = old_pcbb;
return mikasa_pal_load_pcbb (R[16]);
}

static t_stat mikasa_pal_halt (void)
{
t_uint64 old_pc = PC;
t_stat r;

r = mikasa_pal_save_pcbb_state ();
if (r != SCPE_OK)
    return r;
if (mikasa_pal_valid_phys (mikasa_pal_pcbb + MIKASA_PROC_HALT_PCBB,
    sizeof (t_uint64)))
    WritePQ (mikasa_pal_pcbb + MIKASA_PROC_HALT_PCBB, mikasa_pal_pcbb);
if (mikasa_pal_valid_phys (mikasa_pal_pcbb + MIKASA_PROC_HALT_CODE,
    sizeof (t_uint64)))
    WritePQ (mikasa_pal_pcbb + MIKASA_PROC_HALT_CODE, HLT_K_SW_HALT);
mikasa_pal_set_pc ((old_pc - 4) & M64);
sim_debug (MIKASA_DBG_PAL, &mikasa_dev,
    "PAL HALT: pcbb=%llX reason=%u newpc=%llX\n",
    (unsigned long long) mikasa_pal_pcbb,
    HLT_K_SW_HALT,
    (unsigned long long) PC);
return STOP_HALT;
}

static t_stat mikasa_pal_swppal (void)
{
t_stat r;
uint32 new_asn;
t_uint64 new_fen;

sim_debug (MIKASA_DBG_PAL, &mikasa_dev,
    "PAL SWPPAL r16=%llX r17=%llX r18=%llX r19=%llX pc=%llX pal=%u\n",
    (unsigned long long) R[16], (unsigned long long) R[17],
    (unsigned long long) R[18], (unsigned long long) R[19],
    (unsigned long long) PC, pal_mode);

if ((R[16] != 0) && (R[16] <= 255)) {
    R[0] = (R[16] == PAL_UNIX) ? 2 : 1;
    return SCPE_OK;
    }
if (mikasa_srm_os_handoff && !mikasa_direct_apb) {
    mikasa_pal_mark_pal_swapped ();
    mikasa_pal_set_pc (R[16]);
    pal_mode = 1;
    ev4_state.pal_mode = 1;
    return SCPE_OK;
    }
r = mikasa_pal_load_pcbb (R[18]);
if (r != SCPE_OK)
    return r;
new_asn = itlb_asn & ITB_ASN_M_ASN;
new_fen = ReadPQ (mikasa_pal_pcbb + MIKASA_PCB_FEN);
tlb_ia (TLB_CI | TLB_CD | TLB_CA);
mikasa_pal_set_pc (R[17] & ~((t_uint64) 3));
mikasa_pal_set_mode (MODE_K, MPAL_IPL_MASK);
mikasa_pal_mces = 8;
mikasa_pal_sync_abox_crd ();
mikasa_pal_ps = 0;
mikasa_pal_sisr = 0;
ev5_sirr = 0;
mikasa_pal_asten = 0;
mikasa_pal_astsr = 0;
ev5_asten = 0;
ev5_astrr = 0;
dmapen = 1;
mikasa_pal_init_paltemp_regs ();
mikasa_pal_init_ipr_shadows ();
mikasa_pal_set_paltemp_raw (31, mikasa_pal_pcbb);
fpen = (uint32) new_fen & 1;
ev4_write_icsr (mikasa_pal_pcbb_icsr (ev5_icsr, new_asn, new_fen));
mikasa_pal_set_paltemp_raw (2, ev5_icsr);
mikasa_pal_set_vptptr (R[19] |
    (mikasa_pal_ptbr & MIKASA_PAL_PTBR_PHYS));
mikasa_pal_mark_pal_swapped ();
pal_type = PAL_UNIX;
pal_mode = 0;
mikasa_srm_os_handoff = TRUE;
lock_flag = 0;
R[0] = 0;
sim_debug (MIKASA_DBG_PAL, &mikasa_dev,
    "PAL SWPPAL loaded pcbb=%llX ptbr=%llX vtbr=%llX newpc=%llX\n",
    (unsigned long long) mikasa_pal_pcbb,
    (unsigned long long) mikasa_pal_ptbr,
    (unsigned long long) mikasa_pal_vtbr,
    (unsigned long long) PC);
return SCPE_OK;
}

static t_stat mikasa_pal_intexc (uint32 vec, uint32 newmode, uint32 newipl)
{
t_uint64 pa = (mikasa_pal_scbb + vec) & ~((t_uint64) 0xF);
t_uint64 sav_ps = mikasa_pal_read_ps ();
t_uint64 old_sp = SP;
uint32 oldmode = mikasa_pal_current_mode ();
uint32 oldstack = mikasa_pal_stack_mode (oldmode);
uint32 newstack = mikasa_pal_stack_mode (newmode);
uint32 wacc;
uint32 exc;
t_uint64 chkpa;

if (!mikasa_pal_valid_phys (pa, 16))
    return STOP_NSPAL;
if (newmode > MODE_U)
    ABORT (EXC_RSVO);
mikasa_pal_stkp[oldstack] = old_sp;
if (oldstack == MODE_U) {
    mikasa_pal_usp = old_sp;
    mikasa_pal_set_paltemp_raw (18, mikasa_pal_usp);
    }
SP = (mikasa_pal_stkp[newstack] - MIKASA_PAL_STACK_FRAME) & M64;
wacc = ACC_W (newmode);
exc = mikasa_pal_test_va (SP, wacc, &chkpa);
if ((exc != 0) || !mikasa_pal_valid_phys (chkpa, sizeof (t_uint64))) {
    if (newmode == MODE_K)
        return mikasa_pal_mchk_ksp_not_valid ();
    ABORT1 (SP, (exc ? exc : EXC_ACV) + EXC_W);
    }
exc = mikasa_pal_test_va (SP + MIKASA_PAL_STACK_FRAME - 8, wacc, &chkpa);
if ((exc != 0) || !mikasa_pal_valid_phys (chkpa, sizeof (t_uint64))) {
    if (newmode == MODE_K)
        return mikasa_pal_mchk_ksp_not_valid ();
    ABORT1 (SP + MIKASA_PAL_STACK_FRAME - 8,
        (exc ? exc : EXC_ACV) + EXC_W);
    }
tlb_set_cm (newmode);
WriteQ (SP + EV4_OSF_FRM_Q_PS, sav_ps);
WriteQ (SP + EV4_OSF_FRM_Q_PC, PC);
WriteQ (SP + EV4_OSF_FRM_Q_GP, R[29]);
WriteQ (SP + EV4_OSF_FRM_Q_A0, R[16]);
WriteQ (SP + EV4_OSF_FRM_Q_A1, R[17]);
WriteQ (SP + EV4_OSF_FRM_Q_A2, R[18]);
PCQ_ENTRY;
PC = ReadPQ (pa) & M64;
pc_align = (uint32) (PC & 3);
R[2] = PC;
R[3] = ReadPQ (pa + 8);
mikasa_pal_set_mode (newmode, newipl & MIKASA_PSV_M_IPL);
mikasa_pal_ps = mikasa_pal_ps & ~MIKASA_PSV_M_SW;
pal_mode = 0;
lock_flag = 0;
return SCPE_OK;
}

static t_stat mikasa_pal_ent_dispatch_frame_pc (t_uint64 entry, uint32 newmode,
    uint32 newipl, t_uint64 frame_pc)
{
t_uint64 sav_ps = mikasa_pal_read_ps ();
t_uint64 old_sp = SP;
uint32 oldmode = mikasa_pal_current_mode ();
uint32 oldstack = mikasa_pal_stack_mode (oldmode);
uint32 newstack = mikasa_pal_stack_mode (newmode);
uint32 exc;
t_uint64 chkpa;

if (entry == 0)
    return STOP_NSPAL;
if (newmode > MODE_U)
    ABORT (EXC_RSVO);
mikasa_pal_stkp[oldstack] = old_sp;
if (oldstack == MODE_U) {
    mikasa_pal_usp = old_sp;
    mikasa_pal_set_paltemp_raw (18, mikasa_pal_usp);
    }
SP = (mikasa_pal_stkp[newstack] - MIKASA_PAL_STACK_FRAME) & M64;
exc = mikasa_pal_test_va (SP, ACC_W (newmode), &chkpa);
if ((exc != 0) || !mikasa_pal_valid_phys (chkpa, sizeof (t_uint64))) {
    if (newmode == MODE_K)
        return mikasa_pal_mchk_ksp_not_valid ();
    ABORT1 (SP, (exc ? exc : EXC_ACV) + EXC_W);
    }
exc = mikasa_pal_test_va (SP + MIKASA_PAL_STACK_FRAME - 8,
    ACC_W (newmode), &chkpa);
if ((exc != 0) || !mikasa_pal_valid_phys (chkpa, sizeof (t_uint64))) {
    if (newmode == MODE_K)
        return mikasa_pal_mchk_ksp_not_valid ();
    ABORT1 (SP + MIKASA_PAL_STACK_FRAME - 8,
        (exc ? exc : EXC_ACV) + EXC_W);
    }
tlb_set_cm (newmode);
WriteQ (SP + EV4_OSF_FRM_Q_PS, sav_ps);
WriteQ (SP + EV4_OSF_FRM_Q_PC, frame_pc & M64);
WriteQ (SP + EV4_OSF_FRM_Q_GP, R[29]);
WriteQ (SP + EV4_OSF_FRM_Q_A0, R[16]);
WriteQ (SP + EV4_OSF_FRM_Q_A1, R[17]);
WriteQ (SP + EV4_OSF_FRM_Q_A2, R[18]);
R[29] = ev4_state.paltemp_raw[20];                       /* ptKgp */
mikasa_pal_set_pc (entry & ~((t_uint64) 3));
mikasa_pal_set_mode (newmode, newipl);
mikasa_pal_ps = mikasa_pal_ps & ~MIKASA_PSV_M_SW;
pal_mode = 0;
lock_flag = 0;
return SCPE_OK;
}

static t_stat mikasa_pal_ent_dispatch (t_uint64 entry, uint32 newmode,
    uint32 newipl)
{
return mikasa_pal_ent_dispatch_frame_pc (entry, newmode, newipl, PC);
}

static t_stat mikasa_pal_inst_fault (uint32 code)
{
t_stat r = mikasa_pal_ent_dispatch (ev4_state.paltemp_raw[ptEntIF],
    MODE_K, mikasa_pal_ipl);

if (r == SCPE_OK)
    R[16] = code;
return r;
}

static t_stat mikasa_pal_callsys (void)
{
if (mikasa_pal_current_mode () == MODE_K)
    return mikasa_pal_mchk_callsys ();
return mikasa_pal_ent_dispatch (ev4_state.paltemp_raw[ptEntSys],
    MODE_K, mikasa_pal_ipl);
}

static t_stat mikasa_pal_mm_intexc (t_uint64 va, t_uint64 code,
    t_uint64 ref)
{
t_stat r;

r = mikasa_pal_ent_dispatch (ev4_state.paltemp_raw[ptEntMM],
    MODE_K, mikasa_pal_ipl);
if (r != SCPE_OK)
    return r;
mikasa_pal_set_mm_args (va, code, ref);
tlb_is (va, TLB_CI | TLB_CD);
return r;
}

static t_stat mikasa_pal_rei (void)
{
t_uint64 ps;
t_uint64 pc;
t_uint64 gp;
t_uint64 a0;
t_uint64 a1;
t_uint64 a2;
uint32 oldmode = mikasa_pal_current_mode ();
uint32 oldstack = mikasa_pal_stack_mode (oldmode);
uint32 newmode;
uint32 newstack;
uint32 exc;
t_uint64 chkpa;
uint32 ipl;

if (oldmode == MODE_K) {
    exc = mikasa_pal_test_va (SP, cm_racc, &chkpa);
    if ((exc != 0) || !mikasa_pal_valid_phys (chkpa, sizeof (t_uint64)))
        return mikasa_pal_mchk_ksp_not_valid ();
    exc = mikasa_pal_test_va (SP + MIKASA_PAL_STACK_FRAME - 8, cm_racc,
        &chkpa);
    if ((exc != 0) || !mikasa_pal_valid_phys (chkpa, sizeof (t_uint64)))
        return mikasa_pal_mchk_ksp_not_valid ();
    }
ps = ReadQ (SP + EV4_OSF_FRM_Q_PS);
pc = ReadQ (SP + EV4_OSF_FRM_Q_PC);
gp = ReadQ (SP + EV4_OSF_FRM_Q_GP);
a0 = ReadQ (SP + EV4_OSF_FRM_Q_A0);
a1 = ReadQ (SP + EV4_OSF_FRM_Q_A1);
a2 = ReadQ (SP + EV4_OSF_FRM_Q_A2);
newmode = mikasa_pal_ps_mode (ps);
newstack = mikasa_pal_stack_mode (newmode);
ipl = mikasa_pal_ps_ipl (ps);
if ((oldmode != MODE_K) && ((newmode < oldmode) || (ps & MIKASA_PSV_MBZ)))
    ABORT (EXC_RSVO);
SP = (SP + MIKASA_PAL_STACK_FRAME) & M64;
mikasa_pal_stkp[oldstack] = SP;
if (oldstack == MODE_U) {
    mikasa_pal_usp = SP;
    mikasa_pal_set_paltemp_raw (18, mikasa_pal_usp);
    }
else if (oldstack == MODE_K)
    mikasa_pal_set_paltemp_raw (19, mikasa_pal_stkp[MODE_K]);
SP = mikasa_pal_stkp[newstack];
R[29] = gp;
R[16] = a0;
R[17] = a1;
R[18] = a2;
mikasa_pal_set_pc (pc & ~((t_uint64) 3));
mikasa_pal_set_mode (newmode, ipl);
mikasa_pal_ps = ((uint32) (mikasa_pal_pack_ps (ipl, newmode) &
    MIKASA_PSV_MASK));
pal_mode = 0;
lock_flag = 0;
return SCPE_OK;
}

static t_stat mikasa_pal_retsys (void)
{
t_uint64 pc;
t_uint64 gp;
uint32 oldmode = mikasa_pal_current_mode ();
uint32 oldstack = mikasa_pal_stack_mode (oldmode);
t_uint64 chkpa;
uint32 exc;

exc = mikasa_pal_test_va (SP, cm_racc, &chkpa);
if ((exc != 0) || !mikasa_pal_valid_phys (chkpa, sizeof (t_uint64)))
    return mikasa_pal_mchk_ksp_not_valid ();
exc = mikasa_pal_test_va (SP + MIKASA_PAL_STACK_FRAME - 8, cm_racc, &chkpa);
if ((exc != 0) || !mikasa_pal_valid_phys (chkpa, sizeof (t_uint64)))
    return mikasa_pal_mchk_ksp_not_valid ();

pc = ReadQ (SP + EV4_OSF_FRM_Q_PC);
gp = ReadQ (SP + EV4_OSF_FRM_Q_GP);

SP = (SP + MIKASA_PAL_STACK_FRAME) & M64;
mikasa_pal_stkp[oldstack] = SP;
mikasa_pal_stkp[MODE_K] = SP;
mikasa_pal_set_paltemp_raw (19, mikasa_pal_stkp[MODE_K]);
SP = mikasa_pal_stkp[MODE_U];
R[29] = gp;
mikasa_pal_set_pc (pc & ~((t_uint64) 3));
mikasa_pal_set_mode (MODE_U, 0);
mikasa_pal_ps = ((uint32) (mikasa_pal_pack_ps (0, MODE_U) & MIKASA_PSV_MASK));
pal_mode = 0;
lock_flag = 0;
return SCPE_OK;
}

static t_stat mikasa_pal_rti (void)
{
t_uint64 ps;
t_uint64 pc;
t_uint64 gp;
t_uint64 a0;
t_uint64 a1;
t_uint64 a2;
uint32 oldmode = mikasa_pal_current_mode ();
uint32 oldstack = mikasa_pal_stack_mode (oldmode);
uint32 newmode;
uint32 newstack;
uint32 ipl;
uint32 exc;
t_uint64 chkpa;

exc = mikasa_pal_test_va (SP, cm_racc, &chkpa);
if ((exc != 0) || !mikasa_pal_valid_phys (chkpa, sizeof (t_uint64)))
    return mikasa_pal_mchk_ksp_not_valid ();
exc = mikasa_pal_test_va (SP + MIKASA_PAL_STACK_FRAME - 8, cm_racc, &chkpa);
if ((exc != 0) || !mikasa_pal_valid_phys (chkpa, sizeof (t_uint64)))
    return mikasa_pal_mchk_ksp_not_valid ();

ps = ReadQ (SP + EV4_OSF_FRM_Q_PS);
pc = ReadQ (SP + EV4_OSF_FRM_Q_PC);
gp = ReadQ (SP + EV4_OSF_FRM_Q_GP);
a0 = ReadQ (SP + EV4_OSF_FRM_Q_A0);
a1 = ReadQ (SP + EV4_OSF_FRM_Q_A1);
a2 = ReadQ (SP + EV4_OSF_FRM_Q_A2);
newmode = mikasa_pal_ps_mode (ps);
newstack = mikasa_pal_stack_mode (newmode);
ipl = mikasa_pal_ps_ipl (ps);
if (newstack == MODE_U)
    ipl = 0;
if ((oldmode != MODE_K) && ((newmode < oldmode) || (ps & MIKASA_PSV_MBZ)))
    ABORT (EXC_RSVO);

SP = (SP + MIKASA_PAL_STACK_FRAME) & M64;
mikasa_pal_stkp[oldstack] = SP;
if (oldstack == MODE_U) {
    mikasa_pal_usp = SP;
    mikasa_pal_set_paltemp_raw (18, mikasa_pal_usp);
    }
else if (oldstack == MODE_K)
    mikasa_pal_set_paltemp_raw (19, mikasa_pal_stkp[MODE_K]);
SP = mikasa_pal_stkp[newstack];

R[29] = gp;
R[16] = a0;
R[17] = a1;
R[18] = a2;
mikasa_pal_set_pc (pc & ~((t_uint64) 3));
mikasa_pal_ps = ((uint32) (mikasa_pal_pack_ps (ipl, newmode) &
    MIKASA_PSV_MASK));
mikasa_pal_set_mode (newmode, ipl);
pal_mode = 0;
lock_flag = 0;
return SCPE_OK;
}

static t_uint64 mikasa_pal_whami (void)
{
return mikasa_pal_whami_reg;
}

static t_stat mikasa_pal_cserve_start (void)
{
t_uint64 impure = R[17];
t_uint64 cpu_blk = R[3];
t_uint64 hwrpb_pa;
t_uint64 entry_pc;
t_uint64 palbase;
t_uint64 val;

if (!mikasa_pal_valid_phys (cpu_blk + MIKASA_SRM_BOOT_PALBASE,
    sizeof (t_uint64)))
    return SCPE_NOFNC;
mikasa_set_cpu_info ();

if (mikasa_pal_valid_phys (impure + 0x10, sizeof (t_uint64))) {
    val = ReadPQ (impure + 0x10);
    if (mikasa_pal_valid_phys (val, 1))
        mikasa_pal_prbr = val;
    }

hwrpb_pa = mikasa_find_hwrpb_pa ();
entry_pc = ReadPQ (cpu_blk + MIKASA_SRM_BOOT_PC);
palbase = ReadPQ (cpu_blk + MIKASA_SRM_BOOT_PALBASE);
if ((entry_pc == 0) || (entry_pc == M64))
    entry_pc = MIKASA_BOOT_IMAGE_VA;
if (palbase != 0)
    ev5_palbase = palbase;

mikasa_direct_apb = FALSE;
mikasa_srm_bootstrap_pal = TRUE;
mikasa_srm_os_handoff = FALSE;
mikasa_pal_impure = impure;
mikasa_hwrpb_pa = hwrpb_pa;
mikasa_boot_l1_pt_pa = ReadPQ (cpu_blk + MIKASA_SRM_BOOT_PTBR);
if ((mikasa_boot_l1_pt_pa == 0) ||
    !mikasa_pal_valid_phys (mikasa_boot_l1_pt_pa, MIKASA_PAGE_SIZE))
    mikasa_boot_l1_pt_pa = MIKASA_L1_PT_PA;
if (mikasa_pal_valid_phys (hwrpb_pa + 0x78, sizeof (t_uint64))) {
    mikasa_boot_vptb_va = ReadPQ (hwrpb_pa + 0x78);
    if (mikasa_boot_vptb_va == 0)
        mikasa_boot_vptb_va = MIKASA_PT_SPACE_VA;
    }
else mikasa_boot_vptb_va = MIKASA_PT_SPACE_VA;
mikasa_boot_image_pa = MIKASA_BOOT_IMAGE_PA;
if ((entry_pc >= MIKASA_BOOT_IMAGE_VA) &&
    (entry_pc < (MIKASA_BOOT_IMAGE_VA + MIKASA_BOOT_RESERVED_SIZE)) &&
    mikasa_boot_pt_walk (entry_pc, &val)) {
    t_uint64 entry_off = entry_pc - MIKASA_BOOT_IMAGE_VA;

    if ((val >= entry_off) &&
        ADDR_IS_MEM ((val - entry_off) + MIKASA_BOOT_RESERVED_SIZE - 1))
        mikasa_boot_image_pa = (val - entry_off) & ~((t_uint64) VA_M_OFF);
    }
if (mikasa_pal_valid_phys (cpu_blk + MIKASA_CNS_PCBB, sizeof (t_uint64))) {
    val = ReadPQ (cpu_blk + MIKASA_CNS_PCBB);
    if (mikasa_pal_valid_phys (val, MIKASA_PCB_MIN_SIZE))
        mikasa_pal_pcbb = val;
    }
if (mikasa_pal_valid_phys (cpu_blk + MIKASA_CNS_PRBR, sizeof (t_uint64))) {
    val = ReadPQ (cpu_blk + MIKASA_CNS_PRBR);
    if (mikasa_pal_valid_phys (val, 1))
        mikasa_pal_prbr = val;
    }
if (mikasa_pal_valid_phys (cpu_blk + MIKASA_CNS_SCBB, sizeof (t_uint64))) {
    val = ReadPQ (cpu_blk + MIKASA_CNS_SCBB);
    mikasa_pal_scbb = val << VA_N_OFF;
    }
mikasa_pal_ptbr = mikasa_boot_l1_pt_pa;
mikasa_pal_vtbr = mikasa_boot_vptb_va;
mikasa_pal_virbnd = M64;
mikasa_pal_sysptbr = 0;
mikasa_pal_thread = 0;
mikasa_pal_esp = 0;
mikasa_pal_ssp = 0;
mikasa_pal_usp = 0;
mikasa_pal_set_mode (MODE_K, MPAL_IPL_MASK);
mikasa_pal_sisr = 0;
ev5_sirr = 0;
mikasa_pal_asten = 0;
mikasa_pal_astsr = 0;
ev5_asten = 0;
ev5_astrr = 0;
mikasa_pal_mces = 8;
mikasa_pal_sync_abox_crd ();
mikasa_pal_ps = 0;
mikasa_pal_datfx = 0;
mikasa_pal_last_pcc = pcc_l;
mikasa_apb_dma_valid = FALSE;
pal_type = PAL_UNIX;
pal_mode = 0;
dmapen = 1;
fpen = 1;
lock_flag = 0;
tlb_ia (TLB_CI | TLB_CD | TLB_CA);
mikasa_write_console_crb (mikasa_hwrpb_pa);
mikasa_load_tlb_range (MIKASA_HWRPB_VA, mikasa_hwrpb_pa, MIKASA_TLB_SPAN,
    PTE_KRE | PTE_KWE);
mikasa_load_tlb_range (MIKASA_BOOT_IMAGE_VA, mikasa_boot_image_pa,
    MIKASA_BOOT_RESERVED_SIZE, PTE_KRE | PTE_KWE);
R[0] = 0;                                               /* boot device type */
R[1] = 0;                                               /* nexus/adapter */
R[2] = 0;                                               /* controller/unit */
R[3] = 0;                                               /* unit */
R[4] = 0;
R[5] = mikasa_last_osflags;
R[16] = mikasa_hwrpb_pa;
R[17] = 0;
R[18] = 0;
R[19] = 0;
R[20] = 0;
R[21] = 0;
R[26] = 0;
R[28] = 0;
R[29] = 0;
if (mikasa_pal_valid_phys (cpu_blk + MIKASA_CNS_R30, sizeof (t_uint64))) {
    val = ReadPQ (cpu_blk + MIKASA_CNS_R30);
    if (val != 0)
        R[30] = val;
    }
if (R[30] == 0)
    R[30] = MIKASA_BOOT_STACK_TOP;
SP = mikasa_pal_stkp[MODE_K] = R[30];
mikasa_pal_set_pc (entry_pc);
R[27] = entry_pc;
mikasa_pal_init_paltemp_regs ();
mikasa_pal_init_ipr_shadows ();
sim_debug (MIKASA_DBG_PAL, &mikasa_dev,
    "PAL CSERVE START handoff cpu_blk=%llX hwrpb=%llX ptbr=%llX "
    "vptb=%llX bootpa=%llX pc=%llX sp=%llX pcbb=%llX scbb=%llX\n",
    (unsigned long long) cpu_blk, (unsigned long long) mikasa_hwrpb_pa,
    (unsigned long long) mikasa_boot_l1_pt_pa,
    (unsigned long long) mikasa_boot_vptb_va,
    (unsigned long long) mikasa_boot_image_pa,
    (unsigned long long) PC, (unsigned long long) R[30],
    (unsigned long long) mikasa_pal_pcbb,
    (unsigned long long) mikasa_pal_scbb);
return SCPE_OK;
}

static t_stat mikasa_pal_cserve_platform (void)
{
switch ((uint32) R[18]) {

    case MIKASA_PCSERVE_LDQP:
        if (R[16] & 7)
            ABORT1 (R[16], EXC_ALIGN);
        R[0] = ReadPQ (R[16]);
        break;

    case MIKASA_PCSERVE_STQP:
        if (R[16] & 7)
            ABORT1 (R[16], EXC_ALIGN);
        WritePQ (R[16], R[17]);
        R[0] = 1;
        break;

    case MIKASA_CSERVE_RD_ABOX:
        R[0] = mikasa_pal_read_cns_quad (CNS_Q_ABOX_CTL,
            ev4_read_abox_ipr (0));
        break;

    case MIKASA_CSERVE_RD_ICCSR:
        R[0] = ev4_state.paltemp_raw[2];
        break;

    case MIKASA_CSERVE_RD_BIU:
        R[0] = mikasa_pal_read_cns_quad (CNS_Q_BIU_CTL,
            ev4_read_abox_ipr (18));
        break;

    case MIKASA_CSERVE_WR_ABOX:
        ev4_write_abox_ipr (0, R[16]);
        mikasa_pal_write_cns_quad (CNS_Q_ABOX_CTL, R[16]);
        R[0] = 1;
        break;

    case MIKASA_CSERVE_WR_ICCSR:
        ev4_write_icsr (R[16]);
        mikasa_pal_set_paltemp_raw (2, ev5_icsr);
        R[0] = 1;
        break;

    case MIKASA_CSERVE_WR_BIU:
        ev4_write_abox_ipr (18, R[16] | BIU_M_OE);
        mikasa_pal_write_cns_quad (CNS_Q_BIU_CTL, R[16] | BIU_M_OE);
        R[0] = 1;
        break;

    case MIKASA_PCSERVE_JTOPAL:
    {
        t_uint64 jmp_pc = (R[16] & ~((t_uint64) 3)) | 1;
        mikasa_pal_load_jtopal_params ();
        mikasa_pal_set_paltemp_whami (0);
        mikasa_pal_set_pc (jmp_pc);
        pal_mode = 1;
        ev4_state.pal_mode = 1;
        tlb_ia (TLB_CI | TLB_CA);
        R[0] = 1;
    }
        break;

    case MIKASA_CSERVE_WR_INT:
        mikasa_pal_set_ptintmask (R[16]);
        R[0] = 1;
        break;

    case MIKASA_PCSERVE_RD_IMPURE:
        R[0] = mikasa_pal_impure;
        break;

    case MIKASA_CSERVE_PUTC:
        (void) sim_putchar ((int) (R[16] & 0xFF));
        R[0] = 1;
        break;

    default:
        R[0] = 0;
        return SCPE_NOFNC;
        }

return SCPE_OK;
}

static t_stat mikasa_pal_cserve (void)
{
t_stat r;

sim_debug (MIKASA_DBG_PAL, &mikasa_dev,
    "PAL CSERVE a0=%llX a1=%llX a2=%02X pc=%llX pal=%u bootpal=%u "
    "oshandoff=%u\n",
    (unsigned long long) R[16], (unsigned long long) R[17],
    (uint32) R[18], (unsigned long long) PC, pal_mode,
    mikasa_srm_bootstrap_pal, mikasa_srm_os_handoff);

if (mikasa_srm_os_handoff && !mikasa_direct_apb) {
    r = mikasa_pal_cserve_platform ();
    return (r == SCPE_NOFNC) ? SCPE_OK : r;
    }

switch ((uint32) R[16]) {

    case MIKASA_CSERVE_LDQP:
        if (R[17] & 7)
            ABORT1 (R[17], EXC_ALIGN);
        R[0] = ReadPQ (R[17]);
        break;

    case MIKASA_CSERVE_STQP:
        if (R[17] & 7)
            ABORT1 (R[17], EXC_ALIGN);
        WritePQ (R[17], R[18]);
        R[0] = 0;
        break;

    case MIKASA_CSERVE_RD_ABOX:
        R[0] = ev4_read_abox_ipr (((uint32) R[17]) & 0x1F);
        break;

    case MIKASA_CSERVE_RD_ICCSR:
        R[0] = ev4_state.paltemp_raw[2];
        break;

    case MIKASA_CSERVE_RD_BIU:
        /* Minimal BIU view: status/control via ABOX BIU IPRs. */
        if ((((uint32) R[17]) & 1u) == 0)
            R[0] = ev4_read_abox_ipr (10);             /* BIU_STAT */
        else
            R[0] = ev4_read_abox_ipr (18);             /* BIU_CTL */
        break;

    case MIKASA_CSERVE_WR_ABOX:
        ev4_write_abox_ipr ((((uint32) R[17]) & 0x1F), R[18]);
        R[0] = 0;
        break;

    case MIKASA_CSERVE_WR_ICCSR:
        ev4_write_icsr (R[17]);
        mikasa_pal_set_paltemp_raw (2, ev5_icsr);
        R[0] = 0;
        break;

    case MIKASA_CSERVE_WR_BIU:
        /* Minimal BIU write path: control register (includes OE bit). */
        ev4_write_abox_ipr (18, R[18] | BIU_M_OE);     /* BIU_CTL */
        R[0] = 0;
        break;

    case MIKASA_CSERVE_JTOPAL:
    {
        t_uint64 jmp_pc = (R[17] & ~((t_uint64) 3)) | 1;
        mikasa_pal_load_jtopal_params ();
        mikasa_pal_set_paltemp_whami (0);
        mikasa_pal_set_pc (jmp_pc);
        pal_mode = 1;
        ev4_state.pal_mode = 1;
        tlb_ia (TLB_CI | TLB_CA);
    }
        break;

    case MIKASA_CSERVE_RDIO:
        R[0] = ReadPL (R[17]);
        break;

    case MIKASA_CSERVE_WRIO:
        WritePL (R[17], R[18]);
        break;

    case MIKASA_CSERVE_RDWHAMI:
        R[0] = mikasa_pal_whami ();
        break;

    case MIKASA_CSERVE_START:
        return mikasa_pal_cserve_start ();

    case MIKASA_CSERVE_JUMP:
        mikasa_pal_set_pc (R[17]);
        pal_mode = (uint32) R[17] & 1;
        sim_debug (MIKASA_DBG_PAL, &mikasa_dev,
            "PAL CSERVE JUMP newpc=%llX pal=%u\n",
            (unsigned long long) PC, pal_mode);
        break;

    case MIKASA_CSERVE_HALT:
        sim_debug (MIKASA_DBG_PAL, &mikasa_dev,
            "PAL CSERVE HALT pc=%llX\n",
            (unsigned long long) PC);
        return mikasa_pal_halt ();

    case MIKASA_CSERVE_WRIPIR:
    case MIKASA_CSERVE_WRMCES:
    case MIKASA_CSERVE_SWPPAL:
    case MIKASA_CSERVE_IMB:
        R[0] = 0;
        break;

    case MIKASA_CSERVE_WR_INT:
        mikasa_pal_set_ptintmask (R[17]);
        R[0] = 0;
        break;

    case MIKASA_CSERVE_RD_IMPURE:
        R[0] = mikasa_pal_impure;
        break;

    case MIKASA_CSERVE_PUTC:
        (void) sim_putchar ((int) (R[17] & 0xFF));
        R[0] = 0;
        break;

    default:
        return mikasa_pal_cserve_platform ();
        }

return SCPE_OK;
}

static t_bool mikasa_pal_osf_callpal_valid (uint32 fnc)
{
switch (fnc) {
    case MPAL_HALT:
    case MPAL_CFLUSH:
    case MPAL_DRAINA:
    case MPAL_CSERVE:
    case MPAL_SWPPAL:
    case MPAL_MT_IPIR:
    case MPAL_MF_MCES:
    case MPAL_MT_MCES:
    case MPAL_WRFEN:
    case MPAL_WRVPTPTR:
    case MPAL_SWPCTX:
    case MPAL_WRVAL:
    case MPAL_RDVAL:
    case MPAL_TBI:
    case MPAL_WRENT:
    case MPAL_SWPIPL:
    case MPAL_RDPS:
    case MPAL_WRKGP:
    case MPAL_WRUSP:
    case MPAL_MT_PERFMON:
    case MPAL_RDUSP:
    case MPAL_MF_WHAMI:
    case MPAL_RETSYS:
    case MPAL_RTI:
    case MPAL_BPT:
    case MPAL_BUGCHK:
    case MPAL_CALLSYS:
    case MPAL_IMB:
    case MPAL_RD_UNQ:
    case MPAL_WR_UNQ:
    case MPAL_GENTRAP:
        return TRUE;
    default:
        return FALSE;
        }
}

t_stat mikasa_pal_proc_inst (uint32 fnc)
{
uint32 arg32 = (uint32) R[16];

if (!mikasa_direct_apb && !mikasa_srm_bootstrap_pal &&
    !mikasa_srm_os_handoff && (fnc != MPAL_CSERVE))
    return SCPE_NOFNC;
if (mikasa_srm_os_handoff && !mikasa_direct_apb &&
    !mikasa_pal_osf_callpal_valid (fnc))
    return mikasa_pal_inst_fault (MIKASA_OSF_IF_K_OPCDEC);
if (mikasa_pal_route_os_events () && (fnc < 0x40) &&
    (mikasa_pal_current_mode () != MODE_K))
    ABORT (EXC_RSVI);

switch (fnc) {

    case MPAL_HALT:
        sim_debug (MIKASA_DBG_PAL, &mikasa_dev,
            "PAL HALT pc=%llX bootpal=%u direct=%u oshandoff=%u\n",
            (unsigned long long) PC, mikasa_srm_bootstrap_pal,
            mikasa_direct_apb, mikasa_srm_os_handoff);
        if (mikasa_srm_bootstrap_pal && !mikasa_direct_apb)
            return SCPE_NOFNC;
        return mikasa_pal_halt ();

    case MPAL_DRAINA:
        return mikasa_pal_draina ();
    case MPAL_CFLUSH:
        return mikasa_pal_cflush ();
    case MPAL_MT_IPIR:
        break;

    case MPAL_IMB:
        tlb_ia (TLB_CI | TLB_CA);
        break;

    case MPAL_MT_PERFMON:
        R[0] = 0;
        switch (arg32) {
        case 0:
            mikasa_pal_set_paltemp_raw (22, ev4_state.paltemp_raw[22] &
                ~MIKASA_INT_K_ZAP_PC);
            R[0] = 1;
            break;
        case 1:
            mikasa_pal_set_paltemp_raw (22, ev4_state.paltemp_raw[22] |
                MIKASA_INT_K_ZAP_PC);
            R[0] = 1;
            break;
        case 2:
            ev4_write_icsr ((ev5_icsr & ~MIKASA_ICCSR_W_MUX_PC) |
                (R[17] & MIKASA_ICCSR_W_MUX_PC));
            mikasa_pal_set_paltemp_raw (2, ev5_icsr);
            R[0] = 1;
            break;
        case 3:
            ev4_write_icsr ((ev5_icsr & ~MIKASA_ICCSR_W_PME) |
                ((R[17] & 3) << 44));
            mikasa_pal_set_paltemp_raw (2, ev5_icsr);
            R[0] = 1;
            break;
            }
        break;

    case MPAL_LDQP:
        if (R[16] & 7)
            ABORT1 (R[16], EXC_ALIGN);
        R[0] = ReadPQ (R[16]);
        break;

    case MPAL_STQP:
        if (R[16] & 7)
            ABORT1 (R[16], EXC_ALIGN);
        WritePQ (R[16], R[17]);
        break;

    case MPAL_SWPCTX:
        return mikasa_pal_swpctx ();

    case MPAL_MF_ASN:
        R[0] = itlb_asn & ITB_ASN_M_ASN;
        break;

    case MPAL_MT_ASTEN:
        R[0] = mikasa_pal_asten & MPAL_AST_MASK;
        mikasa_pal_asten = ((mikasa_pal_asten & arg32) | (arg32 >> 4)) &
            MPAL_AST_MASK;
        ev5_asten = mikasa_pal_asten;
        break;

    case MPAL_MT_ASTSR:
        R[0] = mikasa_pal_astsr & MPAL_AST_MASK;
        mikasa_pal_astsr = ((mikasa_pal_astsr & arg32) | (arg32 >> 4)) &
            MPAL_AST_MASK;
        ev5_astrr = mikasa_pal_astsr;
        break;

    case MPAL_CSERVE:
        return mikasa_pal_cserve ();

    case MPAL_SWPPAL:
        return mikasa_pal_swppal ();

    case MPAL_MF_FEN:
        R[0] = fpen & 1;
        break;

    case MPAL_MT_FEN:
        fpen = arg32 & 1;
        mikasa_pal_write_fen (fpen);
        mikasa_pal_sync_paltemp_icsr ();
        break;

    case MPAL_WRFEN:
        fpen = arg32 & 1;
        mikasa_pal_write_fen (fpen);
        mikasa_pal_sync_paltemp_icsr ();
        break;

    case MPAL_WRVPTPTR:
        mikasa_pal_set_vptptr (R[16]);
        break;

    case MPAL_WRVAL:
        mikasa_pal_sysval = R[16];
        mikasa_pal_set_paltemp_raw (24, mikasa_pal_sysval);
        break;

    case MPAL_RDVAL:
        R[0] = mikasa_pal_sysval;
        break;

    case MPAL_TBI:
        switch ((int32) R[16]) {
        case -2:                                        /* tbia */
            tlb_ia (TLB_CI | TLB_CD | TLB_CA);
            break;
        case -1:                                        /* tbiap */
            tlb_ia (TLB_CI | TLB_CD);
            break;
        case 1:                                         /* tbisi */
            tlb_is (R[17], TLB_CI | TLB_CA);
            break;
        case 2:                                         /* tbisd */
            tlb_is (R[17], TLB_CD | TLB_CA);
            break;
        case 3:                                         /* tbis */
            tlb_is (R[17], TLB_CI | TLB_CD | TLB_CA);
            break;
        default:
            break;
            }
        break;

    case MPAL_WRENT:
        mikasa_pal_set_paltemp_ent ((uint32) R[17],
            R[16] & ~((t_uint64) 3));
        break;

    case MPAL_MF_IPL:
        R[0] = ev4_state.ipl & 0x7;
        break;

    case MPAL_MT_IPL:
        R[0] = ev4_state.ipl & 0x7;
        ev4_state.ipl = arg32 & 0x7;
        mikasa_pal_ipl = ev4_state.ipl;
        ev5_ipl = mikasa_pal_ipl;
        mikasa_pal_set_hier_from_ipl (mikasa_pal_ipl);
        break;

    case MPAL_MF_MCES:
        R[0] = mikasa_pal_mces & 0x1F;
        break;

    case MPAL_MT_MCES:
        mikasa_pal_mces = (mikasa_pal_mces &
            ~((arg32 & MCES_W1C) | MCES_DIS)) | (arg32 & MCES_DIS);
        mikasa_pal_set_paltemp_raw (25, mikasa_pal_mces);
        mikasa_pal_sync_abox_crd ();
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
        R[0] = mikasa_pal_ptbr_pfn ();
        break;

    case MPAL_MF_SCBB:
        R[0] = mikasa_pal_scbb >> VA_N_OFF;
        break;

    case MPAL_MT_SCBB:
        mikasa_pal_scbb = (R[16] & M32) << VA_N_OFF;
        break;

    case MPAL_MT_SIRR:
        mikasa_pal_sisr = (mikasa_pal_sisr | (1u << (arg32 & 0xF))) &
            MPAL_SISR_MASK;
        ev5_sirr = mikasa_pal_sisr & MPAL_SISR_MASK;
        break;

    case MPAL_MF_SISR:
        mikasa_pal_sisr = ev5_sirr & MPAL_SISR_MASK;
        R[0] = mikasa_pal_sisr;
        break;

    case MPAL_MF_TBCHK:
        R[0] = mikasa_pal_tlb_check (R[16]) ? (Q_SIGN + 1) : Q_SIGN;
        break;

    case MPAL_MT_TBIA:
        tlb_ia (TLB_CI | TLB_CD | TLB_CA);
        break;

    case MPAL_MT_TBIAP:
        tlb_ia (TLB_CI | TLB_CD);
        break;

    case MPAL_MT_TBIS:
        tlb_is (R[16], TLB_CI | TLB_CD | TLB_CA);
        break;

    case MPAL_MT_TBISD:
        tlb_is (R[16], TLB_CD | TLB_CA);
        break;

    case MPAL_MT_TBISI:
        tlb_is (R[16], TLB_CI | TLB_CA);
        break;

    case MPAL_MF_ESP:
        R[0] = mikasa_pal_esp;
        break;

    case MPAL_MF_SSP:
        R[0] = mikasa_pal_ssp;
        break;

    case MPAL_MF_USP:
        R[0] = mikasa_pal_usp;
        break;

    case MPAL_MT_ESP:
        mikasa_pal_esp = R[16];
        mikasa_pal_stkp[MODE_E] = mikasa_pal_esp;
        break;

    case MPAL_MT_SSP:
        mikasa_pal_ssp = R[16];
        mikasa_pal_stkp[MODE_S] = mikasa_pal_ssp;
        break;

    case MPAL_MT_USP:
        mikasa_pal_usp = R[16];
        mikasa_pal_stkp[MODE_U] = mikasa_pal_usp;
        mikasa_pal_set_paltemp_raw (18, mikasa_pal_usp);
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
        if (mikasa_pal_vtbr != R[16])
            tlb_ia (TLB_CI | TLB_CD);
        mikasa_pal_vtbr = R[16];
        break;

    case MPAL_MT_DATFX:
        mikasa_pal_datfx = arg32 & 1;
        mikasa_pal_write_pcbb_flags (Q_SIGN,
            mikasa_pal_datfx ? Q_SIGN : 0);
        break;

    case MIKASA_MPAL_MF_VIRBND:
        R[0] = mikasa_pal_virbnd;
        break;

    case MIKASA_MPAL_MT_VIRBND:
        mikasa_pal_virbnd = R[16];
        break;

    case MIKASA_MPAL_MF_SYSPTBR:
        R[0] = mikasa_pal_sysptbr;
        break;

    case MIKASA_MPAL_MT_SYSPTBR:
        mikasa_pal_sysptbr = R[16];
        break;

    case MPAL_MF_WHAMI:
        R[0] = mikasa_pal_whami ();
        break;

    case MPAL_WTINT:
        sim_idle (MIKASA_CLOCK_TMR, 0);
        R[0] = 0;
        break;

    case MPAL_BPT:
        return mikasa_pal_inst_fault (MIKASA_OSF_IF_K_BPT);

    case MPAL_BUGCHK:
        return mikasa_pal_inst_fault (MIKASA_OSF_IF_K_BUGCHK);

    case MPAL_CHME:
        return mikasa_pal_intexc (MIKASA_SCB_CHME,
            (MODE_E < mikasa_pal_current_mode ()) ?
            MODE_E : mikasa_pal_current_mode (), mikasa_pal_ipl);

    case MPAL_CALLSYS:
        return mikasa_pal_callsys ();

    case MIKASA_MPAL_CHMK:
        return mikasa_pal_intexc (MIKASA_SCB_CHMK, MODE_K, mikasa_pal_ipl);

    case MPAL_CHMS:
        return mikasa_pal_intexc (MIKASA_SCB_CHMS,
            (MODE_S < mikasa_pal_current_mode ()) ?
            MODE_S : mikasa_pal_current_mode (), mikasa_pal_ipl);

    case MPAL_CHMU:
        return mikasa_pal_intexc (MIKASA_SCB_CHMU,
            mikasa_pal_current_mode (), mikasa_pal_ipl);

    case MPAL_REI:
        return mikasa_pal_rei ();

    case MPAL_RETSYS:
        return mikasa_pal_retsys ();

    case MPAL_RTI:
        return mikasa_pal_rti ();

    case MPAL_GENTRAP:
        return mikasa_pal_inst_fault (MIKASA_OSF_IF_K_GENTRAP);

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
        R[0] = mikasa_pal_read_ps ();
        break;

    case MPAL_RDPS:
        R[0] = mikasa_pal_read_ps ();
        break;

    case MPAL_WRKGP:
        mikasa_pal_set_paltemp_raw (20, R[16]);
        break;

    case MPAL_WRUSP:
        mikasa_pal_usp = R[16];
        mikasa_pal_stkp[MODE_U] = mikasa_pal_usp;
        mikasa_pal_set_paltemp_raw (18, mikasa_pal_usp);
        break;

    case MPAL_RDUSP:
        R[0] = mikasa_pal_usp;
        break;

    case MPAL_SWPIPL:
        {
        uint32 new_ps = (uint32) R[16];
        uint32 new_ipl = (new_ps & MIKASA_OSF_PSV_M_IPL);
        uint32 new_mode = mikasa_pal_current_mode ();

        R[0] = mikasa_pal_read_ps ();
        mikasa_pal_set_mode (new_mode, new_ipl);
        }
        break;

    case MPAL_AMOVRR:
    case MPAL_AMOVRM:
        R[0] = 0;
        break;

    case MPAL_SWASTEN:
        {
        uint32 mode = mikasa_pal_current_mode ();
        uint32 mask = 1u << mode;

        R[0] = (mikasa_pal_asten & mask) ? 1 : 0;
        mikasa_pal_asten = (mikasa_pal_asten & ~mask) |
            ((arg32 & 1) ? mask : 0);
        ev5_asten = mikasa_pal_asten & MPAL_AST_MASK;
        }
        break;

    case MPAL_WR_PS_SW:
        mikasa_pal_ps = (mikasa_pal_ps & ~MPAL_PS_SW_MASK) |
            (arg32 & MPAL_PS_SW_MASK);
        break;

    case MPAL_RSCC:
        mikasa_pal_scc = mikasa_pal_scc +
            (t_uint64) ((pcc_l - mikasa_pal_last_pcc) & M32);
        mikasa_pal_last_pcc = pcc_l;
        R[0] = mikasa_pal_scc;
        break;

    case MPAL_RD_UNQ:
        if (mikasa_pal_valid_phys (mikasa_pal_pcbb + MIKASA_PCB_THREAD,
            sizeof (t_uint64)))
            mikasa_pal_thread = ReadPQ (mikasa_pal_pcbb + MIKASA_PCB_THREAD);
        R[0] = mikasa_pal_thread;
        break;

    case MPAL_WR_UNQ:
        mikasa_pal_thread = R[16];
        if (mikasa_pal_valid_phys (mikasa_pal_pcbb + MIKASA_PCB_THREAD,
            sizeof (t_uint64)))
            WritePQ (mikasa_pal_pcbb + MIKASA_PCB_THREAD, mikasa_pal_thread);
        break;

    case MPAL_CLRFEN:
        fpen = 0;
        mikasa_pal_write_fen (0);
        break;

    case MPAL_MIKASA_CCALL:
        if (mikasa_srm_os_handoff && !mikasa_direct_apb)
            return mikasa_pal_inst_fault (MIKASA_OSF_IF_K_OPCDEC);
        return mikasa_console_callback ();

    default:
        if (mikasa_pal_route_os_events ())
            return mikasa_pal_inst_fault (MIKASA_OSF_IF_K_OPCDEC);
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

mikasa_direct_apb = TRUE;
mikasa_srm_bootstrap_pal = FALSE;
mikasa_srm_os_handoff = FALSE;
mikasa_hwrpb_pa = hwrpb_pa;
mikasa_boot_l1_pt_pa = MIKASA_L1_PT_PA;
mikasa_boot_vptb_va = MIKASA_PT_SPACE_VA;
mikasa_boot_image_pa = MIKASA_BOOT_IMAGE_PA;
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
mikasa_apb_trace_count = 0;
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
R[0] = 0;                                           /* boot dev type (disk) */
R[1] = 0;                                           /* nexus/adapter info */
R[2] = 0;                                           /* controller/unit ext */
R[3] = 0;                                           /* unit number */
R[4] = 0;                                           /* reserved boot arg */
R[16] = hwrpb_pa;
R[17] = 0;
R[18] = 0;
R[30] = MIKASA_BOOT_STACK_TOP;
pal_mode = 0;
dmapen = 1;
fpen = 1;
pal_type = PAL_UNIX;
mikasa_hwrpb_pa = hwrpb_pa;
mikasa_last_osflags = osflags;
mikasa_last_boot_bytes = image_bytes;
mikasa_pal_pcbb = MIKASA_IMPURE_PA + CNS_Q_SCRATCH;
mikasa_pal_prbr = 0;
mikasa_pal_ptbr = MIKASA_L1_PT_PA;
mikasa_pal_scbb = 0;
mikasa_pal_vtbr = 0;
mikasa_pal_virbnd = M64;
mikasa_pal_sysptbr = 0;
mikasa_pal_thread = 0;
mikasa_pal_esp = 0;
mikasa_pal_ssp = 0;
mikasa_pal_usp = 0;
mikasa_pal_stkp[MODE_K] = R[30];
mikasa_pal_stkp[MODE_E] = mikasa_pal_esp;
mikasa_pal_stkp[MODE_S] = mikasa_pal_ssp;
mikasa_pal_stkp[MODE_U] = mikasa_pal_usp;
mikasa_pal_impure = MIKASA_IMPURE_PA;
mikasa_pal_ipl = 0x1F;
ev5_ipl = mikasa_pal_ipl;
ev4_state.ipl = mikasa_pal_ipl & 0x7;
ev4_state.itlb_cm = MODE_K & 0x1;
ev4_state.dtlb_cm = MODE_K & 0x1;
ev4_state.ps_sw = (ev4_state.ipl & 0x7) | ((MODE_K & 0x1) << 3);
mikasa_pal_sisr = 0;
ev5_sirr = 0;
mikasa_pal_asten = 0;
mikasa_pal_astsr = 0;
ev5_asten = 0;
ev5_astrr = 0;
mikasa_pal_mces = MIKASA_MCES_DPC;
mikasa_pal_ps = 0;
mikasa_pal_datfx = 0;
mikasa_pal_last_pcc = pcc_l;
mikasa_pal_init_paltemp_regs ();
mikasa_pal_init_ipr_shadows ();
mikasa_pal_reset_impure (mikasa_pal_impure);
WritePQ (mikasa_pal_pcbb + MIKASA_PCB_KSP, mikasa_pal_stkp[MODE_K]);
WritePQ (mikasa_pal_pcbb + MIKASA_PCB_USP, mikasa_pal_usp);
WritePQ (mikasa_pal_pcbb + MIKASA_PCB_PTBR, mikasa_pal_ptbr_pfn ());
WritePL (mikasa_pal_pcbb + MIKASA_PCB_ASN, itlb_asn & ITB_ASN_M_ASN);
WritePQ (mikasa_pal_pcbb + MIKASA_PCB_UNIQUE, mikasa_pal_thread);
WritePQ (mikasa_pal_pcbb + MIKASA_PCB_FEN, fpen & 1);
mikasa_pal_set_paltemp_raw (31, mikasa_pal_pcbb);
mikasa_set_cpu_info ();

return SCPE_OK;
}

t_stat mikasa_reset (DEVICE *dptr)
{
t_stat r;

mikasa_irq_mask = 0;
mikasa_irq_summary = 0;
sim_cancel (&mikasa_unit);
mikasa_direct_apb = FALSE;
mikasa_srm_bootstrap_pal = FALSE;
mikasa_srm_os_handoff = FALSE;
mikasa_hwrpb_pa = MIKASA_HWRPB_PA;
mikasa_boot_l1_pt_pa = MIKASA_L1_PT_PA;
mikasa_boot_vptb_va = MIKASA_PT_SPACE_VA;
mikasa_boot_image_pa = MIKASA_BOOT_IMAGE_PA;
mikasa_pal_pcbb = 0;
mikasa_pal_prbr = 0;
mikasa_pal_ptbr = 0;
mikasa_pal_scbb = 0;
mikasa_pal_vtbr = 0;
mikasa_pal_virbnd = M64;
mikasa_pal_sysptbr = 0;
mikasa_pal_thread = 0;
mikasa_pal_esp = 0;
mikasa_pal_ssp = 0;
mikasa_pal_usp = 0;
memset (mikasa_pal_stkp, 0, sizeof (mikasa_pal_stkp));
mikasa_pal_impure = MIKASA_IMPURE_PA;
mikasa_pal_scc = 0;
mikasa_pal_ipl = MPAL_IPL_MASK;
mikasa_pal_sisr = 0;
mikasa_pal_asten = 0;
mikasa_pal_astsr = 0;
mikasa_pal_mces = MIKASA_MCES_DPC;
mikasa_pal_ps = 0;
mikasa_pal_datfx = 0;
mikasa_pal_last_pcc = pcc_l;
ev4_state.ipl = mikasa_pal_ipl & 0x7;
ev4_state.itlb_cm = MODE_K & 0x1;
ev4_state.dtlb_cm = MODE_K & 0x1;
ev4_state.ps_sw = (ev4_state.ipl & 0x7) | ((MODE_K & 0x1) << 3);
mikasa_pal_init_paltemp_regs ();
mikasa_pal_init_ipr_shadows ();
mikasa_pal_reset_impure (mikasa_pal_impure);
mikasa_uart_lcr = 0;
mikasa_uart_mcr = 0;
mikasa_uart_ier = 0;
mikasa_uart_scr = 0;
mikasa_uart_dll = 0;
mikasa_uart_dlm = 0;
mikasa_uart_rbr = 0;
mikasa_uart_rbr_valid = FALSE;
mikasa_uart_thre_irq = TRUE;
mikasa_uart_msr_delta = 0;
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
mikasa_pic_irr[0] = 0;
mikasa_pic_irr[1] = 0;
mikasa_pic_isr[0] = 0;
mikasa_pic_isr[1] = 0;
mikasa_pic_vec[0] = 0;
mikasa_pic_vec[1] = 8;
mikasa_pic_init[0] = 0;
mikasa_pic_init[1] = 0;
mikasa_pic_read_isr[0] = FALSE;
mikasa_pic_read_isr[1] = FALSE;
mikasa_pic_need_icw4[0] = FALSE;
mikasa_pic_need_icw4[1] = FALSE;
mikasa_pic_auto_eoi[0] = FALSE;
mikasa_pic_auto_eoi[1] = FALSE;
mikasa_pic_level[0] = 0;
mikasa_pic_level[1] = 0;
mikasa_elcr[0] = 0;
mikasa_elcr[1] = 0;
memset (mikasa_dma1_reg, 0, sizeof (mikasa_dma1_reg));
memset (mikasa_dma2_reg, 0, sizeof (mikasa_dma2_reg));
memset (mikasa_dma_page_reg, 0, sizeof (mikasa_dma_page_reg));
mikasa_irq_update ();
mikasa_cfg_index = 0;
memset (mikasa_cfg_reg, 0, sizeof (mikasa_cfg_reg));
mikasa_portb = 0;
mikasa_pit_reset ();
mikasa_kbc_reset ();
mikasa_rtc_index = 0;
mikasa_rtc_periodic_accum = 0;
mikasa_rtc_last_update = 0;
mikasa_rtc_last_alarm = 0;
if (!mikasa_nvram_loaded)
    mikasa_rtc_init_defaults ();
else {
    mikasa_rtc_reg[MIKASA_RTC_INTR] = 0;
    mikasa_rtc_reg[MIKASA_RTC_STATUSD] = MIKASA_RTC_POWER_OK;
    }
mikasa_sio_index = 0;
memset (mikasa_sio_reg, 0, sizeof (mikasa_sio_reg));
memset (mikasa_fdc_reg, 0, sizeof (mikasa_fdc_reg));
mikasa_nmi_ctrl = 0;
mikasa_eisa_cram_page = 0;
memset (mikasa_eisa_cram, 0, sizeof (mikasa_eisa_cram));
mikasa_comanche_init_regs ();
memset (mikasa_epic_reg, 0, sizeof (mikasa_epic_reg));
mikasa_epic_reg[0] = MIKASA_EPIC_DCSR_PASS2;
memset (mikasa_ncr_cfg, 0, sizeof (mikasa_ncr_cfg));
mikasa_ncr_cfg[0x00 >> 2] = 0x00011000u;
mikasa_ncr_cfg[0x04 >> 2] = 0x02000001u;
mikasa_ncr_cfg[0x08 >> 2] = 0x01000001u;
mikasa_ncr_cfg[0x10 >> 2] = 0x00000001u;
mikasa_ncr_cfg[0x3C >> 2] = 0x4011010Cu;
mikasa_ncr_init_regs ();
r = mikasa_ncr_scsi_init ();
if (r != SCPE_OK)
    return r;
memset (mikasa_ncr_sense_key, 0, sizeof (mikasa_ncr_sense_key));
memset (mikasa_ncr_sense_asc, 0, sizeof (mikasa_ncr_sense_asc));
memset (mikasa_ncr_sense_ascq, 0, sizeof (mikasa_ncr_sense_ascq));
memset (mikasa_tulip_cfg, 0, sizeof (mikasa_tulip_cfg));
mikasa_tulip_cfg[0x00 >> 2] = 0x00021011u;
mikasa_tulip_cfg[0x04 >> 2] = 0x02800001u;
mikasa_tulip_cfg[0x08 >> 2] = 0x02000002u;
mikasa_tulip_cfg[0x10 >> 2] = 0x00000001u;
mikasa_tulip_cfg[0x3C >> 2] = 0x0000010Bu;
mikasa_tulip_init_srom ();
mikasa_tulip_reset_regs ();
memset (mikasa_vga_cfg, 0, sizeof (mikasa_vga_cfg));
mikasa_vga_cfg[0x00 >> 2] = 0x00A81013u;
mikasa_vga_cfg[0x04 >> 2] = 0x011F0000u;
mikasa_vga_cfg[0x08 >> 2] = 0x03000002u;
mikasa_vga_cfg[0x10 >> 2] = 0xF8000000u;
mikasa_vga_cfg[0x3C >> 2] = 0x281401FFu;
memset (mikasa_vga_reg, 0, sizeof (mikasa_vga_reg));
mikasa_vga_crtc_index = 0;
mikasa_vga_seq_index = 0;
mikasa_vga_gfx_index = 0;
mikasa_vga_attr_index = 0;
mikasa_vga_attr_addr = TRUE;
memset (mikasa_pceb_cfg, 0, sizeof (mikasa_pceb_cfg));
mikasa_pceb_cfg[0x00 >> 2] = 0x04828086u;
mikasa_pceb_cfg[0x04 >> 2] = 0x02000007u;
mikasa_pceb_cfg[0x08 >> 2] = 0x06020003u;
mikasa_pceb_cfg[0x40 >> 2] = 0x80800020u;
memset (mikasa_ocp_reg, 0, sizeof (mikasa_ocp_reg));
memset (mikasa_ocp_ddram, ' ', sizeof (mikasa_ocp_ddram));
mikasa_ocp_addr = 0;
mikasa_ocp_busy_count = 0;
mikasa_halt_pending = mikasa_halt_switch;
mikasa_unit.wait = MIKASA_CLOCK_DELAY;
sim_activate (&mikasa_unit, sim_rtcn_init_unit (&mikasa_unit,
    mikasa_unit.wait, MIKASA_CLOCK_TMR));
return SCPE_OK;
}
