/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _TIMERP_H
#define _TIMERP_H
#ident	"@(#)debugger:libol/common/TimerP.h	1.2"

// toolkit specific members of Timer class,
// included by ../../gui.d/common/Timer.h

#define	TIMER_TOOLKIT_SPECIFICS 		\
private:					\
	XtIntervalId	timer_id; 		\
public:				  		\
	void		clear_timer_id() { timer_id = (XtIntervalId)0; }	\
	XtIntervalId	get_timer_id() { return timer_id; }

#endif	// _TIMERP_H
