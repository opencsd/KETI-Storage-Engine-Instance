#ifndef _data_blk_H
#define _data_blk_H

#include <stdio.h>
#include <sys/types.h>
#include <stdint.h>
#include <assert.h>

#define TABLE_BLOCK_TYPE 13
#define INDEX_LEAF_BLOCK_TYPE 15
#define HEAD_ROW_PIECE 0x20
#define DELETED_ROW_PIECE 0x10
#define FIRSTPIECE_ROW_PIECE 0x08
#define LASTPIECE_ROW_PIECE 0x04
#define LASTCOL_CONT_ROW_PIECE 0x01
#define SKIP_TAIL_CHECK 0x10
#define TAIL_CRC32 0x02

#define dblk_get_dl(blk)                                                       \
    (_dblk_dl_t *)(&blk->tl.itls[blk->tl.itlcnt])

typedef struct _tsn_s {
    volatile uint32_t base_;
    volatile uint16_t wrap_;
} _tsn_t;

typedef struct _tx_s {
    uint16_t usgmt_id;
    uint16_t slotno;
    uint32_t wrapno;
} _tx_t;

typedef struct _uea_s {
    uint32_t dba;
    uint16_t seqno;
    uint16_t rowno;
} _uea_t;

typedef struct _itl_s {
    _tx_t    xid;
    _uea_t   uea;
    uint8_t flag;
    uint8_t unused;

    union {
        uint16_t tsn_hi;
        uint16_t credit;
    } u;
    uint32_t tsn_low;
} _itl_t;

typedef struct _blk_tl_s {
    uint8_t     itlcnt;
    char        unused[1];
    uint16_t    offset_in_l1;
    uint32_t    l1dba;
    uint32_t    sgmt_id;
    _tsn_t       cleanout_tsn;
    _itl_t itls[1];
} _blk_tl_t;

typedef struct _blk_cl_s {
    uint8_t      type;
    char         filler1;
    char         filler2;
    char         filler3;
    uint32_t     dba;
    uint32_t     tsn_base;
    uint16_t     tsn_wrap;
    uint8_t      seqno;
    uint8_t      flag;
} _blk_cl_t;

typedef struct _blk_s {
    _blk_cl_t cl;
    _blk_tl_t tl;
} _blk_t;

typedef struct _dblk_dl_s {
    int16_t freespace;
    uint16_t freepos;
    uint16_t rowcnt;
    uint16_t symtab_offset;
    uint16_t rowdir[1];
} _dblk_dl_t;

typedef struct _rp_s {
    uint8_t flags;
    uint8_t tx;
    uint8_t colcnt;
} _rp_t;

int table_block_verify(_blk_t *blk, uint32_t dba, uint16_t blksize);
_rp_t * table_block_get_row_piece(_blk_t *blk, int rowno);

#define RP_HDRSIZE(rp)                                                         \
    ((rp->flags & LASTPIECE_ROW_PIECE) != 0 ? sizeof(_rp_t) : sizeof(_rp_t) + 6)

static inline unsigned char *
rp_get_data (_rp_t *rp) {
    return (unsigned char *)rp + RP_HDRSIZE(rp);
}

#endif
