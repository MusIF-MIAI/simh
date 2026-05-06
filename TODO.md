# AlphaServer 1000 Bring-Up Checklist

## Done

- [x] Create and push the `alphaserver-1000` branch to `MusIF-MIAI/simh`.
- [x] Add the `MIKASA-4/266` CPU profile and 1 GB memory support.
- [x] Add Mikasa SRM LFU/payload ROM loading and `BOOT CPU` firmware entry.
- [x] Reach the real Mikasa SRM V5.4-101 banner from `mksrmrom.payload.bin`.
- [x] Add Comanche and EPIC APECS register shells.
- [x] Add APECS sparse I/O, sparse memory, dense memory, and PCI config paths.
- [x] Preserve EPIC DMA window registers and use them for PCI DMA translation.
- [x] Merge byte/word PCI config writes instead of replacing whole dwords.
- [x] Add Intel 82375EB/PCEB PCI/EISA bridge config shell with write masks.
- [x] Add ISA DMA/page-register storage, FDC shell, OCP shell, RTC basics, and
  EISA CRAM/absent-slot behavior.
- [x] Add 8259 PIC init, mask/pending/in-service, EOI, auto-EOI, cascade IACK,
  and ELCR storage.
- [x] Route COM1 receive interrupts through the SRM-programmed PIC path.
- [x] Add NCR/Symbios 53C810 PCI config, I/O BAR, memory BAR, register shell,
  interrupt status, abort, reset, and select-timeout behavior.
- [x] Add NCR SCRIPTS scanning for select, table/direct/indirect moves,
  register ops, load/store, memory copy, and basic conditional flow.
- [x] Add common SCSI-2 disk responses, per-target REQUEST SENSE, MODE SENSE
  pages, read/write paths, write-protect handling, and harmless disk no-ops.
- [x] Add DECchip 21040/Tulip PCI/CSR shell.
- [x] Keep `make alpha -j$(nproc)` and `git diff --check` passing after each
  committed code block.

## Partial / In Progress

- [ ] Replace the NCR SCRIPTS scanner with a real executor for conditional
  control flow, phase mismatch, disconnect/reselect, scatter/gather chaining,
  and full memory/register side effects.
- [ ] Wire the NCR disk path to SIMH `sim_scsi` where it fits, rather than the
  current local SCSI command handling.
- [ ] Complete APECS/DECchip 21071 behavior beyond the current register shells,
  especially error registers, TBIA/HAE details, and DMA corner cases.
- [ ] Validate ICU/PIC level-vs-edge semantics against OpenVMS and FreeBSD
  drivers.
- [ ] Finish MC146818 RTC/NVRAM storage and real AlphaServer 1000 console
  environment defaults.
- [ ] Improve OCP/Halt/Ctrl-P behavior enough for SRM console transitions.
- [ ] Decide whether VGA/Cirrus probing is required for the real AS1000 path.
- [ ] Implement real DECchip 21040 packet I/O and Tulip descriptor processing.
- [ ] Re-run SRM boot/probe smoke tests after the current hardware batch.
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
    scatter/gather chaining, phase mismatch handling, and memory moves;
  - current branch models the main `DSP`, `DSPS`, `DSTAT`, `SIST0`, `SIST1`,
    `ISTAT`, SIR/DIP/SIP, abort, and select-timeout paths used so far;
  - current branch returns common SCSI-2 disk responses, write-protect check
    conditions, and per-target REQUEST SENSE state;
  - use SIMH `sim_scsi` as the backing SCSI command/device layer where it fits;
  - keep target-absent timeout behavior clean with and without attached disks.
- Complete the APECS/DECchip 21071 base enough for firmware and operating
  systems:
  - add the Comanche register block at `0x180000000`;
  - tighten EPIC window registers, HAE, TBIA, base/mask/TBASE, sparse I/O,
    sparse memory, dense memory, and config-cycle behavior;
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
  - time-of-day, status A/B/C/D, update/periodic/alarm flags, and interrupt
    behavior;
  - 128-byte NVRAM storage with a documented default image;
  - plausible defaults for `console serial`, `auto_action`, boot device, and
    boot flags once the AS1000 NVRAM layout is known.
- Improve OCP/Halt/Ctrl-P handling:
  - model the ready/status bits and Halt button behavior expected by SRM;
  - make console-entry decisions observable in debug traces.

## P2 Enumerated Devices

- Add PCI config-shell devices that the AlphaServer 1000 firmware may expect:
  - current branch exposes a DECchip 21040 Ethernet PCI/CSR shell at raw IDSEL
    `11`; packet I/O and real Tulip descriptor processing are still deferred;
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
