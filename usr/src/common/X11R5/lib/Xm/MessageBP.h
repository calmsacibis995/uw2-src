/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)m1.2libs:Xm/MessageBP.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.3
*/ 
/*   $RCSfile: MessageBP.h,v $ $Revision: 1.19 $ $Date: 93/12/10 11:52:19 $ */
/*
*  (c) Copyright 1989, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#ifndef _XmessageP_h
#define _XmessageP_h

#include <Xm/BulletinBP.h>
#include <Xm/MessageB.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Constraint part record for MessageBox widget */

typedef struct _XmMessageBoxConstraintPart
{
   char unused;
} XmMessageBoxConstraintPart, * XmMessageBoxConstraint;


/*  New fields for the MessageBox widget class record  */

typedef struct
{
   XtPointer extension;   /* Pointer to extension record */
} XmMessageBoxClassPart;


/* Full class record declaration */

typedef struct _XmMessageBoxClassRec
{
   CoreClassPart             core_class;
   CompositeClassPart        composite_class;
   ConstraintClassPart       constraint_class;
   XmManagerClassPart        manager_class;
   XmBulletinBoardClassPart  bulletin_board_class;
   XmMessageBoxClassPart     message_box_class;
} XmMessageBoxClassRec;

externalref XmMessageBoxClassRec xmMessageBoxClassRec;


/* New fields for the MessageBox widget record */

typedef struct
{
    unsigned char           dialog_type;
    unsigned char           default_type;
    Boolean		    internal_pixmap;
    Boolean                 minimize_buttons;

    unsigned char           message_alignment;
    XmString                message_string;
    Widget                  message_wid;

    Pixmap                  symbol_pixmap;
    Widget                  symbol_wid;

    XmString                ok_label_string;
    XtCallbackList          ok_callback;
    Widget                  ok_button;

    XmString                cancel_label_string;
    XtCallbackList          cancel_callback;

    XmString                help_label_string;
    Widget                  help_button;

    Widget                  separator;

} XmMessageBoxPart;


/****************************************************************
 *
 * Full instance record declaration
 *
 ****************************************************************/

typedef struct _XmMessageBoxRec
{
    CorePart	         core;
    CompositePart        composite;
    ConstraintPart       constraint;
    XmManagerPart        manager;
    XmBulletinBoardPart  bulletin_board; 
    XmMessageBoxPart     message_box;
} XmMessageBoxRec;


/********    Private Function Declarations    ********/
#ifdef _NO_PROTO

extern XmGeoMatrix _XmMessageBoxGeoMatrixCreate() ;
extern Boolean _XmMessageBoxNoGeoRequest() ;

#else

extern XmGeoMatrix _XmMessageBoxGeoMatrixCreate( 
                        Widget wid,
                        Widget instigator,
                        XtWidgetGeometry *desired) ;
extern Boolean _XmMessageBoxNoGeoRequest( 
                        XmGeoMatrix geoSpec) ;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmMessage_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
