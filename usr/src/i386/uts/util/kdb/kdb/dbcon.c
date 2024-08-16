/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:util/kdb/kdb/dbcon.c	1.13"
#ident	"$Header: $"

/*
 * kernel debugger console interface routines
 */

#include <io/conssw.h>
#include <util/cmn_err.h>	/* for VA_LIST */
#include <util/kdb/kdebugger.h>
#include <util/param.h>
#include <util/types.h>

#define EOT     0x04    /* ascii eot character */

#define XOFF    0x13
#define XON     0x11

#define INTR	0x03	/* ^C interrupt character */
#define INTR2	0x7F	/* DEL interrupt character */

extern void db_flush_input(void);
extern void db_brk_msg(int);
extern int strlen(const char *);
extern int db_msg_pending;
static int pending_c = -1;

uint_t db_nlines = 24;
uint_t db_lines_left = 24;

void dbprintf(const char *fmt, ...);
static void dbprintn(long, int, int, int, int);


/*
 * dbgets reads a line from the debugging console using polled i/o
 */

char *
dbgets(char *buf, int count)
{
    int c;
    int i;

    db_lines_left = db_nlines;	/* reset pagination */
    kdb_output_aborted = B_FALSE;

    count--;
    for (i = 0; i < count; ) {
	if ((c = pending_c) != -1)
	    pending_c = -1;
	else
	    while ((c = DBG_GETCHAR()) == -1) ;
	if (c == INTR || c == INTR2)
	    continue;
	DBG_PUTCHAR(c);
	if (c == '\r') {
	    DBG_PUTCHAR(c = '\n');
	}
	else if (c == '\b') {                /* backspace */
	    DBG_PUTCHAR(' ');
	    DBG_PUTCHAR('\b');
	    if (i > 0)
		    i--;
	    continue;
	}
	else if (c == EOT && i == 0)         /* ctrl-D */
	    return NULL;
	buf[i++] = (char)c;
	if (c == '\n')
	    break;
    }
    buf[i] = '\0';
    return (buf);
}


void
dbputc(int chr)
{
	int c2;

	if (kdb_output_aborted)
		return;

	/* Just in case... */
	if (db_nlines < 2)
		db_nlines = 24;
	if (db_lines_left < 2)
		db_lines_left = 2;

	if (chr == '\n') {
		if (--db_lines_left == 1) {
			db_lines_left = db_nlines + 1;
			dbprintf("\n[MORE]---");
			if (kdb_output_aborted)
				return;
			while ((c2 = DBG_GETCHAR()) == -1)
				;
			dbprintf("\r         ");
			if (kdb_output_aborted)
				return;
			if (c2 == INTR || c2 == INTR2)
				pending_c = c2;
			chr = '\r';
		}
		if (pending_c == -1) {
			if ((pending_c = DBG_GETCHAR()) == XOFF) {
				while ((pending_c = DBG_GETCHAR()) != XON) {
					if (pending_c == INTR ||
					    pending_c == INTR2)
						break;
				}
				if (pending_c == XON)
					pending_c = -1;
			}
		}
		if (pending_c == INTR || pending_c == INTR2) {
			pending_c = -1;
			dbg_putc_count = 0;
			kdb_output_aborted = B_TRUE;
			db_flush_input();
		}
		DBG_PUTCHAR('\r');
	}
	DBG_PUTCHAR(chr);
}


void
db_xprintf(const char *fmt, VA_LIST ap)
{
	int c;
	char *s;
	int prec, width, ljust, zeropad;

loop:
	while ((c = *fmt++) != '%') {
		if (c == '\0')
			return;
		dbputc(c);
		if (kdb_output_aborted)
			return;
	}
	if (*fmt == '%') {
		dbputc('%');
		fmt++;
		goto loop;
	}
	prec = width = ljust = zeropad = 0;
	if (*fmt == '-') {
		ljust = 1;
		++fmt;
	}
	if (*fmt == '0') {
		zeropad = 1;
		++fmt;
	}
	if (*fmt == '*') {
		width = VA_ARG(ap, int);
		++fmt;
	} else {
		while ('0' <= *fmt && *fmt <= '9')
			width = width * 10 + *fmt++ - '0';
	}
	if (*fmt == '.') {
		if (*++fmt == '*') {
			prec = VA_ARG(ap, int);
			++fmt;
		} else {
			while ( '0' <= *fmt && *fmt <= '9' )
				prec = prec * 10 + *fmt++ - '0';
		}
	}
	if ((c = *fmt++) == 'l')
		c = *fmt++;
	if (c <= 'Z' && c >= 'A')
		c += 'a' - 'A';
	if (c == 'd' || c == 'u' || c == 'o' || c == 'x') {
		dbprintn(VA_ARG(ap, long), c == 'o' ? 8 : (c == 'x' ? 16 : 10),
			 width, ljust, zeropad);
	} else if (c == 's') {
		s = VA_ARG(ap, char *);
		if (prec == 0 || prec > strlen(s))
			prec = strlen(s);
		width -= prec;
		if (!ljust) {
			while (width-- > 0) {
				dbputc(' ');
				if (kdb_output_aborted)
					return;
			}
		}
		while (prec-- > 0) {
			dbputc(*s++);
			if (kdb_output_aborted)
				return;
		}
		if (ljust) {
			while (width-- > 0) {
				dbputc(' ');
				if (kdb_output_aborted)
					return;
			}
		}
	} else if (c == 'c') {
		if (!ljust) {
			while (width-- > 1) {
				dbputc(' ');
				if (kdb_output_aborted)
					return;
			}
		}
		dbputc(VA_ARG(ap, int));
		if (ljust) {
			while (width-- > 1) {
				dbputc(' ');
				if (kdb_output_aborted)
					return;
			}
		}
	}
	goto loop;
}

void
dbprintf(const char *fmt, ...)
{
	VA_LIST	ap;

	if (db_msg_pending)
		db_brk_msg(1);

	VA_START(ap, fmt);

	db_xprintf(fmt, ap);
}

static void
dbprintn(long n, int b, int width, int ljust, int zeropad)
{
	int i, nd, c;
	boolean_t flag, minus;
	int plmax;
	char d[12];

	c = 1;
	minus = B_FALSE;
	if ((flag = (n < 0)) != 0)
		n = -n;
	if (b == 8)
		plmax = 11;
	else if (b == 10)
		plmax = 10;
	else if (b == 16)
		plmax = 8;
	if (flag && b == 10) {
		flag = B_FALSE;
		minus = B_TRUE;
	}
	for (i = 0; i < plmax; i++) {
		nd = n % b;
		if (flag) {
			nd = (b - 1) - nd + c;
			if (nd >= b) {
				nd -= b;
				c = 1;
			} else
				c = 0;
		}
		d[i] = (char)nd;
		n = n / b;
		if (n == 0 && !flag)
			break;
	}
	if (i == plmax)
		i--;

	width -= (i + 1) + minus;

	if (!ljust) {
		if (zeropad) {
			if (minus) {
				dbputc('-');
				minus = B_FALSE;
			}
			c = '0';
		} else
			c = ' ';
		while (width-- > 0) {
			dbputc(c);
			if (kdb_output_aborted)
				return;
		}
	}

	if (minus)
		dbputc('-');

	for (; i >= 0; i--)
		dbputc("0123456789ABCDEF"[d[i]]);

	if (ljust) {
		while (width-- > 0) {
			dbputc(' ');
			if (kdb_output_aborted)
				return;
		}
	}
}
