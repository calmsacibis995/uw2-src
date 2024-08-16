/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cg:i386/local.c	1.32.1.24"
/*	local.c - machine dependent stuff for front end
 *	i386 CG
 *		Intel iAPX386
 */

#ifdef NVLT
#ifndef _KMEMUSER
#define _KMEMUSER
#endif
#endif

#include <signal.h>
#include "mfile1.h"
#include "mfile2.h"
#include <string.h>
#include <memory.h>
#include <unistd.h>

#ifdef NVLT
#include <sys/nwctrace.h>
int nvltflag = 0;
	 /* default mask, override with -2N0x... */
unsigned int nvltMask = NVLTM_prof;
#endif NVLT

/* register numbers in the compiler and their external numbers:
**
**	comp	name
**	0-2	eax,edx,ecx
**	3	fp0
**	5-7	ebx,esi,edi
*/

#ifndef TMPDIR
#define TMPDIR	"/tmp"		/* to get TMPDIR, directory for temp files */
#endif

/* bit vectors for register variables */

#define REGREGS		(RS_BIT(REG_EBX)|RS_BIT(REG_ESI)|RS_BIT(REG_EDI))
#define CREGREGS	(RS_BIT(REG_EBX))

/* *** i386 *** */
RST regstused = RS_NONE;
RST regvar = RS_NONE;
/* *** i386 *** */

int tmp_start;			/* start of temp locations on stack */
char request[TOTREGS];	/* list of reg vars used by the fct.
				   Given on either BEGF node or ENDF node */
int r_caller[]={-1};		/* no caller save register */
static int biggest_return;
static int ieeeflag = 1;
static int hostedflag = 1;
static int inlineallocaflag = 0;

static char *tmpfn;
static FILE *tmpfp;

FILE *fopen();
int proflag = 0;
int picflag = 0;
int gotflag;

static int callflag = 0;

int
ieee_fp()
{
	return ieeeflag;
}

int
inline_alloca()
{
	return inlineallocaflag;
}

int 
hosted()
{
	return hostedflag;
}

void
p2abort()
{
	extern int unlink();
	if (tmpfp)
		(void) unlink(tmpfn);
	return;
}

extern thrash_thresh;
void
myflags(cpp)
char **cpp;
{
	/* process flag pointed to by *cpp.  Leave *cpp pointing
	** to last character processed.  All these flags set
	** using -2...
	*/
	switch (**cpp)
	{
		case 'A':	++*cpp; inlineallocaflag = **cpp - '0'; break;
		case 'C':	callflag = 1; break;
		case 'h':	++*cpp; hostedflag = **cpp - '0'; break;
		case 'i':	++*cpp; ieeeflag = **cpp - '0'; break;
		case 'k':	picflag = 1; break;
#ifdef NVLT
		case 'N': {
			char *remember;	
			nvltflag = 3;
			remember = ++*cpp; /* Point at first digit */
			nvltMask = strtol(*cpp,cpp,0);
			if(*cpp == remember) nvltMask = NVLTM_prof;
			--*cpp; /* Point at last digit or N if none */
		}
#endif NVLT
		case 't': {     /* -2t<number> set thrashing limit */
			++*cpp; /* Point at first digit */
			thrash_thresh = strtol(*cpp,cpp,10);
			--*cpp; /* Point at last digit or t if none */
			break;
		}
		default:	break;
	}
}

#define LOC_ATTRIBUTE	01	/* attribute required */
#define LOC_SEEN	02	/* We have entered this section before */
#define LOC_SECTION	04	/* .section required */

static int location_map[] = {
	/* PROG */	0,
	/* ADATA */	1,
	/* DATA */	1,
	/* ISTRNG */	2,
	/* STRNG */	2,
	/* CDATA */	3,
	/* CSTRNG */	4
};
struct location {
	char *name;
	char *attributes;
	unsigned int flags;
} location[] = {
	/* PROG */ 		{ ".text", "ax", 0 },
	/* ADATA, DATA*/	{ ".data", "aw", 0 },
	/* ISTRNG, STRNG */	{ ".data1", "aw", LOC_SECTION|LOC_ATTRIBUTE },
	/* CDATA */		{ ".rodata", "a", LOC_SECTION|LOC_ATTRIBUTE },
	/* CSTRNG */		{ ".rodata1", "a", LOC_SECTION|LOC_ATTRIBUTE }
};

static void 
print_location(s)
int s;
{
	struct location *loc = &location[location_map[s]];
	if (loc->flags & LOC_SECTION)
		emit_str("\t.section");
	fprintf(outfile, "\t%s", loc->name);
	if ((loc->flags & (LOC_ATTRIBUTE|LOC_SEEN)) == LOC_ATTRIBUTE) {
		fprintf(outfile, ",\"%s\"\n", loc->attributes);
		loc->flags |= LOC_SEEN;
	} else
		emit_str("\n");
}

void
cg_section_map(section, name)
int section;
char *name;
{
	struct location *loc = &location[location_map[section]];
	loc->name = name;
	loc->flags = LOC_SECTION|LOC_ATTRIBUTE;
}

extern int lastalign; 

/* location counters for PROG, ADATA, DATA, ISTRNG, STRNG, CDATA, and CSTRNG */
locctr(l)		/* output the location counter */
{
	static int lastloc = UNK;
	int retval = lastloc;		/* value to return at end */

	curloc = l;
	if (curloc != lastloc) lastalign = -1;

	switch (l)
	{
	case CURRENT:
		return ( retval );

	case PROG:
		lastalign = -1;
		/* FALLTHRU */
	case ADATA:
	case CDATA:
	case DATA:
	case STRNG:
	case ISTRNG:
	case CSTRNG:
		if (lastloc == l)
			break;
		outfile = textfile;
		print_location(l);
		break;

	case FORCE_LC(CDATA):
		if (lastloc == l)
			break;
		outfile = textfile;
		print_location(CDATA);
		break; 

	case UNK:
		break;

	default:
		cerror(gettxt(":692", "illegal location counter" ));
	}

	lastloc = l;
	return( retval );		/* declare previous loc. ctr. */
}

/* Can object of type t go in a register?  rbusy is array
** of registers in use.  Return -1 if not in register,
** else return register number.  Also fill in rbusy[].
*/
int
cisreg(t, rbusy)
TWORD t;
char rbusy[TOTREGS];
{
	int i;

	if (picflag)
		rbusy[BASEREG] = 1;

	if ( t & ( TSHORT | TUSHORT
		 | TINT   | TUNSIGNED
		 | TLONG  | TULONG
		 | TPOINT | TPOINT2) )
	{
		/* Have a type we can put in register. */
		for (i = USRREGHI-1; i >= NRGS; --i)
			if (!rbusy[i]) break;
	
		/* If i >= NRGS, i is the register number to
		** allocate.
		*/
	
		/* If candidate is suitable number grab it, adjust rbusy[]. */
		if (i >= NRGS) {
			rbusy[i] = 1;
			regvar |= RS_BIT(i);
			return( i );
		}
	}
	else
	if ( t & ( TCHAR | TUCHAR ) )
	{
		if (! rbusy[NRGS] ) {
			rbusy[NRGS] = 1;
			return( NRGS );
		}
	}

	/* Bad type or no register to allocate. */
	return( -1 );
}

NODE *
clocal(p)			/* manipulate the parse tree for local xforms */
register NODE *p;
{
	register NODE *l, *r;

	/* make enum constants into hard ints */

	if (p->in.op == ICON && p->tn.type == ENUMTY) {
	    p->tn.type = INT;
	    return( p );
	}

	if (!asgbinop(p->in.op) && p->in.op != ASSIGN)
		return (p);
	r = p->in.right;
	if (optype(r->in.op) == LTYPE)
		return (p);
	l = r->in.left;
	if (r->in.op == QUEST ||
		(r->in.op == CONV && l->in.op == QUEST) ||
		(r->in.op == CONV && l->in.op == CONV &&
		l->in.left->in.op == QUEST))
				/* distribute assigns over colons */
	{
		register NODE *pwork;
		extern NODE * tcopy();
		NODE *pcpy = tcopy(p), *pnew;
		int astype = p->in.type;	/* remember result type of asgop */
#ifndef NODBG
		extern int xdebug;
		if (xdebug)
		{
			emit_str("Entering [op]=?: distribution\n");
			e2print(p);
		}
#endif
		pnew = pcpy->in.right;
		while (pnew->in.op != QUEST)
			pnew = pnew->in.left;
		/*
		* pnew is top of new tree
		*/

		/* type of resulting ?: will be same as original type of asgop.
		** type of : must be changed, too
		*/
		pnew->in.type = astype;
		pnew->in.right->in.type = astype;

		if ((pwork = p)->in.right->in.op == QUEST)
		{
			tfree(pwork->in.right);
			pwork->in.right = pnew->in.right->in.left;
			pnew->in.right->in.left = pwork;
			/* at this point, 1/2 distributed. Tree looks like:
			*		ASSIGN|ASGOP
			*	LVAL			QUEST
			*		EXPR1		COLON
			*			ASSIGN|ASGOP	EXPR3
			*		LVAL		EXPR2
			* pnew "holds" new tree from QUEST node
			*/
		}
		else
		{
			NODE *pholdtop = pwork;

			pwork = pwork->in.right;
			while (pwork->in.left->in.op != QUEST)
				pwork = pwork->in.left;
			tfree(pwork->in.left);
			pwork->in.left = pnew->in.right->in.left;
			pnew->in.right->in.left = pholdtop;
			/* at this point, 1/2 distributed. Tree looks like:
			*		ASSIGN|ASGOP
			*	LVAL			ANY # OF CONVs
			*			QUEST
			*		EXPR1		COLON
			*			ASSIGN|ASGOP	EXPR3
			*		LVAL		ANY # OF CONVs
			*			EXPR2
			* pnew "holds" new tree from QUEST node
			*/
		}
		if ((pwork = pcpy)->in.right->in.op == QUEST)
		{
			pwork->in.right = pnew->in.right->in.right;
			pnew->in.right->in.right = pwork;
			/*
			* done with the easy case
			*/
		}
		else
		{
			NODE *pholdtop = pwork;

			pwork = pwork->in.right;
			while (pwork->in.left->in.op != QUEST)
				pwork = pwork->in.left;
			pwork->in.left = pnew->in.right->in.right;
			pnew->in.right->in.right = pholdtop;
			/*
			* done with the CONVs case
			*/
		}
		p = pnew;
#ifndef NODBG
		if (xdebug)
		{
			emit_str("Leaving [op]=?: distribution\n");
			e2print(p);
		}
#endif
	}
	return(p);
}

save_return_value()
{
	extern TWORD get_ret_type();
	if (!picflag) return;
	switch(get_ret_type())
	{
	case TVOID: case TFLOAT: case TDOUBLE: case TLDOUBLE:
		return;
	}
	emit_str("	movl	%eax,%ecx\n");
}

void
restore_return_value()
{
	extern TWORD get_ret_type();
	if (!picflag) return;
	switch(get_ret_type())
	{
	case TVOID: case TFLOAT: case TDOUBLE: case TLDOUBLE:
		return;
	}
	emit_str("	movl	%ecx,%eax\n");
}

static int	toplab;
static int	botlab;
static int	piclab;
static int	picpc;		/* stack temp for pc */
static int	efoff[TOTREGS];
void
efcode(p)			/* wind up a function */
NODE *p;
{
	extern int strftn;	/* non-zero if function is structure function,
				** contains label number of local static value
				*/
	register int i;
	int stack;

	if (p->in.name)
		memcpy(request, p->in.name, sizeof(request));
	if (picflag)
                request[BASEREG] = 1;

	stack = -(p->tn.lval) * SZCHAR;

	deflab(retlab);
	if (callflag)
	{
		int temp;

		emit_str("/ASM\n");
		temp = getlab();
		print_location(PROG);
		emit_str("      pushl   %eax\n");
		fprintf(outfile, "      pushl   $.L%d\n", temp);
		deflab(temp);
		if (picflag) {
			fprintf(outfile,"call	 *_epilogue@GOT(%s)\n", rnames[BASEREG]);
			gotflag |= GOTREF;
		}
		else {
			emit_str("  call    _epilogue\n");
		}
		emit_str("      addl    $4, %esp\n");
		emit_str("      popl    %eax\n");
		emit_str("/ASMEND\n");
        }

#ifdef NVLT
	if (nvltflag == 3)
	{
		int temp;

		emit_str("/ASM\n");
		temp = getlab();
		emit_str("\t.text\n");
		emit_str("\tpushl\t%eax\n");		/* return  
value	*/

		fprintf(outfile, "\tpushl\t$0x%x\n",
			nvltMask | NVLTT_Leave);

		deflab(temp);
		if (picflag) {

			fprintf(outfile,"\tcall\t*NVLTleave@GOT(%s)\n",  
rnames[BASEREG]);
			gotflag |= GOTREF;
		}
		else {
			emit_str("\tcall\tNVLTleave\n");
		}
		emit_str("\taddl\t$4, %esp\n");
		emit_str("\tpopl\t%eax\n");
		emit_str("/ASMEND\n");
        }
#endif NVLT

	restore_globals();


	for (i = REG_EDI; i >= REG_EBX; i--)
		if (request[i]) {
			stack += SZINT;
			efoff[i] = -(stack/SZCHAR);
		}

	stack = -(p->tn.lval) * SZCHAR;
	SETOFF(stack, ALSTACK);

        /* Restore old 386 register variables */
        /* Must be reverse order of the register saves */

	/* %ebx not saved if picflag is set but there was not GOT reference */
        if (request[REG_EBX] && !(picflag && gotflag == NOGOTREF))
                emit_str("\tpopl\t%ebx\n");
        if (request[REG_ESI] || (regstused & RS_BIT(REG_ESI)) )
                emit_str("\tpopl\t%esi\n");
        if (request[REG_EDI] || (regstused & RS_BIT(REG_EDI)) )
                emit_str("\tpopl\t%edi\n");

	fprintf(outfile,"\tleave\n\tret/%d\n",biggest_return);

	/*
	 * The entry sequence according to the 387 spec sheet is always
	 * faster (by 4 cycles!) to do a push/movl/sub rather than an
	 * enter and occasionally a sub also.  Fastest enter is 10 cycles
	 * versus 2/2/2...
	 */
	deflab(botlab);
#ifndef	OLDTMPSRET
	if (strftn)
		emit_str("\tpopl\t%eax\n\txchgl\t%eax,0(%esp)\n");
#endif
	emit_str("\tpushl\t%ebp\n\tmovl\t%esp,%ebp\n");
	if (stack/SZCHAR > 0) {
		if (stack / SZCHAR == 4)
			emit_str("\tpushl\t%eax\n");
		else
			fprintf(outfile, "\tsubl\t$%d,%%esp\n", stack/SZCHAR);
	} 

	/* Save old 386 register variables */
	if (request[REG_EDI] || (regstused & RS_BIT(REG_EDI)) )
		emit_str("\tpushl\t%edi\n");
	if (request[REG_ESI] || (regstused & RS_BIT(REG_ESI)) )
		emit_str("\tpushl\t%esi\n");
	/* %ebx not saved if picflag is set but there was not GOT reference */
        if (request[REG_EBX] && !(picflag && gotflag == NOGOTREF))
		emit_str("\tpushl\t%ebx\n");

	regstused = RS_NONE;
	regvar = RS_NONE;
	if (gotflag != NOGOTREF)
	{
	    fprintf(outfile,"\tcall	.L%d\n", piclab);
	    fprintf(outfile,".L%d:\n", piclab);
	    fprintf(outfile,"\tpopl	%s\n", rnames[BASEREG]);
	    if (gotflag & GOTSWREF)
		fprintf(outfile,"\tmovl    %s,%d(%%ebp)\n", rnames[BASEREG],picpc);
	    fprintf(outfile,"\taddl	$_GLOBAL_OFFSET_TABLE_+[.-.L%d],%s\n",
				piclab, rnames[BASEREG]);
	}
	load_globals();
	jmplab(toplab);
}
void
bfcode(p)			/* begin function code. a is array of n stab */
NODE * p;
{
	extern void fp_init();
	retlab = getlab();	/* common return point */
	toplab = getlab();
	botlab = getlab();
	gotflag = NOGOTREF;
	if (picflag) {
		piclab = getlab();
		picpc  = freetemp(1) / SZCHAR;	/* stack temp for pc */
	}
	jmplab(botlab);
	deflab(toplab);
	if (p->in.type == TSTRUCT)
	{
		/*Save place for structure return on stack*/
		strftn = 1;
		fprintf(outfile,"	movl	%s,%d(%%ebp)\n",rnames[AUXREG],str_spot);
	}
	if (proflag)
	{
	        int temp;

		emit_str("/ASM\n");
		print_location(DATA);
		temp = getlab();
		emit_str("	.align	4\n");
		deflab(temp);
		emit_str("	.long	0\n");
		print_location(PROG);
		if (picflag) {
		    fprintf(outfile,"	leal	.L%d@GOTOFF(%s),%%edx\n",temp, rnames[BASEREG]);
		    fprintf(outfile,"	call	*_mcount@GOT(%s)\n", rnames[BASEREG]);
		    gotflag |= GOTREF;
		}
		else {
		    fprintf(outfile,"	movl	$.L%d,%%edx\n",temp);
		    emit_str("	call	_mcount\n");
		}
		emit_str("/ASMEND\n");
	}
        if (callflag)
        {
		int temp;

		emit_str("/ASM\n");
		temp = getlab();
		print_location(PROG);
		fprintf(outfile, "      pushl   $.L%d\n", temp);
		deflab(temp);
		if (picflag) {
			fprintf(outfile,"	call	*_prologue@GOT(%s)\n", rnames[BASEREG]);
			gotflag |= GOTREF;
		}
		else {
			emit_str("  call    _prologue\n");
		}
		emit_str("      addl    $4, %esp\n");
		emit_str("/ASMEND\n");
	}

#ifdef NVLT
        if (nvltflag == 3)
        {
		int temp;
		extern get_nargs();

		emit_str("/ASM\n");
		temp = getlab();
		emit_str("\t.text\n");

		/*
		 *	NVLTenter( (nvltMask | NVLTT_Enter), argno)
		 */
		fprintf(outfile, "\tpushl\t$%d\n", get_nargs());
		fprintf(outfile, "\tpushl\t$0x%x\n",
			nvltMask | NVLTT_Enter);
		deflab(temp);
		if (picflag) {
			fprintf(outfile,"\tcall *NVLTenter@GOT(%s)\n",
				rnames[BASEREG]);
			gotflag |= GOTREF;
		}
		else {
			emit_str("\tcall\tNVLTenter\n");
		}
		emit_str("\taddl\t$8,%esp\n"); /*  pop 2 arguments to NVLTenter	*/
		emit_str("/ASMEND\n");
	}
#endif NVLT

	fp_init();			/* Init the floating point stack */
}

void
begf(p) /*called for BEGF nodes*/
NODE *p;
{
                        /*save the used registers*/
	if (p->in.name)
        	memcpy(request, p->in.name, sizeof(request));
	else
		memset(request, 0, sizeof(request));
	if (picflag)
		request[BASEREG] = 1;

        (void) locctr(PROG);
        strftn = 0;
	biggest_return = 0;
}

void
myret_type(t)
TWORD t;
{
	if (! (t & (TVOID | TFLOAT | TDOUBLE | TLDOUBLE | TFPTR)) )
	{
		if (!biggest_return)
			biggest_return = 1;
	}
}

void		
sw_direct(p, n, range)
register struct sw *p;
int n;
unsigned long range;
{

	register int i;
	register CONSZ j;
	register int dlab, swlab;

	dlab = (p->slab >= 0) ? p->slab : getlab();
	swlab = getlab();
	if (p[1].sval)
		fprintf(outfile,"\tsubl\t$%ld,%%eax\n", p[1].sval);
	fprintf(outfile,"\tcmpl\t$%ld,%%eax\n\tja\t.L%d\n", range, dlab);
	if (picflag) {
	    fprintf(outfile, "\tleal	.L%d@GOTOFF(%s),%%edx\n",swlab, rnames[BASEREG]);
	    fprintf(outfile, "\tmovl	(%%edx,%%eax,4),%%eax\n");
	    fprintf(outfile, "\taddl	%d(%%ebp),%%eax\n", picpc);
	    fprintf(outfile, "\tjmp	*%%eax\n");
	    gotflag |= GOTSWREF;;	
	}
	else
	    fprintf(outfile,"\tjmp\t*.L%d(,%%eax,4)\n", swlab);
	(void) locctr(FORCE_LC(CDATA));
	defalign(ALPOINT);
	emit_str("/SWBEG\n");
	deflab(swlab);
	for (i = 1, j = p[1].sval; i <= n; ++j)
	     if (picflag)
		fprintf(outfile,"	.long	.L%d-.L%d\n",
		    (j == p[i].sval) ? p[i++].slab : dlab, piclab);
	     else
		fprintf(outfile,"	.long	.L%d\n",
		    (j == p[i].sval) ? p[i++].slab : dlab );
	emit_str("/SWEND\n");
	(void) locctr(PROG);
	if (p->slab < 0)
		deflab(dlab);
}

p2done()
{
        char buf[BUFSIZ];
        int m;
	if (tmpfp)
	{
        	fclose(tmpfp);
        	if (!(tmpfp = fopen(tmpfn, "r")))
                	cerror(gettxt(":693","string file disappeared???"));
#ifndef RODATA
		(void) locctr(DATA);
#endif
        	while (m = fread(buf, 1, BUFSIZ, tmpfp))
                	fwrite(buf, 1, m, outfile);
        	(void) unlink(tmpfn);
	}
	return( ferror(outfile) );
}
