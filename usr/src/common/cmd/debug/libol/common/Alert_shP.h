/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_ALERT_SHP
#define	_ALERT_SHP
#ident	"@(#)debugger:libol/common/Alert_shP.h	1.2"

// toolkit specific members of the Alert_shell class
// included by ../../gui.d/common/Alert_sh.h

class Button_data;
class Alert_shell;

extern Alert_shell *old_alert_shell;

#define ALERT_SHELL_TOOLKIT_SPECIFICS	\
private:				\
	Button_data	*button_data;	\
public:					\
	void		raise();

#endif // _ALERT_SHP
