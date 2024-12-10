#ifndef _EX_H
#define _EX_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define rpcol_get_len(col)                                                     \
        ((int)*(col) <= 250                                                    \
         ? (int)*(col) + 1                                                     \
         : (int)((*((col) + 1) << 8) + *((col) + 2)) + 3)

#define rpcol_get_size(col)                                                    \
        ((int)*(col) <= 250                                                    \
         ? (int)*(col)                                                         \
         : (int)((*((col) + 1) << 8) + *((col) + 2)))

#define rpcol_get_data(col)                                                    \
        ((int)*(col) <= 252                                                    \
         ? col + 1                                                             \
         : col + 3)

typedef unsigned char uchar;
typedef struct _blk_s _blk_t;

typedef enum exp_op_type_e {
    EXP_OP_NONE         =   0,
    EXP_OP_OR           =   1, /* OR */
    EXP_OP_AND          =   2, /* AND */
    EXP_OP_NOT          =   3, /* NOT */
    EXP_OP_EQUAL        =   4, /* EQUAL */
    EXP_OP_NE           =   5, /* <> */
    EXP_OP_GT           =   6, /* > */
    EXP_OP_LT           =   7, /* < */
    EXP_OP_GE           =   8, /* >= */
    EXP_OP_LE           =   9, /* <= */
    EXP_OP_MAX          =   10
} exp_op_type_t;

typedef struct filter_info_s {
    int exp_cnt;
    int col_cnt;

    bool *use_cols;
    int use_col_cnt;

    int all_max_colno;
    int where_max_colno;

    bool *is_char_type;
    exp_op_type_t *op_types;

    int16_t *exp_l_nos;
    int16_t *exp_r_nos;

    int const_vals_cnt;
    uchar **const_vals;
} filter_info_t;

typedef struct exp_ctx_s {
    uchar **column_vals;
    uint16_t *column_val_lens;
} exp_ctx_t;

#define RES_CHUNK_SIZE          (65536 - 3)

typedef struct chunk_s {
    uchar *buf;
    int used;
} chunk_t;

typedef struct chunk_list_s {
    chunk_t **res_chunk;
    int res_chunk_cnt;

    chunk_t **chained_chunk;
    int chained_chunk_cnt;
} chunk_list_t;

void build_data (exp_ctx_t *exp_ctx, uchar *rp_data, int col_cnt,
                 int rp_col_cnt, bool *usecols, int usecol_cnt);
bool exp_eval_condition (uchar *vals_l, uchar *vals_r, exp_op_type_t op_type,
                         bool is_char_type);
bool exp_eval_and_check_true (filter_info_t *filter_info, exp_ctx_t *exp_ctx);

filter_info_t *deserialize_filter_info (uchar **data);
void filter_info_free (filter_info_t *filter_info);

chunk_list_t *create_chunk_list ();

void free_chunk (chunk_t *chunk);
void free_chunk_list (chunk_list_t *chunk_list);

bool write_chunk_list_to_buffer (chunk_list_t *chunk_list, uchar **input_big_chunk,
                                 int big_chunk_size, int *current_chunk_idx,
                                 int *total_len);
void serialize_rpcols_to_chunk (exp_ctx_t *exp_ctx, int col_cnt,
                                chunk_list_t *chunk_list, int total_len);
int set_col_out (exp_ctx_t *exp_ctx, int col_cnt, bool *usecols, int usecol_cnt);


void csd_filter_and_eval_out (_blk_t *blk, filter_info_t *filter_info,
                              chunk_list_t *chunk_list);

#endif /* no _EX_H */
