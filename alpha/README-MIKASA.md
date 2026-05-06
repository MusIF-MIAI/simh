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
- Mikasa platform hardware shell:
  - APECS/DECchip 21071 now exposes the Comanche `0x180000000` register
    block and the EPIC `0x1A0000000` register block through the same system
    DIB.
  - EPIC sparse PCI memory accesses now use `HAXR1` for high-address
    extension, and EPIC DMA window registers preserve the documented writable
    fields used by the PCI DMA mapper.
  - EPIC now models `HAXR0`, `HAXR2`, `PMLT`, the eight TLB tag/data entries,
    `TBIA`, direct PCI DMA windows, and SGMAP DMA windows backed by the
    programmed TBASE scatter/gather table.
  - Comanche and EPIC status/error register writes are masked to documented
    fields, with write-one-to-clear behavior for error latches.
  - Absent PCI sparse/dense memory accesses latch EPIC `DCSR.NDEV` with
    PEAR/SEAR error addresses.
  - ISA legacy DMA controller registers, ISA DMA page registers, and a minimal
    floppy-controller shell are present so standard I/O probes do not fall
    through to unhandled ports.
- Mikasa SRM firmware loader:
  - `SET MIKASA ROM=<path>` accepts the AlphaServer 1000 LFU SRM image
    `mksrmrom.exe` and AXPbox-style `decompressed.rom` files.
  - LFU images are not treated as raw ROM bytes. The loader validates the LFU
    header, skips the `0x240` byte wrapper, and starts the embedded SRM payload
    at `0x00900000`.
  - `SET MIKASA ROMPAYLOAD=<path>` loads a payload already extracted from the
    LFU wrapper. This is the preferred config path for repeatable local boots.
  - `BOOT CPU` starts the loaded SRM image, matching the VAX SIMH convention
    where CPU boot enters the platform firmware.
  - The real `mksrmrom.exe` now runs through ROM copy/decompression, probes
    PCI/EISA, passes the Mikasa ICU presence check, detects the on-board
    `pka` NCR 53C810 SCSI function, and prints the DEC/MIKASA SRM banner:

    ```text
    EPICbus 0, slot  6 -- pka -- NCR 53C810
    V5.4-101, built on Mar 24 1999 at 13:58:27
    ```

  - COM1 receive interrupts are now delivered through the SRM-programmed 8259
    path. SRM enables COM1 `IER=0x0B`, unmasks PIC IRQ4, the APECS PCI IACK
    cycle returns vector `0x04`, and the firmware reads the UART receive
    buffer.
  - `DEP MIKASA SCCSCALE <n>` can be used as a debug accelerator for SRM
    delay loops. The default is `1`, which preserves the normal PAL `RSCC`
    counter behavior.
  - `SET MIKASA NVRAM=<path>` loads or creates a 128-byte MC146818-compatible
    RTC/NVRAM image. RTC writes are persisted immediately when this is set.
- OpenVMS Alpha APB direct loader:
  - Reads LBN/count from the primary boot block offsets used by the dump.
  - Loads the primary bootstrap at physical `0x00200000`.
  - Enters it at virtual `0x20000000`, matching `EXE$K_PRIMBOOT`.
  - Builds a minimal HWRPB at physical `0x00380000`, visible at virtual
    `0x10000000`, matching `EXE$K_HWRPB`.
  - Builds bootstrap page tables and preloads EV5 ITB/DTB entries for HWRPB,
    APB, and the bootstrap page-table window.
  - Handles APB bootstrap TLB misses against the minimal boot page tables.
  - Handles the APB unaligned load/store traps reached while parsing ODS-2
    structures.
  - Emulates the APB memory mailbox disk path used before firmware callbacks
    are reached, including the DMA descriptor window used for multi-block
    reads.
  - Handles the VMS queue/probe PAL calls reached by the APB bootstrap path.
  - Exposes CTB/CRB console callback descriptors and generic callback handlers
    for environment variables and DKA-backed open/read/write services.
  - Passes APB flags in `R5`.
  - Passes HWRPB physical address in `R16`.

Not implemented yet:

- Complete real Mikasa 21071 PCI host bridge behavior. The current Comanche
  and EPIC models cover the register and DMA paths reached so far, but error
  cause generation, remaining HAE/config details, and PCI corner cases are
  still incomplete.
- Full NCR/Symbios 53C810 SCRIPTS/DMA execution. The current frontend handles
  simple script walking, `SEL_ABS`/`SEL_TBL`, table-indirect command/data/status
  moves, controller interrupt status, common SCSI-2 disk commands, and per-target
  REQUEST SENSE state. The integrated controller's PCI interrupt line is
  initialized to ICU IRQ 12, matching the AlphaServer 1000 platform route. Data
  payloads can now span multiple data-phase MOVE segments, but the frontend is
  still not a complete SCRIPTS processor.
  `DIEN`, `SIEN0`, and `SIEN1` now mask IRQ assertion while `DSTAT`/`SIST`
  status bits remain latched until read.
  Handled SCRIPTS fetches and command/data/status/message MOVE transfers now
  update visible `DBC`, `DNAD`, `DSP`, and `DSPS` progress registers.
  SCRIPTS `INT` completion interrupts now prefer the real second-word value
  from the script in `DSPS`, falling back to synthetic phase markers only when
  the current frontend cannot find a matching script interrupt. `DSTAT` causes
  are preserved until the CPU reads the status register, and unhandled
  fetch/DMA script paths raise `DSTAT.BF` instead of failing silently.
  `DSTAT`, `SIST0`, and `SIST1` now have a second-level interrupt stack, with
  the stacked `DSPS` value preserved for DMA interrupt causes.
  Zero-byte direct Block Move and Memory Move instructions raise `DSTAT.IID`,
  and select timeouts clear the expected-disconnect bit.
  `INTFLY` is treated as non-halting and sets `ISTAT.INTF`, with CPU
  write-one clearing, while auxiliary script scans suppress `INTFLY` side
  effects and no longer clobber visible `DBC`/`DSP`/`DSPS` fetch-progress
  registers. Auxiliary scans also suppress SCRIPTS register-op, load/store,
  and memory-copy writes while preserving local SFBR/carry state for
  conditions.
  `SELECT`, direct/table MOVE discovery, scatter/gather MOVE collection, and
  completion `INT` lookup now use a shared SCRIPTS scan pass so conditional
  flow and non-side-effect state stay consistent across the high-level NCR
  frontend.
  SCRIPTS `SET`/`CLR` now implement the documented carry, ACK, ATN, and
  target-mode side effects in `SOCL`, `SBCL`, and `SCNTL0`.
  SCRIPTS `SELECT` now latches the destination ID, table-indirect `SXFER` and
  `SCNTL3` values, and select-with-ATN state into the visible controller
  registers. `WAIT DISCONNECT` now clears connected state as a legal bus-free
  transition, sets `SSTAT2.LDSC`, and clears `SCNTL2.SDU`. `WAIT RESELECT`
  now records an explicit wait state; real reselect events are still not
  generated, but `ISTAT.SIGP` aborts the wait by jumping to the alternate
  address and `CTEST2` mirrors/clears the visible `SIGP` bit on read. `TEMP`
  is updated for SCRIPTS `CALL`/`RETURN`, host ID defaults are exposed through
  `SCID`/`RESPID`, and SCRIPTS register writes use the same writable masks for
  visible controller registers. Additional driver-probed registers now use
  writable masks (`CTEST2/5`, `DMODE`, `DCNTL`, `MACNTL`, `STIME1`,
  `STEST1/2/3`, and `SIEN1`), and `DCNTL.IRQD` suppresses IRQ output without
  clearing latched status. `MESSAGE OUT` buffers are now parsed before command
  execution: `IDENTIFY` records the LUN, queue tags are retained, extended
  messages are skipped cleanly, and ATN is cleared after the handshake.
  `CTEST3` now preserves the 53C810 revision nibble and handles FIFO
  clear/flush writes against the local FIFO-empty model. `SFBR`, `SIDL`, and
  `SBDL` are updated with the first byte of handled input phases, while
  command/data/message output phases update `SODL`/`SBDL`. SCRIPTS selection
  attempts now expose the won-arbitration status bit in `SSTAT0`.
  `DMODE.MAN`
  manual-start mode is honored, so `DSP` writes only auto-start SCRIPTS when
  manual-start is clear; `DCNTL.SSM` single-step mode also requires
  `DCNTL.STD` instead of auto-starting from `DSP`, and a single-step start now
  executes one visible SCRIPTS instruction before raising `DSTAT.SSI`.
  SCRIPTS execution can now be started through `DCNTL.STD` as well as by
  writing `DSP`. Low-level `SCNTL0.START` manual selection and `SCNTL1.RST`
  SCSI bus reset side effects are modeled for drivers that use programmed I/O
  control paths.
  CPU writes no longer overwrite read-only NCR status registers, and completed
  MOVE phases are reflected in `SOCL`, `SBCL`, and `SSTAT1`. The high-level
  NCR path also exposes connected state through `SCNTL1.ISCON` and `ISTAT.CON`
  from successful select through status/message completion, and clears it on
  timeout, reset, and abort. Pending status/message completion is retained in
  an explicit NCR transaction state with target, `DSP`, `DSA`, data phase,
  status byte, and saved completion `DSPS`. When a command has a pending DATA
  phase but the target moves directly to STATUS, `SIST0.MIA` is latched with
  the expected and sampled phases visible through the SCSI phase registers,
  and STATUS/MESSAGE remain pending for the next script restart.
  The local SCSI disk path also handles extended probe/read commands such as
  `READ(12)`, `WRITE(12)`, `REPORT LUNS`, `READ CAPACITY(16)`, `READ(16)`,
  `WRITE(16)`, `WRITE AND VERIFY(16)`, `VERIFY(16)`, and basic INQUIRY EVPD
  pages 0x00/0x80/0x83. It reports formatted 512-byte media for `READ FORMAT
  CAPACITIES`, empty defect lists for `READ DEFECT DATA(10/12)`, zeroed
  diagnostic/read-buffer style replies, and harmless `FORMAT UNIT` and cache
  sync completions.
  CDBs now drive explicit DATA IN, DATA OUT, or no-data phase selection, and
  harmless DATA OUT parameter lists are consumed instead of leaving stale DMA
  progress. MODE SENSE replies include a direct-access block descriptor unless
  the CDB sets DBD, and include common disconnect/reconnect, format-device, and
  control pages. Changeable-page MODE SENSE probes (`PC=1`) keep the page
  layouts but return zeroed value fields.
- Full Ethernet, VGA, full NVRAM, and multiprocessor support. A DECchip 21040
  PCI/CSR shell exists so firmware and OS probes see a plausible DEC Ethernet
  device, including basic reset/status/run-state behavior, but packet I/O is
  not implemented yet. CSR9 EEPROM/SROM bit-bang reads return a deterministic
  DEC OUI station address for early Ethernet driver probes.
- A complete SRM-compatible firmware execution environment. The real SRM image
  now reaches the SRM banner and receives serial input, but it does not reach
  an SRM prompt yet.

So this is not yet a complete OpenVMS boot. It is a controlled first point:
SIMH can identify the machine profile, attach all four recovered disks, load the
same APB that AXPbox/AlphaVM loaded, and enter it with Mikasa-specific HWRPB
state instead of a DS10/ES40 state.

Current verified SRM progress:

```text
Loaded Mikasa SRM ROM payload from ../firmware/as1000/mksrmrom.payload.bin: 475648 bytes at 00900000, PC=00900000 PALBASE=00900000
ff.fe.fd.fc.fb.fa.f9.f8.f7.f6.f5.f3.f2.f1.f0.ef.df.ee.f4.
probing hose 0, PCI
EPICbus 0, slot  6 -- pka -- NCR 53C810
probing hose 1, EISA
ed.ec.eb.....ea.e9.e8.e7.e6.e5.e4.e3.e2.e1.e0.
V5.4-101, built on Mar 24 1999 at 13:58:27
```

Earlier SRM stops fixed during bring-up:

- HALT at `0x900304`, caused by preserving `PCALG` in branch link registers.
  The CPU core now stores aligned return addresses for `BSR`/`BR`/`JMP` links.
- FPDIS while executing ROM PALmode code. Mikasa now decodes the EV4/21064
  IPR layout for `HW_MFPR`/`HW_MTPR`, including ICCSR/HWE.
- `*** Error, incorrect console for AlphaServer 1000A`. The firmware checks
  the AlphaServer 1000 ICU at ISA port `0x536` and expects bit `0x8000` to be
  visible in the 16-bit readback.
- Infinite `LDQ_L`/`STQ_C` retry loop. The common Alpha CPU core now writes
  success value `1` back to the source register on successful `STL_C`/`STQ_C`.

Current verified direct-APB diagnostic stop:

```text
Loaded OpenVMS Alpha APB from DKA0: LBN 4455271, 1049 blocks, 537088 bytes at 00200000
%APB-I-FILENOTLOC, Unable to locate SYSBOOT.EXE
%APB-I-LOADFAIL, Failed to load secondary bootstrap, status = 0910
HALT instruction, PC: 200039E0
```

The APB now reads enough ODS-2 metadata to mount the system volume, find
`SYS0.DIR`, and enter the system-root search. The current stop is still before
the implemented CRB callback counters move: `CALLBACKS`, `GETENVS`, `IOREADS`,
and `IOWRITES` remain zero. The APB mailbox path performs 11 DKA0 reads before
the stop. After the current SCSI/PCI hardware batch, the direct APB smoke still
loads APB from DKA0 and reaches the same `PC: 200039E0` halt. The same
non-regression holds after EPIC `DCSR.NDEV` latching for absent PCI memory
accesses.

Earlier stops were `R0=0x124` (`SS$_INSFMEM`) while mapping bootstrap memory
around the `0x40000000` boot page-table window, unsupported PAL calls such as
`PROBER` and `INSQUEQ`, `%APB-F-MOUNT`, and `%APB-F-BADSYSROOT`. The active
blocker is now the `SYSBOOT.EXE` lookup. `SYSBOOT.EXE;2` is present in the
dump, so the next work is to compare the APB's directory traversal against the
ODS-2 layout under `[SYS0.SYSEXE]` and `[SYS0.SYSCOMMON.SYSEXE]`.

## Fisica dump quick start

From the repository root:

```text
make alpha
BIN/alpha alpha/mikasa-fermi.ini
```

The main config loads the real AlphaServer 1000 SRM firmware and starts it with
`BOOT CPU`. Once the SRM prompt is reached, the expected firmware command is:

```text
BOOT DKA0
```

The real SRM contains long delay loops. For debug runs where wall-clock speed
matters more than cycle precision, add this before `BOOT CPU`:

```text
DEP MIKASA SCCSCALE 65536
```

Do not use that setting when checking timing-sensitive behavior; it exists to
make firmware bring-up interactive.

The config also attaches the four extracted dump images read-only:

```text
DKA0    AXPVMSSYS   system disk
DKA100  FERMI_USR
DKA200  FERMI_USR1
DKA300  SPOOL
```

The installed operating system appears to be OpenVMS Alpha V6.2. Evidence in
`disco1.img` includes a DECshare startup/configuration record for
`AlphaServer 1000 4/266` with `OpenVMS Version: V6.2`, installation text for
`OpenVMS Operating System, Version V6.2`, and SDA text for `OpenVMS (TM) Alpha
Operating System, Version V6.2` dated 2-DEC-1996.

For the old direct APB bypass, use:

```text
BIN/alpha alpha/mikasa-fermi-apb.ini
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

The de-duplicated current message is:

```text
%APB-I-FILENOTLOC, Unable to locate SYSBOOT.EXE
%APB-I-LOADFAIL, Failed to load secondary bootstrap, status = 0910
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

Extract the LFU wrapper once:

```text
alpha/tools/extract-lfu-rom.py ../firmware/as1000/mksrmrom.exe ../firmware/as1000/mksrmrom.payload.bin
```

The current extracted payload is:

```text
../firmware/as1000/mksrmrom.payload.bin
size:   0x74200
sha256: b4c3b18a0417a4d5714358cb85462747e1ac195d6ee421b6da07b6787cad9eb3
```

SIMH then loads the extracted payload:

```text
SET MIKASA ROMPAYLOAD=../firmware/as1000/mksrmrom.payload.bin
BOOT CPU
```

For quick experiments, `SET MIKASA ROM=../firmware/as1000/mksrmrom.exe` still
performs the same LFU stripping in memory, but the payload file is clearer for
the main config.

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

1. Advance the real SRM firmware path to the `P00>>>` prompt.
   The real `mksrmrom.exe` now prints the DEC/MIKASA SRM V5.4-101 banner. The
   serial interrupt path is live. The 53C810 path now decodes absolute and
   table-indirect selectors, follows simple unconditional script control flow,
   handles table-indirect command/data/status moves, and returns SCSI-2 sense
   data for common disk commands. Continue replacing the scanner with a real
   SCRIPTS executor until the probe finishes and `BOOT DKA0` can be issued from
   SRM.

2. Understand the direct-APB `SYSBOOT.EXE` lookup failure.
   The APB now reaches the system root and reads `SYSEXE`-related ODS-2
   metadata. The next check is whether its search path should be
   `[SYS0.SYSEXE]`, `[SYS0.SYSCOMMON.SYSEXE]`, or both, and why it misses the
   on-disk `SYSBOOT.EXE;2` header.

3. Keep tightening the remaining SRM boot memory/page-table context.
   HWRPB VPTB/processor-slot `PTBR`/`PT_VA`, the MDDT cluster layout, and APB
   page-table behavior around `EXE$K_BOOTPT` remain useful checks. Dynamic TLB
   misses are handled for bring-up, but this is not complete SRM PALcode.

4. Tighten the remaining HWRPB/SRM fields.
   CTB/CRB and callback descriptors exist, but PAL revision fields, DSR data,
   and SWRPB/boot context are still minimal.

5. Add more Mikasa platform I/O.
   The current model implements APECS sparse ISA I/O, Comanche/EPIC register
   storage, HAXR1 sparse-memory addressing, EPIC HAXR0/HAXR2/PMLT/TLB/TBIA,
   direct and SGMAP PCI DMA window translation, PCI configuration cycles, the
   raw-IDSEL `7` Intel 82375EB PCI/EISA bridge, the raw-IDSEL `6` NCR 53C810
   PCI configuration/register window, ISA DMA/page-register storage, an FDC
   shell, a raw-IDSEL `11` DECchip 21040 PCI/CSR shell with basic
   reset/status/run-state handling and CSR9 EEPROM/SROM reads,
   ELCR/PIC initialization and auto-EOI handling, and a minimal 8259 PIC/COM1
   receive interrupt path.

6. Continue the NCR/Symbios 53C810 frontend.
   SIMH has a common SCSI backend, but no complete 53C810 PCI DMA frontend in
   this tree. The implementation must be written cleanly for SIMH licensing; do
   not copy GPL code from AXPbox. The immediate missing pieces are conditional
   SCRIPTS execution, phase-mismatch paths, and wiring the transfer layer to
   SIMH `sim_scsi` where practical.

7. Wire the 53C810 frontend to the four DKA images.
   Once APB switches from firmware disk reads to OS disk I/O, the current raw
   loader path is not enough. OpenVMS will need the real SCSI controller model.

8. Add regression tests that do not require redistributing the disk dump.
   A small synthetic OpenVMS-style boot block can verify LBN/count parsing and
   APB placement.
