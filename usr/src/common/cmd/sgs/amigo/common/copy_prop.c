/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)amigo:common/copy_prop.c	1.23"

#include "amigo.h"
#include "costing.h"


Arena copy_arena;

static Expr *assign_array;
#define ASSIGN_ARRAY_SIZE expr_array_size /* For now PSP */
static int last_assign; /* Id ( and index into assign_array ) of last seen assgnment */

static Expr_set kills; /* Working set for expr kills */
static Assign_set avail; /* Working set for current available copies */
static Expr_set bv_scratch;

	/* file static functions */
static void PROTO(	init_copy_prop,(Block));
static void PROTO(	init_block,(Block));
static ND1 * PROTO(	find_and_replace_copies,(int context, ND1 *, Boolean *));
static void PROTO(	collect_local_info,(Block));
static void PROTO(	kill_assigns,(ND1 *, Assign_set));
static Boolean PROTO(	in_scope, (Expr expr, Cgq_index));


	/* debugging */
#ifndef NODBG

static Boolean PROTO(	kills_assign, (int assign, Expr_set kills));
static PROTO(		copy_prop_debug, (Block blocks));

#else

#define kills_assign KILLS_ASSIGN

#endif

	/* predicates */
#define KILLS_ASSIGN(assign,kills) \
	( \
	bv_belongs(LEFT_INFO(assign_array[assign])->setid, kills) \
	|| \
	bv_belongs(RIGHT_INFO(assign_array[assign])->setid, kills) \
	)

	/*
	** Recall that the object associated with an ASSIGN node
	** is the object killed by the ASSIGN node.
	*/

#define OPTABLE_FOR_CP(node) \
	(OPTABLE(node) ||       \
		(!ieee_fp()     \
		&& EXCEPT_OPT(node)\
		&& (node->opt->flags&RAISES_EXCEPTION) == ARITH_EXCEPTION))

#define IS_CONST_COPY_NODE(n) (\
	n->op == ASSIGN \
	&& n->left->opt->flags&OBJECT \
	&& n->left->opt->object->flags&VALID_CANDIDATE \
	&& (n->right->op == ICON || (n->right->op == FCON && !ieee_fp())) \
	&& OPTABLE_FOR_CP(n->right) \
	&& OPTABLE_FOR_CP(n->left) \
	)

	/*
	** Next two variables are used to avoid passing of locals to
	** recursive functions.
	*/

static Block curr_block;
static Cgq_index curr_flow_index;

static Boolean
is_valid_copy_node(n, kills)
ND1 *n;
Expr_set kills;
{
	Boolean retval;
	retval = false;
	if(n->op != ASSIGN)
		return false;
	if(n->left->op == FLD)
		return false;
	else if(! OPTABLE_FOR_CP(n->left))
		return false;
	else if(! (n->left->opt->flags&OBJECT))
		return false;
	else if(! (n->left->opt->object->flags&VALID_CANDIDATE))
		return false;
	else if(! OPTABLE_FOR_CP(n->right))
		return false;
	else if(TY_ISFPTYPE(n->left->type) && ieee_fp())
		return false;
	if ( n->right->opt->flags & OBJECT)
		retval = COST(n->left) >= COST(n->right)
		&&  ! bv_belongs(n->right->opt->setid,kills);
	else if(n->right->op == ICON || n->right->op == FCON)
		retval = COST(n->left) >= COST(n->right);
	else DEBUG_UNCOND(if (aflags[ep].xflag))
		if(n->right->opt->flags & OBJECT_PLUS_CONST) {
			if(!(n->right->left->opt->flags & OBJECT))
				retval = false;
			else if (!OPTABLE(n->right->left))
				retval = false;
			else if(bv_belongs(n->right->left->opt->setid,kills))
				retval = false;
			else
				retval = COST(n->left) >= COST(n->right->left);
		}
	return retval;
}

	/* Entry point */
void
copy_prop(blocks)
Block blocks;
{
	init_copy_prop(blocks);

	avail_in(copy_arena); /* Solve global data flow */
	DEBUG_COND(aflags[cp].level&2, copy_prop_debug(blocks));

	DEPTH_FIRST(blk_ptr)

	Block block;
	curr_block = block = *blk_ptr;
	bv_assign(avail, block->avin); /* Maybe just avail = block->avin?? PSP */

	DEBUG(aflags[cp].level&2,
		("^^^^^^^^^\nChecking for replace in block %d \n",
		block->block_count));

	FOR_ALL_ND1_IN_BLOCK(block,flow,node,index)

	Boolean changed, contains_side;
	int new_assign;

	contains_side = changed = false;
	new_assign = 0;
	curr_flow_index = index;

	DEBUG(aflags[cp].level&2,
		 ("Assigns avail before replace: %s\n",bv_sprint(avail)));
	DEBUG(aflags[cp].level&4,("Checking in block(%d):\n",
		block->block_count));
	DEBUG_COND(aflags[cp].level&4,print_expr(node->opt));
	if(CONTAINS_SIDE_EFFECTS(node)) {
		contains_side = true;
		switch(node->op) {
		case ASSIGN:
				/*
				** If valid copy, need to add to avail
				** later.
				*/
			if(OPTABLE_FOR_CP(node->left) && node->opt->flags&IS_COPY)
				new_assign = node->opt->setid;
			/* FALLTHRU */
		case CALL:
			if(CONTAINS_SIDE_EFFECTS(node->right))
				kill_assigns(node->right,avail);
			break;
		default:
			kill_assigns(node, avail);
			break;
		}
	}
	node = find_and_replace_copies(0, node, &changed);
	if(changed) {
		node = op_optim(node);
			/*
			** Must reset pointer in cgq element. Since
			** we are in a FOR_ALL_ND1 loop, HAS_ND1 is true
			** so call to CGQ_ND1 is legal.
			*/

		CGQ_ND1(flow) = node;
		if (node->op == JUMP)
			flow->cgq_op = CGQ_EXPR_ND2;
		else
			new_expr(node, block);	
		DEBUG(aflags[cp].level&1,
			("New node from copy prop in block(%d):\n",
			block->block_count));
		DEBUG_COND(aflags[cp].level&1,print_expr(node->opt));
		if(IS_CONST_COPY_NODE(node)) {
			if( (node->opt->flags&IS_COPY) == 0 ) {
				if(++last_assign < ASSIGN_ARRAY_SIZE) {
					node->opt->setid = last_assign;
					assign_array[last_assign] = node->opt;
					node->opt->flags |= IS_COPY;
					new_assign = last_assign;
				}
			}
			else
				new_assign = node->opt->setid;
		}
	}
	if(contains_side)
		kill_assigns(node,avail);
	if(new_assign != 0) 
		bv_set_bit(new_assign, avail);

	END_FOR_ALL_ND1_IN_BLOCK
			
	END_DEPTH_FIRST

	arena_term(copy_arena);
}

static void
init_copy_prop(blocks)
Block blocks;
{
	
Block block;
	copy_arena = arena_init();
	assign_array = Arena_alloc(copy_arena, ASSIGN_ARRAY_SIZE, Expr);
	last_assign = 0;
	bv_scratch = Expr_set_alloc(copy_arena);

	kills = Expr_set_alloc(copy_arena);
	avail = Assign_set_alloc(copy_arena);

		/*
		** Next call should be removed if and when we decide
		** how to keep track of scope information.
		*/
	order_cg_q(global_arena, blocks);

	for(block = blocks; block; block = block->next) {
		init_block(block);
		collect_local_info(block);
      	}
		/*
		** We need another pass through the blocks, now
		** that we know all assignments in the function.
		*/

	DEPTH_FIRST(blk_ptr)

	Block block = *blk_ptr;
	int assign;
	bv_init(false, bv_scratch);

	for(assign = 1; assign <= last_assign; assign++) {
		if(kills_assign(assign, block->exprs_killed))
			bv_set_bit(assign, bv_scratch);
	}
	block->assigns_killed = bvclone(copy_arena, bv_scratch);

	END_DEPTH_FIRST
}

static ND1 *
find_and_replace_copies(context, node, changed)
int context;
ND1 *node;
Boolean *changed; /* Pass by ref */
{
	if(OPTABLE_FOR_CP(node) && node->opt->flags & OBJECT) {

		BV_FOR(avail,assign)

			/*
			** If there is an available assignment
			** whose left hand side matches this
			** node and whose right hand side is
			** in scope, substitute the rhs.
			** If we only allow OBJECTS on rhs
			** we can simplify checking for IN SCOPE.
			*/

		if(IS_RVAL(context)
			&& node->opt == LEFT_INFO(assign_array[assign])) {
			Expr exp;
			if(in_scope(exp = RIGHT_INFO(assign_array[assign]),
				curr_flow_index)) {
				T1WORD type = node->type;

START_OPT
				if(!(exp->flags & OBJECT_PLUS_CONST)
					|| context & SIBLING_IS_CONST){ 
DEBUG(aflags[cp].level&2,("Old node before replace:\n"));
DEBUG_COND(aflags[cp].level&2,print_expr(node->opt));
DEBUG((aflags[ep].level&1) && (exp->flags & OBJECT_PLUS_CONST),
	("\tpropagate simple expr in context\n"));
					node = expr_to_nd1(exp);
					node->type = type;
					*changed = true;
					new_expr(node, curr_block);	
					/* May need to call get_context -- PSP */
					return find_and_replace_copies(context,
						node, changed);
				}
STOP_OPT
			}
		}
#ifndef NODBG
		else if(node->opt->object->setid ==
			assign_array[assign]->object->setid) {
			int debug_level = 1;
			DEBUG(aflags[cp].level&debug_level,
				("MISSED ONE +=+=+=+=+=+=+=+\n"));
			DEBUG(aflags[cp].level&debug_level,
				("EXPR:\n"));
			DEBUG_COND(aflags[cp].level&debug_level,
				print_expr(node->opt));
			DEBUG(aflags[cp].level&debug_level,
				("NODE:\n"));
			DEBUG_COND(aflags[cp].level&debug_level,
				tr_e1print(node,"X"));
			DEBUG(aflags[cp].level&debug_level,
				("LEFT_INFO(assign):\n"));
			DEBUG_COND(aflags[cp].level&debug_level,
				print_expr(LEFT_INFO(assign_array[assign])));
		}
#endif

		END_BV_FOR
	}

	if(node->left) {
		node->left =
			find_and_replace_copies(get_context(node, LEFT, context),
				node->left, changed);
	}

	if(node->right) {
		node->right =
			find_and_replace_copies(get_context(node, RIGHT, context),
				node->right, changed);
	}

	return(node);
}


	/*
	** Remove from assign_set all assignments killed by node.
	*/
static void
kill_assigns(node, assign_set)
ND1 *node;
Assign_set assign_set;
{
	bv_init(false,kills);
	get_recursive_kills(kills,node,ACCUMULATE);
	DEBUG_COND(aflags[cp].level&8,print_expr(node->opt));
	DEBUG(aflags[cp].level&8,
		("node_exprs_killed: %s\n",bv_sprint(kills)));

	BV_FOR(assign_set, assign)
		if(kills_assign(assign, kills))
			bv_clear_bit(assign, assign_set);
	END_BV_FOR

	DEBUG(aflags[cp].level&2,
		("assigns after kills_assigns: %s\n",bv_sprint(assign_set)));
}

	/*
	** Following routine should prolly be moved into local_info.c
	** and used to init blocks for any optimization. -- PSP
	*/
static void
init_block(block)
Block block;
{
	block->avin = BVZERO;

	block->reaches_exit = Assign_set_alloc(copy_arena);
	bv_init(false,block->reaches_exit);

	block->assigns_killed = BVZERO;

			/* Need to think about how to allocate exprs_killed: */
	block->exprs_killed = Expr_set_alloc(copy_arena);
	bv_init(false,block->exprs_killed);
}

	/*
	** Add assignments to global assign info.  Calculate
	** all assignments which reach block exit into reaches_exit.
	** Compute expressions killed by this node into exprs_killed.
	*/
static void
collect_local_info(block)
Block block;
{
	bv_init(false,block->exprs_killed);
	FOR_ALL_ND1_IN_BLOCK(block,flow,node,index)

	if(CONTAINS_SIDE_EFFECTS(node)) {
		bv_init(false,kills);
		get_recursive_kills(kills,node,ACCUMULATE);
		bv_or_eq(block->exprs_killed,kills);

		/* May pay to implement reaches_exit as an ordered linked list */
		BV_FOR(block->reaches_exit, assign)
			if(kills_assign(assign,kills)) 
			bv_clear_bit(assign,block->reaches_exit);
		END_BV_FOR
	
		if( is_valid_copy_node(node,kills) ) {

			if( (node->opt->flags&IS_COPY) == 0 ) { /* new assign */

				if(++last_assign < ASSIGN_ARRAY_SIZE) {
					node->opt->setid = last_assign;
					assign_array[last_assign] = node->opt;
					node->opt->flags |= IS_COPY;
				}
			}
			bv_set_bit(node->opt->setid, block->reaches_exit);
		}

		DEBUG(aflags[cp].level&8,
			("node_exprs_killed by %s(%d) in flow(%d): %s\n",
				((node->op==ASSIGN)?"assign":"expr"),
				node->opt->setid,
				CGQ_INDEX_NUM(index),
				bv_sprint(kills)));
			DEBUG(aflags[cp].level&8,
				("reaches exit: %s\n*******\n",
				bv_sprint(block->reaches_exit)));
	}

	END_FOR_ALL_ND1_IN_BLOCK
}

#ifndef NODBG
static
copy_prop_debug(blocks)
Block blocks;
{
	Block block;
	int assign;
	fprintf(stderr,"Copy_prop debugging:\n");
	for(assign = 1; assign <= last_assign; assign++) {
		print_expr(assign_array[assign]);
	}
	for(block=blocks; block; block=block->next) {
		fprintf(stderr,"block(%d) <><><><><><><><><><><><><><>\n",
			block->block_count);

		fprintf(stderr,"\treaches_exit: ");
		bv_print(block->reaches_exit);

		fprintf(stderr,"\tavin: ");
		bv_print(block->avin);

		fprintf(stderr,"\tassigns_killed: ");
		bv_print(block->assigns_killed);
	}
}

static Boolean
kills_assign(assign,kills)
int assign;
Expr_set kills;
{
	Expr exp, ass;
	int bit;
	ass = assign_array[assign];
	exp = LEFT_INFO(ass);
	bit = exp->setid;	
	DEBUG(!bit,("Bad node:\n"));
	DEBUG_COND(!bit,print_expr(exp));
	if(bv_belongs(bit,kills))
		return 1;
	exp = RIGHT_INFO(ass);
	bit = exp->setid;	
	DEBUG(!bit,("Bad node:\n"));
	DEBUG_COND(!bit,print_expr(exp));
	if(bv_belongs(bit,kills))
		return 1;
	return 0;
}
#endif

int cg_q_in_scope(); /* From scopes.c */
	/*
	** Predicate to check if "expr" is in scope at the program
	** point "flow".  To avoid recursion, we could enter scope
	** info in arbitrary exprs, rather than just objects.
	** This function is supposed to return true on any tree
	** all of whose objects are in scope.
	*/
static Boolean
in_scope(expr, index)
Expr expr;
Cgq_index index;
{
	if(!expr)
		return true;
	if(expr->node.op == ICON)
		return true;
	if(expr->node.op == FCON)
		return true;
	if(expr->flags&OBJECT) {
		return cg_q_in_scope(index, expr->object);
	}
	return(
		in_scope(LEFT_INFO(expr), index)
		&&
		in_scope(RIGHT_INFO(expr), index)
	);
}
