/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamintf:common/cmd/oamintf/intf_include/inst_err.h	1.3.5.2"
#ident  "$Header: inst_err.h 2.0 91/07/12 $"

#define USAGE		0
#define ENV_MOD		1
#define EMP_EFILE	2
#define INV_PATH	3
#define INV_ENTRY	4
#define INV_EXP		5
#define INV_EXPL	6
#define INV_FILE	7
#define EFILE_OPN	8
#define INSTALLF	9
#define FILE_OPN 	10
#define FILE_RD		11
#define FILE_WR		12
#define D_CREAT		13
#define FILE_CLS	14
#define OVERWRITE	15
#define RENAME		16
#define WRITE_ERR	17
#define EXPR_COMM	18
#define NO_HDR		19

#define HAS_STR		INV_PATH	/* string argument above this pos */
#define HAS_TWO		OVERWRITE	/* two string arguments */

#define WARN		0
#define ERR		1

#define ERR_RET		1
#define SUCCESS_RET	0
