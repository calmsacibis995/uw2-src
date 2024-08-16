/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:util/mod/modksym.cf/Stubs.c	1.6"
#ident	"$Header: $"

#include <sys/immu.h>
#include <sys/types.h>
#include <sys/mod_obj.h>
#include <sys/mod_k.h>
#include <sys/errno.h>

struct modobj *mod_obj_kern = 0;
size_t mod_obj_size = 0;
int mod_obj_pagesymtab = MOD_NOPAGE;

extern char stext[];
extern char sdata[];

void modinit(void) {}
boolean_t mod_obj_validaddr(unsigned long value) {

	if(value >= (unsigned long) stext && value < (unsigned long) sdata)
		return(B_TRUE);
	return(B_FALSE);
}
unsigned long mod_obj_getsymvalue(const char *n, boolean_t k, int flags) { return(0); }
char *mod_obj_getsymname(unsigned long v, unsigned long *o, int flags, char *r) { return(0); }
int getksym() { return(ENOSYS); }
void mod_obj_modrele(struct modctl *mcp) {}
struct modctl *mod_obj_getsym_mcp(const char *name, boolean_t kernelonly, 
	int flags, Elf32_Sym *retsp) { return (NULL); }
