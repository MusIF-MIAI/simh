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
   folded back in.  The output keeps the user-visible LBN area plus the
   following RCT sectors, with transparent LBN -> RBN forwarding already
   applied, and is sized to be attached directly to SimH RD devices.

   Usage:
     rd_unraw [options] <raw_dump.img> [<out.img>]

   Options:
     --report             print every forwarded LBN -> RBN mapping
     --verify             cross-compare all RCTC copies of the RCT
     --verify-strict      fail with exit 3 on RCT replica disagreement
     --bad-files          map RCT bad sectors to current ODS-2 files
     --force-type=NAME    override geometry detection (RD31|RD32|RD53|RD54)
     --allow-unusable     zero-fill INVALP/INVALS LBNs (default: leave as-is)
     --allow-layout-mismatch
                          continue if geometry and file size disagree
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "../rd_format.h"

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
    { "RD31",  17,  4,  615,  54,  14,  41584,   100,  3,  8 },
    { "RD32",  17,  6,  821,  54,  48,  83236,   200,  4,  8 },
    { "RD53",  17,  8, 1024,  54,  82, 138712,   280,  5,  8 },
    { "RD54",  17, 15, 1225,  54, 201, 311256,   609,  7,  8 },
    { NULL }
    };

/* Candidate XBN format-block locations, in sectors.  VMS RD_GEOM reads
   the three UIB copies as consecutive PBNs 0, 1, and 2.  The legacy
   18/36 candidates are retained as a fallback for dumps produced by
   tools that expanded the old "XBN region replicated three times"
   description differently. */

static const off_t rd_xbn_candidates[] = {
    0,                                                  /* canonical */
    1, 2,                                               /* VMS spare UIB copies */
    18, 36,                                             /* legacy expanded layout */
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

struct rd_badent {
    uint32_t rbn;
    uint32_t lbn;
    uint32_t code;
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
int i;
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
        fprintf (stderr,
            "rd_unraw: notice: recovered geometry from spare XBN copy "
            "at sector %lld\n", (long long)rd_xbn_candidates[i]);
        }
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

/* --- ODS-2 bad-block file reporting --- */

struct ods2_extent {
    uint32_t lbn;
    uint32_t count;
    };

struct ods2_header {
    int valid;
    uint16_t seq;
    uint16_t rvn;
    uint16_t seg;
    uint16_t ext_num;
    uint16_t ext_seq;
    uint16_t ext_rvn;
    uint32_t header_lbn;
    char *name;
    char *path;
    struct ods2_extent *map;
    uint32_t maps;
    uint32_t map_alloc;
    };

struct ods2_ctx {
    int fd;
    const struct rd_geom *g;
    const uint32_t *lbn_map;
    uint16_t cluster;
    uint16_t ibmapvbn;
    uint32_t ibmaplbn;
    uint16_t ibmapsize;
    uint32_t maxfiles;
    uint32_t *index_lbn;
    uint32_t index_blocks;
    uint32_t header_stream0;
    struct ods2_header *hdr;
    uint32_t hdr_count;
    uint32_t *prev_ext;
    uint8_t *bitmap;
    size_t bitmap_len;
    };

static uint16_t le16_at (const uint8_t *b, size_t off)
{
uint16_t v;
memcpy (&v, b + off, sizeof v);
return v;
}

static uint32_t le32_at (const uint8_t *b, size_t off)
{
uint32_t v;
memcpy (&v, b + off, sizeof v);
return v;
}

static uint16_t ods2_sum (const uint8_t *b, uint32_t words)
{
uint32_t i;
uint32_t sum = 0;

for (i = 0; i < words; i++)
    sum += le16_at (b, i * 2);
return (uint16_t) sum;
}

static int ods2_checksum_ok (const uint8_t *b)
{
return ods2_sum (b, 255) == le16_at (b, 510);
}

static char *xstrdup (const char *s)
{
size_t len = strlen (s) + 1;
char *p = (char *) malloc (len);

if (p != NULL)
    memcpy (p, s, len);
return p;
}

static int ods2_read_lbn (const struct ods2_ctx *ctx, uint32_t lbn,
                          uint8_t *buf, uint32_t sectors)
{
uint32_t i;

for (i = 0; i < sectors; i++) {
    uint32_t cur = lbn + i;
    uint32_t rbn = 0;
    off_t off;

    if (cur < ctx->g->user_lbn && ctx->lbn_map != NULL)
        rbn = ctx->lbn_map[cur];
    if (rbn != 0)
        off = (off_t) ctx->g->rbn_off + (off_t)(rbn - 1) * RD_SECT_SIZE;
    else
        off = (off_t) ctx->g->lbn_off + (off_t) cur * RD_SECT_SIZE;
    if (read_at (ctx->fd, buf + (size_t)i * RD_SECT_SIZE,
                 RD_SECT_SIZE, off) != 0)
        return 1;
    }
return 0;
}

static int ods2_add_extent (struct ods2_header *h, uint32_t lbn,
                            uint32_t count)
{
struct ods2_extent *new_map;
uint32_t new_alloc;

if (count == 0)
    return 0;
if (h->maps == h->map_alloc) {
    new_alloc = h->map_alloc ? h->map_alloc * 2 : 8;
    new_map = (struct ods2_extent *) realloc (h->map,
                    (size_t)new_alloc * sizeof (*new_map));
    if (new_map == NULL)
        return 1;
    h->map = new_map;
    h->map_alloc = new_alloc;
    }
h->map[h->maps].lbn = lbn;
h->map[h->maps].count = count;
h->maps++;
return 0;
}

static int ods2_parse_map (struct ods2_header *h, const uint8_t *b)
{
uint32_t off = (uint32_t)b[1] * 2;
uint32_t end = (b[3] == 0 || b[3] == 255) ? 510 : (uint32_t)b[3] * 2;

if (off == 0 || off >= 510)
    return 0;
while (off + 2 <= end) {
    uint16_t w = le16_at (b, off);
    uint32_t fmt = w >> 14;
    uint32_t lbn, count;

    if (w == 0)
        break;
    if (fmt == 0) {
        off += 2;                                      /* placement descriptor */
        continue;
        }
    if (fmt == 1) {
        if (off + 4 > end)
            break;
        count = (w & 0xFF) + 1;
        lbn = (((w >> 8) & 0x3F) << 16) | le16_at (b, off + 2);
        off += 4;
        }
    else if (fmt == 2) {
        if (off + 6 > end)
            break;
        count = (w & 0x3FFF) + 1;
        lbn = ((uint32_t)le16_at (b, off + 4) << 16) | le16_at (b, off + 2);
        off += 6;
        }
    else {
        if (off + 8 > end)
            break;
        count = (((uint32_t)(w & 0x3FFF) << 16) | le16_at (b, off + 2)) + 1;
        lbn = le32_at (b, off + 4);
        off += 8;
        }
    if (ods2_add_extent (h, lbn, count) != 0)
        return 1;
    }
return 0;
}

static int ods2_name_char (int c)
{
return isalnum ((unsigned char)c) || c == '_' || c == '$' ||
       c == '-' || c == '.' || c == ';';
}

static char *ods2_header_name (const uint8_t *b)
{
uint32_t idoff = (uint32_t)b[0] * 2;
uint32_t mpoff = (uint32_t)b[1] * 2;
uint32_t off, start = 0, len = 0, best_start = 0, best_len = 0;
int best_has_version = 0;

if (idoff >= mpoff || mpoff > 510)
    return NULL;
for (off = idoff; off < mpoff; off++) {
    if (ods2_name_char (b[off])) {
        if (len == 0)
            start = off;
        len++;
        }
    else if (len != 0) {
        int has_version = 0;
        uint32_t i;

        for (i = start; i < start + len; i++)
            if (b[i] == ';')
                has_version = 1;
        if ((has_version && !best_has_version) ||
            (has_version == best_has_version && len > best_len)) {
            best_start = start;
            best_len = len;
            best_has_version = has_version;
            }
        len = 0;
        }
    }
if (len != 0) {
    int has_version = 0;
    uint32_t i;

    for (i = start; i < start + len; i++)
        if (b[i] == ';')
            has_version = 1;
    if ((has_version && !best_has_version) ||
        (has_version == best_has_version && len > best_len)) {
        best_start = start;
        best_len = len;
        }
    }
if (best_len != 0) {
    char *name = (char *) malloc (best_len + 1);
    if (name == NULL)
        return NULL;
    memcpy (name, b + best_start, best_len);
    name[best_len] = '\0';
    return name;
    }
return NULL;
}

static uint32_t ods2_next_ext (const struct ods2_ctx *ctx, uint32_t fid)
{
uint32_t next;

if (fid == 0 || fid > ctx->hdr_count || !ctx->hdr[fid].valid)
    return 0;
next = ctx->hdr[fid].ext_num;
if (next == 0 || next > ctx->hdr_count || !ctx->hdr[next].valid)
    return 0;
if (ctx->hdr[next].seq != ctx->hdr[fid].ext_seq ||
    ctx->hdr[next].rvn != ctx->hdr[fid].ext_rvn)
    return 0;
return next;
}

static int ods2_read_file (const struct ods2_ctx *ctx, uint32_t fid,
                           uint8_t **out_buf, size_t *out_len)
{
uint32_t cur = fid, guard = 0;
size_t total = 0, done = 0;
uint8_t *buf;

*out_buf = NULL;
*out_len = 0;
while (cur != 0 && cur <= ctx->hdr_count && ctx->hdr[cur].valid &&
       guard++ < ctx->hdr_count) {
    uint32_t i;
    for (i = 0; i < ctx->hdr[cur].maps; i++)
        total += (size_t)ctx->hdr[cur].map[i].count * RD_SECT_SIZE;
    cur = ods2_next_ext (ctx, cur);
    }
if (total == 0)
    return 0;
buf = (uint8_t *) malloc (total);
if (buf == NULL)
    return 1;
cur = fid;
guard = 0;
while (cur != 0 && cur <= ctx->hdr_count && ctx->hdr[cur].valid &&
       guard++ < ctx->hdr_count) {
    uint32_t i;
    for (i = 0; i < ctx->hdr[cur].maps; i++) {
        uint32_t lbn = ctx->hdr[cur].map[i].lbn;
        uint32_t count = ctx->hdr[cur].map[i].count;
        if (ods2_read_lbn (ctx, lbn, buf + done, count) != 0) {
            free (buf);
            return 1;
            }
        done += (size_t)count * RD_SECT_SIZE;
        }
    cur = ods2_next_ext (ctx, cur);
    }
*out_buf = buf;
*out_len = total;
return 0;
}

static void ods2_set_path (struct ods2_ctx *ctx, uint32_t fid,
                           const char *path)
{
if (fid == 0 || fid > ctx->hdr_count || !ctx->hdr[fid].valid ||
    ctx->hdr[fid].path != NULL)
    return;
ctx->hdr[fid].path = xstrdup (path);
}

static int ods2_seq_match (const struct ods2_ctx *ctx, uint32_t fn,
                           uint32_t seq, uint32_t rvn)
{
if (fn == 0 || fn > ctx->hdr_count || !ctx->hdr[fn].valid)
    return 0;
return ctx->hdr[fn].seq == seq && ctx->hdr[fn].rvn == rvn;
}

static int ods2_ends_dir (const char *name)
{
size_t len = strlen (name);

return len > 4 &&
       toupper ((unsigned char)name[len - 4]) == '.' &&
       toupper ((unsigned char)name[len - 3]) == 'D' &&
       toupper ((unsigned char)name[len - 2]) == 'I' &&
       toupper ((unsigned char)name[len - 1]) == 'R';
}

static int ods2_walk_dir (struct ods2_ctx *ctx, uint32_t fid,
                          const char *dir_path, uint8_t *visited)
{
uint8_t *data = NULL;
size_t len = 0, pos = 0;

if (fid == 0 || fid > ctx->hdr_count || visited[fid])
    return 0;
visited[fid] = 1;
if (ods2_read_file (ctx, fid, &data, &len) != 0)
    return 1;
while (pos + 6 <= len) {
    size_t block_end = ((pos / RD_SECT_SIZE) + 1) * RD_SECT_SIZE;
    uint16_t rec_len = le16_at (data, pos);
    size_t total, name_start, name_end, ver_off;
    uint8_t name_len;
    char name[256];

    if (rec_len == 0 || rec_len == 0xFFFF) {
        pos = block_end;
        continue;
        }
    total = (size_t)rec_len + 2;
    if (total < 8 || pos + total > len) {
        pos = block_end;
        continue;
        }
    name_len = data[pos + 5];
    name_start = pos + 6;
    name_end = name_start + name_len;
    if (name_len == 0 || name_end > pos + total) {
        pos += total;
        continue;
        }
    memcpy (name, data + name_start, name_len);
    name[name_len] = '\0';
    ver_off = name_end + (name_len & 1);
    while (ver_off + 8 <= pos + total) {
        uint16_t ver = le16_at (data, ver_off);
        uint16_t fn = le16_at (data, ver_off + 2);
        uint16_t fs = le16_at (data, ver_off + 4);
        uint16_t rvn = le16_at (data, ver_off + 6);

        if (ods2_seq_match (ctx, fn, fs, rvn)) {
            char path[1024];

            snprintf (path, sizeof path, "%s%s;%u", dir_path, name, ver);
            ods2_set_path (ctx, fn, path);
            if (ods2_ends_dir (name)) {
                char child[1024];
                size_t dlen = strlen (dir_path);
                size_t nlen = strlen (name) - 4;

                if (dlen > 0 && dir_path[dlen - 1] == ']' &&
                    strcmp (name, "000000.DIR") != 0) {
                    snprintf (child, sizeof child, "%.*s.%.*s]",
                              (int)(dlen - 1), dir_path, (int)nlen, name);
                    if (ods2_walk_dir (ctx, fn, child, visited) != 0) {
                        free (data);
                        return 1;
                        }
                    }
                }
            }
        ver_off += 8;
        }
    pos += total;
    }
free (data);
return 0;
}

static int ods2_init (struct ods2_ctx *ctx, int fd, const struct rd_geom *g,
                      const uint32_t *lbn_map)
{
uint8_t home[RD_SECT_SIZE];
uint8_t hdrbuf[RD_SECT_SIZE];
struct ods2_header index_hdr;
uint32_t i, j, total = 0;
uint16_t struclev;

memset (ctx, 0, sizeof (*ctx));
ctx->fd = fd;
ctx->g = g;
ctx->lbn_map = lbn_map;
if (ods2_read_lbn (ctx, 1, home, 1) != 0)
    return 1;
struclev = home[13];
if ((struclev != 2 && struclev != 5) ||
    le16_at (home, 58) != ods2_sum (home, 29) ||
    le16_at (home, 510) != ods2_sum (home, 255))
    return 1;
ctx->cluster = le16_at (home, 14);
ctx->ibmapvbn = le16_at (home, 22);
ctx->ibmaplbn = le32_at (home, 24);
ctx->maxfiles = le32_at (home, 28);
ctx->ibmapsize = le16_at (home, 32);
if (ctx->cluster == 0 || ctx->ibmapvbn == 0 ||
    ctx->ibmaplbn == 0 || ctx->ibmapsize == 0 || ctx->maxfiles == 0)
    return 1;
memset (&index_hdr, 0, sizeof index_hdr);
if (ods2_read_lbn (ctx, ctx->ibmaplbn + ctx->ibmapsize, hdrbuf, 1) != 0 ||
    !ods2_checksum_ok (hdrbuf) || ods2_parse_map (&index_hdr, hdrbuf) != 0)
    return 1;
for (i = 0; i < index_hdr.maps; i++)
    total += index_hdr.map[i].count;
ctx->index_lbn = (uint32_t *) malloc ((size_t)total * sizeof (*ctx->index_lbn));
if (ctx->index_lbn == NULL) {
    free (index_hdr.map);
    return 1;
    }
for (i = 0; i < index_hdr.maps; i++)
    for (j = 0; j < index_hdr.map[i].count; j++)
        ctx->index_lbn[ctx->index_blocks++] = index_hdr.map[i].lbn + j;
free (index_hdr.map);
ctx->header_stream0 = (uint32_t)ctx->ibmapvbn - 1 + ctx->ibmapsize;
ctx->hdr_count = ctx->maxfiles;
if (ctx->header_stream0 >= ctx->index_blocks)
    return 1;
if (ctx->hdr_count > ctx->index_blocks - ctx->header_stream0)
    ctx->hdr_count = ctx->index_blocks - ctx->header_stream0;
ctx->hdr = (struct ods2_header *) calloc ((size_t)ctx->hdr_count + 1,
                                          sizeof (*ctx->hdr));
ctx->prev_ext = (uint32_t *) calloc ((size_t)ctx->hdr_count + 1,
                                     sizeof (*ctx->prev_ext));
if (ctx->hdr == NULL || ctx->prev_ext == NULL)
    return 1;
for (i = 1; i <= ctx->hdr_count; i++) {
    uint32_t lbn = ctx->index_lbn[ctx->header_stream0 + i - 1];
    uint16_t fid_num, structlev;

    if (ods2_read_lbn (ctx, lbn, hdrbuf, 1) != 0 || !ods2_checksum_ok (hdrbuf))
        continue;
    fid_num = le16_at (hdrbuf, 8);
    structlev = le16_at (hdrbuf, 6);
    if (fid_num != i || (structlev != 0x0201 && structlev != 0x0202 &&
                         structlev != 0x0502))
        continue;
    ctx->hdr[i].valid = 1;
    ctx->hdr[i].header_lbn = lbn;
    ctx->hdr[i].seq = le16_at (hdrbuf, 10);
    ctx->hdr[i].rvn = le16_at (hdrbuf, 12);
    ctx->hdr[i].seg = le16_at (hdrbuf, 4);
    ctx->hdr[i].ext_num = le16_at (hdrbuf, 14);
    ctx->hdr[i].ext_seq = le16_at (hdrbuf, 16);
    ctx->hdr[i].ext_rvn = le16_at (hdrbuf, 18);
    ctx->hdr[i].name = ods2_header_name (hdrbuf);
    if (ods2_parse_map (&ctx->hdr[i], hdrbuf) != 0)
        return 1;
    }
for (i = 1; i <= ctx->hdr_count; i++) {
    uint32_t next = ctx->hdr[i].ext_num;
    if (!ctx->hdr[i].valid || next == 0 || next > ctx->hdr_count ||
        !ctx->hdr[next].valid)
        continue;
    if (ctx->hdr[next].seq == ctx->hdr[i].ext_seq &&
        ctx->hdr[next].rvn == ctx->hdr[i].ext_rvn)
        ctx->prev_ext[next] = i;
    }
if (ctx->hdr_count >= 4) {
    uint8_t *visited = (uint8_t *) calloc ((size_t)ctx->hdr_count + 1, 1);
    if (visited == NULL)
        return 1;
    (void)ods2_walk_dir (ctx, 4, "[000000]", visited);
    free (visited);
    }
if (ctx->hdr_count >= 2 && ctx->hdr[2].valid) {
    uint8_t *bitmap_file = NULL;
    size_t bitmap_file_len = 0;
    if (ods2_read_file (ctx, 2, &bitmap_file, &bitmap_file_len) == 0 &&
        bitmap_file_len > RD_SECT_SIZE) {
        ctx->bitmap_len = bitmap_file_len - RD_SECT_SIZE;
        ctx->bitmap = (uint8_t *) malloc (ctx->bitmap_len);
        if (ctx->bitmap != NULL)
            memcpy (ctx->bitmap, bitmap_file + RD_SECT_SIZE, ctx->bitmap_len);
        }
    free (bitmap_file);
    }
return 0;
}

static void ods2_free (struct ods2_ctx *ctx)
{
uint32_t i;

if (ctx->hdr != NULL) {
    for (i = 1; i <= ctx->hdr_count; i++) {
        free (ctx->hdr[i].name);
        free (ctx->hdr[i].path);
        free (ctx->hdr[i].map);
        }
    }
free (ctx->hdr);
free (ctx->prev_ext);
free (ctx->index_lbn);
free (ctx->bitmap);
}

static const char *ods2_alloc_state (const struct ods2_ctx *ctx,
                                     uint32_t lbn)
{
uint32_t idx;

if (ctx->bitmap == NULL || ctx->cluster == 0)
    return "unknown";
idx = lbn / ctx->cluster;
if ((size_t)(idx / 8) >= ctx->bitmap_len)
    return "unknown";
return (ctx->bitmap[idx / 8] & (1u << (idx & 7))) ? "free" : "allocated";
}

static uint32_t ods2_primary_fid (const struct ods2_ctx *ctx, uint32_t fid)
{
uint32_t guard = 0;

while (fid != 0 && fid <= ctx->hdr_count && ctx->hdr[fid].valid &&
       ctx->hdr[fid].seg != 0 && ctx->prev_ext[fid] != 0 &&
       guard++ < ctx->hdr_count)
    fid = ctx->prev_ext[fid];
return fid;
}

static const char *ods2_file_name (const struct ods2_ctx *ctx, uint32_t fid)
{
uint32_t primary = ods2_primary_fid (ctx, fid);

if (primary != 0 && primary <= ctx->hdr_count) {
    if (ctx->hdr[primary].path != NULL)
        return ctx->hdr[primary].path;
    if (ctx->hdr[primary].name != NULL)
        return ctx->hdr[primary].name;
    }
if (fid != 0 && fid <= ctx->hdr_count && ctx->hdr[fid].name != NULL)
    return ctx->hdr[fid].name;
return "-";
}

static uint32_t ods2_find_owner (const struct ods2_ctx *ctx, uint32_t lbn)
{
uint32_t fid;

for (fid = 1; fid <= ctx->hdr_count; fid++) {
    uint32_t i;
    if (!ctx->hdr[fid].valid)
        continue;
    for (i = 0; i < ctx->hdr[fid].maps; i++) {
        uint32_t start = ctx->hdr[fid].map[i].lbn;
        uint32_t end = start + ctx->hdr[fid].map[i].count;
        if (lbn >= start && lbn < end)
            return fid;
        }
    }
return 0;
}

static int build_bad_map (const uint8_t *rct_buf, const struct rd_geom *g,
                          uint32_t **out_lbn_map,
                          struct rd_badent **out_bad,
                          uint32_t *out_bad_count)
{
uint32_t *lbn_map;
struct rd_badent *bad;
uint32_t rbn_idx, count = 0;

*out_lbn_map = NULL;
*out_bad = NULL;
*out_bad_count = 0;
lbn_map = (uint32_t *) calloc (g->user_lbn, sizeof (*lbn_map));
bad = (struct rd_badent *) calloc (g->rbn, sizeof (*bad));
if (lbn_map == NULL || bad == NULL) {
    free (lbn_map);
    free (bad);
    return 1;
    }
for (rbn_idx = 0; rbn_idx < g->rbn; rbn_idx++) {
    uint32_t entry = rct_entry (rct_buf, rbn_idx);
    uint32_t code = RBN_GET_CODE (entry);
    uint32_t lbn = RBN_GET_LBN (entry);

    if (!RBN_IS_FORWARDED (code) && !RBN_IS_BAD (code))
        continue;
    bad[count].rbn = rbn_idx;
    bad[count].lbn = lbn;
    bad[count].code = code;
    count++;
    if (RBN_IS_FORWARDED (code) && lbn < g->user_lbn)
        lbn_map[lbn] = rbn_idx + 1;
    }
*out_lbn_map = lbn_map;
*out_bad = bad;
*out_bad_count = count;
return 0;
}

static int report_bad_files (int raw_fd, const uint8_t *rct_buf,
                             const struct rd_geom *g)
{
uint32_t *lbn_map = NULL;
struct rd_badent *bad = NULL;
uint32_t bad_count = 0, i;
struct ods2_ctx ods;
int have_ods = 0;

memset (&ods, 0, sizeof ods);
if (build_bad_map (rct_buf, g, &lbn_map, &bad, &bad_count) != 0) {
    fprintf (stderr, "rd_unraw: out of memory for bad-file report\n");
    return 1;
    }
if (bad_count == 0) {
    printf ("rd_unraw: no allocated or unusable RCT entries\n");
    free (lbn_map);
    free (bad);
    return 0;
    }
if (ods2_init (&ods, raw_fd, g, lbn_map) == 0)
    have_ods = 1;
else {
    fprintf (stderr,
        "rd_unraw: warning: ODS-2 filesystem not recognized; "
        "file names unavailable\n");
    ods2_free (&ods);
    }

printf ("\nBad-sector file report:\n");
printf ("%-9s %-6s %-8s %-11s %s\n",
        "LBN", "RBN", "RCT", "FS", "Current file");
for (i = 0; i < bad_count; i++) {
    const char *fs = "unknown";
    const char *file = "-";

    if (bad[i].lbn >= g->user_lbn)
        fs = "out-range";
    else if (have_ods) {
        uint32_t owner = ods2_find_owner (&ods, bad[i].lbn);
        fs = ods2_alloc_state (&ods, bad[i].lbn);
        if (owner != 0)
            file = ods2_file_name (&ods, owner);
        }
    printf ("%-9u %-6u %-8s %-11s %s\n",
            bad[i].lbn, bad[i].rbn, code_name (bad[i].code), fs, file);
    }
fflush (stdout);
if (have_ods)
    ods2_free (&ods);
free (lbn_map);
free (bad);
return 0;
}

/* --- output construction --- */

/* Bulk-copy the SimH-visible LBN region from raw to out, sector by
   sector, in reasonable chunks.  This includes the user filesystem
   area and the following RCT sectors; the RBN spare area remains a raw
   dump artifact and is folded into forwarded LBNs below. */

static int copy_lbn_region (int raw_fd, int out_fd, const struct rd_geom *g)
{
const size_t chunk = 64 * 1024;
uint8_t *buf;
uint64_t total = (uint64_t) g->lbn_field * RD_SECT_SIZE;
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
    "  --bad-files        map RCT bad sectors to current ODS-2 files\n"
    "  --force-type=NAME  override geometry detection (RD31|RD32|RD53|RD54)\n"
    "  --allow-unusable   zero-fill INVALP/INVALS LBNs in the output\n"
    "  --allow-layout-mismatch\n"
    "                     continue if geometry and file size disagree\n"
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
int report = 0, verify = 0, verify_strict = 0, bad_files = 0;
int allow_unusable = 0;
int allow_layout_mismatch = 0;
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
    else if (strcmp (a, "--bad-files") == 0)      bad_files = 1;
    else if (strcmp (a, "--allow-unusable") == 0) allow_unusable = 1;
    else if (strcmp (a, "--allow-layout-mismatch") == 0)
        allow_layout_mismatch = 1;
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
        "%llu (XBN %u + DBN %u + LBN %u + RBN %u)*%u\n",
        (unsigned long long) filesize,
        (unsigned long long)(g.total_sects * RD_SECT_SIZE),
        g.xbn, g.dbn, g.lbn_field, g.rbn, RD_SECT_SIZE);
    if (!allow_layout_mismatch) {
        fprintf (stderr,
            "rd_unraw: refusing to decode with mismatched layout; use "
            "--allow-layout-mismatch to override\n");
        rc = 2; goto out;
        }
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
if (bad_files) {
    if (report_bad_files (raw_fd, rct_buf, &g) != 0) {
        rc = 1; goto out;
        }
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
        (unsigned long long)((uint64_t) g.lbn_field * RD_SECT_SIZE),
        out_path);

out:
if (rct_buf) free (rct_buf);
if (out_fd >= 0) close (out_fd);
if (raw_fd >= 0) close (raw_fd);
return rc;
}
