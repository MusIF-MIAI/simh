/* alpha_dka.c: raw SCSI disk bootstrap path for Alpha SRM-style DKA devices

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
#include "scp.h"
#include "sim_fio.h"

#define DKA_NUMUNITS            4
#define DKA_BLOCK_SIZE          512
#define DKA_APB_LOAD_PA         0x00200000
#define DKA_APB_ENTRY_VA        0x20000000
#define DKA_BOOT_COUNT_OFF      0x1E0
#define DKA_BOOT_LBN_OFF        0x1E8
#define DKA_MAX_BOOT_BLOCKS     4096

extern t_uint64 PC;
extern t_uint64 R[32];
extern UNIT cpu_unit;
extern uint32 pc_align;

t_stat dka_reset (DEVICE *dptr);
t_stat dka_attach (UNIT *uptr, CONST char *cptr);
t_stat dka_boot (int32 unitno, DEVICE *dptr);
t_stat dka_set_osflags (UNIT *uptr, int32 val, CONST char *cptr, void *desc);
t_stat dka_show_osflags (FILE *st, UNIT *uptr, int32 val, CONST void *desc);
t_stat mikasa_boot_prepare (CONST char *bootdev, uint32 osflags,
    t_uint64 image_bytes);

static uint32 dka_get_unit_index (UNIT *uptr);
static uint32 dka_get_le32 (const uint8 *buf, uint32 off);
static t_uint64 dka_get_le64 (const uint8 *buf, uint32 off);
static t_stat dka_read_blocks (UNIT *uptr, t_uint64 lbn, uint32 blocks,
    t_uint64 load_pa);

uint32 dka_osflags = 0;

UNIT dka_unit[] = {
    { UDATA (NULL, UNIT_FIX|UNIT_ATTABLE|UNIT_ROABLE, 0) },
    { UDATA (NULL, UNIT_FIX|UNIT_ATTABLE|UNIT_ROABLE, 0) },
    { UDATA (NULL, UNIT_FIX|UNIT_ATTABLE|UNIT_ROABLE, 0) },
    { UDATA (NULL, UNIT_FIX|UNIT_ATTABLE|UNIT_ROABLE, 0) }
    };

REG dka_reg[] = {
    { DRDATA (OSFLAGS, dka_osflags, 32) },
    { NULL }
    };

MTAB dka_mod[] = {
    { MTAB_XTD|MTAB_VDV|MTAB_VALR, 0, "OSFLAGS", "OSFLAGS",
      &dka_set_osflags, &dka_show_osflags, NULL,
      "Set OpenVMS Alpha APB boot flags passed in R5" },
    { 0 }
    };

DEVICE dka0_dev = {
    "DKA0", &dka_unit[0], dka_reg, dka_mod,
    1, 16, 64, 1, 16, 8,
    NULL, NULL, &dka_reset,
    &dka_boot, &dka_attach, NULL, NULL,
    DEV_SECTORS, 0, NULL
    };

DEVICE dka100_dev = {
    "DKA100", &dka_unit[1], dka_reg, dka_mod,
    1, 16, 64, 1, 16, 8,
    NULL, NULL, &dka_reset,
    &dka_boot, &dka_attach, NULL, NULL,
    DEV_SECTORS, 0, NULL
    };

DEVICE dka200_dev = {
    "DKA200", &dka_unit[2], dka_reg, dka_mod,
    1, 16, 64, 1, 16, 8,
    NULL, NULL, &dka_reset,
    &dka_boot, &dka_attach, NULL, NULL,
    DEV_SECTORS, 0, NULL
    };

DEVICE dka300_dev = {
    "DKA300", &dka_unit[3], dka_reg, dka_mod,
    1, 16, 64, 1, 16, 8,
    NULL, NULL, &dka_reset,
    &dka_boot, &dka_attach, NULL, NULL,
    DEV_SECTORS, 0, NULL
    };

static uint32 dka_get_unit_index (UNIT *uptr)
{
return (uint32) (uptr - dka_unit);
}

static uint32 dka_get_le32 (const uint8 *buf, uint32 off)
{
return ((uint32) buf[off]) |
    (((uint32) buf[off + 1]) << 8) |
    (((uint32) buf[off + 2]) << 16) |
    (((uint32) buf[off + 3]) << 24);
}

static t_uint64 dka_get_le64 (const uint8 *buf, uint32 off)
{
return ((t_uint64) dka_get_le32 (buf, off)) |
    (((t_uint64) dka_get_le32 (buf, off + 4)) << 32);
}

static t_stat dka_read_blocks (UNIT *uptr, t_uint64 lbn, uint32 blocks,
    t_uint64 load_pa)
{
uint8 buf[DKA_BLOCK_SIZE];
uint32 block;
uint32 byte;

if (!ADDR_IS_MEM (load_pa + (((t_uint64) blocks) * DKA_BLOCK_SIZE) - 1))
    return SCPE_NXM;

if (sim_fseeko (uptr->fileref, ((t_offset) lbn) * DKA_BLOCK_SIZE, SEEK_SET))
    return SCPE_IOERR;

for (block = 0; block < blocks; block++) {
    if (sim_fread (buf, 1, sizeof (buf), uptr->fileref) != sizeof (buf))
        return SCPE_IOERR;
    for (byte = 0; byte < sizeof (buf); byte++)
        WritePB (load_pa + (((t_uint64) block) * DKA_BLOCK_SIZE) + byte,
            buf[byte]);
    }

return SCPE_OK;
}

t_stat dka_set_osflags (UNIT *uptr, int32 val, CONST char *cptr, void *desc)
{
t_stat r;
t_value flags;

if (cptr == NULL)
    return SCPE_ARG;
flags = get_uint (cptr, 16, 0xFFFFFFFFu, &r);
if (r != SCPE_OK)
    return r;
dka_osflags = (uint32) flags;
return SCPE_OK;
}

t_stat dka_show_osflags (FILE *st, UNIT *uptr, int32 val, CONST void *desc)
{
fprintf (st, "%X", dka_osflags);
return SCPE_OK;
}

t_stat dka_attach (UNIT *uptr, CONST char *cptr)
{
t_stat r;

r = attach_unit (uptr, cptr);
if (r != SCPE_OK)
    return r;
uptr->capac = sim_fsize_ex (uptr->fileref) / DKA_BLOCK_SIZE;
return SCPE_OK;
}

t_stat dka_boot (int32 unitno, DEVICE *dptr)
{
UNIT *uptr;
uint8 block0[DKA_BLOCK_SIZE];
t_uint64 boot_blocks;
t_uint64 boot_lbn;
t_uint64 image_bytes;
t_stat r;
const char *boot_name;

if (unitno != 0)
    return SCPE_ARG;
uptr = dptr->units;
unitno = dka_get_unit_index (uptr);
if ((uptr->flags & UNIT_ATT) == 0)
    return SCPE_UNATT;

if (sim_fseeko (uptr->fileref, 0, SEEK_SET))
    return SCPE_IOERR;
if (sim_fread (block0, 1, sizeof (block0), uptr->fileref) != sizeof (block0))
    return SCPE_IOERR;

boot_blocks = dka_get_le64 (block0, DKA_BOOT_COUNT_OFF);
boot_lbn = dka_get_le64 (block0, DKA_BOOT_LBN_OFF);
if ((boot_blocks == 0) || (boot_blocks > DKA_MAX_BOOT_BLOCKS) ||
    (boot_lbn == 0))
    return sim_messagef (SCPE_FMT,
        "DKA%d: OpenVMS Alpha boot block fields are not valid\n", unitno);

if ((uptr->capac != 0) && ((boot_lbn + boot_blocks) > uptr->capac))
    return sim_messagef (SCPE_FMT,
        "DKA%d: bootstrap LBN range is outside the attached image\n", unitno);

boot_name = sim_uname (uptr);
r = dka_read_blocks (uptr, boot_lbn, (uint32) boot_blocks, DKA_APB_LOAD_PA);
if (r != SCPE_OK)
    return r;
image_bytes = boot_blocks * DKA_BLOCK_SIZE;
r = mikasa_boot_prepare (boot_name, dka_osflags, image_bytes);
if (r != SCPE_OK)
    return r;

PC = DKA_APB_ENTRY_VA;
pc_align = 0;
R[27] = DKA_APB_ENTRY_VA;

sim_printf ("Loaded OpenVMS Alpha APB from %s: LBN %llu, %llu blocks, "
    "%llu bytes at %08llX\n",
    boot_name,
    (unsigned long long) boot_lbn,
    (unsigned long long) boot_blocks,
    (unsigned long long) image_bytes,
    (unsigned long long) DKA_APB_LOAD_PA);

return SCPE_OK;
}

t_stat dka_reset (DEVICE *dptr)
{
return SCPE_OK;
}
