#include "data_blk.h"

static int
block_checksum(_blk_t *blk, uint16_t blksize)
{
    uint32_t value;
    uint32_t tail;

    if ((blk->cl.flag & SKIP_TAIL_CHECK) != 0)
        return 1;

    assert((blk->cl.flag & TAIL_CRC32) == 0);

    value = (uint32_t)(0 | (((_blk_cl_t *)blk)->type << 24)
                         | (((_blk_cl_t *)blk)->seqno << 16)
                         | (((_blk_cl_t *)blk)->tsn_base & 0xffff));

    tail = *(uint32_t *)((char *)(blk) + blksize - sizeof(uint32_t));

    return (value == tail);
}

int table_block_verify(_blk_t *blk, uint32_t dba, uint16_t blksize) {
    if (blk->cl.type != TABLE_BLOCK_TYPE)
        return 0;
    if (blk->cl.dba != dba)
        return 0;
    if (!block_checksum(blk, blksize))
        return 0;
    return 1;
}

_rp_t * table_block_get_row_piece(_blk_t *blk, int rowno) {
    _dblk_dl_t *dl = dblk_get_dl(blk);
    _rp_t *rp;

    if (dl->rowdir[rowno] == 0)
        return NULL;

    rp = ((_rp_t *)((char *)blk + dl->rowdir[rowno]));

    if ((rp->flags & HEAD_ROW_PIECE) == 0 ||
        (rp->flags & DELETED_ROW_PIECE) != 0)
        return NULL;

    return rp;
}
