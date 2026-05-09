# Mikasa PAL Problems in `alpha/alpha_ev5_pal.c`

## Current Implementation Status

The Mikasa path now has a dedicated EV4 bridge (`alpha/alpha_ev4_*`) and a
C PAL implementation in `alpha/alpha_mikasa.c` for the critical reset,
CSERVE, interrupt, MM/TB, privileged CALL_PAL, unprivileged CALL_PAL, and
machine-check/logout flows.  Remaining `ev5_*` references in the Mikasa file
are compatibility mirrors for shared SIMH Alpha state and non-Mikasa paths;
the authoritative Mikasa PAL state is the EV4 state plus the Mikasa PAL fields.

The historical differences below are kept as the problem statement.  Items now
covered by the C model include EV4 dispatch interception, OSF PS/IPL handling,
`ptIntMask`/HIER routing, reset/impure initialization, CSERVE/JTOPAL,
SWPPAL/SWPCTX, TBI/WRENT, CALLSYS/entIF dispatch, and impure logout frames.
The remaining work is focused verification, cleanup of legacy EV5 naming, and
documenting intentionally simulated hardware behavior.

This document compares `alpha/alpha_ev5_pal.c` with the original source in
`mikasa/osfpal.S`, `mikasa/platform.S`, and the related headers. The main
conclusion is that `alpha_ev5_pal.c` is still an EV5 PAL simulator with a
partial EV4/Mikasa adapter, not a faithful translation of the DECchip
21064/21066 OSF/1 PALcode used by Mikasa/AlphaServer 1000.

The `mikasa_pal_*` functions in `alpha/alpha_mikasa.c` mitigate some
bootstrap and OS paths, but they do not make `alpha_ev5_pal.c` equivalent to
the original PAL source.

## Compared Sources

- `alpha/alpha_ev5_pal.c`: the C file under review. It identifies itself as
  EV5 and includes `alpha_ev5_defs.h`.
- `mikasa/osfpal.S`: DECchip 21064/21066 OSF/1 PALcode, with EV4 hardware
  vectors.
- `mikasa/platform.S`: Mikasa platform-specific code, especially interrupt,
  CSERVE, and reset handling.
- `mikasa/dc21064.h`, `mikasa/osf.h`, `mikasa/cserve.h`, `mikasa/impure.h`:
  IPR map, PS/PTE/PCB/impure layout, and CALL_PAL/CSERVE numbers.

## Critical Differences

### 1. Wrong Architecture Base: EV5 Instead of EV4/21064

`alpha_ev5_pal.c` models EV5 registers, vectors, and semantics. The original
Mikasa PAL is for DECchip 21064/21066: the original headers define the `PAL`,
`ABX`, and `IBX` IPR groups and registers such as `iccsr`, `hirr`, `hier`,
`aboxCtl`, `biuCtl`, `ptIntMask`, `ptPrcb`, and `ptVptPtr`.

Effects in the C code:

- Globals and registers are EV5-oriented (`ev5_icsr`, `ev5_mcsr`,
  `ev5_mvptbr`, `ev5_dc_mode`, `ev5_maf_mode`, and others).
- EV4 support is limited to a small `ev4_pal_19`/`ev4_pal_1d` shim.
- Many IPRs that the original source actually uses are missing or always read
  as zero.

Recommendation: split the EV4/Mikasa backend from the EV5 backend. A dedicated
`alpha_ev4_pal.c` or `alpha_mikasa_pal.c` would reduce the risk of continuing
to mix EV5 and 21064 register models.

### 2. Almost All Hardware Vectors Are Wrong for Mikasa

The original PAL uses these EV4 offsets:

- reset `0x0000`
- machine check `0x0020`
- arithmetic trap `0x0060`
- interrupt `0x00E0`
- D-stream fault `0x01E0`
- I-stream TB miss `0x03E0`
- I-stream ACV `0x07E0`
- native DTB miss `0x08E0`
- PAL DTB miss `0x09E0`
- unaligned `0x11E0`
- opcode/fault `0x13E0`
- FEN `0x17E0`
- privileged/unprivileged CALL_PAL `0x2000`/`0x3000`

`alpha_ev5_pal.c` uses the EV5 `PALO_*` offsets instead. Only the Mikasa
interrupt fallback has a special `0x00E0` path; traps, exceptions, TLB misses,
unaligned faults, FEN, and reserved instruction faults still fall through to
EV5 vectors if `mikasa_pal_*` returns `SCPE_NOFNC`.

Examples:

- `pal_proc_intr()` uses `EV4_PALO_INTR` only for interrupts.
- `pal_proc_trap()` falls back to `PALO_TRAP`, which is EV5.
- `pal_proc_excp()` falls back to `PALO_RSVI`, `PALO_ALGN`, `PALO_FDIS`,
  `PALO_DFLT`, `PALO_ITBM`, and `PALO_DTBM`, all EV5 offsets.

Recommendation: define all EV4 vectors from `mikasa/dc21064.h` and use a
complete Mikasa dispatcher. If a Mikasa event is not handled by direct C code,
it must not fall through to an EV5 vector.

### 3. Original Reset Flow Is Not Implemented

The original reset does much more than zero a few variables. It at least:

- saves the impure pointer from the PAL image;
- saves GPRs and IPRs into the impure save-state area;
- initializes `PAL_BASE`, `ptImpure`, `ptPrcb`, `ptPtbr`, `ptVptPtr`,
  `ptKsp`, `ptSysVal`, and `ptWhami`;
- initializes `ICCSR`, `ABOX_CTL`, and `BIU_CTL`;
- saves the signature, SROM revision, processor ID, memory size, cycle count,
  processor mask, and sysctx;
- clears interrupts, error state, and TBs;
- runs `sys_reset`, which initializes `ptIntMask`, serial ports, the PIC, and
  platform state.

`pal_proc_reset_hwre()` in `alpha_ev5_pal.c` only clears `ev5_palbase`, a few
pins/flags, `ev4_hier`, `ev4_ps_sw`, and TLB mode/ASN/superpage state. The
impure state, `ptIntMask`, `ptPrcb`, `ptVptPtr`, `ptWhami`, MCES,
ICCSR/ABOX/BIU state, and console preparation are therefore missing.

Recommendation: if the goal is a built-in C PAL, port at least
`pal_reset_cont`, `sys_reset`, and `sys_reset_swppal`. If the goal is to run
the original assembly PAL in memory, the C reset should prepare a correct IPR
model instead of replacing the original reset with a minimal EV5 reset.

### 4. EV4 IPR Map Is Incomplete

The EV4 shim recognizes the `PAL`, `IBX`, and `ABX` group bits, but implements
only a subset.

Important gaps:

- Ibox: `SL_RCV`, `SIER`, `SL_CLR`, and `SL_XMIT` have no real semantics.
- HIRR/HIER do not include full SLR, CRR, SWR, ATR, PC0/PC1, and CRE/SLE
  semantics.
- Abox: `dtbCtl/tbCtl`, `biuAddr`, `biuStat`, `dcAddr`, `fillAddr`,
  `aboxCtl`, `biuCtl`, `fillSyndrome`, `bcTag`, `flushIc`, `flushIcAsm`,
  `xtbZap`, and `xtbAsm` are absent or ignored.
- `DC_STAT/C_STAT` always reads as zero.
- `ABOX_CTL` and `BIU_CTL` are not stored, but `platform.S` reads and writes
  them through CSERVE and reset.

This breaks original flows such as `sys_cserve_wr_abox`,
`sys_cserve_wr_biu`, machine-check logout, CRD logout, cache scrub, and
platform reset.

Recommendation: create an explicit EV4 IPR table with read/write handlers for
all IPRs defined in `mikasa/dc21064.h`. For IPRs that are not fully simulated,
at least preserve the written value and return it on reads instead of ignoring
the write or returning zero.

### 5. Wrong PS Format for OSF/1 EV4

In the original OSF/1 source, the saved PS has:

- `PS<3>` = current mode;
- `PS<2:0>` = IPL.

`ev4_ps_read()` in `alpha_ev5_pal.c` instead returns `ev5_ipl << 8`, and
`ev4_write_ibox_ipr(... PS ...)` reads the IPL from `val >> 8`. This is an
EV5-like format, not the format used by `pt9_ps`, `rdps`, `swpipl`, `rti`,
`retsys`, and `BUILD_STACK_FRAME` in the original PAL.

Recommendation: for Mikasa/OSF, keep an `ev4_ps` value compatible with
`mikasa/osf.h`: IPL in bits 0..2 and mode in bit 3. PS writes should
synchronize `itlb_cm`, `dtlb_cm`/current mode, and `ev5_ipl` only as derived
simulator state, not as the architectural format exposed to PALcode.

### 6. Interrupt Routing Does Not Follow HIRR/HIER and `platform.S`

The original PAL reads `hirr`, masks it with `hier`, handles CRD/performance
counter interrupts, then calls `sys_interrupt`. The Mikasa platform code uses
the priority order IRQ4, IRQ5, IRQ2, IRQ1, IRQ0; IRQ4 becomes a clock
interrupt, IRQ1/IRQ2 become device interrupts, and IRQ0/IRQ5 become system
machine checks in specific cases.

`pal_eval_intr()` in `alpha_ev5_pal.c` uses a generic priority over
`int_req[]`, `ev5_sirr`, AST, and a few EV5 pins. The `ev4_hirr_bits()` shim
maps only the six hardware IRQs and the HWR bit, without the full HIRR
semantics.

Recommendation: model HIRR/HIER as real EV4 registers and let the Mikasa
interrupt path translate platform IRQs into the same vectors and levels as
`platform.S`. `ptIntMask` must become an effective PALtemp register used by
`swpipl`, `rti`, `retsys`, and reset.

### 7. Memory Management and TB Miss Handling Are EV5, Not the Original PAL

The original PAL performs a three-level page-table walk, handles `ptPtbr`,
`ptVptPtr`, physical mode through the low bit/PTBR<63>, invalidates ITB/DTB,
distinguishes native/PAL DTB misses, and builds ACV/TNV/FOE exceptions with
`a0/a1/a2`.

For generic miss paths, `alpha_ev5_pal.c` sets EV5 registers such as
`MM_STAT`, `VA_FORM`, `IVA_FORM`, `IVPTBR`, and `MVPTBR`, then jumps to EV5
vectors. It does not perform the Mikasa OSF/1 page-table walk. Some cases are
intercepted in `alpha_mikasa.c`, but the fallback in `alpha_ev5_pal.c` is not
the original behavior.

Recommendation: move the Mikasa page-table walk into dedicated EV4 C code
using the PTE layout from `mikasa/osf.h`, or guarantee that all Mikasa TB
misses are always handled before any EV5 fallback.

### 8. OSF/1 CALL_PAL Is Not Implemented in `alpha_ev5_pal.c`

The CALL_PAL offsets are compatible as a formula:
`0x2000/0x3000 + ((fnc & 0x3f) << 6)`, but the behavior is not implemented in
the EV5 C file. The original PAL implements, among others:

- privileged: `halt`, `cflush`, `draina`, `cserve`, `swppal`, `rdmces`,
  `wrmces`, `wrfen`, `wrvptptr`, `swpctx`, `wrval`, `rdval`, `tbi`,
  `wrent`, `swpipl`, `rdps`, `wrkgp`, `wrusp`, `wrperfmon`, `rdusp`,
  `whami`, `retsys`, and `rti`;
- unprivileged: `bpt`, `bugchk`, `callsys`, `imb`, `rdunique`, `wrunique`,
  and `gentrap`.

`alpha_ev5_pal.c` delegates some cases to `mikasa_pal_proc_inst()`, but if
that returns `SCPE_NOFNC`, it only jumps into the PAL image at the computed
offset. If no original PAL image is executable and correctly loaded, the
semantics are missing.

Recommendation: choose one strategy:

1. run the original PAL and complete the IPR/impure model;
2. translate the original CALL_PAL flows to C and stop jumping into EV5
   vectors.

Mixing both models makes it hard to know which state is authoritative.

### 9. Platform CSERVE Is Incomplete

`platform.S` implements CSERVE for:

- physical quadword load/store;
- read/write shadow `ABOX_CTL`, `BIU_CTL`, and `ICCSR`;
- `JTOPAL`, which passes ABOX/BIU and firmware parameters;
- interrupt mask write (`ptIntMask`);
- impure pointer read;
- serial `putc`.

`alpha_ev5_pal.c` does not contain this behavior. Some similar functions exist
in `alpha_mikasa.c`, but they do not match the original OSF/1 CSERVE
one-to-one and do not repair the missing IPR model in `alpha_ev5_pal.c`.

Recommendation: port `sys_cserve_*` from `platform.S` to C, or run the
original PAL code while ensuring that `ABOX_CTL`, `BIU_CTL`, `ICCSR`,
`ptImpure`, `ptIntMask`, and I/O ports are modeled.

### 10. Machine Check, CRD, and Logout Frames Are Missing

The original PAL distinguishes machine checks from PAL, double machine checks,
invalid KSP, ECC/parity/cache errors, CRD, SIO NMI, and DCSR. It builds long
and short logout frames in the impure area and updates MCES.

`alpha_ev5_pal.c` has only generic flags (`ev5_mchk`, `ev5_crd`,
`ev5_pwrfl`) and EV5 dispatch. There are no equivalents for `ptMces`,
logout frames, `pal_mchk_logout`, `pal_crd_logout`, `sys_enter_console`, or
`sys_scrub_mem`.

Recommendation: implement at least an EV4 MCES model and logout frames
compatible with `mikasa/impure.h`; otherwise OS code that inspects logout/MCES
will see impossible state or zeros.

### 11. `HW_REI` and PC/PALmode Do Not Match EV4 Semantics Clearly

The original PAL uses `excAddr` low bits to indicate PAL/native mode and often
clears `PC<1:0>` before returning. `platform.S` also uses `JTOPAL` with
`(addr & ~3) | 1` to enter PAL mode.

`pal_1e()` in `alpha_ev5_pal.c` assigns `PC = ev5_excaddr` and uses only bit 0
for `pal_mode`, without a clear EV4 variant that masks the PC and updates
`pc_align` consistently. This can work only if other simulator pieces
compensate, but it is not a clear translation of the original behavior.

Recommendation: introduce separate EV4 helpers `ev4_palent()` and `ev4_rei()`
with explicit handling for `excAddr`, the PAL bit, `PC`, `pc_align`, PS, and
stack mode.

### 12. TBI and Cache/Invalidate Handling Are Simplified

The original `tbi` PAL call uses a jump table with `tbia`, `tbiap`, `tbisi`,
`tbisd`, and `tbis` cases, plus I-cache flushes and selective ITB/DTB
invalidations. The C shim maps many write IPRs (`ITBZAP`, `ITBASM`, `ITBIS`,
`DTBZAP`, `DTBASM`, `DTBIS`) to invalidations that are too broad or
incomplete, and it ignores I-cache flushes.

Recommendation: use distinct handlers for `TBIA`, `TBIAP`, `TBIS`, `TBISD`,
and `TBISI`, with correct ITB/DTB/ASM/all flags and at least a traceable no-op
for I-cache flush.

## Recommended Repair Plan

1. Add dedicated EV4/Mikasa constants:
   hardware vectors, IPR group bits, PS bits, HIRR/HIER, MCES, PTE, PCB,
   CALL_PAL, and CSERVE values from `mikasa/*.h`.

2. Split Mikasa dispatch from EV5:
   `pal_proc_intr`, `pal_proc_trap`, `pal_proc_excp`, `pal_proc_inst`,
   `pal_19`, `pal_1d`, and `pal_1e` should call EV4 helpers when
   `cpu_model == ALPHA_MODEL_MIKASA_4_266`.

3. Complete the EV4 IPR model:
   start from `dc21064.h` and implement read/write handlers for every IPR used
   by `osfpal.S` and `platform.S`. For hardware registers that are not fully
   simulated, preserve written values and read them back.

4. Fix PS and IPL:
   `pt9`/`pt9_ps` must use the OSF/1 format `IPL<2:0>, CM<3>`.
   `swpipl`, `rti`, `retsys`, and interrupt delivery must be based on
   `ptIntMask` and HIER, not the EV5 format.

5. Implement reset and CSERVE:
   port the essential parts of `pal_reset_cont`, `sys_reset`,
   `sys_reset_swppal`, and `sys_cserve_*`. This is required to make the
   impure area, ABOX/BIU shadows, JTOPAL, and interrupt mask coherent.

6. Implement or intercept all EV4 vectors:
   if a Mikasa C handler is missing, jump to an EV4 original vector loaded in
   memory; never to an EV5 vector.

7. Write focused tests:
   - reset initializes `ptImpure`, `ptPrcb`, `ptVptPtr`, and `ptIntMask`;
   - `mfpr/mtpr pt9` keeps PS bit-compatible with `mikasa/osf.h`;
   - `CSERVE_K_JTOPAL` enters PAL mode at `(a0 & ~3) | 1`;
   - `swpipl` updates HIER from `ptIntMask`;
   - `tbi` distinguishes ITB, DTB, ASM, and all-entry invalidation;
   - a TB miss loads a valid PTE or generates ACV/TNV with `a0/a1/a2`.

## Practical Note

The safest repair is not to keep patching small cases inside the EV5 file. The
problem is structural: the original PAL is EV4 OSF/1 platform-specific, while
the current file is generic EV5 with Mikasa pieces added. Introduce a separate
EV4/Mikasa backend and keep `alpha_ev5_pal.c` as EV5.
