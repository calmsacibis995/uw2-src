/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)m1.2libs:Xm/ProtocolsP.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile: ProtocolsP.h,v $ $Revision: 1.12 $ $Date: 93/03/03 16:29:40 $ */
/*
*  (c) Copyright 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#ifndef _XmProtocolsP_h
#define _XmProtocolsP_h

#include <Xm/Protocols.h>
#include <Xm/ExtObjectP.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _XmProtocolClassPart{
    XtPointer	extension;
}XmProtocolClassPart;

typedef struct _XmProtocolClassRec{
    ObjectClassPart	object_class;
    XmExtClassPart	ext_class;
    XmProtocolClassPart	protocol_class;
}XmProtocolClassRec, *XmProtocolObjectClass;

typedef struct _XmProtocolPart{
    XtCallbackRec	pre_hook, post_hook;
    XtCallbackList	callbacks;
    Atom		atom;
    Boolean		active;
} XmProtocolPart, *XmProtocolPartPtr;

typedef struct _XmProtocolRec{
    ObjectPart			object;
    XmExtPart			ext;
    XmProtocolPart		protocol;
}XmProtocolRec, *XmProtocol, **XmProtocolList;

#ifndef XmIsProtocol
#define XmIsProtocol(w) XtIsSubclass(w, xmProtocolObjectClass)
#endif /* XmIsProtocol */

/* Class record constants */

externalref XmProtocolClassRec 	xmProtocolClassRec;
externalref WidgetClass xmProtocolObjectClass;

typedef struct _XmProtocolMgrRec{
    Atom		property;
    XmProtocolList 	protocols;
    Cardinal		num_protocols;
    Cardinal		max_protocols;
}XmProtocolMgrRec, *XmProtocolMgr, **XmProtocolMgrList;


typedef struct _XmAllProtocolsMgrRec{
  XmProtocolMgrList	protocol_mgrs;
  Cardinal		num_protocol_mgrs;
  Cardinal		max_protocol_mgrs;
  Widget		shell;
}XmAllProtocolsMgrRec, *XmAllProtocolsMgr;
    

/********    Private Function Declarations    ********/
#ifdef _NO_PROTO

extern void _XmInstallProtocols() ;

#else

extern void _XmInstallProtocols( 
                        Widget w) ;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmProtocolsP_h */
