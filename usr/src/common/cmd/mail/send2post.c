/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/send2post.c	1.2.2.4"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)send2post.c	1.3 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	send2post - do mail post processing

    SYNOPSIS
	void send2post(Msg *pmsg, int wherefrom, t_surrtype surr_type);

    DESCRIPTION
	Loop through the surrogate file looking for postprocessing
	commands of the appropriate type (t_postprocess_cmd or t_error_cmd).
	The commands are passed on to send2d_p() for processing.
	wherefrom is either RECIPS_SUCCESS or RECIPS_FAILURE.
*/

void send2post(pmsg, wherefrom, surr_type)
Msg		*pmsg;
int		wherefrom;
t_surrtype	surr_type;
{
    static const char pn[] = "send2post";
    int surr_num;

    Tout(pn, "Postprocessing %s\n",
        (surr_type == t_postprocess_cmd) ? "Successes" : "Errors");

    /* Move all the pertinent messages */
    /* to the first post-processing command. */
    for (surr_num = 0; surr_num < surr_len; surr_num++)
	{
	Dout(pn, 5, "surr_num=%d\n", surr_num);
	if (surrfile[surr_num].surr_type == surr_type)
	    {
	    Dout(pn, 5, "found postprocessor at %d\n", surr_num);
	    send2move(pmsg, wherefrom, surr_num);
	    break;
	    }
	}

    /* Now postprocess the messages. */
    for ( ; surr_num < surr_len; surr_num++)
	{
	Dout(pn, 5, "surr_num=%d\n", surr_num);
	if (surrfile[surr_num].surr_type == surr_type)
	    send2d_p(pmsg, surr_num);
	else
	    send2move(pmsg, surr_num, surr_num + 1);
	}

    /* Move them all back since they're now all at RECIPS_LOCAL. */
    send2move(pmsg, surr_len + RECIPS_LOCAL, wherefrom);
}
