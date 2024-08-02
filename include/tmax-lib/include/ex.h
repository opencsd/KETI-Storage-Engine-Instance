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

#define NULL_DATA   ((uchar *)0)

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

    exp_op_type_t *op_types;

    int16_t *exp_l_nos;
    int16_t *exp_r_nos;

    int const_vals_cnt;
    uchar **const_vals;
} filter_info_t;

typedef struct exp_ctx_s {
    uchar **column_vals;
} exp_ctx_t;

#define RES_CHUNK_SIZE          65536

typedef struct res_chunk_s {
    uchar *res_buf;
    int res_len;
} res_chunk_t;

void build_data (exp_ctx_t *exp_ctx, uchar *rp_data, int col_cnt,
                 int rp_col_cnt);
bool exp_eval_condition (uchar *vals_l, uchar *vals_r, exp_op_type_t op_type);
bool exp_eval_and_check_true (filter_info_t *filter_info, exp_ctx_t *exp_ctx);

filter_info_t *deserialize_filter_info (uchar **data);
res_chunk_t *csd_filter_and_eval_out (_blk_t *blk, filter_info_t *filter_info);

#endif /* no _EX_H */
