#include "ex.h"
#include "data_blk.h"

uchar NULL_COLUMN[4] = {0x00, 0x00, 0x00, 0x00};

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
    printf("malloc1");
    filter_info = (filter_info_t *)malloc(sizeof(filter_info_t));
    printf("malloc2");   
    /* expression & column cnt */
    filter_info->exp_cnt = *((int *)pos);
    pos += sizeof (int);
    exp_cnt = filter_info->exp_cnt;

    filter_info->col_cnt = *((int *)pos);
    pos += sizeof (int);

    filter_info->use_cols = (bool *)malloc(sizeof(bool) * filter_info->col_cnt);
    memcpy (filter_info->use_cols, pos, sizeof(bool) * filter_info->col_cnt);
    pos += sizeof (bool) * filter_info->col_cnt;

    filter_info->use_col_cnt = *((int *)pos);
    pos += sizeof (int);

    filter_info->all_max_colno = *((int *)pos);
    pos += sizeof (int);

    filter_info->where_max_colno = *((int *)pos);
    pos += sizeof (int);

    filter_info->is_char_type = (bool *)malloc(sizeof(bool) * exp_cnt);
    memcpy (filter_info->is_char_type, pos, sizeof(bool) * exp_cnt);
    pos += sizeof (bool) * exp_cnt;

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

void
filter_info_free (filter_info_t *filter_info)
{
    free (filter_info->use_cols);
    free (filter_info->is_char_type);
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
build_data (exp_ctx_t *exp_ctx, uchar *rp_data, int col_cnt, int rp_col_cnt,
            bool *usecols, int usecol_cnt)
{
    int i;
    int remained_col_cnt = usecol_cnt;

    /* column들이 모두 사용될 때 */
    if (usecols == NULL) {
        int min_col_cnt = (col_cnt > rp_col_cnt) ? rp_col_cnt : col_cnt;

        for (i = 0; i < min_col_cnt; i++) {
            exp_ctx->column_vals[i] = rp_data;
            exp_ctx->column_val_lens[i] = rpcol_get_len(rp_data);
            rp_data += rpcol_get_len(rp_data);
        }

        /* set null */
        for (; i < col_cnt; i++) {
            exp_ctx->column_vals[i] = 0;
            exp_ctx->column_val_lens[i] = 1;
        }
    }
    else {
        for (i = 0; i < col_cnt; i++) {
            if (usecols[i]) {
                exp_ctx->column_vals[i] = rp_data;
                exp_ctx->column_val_lens[i] = rpcol_get_len(rp_data);
                remained_col_cnt--;
            }

            if (remained_col_cnt <= 0)
                return;

            if (i < rp_col_cnt - 1)
                rp_data += rpcol_get_len(rp_data);
        }
    }
}

bool
exp_eval_condition (uchar *vals_l, uchar *vals_r, exp_op_type_t op_type,
                    bool is_char_type)
{
    int cmp;
    int minsize, l_size, r_size, prefix_cmp;
    int i;
    uchar *str;

    l_size = rpcol_get_size (vals_l);
    r_size = rpcol_get_size (vals_r);
    minsize = (l_size <= r_size) ? l_size : r_size;

    /* NULL이 있는 경우 false 처리 */
    if (minsize == 0)
        return false;

    prefix_cmp = memcmp (rpcol_get_data (vals_l), rpcol_get_data (vals_r),
                         minsize);
    if (!is_char_type)
        cmp = (prefix_cmp != 0 ? prefix_cmp : (l_size - r_size));
    else {
        if (prefix_cmp == 0) {
            cmp = 0;
            if (l_size < r_size) {
                str = rpcol_get_data (vals_r);
                for (i = l_size; i < r_size; i++) {
                    if (str[i] != ' ') {
                        cmp = -1;
                        break;
                    }
                }
            }
            else if (l_size > r_size) {
                str = rpcol_get_data (vals_l);
                for (i = r_size; i < l_size; i++) {
                    if (str[i] != ' ') {
                        cmp = 1;
                        break;
                    }
                }
            }
        }
        else
            cmp = prefix_cmp;
    }

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
    int const_idx = 0;

    for (i = 0; i < filter_info->exp_cnt; i++) {
        uchar *vals_l;
        uchar *vals_r;
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

        if (rpcol_get_size(vals_l) == 0 || rpcol_get_size(vals_r) == 0)
            return false;

        if (!exp_eval_condition(vals_l, vals_r, filter_info->op_types[i],
            filter_info->is_char_type[i]))
            return false;
    }

    return true;
}

static chunk_t *
create_chunk ()
{
    chunk_t *chunk = (chunk_t *)malloc(sizeof(chunk_t));

    chunk->buf = (uchar *)malloc(sizeof(uchar) * RES_CHUNK_SIZE);
    chunk->used = 0;

    return chunk;
}

static void
add_chunk (chunk_list_t *chunk_list, bool is_chained)
{
    if (is_chained) {
        chunk_list->chained_chunk_cnt++;

        chunk_list->chained_chunk = (chunk_t **)realloc(chunk_list->chained_chunk,
                                                        chunk_list->chained_chunk_cnt *
                                                        sizeof(chunk_t *));

        chunk_list->chained_chunk[chunk_list->chained_chunk_cnt - 1] = create_chunk();
    }
    else {
        chunk_list->res_chunk_cnt++;

        chunk_list->res_chunk = (chunk_t **)realloc(chunk_list->res_chunk,
                                                    chunk_list->res_chunk_cnt *
                                                    sizeof(chunk_t *));

        chunk_list->res_chunk[chunk_list->res_chunk_cnt - 1] = create_chunk();
    }
}

chunk_list_t *
create_chunk_list ()
{
   chunk_list_t *chunk_list = (chunk_list_t *)malloc(sizeof(chunk_list_t));

   chunk_list->res_chunk = NULL;
   chunk_list->res_chunk_cnt = 0;

   chunk_list->chained_chunk = NULL;
   chunk_list->chained_chunk_cnt = 0;

   return chunk_list;
}

void
free_chunk (chunk_t *chunk) {
    free(chunk->buf);
    free(chunk);
}

void
free_chunk_list (chunk_list_t *chunk_list) {
    int i;

    for (i = 0; i < chunk_list->res_chunk_cnt; i++) {
        free_chunk (chunk_list->res_chunk[i]);
    }

    for (i = 0; i < chunk_list->chained_chunk_cnt; i++) {
        free_chunk (chunk_list->chained_chunk[i]);
    }

    free(chunk_list->res_chunk);
    free(chunk_list->chained_chunk);
    free(chunk_list);
}

/* big chunk (통 buffer)에 chunk list 결과 serialize
 * big chunk size를 넘어갈 경우 어느 chunk까지 썼는지 last_chunk_no 에 기록해두고,
 * 다시 함수 들어왔을 때 이어서 쓸 수 있도록 함. */
bool
write_chunk_list_to_buffer (chunk_list_t *chunk_list, uchar **input_big_chunk,
                            int big_chunk_size, int *current_chunk_idx,
                            int *total_len)
{
    int i;
    chunk_t *cur_chunk;
    int buffer_offset = 0;
    int total_chunk_cnt = chunk_list->res_chunk_cnt + chunk_list->chained_chunk_cnt;
    uchar *big_chunk = (uchar *)(*input_big_chunk);
    uchar chunk_flag;

    *total_len = 0;
    for (i = *current_chunk_idx; i < total_chunk_cnt; i++) {
        if (i < chunk_list->res_chunk_cnt) {
            cur_chunk = chunk_list->res_chunk[i];
            chunk_flag = 0;         //  res chunk에서 가져온 경우 chunk_flag 0
        }
        else {
            cur_chunk = chunk_list->chained_chunk[i];
            chunk_flag = 1;         //  chained chunk에서 가져온 경우 chunk_flag 1
        }

        if (buffer_offset + cur_chunk->used + 3 > big_chunk_size) {
            *current_chunk_idx = i;
            return false;
        }

        big_chunk[buffer_offset++] = (cur_chunk->used >> 8) & 0xFF;
        big_chunk[buffer_offset++] = (cur_chunk->used) * 0xFF;
        big_chunk[buffer_offset++] = chunk_flag;
        *total_len += 3;

        memcpy (big_chunk + buffer_offset, cur_chunk->buf, cur_chunk->used);
        buffer_offset += cur_chunk->used;
        *total_len += cur_chunk->used;
    }

    return true;
}

/* - filter 조건을 통과한 row piece들을 chunk에 serialize하는 함수 */
void
serialize_rpcols_to_chunk (exp_ctx_t *exp_ctx, int col_cnt,
                           chunk_list_t *chunk_list, int total_len)
{
    int i;
    int col_len;

    if (chunk_list->res_chunk_cnt == 0 ||
        chunk_list->res_chunk[chunk_list->res_chunk_cnt - 1]->used +
        total_len > RES_CHUNK_SIZE)
        add_chunk(chunk_list, false);

    for (i = 0; i < col_cnt; i++) {
        col_len = rpcol_get_len(exp_ctx->column_vals[i]);
        memcpy(chunk_list->res_chunk[chunk_list->res_chunk_cnt - 1]->buf +
               chunk_list->res_chunk[chunk_list->res_chunk_cnt - 1]->used,
               exp_ctx->column_vals[i], col_len);

        chunk_list->res_chunk[chunk_list->res_chunk_cnt - 1]->used += col_len;
    }
}

static inline void
exp_ctx_init (exp_ctx_t *exp_ctx, int col_cnt)
{
    exp_ctx->column_vals = (uchar **)malloc(sizeof(uchar *) * col_cnt);
    exp_ctx->column_val_lens = (uint16_t *)malloc(sizeof(uint16_t) * col_cnt);
}

static inline void
exp_ctx_free (exp_ctx_t *exp_ctx)
{
    free(exp_ctx->column_vals);
    free(exp_ctx->column_val_lens);
}

int
set_col_out (exp_ctx_t *exp_ctx, int col_cnt, bool *usecols, int usecol_cnt)
{
    int i;
    int total_len = 0;

    if (usecols == NULL) {
        if (usecol_cnt == 0) {
            /* no column needed */
            for (i = 0; i < col_cnt; i++) {
                exp_ctx->column_vals[i] = NULL_COLUMN;
                exp_ctx->column_val_lens[i] = 1;
                total_len += exp_ctx->column_val_lens[i];
            }
        }
        else {
            /* all column needed */
            for (i = 0; i < col_cnt; i++) {
                exp_ctx->column_val_lens[i] = rpcol_get_len (exp_ctx->column_vals[i]);
                total_len += exp_ctx->column_val_lens[i];
            }
        }
    }
    else {
        /* specified column needed */
        for (i = 0; i < col_cnt; i++) {
            if (usecols[i]) {
                exp_ctx->column_val_lens[i] = rpcol_get_len (exp_ctx->column_vals[i]);
            }
            else {
                exp_ctx->column_vals[i] = NULL_COLUMN;
                exp_ctx->column_val_lens[i] = 1;
            }
            total_len += exp_ctx->column_val_lens[i];
        }
    }

    return total_len;
}

static void
serialize_rpcols_to_chained_chunk (exp_ctx_t *exp_ctx, int col_cnt,
                                   chunk_list_t *chunk_list, int total_len)
{
    int i;
    int col_len;

    if (chunk_list->chained_chunk_cnt == 0 ||
        chunk_list->chained_chunk[chunk_list->chained_chunk_cnt - 1]->used +
        total_len > RES_CHUNK_SIZE)
        add_chunk(chunk_list, true);

    for (i = 0; i < col_cnt; i++) {
        col_len = exp_ctx->column_val_lens[i];
        memcpy(chunk_list->chained_chunk[chunk_list->chained_chunk_cnt - 1]->buf,
               exp_ctx->column_vals[i], col_len);

        chunk_list->chained_chunk[chunk_list->chained_chunk_cnt - 1]->used += col_len;
    }
}

static bool
process_chained_row (exp_ctx_t *exp_ctx, _rp_t *rp, chunk_list_t *chunk_list,
                     filter_info_t *filter_info)
{
    int rp_col_cnt = rp->colcnt;
    int normal_col_cnt = ((((rp)->flags & LASTCOL_CONT_ROW_PIECE) != 0) ?
                          rp_col_cnt - 1 : rp_col_cnt);
    int total_len;
    int all_max_colno = filter_info->all_max_colno;
    int where_max_colno = filter_info->where_max_colno;

    /* 현재 block 의 rp만으로도 처리 가능 */
    if (all_max_colno < normal_col_cnt)
        return false;

    build_data (exp_ctx, rp_get_data (rp), rp_col_cnt, rp_col_cnt,
                filter_info->use_cols, ((rp_col_cnt > filter_info->use_col_cnt) ?
                filter_info->use_col_cnt : rp_col_cnt));

    if (filter_info->exp_cnt > 0 && where_max_colno < normal_col_cnt) {
        if (!exp_eval_and_check_true(filter_info, exp_ctx))
            return true;
    }

    total_len = set_col_out (exp_ctx, rp_col_cnt, filter_info->use_cols,
                             filter_info->use_col_cnt);

    serialize_rpcols_to_chained_chunk (exp_ctx, rp_col_cnt, chunk_list,
                                       total_len);

    return true;
}

/* - 읽어들인 block들에 대한 filtering 등 후처리 함수
 * 1) chained rp 처리 필요한지 검토 필요
 * 2) out expression은 tibero server에서 처리
 * 3) 이 함수를 부르는 쪽에서 create_chunk_list() 호출하고, chunk_list를 넘겨줘야 함. */
void
csd_filter_and_eval_out (_blk_t *blk, filter_info_t *filter_info,
                         chunk_list_t *chunk_list)
{
    int rowno, rpcnt;
    _dblk_dl_t *dl;
    _rp_t *rp;
    exp_ctx_t exp_ctx;
    int total_len;

    dl = dblk_get_dl(blk);
    rpcnt = dl->rowcnt;

    exp_ctx_init(&exp_ctx, filter_info->col_cnt);

    for (rowno = 0; rowno < rpcnt; rowno ++) {
        rp = table_block_get_row_piece(blk, rowno);

        if (rp == NULL)
            continue;

        /* chained row 처리 */
        if (((rp)->flags & FIRSTPIECE_ROW_PIECE) == 0 ||
            ((rp)->flags & LASTPIECE_ROW_PIECE) == 0) {
            if (process_chained_row(&exp_ctx, rp, chunk_list, filter_info))
                continue;
        }

        build_data (&exp_ctx, rp_get_data (rp), filter_info->col_cnt,
                    rp->colcnt, filter_info->use_cols, filter_info->use_col_cnt);
        if (filter_info->exp_cnt > 0 &&
            !exp_eval_and_check_true(filter_info, &exp_ctx))
            continue;

        total_len = set_col_out (&exp_ctx, filter_info->col_cnt,
                                 filter_info->use_cols, filter_info->use_col_cnt);

        serialize_rpcols_to_chunk(&exp_ctx, filter_info->col_cnt, chunk_list,
                                  total_len);
    }

    exp_ctx_free(&exp_ctx);
}

/* end of ex.c */
