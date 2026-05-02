/* rd_unraw.c: DEC RD-series raw dump bad-block recovery

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

   Reads a raw MFM dump of a DEC RD-series disk (RD31, RD32, RD53, RD54),
   parses the controller's Replacement Control Table (RCT), and produces
   a "clean" image whose user-data area has the spare-sector contents
   folded back in.  The output is the byte-for-byte filesystem view that
   the on-board HDC9224 controller would have presented through
   transparent LBN -> RBN forwarding, and is sized to be attached
   directly to SimH's microvax2000 RD device.

   Usage:
     rd_unraw [options] <raw_dump.img> [<out.img>]

   Options:
     --report             print every forwarded LBN -> RBN mapping
     --verify             cross-compare all RCTC copies of the RCT
     --verify-strict      fail with exit 3 on RCT replica disagreement
     --force-type=NAME    override geometry detection (RD31|RD32|RD53|RD54)
     --allow-unusable     zero-fill INVALP/INVALS LBNs (default: leave as-is)
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "rd_format.h"

#if !defined(__BYTE_ORDER__) || __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error "rd_unraw assumes a little-endian host (matches VAX byte order)"
#endif

/* Known geometries from vax4xx_rd.c:160-227.  Used as last-resort
   fallback when every XBN format block copy is unreadable. */

struct rd_known {
    const char *name;
    uint32_t sect, surf, cyl;
    uint32_t xbn, dbn, lbn_field, rbn;
    uint32_t rcts, rctc;
    };

static const struct rd_known rd_known_tab[] = {
    { "RD31",  17,  4,  615,  54,  10,  41560,   100,  3,  8 },
    { "RD32",  17,  6,  821,  54,  48,  83236,   200,  4,  8 },
    { "RD53",  17,  8, 1024,  54,  82, 138712,   280,  5,  8 },
    { "RD54",  17, 15, 1225,  54, 201, 311256,   609,  7,  8 },
    { NULL }
    };

/* Candidate XBN format-block byte offsets, in sectors.  The first
   XBN copy lives at sector 0; spares are at sect+1 and 2*(sect+1).
   We don't know `sect` until we parse a block, so try common values:
   sect+1 in {15..19} covers all known RD/RX drives.  Order: most
   common first. */

static const off_t rd_xbn_candidates[] = {
    0,                                                  /* canonical */
    18, 36,                                             /* sect=17 (RD31..RD54) */
    19, 38, 17, 34, 16, 32, 15, 30,                     /* less common */
    -1
    };

/* Convenience for stats counters. */

struct rd_stats {
    uint32_t forwarded;                                 /* ALLOCP+ALLOCS */
    uint32_t unusable;                                  /* INVALP+INVALS */
    uint32_t empty;                                     /* EMPTY+NULL */
    uint32_t unknown_code;                              /* anything else */
    uint32_t replica_mismatches;                        /* per-byte */
    };

/* --- I/O helpers --- */

static int read_at (int fd, void *buf, size_t len, off_t off)
{
ssize_t got = pread (fd, buf, len, off);

if (got < 0) {
    fprintf (stderr, "rd_unraw: pread %ld@%lld: %s\n",
             (long)len, (long long)off, strerror (errno));
    return 1;
    }
if ((size_t) got != len) {
    fprintf (stderr, "rd_unraw: short read %ld@%lld (got %ld)\n",
             (long)len, (long long)off, (long)got);
    return 1;
    }
return 0;
}

static int write_at (int fd, const void *buf, size_t len, off_t off)
{
ssize_t put = pwrite (fd, buf, len, off);

if (put < 0) {
    fprintf (stderr, "rd_unraw: pwrite %ld@%lld: %s\n",
             (long)len, (long long)off, strerror (errno));
    return 1;
    }
if ((size_t) put != len) {
    fprintf (stderr, "rd_unraw: short write %ld@%lld (put %ld)\n",
             (long)len, (long long)off, (long)put);
    return 1;
    }
return 0;
}

/* --- format block discovery --- */

/* Try every candidate XBN format-block location and return the
   sector index at which a valid block was found.  The block contents
   are placed in out_buf and the parsed geometry in out_g.  Returns
   -1 if no candidate was usable. */

static off_t find_format_block (int fd, uint64_t filesize,
                                uint8_t *out_buf, struct rd_geom *out_g)
{
int i, used_spare = 0;
off_t off;

for (i = 0; rd_xbn_candidates[i] >= 0; i++) {
    off = rd_xbn_candidates[i] * RD_SECT_SIZE;
    if ((uint64_t)(off + RD_SECT_SIZE) > filesize)
        continue;
    if (read_at (fd, out_buf, RD_SECT_SIZE, off) != 0)
        continue;
    if (!rd_checksum_ok (out_buf)) {
        if (i == 0)
            fprintf (stderr,
                "rd_unraw: warning: primary XBN at sector 0 has bad checksum, "
                "trying spare copies\n");
        continue;
        }
    if (rd_parse_format_block (out_buf, out_g) != 0) {
        fprintf (stderr,
            "rd_unraw: warning: XBN at sector %lld parses but fields "
            "implausible, skipping\n", (long long)rd_xbn_candidates[i]);
        continue;
        }
    if (i > 0) {
        used_spare = 1;
        fprintf (stderr,
            "rd_unraw: notice: recovered geometry from spare XBN copy "
            "at sector %lld\n", (long long)rd_xbn_candidates[i]);
        }
    (void) used_spare;
    return rd_xbn_candidates[i];
    }
return (off_t) -1;
}

/* Last-resort: file size matches a known geometry exactly.  Caller
   must still warn the user that the format block was unreadable. */

static int match_known_geom (uint64_t filesize, struct rd_geom *out_g)
{
const struct rd_known *k;

for (k = rd_known_tab; k->name; k++) {
    uint64_t total = (uint64_t)(k->xbn + k->dbn + k->lbn_field + k->rbn);
    if (total * RD_SECT_SIZE != filesize)
        continue;
    memset (out_g, 0, sizeof (*out_g));
    out_g->xbn = k->xbn; out_g->dbn = k->dbn;
    out_g->lbn_field = k->lbn_field; out_g->rbn = k->rbn;
    out_g->sect = k->sect; out_g->surf = k->surf; out_g->cyl = k->cyl;
    out_g->rcts = k->rcts; out_g->rctc = k->rctc;
    out_g->user_lbn = k->lbn_field - k->rcts * k->rctc;
    fprintf (stderr,
        "rd_unraw: notice: format block unreadable, but file size "
        "matches %s exactly\n", k->name);
    return 0;
    }
return 1;
}

static int force_known_geom (const char *name, struct rd_geom *out_g)
{
const struct rd_known *k;

for (k = rd_known_tab; k->name; k++) {
    if (strcmp (k->name, name) == 0) {
        memset (out_g, 0, sizeof (*out_g));
        out_g->xbn = k->xbn; out_g->dbn = k->dbn;
        out_g->lbn_field = k->lbn_field; out_g->rbn = k->rbn;
        out_g->sect = k->sect; out_g->surf = k->surf; out_g->cyl = k->cyl;
        out_g->rcts = k->rcts; out_g->rctc = k->rctc;
        out_g->user_lbn = k->lbn_field - k->rcts * k->rctc;
        return 0;
        }
    }
return 1;
}

/* --- RCT loading and verification --- */

/* Load all rctc copies of the RCT into separate buffers.  copies[i]
   points into a single contiguous allocation. */

static uint8_t *load_rct_copies (int fd, const struct rd_geom *g)
{
size_t one_copy = (size_t) g->rcts * RD_SECT_SIZE;
size_t total = one_copy * g->rctc;
uint8_t *buf;
uint32_t i;

buf = (uint8_t *) malloc (total);
if (buf == NULL) {
    fprintf (stderr, "rd_unraw: out of memory for RCT (%zu bytes)\n", total);
    return NULL;
    }
for (i = 0; i < g->rctc; i++) {
    off_t off = (off_t) g->rct_off + (off_t)(i * one_copy);
    if (read_at (fd, buf + i * one_copy, one_copy, off) != 0) {
        free (buf);
        return NULL;
        }
    }
return buf;
}

/* Compare every pair of RCT copies byte-by-byte.  Returns the number
   of byte positions where any pair disagreed.  If verbose, log the
   first few mismatching offsets. */

static uint32_t verify_rct_copies (const uint8_t *buf,
                                   const struct rd_geom *g, int verbose)
{
size_t one_copy = (size_t) g->rcts * RD_SECT_SIZE;
uint32_t mismatches = 0;
size_t off;
uint32_t logged = 0;

for (off = 0; off < one_copy; off++) {
    uint8_t v0 = buf[off];
    uint32_t i;
    for (i = 1; i < g->rctc; i++) {
        if (buf[i * one_copy + off] != v0) {
            mismatches++;
            if (verbose && logged < 16) {
                fprintf (stderr,
                    "rd_unraw: RCT mismatch at copy 0 vs %u, byte +%zu: "
                    "0x%02x vs 0x%02x\n",
                    i, off, v0, buf[i * one_copy + off]);
                logged++;
                }
            break;                                      /* one log per offset */
            }
        }
    }
return mismatches;
}

/* Fetch RCT entry `idx` (0-based, RBN index).  Reads from the chosen
   copy.  Skips the 2-sector overhead at the start of each copy. */

static uint32_t rct_entry (const uint8_t *copy_buf, uint32_t idx)
{
size_t byte_off = (size_t)RCT_OVHD_SECTS * RD_SECT_SIZE + (size_t) idx * 4;
uint32_t v;

memcpy (&v, copy_buf + byte_off, 4);
return v;
}

/* --- RCT walk and recovery --- */

static const char *code_name (uint32_t code)
{
switch (code) {
    case RBN_C_EMPTY:  return "EMPTY";
    case RBN_C_ALLOCP: return "ALLOCP";
    case RBN_C_ALLOCS: return "ALLOCS";
    case RBN_C_INVALP: return "INVALP";
    case RBN_C_INVALS: return "INVALS";
    case RBN_C_NULL:   return "NULL";
    default:           return "?";
    }
}

/* Walk every RBN entry; emit report lines if requested; record stats;
   apply forwarding patches to out_fd if out_fd >= 0.  Returns 0 on
   success (any I/O error stops with nonzero). */

static int walk_rct (int raw_fd, int out_fd,
                     const uint8_t *rct_buf, const struct rd_geom *g,
                     int report, int allow_unusable,
                     struct rd_stats *st)
{
size_t one_copy = (size_t) g->rcts * RD_SECT_SIZE;
const uint8_t *primary = rct_buf;                       /* copy 0 */
uint8_t sect[RD_SECT_SIZE];
uint8_t zero[RD_SECT_SIZE];
uint32_t rbn_idx;

(void) one_copy;
memset (zero, 0, sizeof zero);

for (rbn_idx = 0; rbn_idx < g->rbn; rbn_idx++) {
    uint32_t entry = rct_entry (primary, rbn_idx);
    uint32_t code = RBN_GET_CODE (entry);
    uint32_t lbn = RBN_GET_LBN (entry);

    if (RBN_IS_FORWARDED (code)) {
        if (lbn >= g->user_lbn) {
            fprintf (stderr,
                "rd_unraw: warning: RBN %u code %s points at LBN %u "
                "(out of range, max %u) - skipping\n",
                rbn_idx, code_name (code), lbn, g->user_lbn);
            st->unknown_code++;
            continue;
            }
        if (report)
            printf ("RBN %4u: %-7s -> LBN %u\n",
                    rbn_idx, code_name (code), lbn);
        st->forwarded++;
        if (out_fd >= 0) {
            off_t rbn_off = (off_t) g->rbn_off
                          + (off_t) rbn_idx * RD_SECT_SIZE;
            off_t lbn_off = (off_t) lbn * RD_SECT_SIZE;
            if (read_at (raw_fd, sect, RD_SECT_SIZE, rbn_off) != 0)
                return 1;
            if (write_at (out_fd, sect, RD_SECT_SIZE, lbn_off) != 0)
                return 1;
            }
        }
    else if (RBN_IS_BAD (code)) {
        st->unusable++;
        if (report)
            printf ("RBN %4u: %-7s -> LBN %u (UNRECOVERABLE)\n",
                    rbn_idx, code_name (code), lbn);
        if (allow_unusable && out_fd >= 0 && lbn < g->user_lbn) {
            off_t lbn_off = (off_t) lbn * RD_SECT_SIZE;
            if (write_at (out_fd, zero, RD_SECT_SIZE, lbn_off) != 0)
                return 1;
            }
        }
    else if (code == RBN_C_EMPTY || code == RBN_C_NULL) {
        st->empty++;
        }
    else {
        st->unknown_code++;
        fprintf (stderr,
            "rd_unraw: warning: RBN %u has unknown code 0x%x (entry 0x%08x)\n",
            rbn_idx, code, entry);
        }
    }
return 0;
}

/* --- output construction --- */

/* Bulk-copy the LBN region from raw to out, sector by sector, in
   reasonable chunks. */

static int copy_lbn_region (int raw_fd, int out_fd, const struct rd_geom *g)
{
const size_t chunk = 64 * 1024;
uint8_t *buf;
uint64_t total = (uint64_t) g->user_lbn * RD_SECT_SIZE;
uint64_t done = 0;

buf = (uint8_t *) malloc (chunk);
if (buf == NULL) {
    fprintf (stderr, "rd_unraw: out of memory for copy buffer\n");
    return 1;
    }
while (done < total) {
    size_t want = (size_t)((total - done) < chunk ? (total - done) : chunk);
    if (read_at (raw_fd, buf, want, (off_t)(g->lbn_off + done)) != 0)
        goto fail;
    if (write_at (out_fd, buf, want, (off_t) done) != 0)
        goto fail;
    done += want;
    }
free (buf);
return 0;
fail:
free (buf);
return 1;
}

/* --- main --- */

static void usage (FILE *fp)
{
fprintf (fp,
    "Usage: rd_unraw [options] <raw_dump.img> [<out.img>]\n"
    "\n"
    "Options:\n"
    "  --report           print every forwarded LBN -> RBN mapping\n"
    "  --verify           cross-compare all RCTC copies of the RCT\n"
    "  --verify-strict    fail with exit 3 on RCT replica disagreement\n"
    "  --force-type=NAME  override geometry detection (RD31|RD32|RD53|RD54)\n"
    "  --allow-unusable   zero-fill INVALP/INVALS LBNs in the output\n"
    "  -h, --help         this message\n"
    "\n"
    "If <out.img> is omitted, only the geometry summary (and report,\n"
    "if requested) is printed; no output image is written.\n");
}

int main (int argc, char **argv)
{
const char *in_path = NULL;
const char *out_path = NULL;
const char *force_type = NULL;
int report = 0, verify = 0, verify_strict = 0, allow_unusable = 0;
int i;
int raw_fd = -1, out_fd = -1;
struct stat st;
uint64_t filesize;
uint8_t fmt_buf[RD_SECT_SIZE];
struct rd_geom g;
uint8_t *rct_buf = NULL;
uint32_t mismatches = 0;
struct rd_stats stats;
int rc = 0;

memset (&stats, 0, sizeof stats);

for (i = 1; i < argc; i++) {
    const char *a = argv[i];
    if (strcmp (a, "-h") == 0 || strcmp (a, "--help") == 0) {
        usage (stdout); return 0;
        }
    else if (strcmp (a, "--report") == 0)         report = 1;
    else if (strcmp (a, "--verify") == 0)         verify = 1;
    else if (strcmp (a, "--verify-strict") == 0)  verify = verify_strict = 1;
    else if (strcmp (a, "--allow-unusable") == 0) allow_unusable = 1;
    else if (strncmp (a, "--force-type=", 13) == 0) force_type = a + 13;
    else if (a[0] == '-') {
        fprintf (stderr, "rd_unraw: unknown option '%s'\n", a);
        usage (stderr);
        return 2;
        }
    else if (in_path == NULL)  in_path = a;
    else if (out_path == NULL) out_path = a;
    else {
        fprintf (stderr, "rd_unraw: too many positional args\n");
        return 2;
        }
    }

if (in_path == NULL) {
    fprintf (stderr, "rd_unraw: missing <raw_dump.img>\n");
    usage (stderr);
    return 2;
    }

raw_fd = open (in_path, O_RDONLY);
if (raw_fd < 0) {
    fprintf (stderr, "rd_unraw: open '%s': %s\n", in_path, strerror (errno));
    return 1;
    }
if (fstat (raw_fd, &st) != 0) {
    fprintf (stderr, "rd_unraw: fstat: %s\n", strerror (errno));
    rc = 1; goto out;
    }
filesize = (uint64_t) st.st_size;
if (filesize == 0 || filesize % RD_SECT_SIZE != 0) {
    fprintf (stderr, "rd_unraw: file size %llu is not a positive multiple of %u\n",
             (unsigned long long) filesize, RD_SECT_SIZE);
    rc = 2; goto out;
    }

/* --- geometry detection --- */

if (force_type) {
    if (force_known_geom (force_type, &g) != 0) {
        fprintf (stderr, "rd_unraw: unknown --force-type '%s'\n", force_type);
        rc = 2; goto out;
        }
    fprintf (stderr, "rd_unraw: forced geometry %s\n", force_type);
    }
else {
    off_t fb = find_format_block (raw_fd, filesize, fmt_buf, &g);
    if (fb < 0) {
        fprintf (stderr,
            "rd_unraw: warning: no XBN format-block copy is readable; "
            "falling back to size match\n");
        if (match_known_geom (filesize, &g) != 0) {
            fprintf (stderr,
                "rd_unraw: file size does not match any known RD geometry; "
                "use --force-type=NAME if you know the drive type\n");
            rc = 2; goto out;
            }
        }
    }

if (rd_compute_offsets (&g, filesize) != 0) {
    fprintf (stderr,
        "rd_unraw: warning: file size %llu does not match expected layout "
        "%llu (XBN %u + DBN %u + LBN %u + RBN %u)*%u; continuing anyway\n",
        (unsigned long long) filesize,
        (unsigned long long)(g.total_sects * RD_SECT_SIZE),
        g.xbn, g.dbn, g.lbn_field, g.rbn, RD_SECT_SIZE);
    }

fprintf (stderr,
    "rd_unraw: geometry sect=%u surf=%u cyl=%u  XBN=%u DBN=%u "
    "LBN_field=%u user_LBN=%u  RCTS=%u RCTC=%u  RBN=%u  total=%llu\n",
    g.sect, g.surf, g.cyl, g.xbn, g.dbn,
    g.lbn_field, g.user_lbn, g.rcts, g.rctc, g.rbn,
    (unsigned long long) g.total_sects);
fprintf (stderr,
    "rd_unraw: offsets  LBN @ %llu  RCT @ %llu  RBN @ %llu\n",
    (unsigned long long) g.lbn_off,
    (unsigned long long) g.rct_off,
    (unsigned long long) g.rbn_off);

/* --- RCT loading & verification --- */

rct_buf = load_rct_copies (raw_fd, &g);
if (rct_buf == NULL) {
    rc = 1; goto out;
    }
if (verify) {
    mismatches = verify_rct_copies (rct_buf, &g, 1);
    stats.replica_mismatches = mismatches;
    if (mismatches > 0) {
        fprintf (stderr,
            "rd_unraw: %u byte position(s) where RCT copies disagree\n",
            mismatches);
        if (verify_strict) {
            rc = 3; goto out;
            }
        }
    else {
        fprintf (stderr, "rd_unraw: all %u RCT copies agree\n", g.rctc);
        }
    }

/* --- output --- */

if (out_path) {
    out_fd = open (out_path, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (out_fd < 0) {
        fprintf (stderr, "rd_unraw: open '%s' for output: %s\n",
                 out_path, strerror (errno));
        rc = 1; goto out;
        }
    if (copy_lbn_region (raw_fd, out_fd, &g) != 0) {
        rc = 1; goto out;
        }
    }

if (walk_rct (raw_fd, out_fd, rct_buf, &g, report, allow_unusable, &stats) != 0) {
    rc = 1; goto out;
    }

fprintf (stderr,
    "rd_unraw: stats  forwarded=%u  unusable=%u  empty=%u  unknown=%u",
    stats.forwarded, stats.unusable, stats.empty, stats.unknown_code);
if (verify)
    fprintf (stderr, "  rct_mismatch_bytes=%u", stats.replica_mismatches);
fprintf (stderr, "\n");

if (out_path)
    fprintf (stderr,
        "rd_unraw: wrote %llu byte image to '%s'\n",
        (unsigned long long)((uint64_t) g.user_lbn * RD_SECT_SIZE),
        out_path);

out:
if (rct_buf) free (rct_buf);
if (out_fd >= 0) close (out_fd);
if (raw_fd >= 0) close (raw_fd);
return rc;
}
