/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)optim:i386/loops.c	1.44.1.12"

#include <stdio.h>
#include <values.h>
#include <pfmt.h>
#include <unistd.h>
#include "sched.h"
#include "optutil.h"
#include "fp_timings.h"
#include "regal.h"
#ifdef DEBUG
#define  STDERR	stderr
#define  FPRINST(P) fprinst(P)
#define	loop_of()	{ fprintf(STDERR,"loop of "); FPRINST(first); }
#endif

#define ALL_THE_LOOP(P)	P = first; P != last->forw; P = P->forw

extern NODE *prepend();	/* imull.c */
extern char *itoreg();	/* ebboptim.c */
extern int isregal();	/* ebboptim.c */
extern live_at();		/* ebboptim.c */
extern void rm_dead_insts(); /* peep.c */
extern NODE *next_fp();	/* peep.c */

int zflag = false; /*debugging flag*/
static int label_counter = 0; /*for generating labels*/
static NODE *first, *last; /*first and last insts. in the loop */
static BLOCK *first_block;

#define regs   ( EAX | EBX | ECX | EDX | ESI | EDI | EBI | ESP | EBP )
#define working_regs   ( EAX | EBX | ECX | EDX | ESI | EDI | EBI )
#define scratch_regs	( EAX | ECX | EDX )

#define LOOP_ENTRY 1
#define LOOP_EXIT  2
#define ERROR     -1
#define MAX_CAND	100
/* three defs from regal.c */
#define MAXLDEPTH	10
#define MAXWEIGHT	(MAXINT - 1000)
#define WEIGHT	8

#define i_eax 0
#define i_edx 1
#define i_ecx 2
#define i_ebx 3
#define i_esi 4
#define i_edi 5
#define i_ebi 6

#define first_reg 0
#define last_reg 6

struct m { unsigned int op;
			char *opc;
			} static moves[] = { { MOVB , "movb" } , 
						  { MOVW , "movw" } ,
						  { 0    , NULL   } ,  /* placeholder */
						  { MOVL , "movl" } };

typedef struct one_r {
		unsigned int reg;
		char *names[4]; /* three names and place holder to make it oplength */
		int status;
		int save_at;
	} one_reg;

/* possible values of status */

#define LIVE	0
#define FREE	1
#define NOT_USED	2

static one_reg all_regs[] = {
		/*   reg    name[0]  name[1]       name[3]   status  save_at  */
			{ EAX , { "%al" , "%ax" , NULL, "%eax" } , FREE , 0 } ,
			{ EDX , { "%dl" , "%dx" , NULL, "%edx" } , FREE , 0 } ,
			{ ECX , { "%cl" , "%cx" , NULL, "%ecx" } , FREE , 0 } ,
			{ EBX , { "%bl" , "%bx" , NULL, "%ebx" } , FREE , 0 } ,
			{ ESI , { NULL  , "%si" , NULL, "%esi" } , FREE , 0 } ,
			{ EDI , { NULL  , "%di" , NULL, "%edi" } , FREE , 0 } ,
			{ EBI , { NULL  , "%bi" , NULL, "%ebi" } , FREE , 0 } ,
		};
/* types of icands, values assigned to -> what */
#define NONE 0
#define IMMEDIATE 1
#define ON_STACK 2
#define GLOBAL_VAR 3

typedef struct icand_s {
		int what;           /* is it stack, global or immediate */
		char *name;			/* the char string, for any type. */
		int ebp_offset;		/* only for regals */
		int value;			/* only for immediates */
		int estim;			/* estimated payoff */
		boolean in_byte_inst; /* does it appear with oplength == 1 */
		int reg_assigned;	/* which reg is assigned to the cand */
		boolean changed;	/* to restore after the loop or not */
		int length;			/* oplength of the restore */
	} i_cand;

static i_cand all_icands[MAX_CAND];
static i_cand sorted_icands[MAX_CAND];


static i_cand null_icand = { NONE , NULL , 0 , 0 , 0 , 0 , -1 , 0 , 0 };

/*is p a conditional jump backwards*/
/*return the jump target if it is, NULL otherwise*/
static NODE *
is_back_jmp(p) NODE *p;
{
NODE *q;
	if (!iscbr(p)) return NULL;
	for (q = p; q != &n0; q = q->back) /* search for label backwards */
		if (islabel(q) && !strcmp(p->op1,q->opcode))
			return q;
	return NULL;
}/*end is_back_jmp*/

NODE *
find_last_jmp(given_jmp) NODE *given_jmp;
{
NODE *p;
	for (p = ntail.back; p != given_jmp->back; p = p->back)
		if (iscbr(p) && !strcmp(p->op1,given_jmp->op1))
			return p;
	fatal(gettxt(":418","last jump: not found even it'self\n"));
	/* NOTREACHED */
}/*end find_last_jmp*/

/*Find what registers are free in the loop.
**Mark the free and nonfree ones in all_regs[].
**return number of free regs.
*/
static unsigned int
find_available_regs_in_loop()
{
extern int suppress_enter_leave;
int i;
NODE *p;
unsigned int live_in_loop = 0;
unsigned int used_in_loop = 0;
unsigned int available_regs;
unsigned int saveable_regs;
int free_regs = 0;
	if (suppress_enter_leave) {
		live_in_loop |= EBI;
		used_in_loop |= EBI;
		all_regs[i_ebi].status = LIVE;
	}
	for (ALL_THE_LOOP(p)) {
		live_in_loop |= (p->nlive | p->sets | p->uses);
		used_in_loop |= (p->sets | p->uses);
	}/*for loop*/
	live_in_loop &= working_regs;
	used_in_loop &= working_regs;
	available_regs = working_regs & ~live_in_loop;
	saveable_regs =  working_regs & (live_in_loop & ~used_in_loop);
	for (i = first_reg; i <= last_reg; i++) {
		if ((available_regs & all_regs[i].reg) == all_regs[i].reg) {
			all_regs[i].status = FREE;
			free_regs++;
		} else if ((saveable_regs & all_regs[i].reg) == all_regs[i].reg) {
			all_regs[i].status = NOT_USED;
			free_regs++;
		} else
			all_regs[i].status = LIVE;
	}
	return free_regs;
}/* end find_available_regs_in_loop*/

/*If the original memory operand is used as an eight bit operand than it
**is not replaceable with neither one of the three registers edi, esi or
**ebp. These registers are the last ones to be chosen, therefore no other
**register is available.
*/
static boolean
reg_8_probs(entry) int entry;
{
i_cand *rgl = &sorted_icands[entry];
char *operand = rgl->name;
NODE *p;
	for (ALL_THE_LOOP(p)) {
		if (OpLength(p) == ByTE) {
			if ((p->op1 && !strcmp(operand,p->op1))
				|| (p->op2 && !strcmp(operand,p->op2)))
				return true;
		}
	}
	return false;
}/*end reg_8_probs*/


/*A loop is considered simple if it has only one entery point,
**and either only one exit (by fallthrough), or some jmps, all
**to the same target.
**Three possible return values: 0 for non optimizable, 1 if only
**one exit, 2 if there are "legal" jumps out of the loop.
**in case of several jumps to one target, which is out of the loop,
**return via jtarget, the name of the label.
**A bug in this test: the jumps may be to the same block but to 
**some different but succesive labels. Currently this will return 0.
*/
static int /*not boolean*/
is_simple_loop(jtarget) char **jtarget;
{
extern SWITCH_TBL *get_base_label();
NODE *p,*q;
boolean label_found = false;
boolean first_lbl = true;
char *target = NULL;
SWITCH_TBL *sw;
REF *r;
BLOCK *b;
	/*Special case: if there is a wild jump in the function, of the form
	**jmp	*%eax ; then no loop is simple.
	*/
	for (ALLN(p)) {
		if (p->op == JMP && *(p->op1) == '*' 
		 && !(p->op1[1] == '.' || isalpha(p->op1[1]) || p->op1[1] == '_')) {
			return 0;
		}
	}
	/*first test that jump from inside the loop stay inside */
	/*in particular, if there is a switch in the loop, verify that
	**all of the targets are inside the loop.
	*/
	for (ALL_THE_LOOP(p)) {
		if (is_jmp_ind(p)) {
			sw = get_base_label(p->op1);
			for (r = sw->first_ref ; r != sw->last_ref; r = r->nextref) {
				b = r->switch_entry;
				label_found = false;
				for (ALL_THE_LOOP(q)) {
					if (q == b->firstn) label_found = true;
				}
				if (!label_found) return false;
			}
		} else if (is_any_br(p) && p->op1) {
			label_found = false;
			for (ALL_THE_LOOP(q)) {
				if (is_any_label(q) && !strcmp(p->op1,q->opcode)) {
					label_found = true;
					break;
				}
			}
			if (!label_found) { /*the jmp target is ! inside the loop*/
				if (first_lbl) {
					first_lbl = false;
					target = p->op1; /*save the first such jmp target*/
				} else { /*if this target != first target, it's ! simple*/
					if (strcmp(target,p->op1)) {
						return 0;
					}
				}
			}
		}
	}
	/*second test that jumps from before the loop dont enter*/
	for (p = n0.forw; p != first; p = p->forw) {
		if (is_any_br(p) && p->op1) {
			label_found = false;
			for (ALL_THE_LOOP(q)) {
				if (is_any_label(q) && !strcmp(p->op1,q->opcode)) {
					return 0;
				}
			}
		}
	}
	/*third test that jumps from after the loop dont enter*/
	for (p = last->forw; p != &ntail; p = p->forw) {
		if (is_any_br(p) && p->op1) {
			label_found = false;
			for (ALL_THE_LOOP(q)) {
				if (is_any_label(q) && !strcmp(p->op1,q->opcode)) {
					return 0;
				}
			}
		}
	}
	*jtarget = target;
	return target == NULL ? 1 : 2;
}/*end is_simple_loop*/

/*level is 0 for an innermost loops, one more for each level of nesting.
**use it to save compile time, since it is not reasonable to believe
**that many outer loops will have free registers.
**currently not activated since the above is false.
*/
static int
find_level_of_loop()
{
NODE *p;
int level = 0;
	for (ALL_THE_LOOP(p))
		if (is_back_jmp(p))
			level++;
#ifdef DEBUG
	if (zflag > 1)
		fprintf(STDERR,"loops inside this loop: %d\n",level);
#endif
	return level;
}/*end find_level_of_loop*/

/*The memory reference in pivot tries to disambiguate by seems_same() from
**any other memory reference between first and last. keep your fingers crossed.
**seems_same assumes that it's second parameter is earlier than it's first one.
**Before that test that the registers used by pivot are not set anywhere in
**the loop.
*/
static boolean
disambiged(pivot) NODE *pivot;
{
NODE *q;
char *operand;
char *rival;
unsigned int regset = 0;
boolean retval = true; /* cant return from middle of the function */
	/*find which operand of pivot is the memory reference. point with opernd.*/
	if (!isreg(pivot->op1) && !isconst(pivot->op1)) operand = pivot->op1;
	else operand = pivot->op2;
	if (*operand == '*') ++operand;

	/* if (scanreg(operand,false) == EBP) return true; just kidding */

	/* If this is a read only data item, it disambigs from anything */
	if (is_read_only(operand)) return true;

	/*does pivot index thru a register which is changed inside the loop? */
	for (ALL_THE_LOOP(q)) {
		regset |= q->sets;
	}
	regset &= regs;
	if (scanreg(operand,false) & regset) {
		return false;
	}

	/* add memory info to the NODEs */
	for (ALL_THE_LOOP(q)) { /*add memory knowledge*/
		q->uses |= muses(q);
		q->sets = msets(q);
		q->idus = indexing(q);
	}/*for loop*/
	/* do from first to pivot */
	for (q = first; q != pivot; q = q->forw) {
		if ((q->sets | q->uses) & MEM) {
			if (!q->op1) rival = "";
			else if (q->op1[0] != '*') rival = q->op1;
			else rival = q->op1+1;
			if (!strcmp(rival,operand)) continue;
			if (!q->op2) rival = "";
			else if (q->op2[0] != '*') rival = q->op2;
			else rival = q->op2+1;
			if (!strcmp(rival,operand)) continue;
			if (seems_same(pivot,q)) {
				retval = false;
				break;
			}
		}
	}
	/* do from pivot to last */
	if (retval) {
		for (q = pivot->forw; q != last->forw; q = q->forw) {
			if ((q->sets | q->uses) & MEM) {
				if (!q->op1) rival = "";
				else if (q->op1[0] != '*') rival = q->op1;
				else rival = q->op1+1;
				if (!strcmp(rival,operand)) continue;
				if (!q->op2) rival = "";
				else if (q->op2[0] != '*') rival = q->op2;
				else rival = q->op2+1;
				if (!strcmp(rival,operand)) continue;
				if (seems_same(q,pivot)) {
					retval = false;
					break;
				}
			}
		}
	}
	/* remove memory info from NODEs */
	for (ALL_THE_LOOP(q)) {
		q->uses &= ~MEM;
		q->sets &= ~MEM;
	}
	return retval;
}/*end disambiged*/

/*operand is taken from a NODE. base is either all_icands or sorted_icands.
**find the structure of the operand in base[] and return a pointer to it.
**I wanted to look for immediates by value but there is a problem: if there
**is a search for $0, it will find a global variable with name != NULL and
**value == 0 and return it. must check for strcmp with "$0".
**do it only for $0, save the time for other values.
*/
static i_cand*
lookup_icand(operand,base) char *operand; i_cand *base;
{
i_cand *scan;
int i;
int what;
int x;
	if (!operand) return NULL;
	if (isimmed(operand)) what = IMMEDIATE;
	else if (scanreg(operand,false) & EBP) what = ON_STACK;
	else what = GLOBAL_VAR;

	switch (what) {
		case ON_STACK:
			x = *operand == '*' ? atoi(operand+1) : atoi(operand);
			for (i = 0; i < MAX_CAND; i++) {
				scan = &base[i];
				if (what == scan->what && scan->name != NULL &&
					x == scan->ebp_offset)
					return scan;
			}
			break;
		case IMMEDIATE:
			x = atoi(1+operand);
			for (i = 0; i < MAX_CAND; i++) {
				scan = &base[i];
				if (what == scan->what && isimmed(scan->name) &&
					x == scan->value)
					return scan;
			}
			break;
		case GLOBAL_VAR:
			x = 0;
			for (i = 0; i < MAX_CAND; i++) {
				scan = &base[i];
				if (what == scan->what && scan->name &&
					!strcmp(operand,scan->name))
					return scan;
			}
			break;
	}
	return NULL;
}/*end lookup_icand*/

/*function returns the number of cycles gained by using a register instead
**of memory in the given instruction.
**In a mov instruction there is no direct gain, but it is probable that
**the instruction will be deleted. It's gain should be half, rether than 1.
*/
static int 
pay_off(p) NODE *p;
{
	switch (p->op) {
		case ADDB: case ADDW: case ADDL: case SUBB: case SUBW: case SUBL:
		case ANDB: case ANDW: case ANDL: case ORB: case ORW: case ORL:
		case XORB: case XORW: case XORL:
				return msets(p) & MEM ? 2 : 1;
		case NEGB: case NEGW: case NEGL: case NOTB: case NOTW: case NOTL:
		case DECB: case DECW: case DECL: case INCB: case INCW: case INCL:
				return 2;
		case RCLB: case RCLW: case RCLL:
		case RCRB: case RCRW: case RCRL:
		case ROLB: case ROLW: case ROLL:
		case RORB: case RORW: case RORL:
		case SALB: case SALW: case SALL:
		case SARB: case SARW: case SARL:
		case SHLB: case SHLW: case SHLL:
		case SHRB: case SHRW: case SHRL:
				if (setreg(p->op1) == CL) return 1; else return 2;
		case CMPB: case CMPW: case CMPL:
		case TESTB: case TESTW: case TESTL:
				return 1;
		case POPW: case POPL:
				return 2;
		case PUSHW: case PUSHL:
				return 3;
		case XCHGB: case XCHGW: case XCHGL:
				return 1;
		case IDIVB: case IDIVW: case IDIVL:
				return 1;
		case MULB:
				return 5;
		case MULW:
				return 13;
		case MULL:
				return 29;
		case MOVB: case MOVW: case MOVL:
				return 1;
		case DIVB: case DIVW: case DIVL:
		case IMULB: case IMULW: case IMULL:
		case MOVSBW: case MOVSBL: case MOVSWL:
		case MOVZBW: case MOVZBL: case MOVZWL:
		case CALL:
				return 0;
		default: return ERROR;
		}
		/* NOTREACHED */
}/*end pay_off*/

static void
estim()
{
int i;
long weight =1;
NODE *p;
long int new_estim;
int payoff;
i_cand *rgl;
int depth = 0;

	for (ALL_THE_LOOP(p)) {
		if (p->usage == LOOP_ENTRY) {	/* Adjust weight for loops. */
			depth++;
			if (depth <= MAXLDEPTH) weight *= WEIGHT;
		}
		if(p->usage == LOOP_EXIT) {		/*  Decrease weight for loop exit.	*/
			depth--;
			if (depth <= MAXLDEPTH) weight /= WEIGHT;
		}

		for (i = 1; i < MAXOPS + 1; i++) {
			if ((p->ops[i] == NULL) ||
				(rgl = lookup_icand(p->ops[i],all_icands)) == NULL)       
				continue;		/* Just examine regals */
			
			if (isimmed(rgl->name)) { /* calc payoff for an imm */
				if (isshift(p) || p->op == IMULL || (p->sets & ESP)) payoff = 0;
				else payoff = hasdisplacement(p->op2) ? 1 : 0;
			}
			else payoff = pay_off(p);
			new_estim = rgl->estim;

			if(((MAXWEIGHT - new_estim) / weight) < payoff)
				new_estim = MAXWEIGHT;
			else
				new_estim += (weight * payoff);
			rgl->estim = new_estim;
		}/*for all ops[]*/
	}/*for all nodes*/
}/*end estim*/

/*This function checks whether the given operand participates in an unexpected
**instruction.
*/
static int
is_illegal_icand(pivot,idx) NODE *pivot; int idx;
{
NODE *p;
char *operand = pivot->ops[idx];
	for (ALL_THE_LOOP(p)) {
		if ((p->op1 && !strcmp(p->op1,operand)) ||
			(p->op2 && !strcmp(p->op2,operand))) {
			if (pay_off(p) == ERROR) return true;
		}
	}
	return false;
}/*end is_illegal_icand*/

static int last_icand = 0;
static void
no_icands()
{
int i;
	for (i = 0; i < MAX_CAND; i++) {
		all_icands[i] = null_icand;
		sorted_icands[i] = null_icand;
	}
	last_icand = 0;
	for (i = first_reg; i <= last_reg ; i++) {
		all_regs[i].status = FREE;
		all_regs[i].save_at = 0;
	}
}/*no_icands*/

int n_cand = 0;

static boolean
add_icand(p,m) NODE *p; int m;
{
int i;
int x = 0;
int y = 0;
int what = NONE;
	if (!p->ops[m]) return false;
	if (isimmed(p->ops[m])) {  /* an immediate operand */
		y = strtol(1+p->ops[m],NULL,0);
		what = IMMEDIATE;
		for (i = 0; i < n_cand; i++)
			if (all_icands[i].what == IMMEDIATE && y == all_icands[i].value)
				return false;
	} else if (scanreg(p->ops[m],false) == EBP) { /* on stack */
		x = p->ops[m][0] == '*' ? atoi(p->ops[m]+1) : atoi(p->ops[m]);
		what = ON_STACK;
		for (i = 0; i < n_cand; i++)
			if (all_icands[i].what == ON_STACK && 
				x == all_icands[i].ebp_offset)
				return false;
	} else {  /* a global (static) variable */
		what = GLOBAL_VAR;
		for (i = 0; i < n_cand; i++)
			if (all_icands[i].what == GLOBAL_VAR &&
				!strcmp(p->ops[m],all_icands[i].name))
				return false;
	}
	/*didn't find the candidate, install it */
	all_icands[last_icand].name = p->ops[m];
	all_icands[last_icand].ebp_offset = x;
	if (what == IMMEDIATE) all_icands[last_icand].value = y;
	all_icands[last_icand].what = what;
	last_icand++;
	return true;
}/*end add_icand*/
static int
find_icands()
{
NODE *p;
int i;
i_cand *c;
	n_cand = 0;
	for (ALL_THE_LOOP(p)) {
		for (i = 1; i <= 3; i++) { /* go thru operands of p */
			if (n_cand >= MAX_CAND) return n_cand;
			if (p->ops[i] == NULL) continue;
			if (isreg(p->ops[i]) || (isconst(p->ops[i]) && !isimmed(p->ops[i])))
				continue;
			if (isvolatile(p,i)) continue;
			if (is_illegal_icand(p,i)) continue;
			if (scanreg(p->ops[i],false) == EBP && isregal(atoi(p->ops[i]))) {
				if (add_icand(p,i)) ++n_cand;
			} else if (isimmed(p->ops[i])) {
				if (add_icand(p,i)) ++n_cand;
			} else if (disambiged(p)) {
				if (add_icand(p,i)) ++n_cand;
			}
		}/*for all ops of p*/
	}/*for all p in loop*/
	/* It may be the case that an operand disambiggs from all others in the loop
	** the first time it appears, but not in the second instruction it appears.
	** Therefore we disambig the candidate in the next appearences and remove
	** from the candidates array if it does not disambig in any of the
	** instructions.
	** The reason not to disambig in the second instruction is different lengths
	** of the different instructions.
	*/
	for (ALL_THE_LOOP(p)) {
		for (i = 1; i <= 3; i++) { /* go thru operands of p */
			if ((c = lookup_icand(p->ops[i],all_icands)) != NULL) {
				if (isconst(p->ops[i]) || is_read_only(p->ops[i])) continue;
				if (scanreg(p->ops[i],0) == EBP && isregal(atoi(p->ops[i])))
					continue;
				if (!disambiged(p)) {
					*c = null_icand;
					--n_cand;
				}
			}
		}
	}
	return n_cand;
}/*end find_icands*/

static int
sort_icands (ncands,nregs) int ncands, nregs;
{
int max_estim;
i_cand *rgl = NULL;
int i;
int last_sorted = 0;
	for ( i = 0; i < MAX_CAND; i++) {
		if (isimmed(all_icands[i].name) && all_icands[i].estim == 0) {
			all_icands[i] = null_icand;
			ncands--;
		}
	}
	nregs = ncands = MIN(ncands,nregs); /* this will be the return value */
	while (ncands > 0) {
		max_estim = -1;
		for (i = 0; i < MAX_CAND; i++) { /* find best cand */
			if (all_icands[i].name && all_icands[i].estim > max_estim) {
				rgl = &all_icands[i];
				max_estim = rgl->estim;
			}
		}
		sorted_icands[last_sorted] = *rgl;
		*rgl = null_icand; /* dont find it again */
		++last_sorted;
		ncands--;
	}
	return nregs;
}/*end sort_icands*/

static int
anal_changes_to_icands()
{
NODE *p;
int i;
int len;
int changed_ones = 0;
	for (ALL_THE_LOOP(p)) {
		/* pushl x sets momory, but not location x...*/
		if (msets(p) & MEM && p->op != PUSHL && p->op != PUSHW) {
			for (i = 0; i < MAX_CAND; i++) {
				if (isimmed(sorted_icands[i].name)) continue;
				if (is_read_only(sorted_icands[i].name)) continue;
				if (sorted_icands[i].name &&
					((p->op1 && !strcmp(p->op1,sorted_icands[i].name)) ||
					(p->op2 && !strcmp(p->op2,sorted_icands[i].name)))) {
					sorted_icands[i].changed = true;
					changed_ones++;
					if ((len = OpLength(p)) > sorted_icands[i].length) {
						sorted_icands[i].length = len;
						if (len >= 4) break;
					}
				}
			}
		}
	}
	return changed_ones;
}/*end anal_changes_to_icands*/

/* return value is the number of registers to be saved before the loop
** and restored after it.
*/
static int
assign_regals_2_regs(ncands) int ncands;
{
int i,j;
int x = 0;
unsigned int regs2save = 0;
	/* go thru sorted cands and assign only FREE registers. */
	for (i = 0; i < ncands; i++) {
		if (sorted_icands[i].name == NULL) 
			fatal("assign regals 2 regs, internal error\n");
		sorted_icands[i].in_byte_inst = reg_8_probs(i);
		for (j = first_reg; j <= last_reg; j++) {
			if (all_regs[j].status != FREE) continue;
			if (sorted_icands[i].in_byte_inst && !all_regs[j].names[0])
				continue;
			all_regs[j].status = LIVE;
			sorted_icands[i].reg_assigned = j;
			x++;
			break; /* dont assign another reg to this regal */
		}
	}
	if (x == ncands) { /* all cands were assigned to FREE registers */
		return 0;
	}
	/* make a second pass on the cands and assign NOT_USED registers */
	for (i = 0; i < ncands; i++) {
		if (sorted_icands[i].name == NULL) 
			fatal("assign regals 2 regs, internal error\n");
		if (sorted_icands[i].reg_assigned != -1) continue;
		for (j = first_reg; j <= last_reg; j++) {
			if (all_regs[j].status != NOT_USED) continue;
			if (sorted_icands[i].in_byte_inst && !all_regs[j].names[0])
				continue;
			all_regs[j].status = LIVE;
			sorted_icands[i].reg_assigned = j;
			regs2save |= all_regs[j].reg;
			break; /* dont assign another reg to this regal */
		}
	}
	return regs2save;
}/*end assign_regals_2_regs*/

static BLOCK *
find_block_of_jtarget(jtarget) char *jtarget;
{
BLOCK *b,*target_block;
NODE *p;
boolean found_label = false;
	if (!jtarget) return NULL;
	for (b = b0.next; b; b = b->next) {
		p = b->firstn;
		while (is_any_label(p)) {
			if (!strcmp(jtarget,p->opcode)) {
				found_label = true;
				target_block = b;
				break;
			}
			p = p->forw;
		}
		if (found_label) break;
	}
	return target_block;
}/*end find_block_of_jtarget*/

static void
mark_regs2save(regs2save) unsigned int regs2save;
{
int i;
int n_regs = 0;
int save_at;
	/* count how many registers to save */
	for (i = first_reg; i <= last_reg; i++) {
		if (all_regs[i].reg & regs2save) n_regs += 4;
	}
	save_at = -inc_numauto(n_regs); /* make place on the stack */
	/* assign save places to registers */
	for (i = first_reg; i <= last_reg; i++) {
		if (all_regs[i].reg & regs2save) {
			all_regs[i].save_at = save_at;
			save_at -= 4;
		}
	}
}/*end mark_regs2save*/

static void
save_restore(regs2save) unsigned int regs2save;
{
int i;
NODE *prenew = first;
NODE *postnew = last;
	for (i = first_reg; i <= last_reg; i++) {
		if (all_regs[i].reg & regs2save) {
			prenew = prepend(prenew,NULL);
			chgop(prenew,MOVL,"movl");
			prenew->op1 = all_regs[i].names[3];
			prenew->op2 = getspace(NEWSIZE);
			sprintf(prenew->op2,"%d(%%ebp)",all_regs[i].save_at);
			prenew->sets = 0;
			prenew->uses = EBP | all_regs[i].reg;
			addi(postnew,MOVL,"movl",prenew->op2,prenew->op1);
			postnew->sets = all_regs[i].reg;
			postnew->uses = EBP;
		}
	}
}/*end save_restore*/

static void
insert_moves(jtarget,regs2save,chcands)
char *jtarget; unsigned int regs2save; int chcands;
{
NODE *p = first;
NODE *q = last;
BLOCK *target_block;
int i,entry;
char *new_label,*tmp;
int len;
	if (regs2save == 0 && chcands == 0) jtarget = NULL; /* no break treatment*/
	if (regs2save) mark_regs2save(regs2save);
	if (jtarget) {
		/* construct the jumps and new label before jtarget */
		target_block = find_block_of_jtarget(jtarget);
		new_label = getspace(LABELSIZE);
		sprintf(new_label,".R%d",++label_counter);
		p = prepend(target_block->firstn,NULL);
		chgop(p,LABEL,new_label);
		p->uses = p->sets = 0;
		p = prepend(p,NULL);
		chgop(p,JMP,"jmp");
		p->op1 = jtarget;
		p->uses = p->sets = 0;
		p = p->forw; /* point back to the new label */
		/* place the move instructions between new label and jtarget */
		for (i = 0; sorted_icands[i].name ; i++) {
			entry = sorted_icands[i].reg_assigned;
			if (entry != -1 && sorted_icands[i].changed) {
				len = sorted_icands[i].length -1;
				addi(p,moves[len].op,moves[len].opc,
					all_regs[entry].names[len],sorted_icands[i].name);
				if (*(p->op2) == '*')
					p->op2++;
				p->uses = setreg(p->op1) | scanreg(p->op2,false);
				p->sets = 0;
			}
		}
		/* place the restore instructions after them (by using the same p) */
		if (regs2save) {
			for (i = first_reg; i <= last_reg; i++) {
				if (all_regs[i].reg & regs2save) {
					tmp = getspace(NEWSIZE);
					sprintf(tmp,"%d(%%ebp)",all_regs[i].save_at);
					len =3;/*full reg is always saved; see save_restore() for reference */ 
					addi(p,moves[len].op,moves[len].opc,tmp,all_regs[i].names[len]);
					p->uses = EBP;
					p->sets = all_regs[i].reg;
				}
			}
		}
		/* change referenes to jtarget to reference the new label */
		for (ALL_THE_LOOP(p)) {
			if (is_any_br(p) && !strcmp(p->op1,jtarget)) {
				p->op1 = new_label;
			}
		}
	}/* endif jtarget */
	save_restore(regs2save);
	/* place moves before and after the loop */
	p = first;
	for (i = 0; sorted_icands[i].name ; i++) {
		entry = sorted_icands[i].reg_assigned;
		if (entry != -1) {
			len = sorted_icands[i].length -1;
			if (len == -1) len = 3;
			p = prepend(p,NULL);
			chgop(p,moves[len].op,moves[len].opc);
			p->op1 = sorted_icands[i].name;
			if (*p->op1 == '*')
				p->op1++;
			p->op2 = all_regs[entry].names[len];
			p->uses = scanreg(p->op1,false);
			p->sets = setreg(p->op2);
			if (sorted_icands[i].changed) {
				len = sorted_icands[i].length -1;
				addi(q,moves[len].op,moves[len].opc,all_regs[entry].names[len],
					sorted_icands[i].name);
				if (*q->op2 == '*')
					q->op2++;
				q->uses = setreg(q->op1) | scanreg(q->op2,false);
				q->sets = 0;
			}
		}
	}
}/*end insert_moves*/

static void
replace_regals_w_regs()
{
NODE *p;
int i;
i_cand *rgl;
int entry;
one_reg *i_reg;
	for (ALL_THE_LOOP(p)) {
		for (i = 1 ; i < 3; i++) {
			if ((rgl = lookup_icand(p->ops[i],sorted_icands)) != NULL) {
				if ((isshift(p) || p->op == IMULL || (p->sets & ESP))
					&& isimmed(rgl->name))
					continue;
				entry = rgl->reg_assigned;
				if (entry != -1) {
					i_reg = &all_regs[entry];
					if (p->ops[i][0] == '*') {
						p->ops[i] = getspace(6);
						p->ops[i][0] = '*';
						strcpy(&p->ops[i][1],i_reg->names[OpLength(p) -1]);
					}
				  	else
						p->ops[i] = i_reg->names[OpLength(p) -1];
					new_sets_uses(p);
				}
			}
		}
	}
}/*end replace_regals_w_regs*/

#ifdef DEBUG
void
show_icands(base) i_cand *base;
{
int i;
	if (base == all_icands) fprintf(STDERR,"all ");
	else fprintf(STDERR,"sorted ");
	fprintf(STDERR,"loop of "); FPRINST(first);
	for (i = 0; i < MAX_CAND; i++) {
		if (base[i].name != NULL) {
			fprintf(STDERR,"at %d cand %s with %d \n",i,base[i].name,
			base[i].estim);
		}
	}
}/*end show_icands*/
#endif

/*driver. find the loop, test if it is optimizable and do it.*/
void
loop_regal()
{
NODE *pm;
char *jtarget = NULL;
int nfree_regs;
int n_icands = 0;
unsigned int regs2save;
boolean do_regal; /* a flag to disable */
int changed_cands;
	COND_RETURN("loop_regal");
	ldanal(); /*count on it in find dead regs in the loop*/
	rm_dead_insts(); /* setting of dead regs are dangerous to find_free_reg */
	for (ALLN(pm)) { /* prepare for mark entry end exit of all loops */
		pm->usage = 0;
	}
	for (ALLN(pm)) { /* mark entry end exit of all loops */
		if (iscbr(pm) && lookup_regals(pm->op1,first_label)) {
			if ((first = is_back_jmp(pm)) != NULL) {
				last = find_last_jmp(pm);
				first->usage = LOOP_ENTRY;
				last->usage = LOOP_EXIT;
			}
		}
	}
	for (ALLN(pm)) {
		jtarget = NULL;
		do_regal = true;
		if (iscbr(pm) && lookup_regals(pm->op1,first_label)) {
			if ((first = is_back_jmp(pm)) == NULL) {
#ifdef DEBUG
				if (zflag > 1) {
					fprintf(STDERR,"not a back jmp");
					FPRINST(last);
				}
#endif
				continue;
			}
			last = find_last_jmp(pm);
#ifdef DEBUG
			if (zflag) loop_of();
#endif
			if (is_simple_loop(&jtarget) == 0) {
#ifdef DEBUG
				if (zflag) {
					fprintf(STDERR,"loop not simple: ");
					FPRINST(first);
				}
#endif
				continue;
			}
			no_icands();
			if ((nfree_regs = find_available_regs_in_loop()) == 0) {
#ifdef DEBUG
				if (zflag) {
					fprintf(STDERR,"no free regs in loop.\n");
				}
#endif
				do_regal = false;
			} else if ((n_icands = find_icands()) == 0) {
#ifdef DEBUG
			if (zflag > 1) {
				fprintf(STDERR,"didnt find candidates for ");
				FPRINST(first);
			}
#endif
				do_regal = false;
			}
			if (do_regal) {
#ifdef DEBUG
				if (zflag) {
					fprintf(STDERR,"do something ");
					FPRINST(first);
				}
#endif
				COND_SKIP(continue,"loop %d %s\n",second_idx,first->opcode,0);
				estim();
				n_icands = sort_icands(n_icands,nfree_regs);
				if (n_icands == 0) continue;
				regs2save = assign_regals_2_regs(n_icands);
				changed_cands = anal_changes_to_icands();
				insert_moves(jtarget,regs2save,changed_cands);
				replace_regals_w_regs();
				ldanal();
			}
		}/*endif cmp-jcc*/
	}/*end for loop*/
}/*end loop_regal*/

static char *
remove_scale(op) char *op;
{
char *tmp;
char *s,*t;
int length = strlen(op);
		tmp = (char *) getspace(length);
		for (s = op, t = tmp; *s != '('; s++) {
			*t = *s;
			t++;
		}
		*t++ = *s++; /* copy the '(' */
		if (*s == ',') s++;
		while (!isdigit(*s)) {
			*t++ = *s++;
		}
		*--t = ')';
		*++t = (char) 0;
		return tmp;
}/*end remove_scale*/

static void
descale(compare,jtarget) NODE *compare; char *jtarget;
{
NODE *p;
NODE *increment = NULL;
unsigned int pivot_reg;
int scale = 0, shft, tmp_scale;
int m = 0;
boolean found_scale = false;
int old_val;
NODE *tmp;
BLOCK *b,*target_block;
NODE *target_label;
boolean found_label ;
char *new_label;
#ifdef DEBUG
	if (zflag) {
		fprintf(STDERR,"descale ");
		FPRINST(first);
	}
#endif
	/*The compare has to be a compare register to immediate*/
	if (! (isimmed(compare->op1) && isreg(compare->op2))) {
#ifdef DEBUG
		if (zflag) {
			fprintf(STDERR,"not a compare of imm to reg ");
			FPRINST(compare);
		}
#endif
		return;
	}
	else pivot_reg = setreg(compare->op2);
	/*The increment has to be either an inc or an add of a constant, and to
	**the same register as in the compare.
	**It can also be leal $num(reg),reg ; and then it changes here to an add.
	*/
	for (p = compare; p != first; p = p->back) {
		if ((m = 1, p->op == INCL) || (m = 2, p->op == ADDL)) {
			if (isreg(p->ops[m]) && samereg(p->ops[m],compare->op2)) {
				increment = p;
				break;
			}
		}
		if (p->op == LEAL
			&& isreg(p->op2)
			&& (indexing(p) == p->sets)
			&& isdigit(*p->op1)
			&& samereg(p->op2,compare->op2)) {
			char *tmp;
			increment = p;
			chgop(p,ADDL,"addl");
			tmp = getspace(ADDLSIZE);
			sprintf(tmp,"$%d",atoi(p->op1));
			p->op1 = tmp;
			p->sets |= CONCODES;
			break;
		}
	}
	if (!increment) {
#ifdef DEBUG
		if (zflag) {
			fprintf(STDERR,"didnt find increment, return\n");
		}
#endif
		return;
	}
	if (increment->op == ADDL &&
		! (isconst(increment->op1) && isdigit(increment->op1[1]))) {
#ifdef DEBUG
		if (zflag) {
			fprintf(STDERR,"add of not constant ");
			FPRINST(increment);
		}
#endif
		return;
	}
	/*The register has to be an index register with the same scale at all
	**it's appearences but the compare and increment*/
	for (ALL_THE_LOOP(p)) {
		if (p == increment || p == compare) continue;
		if ((uses_but_not_indexing(p) | p->sets) & pivot_reg) {
#ifdef DEBUG
			if (zflag) {
				fprintf(STDERR,"abuse the reg ");
				FPRINST(p);
			}
#endif
			return;
		}
		if (! (indexing(p) & pivot_reg)) continue;
		if (scanreg(p->op1,true) & pivot_reg) {
			m = 1;
		} else if (scanreg(p->op2,true) & pivot_reg) {
			m = 2;
		}
		if (pivot_reg & setreg(strchr(p->ops[m],'(') + 1)) {
#ifdef DEBUG
			if (zflag) {
				fprintf(STDERR,"pivot reg is base ");
				FPRINST(p);
			}
#endif
			return;
		}
		tmp_scale = has_scale(p->ops[m]);
		if (!found_scale) {
			found_scale = true;
			scale = tmp_scale;
		} else {
			if (scale != tmp_scale) {
#ifdef DEBUG
				if (zflag) {
					fprintf(STDERR,"found two different scales.\n");
				}
#endif
				return;
			}
		}
	}/*for loop*/
	if (scale == 0) {
#ifdef DEBUG
		if (zflag) {
			fprintf(STDERR,"scale == 0, return\n");
		}
#endif
		return;
	}
#ifdef DEBUG
		if (zflag) {
			fprintf(STDERR,"do the descale\n");
		}
#endif
	/*Multiply the increment and compare by scale, prepend mul index by scale 
	**and remove the scales*/
	shft = scale == 8 ? 3 : scale == 4 ? 2 : 1;
	tmp = prepend(first,itoreg(pivot_reg));
	chgop(tmp,SHLL,"shll");
	tmp->op1 = getspace(ADDLSIZE);
	sprintf(tmp->op1,"$%d",shft);
	tmp->op2 = itoreg(pivot_reg);
	new_sets_uses(tmp);

	if (increment->op == INCL) {
		chgop(increment,ADDL,"addl");
		increment->op2 = increment->op1;
		old_val = 1;
	} else {
		old_val = atoi(increment->op1+1);
	}
	increment->op1 = getspace(ADDLSIZE);
	sprintf(increment->op1,"$%d",old_val * scale);

	old_val = atoi(compare->op1+1);
	compare->op1 = getspace(ADDLSIZE);
	sprintf(compare->op1,"$%d",old_val * scale);

	for (ALL_THE_LOOP(p)) {
		if (scanreg(p->op1,true) & pivot_reg) {
			m = 1;
		} else if (scanreg(p->op2,true) & pivot_reg) {
			m = 2;
		} else continue;
		p->ops[m] = remove_scale(p->ops[m]);
	}

	target_block = NULL; target_label = NULL;
	found_label = false;
	if (jtarget) {
		new_label = getspace(LABELSIZE);
		sprintf(new_label,".R%d",++label_counter); /*prepare a new label*/
		/*find the block and node which are the target of the jump*/
		for (b = b0.next; b; b = b->next) {
			p = b->firstn;
			while (is_any_label(p)) {
				if (!strcmp(p->opcode,jtarget)) {
					target_block = b;
					target_label = p;
					found_label = true;
					break;
				}
				p = p->forw;
			}
			if (found_label)
				break;
		}/*for loop*/
		/*add some nodes for break treatment */
		p = prepend(target_block->firstn,NULL); /*add a node before jmp target*/
		chgop(p,JMP,"jmp"); /*make it a jmp to target */
		p->op1 = target_label->opcode; /*address the jmp to the label*/
		p->uses = p->sets = 0;

		p = insert(p); /*add a node after the jmp*/
		chgop(p,LABEL,new_label); /*make it the new label*/
		p->uses = p->sets = 0;

		p = insert(p); /*add a node for the move instruction*/
		chgop(p,SARL,"sarl");
		p->op2 = first->back->op2;
		switch(scale) {
			case 2: p->op1 = "$1"; break;
			case 4: p->op1 = "$2"; break;
			case 8: p->op1 = "$3"; break;
		}
		new_sets_uses(p);
		for (ALL_THE_LOOP(p)) {
			if (is_any_br(p) && !strcmp(p->op1,jtarget)) {
				p->op1 = new_label;
			}
		}
	}/*endif jtarget*/

	tmp = insert(last);
	chgop(tmp,SARL,"sarl");
	tmp->op2 = first->back->op2;
	switch(scale) {
		case 2: tmp->op1 = "$1"; break;
		case 4: tmp->op1 = "$2"; break;
		case 8: tmp->op1 = "$3"; break;
	}
	new_sets_uses(tmp);
}/*end descale*/

void
loop_descale()
{
NODE *pm;
char *jtarget = NULL;
	COND_RETURN("loop_descale");
	for (ALLN(pm)) {
		jtarget = NULL;
		last = pm->forw; if (!last || last == &ntail) return;
		if (pm->op == CMPL && iscbr(last) &&
			lookup_regals(last->op1,first_label)) {
			if ((first = is_back_jmp(last)) == NULL) {
#ifdef DEBUG
				if (zflag > 1) {
					fprintf(STDERR,"not a back jmp");
					FPRINST(last);
				}
#endif
				continue;
			}
			if (is_simple_loop(&jtarget) == 0) {
#ifdef DEBUG
				if (zflag) {
					fprintf(STDERR,"loop not simple: ");
					FPRINST(first);
				}
#endif
				continue;
			}
			COND_SKIP(continue,"loop %d %s\n",second_idx,first->opcode,0);
			descale(pm,jtarget);
			if (jtarget) bldgr(false);
		}/*endif cmp-jcc*/
	}/*end for loop*/
}/*end loop_descale*/


/* end of integer optimization, start fp optimization. */

#define SEEN_FST		23		/* Jordan */
#define SEEN_ARIT		34		/* Hakeem */
#define ARIT_B4_FST		11		/* Isea	  */
#define FST_B4_ARIT		32		/* Magic  */
#define MAX_CANDS		80		/* size of the array */

typedef struct cand_s {
			int ebp_offset;  /* -offset(%ebp) */
			char *name;		 /* the whole string as above */
			int type;        /* preloaded or not */
			int estim;		 /* estimation of gain by using from st vs memory */
			int size;        /* size in bytes */
			boolean type_decided; /* temp to help determine the type */
			boolean	needed;  /* is it live after the loop */
			NODE *last_arit; /* for an fstp type, point to the last arit inst*/
			int	entry;       /* current location on the fp stack */
		} candidate;

typedef struct node_list {
		NODE *element;
		struct node_list *next;
		} NODE_LIST;

static NODE_LIST *open_jumps = NULL;
static candidate all_cands[MAX_CANDS];
static candidate sorted_cands[MAX_CANDS];
static candidate null_cand = { 0 , NULL , 0 , 0 , 0 , 0 , 0 , NULL , -1 };
static candidate *null_cand_p = &null_cand;
static int last_cand = 0;
char *st_i[] = { "%st(0)", "%st(1)", "%st(2)", "%st(3)", "%st(4)",
						"%st(5)", "%st(6)", "%st(7)"  };
static int n_candidates; /*number of operands to assign on stack */

/* Is the instruction one that we know how to deal with */
static boolean
is_fp_optimable(p) NODE *p;
{
	return (is_fld(p) || is_fst(p) || is_fp_arit(p));
}/*end is_fp_optimable*/

static void
no_cands(stack) candidate *stack[8];
{
int i;
	for (i = 0; i < MAX_CANDS; i++) {
		all_cands[i] = null_cand;
		sorted_cands[i] = null_cand;
	}
	last_cand = 0;
	open_jumps = NULL;
	for (i = 0; i < 8; i++) stack[i] = NULL;
}/*end no_cands*/

static void
remember_jcc(p) NODE *p;
{
NODE_LIST *e;
	e = (NODE_LIST *) getspace(sizeof(NODE_LIST));
	e->element = p;
	e->next = open_jumps;
	open_jumps = e;
}/*end remember_jcc*/

static void
forget_jcc(p) NODE *p;
{
NODE_LIST *e,*nexte;
char *label = p->opcode;
	if (!open_jumps) return;
	if (!strcmp(open_jumps->element->op1,label)) {
		open_jumps = NULL;
		return;
	}
	for (e = open_jumps; e->next; e = nexte) {
		nexte = e->next;
		if (!strcmp(e->next->element->op1,label))
			e->next = e->next->next;
	}
}/*end forget_jcc*/

/* Add an operand to the array. If it is new, place a cand structure for
** it in the next entry of all_cands, return value is 1. Otherwise return
** value is 0; follow up on the type of the candidate:
** a candidate can be first used, later set, or first set, later used, or
** only set, or only used. If it is only used it will be discarded and not
** placed on the stack. If it is first used, then it's value has to be
** loaded to the stack before the loop. Otherwise it may not have a valid 
** FP value before the loop. So this distinction is important.
** A variable may be set-before-used in one control flow of the loop and
** used-before-set in a second control flow; such variables are removed from
** the candidate set and return valuse is -1;
*/
static int
add_operand(p) NODE *p;
{
int i;
int x;
    x = atoi(p->op1);
	for (i = 0; i < last_cand; i++) {
		if (all_cands[i].ebp_offset == x) {
			if ((open_jumps || !all_cands[i].type_decided)
				&& (((all_cands[i].type == SEEN_FST) && !is_fstp(p)) ||
				   ((all_cands[i].type == SEEN_ARIT) && is_fstp(p)))) {
				all_cands[i] = null_cand;
				return -1;
			}
			switch (all_cands[i].type) {
				  case SEEN_FST:
					if (is_fp_arit(p) || is_fld(p)) {
						all_cands[i].type = FST_B4_ARIT;
					}
					break;
				  case SEEN_ARIT:
				  /* I want an fstp here, not just fst, so something will be
					 left on the stack.
				  */
					if (is_fstp(p)) {
						all_cands[i].type = ARIT_B4_FST;
					}
					break;
				  case FST_B4_ARIT:
				  case ARIT_B4_FST:
									break;
				  default:
					fatal(gettxt(":419","unknown type of candidate\n"));
			}
			all_cands[i].estim += gain_by_mem2st(p->op);
			return 0;
		}
	}
	/* no entry yet for this candidate */
	all_cands[last_cand].ebp_offset = x;
	all_cands[last_cand].name = p->op1;
	/* This is a problem. Currently, a candidate with only fst and no fstp
	   may enter, and I'm not sure I want such a candidate.
	   Big change to the above: now, an fst counts as an arit! and not as
	   an fst. only fstp is not arit.
	*/
	all_cands[last_cand].type = is_fstp(p) ?  SEEN_FST : SEEN_ARIT ;
	all_cands[last_cand].estim = gain_by_mem2st(p->op);
	all_cands[last_cand].size = OpLength(p);
	all_cands[last_cand].needed = live_at(p,last);
	if (!open_jumps) all_cands[last_cand].type_decided = true;
	++last_cand;
	return 1;
}/*end add_operand*/

static boolean
rm_operand(p) NODE *p;
{
int i;
int x;
	if (ismem(p->op1)) x = atoi(p->op1);
	else if (ismem(p->op2)) x = atoi(p->op2);
	else return false;
	for (i = 0; i < last_cand; i++)
		if (all_cands[i].ebp_offset == x) {
			all_cands[i] = null_cand;
			return true;
		}
	return false;
}/*end rm_operand*/

/* Scan through the loop, find operands and add them to the candidates array */
static int
find_candidates()
{
NODE *p;
int n_candidates = 0;
	for (ALL_THE_LOOP(p)) {
		if (is_any_cbr(p)) remember_jcc(p);
		if (is_any_label(p)) forget_jcc(p);
		if (!isfp(p)) continue;
		if (n_candidates >= MAX_CANDS) return n_candidates;
		if (indexing(p) != EBP) continue;
		if (isvolatile(p,1)) continue;
		if (!isregal(atoi(p->op1)) && !disambiged(p)) continue;
		if (is_fp_optimable(p)) {
			n_candidates += add_operand(p);
		}
	}
	for (ALL_THE_LOOP(p)) {
		if (indexing(p) != EBP) continue;
		if (isvolatile(p,1)) continue;
		if (!is_fp_optimable(p) && rm_operand(p)) n_candidates--; 
	}
	return n_candidates;
}/*end find_candidates*/

/*Sort the array all_candidates filled by find_candidates into the array
**sorted candidates. Take at most as many candidates as free_regs. If
**there were less to begin with, take all. In any case throw away candidates
**with negative payof estimation.
**return the number of sorted candidates.
*/
static int
sort_candidates(free_regs) int free_regs;
{
int last_taken = 0;
int max_estim = 0;
int i;
candidate *c_p = NULL;
	/*throw away cand with negative payof estimate, and those with no fst's.
	**fst only will stay, they are optimizable. Also dont throw away cands with
	**zero payof, there are peepholes afterwards.
	*/
	for (i = 0; i < last_cand; i++) {
		if (all_cands[i].ebp_offset != 0) {
			if (all_cands[i].estim < 0) {
				all_cands[i] = null_cand;
				--n_candidates;
			}
			if (all_cands[i].type == SEEN_ARIT) {
				all_cands[i] = null_cand;
				--n_candidates;
			}
		}
	}
	/* No longer need four types of cands but only two. shrink them */
	for (i = 0; i < last_cand; i++) {
		if (all_cands[i].type == SEEN_ARIT) all_cands[i].type = ARIT_B4_FST;
		else if (all_cands[i].type == SEEN_FST) all_cands[i].type = FST_B4_ARIT;
	}
	if (n_candidates <= free_regs) {
		for (i = 0; i < last_cand; i++) {
			if (all_cands[i].ebp_offset != 0) {
				sorted_cands[last_taken] = all_cands[i];
				last_taken++;
			}
		}
#ifdef DEBUG
		if (last_taken != n_candidates)
			fatal("taken %d out of %d cands.\n",last_taken,n_candidates);
#endif
		return n_candidates;
	}
	if (n_candidates > free_regs) n_candidates = free_regs;
	while (n_candidates) {
		/*scan the original array for the best cand. */
		max_estim = -1;
		for (i = 0; i < last_cand; i++) {
			if (all_cands[i].ebp_offset != 0
				&& all_cands[i].estim > max_estim) {
				max_estim = all_cands[i].estim;
				c_p = &all_cands[i];
			}
		}
		/*found the best cand, now place it in the sorted array */
		sorted_cands[last_taken] = *c_p;
		*c_p = null_cand;
		++last_taken;
		--n_candidates;
	}/*while loop*/
	return last_taken; /*number of taken candidates*/
}/*end sort_candidates*/

/* five states of entries on the simulated stack */
#define CLEAR	0
#define START	1
#define PART	2
#define OVERLAPPED	3
#define LEFTOVER	4

static int
rem_overlapping_cands()
{
NODE *p;
int min_offset, max_offset;
char *access;
int access_size;
int i;
int offset,location;
int size;
int x;
boolean all_clear, all_parts;
	/*find the smallest and biggest offsets in the loop */
	min_offset = max_offset = all_cands[0].ebp_offset;
	for (ALL_THE_LOOP(p)) {
		if (scanreg(p->op1,false) & EBP) {
			offset = atoi(p->op1);
		} else if (scanreg(p->op2,false) & EBP) {
			offset = atoi(p->op2);
		} else {
			continue;
		}
		if (offset < min_offset)
			min_offset = offset;
		if (offset > max_offset)
			max_offset = offset;
	}
	min_offset -= 8;
	max_offset += 8;
	/*assign an array and initialize it's entries to CLEAR */
	access_size = max_offset - min_offset;
	access = (char *) getspace(access_size);
	for (x = 0; x < access_size; x++) {
		access[x] = CLEAR;
	}
	/*go over all operands in the loop and mark them on the array*/
	/*look for ugly overlaps and mark them as such on the array  */
	for (ALL_THE_LOOP(p)) {
		if (scanreg(p->op1,false) & EBP) {
			offset = atoi(p->op1);
		}	else if (scanreg(p->op2,false) & EBP) {
			offset = atoi(p->op2);
		}	else {
				continue;
		}
		location = offset - min_offset;
		size = OpLength(p);
		all_clear = true;
		for (x = location ; x < location + size; x++) {
			if (access[x] != CLEAR) all_clear = false;
		}
		if (all_clear) {
			access[location] = START;
			for (x = location+1; x < location + size; x++) {
				access[x] = PART;
			}
		} else {
			/* check for exact overlap - a legal case */
			all_parts = true;
			for (x = location +1; x < location + size; x++) {
				if (access[x] != PART) all_parts = false;
			}
			/* if not an exact overlap - mark the overlaped entries as  */
			/* overlapped and the rest of hte entries as leftovers.     */
			if (access[location] != START || !all_parts) {
				for (x = location; x < location + size; x++) {
					if (access[x] != CLEAR) access[x] = OVERLAPPED;
					else access[x] = LEFTOVER;
				}
			}
		}
	}/*for all nodes in the loop*/
	/*go over the candidates and remove accessed and unalowed ones */
	for (i = 0; i < last_cand; i++) {
		if (all_cands[i].ebp_offset) {
			offset = all_cands[i].ebp_offset;
			location = offset - min_offset;
			size = all_cands[i].size;
			for (x = location; x < location + size; x++) {
				if (access[x] == OVERLAPPED) {
					all_cands[i] = null_cand;
					n_candidates--;
					break;
				}
			}
		}
	}
	return n_candidates;
}/*end rem_overlapping_cands*/

/*find the number of registers free throughout the entire function.
**If the first block starts at entry > 0, return 0 so that this loop will
**not be optimized.
*/

static int
find_max_free()
{
BLOCK *b = first_block;
int max, max1;
	if (((first_block->marked & 0xf) != 0)
		&& ((first_block->marked & 0xf) != ((first_block->marked >> 8) & 0xf)))
		return 0;
	max = (b->marked >> 4) & 0xf;
	while (b->lastn != last) {
		max1 = (b->marked >>4) & 0xf; /*max number of used entries in stack */
		if  (max1 > max) max = max1;
		b = b->next;
	}
	return (8 - max);
}/*find_max_free*/

static void
insert_fxch(at,i) NODE *at; int i;
{
NODE *p;
	p = insert(at);
	chgop(p,FXCH,"fxch");
	p->op1 = st_i[i];
	p->op2 = NULL;
	p->uses = p->sets = FP0;
}/*end insert_fxch*/

static void
rem_pop_from_fst(p) NODE *p;
{
	switch (p->op) {
		case FST: case FSTS: case FSTL:
			break;
		case FSTP:
			chgop(p,FST,"fst");
			break;
		case FSTPS:
			chgop(p,FSTS,"fsts");
			break;
		case FSTPL:
			chgop(p,FSTL,"fstl");
			break;
		case FSTPT:
		default: fatal("rem_pop_from_fst: cand handle\n");
	}
}/*end rem_pop_from_fst*/

/* If p->op1 is a candidate return a pointer to it's structure.
** Otherwise return a pointer to null_cand.
*/

static candidate *
is_cand(p) NODE *p;
{
int i;
int x;
boolean by_x;
	if (!p || p->op1 == NULL) return &null_cand;
	if (indexing(p) == EBP) {
		by_x = true;
		x = atoi(p->op1);
	} else by_x = false;
	if (by_x)  {
		for (i = 0; i <  n_candidates; i++)
			if (sorted_cands[i].ebp_offset == x)
				return &sorted_cands[i];
	} else {
		for (i = 0; i <  n_candidates; i++)
			if (!strcmp(sorted_cands[i].name,p->op1))
				return &sorted_cands[i];
	}
	return &null_cand;
}/*end is_cand*/

#define PROCESSED	0x1000
#define INSIDE		0x2000
#define IN_THE_LOOP(B) ((B)->marked  & INSIDE)
#define IS_CLEAR(B) (((B)->marked & PROCESSED) == 0)
#define MARK_VISITED(B)  (B)->marked |= PROCESSED

static void
mark_blocks_in_loop()
{
BLOCK *b;
	for (b = b0.next; b; b = b->next)
		b->marked &= 0x0FFF; /* clear bits 12-15 */
	for (b = first_block; b; b = b->next) {
		b->marked |= INSIDE;
		if (b->lastn == last) break;
	}
}/*end mark_blocks_in_loop*/

static void
set_cands_entries(stack) candidate *stack[8];
{
int i;
	for (i = 0; i < 8; i++)
		if (stack[i] && stack[i] != null_cand_p) 
			stack[i]->entry = i;
}/*end set_cands_entries*/

#ifdef DEBUG
void show_stack();
#endif

/* functions to simulate stack operations.
*/

static void
simulate_fld(c,stack) candidate *c; candidate *stack[8];
{
int i;
	if (stack[7]) {
		if (stack[7] == null_cand_p) fatal("cant fld, [7] has null_cand\n");
		else
		fatal("fld: already 8 %d, %s, cant fld; at %d\n",stack[7]->ebp_offset,
		stack[7]->name,c?c->ebp_offset:0);
	}
	for (i = 6; i >= 0; i--) {
		stack[i+1] = stack[i];
	}
	if (((stack[0] = c) != NULL) && (c != null_cand_p))
		c->entry = 0;
	for (i = 0; i < 8; i++)
		if (stack[i] && stack[i] != null_cand_p) stack[i]->entry = i;
}/*end simulate_fld*/

static void
simulate_fst(stack) candidate *stack[8];
{
int i;
int last;
	if (stack[0] == NULL) fatal("Empty stack, can't pop\n"); 
	if (stack[0] != null_cand_p) stack[0]->entry = -1;
	for (last = 7; stack[last] == NULL; last--) ; /*find last entry*/
	for (i = 0; i < 7 ; i++) {
		stack[i] = stack[i+1];
	}
	stack[last] = NULL;
	for (i = 0; i < 8; i++)
		if (stack[i] && stack[i] != null_cand_p) {
			stack[i]->entry = i;
		}
}/*end simulate_fst*/

/*better have a seperate function than activate simulate_fst twice. */
static void
simulate_fcompp(stack) candidate *stack[8];
{
int i;
int last;
	if (stack[0] == NULL || stack[1] == NULL)
		fatal("fcompp: cant pop twice\n");
	if (stack[0] != null_cand_p) stack[0]->entry = -1;
	if (stack[1] != null_cand_p) stack[1]->entry = -1;
	for (last = 7; stack[last] == NULL; last--) ; /*find last entry*/
	for (i = 0; i < 6 ; i++) {
		stack[i] = stack[i+2];
	}
	stack[last] = NULL;
	stack[last-1] = NULL;
	for (i = 0; i < 8; i++)
		if (stack[i] && stack[i] != null_cand_p) stack[i]->entry = i;
}/*end simulate_fcompp*/

static void
simulate_fxch(i,stack) int i; candidate *stack[8];
{
candidate *tmp;
	tmp = stack[0];
	stack[0] = stack[i];
	if (stack[0] && stack[0] != null_cand_p) stack[0]->entry = 0;
	stack[i] = tmp;
	if (stack[i] != null_cand_p) stack[i]->entry = i;
}

/* the target of an fstp instruction is the stack, whereas the target of an
** fstpl instruction is not the fp stack. Hence the different simulations.
*/
static void
simulate_fstp(c,at,stack) candidate *c; int at; candidate *stack[8];
{
	stack[at] = c;
	if (c && c != null_cand_p) {
		c->entry = at;
	}
	simulate_fst(stack);
}

static void
change_name_of_tos(p,stack) NODE *p; candidate *stack[8];
{
	stack[0] = is_cand(p);
	if (stack[0] != null_cand_p) stack[0]->entry = 0;
}/*end change_name_of_tos*/

#ifdef DEBUG
void
show_stack(stack) candidate *stack[8];
{
int i;
	for (i = 0; i < 8; i++) {
		if (stack[i] != NULL) {
			fprintf(STDERR,"st[%d] = ",i);
				if (stack[i] == null_cand_p)
					fprintf(STDERR,"  NC\n");
				else
					fprintf(STDERR," %d entry %d\n",
					stack[i]->ebp_offset,stack[i]->entry);
		}
	}
}
void
show_cands(base) candidate *base;
{
int i;
	if (base == all_cands) fprintf(STDERR,"all ");
	else fprintf(STDERR,"sorted ");
	fprintf(STDERR,"loop of "); FPRINST(first);
	for (i = 0; i < n_candidates; i++) {
		if (base[i].name != NULL) {
			fprintf(STDERR,"at %d cand %s with %d type %d \n",i,base[i].name,
			base[i].estim,base[i].type);
		}
	}
}/*end show_cands*/
#endif

static void
insert_preloads(stack) candidate *stack[8];
{
int i;
NODE *last_prepended = last->forw;
NODE *last_inserted = first->back;
NODE *last_added = last->forw;
	/*insert fld instructions before the loop and fst instructions after. */
	for (i = 0; i <  n_candidates; i++) {
		if (sorted_cands[i].type == ARIT_B4_FST) {
			simulate_fld(&sorted_cands[i],stack);
			last_prepended = prepend(last_prepended,NULL);
			last_inserted = insert(last_inserted);
			switch (sorted_cands[i].size) {
				  case LoNG:
						chgop(last_prepended,FSTPS,"fstps");
						chgop(last_inserted,FLDS,"flds");
						break;
				  case DoBL:
						chgop(last_prepended,FSTPL,"fstpl");
						chgop(last_inserted,FLDL,"fldl");
						break;
				  case TEN:
						chgop(last_prepended,FSTPT,"fstpt");
						chgop(last_inserted,FLDT,"fldt");
						break;
				  default: fatal(gettxt(":420","what oplength is that?\n"));
			}
			last_prepended->op1 = sorted_cands[i].name;
			last_prepended->uses = FP0 | EBP;
			last_prepended->sets = FP0;
			last_inserted->op1 = last_prepended->op1;
			last_inserted->uses = FP0 | EBP;
			last_inserted->sets = FP0;
#ifdef DEBUG
			if (zflag) {
				fprintf(STDERR,"prepended ");
				FPRINST(last_prepended);
				fprintf(STDERR,"inserted ");
				FPRINST(last_inserted);
			}
#endif
		} 
	}
#if 0
	last_added = last_added->back;
	for (i = 0; i <  n_candidates; i++) {
		if (sorted_cands[i].type == FST_B4_ARIT) {
			last_prepended = insert(last_added);
			last_prepended->op1 = "%st(0)";
			last_prepended->op2 = NULL;
			last_prepended->uses = FP0 | EBP;
			last_prepended->sets = FP0;
			chgop(last_prepended,FSTP,"fstp");
		}
	}
#endif
}/*end insert_preloads*/

#define LAST_ARIT_	4
#define HAS_LAST_ARIT	5

static void
mark_last_arits()
{
NODE *p;
BLOCK *b = first_block;
int i;
candidate *cand;

	/* mark the last arit instruction for fst_b4 cands */
	for (ALL_THE_LOOP(p)) p->usage = 0;
	for (ALL_THE_LOOP(p)) {
		if (p == b->lastn->forw) {
			b = b->next;
			for (i = 0; i < n_candidates; i++)
				sorted_cands[i].last_arit = NULL;
		}
		if (isfp(p) && (cand = is_cand(p)) != null_cand_p) {
			if ((is_fp_arit(p) || is_fld(p) || is_fstnst(p))
				&& cand->last_arit) {
				if (is_fstp(cand->last_arit)) {
					cand->last_arit->usage = HAS_LAST_ARIT;
				} else {
					cand->last_arit->usage = 0; /* unmark previous one */
				}
				cand->last_arit = p;
				p->usage = LAST_ARIT_;
			} else if (is_fstp(p)) {
				cand->last_arit = p; /* mark to allow setting */
			}
		}
	}
}/*end mark_last_arits*/

static int
find_depth(stack) candidate *stack[8];
{
int last;
	for (last = 7; stack[last] == NULL; last--) ; /*find last entry*/
	return last;
}/*end find_depth*/

/*For candidates that are first used, add fld instructions before the loop.
**for the others, only assign a slot by writting an index in the cand structure.
**Add fst instruction after the loop for all the candidates. change the code
**to use the fp-stack instead.
*/
static void
regals_to_fps(b,stack) BLOCK *b; candidate *stack[8];
{
NODE *p;
opopcode opop;
candidate  *cur_cand = NULL;
int depth;
candidate *spare_stack[8];
int i;
static NODE *last_added = NULL;

	MARK_VISITED(b);
	set_cands_entries(stack);
	if (b->firstn == first) {
		if (!is_fstp(last->forw)) last_added = last;
		else {
			for ( last_added = last->forw; is_fstp(last_added);
				last_added = last_added->forw )  ;
		}
	}
	/*go on the loop and change reference to candidates to reference %st(i)*/
	for (p = b->firstn; p != b->lastn->forw; p = p->forw) {
		if (isfp(p)) {
			if ((cur_cand = is_cand(p)) != null_cand_p) {
				if (cur_cand->type == ARIT_B4_FST) {
					opop = mem2st(p->op);
					chgop(p,opop.op,opop.op_code);
					p->op1 = st_i[cur_cand->entry];
					p->op2 = NULL;
					if (is_fstp(p)) {
						simulate_fstp(cur_cand,cur_cand->entry,stack);
					} else if (p->op == FLD) {
						simulate_fld(cur_cand,stack);
					} else if (p->op !=FST && p->op !=FCOMP && p->op !=FCOMPP) {
						p->op2 = "%st";
					}
					p->uses = EBP | FP0;
					p->sets = FP0;
				} else if (cur_cand->type == FST_B4_ARIT) {
					if (is_fstp(p)) {
						if (cur_cand->entry < 0) { /*not yet on stack */
							depth = find_depth(stack);
							chgop(p,FSTP,"fstp");
							p->op1 = st_i[depth+1];
							simulate_fstp(cur_cand,depth+1,stack);
							if (cur_cand->needed) {
								p->op4 = "";
								last_added = insert(last_added);
								last_added->op1 = cur_cand->name;
								last_added->op2 = NULL;
								last_added->uses = FP0 | EBP;
								last_added->sets = FP0;
								switch (cur_cand->size) {
									case LoNG:
										chgop(last_added,FSTPS,"fstps");
										break;
									case DoBL:
										chgop(last_added,FSTPL,"fstpl");
										break;
									case TEN:
										chgop(last_added,FSTPT,"fstpt");
										break;
									default:
										fatal("internal error, wrong length\n");
										break;
								}
							} else {
								last_added = insert(last_added);
								chgop(last_added,FSTP,"fstp");
								last_added->op1 = "%st(0)";
								last_added->op2 = NULL;
								last_added->uses = FP0 | EBP;
								last_added->sets = FP0;
								p->op4 = NULL;
							}
						} else {  /* already on stack (how can it be on stack?) */
							opop = mem2st(p->op);
							chgop(p,opop.op,opop.op_code);
							p->op1 = st_i[cur_cand->entry];
							p->op2 = NULL;
							if (p->op == FSTP)
								simulate_fstp(cur_cand,cur_cand->entry,stack);
						}
					} else if (cur_cand->entry >= 0) { /* not fstp, on stack */
						opop = mem2st(p->op);
						chgop(p,opop.op,opop.op_code);
						p->op1 = st_i[cur_cand->entry];
						if (is_fld(p)) {
							p->op2 = NULL;
							simulate_fld(cur_cand,stack);
						} else if (is_fst(p)) {
							p->op2 = NULL;
						} else if (p->op != FCOMP) p->op2 = "%st";
					} else { /* not fst, not on stack, should never happen because
								the fstp treat left it on the stack. */
						if (is_fld(p)) simulate_fld(cur_cand,stack);
						if (FPOP(p)) simulate_fst(stack);
					}
				} else { /* type not arit_b4 and not fst_b4 */
					fatal("fp_loop: funny type\n");
				}
			}	else { /* not a candidate */
				if (FPUSH(p->op)) simulate_fld(null_cand_p,stack);
				if (FPOP(p)) {
					simulate_fst(stack);
				}
				if (p->op == FCOMPP) simulate_fcompp(stack);
				if (p->op == FXCH) 
					if (p->op1 == NULL) simulate_fxch(1,stack);
					else simulate_fxch(p->op1[4] - '0',stack);
			}
		}/*endif isfp*/
	}/*end for loop*/
	for (i = 0; i < 8; i++)
		spare_stack[i] = stack[i];
	if (b->nextl && IS_CLEAR(b->nextl) && IN_THE_LOOP(b->nextl))
		regals_to_fps(b->nextl,stack);
	for (i = 0; i < 8; i++)
		stack[i] = spare_stack[i];
	if (b->nextr && IS_CLEAR(b->nextr) && IN_THE_LOOP(b->nextr))
		regals_to_fps(b->nextr,stack);
}/*end regals_to_fps*/

/*are there any fp inst. in the loop.
** check in addition for no calls.
** and no switch jumps.
*/
static boolean
has_fp()
{
NODE *p;
boolean retval = false;
	for (ALL_THE_LOOP(p)) {
		if (p ->op == CALL) return false;
		if (is_jmp_ind(p)) return false;
		if (isfp(p)) retval = true; /*true unless a call will be found later*/
	}
	return retval;
}/*end has_fp*/

static void
peep_in_the_loop()
{
NODE *p,*q;
NODE *pop = last->forw;
	COND_RETURN("peep_in_the_loop");
	if (!is_fst(pop)) return; /* every candidate has an fst after the loop*/
	while (pop->op == FSTPS || pop->op == FSTPL || pop->op == FSTPT)
		pop = pop->forw; /* skip the preloaded ones */
	if (pop->op != FSTP || pop->op1[4] != '0') return; /* non FST_B4_ARIT */
	for (ALL_THE_LOOP(p)) {
		if (p->op == FSTP && p->op1[4] == '1' && p->op4 == NULL) {
			q = next_fp(p);
			while ((is_fp_arit(q) || is_fld(q)) && q->op1 && !isreg(q->op1))
				q = next_fp(q);
			if (q->op == FADD && q->op1 && q->op1[4] == '1') {
				chgop(q,FADDP,"faddp");
				q->op1 = "%st";
				q->op2 = "%st(1)";
				DELNODE(p);
				DELNODE(pop);
				pop = pop->forw;
				if (pop->op != FSTP) return; /* no more FST_B4 cands */
			}
		}
	}
}/* end peep_in_the_loop*/

/*Coordinate fp optimization. Mark all possible operands as candidates and
**find their estimate payof. Sort the candidates and take the first few,
**as many as there are free stack slots. Then regals_to_fps() will do
**the changes to the function body.
*/
static void
do_fp_loop_opts ()
{
candidate  *stack[8];
int free_regs; /* number of fp stack slots available thru the entire loop */

	stack[0] = NULL; stack[1] = NULL;
	stack[2] = NULL; stack[3] = NULL;
	stack[4] = NULL; stack[5] = NULL;
	stack[6] = NULL; stack[7] = NULL;

	no_cands(stack);
	free_regs = find_max_free();
	if (free_regs == 0) return;
	n_candidates = find_candidates();
	if (n_candidates == 0) return;
	n_candidates = rem_overlapping_cands();
	if (n_candidates == 0) return;
	n_candidates = sort_candidates(free_regs);
	if (n_candidates > 0) {
		insert_preloads(stack);
		mark_last_arits();
		mark_blocks_in_loop();
		regals_to_fps(first_block,stack);
		peep_in_the_loop();
	}
}/*end do_fp_loop_opts*/

typedef struct node_list1 {
			NODE *element;
			struct node_list1 *next;
			int direction;
			unsigned int idx_reg;
	} NODE_LIST1;


static NODE_LIST1 *overlapping_operands = NULL;
static NODE *pivot = NULL;
static unsigned int pivot_idx_reg;

static void
add_operand_2_disambig(p) NODE *p;
{
NODE_LIST1 *e;
	for (e = overlapping_operands; e; e = e->next)
		if (indexing(e->element) == indexing(p))
			return;
	e = (NODE_LIST1 *) getspace(sizeof(NODE_LIST1));
	e->element = p;
	e->next = overlapping_operands;
	e->idx_reg = indexing(p);
	e->direction = 0;
	overlapping_operands = e;
}/*end  add_operand_2_disambig*/

/*It is assumed that the loop has no call instructions and has fp instructions.
**The loop has to be one basic block.
**Exactly one fstp of an operand which is zero offset from a register.
**The register is not changed in the loop.
**Other indexing registers are changed exactly once, by add/sub instructions.
**They are used as pointers, not as array indices.
*/
static boolean
has_run_time_potential()
{
NODE *p;
int i;
NODE_LIST1 *e;
int e_set;
boolean satisfied = false;
int m;
NODE *try_pivot = first;
	pivot = NULL;
	overlapping_operands = NULL;
	if (first_block->lastn != last) return false; /* more than one block*/
	/* find a pivot. If it is the operand of an integer instruction give
	** it up and find another one.
	*/
	while (! satisfied) {
		pivot = NULL; /* find a candidate to be a pivot */
		for (p = try_pivot->forw; p != last; p = p->forw) {
			if (is_fstp(p)) {
				for (i = first_reg; i <= last_reg; i++) {
					if (iszoffset(p->op1,all_regs[i].names[3])) {
						try_pivot = pivot = p;
						pivot_idx_reg = all_regs[i].reg;
						break;
					}
				}
			}
		}
		if (pivot == NULL) {
			satisfied = true;
			break;
		} else { /* verify the pivot */
			satisfied = true;
			for (ALL_THE_LOOP(p)) {
				if ((((m = 1, p->op1 && !strcmp(p->op1,pivot->op1)) ||
					(m = 2, p->op2 && !strcmp(p->op2,pivot->op1)))
					&& (isvolatile(p,m) || !is_fp_optimable(p)))
				  || (p->sets & pivot_idx_reg)) {
						pivot = NULL;
						satisfied = false; /* will try to find another pivot*/
						break;
				}
			}
		}
	}/*while loop*/
	if (pivot == NULL) {
		return false;
	}
	/* collect all undisambiguated operands */
	/* for now, don't allow operands with index register, only base. */
	for (ALL_THE_LOOP(p)) {
		p->sets |= msets(p);
		p->uses |= muses(p);
	}
	for (p = first; p != pivot; p = p->forw) {
		if ((ismem(p->op1) && ((p->op1[0] != '(') || strchr(p->op1,','))) ||
			(ismem(p->op2) && ((p->op2[0] != '(') || strchr(p->op2,',')))) {
			return false;
		}
		if (((p->sets | p->uses) & MEM) &&
			(indexing(p) != pivot_idx_reg && seems_same(p,pivot))) {
			add_operand_2_disambig(p);
		}
	}
	for (p = pivot->forw; p != last; p = p->forw) {
		if ((ismem(p->op1) && (p->op1[0] != '(' || strchr(p->op1,','))) ||
			(ismem(p->op2) && (p->op2[0] != '(' || strchr(p->op2,',')))) {
			return false;
		}
		if (((p->sets | p->uses) & MEM) &&
			(indexing(p) != pivot_idx_reg && seems_same(pivot,p))) {
			add_operand_2_disambig(p);
		}
	}
	for (ALL_THE_LOOP(p)) {
		p->sets = sets(p);
		p->uses = uses(p);
	}
	/* The overlapping operands have to change only once, and by add/sub */
	for (e = overlapping_operands; e; e = e->next) {
		e_set = false;
		for (ALL_THE_LOOP(p)) {
			if (p->sets & e->element->uses & regs) {
				if (e_set) {
					switch (p->op) {
						case ADDL: case INCL:
							if (e->direction != 1) return false;
							break;
						case SUBL: case DECL:
							if (-1 != e->direction) return false;
							break;
						default: return false;
					}
				} else {
					if (p->op == ADDL || p->op == INCL) {
						e->direction = 1;
						e_set = true;
					}
					else if (p->op == SUBL || p->op == DECL) {
						e->direction = -1;
						e_set = true;
					} else {
						return false;
					}
				}
			}
		}
	}
	return true;
} /*end has_run_time_potential*/

/*change the code to make it like:
** 1. Instructions to compare the index registers and jump to either loop.
** 2. One copy of the loop.
** 3. Jump after the second copy.
** 4. Second copy of the loop.
*/

static void
duplicate_the_loop()
{
NODE *p = first;
NODE *p1;
NODE_LIST1 *e;
NODE *new_first,*new_last;
BLOCK *b;
	/* memory disambiguation before the loop */
	for (e = overlapping_operands; e; e = e->next) {
		p = prepend(p,NULL);
		chgop(p,CMPL,"cmpl");
		p->op1 = itoreg(pivot_idx_reg);
		if (e->idx_reg) p->op2 = itoreg(e->idx_reg);
		else if (ismem(e->element->op1)) p->op2 = e->element->op1;
		else if (ismem(e->element->op2)) p->op2 = e->element->op2;
		else {
			fatal("fp_loop: unexpected memory set\n");
		}
		p1 = insert(p);
		if (e->direction > 0) {
			chgop(p1,JBE,"jbe");
		} else if (e->direction < 0) {
			chgop(p1,JAE,"jae");
		} else
			chgop(p1,JNE,"jne");
		p1->op1 = first->opcode;
	}
	/* duplicate the loop */
	new_first = p1 = first->back;
	for (ALL_THE_LOOP(p)) p1 = duplicate(p,p1);
	new_first = new_first->forw;
	new_last = first->back;
	new_first->opcode = getspace(NEWSIZE);
	sprintf(new_first->opcode,".RG%d",++label_counter);
	new_last->op1 = new_first->opcode;
	new_sets_uses(new_last);
	add_label_to_align(new_first->opcode);
	/* after the loop jump to the end of the second loop */
	p1 = insert(new_last);
	chgop(p1,JMP,"jmp");
	if (islabel(first_block->next->firstn))
		p1->op1 = first_block->next->firstn->opcode;
	else {
		p = insert(first_block->lastn);
		p->op = LABEL;
		p->opcode = getspace(NEWSIZE);
		sprintf(p->opcode,".RG%d",++label_counter);
		p1->op1 = p->opcode;
	}
	new_sets_uses(p1);
	/* make the loop the first copy of the loop */
	first = new_first;
	last = new_last;
	bldgr(false);
	for (b = b0.next; b; b = b->next)
		if (b->firstn == first) {
			first_block = b;
			break;
		}
}/* end duplicate_the_loop*/

static void
set_a_candidate(stack) candidate *stack[8];
{
NODE *p;
	no_cands(stack);
	n_candidates = 1;
	sorted_cands[0].ebp_offset = 0;
	sorted_cands[0].name = pivot->op1;
	sorted_cands[0].estim = 0;
	sorted_cands[0].size = OpLength(pivot);
	sorted_cands[0].type_decided = true;
	sorted_cands[0].needed = true; /* not ideal but safe */
	sorted_cands[0].last_arit = NULL;
	sorted_cands[0].entry = 0;
	/* set the type */
	for (ALL_THE_LOOP(p)) {
		if ((p->op1 && !strcmp(p->op1,pivot->op1)) ||
			(p->op2 && !strcmp(p->op2,pivot->op1))) {
			if (is_fstp(p))
				sorted_cands[0].type = FST_B4_ARIT;
			else 
				sorted_cands[0].type = ARIT_B4_FST;
			break;
		}
	}
}/*end set_a_candidate*/

static void
try_again_w_rt_disamb()
{
candidate  *stack[8];

	stack[0] = NULL; stack[1] = NULL;
	stack[2] = NULL; stack[3] = NULL;
	stack[4] = NULL; stack[5] = NULL;
	stack[6] = NULL; stack[7] = NULL;

	duplicate_the_loop();
	set_a_candidate(stack);
	insert_preloads(stack);
	regals_to_fps(first_block,stack);
	check_fp();
}

BLOCK *
find_cur_block(p) NODE *p;
{
BLOCK *b;
NODE *q;
	for (b = b0.next; b; b = b->next)
		for (q = b->firstn; q != b->lastn->forw; q = q->forw)
			if (q == p) return b;
/* NOTREACHED */
}/*end find_cur_block*/


/*Driver of the fp loop optimization. Could be together with the int loop
**optimization. This way, however, we save time. We do not have to execute
**check_fp() when int loop optimization changed the function. We do check_fp()
**one time per function.
**This also has to do with doing only innermost loops. If we do outer loops, we
**have to either do check_fp() every time, or update b->marked.
*/

void
fp_loop()
{
NODE *pm;
char *jtarget;
BLOCK *b;
	COND_RETURN("fp_loop");
	bldgr(false);
	check_fp();		/* FP stack usage by the block */
	regal_ldanal(); /* what candidates are dead after the loop */
	first = last = NULL;
	b = b0.next;
	for (ALLN(pm)) {
		if (pm == b->lastn->forw) b = b->next;
		if (islabel(pm) && lookup_regals(pm->opcode,first_label)) {
			first = pm;
			first_block = b;
			continue;
		}
		else if (iscbr(pm) && first && !strcmp(pm->op1,first->opcode))
			last = pm;
		else continue;
#ifdef DEBUG
		if (zflag) {
			fprintf(STDERR,"loop of "); 
			FPRINST(first);
		}
#endif
		/*do only very simple loops, good enough for fp loops. */
		if (is_simple_loop(&jtarget) != 1) {
#ifdef DEBUG
			if (zflag) {
				fprintf(STDERR,"loop not very simple: ");
				FPRINST(first);
			}
#endif
			continue;
		}
		/*At this point we have a loop and it is simple.*/
		if (has_fp()) {
			COND_SKIP(continue,"loop %d %s\n",second_idx,first->opcode,0);
			do_fp_loop_opts();
			if (has_run_time_potential()) {
				COND_SKIP(continue,"Loop %d %s\n",second_idx,first->opcode,0);
				try_again_w_rt_disamb();
				b = find_cur_block(pm); /* new flow graph, need to update */
			}
		}
	}/*end for loop*/
}/*end fp_loop*/

static boolean
ok_to_decmpl(compare) NODE *compare;
{
NODE *p;
unsigned int reg;
unsigned int idx;
int step,limit;
NODE *increment;
	if (!isconst(compare->op1) || !isreg(compare->op2)) return false;
	if (TEST_CF(last->op)) return false;
	reg = setreg(compare->op2);
	if (last->forw->nlive & reg) return false; 
	for (p = compare->back; p != first->back; p = p->back) {
		if (p->sets & CONCODES)
			if (!(p->sets & reg)) {
				return false;
			} else {
				increment = p;
				break; /* the loop increment instruction */
			}
	}
	limit = atoi(compare->op1+1);
	switch (p->op) {
		case INCL: case DECL:
			break;
		case ADDL: case SUBL:
			if(*(p->op1) != '$')
				return false;
			step = atoi(p->op1+1);
			if (step == 0 || limit % step) return false;
			break;
		case LEAL:
			step = atoi(p->op1);
			if (step == 0 || limit % step) return false;
			break;
		default:
			return false;
	}
	for (ALL_THE_LOOP(p)) {
		if (p != compare) {
			if ((uses_but_not_indexing(p) & ~p->sets) & reg) return false;
			if (((idx = indexing(p)) & reg)) {
				if (p->op1 && strchr(p->op1,',') && has_scale(p->op1)) 
					return false;
				if (p->op2 && strchr(p->op2,',') && has_scale(p->op2))
					return false;
			}
			/* funny way to test: there is no scale, so test if the indexing
			** is exactly reg and there is a ',' ergo it appears twice, which
			** is what we look for
			*/
			if (idx == reg) {
				if ((p->op1 && strchr(p->op1,',')) ||
					(p->op2 && strchr(p->op2,',')))
					return false;
			}
		}
		if (p != increment && (p->sets & reg))
			return false;
	}
	return true;
}/*end ok_to_decmpl*/

static void
add_fix(jtarget,imm,operand) char *jtarget, *imm, *operand;
{
NODE *p,*pnew;
char *new_label;
	/* find the label which is the jump target */
	for (ALLN(p)) {
		if (is_any_label(p) && !strcmp(p->opcode,jtarget)) {
			break;
		}
	}
	/* go back to the beginning of the basic block - skip labels */
	pnew = p;
	while (is_any_label(pnew)) pnew = pnew->back;
	pnew = pnew->forw;
	/* prepend a jmp to jtarget */
	pnew = prepend(pnew,NULL);
	chgop(pnew,JMP,"jmp");
	pnew->op1 = jtarget;
	pnew->uses = pnew->sets = 0;
	/* after the jump, place the new label */
	pnew = insert(pnew);
	pnew->op = LABEL;
	pnew->opcode = getspace(LABELSIZE);
	sprintf(pnew->opcode,".R%d",++label_counter);
	new_label = pnew->opcode;
	pnew->uses = pnew->sets = 0;
	/* after the new label put the addl instruction */
	pnew = insert(pnew);
	chgop(pnew,LEAL,"leal");
	pnew->op1 = getspace(NEWSIZE);
	sprintf(pnew->op1,"%s(%s)",1+imm,operand);
	pnew->op2 = operand;
	pnew->uses = pnew->sets = setreg(operand);
	/* change jmps to jtaarget to jump to the new label */
	for (ALL_THE_LOOP(p)) {
		if (is_any_br(p) && !strcmp(p->op1,jtarget))
			p->op1 = new_label;
	}
}/*end add_fix*/

static void
decmpl(compare,jtarget) NODE *compare; char *jtarget;
{
NODE *p;
char sign,*t,name[100];
int x,size;
unsigned int idx;
int m;
int scale;
char *c;
	idx = compare->uses;
	for (p = first->back; !is_any_label(p); p = p->back)
		if (p->sets & idx) break;
	if (is_any_label(p)) {
		return;
	}
	if (! (  /* loop counter initialized to zero before the loop */
		(p->op == XORL && samereg(p->op1,p->op2)) ||
		(p->op == SUBL && samereg(p->op1,p->op2)) ||
		(p->op == MOVL && !strcmp(p->op1,"$0")))
	)	{
		return;
	}
	size = atoi(compare->op1+1);
	chgop(p,MOVL,"movl");
	p->op1 = getspace(ADDLSIZE);
	sprintf(p->op1,"$%d",-size);
	if (jtarget) add_fix(jtarget,compare->op1,p->op2);
	for (p = p->forw; p != first; p = p->forw) {
		for (m = 1; m <= 2; m++) {
			if (setreg(p->ops[m]) == idx)
				if (p->op == CMPL && m == 2) {
					if (isreg(p->op1)) {
						chgop(p,TESTL,"testl");
						p->op2 = p->op1;
					} else {
						char *tmp = p->op2; /* try to eliminate the compare */
						p->op2 = "$0";		/* and the jump */
						if (!cmp_imm_imm(p,p->forw)) {
							p->op2 = tmp; /* failed, dont optimize */
							return;
						}
					}
				} else 
					p->ops[m] = "$0";
		}
	}
	for (ALL_THE_LOOP(p)) {
		if (indexing(p) & idx) {
			if ((t = strchr(p->op1,'(')) != NULL) m = 1;
			else {
				m = 2;  /* assuming there is an m */
				t = strchr(p->op2,'(');
			}
			for (x = 0; x < 100; x++) name[x] = (char) 0;
			(void) decompose(p->ops[m],&x,name,&scale);
			c = p->ops[m][0] == '*' ? "*" : "";
			x += size;
			if (x > 0) sign = '+';
			else sign = '-';
			p->ops[m] = getspace(strlen(p->ops[m]) + 11);
			if (x == 0)
				sprintf(p->ops[m],"%s%s%s",c,name,t);
			else if (name[0])
				sprintf(p->ops[m],"%s%s%c%d%s",c,name,sign,x,t);
			else
				sprintf(p->ops[m],"%d%s",x,t);
		}
	}
	p = insert(last);
	chgop(p,LEAL,"leal");
	p->op1 = getspace(NEWSIZE);
	sprintf(p->op1,"%d(%s)",size,compare->op2);
	p->op2 = compare->op2;
	new_sets_uses(p);
	DELNODE(compare);
}/*end decmpl*/

void
loop_decmpl()
{
NODE *pm;
char *jtarget = NULL;
	COND_RETURN("loop_decmpl");
	for (ALLN(pm)) {
		jtarget = NULL;
		last = pm->forw; if (!last || last == &ntail) return;
		if ((pm->op == CMPL) && (iscbr(last))) {
			if ((first = is_back_jmp(last)) == NULL) {
#ifdef DEBUG
				if (zflag > 1) {
					fprintf(STDERR,"not a back jmp");
					FPRINST(last);
				}
#endif
				continue;
			}
			if (is_simple_loop(&jtarget) == 0) {
#ifdef DEBUG
				if (zflag) {
					fprintf(STDERR,"loop not simple: ");
					FPRINST(first);
				}
#endif
				continue;
			}
			COND_SKIP(continue,"loop %d %s\n",second_idx,first->opcode,0);
			if (ok_to_decmpl(pm)) decmpl(pm,jtarget);
		}/*endif cmp-jcc*/
	}/*end for loop*/
}/*end loop_decmpl*/
