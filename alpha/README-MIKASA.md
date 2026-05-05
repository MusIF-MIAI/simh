# AlphaServer 1000 "Mikasa" bring-up notes

This directory now contains the first SIMH bring-up slice for an
AlphaServer 1000 4/266, DEC platform name Mikasa.

## Current scope

Implemented:

- `SET CPU MIKASA-4/266`
  - CPU implementation version is exposed as EV4-class.
  - EV56+ architecture extensions are masked off for the 21064A/EV45 profile.
- `SET CPU 1G`
  - Added because the recovered Fisica dump was tested with 1024 MB under SRM.
- SRM-style raw disk devices:
  - `DKA0`
  - `DKA100`
  - `DKA200`
  - `DKA300`
- OpenVMS Alpha APB loader:
  - Reads LBN/count from the primary boot block offsets used by the dump.
  - Loads the primary bootstrap at physical `0x00200000`.
  - Enters it at virtual `0x20000000`, matching `EXE$K_PRIMBOOT`.
  - Builds a minimal HWRPB at physical `0x00002000`, visible at virtual
    `0x10000000`, matching `EXE$K_HWRPB`.
  - Builds bootstrap page tables and preloads EV5 ITB/DTB entries for HWRPB,
    APB, and the bootstrap page-table window.
  - Handles APB bootstrap TLB misses against the minimal boot page tables.
  - Handles the VMS queue/probe PAL calls reached by the APB bootstrap path.
  - Exposes CTB/CRB console callback descriptors and generic callback handlers
    for environment variables and DKA-backed open/read/write services.
  - Passes APB flags in `R5`.
  - Passes HWRPB physical address in `R16`.

Not implemented yet:

- Real Mikasa 21071/CIA PCI host bridge behavior.
- NCR/Symbios 53C810 PCI SCSI DMA engine.
- Network, VGA, NVRAM, and multiprocessor support.
- A complete SRM-compatible boot context. The callback path is present but the
  recovered APB does not reach it yet.

So this is not yet a complete OpenVMS boot. It is a controlled first point:
SIMH can identify the machine profile, attach all four recovered disks, load the
same APB that AXPbox/AlphaVM loaded, and enter it with Mikasa-specific HWRPB
state instead of a DS10/ES40 state.

Current verified stop:

```text
Loaded OpenVMS Alpha APB from DKA0: LBN 4455271, 1049 blocks, 537088 bytes at 00200000
%APB-F-MOUNT, failed to mount volume
HALT instruction, PC: 200037C0
```

The APB reaches this after passing its first HWRPB platform checks, using the
bootstrap virtual mapping, taking dynamic TLB misses, and executing VMS
queue/probe PAL calls. The current failure is still before the implemented CRB
callback counters move: `CALLBACKS`, `GETENVS`, `IOREADS`, and `IOWRITES`
remain zero.

Earlier stops were `R0=0x124` (`SS$_INSFMEM`) while mapping bootstrap memory
around the `0x40000000` boot page-table window, and then unsupported PAL calls
such as `PROBER` and `INSQUEQ`. The active blocker is now the APB mount path.
Disk 1 is an ODS-2 OpenVMS system disk (`AXPVMSSYS`, `DECFILE11B`, and
`[SYS0...]` strings are present), so the next work is to determine whether APB
needs a more exact SRM callback context or has switched to native NCR/Symbios
53C810 disk I/O.

## Fisica dump quick start

From the repository root:

```text
make alpha
BIN/alpha alpha/mikasa-fermi.ini
```

The config attaches the four extracted dump images read-only:

```text
DKA0    AXPVMSSYS   system disk
DKA100  FERMI_USR
DKA200  FERMI_USR1
DKA300  SPOOL
```

The boot command is:

```text
BOOT DKA0
```

For conversational OpenVMS boot, set APB bit 0 before booting:

```text
SET DKA0 OSFLAGS=1
BOOT DKA0
```

Useful status counters after a boot attempt:

```text
EX MIKASA CALLBACKS
EX MIKASA GETENVS
EX MIKASA IOREADS
EX MIKASA IOWRITES
```

## Persistent Debug Console

The debug config puts the SIMH remote console on a telnet socket. A small proxy
keeps that telnet connection open, so local clients can connect, disconnect,
inject commands, and reconnect without ending the SIMH-side session.

SIMH remote master mode also requires the simulator console to be telnet or
serial. `mikasa-fermi-debug.ini` therefore opens a buffered simulator console
on `23232`; normal debugging uses the remote console on `23230` through the
proxy on `23231`.

Start the proxy first:

```text
python3 alpha/tools/simh-console-proxy.py --simh-port 23230 --listen-port 23231
```

Start SIMH with the debug config:

```text
BIN/alpha alpha/mikasa-fermi-debug.ini
```

Then connect to the stable raw proxy port:

```text
nc 127.0.0.1 23231
```

The remote console accepts single commands directly. For one-shot command
injection:

```text
printf 'show cpu history\n' | nc 127.0.0.1 23231
```

Send Ctrl-E through the proxy to enter multi-command mode on the persistent
remote connection.

When disassembling the APB, remember that SIMH `EX -M` takes physical
addresses. APB virtual `0x200030A0` is physical `0x002030A0`, not
`0x200030A0`.

APB console output is not yet routed through the SIMH console callback path.
For the current stop, the useful history extraction is:

```text
timeout 120 bash -lc "printf 'set cpu history=20000\ndo alpha/mikasa-fermi.ini\nshow cpu history\nexit\n' | BIN/alpha > /tmp/mikasa_boot_history.txt"
perl -ane 'if ($F[0] eq "0000000020049E5C") { $c = chr(hex($F[1]) & 255); next if defined($last) && $last eq $c; print $c; $last = $c } END { print "\n" }' /tmp/mikasa_boot_history.txt
```

## SRM Firmware Images

The downloaded AlphaServer 1000 firmware set is outside this repository:

```text
../firmware/as1000/
```

For the recovered AlphaServer 1000 4/266, the SRM candidate is:

```text
../firmware/as1000/mksrmrom.exe  DEC/MIKASA/SRM v5.4-101
```

`alpha/tools/extract-axpbox-srm-rom.py` can parse DEC LFU ROM headers and can
drive AXPbox's ES40 self-decompression path. This is useful for the known ES40
ROM format:

```text
alpha/tools/extract-axpbox-srm-rom.py ../axpbox/cl67srmrom.exe /tmp/cl67.decompressed.rom
```

The same AXPbox EV68 path does not currently complete for `mksrmrom.exe`; with
debug tracing, the PC cycles inside the ROM decompressor around `0x900301` and
`0x902009`. The tool intentionally rejects chunk-limited output so partial
`decompressed.rom` files are not mistaken for usable SRM images.

## Next implementation steps

1. Understand the APB mount failure.
   Confirm whether the APB is attempting SRM callback I/O or native SCSI I/O.
   If callbacks are expected, fix the CRB/IOVEC/boot environment context until
   `CALLBACKS`, `GETENVS`, or `IOREADS` increment. If native SCSI is expected,
   move directly to Mikasa PCI/CIA/53C810 work.

2. Keep tightening the remaining SRM boot memory/page-table context.
   HWRPB VPTB/processor-slot `PTBR`/`PT_VA`, the MDDT cluster layout, and APB
   page-table behavior around `EXE$K_BOOTPT` remain useful checks. Dynamic TLB
   misses are handled for bring-up, but this is not complete SRM PALcode.

3. Tighten the remaining HWRPB/SRM fields.
   CTB/CRB and callback descriptors exist, but PAL revision fields, DSR data,
   and SWRPB/boot context are still minimal.

4. Add Mikasa platform I/O.
   Linux identifies Mikasa PCI interrupt summary/mask registers at ISA ports
   `0x534` and `0x536`, with NCR 810 SCSI on the PCI interrupt summary bit 12.

5. Add a new NCR/Symbios 53C810 frontend.
   SIMH has a common SCSI backend, but no 53C810 PCI DMA frontend in this tree.
   The implementation must be written cleanly for SIMH licensing; do not copy
   GPL code from AXPbox.

6. Wire the 53C810 frontend to the four DKA images.
   Once APB switches from firmware disk reads to OS disk I/O, the current raw
   loader path is not enough. OpenVMS will need the real SCSI controller model.

7. Add regression tests that do not require redistributing the disk dump.
   A small synthetic OpenVMS-style boot block can verify LBN/count parsing and
   APB placement.
