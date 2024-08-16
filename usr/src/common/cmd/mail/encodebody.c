/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/encodebody.c	1.1.1.3"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)encodebody.c	1.3 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	encode_message_body - change encoding of a message from raw to base64 or quoted-printable

    SYNOPSIS
	void encode_message_body(Msg *pmsg, const char *encoding_type)

    DESCRIPTION
	Read from one temp file to another, changing the encoding. The new encoding_type
	must be either "base64" or "quoted-printable".
*/

void encode_message_body(pmsg, encoding_type)
Msg *pmsg;
const char *encoding_type;
{
    Tmpfile *ptmpfile = new_Tmpfile();
    mktmp(ptmpfile);
    rewind(pmsg->tmpfile->tmpf);
    if (strcmp(encoding_type, "base64") == 0)
	pmsg->msgsize = encode_file_to_base64(pmsg->tmpfile->tmpf, ptmpfile->tmpf, 0);
    else
	pmsg->msgsize = encode_file_to_quoted_printable(pmsg->tmpfile->tmpf, ptmpfile->tmpf);
    del_Tmpfile(pmsg->tmpfile);
    pmsg->tmpfile = ptmpfile;
    pmsg->binflag = C_Text;
}
