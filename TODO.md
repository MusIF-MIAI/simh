# AlphaServer 1000 Bring-Up Checklist

## Done

- [x] Create and push the `alphaserver-1000` branch to `MusIF-MIAI/simh`.
- [x] Add the `MIKASA-4/266` CPU profile and 1 GB memory support.
- [x] Add Mikasa SRM LFU/payload ROM loading and `BOOT CPU` firmware entry.
- [x] Reach the real Mikasa SRM V5.4-101 banner from `mksrmrom.payload.bin`.
- [x] Add Comanche and EPIC APECS register shells.
- [x] Add APECS sparse I/O, sparse memory, dense memory, and PCI config paths.
- [x] Preserve EPIC DMA window registers and use them for PCI DMA translation.
- [x] Add EPIC HAXR0/HAXR2/PMLT/TLB/TBIA register behavior and SGMAP-backed
  PCI DMA translation through TBASE.
- [x] Decode APECS sparse-memory device BARs against the full `HAXR1`-extended
  PCI address instead of truncating BAR bases to the low sparse-address bits.
- [x] Mask APECS Comanche/EPIC error/status writes and add write-one-to-clear
  behavior for documented error latches.
- [x] Mask documented Comanche control, tag-enable, video-frame, bank base,
  bank config, and memory-timing fields; keep presence-detect registers
  read-only.
- [x] Latch EPIC `NDEV` with PEAR/SEAR addresses for absent PCI sparse/dense
  memory accesses.
- [x] Merge byte/word PCI config writes instead of replacing whole dwords.
- [x] Add Intel 82375EB/PCEB PCI/EISA bridge config shell with write masks.
- [x] Add ISA DMA/page-register storage, FDC shell, OCP shell, RTC basics, and
  EISA CRAM/absent-slot behavior.
- [x] Add a minimal 8042 keyboard-controller shell with command byte,
  output-port, self-test, keyboard-interface test, keyboard reset, ID, ACK,
  and parameter-command responses for standard I/O probes.
- [x] Add optional file-backed 128-byte RTC/NVRAM image support with default
  RTC values and alarm/update/periodic interrupt state.
- [x] Add 8259 PIC init, mask/pending/in-service, EOI, auto-EOI, cascade IACK,
  and ELCR edge/level-trigger storage.
- [x] Honor 8259 ELCR edge-vs-level behavior: edge IRQs latch only on rising
  edges, level IRQs track asserted inputs, and level IRQs can reassert after
  EOI while the line remains active.
- [x] Route COM1 receive interrupts through the SRM-programmed PIC path.
- [x] Add NCR/Symbios 53C810 PCI config, I/O BAR, memory BAR, register shell,
  interrupt status, abort, reset, and select-timeout behavior.
- [x] Mirror the NCR/Symbios 53C810 operating registers through both halves of
  the 256-byte I/O and memory BAR windows (`0x00..0x7f` and `0x80..0xff`).
- [x] Initialize the integrated NCR 53C810 PCI interrupt line as ICU IRQ 12,
  matching the AlphaServer 1000 platform route.
- [x] Collapse auxiliary NCR SCRIPTS scans behind one shared pass for
  `SELECT`, direct/table MOVE discovery, scatter/gather collection, and `INT`
  completion lookup.
- [x] Add NCR SCRIPTS scanning for select, table/direct/indirect moves,
  register ops, load/store, memory copy, and basic conditional flow.
- [x] Collect multiple NCR data-phase MOVE entries and transfer SCSI payloads
  across scatter/gather DMA segment lists.
- [x] Update NCR `DBC`, `DNAD`, `DSP`, and `DSPS` as handled SCRIPTS fetches
  and data MOVE transfers progress.
- [x] Reflect NCR `COMMAND`, `STATUS`, and `MESSAGE IN` move completion in
  `DBC`, `DNAD`, and current phase status, not only data moves.
- [x] Prefer real NCR SCRIPTS `INT` second-word values for `DSPS` completion
  interrupts, with synthetic values only as fallback.
- [x] Preserve already-latched NCR `DSTAT` causes until the CPU reads `DSTAT`,
  and raise `DSTAT.BF` for script fetch/DMA paths the frontend cannot execute
  instead of failing silently.
- [x] Add second-level NCR interrupt stacking for `DSTAT`, `SIST0`, and
  `SIST1`, including the stacked `DSPS` value for DMA interrupts.
- [x] Raise NCR `DSTAT.IID` for fetched zero-byte direct Block Move and Memory
  Move instructions, and clear selection-disconnect expectation after select
  timeouts.
- [x] Raise NCR `DSTAT.IID` when SCRIPTS Load/Store memory targets the
  controller's own memory-mapped register window.
- [x] Decode NCR SCRIPTS Load/Store separately from Memory Move using the
  three-bit instruction type, keep Load/Store instructions two longwords, and
  enforce documented Load/Store alignment/count limits.
- [x] Compare NCR Transfer Control phases using the documented bits `26..24`
  instead of the data-compare low bits.
- [x] Honor NCR Load/Store restrictions that chip register-window target
  addresses raise `DSTAT.IID` and `SFBR` cannot be loaded by Load/Store.
- [x] Model SCRIPTS Memory Move access to NCR register windows, including
  visible `TEMP` loading and documented zero-count/reserved-bit/alignment
  `DSTAT.IID` checks.
- [x] Preserve read-only NCR `ADDER`, update it from `CTEST5.ADCK/BBCK`
  clock pulses, auto-clear those pulse bits, and use the three-bit SCRIPTS
  instruction type in script tracing too.
- [x] Mask NCR `SIEN1` to the documented `STO`, `GEN`, and `HTH` interrupt
  enable bits.
- [x] Treat NCR SCRIPTS `INTFLY` as a non-halting interrupt-on-the-fly path and
  expose/clear `ISTAT.INTF`.
- [x] Keep NCR `INTFLY` side effects out of auxiliary script scans so debug
  searches do not reassert `ISTAT.INTF`.
- [x] Keep auxiliary NCR script scans from clobbering visible `DBC`, `DSP`, and
  `DSPS` fetch-progress registers while they are only searching for MOVE or
  `INT` instructions.
- [x] Suppress auxiliary NCR scan side effects for SCRIPTS register ops,
  load/store, and memory-copy instructions while preserving local SFBR/carry
  state needed for conditions.
- [x] Implement documented NCR SCRIPTS `SET`/`CLR` side effects for carry,
  `SOCL` ACK/ATN outputs, mirrored bus bits, and `SCNTL0` target mode.
- [x] Latch NCR SCRIPTS `SELECT` destination, table-indirect `SXFER`/`SCNTL3`,
  and select-with-ATN state into visible 53C810 registers.
- [x] Model visible NCR SCRIPTS wait-state behavior for legal disconnect and
  no-event wait-reselect paths.
- [x] Execute non-SELECT NCR control scripts until a halting `INT` or wait
  state, so post-status/message completion scripts do not falsely raise
  `DSTAT.BF`.
- [x] Preserve NCR `WAIT RESELECT` as an explicit wait state and honor
  `ISTAT.SIGP` by jumping to the alternate script address and resuming
  execution; `CTEST2` mirrors and clears the visible `SIGP` bit on read.
- [x] Model additional visible 53C810 register semantics used by real drivers:
  `SCNTL2.SDU`, `SSTAT2.LDSC`, host `SCID`/`RESPID`, selector `SSID`,
  `TEMP` for SCRIPTS `CALL`/`RETURN`, `WAIT RESELECT` `SIGP` branching, and
  masked SCRIPTS writes to writable controller registers.
- [x] Add writable masks for more NCR driver-probed registers (`CTEST2/5`,
  `DMODE`, `DCNTL`, `MACNTL`, `STIME1`, `STEST1/2/3`, and `SIEN1`) and honor
  `DCNTL.IRQD` by suppressing IRQ output without clearing latched status.
- [x] Tighten NCR `DCNTL.PFF` and `STIME1.GEN` register semantics: prefetch
  flush auto-clears and `STIME1` masks reserved bits.
- [x] Consume NCR `MESSAGE OUT` moves before command execution and clear ATN
  after the message-out handshake.
- [x] Parse common NCR `MESSAGE OUT` messages instead of blindly discarding
  them: `IDENTIFY` records LUN, simple/head/ordered queue tags are retained,
  extended messages are skipped cleanly, and non-zero LUNs return check
  condition.
- [x] Model NCR `CTEST3` revision/writable bits and DMA FIFO clear/flush
  effects with the local FIFO-empty state.
- [x] Report NCR `DFIFO.BO[6:0]` consistently with the local FIFO-empty model
  by mirroring the low seven bits of `DBC`.
- [x] Update NCR `SFBR` with the first byte of handled input phases including
  data-in, status, and message-in transfers.
- [x] Reflect first-byte SCSI bus data in low-level NCR latches: input phases
  update `SIDL`/`SBDL`, and output phases update `SODL`/`SBDL`.
- [x] Preserve read-only NCR low-level bus/latch registers (`SSID`, `SBCL`,
  `SIDL`, and `SBDL`) on host/SCRIPTS writes, while routing `SODL` writes to
  the output/bus data latches.
- [x] Tighten documented NCR register masks for `SCNTL3`, `SCID`, `SDID`,
  `GPREG`, `GPCNTL`, and `STEST3`, and implement self-clearing
  `STEST3.CSF` SCSI FIFO/latch-full flag clear.
- [x] Reflect successful NCR SCRIPTS arbitration/select attempts in
  `SSTAT0.WOA`.
- [x] Honor NCR `DMODE.MAN` manual-start mode so `DSP` writes do not start
  SCRIPTS until `DCNTL.STD` is set.
- [x] Honor NCR `DCNTL.SSM` restart semantics by requiring `DCNTL.STD` rather
  than auto-starting from `DSP` writes in single-step mode.
- [x] Execute one visible SCRIPTS instruction under NCR `DCNTL.SSM` and raise
  `DSTAT.SSI`, using current `SFBR` as the initial data-compare state.
- [x] Start NCR SCRIPTS execution from `DCNTL.STD`, not only from writes to
  `DSP`, matching the normal 53C810 driver path.
- [x] Model low-level NCR `SCNTL0.START` manual selection and `SCNTL1.RST`
  SCSI bus reset side effects.
- [x] Preserve NCR read-only status registers on CPU writes and expose current
  MOVE phase through `SOCL`, `SBCL`, and `SSTAT1`.
- [x] Track NCR connected state in `SCNTL1.ISCON`/`ISTAT.CON` across select,
  command, data, status/message, timeout, reset, and abort paths.
- [x] Reflect NCR connected/bus-free state in low-level `SBCL` bus lines by
  asserting `BSY` while connected and clearing bus control lines on bus free.
- [x] Preserve pending NCR status/message completion in an explicit
  transaction state with target, `DSP`, `DSA`, data phase, status byte, and
  saved completion `DSPS`, instead of loose globals.
- [x] Latch NCR `SIST0.MIA` phase mismatch when a pending DATA phase is skipped
  by the target in favor of STATUS.
- [x] Honor NCR `DIEN`, `SIEN0`, and `SIEN1` interrupt masks while preserving
  latched `DSTAT`/`SIST` status.
- [x] Add common SCSI-2 disk responses, per-target REQUEST SENSE, MODE SENSE
  pages, read/write paths, write-protect handling, and harmless disk no-ops.
- [x] Include SCSI MODE SENSE block descriptors unless the CDB sets DBD.
- [x] Add additional SCSI MODE SENSE pages for disconnect/reconnect, format
  device, and control-page probes.
- [x] Honor MODE SENSE page-control `PC=1` by returning page layouts with
  non-changeable fields zeroed.
- [x] Add common extended SCSI disk probes: `READ(12)`, `WRITE(12)`,
  `REPORT LUNS`, and `READ CAPACITY(16)`.
- [x] Add additional SCSI disk probes and 16-byte commands: `FORMAT UNIT`,
  `RECEIVE DIAGNOSTIC RESULTS`, `READ LONG`, `READ(16)`, `WRITE(16)`,
  `WRITE AND VERIFY(16)`, `VERIFY(16)`, and `SYNCHRONIZE CACHE(16)`.
- [x] Add SCSI `READ FORMAT CAPACITIES` and `READ DEFECT DATA(12)` responses
  for older disk geometry/defect-list probes.
- [x] Add INQUIRY EVPD pages 0x00, 0x80, and 0x83 for supported-page,
  per-target serial, and device-identification probes.
- [x] Classify SCSI CDB data phases explicitly so no-data commands no longer
  get a synthetic DATA IN phase, and consume harmless DATA OUT parameter lists.
- [x] Link SIMH `sim_scsi` into the alpha simulator, expose DKA targets as
  `SCSI_DEV` disk units, and attach/detach DKA images through the common
  `sim_disk` backend while preserving the exact recovered image capacities.
- [x] Add a Mikasa NCR `sim_scsi` bus and route safe SCSI-2 commands through
  it (`TEST UNIT READY`, reserve/release, start/stop, and prevent/allow),
  while keeping SRM/VMS-sensitive `INQUIRY`, `READ CAPACITY`, READ/WRITE,
  MODE SENSE, VPD, and REQUEST SENSE paths in the local shim.
- [x] Correct NCR SCRIPTS transfer-control condition polarity: in fetched
  opcodes bit 19 represents the encoded `IFTRUE` sense, while the cleared bit
  is the XOR-produced `IFFALSE` form used by real scripts.
- [x] Execute deferred NCR status/message completion by running the firmware
  control script with STATUS as the current phase, then transition the script
  state to MESSAGE IN after the status byte instead of synthesizing an
  intermediate `DSPS`.
- [x] Return a valid standard INQUIRY "no logical unit present" response for
  non-zero LUNs instead of treating every non-zero LUN INQUIRY as check
  condition.
- [x] Add DECchip 21040/Tulip PCI/CSR shell.
- [x] Add DECchip 21040 software reset, CSR5 write-one-to-clear status,
  CSR6 run-state reporting, and byte/word CSR write merging.
- [x] Add DECchip 21040 CSR9 EEPROM/SROM bit-bang reads with a stable DEC OUI
  MAC address for early Ethernet driver probes.
- [x] Add DECchip 21040 legacy 32-byte Ethernet Address ROM reads and enough
  transmit-descriptor ownership completion for SRM setup frames to clear.
- [x] Re-run SRM ROM smoke after the current APECS/NCR hardware batch; it
  still reaches the Mikasa `V5.4-101` banner and sees pka/ewa.
- [x] Re-run SRM ROM smoke after the NCR progress/status and extended SCSI
  probe batch; it still reaches `V5.4-101` and detects pka/ewa.
- [x] Re-run SRM ROM smoke after the NCR connected-state update; it still
  reaches `V5.4-101` and detects pka/ewa.
- [x] Re-run SRM ROM smoke after explicit SCSI CDB data-phase classification;
  it still reaches `V5.4-101` and detects pka/ewa.
- [x] Re-run SRM ROM smoke after the first NCR `SIST0.MIA` phase-mismatch
  path; it still reaches `V5.4-101` and detects pka/ewa.
- [x] Re-run SRM ROM smoke after suppressing auxiliary NCR scan side effects;
  it still reaches `V5.4-101` and detects pka/ewa.
- [x] Re-run SRM ROM smoke after SCRIPTS `SET`/`CLR` ACK/ATN/target handling;
  it still reaches `V5.4-101` and detects pka/ewa.
- [x] Re-run SRM ROM smoke after SCRIPTS `SELECT` register latching; it still
  reaches `V5.4-101` and detects pka/ewa.
- [x] Re-run SRM ROM smoke after SCRIPTS wait-disconnect/wait-reselect
  handling; it still reaches `V5.4-101` and detects pka/ewa.
- [x] Re-run SRM ROM smoke after consuming NCR `MESSAGE OUT`; it still reaches
  `V5.4-101` and detects pka/ewa.
- [x] Re-run SRM ROM smoke after NCR `CTEST3`/FIFO-empty handling; it still
  reaches `V5.4-101` and detects pka/ewa.
- [x] Re-run SRM ROM smoke after NCR `SFBR` input-phase updates; it still
  reaches `V5.4-101` and detects pka/ewa.
- [x] Re-run SRM ROM smoke after exposing NCR `SSTAT0.WOA`; it still reaches
  `V5.4-101` and detects pka/ewa.
- [x] Re-run SRM ROM smoke after honoring NCR `DMODE.MAN`; it still reaches
  `V5.4-101` and detects pka/ewa.
- [x] Re-run SRM ROM smoke after honoring NCR `DCNTL.SSM` autostart gating;
  it still reaches `V5.4-101` and detects pka/ewa.
- [x] Re-run SRM ROM smoke after initializing the integrated NCR PCI
  interrupt line to IRQ 12; it still reaches `V5.4-101` and detects pka/ewa.
- [x] Re-run SRM ROM smoke after adding SCSI MODE SENSE pages 0x02/0x03/0x0A;
  it still reaches `V5.4-101` and detects pka/ewa.
- [x] Re-run SRM ROM smoke after honoring MODE SENSE changeable-page probes;
  it still reaches `V5.4-101` and detects pka/ewa.
- [x] Re-run direct APB smoke after the SCSI/PCI hardware batch; APB still
  loads from DKA0 and reaches the known `PC: 200039E0` halt.
- [x] Re-run SRM ROM smoke after `READ FORMAT CAPACITIES` and
  `READ DEFECT DATA(12)` support; it still reaches `V5.4-101` and detects
  pka/ewa.
- [x] Re-run SRM ROM smoke after EPIC absent-PCI-memory `NDEV` latching; it
  still reaches `V5.4-101` and detects pka/ewa.
- [x] Re-run direct APB smoke after EPIC `NDEV` latching; APB still loads from
  DKA0 and reaches the known `PC: 200039E0` halt.
- [x] Re-run SRM ROM smoke after unifying auxiliary NCR SCRIPTS scans; it
  still reaches `V5.4-101` and detects pka/ewa.
- [x] Re-run SRM ROM smoke after adding explicit pending NCR transaction
  state; it still reaches `V5.4-101` and detects pka/ewa.
- [x] Re-run SRM ROM smoke after adding visible 53C810 register semantics;
  it still reaches `V5.4-101` and detects pka/ewa.
- [x] Re-run SRM ROM smoke after making auxiliary NCR scans silent for
  fetch-progress registers; it still reaches `V5.4-101` and detects pka/ewa.
- [x] Re-run SRM ROM smoke after adding NCR `DCNTL.SSM` single-step execution;
  it still reaches `V5.4-101` and detects pka/ewa.
- [x] Re-run SRM ROM smoke after preserving NCR `WAIT RESELECT`/`SIGP` wait
  state; it still reaches `V5.4-101` and detects pka/ewa.
- [x] Re-run SRM ROM smoke after latching NCR `DSTAT` causes and surfacing
  script failures as `DSTAT.BF`; it still reaches `V5.4-101` and detects
  pka/ewa.
- [x] Re-run SRM ROM smoke after adding more NCR writable register masks and
  `DCNTL.IRQD`; it still reaches `V5.4-101` and detects pka/ewa.
- [x] Re-run SRM ROM smoke after exposing low-level NCR data latches
  (`SIDL`/`SODL`/`SBDL`); it still reaches `V5.4-101` and detects pka/ewa.
- [x] Re-run SRM ROM smoke after parsing common NCR `MESSAGE OUT` messages; it
  still reaches `V5.4-101` and detects pka/ewa.
- [x] Re-run SRM ROM smoke after adding more SCSI disk probe and 16-byte CDB
  coverage; it still reaches `V5.4-101` and detects pka/ewa.
- [x] Re-run SRM ROM smoke after adding low-level NCR `SCNTL0.START` and
  `SCNTL1.RST`; it still reaches `V5.4-101` and detects pka/ewa.
- [x] Re-run SRM ROM smoke after NCR `DSTAT.IID` zero-count MOVE handling; it
  still reaches `V5.4-101` and detects pka/ewa.
- [x] Re-run SRM ROM smoke after adding NCR interrupt stacking; it still
  reaches `V5.4-101` and detects pka/ewa.
- [x] Re-run SRM ROM smoke after reflecting NCR connected/bus-free state in
  `SBCL`; it still reaches `V5.4-101` and detects pka/ewa.
- [x] Re-run SRM ROM smoke after detecting SCRIPTS Load/Store-to-register
  `DSTAT.IID`; it still reaches `V5.4-101` and detects pka/ewa.
- [x] Re-run SRM ROM smoke after tightening `DCNTL.PFF` and `STIME1.GEN`;
  it still reaches `V5.4-101` and detects pka/ewa.
- [x] Re-run SRM ROM smoke after making `DFIFO.BO[6:0]` match the empty-FIFO
  model; it still reaches `V5.4-101` and detects pka/ewa.
- [x] Re-run SRM ROM smoke after mirroring NCR BAR register aliases; it still
  reaches `V5.4-101` and detects pka/ewa.
- [x] Re-run SRM ROM smoke after preserving NCR low-level read-only bus/latch
  registers and routing `SODL` writes; it still reaches `V5.4-101` and detects
  pka/ewa.
- [x] Re-run SRM ROM smoke after tightening NCR register masks and
  `STEST3.CSF`; it still reaches `V5.4-101` and detects pka/ewa.
- [x] Re-run SRM ROM smoke after separating NCR Load/Store from Memory Move;
  it still reaches `V5.4-101` and detects pka/ewa.
- [x] Re-run SRM ROM smoke after enforcing NCR Load/Store register-window and
  `SFBR` restrictions; it still reaches `V5.4-101` and detects pka/ewa.
- [x] Re-run SRM ROM smoke after preserving NCR `ADDER` and adding
  `CTEST5.ADCK`/`BBCK` pulse side effects; it still reaches `V5.4-101` and
  detects pka/ewa.
- [x] Re-run SRM ROM smoke after masking NCR `SIEN1` to documented bits; it
  still reaches `V5.4-101` and detects pka/ewa.
- [x] Re-run SRM ROM smoke after initial `sim_scsi` wiring; all four ODS2
  dumps attach without size errors, and SRM still reaches `V5.4-101` and
  detects pka/ewa.
- [x] Re-run SRM ROM smoke after fixing NCR SCRIPTS condition polarity and
  deferred STATUS/MESSAGE handling; SRM still reaches `V5.4-101`, pka/ewa are
  detected, and the old post-status `DSTAT.BF`/`DSPS=0x401427F0` failure is
  gone. The remaining blocker is repeated SCSI `INQUIRY` discovery loops with
  no `READ CAPACITY` yet.
- [x] Re-run SRM ROM smoke after PIC/ELCR edge-vs-level handling; it still
  reaches `V5.4-101` and detects pka/ewa.
- [x] Keep `make alpha -j$(nproc)` and `git diff --check` passing after each
  committed code block.

## Partial / In Progress

- [ ] Replace the NCR SCRIPTS scanner with a real executor for conditional
  control flow, phase mismatch, disconnect/reselect, scatter/gather chaining,
  and full memory/register side effects. The current helper scans now share
  one pass and deferred STATUS/MESSAGE control scripts now execute through the
  script interpreter, but command execution is still a high-level SCSI
  transaction frontend rather than a complete instruction-by-instruction
  53C810 engine.
- [ ] Continue expanding the NCR disk path onto SIMH `sim_scsi` where it fits.
  The current branch has the shared bus/device/attach layer and routes only
  conservative no-data status commands through `sim_scsi`; SRM/VMS-sensitive
  probe and disk commands still use the local shim.
- [ ] Debug the current SRM post-banner discovery loop: after the latest NCR
  fixes SRM repeatedly issues standard `INQUIRY` to targets 0-3 and LUNs 0-7,
  then select-times-out targets 4-6. It has not yet advanced to `READ
  CAPACITY` or a visible `>>>` prompt.
- [ ] Complete APECS/DECchip 21071 behavior beyond the current register shells,
  especially actually raising error causes, remaining HAE/config details, and
  DMA corner cases.
- [ ] Validate ICU/PIC interrupt behavior against OpenVMS and FreeBSD drivers
  once OS boot reaches live interrupt use. The 8259 ELCR edge-vs-level model
  is now implemented, but still needs validation under the target OS.
- [ ] Identify the real AlphaServer 1000 NVRAM layout and encode SRM console
  defaults such as console mode, auto_action, boot device, and boot flags.
- [ ] Improve OCP/Halt/Ctrl-P behavior enough for SRM console transitions.
- [ ] Decide whether VGA/Cirrus probing is required for the real AS1000 path.
- [ ] Implement real DECchip 21040 receive path, packet I/O, and full
  descriptor processing beyond the current SRM setup-frame transmit shell.
- [ ] Re-run SRM/APB smoke tests after the next hardware batch.
- [ ] Continue the direct-APB `SYSBOOT.EXE;2` lookup investigation under
  `[SYS0.SYSEXE]` and `[SYS0.SYSCOMMON.SYSEXE]`.

## Not Started / Deferred

- [ ] Full VGA implementation.
- [ ] Full floppy data path.
- [ ] Multiprocessor support.
- [ ] Redistributable regression tests that do not require the recovered disk
  dump or proprietary SRM image.

# AlphaServer 1000 Hardware Bring-Up TODO

This file tracks the next Mikasa work before more boot-chasing.  Until the
hardware base is credible, boot attempts are smoke tests only, not the main
debug method.

## Development Rule

Prefer complete, documented hardware behavior over trace-specific shortcuts.
Temporary shims are acceptable only as named diagnostics or stepping stones.
When adding one, document which real hardware behavior it approximates, which
manual or source reference should replace it, and which acceptance test proves
the real path works.

## P0 Hardware Base

- Replace the current NCR/Symbios 53C810 command shim with a real-enough
  SCRIPTS/DMA engine:
  - current branch follows simple unconditional script flow and decodes
    `SEL_ABS`, `SEL_TBL`, and table-indirect command/data/status/message moves;
  - continue replacing the scanner with actual conditional SCRIPTS execution,
    phase mismatch handling, and memory moves;
  - current branch models the main `DSP`, `DSPS`, `DSTAT`, `SIST0`, `SIST1`,
    `ISTAT`, SIR/DIP/SIP, abort, and select-timeout paths used so far;
  - current branch mirrors 53C810 operating registers through both documented
    halves of the 256-byte I/O and memory BAR windows;
  - current branch latches `DSTAT`/`SIST` causes independently of
    `DIEN`/`SIEN` masks and only asserts IRQ for enabled causes;
  - current branch has a second-level interrupt stack behind `DSTAT`,
    `SIST0`, and `SIST1`, including stacked `DSPS` for DMA interrupt causes;
  - current branch raises `DSTAT.IID` for zero-byte direct Block Move and
    Memory Move instructions, and clears `SCNTL2.SDU` after selection
    timeouts;
  - current branch raises `DSTAT.IID` when SCRIPTS Load/Store memory targets
    the controller's own memory-mapped register window;
  - current branch decodes Load/Store separately from Memory Move using the
    documented three-bit instruction type, keeps Load/Store at two longwords,
    enforces Load/Store alignment/count limits, blocks Load/Store register
    window targets, keeps `SFBR` from being loaded by Load/Store, and lets
    Memory Move reach NCR register windows while loading visible `TEMP`;
  - current branch compares Transfer Control phases using the documented
    `26..24` phase field and can run non-SELECT control scripts through a
    halting `INT` or wait state;
  - current branch preserves read-only `ADDER`, updates `ADDER`/`DNAD`/`DBC`
    from `CTEST5.ADCK` and `CTEST5.BBCK` pulse writes, and auto-clears those
    pulse bits;
  - current branch masks `SIEN1` to the documented `STO`, `GEN`, and `HTH`
    interrupt enable bits;
  - current branch collects multiple data-phase MOVE entries and reads/writes
    SCSI payloads across scatter/gather segment lists;
  - current branch updates visible SCRIPTS/DMA progress registers for handled
    instruction fetches and completed command/data/status/message MOVE
    segments;
  - current branch uses SCRIPTS `INT` instruction arguments for `DSPS`
    completion values when they can be found, rather than always returning
    synthetic phase markers;
  - current branch lets SCRIPTS `INTFLY` continue execution while setting
    `ISTAT.INTF`, which the CPU can clear with the documented write-one path;
    auxiliary scans suppress those side effects, as well as register-op,
    load/store, and memory-copy writes;
  - current branch handles documented `SET`/`CLR` side effects for carry,
    ACK/ATN output latches, sampled bus bits, and target-mode selection;
  - current branch records SCRIPTS `SELECT` destination ID, table-indirect
    transfer settings, and select-with-ATN state in `SDID`, `SXFER`, `SCNTL3`,
    `SOCL`, and `SBCL`;
  - current branch treats `WAIT DISCONNECT` as a legal bus-free transition and
    stops script scans at `WAIT RESELECT` until real reselect events exist;
  - current branch consumes message-out buffers before command execution and
    clears ATN after that handshake;
  - current branch preserves the `CTEST3` revision nibble and handles DMA FIFO
    clear/flush writes consistently with the FIFO-empty model;
  - current branch reports `DFIFO.BO[6:0]` as the low seven bits of `DBC`, so
    documented FIFO residual calculations see an empty FIFO;
  - current branch updates `SFBR` from handled data-in, status, and message-in
    phases so SCRIPTS tests see the first received byte;
  - current branch preserves read-only low-level bus/latch registers (`SSID`,
    `SBCL`, `SIDL`, and `SBDL`) on host/SCRIPTS writes, while `SODL` writes
    update the output and bus data latches;
  - current branch masks `SCNTL3`, `SCID`, `SDID`, `GPREG`, `GPCNTL`, and
    `STEST3` writes to documented 53C810 fields and treats `STEST3.CSF` as a
    self-clearing SCSI FIFO/latch-full flag clear;
  - current branch exposes the won-arbitration status bit after SCRIPTS
    selection attempts;
  - current branch honors `DMODE.MAN`, requiring `DCNTL.STD` to start SCRIPTS
    when manual-start mode is enabled;
  - current branch auto-clears `DCNTL.PFF` and masks `STIME1` to the documented
    `GEN[3:0]` field;
  - current branch also requires `DCNTL.STD` in `DCNTL.SSM` single-step mode
    instead of auto-starting from `DSP` writes;
  - current branch starts SCRIPTS from `DCNTL.STD` as well as the firmware-style
    `DSP` high-byte write;
  - current branch models low-level `SCNTL0.START` manual selection and
    `SCNTL1.RST` SCSI bus reset side effects;
  - current branch preserves read-only NCR status registers and reports the
    active MOVE phase in the SCSI bus/status phase bits;
  - current branch tracks connected state in `SCNTL1.ISCON` and `ISTAT.CON`
    during the high-level 53C810 target transaction;
  - current branch reflects connected/bus-free state in `SBCL` by asserting
    `BSY` while connected and clearing bus control lines on bus free;
  - current branch latches `SIST0.MIA` for the handled case where a target
    returns STATUS instead of an expected DATA phase, while leaving
    STATUS/MESSAGE pending for the next script restart;
  - current branch returns common SCSI-2 disk responses, write-protect check
    conditions, and per-target REQUEST SENSE state;
  - current branch includes MODE SENSE block descriptors unless disabled by
    the CDB, which better matches older direct-access disk drivers;
  - current branch returns additional MODE SENSE pages 0x02, 0x03, and 0x0A
    for older SCSI disk driver geometry/control probes;
  - current branch honors MODE SENSE page-control `PC=1` for changeable-page
    probes by returning page layouts with value fields zeroed;
  - SRM smoke after MODE SENSE changeable-page handling still reaches
    `V5.4-101` and enumerates `pka`/`ewa`;
  - SRM smoke after the additional MODE SENSE pages still reaches `V5.4-101`
    and enumerates `pka`/`ewa`;
  - SRM smoke after MODE SENSE descriptors still reaches `V5.4-101` and
    enumerates `pka`/`ewa`;
  - current branch returns basic INQUIRY EVPD supported-page, unit-serial, and
    device-identification data for generic SCSI probes;
  - current branch reports formatted 512-byte media for `READ FORMAT
    CAPACITIES` and empty defect lists for `READ DEFECT DATA(10/12)`;
  - current branch handles more SCSI disk probes and 16-byte commands:
    `FORMAT UNIT`, `RECEIVE DIAGNOSTIC RESULTS`, `READ LONG`, `READ(16)`,
    `WRITE(16)`, `WRITE AND VERIFY(16)`, `VERIFY(16)`, and `SYNCHRONIZE
    CACHE(16)`;
  - SRM smoke after the SCSI capacity/defect-list probe responses still
    reaches `V5.4-101` and enumerates `pka`/`ewa`;
  - current branch classifies CDB data phases explicitly and consumes
    data-out parameter lists for harmless no-op disk commands;
  - current branch links SIMH `sim_scsi`, exposes DKA targets as `SCSI_DEV`
    disk units, uses the common `sim_disk` attach/detach backend, and routes
    only conservative no-data status commands through a Mikasa NCR `sim_scsi`
    bus;
  - continue moving compatible READ/WRITE and SCSI probe paths to `sim_scsi`
    only where the common backend can preserve VMS-visible behavior;
  - keep target-absent timeout behavior clean with and without attached disks.
- Complete the APECS/DECchip 21071 base enough for firmware and operating
  systems:
  - add the Comanche register block at `0x180000000`;
  - current branch preserves EPIC HAXR0/HAXR1/HAXR2/PMLT, window
    base/mask/TBASE, TLB tag/data, and TBIA state;
  - current branch decodes sparse-memory device BARs against the full
    `HAXR1`-extended PCI address, so firmware BARs at `0x81000000` reach the
    NCR/Tulip register windows instead of becoming false `EPIC.NDEV` accesses;
  - current branch translates direct windows and SGMAP windows through TBASE
    by reading the programmed scatter/gather table and filling the EPIC TLB;
  - current branch masks Comanche/EPIC error registers and handles
    write-one-to-clear error/status bits, masks documented Comanche control
    and memory-bank fields, and keeps presence-detect registers read-only, but
    does not yet raise all real error causes;
  - current branch latches EPIC `DCSR.NDEV` plus PEAR/SEAR addresses for
    absent PCI sparse/dense memory accesses;
  - SRM smoke after EPIC `NDEV` latching still reaches `V5.4-101` and
    enumerates `pka`/`ewa`;
  - continue tightening HAE/config-cycle behavior, sparse I/O, sparse memory,
    dense memory, error registers, and DMA corner cases;
  - keep PCI DMA translation tied to the programmed APECS windows rather than
    hard-coded boot assumptions.
- Tighten interrupt routing:
  - keep AS1000 ICU bit 12 -> EV4 hardware IRQ2 -> SRM vector `0x9C0` for NCR;
  - current branch initializes the integrated NCR PCI interrupt-line register
    to IRQ 12 instead of the generic `0xff` unassigned value;
  - SRM smoke after the NCR PCI interrupt-line update still reaches
    `V5.4-101` and enumerates `pka`/`ewa`;
  - keep COM1 IRQ4 through the 8259/PIC IACK path;
  - current branch handles PIC init sequencing, ICW4 auto-EOI, ELCR
    edge/level-trigger state, mask/pending/in-service state, EOI, and cascade
    IACK paths;
  - current branch masks ELCR reserved bits to AT-compatible writable lines,
    latches edge IRQs on rising edges only, tracks level IRQ inputs, and
    reasserts active level IRQs after EOI;
  - continue validating interrupt behavior against OpenVMS and FreeBSD drivers.

## P1 Firmware Platform Devices

- Make the ISA/EISA base less synthetic:
  - improve Intel 82375EB/PCEB config space and EISA CRAM handling;
  - add ISA DMA page registers and harmless behavior for common DMA channels;
  - current branch includes a minimal 8042 keyboard-controller shell for
    command-byte, output-port, self-test, keyboard reset, ID, ACK, and common
    parameter-command probes without changing the serial-console path;
  - return documented absent-slot behavior for EISA probes.
- Implement MC146818 RTC/NVRAM more fully:
  - current branch has host-backed time-of-day, status A/B/C/D,
    update/periodic/alarm flags, interrupt behavior, and optional file-backed
    128-byte NVRAM storage;
  - plausible defaults for `console serial`, `auto_action`, boot device, and
    boot flags still require the AS1000 NVRAM layout.
- Improve OCP/Halt/Ctrl-P handling:
  - model the ready/status bits and Halt button behavior expected by SRM;
  - make console-entry decisions observable in debug traces.

## P2 Enumerated Devices

- Add PCI config-shell devices that the AlphaServer 1000 firmware may expect:
  - current branch exposes a DECchip 21040 Ethernet PCI/CSR shell at raw IDSEL
    `11`, including software reset, status W1C, and CSR6 run-state reporting;
    CSR9 EEPROM/SROM serial reads now return a deterministic station address,
    but packet I/O and real Tulip descriptor processing are still deferred;
  - VGA/Cirrus shell only if firmware probes require it;
  - floppy/FDC shell only enough for `DVA0` probing.
- Defer full Ethernet, VGA, and floppy data paths until SRM reaches a prompt
  and disk boot needs them.

## References To Use First

- `references/ncr53c810/` for NCR/LSI 53C810 SCRIPTS, DMA, and interrupt
  semantics.
- `references/freebsd/src-6.4.0/sys/alpha` for AS1000, APECS, interrupt, and
  platform behavior.
- `../firmware/as1000/EK-AXPFW-RM-B01.pdf.txt` for SRM-visible AS1000 device
  layout and console behavior.
- `../sources` for OpenVMS bootstrap/HWRPB/PAL expectations when the firmware
  manuals are not enough.

## Acceptance Tests

- `make alpha -j$(nproc)`
- `git diff --check`
- SRM ROM smoke test still reaches at least:

```text
V5.4-101, built on Mar 24 1999 at 13:58:27
```

- With `SET MIKASA DEBUG=IO,PCI,RTC,UART`, unhandled ISA/APECS accesses should
  decrease, not move to a new port loop.
- With four disks attached, SRM must not spin forever on the same target 0-6
  `INQUIRY` scan.
- With no disks attached, target timeouts must complete cleanly instead of
  causing an endless low-level controller loop.
- The direct APB diagnostic must not regress before the known
  `SYSBOOT.EXE` lookup stop.
