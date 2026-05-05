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
  - Loads the primary bootstrap at physical `0x20000000`.
  - Builds a minimal HWRPB at physical `0x10000000`.
  - Passes APB flags in `R5`.
  - Passes HWRPB physical address in `R16`.

Not implemented yet:

- Real Mikasa 21071/CIA PCI host bridge behavior.
- NCR/Symbios 53C810 PCI SCSI DMA engine.
- SRM console callbacks used by OpenVMS after APB starts.
- Full console terminal block/CRB in HWRPB.
- Network, VGA, NVRAM, and multiprocessor support.

So this is not yet a complete OpenVMS boot. It is a controlled first point:
SIMH can identify the machine profile, attach all four recovered disks, load the
same APB that AXPbox/AlphaVM loaded, and enter it with Mikasa-specific HWRPB
state instead of a DS10/ES40 state.

Current verified stop:

```text
Loaded OpenVMS Alpha APB from DKA0: LBN 4455271, 1049 blocks, 537088 bytes at 20000000

VMS/APB bugcheck, PC: 2000B2F0 (LDL R25,0(R3))
```

The APB reaches this after passing its first HWRPB platform checks. The next
missing piece is the firmware callback path that APB uses for console/disk
services, followed by real 53C810 SCSI once OpenVMS takes over.

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

## Next implementation steps

1. Replace the minimal HWRPB with the exact SRM layout APB expects.
   Required fields to tighten first:
   - console terminal block table
   - console callback routine block
   - processor slot PAL revision values
   - Dynamic System Recognition data block, if this OpenVMS release uses it

2. Add Mikasa platform I/O.
   Linux identifies Mikasa PCI interrupt summary/mask registers at ISA ports
   `0x534` and `0x536`, with NCR 810 SCSI on the PCI interrupt summary bit 12.

3. Add a new NCR/Symbios 53C810 frontend.
   SIMH has a common SCSI backend, but no 53C810 PCI DMA frontend in this tree.
   The implementation must be written cleanly for SIMH licensing; do not copy
   GPL code from AXPbox.

4. Wire the 53C810 frontend to the four DKA images.
   Once APB switches from firmware disk reads to OS disk I/O, the current raw
   loader path is not enough. OpenVMS will need the real SCSI controller model.

5. Add regression tests that do not require redistributing the disk dump.
   A small synthetic OpenVMS-style boot block can verify LBN/count parsing and
   APB placement.
