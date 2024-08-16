/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)unixtsa:common/cmd/unixtsa/tsaunix/respond.h	1.2"
//****************************************************************************
// respond.h
//****************************************************************************

#define STRSIZE(s) (strlen(s)+1)
#define THREAD_BUFFER_SIZE 0x4000
#define INPUT_OUTPUT_SIZE	0x200
#define NOSEND 0
#define SEND	1

typedef struct
{
	UINT32 size;
	UINT16 fcode;
}
SMSP_HEADER;

typedef struct
{
	UINT32 size;
	void *address;
}
FRAG;


//****************************************************************************
//  Function Prototypes
//****************************************************************************

void ServiceRequest(void);

CCODE LoadReplyBuffer(SMDR_THREAD *thread, void *buffer, UINT32 size);

