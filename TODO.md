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
  - execute the SRM PKE script flow instead of recognizing only the first
    selection/CDB pattern;
  - implement table-indirect DSA moves for command, data, status, and message
    phases;
  - model `DSP`, `DSPS`, `DSTAT`, `SIST0`, `SIST1`, `ISTAT`, SIR/DIP/SIP, and
    select timeout semantics from the NCR/LSI manuals;
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
  - model level/edge, mask, pending, in-service, EOI, and cascade behavior
    closely enough for SRM and OpenVMS drivers.

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
  - DECchip 21040 Ethernet at the documented slot when enabled;
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
