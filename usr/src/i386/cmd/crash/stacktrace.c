/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:i386/cmd/crash/stacktrace.c	1.2.1.4"
#ident	"$Header: $"

/*
 * i386 kernel stack trace routines.
 */

/*
 * The following section customizes the stack-trace code for a particular
 * environment (e.g. KDB vs crash).  Only this section should need to change.
 *
 * The following interfaces must be provided (typically as macros):
 *
 * int ST_READ_TEXT(vaddr_t addr, void *buf, size_t n)
 *	Reads n bytes from kernel text address, addr, into buf;
 *	returns -1 if the read failed.
 *
 * int ST_READ_STACK(vaddr_t addr, void *buf, size_t n)
 *	Reads n bytes from kernel stack address, addr, into buf;
 *	returns -1 if the read failed.
 *
 * boolean_t ST_VALID_TEXT_ADDR(vaddr_t addr)
 *	Returns true if addr appears to be a valid kernel text address.
 *
 * vaddr_t ST_SYMVAL(vaddr_t addr)
 *	Returns the address of the kernel symbol closest to but not
 *	greater than addr.
 *
 * void ST_REGISTER_TRAP_FRAME(vaddr_t trap_r0ptr, boolean_t istrap)
 *	Hook to allow the caller to keep track of trap/interrupt frames.
 *	It is called for each trap or interrupt frame with the frame pointer
 *	(as stack address) and a flag, istrap, which is true if it is a
 *	trap frame rather than an interrupt frame.
 *
 * int ST_SHOW_REGSET(int (*prf)())
 *	Hook to allow printing of a register-set number in a trap or
 *	interrupt frame.  If register-set numbers are not used, this can
 *	just print blank spaces.  Whatever is printed, blank or not,
 *	should be exactly 10 characters.  Returns -1 if output was aborted.
 *
 * int ST_PRF_RET_LEN(int (*prf)(), const char *fmt, ulong_t arg)
 *	Prints the fmt and arg as if (*prf)(fmt, arg), but return the
 *	number of characters printed.  Returns -1 if output was aborted.
 *
 * int ST_SHOW_SYM_ADDR(vaddr_t addr, int (*prf)())
 *	Prints the address, addr, in symbolic form, using the print
 *	function, prf.  Returns -1 if output was aborted.
 *
 * TRAP, NMITRAP, SYSTRAP, SIGCLEAN, EVT_PROCESS:
 *	Addresses of the corresponding kernel routines.
 *
 * vaddr_t *structret_funcs;
 * size_t structret_funcs_size;
 *	Array of kernel function addresses for functions which return
 *	structures (this table is obtained from the kernel).
 *	structret_funcs_size is the size in bytes of the structret_funcs
 *	array.
 */

#include <sys/types.h>
#include <sys/seg.h>
#include <sys/tss.h>
#include <sys/reg.h>
#include <sys/debug.h>
#include <sys/inline.h>
#include <sys/lwp.h>
#include <sys/db_as.h>
#include <sys/kdebugger.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/ksynch.h>
#include <sys/vmparam.h>
#include <sys/sysmacros.h>
#include <vm/ublock.h>

#include <a.out.h>

#include <stdio.h>
#include "crash.h"

#define ST_READ_TEXT(addr, buf, n) \
		(readmem(addr,1,0,buf,n,"stack_backtrace"))

#define ST_READ_STACK(addr, buf, n) (read_stack(addr,buf,n))


/*
 * Does this lwp have saved extension stack memory? Initialize for
 * ext_read accordingly.
 */
#define ST_READ_STACK_INIT(lwpp) { \
	ext_stack_end = NULL; \
	if ((lwpp) != NULL && (lwpp)->l_ubinfo.ub_stack_mem != 0) { \
		ext_stack_begin = (vaddr_t)(lwpp)->l_ubinfo.ub_stack; \
		ext_stack_end = (vaddr_t)(lwpp)->l_up & PAGEMASK; \
		ext_stack_diff = (vaddr_t)(lwpp)->l_ubinfo.ub_stack_mem - \
				 ext_stack_begin; \
	} \
}

STATIC vaddr_t ext_stack_begin;
STATIC vaddr_t ext_stack_end;
STATIC size_t ext_stack_diff;
STATIC int ext_read(vaddr_t, void *, uint_t);

#define ST_VALID_TEXT_ADDR(addr) \
		((addr) >=Stext->n_value && (addr) < Etext->n_value )

#define ST_SYMVAL(addr) (findsymval(addr))

#define ST_REGISTER_TRAP_FRAME(trap_r0ptr, istrap) 
#define ST_SHOW_REGSET(prf) (fprintf(fp,"         "))

#define ST_PRF_RET_LEN(prf, fmt, arg)  (db_pr(fmt,arg))

#define ST_SHOW_SYM_ADDR(addr, prf)  (pr_db_sym_off(addr))

extern struct syment *Stext, *Etext;
extern struct syment *Trap, *Nmi, *Systrap, *Sigclean, *Evt_process;

#define TRAP Trap->n_value
#define NMITRAP Nmi->n_value
#define SYSTRAP Systrap->n_value
#define SIGCLEAN Sigclean->n_value
#define EVT_PROCESS Evt_process->n_value
#define BCOPY(from, to, count) memcpy((to), (from), (count))

#ifdef ASSERT
#undef ASSERT 
#endif
#define ASSERT(EX) ((void)((EX) || fprintf(fp,"error: %s %s line %d \n", \
		#EX,__FILE__,__LINE__)))

vaddr_t *structret_funcs;
size_t structret_funcs_size=0;
vaddr_t KVUBLK;

st_init()
{
	struct syment *Sr, *Sf;

	if (!(Sf = symsrch("structret_funcs"))) {
		fprintf(fp,"cant find symbol: structret_funcs\n");
		fprintf(fp,"stacktrace may be inaccurate\n");
	}

	if (!(Sr = symsrch("structret_funcs_size"))) {
		fprintf(fp,"cant find symbol: structret_funcs_size\n");
		fprintf(fp,"stacktrace may be inaccurate\n");

	structret_funcs_size = 28; /*KLUGE  remove  notdef */

	} else {
		fprintf(fp,"FIX st_init\n");
		readmem(Sr->n_value,1,0,
			&structret_funcs_size,sizeof(size_t),"structret_size");
	}

	structret_funcs = (vaddr_t*) malloc(structret_funcs_size);

	readmem(Sf->n_value, 1, 0, structret_funcs, structret_funcs_size,
		"stack_backtrace");
}

db_pr(fmt,arg)
char * fmt;
{
	char buf[80];
	sprintf(buf,fmt,arg);
	fprintf(fp,"%s",buf);
	return(strlen(buf));
}

pr_db_sym_off(addr)
{
	char * p;
	extern char* db_sym_off();

	p = db_sym_off(addr, MMU_PAGESIZE*2);
	fprintf(fp,"%s",p);
	return(strlen(p));
}

boolean_t stack_unknown;

/* convert (and copy from) a kernel stack address to 
 * an address in our local copy of the stack
 */
read_stack(vaddr_t addr, void* buf, size_t n)
{
	extern  char *ublock;
	char * d;

	if (stack_unknown) {
		readmem(addr, 1, -1, buf, n, "stack");
		return 0;
	}

	if (addr < ext_stack_end)
		return(ext_read(addr, buf, n));

	d = ublock + (int)((((char *)addr) - (unsigned int)KVUBLK ));
	memcpy(buf, d, n);
	return 0;
}

/*
 * STATIC int
 * ext_read(vaddr_t addr, void *buf, uint_t n)
 *	Read some stack data via db_read(), but when appropriate, get
 *	the contents from the extension stack fragment in ub_stack_mem.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC int
ext_read(vaddr_t addr, void *buf, uint_t n)
{
	vaddr_t ext_addr;
	size_t size;

	if (addr < ext_stack_begin) {
		size = MIN(n, ext_stack_begin - addr);
		readmem(addr, 1, 0, buf, size, "stack extension ovflow");
		n -= size;
		if (n == 0)
			return;
		addr += size;
		buf = (caddr_t)buf + size;
	}

	if (addr <= ext_stack_end) {
		ext_addr = addr;
		ext_addr += ext_stack_diff;
		size = MIN(n, ext_stack_end - addr);
		readmem(ext_addr, 1, 0, buf, size, "stack extension");
		n -= size;
		if (n == 0)
			return;
		addr += size;
		buf = (caddr_t)buf + size;
	}

	read_stack(addr, buf, n);
}

/*
 * The rest of the file should be independent of the environment.
 */

#ifndef OPC_CALL_REL
#define OPC_CALL_REL	0xE8
#endif
#ifndef OPC_CALL_DIR
#define OPC_CALL_DIR	0x9A
#endif
#ifndef OPC_CALL_IND
#define OPC_CALL_IND	0xFF
#endif

#define FRAMEREG(r0ptr, reg, valp) \
		ST_READ_STACK((r0ptr) + (reg) * sizeof(int), valp, sizeof(int))

#define IS_TRAP_ROUTINE(eip) \
		((eip) == TRAP || \
		 (eip) == NMITRAP || \
		 (eip) == SYSTRAP || \
		 (eip) == SIGCLEAN || \
		 (eip) == EVT_PROCESS)

static boolean_t st_is_after_call(vaddr_t, vaddr_t *);
static boolean_t st_is_ktrap(vaddr_t);
static int nframe(vaddr_t, char, vaddr_t, vaddr_t, uint_t, int (*)());
static int tframe(vaddr_t, vaddr_t, int (*)(), boolean_t);
static int iframe(vaddr_t, vaddr_t, int (*)());

int st_frameregs(vaddr_t r0ptr, int (*prf)(), boolean_t full_frame);

uint_t st_max_args = 3;

enum ret_type { RT_RET, RT_IRET };	/* type of return instruction */
static enum ret_type ret_type;

/*
 * find_return() "disassembles" instructions using a finite state machine
 * controlled by the states[] table.  The format of this table is
 * defined by the following structure.  Note that, since find_return()
 * only cares about instructions' effects on esp, ebp, and eip, it doesn't
 * need to distinguish all of the actual machine instructions; in effect,
 * it interprets a pseudo-machine with many types of actual machine
 * instructions grouped into single "instructions" of the pseudo-machine
 * (these are referred to as "actions" below).
 */
struct state {
	uchar_t st_opc;
	uchar_t st_mask;
	uchar_t st_action;
	uchar_t st_val;
};

/* st_action is split into a skip count and an action w/these masks: */
#define SKIPMASK	0x0F
#define ACTMASK		0xF0

#define SK_W	0x0A	/* special skip value: opsize */

#define STDVAL		0x80	/* action code bit: use "standard" values */

/* action codes */
#define A_TERM		0x00
#define A_CONT		0x10
#define A_GOTO		0x20
#define A_RET		0x30
#define A_EXT		0x70
#define A_SETESP	(STDVAL|0x00)
#define A_SETEBP	(STDVAL|0x10)
#define A_JMP		(STDVAL|0x20)
#define A_JC		(STDVAL|0x30)
#define A_CALL		(STDVAL|0x40)

/* extended actions; stored in value field when main action is A_EXT */
#define A_ILL		0x01
#define A_OPSIZE	0x02
#define A_PUSHEBP	0x03
#define A_POPEBP	0x04
#define A_PUSHA		0x05
#define A_POPA		0x06
#define A_ENTER		0x07
#define A_LEAVE		0x08

/* "standard" value base codes */
#define VALBASEMASK	0xC0
#define B_ZERO		0x00
#define B_ESP		0x40
#define B_EBP		0x80
#define B_EIP		0xC0

/* "standard" value modifier codes */
#define M_ADD		0x00
#define M_SUB		0x20

#define M_CONST		0x00
#ifdef M_READ
#undef M_READ
#endif
#define M_READ		0x10

#define MODVALMASK	0x0F
#define MODSIZEMASK	0x03
#define M_0		0x00
#define M_1		0x01
#define M_2		0x02
#define M_4		0x04
#define M_8		0x08
#define M_1S		0x09
#define M_W		0x03	/* 2 or 4, depending on opsize */
#define M_U2		0x0E	/* 0 if opsize==4, else UNKNOWN */
#define M_UNKNOWN	0x0F

STATIC struct state states[] = {
	{ 0x0F, 0x00, A_CONT, 0x0F },			/* 0x0F prefix */
	{ 0x06, 0x18, A_SETESP, B_ESP+M_SUB+M_W },	/* push es/cs/ss/ds */
	{ 0x07, 0x18, A_SETESP, B_ESP+M_W },		/* pop es/ss/ds */
	{ 0x01, 0x38, A_CONT, 0x11 },			/* r/m E unknown */
	{ 0x03, 0x38, A_CONT, 0x13 },			/* r/m G unknown */
	{ 0x44, 0x00, A_SETESP, B_ESP+M_1 },		/* inc esp */
	{ 0x45, 0x00, A_SETEBP, B_EBP+M_1 },		/* inc ebp */
	{ 0x4C, 0x00, A_SETESP, B_ESP+M_SUB+M_1 },	/* dec esp */
	{ 0x4D, 0x00, A_SETEBP, B_EBP+M_SUB+M_1 },	/* dec ebp */
	{ 0x55, 0x00, A_EXT, A_PUSHEBP },		/* push ebp */
	{ 0x50, 0x07, A_SETESP, B_ESP+M_SUB+M_W },	/* push reg */
	{ 0x5C, 0x00, A_SETESP, M_UNKNOWN },		/* pop esp */
	{ 0x5D, 0x00, A_EXT, A_POPEBP },		/* pop ebp */
	{ 0x58, 0x07, A_SETESP, B_ESP+M_W },		/* pop reg */
	{ 0x60, 0x00, A_EXT, A_PUSHA },			/* pusha */
	{ 0x61, 0x00, A_EXT, A_POPA },			/* popa */
	{ 0x63, 0x00, A_CONT, 0x11 },			/* r/m E unk (arpl) */
	{ 0x66, 0x00, A_EXT, A_OPSIZE },		/* data16 */
	{ 0x67, 0x00, A_EXT, A_ILL },			/* addr16 */
	{ 0x68, 0x00, A_SETESP+SK_W, B_ESP+M_SUB+M_W },	/* push Iv */
	{ 0x69, 0x00, A_CONT, 0x69 },			/* r/m + Iv to ignore */
	{ 0x6A, 0x00, A_SETESP+1, B_ESP+M_SUB+M_W },	/* push Ib */
	{ 0x6B, 0x00, A_CONT, 0xC0 },			/* r/m + Ib to ignore */
	{ 0x70, 0x0F, A_JC, B_EIP+M_READ+M_1S },	/* jcc Jb */
	{ 0x81, 0x00, A_CONT, 0x81 },			/* Grp1 E,Iv */
	{ 0x82, 0x00, A_TERM+1, 0 },			/* mov al,Ib */
	{ 0x83, 0x00, A_CONT, 0x83 },			/* Grp1 E,Ib */
	{ 0x87, 0x00, A_CONT, 0x11 },			/* r/m E unk (xchg) */
	{ 0x89, 0x00, A_CONT, 0x89 },			/* mov Ev,G */
	{ 0x8B, 0x00, A_CONT, 0x8B },			/* mov Gv,E */
	{ 0x8C, 0x00, A_CONT, 0x11 },			/* r/m E unknown */
	{ 0x8D, 0x00, A_CONT, 0x8D },			/* lea Gv,M */
	{ 0x8F, 0x00, A_CONT, 0x8F },			/* pop Ev */
	{ 0x94, 0x00, A_SETESP, M_UNKNOWN },		/* xchg eax,esp */
	{ 0x95, 0x00, A_SETEBP, M_UNKNOWN },		/* xchg eax,ebp */
	{ 0x9A, 0x00, A_EXT, A_ILL },			/* call far */
	{ 0x9C, 0x00, A_SETESP, B_ESP+M_SUB+M_W },	/* pushf */
	{ 0x9D, 0x00, A_SETESP, B_ESP+M_W },		/* popf */
	{ 0xBC, 0x00, A_SETESP, B_ZERO+M_READ+M_W },	/* mov esp,Iv */
	{ 0xBD, 0x00, A_SETEBP, B_ZERO+M_READ+M_W },	/* mov ebp,Iv */
	{ 0xC0, 0x08, A_CONT, 0xC0 },			/* r/m + Ib to ignore */
	{ 0xC1, 0x00, A_CONT, 0xC1 },			/* r/m + Ib E unk */
	{ 0xC2, 0x01, A_RET, RT_RET },			/* ret near */
	{ 0xC4, 0x01, A_CONT, 0x11 },			/* r/m E unk (l[de]s) */
	{ 0xC7, 0x00, A_CONT, 0xC7 },			/* mov Ev,Iv */
	{ 0xC8, 0x00, A_EXT, A_ENTER },			/* enter Iw,Ib */
	{ 0xC9, 0x00, A_EXT, A_LEAVE },			/* leave */
	{ 0xCA, 0x01, A_EXT, A_ILL },			/* ret far */
	{ 0xCF, 0x00, A_RET, RT_IRET },			/* iret */
	{ 0xD1, 0x02, A_CONT, 0x11 },			/* r/m E unknown */
	{ 0xD6, 0x00, A_EXT, A_ILL },
	{ 0xD8, 0x07, A_CONT, 0x80 },			/* r/m to ign (esc) */
	{ 0xE0, 0x03, A_JC, B_EIP+M_READ+M_1S },	/* loop[n][e]/jcxz Jb */
	{ 0xE8, 0x00, A_CALL, B_EIP+M_READ+M_W },	/* call Jv */
	{ 0xE9, 0x00, A_JMP, B_EIP+M_READ+M_W },	/* jmp Jv */
	{ 0xEA, 0x00, A_EXT, A_ILL },			/* jmp far */
	{ 0xEB, 0x00, A_JMP, B_EIP+M_READ+M_1S },	/* jmp Jb */
	{ 0xF1, 0x00, A_EXT, A_ILL },
	{ 0xF6, 0x00, A_CONT, 0xF6 },			/* Grp3 Eb */
	{ 0xF7, 0x00, A_CONT, 0xF7 },			/* Grp3 Ev */
	{ 0xFE, 0x00, A_CONT, 0xFE },			/* Grp4 */
	{ 0xFF, 0x00, A_CONT, 0xFF },			/* Grp5 */
	{ 0xB0, 0x07, A_TERM+1, 0 },
	{ 0xB8, 0x07, A_TERM+SK_W, 0 },
	{ 0x40, 0x0F, A_TERM, 0 },
	{ 0x90, 0x0F, A_TERM, 0 },
	{ 0xF0, 0x0F, A_TERM, 0 },
	{ 0xE4, 0x03, A_TERM+1, 0 },
	{ 0xA8, 0x00, A_TERM+1, 0 },
	{ 0xA0, 0x03, A_TERM+SK_W, 0 },
	{ 0xA9, 0x00, A_TERM+SK_W, 0 },
	{ 0x05, 0x38, A_TERM+SK_W, 0 },
	{ 0x04, 0x38, A_TERM+1, 0 },
	{ 0xCD, 0x00, A_TERM+1, 0 },
	{ 0xC6, 0x20, A_CONT, 0xC0 },			/* r/m + Ib to ign */
	{ 0x80, 0x00, A_CONT, 0xC0 },			/* r/m + Ib to ign */
	{ 0x80, 0x0F, A_CONT, 0x80 },			/* r/m to ignore */
	{ 0x04, 0xFB, A_TERM, 0 },
	{ 0x0F, 0xFF, A_CONT, 0x80 },			/* r/m to ignore */
	/* 0x0F: 0x0F prefix opcodes */
	{ 0x00, 0x00, A_CONT, 0x06 },			/* Grp6 */
	{ 0x01, 0x00, A_CONT, 0x07 },			/* Grp7 */
	{ 0x02, 0x01, A_CONT, 0x13 },			/* r/m G unknown */
	{ 0x08, 0x00, A_TERM, 0 },			/* invd */
	{ 0x06, 0x00, A_TERM, 0 },
	{ 0x90, 0x0F, A_TERM, 0 },
	{ 0x25, 0x02, A_EXT, A_ILL },
	{ 0x20, 0x07, A_TERM+1, 0 },			/* mov to/from spec */
	{ 0x80, 0x0F, A_JC, B_EIP+M_READ+M_W },		/* jcc Jv */
	{ 0xA0, 0x08, A_SETESP, B_ESP+M_SUB+M_W },	/* push fs/gs */
	{ 0xA1, 0x08, A_SETESP, B_ESP+M_W },		/* pop fs/gs */
	{ 0xA4, 0x08, A_CONT, 0xC1 },			/* r/m + Ib E unk */
	{ 0xB4, 0x01, A_CONT, 0x11 },			/* r/m E unk (l[fg]s) */
	{ 0xB2, 0x00, A_CONT, 0x11 },			/* r/m E unk (lss) */
	{ 0xBA, 0x00, A_CONT, 0x08 },			/* Grp8 E,Ib */
	{ 0xA2, 0x08, A_EXT, A_ILL },
	{ 0xA3, 0x00, A_CONT, 0x80 },			/* r/m to ignore */
	{ 0xA3, 0x18, A_CONT, 0x11 },			/* r/m E unknown */
	{ 0xA5, 0x08, A_CONT, 0x11 },			/* r/m E unknown */
	{ 0xAF, 0x00, A_CONT, 0x13 },			/* r/m G unknown */
	{ 0xB4, 0x0B, A_CONT, 0x13 },			/* r/m G unknown */
	{ 0x81, 0xFF, A_EXT, A_ILL },
	/* 0x81: Grp1 E,Iv */
	{ 0xC4, 0x00, A_SETESP, B_ESP+M_READ+M_W },	/* add esp,Iv */
	{ 0xC5, 0x00, A_SETEBP, B_EBP+M_READ+M_W },	/* add ebp,Iv */
	{ 0xEC, 0x00, A_SETESP, B_ESP+M_SUB+M_READ+M_W }, /* sub esp,Iv */
	{ 0xED, 0x00, A_SETEBP, B_EBP+M_SUB+M_READ+M_W }, /* sub ebp,Iv */
	{ 0xFC, 0x02, A_TERM+SK_W, 0 },			/* cmp esp/ebp,Iv */
	{ 0x83, 0xFF, A_GOTO, 0xA3 },			/* r/m + Iv E unk */
	/* 0x83: Grp1 E,Ib */
	{ 0xC4, 0x00, A_SETESP, B_ESP+M_READ+M_1S },	/* add esp,Ib */
	{ 0xC5, 0x00, A_SETEBP, B_EBP+M_READ+M_1S },	/* add ebp,Ib */
	{ 0xEC, 0x00, A_SETESP, B_ESP+M_SUB+M_READ+M_1S }, /* sub esp,Ib */
	{ 0xED, 0x00, A_SETEBP, B_EBP+M_SUB+M_READ+M_1S }, /* sub ebp,Ib */
	{ 0xFC, 0x02, A_TERM+1, 0 },			/* cmp esp/ebp,Ib */
	{ 0xF7, 0xFF, A_GOTO, 0xC1 },			/* r/m + Ib E unk */
	/* 0xF7: Grp3 Ev */
	{ 0x08, 0xC7, A_EXT, A_ILL },
	{ 0x00, 0xC7, A_GOTO, 0x69 },			/* r/m + Iv to ign */
	{ 0xD4, 0x08, A_SETESP, M_UNKNOWN },		/* not/neg esp */
	{ 0xD5, 0x08, A_SETEBP, M_UNKNOWN },		/* not/neg ebp */
	{ 0xF6, 0xFF, A_GOTO, 0x80 },			/* r/m to ignore */
	/* 0xF6: Grp3 Eb */
	{ 0x08, 0xC7, A_EXT, A_ILL },
	{ 0xFE, 0xFF, A_GOTO, 0xC0 },			/* r/m + Ib to ign */
	/* 0xFE: Grp4 */
	{ 0x00, 0xCF, A_GOTO, 0x80 },			/* r/m to ignore */
	{ 0xFF, 0xFF, A_EXT, A_ILL },
	/* 0xFF: Grp5 */
	{ 0xC4, 0x00, A_SETESP, B_ESP+M_1 },		/* inc esp */
	{ 0xC5, 0x00, A_SETEBP, B_EBP+M_1 },		/* inc ebp */
	{ 0xCC, 0x00, A_SETESP, B_ESP+M_SUB+M_1 },	/* dec esp */
	{ 0xCD, 0x00, A_SETEBP, B_EBP+M_SUB+M_1 },	/* dec ebp */
	{ 0xF5, 0x00, A_EXT, A_PUSHEBP },		/* push ebp */
	{ 0xF0, 0x07, A_SETESP, B_ESP+M_SUB+M_W },	/* push reg */
	{ 0x34, 0x00, A_CONT, 0x34 },			/* push sib00 */
	{ 0x35, 0x00, A_SETESP+4, B_ESP+M_SUB+M_W },
	{ 0x74, 0x07, A_SETESP+2, B_ESP+M_SUB+M_W },
	{ 0x70, 0x07, A_SETESP+1, B_ESP+M_SUB+M_W },
	{ 0xB4, 0x07, A_SETESP+5, B_ESP+M_SUB+M_W },
	{ 0xB0, 0x07, A_SETESP+4, B_ESP+M_SUB+M_W },
	{ 0x30, 0xC7, A_SETESP, B_ESP+M_SUB+M_W },
	{ 0x20, 0xC7, A_EXT, A_ILL },
	{ 0x00, 0xF7, A_GOTO, 0x80 },			/* r/m to ignore */
	{ 0x08, 0xC7, A_GOTO, 0x80 },			/* r/m to ignore */
	{ 0x34, 0xFF, A_EXT, A_ILL },
	/* 0x34: push sib00 */
	{ 0x05, 0xF8, A_SETESP+4, B_ESP+M_SUB+M_W },
	{ 0x06, 0xFF, A_SETESP, B_ESP+M_SUB+M_W },
	/* 0x06: Grp6 */
	{ 0x30, 0xCF, A_EXT, A_ILL },
	{ 0xC4, 0x08, A_SETESP, M_UNKNOWN },		/* sldt/str esp */
	{ 0xC5, 0x08, A_SETEBP, M_UNKNOWN },		/* sldt/str ebp */
	{ 0x07, 0xFF, A_GOTO, 0x80 },			/* r/m to ignore */
	/* 0x07: Grp7 */
	{ 0x28, 0xD7, A_EXT, A_ILL },
	{ 0xE4, 0x00, A_SETESP, M_UNKNOWN },		/* smsw esp */
	{ 0xE5, 0x00, A_SETEBP, M_UNKNOWN },		/* smsw ebp */
	{ 0x08, 0xFF, A_GOTO, 0x80 },			/* r/m to ignore */
	/* 0x08: Grp8 E,Ib */
	{ 0x00, 0xDF, A_EXT, A_ILL },
	{ 0x89, 0xFF, A_GOTO, 0xC1 },			/* r/m + Ib E unk */
	/* 0x89: mov Ev,G */
	{ 0xE4, 0x00, A_TERM, 0 },			/* mov esp,esp */
	{ 0xEC, 0x00, A_SETESP, B_EBP+M_U2 },		/* mov esp,ebp */
	{ 0xC4, 0x38, A_SETESP, M_UNKNOWN },		/* mov esp,reg */
	{ 0xED, 0x00, A_TERM, 0 },			/* mov ebp,ebp */
	{ 0xE5, 0x00, A_SETEBP, B_ESP+M_U2 },		/* mov ebp,esp */
	{ 0xC5, 0x38, A_SETEBP, M_UNKNOWN },		/* mov ebp,reg */
	{ 0x8B, 0xFF, A_GOTO, 0x80 },			/* r/m to ignore */
	/* 0x8B: mov Gv,E */
	{ 0xE4, 0x00, A_TERM, 0 },			/* mov esp,esp */
	{ 0xE5, 0x00, A_SETESP, B_EBP+M_U2 },           /* mov esp,ebp */
	{ 0xED, 0x00, A_TERM, 0 },			/* mov ebp,ebp */
	{ 0xEC, 0x00, A_SETEBP, B_ESP+M_U2 },           /* mov ebp,esp */
	{ 0x8D, 0xFF, A_GOTO, 0x11 },			/* r/m E unknown */
	/* 0x8D: lea Gv,M (assume opsize 4) */
	{ 0x64, 0x00, A_CONT, 0x64 },			/* lea esp,sib(d8) */
	{ 0x6C, 0x00, A_CONT, 0x6C },			/* lea ebp,sib(d8) */
	{ 0x6D, 0x00, A_SETEBP, B_EBP+M_READ+M_1S },	/* lea ebp,d8(ebp) */
	{ 0xA4, 0x00, A_CONT, 0xA4 },			/* lea esp,sib(d32) */
	{ 0xAC, 0x00, A_CONT, 0xAC },			/* lea ebp,sib(d32) */
	{ 0xAD, 0x00, A_SETEBP, B_EBP+M_READ+M_4 },	/* lea ebp,d32(ebp) */
	{ 0x64, 0xFF, A_GOTO, 0x80 },			/* r/m to ignore */
	/* 0x64: lea esp,sib(d8) */
	{ 0x24, 0x00, A_SETESP, B_ESP+M_READ+M_1S },	/* lea esp,d8(esp) */
	{ 0x6C, 0xFF, A_TERM+1, 0 },
	/* 0x6C: lea ebp,sib(d8) */
	{ 0x2C, 0x00, A_SETEBP, B_EBP+M_READ+M_1S },	/* lea ebp,d8(ebp) */
	{ 0xA4, 0xFF, A_TERM+1, 0 },
	/* 0xA4: lea esp,sib(d32) */
	{ 0x24, 0x00, A_SETESP, B_ESP+M_READ+M_4 },	/* lea esp,d32(esp) */
	{ 0xAC, 0xFF, A_TERM+4, 0 },
	/* 0xAC: lea ebp,sib(d32) */
	{ 0x2C, 0x00, A_SETEBP, B_EBP+M_READ+M_4 },	/* lea ebp,d32(ebp) */
	{ 0x8F, 0xFF, A_TERM+4, 0 },
	/* 0x8F: pop Ev */
	{ 0xC5, 0x00, A_EXT, A_POPEBP },		/* pop ebp */
	{ 0xC0, 0x07, A_SETESP, B_ESP+M_W },		/* pop reg */
	{ 0x04, 0x00, A_CONT, 0x90 },			/* pop sib00 */
	{ 0x05, 0x00, A_SETESP+4, B_ESP+M_W },
	{ 0x44, 0x07, A_SETESP+2, B_ESP+M_W },
	{ 0x40, 0x07, A_SETESP+1, B_ESP+M_W },
	{ 0x84, 0x07, A_SETESP+5, B_ESP+M_W },
	{ 0x80, 0x07, A_SETESP+4, B_ESP+M_W },
	{ 0x90, 0xFF, A_SETESP, B_ESP+M_W },
	/* 0x90: pop sib00 */
	{ 0x05, 0xF8, A_SETESP+4, B_ESP+M_W },
	{ 0xC7, 0xFF, A_SETESP, B_ESP+M_W },
	/* 0xC7: mov Ev,Iv */
	{ 0xC4, 0x00, A_SETESP, M_READ+M_W+M_U2 },	/* mov esp,Iv */
	{ 0xC5, 0x00, A_SETEBP, M_READ+M_W+M_U2 },	/* mov ebp,Iv */
	{ 0x11, 0xFF, A_GOTO, 0x69 },			/* r/m + Iv to ign */
	/* 0x11: r/m E unknown */
	{ 0xC4, 0x38, A_SETESP, M_UNKNOWN },
	{ 0xC5, 0x38, A_SETEBP, M_UNKNOWN },
	{ 0x13, 0xFF, A_GOTO, 0x80 },			/* r/m to ignore */
	/* 0x13: r/m G unknown */
	{ 0x00, 0xDF, A_GOTO, 0x80 },			/* r/m to ignore */
	{ 0x10, 0xEF, A_GOTO, 0x80 },			/* r/m to ignore */
	{ 0xE0, 0x07, A_SETESP, M_UNKNOWN },
	{ 0xE8, 0x07, A_SETEBP, M_UNKNOWN },
	{ 0x24, 0x00, A_CONT, 0x14 },			/* sib00 esp unknown */
	{ 0x2C, 0x00, A_CONT, 0x1C },			/* sib00 ebp unknown */
	{ 0x25, 0x00, A_SETESP+4, M_UNKNOWN },
	{ 0x2D, 0x00, A_SETEBP+4, M_UNKNOWN },
	{ 0x64, 0x00, A_SETESP+2, M_UNKNOWN },
	{ 0x6C, 0x00, A_SETEBP+2, M_UNKNOWN },
	{ 0x60, 0x07, A_SETESP+1, M_UNKNOWN },
	{ 0x68, 0x07, A_SETEBP+1, M_UNKNOWN },
	{ 0xA4, 0x00, A_SETESP+5, M_UNKNOWN },
	{ 0xAC, 0x00, A_SETEBP+5, M_UNKNOWN },
	{ 0xA0, 0x07, A_SETESP+4, M_UNKNOWN },
	{ 0xA8, 0x07, A_SETEBP+4, M_UNKNOWN },
	{ 0x00, 0xF7, A_SETESP, M_UNKNOWN },
	{ 0x14, 0xFF, A_SETEBP, M_UNKNOWN },
	/* 0x14: sib00 esp unknown */
	{ 0x05, 0xF8, A_SETESP+4, M_UNKNOWN },
	{ 0x1C, 0xFF, A_SETESP, M_UNKNOWN },
	/* 0x1C: sib00 ebp unknown */
	{ 0x05, 0xF8, A_SETEBP+4, M_UNKNOWN },
	{ 0xA3, 0xFF, A_SETEBP, M_UNKNOWN },
	/* 0xA3: r/m + Iv E unknown */
	{ 0xC4, 0x38, A_SETESP+SK_W, M_UNKNOWN },
	{ 0xC5, 0x38, A_SETEBP+SK_W, M_UNKNOWN },
	{ 0xC1, 0xFF, A_GOTO, 0x69 },			/* r/m + Iv to ign */
	/* 0xC1: r/m + Ib E unknown */
	{ 0xC4, 0x38, A_SETESP+1, M_UNKNOWN },
	{ 0xC5, 0x38, A_SETEBP+1, M_UNKNOWN },
	{ 0x80, 0xFF, A_GOTO, 0xC0 },			/* r/m + Ib to ign */
	/* 0x80: r/m to ignore */
	{ 0xC0, 0x3F, A_TERM, 0 },
	{ 0x04, 0x38, A_CONT, 0x84 },			/* sib00 to ignore */
	{ 0x05, 0x38, A_TERM+4, 0 },
	{ 0x44, 0x38, A_TERM+2, 0 },
	{ 0x40, 0x3F, A_TERM+1, 0 },
	{ 0x84, 0x38, A_TERM+5, 0 },
	{ 0x80, 0x3F, A_TERM+4, 0 },
	{ 0x84, 0xFF, A_TERM, 0 },
	/* 0x84: sib00 to ignore */
	{ 0x05, 0xF8, A_TERM+4, 0 },
	{ 0x69, 0xFF, A_TERM, 0 },
	/* 0x69: r/m + Iv to ignore */
	{ 0xC0, 0x3F, A_TERM+SK_W, 0 },
	{ 0x04, 0x38, A_CONT, 0x6D },			/* sib00 + Iv to ign */
	{ 0x05, 0x38, A_TERM+SK_W+4, 0 },
	{ 0x44, 0x38, A_TERM+SK_W+2, 0 },
	{ 0x40, 0x3F, A_TERM+SK_W+1, 0 },
	{ 0x84, 0x38, A_TERM+SK_W+5, 0 },
	{ 0x80, 0x3F, A_TERM+SK_W+4, 0 },
	{ 0x6D, 0xFF, A_TERM+SK_W, 0 },
	/* 0x6D: sib00 + Iv to ignore */
	{ 0x05, 0xF8, A_TERM+SK_W+4, 0 },
	{ 0xC0, 0xFF, A_TERM+SK_W, 0 },
	/* 0xC0: r/m + Ib to ignore */
	{ 0xC0, 0x3F, A_TERM+1, 0 },
	{ 0x04, 0x38, A_CONT, 0xC4 },			/* sib00 + Ib to ign */
	{ 0x05, 0x38, A_TERM+5, 0 },
	{ 0x44, 0x38, A_TERM+3, 0 },
	{ 0x40, 0x3F, A_TERM+2, 0 },
	{ 0x84, 0x38, A_TERM+6, 0 },
	{ 0x80, 0x3F, A_TERM+5, 0 },
	{ 0xC4, 0xFF, A_TERM+1, 0 },
	/* 0xC4: sib00 + Ib to ign */
	{ 0x05, 0xF8, A_TERM+5, 0 },
	{ 0x00, 0xFF, A_TERM+1, 0 }
};

#define MAXINST		3000
#define MAXBRANCH	64

#ifdef DEBUG
boolean_t find_return_verbose = B_FALSE;
#endif

/*
 * static boolean_t
 * find_return(ulong_t start_eip, ulong_t *espp, ulong_t *ebpp, int (*prf)(),
 *	       int *sp_delta_p)
 *	Find the esp value for the next return instruction (ret or iret).
 *
 * Calling/Exit State:
 *	Given initial values of eip, esp, and ebp in start_eip, *espp, and
 *	*ebpp, respectively, find_return emulates the machine until it
 *	finds a ret or iret instruction.  If it does, *espp and *ebpp are
 *	advanced, the global variable ret_type is set to the type of return
 *	instruction, and B_TRUE is returned.  If it fails for some reason,
 *	B_FALSE is returned.  *prf is a printf-like function used to print
 *	any (error) messages; if it is NULL, nothing will be printed.
 *	If sp_delta_p is non-NULL, emulation will terminate at the first
 *	adjustment of esp, instead of at a return instruction, and *esp
 *	will be set to the amount of this adjustment (useful for estimating
 *	the number of arguments to a routine).
 */
static boolean_t
find_return(ulong_t start_eip, ulong_t *espp, ulong_t *ebpp, int (*prf)(),
	    int *sp_delta_p)
{
	vaddr_t addr;
	ulong_t eip;
	ulong_t esp, ebp, newesp;
	ulong_t unk_addr;
	ulong_t pushed_ebp, ebp_addr;
	ulong_t branch_addr[MAXBRANCH];
	uint_t branch_tried[MAXBRANCH];
	uint_t ninst, nbranch;
	uint_t br, last_try, last_new_branch, start_time;
	boolean_t looped;
	struct state *stp;
	union op {
		signed char sb;
		uchar_t b;
		ushort_t w;
		ulong_t l;
	} operand;
	uchar_t opc, action;
	ulong_t val;
	int mod, sz;
	int opsize;
	vaddr_t *srfp;

	ninst = nbranch = 0;
	unk_addr = 0;
	looped = B_FALSE;
restart:
	addr = eip = start_eip;
	esp = *espp;
	ebp = *ebpp;
	ebp_addr = UNKNOWN;
	start_time = last_try = last_new_branch = ninst;
	opsize = 4;

	/* Disassemble instructions until we find a return instruction. */
	for (stp = &states[0];;) {
		if (++ninst > MAXINST) {
			if (prf)
				(*prf) ("<<giving up at %x after disassembling"
					" %d instructions>>\n", addr,
					MAXINST);
			return B_FALSE;
		}
		if (ST_READ_TEXT(addr, &opc, 1) == -1) {
read_failed:
			if (prf &&
			    (*prf) ("<<read failed at %x>>\n", addr) == -1)
				return B_FALSE;
			action = 0;
			goto ill;
		}
		while (stp->st_mask != 0xFF &&
		       (opc & ~stp->st_mask) != stp->st_opc) {
			++stp;
		}
		newesp = esp;
		addr += 1 + (val = (stp->st_action & SKIPMASK));
		if (val >= SK_W)
			addr += opsize - SK_W;
		action = (stp->st_action & ACTMASK);
		if (action & STDVAL) {
			switch (stp->st_val & VALBASEMASK) {
			case B_ZERO:
			case B_EIP:
				val = 0;  break;
			case B_ESP:
				val = esp;  break;
			case B_EBP:
				val = ebp;  break;
			}
			if (val != UNKNOWN) {
				if (stp->st_val & M_READ) {
					sz = (stp->st_val & MODSIZEMASK);
					if (sz == M_W)
						sz = opsize;
					if (ST_READ_TEXT(addr, &operand,
							 sz) == -1)
						goto read_failed;
					addr += sz;
					switch (stp->st_val & MODVALMASK) {
					case M_4:
						mod = operand.l;  break;
					case M_2:
						mod = operand.w;  break;
					case M_1:
						mod = operand.b;  break;
					case M_1S:
						sz = 4;
						mod = operand.sb;  break;
					case M_W:
						if (sz == 2)
							mod = operand.w;
						else
							mod = operand.l;
						break;
					}
				} else {
					mod = (stp->st_val & MODVALMASK);
					switch (mod) {
					case M_U2:
						if (opsize == 4) {
							mod = 0;
							break;
						}
						/* FALLTHROUGH */
					case M_UNKNOWN:
						mod = UNKNOWN;  break;
					case M_W:
						mod = opsize;  break;
					}
					sz = 4;
				}
				if (stp->st_val & M_SUB)
					mod = -mod;
				switch (sz) {
				case 4:
					val += mod;  break;
				case 2:
					((union op *)&val)->w += mod;  break;
				case 1:
					((union op *)&val)->b += mod;  break;
				}
			}
			if ((stp->st_val & VALBASEMASK) == B_EIP)
				val += addr;
		} else if (action == A_EXT)
			action = stp->st_val;
		else
			val = stp->st_val;

#ifdef DEBUG
		if (find_return_verbose && prf && action != A_GOTO &&
		    action != A_CONT && action != A_OPSIZE && action != A_ILL) {
			if ((*prf) ("0x%08x:  ", eip) == -1)
				return B_FALSE;
			switch (action) {
			case A_TERM:
				if ((*prf) ("-      ") == -1)
					return B_FALSE;
				break;
			case A_RET:
				if ((*prf) ("RET    ") == -1)
					return B_FALSE;
				break;
			case A_SETESP:
				if ((*prf) ("SETESP ") == -1)
					return B_FALSE;
				break;
			case A_SETEBP:
				if ((*prf) ("SETEBP ") == -1)
					return B_FALSE;
				break;
			case A_PUSHEBP:
				if ((*prf) ("PUSHEBP") == -1)
					return B_FALSE;
				break;
			case A_POPEBP:
				if ((*prf) ("POPEBP ") == -1)
					return B_FALSE;
				break;
			case A_PUSHA:
				if ((*prf) ("PUSHA  ") == -1)
					return B_FALSE;
				break;
			case A_POPA:
				if ((*prf) ("POPA   ") == -1)
					return B_FALSE;
				break;
			case A_ENTER:
				if ((*prf) ("ENTER  ") == -1)
					return B_FALSE;
				break;
			case A_LEAVE:
				if ((*prf) ("LEAVE  ") == -1)
					return B_FALSE;
				break;
			case A_JMP:
				if ((*prf) ("JMP    ") == -1)
					return B_FALSE;
				break;
			case A_JC:
				if ((*prf) ("JC     ") == -1)
					return B_FALSE;
				break;
			case A_CALL:
				if ((*prf) ("CALL   ") == -1)
					return B_FALSE;
				break;
			}
			if ((*prf) ("   esp 0x%08x  ebp 0x%08x\n",
				    esp, ebp) == -1)
				return B_FALSE;
		}
#endif /* DEBUG */

		switch (action) {
		case A_RET:
			if (esp != UNKNOWN) {
				ret_type = val;
				*espp = esp;
				*ebpp = ebp;
				if (sp_delta_p)
					*sp_delta_p = 0;
				return B_TRUE;
			}
			if (last_try != start_time)
				goto restart;
			if (!looped && prf) {
				(*prf) ("<<unknown esp adjustment at %x>>\n",
					unk_addr);
			}
			return B_FALSE;
		case A_CALL:
			/*
			 * If we're looking for 1st delta, bail out on a
			 * call instruction.
			 */
			if (sp_delta_p)
				return B_FALSE;
			/*
			 * Calls have no net effect on %esp, unless
			 * the function returns a structure.  We detect
			 * such functions via an explicit list.  These
			 * functions pop one longword off the stack.
			 */
			if (esp != UNKNOWN) {
				if (opsize == 2)
					val &= 0xFFFF;
				srfp = structret_funcs;
				while ((char *)srfp < (char *)structret_funcs +
							structret_funcs_size) {
					if (*srfp++ == val) {
						/* It returns a struct. */
						newesp = esp + 4;
					}
				}
			}
			break;
		case A_SETESP:
			newesp = val;
			break;
		case A_SETEBP:
			ebp = val;
			break;
		case A_PUSHEBP:
			if (esp != UNKNOWN) {
				if (opsize == 4) {
					newesp = esp - 4;
					if (ebp_addr == UNKNOWN) {
						ebp_addr = newesp;
						pushed_ebp = ebp;
					}
				} else
					newesp = esp - opsize;
			}
			break;
		case A_POPEBP:
			if (esp != UNKNOWN) {
				if (opsize == 4) {
					if (ebp_addr == esp) {
						ebp = pushed_ebp;
						ebp_addr = UNKNOWN;
					} else {
						if (ST_READ_STACK(esp, &ebp,
							    sizeof ebp) == -1)
							goto read_failed;
					}
				} else
					ebp = UNKNOWN;
				newesp = esp + opsize;
			} else
				ebp = UNKNOWN;
			break;
		case A_PUSHA:
			if (esp != UNKNOWN)
				newesp = esp - 8 * opsize;
			break;
		case A_POPA:
			if (esp != UNKNOWN) {
				if (opsize == 4) {
					if (ebp_addr == esp + 2 * sizeof(int)) {
						ebp = pushed_ebp;
						ebp_addr = UNKNOWN;
					} else {
						if (ST_READ_STACK(esp +
						     2 * sizeof(int),
						     &ebp, sizeof ebp) == -1)
							goto read_failed;
					}
				} else
					ebp = UNKNOWN;
				newesp = esp + 8 * opsize;
			} else
				ebp = UNKNOWN;
			break;
		case A_LEAVE:
			if (opsize != 4)
				ebp = UNKNOWN;
			if ((newesp = ebp) != UNKNOWN) {
				if (ST_READ_STACK(newesp, &ebp,
						  sizeof ebp) == -1)
					goto read_failed;
				newesp += 4;
			}
			break;
		case A_ENTER:
			/* TEMP: consider illegal for now */
			/* FALLTHROUGH */
		case A_ILL:
ill:
			if (last_try != start_time)
				goto restart;
			if (prf && action == A_ILL)
				(*prf) ("<<illegal opcode at %x>>\n", eip);
			return B_FALSE;
		case A_JMP:
		case A_JC:
			/* Handle branch instructions; val is destination */
			if (opsize == 2)
				val &= 0xFFFF;
			for (br = 0; br < nbranch; br++) {
				if (branch_addr[br] == eip)
					break;
			}
			if (br < nbranch) {
				if (branch_tried[br]) {
					if (branch_tried[br] >= last_try) {
						/* We looped; give up. */
						if (last_try != start_time) {
							looped = B_TRUE;
							goto restart;
						}
						return B_FALSE;
					}
					if (action == A_JC &&
					 branch_tried[br] >= last_new_branch) {
						branch_tried[br] = ninst;
						/* don't branch this time */
					} else {
						branch_tried[br] = ninst;
						/* follow jump address */
						addr = val;
					}
				} else {
					last_try = branch_tried[br] = ninst;
					/* follow jump address */
					addr = val;
				}
			} else if (nbranch < MAXBRANCH) {
				last_try = last_new_branch = ninst;
				branch_addr[nbranch++] = eip;
				if (action == A_JMP) {
					branch_tried[br] = ninst;
					/* follow jump address */
					addr = val;
				} else {
					branch_tried[br] = 0;
					/* don't take branch this time */
				}
			}
			break;
		case A_OPSIZE:
			/* toggle between 4 and 2 */
			opsize = 6 - opsize;
			stp = &states[0];
			continue;
		case A_GOTO:
			addr--;
			/* FALLTHROUGH */
		case A_CONT:
			while ((++stp)->st_mask != 0xFF || stp->st_opc != val) {
				ASSERT(stp <
				   &states[sizeof states / sizeof states[0]]);
			}
			++stp;
			continue;
		}

		if (newesp != esp) {
			if (sp_delta_p) {
				if (newesp == UNKNOWN)
					return B_FALSE;
				*sp_delta_p = newesp - esp;
				return B_TRUE;
			}
			if ((esp = newesp) == UNKNOWN && unk_addr == 0)
				unk_addr = eip;
		}

		eip = addr;
		stp = &states[0];
		opsize = 4;
	}
}


/*
 * int
 * stacktrace(int (*prf)(), ulong_t esp, ulong_t ebp, ulong_t eip,
 *	      ulong_t trap_r0ptr, ulong_t silent_entry,
 *	      int (*entry_frame_prf)(), lwp_t *lwpp)
 *	Print a kernel stack trace.
 *
 * Calling/Exit State:
 *	*prf is a printf-like function used to print any messages; if it
 *	is NULL, nothing will be printed; if the prf function returns -1,
 *	the stacktrace operation will be aborted and return -1.
 *
 *	The initial values of the esp, ebp, and eip registers are passed in,
 *	which, except for esp may be the value UNKNOWN if the corresponding
 *	register value is unknown.
 *
 *	If the caller has a pointer to the first trap frame on the stack,
 *	it may be passed in trap_r0ptr (which should be 0 otherwise).  This
 *	may be used to force synchronization if the normal methods run into
 *	trouble.
 *
 *	If silent_entry is non-zero, it will be taken as the address of a
 *	kernel function; stack frames up to and including the call to this
 *	function will not be printed.  In this case, *entry_frame_prf will
 *	be called prior to printing the first (subsequent) frame.
 */
int
stacktrace(int (*prf)(), vaddr_t kvublk, ulong_t esp, ulong_t ebp, ulong_t eip,
	   ulong_t trap_r0ptr, ulong_t silent_entry, int (*entry_frame_prf)(),
	lwp_t *lwpp)
{
	ulong_t	nexteip;	/* program counter (eip) in calling function */
	ulong_t	fn_entry;	/* entry point for current function */
	ulong_t	fn_start;	/* start of current function (from symbols) */
	int	sp_delta;	/* first adjustment of esp after func call */
	boolean_t ktrap;	/* interrupt/trap was from kernel mode */
	int	narg;		/* # arguments */
	char	tag;		/* call-type tag: '*' indirect or '~' close */
	ulong_t	reg;		/* temporary register value */
	int	stat = 0;	/* print status */
	lwp_t	lwp;

	KVUBLK = kvublk;

	if (silent_entry) {
		if (prf)
			silent_entry = ST_SYMVAL(silent_entry);
		else
			silent_entry = 0;
	}

	if (lwpp != NULL) {
		readmem(lwpp, 1, -1, (caddr_t)&lwp, sizeof(lwp_t),
			"lwp structure");
		ST_READ_STACK_INIT(&lwp);
	}

	/*
	 * Find start of stack trace.
	 *
	 * There are several ways we can do this, depending on the
	 * information we have available.
	 *
	 * If we have a trap frame pointer, we can start directly at
	 * the trap frame.
	 *
	 * If we have non-UNKNOWN values for eip and esp (and, preferably,
	 * ebp), that's where we start.
	 *
	 * Otherwise, we try to sync up with the first valid frame by
	 * advancing esp until it points to an address with the following
	 * properties:
	 *
	 *	*esp is a valid potential return address (i.e. it is a
	 *	valid kernel (text) address, and the instruction preceding
	 *	*esp is a call instruction).
	 *
	 *	Given this value of esp, and eip set to *esp, find_return
	 *	successfully finds another return address at nextesp.
	 *
	 *	*nextesp is a valid return address and the call instruction
	 *	(if a direct call) is a call to the routine containing eip.
	 */

	if (trap_r0ptr && !silent_entry) {
		eip = TRAP;
		goto trap_frame;
	}

	if (esp == UNKNOWN) {
		if (prf)
			return (*prf) ("<<must start with a known esp>>\n");
		return 0;
	}

	stack_unknown = B_FALSE;
	if (eip == UNKNOWN) {
		/* Attempt to sync up with first valid frame */
		ulong_t sp_start = esp;

		stack_unknown = B_TRUE;
		for (;; esp += sizeof(ulong_t)) {
			/* Give up eventually */
			if (esp - sp_start >= PAGESIZE / 4 &&
			    esp / PAGESIZE != sp_start / PAGESIZE) {
				if (prf)
					return (*prf)
					    ("<<no stack frames found>>\n");
				return 0;
			}
			if (ST_READ_STACK(esp, &eip, sizeof(ulong_t)) == -1)
				goto read_error;

			if (!ST_VALID_TEXT_ADDR(eip))
				continue;
			if (!st_is_after_call(eip, &fn_entry))
				continue;
			ebp = UNKNOWN;
			reg = esp + sizeof(ulong_t);
			if (!find_return(eip, &reg, &ebp, NULL, NULL))
				continue;
			if (ST_READ_STACK(reg, &nexteip, sizeof(ulong_t)) == -1)
				goto read_error;

			if (!ST_VALID_TEXT_ADDR(nexteip))
				continue;
			if (!st_is_after_call(nexteip, &fn_entry))
				continue;
			fn_start = ST_SYMVAL(eip);
			if (fn_entry == 0 || ST_SYMVAL(fn_entry) == fn_start) {
				esp += sizeof(ulong_t);
				break;
			}
		}
		ebp = UNKNOWN;
	}

	while (ST_VALID_TEXT_ADDR(eip)) {
		/* look through the stack for a valid ret addr */
		fn_start = ST_SYMVAL(eip);
		if (!find_return(eip, &esp, &ebp, prf, (int *)NULL) ||
		    esp == 0) {
			stat = nframe(eip, '>', 0, 0, 0, prf);
			break;
		}
		if (ST_READ_STACK(esp, &nexteip, sizeof(ulong_t)) == -1)
			goto read_error;
		if (ret_type == RT_IRET) {
			silent_entry = 0;
			if (iframe(eip, esp - T_EIP * sizeof(int), prf) == -1)
				return -1;
			if (!st_is_ktrap(esp - T_EIP * sizeof(int)))
				break;
			eip = nexteip;
			esp += 3 * sizeof(int); /* adjust for 'iret' inst */
			continue;
		}
		if (!ST_VALID_TEXT_ADDR(nexteip) ||
		    !st_is_after_call(nexteip, &fn_entry)) {
			if (prf) {
				stat = nframe(eip, '>', 0, 0, 0, prf) |
				        (*prf) ("<<invalid return address: %08X"
						" (esp = %08X)>>\n",
						nexteip, esp);
			}
			break;
		}
		esp += sizeof(ulong_t);	  /* adjust for 'ret' inst */
		if (silent_entry) {
			if (fn_start == silent_entry) {
				stat = (*entry_frame_prf) (esp, nexteip, prf);
				if (stat == -1)
					break;
done_silent:
				silent_entry = 0;
				if (trap_r0ptr) {
					eip = TRAP;
					goto trap_frame;
				}
			}
		} else {
			if (fn_entry == 0) {
				tag = '*';
				fn_entry = eip;
			} else {
				if (fn_entry < fn_start || fn_entry > eip) {
					if (nframe(eip, '>', 0, 0, 0,
						   prf) == -1) {
						return -1;
					}
				}
				tag = ' ';
			}
			if (IS_TRAP_ROUTINE(fn_entry))
				narg = 0;
			else if (find_return(nexteip, &esp, &ebp,
					     (int (*)())NULL, &sp_delta) &&
				 sp_delta > 0) {
				narg = sp_delta / sizeof(int);
				if (narg > 7 && narg > st_max_args) {
					if ((narg = st_max_args) < 7)
						narg = 7;
				}
			} else
				narg = st_max_args;
			if (nframe(fn_entry, tag, nexteip, esp, narg, prf)
								== -1)
				return -1;
			if (IS_TRAP_ROUTINE(fn_entry)) {
				trap_r0ptr = esp - sizeof(int) *
					((fn_entry == TRAP)
							? T_TRAPNO : T_EDI);
				silent_entry = 0;
trap_frame:
				ktrap = st_is_ktrap(trap_r0ptr);
				if (tframe(eip, trap_r0ptr, prf, ktrap) == -1)
					return -1;
				esp = trap_r0ptr + sizeof(int) *
						((ktrap ? T_EFL : T_SS) + 1);
				if (!ktrap)
					break;
				if (FRAMEREG(trap_r0ptr, T_CS, &reg) == -1)
					goto read_error;
				if ((ushort_t)reg != KCSSEL)
					break;
				if (FRAMEREG(trap_r0ptr, T_EIP, &eip) == -1)
					goto read_error;
				if (FRAMEREG(trap_r0ptr, T_EBP, &ebp) == -1)
					goto read_error;
				continue;
			}
		}
		if (trap_r0ptr && silent_entry && esp >= trap_r0ptr) {
			/*
			 * If we've missed the known trap frame (only
			 * possible if silent_entry, since otherwise
			 * we went directly to the trap frame), assume
			 * the stack trace is incorrect, and start from
			 * the trap frame now.
			 */
			goto done_silent;
		}
		eip = nexteip;
	}

	return stat;

read_error:
	if (prf)
		(*prf) ("<<stack read error>>\n");
	return -1;
}

#define LINE_WIDTH	80
#define FUNC_WIDTH	(LINE_WIDTH - 1 - 26)

/*
 * static int
 * nframe(vaddr_t eip, char tag, vaddr_t preveip, vaddr_t ap, uint_t narg,
 *        int (*prf)())
 *	Print a "normal" stack frame.
 *
 * Calling/Exit State:
 *	eip is the function called; preveip is the return address;
 *	tag is a character to prepend to the output; ap is the stack
 *	address of the first argument to the function; narg is the
 *	number of arguments to print.
 *
 *	*prf is a printf-like function used to do the printing; if it
 *	returns -1, the operation will be aborted and nframe will return -1.
 */
static int
nframe(vaddr_t eip, char tag, vaddr_t preveip, vaddr_t ap, uint_t narg,
       int (*prf)())
{
	ulong_t esp = ap - sizeof(ulong_t);
	ulong_t arg;
	uint_t nc, col;

	if (prf == NULL)
		return 0;

	if ((*prf) ("%c", tag) == -1 ||
	    (nc = ST_SHOW_SYM_ADDR(eip, prf)) == -1)
		return -1;
	if (ap == 0)
		return (*prf) ("()\n");

	if ((*prf) ("(") == -1)
		return -1;
	col = nc + 2;
	while (narg-- != 0) {
		if (ST_READ_STACK(ap, &arg, sizeof arg) == -1 ||
		    (nc = ST_PRF_RET_LEN(prf, "%x", arg)) == -1)
			return -1;
		col += nc;
		ap += sizeof(ulong_t);
		if (narg != 0) {
			if ((*prf) (" ") == -1)
				return -1;
			col++;
		}
	}
	if ((*prf) (")") == -1)
		return -1;
	col++;

	if (col > FUNC_WIDTH) {
		while (col < LINE_WIDTH - 1) {
			if ((*prf) (".") == -1)
				return -1;
			col++;
		}
		if ((*prf) ("\n") == -1)
			return -1;
		if ((*prf) ("      ") == -1)
			return -1;
		col = 6;
	}
	while (col <= FUNC_WIDTH) {
		if ((*prf) (".") == -1)
			return -1;
		col++;
	}

	if ((*prf) ("esp:%08x", esp) == -1)
		return -1;
	if (preveip)
		return (*prf) (" ret:%08x\n", preveip);
	else
		return (*prf) ("\n");
}


/*
 * static int
 * tframe(vaddr_t eip, vaddr_t trap_r0ptr, int (*prf)(), boolean_t ktrap)
 *	Print a trap stack frame.
 *
 * Calling/Exit State:
 *	eip is the trap function called; trap_r0ptr is the stack address
 *	of the trap frame; if ktrap is true, the trap was from kernel mode,
 *	otherwise from user mode.
 *
 *	*prf is a printf-like function used to do the printing; if it
 *	returns -1, the operation will be aborted and tframe will return -1.
 */
static int
tframe(vaddr_t eip, vaddr_t trap_r0ptr, int (*prf)(), boolean_t ktrap)
{
	vaddr_t fn_start;
	ulong_t reg;

	ST_REGISTER_TRAP_FRAME(trap_r0ptr, B_TRUE);

	if (prf == NULL)
		return 0;

	fn_start = ST_SYMVAL(eip);
	if (fn_start == TRAP) {
		if (FRAMEREG(trap_r0ptr, T_TRAPNO, &reg) == -1 ||
		    (*prf) ("TRAP 0x%x", reg) == -1)
			return -1;
	} else if (fn_start == SYSTRAP) {
		if ((*prf) ("SYSTEM CALL") == -1)
			return -1;
	} else if (fn_start == SIGCLEAN) {
		if ((*prf) ("SIGNAL RETURN") == -1)
			return -1;
	} else if (fn_start == EVT_PROCESS) {
		if ((*prf) ("TRAP EVENT") == -1)
			return -1;
	} else {
		if ((*prf) ("?TRAP TO ") == -1 ||
		    ST_SHOW_SYM_ADDR(eip, prf) == -1 ||
		    FRAMEREG(trap_r0ptr, T_TRAPNO, &reg) == -1 ||
		    (*prf) (" (trap 0x%x)", reg) == -1)
			return -1;
	}
	if (FRAMEREG(trap_r0ptr, T_CS, &reg) == -1 ||
	    (*prf) (" from %x:", (ushort_t)reg) == -1 ||
	    FRAMEREG(trap_r0ptr, T_EIP, &reg) == -1 ||
	    (*prf) ("%x (r0ptr:%08x", reg, trap_r0ptr) == -1)
		return -1;
	if (ktrap) {
		if ((*prf) (")\n") == -1)
			return -1;
	} else {
		if (FRAMEREG(trap_r0ptr, T_SS, &reg) == -1 ||
		    (*prf) (", ss:esp: %x:", (ushort_t)reg) == -1 ||
		    FRAMEREG(trap_r0ptr, T_UESP, &reg) == -1 ||
		    (*prf) ("%x)\n", reg) == -1)
			return -1;
	}
	return st_frameregs(trap_r0ptr, prf, 1);
}


/*
 * static int
 * iframe(vaddr_t eip, vaddr_t trap_r0ptr, int (*prf)())
 *	Print an interrupt stack frame.
 *
 * Calling/Exit State:
 *	eip is the trap function called; trap_r0ptr is the stack address
 *	of the interrupt frame.
 *
 *	*prf is a printf-like function used to do the printing; if it
 *	returns -1, the operation will be aborted and iframe will return -1.
 */
static int
iframe(vaddr_t eip, vaddr_t trap_r0ptr, int (*prf)())
{
	ulong_t reg;

	ST_REGISTER_TRAP_FRAME(trap_r0ptr, B_FALSE);

	if (prf == NULL)
		return 0;

	if ((*prf) ("INTERRUPT TO ") == -1 ||
	    ST_SHOW_SYM_ADDR(eip, prf) == -1 ||
	    FRAMEREG(trap_r0ptr, T_CS, &reg) == -1 ||
	    (*prf) (" from %x:", (ushort_t)reg) == -1 ||
	    FRAMEREG(trap_r0ptr, T_EIP, &reg) == -1 ||
	    (*prf) ("%x (r0ptr:%08x", reg, trap_r0ptr) == -1)
		return -1;
	if (st_is_ktrap(trap_r0ptr)) {
		if ((*prf) (")\n") == -1)
			return -1;
	} else {
		if (FRAMEREG(trap_r0ptr, T_SS, &reg) == -1 ||
		    (*prf) (", ss:esp: %x:", (ushort_t)reg) == -1 ||
		    FRAMEREG(trap_r0ptr, T_UESP, &reg) == -1 ||
		    (*prf) ("%x)\n", reg) == -1)
			return -1;
	}
	return st_frameregs(trap_r0ptr, prf, 0);
}


/*
 * int
 * st_frameregs(vaddr_t r0ptr, int (*prf)(), boolean_t full_frame)
 *	Print registers for a trap/interrupt stack frame.
 *
 * Calling/Exit State:
 *	r0ptr is the stack address of the frame; if full_frame is true,
 *	it is a trap frame, with all registers saved, else it is an
 *	interrupt frame, which only saves some registers.
 *
 *	*prf is a printf-like function used to do the printing; if it
 *	returns -1, the operation will be aborted and it will return -1.
 */
int
st_frameregs(vaddr_t r0ptr, int (*prf)(), boolean_t full_frame)
{
	ulong_t reg;

	if (FRAMEREG(r0ptr, T_EAX, &reg) == -1 ||
	    (*prf) ("   eax:%8x ebx:", reg) == -1)
		return -1;
	if (full_frame) {
		if (FRAMEREG(r0ptr, T_EBX, &reg) == -1 ||
		    (*prf) ("%8x", reg) == -1)
			return -1;
	} else {
		if ((*prf) (" -------") == -1)
			return -1;
	}
	if (FRAMEREG(r0ptr, T_ECX, &reg) == -1 ||
	    (*prf) (" ecx:%8x", reg) == -1 ||
	    FRAMEREG(r0ptr, T_EDX, &reg) == -1 ||
	    (*prf) (" edx:%8x", reg) == -1 ||
	    FRAMEREG(r0ptr, T_EFL, &reg) == -1 ||
	    (*prf) (" efl:%8x", reg) == -1)
		return -1;
	if (!st_is_ktrap(r0ptr)) {
		if (FRAMEREG(r0ptr, T_DS, &reg) == -1 ||
		    (*prf) ("   ds:%4x", (ushort_t)reg) == -1)
		return -1;
	}
	if ((*prf) ("\n   esi:") == -1)
		return -1;
	if (full_frame) {
		if (FRAMEREG(r0ptr, T_ESI, &reg) == -1 ||
		    (*prf) ("%8x", reg) == -1 ||
		    FRAMEREG(r0ptr, T_EDI, &reg) == -1 ||
		    (*prf) (" edi:%8x", reg) == -1)
			return -1;
	} else {
		if ((*prf) (" ------- edi: -------") == -1)
			return -1;
	}
	if ((*prf) (" esp:%8x", r0ptr + sizeof(int) *
			      ((st_is_ktrap(r0ptr) ? T_EFL : T_SS) + 1)) == -1)
		return -1;
	if (full_frame) {
		if (FRAMEREG(r0ptr, T_EBP, &reg) == -1 ||
		    (*prf) (" ebp:%8x ", reg) == -1)
			return -1;
	} else {
		if ((*prf) (" ebp: ------- ") == -1)
			return -1;
	}
	if (ST_SHOW_REGSET(prf) == -1)
		return -1;
	if (!st_is_ktrap(r0ptr)) {
		if (FRAMEREG(r0ptr, T_ES, &reg) == -1 ||
		    (*prf) ("     es:%4x\n", (ushort_t)reg) == -1)
			return -1;
	} else if ((*prf) ("\n") == -1)
		return -1;
	return 0;
}


/*
 * static boolean_t
 * st_is_after_call(vaddr_t addr, vaddr_t *dst_addr_p)
 *	Determine if an address is after a call instruction.
 *
 * Calling/Exit State:
 *	addr is a kernel text address.  If the instruction preceding addr
 *	is a call instruction, *dst_addr_p is set to the destination of
 *	the call if known, else 0 (indirect call), and B_TRUE is returned.
 */
static boolean_t
st_is_after_call(vaddr_t addr, vaddr_t *dst_addr_p)
{
	uchar_t	opc[7], *opp;

	if (ST_READ_TEXT(addr - 7, opc, sizeof(opc)) == -1)
		return B_FALSE;
	if (opc[2] == OPC_CALL_REL) {
		((uchar_t *)dst_addr_p)[0] = opc[3];
		((uchar_t *)dst_addr_p)[1] = opc[4];
		((uchar_t *)dst_addr_p)[2] = opc[5];
		((uchar_t *)dst_addr_p)[3] = opc[6];
		*dst_addr_p += addr;
		if (ST_VALID_TEXT_ADDR(*dst_addr_p))
			return B_TRUE;
	}
	if (opc[0] == OPC_CALL_DIR) {
		((uchar_t *)dst_addr_p)[0] = opc[1];
		((uchar_t *)dst_addr_p)[1] = opc[2];
		((uchar_t *)dst_addr_p)[2] = opc[3];
		((uchar_t *)dst_addr_p)[3] = opc[4];
		if (ST_VALID_TEXT_ADDR(*dst_addr_p))
			return B_TRUE;
	}
	for (opp = opc + 5; opp >= opc; opp--) {
		if (*opp != OPC_CALL_IND)
			continue;
		if ((opp[1] & 0x38) == 0x10) {
			*dst_addr_p = 0;
			return B_TRUE;
		}
	}
	return B_FALSE;
}


/*
 * static boolean_t
 * st_is_ktrap(vaddr_t r0ptr)
 *	Determine if a trap/interrupt was from kernel mode.
 *
 * Calling/Exit State:
 *	r0ptr is the stack address of an interrupt or trap frame.
 *	If the trap or interrupt was from kernel mode, return true.
 */
static boolean_t
st_is_ktrap(vaddr_t r0ptr)
{
	ulong_t reg;

	if (FRAMEREG(r0ptr, T_EFL, &reg) == -1)
		return B_FALSE;
	if (reg & PS_VM)
		return B_FALSE;
	if (FRAMEREG(r0ptr, T_CS, &reg) == -1)
		return B_FALSE;
	return ((reg & RPL_MASK) == KERNEL_RPL);
}
