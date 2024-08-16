/
/ Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990
/ Sequent Computer Systems, Inc.   All rights reserved.
/
/ This software is furnished under a license and may be used
/ only in accordance with the terms of that license and with the
/ inclusion of the above copyright notice.   This software may not
/ be provided or otherwise made available to, or used by, any
/ other person.  No title to or ownership of the software is
/ hereby transferred.
/

		.ident	"@(#)stand:i386sym/standalone/i386/asm.m4	1.1"

include(assym.h)

/* Stack Pointer Based */
define(`SPARG0',`4+0(%esp)')
define(`SPARG1',`4+4(%esp)')
define(`SPARG2',`4+8(%esp)')
define(`SPARG3',`4+12(%esp)')
define(`SPARG4',`4+16(%esp)')
define(`SPARG5',`4+20(%esp)')
define(`SPARG6',`4+24(%esp)')
define(`SPARG7',`4+28(%esp)')

/* Frame Pointer Based */
define(`FPARG0',`8+0(%ebp)')
define(`FPARG1',`8+4(%ebp)')
define(`FPARG2',`8+8(%ebp)')
define(`FPARG3',`8+12(%ebp)')
define(`FPARG4',`8+16(%ebp)')
define(`FPARG5',`8+20(%ebp)')
define(`FPARG6',`8+24(%ebp)')
define(`FPARG7',`8+28(%ebp)')

/* Other nifty stuff */
define(`CALL',`call')
define(`RETURN',`ret')
define(`ENTER',`pushl %ebp; movl %esp,%ebp')
define(`EXIT',`leave')
