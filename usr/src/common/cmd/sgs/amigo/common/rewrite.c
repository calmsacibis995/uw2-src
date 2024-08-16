/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)amigo:common/rewrite.c	1.3"

/* Perform amigo-specific tree rewrites as opposed to tree rewrites
** done in op_optim().  
*/

#include <amigo.h>

static void pre_recurse(), post_recurse(), rewrite_mul(),
	rewrite_div();

static ND1 *canonicalize();

void
pre_amigo_rewrite()
{
	CGQ_FOR_ALL(flow, index)

		ND1 *node = HAS_ND1(flow);
		if (node) 
			pre_recurse(node);

	CGQ_END_FOR_ALL
}

void
post_amigo_rewrite()
{
	CGQ_FOR_ALL(flow, index)

		ND1 *node = HAS_ND1(flow);
		if (node) 
			post_recurse(node);

	CGQ_END_FOR_ALL
}

static void
pre_recurse(node)
ND1 *node;
{
	switch (node->op) {
	case DIV:
	case ASG DIV:
		 rewrite_div(node);
	}

	switch(optype(node->op)) {
	case BITYPE:
		pre_recurse(node->right);
		/* FALLTHRU */
	case UTYPE:
		pre_recurse(node->left);
	}
}

static void
post_recurse(node)
ND1 *node;
{
	switch (node->op) {
	case MUL:
	case ASG MUL:
		 rewrite_mul(node);
	}

	switch(optype(node->op)) {
	case BITYPE:
		post_recurse(node->right);
		/* FALLTHRU */
	case UTYPE:
		post_recurse(node->left);
	}
}

#define FCON1(n) (n->op == FCON && FP_CMPX(n->xval, FP_ATOF("1.0")) == 0)

/* The following routine rewrites expressions of the form
** a / b to a * 1/b.  We hope to get a cse on the 1/b.
** This is a kind of strength reduction from division to
** multiplation.  If it doesn't work out, we will fold
** the multiplication later.
**
** To avoid obscurities, we will insist that the operator and operands
** all have the same type.
*/
static void
rewrite_div(node)
ND1 *node;
{
	if(TY_ISFPTYPE(node->type)
	 && node->type == node->left->type
	 && node->type == node->right->type
	 && !FCON1(node->left)
	 && !ieee_fp()
	) {
		ND1 *fcon = t1alloc();
		ND1 *div = t1alloc();

		fcon->op = FCON;
		fcon->type = node->type;
		fcon->xval = FP_ATOF("1.0");
		fcon->flags = 0;

		div->op = DIV;
		div->type = node->type;
		div->left = fcon;
		div->right = node->right;
		div->flags = 0;

		node->op += MUL - DIV;
		node->right = div;
	}
}

/* The following routine rewrites expressions of the form
** a * 1/b to a/b.  This undoes the wasted work of rewrite_div().
*/
static void
rewrite_mul(node)
ND1 *node;
{
	if (TY_ISFPTYPE(node->type)
	 && node->type == node->left->type
	 && node->type == node->right->type
	 && node->right->op == DIV
	 && FCON1(node->right->left)
	 && !ieee_fp()
	) {
		ND1 *r = node->right->right;
		nfree(node->right->left);
		nfree(node->right);
		node->right = r;
		node->op += DIV - MUL;
	}
}

	/*
	** AMIGO creates trees that op_optim does not always
	** know how to deal with.  amigo_op_optim() is an
	** interface which tries to remedy this problem.
	*/
ND1 *
amigo_op_optim(node)
ND1 *node;
{
	node = canonicalize(node);
	return op_optim(node);
}

static ND1 *
canonicalize(node)
ND1 *node;
{
	int op = node->op;
		/*
		** Put pointers on the left where op_optim 
		** expects them.
		*/
	if (op == PLUS && TY_ISPTR(node->right->type)) {
		ND1 *tmp = node->left;
		node->left = node->right;
		node->right = tmp;
	}
	switch(optype(op)) {
	case BITYPE:	node->right = canonicalize(node->right);
			/* FALLTHRU */
	case UTYPE:	node->left = canonicalize(node->left);
	}
	node->flags &= ~FF_WASOPT;
	return node;
}
