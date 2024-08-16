/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)winxksh:libwin/eval.c	1.1"
stub()
{
	beep();
}

win_eval(x, y)
unsigned long x, y;
{
	extern unsigned char Fld_changed;

	if (Fld_changed)
		env_set("FLD_CHANGED=1");
	else
		env_set("FLD_CHANGED=0");
	return(ksh_eval(y));
}

help_eval(x, y)
unsigned long x, y;
{
	char buf[10];

	sprintf(buf, "page=%d", x);
	env_set(buf);
	return(ksh_eval(y));
}
