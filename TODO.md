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
- [x] Mask APECS Comanche/EPIC error/status writes and add write-one-to-clear
  behavior for documented error latches.
- [x] Merge byte/word PCI config writes instead of replacing whole dwords.
- [x] Add Intel 82375EB/PCEB PCI/EISA bridge config shell with write masks.
- [x] Add ISA DMA/page-register storage, FDC shell, OCP shell, RTC basics, and
  EISA CRAM/absent-slot behavior.
- [x] Add optional file-backed 128-byte RTC/NVRAM image support with default
  RTC values and alarm/update/periodic interrupt state.
- [x] Add 8259 PIC init, mask/pending/in-service, EOI, auto-EOI, cascade IACK,
  and ELCR storage.
- [x] Route COM1 receive interrupts through the SRM-programmed PIC path.
- [x] Add NCR/Symbios 53C810 PCI config, I/O BAR, memory BAR, register shell,
  interrupt status, abort, reset, and select-timeout behavior.
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
- [x] Treat NCR SCRIPTS `INTFLY` as a non-halting interrupt-on-the-fly path and
  expose/clear `ISTAT.INTF`.
- [x] Keep NCR `INTFLY` side effects out of auxiliary script scans so debug
  searches do not reassert `ISTAT.INTF`.
- [x] Suppress auxiliary NCR scan side effects for SCRIPTS register ops,
  load/store, and memory-copy instructions while preserving local SFBR/carry
  state needed for conditions.
- [x] Implement documented NCR SCRIPTS `SET`/`CLR` side effects for carry,
  `SOCL` ACK/ATN outputs, mirrored bus bits, and `SCNTL0` target mode.
- [x] Latch NCR SCRIPTS `SELECT` destination, table-indirect `SXFER`/`SCNTL3`,
  and select-with-ATN state into visible 53C810 registers.
- [x] Model visible NCR SCRIPTS wait-state behavior for legal disconnect and
  no-event wait-reselect paths.
- [x] Consume NCR `MESSAGE OUT` moves before command execution and clear ATN
  after the message-out handshake.
- [x] Start NCR SCRIPTS execution from `DCNTL.STD`, not only from writes to
  `DSP`, matching the normal 53C810 driver path.
- [x] Preserve NCR read-only status registers on CPU writes and expose current
  MOVE phase through `SOCL`, `SBCL`, and `SSTAT1`.
- [x] Track NCR connected state in `SCNTL1.ISCON`/`ISTAT.CON` across select,
  command, data, status/message, timeout, reset, and abort paths.
- [x] Latch NCR `SIST0.MIA` phase mismatch when a pending DATA phase is skipped
  by the target in favor of STATUS.
- [x] Honor NCR `DIEN`, `SIEN0`, and `SIEN1` interrupt masks while preserving
  latched `DSTAT`/`SIST` status.
- [x] Add common SCSI-2 disk responses, per-target REQUEST SENSE, MODE SENSE
  pages, read/write paths, write-protect handling, and harmless disk no-ops.
- [x] Add common extended SCSI disk probes: `READ(12)`, `WRITE(12)`,
  `REPORT LUNS`, and `READ CAPACITY(16)`.
- [x] Add INQUIRY EVPD pages 0x00, 0x80, and 0x83 for supported-page,
  per-target serial, and device-identification probes.
- [x] Classify SCSI CDB data phases explicitly so no-data commands no longer
  get a synthetic DATA IN phase, and consume harmless DATA OUT parameter lists.
- [x] Add DECchip 21040/Tulip PCI/CSR shell.
- [x] Add DECchip 21040 software reset, CSR5 write-one-to-clear status,
  CSR6 run-state reporting, and byte/word CSR write merging.
- [x] Add DECchip 21040 CSR9 EEPROM/SROM bit-bang reads with a stable DEC OUI
  MAC address for early Ethernet driver probes.
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
- [x] Keep `make alpha -j$(nproc)` and `git diff --check` passing after each
  committed code block.

## Partial / In Progress

- [ ] Replace the NCR SCRIPTS scanner with a real executor for conditional
  control flow, phase mismatch, disconnect/reselect, scatter/gather chaining,
  and full memory/register side effects.
- [ ] Wire the NCR disk path to SIMH `sim_scsi` where it fits, rather than the
  current local SCSI command handling.
- [ ] Complete APECS/DECchip 21071 behavior beyond the current register shells,
  especially actually raising error causes, remaining HAE/config details, and
  DMA corner cases.
- [ ] Validate ICU/PIC level-vs-edge semantics against OpenVMS and FreeBSD
  drivers.
- [ ] Identify the real AlphaServer 1000 NVRAM layout and encode SRM console
  defaults such as console mode, auto_action, boot device, and boot flags.
- [ ] Improve OCP/Halt/Ctrl-P behavior enough for SRM console transitions.
- [ ] Decide whether VGA/Cirrus probing is required for the real AS1000 path.
- [ ] Implement real DECchip 21040 packet I/O and Tulip descriptor processing.
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
  - current branch latches `DSTAT`/`SIST` causes independently of
    `DIEN`/`SIEN` masks and only asserts IRQ for enabled causes;
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
  - current branch starts SCRIPTS from `DCNTL.STD` as well as the firmware-style
    `DSP` high-byte write;
  - current branch preserves read-only NCR status registers and reports the
    active MOVE phase in the SCSI bus/status phase bits;
  - current branch tracks connected state in `SCNTL1.ISCON` and `ISTAT.CON`
    during the high-level 53C810 target transaction;
  - current branch latches `SIST0.MIA` for the handled case where a target
    returns STATUS instead of an expected DATA phase, while leaving
    STATUS/MESSAGE pending for the next script restart;
  - current branch returns common SCSI-2 disk responses, write-protect check
    conditions, and per-target REQUEST SENSE state;
  - current branch returns basic INQUIRY EVPD supported-page, unit-serial, and
    device-identification data for generic SCSI probes;
  - current branch classifies CDB data phases explicitly and consumes
    data-out parameter lists for harmless no-op disk commands;
  - use SIMH `sim_scsi` as the backing SCSI command/device layer where it fits;
  - keep target-absent timeout behavior clean with and without attached disks.
- Complete the APECS/DECchip 21071 base enough for firmware and operating
  systems:
  - add the Comanche register block at `0x180000000`;
  - current branch preserves EPIC HAXR0/HAXR1/HAXR2/PMLT, window
    base/mask/TBASE, TLB tag/data, and TBIA state;
  - current branch translates direct windows and SGMAP windows through TBASE
    by reading the programmed scatter/gather table and filling the EPIC TLB;
  - current branch masks Comanche/EPIC error registers and handles
    write-one-to-clear error/status bits, but does not yet raise all real
    error causes;
  - continue tightening HAE/config-cycle behavior, sparse I/O, sparse memory,
    dense memory, error registers, and DMA corner cases;
  - keep PCI DMA translation tied to the programmed APECS windows rather than
    hard-coded boot assumptions.
- Tighten interrupt routing:
  - keep AS1000 ICU bit 12 -> EV4 hardware IRQ2 -> SRM vector `0x9C0` for NCR;
  - keep COM1 IRQ4 through the 8259/PIC IACK path;
  - current branch handles PIC init sequencing, ICW4 auto-EOI, ELCR storage,
    mask/pending/in-service state, EOI, and cascade IACK paths;
  - continue checking level/edge semantics against OpenVMS and FreeBSD drivers.

## P1 Firmware Platform Devices

- Make the ISA/EISA base less synthetic:
  - improve Intel 82375EB/PCEB config space and EISA CRAM handling;
  - add ISA DMA page registers and harmless behavior for common DMA channels;
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
