/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/* Copyright (c) 1981 Regents of the University of California */
#ident	"@(#)vi:port/ex_addr.c	1.7.1.6"
#ident  "$Header: ex_addr.c 1.2 91/06/27 $"
#include "ex.h"
#include "ex_re.h"

/*
 * Routines for address parsing and assignment and checking of address bounds
 * in command mode.  The routine address is called from ex_cmds.c
 * to parse each component of a command (terminated by , ; or the beginning
 * of the command itself.  It is also called by the scanning routine
 * in ex_voperate.c from within open/visual.
 *
 * Other routines here manipulate the externals addr1 and addr2.
 * These are the first and last lines for the current command.
 *
 * The variable bigmove remembers whether a non-local glitch of . was
 * involved in an address expression, so we can set the previous context
 * mark '' when such a motion occurs.
 */

static	bool bigmove;

/*
 * Set up addr1 and addr2 for commands whose default address is dot.
 */
setdot()
{

	setdot1();
	if (bigmove)
		markDOT();
}

/*
 * Call setdot1 to set up default addresses without ever
 * setting the previous context mark.
 */
setdot1()
{

	if (addr2 == 0)
		addr1 = addr2 = dot;
	if (addr1 > addr2) {
		notempty();
		error(":7", "Addr1 > addr2|First address exceeds second");
	}
}

/*
 * Ex allows you to say
 *	delete 5
 * to delete 5 lines, etc.
 * Such nonsense is implemented by setcount.
 */
setcount()
{
	register int cnt;

	pastwh();
	if (!isdigit(peekchar())) {
		setdot();
		return;
	}
	addr1 = addr2;
	setdot();
	cnt = getnum();
	if (cnt <= 0)
		error(":8", "Bad count|Nonzero count required");
	addr2 += cnt - 1;
	if (addr2 > dol)
		addr2 = dol;
	nonzero();
}

/*
 * Parse a number out of the command input stream.
 */
getnum()
{
	register int cnt;

	for (cnt = 0; isdigit(peekcd());)
		cnt = cnt * 10 + getchar() - '0';
	return (cnt);
}

/*
 * Set the default addresses for commands which use the whole
 * buffer as default, notably write.
 */
setall()
{

	if (addr2 == 0) {
		addr1 = one;
		addr2 = dol;
		if (dol == zero) {
			dot = zero;
			return;
		}
	}
	/*
	 * Don't want to set previous context mark so use setdot1().
	 */
	setdot1();
}

/*
 * No address allowed on, e.g. the file command.
 */
setnoaddr()
{

	if (addr2 != 0)
		error(":9", "No address allowed@on this command");
}

/*
 * Parse an address.
 * Just about any sequence of address characters is legal.
 *
 * If you are tricky you can use this routine and the = command
 * to do simple addition and subtraction of cardinals less
 * than the number of lines in the file.
 */
line *
address(inline)
	unsigned char *inline;
{
	register line *addr;
	register int offset, c;
	short lastsign;

	bigmove = 0;
	lastsign = 0;
	offset = 0;
	addr = 0;
	for (;;) {
		if (isdigit(peekcd())) {
			if (addr == 0) {
				addr = zero;
				bigmove = 1;
			}
			loc1 = 0;
			addr += offset;
			offset = getnum();
			if (lastsign >= 0)
				addr += offset;
			else
				addr -= offset;
			lastsign = 0;
			offset = 0;
		}
		switch (c = getcd()) {

		case '?':
		case '/':
		case '$':
		case '\'':
		case '\\':
			bigmove++;
		case '.':
			if (addr || offset)
				error(":10", "Badly formed address");
		}
		offset += lastsign;
		lastsign = 0;
		switch (c) {

		case ' ':
		case '\t':
			continue;

		case '+':
			lastsign = 1;
			if (addr == 0)
				addr = dot;
			continue;

		case '^':
		case '-':
			lastsign = -1;
			if (addr == 0)
				addr = dot;
			continue;

		case '\\':
		case '?':
		case '/':
			c = vi_compile(c, &scanre);
			notempty();
			addr = dot;
			if (inline && execute(dot, 1)) {
				if (c == '/') {
					while (loc1 <= (char *)inline) {
						if (loc1 == loc2)
							loc2++;
						if (!execute((line *)0, 1))
							goto nope;
					}
					break;
				} else if (loc1 < (char *)inline) {
					unsigned char *last;
doques:

					do {
						last = (unsigned char *)loc1;
						if (loc1 == loc2)
							loc2++;
						if (!execute((line *)0, 1))
							break;
					} while (loc1 < (char *)inline);
					loc1 = (char *)last;
					break;
				}
			}
nope:
			for (;;) {
				if (c == '/') {
					addr++;
					if (addr > dol) {
						if (value(vi_WRAPSCAN) == 0)
error(":11", 
	"No match to BOTTOM|Address search hit BOTTOM without matching pattern")
;
						addr = zero;
					}
				} else {
					addr--;
					if (addr < zero) {
						if (value(vi_WRAPSCAN) == 0)
error(":12", "No match to TOP|Address search hit TOP without matching pattern");
						addr = dol;
					}
				}
				if (execute(addr, 1)) {
					if (inline && c == '?') {
						inline = &linebuf[LBSIZE];
						goto doques;
					}
					break;
				}
				if (addr == dot)
					error(":13", "Fail|Pattern not found");
			}
			continue;

		case '$':
			addr = dol;
			continue;

		case '.':
			addr = dot;
			continue;

		case '\'':
			c = markreg(getchar());
			if (c == 0)
				error(":14", "Marks are ' and a-z");
			addr = getmark(c);
			if (addr == 0)
				error(":15", "Undefined mark@referenced");
			break;

		default:
			ungetchar(c);
			if (offset) {
				if (addr == 0)
					addr = dot;
				addr += offset;
				loc1 = 0;
			}
			if (addr == 0) {
				bigmove = 0;
				return (0);
			}
			if (addr != zero)
				notempty();
			addr += lastsign;
			if (addr < zero)
				error(":16", 
					"Negative address@- first buffer line is 1")
				;
			if (addr > dol)
				error(":17", "Not that many lines@in buffer");
			return (addr);
		}
	}
}

/*
 * Abbreviations to make code smaller
 * Left over from squashing ex version 1.1 into
 * 11/34's and 11/40's.
 */
setCNL()
{

	setcount();
	donewline();
}

setNAEOL()
{

	setnoaddr();
	eol();
}
