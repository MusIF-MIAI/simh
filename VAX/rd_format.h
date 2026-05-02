/* rd_format.h: DEC RD-series MFM disk physical format

   Copyright (c) 2026, OpenSimH contributors

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
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.

   --

   Shared between rd_unraw (offline recovery tool) and the DECRAW
   attach mode in vax4xx_rd.c.

   On-disk layout, validated against pdp11_rq.c:224-243 and OpenVMS
   class driver dvdriver.lis:

     XBN  - Extended blocks, 3 copies of the format block
     DBN  - Diagnostic blocks
     LBN  - User logical blocks (filesystem area)
     RCT  - Replacement Control Table, RCTC copies of RCTS sectors
     RBN  - Replacement (spare) blocks

   The "LBN" field stored in the format block (UIB_L_LBNSIZE) is
   inclusive of the RCT region; the user-visible LBN count is
   LBN_field - RCTS * RCTC. See dvdriver.lis line 4740 (REPLACE_STEP_3
   computes UCB$L_MAXBLOCK = UIB_L_LBNSIZE - RCTSIZE * RCTCOPIES).
*/

#ifndef RD_FORMAT_H_
#define RD_FORMAT_H_ 0

#include <stdint.h>
#include <string.h>

#define RD_SECT_SIZE        512                         /* bytes/sector */

/* UIB (Unit Information Block) field offsets within the 512-byte
   format block, expressed in 16-bit little-endian word indices.
   Cross-references: dvdriver.lis 824-1031, vax4xx_rd.c:641-687. */

#define UIB_W_XBN_LO        5                           /* XBN size, longword */
#define UIB_W_DBN_LO        7                           /* DBN size, longword */
#define UIB_W_LBN_LO        9                           /* LBN size, longword (incl. RCT) */
#define UIB_W_RBN_LO        11                          /* RBN size, longword */
#define UIB_W_SECT          13                          /* sectors/track */
#define UIB_W_SURF          14                          /* surfaces */
#define UIB_W_CYL           15                          /* cylinders */
#define UIB_W_RCTS          20                          /* RCT size, sectors/copy */
#define UIB_W_RCTC          21                          /* RCT copies */
#define UIB_W_CHECKSUM      255                         /* sum [0..254] mod 0x10000 */

/* RCT entry codes - VMS authoritative names from dvdriver.lis 915-926. */

#define RBN_C_EMPTY         0x0                         /* unallocated */
#define RBN_C_ALLOCP        0x2                         /* allocated, primary */
#define RBN_C_ALLOCS        0x3                         /* allocated, secondary */
#define RBN_C_INVALP        0x4                         /* unusable */
#define RBN_C_INVALS        0x5                         /* alternate unusable */
#define RBN_C_NULL          0x8                         /* null - no RBN sector */

#define RBN_M_CODE          0xF0000000
#define RBN_V_CODE          28
#define RBN_M_LBN           0x0FFFFFFF

#define RBN_GET_CODE(e)     (((e) >> RBN_V_CODE) & 0xF)
#define RBN_GET_LBN(e)      ((e) & RBN_M_LBN)

#define RBN_IS_FORWARDED(c) ((c) == RBN_C_ALLOCP || (c) == RBN_C_ALLOCS)
#define RBN_IS_BAD(c)       ((c) == RBN_C_INVALP || (c) == RBN_C_INVALS)

/* RCT overhead: each copy starts with 2 sectors that hold no
   forwarding entries - sector 0 is the status header, sector 1 is
   the data buffer for replacement-in-progress recovery. The actual
   entry array starts at sector 2 of each copy. */

#define RCT_OVHD_SECTS      2
#define RCT_ENTS_PER_SECT   (RD_SECT_SIZE / 4)          /* 128 longwords */

/* Geometry parsed from a format block. */

struct rd_geom {
    uint32_t xbn;                                       /* XBN sectors total */
    uint32_t dbn;                                       /* DBN sectors */
    uint32_t lbn_field;                                 /* raw LBN field (incl. RCT) */
    uint32_t user_lbn;                                  /* derived: lbn_field - rcts*rctc */
    uint32_t rbn;                                       /* RBN sectors */
    uint32_t sect;                                      /* sectors/track */
    uint32_t surf;                                      /* surfaces */
    uint32_t cyl;                                       /* cylinders */
    uint32_t rcts;                                      /* RCT sectors per copy */
    uint32_t rctc;                                      /* RCT copy count */
    /* derived byte offsets from start of dump file */
    uint64_t xbn_off;
    uint64_t lbn_off;
    uint64_t rct_off;
    uint64_t rbn_off;
    uint64_t total_sects;
};

/* Read a little-endian 16-bit word from buf at word index w. */

static uint16_t rd_word (const uint8_t *buf, unsigned int w)
{
uint16_t v;

memcpy (&v, buf + w * 2, 2);
return v;
}

/* Read a little-endian 32-bit longword from buf at word index w. */

static uint32_t rd_long (const uint8_t *buf, unsigned int w)
{
uint32_t v;

memcpy (&v, buf + w * 2, 4);
return v;
}

/* Verify the format block signature and checksum.  VMS first checks
   that bytes 0..8 are zero and byte 9 is 0x36, then validates the
   word checksum.  Returns 1 on match, 0 on mismatch. */

static int rd_checksum_ok (const uint8_t *buf)
{
unsigned int i;
uint32_t sum = 0;

for (i = 0; i < 9; i++)
    if (buf[i] != 0)
        return 0;
if (buf[9] != 0x36)
    return 0;

for (i = 0; i < 255; i++)
    sum += rd_word (buf, i);
return ((sum & 0xFFFF) == rd_word (buf, UIB_W_CHECKSUM));
}

/* Parse a 512-byte format block into rd_geom.  Performs only basic
   plausibility checks on the fields - caller must additionally
   validate against the file size via rd_compute_offsets.

   Returns 0 on apparent success, nonzero (a stage code) if any
   field is wildly out of range. */

static int rd_parse_format_block (const uint8_t *buf, struct rd_geom *g)
{
memset (g, 0, sizeof (*g));
g->xbn       = rd_long (buf, UIB_W_XBN_LO);
g->dbn       = rd_long (buf, UIB_W_DBN_LO);
g->lbn_field = rd_long (buf, UIB_W_LBN_LO);
g->rbn       = rd_long (buf, UIB_W_RBN_LO);
g->sect      = rd_word (buf, UIB_W_SECT);
g->surf      = rd_word (buf, UIB_W_SURF);
g->cyl       = rd_word (buf, UIB_W_CYL);
g->rcts      = rd_word (buf, UIB_W_RCTS);
g->rctc      = rd_word (buf, UIB_W_RCTC);

if (g->sect == 0 || g->sect > 64) return 1;
if (g->surf == 0 || g->surf > 32) return 2;
if (g->cyl  == 0 || g->cyl  > 32768) return 3;
if (g->rcts <= RCT_OVHD_SECTS || g->rcts > 64) return 4;
if (g->rctc == 0 || g->rctc > 32) return 5;
if (g->lbn_field <= g->rcts * g->rctc) return 6;
if (g->rbn  == 0 || g->rbn > 4096) return 7;

g->user_lbn = g->lbn_field - g->rcts * g->rctc;
return 0;
}

/* Compute derived byte offsets and validate against filesize.
   Returns 0 if filesize matches the implied physical layout,
   nonzero otherwise (the offsets are still populated). */

static int rd_compute_offsets (struct rd_geom *g, uint64_t filesize)
{
uint64_t expected;

g->total_sects = (uint64_t) g->xbn + g->dbn + g->lbn_field + g->rbn;
expected = g->total_sects * RD_SECT_SIZE;
g->xbn_off = 0;
g->lbn_off = (uint64_t) (g->xbn + g->dbn) * RD_SECT_SIZE;
g->rct_off = g->lbn_off + (uint64_t) g->user_lbn * RD_SECT_SIZE;
g->rbn_off = g->rct_off + (uint64_t) g->rcts * g->rctc * RD_SECT_SIZE;
return (filesize == expected) ? 0 : 1;
}

#endif /* RD_FORMAT_H_ */
