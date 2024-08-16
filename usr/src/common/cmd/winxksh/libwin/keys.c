/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)winxksh:libwin/keys.c	1.4"

#include	<stdlib.h>
#include	<string.h>
#include	<curses.h>
#include	"win.h"
#include	"form.h"
#include	"fld.h"
#include	"keys.h"
#include	"nav.h"

static unsigned user_help_key = KEY_F(1);
static unsigned user_done_key = KEY_F(10);
static unsigned user_choice_key = KEY_F(2);

extern int Wcurrent;

static int hotkey(int num);


extern FWINDOW *windows[];


KEY_MAP def_keymap = {
			19,
			{
				{ KEY_F(1),  KEY_HELP },
				{ KEY_F(2),  KEY_CHOICES },
				{ KEY_F(10), KEY_DONE },
				{ ENTER,     KEY_ENTR },
				{ TAB,	     KEY_TAB },
				{ BTAB,	     KEY_BTAB },
				{ KEY_LEFT,  KEY_LEFT },
				{ KEY_RIGHT, KEY_RIGHT },
				{ KEY_UP,    KEY_UP },
				{ KEY_DOWN,  KEY_DOWN },
				{ KEY_HOME,  KEY_HOME },
				{ KEY_END,   KEY_END },
				{ BS,	     KEY_ERASE },
				{ ESC,	     KEY_INVALID },
				{ DEL,	     KEY_INVALID },
				{ CTLF,	     KEY_NPAGE },
				{ KEY_NPAGE, KEY_NPAGE },
				{ CTLB,	     KEY_PPAGE },
				{ KEY_PPAGE, KEY_PPAGE },
			}
};


void set_hotkey(int num, int (*func)(void *), void *parm)
{
	int i;
	HOT_KEY *hotkeys;

	hotkeys = windows[Wcurrent]->hotkeys;

	for (i = 0; i < windows[Wcurrent]->hotkey_count; i++)
		if (hotkeys[i].num == num)
			break;
	if (i == windows[Wcurrent]->hotkey_count) {
		windows[Wcurrent]->hotkey_count++;
		hotkeys = realloc(hotkeys,windows[Wcurrent]->hotkey_count*sizeof(HOT_KEY));
		if (hotkeys == NULL)
			return;		/* Error, cannot assign new hotkey */
	}
	hotkeys[i].num = num;
	hotkeys[i].func = func;
	hotkeys[i].parm = parm;
	windows[Wcurrent]->hotkeys = hotkeys;
}

static int hotkey(int num)
{
	int i;

	for (i = 0; i < windows[Wcurrent]->hotkey_count; i++)
		if (windows[Wcurrent]->hotkeys[i].num == num) {
			windows[Wcurrent]->hotkeys[i].func(windows[Wcurrent]->hotkeys[i].parm);
			return(1);
		}
	return(0);
}

int getkey(void)
{
	unsigned x;
	static const char *numbers = "1234567890";
	char *s;

	x = getch();
	if (!x) {
		x = (getch())<<8;
	}
	if (x == CTLF) {
		x = getch();
		if ((s=strchr(numbers,(char)x)))
			x = KEY_F(s-numbers+1);
	}
	if (hotkey(x))
		return KEY_HOTKEY;
	return x;
}

/*
 * Translate keyboard input into internal format for use by other functions.
 */
int translate(int key)
{
	int i;

	if (windows[Wcurrent]->keymap.size == 0)
		return key;	/* No keyboard mapping */

	/* Check the current keymap, if a match is found return it */
	for(i = 0; i < windows[Wcurrent]->keymap.size; i++) {
		if (key == windows[Wcurrent]->keymap.map[i].src)
			return windows[Wcurrent]->keymap.map[i].dst;
	}
	/* Test for invalid special key */
	if (key >= KEY_MIN && key <= KEY_MAX)
		return KEY_INVALID;	/* return the error */

	/* otherwise return the character */
	return key;
}

void set_user_help_key(int key)
{
	user_help_key = key;
}

void set_user_done_key(int key)
{
	user_done_key = key;
}

void set_user_choice_key(int key)
{
	user_choice_key = key;
}
