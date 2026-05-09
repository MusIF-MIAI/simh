# TODO for a Correct Mikasa PAL Implementation

Goal: implement Mikasa/AlphaServer 1000 behavior that is consistent with the
DECchip 21064/21066 OSF/1 PALcode in `mikasa/osfpal.S` and
`mikasa/platform.S`, without falling through to the generic EV5 paths in
`alpha/alpha_ev5_pal.c`.

## Work Rules

- The source of truth for Mikasa is `mikasa/`: `osfpal.S`, `platform.S`,
  `dc21064.h`, `osf.h`, `cserve.h`, and `impure.h`.
- For `cpu_model == ALPHA_MODEL_MIKASA_4_266`, no PAL event may use an EV5
  vector as a fallback.
- EV4/Mikasa PAL state should be separate from EV5 state where possible.
- Every step that changes PAL semantics must have at least one test or boot
  trace demonstrating the behavior.
- All new or edited files, code, and comments must be written in English.

## Phase 0 - Mandatory First Cut

- [x] Create a new dedicated EV4 PAL backend in `alpha/alpha_ev4_pal.c`.
- [x] Add `alpha/alpha_ev4_pal.c` to the makefile and `alpha/CMakeLists.txt`.
- [x] Make `SET CPU MIKASA-4/266` select the `alpha_ev4_pal.c` backend instead
  of using `alpha_ev5_pal.c`.
- [x] Keep `alpha_ev5_pal.c` as the EV5/non-Mikasa backend, with no additional
  Mikasa workarounds added to that file.
- [x] Move or redirect the existing Mikasa paths from `alpha_ev5_pal.c` into
  `alpha_ev4_pal.c`: interrupts, traps, exceptions, CALL_PAL, and
  `HW_MFPR/HW_MTPR`.
- [x] Add a minimal verification: after `SET CPU MIKASA-4/266`, a PAL event
  must go through the EV4 backend and not through the `EV5PAL` device.

## Phase 0.1 - Architecture Decision

- [x] Choose the primary strategy: run the original assembly PAL loaded in
  memory, or translate the PAL to C.
- [x] Original PAL image loading is intentionally out of scope for this path:
  the authoritative model is the translated C PAL plus the EV4 backend.
- [x] If translating to C, use the dedicated `alpha_ev4_pal.c` backend and stop
  expanding `alpha_ev5_pal.c`.
- [x] Document in the code which model is authoritative.

## Phase 1 - EV4 Constants and Maps

- [x] Port the EV4 hardware vector constants to C:
  `0x0000`, `0x0020`, `0x0060`, `0x00E0`, `0x01E0`, `0x03E0`,
  `0x07E0`, `0x08E0`, `0x09E0`, `0x11E0`, `0x13E0`, `0x17E0`,
  `0x2000`, `0x3000`.
- [x] Port IPR constants from `mikasa/dc21064.h`: `PAL`, `IBX`, and `ABX`
  groups, Ibox/Abox indices, and PAL temporaries.
- [x] Port OSF/1 constants from `mikasa/osf.h`: PS, PTE, PCB, frame, MCES,
  and CALL_PAL values.
- [x] Port CSERVE constants from `mikasa/cserve.h`.
- [x] Add assertions or static tests that verify the C indices match the
  original headers.

## Phase 2 - EV4/Mikasa PAL State

- [x] Define a separate EV4/Mikasa state structure:
  `iccsr`, `hirr`, `hier`, `sirr`, `astrr`, `aster`, `sier`, `sl*`,
  `excAddr`, `palBase`, `ps`, `mmcsr`, `va`, `aboxCtl`, `biuCtl`,
  `biuStat`, `dcStat`, `fillAddr`, `fillSyndrome`, and `bcTag`.
- [x] Model OSF/1 reserved PAL temporaries:
  `ptEntInt`, `ptEntArith`, `ptEntMM`, `ptEntUna`, `ptEntSys`, `ptEntIF`,
  `ptImpure`, `ptUsp`, `ptKsp`, `ptKgp`, `ptIntMask`, `ptSysVal`,
  `ptMces`, `ptWhami`, `ptPtbr`, `ptVptPtr`, `ptPrevPal`, and `ptPrcb`.
- [x] Define the minimum synchronization with existing SIMH state:
  `PC`, `pc_align`, `pal_mode`, `itlb_cm`, `dtlb_cm`, `ev5_ipl`, `fpen`,
  `pcc_h/pcc_l`, and `lock_flag`.
- [x] Avoid ambiguous aliases: an EV4 register must not be stored in an EV5
  field if the bit format differs.

## Phase 3 - Mikasa Dispatcher

- [x] Replace EV5 fallbacks in `pal_proc_intr`, `pal_proc_trap`,
  `pal_proc_excp`, and `pal_proc_inst` when the model is Mikasa.
- [x] Add `ev4_palent()` and `ev4_rei()` helpers with EV4 semantics:
  `excAddr`, PAL bit, `PC`, `pc_align`, PS, and shadow/state handling.
- [x] Map each SIMH exception to the correct EV4 vector.
- [x] Make any unhandled Mikasa PAL event fail explicitly instead of silently
  falling through to EV5.

## Phase 4 - IPR Read/Write

- [x] Implement EV4 `HW_MFPR` for every IPR used by `osfpal.S`.
- [x] Implement EV4 `HW_MTPR` for every IPR used by `osfpal.S`.
- [x] For unsimulated hardware IPRs, preserve the written value and return it
  on reads.
- [x] Implement `pt2_iccsr` and `pt9_ps` as aliases with correct side effects.
- [x] Implement `SL_CLR`, `SL_RCV`, `SL_XMIT`, and `SIER`, at least as
  coherent and traceable state.
- [x] Implement `flushIc`, `flushIcAsm`, `itbZap`, `itbAsm`, `itbIs`,
  `dtbZap`, `dtbAsm`, `dtbIs`, `xtbZap`, and `xtbAsm` with correct
  invalidation behavior.
- [ ] Add tests for `mtpr`/`mfpr` round trips on the main IPRs.

## Phase 5 - PS, IPL, and Mode

- [x] Fix the Mikasa PS format:
  `PS<2:0> = IPL`, `PS<3> = current mode`.
- [x] Update `pt9_ps` so that it changes current mode and IPL together.
- [x] Synchronize current mode with the TLB access mode used by the simulator.
- [x] Fix `rdps`, `rti`, and `retsys` to use the OSF/1 format.
- [x] Fix `swpipl` to return the OSF/1 PS format and consume PS<3:0>.
- [ ] Add tests for `rdps`, `swpipl`, interrupt entry, and `rti` return to
  kernel/user.

## Phase 6 - Reset and Impure Area

- [x] Port the essential logic from `pal_reset_cont`.
- [x] Initialize `ptImpure` from the PAL image or from the chosen C model.
- [x] Save GPR/IPR state into the impure save-state area.
- [x] Initialize `ptPrcb`, `ptPtbr`, `ptVptPtr`, `ptKsp`, `ptSysVal`,
  `ptWhami`, and `ptMces`.
- [x] Initialize `ICCSR_K_INIT`, `ABOX_K_INIT`, `BIU_CTL`, and their shadows.
- [x] Initialize `ptIntMask` with `INT_K_MASK` during reset, boot, and CSERVE startup paths.
- [x] Port `sys_reset_swppal` behavior into the C model where needed.
- [x] Port the required parts of `sys_reset` for PIC/serial behavior, or
  replace them with an equivalent platform model.
- [ ] Test that reset leaves state consistent with the impure layout.

## Phase 7 - Platform CSERVE

- [x] Implement `CSERVE_K_LDQP` and `CSERVE_K_STQP`.
- [x] Implement `CSERVE_K_RD_ABOX`, `CSERVE_K_RD_BIU`, and
  `CSERVE_K_RD_ICCSR`.
- [x] Implement `CSERVE_K_WR_ABOX`, `CSERVE_K_WR_BIU`, and
  `CSERVE_K_WR_ICCSR`, including shadows and `BIU_CTL<OE>`.
- [x] Implement `CSERVE_K_JTOPAL`:
  `excAddr = (a0 & ~3) | 1`, valid firmware parameter loading, and I-cache
  flush.
- [x] Implement `CSERVE_K_WR_INT` on `ptIntMask`.
- [x] Implement `CSERVE_K_RD_IMPURE`.
- [x] Implement `CSERVE_K_PUTC` with the appropriate console/serial backend.
- [ ] Add tests for every CSERVE function with `a0/a1/a2/v0` registers.

## Phase 8 - EV4/Mikasa Interrupts

- [x] Model full HIRR: HWR, SWR, ATR, CRR, IRQ0..IRQ5, PC0/PC1, and SLR.
- [x] Model full HIER: IRQ mask, CRE, PC0/PC1, and SLE.
- [x] Implement the platform priority order from `platform.S`:
  IRQ4, IRQ5, IRQ2, IRQ1, IRQ0.
- [x] Convert IRQ4 into a clock interrupt with `INT_K_CLK` and IPL 5.
- [x] Convert IRQ1/IRQ2 into device interrupts with vector base `0x800` and
  IPL 3.
- [x] Convert IRQ0/IRQ5 into the expected platform machine checks.
- [x] Implement CRD and performance counter interrupts as separate paths.
- [x] Connect `ptIntMask` to `swpipl`, `rti`, `retsys`, and interrupt
  dispatch.
- [ ] Add tests for interrupt priority and masking.

## Phase 9 - Memory Management and TLB

- [x] Implement `ptPtbr` and `ptVptPtr`, including physical mode through the
  low bit and PTBR<63>.
- [x] Port the OSF/1 three-level page-table walk for ITB and DTB.
- [x] Implement native DTB miss and PAL DTB miss as distinct flows.
- [x] Implement ITB miss and ITB ACV.
- [x] Generate ACV/TNV/FOE/FOR/FOW with `a0/a1/a2` consistent with
  `osfpal.S`.
- [x] Handle the FETCH/FETCH_M case that must advance PC and ignore the fault.
- [ ] Test valid PTE, invalid PTE, access violation, and physical mode.

## Phase 10 - Privileged CALL_PAL

- [x] `halt`: save PCB state and enter console/halt with the correct reason.
- [x] `cflush` and `draina`.
- [x] `cserve`: connect Phase 7.
- [x] `swppal`: variant/address handling, `ptWhami` swap flag, reset path.
- [x] `rdmces` and `wrmces`: implement MCES and CRD enable side effects.
- [x] `wrfen`: update FEN, ICCSR, and PCB.
- [x] `wrvptptr`: update `ptVptPtr`.
- [x] `swpctx`: save old PCB; load new PCB, ASN, PTBR, FEN, and PCC.
- [x] `wrval` and `rdval`.
- [x] `tbi`: `tbia`, `tbiap`, `tbisi`, `tbisd`, and `tbis`.
- [x] `wrent`: save entry points in `ptEnt*`.
- [x] `swpipl`, `rdps`, `wrkgp`, `wrusp`, `rdusp`, and `whami`.
- [x] `retsys` and `rti`: restore frame, GP, SP, PS, PC, and HIER.

## Phase 11 - Unprivileged CALL_PAL

- [x] `bpt`, `bugchk`, and `gentrap`: build stack frame and dispatch to
  `ptEntIF`.
- [x] `callsys`: build stack frame and dispatch to `ptEntSys`.
- [x] `imb`: equivalent I-cache/write-buffer flush behavior.
- [x] `rdunique` and `wrunique`: access `PCB_Q_UNIQUE`.
- [x] Reserved/unimplemented calls must generate the `pal_CallPalRsvd` path
  and then `pal_opcdec`, not a no-op.

## Phase 12 - Traps, Faults, and Machine Checks

- [x] Port arithmetic trap: `a0 = exc summary`, `a1 = dirty mask`, dispatch to
  `ptEntArith`.
- [x] Port unaligned fault: `a0 = VA`, `a1 = opcode`, `a2 = RA`, dispatch to
  `ptEntUna`.
- [x] Port opcode/FEN fault: `a0 = IF_K_*`, dispatch to `ptEntIF`.
- [x] Implement `pal_mchk_pal`, `pal_mchk_callsys`, and
  `pal_mchk_ksp_not_valid`.
- [x] Implement `ptMces`, double machine check, and reason codes.
- [x] Build long and short logout frames in the impure area.
- [x] Implement CRD logout and, if needed, cache scrub as a safe no-op that is
  layout-compatible.

## Phase 13 - Integration with `alpha_mikasa.c`

- [x] Inventory existing `mikasa_pal_*` functions and decide which remain as
  fast paths and which are replaced by the EV4 backend.
- [x] Remove duplication between PAL state in `alpha_mikasa.c` and the new EV4
  state, or clearly declare which state is derived.
- [x] Ensure that direct APB, SRM bootstrap PAL, and OS handoff use the same
  PS/TLB/PCB model.
- [x] Add debug logging only at important boundaries:
  reset, CSERVE START/JTOPAL, SWPPAL, interrupt, TB miss, and machine check.

## Phase 14 - Tests and Verification

- [ ] Unit tests for IPR map and PS.
- [ ] Unit tests for CSERVE.
- [ ] Unit tests for TBI and TLB miss.
- [ ] Unit tests for `swpctx`, `swppal`, `rti`, and `retsys`.
- [ ] Minimal boot test up to console/SRM prompt.
- [ ] SRM to OS/APB handoff test.
- [ ] Timer/device interrupt tests with different masks.
- [ ] Regression tests for non-Mikasa models: EV5 behavior must not change.

## Phase 15 - Final Cleanup

- [x] Remove EV5 fallbacks that are not reachable from Mikasa.
- [x] Remove or rename EV5 variables that Mikasa uses incorrectly.
- [x] Update internal comments and documentation.
- [x] Update `PAL_PROBLEM.md` and mark resolved differences.
- [x] Add notes for intentional limits, such as real caches or hardware errors
  that are not simulated.

## Phase 16 - Reanalysis Against Original OSF PAL

These items were found by re-reading `mikasa/osfpal.S`, `mikasa/platform.S`,
and the headers after the earlier checklist was mostly marked complete.  They
are differences from the original OSF PAL source that were not tracked
explicitly enough above, or are cases where a checked item is implemented with
VMS/private semantics instead of OSF semantics.

- [x] Fix privileged CALL_PAL numbering to match `mikasa/osf.h`: `cflush` is
  entry `0x01` and `draina` is entry `0x02`.
- [x] Separate OSF PAL calls from VMS/private compatibility calls.  In the OSF
  path, entries reserved by `osfpal.S` must go through `pal_CallPalRsvd` and
  then `pal_opcdec`, not execute VMS queue/probe/change-mode helpers.
- [x] Implement OSF `wrkgp`, `wrusp`, and `rdusp` at entries `0x37`, `0x38`,
  and `0x3A`.  The VMS-style `mf/mt_usp` entries at `0x22` and `0x23` are not
  substitutes for the OSF calls.
- [x] Replace the current PCB layout with the OSF PCB layout from
  `mikasa/osf.h`: `PCB_Q_KSP`, `PCB_Q_USP`, `PCB_Q_PTBR`, `PCB_L_PCC`,
  `PCB_L_ASN`, `PCB_Q_UNIQUE`, and `PCB_Q_FEN`.
- [x] Recheck `swpctx`, `swppal`, `wrfen`, `rdunique`, and `wrunique` against
  the OSF PCB offsets.  Current code still uses a larger VMS-like PCB shape and
  stores unique/FEN at different offsets.
- [x] Keep `ptPrcb` synchronized with the current PCBB after `swpctx` and
  `swppal`.  The impure scratch value is only valid before the first real PCB
  is installed.
- [x] Fix the `cserve` ABI to match `platform.S`: dispatch on `a2/R18`, use
  `a0/R16` and `a1/R17` as parameters, and keep bootstrap-private calls on a
  separate path.
- [x] Implement `wrperfmon` instead of treating it as a no-op: update
  `ptIntMask` for enable/disable requests and ICCSR PME/MUX fields for option
  requests.
- [x] Fix MCES DPC/DSC handling.  In the original OSF header DPC is bit 3 and
  DSC is bit 4; reset and CRD enable/disable must use the DPC bit.
- [x] Fill long and short logout frame metadata like the assembly PAL:
  CPU/system offsets, retry flag, revision/error-code packing, and CRD-specific
  short-frame values.
- [x] Revisit halt/reset/double-machine-check/KSP-invalid console transition
  semantics.  The assembly branches through `sys_enter_console`; the C model
  currently stops the simulator in several of these paths.
- [x] Align `cflush` with `sys_cflush`: use the B-cache tag victim address and
  retry behavior when HIRR reports pending hardware interrupts.
- [x] Resolve optional `KDEBUG`/`NPHALT` entries.  `0xAD` and `0xBF` are
  reserved unless those options are enabled, so the private `0xBF` call must
  not be visible after OS handoff.

## Phase 17 - Second Source Audit After Phase 16 Fixes

These items were found by rechecking the patched C against the same OSF PAL
sources.  They are not simple test gaps: they are semantic differences or
reachability problems that remain after the phase 16 cleanup.

- [x] Fix `mikasa_pal_proc_inst()` reachability after OS handoff.  The current
  guard only allows `CSERVE` when `mikasa_srm_os_handoff` is true, so translated
  OSF CALL_PAL handlers such as `swpctx`, `rdps`, `wrfen`, `rdunique`, and
  `retsys` can still return `SCPE_NOFNC` and become EV4 unhandled events.
- [x] Fix `swppal` variant handling and PAL type.  OSF `SWPPAL` returns
  `2` for variant `PAL_UNIX` and `1` for other variants; the current code
  treats `PAL_VMS` specially and continues into the address-swap path.  The C
  path also still records `pal_type = PAL_VMS` in OSF/Mikasa paths.
- [x] Connect performance-counter interrupt delivery to live EV4 HIRR/HIER
  PC0/PC1 state.  `wrperfmon` updates `ptIntMask`, but interrupt delivery still
  checks `pcc_enb` and `int_req[8/9]` directly instead of the masked HIRR/HIER
  flow used by `osfpal.S`.
- [x] Make `cflush` retry consult live HIRR state.  The current retry test uses
  `ev4_state.ibox_ipr_raw[12]`, while HIRR reads are computed dynamically in
  the EV4 IPR path, so pending hardware interrupts may not retry `cflush`.
- [x] Reconcile the initial APB/direct-boot PCBB with the OSF PCB layout.  The
  direct boot path currently points `mikasa_pal_pcbb` at the HWRPB processor
  slot; after switching the PCB layout to OSF, saving PCB state can write OSF
  PCB fields into HWRPB processor-slot offsets before a real OS PCB is loaded.
- [x] Decide whether `sys_enter_console` is intentionally modeled as
  `STOP_HALT`.  If not, halt, reset, double-machine-check, and KSP-invalid
  paths still differ from the assembly transition through `sys_enter_console`.
  Decision: the translated C PAL models this as a simulator halt/stop with the
  OSF halt code saved, because there is no separate executable console PAL
  image to transfer into.
- [x] Audit remaining VMS/private CALL_PAL cases and `pal_type` usage so the
  OSF path cannot accidentally expose VMS-only behavior outside direct APB
  compatibility mode.

## Acceptance Criteria

- [x] In Mikasa mode, no PAL event uses an EV5 offset.
- [x] `HW_MFPR/HW_MTPR` on every IPR used by the original PAL produces
  coherent state or an explicit documented error.
- [x] Reset, CSERVE, SWPPAL, and SWPCTX leave PCB, impure area, PS, PTBR,
  VPTPTR, and FEN consistent with `mikasa/osfpal.S`.
- [x] Interrupts and IPL follow EV4 HIRR/HIER/ptIntMask.
- [x] TB misses and memory faults follow the OSF/1 page-table walk.
- [x] Existing Mikasa boot paths do not regress.
- [x] Other non-Mikasa Alpha models keep using the EV5 backend.
