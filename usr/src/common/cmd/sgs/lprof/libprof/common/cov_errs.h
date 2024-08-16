/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)lprof:libprof/common/cov_errs.h	1.2.1.2"
#define COV001	"uxcds:1298:dynamic structure allocation failed"
#define COV002	"uxcds:1299:covfile objectfile entry read failed"


#define	COV300	"file pointer manipulation attempt failed"
#define COV301	"new covfile write failed"
#define COV302  "uxcds:1302:_CAcomp_covf: covfile header write failed\n"
#define COV303  "covfile header read failed"
#define COV304  "uxcds:1304:_CAobj_entry: covfile object file table full\n"
#define COV305  "uxcds:1305:_CAobj_entry: unable to open object file\n"



#define COV350	"uxcds:1306:Failure returned by _CAhead_compare."
#define COV351	"uxcds:1307:Invalid code returned by CAobj_match."
#define COV352	"uxcds:1308:Invalid code returned by _CAobj_compare."
#define	COV353	"uxcds:1309:Bad code returned by CAor."
#define COV354	"Bad code returned by CAadd_olist."
#define COV355	"uxcds:1311:Bad code returned by CAadd_flist."
#define COV356	"Bad value returned by _CAtraverse."
#define COV357	"uxcds:1313:Bad value returned by _CAcomp_covf."
#define COV358	"Bad value returned by _CAclose_covf."
#define COV359	"uxcds:1315:Bad code returned by _CAfree_list."
#define COV360	"uxcds:1316:Bad value returned by CAobj_match/_CAobj_compare."
