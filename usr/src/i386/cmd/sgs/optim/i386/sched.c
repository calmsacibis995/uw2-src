/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)optim:i386/sched.c	1.45"

#include <stdio.h>
#include <malloc.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include "sched.h"      /* includes "optim.h" and "defs" */
#include "optutil.h"
typedef NODE * NODEP;
typedef tdependent * tdependentp;
#define BLOCK_SIZE_LIMIT	100
typedef struct  { NODE *setting;
				  boolean isconstant;
				} aset;

static unsigned int regmasx[NREGS] = {EAX,EDX,EBX,ECX,ESI,EDI,EBP,ESP};
static int depths[NREGS]; /*eight registers, without CC */
static int fullregs[] = {Eax,Edx,Ebx,Ecx,Esi,Edi,Ebp};
static int halfregs[] = {AX,DX,BX,CX,SI,DI,BP};
static int justhalfs[] = {Ax,Dx,Bx,Cx};
static int qregs[] = { AL|AH , DL|DH , BL|BH , CL|CH };
static aset   setsR[NREGS];  /*eight registers and CC */
static NODEP  sets_by_var[NREGS];
static NODEP  usesR[NREGS];
static NODEP  setsH[7];      /*for 16 bit registers*/
static NODEP  setsQ[4];      /*for pairs of 8 bit registers*/
static NODEP  last_volatile;
static NODEP  next_of_prefix;
static tdependent  usesMEM, setsMEM;
static int concurrent_value = 0;
int vflag = 0;
static clistmem m0,m1,*fp0;
static NODE *prev;
static NODE * last_is_br;
static BLOCK * b;
static alist a0 = { NULL , NULL };
static alist *acur = &a0;
static int alloc_index=0;
static int float_used = 0;
static tdependent dummy_to_show_agi;
static NODE *last_float = NULL; /* Last FP instruction. Place fxch aftr FP 
                                  or as a filler for AGI and it will be 
                                  paired. (in most cases)                   */

#ifdef STATISTICS
int orig_agi =0, final_agi=0, agi_work =0;
int orig_conc =0, final_conc=0, conc_work =0;
int total_inst=0, on_X86=0, on_V=0, pairables =0;
int org_total=0,org_on_U=0,org_on_V=0,org_pairable=0;
int fin_total=0,fin_on_U=0,fin_on_V=0,fin_pairable=0;
static void do_stats();
static int count_agi(), count_concurrent(), son_of();
#endif

extern BLOCK b0;      /* declared in optim.c */
extern NODE  n0;      /* declared in optim.c */
extern int i486_opts;    /* declared in local.c */
extern NODE *insert();
extern int ptm_opts;    /* declared in local.c */
extern int blend_opts;    /* declared in local.c */
extern int fp_removed;	/* declared in local.c */
extern int double_aligned;	/* declared in local.c */
extern void set_refs_to_blocks();


/* functions declared in this module  */
static void  bldag(), prun(), init_block(), update_static_vars(),
			 installCCdep(), install_dependencies(), /*dis2prefix(),*/
			 nail(),  calc_depth(), calc_width(),
			 update_one(), isdependent(),
			 initnode(), relevants(),installM(),
			 make_clist(), update_clist(), dispatch(),
      		 pick1or2(), dispatch1or2(), remove1or2(),
			 check_fp_stack(), check_fp_block(),
			 reorder(), install1(),init_alloc(),
			 count_depths_before_reorder(), fix_offsets(),
			 add_fp_dependency();

static	int   before(), natural_order(), agi_depends(), depth(),
			  no_gain_in_schedule(),
			  depth_4list();
int		      seems_same(),decompose();
static	NODE  *pick_one();
static BLOCK  *find_big_block_end();
static unsigned int  base();
static void  set_fp_regs(), clean_fp_regs(), reset_fp_reg(),
			 fp_start(), fp_set_resource(), init_fregs(), pair(),
			 freefreg(), fp_ticks();
void	     check_fp();
static unsigned int fmsets(), fmuses();
static NODE *fp(), *EX86(), *islefthy(), *isright(), *Eunsafe_right(),
			*Eanti_dependent(), *Edependent();
static int find_weight(), fp_time(), check_for_fxch();

extern unsigned msets(), muses(), indexing();
static tdependent
* next_dep_str();

#define has_agi_dep(p) ((p)->agi_dep != NULL)
#define makes_new_agi(p) ((p)->usage == AGI)

void
schedule()        /* driver of the scheduler */
{
	if (! i486_opts && !blend_opts && ! ptm_opts)
		return;
	COND_RETURN("schedule");
#ifdef STATISTICS
	orig_agi += count_agi();
    if (! ptm_opts)
		orig_conc += count_concurrent();
#endif
	init_alloc();
	bldgr(true);
	if (ptm_opts)
		check_fp();
	for (b=b0.next; b != NULL ; b=b->next){ /*foreach block*/
		if (! ptm_opts && !blend_opts && no_gain_in_schedule()) continue;
    	init_block();
#if 0
		if (start && last_func() && last_one()) {
			d++;
			if (d < start || d > finish) continue;
			else  fprintf(stderr,"%d %s ",d,b->firstn->opcode);
		}
#endif
		COND_SKIP(continue,"%d %s ",second_idx,b->firstn->opcode,0);
     	/* scepial case: all labels at the beginning of the block */
     	/* must remain first. optput them.                        */
     	/* the labels are NOT counted by bldgr() into b->length.  */
     	while (islabel(b->firstn)) {
			initnode(b->firstn);
        	b->firstn = b->firstn->forw; /*advance the block's beginning */ 
     	}/*end while loop*/
		prev = b->firstn->back; /*last node before present block*/

     	/* second special case: if the last instruction is a      */
     	/* branch, then force it to be the last one. dependencies */
     	/* dont take care of this.                                */
		/* Do it also for a call to a .label which appears in Pic */
       	if (isbr(b->lastn) || is_safe_asm(b->lastn) || b->lastn->op == ASMS ||
			(b->lastn->op == CALL && b->lastn->op1[0] == '.')) {
       		last_is_br = b->lastn;
       		b->lastn = b->lastn->back;
       		b->length--;
	   		initnode(last_is_br);
	 	}/*endif*/

    	if (ptm_opts)
	  		set_fp_regs(b); /* replace float stack with regs */
    	else
	  		concurrent_value = 0;

		if (b->length )
    		bldag(); /*construct dependency graph*/
		count_depths_before_reorder(); /* mark values of index regs at nodes */
#ifdef STATISTICS
   		if (ptm_opts)
	 		do_stats(&org_total,&org_on_U,&org_on_V,&org_pairable);
#endif
		prun();  /*reorder the instructions*/
    	if (ptm_opts)
      		if (float_used)
   	 			clean_fp_regs(b);
    	if (last_is_br) { /*put it back*/
	  		b->lastn = last_is_br;
	  		prev = b->lastn;
#ifdef STATISTICS
	  		total_inst++;
	  		pairables++;
#endif
	  		if ((b->lastn->back->pairtype == WDA ||
		   		b->lastn->back->pairtype == WD1)
				&& b->lastn->op != ASMS
				&& !is_safe_asm(b->lastn)
	  			&& b->lastn->back->usage == ALONE) {   /*pairable, wasnt*/
				b->lastn->back->usage = ON_X;		   /*paired by sched,*/
		 		b->lastn->usage = ON_J;                /*paired with last*/
#ifdef STATISTICS
	     		on_V++;                            
#endif
	  		} else {
#ifdef STATISTICS
	    		on_X86++;
#endif
				b->lastn->usage = ALONE;
	  		}

		}
#ifdef STATISTICS
   		if (ptm_opts || blend_opts)
	 		do_stats(&fin_total,&fin_on_U,&fin_on_V,&fin_pairable);
#endif

 	}/*foreach block*/
#ifdef STATISTICS
 	if (! ptm_opts)
	{
    	final_agi += count_agi();
    	final_conc += count_concurrent();
 	}
#endif
	/*(void) count_agi();*/
}/*end schedule*/

static void
bldag()
/*construct dependency graph for the instructions of a basic block*/
{
NODE * p;
int i;
int dummy;
int found_unexpected = false;

	for (p= b->firstn; p != b->lastn->forw; p = p->forw) {
    	initnode(p);        /* initialize fields in instruction NODEs */

		if (isprefix(p)) {
			p->uses |= muses(p->forw);
			p->sets |= msets(p->forw);
			p->idxs |= 
			(ptm_opts || blend_opts) ? indexing(p->forw) :
			base(p->forw);
		}
	/*special case: some opcodes are really prefixes. next operation */
  	/*must be scheduled after them, and resource dependencies dont   */
  	/*take care of that.                                             */
  		else if (isprefix(p->back)) {
			install1(p->back,p,DEP);
			continue;
		}

  		/*if p is hard to handle, nail it: make it dependent on every*/
  		/*former instruction, and every later instruction on it.     */
		found_unexpected |= Unexpected(p);

  		/* build the DAG.                                            */
  		install_dependencies(p);
  		update_static_vars(p);
	}/*for loop*/

	if (found_unexpected) nail();

/*Have to add anti dependencies which were not found in the         */
/*forward loop. Do it in a backwards loop.                          */
/*These occur when there are several consecutive uses of a register */
/*and then one set. The forward loop finds only the dependency      */
/*between the last use and the set, but there has to be dependency  */
/*between each use and the set.                                     */
	for (i=0; i < NREGS; i++) {
		setsR[i].setting = NULL;
		setsR[i].isconstant = false;
		sets_by_var[i] = NULL;
	}
	for (p = b->lastn; p != b->firstn->back; p = p->back) {
		if (isprefix(p->back))
			continue;
		if (isprefix(p)) {
			p->sets |= p->forw->sets;
			p->uses |= p->forw->uses;
			p->idus |= p->forw->idus;
			p->idxs |= (ptm_opts || blend_opts) ? indexing(p->forw) :
						base(p->forw);
		}
		/* If p sets esp and ebp is frame pointer, then instructions that
		** use ebp with negative offset should anti-depend on instructions that
		** set esp, so that there shall not be a reference below esp.
		*/
		if (!fp_removed) {
			if (p->idus & EBP && setsR[7].setting)
	  			install1(p,setsR[7].setting,ANTI);
		}
  		for (i = 0; i < NREGS; i++) {
    		if (p->uses & regmasx[i] && setsR[i].setting)
	  			install1(p,setsR[i].setting,ANTI);
    		if (p->idus & regmasx[i] && setsR[i].setting)
				if (! setsR[i].isconstant )
	  				install1(p,setsR[i].setting,ANTI);
				else if (sets_by_var[i])
					install1(p,sets_by_var[i],ANTI);
			/*mark the setting instructions */
			if (p->sets & regmasx[i]) {
	  			setsR[i].setting = p;
				if (change_by_const(p,regmasx[i],&dummy))
					setsR[i].isconstant = true;
				else {
					setsR[i].isconstant = false;
					sets_by_var[i] = p;
				}
			}
  		}/*for loop*/
	}/*for loop*/
	 
	installCCdep();
	add_fp_dependency(b);

	/* calculate for each node the number of it's dependent sons, */
	/* and the length of the longest chain under it.              */
    calc_width();
    calc_depth(); 
}/* end bldag*/

static void
init_block()
{
int i;
	last_volatile = last_is_br = NULL;
	acur = &a0;
	alloc_index = 0;
	for (i=0; i< NREGS; i++) {
		depths[i] = 0;
	}
	for (i=0; i< NREGS; i++) {
		setsR[i].isconstant = false;
  		setsR[i].setting = NULL;
  		usesR[i] = NULL;
		sets_by_var[i] = NULL;
	}
	for (i=0; i<7; i++) {
  		setsH[i] = NULL;
	}
	for (i=0; i<4; i++) {
  		setsQ[i] = NULL;
	}
	usesMEM.next = setsMEM.next =  NULL;
}/*end init_block*/

static void
initnode(p) NODE *p;
{
  p->dependent = p->may_dep = p->anti_dep = p->agi_dep = NULL;
  p->dependents = p->chain_length = p->nparents = 0;
  p->extra = p->extra2 = 0;
  p->usage = NONE;
  p->uses = muses(p); /*uses directly, not as index registers. */
  p->sets = msets(p);
  /*In  P5, p->idus == p->idxs. In intel486, p->idus is all the indexing
  **but p->idxs is only the base register. A register is considered (here)
  **as base, as opposed to index, if base and index registers can not be
  **swapped.
  */
  p->idus = indexing(p);
  p->idxs = (ptm_opts || blend_opts) ? indexing(p) : base(p);

  /* If and instruction has an immediate operand and no displacement
  ** then it will be a passimization to allow reordering it over
  ** a change by const - this would add the displacement and make it
  ** two cycle on 486 and unpairable on p5. disallow it by the following:
  */
  if (p->op1 && isconst(p->op1) && p->op2 && !hasdisplacement(p->op2))
	p->uses |= p->idus;

  if (p->op == ADDL && p->sets == (ESP | CONCODES) && 
      (p->forw->op == RET || p->forw->forw->op == RET)) {
  	p->sets = (unsigned) ESP | MEM | CONCODES;
  }
  else if (p->op == SUBL && p->sets == (ESP | CONCODES) && 
             b->firstn == n0.forw->forw) {
  	p->sets = (unsigned) ESP | MEM | CONCODES;
  } else if (p->op == CALL && p->op3)
  	p->sets |= ESP; /* /TMPSRET function it will do one POP */
  if (ptm_opts || blend_opts)
	p->pairtype = pairable(p);
}/*end initnode*/

/*mark p as the last instruction to use the registers it uses, set the
**registers it sets, etc.
*/
static void
update_static_vars(p)
NODE *p;
{
int i;
int dummy;

	for (i = 0; i < NREGS; i++) {
  		if (p->sets & regmasx[i]) {
			setsR[i].setting = p;
			if (change_by_const(p,regmasx[i],&dummy)) {
				setsR[i].isconstant = true;
			} else {
				setsR[i].isconstant = false;
				sets_by_var[i] = p;
			}
		}
  		if (p->uses & regmasx[i])
			usesR[i] = p;
	}/*for loop*/
	if (!ptm_opts && !blend_opts) {
		for (i = 0; i < 7; i++) {
  			if ((p->sets & halfregs[i]) && ! (p->sets & fullregs[i]))
     			setsH[i] = p;
		}/*for loop*/
		for (i = 0; i < 4; i++) {
  			if ((p->sets & qregs[i]) && ! (p->sets & justhalfs[i]))
	 			setsQ[i] = p;
		}/*for loop*/
	}
	if (p->sets & MEM)
		update_one(p,&setsMEM);
	if (p->uses & MEM)
		update_one(p,&usesMEM);
	if (isvolatile(p,1) || isvolatile(p,2))
		last_volatile = p;
}/*end update_static_vars*/

/*add an instruction to a list of instructions reading from / writting to
**memory.*/
static void
update_one(p,reg) NODE *p; tdependent * reg;
{
 tdependent *tmp;
	tmp = reg->next;
	reg->next = next_dep_str();
	reg->next->next = tmp;
	reg->next->depends = p;
}/*end update_one*/

/*make p dependent on previous instructions using the same resources*/
static void
install_dependencies(p)
NODE *p;
{
tdependentp sures,possibles;
int i;

	/*Look explicitely for all kinds of dependencies and install them.
	**Do not look for ANTI dependencies because the forward loop is 
	**not suited for that, it is done in a backwards loop in bldag after
	**the return from this function.
	*/

	for (i=0; i < NREGS; i++) {  /* for all registers */
		/*Output dependency: p sets vs the previous setting. */
  		if (p->sets & regmasx[i]) {
			if (setsR[i].setting && !(ispp(setsR[i].setting) && p->uses&ESP))
      			install1(setsR[i].setting,p,DEP);
  		}/*endif p->sets & regmasx[i]*/
		/*Dependency between different sized registers causes one clock
		**penalty on the intel486, not on the P5. On the P5 we mark it
		**as dependency.
		*/
  		if (i < 7) {  /*seven word addressable registers*/
  			if (p->uses & regmasx[i] && setsH[i])       /* 16 - 32 bit*/
				if (ptm_opts || blend_opts)
	  				install1(setsH[i],p,DEP);
				else
	  				install1(setsH[i],p,AGI);
  		}/*endif i < 7*/
		if (i < 4) { /*four byte addressable registers*/
  			if (p->uses & regmasx[i] && setsQ[i])      /*  8 - 32 bit*/
				if (ptm_opts || blend_opts)
	  				install1(setsQ[i],p,DEP);
				else
	  				install1(setsQ[i],p,AGI);
  		}/*endif i < 4*/
		/*Look for a regular dependence, where the previous instruction
		**set a register and the present instruction uses it.
		**If this register is %esp then make the dependence an ANTI
		**dependence. For the intel486 there is no difference. For the P5
		**it enables the two instruction to be selected together as a pair,
		**which is alright with the processor.
		**Only direct usage of registers is stored in p->uses, not indexing.
		*/
  		if (p->uses & regmasx[i]) 
			if (setsR[i].setting && !(ispp(setsR[i].setting) && p->idus&ESP))
	  			install1(setsR[i].setting,p,DEP);
			else if (setsR[i].setting)
	  			install1(setsR[i].setting,p,ANTI);
		/*p->idus is used to account for dependencies which are not covered by
		**neither p->uses (no indexing there) nor by p->idxs (only base reg.).
		**Look through it only for dependencies, not for AGI dependencies. 
		**that is only through p->idxs.
		**The usage of p->idus is to add dependencies to previous setting
		**instruction which change the registers not by a constant.
		**Through p->uses any previous setting was installed.
		*/
		if ((p->idus & regmasx[i]) && (setsR[i].setting))
			if (!setsR[i].isconstant)
				/*add dependencies as in p->uses, only if !isconstant*/
				if (!(ispp(setsR[i].setting) && p->idus&ESP))
	  				install1(setsR[i].setting,p,DEP);
				else
	  				install1( setsR[i].setting,p,ANTI);
			/*if the last set was by a constant, there may be stored a previous
			**instruction which sets the register not by constant. A dependency
			**to that instruction has to be added.
			*/
			else if (sets_by_var[i]) 
				install1(sets_by_var[i],p,DEP);
		/*Here look for AGI dependencies. If the present instruction uses
		**a register as an index and a previous one set it, not by constant,
		**then it is an AGI.
		**An exception in the P5: if the present instruction has a prefix,
		**there will not be a penalty.
		*/
  		if (p->idxs & regmasx[i])
			if (setsR[i].setting && !setsR[i].isconstant
				&& !(ispp(setsR[i].setting) && p->uses&ESP))
				if ((!ptm_opts && !blend_opts) || !Hasprefix(p))
	  				install1(setsR[i].setting,p,AGI);

 	}/* for all registers loop*/
 
/*Every volatile instruction depends on the previous volatile one*/
	if (last_volatile && (isvolatile(p,1) || isvolatile(p,2)))
		install1(last_volatile,p,ANTI);


	/*             Treat memory references seperately.            */
	/* only plain dependency, no anti or agi dependencies.        */

 	if (p->sets & MEM) {
   		if (setsMEM.next) {                    /*output dependency*/
     		relevants(p,setsMEM.next,&sures,&possibles);
     		if (sures)
       			installM(p,sures,1); 
	 		if (possibles)
	   			installM(p,possibles,0);
   		}/*endif setsMEM.next != NULL*/
   		if (usesMEM.next) {                      /*anti dependency*/
     		relevants(p,usesMEM.next,&sures,&possibles);
     		if (sures)
       			installM(p,sures,1); 
	 		if (possibles)
	   			installM(p,possibles,0);
   		}/*endif usesMEM.next != NULL*/
 	}/*endif (p->sets & MEM) */
 	if (p->uses & MEM) {                          /*     dependency*/
   		if (setsMEM.next) {
     		relevants(p,setsMEM.next,&sures,&possibles);
     		if (sures)
       			installM(p,sures,1);
	 		if (possibles)
	   			installM(p,possibles,0);
   		}/*endif setsMEM.next*/
 	}/*endif (puses & MEM) */
}/*end install_dependencies */

/*install one (1) dependency, that setsREG.
**father is the NODE on which to hang.
**depend is a pointer to the dependency pointer in the NODE.
**depp is the dependent NODE to be hanged.
**type is the dependency type, to verify if there allready
**exists a dependency between the two nodes.
**In that case dont add a dependency.
*/
static void
install1(father,son,type) 
NODE *father;  NODE * son; int type;
{
tdependent **header;
tdependent *tmp;
int dtype;
	isdependent(father,son,&header,&dtype);
	if (dtype == NO_DEP) {
		if (type < AGI && agi_depends(father,son))
			type = AGI; 
		tmp = father->depend[type];
		father->depend[type] = next_dep_str();
		father->depend[type]->depends = son;
		father->depend[type]->next = tmp;
		son->nparents++;
	}/*endif dtype == NO_DEP*/
	else if (dtype < type) {
		tmp = father->depend[type];
		father->depend[type] = *header;
		*header = (*header)->next;
		father->depend[type]->next = tmp;
	}/*end else if*/
}/*end install1*/


static void
installCCdep()
{
	NODE *p,*q,*last_set = NULL,*last_use = NULL;
	for (p = b->lastn; p != b->firstn->back; p = p->back) {
		if (p->sets & CONCODES) {
			if (last_use != NULL) { /* Use of CC depends on setting */
				install1(p,last_use,ANTI); /* Can be in the same cycle */
				last_use = NULL;
			}
			if (last_set == NULL)
				last_set = p; /* last set before use of condition code */
			else  /* Every other sets must come before the last set */
				if (last_set->op != CALL)
					install1(p,last_set,ANTI);
		}
		if (p->uses & CONCODES) {
			last_use = p;
			if (last_set != NULL) {
				for (q = p; q != last_set->forw; q = q->forw)
					if (p != q && q->sets & CONCODES)
						install1(p,q,ANTI); /* make last use before all sets  */
				last_set = NULL;
			}
		}
	}
}/*end installCCdep*/

/*check whether there is a dependency between father and son, if so,
**of what type is it.
*/
static void
isdependent(father,son,header,type)
NODE *father, *son; tdependent ***header; int *type;
{
tdependent **tp;
int i;
	for (i = ANTI ; i <= AGI ; i++)
		for (tp = &father->depend[i]; *tp; tp = &(*tp)->next)
			if ((*tp)->depends == son) {
	  			*type = i;
	  			*header = tp;
	  			return;
			}/*endif*/
  	*type = NO_DEP;
  	*header = NULL;
}/*end isdependent*/

/*install dependencies between one son and many parents. Used only
**in the context of dependency via the memory resource.
*/
static void
installM(son,parents,sure) NODE *son; tdependent *parents;int sure;
{
tdependent *tp;
	if (sure)
    	for (tp = parents; tp; tp = tp->next)
	  		install1(tp->depends,son,DEP);
  	else
    	for (tp = parents; tp; tp = tp->next)
	  		install1(tp->depends,son,MAY);
}/*end installM*/


/*build list of instructions which cant be proven not to depend
**on p, out of the list of all memory refernces.
**If there is a certain equality between two memory locations references
**by two instructions, put them on same_list. If equality is unresolvable,
**put them on unsafe_list.
*/

static void
relevants (p,setsmem,same_list,unsafe_list)
NODE * p; tdependent *setsmem; tdependent **same_list,**unsafe_list;
{
tdependent *tmp; 
tdependent * tp;
*same_list = *unsafe_list = NULL;
	for (tp = setsmem; tp ; tp = tp->next) {
    	switch (seems_same(p,tp->depends)) {
	  		case different:
					  	break;
	  		case same:
					  tmp = *same_list;
					  *same_list = next_dep_str();
					  (*same_list)->next = tmp;
					  (*same_list)->depends = tp->depends;
					  break;
	  		case unsafe:
					  tmp = *unsafe_list;
					  *unsafe_list = next_dep_str();
					  (*unsafe_list)->next = tmp;
					  (*unsafe_list)->depends = tp->depends;
					  break;
    	}/*end switch*/
  	}/*for loop*/
}/*end relevants*/

/*try to prove that two memory refernces are not the same return
**value 1 if they seem the same, 0 if they proven different.
**2 if dont know.
**p2 is an earlier instruction in the block then p1.
*/

/* name1 and name2 are common to seems_same and decompose */
static char name1_buf[MAX_LABEL_LENGTH],name2_buf[MAX_LABEL_LENGTH];
int
seems_same(p1,p2) NODE *p1, *p2;
{
int y1,y2,x1 =0,x2 =0;
unsigned char c;
int n1 = 0, n2 = 0;
int scale1,scale2;
int m1,m2;           /*pointers to the memory referncing operands*/
unsigned pidx1,pidx2;
extern int isregal();
char *name1,*name2;
unsigned int frame_pointer;
	pidx1 = p1->idus;
	pidx2 = p2->idus;
	if (fp_removed)
		if (double_aligned)
			frame_pointer = EBP | ESP;
		else 
			frame_pointer = ESP;
	else 
		frame_pointer = EBP;
	/* if we optimized ebp elimination for double alignment, and P1 indexes
	** esp and P2 indexes ebp, then the first one is a parameter and the
	** second one is an auto, ergo they are different.
	*/
	if (frame_pointer == (EBP | ESP))
		if ((pidx1 == EBP && pidx2 == ESP) || (pidx1 == ESP && pidx2 == EBP))
			return different;
 	/* find which are the operands referencing memory in both p1,p2  */
	if (ismem(p1->op1)) m1 = 1;
	else if (ismem(p1->op2)) m1 = 2;
	else m1 = 0;
	if (ismem(p2->op1)) m2 = 1;
	else if (ismem(p2->op2)) m2 = 2;
	else m2 = 0;
	/* read only data items are different from any thing else */
	/* currently, nobody calls seems same with two read instructions, the 
	** second test is for completeness.
	*/
	if (is_read_only(p1->ops[m1]) || is_read_only(p2->ops[m2]))
		if (!strcmp(p1->ops[m1],p2->ops[m2])) return same;
		else return different;

	if ((p1->op == ADDL || p1->op == SUBL) && 
								p1->sets == (ESP | MEM | CONCODES)) {
		c = m2 ? *p2->ops[m2] : (char) 0;
		return (isalpha(c) || c == '_' || c == '.') ? different : unsafe;
	} else if ((p2->op == ADDL || p2->op == SUBL) && 
								p2->sets == (ESP | MEM | CONCODES)) {
		c = m1 ? *p1->ops[m1] : (char) 0;
		return (isalpha(c) || c == '_' || c == '.') ? different : unsafe;
	}
	name1 = (strlen(p1->ops[m1]) < ((unsigned ) MAX_LABEL_LENGTH)) ? 
			                       name1_buf : getspace(strlen(p1->ops[m1]));
	name2 = (strlen(p2->ops[m2]) < ((unsigned ) MAX_LABEL_LENGTH)) ? 
			                       name2_buf : getspace(strlen(p2->ops[m2])); 

    *name1 = *name2 = '\0';
	if (m1) n1 = decompose(p1->ops[m1],&x1,name1,&scale1);
	if (m2) n2 = decompose(p2->ops[m2],&x2,name2,&scale2);
	/* if one operand is of the form n(%ebp) and was commented by
	** the compiler for being assigned to a register, then it is
	** different from the second one.
	** if ebp elimination was done, then it became m(%esp)
	*/
	if (fp_removed) {
		if (!double_aligned) { /*autos and params via esp */
			y1 = p1->ebp_offset;
			y2 = p2->ebp_offset;
		} else { /* autos via esp, params via ebp */
			if (pidx1 & EBP)
				y1 = x1;
			else if (pidx1 & ESP)
				y1 = p1->ebp_offset;
			else
				y1 = 0;
			if (pidx2 & EBP)
				y2 = x2;
			else if (pidx2 & ESP)
				y2 = p2->ebp_offset;
			else
				y2 = 0;
		}
	} else {
		y1 = x1;
		y2 = x2;
	}

	/* if both are regals                               */
	if ((double_aligned && (pidx1 == ESP || pidx1 == EBP)
						&& (pidx2 == ESP || pidx2 == EBP))
	  || (!double_aligned && (pidx1 == frame_pointer)
						  && (pidx2 == frame_pointer)))
	if (isregal(y1) && isregal(y2))
  		if ((absdiff(y1,y2)) < (int) (MAX((OpLength(p1)),(OpLength(p2)))))
			return same;
		else
			return different;

	/* if p1 is regal and p2 is not                     */

	if (((double_aligned && (pidx1 == ESP || pidx1 == EBP))
	  || (!double_aligned && (pidx1 == frame_pointer))) 
	 && isregal(y1)) {
		if ((m2 && !strcmp(p1->ops[m1],p2->ops[m2])) ||
  		   ((absdiff(y1,y2)) < (int) (MAX((OpLength(p1)),(OpLength(p2))))))
			return same;
		else
			return different;
	 }

	/* if p2 is regal  and p1 is not                    */

	if (((double_aligned && (pidx2 == ESP || pidx2 == EBP))
	  || (!double_aligned && (pidx2 == frame_pointer)))
	 && isregal(y2)) {
		if ((m1 && !strcmp(p1->ops[m1],p2->ops[m2])) ||
  		   ((absdiff(y1,y2)) < (int) (MAX((OpLength(p1)),(OpLength(p2))))))
			return same;
		else
			return different;
	 }


 	if (isprefix(p1) || isprefix(p2))
		return unsafe;
 	if (p1->op == CALL || p2->op == CALL)
		return unsafe;
	/*if one instruction is push/pop and the second one uses esp
	**then we lie and answer no, we want the dependency type 
	**in that case to be ANTI dependency. Sorry about that...
	*/
 	if (ispp(p1) && ispp(p2))
		return different;
 	if (ispp(p1)) {
   		if (p2->uses & ESP)
			if (!strcmp(p1->op1,p2->ops[m2]))
				return same;
			else
				return different;
   		else
			pidx1 &= ~ESP;
	}
 	if (ispp(p2)) {
   		if (p1->uses & ESP)
			if (!strcmp(p2->op1,p1->ops[m1]))
				return same;
			else
				return different;
   		else
			pidx2 &= ~ESP;
	}

 	/*The above were cases where an instruction may have two memory  */
 	/* references. Next we treat instructions with exactly one       */
 	/* memory reference.                                             */

	/* check whether both dont use registers. In that case, both are of */
	/* the form n+Label. If labels are different then they are different*/
	/* if Labels are same then if the numbers are different enaugh they */
	/* are different, otherwise same.                                   */
	if ((pidx1 | pidx2) == 0) {
  		if (n1 != n2)
			n2 = 1; /*strings are different*/
  		else
			n2 = strncmp(name1,name2,n1);
  		if (n2)
			return different; /*strings are different -> not same */
  		if ((absdiff(x1,x2)) < (int) (MAX((OpLength(p1)),(OpLength(p2)))))
			return same;
  		else
			return different;
	}

	/* if one instruction indexs a register and the other one doesnt     */
	/* then if the register is either ESP or EBP then they are different */
	/*else if the labels are different then they are different, else same*/
	if ((! pidx1) && pidx2)              /*only p2 indexs a register*/
  		if (pidx2 & (ESP | frame_pointer))
			return different;
  		else {
			if (n1 != n2)
				n1 = 1;
			else
				n1 = strncmp(name1,name2,n1);
			if (n2 == 0)
				return unsafe;   /*if p2 has no fixup, they are same*/
			/*according to labels same or different*/
			if (n1)
				return different;
			else
				return unsafe;
  		}/*end of this case*/
	if ((! pidx2) && pidx1)              /*only p1 indexs a register*/
  		if (pidx1 & (ESP | frame_pointer))
			return different;
  		else {
			if (n1 != n2)
				n2 = 1;
			else
				n2 = strncmp(name1,name2,n1);
			if (n1 == 0)
				return unsafe;   /*if p1 has no fixup, they are same*/
			/*according to labels same or different*/
			if (n2)
				return different;
			else
				return unsafe;
  		}/*end of this case*/

	/* Now both use registers.
	** If both do not have labels then if the registers are different
	** the memory locations are (suspected) different. If the
	** registers are same, were not changed and the difference
	** between the numbers is big enaugh - different, else same.
	** If they both have labels and the labals are different then
	** the memory references are different.
	** If the labales are different and the registers are different then
	** the memory references are (suspected) same.
	** If the registers are same and were not changed between the 
	** two instructions and the difference between the numbers is big
	** enaugh then the memory references are different.
	** otherwise same.
	*/
   	if (n1 + n2 == 0) { /*both dont have labels */
	 	if (pidx1 != pidx2)
			return unsafe;
     	if (scale1 != scale2 || changed(p2,p1,pidx1))
			return unsafe;
	 	if (absdiff(x1,x2) < (int) MAX(OpLength(p1),OpLength(p2)))
			return same;
	 	else
			return different;
   	}/*endif both dont have labels*/
   	else if (n1 == 0) {   /*one has label, the other one has not */
	 	if (pidx1 & (frame_pointer | ESP))
			return different;
	 	else
			return unsafe;
	} else if (n2 == 0)  {  /*the second has label, the first has not */
	 	if ( pidx2 & (frame_pointer | ESP))
			return different;
	 	else
			return unsafe;
	}
   	/*Both have labels. */
   	if (n1 != n2)
		n2 = 1; /*labels are different*/
   	else
		n2 = strncmp(name1,name2,n1);
   	if (n2)   /*the labels are different*/
		return different;
   	if (pidx1 != pidx2 || scale1 != scale2 || changed(p2,p1,pidx1))
		return unsafe;
   	if (absdiff(x1,x2) < (int) MAX(OpLength(p1),OpLength(p2)))
		return same;
   	else
		return different;
}/*end seems_same*/

/*did reg change between first and second instructions.           */
int
changed(first,second,regs) NODE *first,*second; unsigned int regs;
{
 NODE *p;
	if (!fp_removed && (regs == EBP)) return false;
 	for (p=first->forw; p != second; p = p->forw) {
		if (p->sets & regs) return true;
		if (islabel(p))	return true; /*if not in same BB, give up */
	}
 	return false;
}/*end changed*/

/*For each node find the length of the longenst chain to the leaves
**in the DAG.
*/
static void
calc_depth()
{ 
NODE * p;
	for (p = b->firstn; p != b->lastn->forw; p = p->forw) {
  		if (p->nparents == 0)
			(void) depth(p); /*initiate from every root*/
  	}/* for loop*/
}/*end calc_depth*/

static int
depth(p) NODE *p;
{
 	if (!p->dependent  && !p->anti_dep && !p->agi_dep && !p->may_dep) {
   		p->chain_length = 0;
   		return 0;
	}
 	else if (p->chain_length != 0)
		return (p->chain_length);
 	else {
		p->chain_length =
          1+ depth_4list(p);
		return (p->chain_length);
	}/*else*/
}/*end depth*/

static int
depth_4list(p)
NODE *p;
{
  tdependent * tp;
  int value =0;
  int value1,i;

  	for(i=ANTI;i<=AGI;i++){
		for(tp= p->depend[i]; tp != NULL ; tp = tp->next) {
      		value1 = depth(tp->depends);
      		if (value1 > value)
   	    		value = value1;
 		}/*end for loop*/
  	}/*end for loop*/
  	return value;
}/*end depth_4lists*/


/*For each node, how many direct sons it has.*/
static void
calc_width()
{
tdependent * tp;
NODE * p;
int i;
  	for (p = b->firstn; p != b->lastn->forw; p = p->forw) {
    	for (i = ANTI ; i <= AGI ; i++)
    		for (tp = p->depend[i]; tp ; tp = tp->next)
				p->dependents++; 
  	}/*for all instructions in the block*/
}/*end calc_width*/

static void
nail()
{
NODE *nailed;
NODE *p;
	for (nailed = b->firstn; nailed != b->lastn; nailed = nailed->forw) {
		if (Unexpected(nailed)) {
   			for (p=b->firstn; p != nailed; p=p->forw)
	 			install1(p,nailed,DEP);
   			for (p=nailed->forw; p != b->lastn->forw; p = p->forw)
	 			install1(nailed,p,DEP);
		}
	}
}/*end nail*/

/*The second module of the scheduler: reorder the instruction of the basic
**block.
*/
static NODE *lastX86 = NULL, *lastV = NULL;
static int successive_memories;
static int scheded_inst_no = 0;

int come_from;

static void
prun()
{
NODE * last_scheded = NULL,*new_scheded;
  	m0.next = &m1;
  	m1.back = &m0;
  	scheded_inst_no = 0;
    fp_start();
  	if (ptm_opts || blend_opts)
    	lastX86 = lastV = NULL;
  	else
    	successive_memories = 0;
  	while (scheded_inst_no < b->length){ /*while not all were scheded*/
    	make_clist();    /* make it already ordered */
    	if (ptm_opts || blend_opts) {
      		pick1or2();
      		m0.next = fp0;
      		dispatch1or2();
      		remove1or2();
      		scheded_inst_no += ( lastV == NULL ? 1:2);
#ifdef STATISTICS
      		total_inst += ( lastV == NULL ? 1:2);
#endif
		}
    	else { /*if i486 optimizations*/
      		if ( last_scheded) {
      			new_scheded = pick_one(last_scheded);
				last_scheded->usage = ((FP(new_scheded->op) && concurrent_value)
			   	|| agi_depends(last_scheded,new_scheded)
			   	|| successive_memories > 3) ? LOST_CYCLE : NONE;
				last_scheded = new_scheded; 
			} else
      			last_scheded = pick_one(last_scheded);
      		m0.next = fp0;
      		dispatch(last_scheded);
      		update_clist(last_scheded); /*remove the last scheded from clist*/
	  		if (touchMEM(last_scheded) && (get_exe_time(last_scheded) == 1))
				successive_memories++;
	  		else
				successive_memories = 0;
      		scheded_inst_no++;
    	}
  	}/*while loop*/
	if ( last_scheded) 
		last_scheded->usage = NONE;
}/*end prun*/

/*collect all nodes with currently zero parents*/
static void
make_clist()
{
NODE * p;
clistmem * tmp;
clistmem * scan;
static clistmem free_list[BLOCK_SIZE_LIMIT+2];
static	int next_free = 0;
	/*loop on all instructions and find the ones with no parents */
	/*insert the members into places according to the order      */
	/*induced by before().                                      */
   if (m0.next == &m1)
   		next_free = 0;
   for (p = prev->forw ; p != b->lastn->forw; p = p->forw) {
    	if (p->nparents == 0) {                             /*found one*/
      		p->nparents= -1;
	  		/*find where to insert the new member. point there with scan*/
	    	scan = m0.next;
	    	while ((scan != &m1) && (before(scan->member,p) == 1))
		  		scan = scan->next;
	    	tmp = scan->back;
			tmp->next = &free_list[next_free++];
        	tmp->next->next = scan;
        	tmp->next->back = tmp;
        	scan->back = tmp->next;
        	tmp->next->member = p;
    	}/*endif nparents == 0 */
  	}/*for loop*/
  	if (m0.next == &m1)
		fatal(gettxt(":1430","Empty candidate list\n"));
}/*end make_clist*/


/*Who comes first in the candidate list*/
static int
before(p1,p2) NODE *p1, *p2;
{
	if (FP(p1->op)) {
		if (ptm_opts) {
			if (! FP(p2->op))
				return 1;
			if (FPPREFERENCE(p1->op) > FPPREFERENCE(p2->op))
				return 1;
			if (FPPREFERENCE(p1->op) < FPPREFERENCE(p2->op)) 
				return 2;
			return natural_order(p1,p2);
		} else return 1;
	}	
	else if (FP(p2->op))
		return 2;
	if (ptm_opts || blend_opts) {
  		if ((p1->anti_dep) && (! p2->anti_dep))
			return 1;
  		if ((p2->anti_dep) && (! p1->anti_dep))
			return 2;
  		if (p1->pairtype < p2->pairtype)
			return 1;
  		if (p1->pairtype > p2->pairtype)
			return 2;
	}
	else
	{
  		if ((has_agi_dep(p1)) && (! has_agi_dep(p2))) {
			return 1;
		}
  		if ((has_agi_dep(p2)) && (! has_agi_dep(p1))) {
			return 2;
		}
	}
	if (p1->chain_length > p2->chain_length) {
		return 1;
	}
	if (p1->chain_length < p2->chain_length) {
		return 2;
	}
	if (p1->dependents   > p2->dependents) {
		return 1;
	}
	if (p2->dependents   > p1->dependents) {
		return 2;
	}
	return natural_order(p1,p2);
}/*end before*/

/*who is first in the original code*/
static int
natural_order(p1,p2) NODE *p1, *p2;
{
 NODE *p;
 	for(p = b->firstn; p != b->lastn->forw; p=p->forw) { 
   		if (p == p1)
			return 1;
   		if (p == p2)
			return 2;
 	}/*for loop*/
/* NOTREACHED */
}/*end natural_order*/

/*Pick the following instruction. Used for i486 optimizations.*/
static NODE *
pick_one(last_scheded) NODE * last_scheded;
{
clistmem *clp;
int one_cycle;
static NODE *current_fp;

  	fp0 = m0.next;
  	if (last_scheded && isprefix(last_scheded))  {
		next_of_prefix->nparents = -1;
		if (vflag)
			next_of_prefix->op4 = "\t\t  / prefixed ";
    	return next_of_prefix;
  	}
  	if ( concurrent_value && FP(m0.next->member->op) &&
  		m0.next->next != &m1 && !current_fp) /* if fp busy and there is at list one more
  		                        candidate skip the fp node in the list */
  		m0.next = m0.next->next;
  	else if (FP(m0.next->member->op)) {
		current_fp = NULL;
  		if (vflag)
			m0.next->member->op4 = concurrent_value ? 
						"\t\t / FP concurrent(+)" : "\t\t / FP concurrent(0)";
		return m0.next->member;
  	}
  	if (last_scheded == NULL)  {
		if (vflag)
			m0.next->member->op4 = "\t\t / m0.next";
		return m0.next->member;
  	}
	one_cycle = get_exe_time(last_scheded) == 1;
  	switch (successive_memories) {
    	case 0: case 1:
      		for (clp = m0.next; clp != &m1; clp = clp->next)
	    		if (! agi_depends(last_scheded,clp->member) &&
					(!(Hasprefix(clp->member) && one_cycle))) {
		   			if (vflag)
						clp->member->op4 = "\t\t / sm 0,1 ";
					return clp->member;
				}
	  			break;
    	case 2:
	  		for (clp = m0.next; clp != &m1; clp = clp->next)
				if ( !agi_depends(last_scheded,clp->member) &&
					(!(Hasprefix(clp->member) && one_cycle))  &&
			 		!touchMEM(clp->member)) {
			   		if (vflag)
						clp->member->op4 = "\t\t / sm2, no mem";
					return clp->member;
				}
      		for (clp = m0.next; clp != &m1; clp = clp->next)
	    		if (! agi_depends(last_scheded,clp->member)  &&
					(!(Hasprefix(clp->member) && one_cycle))) {
		   			if (vflag)
						clp->member->op4 = "\t\t / sm2, ";
					return clp->member;
				}
	  		break;
    	default:
	  		for (clp = m0.next; clp != &m1; clp = clp->next)
				if ( !agi_depends(last_scheded,clp->member) &&
					( ! (Hasprefix(clp->member) &&
				 	one_cycle)) &&
			 		!touchMEM(clp->member)) {
			   		if (vflag)
						clp->member->op4 = "\t\t / smd, no nothimg";
					return clp->member;
				}
#if we_want_the_agis
      		for (clp = m0.next; clp != &m1; clp = clp->next)
	    		if (! touchMEM(clp->member)) {
		  			if (vflag)
						clp->member->op4 = "\t\t / smd, no mem";
					return clp->member;
				}
#endif
      		for (clp = m0.next; clp != &m1; clp = clp->next)
				if ( !agi_depends(last_scheded,clp->member) &&
					(!(Hasprefix(clp->member) && one_cycle))) {
		  			if (vflag)
						clp->member->op4 = "\t\t / smd, no agi";
					return clp->member;
		 		}
	  		break;
  	}/*end switch*/
	for (clp = m0.next; clp != &m1; clp = clp->next) {
		if ( !agi_depends(last_scheded,clp->member) )  {
			if (vflag)
				clp->member->op4 = "\t\t /last option, no agi";
			return clp->member;
	 	}
	}
	if (vflag)
		m0.next->member->op4 = "\t\t / last option";
	return m0.next->member;
}/*end pick_one*/

/*Does p1 agi - depend on p */
static int
agi_depends(p,p1) NODE *p, *p1;
{
	
	if (!p || !p1 || ! (p->sets & ~(MEM | CONCODES)))
		return false;  /* No register setting by p */
	if (ispp(p) && ! (p->sets & p1->idxs & ~ESP))
		return false;  /* No AGI with ESP setting by push/pop */
	if (p->sets & p1->idxs)
		return true; /* found AGI */
	if( ptm_opts || blend_opts)
		return false;
	if ((p->sets & p1->uses & ~(MEM | CONCODES)) /* sets and uses same reg */
		 && ! (p->sets & R16MSB) /* less then full reg set */
		 && (p1->uses & R16MSB)) /* full reg uses */
		return true;
	return false;
}/*end agi_depends*/

#if 0       /* implemented as a macro */
static boolean
has_agi_dep(p) NODE *p;
{
	if (p->agi_dep) return true;
	return false;
}/*end has_agi_dep*/
#endif

/* for each instruction which is not yet scheduled:
** mark the usage field of the node if moving if forward relative to it's
** current location may intruduce an AGI.
*/
static void
mark_new_agis()
{
NODE *first = lastV ? lastV->forw : lastX86 ? lastX86->forw : b->firstn;
NODE *q;
unsigned int indexes = 0;
	for (q = first; q != b->lastn->forw; q = q->forw)
		q->usage = NONE;
	for (q = first; q != b->lastn->forw; q = q->forw) {
		if (q->sets & indexes) q->usage = AGI;
		indexes |= q->idxs;
	}
}/*end mark_new_agi*/

/*Remove the issued instruction from the candidate list.
**Dont assume that last_scheded is really in the clist
*/
static void
update_clist (last_scheded) NODE * last_scheded;
{
clistmem * clp;
  	for (clp=m0.next;clp!=&m1 && clp->member!=last_scheded;clp=clp->next)
	;
  	/*clp now points to the clist member with the scheded instruction*/
  	/*delete that member from the clist                          */
  	if (clp != &m1) {
    	clp->back->next = clp->next;
		clp->next->back = clp->back;
    }/*endif clp*/
}/*end update_clist*/


/*Make some book keeping for the issued instruction.*/
static void
dispatch(p) NODE * p;
{
tdependent * tp;
int reg,i;
int amount;
	if (p->agi_dep == &dummy_to_show_agi) p->agi_dep = NULL;
 	/*decrement the # of parents of the dependents of p if any.     */
	for (i = ANTI ; i <= AGI ; i++)
  		for (tp=p->depend[i]; tp ; tp= tp->next)
    		tp->depends->nparents--;
	
	/* record the amount of constant changes for the register(s) set by p*/
	for (reg = 0; reg < NREGS; reg++) {
		if (p->sets & regmasx[reg]) {
			if (change_by_const(p,regmasx[reg],&amount)) {
				depths[reg] += amount;
			} else {
				depths[reg] = 0;
			}
		}
	}

  	if (! ptm_opts)
		if (FP(p->op))
			concurrent_value = CONCURRENT(p->op);
		else {
			if ( prev != b->firstn->back && agi_depends(prev,p))
				concurrent_value--;
			concurrent_value -= get_exe_time(p);
			if (concurrent_value < 0)
			 	concurrent_value = 0;
		}
  	reorder(p); /* change pointers of NODEs to move p to new place */
	fix_offsets(p); /*account for changes of index regs by consts  */
	p->uses |= p->idus; /* no more uses but not indexing */
  	if ( FP(p->op)) {
  		if (ptm_opts)
  			reset_fp_reg(p);
  		else if (blend_opts)
  			 fp_set_resource(p);
  	} 
}/*end dispatch*/

static void
remove1or2()
{
  	update_clist(lastX86);
  	if (lastV)
		update_clist(lastV);
}/*end remove1or2*/

static void
dispatch1or2()
{	/* first count the AGI cycle and use this cycle for fxch */
	if ( agi_depends(prev,lastX86) || (lastV && agi_depends(prev,lastV))
      || ((prev->usage == ON_J ) && 
         agi_depends(prev->back,lastX86) ||
			(lastV && agi_depends(prev->back,lastV)))) {
		fp_ticks(1);
		last_float = prev;
	} 
	if (! lastV) {
		dispatch(lastX86);
		if (! FP(lastX86->op))
			fp_ticks(get_exe_time(lastX86));
	}/*endif*/
  	else {
    	dispatch(lastX86);
    	dispatch(lastV);
    	fp_ticks(get_exe_time(lastX86) + get_exe_time(lastV) -1);
  	}
}/*end dispatch1or2*/


/*In case of pentium optimizations, find the best choice of the next one
**or two instructions to be issued.
*/
/*definitions of weights of quolities of pairs */
#define NOT_MEM	1
#define UNSAFE	2
#define RIGHT	4
#define DEPENDENT	8
#define ANTI_D	16
#define HAS_AGI_DEP	32
#define REVERSE_AGI	128
#define BOTH_AGI_UP	256
#define NONE_AGI_UP	512

static NODE *best_left,*best_right;
static int best_weight;

static void
pick1or2()
{
clistmem * clp;
NODE * pickedL;
	fp0 = m0.next;
	best_left = best_right = NULL;
	best_weight = 0;
	/*  Special case                           */
	if (lastX86 && isprefix(lastX86)) {
  		lastX86 = next_of_prefix;
  		lastX86->nparents = -1;
  		lastV = NULL;
  		lastX86->usage = ALONE;
  		if (vflag)
			lastX86->op4 = "\t\t / prefixed";
#ifdef STATISTICS
  		on_X86++;
#endif
  		return;
	}
	/*       FIRST PREFERENCE:                */
	/*  float instruction.                    */
 	if ((pickedL = fp()) != NULL) {
   		lastX86 = pickedL;
   		lastV = NULL;
#ifdef STATISTICS
  		on_X86++;
#endif
   		return;
 	}
 
 	/*       SECOND PREFERENCE:                */
 	/*X86 no agi up, having agi dependents    */
 	if ((pickedL = EX86(1)) != NULL) {
   		lastX86 = pickedL;
   		lastX86->usage = ALONE;
   		if (vflag)
			lastX86->op4 = "\t\t/ X8601";
   		lastV = NULL;
#ifdef STATISTICS
  		on_X86++;
#endif
   		return;
 	}
 
	/* find best pair and it's weight */
	pair();

 	/* THIRD PREFERENCE                       */
 	/*a pair, no agi up, both have agi dependents*/
 	if (best_weight >= (NONE_AGI_UP + HAS_AGI_DEP + HAS_AGI_DEP )) {
		lastX86 = best_left;
		lastV = best_right;
        lastV->nparents = -1;
		lastX86->usage = ON_X;
		lastV->usage = ON_J;
   		if (vflag) {
   			lastX86->op4 = " \t\t/ left 1 ";
			lastV->op4 = getspace(32);
			sprintf(lastV->op4,"\t/ right 1-%d:%d",come_from,best_weight);
		}
#ifdef STATISTICS
#endif
		return;
	}

 	/*     FOURTH  PREFERENCE                   */
  	/*X86 no agi up, no agi dependents        */
 	if ((pickedL = EX86(0)) != NULL) {
   		lastX86 = pickedL;
   		lastX86->usage = ALONE;
   		if (vflag)
			lastX86->op4 = "\t\t/ X8600";
   		lastV = NULL;;
#ifdef STATISTICS
  		on_X86++;
#endif
   		return;
 	}

	/* FIFTH PREFERENCE                         */
	/* a pair with no agi up, no agi dependents. */
	if (best_weight >= NONE_AGI_UP) {
		lastX86 = best_left;
		lastV = best_right;
        lastV->nparents = -1;
		lastX86->usage = ON_X;
		lastV->usage = ON_J;
   		if (vflag) {
   			lastX86->op4 = " \t\t/ left 2 ";
			lastV->op4 = getspace(32);
			sprintf(lastV->op4,"\t/ right 2-%d:%d",come_from,best_weight);
		}
#ifdef STATISTICS
#endif
		return;
	}

 	/* SIXTH  PREFERENCE                      */
 	/* a pairable with no pair and no agi up     */
 	for (clp = m0.next; clp != &m1; clp = clp->next)
   		if (! (agi_depends(lastX86,clp->member) ||
              (agi_depends(lastV,clp->member)))) {
     		lastX86 = clp->member;
	 		lastX86->usage = ALONE;
     		if (vflag) 
				lastX86->op4 = "\t\t/ single";
     		lastV = NULL;
#ifdef STATISTICS
			on_X86++;
  			if (lastX86->pairtype != X86)
				pairables++;
#endif
     		return;
  		}

 	/* SEVENTH  PREFERENCE                        */
	/* anything that is decided best by pair() will do */
 	if (best_weight) {
		lastX86 = best_left;
		lastV = best_right;
        lastV->nparents = -1;
		lastX86->usage = ON_X;
		lastV->usage = ON_J;
   		if (vflag) {
   			lastX86->op4 = " \t\t/ left 3 ";
			lastV->op4 = getspace(32);
			sprintf(lastV->op4,"\t/ right 3-%d:%d",come_from,best_weight);
		}
#ifdef STATISTICS
#endif
		return;
	}


    

	/* TWELEVETH AND LAST                          */
	lastX86 = m0.next->member;
   	lastX86->usage = ALONE;
   	if (vflag) 
		lastX86->op4 = "\t\t/ last option";
   	lastV = NULL;
#ifdef STATISTICS
  	on_X86++;
  	if (lastX86->pairtype != X86)
		pairables++;
#endif
	return;
}/*end pick1or2*/


/*scan the c-list for an X86 type instruction*/
static NODE *
EX86(agidown)
int agidown;
{
clistmem *clp;
 	for (clp = m0.next; clp != &m1; clp = clp->next){
   		if (clp->member->pairtype == X86
		 && (! agi_depends(lastX86,clp->member))
		 && (! agi_depends(lastV,clp->member))
		 && ((! agidown) || has_agi_dep(clp->member)))
	 		return clp->member;
 	}/*end for loop*/

 	return NULL;
}/*end EX86*/

static NODE *
fp()
{	int time;
clistmem *clp;
	int min_time,min_add_fxch,add_fxch;
	NODE *min_time_p;
	min_time = 1000;
	min_add_fxch = true;
	min_time_p = NULL;
	for (clp = m0.next; clp != &m1; clp = clp->next){
		if (FP(clp->member->op)) {
			time = (agi_depends(lastX86,clp->member) || 
										agi_depends(lastV,clp->member));
			add_fxch = check_for_fxch(clp->member);
			if (! (time += fp_time(clp->member))) {
		        if (!add_fxch ) {
		        	if (vflag) 
						clp->member->op4 = "\t\t/ fp (T0)";
					clp->member->usage = ALONE;
	 				return clp->member;
	 			}
	 			min_time_p = clp->member;
	 			min_time = 0;
	 			min_add_fxch = true;
	 			
	 		} else {
	 			if (time < min_time || 
	 			    (time == min_time && min_add_fxch && ! add_fxch)) {
	 				min_time_p = clp->member;
	 				min_time = time;
	 				min_add_fxch = add_fxch;
	 			}
	 		}	
		} else {
			if (! min_time) { /* fxch added but no extra time */ 
				if (vflag) 
					min_time_p->op4 = "\t\t/ fp (T0)";
				min_time_p->usage = ALONE;
	 			return min_time_p;
			} 
			fp0 = m0.next; /* non fp was found don't look for fp any more */
			m0.next = clp;
			return NULL;
		}
 	}/*end for loop*/
	if (min_time_p != NULL ) {
	  if (vflag)
      	if ( min_time)
      		min_time_p->op4 = "\t\t/ fp (T+)";
      	else
			min_time_p->op4 = "\t\t/ fp (T0)";
	  min_time_p->usage = ALONE;
	}
	return min_time_p; /* only FP in the list */
}/*end fp() */

static void
mark_cur_agis()
{
NODE *first = lastV ? lastV->forw : lastX86 ? lastX86->forw : b->firstn;
NODE *q;
unsigned int indexes = 0;
	/* unmark previous marks */
	for (q = first; q != b->lastn->forw; q = q->forw)
		if (q->agi_dep == &dummy_to_show_agi) q->agi_dep = NULL;
	/* Go on the block backwards. Collect all indexed registers.
	** If an instruction sets any of the above registers - mark it.
	** Do the test before the collect in order not to mark
	** movl (%eax),%eax
	*/
	for (q = b->lastn; q != first->back; q = q->back) {
		if (!(q->agi_dep) && q->sets & indexes)
			q->agi_dep = &dummy_to_show_agi;
		indexes |= q->idus;
	}
}/*end mark_cur_agis*/

/*Scan the c-list for a pair of instructions.*/
static void
pair()
{
NODE  *pickedL=NULL,*pickedR=NULL;
clistmem *clpl=NULL,*clpr=NULL;
int cur_weight;

	mark_cur_agis();
	mark_new_agis();
   	for (clpl = m0.next; clpl != &m1; clpl = clpl->next) {
     	if ((pickedL = islefthy(clpl->member)) != NULL) {
             /*is there an anti dependent     */
       		if ((pickedR = Eanti_dependent(pickedL)) != NULL) {
				cur_weight = find_weight(pickedL,pickedR) + ANTI_D;
				if (cur_weight > best_weight) {
					best_weight = cur_weight;
					best_left = pickedL;
					best_right = pickedR;
					come_from = 1;
				}
       		}

        /* a dependent, like PUSH PUSH */

       		if (ispp(pickedL) && isior(pickedL->op1))
       			if ((pickedR = Edependent(pickedL)) != NULL) {
					cur_weight = find_weight(pickedL,pickedR) + DEPENDENT;
					if (cur_weight > best_weight) {
						best_weight = cur_weight;
						best_left = pickedL;
						best_right = pickedR;
						come_from = 2;
					}
       			}

          /* a clist member */

       		for (clpr = m0.next; clpr != &m1; clpr = clpr->next) {
         		if ((clpl != clpr && !agi_depends(clpl->member,clpr->member)) &&
           			((pickedR = isright(clpr->member)) != NULL)) {
					cur_weight = find_weight(pickedL,pickedR) + RIGHT;
					if (cur_weight > best_weight) {
						best_weight = cur_weight;
						best_left = pickedL;
						best_right = pickedR;
						come_from = 3;
					}
        		}/*endif found right*/
	   		}/*for all clist members*/


 	/* an unsafe pair.                          */
       		if ((pickedR = Eunsafe_right(pickedL)) != NULL) {
				cur_weight = find_weight(pickedL,pickedR) + UNSAFE;
				if (cur_weight > best_weight) {
					best_weight = cur_weight;
					best_left = pickedL;
					best_right = pickedR;
					come_from = 4;
				}
       		}

		}/*endif Islefthy*/
  	}/*for all clist members*/
}/*end pair*/

static NODE *
Eunsafe_right(left) NODE *left;
{
tdependent *tp;

	if (left->may_dep)
		for (tp = left->may_dep; tp; tp=tp->next)
			if (tp->depends->nparents == 1
			&& (tp->depends->pairtype == WDA || tp->depends->pairtype == WD2))
				return tp->depends;
	return NULL;
}/*end Eunsafe_right*/

static int
find_weight(left,right) NODE *left,*right;
{
int weight = 0;
boolean agi_left,agi_right;
boolean left_has_agi_dep = has_agi_dep(left);
boolean right_has_agi_dep = has_agi_dep(right);

	agi_left = agi_depends(lastX86,left) || agi_depends(lastV,left);
	/*if right is multy cycle, it does not have AGI with last X86 */
	if (get_exe_time(right) > 1)
		agi_right = agi_depends(lastV,right);
	else
		agi_right = agi_depends(lastX86,right) || agi_depends(lastV,right);
	if (!touchMEM(left) || !touchMEM(right)) weight += NOT_MEM;
	if (agi_depends(right,left) && right_has_agi_dep) weight += REVERSE_AGI;
	if (left->agi_dep) weight += HAS_AGI_DEP;
	if (right->agi_dep) weight += HAS_AGI_DEP;
	if (! left_has_agi_dep && makes_new_agi(left)) weight -= HAS_AGI_DEP;
	if (! right_has_agi_dep && makes_new_agi(right)) weight -= HAS_AGI_DEP;
	if (agi_left && agi_right) weight += BOTH_AGI_UP;
	if (!agi_left && !agi_right) weight += NONE_AGI_UP;
	return weight;
}/*end find_weight*/

/*Is p suitable to run on the U pipe*/
static NODE *
islefthy(p) NODE * p;
{
	if ((p->pairtype == WD1) || (p->pairtype == WDA))
		return p;
	return NULL;
}/*end islefthy*/

/*Is p suitable to run on the V pipe*/
static NODE *
isright(p) NODE *p;
{
	if ((p->pairtype == WD2) || (p->pairtype == WDA))
		return p;
	return NULL;
}/*end isright*/

/*Does p have an anti dependent son which may be paired with it*/
static NODE *
Eanti_dependent(p) NODE *p;
{
tdependent * tp;
#if 0
NODE *q;
	for (q = p->forw; q != b->lastn->forw; q = q->forw) {
		if (((p->uses | p->idus) & q->sets & ~MEM & ~CONCODES)
		 && ((q->pairtype == WD2) || (q->pairtype == WDA))
		 && (q->nparents == -1))
		 return q;
	}
#endif
	for (tp = p->anti_dep; tp; tp = tp->next) {
		if (((tp->depends->pairtype == WD2) ||
			(tp->depends->pairtype == WDA)) &&
			(tp->depends->nparents == 1))
			return tp->depends;
	}
return NULL;
}/*end Eanti_dependent*/

/*Does p have a dependent son which may be paired with it*/
static NODE *
Edependent(pickedL) NODE * pickedL;
{
tdependent * tp;
	if (pickedL->op == PUSHL)
		for (tp = pickedL->dependent; tp; tp=tp->next)
			if ((tp->depends->op == CALL) && (tp->depends->nparents == 1))
				return tp->depends;
	return NULL;
}/*end Edependent*/


/*remove pointers to put p next after the previously scheduled instruction*/
static void
reorder(p) NODE *p;
{
 	if (((ptm_opts || blend_opts) && scheded_inst_no == 0 && p == lastX86) ||
     	(!ptm_opts && !blend_opts && scheded_inst_no == 0))
		b->firstn = p;
	if (isprefix(p))
		next_of_prefix = p->forw;
  	if (prev->forw != p) {
		if (p == b->lastn)
	  		b->lastn = b->lastn->back;
		p->back->forw = p->forw;
		p->forw->back = p->back;
		p->forw = prev->forw;
		p->back = prev;
		prev->forw->back = p;
		prev->forw = p;
  	}
  	prev = p;
}/*end reorder*/

static void
count_depths_before_reorder()
{
NODE *p;
unsigned int regnum;
int reg;
int amount;
int m;
char *t;
int offset;
NODE *q;
	for (p = b->firstn; p != b->lastn->forw; p = p->forw) {
		p->esp_offset = depths[7];
		for (reg = 0; reg < 8; reg++) {
			if (p->sets & regmasx[reg]) {
				if (change_by_const(p,regmasx[reg],&amount)) {
					depths[reg] += amount;
				} else {
					depths[reg] = 0;
				}
			}
		}/* for all reg*/
		if (p->sets & ESP) { /*disallow negative esp offset */
			for (q = p->back; q != b->firstn->back; q = q->back) {
				if (q->idus == ESP) {
					if (q->op1 && strchr(q->op1,'(')) m = 1;
					else if (q->op2 && strchr(q->op2,'(')) m = 2;
					else m = 0;
					if (m) {
						offset = q->esp_offset + strtol(q->ops[m],NULL,10);
						if (offset < p->esp_offset + amount) {
							install1(q,p,ANTI);
						}
					}
				}
			}
		}
		if (p->idus) {
			if (p->idus == ESP) {  /*disallow negative esp offset */
				if (p->op1 && strchr(p->ops[1],'(')) m = 1;
				else if (p->op2 && strchr(p->ops[2],'(')) m = 2;
				else m = 0;
				if (m) { /* indexing is explicite */
					offset = p->esp_offset + strtol(p->ops[m],NULL,10);
					for (q = p->back; q != b->firstn->back; q = q->back) {
						if (q->sets & ESP) {
							if (offset < q->esp_offset) {
								install1(q,p,AGI);
								break;
							}
						}
					}
				}
			}
			for (m = 1; m <=2; m++) {
				if (!p->ops[m] || (t = strchr(p->ops[m],'(')) == 0)
					continue;
				regnum = setreg(t+1); /*base reg*/
				for (reg = 0; reg < NREGS; reg++)
					if (regmasx[reg] == regnum)
						break;
				p->extra = depths[reg];
				t = strchr(t,',');
				if (t) {
					regnum = setreg(t+1); /*index register*/
					for (reg = 0; reg < NREGS; reg++)
						if (regmasx[reg] == regnum)
							break;
					p->extra2 = depths[reg];
				}
			}/*for loop*/
		}/*endif p->idus*/
	}/*end for loop*/
	for (reg = 0; reg < NREGS; reg++) /*prepare for use at prun()*/
		depths[reg] = 0;
}/*end count_depths_before_reorder*/

static void
fix_offsets(p) NODE *p;
{
int reg;
int change =0;
char name_buf[MAX_LABEL_LENGTH];
char *name;
int m,x,scale;
unsigned int base,index;
char *t,*rand,*fptr;
char sign;

	if (p->idus == 0) {
		return; /* no need to fix anything */
	}

	if (!isreg(p->op1) && ! ISFREG(p->op1) && !isconst(p->op1))
		m = 1;
	else
		m = 2;

	if (!p->ops[m]) return; /* no real index, ops[2] in null. */
	t = strchr(p->ops[m],'(');
	if (!t) return;	/*indexing only implicitely*/
	base = setreg(1+t); /*base register*/
	t = strchr(t,',');
	if (t)
		index = setreg(t+1); /*index register*/
	else
		index = 0;
	fptr = (*p->ops[m] == '*') ? "*" : ""; /*  function pointer call */
	name = (strlen(p->ops[m]) < ((unsigned ) MAX_LABEL_LENGTH)) ? 
			                       name_buf : getspace(strlen(p->ops[m])); 
	(void) decompose(p->ops[m],&x,name,&scale); /*rest components*/
	for (reg = 0; reg < NREGS; reg++) {
		if (base == regmasx[reg]) {
			change += p->extra - depths[reg]; /*zero if they are equal*/
		}
		if (index == regmasx[reg]) {
			change += (p->extra2 - depths[reg]) * scale;
		}
	}/*for all regs*/
	if (change) {
		rand = getspace(strlen(p->ops[m]) + 12);
		change +=x;
		t = strchr(p->ops[m],'(');
		if (name[0]) {
			if (change > 0) sign = '+';
			else { 
				sign = '-';
				change = -change;
			}
			sprintf(rand,"%s%s%c%d%s",fptr,name,sign,change,t);
		} else {
			sprintf(rand,"%s%d%s",fptr,change,t);
			if ((change < 0) && (p->idus == ESP)) {
				p->ops[m] = rand;
				fatal(gettxt(":1431","got a negative esp offset\n"));
			}
		}
		p->ops[m] = rand;
	}/*endif change*/
}/*end fix_offsets*/

#ifdef STATISTICS
static int
count_agi() 
{
 NODE *p;
 int count =0;
 	for (p = n0.forw; p->forw != &ntail; p =p->forw) {
		if (islabel(p)) fprinst(p);
   		if (sets(p) & ((ptm_opts || blend_opts)
			? indexing(p->forw) : base(p->forw)) &&  
   		!(ispp(p) && ispp(p->forw))) {
	 		count++;
   		}
	}
 	return count;
}/*end count_agi*/

static int
count_concurrent()
{
 NODE *p;
 int count =0,count1 = 0;
 	for (p = n0.forw; p->forw != &ntail; p =p->forw) {
   		if (FP(p->op)) {
	   		count += count1;	 
   			count1 = CONCURRENT(p->op);
		} 
		else {
			count1 -= get_exe_time(p);
			if (count1 < 0)
			 	count1 = 0;
		}
 	}
 	return count + count1;
}/*end count_concurrent*/

static void
do_stats(total,on_U,on_V,pairables)
int *total,*on_U,*on_V,*pairables;
{
 NODE *p;
 NODE *last;
 NODE *l_on_X = NULL, *l_on_J = NULL;

 	last = last_is_br ? last_is_br : b->lastn;
 	for (p=b->firstn; p != last->forw; p=p->forw) {
   		if (p->op == FXCH)
			continue;
   		/*check if the two instructions are a pair or not */
   		if (p != last && (p->pairtype == WDA || p->pairtype == WD1)
		 && (p->forw->pairtype == WDA || p->forw->pairtype == WD2)
		 && ! son_of(p->forw,p)
		 && ( agi_depends(l_on_X,p) || agi_depends(l_on_J,p) ||
	     	 ! agi_depends(l_on_J,p->forw))
		 ) {
	  	 	 (*total) +=2;
	  	 	 (*on_U)++;
	  	 	 (*on_V)++;
	  	 	 (*pairables) +=2;
	  	 	 l_on_X = p;
	  	 	 l_on_J = p->forw;
	  	 	 p=p->forw; /*the next instruction is also taken care of*/
		 }
   		else if(p->pairtype == X86) {
	 		(*total)++;
	 		(*on_U)++;
	  		l_on_X = p;
	  		l_on_J = NULL;
   		}
   		else {
	 		(*total)++;
	 		(*on_U)++;
	 		(*pairables)++;
	  		l_on_X = p;
	  		l_on_J = NULL;
   		}
	 
  	}
}/*end do_stats*/

static int
son_of(son,father) NODE *son,*father;
{
 tdependent *tp;
 
 	if (father->op == PUSHL || father->op == PUSHW)
  		if ((son->op == PUSHL || son->op == PUSHW)
    	 && isior(father->op1)
    	 && isior(son->op1))
		return 0;
  	else if (son->op == CALL)
		return 0;

  	if (father->op == POPL || father->op == POPW)
		if ((son->op == POPL || son->op == POPW)
	  	 && isreg(father->op1)
	  	 && isreg(son->op1))
		return 0;
	else if (son->op == RET)
		return 0;

  	if ((father->op == CMPL || father->op == CMPW || father->op == CMPB)
     && isbr(son))
		return 0;

 	for (tp = father->dependent; tp; tp=tp->next)
   		if (tp->depends == son)
	 		return 1;
 	for (tp = father->agi_dep; tp; tp=tp->next)
   		if (tp->depends == son)
	 		return 1;
 	return 0;
}/*end son_of*/
#endif

/*menage space allocation for tdependent structures.                */

static void
init_alloc()
{
  if (! a0.a)
	if ((a0.a = (tdependent *) calloc(ASIZE,sizeof(tdependent))) ==NULL)
	  fatal(gettxt(":429","sched: can't malloc\n"));
}/*end init_alloc*/


static tdependent *
next_dep_str()
{
  if (alloc_index < ASIZE) {
	alloc_index++;
	return &(acur->a[alloc_index-1]);
  }
  else {
	if (acur->next == NULL) {
	  acur->next =  tmalloc(alist);
      if ((acur->next->a = (tdependent *)
						   malloc(ASIZE*sizeof(tdependent))) == NULL)
	    fatal("sched: cant malloc\n");
	acur->next->next = NULL;
	}
    acur = acur->next;
    alloc_index = 1;
    return &(acur->a[0]);
  }
}/*end next_dep_str*/

/* get cycle count of integer instruction */
int
get_exe_time(p) NODE *p;
{	int time,flags;
	flags = CYCLES(p->op);
	time =  flags & 0xff;
  if (ptm_opts) {
	if (ismem(p->op1))
		time += (flags & M1) >> 8;
	else if (ismem(p->op2))
		time += (flags & M2) >> 12;
  } else {
	if (ismem(p->op1)) {
		time += (flags & M1) >> 8;
		if ( strchr(p->op1,',') )
			time++; /* Extra cycle for index in i486 */
		if ( p->op2 && *p->op2 == '$' && *p->op1 != '(' )
			time++;	 /* Displacement and immediate add one cycle */  
	}
	else if (ismem(p->op2)) {
		time += (flags & M2) >> 12;
		if ( strchr(p->op1,',') )
			time++; /* Extra cycle for index in i486 */
		if ( *p->op1 == '$' &&  *p->op2 != '(')
			time++;	 /* Displacement and immediate add one cycle */  
	}
  }
	return time;
}/*end get_exe_time*/

/* set register use bits */
	unsigned int
muses(p) NODE *p;
{

	unsigned using;
	unsigned int op;

	op = p->op > SAFE_ASM ? p->op - SAFE_ASM : p->op;
  if (ptm_opts)
	if (float_used && FP(op)) {
		return fmuses(p);
	}
	using = uses_but_not_indexing(p);
	switch (op) {

	case MOVSBL:  case MOVSBW:  case MOVSWL:
	case MOVZBL:  case MOVZBW:  case MOVZWL:
	case MOVL:  case MOVW:  case MOVB:
		using |= ismem(p->op1);
		break;
	/* special case for sched: lea has () but doesnt use memory */
	case LEAL:  case LEAW:
	case FSTCW: case FST: case FSTP: case FSTS: case FSTL: case FSTPS:
	case FSTPL: case FSTPT: case FIST: case FISTL: case FISTP: case FISTPL:
	case FISTPLL: case FBSTP: case FSAVE: case FNSAVE: case FSTENV:
	case FNSTSW:
		break;
	case POPL: case POPW: case POPA:
	case LCALL: case CALL:
	case SCMPL:  case SCMPW:  case SCMPB:
	case SMOVL:  case SMOVW:  case SMOVB:
	case SCAB: case SCAW: case SCAL: case SLODB:
	case SLODW: case SLODL: case SCASB: case SCASL:
	case SCASW: case SSCAB: case SSCAL: case SSCAW:
		using |= MEM;
		break;
	default:
		using |= (ismem(p->op1) | ismem(p->op2));
	}
	return(using);
}/*end muses*/

/* set register destination bits */
	unsigned int
msets(p) NODE *p;
{

  	int setting;
	char * dst();
	char *cp;
	unsigned int op;
	op = p->op > SAFE_ASM ? p->op - SAFE_ASM : p->op;
  	if (ptm_opts)
		if (float_used && FP(op))
			return fmsets(p);

	setting = p->sets;
	switch (op) {
	case XCHGL: case XCHGW: case XCHGB:
		return( setting | ismem(p->op1) | ismem(p->op2) );
	case SMOVL:  case SMOVW:  case SMOVB:
	case SSTOB: case SSTOW: case SSTOL: case STOSB: case STOSL: case STOSW:
		return(setting | MEM);
	case CALL:   case LCALL:
		return(setting |  MEM ) ;
	case PUSHW: case PUSHL: case PUSHA:
		return(setting |  MEM );
	default:
		cp = dst(p);
		return( setting | ismem(cp) );
	}
}/*end msets*/


	unsigned
indexing(p) NODE * p;
{
unsigned index = 0;         /* to be the result */
char *cp;
  if (p->op1 != NULL && (cp = strchr(p->op1,'(')) != NULL)
    index = scanreg(cp,false);
  else if (p->op2 != NULL && (cp = strchr(p->op2,'(')) != NULL)
    index = scanreg(cp,false);
  index &= ~MEM; 
  /* opcodes that use special registers as pointers to memory */
  switch (p->op) {
    case PUSHL: case PUSHW: case POPL: case POPW: case PUSHA:
		index |= ESP;
		break;
/* This is not an index for AGI
	case SCMPL:  case SCMPW:  case SCMPB:
	case SMOVL:  case SMOVW:  case SMOVB:
		index |= (ESI | EDI);
		break;
*/
	default: break;
	}/*end switch*/
return index;
}/*end indexing*/

/* Find base register in p. If op has the form 
	(reg1,reg2) where reg1 != reg2,
	reg1 will not be a base because postpeep can exchange
	reg1 with reg 2 if reg1 will couse AGI. 
*/
static	unsigned
base(p) NODE * p;
{
                     
	unsigned base = 0;         /* to be the result */
	char *cp,*cp1;
	int i;
	char reg_buff[12];

	if ( (p->op1 != NULL && (cp = strchr(p->op1,'(')) != NULL)
      || (p->op2 != NULL && (cp = strchr(p->op2,'(')) != NULL)) {
		for(i = 0, cp1=cp;*cp1;cp1++) /*check if may exchange base with index */
			if (*cp1 == ',')
				i++;
		if (i != 1) { /* if not 2 regs without scale */
		for(i =0 ; cp[i] && (cp[i] != ',')  ; i++)
				reg_buff[i] = cp[i];
			reg_buff[i] = '\0'; /* copy base only */
    		base = scanreg(reg_buff,false);
		}
	}
  /* opcodes that use special registers as pointers to memory */
  	switch (p->op) {
    	case PUSHL: case PUSHW: case POPL: case POPW: case PUSHA:
			base |= ESP;
			break;
/* This is not an index for AGI
		case SCMPL:  case SCMPW:  case SCMPB:
		case SMOVL:  case SMOVW:  case SMOVB:
			base |= (ESI | EDI);
			break;
*/
		default: break;
	}/*end switch*/
	return base;
}/*end base*/

static int
no_gain_in_schedule()
{
NODE *p;
unsigned int setsp;
	for(p = b->firstn; p != b->lastn->forw; p = p->forw) {
		p->usage = NONE;
		setsp = p->sets;
		if (FP(p->op) && CONCURRENT(p->op))
			return false;
		else if (! ispp(p) && (setsp & base(p->forw)))
			return false;
		if ((p->sets & p->forw->uses & ~(MEM | CONCODES)) /* sets and uses same reg */
			 && ! (p->sets & R16MSB) /* less then full reg set */
			 && (p->forw->uses & R16MSB)) /* full reg uses */
			return false;
	}/*for loop*/
	return true;
}/*end no_gain_in_schedule*/

/* find_big_block_end(b) Return b if it was not bracked because of size or 
**	the block with last part of the original basic block before breaking it.
*/ 
static BLOCK *
find_big_block_end(b) BLOCK *b; {
	for (;b != NULL ; b = b->next) {	
		if ( isbr(b->lastn) || islabel(b->lastn) || b->lastn->op == CALL ||
		  isbr(b->lastn->forw) || islabel(b->lastn->forw) || 
		  b->lastn->forw->op == CALL || b->lastn->op == ASMS || 
		  is_safe_asm(b->lastn) || b->next == NULL)
			return b;
	}
	/* NOTREACHED */
}

/* Some Floating point scheduling code */

/*
 b->marked bits are used for : 
	3  - 0  : entry. Num. of regs in entry point used by this basic block
	7  - 4  : use.   Max number of registers used in the basic block 
	11 - 8  : exit.  Num of regs used in the basic block and not poped there.
	15 - 12 : status clear or processed
*/


#define PROCESSED 0x7000
#define is_clear(b)	((b->marked & 0xf000) == 0)

/*  Set the value of use and exit */
static void
check_fp_stack(b,last) BLOCK *b; NODE *last;
{
NODE * p;
int entry, use, cur_stack, max_use;
boolean open_call = 0;
		
	max_use = entry = cur_stack = ENTRY(b); 
    for (p=b->firstn; p != last->forw ; p = p->forw) {
		if (p->op == CALL) {
			open_call = true;
		}
		if (FP(p->op)) { /* if FP instruction */	
			if (FNOST(p->op)) /* If no use of stack */
				continue;
			use = 0;
			if (isreg(p->op1) && p->op1[1] == 's' && p->op1[3] == '(')
				use = p->op1[4] - '0'; /* Find the deepest reg used by op1 */
			if (isreg(p->op2) && p->op2[1] == 's' && p->op2[3] == '(')
				use = p->op2[4] - '0'; /* Find the deepest reg used by op2 */
			if (FST1_SRC(p->op) && (use < 1))
				use = 1; /* Find the deepest reg used by this op */
			if ((p->op1 == NULL) && (FNOARG(p->op)))
				use = 1;
			/* if push add one to current st location */
			if (FPUSH(p->op) ||
				((p->op == FSTP || p->op == FST) && use >= cur_stack)) {
				cur_stack++;
				if ( cur_stack > max_use) /* if we didn't use it mark as used*/ 
					max_use = cur_stack;
			}
			/* Was it the deepest ever in this block?*/
			if ((cur_stack <= use) && !open_call) {
				entry += 1 + use - cur_stack;
				cur_stack = use + 1;
			}
			if ( use > max_use) /* set max use */
				max_use = use;
			
			if (FPOP(p) && !(p->op == FSTP && use > cur_stack)) {
				if (cur_stack) cur_stack--;
				else if (open_call) open_call = false;
				else  entry++;
			}
			if (p->op == FCOMPP) { /* FCOMPP will pop twice */
				if (cur_stack >= 2) cur_stack -= 2;
				else if (open_call && cur_stack) {
					cur_stack = 0;
					open_call = false;
				} else {
					cur_stack = 0;
					open_call = false;
					entry++;
				}
			}	 
		}/*endif FP*/
	} /* end for the big block */
	if (entry < 0 || entry > 8 || max_use < 0 || max_use > 8 || cur_stack < 0
		|| cur_stack > 8) {
		fprinst(b->firstn);
		fatal(gettxt(":430","calcs out of bound, entry %d max %d exit %d\n"),entry,max_use,
			cur_stack);
	}
	b->marked &= 0xf000;
	b->marked |= entry | (max_use << 4) | (cur_stack << 8); /*mark block data*/
}

void
check_fp()
{
BLOCK *b,*b1;

	set_refs_to_blocks(); /* connect switch tables nanes to blocks */
	for (b = b0.next; b ; b = b->next)
		b->marked = 0;
	for (b = b0.next; b ; ) {
		if (is_clear(b)) {
			b1 = find_big_block_end(b);
			check_fp_block(b,b1->lastn);
			b = b1->next;
		} else b = b->next;
	}
}

static void
check_fp_block(b,last) BLOCK *b; NODE *last;
{
BLOCK *b1,*b2;
REF *r;
SWITCH_TBL *sw,*get_base_label();
	b->marked |= PROCESSED;
	for (b1 = b; b1->lastn != last; b1 = b1->next)
		b1->marked |= PROCESSED; /* mark all block in the big block */
	check_fp_stack(b,last);
	if (b->nextl) {
		if  ((is_clear(b->nextl)) || (exit(b) != ENTRY(b->nextl))) {
			set_entry(b->nextl,exit(b));
			b1 = find_big_block_end(b->nextl);
			check_fp_block(b->nextl,b1->lastn);
		}
	}
	if (b->nextr) {
		if  ((is_clear(b->nextr)) || (exit(b) != ENTRY(b->nextr))) {
			set_entry(b->nextr,exit(b));
			b1 = find_big_block_end(b->nextr);
			check_fp_block(b->nextr,b1->lastn);
		}
	}
	if (is_jmp_ind(b->lastn)) {
		sw = get_base_label(b->lastn->op1);
		for (r = sw->first_ref; r; r = r->nextref) {
			if (sw->switch_table_name == r->switch_table_name) {
				b1 = r->switch_entry;
				if ((is_clear(b1)) || (exit(b) != ENTRY(b1))) {
					set_entry(b1,exit(b));
					b2 = find_big_block_end(b1);
					check_fp_block(b1,b2->lastn);
				}
			}
			if ( r == sw->last_ref )
				break;
		}
	}
}

static BLOCK *last_block = NULL; /* The last block that got virtual FP regs */

static int fp_stack[8] = { 0,0,0,0,0,0,0,0}; /* Stack used by set_fp_regs */
static int tos = -1;  /* Pointer to the input top of stack */
              
static int fp_free[8] = { 0,1,2,3,4,5,6,7};  /* free registers pool */
static int free_regs = 8;      /* index in fp_free */
static int input_stack[8] = { 0,0,0,0,0,0,0,0}; /* block entry point regs */
static int input_stack_index = 0; /* index in input_stack */
static int output_stack[2] = { 0, 0};       /* regs in the block exit point */
static int code_tos = -1;  /* top of stack in the output code stack */ 
static int code_fp_stack[8];  /* output code stack */
static char *fp_str[8] = {"FF#0","FF#1","FF#2","FF#3",
                   "FF#4","FF#5","FF#6","FF#7"}; /* virtual registers names */
static char *fkill_str[8] = {"FK#0","FK#1","FK#2","FK#3",
                   "FK#4","FK#5","FK#6","FK#7"}; /*Marker for register kill */
static char *fnew_str[8] = {"FN#0","FN#1","FN#2","FN#3", /*Marker for new register */
                   "FN#4","FN#5","FN#6","FN#7"};




static void
init_fregs(b) BLOCK *b; { /* initialize data for FP register transformation */ 
	int i;
	tos = -1;

	input_stack_index = 0;
	/*free_regs = 8 - ((b->marked >>4) & 0xf);*/
	free_regs = 8 - ((b->marked ) & 0xf);
	if ( ((b->marked & 0xf0) != 0x80) && (b->lastn->op != CALL))
		free_regs--; /* We can have up to one register error in case block
** after call. So use only 7 regs if in the original block less the 8 were used
   and there was no call in this block */ 
	for ( i = 7; i >=0 ; i--)
	{	fp_free[i] = i; /* Get different free reg every time */
		fp_stack[i] = 0;
	}
}
	
/* get one free reg */
static char *
getfreg(p) NODE * p;
{
	int reg,i;

    if (tos > 7) /* can't have more then 8 FP regs */
		fatal(gettxt(":1433","no reg to get\n"));
    reg = fp_free[0]; /* get the free reg */
    p->op3 = fnew_str[reg];
    free_regs--;
    for ( i = 0; i < free_regs ; i++) /* rotate the free regs by one */
        fp_free[i] = fp_free[i+1];
    fp_stack[++tos] = reg; /* add this reg to the FP stack */
    return fp_str[reg]; /* return the new reg */
}

/* If reg to the bottom of the stack if used without push */
static void 
addfreg()
{
	int reg,i;

    if (tos  > 7) /* can't have more then 8 FP regs */
		fatal(gettxt(":1433","no reg to get\n"));
    reg = fp_free[0];
    free_regs--;
    for ( i = 0; i < free_regs ; i++)
        fp_free[i] = fp_free[i+1];
    for (i = tos +1 ; i ; i--) /* Move all the stack by one up */
        fp_stack[i] = fp_stack[i-1];
    fp_stack[0] = reg;
    tos++;
}

static void 
freefreg(p) NODE * p; { /* pop fp reg and free the stack */

    if (tos < 0) /* can't free reg if not on the stack */
		fatal(gettxt(":1434","no reg to free\n"));
    p->op3 = fkill_str[fp_stack[tos]];
    fp_free[free_regs++] = fp_stack[tos--]; /* Pop it and add as last free */
}

static int last_st_num; /* mark the number of the last st_num found by st_num */

/* convert %st(?) reg to FF#?  */   
static char *
st_num(cp) char *cp;
{
	int offset;
    last_st_num = -1;
    if (cp == NULL)         
        return cp;  
    if (cp[0] == '%' && cp[1] == 's' && cp[2] == 't') { /* if %st[(?)] */   
        last_st_num = offset =  (cp[3] == '(') ? (cp[4] -'0') : 0;
        if ( offset <= tos) /* the reg is on the stack */
            return fp_str[fp_stack[tos - offset]];
        while ( offset > tos) { /*if not pushed it yet, add it to the bottom */
            addfreg();
            input_stack[input_stack_index++] = fp_stack[0];
        }
        return fp_str[fp_stack[0]];
    }
    return cp;
}

static void
break_fi(b,p) BLOCK *b; NODE*p;  /* split FI?? to FILD and F?? */
{
int newop;
char *newopcode;
NODE *pn;
	switch (p->op) {
		case	FIADD:
						newop = FADD; newopcode = "fadd";
						break;
		case	FIADDL:
						newop = FADD; newopcode = "fadd";
						break;
		case	FICOM:
						newop = FCOM; newopcode = "fcom";
						break;
		case	FICOML:
						newop = FCOM; newopcode = "fcom";
						break;
		case	FIDIV:
						newop = FDIVR; newopcode = "fdivr";
						break;
		case	FIDIVL:
						newop = FDIVR; newopcode = "fdivr";
						break;
		case	FIDIVR:
						newop = FDIV; newopcode = "fdiv";
						break;
		case	FIDIVRL:
						newop = FDIV; newopcode = "fdiv";
						break;
		case	FIMUL:
						newop = FMUL; newopcode = "fmul";
						break;
		case	FIMULL:
						newop = FMUL; newopcode = "fmul";
						break;
		case	FISUB:
						newop = FSUBR; newopcode = "fsubr";
						break;
		case	FISUBL:
						newop = FSUBR; newopcode = "fsubr";
						break;
		case	FISUBR:
						newop = FSUB; newopcode = "fsub";
						break;
		case	FISUBRL:
						newop = FSUB; newopcode = "fsub";
						break;
		default:
			return; /*dont change other opcodes*/
	}/*end switch*/
	pn = insert(p);
	if (OpLength(p) == WoRD)
		chgop(p,FILD,"fild");
	else if (OpLength(p) == LoNG)
		chgop(p,FILDL,"fildl");
	else {
		fatal(gettxt(":1435","Unexpected op length\n"));
	}
	chgop(pn,newop,newopcode);
	pn->op1 = pn->op2 = NULL;
	pn->sets = sets(pn);
	pn->uses = uses_but_not_indexing(pn);
	pn->idxs = indexing(pn);
	b->length++;
	if (p == b->lastn)
		b->lastn = pn;
	if (newop == FCOM) {
		pn = insert(p);
		chgop(p,FXCH,"fxch");
		p->op1 = p->op2 = NULL;
		new_sets_uses(pn);
		b->length++;
	}/*endif FCOM*/
}/*end break_fi*/

/* This is the main entry point for register converting from stack to virtual
** it will check if the we do not have more then 2 registers in the exit point
** of the block. Then if we are not using unexpected floating point 
** instructions.
** Then it will emulate the fp stack and convert the operands from stack to 
** vertual registers block. For example:
**  "flds a(%eax)" will be "flds a(%eax),FF#0"
*/
static void
set_fp_regs(b) BLOCK * b; { /* replace fp stack with registers */
	NODE * p, * last_p;
	BLOCK *last_b;
	int reg_num1,tmp,i;
	int use;

	last_b = find_big_block_end(b);
	if (last_b == last_block) /* If we did this block already */
		return;
	last_block = last_b;
	last_p = last_block->lastn->forw;
	if ((b->marked & 0xf00) > 0x200) /* Can't do FP with more then 2 regs */
		return;                      /* in the end of basic block.        */
/*for all instructions in the block from the top to the bottom */
    for (p=b->firstn; p !=last_p; p = p->forw) {	
		if (FP(p->op) && FUNEXPECTED(p->op))
			return; /* using unexpected instructions */
	}
	init_fregs(b);
    for (p=b->firstn; p !=last_p; p = p->forw) 
    {	if (p == b->lastn->forw)
    		b = b->next; /* Must know the current block to decrement length */
        if (FP(p->op)) { /* if it is floating point instruction */
            float_used = 1;
			if (isreg(p->op1) && p->op1[3] == '(') use = p->op1[4] - '0';
			else if (isreg(p->op2) && p->op2[3] == '(') use = p->op2[4] - '0';
			else use = 0;
			if (free_regs && p->opcode[1] == 'i')
				break_fi(b,p);  /* split FI?? to FILD and F?? */
            if (p->op == FXCH && (p->op1 == NULL)) 
				p->op1 = "%st(1)";
            if (FNOARG(p->op) && (p->op1 == NULL)) {
            	if (p->op == FCOM || p->op == FCOMP)
                    p->op1 =  st_num("%st(1)");
                else {
                	switch(p->op) { /* "F{OP}" == "F{OP}P %st,%st(1)"   */
                    	case FADD: 
                            p->op = FADDP;
                            p->opcode = "faddp";
                            break;
                        case FDIV:
                            p->op = FDIVP;
                            p->opcode = "fdivp";
                            break;
                        case FDIVR:
                            p->op = FDIVRP;
                            p->opcode = "fdivrp";
                            break;
                        case FMUL:
                            p->op = FMULP;
                            p->opcode = "fmulp";
                            break;
                        case FSUB:
                            p->op = FSUBP;
                            p->opcode = "fsubp";
                            break;
                        case FSUBR:
                            p->op = FSUBRP;
                            p->opcode = "fsubrp";
                            break;
                        default:
                            break;
                    }
                    p->op1 =  st_num("%st");
                    p->op2 =  st_num("%st(1)");
                }
            }
            p->op1 = st_num(p->op1); /* if %st(?) used convert to FF#x */
            reg_num1 = tos - last_st_num;
            p->op2 = st_num(p->op2);
            if (p->op == FXCH) { /*Change regs on the stack and delete fxch */
            	tmp = fp_stack[reg_num1];
                fp_stack[reg_num1] = fp_stack[tos];
                fp_stack[tos] = tmp;
                b->length--;
                if (b->firstn == p)
                	b->firstn = p->forw;
                if (b->lastn == p)
                	b->lastn = p->back;
                DELNODE(p);
                continue;
            }
            if (p->op == FCOMPP) /* bug fix: p->op1 is " " and not NULL */
                p->op1 = NULL;
            if (FST1_SRC(p->op) && (p->op1 == NULL)) /* only FCOMPP */
                p->op1 =  st_num("%st(1)");
            if (FST_SRC(p->op)) { /* if source is %st(0) */ 
            	p->op2 = p->op1;
                p->op1 =  st_num("%st");
            } else if (FST_DEST(p->op)) /* if destination is %st(0) */
                p->op2 = st_num("%st"); 
            if (FPUSH(p->op) || (p->op == FSTP && use > tos)) {
            	if (p->op1 == NULL)
                    p->op1 = getfreg(p);
                else
                    p->op2 = getfreg(p);
            }   
            else if (FPOP(p) && !(p->op == FSTP && use > tos))
                freefreg(p);
            else if (p->op == FCOMPP) { /* FCOMPP, special case: pop twice */
                freefreg(p);
                freefreg(p);
            }
        }
    }
    if (tos == 1) { /* Mark the output stack reset regs may add fxch */	
    	output_stack[0] = fp_stack[0];
    	output_stack[1] = fp_stack[1];
    } else if (tos > 1) /* no set_fp_regs with more then 2 exit regs */
    	fatal(gettxt(":1436","set fp stack internal error \n"));
    for (i = 0; i < input_stack_index; i++)  /* copy the input to code stacks */
		code_fp_stack[++code_tos] = input_stack[input_stack_index - (1 +i)];
	input_stack_index = 0;
    
}

/* service functions for virtual to stack conversion */ 

static int
str2reg(reg_string) char *reg_string; { /* convert FF#x to x */ 

   if (reg_string[0] == 'F' && reg_string[1] == 'F' && reg_string[2] == '#')
        return (reg_string[3] - '0');
    fatal(gettxt(":1437","sched: get_reg with %s and not reg\n"),reg_string);
    return 0;
}
static char *st_str[8] = {"%st","%st(1)","%st(2)","%st(3)",
                          "%st(4)","%st(5)","%st(6)","%st(7)"};
/* convert FF#x to %st(?) */   
static char *
replace_freg(reg_string) char *reg_string;
{
	int reg,i;
    if (reg_string[0] == 'F' && reg_string[1] == 'F' && reg_string[2] == '#') {
    	reg = reg_string[3] - '0';
        for (i = code_tos  ; 0 <= i ; i--) {   /* find the reg in the stack */
        	if ( code_fp_stack[i] == reg)
                return st_str[code_tos -i]; /* return it's string */
        }
        fatal(gettxt(":1438","reg %d not found in stack\n"),reg);
    }
    return reg_string; /* return the string if FF reg was not found */
}

static void 
fxch(reg,last_float) int reg; NODE *last_float; { /* Add fxch if needed */
   int i;
	NODE *new;
    if (reg == code_fp_stack[code_tos]) /* check if reg is O.K. */
    	return; /* fxch is not needed */
    for (i = code_tos - 1 ; 0 <= i ; i--) { /* find the reg in the stack */
    	if ( code_fp_stack[i] == reg)  { /* if reg found */ 
			code_fp_stack[i] = code_fp_stack[code_tos];
            code_fp_stack[code_tos] = reg;  /* replace reg on stack */
            new = insert(last_float); /* add fxch after last scheduled FP */
			new->op = FXCH;
			new->opcode = "fxch\0";
			new->op1 = st_str[code_tos - i];
			new->op2 = new->op3 = NULL;
			if (vflag) {
				new->op4 = "\t/ added\0";
			}
			new->dependent=new->anti_dep=new->agi_dep =new->may_dep = NULL;
			return;
       }
    }
}

static void
clean_fp_regs(b) BLOCK * b; {/* Some cleanup in the end of big basic block */
	if (b != last_block) /* If not the end of BIG basic block */
		return; 
    if (code_tos == 1 && output_stack[1] != code_fp_stack[1]) 
		fxch(output_stack[1],last_float); 
    code_tos = -1;
    float_used= 0;
    last_block = NULL;
}

static int
check_for_fxch(p) NODE *p; { /* check if instruction p will add fxch */    
	if (! float_used)
		return false;
    if (p->op3 != NULL && p->op3[1] == 'N') /* new reg */
    	return false; /* push will never add fxch */
	if (p->op == FCOMPP) {
		if (code_fp_stack[code_tos] != str2reg(p->op1))
			return true;
		 return code_fp_stack[code_tos -1] != str2reg(p->op2);
    } else if (p->op3 != NULL && p->op3[1] == 'K') /* kill reg */
    	return code_fp_stack[code_tos] != p->op3[3] - '0'; /* pop from tos */
	else if  (FST_SRC(p->op)) 
	 	return code_fp_stack[code_tos] != str2reg(p->op1); /* src is tos */
	else if (FST_DEST(p->op)) /* dest is tos */
    {   if (p->op2 == NULL) /* if dest is op2 */
        	return code_fp_stack[code_tos] != str2reg(p->op1);
        else
        	return code_fp_stack[code_tos] != str2reg(p->op2);
    }
    return false;
}

static void
reset_fp_reg(p) NODE * p; { /* Convert back the virtual registers to stack */
	int reg;
    NODE *new = NULL;
  	fp_set_resource(p); /* Mark resources for the scheduler */
    if (! float_used)
    	return;
    if (p->op3 != NULL && (p->op3[1] == 'N')) /* new reg Do push */
    {   if (p->op2 == NULL) {    /* FLDZ,FLD1 etc. */
	    	code_fp_stack[++code_tos] = str2reg(p->op1);
			p->op1 = NULL;
		} else {
		    p->op1 = replace_freg(p->op1);
    		code_fp_stack[++code_tos] = str2reg(p->op2);
		}
        p->op3 = p->op2 = NULL;
	}
    if (p->op == FCOMPP) /* Do pop twice */
    {   reg = str2reg(p->op1);
        fxch(reg,last_float);
        reg =str2reg(p->op2);
        if (reg != code_fp_stack[code_tos - 1]) { /* if PO2 != %st(1) */
			p->op = FCOMP; /* do: fcomp %st(?); fxch %st(?) ; fstp %st(0) */
            p->opcode = "fcomp";
            p->op1 = replace_freg(p->op2);
            p->op2 = NULL;
			new = insert(p);
			new->op = FSTP;
			new->opcode = "fstp";
			new->op1 = "%st(0)";
			new->op2 = new->op3 = NULL;
			new->op4 = "\t/ added";
			new->dependent=new->anti_dep=new->agi_dep = NULL;
			reorder(new);
			last_float = p;
        } else
            p->op1 = p->op2 = NULL;
        code_tos -= 1;
    } else if  (FST_SRC(p->op)) { /* set %st as op1 */
    	reg =  str2reg(p->op1);
        fxch(reg,last_float);
        p->op1 = p->op2;
        p->op2 = NULL;
        if (p->op == FSTP && ISFREG(p->op1) && reg == str2reg(p->op1))
        	p->op1 = "%st(0)"; /* fstp must have %st(0) and not %st */
    } else if (FST_DEST(p->op)) { /* set destination as %st */
		if (p->op2 == NULL) {
			reg =  str2reg(p->op1);
            p->op1 = NULL;
        } else {
		reg =  str2reg(p->op2);
            p->op2 = NULL;
        }
		
        fxch(reg,last_float);
    }
    if (p->op3 != NULL && p->op3[1] == 'K')  { /* kill reg from top of stack */
		reg = p->op3[3] - '0';
        fxch(reg,last_float);
    } else 
    if (    ISFREG(p->op2) 
        && ((p->op2[3] - '0') != code_fp_stack[code_tos]
        || (p->op1[3] - '0') != code_fp_stack[code_tos]))
    	 /* for "F* %st(i),%st(j)"  i or j must be 0 */
        fxch(p->op2[3] - '0',last_float);
    if (p->op1 != NULL)
        p->op1 = replace_freg(p->op1);
    if (p->op2 != NULL)
        p->op2 = replace_freg(p->op2);
    if (p->op3 != NULL && p->op3[1] == 'K') /* pop reg */
        code_tos--;
    last_float = (new != NULL) ? new : p;
    p->op3 = NULL;
    if ((FNOARG(p->op) || p->op == FLD) && strcmp(p->op1,"%st") == 0)
    	p->op1 = "%st(0)";	
	if (FPOP(p) && p->op2 && !strcmp(p->op2,"%st")) p->op2 = "%st(0)";
}


static unsigned int
fmsets(p) NODE *p; { /* set new FP register destination bits */
    char *cp;
	unsigned int op;
	op = p->op > SAFE_ASM ? p->op - SAFE_ASM : p->op;
    if (op == FSTSW)
        return EAX;
    if (FDEST1(op))
        cp = p->op1;
    else if FDEST2(op)
        cp = p->op2;
    else 
        return 0;
    if (cp != NULL && cp[2]  != '#' )
        return  MEM;
    return 0;
}


static unsigned int
fmuses(p) NODE *p; { /* set new FP register  use bits */
    if (p->op1 && FSRC1(p->op) && p->op1[2] != '#') 
        return  MEM;
    if (p->op2 && FSRC2(p->op) && p->op2[2] != '#')  
        return  MEM;
    return 0;
}
static NODE *fp_regs[8];

static void 
add_fp_dependency(b) BLOCK * b; { /* add dependency caused by fp regs */
	NODE *p,*p1 = NULL;
    NODE *p_comp = NULL;
    int i,reg,reg1;

    if (! float_used) {/*no virtual regs, every fp depend on it's predecessor*/
    	for (p=b->firstn; p !=b->lastn->forw; p = p->forw) {
    	   if (FP(p->op) || (p->op == CALL)) {   
				if (p1 != NULL)
                    install1(p1,p,DEP);
                p1 = p;
            }
        }
        return;
    }
    for (i = 0 ; i< 8 ; i++)
        fp_regs[i] = NULL;   
/* Forward dependency are from the type
** (1)	Fop1 FF#1,FF#2
** (2)  Fop2 FF#2,FF#3
** (2) is forward depend on (1) */
 
    for (p=b->firstn; p !=b->lastn->forw; p = p->forw) {/* forward dependency */
    	if (FP(p->op)) {   
        	if (FSETCC(p->op))
                p_comp = p;
            if (p->op == FSTSW && p_comp != NULL) /* FSTSW depends on fcom */
                install1(p_comp,p,DEP);
            if (p->op == FLDCW) { /* Every fp depends on the control word */
        		for (i = 0 ; i< 8 ; i++)
            	    fp_regs[i] = p;
        	}   
            for (reg = -1,i = 1 ; i < 3 ; i++) { /* add register dependency */
            	if ((p->ops[i] != NULL) && (p->ops[i][2] == '#')) {   
                	reg1 = p->ops[i][3] - '0';
                    if (reg == reg1) /* fadd F1,F1 will be linked only once */
                        break;
                    reg = reg1;
                    if ((p1 = fp_regs[reg]) != NULL)
                        install1(p1,p,DEP);
                }
            }
            if (p->op3 && ((p->op3[1] == 'N') || (p->op3[1] == 'K')))
                fp_regs[p->op3[3] - '0'] = p; /* Add kill/new reg dependency */
            if (p->op == FCOMPP) /* Kill without FK setting */
                fp_regs[p->op1[3] - '0'] = p;
            if (FDEST1(p->op) && (p->op1[2] == '#'))
                fp_regs[p->op1[3] - '0'] = p;
            if (FDEST2(p->op) && (p->op2[2] == '#'))
                fp_regs[p->op2[3] - '0'] = p;
        }
        else if (p->op == CALL) /* Don't schedule FP over calls */
        	for (i = 0 ; i< 8 ; i++)
                fp_regs[i] = p;
    }
    for (i = 0 ; i< 8 ; i++)
        fp_regs[i] = NULL;
    p_comp = NULL;   
/* Backward dependency can be from the type 
** (1)	Fop1 FF#1,FF#2
** (2)  Fop2 FF#3,FF#1
** (2) is backward  depend on (1) */
 
    for (p=b->lastn; p !=b->firstn->back; p = p->back) { 
    	if (FP(p->op)) {   
        	if (FSETCC(p->op))
                p_comp = p;
            if ((p->op == FSTSW) && (p_comp != NULL)) /*fcom depends on FSTSW */
                install1(p,p_comp,DEP);
            if (p->op == FLDCW) /* Every fp depends on the control word */
        	    for (i = 0 ; i< 8 ; i++)
            	    fp_regs[i] = p;
            for (reg = -1,i = 1 ; i < 3 ; i++) { /* add register dependency */
            if ((p->ops[i] != NULL) && (p->ops[i][2] == '#')) {   
                	reg1 = p->ops[i][3] - '0';
                    if (reg == reg1) /* fadd F1,F1 will be linked only once */
                        break;
                    reg = reg1;
                    if ((p1 = fp_regs[reg]) != NULL)
                        install1(p,p1,DEP);
                }
            }
            if (p->op == FCOMPP) /* Kill without FK setting */
                fp_regs[p->op1[3] - '0'] = p;
            if (p->op3 && ((p->op3[1] == 'N') || (p->op3[1] == 'K'))) 
                fp_regs[p->op3[3] - '0'] = p; /* Add kill/new reg dependency */
            if (FDEST1(p->op) && (p->op1[2] == '#'))
                fp_regs[p->op1[3] - '0'] = p;
            if (FDEST2(p->op) && (p->op2[2] == '#'))
                fp_regs[p->op2[3] - '0'] = p;
        }
        else if (p->op == CALL)
			for (i = 0 ; i< 8 ; i++)
                fp_regs[i] = p;
    }
}
static int resource_regs[11];
#define resource_fp resource_regs[8] 
#define resource_mul resource_regs[9] 
#define resource_com resource_regs[10] 
static void 
fp_ticks(time) int time; { /* The clock ticks time cycles, set the resources */
    int i;
    if ( ! time) /* The clock did not tick */
        return;
    for (i = 10 ; i >= 0  ; i--) { /* decrement all resources */
        if (resource_regs[i] > time)
            resource_regs[i] -= time;
        else
            resource_regs[i] = 0;
    }
}

int 
fp_time(p) NODE *p; { /* How much time will we wait for this instruction ? */
	int time = 0;
    if (float_used) {
    	if (FSRC1(p->op) && (p->op1[2] == '#')) /* time for reg to have it's data */
        	time = resource_regs[p->op1[3] - '0'];
    	if (FSRC2(p->op) && (p->op2[2] == '#'))
        	time = MAX(resource_regs[p->op2[3] - '0'],time);
  
    } /* else Check for TOS (2 source registers one of them must be TOS */ 
	else if (blend_opts) {
    	return resource_fp;
	} else if ( FST_SRC(p->op) || (FSRC1(p->op) && FSRC2(p->op)))
    		time = resource_regs[0];
    if ( time && (p->op > FSTPT || p->op < FST))
    	time--; 
    if (resource_mul > time)
    	time = resource_mul;
    if (resource_fp > time)
    	time = resource_fp;
	if (p->op == FSTSW) 
        time = MAX(resource_com,time);
    return time;
}


static void 
fp_set_resource(p) NODE *p; { /* set resources used by this instruction */
	if (!ptm_opts) {
        resource_fp =  CONCURRENT(p->op);
		return;
	}
   	if (p->op >= FST && p->op <= FSTPT)
	   fp_ticks(2+fp_time(p));
	else	
	   fp_ticks(1+fp_time(p));
   	if (F_FLD(p->op) || p->op == FABS)
        return;
    if (F_DIV(p->op))
        resource_fp = 11;
    else if (FSETCC(p->op))
		resource_com = 3;
	else if (F_MUL(p->op))
		resource_mul = 1; /* no back to back mul */
    if (float_used) {
	    if (FDEST1(p->op) && (p->op1[2] == '#'))
    	    resource_regs[p->op1[3] - '0'] = 3;
    	if (FDEST2(p->op) && (p->op2[2] == '#'))
        	resource_regs[p->op2[3] - '0'] = 3;
	} else { /* Chech only for TOS setting.   */
		if (FPOP(p) || F_FLD(p->op) || p->op == FXCH || FSETCC(p->op)
			|| ((p->op2) && !strcmp(p->op2,"%st"))) 
			   resource_regs[0] = 0;
		else
			   resource_regs[0] = 3;
	}
}
static void 
fp_start() { /* reset all resources */
   int i;
    for (i = 10 ; i >= 0  ; i--)
        resource_regs[i] = 0;
}
