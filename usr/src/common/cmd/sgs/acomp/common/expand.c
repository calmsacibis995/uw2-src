/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)acomp:common/expand.c	1.31"
/* expand.c */

/* This module implements inline expansion. The parser calls inline_begf
   at the beginning of each function, and calls inline_endf at the end of
   each function that can be optimized. After each function has been parsed
   the parser calls inline_eof.

   Inline expansion proceeds in three major phases.
	1. The construction of the call graph.

	2. Construction of a call tree that indictes which calls to expand.

	3. The inline expansions
*/

#ifndef NO_AMIGO

#include "p1.h"
#include "mfile2.h"
#include "arena.h"
#include "bitvector.h"
#include <unistd.h>

/* should be exported by arena.h */
extern Arena global_arena;

int do_inline = 0;		/* For now default is NO inlining */

static int verbose_cpp_diag = 0;	/* Flag to control printing of C++
					   inlining warnings.  Turned on with
					   -1I32 to print warnings. */

/* Various settings of the do_inline switch. */
#define NO_INLINE	0	/* No inlining is to be performed. */
#define FULL_C_INLINE	1	/* Inline based upon a set of hueristics. */
#define CPP_INLINE_ONLY	2	/* Attempt to inline only those C++ functions
				   marked by a #pragma cplusplus_inline or
				   #pragma compiler_generated_inline. */
#define FULL_CPP_INLINE	3	/* Inline based on the heuristcs but
				   heavily weight the benefit of inlining
				   for explicit or implicit C++ inline
				   function. */
#define INLINE_ALL	4	/* Attempt to do all possible inlining,
				   without concern for growth limits. */
#define REM_UNREF_STAT	8	/* Only run enough of the inliner logic to
				   locate and eliminate all unreferenced
				   static functions. */

#define VERBOSE_CPP_SW	32	/* Inlining option from CC driver to 
				   elicit C++ inlining warnings. */ 

/* Maximimum number of nodes permissible in function to be inlined */
#define IL_EXPANSION 20
#define IL_BENEFIT   20

/* limits functions saved for inlining */
#define NODE_MAX 150000
#define FUNC_MAX 200

/* Weight to be applied when the call is to a C++ "inline" function. */
#define CPP_WEIGHT	300	/* Extremely "heavy" value to be certain
				   that C++ inline functions will be 
				   inlined when feasible. */

static int node_max = NODE_MAX;
static int func_max = FUNC_MAX;

typedef int Call_site;
typedef struct Funct *Function;


/* The following value are used for the flags in 

	Call_list
	Summary
	Funct

   to reflect the varios reasons that an inline request cannot
   be honored.  For C++ code, warning diagnostics will be emitted
   to explain why a user specified or implied function inline
   is not done.

   Care should be taken to avoid conflicts with the values of other
   flags used in each struct.  
*/
/* values for Call-list flag */
#define CNI_START_LOOP	(1<<8)		/* Inside the code for a start 
					   of a loop */
#define CNI_COLON_EXPR	(1<<9)		/* Part of a colon expression */
#define CNI_BOOL_COMMA	(1<<10)		/* Right operand of a &&, || or 
					   comma expr. */
#define CNI_ARG_COUNT	(1<<11)		/* Argument count does not match 
					   formal param count. */
#define CNI_ARG_TYPE	(1<<12)		/* Argument machine type is not
					   compatible with that of the formal
					   parameter. */

#define CNI_AT_CALL_SITE (CNI_START_LOOP | \
			  CNI_COLON_EXPR | \
			  CNI_BOOL_COMMA | \
			  CNI_ARG_COUNT  | \
			  CNI_ARG_TYPE)


/* values for Summary flag */
#define CNI_IS_VARARG	(1<<16)		/* variable argument function */
#define CNI_RET_SU	(1<<17)		/* returns a struct or union */
#define CNI_SU_ARG	(1<<18)		/* has struct or union argument */
#define CNI_STATIC_LOCAL (1<<19)	/* has static local variable */
#define CNI_UNKNWN_PRMS (1<<20)		/* unknown param list */

#define CNI_FUNCTION (CNI_IS_VARARG    | \
		      CNI_RET_SU       | \
		      CNI_SU_ARG       | \
		      CNI_STATIC_LOCAL | \
		      CNI_UNKNWN_PRMS)

/* values for Funct flags */
#define CPP_USER_INLINE	(1<<16)		/* #pragma Cplusplus_inline */
#define CPP_CPFE_INLINE	(1<<17)		/* #pragma compiler_generated_inline */



/* following used for args  */
#define ARG_CONST 1
#define ARG_KNOWN_PTR 2
#define ARG_COPY_PROP_CAND 4

/* value for Call_list flags */
#define DELETE_ON_EXPAND 1
#define DELETED 2
#define NOT_PRECOMPUTE 4
#define INLINE_AS_EXPR 8		/* This call can only be handled if
					   the function is a single expression
					   that can be inserted directly 
					   as a replacement for the call. */


/* maximum value of f_value field */
#define MAX_VALUE 1<<30

struct Call_list {
	int flags;
	int order;		/* used to sequence calls */
	int value;		/* benefit of expansion */
	int f_value;		/* value weighted by frequency */
	int frequency;		/* how often function is called */
	Function func;		/* called function */
	Call_site call_site;
	int arg_count;
	char *args;		/* argument descriptors */
	T1WORD *types;		/* argument types */
	ND1 **arg_nodes;	/* argument nodes */
	int lineno;		/* line number of this call. */
	struct Call_list *next;
};

struct Formal {
	short uses;
	short read_only;
	short op_const;
};

/* following used in summary flags */
#define CONTAINS_LOOP	01
#define LEAF_FUNCTION	02
#define HAS_LOCALS	04

struct Summary {
	struct Call_list *call_list;
	int function_count;	/* bitvector index used for this function */
	int node_count;
	int formal_count;
	int flags;
	struct Formal *formals;
	T1WORD *types;		/* an array of formal types */

	ND1 *single_expression_return;
				/* Pointer to the ND1 that is the expression
				   returned in the single return statement
				   in this function.  If set there are
				   no other statements. */
};

struct Call_table_list {
	ND1 *node;
	Cgq_index index;
	struct Call_table_list *next;
};

/* flags for FUNC */
#define GENERATED 1	/* code already generated */
#define DEFINED 2	/* have definition */
#define CALLED 4	/* target edge in call graph */
#define CALLED_ONCE 8
#define NODELETE 32	/* cannot be deleted (address has been taken) 
			   or there exists a non-optimizable function
			*/
#define RECURSIVE 64	/* Function is recursive among currently known
			** functions.  We care about these because
			** they cause explosive growth, or, in the
			** absence of a control mechanism, infinite
			** growth. 
			*/

struct Funct {
	int flags;
	Cgq_index save_index;	/* last index for cgq */
	char *name;		/* function name */
	Arena save_arena;
	struct td save_cgq;
	SX sid;
	struct Summary summary;
	struct Funct *next;
	int lineno;			/* line number at inline_begf */
	char * filename;		/* file name at inline_begf */

	/* root of call_tree for this function */
	struct Call_tree_list *call_tree;

	struct Call_list *unique_call;	/* one call to this function */
	struct Funct *unique_caller;	/* caller of single call */

	/* following used to construct tables during open */

	/* min is lowest sid of auto or param, max is highest sid of auto
	   or param + 1, we assume that args and autos declared after
	   function, also ties us into a particular implementation of
	   symbol table
	*/
	int min_symbol, max_symbol;

	/* min is lowest label number, max is highest + 1 */
	/* used to map labels */
	int min_label, max_label;

	int expr_count;
	int times_called;	/* count of known calls to this function.
				   Decremented as functions are inlined. */

	int visited;		/* Used in searches */

	/* used to construct Call_table when function is opened */
	struct Call_table_list *call_table_list;
};

struct Call_table {
	Cgq_index index;
	ND1 *node;
};

/* flags for Funct_copy */ 
#define EDITING 1	/* changing function defintion */
#define EDIT_COPY 2	/* changing function definition after making copy */
#define INLINING 0	/* copying function into caller */

struct Funct_copy {
	Arena arena;
	int flags;
	struct Funct *func;
	struct Call_table *call_table;
	int *label_table;
	SX *symbol_table;
	Cgq_index next_index;	/* used for reading */
};


/* datastructure for forest of call trees */
struct Call_tree_list {
	int open_flags;		/* same flags as Func_copy */
	struct Call_tree *tree;
	struct Call_tree_list *next;
};

struct Call_tree  {
	struct Funct *func;
	struct Tree_edge *edges;
};

struct Tree_edge {
	struct Call_list *call_list;	/* pointer to the Call_list struct
					   that is associated with this edge.
					*/
	struct Call_tree *callee;
	Call_site site;
	int order;		/* expansion order */
#ifndef NODBG
	int count;
#endif
	struct  Tree_edge *next;
};

/* used by get_precompute */
#define LEFT 0
#define RIGHT 1

static Function functions;
static Arena inline_arena = 0;
static int func_cnt;
static Function curr_func;
static int node_count = 0;
static int all_processed = 1;	/* every function in file processed */
static struct Funct *last_begf_seen = 0;
#ifndef NODBG
static int unref_stat_func_cnt;
static int deleted_stat_func_cnt;
static int cplusplus_func_not_callable;
static int call_to_cpp_function_not_inlined;
static int call_sites_expanded;
static int final_function_count;
static int added_node_count;
#endif  /* ! NODBG */



static void make_summary();
static int get_precompute();
static int read_cgq_item();
static void expand_calls();
static Cgq_index extend_scope();
static struct Tree_edge * add_edge();
static Function get_function();
static int process_node();
static int process_args();
static int make_value();
static int eval_value();
static struct Expansion_sites *make_site();
static void push_site();
static int get_pointer() ;
static void process_deletes();
static NODE *copy_tree();
static void restore();
static void enter_expansion();
static void copy();
static Function filter();
static void expand_call_site_tables();
static Call_site mark_call_site();
static int translate_label();
static SX translate_symbol();
static void sort_call_list();
static ND1 *next_argument();
static struct Call_table * get_call_site();
static struct Call_tree_list * top_down_order();
static ND1 * make_assign();
static struct Call_tree_list * make_call_trees();
static struct Call_tree_list * order_trees(); 
static void close_function();
static struct Funct_copy * open_function();
static void inline_copy();
static void substitute_args();
static void inline_expr();
static struct Summary * get_summary();
static void mark_recursive_functions();
static int is_recursive();

static void rewrite_top_level_question();
static void remove_unref_static_functions();
static void check_arg_types_against_formals();
static void try_relax_call_site_restrictions();
static Function emit_funct_inlining_diagnostics();
static void free_function_memory();

#ifndef NODBG
static void pr_graph();
void put_call_tree_list();
#endif

	/* Defines for debugging follow. To set one or more debugging
	   options add the values for the options obtaining x, and then
	   pass -IDx to acomp. -ID0 turns of debugging.

	   If a function contains inlining, and that function is not
	   processed by amigo, because of the -Gfuncs AMIGO's debugging option,
	   then the generated code will be bad.
	*/

#ifndef NODBG
int dbg_inline_flags = 0;

/* Print an inlining summary at the end of processing. */
#define DSUMMARY 1

/* indicates expansion sites and function deletions. Prints cgq_item containing
   call
*/
#define DEXPAND 2

/* prints the cgq after each function is parsed (not real useful) */
#define DCGQ 4 

/* same as DEXPAND, but prints expansion and cgq_item containing call after 
   expansion
*/ 
#define DEXPAND1 8

/* prints the call graph */
#define DGRAPH	16

/* Indicates the expansion sites. Also used to turn on and off selected
   inlining. However, if h calls f calls g. You cannot turn off "h calls f"
   without turning off "f calls g". Sites are numbered in the output.

   To inline sites 1,2,3,4,7,10, for example. Set the environment variable
   DCOUNT="1 2 3 4 7 10", and run acomp: acomp -I64
*/
#define DCOUNT 64

/* prints out the costs as an expansion is selected. Should be combined with
   DCGQ
*/
#define DBUILD_TREE 128

#endif


/* cannot delete a function whose address has been taken in the
   initialization of a static
*/
void
inline_address_function(func)
SX func;
{
	get_function(func)->flags |= NODELETE;
}

void
inline_flags(arg)
char *arg;
{
	static int zero_seen;
	int flags;

	if (*arg == 'D') {
		/* Process inline debugging options. */
		arg++;
#ifndef NODBG
		dbg_inline_flags |= atoi(arg);
#endif  /* ! NODBG */
	} else {
		/* Process inline options. */
		if (zero_seen)
			return;
		flags = atoi(arg);
		if (flags == 0) {
			zero_seen = 1;
			do_inline = 0;
		}
		else if (flags & VERBOSE_CPP_SW) {
			verbose_cpp_diag = 1;	/* print inlining warnings. */
			flags &= ~VERBOSE_CPP_SW;
		}
		/* Or in the current value of the flags.  OR-ing in a zero
		   does not hurt. */
		do_inline |= flags;
	}  /* if */
}

#ifndef NODBG
static int 
	il_leaf_only = 0,	/* only inline leaf functions */
	il_copy_only = 0,	/* only inline funcs with NAME/ICON params */
	il_read_only = 0,	/* as above, but read only params */
	il_no_loop_only = 0;	/* only inline functions with no loops */
#endif

int inline_expansion = IL_EXPANSION;
int inline_benefit = IL_BENEFIT;


static int func_lineno;		/* Line number at the beginning of the
				   the function definition. */
static char * filename;		/* Current file name at the start of
				   the function definition. */


	
void
inline_begf(sid) SX sid; {

	/* Pick up the current source file and line number. */
	func_lineno = er_getline();
	filename = st_lookup(er_curname());

   	if (!inline_arena) {
		inline_arena = arena_init();
#ifndef NODBG
		if (dbg_inline_flags) {
			arena_init_debug(inline_arena, "INLINE_ARENA");
		}  /* if */
#endif  /* NODBG */
	}  /* if */

	if ( all_processed && ( ! last_begf_seen) ) {
		/* This is the very first inline_begf() call, not a restart
		   because the maximum number of functions or nodes were 
		   processed. */
#ifndef NODBG
		/* Pick up any runtime alterations to inlining heuristics. */
		char *arg;
		if ((arg = getenv("IL_LEAF_ONLY")) != NULL)
			il_leaf_only = atoi(arg);
		if ((arg = getenv("IL_COPY_ONLY")) != NULL)
			il_copy_only = atoi(arg);
		if ((arg = getenv("IL_READ_ONLY")) != NULL)
			il_read_only = atoi(arg);
		if ((arg = getenv("IL_NO_LOOP_ONLY")) != NULL)
			il_no_loop_only = atoi(arg);
		if ((arg = getenv("IL_BENEFIT")) != NULL)
			inline_benefit = atoi(arg);
		if ((arg = getenv("IL_NODE_MAX")) != NULL)
			node_max = atoi(arg);
		if ((arg = getenv("IL_FUNC_MAX")) != NULL)
			func_max = atoi(arg);
#endif
	}

	/* don't see the endf if function not optimizable */
	else if ((!last_begf_seen) || !(last_begf_seen->flags & DEFINED)) {
		all_processed = 0;
	}  /* if */

	last_begf_seen = get_function(sid);

	/* used for making label and symbol translation tables */
	last_begf_seen->min_label = getlab() + 1;
	last_begf_seen->min_symbol= sid - TY_NPARAM(SY_TYPE(sid));
}

void
inline_endf(sid)
SX sid;
{
    Function new_func;

    ++func_cnt;
    new_func = get_function(sid);
    new_func->flags |= DEFINED;

    /* Before we save the current pointers into the CG queue and the
       current maximum label value, run through the CG queue looking for
       top level "?" operators or assignment whose right-hand operand is
       a "?" operator and rewrite these statements as an if-then-else
       equivalent set of CGQ items.

       NOTE: Currently this is only done when inlining C++ code (-1I2),
	     inlining the world (-1I4) or some variation of do_inline value
	     containing the 2 or 4 bit.  This performance enhancement should
	     be extended to C inlining as well.
    */
    if (do_inline & (CPP_INLINE_ONLY | INLINE_ALL)) {
	rewrite_top_level_question();
    }  /* if */



    new_func->save_index = cg_inline_endf();
    new_func->save_cgq = td_cgq;

    /* The following assignment to the sid may seem redundant, but it is not.
       The lookup in get_function may have found the function under 
       another sid number.  The lookup in get_function() is based on the
       function name which must be unique in a compilation unit.

		void foo() {
			extern void bar();		<< sid = 50

			bar();
		}
		static void bar() {}			<< sid = 65

       These are the same functions; the function declaration inside foo()
       was promoted to the file scope and the second symbol table entry
       was created with the SAMEAS rule.  When checking for static
       functions, we want to see the definition; not the earlier
       declaration.
    */
    new_func->sid = sid;

    new_func->save_arena = global_arena;

    /* Record the file name and line number of the beginning of the function
       definition. */
    new_func->lineno = func_lineno;
    new_func->filename = filename;

    /* used for making label and symbol translation tables */
    new_func->max_label = getlab();
    new_func->max_symbol = SY_MAX+1;		/* SY_MAX is id of last symbol*/

    curr_func = new_func;
    make_summary(sid, &new_func->summary);

    node_count += new_func->summary.node_count;

#ifndef NODBG
    if (dbg_inline_flags & DCGQ) {
	DPRINTF("\t\tputting CGQ for %s(%d)\n\n", SY_NAME(sid), sid);
	cg_putq(3); 
    }
#endif
    tinit();	/* get new arena */

    /* If inlining ALL functions, avoid limit checks.  This is a C++
       testing option and C compiler aborts will be tolerated.
    */
    if ( ! (do_inline & INLINE_ALL) ) {

	/* if exceed maximum function saving, then inline expand what we
	   have and start saving functions again
	*/
	if ((node_max && node_count > node_max) || 
	    (func_max && func_cnt > func_max)) {
#ifndef NODBG
		if (dbg_inline_flags) {
			arena_debug(inline_arena, "inline_endf()");
		}  /* if */
#endif
		if (verbose_cpp_diag) {
		   WERROR(gettxt(":1556",
		  "exceeded inline function limit of %d or node limit %d"),
			  func_max, node_max);
		   WERROR(gettxt(":1557",
			  "\tfull benefits of inlining will not be realized"));
		}  /* if */
		all_processed = 0;
		inline_eof();
		func_cnt = 0;
		functions = 0;
		last_begf_seen = 0;
		arena_term(inline_arena);
		inline_arena = 0;
		node_count = 0;
#ifndef NODBG
		/* Reset debugging counters. */
		unref_stat_func_cnt = 0;
		deleted_stat_func_cnt = 0;
		call_to_cpp_function_not_inlined = 0;
		cplusplus_func_not_callable = 0;
		call_sites_expanded = 0;
		final_function_count = 0;
		added_node_count = 0;

		if (dbg_inline_flags)
			DPRINTF("***** restarting inlining\n");
#endif  /* ! NODBG */
	}
      }  /* if */
}


void
mark_cpp_inline(sid, fe_generated)
SX  sid;
int fe_generated;
/*
   Special C++ routine to mark that a #pragma Cplusplus_inline or a
   #pragma compiler_generated_inline has been detected following the
   definition of a function.
*/
{
    Function new_func;

    new_func = get_function(sid);
    if (fe_generated) {
	new_func->flags |= CPP_CPFE_INLINE;
    } else {
	new_func->flags |= CPP_USER_INLINE;
    }  /* if */
}  /* mark_cpp_inline */



/****************************************************************************/
/****************************************************************************/
/*********************** make_summary pass **********************************/
/****************************************************************************/
/****************************************************************************/



static int
call_in_expression(node)
ND1  *node;
{
    int return_value = 0;

    if callop(node->op) 
	return 1;		/* Definitely a call in this expression. */

    if (optype(node->op) != LTYPE) {
	return_value = call_in_expression(node->left);
    }  /* if */
    if ( ! return_value && (optype(node->op) == BITYPE)) {
	return_value = call_in_expression(node->right);
    }  /* if */
    return return_value;
}  /* call_in_expression */


/* Run through the current CG queue looking for top-level "?" operators
   and rewrite these into the if-then-else equivalent.  Rewriting such
   an expression could make other opportunities possible.
*/
static void
rewrite_top_level_question()
{
    ND1 *lhs;
    ND1 *if_expr;
    ND1 *quest_expr;
    ND1 *then_expr;
    ND1 *else_expr;

    CGQ_FOR_ALL(cgq,index)

	if (cgq->cgq_op == CGQ_EXPR_ND1) {
	    /* This is an ND1 expression; check for candidates. */
	    ND1 * node = cgq->cgq_arg.cgq_nd1;
	    if_expr = 0;
	    if (node->op == QUEST) {
		lhs = 0;
		quest_expr = node;
		if_expr = node->left;
		then_expr = node->right->left;
		else_expr = node->right->right;

	    } else if ((asgop(node->op)) &&
		       ((quest_expr = node->right)->op == QUEST)) {
		lhs = node->left;
		if_expr = quest_expr->left;
		then_expr = quest_expr->right->left;
		else_expr = quest_expr->right->right;
	    }  /* if */

	    if (if_expr &&
		(call_in_expression(then_expr) ||
		 call_in_expression(else_expr))) {
		/* We have located a QUEST operator that may be able
		   to benefit from inlining if rewritten as an
		   if-then-else construct.  These new CG queue
		   items will be inserted before the current item
		   (current index value).
		*/
		Cgq_index prev_index;
		int else_label;
		int end_label;
		cgq_t *new_cgq;
		ND2   *pass2_node;
		int   cgq_ln = cgq->cgq_ln;
		int   cgq_dbln = cgq->cgq_dbln;

		else_label = getlab();
		end_label = getlab();
		/* If the question operator has an implied cast, it should
		   be expressed as an explicit conversion. */
		if (quest_expr->type != quest_expr->right->type) {
		    then_expr = tr_generic(CONV, then_expr, quest_expr->type);
		    else_expr = tr_generic(CONV, else_expr, quest_expr->type);
		}  /* if */

		/* Build the if portion of the code. */
		new_cgq = cg_q_insert(prev_index = CGQ_PREV_INDEX(cgq));
		new_cgq->cgq_ln = cgq_ln;
		new_cgq->cgq_dbln = cgq_dbln;
		new_cgq->cgq_op = CGQ_EXPR_ND1;
		new_cgq->cgq_arg.cgq_nd1 =
			       tr_build(CBRANCH, if_expr, tr_icon(else_label));

		/* Build the then part of the code. */
		new_cgq = cg_q_insert(CGQ_INDEX_OF_ELEM(new_cgq));
		new_cgq->cgq_ln = cgq_ln;
		new_cgq->cgq_dbln = cgq_dbln;
		new_cgq->cgq_op = CGQ_EXPR_ND1;
		new_cgq->cgq_arg.cgq_nd1 =
			(lhs ? tr_build(node->op, tr_copy(lhs), then_expr)
			     : then_expr);

		/* Build JUMP to end_label. */
		new_cgq = cg_q_insert(CGQ_INDEX_OF_ELEM(new_cgq));
		new_cgq->cgq_ln = cgq_ln;
		new_cgq->cgq_dbln = cgq_dbln;
		new_cgq->cgq_op = CGQ_EXPR_ND2;
		new_cgq->cgq_arg.cgq_nd2 = pass2_node = (ND2 *)talloc();
		pass2_node->op = JUMP;
		pass2_node->type = T2_INT;
		pass2_node->label = end_label;

		/* Build else_label item. */
		new_cgq = cg_q_insert(CGQ_INDEX_OF_ELEM(new_cgq));
		new_cgq->cgq_ln = cgq_ln;
		new_cgq->cgq_dbln = cgq_dbln;
		new_cgq->cgq_op = CGQ_EXPR_ND2;
		new_cgq->cgq_arg.cgq_nd2 = pass2_node = (ND2 *)talloc();
		pass2_node->op = LABELOP;
		pass2_node->label = else_label;

		/* Build the else part of the code. */
		new_cgq = cg_q_insert(CGQ_INDEX_OF_ELEM(new_cgq));
		new_cgq->cgq_ln = cgq_ln;
		new_cgq->cgq_dbln = cgq_dbln;
		new_cgq->cgq_op = CGQ_EXPR_ND1;
		new_cgq->cgq_arg.cgq_nd1 =
			(lhs ? tr_build(node->op, lhs, else_expr)
			     : else_expr);

		/* Build end_label item. */
		new_cgq = cg_q_insert(CGQ_INDEX_OF_ELEM(new_cgq));
		new_cgq->cgq_ln = cgq_ln;
		new_cgq->cgq_dbln = cgq_dbln;
		new_cgq->cgq_op = CGQ_EXPR_ND2;
		new_cgq->cgq_arg.cgq_nd2 = pass2_node = (ND2 *)talloc();
		pass2_node->op = LABELOP;
		pass2_node->label = end_label;

		/* Remove the current cgq item. */
		cg_q_remove(index);

		/* Just cut out the index-th item.  Back the index
		   up to the item just before the insertion point.
		   This will allow us to pick up any QUEST - COLON
		   expressions that have just been brought to the
		   surface. */
		index = prev_index;

	    }  /* if */
	}  /* if */

    CGQ_END_FOR_ALL
}  /* rewrite_top_level_question */



/* make_summary records the size of the function, information about its 
   parameters, and information about its calls
*/
static void
make_summary(sid,summary)
SX sid;
struct Summary *summary;
{
	T1WORD type = SY_TYPE(sid);
	int nparam = TY_NPARAM(type);
	int function_cannot_be_inlined = 0;
	int do_not_inline_at_this_site = 0;
	T1WORD *types;
	ND1 *first_expr = 0;
	ND1 *node;
	ND1 *return_expr = 0;
	int seen_start_of_block = 0;
	int looking_for_single_expr_function = 1;
	
	int i;
	int depth=0;	/* loop nesting depth */

	summary->flags = LEAF_FUNCTION;		/* assume for now */

	/* temporary hack until ty_nparam is fixed */
	if (nparam == -1) {
		function_cannot_be_inlined = 1;
		/* Mark the reason that this function can never be inlined -
		   parameter list is undefined? */
		summary->flags |= CNI_UNKNWN_PRMS;
	}  /* if */

	if (TY_ISVARARG(type) || TY_ISSU(TY_DECREF(type))) {
		function_cannot_be_inlined = 1;
		if (TY_ISVARARG(type)) {
			/* Mark this function as cannot be inlined because
			   of a variable length argument list. */
			summary->flags |= CNI_IS_VARARG;
		}  /* if */
		if (TY_ISSU(TY_DECREF(type))) {
			/* Mark this function as cannot be inlined because
			   it returns a struct or union type. */
			summary->flags |= CNI_RET_SU;
		}  /* if */
	}  /* if */

	if (function_cannot_be_inlined == 0) {
		summary->types=types=Arena_alloc(global_arena, nparam, T1WORD);
		for (i=0; i < nparam; ++i) {
			types[i] = TY_PROPRM(type,i);
			if (TY_ISSU(types[i])) {
				function_cannot_be_inlined = 1;
				/* Function has a struct or union 
				   argument. */
				summary->flags |= CNI_SU_ARG;
			}  /* if */
		}
	}
	if (function_cannot_be_inlined)
		summary->formal_count = -1;
	else {
		int i;
		summary->formal_count = nparam;
		summary->formals = Arena_alloc(global_arena, nparam, struct Formal);

		for (i=0; i<nparam; i++) {
			summary->formals[i].read_only = 1;
			summary->formals[i].op_const = 0;
			summary->formals[i].uses = 0;
		}
	}

	summary->node_count = 0;
	summary->call_list = 0;

	i = 0;
	CGQ_FOR_ALL(cgq,index)
		switch(cgq->cgq_op) {
		case CGQ_CALL:
			if (cgq->cgq_func == db_s_block) {
				seen_start_of_block = 1;
			}  /* if */
		case CGQ_CALL_SID:
			if (cgq->cgq_func == cg_copyprm) {
				/* map param symbols to an index into args
				   and types arrays for the function
				*/
				SY_AUX(cgq->cgq_arg.cgq_sid).arg = i;
				++i;
			} else
			if (seen_start_of_block &&
			    cgq->cgq_func == db_symbol) {
				/* This function has local variables. */
				summary->flags |= HAS_LOCALS;
			}  /* if */
			break;
		case CGQ_CALL_INT:
			if (cgq->cgq_func == os_loop) {
				summary->flags |= CONTAINS_LOOP;

				/* There cannot be a loop between an
				** OI_LSTART and an OI_LBODY.  Some
				** optimizations in amigo may assume
				** this, so prevent inlining from
				** expanding a function here.  It may
				** be better just to restrict inlining
				** from inserting a function with a 
				** loop here.  That is left as an
				** exercise for the reader.
				*/
				if (cgq->cgq_arg.cgq_int == OI_LSTART) {
					/* Calls cannot be expanded when 
					   part of the loop initialization
					   code. */
					do_not_inline_at_this_site = 
					    NOT_PRECOMPUTE | CNI_START_LOOP;
				}
				else if (cgq->cgq_arg.cgq_int == OI_LBODY) {
					do_not_inline_at_this_site = 0;
					++depth;
				}
				else if (cgq->cgq_arg.cgq_int == OI_LEND)
					--depth;
			}
			break;
		case CGQ_EXPR_ND2:

			if (cgq->cgq_arg.cgq_nd2->op == DEFNAM &&
			   (cgq->cgq_arg.cgq_nd2->lval&FUNCT) == 0) {
				/* no expansion if encountered a function
				   static definition
				*/
				summary->formal_count = -1;
				/* This function should not be inlined
				   because it contains a static local
				   variable. */
				summary->flags |= CNI_STATIC_LOCAL;
			}  /* if */
			break;
		case CGQ_EXPR_ND1:
			if (looking_for_single_expr_function) {
			    node = cgq->cgq_arg.cgq_nd1;
			    if (first_expr == (ND1 *)0) {
				/* This is the first user statement of the
				   function. */
				/* Check if this is an assignment to the 
				   return value RNODE. */
				if ((node->op == ASSIGN) &&
				    (node->left->op == RNODE)) {
				    /* This looks like one.  Save the pointer
				       to the right node. */
				    first_expr = node->right;

				} else
				/* Check if this is a function returning a
				   type of void. */
				if (TY_ERETTYPE(type) == TY_VOID &&
				    node->op == RETURN) {
				    /* This is a function returning void. */

				    first_expr = node;

				} else {
				    /* This is not an assignment to the
				       return value. */
				    looking_for_single_expr_function = 0;
				}  /* if */
			    } else {
				/* As a safety check, be certain that the
				   single expression assignment to the return
				   value is followed by a return statement.
				   Any additional expressions indicates more
				   than the simple return(expr) that we can
				   deal with. */
				if ((return_expr == 0) &&
				    (node->op == RETURN)) {
				    /* Found the single expression followed
				       by a return. */
				    return_expr = node;
				} else {
				    /* This indicates that something is amiss. */
				    looking_for_single_expr_function = 0;
				    first_expr = 0;
				}  /* if */
			    }  /* if */
			}  /* if */

			(void)process_node(cgq->cgq_arg.cgq_nd1, index,
					   summary, depth, 0,
					   do_not_inline_at_this_site);
			break;
		case CGQ_FIX_SWBEG:
			(void)process_node((ND1 *)cgq->cgq_arg.cgq_nd2->left,
					   index, summary, depth, 0,
					   do_not_inline_at_this_site);
			break;
		}
	CGQ_END_FOR_ALL

	if ((first_expr) &&
            ! (summary->flags & CNI_FUNCTION) &&
	    (summary->flags & LEAF_FUNCTION)) {

	    /* Have a simple expression function that we can try to 
	       inline directly into place of the call() expression. */
	    summary->single_expression_return = first_expr;
	}  /* if */
}  /* make_summary */




#define IS_PARAM(node) \
	(((node)->op == NAME && \
	(node)->rval > 0 && \
	SY_CLASS((node)->rval) == SC_PARAM) \
		? SY_AUX((node)->rval).arg : -1)

/* This routine analyzes a function and the uses of its formals.
   It also records calls from this function.
*/

#define PROCESS(side) \
	process_node(side == LEFT ? node->left : node->right \
		,index,summary, depth,order, get_precompute(no_pre, side,node))




static int
process_node(node, index, summary,depth,order,no_pre)
ND1 *node;
struct Summary *summary;
Cgq_index index;
int depth;	/* loop nesting */
int order;	/* depth first ordering */
int no_pre;	/* node cannot be pre-computed */
{
	int param;

	if (optype(node->op) != LTYPE) {
		if (callop(node->op)) {
			summary->flags &= ~LEAF_FUNCTION;

			/* Must not call process_node for left child of
			** call, since this code will think the function
			** address has escaped. See ICON case below.
			*/
			if (node->left->op == ICON)
				++summary->node_count;
			else
				order = PROCESS(LEFT);
		}
		else
			order = PROCESS(LEFT);
	}
	if (optype(node->op) == BITYPE)
		order = PROCESS(RIGHT);

	switch(node->op) {
	case ICON:
		if (node->rval > 0 && TY_ISFTN(SY_TYPE(node->rval)))
			/* cannot delete a function if address taken, since 
			   the function may be called indirectly
			*/
			get_function(node->rval)->flags |= NODELETE;
		break;
	case STCALL: case UNARY STCALL:
	case CALL: case UNARY CALL:
		if (node->left->op==ICON && node->left->rval>0) {
			struct Call_list *new_call = 
			    Arena_alloc(inline_arena,1,struct Call_list);
			new_call->next = summary->call_list;
			summary->call_list = new_call;
			new_call->func = get_function(node->left->rval);

			/* Increment the count of calls of function "func". */
			++new_call->func->times_called;

			/* Pick up the DEBUG line number of this call
			   in case needed for warning later. */
			if (verbose_cpp_diag) {
			    new_call->lineno = CGQ_DBLN_OF_INDEX(index);
			}  /* if */

			new_call->func->flags |= CALLED;
			/* Set or reset CALLED_ONCE as indicated. */
			/* THIS MAY GO AWAY IF ONCE_CALLED GOES_AWAY */
			if (new_call->func->times_called == 1) {
			    new_call->func->flags |= CALLED_ONCE;
			} else {
			    new_call->func->flags &= ~CALLED_ONCE;
			}  /* if */

			new_call->flags = no_pre;

			new_call->call_site = mark_call_site(node,index);
			new_call->order = order;
			new_call->frequency = depth;
			if (node->op == CALL) 
				new_call->arg_count =
			    	process_args(new_call,node->right,0);
			else
				new_call->arg_count = 0;
			++order;
		}
		break;
	case UNARY AND:
	ASSIGN_CASES:
		
		if (summary->formal_count != -1 &&(param= IS_PARAM(node->left)) >=0 )
			summary->formals[param].read_only = 0;
	
		break;
	}  /* switch */

	if (summary->formal_count != -1) {
	    switch(optype(node->op)) {
	    case BITYPE:
		param = IS_PARAM(node->right);
		if (param >= 0) {
			if (node->left->op == ICON || node->left->op == FCON)
				summary->formals[param].op_const++;
		}
		param = IS_PARAM(node->left);
		if (param >= 0) {
			if (node->right->op == ICON || node->right->op == FCON)
				summary->formals[param].op_const++;
		}
		break;
	    case UTYPE:
		param = IS_PARAM(node->left);
		if (param >= 0)
			summary->formals[param].op_const++;
		break;
	    case LTYPE:
		param = IS_PARAM(node);
		if (param >= 0)
			summary->formals[param].uses++;
	      }  /* switch */
	}  /* if */
		
	++summary->node_count;
	return order;
}

#define KNOWN_PTR 1
#define INT_TYPE 2
#define BAD_TYPE 3

/* analyzes an argument to a function call as to whether it is a pointer
   to a known variable, it is a constant, or it is a copy propagation 
   candidate 
*/
static void
process_arg(call_list, node, arg_cnt) 
struct Call_list *call_list;
ND1 *node;
int arg_cnt;
{
	call_list->types[arg_cnt] = node->type;
	call_list->arg_nodes[arg_cnt] = node;
	if (TY_ISPTR(node->type) && get_pointer(node) == KNOWN_PTR)
		call_list->args[arg_cnt] = ARG_KNOWN_PTR;
	else if (node->op == ICON)
		 call_list->args[arg_cnt] = ARG_CONST;
	else if (node->op == FCON)
		 call_list->args[arg_cnt] = ARG_CONST;
	else if (node->op == NAME)
		 call_list->args[arg_cnt] = ARG_COPY_PROP_CAND;
	else
		 call_list->args[arg_cnt] = 0;
		
}
/* If there are n arguments and n > 1, then process_args calls itself for
   the first n-1 arguments, and then calls process_arg for the last argument.
   Returns the number of arguments. 

   Input looks like following:
			       CM
      			     .      .

			CM		ARG (n'th arg)
	             .      .

		CM		ARG(n-1'th arg)
	    .

	CM

    Last CM looks like:
				CM
			     .       .
			ARG		ARG
	
*/

static int
process_args(call_list, args, depth)
struct Call_list *call_list;
ND1 *args ;
int depth;	/* depth in argument tree */
{
	/* returns numer of arguments below node */
	int arg_cnt;
	if (args->op == CM) {
		arg_cnt=process_args(call_list,args->left,depth+1);
		process_arg(call_list,args->right->left,arg_cnt);
		return arg_cnt+1;
	}
	else {	/* args->op == ARG */
		call_list->args = Arena_alloc(global_arena, depth+1 ,char);
		call_list->types = Arena_alloc(global_arena, depth+1, T1WORD);
		call_list->arg_nodes = Arena_alloc(global_arena, depth+1,
						   ND1 *);
		process_arg(call_list, args->left, 0);
		return 1;
	}
}

/* indicates whether the left or right child of a node can be evaluated into
    a temp before the other child is evaluted
*/
static int
get_precompute(no_pre, left_right,node)
int no_pre; 	/* if the node itself can be precomputed */
int left_right;
ND1 *node;
{
	switch(node->op) {

	case COLON:
		/* Function call used in either node of a COLON operator
		   cannot be precomputed. */
		return (no_pre | NOT_PRECOMPUTE | CNI_COLON_EXPR);

	case ANDAND:
	case OROR:
	case COMOP:
		if (left_right == RIGHT)
			/* right operands of a logical &&, || or COMMA
			   operator cannot be pre evaluated. */
			return (no_pre | NOT_PRECOMPUTE | CNI_BOOL_COMMA);
	/* FALLTHRU */
	default:
		return no_pre;
	}
}

/* Determines whether the optimizer can statically determine a pointer targ 
   Will recognize: &name, ICON which is an address constant, and known pointer
   + integer
*/
static int
get_pointer(node) 
ND1 *node;
{
	int ltype, rtype;
	switch(node->op) {
	case UNARY AND:
		if (node->left->op == NAME)
			return KNOWN_PTR;
		else
			return BAD_TYPE;
	case ICON:
		if (node->rval !=0)
			return KNOWN_PTR;
		else
			return INT_TYPE;
	case PLUS: case MINUS:
		ltype = get_pointer(node->left);
		rtype = get_pointer(node->right);
		if (ltype == KNOWN_PTR && rtype == INT_TYPE)
			return KNOWN_PTR;
		else if (rtype == KNOWN_PTR && ltype == INT_TYPE)
			return KNOWN_PTR;
		else if (rtype == INT_TYPE && ltype == INT_TYPE)
			return INT_TYPE;
		else
			return BAD_TYPE;
	
	default:
		if (TY_ISINTTYPE(node->type))
			return INT_TYPE;
		else
			return BAD_TYPE;
	}
}

/* finds the function structure associated with a symbol index, by looking
   a sequential search through the function structures for the name. Cannot
   simple store a pointer to function structure in symbol table because of
   SY_SAMEAS
*/
static Function
get_function(sid)
SX sid;
{
	char *name = SY_NAME(sid);
	Function new_func;
	Function f;

	/* point symbol table at function? */
	for (f=functions; f; f = f->next) {
		if ((f->name == name))
			return f;
	}

   	if (!inline_arena) {
		inline_arena = arena_init();
	}

    	new_func = Arena_alloc(inline_arena,1, struct Funct);
	new_func->expr_count = 0;
	new_func->call_table_list = 0;
    	new_func->flags = 0;
    	new_func->next = functions;
	new_func->name = name;
	new_func->save_arena = 0;
	new_func->call_tree = 0;
	new_func->sid = sid;
    	new_func->summary.call_list = 0;
	new_func->summary.node_count = -1;
	new_func->times_called = 0;		/* Counter for # times this
						   function is called. */
	new_func->summary.single_expression_return = 0;
    	functions = new_func;
    	return new_func;
}

/* Records call site information for the current function. Returns an integer
   used to access this information. If the current function is inlined, the
   call site information changes since the function is copied. The integer is
   stored in the call site to facilitate this change.
*/
static Call_site
mark_call_site(node,index) ND1* node; Cgq_index index;
{
	struct Call_table_list *call_table_list= Arena_alloc(inline_arena,
	    1, struct Call_table_list);
	call_table_list->node = node;
	call_table_list->index = index;
	call_table_list->next = curr_func->call_table_list;
	curr_func->call_table_list = call_table_list;
	++curr_func->expr_count;

	node->opt = (struct Expr_info *) curr_func->expr_count;

	return curr_func->expr_count;
}



/****************************************************************************/
/****************************************************************************/
/******************** driver to make call trees and then expand *************/
/****************************************************************************/
/****************************************************************************/

/* sort_call_list sorts the edges of the call graph constructed by make_summary
   by the benefit of expanding the call. If every function in the file is 
   optimizable, and therefore has been processed by make_summary, then 
   process_deletes deletes any static functions that are not referenced in the 
   call_graph and have not had its address taken. static functions that are
   called a single time are marked for deletion when they are expanded.

   For each function in the file, make_call_trees, indicates the calls that
   are to be expanded. Expanded calls that in turn contain expanded calls
   are indicated in the call_tree. order_trees attempts to order the call_trees
   so that if the call tree for f contains a call to g, then f's call tree
   is processed before g's call tree, since processing g's call tree changes
   g's defintion. With recursive calls, a call to a g will be processed after
   starting to process g's call tree, in this case g must be copied before
   processing its call tree.

   expand_calls performs the expansions for the call_trees for each function.
   After the expansions, code is generated for the function. After the
   calls to expand_calls, code is generated for those functions that contain
   no expanded calls.

*/

static int function_no_longer_called = 0;
					/* static flag used to signal when
					   the number of remaining calls for
					   some inlined function has now
					   reached 0. */

void
inline_eof() {
	Function f = functions;
	struct Call_tree_list *call_tree;

	if (!last_begf_seen)
		return;

	if (!(last_begf_seen->flags&DEFINED))
		all_processed = 0;

#ifndef NODBG
	if (dbg_inline_flags & DGRAPH) pr_graph();
#endif

	/* Remove undefined functions from the list so that we can count
	   the unreferenced statics removed. */
	functions = filter(functions);

#ifndef NODBG
	/* Reset the deleted static function count.  What has been removed
	   up to this point is the undefined functions, i.e externals.*/
	deleted_stat_func_cnt = 0;
#endif  /* ! NODBG */

	if (all_processed) {
		/* Remove unreferenced static functions, including
		   nested function calls. */
		remove_unref_static_functions();
	}  /* if */

	/* Walk through the list of functions and delete anything that
	   appears as undefined.  This could be external functions
	   called from this code or defined static functions that are
	   never called in this code (now marked as undefined. */
	functions = filter(functions);

#ifndef NODBG
	/* Any functions deleted just now were static functions
	   that were never called from a "live" function. */
	unref_stat_func_cnt = deleted_stat_func_cnt;
	deleted_stat_func_cnt = 0;
#endif  /* ! NODBG */


	/* Check if only the removal of unreferenced static functions
	   was requested.  If so, do not do the rest of the processing.
	*/
	if (do_inline != REM_UNREF_STAT) {

	    mark_recursive_functions(functions);

	    /* Check that the argument count and types agree with the
	       formals of the function definition.  This also sets 
	       some additional "cannot inline" flags for C++. */
	    check_arg_types_against_formals(functions);

	    /* If doing C++ inlining (-1I2 or -1I3), check if any of the
	       inlining restrictions can be relaxed.  Relaxation is
	       done on a case by case basis.

	       This can and should be extended to C inlining in a later
	       release of the C cmpiler.
	    */
	    if (do_inline & (CPP_INLINE_ONLY | INLINE_ALL)) {
		try_relax_call_site_restrictions(functions);
	    }  /* if */

#ifndef NODBG
	    if (verbose_cpp_diag || (dbg_inline_flags & DSUMMARY)) {
		/* If a summary is requested for debugging, we need to run
		   through the list of functions to count the functions
		   which were not candidates for inlining, and those
		   call sites that could not be expanded. */
#else
	    if (verbose_cpp_diag) {
#endif
		functions = emit_funct_inlining_diagnostics(functions);
	    }  /* if */
#ifndef NODBG
	    if (dbg_inline_flags & DGRAPH) pr_graph();
#endif

	    for (f=functions; f; f = f->next) 
		sort_call_list(f);

	    if (all_processed) {
		process_deletes(functions);
		functions = filter(functions);
	    }

#ifndef NODBG
	    if (dbg_inline_flags & DGRAPH) pr_graph();
#endif

	    call_tree = make_call_trees(functions);

	    call_tree = order_trees(call_tree);

	    for ( ; call_tree; call_tree = call_tree->next) {
		struct Call_tree *caller = call_tree->tree;
		struct Funct_copy *fc; 

		/* Reset the flag used to signal that the number of times
		   a static function is called has reached zero. */
		function_no_longer_called = 0;

		/* Check if this function is really needed in the output.
		   A static function that has been inlined everywhere that
		   it was called and whose address has not escaped can be
		   deleted.

		   This typically would be checked as:
			if ((f->times_called <= 0) &&
			    (SY_CLASS(f->sid) == SC_STATIC) &&
			    ((f->flags &  NODELETE) == 0) ) {

		   but that check has been reduced by 
		   remove_unref_static_functions() to ~DEFINED.
		*/
		if ( ! (caller->func->flags & DEFINED)) {
		    /* This static function has had all its calls inlined
		       at this time and is no longer needed as a 
		       stand-alone function.  */
		    continue;
		}  /* if */

		if (caller->edges == 0) {
		    /* Either no function calls or no function calls
		       that can be inlined; continue with the next
		       function. */
		    continue;
		}  /* if */
		fc = open_function(caller->func, call_tree->open_flags);
		expand_calls(fc, caller->edges,0);

		if (all_processed && function_no_longer_called) {
		    /* Check for any new unreferenced static functions,
		       including nested function calls and walk through
		       the existing list of functions and delete them
		       and free any memory tied up in that functions
		       arena and cgq tree.

		       This may seem untimely, but many small functions
		       can tie up memory that the global optimizer
		       may need.

		       Mark the current function as generated; we will do 
		       momentarily. */

		    caller->func->flags |= GENERATED;
		    remove_unref_static_functions();
		    functions = filter(functions);
		}  /* if */

		close_function(fc);	/* calls amigo and generates code */

	    }  /* for */

	    /* Walk through the list of functions and delete any functions
	       completely inlined in the above process. */
	    if (all_processed) {
		functions = filter(functions);
	    }  /* if */

	}  /* if ! REM_UNREF_STAT */


	for (f=functions; f; f=f->next) {
#ifndef NODBG
		final_function_count++;
#endif  /* ! NODBG */

		if (!(f->flags & GENERATED)) {
			restore(f);

			/* calls amigo and generates code */
			cg_setcurfunc(f->sid);
			cg_endf(al_endf());
		}  /* if */
	}  /* for */

#ifndef NODBG
	if (dbg_inline_flags & DSUMMARY ) {
	    DPRINTF ("Inlining summary:\n");
	    DPRINTF ("\tinput function count: %d     node count: %d\n",
		     func_cnt, node_count);
	    DPRINTF ("\tunreferenced static functions (never called): %d\n",
		     unref_stat_func_cnt);
	    DPRINTF ("\tstatic functions totally inlined (deleted): %d\n",
		     deleted_stat_func_cnt);
	    DPRINTF ("\tC++ inline functions not inlined (function attributes): %d\n",
		     cplusplus_func_not_callable);
	    DPRINTF ("\tcalls to C++ inlined functions not expanded (call site attr): %d \n",
		     call_to_cpp_function_not_inlined);
	    DPRINTF ("\ttotal number of calls inlined: %d\n",
		     call_sites_expanded);
	    DPRINTF ("\toutput function count: %d     node count: %d\n",
		     final_function_count, node_count + added_node_count);
	}  /* if */
#endif  /* ! NODBG */

}  /* inline_eof */


static Function
emit_funct_inlining_diagnostics(functions)
Function functions;
/*
   Walk through the list of functions pointed to by the formal parameter 
   functs.  Emit diagnostics for any C++ inline functions which cannot
   be inlined as requested.  As each function is processed, run through
   the list of calls made from that function and emit diagnostics for
   any calls to C++ inline functions which cannot be expanded because of
   limitations at that call site.
*/
{
    Function function_ptr;
    char * function_name;
    struct Call_list * call_list;
    SX     sid;

    /* Run through the linked list of functions in this compilation unit.
       This will be eith LIFO or FIFO order with respect to how the
       C++ frontend puts out the functions.  That may not be in the order
       that they appeared in the user source and header files.  Reorder
       the functions tobe in a reasonable line number sequence.

       This routine is called only when the user specifically asked for
       this supplementary information or when the compiler developers are
       asking for additional debugging information, i.e number
       of function calls not inlined.  Therefore the time associated with
       the sort should be acceptable.
    */
    {
	struct Funct min;
	struct Funct max;
	Function     next_function;

	min.lineno = -1;
	max.lineno = MAX_VALUE;
	max.next = 0;
	min.next = &max;

	for (function_ptr = functions;
	     function_ptr;
	     function_ptr = next_function) {

	    Function insert;
	    int lineno = function_ptr->lineno;

	    next_function = function_ptr->next;

	    for (insert = &min;
		 lineno > insert->next->lineno;
		 insert = insert->next)  ;

	    /* insert->lineno < lineno <= insert->next->lineno */
	    function_ptr->next = insert->next;
	    insert->next = function_ptr;
	}  /* for */

	/* Find the predecessor to max. */
	for (function_ptr = &min;
	     function_ptr->next != &max;
	     function_ptr = function_ptr->next) ;

	/* Cut max from the list and set the call_list to be the
	   sorted list. */
	function_ptr->next = 0;
	functions = min.next;
    } /* block */

    for (function_ptr = functions ;
	 function_ptr;
	 function_ptr = function_ptr->next) {

	if ( (function_ptr->flags & CPP_USER_INLINE) &&
	     ( (function_ptr->summary.flags & CNI_FUNCTION) ||
	       (function_ptr->flags & RECURSIVE)) ) {

#ifndef NODBG
	    /* Increment the debugging counter for C++ function flatly
	       rejected for inlining. */
	    cplusplus_func_not_callable++;
#endif  /* ! NODBG */

	    if (verbose_cpp_diag && (function_ptr->flags & CPP_USER_INLINE)) {
			/* Get the function name and sid.*/
		function_name = function_ptr->name;
		if (function_ptr->summary.flags & CNI_IS_VARARG) {
		    WLFERROR(function_ptr->lineno, function_ptr->filename,
			     gettxt(":1558",
			     "cannot inline function %s with a variable argument list"),
			     function_name);
		}  /* if */
		if (function_ptr->summary.flags & CNI_RET_SU) {
		    WLFERROR(function_ptr->lineno, function_ptr->filename,
			     gettxt(":1559",
			     "cannot inline function %s which returns a struct, union or class"),
			     function_name);
		}  /* if */
		if (function_ptr->summary.flags & CNI_SU_ARG) {
		    WLFERROR(function_ptr->lineno, function_ptr->filename,
			     gettxt(":1560",
			     "cannot inline function %s which has a struct, union or class formal parameter"),
			     function_name);
		}  /* if */
		if (function_ptr->summary.flags & CNI_STATIC_LOCAL) {
		    WLFERROR(function_ptr->lineno, function_ptr->filename,
			     gettxt(":1561",
			     "cannot inline function %s which has a local static variable"),
			     function_name);
		}  /* if */
		if (function_ptr->summary.flags & CNI_UNKNWN_PRMS) {
		    WLFERROR(function_ptr->lineno, function_ptr->filename,
			     gettxt(":1562",
			     "cannot inline function %s"),
			     function_name);
		}  /* if */
		if (function_ptr->flags & RECURSIVE) {
		    WLFERROR(function_ptr->lineno, function_ptr->filename,
			     gettxt(":1563",
			     "recursion encountered; cannot inline function %s"),
			     function_name);
		}  /* if */
	    }  /* if */
	}  /* if */

	/* Run through the Call_list of this function.  These calls are
	   currently in LIFO order; reorder so that diagnostics, if any,
	   will be emitted in line number order. */
	{
	    struct Call_list min;
	    struct Call_list max;
	    struct Call_list *next_call_list;

	    min.lineno = -1;
	    max.lineno = MAX_VALUE;
	    max.next = 0;
	    min.next = &max;

	    for (call_list = function_ptr->summary.call_list;
		 call_list;
		 call_list = next_call_list) {

		struct Call_list * insert;
		int lineno = call_list->lineno;

		next_call_list = call_list->next;
		for (insert = &min;
		     lineno > insert->next->lineno;
		     insert = insert->next)  ;

		/* insert->lineno <= lineno < insert->next->lineno */
		call_list->next = insert->next;
		insert->next = call_list;
	    }  /* for */

	    /* Find the predecessor to max. */
	    for (call_list = &min;
		 call_list->next != &max;
		 call_list = call_list->next) ;

	    /* Cut max from the list and set the call_list to be the
	       sorted list. */
	    call_list->next = 0;
	    function_ptr->summary.call_list = min.next;
	} /* block */

	for (call_list = function_ptr->summary.call_list;
	     call_list;
	     call_list = call_list->next) {

	    if ( (call_list->flags & CNI_AT_CALL_SITE) &&
		 (call_list->func->flags & 
			(CPP_USER_INLINE | CPP_CPFE_INLINE)) ) {
		/* This is a trackable (reportable or reflected in debugging
		   counts) diagnostic.  Proceed with the 
		   following code and emit the appropriate diagnostics. */
#ifndef NODBG
		/* Increment the counter of where a call to a C++ inline cannot
		   be inlined. */
		call_to_cpp_function_not_inlined++;
#endif  /* ! NODBG */
	    } else {
		/* No C++ diagnostic is to be emitted for this call site;
		   continue with the next call site. */
		continue;
	    }  /* if */

	    /* If we have fallen through to here, this call to a C++ inline 
	       function cannot be expanded at this time, given the
	       current limitations of the inliner. /

	    /* Only emit a diagnostic if the verbose_cpp_flag is set and
	       this is not a compiler generated inline. */
	    if ( ! (verbose_cpp_diag &&
		    (call_list->func->flags & CPP_USER_INLINE))) {
		 continue;
	    }  /* if */

	    function_name = call_list->func->name;

	    if (call_list->flags & CNI_START_LOOP) {
		WLFERROR(call_list->lineno, function_ptr->filename,
			 gettxt(":1564",
			  "cannot inline function %s inside loop initialization"),
			 function_name);
	    }  /* if */
	    if (call_list->flags & CNI_COLON_EXPR) {
		WLFERROR(call_list->lineno, function_ptr->filename,
			 gettxt(":1565",
			  "cannot inline function %s as an operand of a colon operator"),
			 function_name);
	    }  /* if */
	    if (call_list->flags & CNI_BOOL_COMMA) {
		WLFERROR(call_list->lineno, function_ptr->filename,
			 gettxt(":1566",
			  "cannot inline function %s as the right operand of a &&, || or comma expression"),
			 function_name);
	    }  /* if */
	    if (call_list->flags & CNI_ARG_COUNT) {
		WLFERROR(call_list->lineno, function_ptr->filename,
			 gettxt(":1567",
			  "cannot inline function %s - number of arguments mismatch"),
			 function_name);
	    }  /* if */
	    if (call_list->flags & CNI_ARG_TYPE) {
		WLFERROR(call_list->lineno, function_ptr->filename,
			 gettxt(":1568",
			  "cannot inline function %s - argument type mismatch"),
			 function_name);
	    }  /* if */
	}  /* for */
    }  /* for */

    return functions;
}  /* emit_funct_inlining_diagnostics */




/****************************************************************************/
/****************************************************************************/
/******************** manipulate call graph**********************************/
/****************************************************************************/
/****************************************************************************/


static void
remove_unref_static_functions()
/*
   Walk through the complete list of functions and remove any unreference
   static functions whose address has not escaped.

   NOTE: This can only be accomplished if all functions have been processed
         and available on the single list pointed to by "functions".
*/
{
	Function f;
	int	not_finished = 1;

	while (not_finished) {
	    not_finished = 0;		/* Assume that no function remains
					   uncalled. */ 
	    for (f = functions; f; f = f->next) {

		/* Check for static functions that have not been called
		   and have not had their address taken. */
		if ((f->flags & DEFINED) &&
		    (SY_CLASS(f->sid) == SC_STATIC) &&
		    (f->times_called == 0) &&
		    ((f->flags & NODELETE) == 0) ) {

		    struct Call_list *call_list;

		    /* An unreferenced, defined, static function. */
		    f->flags &= ~DEFINED;
#ifndef NODBG
		    if (dbg_inline_flags & (DEXPAND | DEXPAND1)) 
			DPRINTF("deleting function=%s(%d)\n",
				SY_NAME(f->sid), f->sid);
#endif
		    /* Walk through the call_list, decrementing the 
		       times_called of any function called by this
		       routine. */
		    for (call_list = f->summary.call_list;
			 call_list;
			 call_list = call_list->next) {

			int times_called;
			if ((call_list->flags & DELETED) == 0) {
			    times_called = --(call_list->func->times_called);

			    /* Reset/set CALLED and CALLED_ONCE flags as
			       needed. */
			    if (times_called == 0) {
				not_finished = 1;
				call_list->func->flags &=
						~(CALLED | CALLED_ONCE);
			    } else if (times_called == 1) {
				call_list->func->flags |= CALLED_ONCE;
			    }  /* if */
			}  /* if */
		    }  /* for */
		}  /* if */
	    }  /* for */

#ifndef NODBG
	    if (dbg_inline_flags & DGRAPH) pr_graph();
#endif
	}  /* while */
}



static void
mark_recursive_functions(func)
Function func;
{
	Function f, f2;
	f = func;
	while (f) {
		for (f2=func; f2; f2=f2->next)
			f2->visited = 0;
		if (is_recursive(f->sid, f)) {
			f->flags |= RECURSIVE;
#ifndef NODBG
			if (dbg_inline_flags)
				DPRINTF("Recursive: %s\n", SY_NAME(f->sid));
#endif
		}
		f = f->next;
	}
}

static int 
is_recursive(sid, f)
SX sid;
Function f;
{
	struct Call_list *call;

	f->visited = 1;
	for (call=f->summary.call_list; call; call=call->next) {
		if (call->flags == DELETED)
			continue;
		if (call->func->sid == sid)
			return 1;		/* directly recursive. */
	}
	for (call=f->summary.call_list; call; call=call->next) {
		if (call->flags == DELETED)
			continue;
		if (call->func->visited)
			continue;
		if (  (do_inline == CPP_INLINE_ONLY) &&
		    ! (call->func->flags &
				(CPP_USER_INLINE | CPP_CPFE_INLINE)) ) {
		    /* Doing C++ inlining only and the function called is
		       not marked for C++ inlining.  Recursion will not be
		       an issue for this call.  

		       NOTE: This is based on the fact that this function
		       will not be considered for inlining by eval_value().
		    */
		    continue;
		}  /* if */

		if (is_recursive(sid, call->func))
			return 1;
	}
	return 0;
}  /* is_recursive */


static void
check_arg_types_against_formals(funcs)
Function funcs;
/*
   Walk through the list of functions pointed to by the parameter funcs,
   and for each call made by that function, check that the argument count
   and types are machine compatible with those of the function definition.
*/
{
    struct Call_list * call_list_ptr;
    struct Summary * callee_summary;
    int formal_count;
    int count;

    /* Walk through the list of functions. */
    for ( ;funcs; funcs = funcs->next) {


	/* Walk through the call sites in this function an check if any
	   restrictions can be relaxed. */
	for (call_list_ptr = funcs->summary.call_list;
	     call_list_ptr;
	     call_list_ptr = call_list_ptr->next) {

	    /* Nothing to check if the definition of the called function
	       has not been seen. */
	    if ( ! (callee_summary = get_summary(call_list_ptr->func))) 
		continue;

	    /* Check that number of arguments is identical to the number
	       of formals. */
	    if (call_list_ptr->arg_count != 
			(formal_count = callee_summary->formal_count)) {
		if (formal_count != -1) {
		    /* The number of formal params does not match with the
		       actual arguments.  This call site cannot be expanded.*/
		    call_list_ptr->flags |= CNI_ARG_COUNT;
		}  /* if */
		continue;
	    }  /* if */

	    /* evaluate argument parameter pairings */
	    for (count=0; count < formal_count; ++count) {
		struct Formal *formal = &callee_summary->formals[count];
		if (cg_machine_type(call_list_ptr->types[count]) != 
		    cg_machine_type(callee_summary->types[count])) {
			/* This call site cannot be expanded because of
			   differences between the underlying machine data
			   type of the argument and the formals. */
			call_list_ptr->flags |= CNI_ARG_TYPE;
			break;
		}  /* if */
	    }  /* for */
	}  /* for */
    }  /* for */
}  /* check_arg_type_against_formals */


static void
remove_inline_restriction_at_site(call_list_ptr, flag)
struct Call_list *call_list_ptr;
int flag;
/*
   Remove the inlining restriction(s) denoted by the value of flag at
   the call site denoted by call_list_ptr.  If there are no remaining
   restrictions at this call site, turn off the NOT_PRECOMPUTE flag.
*/
{
    int new_flags;

    new_flags = call_list_ptr->flags & ~flag;
    if ( ! (new_flags & CNI_AT_CALL_SITE)) {
	new_flags &= ~NOT_PRECOMPUTE;
    }  /* if */
    call_list_ptr->flags = new_flags;
}  /*remove_inline_restriction_at_site */


static int
able_to_inline_as_an_expression(call_list_ptr)
struct Call_list *call_list_ptr;
/*
   Analyze this particular call to determine if expression inlining is
   possible.
*/
{
    Function callee = call_list_ptr->func;
    struct Formal *formal_ptr;
    int arg_count;
    int count;

    if ( ! callee->summary.single_expression_return ||
	 (callee->summary.flags & CNI_FUNCTION) ||
	 (call_list_ptr->flags & (CNI_ARG_COUNT | CNI_ARG_TYPE))) {
	return 0;
    }  /* if */

    /* If there are no formals, that is OK. */
    if ((arg_count = call_list_ptr->arg_count) == 0) return 1;
    for (count=0; count < arg_count; count++) {
	formal_ptr = &callee->summary.formals[count];
	if ( ! formal_ptr->read_only) return 0;
	if (formal_ptr->uses == 1) continue;

	/* Multiple uses of the argument in the function called. */
	if (call_list_ptr->arg_nodes[count]->flags & FF_SEFF) return 0;
    }  /* for */
    return 1;
}  /* able_to_inline_as_an_expression */



static void
try_relax_call_site_restrictions(funcs)
Function funcs;
/*
   Walk through the list of functions and check each call made by
   that function for existing call site restrictions.  Try to relax 
   these restrictions where possible.
*/
{
    struct Call_list *call_list_ptr;
    int flags;

    /* Walk through the list of functions. */
    for ( ; funcs; funcs = funcs->next) {

	/* Walk through the call sites in this function an check if any
	   restrictions can be relaxed. */
	for (call_list_ptr = funcs->summary.call_list;
	     call_list_ptr;
	     call_list_ptr = call_list_ptr->next) {

	    /*
	    ** Relax inlining inside loop initialization, if possible.
	    */
	    if (call_list_ptr->flags & CNI_START_LOOP) {
		/* Check if this is a call to a leaf function that does
		   not contain any loops. */
		if ( ((flags = call_list_ptr->func->summary.flags)
					 & LEAF_FUNCTION) &&
		     ( ! (flags & CONTAINS_LOOP))) {
		    /* This call should be safe to inline.  It does not contain
		       a loop and does not make a call to other functions which
		       may contain a loop. */
		    remove_inline_restriction_at_site(call_list_ptr,
						      CNI_START_LOOP);
		}  /* if */
	    }  /* if */

	    /*
	    ** If expression inlining is possible, relax restrictions
	    ** for right operands of BOOLEAN or COMMA operators or
	    ** either operand of a COLON operator, and use expression
	    ** inlining over block-copy inlining.
	    */
	    if (able_to_inline_as_an_expression(call_list_ptr)) {
		if (call_list_ptr->func->flags &
		                      (CPP_USER_INLINE | CPP_CPFE_INLINE)) {
		    /* But only if doing C++ inlining. */
		    call_list_ptr->flags |= INLINE_AS_EXPR;

		    /* Check if any inlining restrictions can be relaxed. */
		    if (call_list_ptr->flags &
					(CNI_COLON_EXPR | CNI_BOOL_COMMA)) {
			remove_inline_restriction_at_site(call_list_ptr,
					(CNI_COLON_EXPR | CNI_BOOL_COMMA));
		    }  /* if */
		}  /* if */
	    }  /* if */
	}  /* for */
    }  /* for */

}  /* try_relax_call_site_restrictions */

		

static void
sort_call_list(f)
Function f;
{
	struct Call_list min;
	struct Call_list max;

	struct Call_list *call_list=get_summary(f)->call_list;
	struct Call_list *next_call;

	min.f_value = -1;
	max.f_value = MAX_VALUE;
	max.next = &min;
	min.next = 0;

	/* sort the call_list by benefit calculated by make_value */
	for ( ; call_list; call_list=next_call) {

		/* evaluates the benefit of the call based on size of called
		   function and parameter argument parings
		*/
		int value = call_list->value = eval_value(call_list);

		struct Call_list *insert;
		next_call = call_list->next;

		/* weights the value by whether call occurs in loop */

		if (value >= 0)
			value = make_value(call_list->frequency+1,value+1);
		call_list->f_value = value;

		/* Uncalled functions have already been deleted and
		   the functions called from these have been considered.
		   Functions called only once can be deleted if the single
		   call is expanded.  Set up that "unique" caller 
		   information.
		*/
		if (call_list->func->flags & CALLED_ONCE) {
			call_list->func->unique_caller = f;
			call_list->func->unique_call = call_list;
		}  /* if */

		/* Previously, (earlier versions of inlining), dropped
		   calls that could not be inlined (value < 0) from the 
		   call-list.  These calls will now be kept in the list
		   for use in tracking all the calls to a particular
		   function, but only for defined "static" functions. */
		if ((value < 0) &&
		    ( ! (call_list->func->flags & DEFINED) ||
		      ! (SY_CLASS(call_list->func->sid) == SC_STATIC))) {
			continue;
		}  /* if */

		for (insert = &max;
		     value < insert->next->f_value; 
		     insert=insert->next);

		/* insert->f_value > value >= insert->next->f_value */

		call_list->next = insert->next;
		insert->next = call_list;
	}  /* for */

	/* find the predecessor to min */
	for (call_list = &max;
	     call_list->next != &min; 
	     call_list = call_list->next);

	/* delete min from list */
	call_list->next = 0;

	/* set call_list to sorted list */
	get_summary(f)->call_list = max.next;

}  /* sort_call_list */



/* Evaluates the benefit of inlining a particular call */
static int
eval_value(call)
struct Call_list *call;
{
	struct Summary *callee_summary = get_summary(call->func);
	int count;
	int value = 0;

	/* Return if definition of called function not seen */
	if (!callee_summary)
		return -1;

	/* Return a -1 if arguments and formals don't have the same number
	   or if the call cannot be precomputed (e.g., contained in rhs
	   of a boolean).  The formal_count has been set to a -1 if the
	   called function doesn't meet certain inlining criteria
	   (e.g., a structure argument)
	*/
	if (call->arg_count != callee_summary->formal_count) {
		/* The number of formal parameters does not match with the
		   actual argument count.  This includes situations where the
		   function cannot be considered for inlining and the
		   formal_count has been set to -1.  This call site cannot
		   be expanded.*/
		return -1;
	}  /* if */

	if (call->flags & (NOT_PRECOMPUTE | CNI_ARG_TYPE)) {
		return -1;
	}  /* if */

#ifndef NODBG
	if (il_leaf_only && !(callee_summary->flags & LEAF_FUNCTION))
		return -1;
	if (il_no_loop_only && (callee_summary->flags & CONTAINS_LOOP))
		return -1;
	if (il_read_only || il_copy_only) {
		for (count=0; count < call->arg_count; ++count) {
			if (!callee_summary->formals[count].read_only
			  	&& il_read_only)
				return -1;
			if (call->args[count]&ARG_CONST) 
				continue;
			if (call->args[count]&ARG_COPY_PROP_CAND) 
				continue;
			return -1;
		}
	}
#endif

	if (do_inline & (FULL_C_INLINE | INLINE_ALL)) {
	    if (callee_summary->flags & LEAF_FUNCTION)
		++value;
	    for (count=0; count < call->arg_count; ++count) {
		struct Formal *formal = &callee_summary->formals[count];
		if (call->args[count]&ARG_CONST) {
			if (formal->read_only)
				value += formal->uses + formal->op_const;
		}
		if (call->args[count]&ARG_KNOWN_PTR) 
			++value;
		if (call->args[count]&ARG_COPY_PROP_CAND)  {
			if (formal->read_only)
				++value;
		}
	    }  /* for */
	}  /* if */

	/* If looking at a call to a C++ inline function, heavily weight
	   the inline benefit value. */
	if (do_inline & CPP_INLINE_ONLY) {
	    if (call->func->flags & (CPP_USER_INLINE | CPP_CPFE_INLINE)) {
		value += CPP_WEIGHT;
	    }
	    else if ( ! (do_inline & (FULL_C_INLINE | INLINE_ALL)) ) {
		/* Only doing C++ inlining, suppress all other inlining. */
		value = -1;
	    }  /* if */
 	}  /* if */

	return value;
}



/* deletes functions that are not called and have not had address taken by
   marking functions is not defined. static functions that are called once
   are marked so that they will be deleted if the unique call is expanded
*/
static void
process_deletes(f) Function f;
{
	/* eliminate access to Function */
	for ( ; f; f= f->next) {
		if ((f->flags&(CALLED|NODELETE)) == 0 &&
		    SY_CLASS(f->sid) == SC_STATIC) { 
			f->flags &= ~DEFINED;
#ifndef NODBG
			if (dbg_inline_flags & (DEXPAND | DEXPAND1)) 
				DPRINTF("deleting function=%s(%d)\n",
					SY_NAME(f->sid), f->sid);
#endif
		}

		/* check for a unique call to static function */
		else if ((f->flags & (CALLED_ONCE|NODELETE)) == CALLED_ONCE &&
			 SY_CLASS(f->sid) == SC_STATIC &&
			 (f->unique_caller->flags&CALLED) == 0 &&
			 (f->unique_call->value != -1) ) {

			/* mark call site deleted and insert new call site
			   at front of call_list
			*/
			struct Call_list *new_call = 
			    Arena_alloc(inline_arena,1,struct Call_list);

			*new_call = *f->unique_call;
			f->unique_call->flags = DELETED;
			new_call->flags |= DELETE_ON_EXPAND;
			new_call->next =
			    f->unique_caller->summary.call_list;
			f->unique_caller->summary.call_list = new_call;
		}
	}
}


/* Given a function that is not needed for code generation, free the
   memory currently used in its memory arena (formerly global_arena)
   and its cgq_td (C trees table).
*/
static void
free_function_memory(f)
Function f;
{
    if (f->save_arena != 0) {
	arena_term(f->save_arena);
	f->save_arena = 0;

	/* This free is similiar to what is done in cg_endf when inlining is
	   ON - do_inline <> 0. */
#ifndef NO_AMIGO
	if (f->save_cgq.td_start != 0) {
	    free(f->save_cgq.td_start);
	    f->save_cgq.td_start = 0;
	}  /* if */
#endif
    }  /* if */
}  /* free_function_memory */


/* Given a list of function, deletes those functions that have not been
   defined, or have already been sent to code generator. This reverses the
   order of the list; there is really not reason to reverse this order
*/
static Function
filter(f) Function f;
{
	Function last=0;
	while (f) {
		Function next;

		next = f->next;
		if ( ! (f->flags & (DEFINED | GENERATED))) {
			/* This function can be deleted from the chain.
			   Free and memory tied up in its global_arena
			   and cgq_td. */
			free_function_memory(f);
#ifndef NODBG
			/* Increment the deleted static function counter. */
			deleted_stat_func_cnt++;
			if (f->summary.node_count != -1) {
				added_node_count -= f->summary.node_count;
			}  /* if */
#endif  /* ! NODBG */
			f = next;
			continue;
		}  /* if */
		f->next = last;
		last = f;
		f = next;
	}
	return last;
}

/****************************************************************************/
/****************************************************************************/
/******************** make call trees ***************************************/
/****************************************************************************/
/****************************************************************************/
/* Initially, for each function, make_call_tree constructions a singleton tree
   with the function at the root. For every node in the call trees that 
   contains an unexpanded call there corresponds an Expansion_site structure.
   push_site maintains a sorted list of the Expansion_sites. make_call_tree
   calls push_site to process the singleton call trees. It then calls
   expand_list to add to the call trees until the expected space increase
   due to the expansions exceed a thresh-hold.
*/


/* creates a linked list of singleton call_trees. Each singleton tree is
   recorded as sites for inline expansion by make_site. expand_list, fills
   out the call_trees.
*/
static Arena call_tree_arena;
static struct Call_tree_list *
make_call_trees(funcs)
Function funcs;
{
	static void expand_list();
	struct Call_tree_list *tree_list = 0;
	static void init_expansions();
	init_expansions();

	call_tree_arena = arena_init();
	for ( ; funcs; funcs=funcs->next) {
		struct Expansion_sites *new_site;
		struct Call_tree *tree_node;
		struct Call_tree_list *new_list;
		if (!(funcs->flags&DEFINED))
			continue;
		new_list = Arena_alloc(inline_arena,1, struct Call_tree_list);
		new_list->next = tree_list;
		new_list->open_flags = EDITING;
		tree_list = new_list;
		funcs->call_tree = tree_list;

		tree_list->tree = tree_node = 
		    Arena_alloc(inline_arena,1, struct Call_tree);
		tree_node->func = funcs;
		tree_node->edges = 0;
		new_site = make_site(new_list,tree_node,1);
		if (new_site)
			push_site(new_site);
	}
	/* put expand_list inline */
	expand_list();
	arena_term(call_tree_arena);
	return tree_list;
}


/* Each node in the call_trees containing an unexpanded call has an
   Expansion_site.
*/
struct Expansion_sites {
	struct Call_tree *tree;		  /* a node in the call tree */
	struct Call_tree_list *tree_list;/* list that contains the tree */

	/* Estimates the number of times this node in the call tree reached
	   given that the root is reached once. Estimation based on the
	   amount of nesting at the call sites
	*/
	int caller_frequency;

	int value;			/* benefit of the expansion */
	struct Call_list *next_call;	/* next call to expand */
	struct Expansion_sites *next;	/* next call to expand */
};

static struct Expansion_sites last_site = {0,0,0, -1, 0, 0};
static struct Expansion_sites best_site ={0,0,0,MAX_VALUE,0,0};

static void
init_expansions() {
	best_site.next = &last_site;
}

/* Iteratively removes the best site from the sorted list of sites, adds
   a new node for callee to the call_tree, and adds an arc from the caller
   to the callee. If not all sites at caller have been expanded, repushes
   the caller back into sorted list. Pushes callee into sorted list.
*/
static void
expand_list() {
	struct Expansion_sites *caller;
	while ( ( caller = best_site.next) != &last_site) {
		static void back_off();
		int caller_frequency;
		struct Tree_edge *new_edge;
		struct Call_list *func_call = caller->next_call;
		struct Summary *summary;
		int limit;
		int skip_site = 0;

		if (func_call->func->flags & RECURSIVE) {
			skip_site = 1;
		}
		summary = get_summary(func_call->func);

		/* Calculate a "limit" which is some sense of the worth
		** of inlining this particular call based on an
		** arbitrary overhead associated with pushing arguments
		** to make the call and weighted by perceived optimization
		** based on the attributes of the arguments, how the formals
		** will be used and whether the call is contained within 
		** a loop or loops.
		*/
		limit = inline_expansion + (inline_benefit * func_call->value);

		/* If trying to inline everything or only doing C++ inlining,
		   avoid the limit check. */
		if ( ! (do_inline & INLINE_ALL) &&
		     ! (do_inline == CPP_INLINE_ONLY) ) {

		    if (summary->node_count > limit) {
			skip_site = 1;
		    }
		}  /* if */

		if ((func_call->flags & DELETE_ON_EXPAND) && !skip_site) {
			struct Expansion_sites *dsite;
			struct Call_tree_list *dtree_list = 
			    func_call->func->call_tree;

			/* mark GENERATED to suppress code generation */
			func_call->func->flags |= GENERATED;
#ifndef NODBG
			if (dbg_inline_flags & (DEXPAND | DEXPAND1))
				DPRINTF("deleting function=%s(%d)\n",
			    	    SY_NAME(func_call->func->sid), 
			    	    func_call->func->sid);

			/* DCOUNT may suppress this expansions, so don't
			   delete the function
			*/
			if (dbg_inline_flags & DCOUNT)
				func_call->func->flags &= ~GENERATED;
#endif
			back_off(dtree_list->tree);

			/* remove sites rooted at func_call->func */
			for (dsite = &best_site; dsite != &last_site; 
				dsite = dsite->next) {
				if (dsite->next->tree_list == dtree_list)
					dsite->next = dsite->next->next;
			}  /* for */
		}  /* if */

		if (!skip_site) {

			caller_frequency = caller->caller_frequency;

			/* add an edge in the tree */
			new_edge = add_edge(caller->tree, func_call);
		}  /* if */

		/* delete site from list */
		best_site.next = caller->next;

		/* push site back onto the list, with new next_call */
		caller->next_call = func_call->next;
		push_site(caller);

		if (!skip_site) {

			/* push the calls from the edge target */
			caller  = make_site(caller->tree_list,new_edge->callee, 
			    caller_frequency+ func_call->frequency);
			if (caller)
				push_site(caller);
		}  /* if */
	}  /* while */
}  /* expand_list */


/* Back off all expansions rooted at call_tree */
static void
back_off(call_tree)
struct Call_tree *call_tree;
{
	struct Tree_edge *edge;
	for (edge = call_tree->edges; edge; edge = edge->next) {
		back_off(edge->callee);
	}
	call_tree->edges = 0;
}

/* makes an Expansion_site */
static struct Expansion_sites *
make_site(tree_list,tree, caller_frequency) 
struct Call_tree_list *tree_list;
struct Call_tree *tree;
int caller_frequency;
{
	struct Call_list *first_call = tree->func->summary.call_list;
	struct Expansion_sites *new_site;

	/* skip deleted calls */
	while(first_call && 
	      (first_call->value < 0 || first_call->flags == DELETED ) )
		first_call = first_call->next;

	/* if no calls left then don't create site */
	if (!first_call)
		return 0;

	new_site = Arena_alloc(call_tree_arena, 1, struct Expansion_sites);
	new_site->tree= tree;
	new_site->tree_list= tree_list;
	new_site->next_call = first_call;
	new_site->caller_frequency = caller_frequency;
	return new_site;
}

/* splice a new site into the sorted list of sites */
static void
push_site(new_site)
struct Expansion_sites *new_site;
{
	struct Expansion_sites *insert;
	struct Call_list *first_call = new_site->next_call;

	
	while(first_call && 
	      (first_call->value < 0 || first_call->flags == DELETED ) )
		first_call = first_call->next;

	if (!first_call)
		return;

	/* first call is the next unexpanded call at new_site */
	new_site->next_call = first_call;

	/* value depends on value of the call, frequency of calling
	   function, and frequency of the call site
	*/
	new_site->value = make_value(new_site->caller_frequency+
	    first_call->frequency, first_call->value+1);

	/* insert the new_site into the ordered list */
	for (insert = &best_site; new_site->value < insert->next->value; 
	    insert=insert->next);
	/* insert->value > value >= insert->next->value */
	new_site->next = insert->next;
	insert->next = new_site;
}

/* weights value by frequency */
static int
make_value(frequency, value)
int frequency, value;
{
	if (MAX_VALUE / value > frequency)
		return MAX_VALUE - 1;
	return value*frequency;
}

/* adds an edge to the calling tree */
static struct Tree_edge *
add_edge(tree, call)
struct Call_tree *tree;
struct Call_list *call;
{
	static int count=0;
	struct Tree_edge *new_tree_edge = 
	   Arena_alloc(inline_arena,1, struct Tree_edge);
	new_tree_edge->next = tree->edges;
	tree->edges = new_tree_edge;
	new_tree_edge->site  = call->call_site;
#ifndef NODBG
	new_tree_edge->count = ++count;
#endif
	new_tree_edge->callee = Arena_alloc(inline_arena,1, struct Call_tree);
	new_tree_edge->callee->func = call->func;
	new_tree_edge->order = call->order;
	new_tree_edge->callee->edges = 0;
	new_tree_edge->call_list = call;
	return new_tree_edge;
}

/****************************************************************************/
/****************************************************************************/
/************************** order the trees *********************************/
/****************************************************************************/
/****************************************************************************/
/* Orders the processing of functions for modification by inline expansion.
   First create an expansion graph with nodes corresponding to the functions.
   An edge from p to q indicates that q is inlined into p. It may be the
   case the p calls r1 calls r2 .. call rn  calls q and r1 .. rn are all
   expanded. Then orders the graph so that if q is inlined in p, then
   p is processed before q. If q were processed before p, then it would
   have to be copied before processing.
*/



/* flags for visited in Expansions */
#define NOT_VISITED 0
#define VISITED 1
#define VISITING 2

static struct Expansions {
	int visited;
	Bit_vector callees;
	struct Call_tree_list *tree_root;
} *expansions;

#define FUNCT_TO_SLOT(function) \
	((function)->flags & DEFINED ? (function)->summary.function_count : 0)

#define SLOT_TO_FUNC(slot) expansions[slot].tree_root->tree->func


/* creates the expansion graph */
static struct Call_tree_list *
order_trees(trees) 
struct Call_tree_list *trees;
{
	Arena order_arena = arena_init();
	int max_count = 1;
	int func_count;
	struct Call_tree_list new_head;
	struct Call_tree_list *curr_list = &new_head;
	new_head.next = trees;

	/* Count the functions containing expansions to determine the
	   the size of the bit vector required. */
	while (curr_list->next != 0) {
	    if ((curr_list->next->tree->func->flags&GENERATED) ||
		curr_list->next->tree->edges == 0) {
		    curr_list->next->tree->func->summary.function_count = 0;
		    curr_list->next = curr_list->next->next;
		    continue;
	    }
	    else {
		++max_count;
		curr_list = curr_list->next;
	    }
	}

	trees = new_head.next;

	/* initialize expansions */
	expansions = Arena_alloc(order_arena, max_count, struct Expansions);

	func_count = 1;
	for (curr_list = trees; curr_list; curr_list = curr_list->next) {
		expansions[func_count].callees = 
		    bv_alloc(max_count, order_arena);
		bv_init(false,expansions[func_count].callees);
		expansions[func_count].tree_root = curr_list; 
		expansions[func_count].visited = NOT_VISITED;
		curr_list->tree->func->summary.function_count = func_count;
		++func_count;
	}

	/* populate expansions */
	func_count = 1;
	for (curr_list = trees; curr_list; curr_list = curr_list->next) {
		enter_expansion(expansions[func_count].callees, curr_list->tree);
		++func_count;
	}

	/* order functions */
	curr_list = 0;
	for (func_count=1; func_count < max_count; ++func_count) {
		if (!expansions[func_count].visited)
			curr_list = top_down_order(expansions+func_count,
		    	    curr_list);
	}

	arena_term(order_arena);
	return curr_list;
}


/* Orders the functions for modification by inline expansion as follows:
   If there is a path from p to q in the expansion graph and not a path from
   q to p then p will appear before q in the ordered list. If there is a cycle
   including p and q, then which ever function is placed earlier in the list
   is marked for copy, since this function will be referenced after it has
   been modified by inline expansion
*/
static struct Call_tree_list *
top_down_order(caller, curr_list)
struct Expansions *caller;
struct Call_tree_list *curr_list;
{
	caller->visited = VISITING;
	BV_FOR(caller->callees, callee)
		if (expansions[callee].visited) {

			/* if VISITING then callee will appear before caller */
			if (expansions[callee].visited == VISITING)
				expansions[callee].tree_root->open_flags =
				    EDIT_COPY;
		}
		else {
			curr_list = top_down_order(expansions+callee,curr_list);
		}
	END_BV_FOR

	caller->visited = VISITED;
	caller->tree_root->next = curr_list;
	return caller->tree_root;
}


/* enter every function below the root of call_tree into root_calls */
static void
enter_expansion(root_calls, call_tree) 
Bit_vector root_calls;
struct Call_tree *call_tree;
{
	struct Tree_edge *edge;
	for (edge = call_tree->edges; edge; edge = edge->next) {
		struct Call_tree *callee = edge->callee;
		int slot = FUNCT_TO_SLOT(callee->func);
		if (slot > 0) 
			bv_set_bit(slot, root_calls);
		enter_expansion(root_calls, callee);
		
	}
}

/*****************************************************************************/
/*****************************************************************************/
/*************************** perform inline expansion*************************/
/*****************************************************************************/
/*****************************************************************************/

/* Inline expansion is done by replacing the call to be expanded by a variable
   or a NOP (if call returns no value). The code for the inlined function
   is then inserted before the statement containing the call. The code is
   preceded by a label (to help register allocation) and followed by a
   label where control goes for a return.

   1. return is mapped to "goto endlabel;"
   2. return expr is mapped to "return_variable = expr; goto endlabel;"
   3. automatics and parameters in the inlined procedure are mapped to new
      automatics.
   4. An argument is assigned to the automatic mapped to the corresponding
      formal.
   5. labels defined in the inlined procedure are mapped to new labels.

   For C++ inlining (to be available for C in a later release), if the 
   called function is a single statement returning an expression, the
   function call may be inlined by replacing the call ND1 with the 
   expression tree from the called function and replacing references to
   the formals with references to the actual arguments of the call.
*/



/* The calls in f scheduled for expansion are ordered so that if both g
   and h are scheduled for expansion, and g is called in h's arguments, then
   g is expanded before h. The ordered calls are then expanded. After c is
   expanded, any calls from c are expanded by a recursive call to expand_calls
*/
static void
expand_calls(f, calls,level)
struct Funct_copy *f;	/* caller */
struct Tree_edge *calls;/* calls from caller */
int level;	/* for debugging only */
{
#define MAX_ORDER 1<<30
	struct Tree_edge *next_call=0;	/* init'ed for lint */
	struct Tree_edge max;
	struct Tree_edge min;
	struct Call_list * call_list;

	max.order = MAX_ORDER;
	max.next = 0;
	min.order = -1;
	min.next = &max;

	/* order the calls */
	for ( ; calls ; calls = next_call) {
		struct Tree_edge *e;
		next_call = calls->next;

		/* after iter, know that edge should be between e and e->next*/
		for (e = &min; calls->order > e->next->order; e=e->next);

		calls->next = e->next;
		e->next = calls;
	}


	/* expand the calls in order */
	for (calls=min.next ; calls->order != MAX_ORDER; calls = calls->next) {
		struct Funct_copy *callee;
		struct Call_table *e = get_call_site(calls->site, f);
#ifndef NODBG
		static int do_expand();
		if ((dbg_inline_flags & DCOUNT) && !do_expand(calls->count))
			continue;
#endif

		callee = open_function(calls->callee->func,INLINING);
#ifndef NODBG
		if (dbg_inline_flags & (DEXPAND | DEXPAND1 | DCOUNT)) {
			DPRINTF("Inlining %s %s() in %s(), size %d\n",
			(SY_CLASS(callee->func->sid) == SC_STATIC)  ? "static"
							 : "nonstatic",
			SY_NAME(callee->func->sid),
			SY_NAME(f->func->sid),
			callee->func->summary.node_count);
		}
#endif
		if (calls->call_list->flags & INLINE_AS_EXPR) {
			inline_expr(e->node, e->index, callee);
		} else {
			inline_copy(e->node, e->index, callee);
		}  /* if */
#ifndef NODBG
		/* Increment the call site expanded counter and 
		   added node counter. */
		call_sites_expanded++;
		added_node_count += callee->func->summary.node_count;
#endif  /* ! NODBG */

		/* One call to the callee has been resolved; decrement the
		   counter for the remaining calls to callee.*/
		if ( --calls->callee->func->times_called == 0) {
		    /* The number of times that this function remains to 
		       be called has reached zero.  Signal a run through
		       the list of functions to free nested calls. */
		    function_no_longer_called = 1;
		}  /* if */

		/* After inlining callee, all the functions called by
		   callee must have their times_called counter
		   incremented.  Walk through the Call_list for the
		   inlined function to increment the references to each
		   called function.  The Call_list is walked, because the
		   the existing call trees only reflects the functions
		   that can be inlined.
		*/
		for (call_list = calls->callee->func->summary.call_list;
		     call_list;
		     call_list = call_list->next) {

		    ++call_list->func->times_called;
		}  /* for */

		expand_calls(callee, calls->callee->edges,level+1);
		close_function(callee);
	}
}


static void 
inline_copy(site, index, call)
ND1 *site;		/* points to a call */
Cgq_index index;	/* index of tree containing the call */
struct Funct_copy *call;
{
	Cgq_index prev;
	cgq_t *cgq,*lab;
	
	int return_label;
	SX return_value = 0;
	ND2 *end;
	Cgq_index call_index = index;
	int cgq_ln, cgq_dbln;

#ifndef NODBG
	if (dbg_inline_flags & (DEXPAND1)) {
		DPRINTF("\t\tBefore: \n\n");
		cg_putq_between(3, index, index);
	}
#endif
	/* Save the index of the previous cgq item as well as the
	   line numbers associated with the call site. */
	prev = (cgq = CGQ_ELEM_OF_INDEX(index))->cgq_prev;
	cgq_ln = cgq->cgq_ln;
	cgq_dbln = cgq->cgq_dbln;

	/* create a label before expansion to help register allocation */
	index = CGQ_INDEX_OF_ELEM(lab=cg_q_insert(prev));
	lab->cgq_ln = cgq_ln;
	lab->cgq_dbln = cgq_dbln;
	lab->cgq_op = CGQ_EXPR_ND2;
	lab->cgq_arg.cgq_nd2=end=(ND2 *)talloc();
	end->op=LABELOP;
	end->label = getlab();

	/* first item in expanded function */
	index = CGQ_INDEX_OF_ELEM(cg_q_insert(index));

	/* label after function */
	lab = cg_q_insert(index);
	lab->cgq_ln = cgq_ln;
	lab->cgq_dbln = cgq_dbln;
	lab->cgq_op = CGQ_EXPR_ND2;
	lab->cgq_arg.cgq_nd2=end=(ND2 *)talloc();
	end->op=LABELOP;
	end->label =return_label =getlab();

	if (TY_TYPE(site->type) != TY_VOID) {

		/* create scope markers for return value */
		TEMP_SCOPE_ID scope;
		Cgq_index end_scope_index;

		scope = al_create_scope(index,
				(end_scope_index = extend_scope(call_index)));
		return_value = sy_temp(site->type);
		SY_NAME(return_value) = "RNODE";
		al_add_to_scope(return_value,scope);

		/* Synchronize lines numbers. */
 		cgq = CGQ_ELEM_OF_INDEX(CGQ_ELEM_OF_INDEX(index)->cgq_prev);
		cgq->cgq_ln = cgq_ln;
		cgq->cgq_dbln = cgq_dbln;
 		cgq = CGQ_ELEM_OF_INDEX(
			CGQ_ELEM_OF_INDEX(end_scope_index)->cgq_next);
		cgq->cgq_ln = cgq_ln;
		cgq->cgq_dbln = cgq_dbln;

		/* replace call by reference to return_value */
		site->op = NAME;
		site->lval=0;
		site->rval = return_value;
	}
	else {
		/* replace call by a no-opt */
		site->op = ICON;
		site->type = TY_INT;
		site->lval = site->rval = 0;
	}

	/* read in cgq items from the inlined function to the calling
	   function until encounter a db_symbol. This symbol declares an
	   argument. The read_cgq_item has replaced the sid in the db_symbol
	   with a new sid for an automatic. An assignment of an argument to
	   the db_symbol is then inserted.
	*/
	cgq=CGQ_ELEM_OF_INDEX(index);
	for (;;) {
		read_cgq_item(call,index, return_value);
		if (cgq->cgq_op ==CGQ_CALL && cgq->cgq_func == db_s_block)
			break;
		if (cgq->cgq_op == CGQ_CALL_SID && cgq->cgq_func == db_symbol) {
			SX param=cgq->cgq_arg.cgq_sid;

			cgq->cgq_ln = cgq_ln;
			cgq->cgq_dbln = cgq_dbln;

			/* insert a cgq item for assignment to arg */
			index = CGQ_INDEX_OF_ELEM((cgq=cg_q_insert(index)));
			cgq->cgq_ln = cgq_ln;
			cgq->cgq_dbln = cgq_dbln;
			cgq->cgq_op = CGQ_EXPR_ND1;
			cgq->cgq_arg.cgq_nd1 = 
			    make_assign(param, next_argument(&site->right));

			/* insert an element for the next read */
			index = CGQ_INDEX_OF_ELEM(cgq=cg_q_insert(index));
		}
	} 
	/* Any arguments and the db_s_block are now in caller's cgq.
	   Update the line numbers in the db_s_block cgq item. */
	cgq->cgq_ln = cgq_ln;
	cgq->cgq_dbln = cgq_dbln;

	/* Read in the rest of the function body. read_cgq_item copies each
	   cgq item into the caller's cgq. read_cgq_item creates new symbols
	   for automatics defined in the function body and creates new labels
	   for labels defined in the body. References to automatics and labels
	   are mapped to the new symbols and labels.
	*/
	do {
		ND2 *jump;

		index = CGQ_INDEX_OF_ELEM(cgq=cg_q_insert(index));
		read_cgq_item(call,index,return_value);
		cgq->cgq_ln = cgq_ln;
		cgq->cgq_dbln = cgq_dbln;

		/* translate RETURNS */
		if (cgq->cgq_op == CGQ_EXPR_ND1 && 
		    cgq->cgq_arg.cgq_nd1->op == RETURN) {

			/* make a goto */
			cgq->cgq_op = CGQ_EXPR_ND2;
			cgq->cgq_arg.cgq_nd2=jump = (ND2*)talloc();
			jump->op = JUMP;
			jump->type = T2_INT;
			jump->label = return_label;
		}
	} while(!(cgq->cgq_op ==CGQ_CALL && cgq->cgq_func == db_e_fcode));
	cg_q_delete(index);	/* delete the db_e_fcode */

#ifndef NODBG
	if (dbg_inline_flags & DEXPAND1) {
		DPRINTF("\t\tAfter: \n\n");
		cg_putq_between(3,prev,call_index);
	}
#endif
}  /* inline_copy */



/*
   Data structure used to keep track of information for each parameter
   processed when inlining functions calls as an expression. 
*/
struct expr_inline_arg_info {
	ND1 * arg_node;			/* Pointer to the ND1 which is the
					   argument. */
	int arg_uses;			/* Number of remaining uses of the
					   formal param corresponding to
					   this argument. */
};


static struct expr_inline_arg_info *arg_info;


static void substitute_args(node_address)
ND1 **node_address;
/*
   Walk down the expression pointed to by ND1 pointer at node_address
   and replace each formal parameter with its corresponding argument.
*/
{
    int param;

    if (optype((*node_address)->op) == LTYPE) {
	param = IS_PARAM(*node_address);
	if (param >= 0) {
	    /* Replace this formal parameter with the corresponding
	       argument. */
	    nfree(*node_address);
	    if (--(arg_info[param].arg_uses) == 0) {
		/* Last use or only use of the argument; use it directly. */
	        *node_address = arg_info[param].arg_node;
	    } else {
		/* Additional uses of this arguemnt to be located.  Use
		   a copy at this location. */
		*node_address = (ND1 *)tcopy(arg_info[param].arg_node);
	    }  /* if */
	}  /* if */
    } else {	
	substitute_args( & (*node_address)->left);
	if (optype((*node_address)->op) == BITYPE) {
	    substitute_args( & (*node_address)->right);
	}  /* if */
    }  /* if */

}  /* substitue_args */



static void 
inline_expr(site, index, call)
ND1 *site;			/* points to a call */
Cgq_index index;		/* index of tree containing the call */
struct Funct_copy *call;
/*
   Replace the call tree pointed to by site with the single expression
   (one statement) function designated by call.  Substitute the
   actual arguments of the call for the corresponding formal parameters in
   the expression.
*/
{
    int param_count;
    int param;
    ND1 *node;
    ND1 *extra_node;

#ifndef NODBG
    Cgq_index earliest_inserted_index = index;
    Cgq_index last_inserted_index = index;

    if (dbg_inline_flags & (DEXPAND1)) {
	DPRINTF("\t\tBefore: \n\n");
	cg_putq_between(3, index, index);
    }  /* if */
#endif

    /* Make a copy of the called function's single expression in the
       current functions address space. */
    node = (ND1 *)tcopy(call->func->summary.single_expression_return);

    /* Check if this inlined function made use of any local variables.
       If so, the db_symbol CGQ items must be inserted before this CGQ
       item and the symbol table ids remapped. */
    if (call->func->summary.flags & HAS_LOCALS) {
	int	seen_start_of_block = 0;
	int	cgq_ln;
	int	cgq_dbln;
	Cgq_index insert_index;
	cgq_t	*inserted_cgq;
	Cgq_index end_scope_index;


	cgq_ln = CGQ_ELEM_OF_INDEX(index)->cgq_ln;
	cgq_dbln = CGQ_ELEM_OF_INDEX(index)->cgq_dbln;
	insert_index = CGQ_PREV_INDEX(CGQ_ELEM_OF_INDEX(index));

	/* determine where the end of this pseudo-scope is. */
	end_scope_index = extend_scope(index);

	CGQ_FOR_ALL_QUEUE(cgq, callee_index, &call->func->save_cgq)
	    switch(cgq->cgq_op) {

	    case CGQ_CALL:
		if (cgq->cgq_func == db_s_block) {
		    seen_start_of_block = 1;
		}  /* if */

	    case CGQ_CALL_SID:
		if (seen_start_of_block &&
		    cgq->cgq_func == db_symbol) {
		    /* This is an auto variable of the function being
		       inlined. */
		    SX sid;

		    inserted_cgq = cg_q_insert(insert_index);
		    insert_index = CGQ_INDEX_OF_ELEM(inserted_cgq);
		    inserted_cgq->cgq_op = cgq->cgq_op;
		    inserted_cgq->cgq_func = cgq->cgq_func;
		    inserted_cgq->cgq_arg.cgq_sid = sid = 
				translate_symbol(cgq->cgq_arg.cgq_sid, call,
						 0 /* do not remap params */);
		    inserted_cgq->cgq_location = cgq->cgq_location;
		    inserted_cgq->cgq_ln = cgq_ln;
		    inserted_cgq->cgq_dbln = cgq_dbln;
		    inserted_cgq->cgq_scope_marker = cgq->cgq_scope_marker;
		    inserted_cgq->cgq_flags = 
					cgq->cgq_flags | CGQ_NO_DEBUG;

		    /* Put an "os_symbol" entry after the end_scope_index
		       CG Q entry. */
		    inserted_cgq = cg_q_insert(end_scope_index);
		    inserted_cgq->cgq_op = CGQ_CALL_SID;
		    inserted_cgq->cgq_func = os_symbol;
		    inserted_cgq->cgq_arg.cgq_sid = sid;
		    inserted_cgq->cgq_ln = cgq_ln;
		    inserted_cgq->cgq_dbln = cgq_dbln;
#ifndef NODBG
		    if (earliest_inserted_index == index) {
			/* This is the first or only local. */
			earliest_inserted_index = insert_index;
			last_inserted_index = CGQ_INDEX_OF_ELEM(inserted_cgq);
		    }  /* if */
#endif
		    /* Put an "db_sy_clear" entry after the end_scope_index
		       CG Q entry; in front of the os_symbol. */
		    inserted_cgq = cg_q_insert(end_scope_index);
		    inserted_cgq->cgq_op = CGQ_CALL_SID;
		    inserted_cgq->cgq_func = db_sy_clear;
		    inserted_cgq->cgq_arg.cgq_sid = sid;
		    inserted_cgq->cgq_ln = cgq_ln;
		    inserted_cgq->cgq_dbln = cgq_dbln;
		}  /* if */
	    }  /* switch */
	CGQ_END_FOR_ALL

	/* Remap the called functions locals, but not the formals. */
	extra_node = (ND1 *)copy_tree(node, call, index, 0,
				      0 /* do not remap formals */);
	tfree(node);
	node = extra_node;
    }  /* if */

    tfree(site->left);			/* Free the left node - the call. */
    if ((param_count = call->func->summary.formal_count) > 0) {
	/* At least one parameter.  We need to substitute each reference
	   to a formal with its corresponding actual argument expression. */
	arg_info = Arena_alloc(call->arena, param_count,
			       struct expr_inline_arg_info);
	for (param = 0; param < param_count; param++) {
	    arg_info[param].arg_node = next_argument(&site->right);
	    arg_info[param].arg_uses = call->func->summary.formals[param].uses;
	}  /* for */

	/* Free the remaining nodes of the argument tree (right). */
	tfree(site->right);

	substitute_args(&node);
    }  /* if */

    /* Substitute the new expression for the call site. */
    *site = *node;

    /* Free the single node at the top of the copy; it's values have
       been copied into *site. */
    nfree(node);

    /* Check for the special case of an empty function; just consisting of a
       RETURN node. */
    if (site->op == RETURN) {
	/* Replace the return by a no-op. */
	site->op = ICON;
	site->type = TY_INT;
	site->lval = site->rval = 0;
    }  /* if */


#ifndef NODBG
    if (dbg_inline_flags & (DEXPAND1)) {
	DPRINTF("\t\tAfter: \n\n");
	cg_putq_between(3, earliest_inserted_index, last_inserted_index);
    }  /* if */
#endif
}  /* inline_expr */



/* Obtain the cgq for the opened functions, intialize tables used to map
   symbols and labels, and tables that obtain Cgq_index and node pointers
   for Call_sites. Symbol tables and label tables used for INLINING. Call_site
   table used in all modes of open. Since saved cgq is not modified for
   open(EDIT_COPY) or open(EDITING), the Call_site table can be translated
   at once. For open(EDITING), the Call_site table is filled in as each
   cgq_item containing a Call_site is read by read_cgq_item
*/
static
struct Funct_copy *
open_function(f,flag) Function f; int flag; {
	Arena a = arena_init();
	struct Funct_copy *fc = Arena_alloc(a,1,struct Funct_copy);
	fc->func = f;
	fc->arena = a;
	fc->flags = flag;
	fc->call_table = Arena_alloc(a, f->expr_count+1, struct Call_table);

	if (!(f->flags & DEFINED))
		cerror(gettxt(":228","Function \"%s\"does not exist"), f->name);

	if (flag == INLINING) {
		fc->next_index = CGQ_FIRST_INDEX;
		fc->label_table = Arena_alloc(a, f->max_label-f->min_label,int);
		fc->symbol_table = 
		    Arena_alloc(a,f->max_symbol-f->min_symbol, SX );
		memset(fc->label_table,0,sizeof(int)*
		    (f->max_label - f->min_label));
		memset(fc->symbol_table,0,sizeof(SX)*
		    (f->max_symbol - f->min_symbol));
	}
	else if (flag == EDIT_COPY) {

		/* expand linked list form of Call_site mappings in Function
		   to array format used in Funct_copy
		*/
		expand_call_site_tables(fc);

		/* makes f the "current" cgq. The cgq accessed and modified
		   by acomp's primitivies (cgq_insert, CGQ_FOR_ALL, etc)
		*/
		restore(f);

		/* creates a copy of the cgq, and stores pointer in f */
		copy(f);
	}
	else if (flag == EDITING) {
		/* editing destroys the definition */
		f->flags &= ~DEFINED;
		expand_call_site_tables(fc);

		/* makes f the "current" cgq. The cgq accessed and modified
		   by acomp's primitivies (cgq_insert, CGQ_FOR_ALL, etc)
		*/
		restore(f);
	}
	else {
		cerror(gettxt(":229","bad flag to open_function"));
	}
	return fc;
}
/* Deletes tables created by open, generates code for functions opened for
   EDIT or EDIT_COPY
*/
static void
close_function(f) struct Funct_copy *f; {
	if (f->flags == EDIT_COPY || f->flags == EDITING) {
		cg_setcurfunc(f->func->sid);
		cg_endf(al_endf());
		f->func->flags |= GENERATED;
	}
	arena_term(f->arena);
}



/* makes f the "current" cgq. The cgq accessed and modified
   by acomp's primitivies (cgq_insert, CGQ_FOR_ALL, etc)
*/
static void
restore(f) Function f; 
{
	global_arena = f->save_arena;
	td_cgq = f->save_cgq;
	al_begf();      /* marks f as no setjmp or asm since f is optimizable */
	cg_restore(f->save_index);
}

/* copies the CGQ for open(EDIT_COPY) */
static void
copy(f) Function f;
{
	Arena save_arena;

	f->save_arena = arena_init();
	f->save_index = CGQ_FIRST_INDEX;
	f->save_cgq.td_used = 0;
	f->save_cgq.td_start = (myVOID *)malloc(td_cgq.td_allo*sizeof(cgq_t));
	if (!f->save_cgq.td_start)
		cerror(gettxt(":130","No space for cgq"));

	/* do not need call_table_list and expr_count */

	save_arena = global_arena;
	global_arena = f->save_arena;

	/* access a cgq item from the current cgq, and copy it into an empty
	   element in the new cgq (f->save_cgq). Point the forward pointer
	   of the previous element to the copied item
	*/
	CGQ_FOR_ALL(src_pt, src_index)
		Cgq_index targ_index = TD_USED(f->save_cgq)*sizeof(cgq_t);
		cgq_t *targ_pt = CGQ_ELEM_OF_INDEX_Q(targ_index,&f->save_cgq);

		/* copy cgq item, RMA should do field by field */
		*targ_pt = *src_pt;

		targ_pt->cgq_prev = f->save_index;

		/* set forward pointer of previous element to targ_index. On
		   first item, this action does nothing harmful or useful, but
		   avoids making a special case
		*/
		CGQ_ELEM_OF_INDEX_Q(f->save_index,&f->save_cgq)->cgq_next =
				targ_index;

		/* copy trees */
		switch(src_pt->cgq_op) {
		case CGQ_FIX_SWBEG:
		case CGQ_EXPR_ND1:
		case CGQ_EXPR_ND2:
			targ_pt->cgq_arg.cgq_nd1 = 
			    (ND1 *)tcopy(targ_pt->cgq_arg.cgq_nd1);
		}

		++TD_USED(f->save_cgq);
		TD_CHKMAX(f->save_cgq);
		f->save_index = targ_index;
	CGQ_END_FOR_ALL

	CGQ_ELEM_OF_INDEX_Q(f->save_index,&f->save_cgq)->cgq_next = CGQ_NULL_INDEX;

	global_arena = save_arena;
}

/* RMA does it need to return? */
/* Copies the next item from f's cgq into the slot in the current cgq indexed
   by buffer index. Following transformations are done (by routines called
   from read_cgq_item):
	1. References to RNODE replaced by the return variable.
	2. Labels are mapped to new labels.
	3. Automatics and parameters are mapped to new automatics and
	   are flagged to suppress emitting symbolic debugging information.
	4. When copying a tree that was passed to mark_call_site, the new
	   location of the tree is noted in "f". 
*/
static int
read_cgq_item(f,buffer_index,return_variable)
struct Funct_copy *f;
Cgq_index buffer_index;
int return_variable;
{
	cgq_t *place=CGQ_ELEM_OF_INDEX(buffer_index);
	Cgq_index save_prev = place->cgq_prev;
	Cgq_index save_next = place->cgq_next;
	
	struct td *cgq;
	Cgq_index input_index = f->next_index;

	cgq = &f->func->save_cgq;

	if (input_index == CGQ_NULL_INDEX || input_index > CGQ_LAST_ITEM(cgq)) {
		cerror(gettxt(":230","reading eof"));
		return 0;
	}

	/* copy the cgq item (RMA inefficient) */
	*place = *CGQ_ELEM_OF_INDEX_Q(input_index,cgq);
	f->next_index = place->cgq_next;

	place->cgq_prev = save_prev;
	place->cgq_next = save_next;

	switch (place->cgq_op) {
		case CGQ_EXPR_ND1:
		case CGQ_EXPR_ND2:
		case CGQ_FIX_SWBEG:
			/* copy the trees and perform transformations */
			place->cgq_arg.cgq_nd1=
			    (ND1 *)copy_tree((NODE *)place->cgq_arg.cgq_nd1,f,
					     buffer_index, return_variable,
					     1 /*remap params */);
			break;
		case CGQ_CALL_SID:
			place->cgq_arg.cgq_sid =
			    translate_symbol(place->cgq_arg.cgq_sid,f,
					     1 /* remap params */);
			if (place->cgq_func == db_symbol) {
				place->cgq_flags |= CGQ_NO_DEBUG;
			}  /* if */
	}
	return 1;
}
/* copies trees and performs transformations described in read_cgq_item */
static NODE *
copy_tree(in,f,index, return_variable, remap_formals)
NODE *in; 
struct Funct_copy *f;
Cgq_index index;
int return_variable;
int remap_formals;		/* Non-zero is formals are to be mapped
				   also.  This is set for copy block
				   inlining and not set for expression
				   inlining. */
{
	/* should call translate type */
	/* should call translate string */
	NODE *out=talloc();

	*out = *in;	/* we can be much more efficient */

	/* non-zero "opt" indicates expression tagged by mark_call_site,
	   more new location of this expression in the call_table
	*/
	if (in->fn.opt) {
		f->call_table[(int)in->fn.opt].node = (ND1 *)out;
		f->call_table[(int)in->fn.opt].index = index;
	}

	if (optype(in->tn.op) == UTYPE || optype(in->tn.op) == BITYPE)
		out->tn.left = copy_tree(in->tn.left,f,index,return_variable,
					 remap_formals);
	if (optype(in->tn.op) == BITYPE)
		out->tn.right = copy_tree(in->tn.right,f,index,return_variable,
					  remap_formals);

	switch(in->tn.op) {
	case CBRANCH:
		((ND1 *)out)->right->lval = 
		    translate_label((int)((ND1 *)in)->right->lval,f);
		break;
	case ICON:
	case NAME:
		if (in->fn.rval > 0)
			out->fn.rval = translate_symbol(in->fn.rval,f,
							remap_formals);
		else if (in->fn.rval < 0)
			out->fn.rval = -translate_label((int)-in->fn.rval,f);
		break;
	case RNODE:
		/* translate RNODE to return value */
		out->fn.op = NAME;
		out->fn.flags = 0;
		out->fn.lval = 0;
		out->fn.rval = return_variable;
		break;
	case LABELOP:
	case JUMP:
	case SWCASE:
		out->tn.label = translate_label(in->tn.label,f);
		break;
	case SWEND:
		if (out->tn.lval != -1)
			out->tn.lval = translate_label((int)in->tn.lval,f);
		break;
	}
	return out;
}

/*
   Returns the mapped value of a label. If no mapped value exists (mapped
   value is 0), then set the mapped value to a new label
*/
static int
translate_label(before,f) int before; struct Funct_copy *f; {
	int *label;
	label = &f->label_table[before-f->func->min_label];
	if (*label == 0) {
		*label = getlab();	/* gets a new label */
	}
	return *label;
}

/*
   Returns the mapped value of a symbol table index
*/
static int
translate_symbol(before, f, remap_formals)
SX before;
struct Funct_copy *f;
int remap_formals;		/* non-zero if params are to be remapped. */ 
{

	SX after;

	/* check if mapping appropriate */
	if (!(SY_CLASS(before) == SC_LABEL || SY_CLASS(before) == SC_AUTO || 

		/* Obscure point: The check for SY_LEVEL == 1 insures the 
	   	  "before" is a parameter to f, and not a parameter to 
	   	  a function prototype declared in f)
		*/
		(SY_CLASS(before) == SC_PARAM && SY_LEVEL(before) == 1
					&& remap_formals)))

		return before;

	/* map the symbol if not mapped already */
	if (!(after=f->symbol_table[before-f->func->min_symbol])) {
		after = sy_temp(SY_TYPE(before));	/* new sumbol */
		SY_NAME(after) = SY_NAME(before);
		SY_FLAGS(after) = SY_FLAGS(before);
		f->symbol_table[before-f->func->min_symbol] = after;

		/* SY_OFFSET of a label symbol is the label number */
		if (SY_CLASS(before) == SC_LABEL) {
			SY_CLASS(after) = SC_LABEL;
			SY_OFFSET(after) = translate_label(SY_OFFSET(before),f);
		}
	}

	return after;
}

static struct Call_table *
get_call_site(handle,fc ) Call_site handle; struct Funct_copy *fc; {
	return &fc->call_table[handle];
}
/* if not SWBEG return index, else return index of SWEND, prevents register
   allocation from placing code between SWBEG and SWEND
*/
static Cgq_index
extend_scope(index)
Cgq_index index;
{
	cgq_t *elem = CGQ_ELEM_OF_INDEX(index);
	if (elem->cgq_op != CGQ_FIX_SWBEG) 
		return index;
	while (elem->cgq_arg.cgq_nd2->op != SWEND) {
		index = elem->cgq_next;
#ifndef NODBG
		if (index == CGQ_NULL_INDEX)
			cerror("SWEND not found");
#endif
		elem=CGQ_ELEM_OF_INDEX(index);
	}
	return index;
}

static struct Summary *
get_summary(f)
Function f;
{
	if (f->summary.node_count == -1)	/* check if has summary */
		return 0;
	else
		return &f->summary;
}

/* if one argument returns it, otherwise delete the first argument,
   and return it
*/
static ND1 *
next_argument(arg_adr) ND1 **arg_adr;
{

	ND1 *arg = *arg_adr;

	if (arg->op == CM) {
		for ( ; arg->left->op == CM; arg=arg->left) {
			arg_adr = &arg->left;
		}
		/* arg is lowest CM */
		nfree(arg);

		arg=arg->left;

		/* replace CM by right child */
		*arg_adr = (*arg_adr)->right;	
	}

	/* arg points to left-most  ARG node */
	nfree(arg);
	return arg->left;
}


/* converts index list and node list to index and node tables */
static void
expand_call_site_tables(f) struct Funct_copy *f;
{
	struct Call_table_list *e;
	struct Call_table *call_table=f->call_table;
	int count;

	/* The Call_table_list pointed to by the Funct struct is
	   in LIFO sequence. */
	for (e=f->func->call_table_list,count=f->func->expr_count; e; 
	    e = e->next,--count) {
		call_table[count].node = e->node;
		call_table[count].index = e->index;
	}
}


/* build an assignment of node to NAME(sid) */
static ND1 * 
make_assign(sid, node) SX sid; ND1 *node; {
	ND1 *assign = t1alloc();
	ND1 *targ;
	assign->op = ASSIGN;
	assign->flags = FF_SEFF;
	assign->type = SY_TYPE(sid);
	if (TY_UNQUAL(assign->type) != TY_UNQUAL(node->type)) {
		ND1 *convert = t1alloc();
		convert->left = node;
		convert->op = CONV;
		convert->type = assign->type;
		convert->flags = FF_ISCAST;
		node = convert;
	}
	assign->right = node;
	assign->left = targ = t1alloc();
	targ->op = NAME;
	targ->rval = sid;
	targ->lval = 0;
	targ->type = assign->type;
	return assign;
}
#ifndef NODBG



/* print the function flags */
static void
pr_funct_flags(flags) int flags; {
	DPRINTF("    function flags:");
	if (flags & GENERATED)		DPRINTF(" GENERATED");
	if (flags & DEFINED)		DPRINTF(" DEFINED");
	if (flags & CALLED)		DPRINTF(" CALLED");
	if (flags & CALLED_ONCE)	DPRINTF(" CALLED_ONCE");
	if (flags & NODELETE)		DPRINTF(" NODELETE");
	if (flags & RECURSIVE)		DPRINTF(" RECURSIVE");

	if (flags & CPP_USER_INLINE)	DPRINTF(" C++_INLINE");
	if (flags & CPP_CPFE_INLINE)	DPRINTF(" C++_FE_INLINE");
	DPRINTF("\n");
}

/* print the function summary flags */
static void
pr_summary_flags(flags) int flags; {
	DPRINTF("    summary flags:");
	if (flags & CONTAINS_LOOP)	DPRINTF(" CONTAINS_LOOP");
	if (flags & LEAF_FUNCTION)	DPRINTF(" LEAF_FUNCTION");
	if (flags & HAS_LOCALS)		DPRINTF(" HAS_LOCALS");

	if (flags & CNI_IS_VARARG)	DPRINTF(" IS_VARARG");
	if (flags & CNI_RET_SU)		DPRINTF(" RET_SU");
	if (flags & CNI_SU_ARG)		DPRINTF(" SU_ARG");
	if (flags & CNI_STATIC_LOCAL)	DPRINTF(" STATIC_LOCAL");
	if (flags & CNI_UNKNWN_PRMS)	DPRINTF(" UNKNOWN_PARAMS");
	DPRINTF("\n");
}




/* prints the call graph */
static void
pr_graph() {
	Function f;
	for (f=functions; f; f = f->next) {
		int i;
		struct Call_list *call;
		if (!(f->flags&DEFINED))
			continue;

		/* print the function and param info */
		DPRINTF("function=%s(%d) nodes=%d times called=%d parmcnt=%d params=(",
		    SY_NAME(f->sid),f->sid, f->summary.node_count, 
		    f->times_called,f->summary.formal_count);

		/* params */
		for (i=0; i<f->summary.formal_count; ++i)
			DPRINTF("%d ", f->summary.formals[i]);
		DPRINTF(")\n");

		/* print out flags settings */
		if (f->flags) pr_funct_flags(f->flags);
		if (f->summary.flags) pr_summary_flags(f->summary.flags);

		/* print the calls */
		for (call=f->summary.call_list; call; call=call->next) {
		    if (call->flags == DELETED)
			continue;
		    DPRINTF("\tcall %s order=%d site=%d f=%d val=%d func=%s(%d) argcnt=%d args=( ",
		    call->flags & DELETE_ON_EXPAND ? "(delete_on_expand)" : "",
		    call->order, call->call_site, call->frequency, 
		    call->value,
		    SY_NAME(call->func->sid),
		    call->func->sid, call->arg_count);

			/* call args */
			for(i=0; i < call->arg_count; ++i)
				DPRINTF("%d ", call->args[i]);
			DPRINTF(")\n");
		}


	}

}
/* puts the cgq associated with a function */
void 
putq(f,level)
Function f;
int level;
{
	struct td save_q;
	Cgq_index save_index;

	DPRINTF("Function = %s(%d)\n", SY_NAME(f->sid), f->sid);
	if (level == 0)
		return;

	/* save cgq */
	save_index = cg_inline_endf();
	save_q = td_cgq;

	/* clobber cgq with functions cgq */
	td_cgq = f->save_cgq;
	cg_restore(f->save_index);

	cg_putq(3);

	/* restore cgq */
	td_cgq = save_q;
	cg_restore(save_index);
}

/* prints the call trees showing the scheduled expansions */
void put_call_tree(tree,level) 
struct Call_tree *tree;
int level;
{
	struct Tree_edge *edge;
	DPRINTF("%d %s(%d) \n ",level,tree->func->name, tree->func->sid);
	for (edge = tree->edges; edge; edge = edge->next) {
		DPRINTF("call at site %d from %d to ", edge->site, level);
		put_call_tree(edge->callee,++level);
	}
}
void put_expansion(e)
struct Expansions *e;
{
	DPRINTF("expansion for func %s(%d) expand=%d visit=%d calls",
		e->tree_root->tree->func->name,
		e->tree_root->tree->func->sid,
		e->tree_root->tree->func->summary.function_count,
		e->visited);
	BV_FOR(e->callees,f)
		DPRINTF("expand=%d %s(%d) ", SLOT_TO_FUNC(f)->summary.function_count,
		SLOT_TO_FUNC(f)->name, SLOT_TO_FUNC(f)->sid);
	END_BV_FOR
	DPRINTF("\n");
}

void 
put_call_tree_list(list)
struct Call_tree_list *list;
{
	for (; list; list=list->next)
		DPRINTF("list_flags=0x%x func=%s(%d) fun_flags=0x%x edges=0x%x slot=%d\n",
		    list->open_flags, list->tree->func->name,
		    list->tree->func->sid,
		    list->tree->func->flags,
		    list->tree->edges,
		    list->tree->func->summary.function_count);
}
/* used when dbg_inline_flags & DCOUNT, determines whether count is in the list
   of integers listed in the environment variable DCOUNT
*/
static int
do_expand(count)
int count;
{
	char *getenv();
	char *dcount = getenv("DCOUNT");
	char *value_pt;
	int value;

	/* expand if DCOUNT not set */
	if (dcount == 0)
		return 1;

	/* check for count in list of integers at dcount */
	for (value_pt = dcount; (value=strtol(value_pt, &value_pt, 10)) != 0;)
		if (value == count)
			return 1;
	return 0;
}

void put_call_list(list)
struct Call_list *list;
{
	for (;list;list=list->next) {
		
		if (list->flags == DELETED)
			continue;
		DPRINTF("site=%d %s func=%s(%d)\n", list->call_site,
			list->flags & DELETE_ON_EXPAND ? "delete" : "",
			list->func->name, list->func->sid);
		list = list->next;
	}
	DPRINTF("\n");
}
#endif
#endif /* NO_AMIGO */
