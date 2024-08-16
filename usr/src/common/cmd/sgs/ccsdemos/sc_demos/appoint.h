/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)ccsdemos:sc_demos/appoint.h	1.1" */

#include <Time.h>
#include <String.h>

// an Appointment consists of a time and a description
struct Appointment {
    Time time;
    String desc;
};

inline int operator<(const Appointment& a, const Appointment& b) {
    return a.time < b.time;
}
