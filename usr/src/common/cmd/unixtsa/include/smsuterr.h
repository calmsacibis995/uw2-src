/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)unixtsa:common/cmd/unixtsa/include/smsuterr.h	1.1"

#if !defined(_SMSUTERR_H_INCLUDED)
#define _SMSUTERR_H_INCLUDED

#define NWSMUT_ERROR_CODE(err)		   (0xFFFB0000L | err)
#define NWSMUT_BEGIN_ERROR_CODES		NWSMUT_ERROR_CODE(0xFFFF)

#define NWSMUT_INVALID_HANDLE			NWSMUT_ERROR_CODE(0xFFFF) /* Handle is tagged INVALID or ptr is NULL */
#define NWSMUT_INVALID_OFFSET_TO_END	NWSMUT_ERROR_CODE(0xFFFE) /* The offset to end field did not offset to the correct end field */
#define NWSMUT_INVALID_PARAMETER		NWSMUT_ERROR_CODE(0xFFFD) /* One or more of the paremeters is NULL or invalid */
#define NWSMUT_NO_MORE_NAMES			NWSMUT_ERROR_CODE(0xFFFC) /* No more entries in list or nameSpace type does not exist */
#define NWSMUT_OUT_OF_MEMORY			NWSMUT_ERROR_CODE(0xFFFB) /* Server out of memory or memory allocation failed */
#define NWSMUT_BUFFER_OVERFLOW			NWSMUT_ERROR_CODE(0xFFFA) /* Field identifier buffer overflow */
#define NWSMUT_BUFFER_UNDERFLOW			NWSMUT_ERROR_CODE(0xFFF0) /* Field identifier buffer underflow */
#define NWSMUT_INVALID_FIELD_ID			NWSMUT_ERROR_CODE(0xFFF9) /* Invalid field identifier encountered */
#define NWSMUT_INVALID_MESSAGE_NUMBER	NWSMUT_ERROR_CODE(0xFFF8) /* Invalid message number encountered */

#define NWSMUT_END_ERROR_CODES			NWSMUT_ERROR_CODE(0xFFF8) 
#endif

