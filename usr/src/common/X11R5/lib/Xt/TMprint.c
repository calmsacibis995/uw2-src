/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xt:TMprint.c	1.3"
/* $XConsortium: TMprint.c,v 1.9 92/04/03 15:45:50 converse Exp $ */
/*LINTLIBRARY*/

/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#include "IntrinsicI.h"
#include <stdio.h>

typedef struct _TMStringBufRec{
    String	start;
    String	current;
    Cardinal	max;
}TMStringBufRec, *TMStringBuf;


#define STR_THRESHOLD 25
#define STR_INCAMOUNT 100
#define CHECK_STR_OVERFLOW(sb) \
if (sb->current - sb->start > (int)sb->max - STR_THRESHOLD) 	\
{ String old = sb->start; \
  sb->start = XtRealloc(old, (Cardinal)(sb->max += STR_INCAMOUNT)); \
  sb->current = sb->current - old + sb->start; \
}

#define ExpandForChars(sb, nchars ) \
    if (sb->current - sb->start > sb->max - STR_THRESHOLD - nchars) { 		\
	String old = sb->start;					\
	sb->start = XtRealloc(old,				\
	    (Cardinal)(sb->max += STR_INCAMOUNT + nchars));	\
	sb->current = sb->current - old + sb->start;		\
    }

#define ExpandToFit(sb, more) \
{								\
	int l = strlen(more);					\
	ExpandForChars(sb, l);					\
      }

static void PrintModifiers(sb, mask, mod)
    TMStringBuf	sb;
    unsigned long mask, mod;
{
    Boolean notfirst = False;
    CHECK_STR_OVERFLOW(sb);

    if (mask == ~0L && mod == 0) {
	*sb->current++ = '!';
	*sb->current = '\0';
	return;
    }

#define PRINTMOD(modmask,modstring) \
    if (mask & modmask) {		 \
	if (! (mod & modmask)) {	 \
	    *sb->current++ = '~';		 \
	    notfirst = True;		 \
	}				 \
	else if (notfirst)		 \
	    *sb->current++ = ' ';		 \
	else notfirst = True;		 \
	strcpy(sb->current, modstring);		 \
	sb->current += strlen(sb->current);		 \
    }

    PRINTMOD(ShiftMask, "Shift");
    PRINTMOD(ControlMask, "Ctrl");	/* name is not CtrlMask... */
    PRINTMOD(LockMask, "Lock");
    PRINTMOD(Mod1Mask, "Mod1");
    CHECK_STR_OVERFLOW(sb);
    PRINTMOD(Mod2Mask, "Mod2");
    PRINTMOD(Mod3Mask, "Mod3");
    PRINTMOD(Mod4Mask, "Mod4");
    PRINTMOD(Mod5Mask, "Mod5");
    CHECK_STR_OVERFLOW(sb);
    PRINTMOD(Button1Mask, "Button1");
    PRINTMOD(Button2Mask, "Button2");
    PRINTMOD(Button3Mask, "Button3");
    CHECK_STR_OVERFLOW(sb);
    PRINTMOD(Button4Mask, "Button4");
    PRINTMOD(Button5Mask, "Button5");

#undef PRINTMOD
}

static void PrintEventType(sb, event)
    TMStringBuf	sb;
    unsigned long event;
{
    CHECK_STR_OVERFLOW(sb);
    switch (event) {
#define PRINTEVENT(event, name) case event: (void) strcpy(sb->current, name); break;
	PRINTEVENT(KeyPress, "<KeyPress>")
	PRINTEVENT(KeyRelease, "<KeyRelease>")
	PRINTEVENT(ButtonPress, "<ButtonPress>")
	PRINTEVENT(ButtonRelease, "<ButtonRelease>")
	PRINTEVENT(MotionNotify, "<MotionNotify>")
	PRINTEVENT(EnterNotify, "<EnterNotify>")
	PRINTEVENT(LeaveNotify, "<LeaveNotify>")
	PRINTEVENT(FocusIn, "<FocusIn>")
	PRINTEVENT(FocusOut, "<FocusOut>")
	PRINTEVENT(KeymapNotify, "<KeymapNotify>")
	PRINTEVENT(Expose, "<Expose>")
	PRINTEVENT(GraphicsExpose, "<GraphicsExpose>")
	PRINTEVENT(NoExpose, "<NoExpose>")
	PRINTEVENT(VisibilityNotify, "<VisibilityNotify>")
	PRINTEVENT(CreateNotify, "<CreateNotify>")
	PRINTEVENT(DestroyNotify, "<DestroyNotify>")
	PRINTEVENT(UnmapNotify, "<UnmapNotify>")
	PRINTEVENT(MapNotify, "<MapNotify>")
	PRINTEVENT(MapRequest, "<MapRequest>")
	PRINTEVENT(ReparentNotify, "<ReparentNotify>")
	PRINTEVENT(ConfigureNotify, "<ConfigureNotify>")
	PRINTEVENT(ConfigureRequest, "<ConfigureRequest>")
	PRINTEVENT(GravityNotify, "<GravityNotify>")
	PRINTEVENT(ResizeRequest, "<ResizeRequest>")
	PRINTEVENT(CirculateNotify, "<CirculateNotify>")
	PRINTEVENT(CirculateRequest, "<CirculateRequest>")
	PRINTEVENT(PropertyNotify, "<PropertyNotify>")
	PRINTEVENT(SelectionClear, "<SelectionClear>")
	PRINTEVENT(SelectionRequest, "<SelectionRequest>")
	PRINTEVENT(SelectionNotify, "<SelectionNotify>")
	PRINTEVENT(ColormapNotify, "<ColormapNotify>")
	PRINTEVENT(ClientMessage, "<ClientMessage>")
	case _XtEventTimerEventType:
	    (void) strcpy(sb->current,"<EventTimer>");
	    break;
	default:
	    (void) sprintf(sb->current, "<0x%x>", (int) event);
#undef PRINTEVENT
    }
    sb->current += strlen(sb->current);
}

static void PrintCode(sb, mask, code)
    TMStringBuf sb;
    unsigned long mask, code;
{
    CHECK_STR_OVERFLOW(sb);
    if (mask != 0) {
	if (mask != (unsigned long)~0L)
	    (void) sprintf(sb->current, "0x%lx:0x%lx", mask, code);
	else (void) sprintf(sb->current, /*"0x%lx"*/ "%d", code);
	sb->current += strlen(sb->current);
    }
}

static void PrintKeysym(sb, keysym)
    TMStringBuf sb;
    KeySym keysym;
{
    String keysymName;

    if (keysym == 0) return;

    CHECK_STR_OVERFLOW(sb);
    keysymName = XKeysymToString(keysym);
    if (keysymName == NULL)
      PrintCode(sb,(unsigned long)~0L,(unsigned long)keysym);
    else {
      ExpandToFit(sb, keysymName);
      strcpy(sb->current, keysymName);
      sb->current += strlen(sb->current);
    }
}

static void PrintAtom(sb, dpy, atom)
    TMStringBuf sb;
    Display *dpy;
    Atom atom;
{
    String atomName;

    if (atom == 0) return;

    atomName = (dpy ? XGetAtomName(dpy, atom) : NULL);

    if (! atomName)
      PrintCode(sb,(unsigned long)~0L,(unsigned long)atom);
    else {
      ExpandToFit( sb, atomName );
      strcpy(sb->current, atomName);
      sb->current += strlen(sb->current);
      XFree(atomName);
    }
}

static	void PrintLateModifiers(sb, lateModifiers)
    TMStringBuf	sb;
    LateBindingsPtr lateModifiers;
{
    for (; lateModifiers->keysym; lateModifiers++) {
	CHECK_STR_OVERFLOW(sb);
	if (lateModifiers->knot) {
	    *sb->current++ = '~';
	} else {
	    *sb->current++ = ' ';
	}
	strcpy(sb->current, XKeysymToString(lateModifiers->keysym));
	sb->current += strlen(sb->current);
	if (lateModifiers->pair) {
	    *(sb->current -= 2) = '\0';	/* strip "_L" */
	    lateModifiers++;	/* skip _R keysym */
	}
    }
}

static void PrintEvent(sb, typeMatch, modMatch, dpy)
    TMStringBuf sb;
    register TMTypeMatch typeMatch;
    register TMModifierMatch modMatch;
    Display *dpy;
{
    if (modMatch->standard) *sb->current++ = ':';

    PrintModifiers(sb, modMatch->modifierMask, modMatch->modifiers);
    if (modMatch->lateModifiers != NULL)
      PrintLateModifiers(sb, modMatch->lateModifiers);
    PrintEventType(sb, typeMatch->eventType);
    switch (typeMatch->eventType) {
      case KeyPress:
      case KeyRelease:
	PrintKeysym(sb, (KeySym)typeMatch->eventCode);
	break;

      case PropertyNotify:
      case SelectionClear:
      case SelectionRequest:
      case SelectionNotify:
      case ClientMessage:
	PrintAtom(sb, dpy, (Atom)typeMatch->eventCode);
	break;

      default:
	PrintCode(sb, typeMatch->eventCodeMask, typeMatch->eventCode);
    }
}

static void PrintParams(sb, params, num_params)
    TMStringBuf	sb;
    String	*params;
    Cardinal num_params;
{
    register Cardinal i;
    for (i = 0; i<num_params; i++) {
	ExpandToFit( sb, params[i] );
	if (i != 0) {
	    *sb->current++ = ',';
	    *sb->current++ = ' ';
	}
	*sb->current++ = '"';
	strcpy(sb->current, params[i]);
	sb->current += strlen(sb->current);
	*sb->current++ = '"';
    }
    *sb->current = '\0';
}

static void PrintActions(sb, actions, quarkTbl, accelWidget)
    TMStringBuf	sb;
    register ActionPtr actions;
    XrmQuark *quarkTbl;
    Widget   accelWidget;
{
    while (actions != NULL) {
	String proc;

	*sb->current++ = ' ';

	if (accelWidget) {
	    /* accelerator */
	    String name = XtName(accelWidget);
	    int nameLen = strlen(name);
	    ExpandForChars(sb,  nameLen );
	    XtBCopy( name, sb->current, nameLen );
	    sb->current += nameLen;
	    *sb->current++ = '`';
	}
	proc = XrmQuarkToString(quarkTbl[actions->idx]);
	ExpandToFit( sb, proc );
	strcpy(sb->current, proc);
	sb->current += strlen(proc);
	*sb->current++ = '(';
	PrintParams(sb, actions->params, actions->num_params);
	*sb->current++ = ')';
	actions = actions->next;
    }
    *sb->current = '\0';
}

static Boolean LookAheadForCycleOrMulticlick(state, state_return, countP,
					     nextLevelP)
    register StatePtr state;
    StatePtr *state_return;	/* state to print, usually startState */
    int *countP;
    StatePtr *nextLevelP;
{
    int repeatCount = 0;
    StatePtr	startState = state;
    Boolean	isCycle = startState->isCycleEnd;
    TMTypeMatch sTypeMatch = TMGetTypeMatch(startState->typeIndex);
    TMModifierMatch sModMatch = TMGetModifierMatch(startState->modIndex);

    *state_return = startState;

    for (state = state->nextLevel; state != NULL; state = state->nextLevel) {
	TMTypeMatch typeMatch = TMGetTypeMatch(state->typeIndex);
	TMModifierMatch modMatch = TMGetModifierMatch(state->modIndex);

	/* try to pick up the correct state with actions, to be printed */
	/* This is to accommodate <ButtonUp>(2+), for example */
	if (state->isCycleStart)
	    *state_return = state;

	if (state->isCycleEnd) {
	    *countP = repeatCount;
	    return True;
	}
	if ((startState->typeIndex == state->typeIndex) &&
	    (startState->modIndex == state->modIndex)) {
	    repeatCount++;
	    *nextLevelP = state;
	}
	else if (typeMatch->eventType == _XtEventTimerEventType)
	  continue;
	else /* not same event as starting event and not timer */ {
	    unsigned int type = sTypeMatch->eventType;
	    unsigned int t = typeMatch->eventType;
	    if (   (type == ButtonPress	  && t != ButtonRelease)
		|| (type == ButtonRelease && t != ButtonPress)
		|| (type == KeyPress	  && t != KeyRelease)
		|| (type == KeyRelease	  && t != KeyPress)
		|| typeMatch->eventCode != sTypeMatch->eventCode
		|| modMatch->modifiers != sModMatch->modifiers
		|| modMatch->modifierMask != sModMatch->modifierMask
		|| modMatch->lateModifiers != sModMatch->lateModifiers
		|| typeMatch->eventCodeMask != sTypeMatch->eventCodeMask
		|| typeMatch->matchEvent != sTypeMatch->matchEvent
		|| modMatch->standard != sModMatch->standard)
		/* not inverse of starting event, either */
		break;
	}
    }
    *countP = repeatCount;
    return isCycle;
}

static void PrintComplexState(sb, includeRHS, state, stateTree, accelWidget, dpy)
    TMStringBuf	sb;
    Boolean	includeRHS;
    StatePtr 	state;
    TMStateTree stateTree;
    Widget	accelWidget;
    Display 	*dpy;
{
    int 		clickCount = 0;
    Boolean 		cycle;
    StatePtr 		nextLevel = NULL;
    StatePtr		triggerState = NULL;

    /* print the current state */
    if (! state) return;
    
    cycle = LookAheadForCycleOrMulticlick(state, &triggerState, &clickCount,
					  &nextLevel);

    PrintEvent(sb, TMGetTypeMatch(triggerState->typeIndex),
	       TMGetModifierMatch(triggerState->modIndex), dpy);

    if (cycle || clickCount) {
	if (clickCount)
	    sprintf(sb->current, "(%d%s)", clickCount+1, cycle ? "+" : "");
	else
	    (void) strncpy(sb->current, "(+)", 4);
	sb->current += strlen(sb->current);
	if (! state->actions && nextLevel)
	    state = nextLevel;
	while (! state->actions && ! state->isCycleEnd)
	    state = state->nextLevel;	/* should be trigger state */
    }
    
    if (state->actions) {
	if (includeRHS) {
	    CHECK_STR_OVERFLOW(sb);
	    *sb->current++ = ':';
	    PrintActions(sb, 
			 state->actions,
			 ((TMSimpleStateTree)stateTree)->quarkTbl,
			 accelWidget);
	    *sb->current++ = '\n';
	}
    } 
    else {
	if (state->nextLevel && !cycle && !clickCount)
	    *sb->current++ = ',';
	else {
	    /* no actions are attached to this production */
	    *sb->current++ = ':';
	    *sb->current++ = '\n';
	}
    }
    *sb->current = '\0';
    
    /* print succeeding states */
    if (state->nextLevel && !cycle && !clickCount)
	PrintComplexState(sb, includeRHS, state->nextLevel,
			  stateTree, accelWidget, dpy);
}

typedef struct{
    TMShortCard	tIndex;
    TMShortCard	bIndex;
}PrintRec, *Print;

static int FindNextMatch(printData, numPrints, xlations,
			 branchHead, nextLevel, startIndex)
    PrintRec		*printData;
    TMShortCard		numPrints;
    XtTranslations 	xlations;
    TMBranchHead	branchHead;
    StatePtr 		nextLevel;
    TMShortCard		startIndex;
{
    TMShortCard		i;
    TMComplexStateTree 	stateTree;
    StatePtr		currState, candState;
    Boolean		noMatch = True;
    TMBranchHead	prBranchHead;

    for (i = startIndex; noMatch && i < numPrints; i++) {
	stateTree = (TMComplexStateTree)
	  xlations->stateTreeTbl[printData[i].tIndex];
	prBranchHead = 
	  &(stateTree->branchHeadTbl[printData[i].bIndex]);

	if ((prBranchHead->typeIndex == branchHead->typeIndex) &&
	    (prBranchHead->modIndex == branchHead->modIndex)) {
	    if (prBranchHead->isSimple) {
		if (!nextLevel)
		  return i;
	    }
	    else {
		currState = TMComplexBranchHead(stateTree, prBranchHead);
		currState = currState->nextLevel;
		candState = nextLevel;
		for (;
		     ((currState && !currState->isCycleEnd) &&
		      (candState && !candState->isCycleEnd));
		     currState = currState->nextLevel,
		     candState = candState->nextLevel) {
		    if ((currState->typeIndex != candState->typeIndex) ||
			(currState->modIndex != candState->modIndex))
		      break;
		}
		if (candState == currState) {
		    return i;
		}
	    }
	}
    }
    return TM_NO_MATCH;
}
    
static void ProcessLaterMatches(printData,xlations,tIndex,bIndex,numPrintsRtn)
    PrintRec	*printData;
    XtTranslations xlations;
    TMShortCard	tIndex;
    int bIndex;
    TMShortCard	*numPrintsRtn;
{
    TMComplexStateTree 	stateTree; 
    int			i, j;
    TMBranchHead	branchHead, matchBranch = NULL;

    for (i = tIndex; i < (int)xlations->numStateTrees; i++) {
	stateTree = (TMComplexStateTree)xlations->stateTreeTbl[i];    
	if (i == tIndex) {
	    matchBranch = &stateTree->branchHeadTbl[bIndex];
	    j = bIndex+1;
	}
	else j = 0;
	for (branchHead = &stateTree->branchHeadTbl[j];
	     j < (int)stateTree->numBranchHeads;
	     j++, branchHead++) {
	    if ((branchHead->typeIndex == matchBranch->typeIndex) &&
		(branchHead->modIndex == matchBranch->modIndex)) {
		StatePtr state;
		if (!branchHead->isSimple)
		  state = TMComplexBranchHead(stateTree, branchHead);
		else
		  state = NULL;
		if ((!branchHead->isSimple || branchHead->hasActions) &&
		    (FindNextMatch(printData, 
				   *numPrintsRtn,
				   xlations,
				   branchHead,
				   (state ? state->nextLevel : NULL),
				   0) == TM_NO_MATCH)) {
		    printData[*numPrintsRtn].tIndex = i;
		    printData[*numPrintsRtn].bIndex = j;
		    (*numPrintsRtn)++;
		}
	    }
	}
    }
}

static void ProcessStateTree(printData, xlations, tIndex, numPrintsRtn)
    PrintRec	*printData;
    XtTranslations xlations;
    TMShortCard	tIndex;
    TMShortCard	*numPrintsRtn;
{
    TMComplexStateTree stateTree; 
    int			i;
    TMBranchHead	branchHead;

    stateTree = (TMComplexStateTree)xlations->stateTreeTbl[tIndex];    
    
    for (i = 0, branchHead = stateTree->branchHeadTbl;
	 i < (int)stateTree->numBranchHeads;
	 i++, branchHead++) {
	StatePtr state;
	if (!branchHead->isSimple)
	  state = TMComplexBranchHead(stateTree, branchHead);
	else
	  state = NULL;
	if (FindNextMatch(printData, *numPrintsRtn, xlations, branchHead,
			  (state ? state->nextLevel : NULL), 0) 
	    == TM_NO_MATCH) {
	    if (!branchHead->isSimple || branchHead->hasActions) {
		printData[*numPrintsRtn].tIndex = tIndex;
		printData[*numPrintsRtn].bIndex = i;
		(*numPrintsRtn)++;
	    }
	    if (_XtGlobalTM.newMatchSemantics == False)
	      ProcessLaterMatches(printData, 
				  xlations, 
				  tIndex, 
				  i,
				  numPrintsRtn);
	}
    }
}

static void PrintState(sb, tree, branchHead, includeRHS, accelWidget, dpy)
    TMStringBuf	sb;
    TMStateTree	tree;
    TMBranchHead branchHead;
    Boolean	includeRHS;
    Widget	accelWidget;
    Display 	*dpy;
{
    TMComplexStateTree stateTree = (TMComplexStateTree)tree;
    if (branchHead->isSimple) {
	PrintEvent(sb,
		   TMGetTypeMatch(branchHead->typeIndex),
		   TMGetModifierMatch(branchHead->modIndex),
		   dpy);
	if (includeRHS) {
	    ActionRec	actRec;
	    
	    CHECK_STR_OVERFLOW(sb);
	    *sb->current++ = ':';
	    actRec.idx = TMBranchMore(branchHead);
	    actRec.num_params = 0;
	    actRec.params = NULL;
	    actRec.next = NULL;
	    PrintActions(sb, 
			 &actRec,
			 stateTree->quarkTbl,
			 accelWidget);
	    *sb->current++ = '\n';
	}
	else
	  *sb->current++ = ',';
#ifdef TRACE_TM
	if (!branchHead->hasActions)
	  printf(" !! no actions !! ");
#endif
    }
	else { /* it's a complex branchHead */
	    StatePtr state = TMComplexBranchHead(stateTree, branchHead);
	    PrintComplexState(sb,
			      includeRHS,
			      state,
			      tree,
			      accelWidget,
			      (Display *)NULL);
	}
    *sb->current = '\0';
}

#if NeedFunctionPrototypes
String _XtPrintXlations(
    Widget		w,
    XtTranslations 	xlations,
    Widget		accelWidget,
    _XtBoolean		includeRHS
    )
#else
String _XtPrintXlations(w, xlations, accelWidget, includeRHS)
    Widget		w;
    XtTranslations 	xlations;
    Widget		accelWidget;
    Boolean		includeRHS;
#endif
{
    register Cardinal 	i;
#define STACKPRINTSIZE 250
    PrintRec		stackPrints[STACKPRINTSIZE];
    PrintRec		*prints;
    TMStringBufRec	sbRec, *sb = &sbRec;
    TMShortCard		numPrints, maxPrints;
#ifdef TRACE_TM
    TMBindData		bindData = (TMBindData)w->core.tm.proc_table;
    Boolean		hasAccel = (accelWidget ? True : False);
#endif /* TRACE_TM */
    if (xlations == NULL) return NULL;

    sb->current = sb->start = XtMalloc((Cardinal)1000);
    sb->max = 1000;
    maxPrints = 0;
    for (i = 0; i < xlations->numStateTrees; i++)
	maxPrints += 
	  ((TMSimpleStateTree)(xlations->stateTreeTbl[i]))->numBranchHeads;
    prints = (PrintRec *)
      XtStackAlloc(maxPrints * sizeof(PrintRec), stackPrints);

    numPrints = 0;
    for (i = 0; i < xlations->numStateTrees; i++)
      ProcessStateTree(prints, xlations, i, &numPrints);

    for (i = 0; i < numPrints; i++) {
	TMSimpleStateTree stateTree = (TMSimpleStateTree)
	  xlations->stateTreeTbl[prints[i].tIndex];
	TMBranchHead branchHead = 
	  &stateTree->branchHeadTbl[prints[i].bIndex];
#ifdef TRACE_TM	
	TMComplexBindProcs	complexBindProcs;

	if (hasAccel == False) {
	    accelWidget = NULL;
	    if (bindData->simple.isComplex) {
		complexBindProcs = TMGetComplexBindEntry(bindData, 0);
		accelWidget = complexBindProcs[prints[i].tIndex].widget;
	    }
	}
#endif /* TRACE_TM */
	PrintState(sb, (TMStateTree)stateTree, branchHead,
		   includeRHS, accelWidget, XtDisplay(w));
    }
    XtStackFree((XtPointer)prints, (XtPointer)stackPrints);
    return (sb->start);
}


#ifndef NO_MIT_HACKS
/*ARGSUSED*/
void _XtDisplayTranslations(widget, event, params, num_params)
    Widget widget;
    XEvent *event;
    String *params;
    Cardinal *num_params;
{
    String 	xString;

    xString =  _XtPrintXlations(widget,
				widget->core.tm.translations,
				NULL,
				True);
    printf("%s\n",xString);
    XtFree(xString);
}

/*ARGSUSED*/
void _XtDisplayAccelerators(widget, event, params, num_params)
    Widget widget;
    XEvent *event;
    String *params;
    Cardinal *num_params;
{
    String 	xString;

    
    xString =  _XtPrintXlations(widget, 
				widget->core.accelerators,
				NULL,
				True);
    printf("%s\n",xString);
    XtFree(xString);
}

/*ARGSUSED*/
void _XtDisplayInstalledAccelerators(widget, event, params, num_params)
    Widget widget;
    XEvent *event;
    String *params;
    Cardinal *num_params;
{
    Widget eventWidget
	= XtWindowToWidget(event->xany.display, event->xany.window);
    register Cardinal 	i;
    TMStringBufRec	sbRec, *sb = &sbRec;
    XtTranslations	xlations;
#define STACKPRINTSIZE 250
    PrintRec		stackPrints[STACKPRINTSIZE];
    PrintRec		*prints;
    TMShortCard		numPrints, maxPrints;
    TMBindData	bindData = (TMBindData) eventWidget->core.tm.proc_table;
    TMComplexBindProcs	complexBindProcs;

    if ((eventWidget == NULL) ||
	((xlations = eventWidget->core.tm.translations) == NULL) ||
	(bindData->simple.isComplex == False))
      return;

    sb->current = sb->start = XtMalloc((Cardinal)1000);
    sb->start[0] = '\0';
    sb->max = 1000;
    maxPrints = 0;
    for (i = 0; i < xlations->numStateTrees; i++)
	maxPrints += 
	  ((TMSimpleStateTree)xlations->stateTreeTbl[i])->numBranchHeads;
    prints = (PrintRec *)
      XtStackAlloc(maxPrints * sizeof(PrintRec), stackPrints);

    numPrints = 0;

    complexBindProcs = TMGetComplexBindEntry(bindData, 0);
    for (i = 0;
	 i < xlations->numStateTrees;
	 i++, complexBindProcs++) {
	if (complexBindProcs->widget)
	  {
	      ProcessStateTree(prints, xlations, i, &numPrints);
	  }
    }
    for (i = 0; i < numPrints; i++) {
	TMSimpleStateTree stateTree = (TMSimpleStateTree)
	  xlations->stateTreeTbl[prints[i].tIndex];
	TMBranchHead branchHead = 
	  &stateTree->branchHeadTbl[prints[i].bIndex];

	complexBindProcs = TMGetComplexBindEntry(bindData, 0);
	
	PrintState(sb, (TMStateTree)stateTree, branchHead, True, 
		   complexBindProcs[prints[i].tIndex].widget, 
		   XtDisplay(widget));
    }
    XtStackFree((XtPointer)prints, (XtPointer)stackPrints);
    printf("%s\n", sb->start);
    XtFree(sb->start);
}
#endif /*NO_MIT_HACKS*/

String _XtPrintActions(actions, quarkTbl)
    register ActionRec *actions;
    XrmQuark		*quarkTbl;
{
    TMStringBufRec	sbRec, *sb = &sbRec;
 
    sb->max = 1000;
    sb->current = sb->start = XtMalloc((Cardinal)1000);
    PrintActions(sb, 
		 actions,
		 quarkTbl,
		 (Widget)NULL);
    return sb->start;
}

String _XtPrintState(stateTree, branchHead)
    TMStateTree		stateTree;
    TMBranchHead	branchHead;
{
    TMStringBufRec	sbRec, *sb = &sbRec;

    sb->current = sb->start = XtMalloc((Cardinal)1000);
    sb->max = 1000;
    PrintState(sb, stateTree, branchHead,
	       True, (Widget)NULL, (Display *)NULL);
    return sb->start;
}


String _XtPrintEventSeq(eventSeq, dpy)
    register EventSeqPtr eventSeq;
    Display *dpy;
{
    TMStringBufRec	sbRec, *sb = &sbRec;
    TMTypeMatch		typeMatch;
    TMModifierMatch	modMatch;
#define MAXSEQS 100
    EventSeqPtr		eventSeqs[MAXSEQS];
    TMShortCard		i, j;
    Boolean		cycle = False;

    sb->current = sb->start = XtMalloc((Cardinal)1000);
    sb->max = 1000;
    for (i = 0;
	 i < MAXSEQS && eventSeq != NULL && !cycle;
	 eventSeq = eventSeq->next, i++) 
      {
	  eventSeqs[i] = eventSeq;
	  for (j = 0; j < i && !cycle; j++)
	    if (eventSeqs[j] == eventSeq)
	      cycle = True;
      }
    for (j = 0; j < i; j++) {
	typeMatch = 
	  TMGetTypeMatch(_XtGetTypeIndex(&eventSeqs[j]->event));
	modMatch = 
	  TMGetModifierMatch(_XtGetModifierIndex(&eventSeqs[j]->event));
	PrintEvent(sb, typeMatch, modMatch, dpy);
	if (j < i)
	  *sb->current++ = ',';
    }
    return sb->start;
}

