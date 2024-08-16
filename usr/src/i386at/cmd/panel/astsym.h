/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)front_panel:astsym.h	1.1"
	define(`_A_MASKABLEINTEOI',`ifelse($#,0,`200',`200'($@))')
	define(`_A_REGSETLOCALINTMASK',`ifelse($#,0,`288',`288'($@))')
	define(`_A_SPI',`ifelse($#,0,`24',`24'($@))')
	define(`_A_IPI',`ifelse($#,0,`26',`26'($@))')
