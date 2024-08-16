/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)shlib:OlMinStr.h	1.10"
#endif

#ifdef OL_USE_DEFINE
#ifndef __Ol_OlMinStr_h__
#define __Ol_OlMinStr_h__

/*
 * Define __OlStrings_h_ here to override the real OlStrings.h.
 * This is needed for C files that have "Too Much Defining" problems.
 */
#define	__OlStrings_h_

#define	XtCClientData	       "ClientData"
#define	XtCDefault	       "Default"
#define	XtCDelete	       "Delete"
#define	XtCDim		       "Dim"
#define	XtCEditOff	       "EditOff"
#define	XtCEditOn	       "EditOn"
#define	XtCFont		       "Font"
#define	XtCFontColor	       "FontColor"
#define	XtCGravity	       "Gravity"
#define	XtCHPad		       "HPad"
#define	XtCHelpInfo	       "HelpInfo"
#define	XtCInsert	       "Insert"
#define	XtCItemFields	       "ItemFields"
#define	XtCItemGravity	       "ItemGravity"
#define	XtCItemMaxHeight       "ItemMaxHeight"
#define	XtCItemMaxWidth	       "ItemMaxWidth"
#define	XtCItemMinHeight       "ItemMinHeight"
#define	XtCItemMinWidth	       "ItemMinWidth"
#define	XtCItems	       "Items"
#define	XtCLabel	       "Label"
#define	XtCLabelImage	       "LabelImage"
#define	XtCLabelJustify	       "LabelJustify"
#define	XtCLabelPixmap	       "LabelPixmap"
#define	XtCLabelTile	       "LabelTile"
#define	XtCLayoutHeight	       "LayoutHeight"
#define	XtCLayoutType	       "LayoutType"
#define	XtCLayoutWidth	       "LayoutWidth"
#define	XtCManaged	       "Managed"
#define	XtCMeasure	       "Measure"
#define	XtCMove		       "Move"
#define	XtCNoneSet	       "NoneSet"
#define	XtCNumItemFields	"NumItemFields"
#define	XtCNumItems		"NumItems"
#define	XtCProportionLength    "ProportionLength"
#define	XtCRecomputeWidth      "RecomputeWidth"
#define	XtCSameHeight	       "SameHeight"
#define	XtCSameWidth	       "SameWidth"
#define	XtCSelectProc	       "SelectProc"
#define	XtCSelectable	       "Selectable"
#define	XtCSet		       "Set"
#define	XtCSliderMax	       "SliderMax"
#define	XtCSliderMoved	       "SliderMoved"
#define	XtCSliderValue	       "SliderValue"
#define	XtCTouch	       "Touch"
#define	XtCUnselectProc	       "UnselectProc"
#define	XtCUpdateView	       "UpdateView"
#define	XtCUserData	       "UserData"
#define	XtCUserDeleteItems     "UserDeleteItems"
#define	XtCUserMakeCurrent     "UserMakeCurrent"
#define	XtCVPad		       "VPad"
#define	XtCView		       "View"
#define	XtCViewHeight	       "ViewHeight"
#define	XtNclientData	       "clientData"
#define	XtNdefault	       "default"
#define	XtNdelete	       "delete"
#define	XtNdim		       "dim"
#define	XtNeditOff	       "editOff"
#define	XtNeditOn	       "editOn"
#define	XtNfont		       "font"
#define	XtNfontColor	       "fontColor"
#define	XtNgravity	       "gravity"
#define	XtNhPad		       "hPad"
#define	XtNhelpInfo	       "helpInfo"
#define	XtNinsert	       "insert"
#define	XtNitemFields	       "itemFields"
#define	XtNitemGravity	       "itemGravity"
#define	XtNitemMaxHeight       "itemMaxHeight"
#define	XtNitemMaxWidth	       "itemMaxWidth"
#define	XtNitemMinHeight       "itemMinHeight"
#define	XtNitemMinWidth	       "itemMinWidth"
#define	XtNitems	       "items"
#define	XtNitemsTouched	       "itemsTouched"
#define	XtNlabel	       "label"
#define	XtNlabelImage	       "labelImage"
#define	XtNlabelJustify	       "labelJustify"
#define	XtNlabelPixmap	       "labelPixmap"
#define	XtNlabelTile	       "labelTile"
#define	XtNlayoutHeight	       "layoutHeight"
#define	XtNlayoutType	       "layoutType"
#define	XtNlayoutWidth	       "layoutWidth"
#define	XtNmanaged	       "managed"
#define	XtNmeasure	       "measure"
#define	XtNmove		       "move"
#define	XtNnoneSet	       "noneSet"
#define	XtNnumItemFields       "numItemFields"
#define	XtNnumItems	       "numItems"
#define XtNposition	       "position"
#define	XtNproportionLength    "proportionLength"
#define	XtNrecomputeWidth      "recomputeWidth"
#define	XtNsameHeight	       "sameHeight"
#define	XtNsameWidth	       "sameWidth"
#define	XtNselectProc	       "selectProc"
#define	XtNselectable	       "selectable"
#define	XtNset		       "set"
#define	XtNsliderMax	       "sliderMax"
#define	XtNsliderMoved	       "sliderMoved"
#define	XtNsliderValue	       "sliderValue"
#define	XtNtouch	       "touch"
#define	XtNunselectProc	       "unselectProc"
#define	XtNupdateView	       "updateView"
#define	XtNuserData	       "userData"
#define	XtNuserDeleteItems     "userDeleteItems"
#define	XtNuserMakeCurrent     "userMakeCurrent"
#define	XtNvPad		       "vPad"
#define	XtNview		       "view"
#define	XtNviewHeight	       "viewHeight"
#define XtCCallbackProc	       "CallbackProc"
#define XtCToken	       "Token"
#define XtNcontainerType       "containerType"
#define XtNitemState           "itemState"
#define XtNrepeatRate          "repeatRate"
#define XtRCallbackProc		"CallbackProc"
#define XtRCardinal		"Cardinal"
#define XtRGravity		"Gravity"
#define XtROlDefine		"OlDefine"
#define XtNbuttonType		"buttonType"
#define XtNpreview		"preview"
#define XtNbusy			"busy"
#define XtNselectBtn		"selectBtn"
#define XtNmenuBtn		"menuBtn"
#define XtNmenuDefaultBtn	"menuDefaultBtn"
#define XtNmenu			"menu"
#define XtNmenuName		"menuName"
#define XtNmenuPane		"menuPane"
#define XtNmenuPositioner	"menuPositioner"
#define XtNpushpin		"pushpin"
#define XtNpushpinDefault	"pushpinDefault"
#define XtNshellBehavior	"shellBehavior"
#define XtNtrigger		"trigger"
#define XtCTrigger		"Trigger"
#define XtCButtonType		"ButtonType"
#define XtCPreview		"Preview"
#define XtCShellBehavior	"ShellBehavior"
#define XtNpaneName		"paneName"
#define XtNmenuAugment		"menuAugment"
#define XtNrevertButton		"revertButton"
#define XtNcenter		"center"
#define XtNpostSelect		"postSelect"
#define XtNsameSize		"sameSize"
#define XtCItemsTouched		"ItemsTouched"
#define XtNtraversalManager	"traversalManager"
#define XtCTraversalManager	"TraversalManager"
#define XtNrecomputeSize	"recomputeSize"
#define XtCRecomputeSize	"RecomputeSize"
#define XtNdefaultData		"defaultData"
#define XtCDefaultData		"DefaultData"
#define XtNresetDefault		"resetDefault"
#define XtCResetDefault		"ResetDefault"
#define XtNparentReset		"parentReset"
#define XtNunselect		"unselect"
#define XtNselect		"select"
#define XtNreferenceWidget	"referenceWidget"
#define XtCReferenceWidget	"ReferenceWidget"
#define XtRWidget		"Widget"	/* Must remove for R4 */
#define XtNtraversalOn		"traversalOn"
#define XtCTraversalOn		"TraversalOn"
#define XtNunitType		"unitType"
#define XtCUnitType		"UnitType"
#define XtNsliderMin		"sliderMin"
#define XtCSliderMin		"SliderMin"
#define XtNgranularity		"granularity"
#define XtCGranularity		"Granularity"
#define XtNshowPage		"showPage"
#define XtCShowPage		"ShowPage"
#define XtNcurrentPage		"currentPage"
#define XtCCurrentPage		"CurrentPage"
#define XtCRepeatRate		"RepeatRate"
#define XtNinitialDelay		"initialDelay"
#define XtCInitialDelay		"InitialDelay"
#define XtNscale		"scale"
#define XtCScale		"Scale"
#define XtNpointerWarping	"pointerWarping"
#define XtCPointerWarping	"PointerWarping"

/* TextPane.c */
#define XtNdisplayPosition	"displayPosition"
#define XtNcursorPosition	"cursorPosition"
#define XtNleftMargin		"leftMargin"
#define XtNrightMargin		"rightMargin"
#define XtNtopMargin		"topMargin"
#define XtNbottomMargin		"bottomMargin"
#define XtNsourceType		"sourceType"
#define XtCSourceType		"SourceType"
#define XtNmotionVerification	"motionVerification"
#define XtNmodifyVerification	"modifyVerification"
#define XtNleaveVerification	"leaveVerification"
#define XtNexecute		"execute"
#define XtNwrap			"wrap"
#define XtCWrap			"Wrap"
#define XtNwrapForm		"wrapForm"
#define XtCWrapForm		"WrapForm"
#define XtNwrapBreak		"wrapBreak"
#define XtCWrapBreak		"WrapBreak"
#define XtNgrow			"grow"
#define XtCGrow			"Grow"
#define XtNtextClearBuffer	"textClearBuffer"
#define XtCTextClearBuffer	"TextClearBuffer"
#define XtNtextCopyBuffer	"textCopyBuffer"
#define XtCTextCopyBuffer	"TextCopyBuffer"
#define XtNtextGetInsertPoint	"textGetInsertPoint"
#define XtCTextGetInsertPoint	"TextGetInsertPoint"
#define XtNtextGetLastPos	"textGetLastPos"
#define XtCTextGetLastPos	"TextGetLastPos"
#define XtNtextInsert		"textInsert"
#define XtCTextInsert		"TextInsert"
#define XtNscroll		"scroll"
#define XtCScroll		"Scroll"
#define XtNtextReadSubStr	"textReadSubStr"
#define XtCTextReadSubStr	"TextReadSubStr"
#define XtNtextRedraw		"textRedraw"
#define XtCTextRedraw		"TextRedraw"
#define XtNtextReplace		"textReplace"
#define XtCTextReplace		"TextReplace"
#define XtNtextSetInsertPoint	"textSetInsertPoint"
#define XtCTextSetInsertPoint	"TextSetInsertPoint"
#define XtNtextSetSource	"textSetSource"
#define XtCTextSetSource	"TextSetSource"
#define XtNtextUpdate		"textUpdate"
#define XtCTextUpdate		"TextUpdate"
#define XtNverticalSBWidget	"verticalSBWidget"
#define XtCVerticalSBWidget	"VerticalSBWidget"


/* PopupWindo.c */
#define	XtNupperControlArea     "upperControlArea"
#define XtCUpperControlArea     "UpperControlArea"
#define XtNlowerControlArea	"lowerControlArea"
#define XtCLowerControlArea	"LowerControlArea"
#define XtNfooterPanel		"footerPanel"
#define XtCFooterPanel		"FooterPanel"
#define XtNapply		"apply"
#define XtNsetDefaults		"setDefaults"
#define XtNreset		"reset"
#define XtNresetFactory		"resetFactory"
#define XtNverify		"verify"
#define XtNpropertyChange	"propertyChange"
#define XtCPropertyChange	"PropertyChange"
#define XtCPushpin		"Pushpin"
#define XtNalignCaptions	"alignCaptions"

/* Help.c */
#define XtNverticalSB		"verticalSB"
#define XtNmouseX		"mouseX"
#define XtNmouseY		"mouseY"

/* CheckBox.c */
#define XtNlabelType		"labelType"
#define XtCLabelType		"LabelType"
#endif /* __Ol_OlMinStr_h__ */
#endif /* OL_USE_DEFINE */
