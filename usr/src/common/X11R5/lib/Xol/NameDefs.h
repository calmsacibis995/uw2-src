/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)olmisc:NameDefs.h	1.2"

#ifndef _XolNameDefs_h_
#define _XolNameDefs_h_

#if defined(__STDC__)
#define CLASSNAMECONCAT(A,B)	A ## B
#else
#define CLASSNAMECONCAT(A,B)	A/**/B
#endif

#define QUOTE_CLASSNAMECONCAT(X,Y)	CLASSNAMECONCAT(X,Y)

#define AcceptFocus		QUOTE_CLASSNAMECONCAT(ClassName,AcceptFocus)
#define Activate		QUOTE_CLASSNAMECONCAT(ClassName,Activate)
#define ActivateWidget		QUOTE_CLASSNAMECONCAT(ClassName,ActivateWidget)
#define AnalyzeItems		QUOTE_CLASSNAMECONCAT(ClassName,AnalyzeItems)
#define ChangeManaged		QUOTE_CLASSNAMECONCAT(ClassName,ChangeManaged)
#define CheckResources		QUOTE_CLASSNAMECONCAT(ClassName,CheckResources)
#define CheckValues		QUOTE_CLASSNAMECONCAT(ClassName,CheckValues)
#define ClassInitialize		QUOTE_CLASSNAMECONCAT(ClassName,ClassInitialize)
#define ClassPartInitialize	QUOTE_CLASSNAMECONCAT(ClassName,ClassPartInitialize)
#define ComputeTextSize		QUOTE_CLASSNAMECONCAT(ClassName,ComputeTextSize)
#define ConstraintDestroy	QUOTE_CLASSNAMECONCAT(ClassName,ConstraintDestroy)
#define ConstraintInitialize	QUOTE_CLASSNAMECONCAT(ClassName,ConstraintInitialize)
#define ConstraintSetValues	QUOTE_CLASSNAMECONCAT(ClassName,ConstraintSetValues)
#define Destroy			QUOTE_CLASSNAMECONCAT(ClassName,Destroy)
#define DrawBorder		QUOTE_CLASSNAMECONCAT(ClassName,DrawBorder)
#define DrawItem		QUOTE_CLASSNAMECONCAT(ClassName,DrawItem)
#define ExposeProc		QUOTE_CLASSNAMECONCAT(ClassName,ExposeProc)
#define FindWidget		QUOTE_CLASSNAMECONCAT(ClassName,FindWidget)
#define FreeCrayons		QUOTE_CLASSNAMECONCAT(ClassName,FreeCrayons)
#define FreeGCs			QUOTE_CLASSNAMECONCAT(ClassName,FreeGCs)
#define GeometryHandler		QUOTE_CLASSNAMECONCAT(ClassName,GeometryHandler)
#define GeometryManager		QUOTE_CLASSNAMECONCAT(ClassName,GeometryManager)
#define GetCrayons		QUOTE_CLASSNAMECONCAT(ClassName,GetCrayons)
#define GetGCs			QUOTE_CLASSNAMECONCAT(ClassName,GetGCs)
#define GetIndex		QUOTE_CLASSNAMECONCAT(ClassName,GetIndex)
#define GetValuesHook		QUOTE_CLASSNAMECONCAT(ClassName,GetValuesHook)
#define HandleButton		QUOTE_CLASSNAMECONCAT(ClassName,HandleButton)
#define HighlightHandler	QUOTE_CLASSNAMECONCAT(ClassName,HighlightHandler)
#define Initialize		QUOTE_CLASSNAMECONCAT(ClassName,Initialize)
#define InitializeHook		QUOTE_CLASSNAMECONCAT(ClassName,InitializeHook)
#define InsertChild		QUOTE_CLASSNAMECONCAT(ClassName,InsertChild)
#define InsertPosition		QUOTE_CLASSNAMECONCAT(ClassName,InsertPosition)
#define ItemDimensions		QUOTE_CLASSNAMECONCAT(ClassName,ItemDimensions)
#define ItemInitialize		QUOTE_CLASSNAMECONCAT(ClassName,ItemInitialize)
#define ItemLocCursorDims	QUOTE_CLASSNAMECONCAT(ClassName,ItemLocCursorDims)
#define ItemSetValues		QUOTE_CLASSNAMECONCAT(ClassName,ItemSetValues)
#define ItemsTouched		QUOTE_CLASSNAMECONCAT(ClassName,ItemsTouched)
#define Layout			QUOTE_CLASSNAMECONCAT(ClassName,Layout)
#define MemMove			QUOTE_CLASSNAMECONCAT(ClassName,MemMove)
#define ModifyVerificationCB	QUOTE_CLASSNAMECONCAT(ClassName,ModifyVerificationCB)
#define PollMouse		QUOTE_CLASSNAMECONCAT(ClassName,PollMouse)
#define QueryGeom		QUOTE_CLASSNAMECONCAT(ClassName,QueryGeom)
#define QueryGeometry		QUOTE_CLASSNAMECONCAT(ClassName,QueryGeometry)
#define Realize			QUOTE_CLASSNAMECONCAT(ClassName,Realize)
#define Redisplay		QUOTE_CLASSNAMECONCAT(ClassName,Redisplay)
#define RegisterFocus		QUOTE_CLASSNAMECONCAT(ClassName,RegisterFocus)
#define Resize			QUOTE_CLASSNAMECONCAT(ClassName,Resize)
#define SaveString		QUOTE_CLASSNAMECONCAT(ClassName,SaveString)
#define ScrollKeyPress		QUOTE_CLASSNAMECONCAT(ClassName,ScrollKeyPress)
#define SelectDown		QUOTE_CLASSNAMECONCAT(ClassName,SelectDown)
#define SelectUp		QUOTE_CLASSNAMECONCAT(ClassName,SelectUp)
#define SetValues		QUOTE_CLASSNAMECONCAT(ClassName,SetValues)
#define Step			QUOTE_CLASSNAMECONCAT(ClassName,Step)
#define Subtract		QUOTE_CLASSNAMECONCAT(ClassName,Subtract)
#define TimerEvent		QUOTE_CLASSNAMECONCAT(ClassName,TimerEvent)
#define TransparentProc		QUOTE_CLASSNAMECONCAT(ClassName,TransparentProc)
#define TraversalHandler	QUOTE_CLASSNAMECONCAT(ClassName,TraversalHandler)
#define TraverseItems		QUOTE_CLASSNAMECONCAT(ClassName,TraverseItems)
#define WMMessageHandler	QUOTE_CLASSNAMECONCAT(ClassName,WMMessageHandler)
#define WMMsgHandler		QUOTE_CLASSNAMECONCAT(ClassName,WMMsgHandler)

#endif
