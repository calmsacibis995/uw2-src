/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)amigo:common/blocks.c	1.47"
#include "amigo.h"
#include "l_unroll.h"
#include <memory.h>
#include "scopes.h"
#include <unistd.h>

#define PREV_IS_PRED 1
#define PREV_NOT_PRED 0

struct Branch_descriptor {
	int target_label;	/* target of this branch */
	Block block;		/* block in which branch is found */
	ND1* tree;
};

static Block remember_prev;

static branch_count, max_label, min_label;
		/*
		** label_table: ( dynamic ) table indexed by label-min_label;
		** branch: table of branches, to be sorted on 
		*/
void label_table_alloc();
void label_table_zero();
struct Label_descriptor *label_table;
#define LABEL_INDEX(label) ((label) - min_label)

int
get_branch_count()
{
	return branch_count;
}

int
get_max_label()
{
	return max_label;
}

int get_min_label()
{
	return min_label;
}

void
init_branch_count()
{
	branch_count = 0;
}

void
init_max_label()
{
	max_label = 0;
}

void
init_min_label()
{
	min_label = 0;
}

void
update_max_label(lbl)
int lbl;
{
	if(lbl > max_label)
		 max_label = lbl;
}

void
update_min_label(lbl)
int lbl;
{
	if(min_label == 0 || lbl < min_label)
		min_label = lbl;
}

void
update_branch_count()
{
	branch_count++;
}

struct Label_descriptor *
get_label_descriptor(label)
int label;
{
	return label_table + LABEL_INDEX(label);
}

DEBUG_UNCOND(static void print_depth_first();)

struct Block_list *
init_block_list(block)
struct Block *block;
{
	struct Block_list *list=Arena_alloc(GLOBAL,1,struct Block_list);
	list->block=block;
	list->next=0;
	return list;
}

static void
add_block(list,block)
struct Block_list **list;
struct Block *block;
{
	struct Block_list *new=Arena_alloc(GLOBAL,1,struct Block_list);
	new->block = block;
	new->next = *list;
	*list = new;

}

static int block_count;

Block
new_block_alloc()
{
	Block new;
	++block_count;
	new = Arena_alloc(GLOBAL,1,struct Block);
	(void)memset((char *)new, 0, sizeof(struct Block));
	new->block_count = block_count;
	new->first = new->last = CGQ_NULL_INDEX;
	return new;
}

static Block
new_block(prev,prev_is_pred,flowindex)
struct Block *prev;	/* lexically previous block */
int prev_is_pred;	/* set if prev is a predecessor of this block */
Cgq_index flowindex;	/* first flownode of this block */
{
	struct Block *curr;
	if (flowindex == CGQ_NULL_INDEX)
		return 0;
	else
		curr = new_block_alloc();

	if (prev) {
		prev->next = curr;
		remember_prev = prev;
	}
	else
		remember_prev = 0;

	if (prev_is_pred) {
		curr->pred = init_block_list(prev);
		prev->succ = init_block_list(curr);
	}
	else
		curr->pred = 0;

	curr->first = curr->last = flowindex;
	return curr;
}


static void resolve(), remove_jump_to_jump();
struct Branch_descriptor *curr_branch, *branch_table;

#ifndef NODBG
static void
debug_tables()
{
	static count=0;
	int lbl; 
	struct Branch_descriptor *br;
	DPRINTF("\n============== %d\n",count);
	DPRINTF("max_label%d, min_label%d",max_label,min_label);

	for(lbl=min_label;lbl <= max_label; lbl++) 
		DPRINTF("lbl %d: map_label %d; block %x\n",
			lbl, label_table[LABEL_INDEX(lbl)].map_label,
			label_table[LABEL_INDEX(lbl)].block);

	for(br=branch_table; br < curr_branch; br++) 
		DPRINTF("branch %x, branch target %d, block %x\n",
			br, br->target_label, br->block);
	DPRINTF("============== %d\n",count);
	count++;
}
#endif

static void
enter_branch(str,label, block, node)
char *str;
int label;
Block block;
ND1 *node;
{
	DEBUG(aflags[jj].level&2, ("\tenter_branch called from %s, target %d\n",str,label));
	curr_branch->target_label = label;
	curr_branch->block = block;
	curr_branch->tree = node;
	++curr_branch;
	DEBUG_COND(aflags[jj].level&8, debug_tables());
}


#define IS_RETREATING_EDGE(b1, b2)	(b1->dfn > b2->dfn)

static int *block_weight;

static void 
block_flow(blocks, arena)
Block blocks;
Arena arena;
{
	struct Block *b;

	block_weight = Arena_alloc(arena, block_count+1, int);

	/* Initialize blocks that are not really in the flow graph */
	for (b=blocks; b; b=b->next) 
		block_weight[b->block_count] = 0;

	DEPTH_FIRST(blk_ptr)

		Block b = *blk_ptr;
		struct Block_list *pred = b->pred;
		
		block_weight[b->block_count] = pred ? 0 : BLOCK_WEIGHT_INIT;
		while (pred) {
			Block pb = pred->block;
			Block_list succ;
			int add;
			int forward_edges;
			if (!IS_RETREATING_EDGE(b, pb)) {
				pred = pred->next;
				continue;
			}
			forward_edges = 0;
			for (succ=pb->succ; succ; succ=succ->next) 
				if (IS_RETREATING_EDGE(succ->block, pb))
					forward_edges++;
			assert(forward_edges);
			add = block_weight[pb->block_count] / forward_edges;
			if (add == 0)
				add = 1;
			block_weight[b->block_count] += add;
			pred = pred->next;
		}
	END_DEPTH_FIRST
}

static int 
loop_has_guard(loop)
Loop loop;
{
	ND1 *node;
	Cgq_index start = loop->loop_unroll->loop_start;
	Cgq_index body = loop->loop_unroll->loop_body;
	CGQ_FOR_ALL_BETWEEN(flow, index, start, body)

		if ((node = HAS_ND1(flow)) != NULL)
			return node->op == CBRANCH;

	CGQ_END_FOR_ALL_BETWEEN
	return 0;
}

static int
loop_nesting(block)
Block block;
{
        Loop loop = block->loop;
	int weight = 1;
        while (loop){
		if (loop_has_guard(loop))
			weight *= 2 * LOOP_COUNT;
		else
			weight *= LOOP_COUNT;

		/* If the loop was unrolled, it won't execute as
		** many times as before, but it may have a guard 
		** that loop_has_guard() could not find.  In fact,
		** it may have had two guards.
		*/
		if (LOOP_FLAGS(loop) & UNROLLED)
			weight /= 2;
                if (weight > MAX_WEIGHT) return MAX_WEIGHT;
                loop = loop->parent;
        }
        return weight;
}

void
get_flow_weights(blocks, arena)
Block blocks;
Arena arena;
{
	block_flow(blocks, arena);
	for (; blocks; blocks=blocks->next) {
		block_weight[blocks->block_count] *= loop_nesting(blocks);
		DEBUG(aflags[ra].level, ("block %d flow = %d\n", 
			blocks->block_count, block_weight[blocks->block_count]));
	}
}

int
get_block_weight(block)
Block block;
{
	return block_weight[block->block_count];
}

static Block f_end_block, f_end_block_pred; /* block containing db_e_fcode */

Block
get_f_end_block()
{
	return f_end_block;
}

Block
get_f_end_block_pred()
{
	return f_end_block_pred;
}

void
enter_f_end_block_pred(block)
Block block;
{
	f_end_block_pred = block;
}

static void
enter_f_end_block(block)
Block block;
{
	f_end_block = block;
	enter_f_end_block_pred(remember_prev);
}

Block
basic_block(has_loop) 
Boolean *has_loop;
{
	Block curr_block, first_block;
	int curly_level = 0;

	int last_label_seen; /* Used to determine if a JUMP is a potential
			   target of another JUMP or branch. */
	Arena local_arena = arena_init();
	Cgq_index last_index = 0;

	block_count = 0;
	if(get_branch_count()) 
		curr_branch = branch_table =
			Arena_alloc(local_arena,get_branch_count(),struct Branch_descriptor);
	label_table_alloc(local_arena);
	label_table_zero();

	curr_block = first_block =
		new_block((struct Block *)0,PREV_NOT_PRED,CGQ_FIRST_INDEX);

	last_label_seen = 0;


	CGQ_FOR_ALL(root,index)
			/*
			** Need a flag to control the updating of last_index.
			*/
		Boolean index_deleted = false;
		if (HAS_ND1(root)) {
			curr_block->block_flags |= BL_HAS_EXPR;
				/*
				** No ND1 is a JUMP so any branch to 
				** last_label_seen is not a branch to a jump.
				*/
			last_label_seen = 0;
		}

		if (root->cgq_op == CGQ_CALL_INT || root->cgq_op == CGQ_CALL ||
		    root->cgq_op == CGQ_CALL_SID) {
			/*
			** Why do we have cgq_op and cgq_func anyway??
			*/
			if (root->cgq_func == os_loop 
				&& root->cgq_op == CGQ_CALL_INT) {
				*has_loop = true;
				switch(root->cgq_arg.cgq_int) {
				case OI_LSTART:
					curr_block->block_flags |= BL_PREHEADER;
					/* FALLTHRU */
				case OI_LBODY:
					/* set HAS_EXPR here, so we don't have
					   to update this flag if exprs are
					   added to the loop header -- no harm
					   if flag is falsely set
					*/
					curr_block->block_flags |= BL_HAS_EXPR;
					/* FALLTHRU */
				case OI_LEND:
				case OI_LCOND:
					curr_block->block_flags |= BL_HAS_OS_LOOP;
					break;
				default:
					break;
				}
			}	
			/* if cg_setlocctr not at beginning of current block
			   then start a new block
			*/
			else if (root->cgq_func == cg_setlocctr
				&& curr_block->first != index 
				&& root->cgq_op == CGQ_CALL) {

				curr_block->last = last_index;
				curr_block = new_block(curr_block, 
				    PREV_IS_PRED, index);
			}
			else if(root->cgq_func == db_s_block 
				&& root->cgq_op == CGQ_CALL) {

				DEBUG_UNCOND( if(aflags[ls].xflag) )
				if(curly_level++ > 0) {
					if(curr_block->first == index)
						curr_block->first =
							CGQ_ELEM_OF_INDEX(index)->cgq_next;
					if(curr_block->last == index)
						curr_block->last =
							CGQ_ELEM_OF_INDEX(index)->cgq_next;
					cg_q_delete(index);
					index_deleted = true;
				}

			}
			else if(root->cgq_func == db_e_block 
				&& root->cgq_op == CGQ_CALL) {

				DEBUG_UNCOND( if(aflags[ls].xflag) )
				if(--curly_level > 0) {
					if(curr_block->last == index)
						curr_block->last =
							CGQ_ELEM_OF_INDEX(index)->cgq_next;
					if(curr_block->first == index)
						curr_block->first =
							CGQ_ELEM_OF_INDEX(index)->cgq_next;
					cg_q_delete(index);
					index_deleted = true;
				}
			}
			else if( root->cgq_func == db_e_fcode
				 && root->cgq_op == CGQ_CALL) {
#ifndef NODBG
				if(aflags[ls].xflag)
#endif
					enter_f_end_block(curr_block);
			}
			else if((root->cgq_func == db_symbol || root->cgq_func == db_sy_clear)
				&& root->cgq_op == CGQ_CALL_SID &&
				curly_level != 1) {
				if(! ( root->cgq_func == db_sy_clear && TY_ISFTN(SY_TYPE(root->cgq_arg.cgq_sid))))
					curr_block->block_flags |= BL_HAS_LOCAL;
			}
		}

		else if (root->cgq_op == CGQ_EXPR_ND1 || 
		    root->cgq_op == CGQ_EXPR_ND2) 
		    switch(root->cgq_arg.cgq_nd2->op) {
		    int fallthru;
		case SWEND:
			if(root->cgq_arg.cgq_nd2->lval != -1) {/* default case*/
			enter_branch("SWEND",root->cgq_arg.cgq_nd2->lval,curr_block,
				(ND1 *)root->cgq_arg.cgq_nd2);
			fallthru = PREV_NOT_PRED;/* no fallthru from SWEND */
			}
			else /* no default case */
				/* fallthru if no case is taken */
				fallthru = PREV_IS_PRED;

			curr_block->last = index;
			curr_block = new_block(curr_block,fallthru,
				CGQ_NEXT_INDEX(root));
			break;
		case SWCASE: /* don't start a new block */
			enter_branch("SWCASE",root->cgq_arg.cgq_nd2->label,curr_block,
				(ND1 *)root->cgq_arg.cgq_nd2);
			break;
		case JUMP:
			enter_branch("JUMP",root->cgq_arg.cgq_nd2->label,
				curr_block, (ND1 *)root->cgq_arg.cgq_nd2);

			curr_block->last = index;
			curr_block = new_block(curr_block,PREV_NOT_PRED,
				CGQ_NEXT_INDEX(root));
			if(last_label_seen){
					/*
					** Any branch to here is a jump to
					** jump. 
					*/
				label_table[LABEL_INDEX(last_label_seen)].map_label =
					root->cgq_arg.cgq_nd2->label;
				last_label_seen = 0;	
			}
			break;
		case CBRANCH:
			enter_branch("CBRANCH",
				root->cgq_arg.cgq_nd1->right->lval,
				curr_block,
				root->cgq_arg.cgq_nd1);
			curr_block->last = index;
			curr_block = new_block(curr_block,PREV_IS_PRED,
				CGQ_NEXT_INDEX(root));
			break;
		case LABELOP:
			/* if LABELOP is not beginning of current block
			   and not the second item in current block following
			   a cg_setlocctr, then begin a new block
			*/
			if (curr_block->first != index  &&
			    !(last_index == curr_block->first &&
			    CGQ_ELEM_OF_INDEX(last_index)->cgq_op == CGQ_CALL_INT &&
			    CGQ_ELEM_OF_INDEX(last_index)->cgq_func == cg_setlocctr)) {
				curr_block->last = last_index;
				curr_block = new_block(curr_block, 
				PREV_IS_PRED, index);
			}
			last_label_seen = root->cgq_arg.cgq_nd2->label;
			label_table[LABEL_INDEX(last_label_seen)].block = curr_block;
			break;
		case RETURN:
			curr_block->last=index;
			curr_block = new_block(curr_block,PREV_NOT_PRED,
				CGQ_NEXT_INDEX(root));
			break;
		}
		if(! index_deleted)
			last_index = index;
	CGQ_END_FOR_ALL

	DEBUG_COND(aflags[jj].level&2, debug_tables());

	curr_block->last = last_index;

	if (branch_count) {
		remove_jump_to_jump(branch_table,curr_branch);
		resolve(branch_table,curr_branch);
	}
	CGQ_ELEM_OF_INDEX(first_block->first)->cgq_prev = CGQ_NULL_INDEX;
	arena_term(local_arena);
	(void)order_blocks(first_block);
	return first_block;
}

	/*
	** Go down the block list in lexical order to set
	** the lexical number.
	*/
int
order_blocks(blocks)
Block blocks;
{
	Block blk;
	int count;
	count = 0;
	for(blk = blocks; blk; blk = blk->next) {
		blk->lex_block_num = ++count;
	}
	return count;
}

void
label_table_alloc(arena)
Arena arena;
{
	label_table = Arena_alloc(arena, max_label-min_label+1,
		struct Label_descriptor);
}

void
label_table_zero()
{
	if (max_label) {
				/*
				** Labels are stored in label_table  at offset
				** label-min_label.  E.g., if min_label == 6,
				** max_label == 9, use slots 0,1,2,3.
				*/	
		(void)memset((char *)label_table,0,
			(max_label-min_label+1)*sizeof(struct Label_descriptor));
	}
}


static int df_num = 0;
static void visit();
static int *visited; /* array of flags */
static Block *dfb;	/* array of blocks in depth first ordering */

#ifndef NODBG
static void
print_depth_first()
{
	int i;
	DPRINTF("depth first order: ( ");
	for(i = df_num; i<= block_count; i++)
		DPRINTF("%d ",dfb[i]->block_count);	
	DPRINTF(")\n");
}
#endif

int
get_block_count() { return block_count; }

Block *
get_first_depth_first_block_ptr() { return &dfb[df_num]; } 

Block *
get_last_depth_first_block_ptr() { return &dfb[get_block_count()]; } 


	/* Sort the blocks in depth-first order, i.e., shallower before
	** deeper.  See Aho,Sethi & Ullman, p 662.
	*/
void
init_depth_first(first_block)
Block first_block;
{
		/* allow 1-indexing for arrays */
	visited = Arena_alloc(GLOBAL, (block_count + 1), int);
	dfb = Arena_alloc(GLOBAL, (block_count + 1), Block);
	/* dfb = (Block *)malloc(sizeof(Block) * (block_count + 1)); */
	if(!(visited && dfb)) Amigo_fatal(gettxt(":568","Malloc failed"));
		/* mark all blocks 'not visited' */
	(void)memset((char *)visited, 0, sizeof(int) * (block_count + 1));
	(void)memset((char *)dfb, 0, sizeof(Block) * (block_count + 1));
	df_num = block_count;
	visit(first_block);
	df_num++; /* Will be 1 if no dead blocks */
	DEBUG_COND(aflags[df].level&4, print_depth_first());
	DEBUG(aflags[df].level&4, ("dead block count: %d\n",df_num-1));
}

static void remove_from_flow_graph();

void
remove_unreachable(first_block)
Block first_block;
{
	Block curr;
	if (df_num == 1) /* no unreachable blocks */
		return;

	/* delete unreachable code */
	for (curr = first_block; curr;  curr = curr->next) {
		Cgq_index before, after, index, new_index;

		if (visited[curr->block_count]) 
			continue;
START_OPT
		DEBUG(aflags[un].level&1,("block=%d(%d-%d) unreachable\n",
			curr->block_count,
			CGQ_INDEX_NUM(curr->first),
			CGQ_INDEX_NUM(curr->last)));

		/* since we never delete the first block (it is reachable)
		   the before entry will not be null, since it is only
		   null for the first block
		*/
		before=CGQ_PREV_INDEX(CGQ_ELEM_OF_INDEX(curr->first));
		after= CGQ_NEXT_INDEX(CGQ_ELEM_OF_INDEX(curr->last));

		/* delete entries in the basic block */
		for (index=curr->first; index != after; index = new_index) {
			cgq_t *elem = CGQ_ELEM_OF_INDEX(index);
			new_index = CGQ_NEXT_INDEX(elem);

			if (elem->cgq_op == CGQ_EXPR_ND1 ||

				( elem->cgq_op == CGQ_EXPR_ND2 &&
					( elem->cgq_arg.cgq_nd2->op == JUMP
#ifdef DBLINE
					|| elem->cgq_arg.cgq_nd2->op == DBLINE 
#endif
					|| elem->cgq_arg.cgq_nd2->op == SWCASE
					)
				) 

				|| (elem->cgq_op == CGQ_CALL && 
				elem->cgq_func == db_lineno)
			) {
				DEBUG(aflags[un].level&2,
					("Deleting cgq item %d\n",
					CGQ_INDEX_NUM(index)));
				amigo_delete(curr, index);
			}
		}

		/* if after == CGQ_NULL_INDEX then curr is the last block
		   in the program, since we will not delete the last entry
		   in the last block (it is a call to al_e_fcode), we need
		   not change "last"
		*/
		if (after != CGQ_NULL_INDEX)
			curr->last=CGQ_PREV_INDEX(CGQ_ELEM_OF_INDEX(after));

		curr->first=CGQ_NEXT_INDEX(CGQ_ELEM_OF_INDEX(before));
		/* if entire block has been deleted, "first" will be the
		   successor of "last" which marks block as empty to
		   CGQ_FOR_ALL_BETWEEN
		*/
		curr->block_flags &= ~BL_HAS_EXPR;
		remove_from_flow_graph(curr);
STOP_OPT
	}
}

static void
remove_from_flow_graph(block)
Block block;
{
	/*
	** Remove this block from each successor's predecessor list.
	** Then empty the successor list.
	** 
	** Remove this block from each predecessor's successor list.
	** Then empty the predecessor list.
	*/
	Block_list bl;
	for(bl = block->succ; bl; bl = bl->next) {
		Block_list b, last = 0;
		for(b = bl->block->pred; b; last = b, b = b->next) {
			if(b->block == block) { /* found it */
				if(last) 
					last->next = b->next;
				else
					bl->block->pred = b->next;
				DEBUG(aflags[un].level&2,
					("Removing block %d from pred list of block %d\n",
					block->block_count,
					bl->block->block_count));
				break;	
			}
		}
	}

	block->succ = 0;

	for(bl = block->pred; bl; bl = bl->next) {
		Block_list b, last = 0;
		for(b = bl->block->succ; b; last = b, b = b->next) {
			if(b->block == block) { /* found it */
				if(last) 
					last->next = b->next;
				else
					bl->block->succ = b->next;
				DEBUG(aflags[un].level&2,
					("Removing block %d from succ list of block %d\n",
					block->block_count,
					bl->block->block_count));
				break;	
			}
		}
	}

	block->pred = 0;
}

static void
visit(bl)
Block bl;
{
	Block_list child;
	visited[bl->block_count] = true;
	for(child = bl->succ; child; child = child->next) {
		if(!visited[child->block->block_count]) visit(child->block);
	}
	bl->dfn = df_num;
	dfb[df_num--] = bl;
}

static
bcomp(first,second) 
const myVOID *first, *second;
{
	return ((struct Branch_descriptor *)first)->target_label - 
		((struct Branch_descriptor *)second)->target_label;
}

static void 
remove_jump_to_jump(branch, branch_stop)
struct Branch_descriptor branch[], *branch_stop;
{
	DEBUG_UNCOND( { int jump_removed = 0;)
	for (; branch < branch_stop; ++branch) {
		int label_index = LABEL_INDEX(branch->target_label);

		if(label_index < 0 || label_index > LABEL_INDEX(max_label) 
			|| label_table[label_index].block == 0) {
			assert(label_index >= 0);
			assert(label_index <= LABEL_INDEX(max_label));
			DEBUG_UNCOND(pr_label_table(min_label,max_label);)
			assert(label_table[label_index].block != 0);
			Amigo_fatal("label_index < 0 \
|| label_index > LABEL_INDEX(max_label) \
||label_table[label_index].block == 0)");
		}
		if(label_table[label_index].map_label) {
			ND1 * tr = branch->tree;
				/*
				** If this "branch" is really a branch,
				** and not a SWEND or SWCASE, then see
				** if it's a branch to a JUMP
				*/
			if(tr->op == CBRANCH) { 
				tr->right->lval = branch->target_label =
					label_table[label_index].map_label;
				new_expr(tr,0); /* It changed, rehash */
				DEBUG_UNCOND(jump_removed++;)
			}
			else if(tr->op == JUMP) {
					((ND2 *)tr)->label =
						branch->target_label =
						label_table[label_index].map_label;
					DEBUG_UNCOND(jump_removed++;)
			}
		}
	}
	DEBUG(aflags[jj].level, ("Jump to jumps removed: %d\n",jump_removed));
	DEBUG_UNCOND(})
}

static void 
resolve (branch, branch_stop)
struct Branch_descriptor branch[], *branch_stop;
{
		/* sort branch table on target labels */
	qsort((char *)branch,(unsigned)(branch_stop-branch),sizeof(*branch), bcomp);

		/* Complete flow graph info to account for branches */
	for (; branch < branch_stop; ++branch) {
		int label_index = LABEL_INDEX(branch->target_label);

		if (label_index < 0 || label_index > LABEL_INDEX(max_label) ||
			label_table[label_index].block == 0)
			Amigo_fatal("unresolved_branch");

		add_block(&branch->block->succ,label_table[label_index].block);
		add_block(&label_table[label_index].block->pred,branch->block);
	}
}

#ifndef NODBG
void 
pr_block(block) struct Block *block;
{
	fprintf(stderr,
		"block=%d lex_block_num=%d first=%d last=%d next=%d loop=%d scope=%d block_flags=%d\n",
		block->block_count,
		block->lex_block_num,
		CGQ_INDEX_NUM(block->first),
		CGQ_INDEX_NUM(block->last),
		block->next ? block->next->block_count : 0,
		block->loop ? block->loop->id : 0,
		block->scope,
		block->block_flags
		);
	pr_block_list(block->pred,"\n\tpreds");
	pr_block_list(block->succ," succs");
	fprintf(stderr, "\n\tkills: ");
	pr_object_list(block->kills);
      	fprintf(stderr, "block ind vars: ");
      	pr_object_set(block->ind_vars);
	fflush(stderr);
}

void 
pr_blocks(block)
struct Block *block;
{
	for (; block; block = block->next)
		pr_block(block);

	fprintf(stderr,"End_block is: %d\n", get_f_end_block()->block_count);
	
}

void 
pr_block_list (list,msg)
struct Block_list *list;
char *msg;
{
	fprintf(stderr," %s ( ", msg);
	for (; list ; list = list->next)
		fprintf(stderr,"%d ", list->block->block_count);
	fputs(")",stderr);

}

void
pr_label_table(from, to)
int from, to;
{
	int i;
	fprintf(stderr,"%s%s%d%s%d%s",
		"\n\t()()()()()()^^^^",
		"LABEL_TABLE (min_label: ",
		from,
		" max_label: ",
		to,
		"^^^^()()()()()()\n");
	fprintf(stderr,"%s%s%s",
		"\n\t()()()()()()^^^^",
		"label map_label block"
		,"^^^^()()()()()()\n");
	for(i = from; i <= to; i++) {
		int index =  LABEL_INDEX(i);
		fprintf(stderr, "\t%d\t%d\t%d\n",
			i,
			label_table[index].map_label,
			label_table[index].block ?
				label_table[index].block->block_count : -1);
	}
}
#endif
