/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)lprof:bblk/i386/macglb.c	1.1"
/*
* The common global variable (cmd_tbl[]) is initialized with 386
*              dependent jump and branch opcodes.
*/

/* To add instructions to this table, must make sure CAinstr() in mac.c
   contains check to first letter of instruction */

	/* command table of all possible branch and jump instruction for the 386 */
	/* The table will be searched by the CAinstr() function */
char *cmd_tbl[] = {
	"call",
	"ja",
	"jae",
	"jb",
	"jbe",
	"jc",
	"jcxz",
	"je",
	"jecxz",
	"jg",
	"jge",
	"jl",
	"jle",
	"jmp",
	"jna",
	"jnae",
	"jnb",
	"jnbe",
	"jnc",
	"jne",
	"jng",
	"jnge",
	"jnl",
	"jnle",
	"jno",
	"jnp",
	"jns",
	"jnz",
	"jo",
	"jp",
	"jpe",
	"jpo",
	"js",
	"jz",
	"lcall",
	"ljmp",
	"lret",
	"loop",
	"loope",
	"loopne",
	"loopnz",
	"loopz",
	"ret",
};

char *tblptr;				/* table entry character pointer */

int tblsize = sizeof(cmd_tbl) / sizeof(tblptr);	/* command table size */
