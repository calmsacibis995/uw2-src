/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)amigo:common/l_unroll.h	1.7"

typedef struct Loop_unroll_info {
	Cgq_index loop_cbranch_index, loop_label_index, loop_increment_index,
		loop_start, loop_cond, loop_body, loop_end;
	Block loop_start_block, loop_cond_block, loop_end_block;
		/* loop->header is the block containing os_loop_body  */
		/* loop_end_block not required? it's in loop->end */
	Block loop_test_block;
	Block body_begin; /* First block of loop body */
	Block body_end;
	Expr loop_counter; /* Loop variable */
	void *sr_candidates;
	ND1 *begin_limit, *end_limit;
	long increment, iterations;
	int label; /* Top of loop.  Needed?? */
	int rel_op; /* Op code of rel op in loop test */
} *Loop_unroll_info;

#define SR_TEMP_LIST(loop) (loop)->loop_unroll->sr_candidates

struct Label_descriptor {
	int map_label;	/* 
			** if map_label is not zero, any branch to label
			** with this descriptor should be replaced by branch
			** to map_label -- this assumes 0 is not a valid
			** label.
			** This descriptor is reused by loop_unrolling
			** to map copied labels.
			*/
	Block block; /* location of label */
};

	/* interface to blocks.c */

		/* label_table interface */
extern struct Label_descriptor *label_table;
extern PROTO(get_max_label,(void));
extern PROTO(get_min_label,(void));
extern struct Label_descriptor *PROTO(get_label_descriptor, (int label));
extern void PROTO(label_table_alloc, (Arena));
extern void PROTO(label_table_zero, (void));

extern Block PROTO(new_block_alloc, (void));
#define CGQ_CONTINUE continue /* move to cgstuff.h if it works */

#define BEGIN_LIMIT(loop) (loop->loop_unroll->begin_limit->lval) /* for now */
#define END_LIMIT(loop) (loop->loop_unroll->end_limit ? loop->loop_unroll->end_limit->lval : 0) /* for now */
#define ITERATIONS(loop) (loop->loop_unroll->iterations) /* for now */
#define INCREMENT(loop) (loop->loop_unroll->increment) /* for now */
