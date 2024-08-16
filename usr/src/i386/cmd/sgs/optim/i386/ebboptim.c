/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)optim:i386/ebboptim.c	1.8.7.12"

#include <unistd.h>
#include "sched.h" /*include optim.h and defs.h */
#include "optutil.h"

/* All following optimizations have to do with values in registers.
** Therefore they operate on generalized basic block. These are
** consecutive basic blocks in which the first one begins with a
** label and the next ones do not. Hence the only entry to them
** is by fall through.
*/

static void remove_register(), rmrdmv();
void replace_registers();
static int try_forward();
static void try_backward(); 
static boolean zvtr();
static int is_clearing_op();
static int check1();
static int regis();
static boolean isadc();
int  isbase();
void  remove_base();
void remove_index();
int fflag = 0;
extern int fp_removed;	/*declared in local.c */
extern int ptm_opts;   /*declared in local.c */
extern int blend_opts;	/*declared in local.c */
extern int i486_opts;   /*declared in local.c */
extern void bldgr(), ldelin2(), chgop(), regal_ldanal();
extern unsigned indexing(), scanreg(), setreg(), uses(), sets();
extern int samereg(), isindex(), isfp();
extern char *dst();
extern unsigned int hard_uses();

void
ebboptim(opt) int opt;
/* The dirver of the optimizations. Determines the first and last
** nodes of the extended basic block and calls the specific function
** according to it's parameter.
*/
{
BLOCK *b , *firstb;
int found = 0;

#ifdef DEBUG
	if (opt == COPY_PROP) {
		COND_RETURN("rmrdmv");
	} else if (opt == ZERO_PROP) {
		COND_RETURN("zvtr");
	}
#endif

	bldgr(false);

	for (b = firstb = b0.next ; b ; b = b->next) {
		firstb = b;
		while (!( islabel(b->lastn->forw) || isuncbr(b->lastn) 
				  || b->lastn->forw == &ntail)) 
			b = b->next;
		switch (opt) {
			case ZERO_PROP:
				found |= zvtr(firstb->firstn,b->lastn); /*zero value trace*/
				break;
			case COPY_PROP:
				rmrdmv(firstb,b->lastn); /*remove redundant mov reg,reg*/
				break;
			default:
				fatal(gettxt(":1391","unknown optimization for ebboptim\n"));
		}
	}/*for loop*/
	if (found)
		ldanal();

}/*end ebboptim*/

/* A basic block level optimization. Motivation came from the following
** basic block, lhs before, rhs after:

** .L295:                                    .L295:
**	movl	%esi,%ebp              <  
**	movl	%ebx,%ecx              <  
**	addl	$4,%esi                <  
**	addl	$4,%ebx                <  
**	flds	(%edi)                    	flds	(%edi)
**	fmuls	(%ecx)                 |  	fmuls	(%ebx)
**	fadds	(%ebp)                 |  	addl	$4,%ebx
**	fstps	(%ebp)                 |  	fadds	(%esi)
**	                               >  	fstps	(%esi)
**	                               >  	addl	$4,%esi
**	cmpl	%eax,%ebx                 	cmpl	%eax,%ebx
**	jna	.L295                     	jna	.L295
**
** Here the optimization was done twice, and the two first instructions
** were deleted.
*/

void
bbopt()
{
BLOCK *b;
NODE *p,*q,*qforw;
NODE *first;
NODE *second;
NODE *last;
NODE *limit;
boolean uniq_change;
unsigned int srcreg,dstreg;
boolean first_seen;
char *tmp;
char *t,*t1,*s;
int m;
boolean success = false;
boolean memused;
boolean doit;
unsigned int idxreg;
	COND_RETURN("bbopt");
	bldgr(false);
	for (b = b0.next; b; b = b->next) {
#if 0
		if (start && last_func() && last_one()) {
			d++;
			if (d < start || d > finish) continue;
			else  fprintf(stderr,"%d %s ",d,b->firstn->opcode);
		}
#endif
		COND_SKIP(continue,"%d %s ",second_idx,b->firstn->opcode,0);
		for (p = b->firstn; p != b->lastn->forw; p = p->forw) {
			if (p->op == MOVL && isreg(p->op1) && isreg(p->op2)) {
				first = last = second = NULL;
				uniq_change = true;
				srcreg = setreg(p->op1);
				dstreg = setreg(p->op2);
				/* find q that uses dstreg as an index */
				for (q = p; q != b->lastn->forw; q = q->forw) {
					unsigned int qidx;
					if ((qidx = indexing(q)) == dstreg) {
						if (!first)
							first = q; /* remember the first one */
						if (!(q->sets & dstreg) && !(q->nlive & dstreg))
							last = q; /* remember the last one */
						if (q->uses & srcreg) {
							last = NULL;
							break; /*disqualify */
						}
					}
					if ((qidx & dstreg) && (qidx != dstreg)) {
						last = NULL;
						break; /*disqualify */
					}
				}
				if (!last) {
					continue; /* goto next movl*/
				}
				second = NULL;
				/* find a setting of srcreg between p and last, only one */
				/* If that inst uses or sets memory, we conservatively do
				** not move it over any other memory usage, to save anal time.
				*/
				first_seen = false;
				memused = false;
				for (q = p->forw; q != last; q = q->forw) {
					if (q == first) first_seen = true;
					/* disable if there is a direct use of dstreg, not as
					** index. It can be added, but take care for only
					** explicite usage.
					*/
					if ((q->uses & srcreg) && !(q->sets & srcreg)) {
						second = NULL;
						break;
					}
					if (uses_but_not_indexing(q) & dstreg) {
						second = NULL;
						break;
					}
					if (q->sets & dstreg) {
						second = NULL; /* disable */
						break;
					}
					if ((muses(q) & MEM) || (msets(q) & MEM)) memused = true;
					if ((uses_but_not_indexing(q) | q->sets) & srcreg) {
						if (!first_seen && !second) {
							unsigned int otherreg;
							second = q;
							otherreg = second->uses & ~(srcreg|dstreg);
							if ((otherreg & last->nlive) 
								||changed(second,last,otherreg)) {
								/* we cant move second because of changing
								** other reg used in second
								*/
								uniq_change = false;
								break;
							}
							/* might have to disable this second: */
							if (second->uses & dstreg ||
							((second->sets & ~CONCODES) != srcreg) ||
							(memused &&
							((muses(second) & MEM) || (msets(second) & MEM)))) {
								uniq_change = false;
								break;
							}
						} else {
							uniq_change = false;
							break;
						}
					}
				}
				if (!uniq_change || !second) {
					continue; /* do next movl */
				}
				if ((second->sets & CONCODES) && (last->nlive & CONCODES)) {
					continue; /* do next movl */
				}
				if (second->sets & srcreg && last->sets & srcreg)
					continue;
				limit = last->forw;
				for (q = p; q != limit; q = qforw) {
					qforw = q->forw;
					if (indexing(q) == dstreg) {
						if (scanreg(q->op1,false) == dstreg) m = 1;
						else m = 2;
						tmp = getspace(strlen(q->ops[m]));
						t = strstr(q->ops[m],p->op2);
						s = tmp;
						for (t1 = q->ops[m]; t1 != t; t1++)
							*s++ = *t1;
						*s = '\0';
						strcat(tmp,p->op1);
						t+=4;
						strcat(tmp,t);
						q->ops[m] = tmp;
						q->uses = uses(q);
						DELNODE(second);
						APPNODE(second,q);
					}
				}
				DELNODE(p);
				success = true;
				if (b->next) b->lastn = b->next->firstn->back;
			}/*endif mov %reg,%reg*/
		}/*for all p in b */
		for (p = b->firstn; p != b->lastn->forw; p = p->forw) {
			if (LEAL == p->op) {
				dstreg = setreg(p->op2);
				idxreg = indexing(p);
				doit = true;
				last = NULL;
				if (idxreg & dstreg) continue;
				for (q = p->forw; q != b->lastn->forw; q = q->forw) {
					if (indexing(q) & dstreg) last = q;
				}
				if (last == NULL) continue;
				if (last->nlive & dstreg) continue;
				for (q = p->forw; q != last->forw; q = q->forw) {
					if (q->sets & (dstreg | idxreg)) {
						doit = false;
						break;
					}
					if (q->uses & dstreg) {
						if (scanreg(q->op1,0) & dstreg) m = 1;
						else if (scanreg(q->op2,0) & dstreg) m = 2;
						else {
							doit = false;
							break;
						}
						if (!iszoffset(q->ops[m],p->op2)) {
							doit = false;
							break;
						}
					}
				}
				if (doit) {
					for (q = p; q != last->forw; q = q->forw) {
						for (m = 1; m <= 2; m++) {
							if (iszoffset(q->ops[m],p->op2)) {
								q->ops[m] = p->op1;
								new_sets_uses(q);
							}
						}
					}
					DELNODE(p);
					success = true;
				}
			}/*endif leal*/
		}
	}/*end block*/
	if (success) ldanal();
}/*end bbopt*/

static unsigned int regmasx[] = {EAX,EDX,EBX,ECX,ESI,EDI,EBI};
const int first_reg = 0, last_reg = 6;

#if 0
/* common super expression: several expressions are common to two consequtive
** basic blocks. The type of expression looked for here is: a register is used,
** before that it is calculated. The same calc is done in the previus basic
** block, therefore all the calc is redundat.
*/
void 
cSe(firstb,lastb) BLOCK *firstb,*lastb;
{
BLOCK *b;
	/*fprintf(stderr,"ebb of "); fprinst(firstb->firstn); */
	/*for (b = firstb; b != lastb->next; b = b->next) {*/
		/*fprinst(b->firstn); */
	/*}*/
	/*fprintf(stderr,"that's all blocks\n");*/
	for (b = firstb; b != lastb; ) {
		if (b->next == NULL) {
			/*fprintf(stderr,"no next block\n");*/
			return;
		}
		do_cSe(b,b->next);
		if (b->next == lastb) {
			/*fprintf(stderr,"next is last\n");*/
			return;
		}
		if (b->next->next == NULL) {
			/*fprintf(stderr,"no other two blocks\n");*/
			return;
		}
		b = b->next;
	}
}/*end cSe*/

char *itoreg();

void
do_cSe(b1,b2) BLOCK *b1,*b2;
{
NODE *p,*p2,*q;
unsigned int usereg;
int reg;
	/*fprintf(stderr,"block 1 of "); fprinst(b1->firstn); */
	/*fprintf(stderr,"block 2 of "); fprinst(b2->firstn); */
	p2 = b2->lastn;
	/* find an instruction that only uses one single register near the
	** end of the basic block;
	*/
	for ( ; p2 != b2->firstn->back; p2 = p2->back) {
		if (p2->uses & (REGS &~CONCODES)) break;
	}
	if (! (p2->uses & (REGS &~CONCODES))) {
		/*fprintf(stderr,"no reg usage in the block\n");*/
		return;
	}
	usereg = 0;
	for (reg = first_reg; reg <= last_reg; reg++) {
		if ((regmasx[reg] & p2->uses) == p2->uses) {
			usereg = p2->uses;
			break;
		}
	}
	if (usereg == 0) {
		/*fprintf(stderr,"no use of a single reg "); fprinst(p2); */
		return;
	}
	/*fprintf(stderr,"usereg is %x   ",usereg); fprinst(p2); */
	/*fprintf(stderr,"usereg is %x %s\n",usereg,itoreg(usereg));*/
	if (b1->lastn->nlive & usereg) {
		/*fprintf(stderr,"usereg live at b2 entry"); fprinst(b2->firstn); */
		return;
	}
	for (p = p2; p != b2->lastn->forw; p = p->forw) {
		if (p->sets & usereg) {
			/*fprintf(stderr,"usereg is set after it is last used\n");*/
			return;
		}
	}
	for (p = p2; p != b2->firstn->back; p = p->back) {
		if (p->sets & usereg) break;
	}
	if (p == b2->firstn->back) {
		/*fprintf(stderr,"usereg is not set in b2\n");*/
		return;
	}
	/* p2 points to the useage of our register, p points to it's last setting*/
	p2 = p;
	/* find the last setting of this register in b1 */
	for (q = b1->lastn; q != b1->firstn->back; q = q->back) {
		if (q->sets & usereg) break;	
	}
	if (q == b1->firstn->back) {
		/*fprintf(stderr,"no set of usereg in b1\n");*/
		return;
	}
	/* p and q point to the last seting of usereg in b2 and b1, repectrivelly
	/* go back over the blocks and compare the instructions.
	** do not eliminate call instructions.
	** do not eliminate setting of memory.
	*/
	for ( ; q != b1->firstn->back; q = q->back, p = p->back) {
		if (p->op == CALL || q->op == CALL || !same_inst(p,q)
			|| (msets(p) & MEM)) {
			/*fprintf(stderr,"two nonsane ones: "); fprinst(p); fprinst(q); */
			return;
		}
	}
	 if (p != b2->firstn->back) {
		return;
	 }
	/*fprintf(stderr,"all good, delete them\n");*/
	for ( ; p2 != b2->firstn->back; p2 = p2->back) DELNODE(p2);
}/*end do_cSe*/
#endif

/* This module keeps track of zero values in registers. If there
** is a zero value in a register and it is used as an index, then
** change the operand so that the register will not be used.
** In addition, some optimizations are possible. If the constant
** 0 is used, replace it by a register holding zero.
** If zero value is moved from one register to another, which already
** contains zero, delete the node.
*/
extern BLOCK b0;
extern int suppress_enter_leave;
static char *registers[] =
	{ "%eax", "%edx", "%ebx", "%ecx", "%esi", "%edi" , "%ebi" };
#define Mark_z_val(reg) zero_flags |= reg
#define Mark_non_z(reg) zero_flags &= ~(reg)
#define Have_z_val(reg) (zero_flags && (zero_flags | (reg)) == zero_flags)
static int nregs;

/* full_reg() will return the 32 bit register that contains the 
input parameter */
unsigned int
full_reg(i) unsigned int i;
{
  switch (i) {
	case EAX: return EAX; 
	case EDX: return EDX; 
	case EBX: return EBX;
	case ECX: return ECX; 
	case ESI: return ESI; 
	case EDI: return EDI;
	case EBP: return EBP;
	case ESP: return ESP;
	case AX:  return EAX;
	case DX:  return EDX;
	case BX:  return EBX;
	case CX:  return ECX;
	case SI:  return ESI; 
	case DI:  return EDI; 
	case AH:  return EAX; 
	case DH:  return EDX; 
	case BH:  return EBX; 
	case CH:  return ECX; 
	case AL:  return EAX; 
	case DL:  return EDX; 
	case BL:  return EBX; 
	case CL:  return ECX; 
	case EBI: return EBI; 
	case BI:  return EBI; 
	case BP:  return EBP; 
  /* NOTREACHED */
  }/*end switch*/
} /*end full_reg*/

/* itoreg() converts register bits to register string */
char *
itoreg(i) unsigned int i;
{
  switch (i) {
	case EAX:  return "%eax";
	case EDX:  return "%edx";
	case EBX:  return "%ebx";
	case ECX:  return "%ecx";
	case ESI:  return "%esi";
	case EDI:  return "%edi";
	case EBP:  return "%ebp";
	case ESP:  return "%esp";
	case AX:  return "%ax";
	case DX:  return "%dx";
	case BX:  return "%bx";
	case CX:  return "%cx";
	case SI:  return "%si";
	case DI:  return "%di";
	case AH:  return "%ah";
	case DH:  return "%dh";
	case BH:  return "%bh";
	case CH:  return "%ch";
	case AL:  return "%al";
	case DL:  return "%dl";
	case BL:  return "%bl";
	case CL:  return "%cl";
	case EBI: return "%ebi";
	case BI:  return "%bi";
  /* NOTREACHED */
  }/*end switch*/
}/*end itoreg*/

char *
itohalfreg(i) unsigned int i;
{
  switch (i) {
	case EAX:  return "%ax";
	case EDX:  return "%dx";
	case EBX:  return "%bx";
	case ECX:  return "%cx";
	case ESI:  return "%si";
	case EDI:  return "%di";
	case EBP:  return "%bp";
	case AX:  return "%ax";
	case DX:  return "%dx";
	case BX:  return "%bx";
	case CX:  return "%cx";
	case SI:  return "%si";
	case DI:  return "%di";
	case BP:  return "%bp";
	case AH:  return "%ax";
	case DH:  return "%dx";
	case BH:  return "%bx";
	case CH:  return "%cx";
	case AL:  return "%ax";
	case DL:  return "%dx";
	case BL:  return "%bx";
	case CL:  return "%cx";
	case EBI: return "%bi";
	case BI:  return "%bi";
  /* NOTREACHED */
  }/*end switch*/
}/*end itoreg*/


char *
itoqreg(i) unsigned int i;
{
  switch (i) {
	case EAX:  return "%al";
	case EDX:  return "%dl";
	case EBX:  return "%bl";
	case ECX:  return "%cl";
	case AX:  return "%al";
	case DX:  return "%dl";
	case BX:  return "%bl";
	case CX:  return "%cl";
	case AH:  return "%ah";
	case DH:  return "%dh";
	case BH:  return "%bh";
	case CH:  return "%ch";
	case AL:  return "%al";
	case DL:  return "%dl";
	case BL:  return "%bl";
	case CL:  return "%cl";
	default:  return NULL;
  /* NOTREACHED */
  }/*end switch*/
}/*end itoqreg */

static unsigned int zero_flags = 0;

/*Does instruction set a register to zero.
**true only for long instructions, that set the whole register.
*/
static boolean
set_reg_to_0(p) NODE *p;
{
	if (!isreg(p->op2))
		return false;
	switch (p->op) {
		case ROLL: case RORL: case SALL:
		case ROLW: case RORW: case SALW:
		case ROLB: case RORB: case SALB:
		case SARL: case SHLL: case SHRL:
		case SARW: case SHLW: case SHRW:
		case SARB: case SHLB: case SHRB:
			if (Have_z_val(p->sets & ~CONCODES))
				return true;
			else
				return false;
		case XORL: case SUBL:
			if (samereg(p->op1,p->op2))
				return true;
			/* FALLTHRUOGH */
		case XORW: case SUBW:
		case XORB: case SUBB:
			if (Have_z_val(p->sets &~CONCODES)
			 && isreg(p->op1) && Have_z_val(p->uses))
				return true;
			if (!strcmp(p->op1,"$0")
			 && Have_z_val(p->sets & ~CONCODES))
				return true;
			return false;
		case IMULB: case IMULW: case IMULL:
			if (p->op3) {
				if (!strcmp(p->op1,"$0"))
					return true;
				if (Have_z_val(p->uses))
					return true;
				return false;
			}
			/*FALLTHROUGH*/
		case ANDL: case MULL:
			if (isreg(p->op1) && Have_z_val(p->uses))
				return true;
			if (!strcmp(p->op1,"$0"))
				return true;
			/* FALLTHROUGH */
		case ANDW: case MULW:
		case ANDB: case MULB:
			if (Have_z_val(p->sets & ~CONCODES))
				return true;
			return false;
		case ADDL: case ORL:
			if (((isreg(p->op1) && Have_z_val(p->uses))
			   || !strcmp(p->op1,"$0"))
			 && Have_z_val(p->sets))
				return true;
			else
				return false;
		case MOVB: case MOVW:
			if (! Have_z_val(p->sets))
				return false;
			/* FALLTHROUGH */
		case MOVL:
		case MOVZWL: case MOVZBL:
		case MOVSBL: case MOVSWL:
		case MOVZBW: case MOVSBW:
			if (!strcmp(p->op1,"$0"))
				return true;
			if (isreg(p->op1) && Have_z_val(p->uses))
				return true;
			return false;
		case LEAL:
			if (Have_z_val(p->uses) && !hasdisplacement(p->op1))
				return true;
			else 
				return false;
		default:
			return false;
	}/*end switch*/
/*NOTREACHED*/
}/*end set_reg_to_0*/


/*main function of the zero tracing module*/

static boolean
zvtr(firsti,lasti) NODE *firsti , *lasti;
{
unsigned int pidx;
NODE *p, *nextp = NULL; /*init to prevent lint */
int i,retval = false;
boolean enter;
  nregs =  suppress_enter_leave ? (NREGS-2) : (NREGS -1);
  zero_flags = 0; /*no reg is known to hold zero */
  for (p = firsti ; p != lasti->forw; p = nextp) {
	if (p->op == ASMS || is_safe_asm(p)) /*asm ends the ebb, skip the rest*/
		return retval;
#if 0
	if (start && last_func() && last_one()) {
		d++;
		if (d < start || d > finish) continue;
		else  fprintf(stderr,"%d ",d);
	}
#endif
	COND_SKIP(continue,"%d ",second_idx,0,0);
	nextp = p->forw;
	/*If immediate 0 is used in an instruction, we might be
	**able to replace it by a register known to hold zero.
	**do not do it for cmp, because if it is redundant, w2opt()
	**will delete it.
	**Next try the same for movb and movw, but then replace an $0 by, say, %al
	**only if %eax holds zero, not if %al holds zero. Dont go into tracking
	**values of parts of registers.
	*/
	if ((p->op == MOVL || p->op == PUSHL)
	  && strcmp(p->op1,"$0") == 0)
	  for(i = 0; i < nregs; i++)
		if (Have_z_val(regmasx[i])) {
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"make $0 a register: ");
				fprinst(p);
			}
#endif
		  p->op1 = registers[i];
		  p->uses |= regmasx[i];
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"became: ");
				fprinst(p);
			}
#endif
		  p->zero_op1 = true;
		  retval = true;
		  break;
		}
	if ((p->op == MOVW) && strcmp(p->op1,"$0") == 0)
	  for(i = 0; i < nregs; i++)
		if (Have_z_val(regmasx[i])) {
			p->op1 = itohalfreg(L2W(regmasx[i]));
			p->uses |= L2W(regmasx[i]);
			p->zero_op1 = true;
		  	retval = true;
		  	break;
		}
	if ((p->op == MOVB) && strcmp(p->op1,"$0") == 0)
	  for(i = 0; i < 4; i++)  /* 4 byte addressable registers. */
		if (Have_z_val(regmasx[i])) {
			p->op1 = itoqreg(L2B(regmasx[i]));
			p->uses |= L2B(regmasx[i]);
			p->zero_op1 = true;
		  	retval = true;
		  	break;
		}
	/*Remove useless arithmetic instructions, when the operand
	**is known to hold zero. Just change register to immediate
	**zero, and w1opt() will take care of the rest.
	*/
	if (isreg(p->op2))
		switch (p->op) {
			case LEAL: case LEAW:
				if (*p->op1 != '(' || !Have_z_val(p->uses))
					break;
				if (!Have_z_val(p->sets & ~ CONCODES)) {
#ifdef DEBUG
					if (fflag) {
						fprintf(stderr,"make lea a xor: ");
						fprinst(p);
					}
#endif
					chgop(p,XORL,"xorl");
					p->op1 = p->op2;
					p->uses = 0;
					retval = true;
#ifdef DEBUG
					if (fflag) {
						fprintf(stderr,"became: ");
						fprinst(p);
					}
#endif
				}
				/*FALLTHROUGH*/
			case ROLL: case RORL: case SALL:
			case ROLW: case RORW: case SALW:
			case ROLB: case RORB: case SALB:
			case SARL: case SHLL: case SHRL:
			case SARW: case SHLW: case SHRW:
			case SARB: case SHLB: case SHRB:
				if (Have_z_val(p->sets & ~CONCODES)) {
#ifdef DEBUG
					if (fflag) {
						fprintf(stderr,"delete: ");
						fprinst(p);
					}
#endif
					ldelin2(p);
					DELNODE(p);
					retval = true;
				}
				break;
/*
			case IMULL: case IMULW: case IMULB:
			case MULL: case MULW: case MULB:
			Amigo finds all the cases of mull by 0  
*/
			case ANDB: case ANDW: case ANDL:
			if (p->nlive & CONCODES)
				break;
			if ((isreg(p->op1) && Have_z_val(scanreg(p->op1,false)))
			 || !strcmp(p->op1,"$0"))
				if (Have_z_val(p->sets & ~CONCODES)) {
#ifdef DEBUG
					if (fflag) {
						fprintf(stderr,"delnode");
						fprinst(p);
					}
#endif
					DELNODE(p);
					retval = true;
				}
				break;
			case SUBB: case SUBW: case SUBL:
			case ADDB: case ADDW: case ADDL:
			case ORB: case ORW: case ORL:
				if (p->nlive & CONCODES)
					break;
				if (isreg(p->op1) && Have_z_val(scanreg(p->op1,false))) {
#ifdef DEBUG
					if (fflag) {
						fprintf(stderr,"delete arithmetic ");
						fprinst(p);
					}
#endif
					DELNODE(p);
					retval = true;
				} else if ((p->op != SUBB && p->op != SUBW && p->op != SUBL)
						 && Have_z_val(p->sets & ~ CONCODES)) {
#ifdef DEBUG
					if (fflag) {
						fprintf(stderr,"change arithmetic to mov");
						fprinst(p);
					}
#endif
					switch (OpLength(p)) {
						case ByTE: chgop(p,MOVB,"movb"); break;
						case WoRD: chgop(p,MOVW,"movw"); break;
						case LoNG: chgop(p,MOVL,"movl"); break;
					}
					p->sets &= ~CONCODES;
					p->uses &= ~p->sets;
					retval = true;
#ifdef DEBUG
					if (fflag) {
						fprintf(stderr,"became");
						fprinst(p);
					}
#endif
				}
				break;
		}/*end switch*/
	/*If there is an operand with a register as base or index, and
	**the register is known to hold zero value, then remove
	**the register from the operand.
	*/
	if (! (pidx = scanreg(p->op1,true))) 
		pidx = scanreg(p->op2,true);
	if (pidx)
		for(i = 0; i < nregs; i++)
			if ((pidx  & regmasx[i]) && Have_z_val(regmasx[i])) {
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"change from ");
					fprinst(p);
				}
#endif
				remove_register(p,regmasx[i],pidx);
				p->uses = uses(p);
				pidx &=  ~regmasx[i];
				retval = true;
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"to ");
					fprinst(p);
				}
#endif
			}


	/* if this inst is a cmp and the next one is a jump then try to */
	/* eliminate them.                                              */
	enter = true;
	if ((p->op == CMPL || p->op == CMPW || p->op == CMPB)
	&&  p->forw->op >= JA && p->forw->op <=JZ /*followed by a conditional jump*/
	&&  !(p->forw->nlive & CONCODES)	/* condition codes are not live */
	) { int n1,n2;
	   if (isnumlit(p->op1))
		n2 = atoi(p->op1+1);
	   else if (isreg(p->op1) && Have_z_val(scanreg(p->op1,false)))
		n2 = 0;
	   else enter = false;
	   if (isnumlit(p->op2+1))
		n1 = atoi(p->op2);
	   else if (isreg(p->op2) && Have_z_val(scanreg(p->op2,false)))
		n1 = 0;
	   else enter = false;
	   if (enter) {
		boolean willjump = true;

		DELNODE(p);
		switch (p->forw->op) {
			case JA: case JNBE:
				willjump = (unsigned)n1 > (unsigned)n2;
				break;
			case JAE: case JNB:
				willjump = (unsigned)n1 >= (unsigned)n2;
				break;
			case JB: case JNAE:
				willjump = (unsigned)n1 < (unsigned)n2;
				break;
			case JBE: case JNA:
				willjump = (unsigned)n1 <= (unsigned)n2;
				break;
			case JG: case JNLE:
				willjump = n1 > n2;
				break;
			case JGE: case JNL:
				willjump = n1 >= n2;
				break;
			case JL: case JNGE:
				willjump = n1 < n2;
				break;
			case JLE: case JNG:
				willjump = n1 <= n2;
				break;
			case JE: case JZ:
				willjump = n1 == n2;
				break;
			case JNE: case JNZ:
				willjump = n1 != n2;
				break;
			default: fatal(gettxt(":1392","MARC: jump is %d\n"), p->forw->op);
		}
		if (willjump)
			chgop(p->forw,JMP,"jmp");
		else
			DELNODE(p->forw);
			
		retval = true;
	   }
	}

	/*same as above for test - jcc */

	enter = true;
	if ((p->op == TESTL || p->op == TESTW || p->op == TESTB)
	&& p->forw->op >= JA && p->forw->op <= JZ /*followed by a conditional jump*/
	&&  !(p->forw->nlive & CONCODES)	/* condition codes are not live */
	) { int n1,n2;
	   if (isnumlit(p->op1))
		n2 = atoi(p->op1+1);
	   else if (isreg(p->op1) && Have_z_val(scanreg(p->op1,false)))
		n2 = 0;
	   else 
		enter = false;
	   if (isnumlit(p->op2))
		n1 = atoi(p->op2+1);
	   else if (isreg(p->op2) && Have_z_val(scanreg(p->op2,false)))
		n1 = 0;
	   else
		enter = false;
	   if (enter) {
		boolean willjump = true;
		int res = n1 & n2;

		DELNODE(p);
		switch (p->forw->op) {
			case JA: case JNBE:
				willjump = res != 0;
				break;
			case JAE: case JNB:
				willjump = true;
				break;
			case JB: case JNAE:
				willjump = res != 0;
				break;
			case JBE: case JNA:
				willjump = true;
				break;
			case JG: case JNLE:
				willjump = res > 0;
				break;
			case JGE: case JNL:
				willjump = res >= 0;
				break;
			case JL: case JNGE:
				willjump = res < 0;
				break;
			case JLE: case JNG:
				willjump = res <= 0;
				break;
			case JE: case JZ:
				willjump = res == 0;
				break;
			case JNE: case JNZ:
				willjump = res != 0;
				break;
			default: fatal(gettxt(":1392","MARC: jump is %d\n"), p->forw->op);
		}
		if (willjump)
			chgop(p->forw,JMP,"jmp");
		else
			DELNODE(p->forw);
			
		retval = true;
	   }
	}
/* END OF TREATMENT OF CMP, TEST */

	/*Find out that an instruction sets a register to zero
	**and mark it.
	**If possible, change the zero setting to be of the form
	** movl %eax,%ebx , where %eax is known to hold zero.
	**rmrdmv() will prevent this assignment if possible.
	*/
	if (set_reg_to_0(p)) {
		if (!(p->nlive & CONCODES) && (OpLength(p) == LoNG)) {
			for(i = 0; i < nregs; i++)
				if (Have_z_val(regmasx[i])
				   && regmasx[i] != (p->sets & ~CONCODES))
					break;
			if (i < nregs)  {
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"reduce to mov: ");
					fprinst(p);
				}
#endif
				chgop(p,MOVL,"movl");
				p->op1 = registers[i];
				p->uses = regmasx[i];
				p->sets &= ~CONCODES;
				if (p->op3) {
					p->op2 = p->op3;
					p->op3 = NULL;
				}
				retval = true;
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"became: ");
					fprinst(p);
				}
#endif
			}/*endif i < nregs*/
		} /* endif */
		Mark_z_val(p->sets & ~CONCODES);
#ifdef DEBUG
		if (fflag) {
			fprintf(stderr,"Mark z reg op2 ");
			fprinst(p);
		}
#endif
	} else {
	/*Find out that an instruction sets a register to non zero
	**and mark it.
	*/
		for(i = 0; i < nregs; i++)
			if (p->sets & regmasx[i]) {
				Mark_non_z(regmasx[i]);
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"Mark non z reg op2 ");
				fprinst(p);
			}
#endif
		}
	}
  }/*for all nodes*/
  return retval;
}/*end zvtr*/

/*do string operations to remove a register from the operand. activated
**if the register is used as either a base or index and is known to hold zero.
*/
static void
remove_register(p,reg,pidx) NODE *p; unsigned int reg; unsigned int pidx;
{
 int m;
 int length;
 char *tmpop;
 char *t;
	if (isreg(p->op1) || *p->op1 == '$' )
		m=2;
	else
		m=1;
	if (reg == pidx) {  /*if only one register in the indexing,*/
		t = p->ops[m];    /*then leave only the displacement.    */
		length = 0;
		while(*t != '(') {
			t++;
			length++;
		}
		tmpop = getspace((unsigned)length);
		(void) strncpy(tmpop,p->ops[m],length+1);
		tmpop[length] = (char) 0;
		p->ops[m] = tmpop;
	} else                       /*there are both base and index*/
	 /*this code assumes that the zero holding register appears
	 **only once in the operand.                             */
		if (isbase(reg,p->ops[m]))
			remove_base(&(p->ops[m]));
		else
			remove_index(&(p->ops[m]));
		if (*p->ops[m] == '\0' ) /* reference a null pointer!*/
			p->ops[m] = "0";
}/*end remove_register*/

int
isbase(reg,op) unsigned int reg; char *op; {
	return (reg & setreg(strchr(op,'(') + 1)); /*base register*/
}/*end isbase*/

void
remove_index(op) char **op;
{
char *tmpop;
char *t;
int length;
  t = strchr(*op,',');
  length = t - *op +2;
  tmpop = getspace((unsigned)length);
  (void) strncpy(tmpop,*op,length-2);
  if (tmpop[length -3] == '(')
	tmpop[length-3] = '\0';
  else {
	tmpop[length-2] = ')';
	tmpop[length-1] = '\0';
  }
  *op = tmpop;
}/*end remove_index*/

void
remove_base(op) char **op;
{
int scale;
char *tmpop,*t,*s;
	t = strchr(*op,')');
	t--;
	if (isdigit(*t))
		scale = 1;
	else
		scale = 0;
	tmpop = getspace(strlen(*op) - 4);
	t = tmpop;
	s = *op;
	while (*s != '%')
		*t++ = *s++;
	s+= 5-scale;
	while (*t++ = *s++);
	if (*(t - 2 ) == '(' )
		*(t -2 ) = '\0';	
	*op = tmpop;
}/*end remove_base*/

/* fix_live() perform live/dead analysis over a block.  */
static void
fix_live(first,last,reg_set,reg_reset)  NODE *first, *last; 
unsigned int reg_set,reg_reset; {
unsigned int live;
	if (last->back->forw != last)
		live =  last->nlive | last->uses; /* from try_backward() */
	else
		live =  (last->nlive | last->uses) & ( ~last->sets | last->uses);
	for (last = last->back; first->back != last; last = last->back) {
		last->nlive &=  ~(reg_reset | reg_set); /* clear old live */
		last->nlive |= ( reg_set & live); /* set new live */
		live = (last->nlive | last->uses) & ( ~last->sets | last->uses);
	}
}/*end fix_live*/
/* is_new_value() will return 1 if the dest register is set and not used in 
the instruction p. It will return 2 if we can convert the instruction to less
good instruction that sets the dest register without using it. 
for every other instruction it will return 0 
*/
static int
is_new_value(p,reg) NODE *p; unsigned int reg;
{	char *t;
	switch(p->op) {
		case MOVB:	case MOVW:	case MOVL:
		case MOVZBW: case MOVZBL: case MOVZWL:
		case MOVSBW: case MOVSBL: case MOVSWL:
		case CALL: case LCALL:
				return 1;
		case LEAW: case LEAL:
			if (! isdeadcc(p)) /* can't replace the LEAL */ 
				return 1;
			t = strchr(p->op1,')') -1;
			if (*t == 2 || *t == 4 || *t == 8) /* has scale */
				return 1;
			if  (p->uses == reg)  /* add offset to reg only */
				return 1;
			if (*p->op1 == '(') /* no offset */
				return 1;
			return 2;
		case ADDL:
			if (! isdeadcc(p))
				return 0;
			if ( isreg(p->op1)  /* can be leal [%reg1,%reg2],%reg2 */
			 || *p->op1 == '$')  /* 'addl $val,%reg' => 'leal val[%reg],%reg' */
				return 2;
			return 0;
		case SUBL: 
			if (! isdeadcc(p))
				return 0;
			if (*p->op1 == '$' && isdigit(p->op1[1]))
				return 2;  /* 'subl $val,%reg' => 'leal -val[%reg],%reg' */
			return 0;
		case DECL: case INCL:
			return isdeadcc(p) ?  2 : 0;
		case SHLL:
			if (*p->op1 == '$' && p->op1[2] == 0 && 
				(p->op1[1] == '1' || p->op1[1] == '2' || p->op1[1] == '3'))
			return isdeadcc(p) ?  2 : 0;
				  
		default:
			return 0;
	}/*end switch*/
	/*NOTREACHED*/
}/*end is_new_value*/

/* new_leal() will convert the instruction to an leal instruction. It will 
be used to convert instruction that uses and sets the same register to an 
instruction that uses some registers and sets other register. 
*/ 
static void
new_leal(p) NODE *p; {
	char *tmp;
	switch(p->op) {
		case ADDL:
			if ( isreg(p->op1))  { /* can be leal (%reg1,%reg2),%reg2 */
				tmp = getspace(12);
				sprintf(tmp,"(%s,%s)",p->op1,p->op2);
				p->op1 = tmp;
			} else  { 
				tmp = getspace(strlen(p->op1) + 8);
				sprintf(tmp,"%s(%s)",&p->op1[1],p->op2);
				p->op1 = tmp;
			}
			break;
		case SUBL: 
			tmp = getspace(strlen(p->op1) + 8);
			if (p->op1[1] == '-')
				sprintf(tmp,"%s(%s)",&p->op1[2],p->op2);
			else 
				sprintf(tmp,"-%s(%s)",&p->op1[1],p->op2);
			p->op1 = tmp;
			break;
		case INCL:
			p->op2 = p->op1;
			p->op1 = getspace(8);
			sprintf(p->op1,"1(%s)",p->op2);
			break;				
		case DECL: 
			p->op2 = p->op1;
			p->op1 = getspace(9);
			sprintf(p->op1,"-1(%s)",p->op2);
			break;				
		case SHLL:
			tmp = getspace(10);
			sprintf(tmp,"(,%s,%d)",p->op2, 1 << (p->op1[1] - '0'));
			p->op1 = tmp;
			break; 
	}
	chgop(p, LEAL, "leal");
}

#define is8bit(cp)  (cp && *cp == '%' && (cp[2] == 'l'|| cp[2] == 'h'))

/* This module eliminates redundant moves from register to
**  register. If there is a move from register r1 to register r2
**  and then a series of uses of r2 then remove the mov instruction
**  and change all uses of r2 to use r1.
**  Some conditions may disable this optimization. They are looked for
**  and explained in comments.
*/
static void
try_backward(cb,movinst,firsti,lasti) BLOCK *cb; NODE *firsti, *lasti, *movinst;
{
  NODE *q,*new_set_src = NULL, *firstset = NULL, *may_new_src = NULL;
  NODE *jtarget;
  BLOCK *cb1;
  char *tmp;
  int not8adr,new_set;
  unsigned int srcreg,dstreg,use_target;
  boolean   give_up = 0;
  int movsize = OpLength(movinst);
	if (movinst == firsti) /*  Can't go backward from the first instruction */
		return;
#ifdef DEBUG
	if (fflag) {
		fprintf(stderr,"%c move inst ",CC);
		fprinst(movinst);
	}
#endif
	srcreg = movinst->uses;
	dstreg = movinst->sets;
	not8adr = (dstreg & (ESI|EDI|EBI)) && (srcreg & (EAX|EBX|ECX|EDX));
	for (cb1 = cb, q = movinst->back; q != firsti->back; q = q->back) {
		if (q->op == ASMS || is_safe_asm(q))
			return;
		if (q == cb1->firstn->back)
			cb1 = cb1->prev;
		if ((q->uses & srcreg) && /*dont mess with implicit uses of registers*/
			((isshift(q) && (srcreg & CL) && (isreg(q->op1)))
			|| (hard_uses(q) & srcreg)
			|| (! ((scanreg(q->op1,false) & srcreg) 
			|| (scanreg(q->op2,false) & srcreg))))) {
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"%c funny uses, give up.\n",CC);
				fprinst(q);
			}
#endif
			give_up = true;
			break;
		}
		if ((q->sets & srcreg) /*dont mess with implicit set of registers*/ 
		&& ( setreg(dst(q)) != (q->sets & ~CONCODES)
		) 
		) {
			give_up = true;
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"implicit set. give up.");
				fprinst(q);
			}
#endif
			break;
		}
		if (OpLength(q) == ByTE
		  && not8adr  /* Can't replace %esi with %eax if %al is used */
		  && (  (is8bit(q->op1) && (setreg(q->op1) & srcreg)) 
			  || (is8bit(q->op2) && (setreg(q->op2) & srcreg)) 
			 ) 
		) {
			give_up = true;
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"esi edi vs byte. give up.");
				fprinst(q);
			}
#endif
			break;
		}

		if (isbr(q)) {
			if (cb1->nextr) {
				jtarget = cb1->nextr->firstn;
				while (islabel(jtarget))
					jtarget = jtarget->forw;
				use_target = jtarget->uses | jtarget->nlive;
				if ((jtarget->sets & srcreg) && ! (jtarget->uses & srcreg))
					use_target &= ~srcreg;
			} else {
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"Jump to undef location or end of block: ");
					fprinst(q);
				}
#endif
				give_up = true;
				break;
			}
			if	(!isuncbr(q) /*otherwise jtarget is undefined*/
				&& ( ( q->nlive & srcreg && use_target & srcreg)
				|| (q->nlive & dstreg && use_target & dstreg))
				) {
				give_up = true;
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"src live at branch, give up.");
					fprinst(q);
				}
#endif
				break;
			}
		}
/* If we can separate between the set and the use of the src we may do it.
   We can use the src register and sets the destination register
   for example "inc %src" can be "leal 1(%src),%dest" 
   If by doing it we will get less good instruction we will do it only
   if we can't do the mov reg,reg any other way */
		if(( q->uses & q->sets & srcreg) &&  
		   ((new_set = is_new_value(q,srcreg)) != 0)){
				if ( (! may_new_src) && new_set == 2 && movsize == LoNG) {
					may_new_src = q;
			} else if (new_set == 1 && movsize == LoNG) {
				firstset = new_set_src = q;
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"separate ");
					fprinst(q);
				}
#endif
				break;
			}
		}
/* Chack if we used full register and moved less then full register */
		if (q->uses & srcreg) {
			if ((movsize < LoNG) && 
				((((int) OpLength(q)) > movsize) || (indexing(q) & srcreg))
				 )  {
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"%c big use of src. give up.\n",CC);
					fprinst(q);
					}
#endif
				give_up = true;
				break;
			}
		} else if (q->sets & srcreg) {
			if ((q->sets & srcreg) != srcreg) { /* set less than reg */
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"%c set only part of src. \n",CC);
					fprinst(q);
				}
#endif
				continue;
			}
			if (((int) OpLength(q)) > movsize) {
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"%c set more then src. \n",CC);
					fprinst(q);
				}
#endif
				give_up = true;
				break;
			} else {
				firstset = q; /* A good targeting set of register */
				break; /* no further questions */
			}
		}
		if ((q->sets | q->uses | q->nlive) & dstreg) {
		   /*The old dst reg is set or used. Don't check it for the 
		     first source setting. So it must be the last test  */
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"%c dst is used or set. give up.\n",CC);
				fprinst(q);
			}
#endif
			give_up = true;
			break;
		}
	}/*end inner loop*/
	if (give_up) {
		if (may_new_src) { /* Go back to separate.  */
			firstset = new_set_src = may_new_src;
		}
		else {
#ifdef DEBUG
			if (fflag) {
					fprintf(stderr,"check point 1, have to give up.\n");
			}
#endif
			return;
		}
	} else 
		may_new_src = NULL;
	if ( ! firstset || (q == firsti->back)) {
#ifdef DEBUG
		if (fflag) {
			fprintf(stderr,"check point, no set in block.\n");
		}
#endif
		return;
	}
	if ( firstset->nlive & dstreg) {
#ifdef DEBUG
		if (fflag) {
			fprintf(stderr,"check point, srcreg is set and dstreg is needed.\n");
		}
#endif
		return;
	}
	if ( srcreg & movinst->nlive){ /* Replace forward and backward */
#ifdef DEBUG
			if (fflag) {
			fprintf(stderr,"%c try change forward",CC);
				fprinst(q);
			}
#endif
		tmp = movinst->op1;
		movinst->op1 = movinst->op2;
		movinst->op2 = tmp;
		movinst->uses = dstreg;
		movinst->sets = srcreg;
		if (! try_forward(cb,movinst,lasti)) {
			movinst->uses = srcreg;
			movinst->sets = dstreg;
			movinst->op2 = movinst->op1;
			movinst->op1 = tmp;
			return;
		}
	} else
		tmp = NULL;
	if (may_new_src && may_new_src->op != LEAL)
		new_leal(firstset);
	for (q = movinst->back; q != firstset->back; q = q->back) {
		if (q->uses & srcreg || q->sets & srcreg) { 
#ifdef DEBUG
			if (fflag) {
			fprintf(stderr,"%c finally change ",CC);
				fprinst(q);
			}
#endif
			if (q == new_set_src)
				replace_registers(q,srcreg,dstreg,2);
			else
				replace_registers(q,srcreg,dstreg,3);
			new_sets_uses(q);
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"%c to ",CC);
				fprinst(q);
			}
#endif
		}/*endif q->uses & srcreg*/
	}/*for loop*/
	if (! tmp) {
		ldelin2(movinst);
		DELNODE(movinst);
	} 
	fix_live(firstset,movinst,dstreg,srcreg);
	return;
}/*end try_backward*/

/*Test the conditions under which it is ok to remove the copy instruction.
**This is either if there is no instruction that make it illegal, and then
**the give_up flag is set, or if an alternative copy is met, and then the
**testing is stopped.
*/
static int
try_forward(cb,movinst,lasti)  BLOCK *cb; NODE *lasti, *movinst;
{
  NODE *q,*new_set_dst = NULL, *lastuse = NULL, *may_new_dest = NULL;
  NODE *jtarget;
  NODE *srcset = NULL;
  BLOCK *cb1;
  int not8adr,new_set;
  unsigned int srcreg,dstreg,use_target;
  boolean dst_is_changed = 0, srcset_now, give_up = 0;
  int movsize = OpLength(movinst);

#ifdef DEBUG
	if (fflag) {
		fprintf(stderr,"%c move inst ",CC);
		fprinst(movinst);
	}
#endif
	srcreg = movinst->uses;
	dstreg = movinst->sets;
	not8adr = (srcreg & (ESI|EDI|EBI)) && (dstreg & (EAX|EBX|ECX|EDX));
	/* go from the copy inst. to the end of the ebb, and do the checks */
	for (cb1 = cb, q = movinst->forw; q != lasti->forw; q = q->forw) {
#ifdef DEBUG
		if (fflag) {
			fprintf(stderr,"check ");
			fprinst(q);
		}
#endif
		if (q->op == ASMS || is_safe_asm(q))  /*disable */
			return 0;
		srcset_now = false; /*init*/
		if (q == cb1->lastn->forw)
			cb1 = cb1->next;
		/*dont mess with implicit uses of registers*/
		/*these uses can not be changed to use the second register*/
		if ((q->uses & dstreg) &&
			((isshift(q) && (dstreg & CL) && (isreg(q->op1)))
			|| (hard_uses(q) & dstreg)
			|| (! ((scanreg(q->op1,false) & dstreg) 
			|| (scanreg(q->op2,false) & dstreg))))) {
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"%c funny uses, give up.\n",CC);
				fprinst(q);
			}
#endif
			give_up = true;
			break;
		}
		/*If there is a usage of the destination register as a byte register*/
		/*and the source register is esi, edi or ebp then can not replace them*/
		if (OpLength(q) == ByTE
		  && not8adr
		  && (  (is8bit(q->op1) && (setreg(q->op1) & dstreg)) 
			  || (is8bit(q->op2) && (setreg(q->op2) & dstreg))) 
		) {
			give_up = true;
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"esi edi vs byte. give up.");
				fprinst(q);
			}
#endif
			break;
		}

		/*Check if the copy is needed at the destination of a jump*/
		if (isbr(q)) {
			/*find the destination of the jump, go beyond the label(s)*/
			/*and find what registers are live there. l/d anal needs a*/
			/*little correction here, add use bits.*/
			if (cb1->nextr) {
				jtarget = cb1->nextr->firstn;
				while (islabel(jtarget))
					jtarget = jtarget->forw;
				use_target = jtarget->uses | jtarget->nlive;
				/*In the following case, the register is marked, but it's*/
				/*previous value is irrelevant.                          */
				if ((jtarget->sets & dstreg) && ! (jtarget->uses & dstreg))
					use_target &= ~dstreg;
			} else { /*nextr == NULL -> jtarget == NULL */
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"Jump to undef location or end of block: ");
					fprinst(q);
				}
#endif
				give_up = true;
				break;
			}
			/*Two cases here not to be able to eliminate the copy: */
			/*1. If the dest register is live at a branch target, or */
			/*2. If the dest reg is changed and the source reg is needed */
			if	(!isuncbr(q) /*otherwise jtarget is undefined*/
			  && ((q->nlive & dstreg && use_target & dstreg)
			  || (dst_is_changed && q->nlive & srcreg
			  && use_target & srcreg))
			) {
				give_up = true;
#ifdef DEBUG
				if (fflag) {
					if (q->nlive & dstreg && use_target & dstreg)
						fprintf(stderr,"dstreg used at target.\n");
					if (dst_is_changed && q->nlive & srcreg
						&& use_target & srcreg)
						fprintf(stderr,"dst changed, src used at target\n");
					fprinst(jtarget);
					fprintf(stderr,"dst live at branch, give up.");
					fprinst(q);
				}
#endif
				break;
			}
		}/*endif isbr*/
/* If we can separate between the set and the use of the src we may do it.
   We can use the src register and sets the destination register
   for example "inc %src" can be "leal 1(%src),%dest" 
   If by doing it we will get less good instruction we will do it only
   if we can't do the mov reg,reg any other way */
		if(( q->uses & q->sets & dstreg) && 
		   ((new_set = is_new_value(q,dstreg)) != 0)){
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"uses & sets dst, new val ");
				fprinst(q);
			}
#endif
			if ( ! srcset) {
				if ( (! may_new_dest) && new_set == 2 && movsize == LoNG) {
					may_new_dest = q;
				} else if (new_set == 1 && movsize == LoNG) {
					lastuse = new_set_dst = q;
#ifdef DEBUG
					if (fflag) {
						fprintf(stderr,"separate, nfq ");
						fprinst(q);
					}
#endif
					break;
				}
			} else {
				give_up = true; /*give up*/
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"sets & uses dst, src is set, give up.");
					fprinst(q);
				}
#endif
				break; /* no further questions */
			}
		}/*endif q sets dest a new value*/
		if (q->sets & srcreg) {
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"q sets src ");
				fprinst(q);
			}
#endif
		   /*mark the state that src reg is set. */
			if (!srcset)
				srcset_now = true;
			srcset = q;
				
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"%c set src ",CC);
				fprinst(srcset);
			}
#endif
		}
		if (q->uses & dstreg) {
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"q uses dst ");
				fprinst(q);
			}
#endif
			if ((movsize < LoNG) && 
				((((int) OpLength(q)) > movsize) || (indexing(q) & dstreg))
				 )  {
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"%c big use of dst. give up.\n",CC);
					fprinst(q);
					}
#endif
				 give_up = true;
				 break;
			}
			lastuse = q;
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"/ lastuse "); fprinst(lastuse);
			}
#endif
			if (q->sets & dstreg) { 
				if (q->nlive & srcreg) {
					give_up = true; /*give up*/
#ifdef DEBUG
					if (fflag) {
						fprintf(stderr,"src live and dst set, give up ");
						fprinst(q);
					}
#endif
					break; /* no further questions */
				}
				dst_is_changed = 1;
			}
			if (((!srcset) || srcset_now ) && !(q->nlive & dstreg)
				 && (!(q->uses & srcreg) || !dst_is_changed)  ) {
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"src not set, dst not live, nfq");
					fprinst(q);
				}
#endif
				break; /* no further questions */
			}
			if (srcset) {
				/* src reg was changed, now dst is used, give up*/
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"%c uses dstreg and src set, give up.\n",CC);
					fprintf(stderr,"%c uses:",CC); fprinst(q);
				}
#endif
				give_up = true; /*give up*/
				break;
			}
		} else if (q->sets & dstreg) {
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"q no uses dst, q sets dst");
				fprinst(q);
			}
#endif
			if ((q->sets & dstreg) != dstreg) { /* set less than reg */
				dst_is_changed = 1;
				lastuse = q;
			} else {
				if (!lastuse)
					lastuse = q->back;
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"q ! use dst, q set all dst ");
					fprinst(q);
				}
#endif
				break; /* no further questions */
			}
		}
#ifdef DEBUG
		else {
			if (fflag) {
				fprintf(stderr,"q no uses no sets dst ");
				fprinst(q);
			}
		}
#endif
		if (q->uses & srcreg && dst_is_changed) {
			give_up = true;
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"src needed after dst is set.\n");
				fprinst(q);
			}
#endif
			break;
		}/*end if*/
	}/*end inner loop*/
	if (give_up) {
		/*try to recover from the give up: */
		if (may_new_dest) {
			lastuse = new_set_dst = may_new_dest;
			if (lastuse->op != LEAL)
				new_leal(lastuse);
		} else {

#ifdef DEBUG
			if (fflag) {
					fprintf(stderr,"check point 2, have to give up.\n");
			}
#endif
			return 0;
		}
	}/*endif giveup*/
	/*giveup was not set, but other conditions may prevent the optimization*/
	if ( ! lastuse) {
#ifdef DEBUG
		if (fflag) {
			fprintf(stderr,"check point, no last use last change.\n");
		}
#endif
		return 0;
	}
	if ((q == lasti->forw) && (lasti->nlive & dstreg)) {
#ifdef DEBUG
		if (fflag) {
			fprintf(stderr,"check point, dest live in the end.\n");
		}
#endif
		return 0;
	}
#ifdef DEBUG
	else if (fflag) {
		if (q != lasti->forw)
			fprintf(stderr,"iner loop broke before end.\n");
		if (!(lasti->nlive & dstreg))
			fprintf(stderr,"dest reg dont live in the end.\n");
	}
#endif
	if ( dst_is_changed && lastuse->nlive & srcreg) {
#ifdef DEBUG
		if (fflag) {
			fprintf(stderr,"check point, dstreg is changed and srcreg is needed.\n");
		}
#endif
		return 0;
	}
	for (q = movinst; q != lastuse->forw; q = q->forw) {
		if (q->uses & dstreg || q->sets & dstreg) { 
#ifdef DEBUG
			if (fflag) {
			fprintf(stderr,"%c finally change ",CC);
				fprinst(q);
			}
#endif
			if (q == new_set_dst)
				replace_registers(q,dstreg,srcreg,1);
			else
				replace_registers(q,dstreg,srcreg,3);
			new_sets_uses(q);
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"%c to ",CC);
				fprinst(q);
			}
#endif
		}/*endif q->uses & dstreg*/
	}/*for loop*/
	fix_live(movinst,lastuse,srcreg,dstreg);
	ldelin2(movinst);
	DELNODE(movinst);
	return true;
}/*end try_forward*/

static void
rmrdmv(cb,lasti) BLOCK *cb; NODE *lasti;
{
  NODE *p;
  NODE *firsti = cb->firstn;
#ifdef DEBUG
	if (fflag) {
		fprintf(stderr,"new ebb ");
		fprinst(cb->firstn);
		fprintf(stderr,"until ");
		fprinst(lasti);
	}
#endif
	for (p = firsti; p != lasti->forw; p = p->forw) {  /* find the copy inst. */
		if (p->op == ASMS)  /* disable for all the ebb if found an asm */
			return;
		if (p == cb->lastn->forw)  /* update current basic block */
			cb = cb->next;
		if (( p->op == MOVL || p->op == MOVB || p->op == MOVW )
		   && isreg(p->op1)
		   && isreg(p->op2)
		   && !samereg(p->op1,p->op2) /*dont do it for same register*/
		   && ! (p->sets == EBP) /*save time for move esp,ebp, it fails anyway*/
		 )  {
#if 0
					if (start && last_func() && last_one()) {
						d++;
						if (d < start || d > finish) continue;
						else  fprintf(stderr,"%d ",d);
					}
#endif
			COND_SKIP(continue,"%d ",second_idx,0,0);
			if ( ! try_forward(cb,p,lasti))  /* copy propagation */
				try_backward(cb,p,firsti,lasti);  /* targeting */
		 }
	}/*main for loop*/
}/*end rmrdmv*/

void
replace_registers(p,old,new,limit) 
NODE *p;
unsigned int old,new;
int limit;
{
  int m;
  int length;
  unsigned int net_dst;
  char *oldname, *newname;
  char *opst,*opst2,*opst3;
  for (m = (limit == 2 ) ? 2: 1 ; m <= limit; m++) {
	  if (p->ops[m] && (old & (net_dst = scanreg(p->ops[m],false) & full_reg(old)))) {
		if (net_dst == old) {
			oldname = itoreg(old);
			newname = itoreg(new);
		}
		else if (net_dst == L2W(old)) {
			oldname = itoreg(net_dst);
			newname = itoreg(L2W(new));
		}
		else if (net_dst == L2B(old)) {
			oldname = itoreg(net_dst);
			newname = itoreg(L2B(new));
		} 
		else if (net_dst == L2H(old)) {
			oldname = itoreg(net_dst);
			newname = itoreg(L2H(new));
		} 
		else if ( net_dst ==  full_reg(old)) {
			oldname = itoreg(net_dst);
			newname = itoreg(full_reg(new));
		}
		opst = getspace(strlen(p->ops[m]));
		opst2 = p->ops[m];
		p->ops[m] = opst;
		length = strlen(newname);
		while(*opst2 != '\0') {
		  if (*opst2 == *oldname
		   && strncmp(opst2,oldname,length) == 0) {
			  for(opst3 = newname; *opst3;)
				  *opst++ = *opst3++;
			  opst2 += length;
			  continue;
		  }
		  *opst++ = *opst2++;
		}
		*opst = '\0';
	  }/*endif*/
	}/*for loop*/
}/*end replace_registers*/

/* This group of functions  are for spilling elimination. A temp is a variable
that for every basic block is set before used. If there is a free register
the temp can be reallocated to this register. In this case rmrdmv() that should
come after this optimization will sometime alienate the temp reg completely.
If is a specific basic block temp is referenced only once (set only) it will
be removed.
*/  
/* First few function menage the operands declared REGAL by the compiler. */
typedef struct auto_reg
{
  int	offset;		/* offset of regal in the stack */
  struct auto_reg *reg_hash_next;	/* next outo reg in hash chain */
  unsigned bits; 	/* the live/dead register bit */
  int size;		   /* size of the quantity in bytes. */
  boolean	valid; /* we save regals as valid and aliases as invalid */
  boolean partof;  /* is this a high part of a 64 bit regal */
  int param2auto;  /* if it was a double param, it's location as an auto */
  int estim;	   /* for double params, payoff estimation to move them */
} AUTO_REG;

#define HASH(X)  (((-X) >> 2) & 0x7f) /* hash function is (-x/4)%128 */
AUTO_REG *regals[0x80];
static unsigned reg_bits = 1;
#define INVALID		-1

void
init_reg_bits()
{
	reg_bits = 1; /* function called from optim.c, variable is static. */
}

void
init_regals()
{
	int i;
	reg_bits = 1;
	for (i=0; i < 0x80; i++)
		regals[i] = NULL;
}/*end init_regals*/


/*This is a costing function for double size parameters: do we want to
**move them to the autos area. The move costs two machine cycles on the
**P5 and four on the i486. If a param is referenced in a loop then move
**it. Else it has to be references enough times to make it worth. A mis
**aligned access is three cycles, and a we can not know if an access to 
**a parameter is aligned or not, hence we analize as 1.5. Ergo on i486
**there should be more then 4 accesses and on the P5 more then two.
*/
#define	MAXWEIGHT	(MAXINT - 1000)
#define	WEIGHT		8
#define MAXLDEPTH	10
void
estimate_double_params()
{
int i,m,x;
AUTO_REG *r;
NODE *p;
int weight = 1;
int depth = 0;
	for (ALLN(p)) {
		if (p->op == LCMT)
			if (*p->opcode == 'H') {
     			++depth;
	  			if(depth <= MAXLDEPTH)
	    			weight *= WEIGHT;
			} else if (*p->opcode == 'E') {
	 			if(depth <= MAXLDEPTH)
	    			weight /= WEIGHT;
	  			--depth;
			}
		if (OpLength(p) == DoBL &&
			((m = 1, p->op1 && scanreg(p->op1,false) & EBP) ||
			(m = 2, p->op2 && scanreg(p->op2,false) & EBP))) {
			x = atoi(p->ops[m]);
			if (x > 0) {
				for (i=0; i < 0x80; i++)
					for (r= regals[i]; r != NULL ; r = r->reg_hash_next)
						if (r->size == DoBL && r->offset == x && r->valid)
							r->estim += weight;
				
			}
		}
	}
}/*end estimate_double_params*/

int
double_params()
{
int n=0;
int i;
AUTO_REG *r;
const int min = (ptm_opts || blend_opts) ? 2 : 4;
	for (i=0; i < 0x80; i++)
		for (r= regals[i]; r != NULL ; r = r->reg_hash_next)
			if ((r->offset > 0) && (r->size == DoBL)
				&& r->valid && r->estim > min)
				n++;
	return n;
}/*end double_params*/

int
is_double_param_regal(x) int x;
{
int i;
AUTO_REG *r;
const int min = (ptm_opts || blend_opts) ? 2 : 4;
	if (x < 0) return 0;
	for (i=0; i < 0x80; i++)
		for (r= regals[i]; r != NULL ; r = r->reg_hash_next)
			if ((r->offset == x) && (r->size == DoBL) 
				&& r->valid && r->estim > min)
				return r->param2auto;
	return 0;
}/*end is_double_param_regal*/

int
next_double_param()
{
static int x;
static int i=0;
static AUTO_REG *r,*r1=NULL;
int first = 1;
const int min = (ptm_opts || blend_opts) ? 2 : 4;
	for ( ; i < 0x80; i++) {
		if (first) {
			r = r1 ? r1 : regals[0];
			first = 0;
		} else {
			r = regals[i];
		}
		for ( ; r != NULL ; r = r->reg_hash_next)
			if ((r->offset > 0) && (r->size == DoBL) 
				&& r->valid && r->estim > min) {
				x =  r->offset;
				r1 = r->reg_hash_next;
				return x;
			}
	}
	return 0;
}/*end next_double_param*/

void
set_param2auto(param,autom) int param,autom;
{
int entry = HASH(param);
AUTO_REG *r;
	for (r = regals[entry]; r; r = r->reg_hash_next) {
		if (r->offset == param) {
			r->param2auto = autom;
			return;
		}
	}
	fatal("didnt find param to set auto\n");
}/*set_param2auto*/

/* Get regal and initialize it and add to the hash table.
** Called from parse_regal to save FP regals which are otherwise ignored,
** and from ratable, to save regals which were saved for raoptim().
** Called also from parse_alias to invalidate FP REGALs marked as ALIASes.
** Invalidate by set the parameter valid to false.
** No assumption on the order between the call from parse_alias and the call
** from parse_regal.
** If an FP regal with size 8 bytes is installed, it's higher half is also
** installed and marked as partof.
** partof is to utilize removal of integer operation working on halfs of the
** regal. Therefore getregal() which is activated from rm_tmpvars(), recogizes
** regals which are partof bigger regals, but isregal doesn't. isregal() is
** used from everywhere else, e.g. schedule(), loop_regal();
*/
void
save_regal(s,size,valid,partof) char *s; int size;
{
	AUTO_REG *r;
	int entry;
	char xtos[ADDLSIZE];
	int x = atoi((*s == '*') ? &s[1] : s); /* get offset from %ebp */

	entry = HASH(x);
	for (r= regals[entry]; r != NULL ; r = r->reg_hash_next)
		if (r->offset == x) /* If regal was found */ {
			if (!valid) r->valid = false;
			return;
		}
	r = GETSTR(AUTO_REG);
	r->offset = x;
	r->size = size;
	r->valid = valid;
	r->partof = partof;
	r->param2auto = 0;
	r->reg_hash_next = regals[entry]; /* link to the hash table */
	regals[entry] = r; 
	r->bits = 0;
	r->estim = 0;
	if (size == DoBL) {
		sprintf(xtos,"%d",x+4);
		save_regal(xtos,LoNG,valid,true);
	}
}

/* look for regal in hash table x == offset from %ebp
** Return true only if found the offset, it is valid and not partof a
** bigger REGAL.
*/
int
isregal(x) int x;
{
  int entry;
  AUTO_REG *r;

  if (x == 0)
	return NULL;
  
  entry = HASH(x);
  for (r= regals[entry]; r != NULL ; r = r->reg_hash_next)
	if (r->offset == x) /* If regal was found */
		if (r->valid && !r->partof)
			return true;
		else
			return false;
  return false;
}

/* look for regal in hash table, one op has the form "[*]num(%ebp)".
** If found return the reg structure.
** But only if the REGAL found is valid, and was not invalidated by
** set_regal_bits(). This is done by setting INVALID to reg->bits, which
** invalidates for use here since there are not enough bits to do live-dead
** analisys. It stays regal for isregal.
*/
static AUTO_REG *
getregal(p) NODE *p;
{
	int entry;
	AUTO_REG *r;
	int x;
	char *s = NULL;

	if (p->op1 && isindex(p->op1,"%ebp"))
		s = p->op1;
	else if (p->op2 && isindex(p->op2,"%ebp"))
		s = p->op2;
	else return NULL;
	x =  (*s == '*') ? atoi(&s[1]) : atoi(s); 
	entry = HASH(x);
	for (r= regals[entry]; r != NULL ; r = r->reg_hash_next)
		if ((r->offset == x ) && r->valid && (r->bits != INVALID))
			return r; /* regal found return it */
	return NULL;
}


static AUTO_REG *
get_high_partof_regal(p) NODE *p; 
{
	int entry;
	AUTO_REG *r;
	int x;
	char *s = NULL;

	if (p->op1 && isindex(p->op1,"%ebp"))
		s = p->op1;
	else if (p->op2 && isindex(p->op2,"%ebp"))
		s = p->op2;
	else return NULL;
	x =  (*s == '*') ? atoi(&s[1]) : atoi(s); 
	x += 4;
	entry = HASH(x);
	for (r= regals[entry]; r != NULL ; r = r->reg_hash_next)
		if ((r->offset == x) && r->valid && (r->bits != INVALID))
			return r; /* regal found return it */
	return NULL;
}/*end get_high_partof_regal*/

static unsigned int
bits_of_high_part(p) NODE *p;
{
AUTO_REG *reg;
	reg = get_high_partof_regal(p);
	if (!reg)  return 0;
	if (reg->bits == 0) {
		if (reg_bits) {
			reg->bits = reg_bits;
			reg_bits <<= 1;
		} else {
			reg->bits = INVALID;
			return 0;
		}
	}
	return reg->bits;
}/*end bits_of_high_part*/

/* Set p->nrlive and p->nrdead to the bit correspondind to the REGAL operand
** in p. If no REGAL operand, set both to zero. If already processed 32 REGAL
** in the current function - no bits for more REGALS, invalidate all the coming
** ones.
** Invalidate a REGAL if it's address is taken. Workaround to a forsaken bug.
*/
void
set_regal_bits(p) NODE *p;
{
	AUTO_REG *reg = NULL;
	unsigned bits =0;
	if (p->op == ASMS) {
		p->nrlive = (unsigned) ~0;
		p->nrdead = 0;
		return;
	}
	if (!(reg = getregal(p))) {
		p->nrlive = 0;
		p->nrdead = 0;
		return;
	}
	if ( p->op == LEAL || p->op == LEAW || !reg_bits)
	{	reg->bits = INVALID; /* Mark this register as not valid */
#ifdef DEBUG
		if (fflag) 
			fprintf(stderr,"kill regal offset = %d\n", reg->offset);
#endif
		p->nrlive = 0;
		p->nrdead = 0;
		if (reg->size == DoBL) {
			if ((reg = get_high_partof_regal(p)) != NULL)
				reg->bits = INVALID;
		}
		return;
	}
	if (reg->bits)
		bits= reg->bits;
	else if (reg_bits)
	{	bits = reg->bits = reg_bits;
#ifdef DEBUG
		if (fflag) 
			fprintf(stderr,"regal offset= %d, bit is %8.8x\n",reg->offset,bits);
#endif
		reg_bits <<= 1;
	}
	if (OpLength(p) == DoBL) {
		bits |= bits_of_high_part(p);
	}
	if ( MEM & muses(p)) {
		p->nrlive = bits;
		p->nrdead = 0;
	}
	else if (MEM & msets(p)) {
		if (OpLength(p) != reg->size) {
			p->nrlive |= bits;
			p->nrdead = 0;
		} else {
			p->nrlive = 0;
			p->nrdead = bits;
		}
	}
	return;	
}

static unsigned used_first;
extern int asmflag;
unsigned use_first() {
 BLOCK *b = b0.next;
 NODE *firsti = n0.forw , *lasti = n0.forw,*p;
 unsigned use,set;
	if (asmflag)
		return used_first = ((unsigned) ~0);
	used_first = 0;
	while (lasti != &ntail) {
		firsti = b->firstn;
		do
			b = b->next;
		while (b && ! islabel(b->firstn));
		lasti = b ? b->firstn : &ntail;
		for (set = use = 0, p = firsti; p != lasti; p = p->forw) {
			if (p->nrlive & ~set) 
				use |= p->nrlive;
			else if (p->nrdead & ~use)
				set |= p->nrdead;
		}
		used_first |= use; 
	}/*while loop*/
	return used_first;
}/*end use_first */


/* use *%reg and not %reg in case ao function call */
static char *
itostarreg(i) unsigned int i; { /* *%reg is used by indirect call */
  switch (i) {
	case EAX:  return "*%eax";
	case EDX:  return "*%edx";
	case EBX:  return "*%ebx";
	case ECX:  return "*%ecx";
	case ESI:  return "*%esi";
	case EDI:  return "*%edi";
	case EBI:  return "*%ebi";
	case EBP:  return "*%ebp";
	case ESP:  return "*%esp";
  }/*end switch*/
  /* NOTREACHED */
}/*end itostarreg*/

boolean 
live_at(live,at) NODE *live,*at;
{
AUTO_REG *r = getregal(live);
	if (!r) return true; /* it is not a REGAL, assume it is live. */
	return (r->bits & at->nrlive);
}/*end live_at*/

static int
find_free_reg(p,b,reg_bit) NODE *p;BLOCK *b;
						unsigned reg_bit; {	/* find reg that is free from p
		 to last_p instruction block  */

	int first_reg = 0,last_reg = nregs;
	int i;
	AUTO_REG *r;
	unsigned int bits = 0;
	unsigned live = EAX | EDX | ECX | EBX | ESI | EDI | EBI;
	static int regs[] = { EAX,EDX,ECX,EBX,ESI,EDI,EBI};
	NODE *jtarget = NULL;
	if (suppress_enter_leave) nregs=6 , last_reg=6;
	else nregs = 7;
	if (! (p->nrdead & reg_bit)) /* /REGAL  was live before */
		return 0;
	if (isfp(p)) return 0;
	if (OpLength(p) == ByTE)
		last_reg = 4; /* no ESI or EDI */
	if (isuncbr(p) || islabel(p->forw)) /* last instruction should have deleted if not used */
		return 0;
	for(p = p->forw ; p != &ntail ; p = p->forw) {
		if (p->op == ASMS)
			return 0; 
		live &= ~(p->nlive | p->uses | p->sets);
		if (p->op == CALL || p->op == LCALL)
			first_reg = 3; 
		if ((reg_bit & used_first) && isbr(p)) {
			if (isuncbr(p) || (b->nextr == NULL && (b->nextl == NULL ||
				( !isuncbr(b->lastn))))) /* a return, or an unconditional indexed
										jump, or a switch. */
				return 0;
			if (b->nextr) {
				jtarget = b->nextr->firstn;
				while (islabel(jtarget))
					jtarget = jtarget->forw;
			}
			if ((r = getregal(jtarget)) != NULL) {
				bits = r->bits;
				bits |= bits_of_high_part(jtarget);
				if (((bits | jtarget->nrlive) & reg_bit) & ~jtarget->nrdead)
					return 0; /* used (and not killed) or live in jump target */
			}
			else if (reg_bit & jtarget->nrlive)
				return 0; /* live in jump target */
		}
		r = getregal(p);
		if (r) {
			bits = r->bits;
			bits |= bits_of_high_part(p);
		}
		if (r && (bits & reg_bit)) {	
			if (isfp(p))
				return 0;
			if (OpLength(p) == ByTE) /* If byte register is needed */
				last_reg = 4; /* no ESI or EDI */
		}
		if (!(reg_bit & p->nrlive))
			break;
		if (p == b->lastn) {
			if (isuncbr(p) || islabel(p->forw)) {
				return 0;
			} 
			b = b->next;
		} 
	}
	for (i = first_reg ; i < last_reg; i++)
		if ((regs[i] & live) == regs[i]){
#ifdef DEBUG
			if (fflag) { 
				fprintf(stderr,"last checked: ");
				fprinst(p);
			}
#endif
			return regs[i];
		}
	return 0;
}
	  
/* Link all temp in the block and then try to eliminate them one by one */
void
rm_tmpvars()
{
	NODE *p,*q;
	AUTO_REG *reg;
	unsigned int r;
	char *r_str,*r_str1;
	BLOCK *b;
	COND_RETURN("rm_tmpvars");
	bldgr(false);
	b = b0.next;
	regal_ldanal();
	for (p = n0.forw; p != &ntail; p = p->forw) {
		if (p == b->lastn->forw)
			b = b->next;
		reg = getregal(p); 
		if (! reg || reg->size == TEN) { /* does p uses regal, no long doubles */
			continue;
		}
		if ((! (reg->bits & p->nrlive)) && /* /REGAL is dead after */ 
			(! (p->sets & p->nlive)) && /*if register was set it was not used */
			(p->op !=CALL) && (p->op != LCALL)) { /* Not /REGAL MEM was set */ 
#if 0
				if (start && last_func() && last_one()) {
					d++;
					if (d < start || d > finish) continue;
					else  fprintf(stderr,"%d ",d);
				}
#endif
				COND_SKIP(continue,"%d ",second_idx,0,0);
				if (! isfp(p)) {     /* don't remove FP code */
#ifdef DEBUG
					if (fflag) { 
						fprintf(stderr," deleted reg->bits = %x ",reg->bits); 
						fprintf(stderr," p->nrlive = %8.8x \n",p->nrlive);
						fprinst(p);
					}
#endif
					ldelin2(p); /* temp was set and never used. */
					DELNODE(p); /* Remove it */
					continue;
				} else {  /* fp setting */
					if (p->op == FSTPL || p->op == FSTPS) {
						chgop(p,FSTP,"fstp");
						p->op1 = "%st(0)";
						continue;
					} else if (p->op == FSTL || p->op == FSTS) {
						DELNODE(p);
						continue;
					}
				}
		}
		if ((r = find_free_reg(p,b,reg->bits)) != NULL) { /* reg was found */ 
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"found  r = %s \n",itoreg(r));
				fprinst(p);
			}
#endif
			r_str = itoreg(r);
			for(q = p ; ; ) {
				if( reg == getregal(q)) { /* replace temp by reg */
					switch(OpLength(q)) {
						case ByTE:
							r_str1 = itoreg(L2B(r));
							break;
						case WoRD:
							r_str1 = itoreg(L2W(r));
							break;
						default:
							r_str1 = r_str;
					}
					if (isindex(q->op1,"%ebp")) {
						if (q->op == MOVSBW || q->op == MOVSBL || 
										q->op == MOVZBW || q->op == MOVZBL)
							q->op1 = itoreg(L2B(r));
						else if (q->op == MOVSWL || q->op == MOVZWL)
							q->op1 = itoreg(L2W(r));
						else if (q->op == CALL)
							q->op1 = itostarreg(r);
						else 
							q->op1 = r_str1;
					} else
						q->op2 = r_str1;
					new_sets_uses(q);
				}
				if (! (q->nrlive & reg->bits))
					break;
				else 
					q->nrlive &= ~reg->bits;
				q->nlive |= r; /* Mark reg as live */
				q= q->forw;
			}
		}
	}
}

/* new_offset will get a str for operand. Register name that is used as base 
 or index in str and the register content value in val. It will remove the 
 register from the operand and add the val to the displacement.
*/
static void
new_offset(str,reg,val)
char **str;
unsigned int reg;
int val;
{
int change = 0;
char name_buf[MAX_LABEL_LENGTH];
char *name;
int x,scale;
unsigned int base,index;
char *t,*rand,*fptr;
char sign;
	fptr = (**str == '*') ? "*" : ""; /*  function pointer call */
	t = strchr(*str,'(');
	base = setreg(1+t); /*base register*/
	t = strchr(t,',');
	if (t)
		index = setreg(t+1); /*index register*/
	else
		index = 0;
	name = (strlen(*str) < ((unsigned ) MAX_LABEL_LENGTH)) ? 
			                       name_buf : getspace(strlen(*str)); 
	(void) decompose(*str,&x,name,&scale); /*rest components*/
	if (index == reg) { /* Do index first. If not the index will become base */
		change = val * scale;
		remove_index(str);
	}
	if (base == reg) { /* The register is used as base */
		change += val;
		remove_base(str);
	}
	rand = getspace(strlen(*str) + 12);
	change +=x;
	t = strchr(*str,'(');
	if ( change == 0) { 
		if ( name[0] || t ) { 
			if (t)
				sprintf(rand,"%s%s%s",fptr,name,t);
			else
				sprintf(rand,"%s%s",fptr,name);
		} else 
			rand = "0";
	}
	else if (name[0]) {
		if (change > 0) sign = '+';
		else { 
			sign = '-';
			change = -change;
		}
		if (t)
			sprintf(rand,"%s%s%c%d%s",fptr,name,sign,change,t);
		else
			sprintf(rand,"%s%s%c%d",fptr,name,sign,change);
	} else {
		if (t)
			sprintf(rand,"%s%d%s",fptr,change,t);
		else
			sprintf(rand,"%s%d",fptr,change);
	}
	*str = rand;
}/*end new_offsets*/

static int 
reg_index(reg) unsigned int reg;
{
	switch (reg) {
		case EAX: return 0;
		case EDX: return 1;
		case ECX: return 2;
		case EBX: return 3;
		case ESI: return 4;
		case EDI: return 5;
		case EBI: return 6;
		default:
			return 7;
	}
}

/* const_index will find base/index registers with constant value and will 
remove the register and add the value to the displacement */
void
const_index()
{
	NODE *p;
	unsigned int pidx;
	int i,i1,i2,m,dummy;
	int const_val[NREGS -1]; /* The integer value of the constant  */
	char  *const_str[NREGS -1]; /* The constant string  */
	static int regmasx[] = { EAX,EDX,ECX,EBX,ESI,EDI,EBI,0};
	boolean ispwrof2();
	COND_RETURN("const_index");

	for (p = n0.forw; p != &ntail; p = p->forw) {
		if (isuncbr(p) || islabel(p) || p->op == ASMS || is_safe_asm(p)) {
			for(i = 0; i < (NREGS -1); i++) { /* new EBB, reset all values */
				const_val[i] = 0;
				const_str[i] = NULL;
			}
			continue;
		}
#if 0
		if (start && last_func() && last_one()) {
			d++;
			if (d < start || d > finish) continue;
			else  fprintf(stderr,"%d ",d);
		}
#endif
		 COND_SKIP(continue,"%d ",second_idx,0,0);
		 /* If idiv by constant can be removed by w3opt mark the register */
		if (p->op == CLTD
		  && (p->forw->op == IDIVL) 
		  && isreg(p->forw->op1)
		  && ispwrof2(m = const_val[reg_index(setreg(p->forw->op1))],&dummy)
		  && (p->back->op != MOVL || strcmp(p->back->op2, p->forw->op1)))
		 {
			NODE * pnew, *prepend();
			pnew = prepend(p,p->op2);
			chgop(pnew, MOVL, "movl");
			pnew->op1 = getspace(ADDLSIZE);
			sprintf(pnew->op1,"$%d",m); 
			pnew->op2 = p->forw->op1;
			new_sets_uses(pnew);
			pnew->nlive |= pnew->sets; 
			lexchin(p,pnew);		/* preserve line number info */ 
		}
		if (isconst(p->op1) ) {
			 if (isshift(p))  {
				if ( isdigit(p->op1[1]) && isreg(p->op2) &&
				     ( i = const_val[reg_index(setreg(p->op2))])) {
				     i1 = strtol(p->op1+1,NULL,0);
					switch (p->op) {
						case SHLL: 
							i = i << i1;
							break;
						case SHRL: 
							i = i >> i1;
							break;
						default:
							i = -1;
					}
					if ( i != -1 ) {
#ifdef DEBUG
						if (fflag) {
							fprinst(p);
							fprintf(stderr,"shift %d by constant %s: ",
								const_val[reg_index(setreg(p->op2))],p->op1 );
						}
#endif
						chgop(p, MOVL, "movl");
						p->op1 = getspace(ADDLSIZE);
						sprintf(p->op1,"$%d",i); 
						p->uses = 0;
						p->sets = setreg(p->op2);
#ifdef DEBUG
						if (fflag) {
							fprinst(p);
						}
#endif
					}
				}
			} else if (! p->op3 && !(p->sets & ESP))
			/* can't replace const in imull with op3, dont want to complicate
			** settings of esp, make fp_elimination harder.
			*/
				for ( i = 0 ; i < (NREGS-1) ; i++) {
					if ( OpLength(p) == ByTE && (i > 3)) 
						break; /* Must use byte register */
					if (const_str[i] && (! strcmp(p->op1,const_str[i]))) {
						unsigned int reg;
						switch (OpLength(p)) {
							case ByTE: reg = L2B(regmasx[i]); break;
							case WoRD: reg = L2W(regmasx[i]); break;
							case LoNG: reg = regmasx[i];      break;
						}
						p->op1 = itoreg(reg);
						p->uses |= reg; 
#ifdef DEBUG
						if (fflag) {
							fprintf(stderr,"replace %s by %s in ",const_str[i],p->op1);
							fprinst(p);
						}
#endif
					}
				}
		} 
		if (! (pidx = scanreg(p->op1,true))) {
			pidx = scanreg(p->op2,true);
			m = 2;
		} else
			m = 1;
		if (pidx) {
			for(i = 0; i < (NREGS -1); i++) {
				if ((pidx  & regmasx[i]) && const_val[i]) { 
#ifdef DEBUG
					if (fflag) {
						fprintf(stderr,"found reg %d with %d in: ",i,const_val[i]);
						fprintf(stderr," was p->ops[%d] = %s \n",m,p->ops[m]);
					}
#endif
					new_offset(&p->ops[m],regmasx[i],const_val[i]);
					new_sets_uses(p);
#ifdef DEBUG
					if (fflag) {
						fprintf(stderr," now p->ops[%d] = %s \n",m,p->ops[m]);
						fprinst(p);
					}
#endif
				}
			}
		}

		if (p->op == LEAL  && (! strchr(p->op1,'(')) 
		/* &&  ( p->op1[1] == '-' || isdigit(p->op1[1])) */ ) {
				char *tmp;
				chgop(p, MOVL, "movl");
				tmp = p->op1;
				p->op1 = getspace(strlen(tmp) + 2); /* 1 for '$' and 1 for \0 */
				sprintf(p->op1, "$%s", tmp);
				p->uses = 0;
				p->sets = setreg(p->op2);
		}
		if (p->op == MOVL && isconst(p->op1)
			&& isreg(p->op2)) { /* found movl $val,%reg */
			const_str[i1=reg_index(p->sets)] = p->op1;
			if ( p->op1[1] == '-' || isdigit(p->op1[1])) 
				const_val[i1] = atoi(p->op1+1); /* save val */
			else
				const_val[i1] = 0;
		} else if ((p->op == MOVL && isreg(p->op1) && isreg(p->op2))
			&& (! ((p->sets | p->uses) & (EBP | ESP))))  {
				const_str[i1 = reg_index(p->sets)] = 
									const_str[i2 = reg_index(p->uses)];
				const_val[i1] = const_val[i2];
		} else { /* kill value for set registers */
			for ( i = 0 ; i < (NREGS -1) ; i++) {
				if (p->sets & regmasx[i]) {
					const_val[i] = 0;
					const_str[i] = NULL;
				}
			}
		}
	}
}/*end const_index*/



#define NEXTBL(b)    b->nextl != NULL
/*  setcond replaces jumps with setcc in cases of this sort :
 **  a = (b < c) 0:1
 **  This is usualy translated to a jcc sequence.
 **  setcond replaces the jcc with setcc 
 **  The block structure saught after is this :
 **   b1 --------> b2 ------>b3
 **   |                       ^
 **    -> b4 -----------------|
 **  Two possibilities are checked : fall through to b2 and b3 or fall through
 **  to b2 and a jmp tp b3 .
 **   
 */
 /* Does the jcc depend exclyusivelly on the carry flag: that's the question*/
static boolean
isadc(opc1) int opc1;
{
	if (opc1 == JAE || opc1 == JB || opc1 == JC ||
		opc1 == JNAE || opc1 == JNB || opc1 == JNC)
		return true;
	else
		return false;
}
/* A jcc is convertible to setcc only if the regsiter to be set is byte 
** addressable. It is convertable to adc for any register since adc can work
** on all sizes.
*/
static int
regis(opn,copm)	char *opn;	int	copm; 
{
	if ((setreg(opn) & (EAX | EBX | ECX | EDX)) || isadc(copm))
		return setreg(opn);
	return 0;

}
/* Is the opcode a move or xor - that clears a register */
static int
is_clearing_op(opci) int opci; 
{
  return((opci == MOVL) || (opci == MOVB) || (opci == MOVW) ||
         (opci == XORL) || (opci == XORB) || (opci == XORW));
      
} 
static boolean
check1(opc2,opc4) int opc2,opc4;
{
    return((opc2 ==XORL) && (opc4== MOVL)  ||
            (opc2 ==XORB) && (opc4== MOVB) ||
            (opc2 ==XORW) && (opc4== MOVW))  ;
}

void
setcond()
{
extern BLOCK b0;
BLOCK *b1,*b2,*b3,*b4;
NODE *p1,*p2,*pnew1,*pnew2;
int opc1,opc2,opc4;
boolean b4_sets;
int resreg;
boolean GoThru;
int do_for_i486;
char *regw;

	COND_RETURN("setcond");
	bldgr(false); 
	for(b1 = b0.next; b1 ; b1 = b1->next) {
		b4_sets = false;
		GoThru=false;

		/* The enter condition contains 2 main parts, the second is entered
		** if and only if the first fails   */

		if (! (((b4=b1->nextr) != NULL)   /* Looking for the right structure */
			&&((b2=b1->nextl) != NULL)
			&&((b3=b2->nextl) != NULL)))
							goto Ncase;	             

		if (! ((b4->nextl == b3)
			&&(b2->length == 1)          /* Block 2 must contain one inst. */ 
			&&(b4->length == 2)))        /* Block 4 must contain 2 insts.  */
							goto Ncase;			    

		if (! ((b4->lastn->op == JMP)    /* Last inst in p4 is a jmp      */
			&& is_clearing_op(opc2=b2->lastn->op)  /* OpCodes are mov and xor */
			&& is_clearing_op(opc4=b4->firstn->forw->op)))
							goto Ncase;	


	                                     /* Operand 2 in both first inst */
                                         /*  is a register that has Rl & Rh  */
		if (! (isreg(b2->firstn->op2)    
			&& isreg(b4->firstn->forw->op2)
			&& !strcmp(b2->firstn->op2,b4->firstn->forw->op2)
			&&((opc1=b1->lastn->op) != JCXZ)
                                        /* JCXZ doesn't have matching setcc  */
			&&(resreg = regis(b2->lastn->op2,opc1))))
			            	goto Ncase;	                                    
	
	
	       /* The final condition is :
	          the first assignment is a zero and the second other const
	          or vice-versa. It could be put to long/word/byte         */  
	
		if(	(b4_sets = (check1(opc2,opc4) &&
						!strcmp(b4->firstn->forw->op1,"$1")))   ||
					(check1(opc4,opc2) &&
						!strcmp(b2->firstn->op1,"$1"))          ||
					(opc2 ==MOVL) && (opc4== MOVL) && 
						((b4_sets =  (!strcmp(b2->firstn->op1,"$0") && 
									!strcmp(b4->firstn->forw->op1,"$1"))) ||
									!strcmp(b4->firstn->forw->op1,"$0") &&
									!strcmp(b2->firstn->op1,"$1"))
	          ) 
					GoThru = true;	          
	
	/* The second part of the enter condition                               */

	Ncase:
	if(!GoThru) {

		if (! (((b2=b1->nextr) != NULL)   /* Looking for the right structure */
			&&((b4=b1->nextl) != NULL)
			&&((b3=b2->nextl) != NULL)
			&& NEXTBL(b4)))   
							continue;			    
	
		if (! ((b4->nextl == b3)
				&&(b2->length == 1)      /* Block 2 must contain one inst */ 
				&&(b4->length == 2)))    /* Block 4 must contain 2 inst  */
							continue;				
		if (! ((b4->lastn->op == JMP)    /* Last inst in p4 is a jmp      */
				&& is_clearing_op(opc2=b2->lastn->op)
				&& is_clearing_op(opc4=b4->firstn->op)))
							continue;				

										/* Operand 2 in both first inst */
										/* of b4 and b2 is a register       */
	                                    /* It should be the same register   */
	                                    /* And one of the list checked      */
		if (! (isreg(b2->lastn->op2)      				
			&& isreg(b4->firstn->op2) 
			&& !strcmp(b2->lastn->op2,b4->firstn->op2)
			&& ((opc1=b1->lastn->op) != JCXZ)
	        && (resreg = regis(b2->lastn->op2,opc1))))
			                            /* JCXZ doesn't have matching setcc */
							continue;	                                    

		/* The final condition is :
				the first assignment is a zero and the second other const
				or vice-versa. It could be put to long/word/byte           */  

		if(	(check1(opc2,opc4) &&  
					(!strcmp(b4->firstn->op1,"$1")))              ||
				(b4_sets = (check1(opc4,opc2)  &&
					(!strcmp(b2->lastn->op1,"$1"))))      ||
				(opc2 ==MOVL) && (opc4== MOVL) && 
					(!strcmp(b2->lastn->op1,"$0") && 
					!strcmp(b4->firstn->op1,"$1") ||
				(b4_sets=!strcmp(b4->firstn->op1,"$0") &&
					!strcmp(b2->lastn->op1,"$1"))))
	           GoThru = true;
		else   GoThru = false;	    
	}

	if (GoThru && ((do_for_i486 = isadc(opc1)) || blend_opts)) {
#if 0
		if (start && last_func() && last_one()) {
			d++;
			if (d < start || d > finish) continue;
			else  fprintf(stderr,"%d ",d);
		}
#endif
				COND_SKIP(continue,"%d ",second_idx,0,0);
				p1=b1->lastn->back;   /* p1 holds the compare statement */
				p2=b1->lastn;         /* p2 holds the jcc statement     */
				p1->nlive |= resreg;  /* Update live/dead data          */
				p2->nlive |= resreg;
				if(b4_sets)    /* Jcc is transfered to setcc     */
					br2set(p2); 
				else   
					br2nset(p2);  
				if	(do_for_i486) {		  /* Transfer to adc[bwl]        */
			    	pnew2=insert(p1);     /* Implement 'mov  $0,reg'     */
		        	pnew2->op1 = "$0";
		        	pnew2->op2 = b2->lastn->op2;
					pnew2->sets = resreg;
		        	pnew2->uses = 0;
        			pnew2->nlive = p1->nlive | resreg | CONCODES;
					switch (OpLength(b2->lastn)) {
						case ByTE:
		        			chgop(pnew2,MOVB,"movb");
							break;
						case WoRD:
		        			chgop(pnew2,MOVW,"movw");
							break;
						case LoNG:
		        			chgop(pnew2,MOVL,"movl");
							break;
					}
		        	switch (p2->op) {  /* change setcc to sbb or adc */
			    		case SETAE : case SETNB : case SETNC : 
							switch (OpLength(b2->lastn)) {
								case ByTE:
									chgop(p2,SBBB,"sbbb");
									break;
								case WoRD:
		        					chgop(p2,SBBW,"sbbw");
									break;
								case LoNG:
		    						chgop(p2,SBBL,"sbbl");
									break;
							}
							p2->op2 = pnew2->op2;     
							p2->op1 = "$-1";
							p2->uses = resreg | CONCODES;
							p2->sets = resreg | CONCODES;
							break;
						case SETB : case SETC : case SETNAE :
							switch (OpLength(b2->lastn)) {
								case ByTE:
									chgop(p2,ADCB,"adcb");
									break;
								case WoRD:
		        					chgop(p2,ADCW,"adcw");
									break;
								case LoNG:
		    						chgop(p2,ADCL,"adcl");
									break;
							}
							p2->op2 = pnew2->op2;     
							p2->op1 = "$0";
							p2->uses = resreg | CONCODES;
							p2->sets = resreg | CONCODES;
							break;
			    	}/*end switch*/
		      	} else {					  /*  Transfer to setcc byte    */
			    	if (OpLength(b2->lastn) > ByTE) {
		        		pnew2=insert(p1);     /* Implement 'mov  $0,reg'    */
			        	pnew2->op1 = "$0";
			        	pnew2->op2 = b2->lastn->op2;
						pnew2->sets = resreg;
		    	    	pnew2->uses = 0;
        				pnew2->nlive = p1->nlive | resreg | CONCODES;
						if(resreg & (Eax | Ebx | Ecx | Edx))
		        			chgop(pnew2,MOVL,"movl");
		        		else 
		        			chgop(pnew2,MOVW,"movw");
		        	}            
					regw = sreg2b(resreg);
					p2->op1 = regw;
					p2->sets = setreg(regw);
				}
				pnew1 = insert(p2);
				chgop(pnew1,JMP,"jmp");
				pnew1->op1 = b3->firstn->opcode;
				pnew1->op2 = NULL;
				pnew1->sets= pnew1->uses = 0;
			}/*endif do*/
	}/*end for b1*/
}/*end setcond*/

/* Zero propagation changes instructions that set a register to 0, to use
** a second register, known to hold zero, and make a movl %reg1,%reg2.
** This is done hoping that copy propagation will kill it.
** This function deals with the case that copy propagation didn't kill it:
** we are left with long live ranges and false dependencies. Change it
** back to xorl %reg2,%reg2.
*/
void
clean_zero()
{
	NODE *p;
	int found = 0;
	int i;

	COND_RETURN("clean_zero");
	nregs =  suppress_enter_leave ? (NREGS-2) : (NREGS -1);
	for (p = n0.forw; p != &ntail; p = p->forw) {
		if (isuncbr(p) || islabel(p) || p->op == ASMS || is_safe_asm(p))
  			zero_flags = 0; /*no reg is known to hold zero */
		if (set_reg_to_0(p)) {
			Mark_z_val(p->sets & ~CONCODES);
			if (!(p->nlive & CONCODES) && (OpLength(p) == LoNG)) {
				if (p->op != XORL ) {
#if 0
					if (start && last_func() && last_one()) {
						d++;
						if (d < start || d > finish) continue;
						else  fprintf(stderr,"%d ",d);
					}
#endif
					COND_SKIP(continue,"%d ",second_idx,0,0);
#ifdef DEBUG
					if (fflag) {
						fprintf(stderr,"reduce to xor: ");
						fprinst(p);
					}
#endif
					chgop(p,XORL,"xorl");
					p->op1 = p->op2 = itoreg(p->sets & ~CONCODES); 
					p->uses = 0;
					p->sets |= CONCODES;
					found = true;
					p->op3 = NULL;
#ifdef DEBUG
					if (fflag) {
						fprintf(stderr,"became: ");
						fprinst(p);
					}
#endif
				}
			}
		} else { 
			for(i = 0; i < nregs; i++)
				if (p->sets & regmasx[i])
					Mark_non_z(regmasx[i]);
		} /* endif */
	} /* for */
	if (found)
		ldanal();
}/*end clean_zero*/
