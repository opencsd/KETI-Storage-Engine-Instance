#include "ex.h"
#include "data_blk.h"

/* - filter info deserialize 함수
 * 1) allocator 유무 확인 필요. 현재는 malloc 사용
 * 2) byte stream으로 보내주는 것으로 가정함. 전송 방식이 달라지면 수정 필요 */
filter_info_t *
deserialize_filter_info(uchar **data)
{
    filter_info_t *filter_info;
    uchar *pos = (uchar *)(*data);
    int exp_cnt;
    int i;
    int rp_len;

    filter_info = (filter_info_t *)malloc(sizeof(filter_info_t));

    /* expression & column cnt */
    filter_info->exp_cnt = *((int *)pos);
    pos += sizeof (int);
    exp_cnt = filter_info->exp_cnt;

    filter_info->col_cnt = *((int *)pos);
    pos += sizeof (int);

    /* op_types */
    filter_info->op_types =
        (exp_op_type_t *)malloc(sizeof(exp_op_type_t) * exp_cnt);
    memcpy (filter_info->op_types, pos, sizeof(exp_op_type_t) * exp_cnt);
    pos += sizeof (exp_op_type_t) * exp_cnt;

    /* exp nos */
    filter_info->exp_l_nos = (int16_t *)malloc(sizeof(int16_t) * exp_cnt);
    memcpy (filter_info->exp_l_nos, pos, sizeof(int16_t) * exp_cnt);
    pos += sizeof (int16_t) * exp_cnt;

    filter_info->exp_r_nos = (int16_t *)malloc(sizeof(int16_t) * exp_cnt);
    memcpy (filter_info->exp_r_nos, pos, sizeof(int16_t) * exp_cnt);
    pos += sizeof (int16_t) * exp_cnt;

    /* constant values */
    filter_info->const_vals_cnt = *((int *)pos);
    pos += sizeof (int);

    if (filter_info->const_vals_cnt != 0) {
        filter_info->const_vals =
            (uchar **)malloc(sizeof(uchar *) * filter_info->const_vals_cnt);

        for (i = 0; i < filter_info->const_vals_cnt; i++) {
            rp_len = rpcol_get_len (pos);
            filter_info->const_vals[i] = (uchar *)malloc(sizeof(uchar) * rp_len);
            memcpy (filter_info->const_vals[i], pos, rp_len);
            pos += rp_len;
        }
    }
    else
        filter_info->const_vals = NULL;

    return filter_info;
}

static void
filter_info_free (filter_info_t *filter_info)
{
    free (filter_info->op_types);
    free (filter_info->exp_l_nos);
    free (filter_info->exp_r_nos);

    if (filter_info->const_vals) {
        int i;
        for (i = 0; i < filter_info->const_vals_cnt; i++)
            free (filter_info->const_vals[i]);

        free (filter_info->const_vals);
    }

    free(filter_info);
}

void
build_data (exp_ctx_t *exp_ctx, uchar *rp_data, int col_cnt, int rp_col_cnt)
{
    int i;

    for (i = 0; i < rp_col_cnt; i++) {
        exp_ctx->column_vals[i] = rp_data;
        rp_data += rpcol_get_len(rp_data);
    }

    /* TODO : NULL 처리 확인
    for (; i < col_cnt; i++) {
        exp_ctx->column_vals[i] = NULL_DATA;
    }
    */

}

/* 1) number, string type compare 비교 수정 필요 */
bool
exp_eval_condition (uchar *vals_l, uchar *vals_r, exp_op_type_t op_type)
{
    int cmp;
    int minsize, l_size, r_size, prefix_cmp;
/* 일반 compare
    cmp = memcmp (rpcol_get_data (vals_l), rpcol_get_data (vals_r),
                  rpcol_get_size (vals_l));
 */

    l_size = vals_l[0];
    r_size = vals_r[0];
    minsize = (l_size <= r_size) ? l_size : r_size;
    prefix_cmp = memcmp (((char *) vals_l + 1), ((char *) vals_r + 1), minsize);
    cmp = (prefix_cmp != 0 ? prefix_cmp : (l_size - r_size));

    switch (op_type) {
    case EXP_OP_EQUAL:
        if (cmp == 0)
            return true;
        else
            return false;
    case EXP_OP_NE:
        if (cmp != 0)
            return true;
        else
            return false;
    case EXP_OP_GT:
        if (cmp > 0)
            return true;
        else
            return false;
    case EXP_OP_LT:
        if (cmp < 0)
            return true;
        else
            return false;
    case EXP_OP_GE:
        if (cmp < 0)
            return false;
        else
            return true;
    case EXP_OP_LE:
        if (cmp > 0)
            return false;
        else
            return true;
    default:
        return false;
    }

    return false;
}


/* - filter에 사용된 expression evaluation하여 filtering 하는 함수 */
bool
exp_eval_and_check_true (filter_info_t *filter_info, exp_ctx_t *exp_ctx)
{
    int i;

    for (i = 0; i < filter_info->exp_cnt; i++) {
        uchar *vals_l;
        uchar *vals_r;
        int const_idx = 0;
        int exp_nos;

        /* exp_l_nos / exp_r_nos 가 -1이면, constant values */
        exp_nos = filter_info->exp_l_nos[i];
        if (exp_nos != -1)
            vals_l = exp_ctx->column_vals[exp_nos];
        else
            vals_l = filter_info->const_vals[const_idx++];
        exp_nos = filter_info->exp_r_nos[i];
        if (exp_nos != -1)
            vals_r = exp_ctx->column_vals[exp_nos];
        else
            vals_r = filter_info->const_vals[const_idx++];

        if (vals_l == NULL_DATA || vals_r == NULL_DATA)
            return false;

        if (!exp_eval_condition(vals_l, vals_r, filter_info->op_types[i]))
            return false;
    }

    return true;
}

static inline res_chunk_t *
res_chunk_init ()
{
   res_chunk_t *res_chunk = (res_chunk_t *)malloc(sizeof(res_chunk_t));

   res_chunk->res_buf = (uchar *)malloc(sizeof(uchar) * RES_CHUNK_SIZE);
   res_chunk->res_len = 0;

   return res_chunk;
}

/* - filter 조건을 통과한 row piece들을 chunk에 serialize하는 함수
 * 1) chunk에 serialize하는 data size가 RES_CHUNK_SIZE(64K) 넘어가는 경우 */
static void
serialize_rpcols_to_chunk (exp_ctx_t *exp_ctx, int col_cnt,
                           res_chunk_t *res_chunk)
{
    int i;
    int col_len;

    for (i = 0; i < col_cnt; i++) {
        col_len = rpcol_get_len(exp_ctx->column_vals[i]);
        memcpy(res_chunk->res_buf, exp_ctx->column_vals[i], col_len);
        res_chunk->res_len += col_len;
    }

}

static inline void
exp_ctx_init (exp_ctx_t *exp_ctx, int col_cnt)
{
    exp_ctx->column_vals = (uchar **)malloc(sizeof(uchar *) * col_cnt);
}

static inline void
exp_ctx_free (exp_ctx_t *exp_ctx)
{
    free(exp_ctx->column_vals);
}

/* - 읽어들인 block들에 대한 filtering 등 후처리 함수
 * 1) chained rp 처리 필요한지 검토 필요
 * 2) out expression은 tibero server에서 처리 */
res_chunk_t *
csd_filter_and_eval_out (_blk_t *blk, filter_info_t *filter_info)
{
    int rowno, rpcnt;
    _dblk_dl_t *dl;
    _rp_t *rp;
    exp_ctx_t exp_ctx;
    res_chunk_t *res_chunk;

    dl = dblk_get_dl(blk);
    rpcnt = dl->rowcnt;

    exp_ctx_init(&exp_ctx, filter_info->col_cnt);
    res_chunk = res_chunk_init();

    for (rowno = 0; rowno < rpcnt; rowno ++) {
        rp = table_block_get_row_piece(blk, rowno);

        if (rp == NULL)
            continue;

        build_data (&exp_ctx, rp_get_data (rp), filter_info->col_cnt,
                    rp->colcnt);
        if (filter_info->exp_cnt > 0 &&
            !exp_eval_and_check_true(filter_info, &exp_ctx))
            continue;

        serialize_rpcols_to_chunk(&exp_ctx, filter_info->col_cnt, res_chunk);
    }

    exp_ctx_free(&exp_ctx);
    filter_info_free(filter_info);

    return res_chunk;
}

/* end of ex.c */
