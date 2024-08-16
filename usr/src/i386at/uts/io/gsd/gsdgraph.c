/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/gsd/gsdgraph.c	1.1"
#ident	"$Header: $"

/*
 * gsdgraph.c, graphics routines.
 */

#include <io/ansi/at_ansi.h>
#include <io/ws/ws.h>
#include <io/gsd/gsd.h>
#include <util/cmn_err.h>
#include <util/types.h>

#define	TXT2GR(TSP, POS)	(((POS)/(TSP)->t_cols)*(TSP)->t_cols*16+((POS)%(TSP)->t_cols))

STATIC void gdv_xferlines(channel_t *, ushort, ushort, int, char);
STATIC void gdv_xferchars(channel_t *, ushort, ushort, int, char);

extern wstation_t Kdws;

/*
 * void
 * gs_writebitmap(channel_t *, int dest, unsigned char *, int, uchar_t)
 *
 * Calling/Exit status:
 *
 * Description:
 *	This routines writes (8*n) x 16 to the screen using the forground
 *	and background color specified by attr. If the background color is
 *	black, color handling is not required.
 *
 */
void
gs_writebitmap(channel_t *chp, int dest, unsigned char *bitmap, int size, uchar_t attr)
{
	if (WS_ISACTIVECHAN(&Kdws, chp))
	{
		volatile char	*scrptr = (char *) ((chp)->ch_vstate.v_scrp);
		termstate_t	*tsp = &chp->ch_tstate;
		int		bpr = tsp->t_cols;
		unsigned char	fg = (attr & 0xf);
		unsigned char	bg = (attr >> 4) & 0xf;
		int		tonext;
		int		index, row, col;

		switch(size) {
		case 1:
			tonext = tsp->t_cols;
			if (bg == 0 || (fg&bg) == bg) {
				outb(0x3ce, 0x00); /* 0x00 - set/reset reg */
				outb(0x3cf, fg&bg);
				outb(0x3ce, 0x01); /* 0x01 - s/r enable reg */
				outb(0x3cf, (fg^0xf)|(fg&bg));

#if 0
				outb(0x3c4, 2); /* color map reg */
				outb(0x3c5, 0xf);
#endif
				scrptr = (char *) ((chp)->ch_vstate.v_scrp) +
							TXT2GR(tsp, dest);
				*scrptr = *bitmap++; scrptr += tonext;
				*scrptr = *bitmap++; scrptr += tonext;
				*scrptr = *bitmap++; scrptr += tonext;
				*scrptr = *bitmap++; scrptr += tonext;
				*scrptr = *bitmap++; scrptr += tonext;
				*scrptr = *bitmap++; scrptr += tonext;
				*scrptr = *bitmap++; scrptr += tonext;
				*scrptr = *bitmap++; scrptr += tonext;
				*scrptr = *bitmap++; scrptr += tonext;
				*scrptr = *bitmap++; scrptr += tonext;
				*scrptr = *bitmap++; scrptr += tonext;
				*scrptr = *bitmap++; scrptr += tonext;
				*scrptr = *bitmap++; scrptr += tonext;
				*scrptr = *bitmap++; scrptr += tonext;
				*scrptr = *bitmap++; scrptr += tonext;
				*scrptr = *bitmap++;
#if 0
				outb(0x3c5, 0xf);
#endif

				outb(0x3ce, 0x00); /* 0x00 - set/reset reg */
				outb(0x3cf, 0x0);
				outb(0x3ce, 0x01); /* 0x01 - s/r enable reg */
				outb(0x3cf, 0x0);
			} else {
				unsigned char	write_mode;

				scrptr = (char *) ((chp)->ch_vstate.v_scrp) +
							TXT2GR(tsp, dest);

				outb(0x3ce, 0x00); /* 0x00 - set/reset reg */
				outb(0x3cf, bg);
				outb(0x3ce, 0x01); /* 0x01 - s/r enable reg */
				outb(0x3cf, 0xf);

				*scrptr = 0xff;

				outb(0x3ce, 0x00); /* 0x00 - set/reset reg */
				outb(0x3cf, 0x0);
				outb(0x3ce, 0x01); /* 0x01 - s/r enable reg */
				outb(0x3cf, 0x0);

				scrptr = (char *) ((chp)->ch_vstate.v_scrp) +
							TXT2GR(tsp, dest);

				outb(0x3ce, 0x05); /* 0x05 - mode register */
				write_mode = inb(0x3cf); /* write mode 2 */
				outb(0x3cf, (write_mode & 0xfc) | 0x2);

				outb(0x3ce, 0x08); /* 0x08 - bit plane mask */

				*scrptr;
				outb(0x3cf, *bitmap++);
				*scrptr = fg; scrptr += tonext;
				outb(0x3cf, *bitmap++);
				*scrptr = fg; scrptr += tonext;
				outb(0x3cf, *bitmap++);
				*scrptr = fg; scrptr += tonext;
				outb(0x3cf, *bitmap++);
				*scrptr = fg; scrptr += tonext;
				outb(0x3cf, *bitmap++);
				*scrptr = fg; scrptr += tonext;
				outb(0x3cf, *bitmap++);
				*scrptr = fg; scrptr += tonext;
				outb(0x3cf, *bitmap++);
				*scrptr = fg; scrptr += tonext;
				outb(0x3cf, *bitmap++);
				*scrptr = fg; scrptr += tonext;
				outb(0x3cf, *bitmap++);
				*scrptr = fg; scrptr += tonext;
				outb(0x3cf, *bitmap++);
				*scrptr = fg; scrptr += tonext;
				outb(0x3cf, *bitmap++);
				*scrptr = fg; scrptr += tonext;
				outb(0x3cf, *bitmap++);
				*scrptr = fg; scrptr += tonext;
				outb(0x3cf, *bitmap++);
				*scrptr = fg; scrptr += tonext;
				outb(0x3cf, *bitmap++);
				*scrptr = fg; scrptr += tonext;
				outb(0x3cf, *bitmap++);
				*scrptr = fg; scrptr += tonext;
				outb(0x3cf, *bitmap);
				*scrptr = fg;

				outb(0x3cf, 0xff);
				outb(0x3ce, 0x05);
				outb(0x3cf, write_mode);
			}
			break;

		default:
			if (bg == 0 || (fg&bg) == bg) {
				scrptr = (char *) ((chp)->ch_vstate.v_scrp) +
							TXT2GR(tsp, dest);

				outb(0x3ce, 0x00); /* 0x00 - set/reset reg */
				outb(0x3cf, fg&bg);
				outb(0x3ce, 0x01); /* 0x01 - s/r enable reg */
				outb(0x3cf, (fg^0xf)|(fg&bg));

#if 0
				outb(0x3c4, 2);
				outb(0x3c5, fg);
#endif

				index = 0;
				for (row = 0; row < 16; row++, index += bpr) {
					int index1 = index;
					for (col = 0; col < size; col++, index1++) {
						scrptr[index1] = *bitmap++;
					}
				}
#if 0
				outb(0x3c5, 0xf);
#endif
			} else {
				unsigned char	write_mode;

				scrptr = (char *) ((chp)->ch_vstate.v_scrp) +
							TXT2GR(tsp, dest);

				outb(0x3ce, 0x00); /* 0x00 - set/reset reg */
				outb(0x3cf, bg);
				outb(0x3ce, 0x01); /* 0x01 - s/r enable reg */
				outb(0x3cf, 0xf);

				*scrptr = 0xff;

				outb(0x3ce, 0x00); /* 0x00 - set/reset reg */
				outb(0x3cf, 0x0);
				outb(0x3ce, 0x01); /* 0x01 - s/r enable reg */
				outb(0x3cf, 0x0);

				scrptr = (char *) ((chp)->ch_vstate.v_scrp) +
							TXT2GR(tsp, dest);

				outb(0x3ce, 0x05); /* 0x05 - mode register */
				write_mode = inb(0x3cf); /* write mode 2 */
				outb(0x3cf, (write_mode & 0xfc) | 0x2);

				outb(0x3ce, 0x08); /* 0x08 - bit plane mask */

				*scrptr;
				index = 0;
				for (row = 0; row < 16; row++, index += bpr) {
					int index1 = index;
					for (col = 0; col < size; col++, index1++) {
						outb(0x3cf, *bitmap++);
						scrptr[index1] = fg;
					}
				}

				outb(0x3cf, 0xff);
				outb(0x3ce, 0x05);
				outb(0x3cf, write_mode);
			}
		}
	}
}

/*
 * int
 * gdv_mvword(channel_t *, ushort, ushort, int, char)
 *
 * Calling/Exit State:
 *      - w_rwlock is held in shared mode.
 *      - ch_mutex basic lock is also held.
 *      - This routine is called by the workstation code
 *        for moving the screen contents around.
 *
 * Description:
 *      Move words around in memory.
 */
int
gdv_mvword(channel_t *chp, ushort from, ushort to, int count, char dir)
{
	uchar_t		*scrptr = (uchar_t *) ((chp)->ch_vstate.v_scrp);
	termstate_t	*tsp = &chp->ch_tstate;
	wchar_t		*scrbuf = chp->ch_scrbuf;
	uchar_t		*attrbuf = chp->ch_attrbuf;
	wchar_t		*wsrc, *wdst;
	uchar_t		*asrc, *adst;
	int		cnt;

	if (count <= 0)
		return (0);

	/*
	 * Move characters in text buffer.
	 */
	cnt = count;
	wsrc = scrbuf + from;
	wdst = scrbuf + to;
	if (dir == UP) {
		wsrc = scrbuf + from + (count - 1);
		wdst = scrbuf + to + (count - 1);
		asrc = attrbuf + from + (count - 1);
		adst = attrbuf + to + (count - 1);
		while (--cnt >= 0) {
			*wdst-- = *wsrc--;
			*adst-- = *asrc--;
		}
	} else {
		wsrc = scrbuf + from;
		wdst = scrbuf + to;
		asrc = attrbuf + from;
		adst = attrbuf + to;
		while (--cnt >= 0) {
			*wdst++ = *wsrc++;
			*adst++ = *asrc++;
		}
	}

	/*
	 * If we move complete lines (e.g. scroll), copy a block of memory.
	 */
	if ((from % tsp->t_cols) == 0 &&
	    (to % tsp->t_cols) == 0 &&
	    (count % tsp->t_cols) == 0)
		gdv_xferlines(chp, from, to, count, dir);
	else
		gdv_xferchars(chp, from, to, count, dir);

	return (0);
}

/*
 * STATIC void
 * gdv_xferlines(channel_t *, ushort, ushort, int, char)
 *
 * Calling/Exit State:
 *      - w_rwlock is held in exclusive/shared mode.
 *      - channel for which the screen is saved/restored
 *        is also locked.
 *
 * Description:
 *	Move lines in video memory.
 */
STATIC void
gdv_xferlines(channel_t *chp, ushort from, ushort to, int count, char dir)
{
	if (WS_ISACTIVECHAN(&Kdws, chp)) {
		uchar_t	*scrptr = (uchar_t *) ((chp)->ch_vstate.v_scrp);
		termstate_t *tsp = &chp->ch_tstate;
		uchar_t	val;
		uchar_t *src;
		uchar_t *dst;

		if (dir == UP) {
			from += (count-1);
			to += (count-1);
		}
		src = scrptr+TXT2GR(tsp, from);
		dst = scrptr+TXT2GR(tsp, to);
		count *= 16;

		kdv_disp(0);
		/*
		 * select write mode 1
		 */
		outb(0x3ce, 0x05); /* 0x05 - mode register */
		val = inb(0x3cf);
		outb(0x3cf, (val & 0xfc) | 0x1);

		if (dir == UP) {
			while (--count >= 0)
				*dst-- = *src--;
		}
		else {
			while (--count >= 0)
				*dst++ = *src++;
		}
		/*
		 * restore previous write mode
		 */
		outb(0x3cf, val);
		kdv_disp(1);
	}
}

/*
 * STATIC void
 * gdv_ferchars(channel_t *, ushort, ushort, int, char)
 *
 * Calling/Exit State:
 *      - w_rwlock is held in exclusive/shared mode.
 *      - channel for which the screen is saved/restored
 *        is also locked.
 *
 * Description:
 *      Move lines in video memory.
 */
STATIC void
gdv_xferchars(channel_t *chp, ushort from, ushort to, int count, char dir)
{
	if (WS_ISACTIVECHAN(&Kdws, chp)) {
		uchar_t	*scrptr = (uchar_t *) ((chp)->ch_vstate.v_scrp);
		termstate_t *tsp = &chp->ch_tstate;
		uchar_t	val;
		uchar_t *src;
		uchar_t *dst;
		int	cols = tsp->t_cols;
		int	i;

		if (dir == UP) {
			from += (count-1);
			to += (count-1);
		}
		src = scrptr+TXT2GR(tsp, from);
		dst = scrptr+TXT2GR(tsp, to);

		kdv_disp(0);
		/*
		 * select write mode 1
		 */
		outb(0x3ce, 0x05);
		val = inb(0x3cf);
		outb(0x3cf, (val & 0xfc) | 0x1);

		if (dir == UP) {
			while (--count >= 0) {
				uchar_t *s = src;
				uchar_t *d = dst;

				*d = *s; s += cols; d += cols;
				*d = *s; s += cols; d += cols;
				*d = *s; s += cols; d += cols;
				*d = *s; s += cols; d += cols;
				*d = *s; s += cols; d += cols;
				*d = *s; s += cols; d += cols;
				*d = *s; s += cols; d += cols;
				*d = *s; s += cols; d += cols;
				*d = *s; s += cols; d += cols;
				*d = *s; s += cols; d += cols;
				*d = *s; s += cols; d += cols;
				*d = *s; s += cols; d += cols;
				*d = *s; s += cols; d += cols;
				*d = *s; s += cols; d += cols;
				*d = *s; s += cols; d += cols;
				*d = *s;
				/*
				 * recalculate src and dst pointer when
				 * from and to wraps the screen.
				 */
				if ((from-- % cols) == 0)
					src = scrptr+TXT2GR(tsp, from);
				else
					--src;
				if ((to-- % cols) == 0)
					dst = scrptr+TXT2GR(tsp, to);
				else
					--dst;
			}
		}
		else {
			while (--count >= 0) {
				uchar_t *s = src;
				uchar_t *d = dst;

				*d = *s; s += cols; d += cols;
				*d = *s; s += cols; d += cols;
				*d = *s; s += cols; d += cols;
				*d = *s; s += cols; d += cols;
				*d = *s; s += cols; d += cols;
				*d = *s; s += cols; d += cols;
				*d = *s; s += cols; d += cols;
				*d = *s; s += cols; d += cols;
				*d = *s; s += cols; d += cols;
				*d = *s; s += cols; d += cols;
				*d = *s; s += cols; d += cols;
				*d = *s; s += cols; d += cols;
				*d = *s; s += cols; d += cols;
				*d = *s; s += cols; d += cols;
				*d = *s; s += cols; d += cols;
				*d = *s;

				/*
				 * recalculate src and dst pointer when
				 * from and to wraps the screen.
				 */
				if ((++from % cols) == 0)
					src = scrptr+TXT2GR(tsp, from);
				else
					++src;
				if ((++to % cols) == 0)
					dst = scrptr+TXT2GR(tsp, to);
				else
					++dst;
			}
		}
		/*
		 * restore previous write mode
		 */
		outb(0x3cf, val);
		kdv_disp(1);
	}
}
