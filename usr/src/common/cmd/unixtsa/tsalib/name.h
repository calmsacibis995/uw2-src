/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)unixtsa:common/cmd/unixtsa/tsalib/name.h	1.1"

/* Handle tags */
#define VALID		0x2AAAAAAAL
#define INVALID 	0x15555555L

#define HANDLE ((NWSM_NAME HUGE *)*handle)
#define DATA_SET_PTR ((NWSM_DATA_SET_NAME_LIST HUGE *)(HANDLE->buffer))

/*	number of bytes for nameSpaceType, type, count and nameLength */
#define STRUCT_SIZE 11

typedef struct  /* handle structure */
{
	UINT32    valid;   /* valid is 0x2AAAAAAA , invalid is 0x15555555 */
	BUFFERPTR buffer;  /* pointer to a selection or a data set name list */
	BUFFERPTR bufferEnd;
	BUFFERPTR ptr;     /* points to the next element in the list */
	UINT16	  count;   /* contains the number of elements in the list */
	UINT16	  index;   /* contains the index to the element to be read */
} NWSM_NAME;

