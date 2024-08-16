/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)winxksh:libwin/keys.h	1.2"
#ifndef	_KEYS_H
#define	_KEYS_H

#include	<curses.h>

#define	LFT_ARROW	KEY_LEFT
#define	RGT_ARROW	KEY_RIGHT
#define	UP_ARROW	KEY_UP
#define	DN_ARROW	KEY_DOWN
#define	HOME		KEY_HOME
#define	END		KEY_END
#define	BTAB		KEY_BTAB
#define	BS		0x0008
#define	TAB		0x0009
#define	ENTER		0x000a
#define DEL		0x007f
#define	F1		KEY_F(1)
#define	F2		KEY_F(2)
#define	F3		KEY_F(3)
#define	F4		KEY_F(4)
#define	F5		KEY_F(5)
#define	F6		KEY_F(6)
#define	F7		KEY_F(7)
#define	F8		KEY_F(8)
#define	F9		KEY_F(9)
#define	F10		KEY_F(10)
#define	F11		KEY_F(11)
#define	F12		KEY_F(12)
#define	ESC		0x001b
#define	CTLB		0x0002
#define	CTLF		0x0006
#define	PGDN		KEY_NPAGE
#define	PGUP		KEY_PPAGE


#define	KEY_INVALID	-1
#define KEY_HOTKEY -2


#include	"syskeys.h"


typedef struct {
	int num;
	int (*func)();
	void *parm;
} HOT_KEY;

#define	MAX_HOT_KEYS	20


typedef struct {
	int	size;
	struct {
		int src;
		int dst;
	} map[64];
} KEY_MAP;

void set_hotkey(int num, int (*func)(void *), void *parm);
int getkey(void);
int translate(int key);
void set_user_help_key(int key);
void set_user_done_key(int key);
void set_user_choice_key(int key);

#endif _KEYS_H
