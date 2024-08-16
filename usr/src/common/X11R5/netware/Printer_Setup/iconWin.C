#ident	"@(#)prtsetup2:iconWin.C	1.16"
/*----------------------------------------------------------------------------
 *	iconWin.c
 */

#include <iostream.h>

#include <X11/Intrinsic.h>

#include <Xm/Form.h>
#include <Xm/ScrolledW.h>
#include <Xm/Screen.h>

#include "BasicComponent.h"
#include "iconWin.h"

/*----------------------------------------------------------------------------
 *
 */
static String fields[] = {
	XmNx,
	XmNy,
	XmNwidth,
	XmNheight,
	XmNlabelString,
	XmNset,
	XmNsensitive,
	XmNobjectData,
	XmNuserData,
	XmNmanaged,
	XmNmappedWhenManaged
};

/*----------------------------------------------------------------------------
 *
 */
// Test this stuff out
#define cur_width 	16
#define cur_height 	16
#define cur_x_hot	0
#define cur_y_hot	0

static _XmConst unsigned char nocur_bits[] = {
	0x00, 0x00, 0xf0, 0x0f, 0x18, 0x18, 0x0c, 0x30, 0x06, 0x68, 0x02, 0x44,
	0x02, 0x42, 0x02, 0x41, 0x82, 0x40, 0x42, 0x40, 0x22, 0x40, 0x16, 0x60,
	0x0c, 0x30, 0x18, 0x18, 0xf0, 0x0f, 0x00, 0x00
};

static _XmConst unsigned char yescur_bits[] = {
	0x00, 0x00, 0xf0, 0x0f, 0x18, 0x18, 0x0c, 0x30, 0x16, 0x60, 0x32, 0x4c,
	0x4a, 0x52, 0x02, 0x40, 0x42, 0x42, 0x82, 0x41, 0x12, 0x48, 0x26, 0x64,
	0xcc, 0x33, 0x18, 0x18, 0xf0, 0x0f, 0x00, 0x00
};

static _XmConst unsigned char msk_bits[] = {
	0x00, 0x00, 0xf0, 0x0f, 0xf8, 0x1f, 0xfc, 0x3f, 0xf3, 0x7f, 0xfe, 0x7f,
	0xfe, 0x7f, 0xfe, 0x7f, 0xfe, 0x7f, 0xfe, 0x7f, 0xfe, 0x7f, 0xfe, 0x7f,
	0xfc, 0x3f, 0xf8, 0x1f, 0xf0, 0x0f, 0x00, 0x00
};

/*----------------------------------------------------------------------------
 *
 */
IconWin::IconWin (Widget	parent,
				  char*		name,
				  char**	pixmapFiles, 
				  short		pixmapCnt)
	   : BasicComponent (name)
{
	static Atom				 	targets[1] = { XA_STRING };
	Screen*						screen = XtScreen (parent);
	Widget						swin;

	// Initialize IconWin data objects
	_pixmapCnt = pixmapCnt;
	_pixmapList = pixmapFiles;
	_list = new IconList (); 

	_row = 1;
	_col = 1;
	_hOffset = 0;
	_vOffset = 0;

	// Init Default Icons for DnD
	InitDnDIcons (screen);
	
	// Create Widgets
	_w = XmCreateForm (parent, name, NULL, 0);

	swin = XtVaCreateManagedWidget ("PrinterSW", 
									xmScrolledWindowWidgetClass, _w,
									XmNscrollingPolicy, XmAPPLICATION_DEFINED,
									XmNvisualPolicy, XmVARIABLE,
									XmNscrollBarDisplayPolicy, XmSTATIC,
									XmNshadowThickness, 0,
									XmNtopAttachment, XmATTACH_FORM,
									XmNbottomAttachment, XmATTACH_FORM,
									XmNrightAttachment, XmATTACH_FORM,
									XmNleftAttachment, XmATTACH_FORM,
									0);

	_iconBox = XtVaCreateManagedWidget ("iconBox", exmFlatIconBoxWidgetClass,
									swin,
									XmNdrawProc, DmDrawIcon,
									XmNselectProc, SelectCB,
									XmNdblSelectProc, DblSelectCB,
									XmNtargets, targets,
									XmNnumTargets, XtNumber (targets),
									XmNclientData, this,
									XmNtriggerMsgProc, TriggerMsg,
									XmNdropProc, &DropProcCB,
									XmNdragCursorProc, CreateIconCursor,
									XmNconvertProc, DmDnDConvertSelectionProc,
									XmNdragDropFinishProc, DmDnDFinishProc,
									XmNunselectProc, UnselectCB,
									XmNdragOperations, XmDROP_LINK,
									0);	

	manage ();
} 

/*----------------------------------------------------------------------------
 *
 */
IconWin::~IconWin ()
{
} 

//--------------------------------------------------------------
//	This function is called when an IconObj has been
//	added or deleted from the list. Reads the list
//	of icon objects, builds an array of items based on 
//	the printer information, and calls XtVaSetValues.
//--------------------------------------------------------------
void
IconWin::ReGenWin (Boolean firstTime)
{
	static DisplayItem*			displayList = 0;
	DisplayItem*				displayItem;
	short						cnt;
	short						rowCnt = 1;
	IconObj*					obj;
	DmFclassPtr 				ptr;
	XmString					str;

	XtVaGetValues (_iconBox, XmNgridWidth, &_cellWidth, 0);
	XtVaGetValues (_iconBox, XmNgridHeight, &_cellHeight, 0);

	cnt = _list->GetObjCnt ();

	while ((rowCnt * (rowCnt + 1)) < cnt) {
		rowCnt++;
	}
	_row = rowCnt;
	_col = rowCnt + 1;

	if (displayList) {					// Now build an array for the objects	
//		delete (displayList);			// DONE BY FLAT-ICON-BOX ?????????????
	}
	if (!(displayList = new DisplayItem[cnt + 1])) {
		return;
	}

	if (firstTime) {
		if (obj = _list->GetFirst ()) {
			_list->UpdateSelected (obj);
			NotifyParentOfSel (obj);
		}
	}

	for (obj = _list->GetFirst (), displayItem = displayList; 
		 obj; 
		 obj = _list->GetNext (), displayItem++) {

		displayItem->width = _cellWidth;
		displayItem->height = _cellHeight;
		str = XmStringCreateSimple (obj->GetLabel ()); 

		displayItem->label = (XtArgVal)_XmStringCreate (str);
		XmStringFree (str);
		if (obj == _list->GetSelected ()) {
			displayItem->set = True;
		}
		else {
			displayItem->set = False;
		}
		displayItem->sensitive = True;
		displayItem->managed = True;
		displayItem->mappedWhenManaged = True;

		displayItem->obj = (XtArgVal)XtMalloc (sizeof (DmObjectRec));
		memset ((void*)displayItem->obj, 0, sizeof (DmObjectRec));
		ptr = (DmFclassPtr)XtMalloc (sizeof (DmFclassRec));
		memset (ptr, 0, sizeof (DmFclassRec));
//		DtSetProperty (&(ptr->plist), ICONFILE, obj->GetPixmap(), NULL);
		((DmObjectRec*)displayItem->obj)->fcp = ptr;
		DmSetObjProperty ((DmObjectPtr)displayItem->obj, 
						  ICONLABEL,
						  obj->GetLabel (),
						  NULL);
		DmSetObjProperty ((DmObjectPtr)displayItem->obj, 
						  ICONFILE,
						  obj->GetPixmap(),
						  NULL);
//		DtSetProperty (&(ptr->plist), ICONLABEL, obj->GetLabel(), NULL);
		DmInitObjType (_w, (DmObjectPtr)displayItem->obj);
//		ptr->cursor = NULL;
//		ptr->glyph = DmGetPixmap (_scrn, obj->GetPixmap ());
//		ptr->glyph->pix = DmMaskPixmap (_iconBox, ptr->glyph);
//		Dm__CreateIconMask (_di->Screen (), ptr->glyph);
		displayItem->userData = (XtArgVal)obj;
	}

	LayoutIcons (displayList, cnt);
	XtVaSetValues (_iconBox, 
				   XmNgridColumns, _col,
				   XmNgridRows, _row,
				   XmNgridWidth, _cellWidth,
				   XmNgridHeight, _cellHeight,
				   XmNnumItems, cnt,
				   XmNnumItemFields, 11 /*XtNumber (fields)*/,
				   XmNitemFields, fields,
				   XmNitems, displayList,
				   0);
	XtVaSetValues (_iconBox, XmNitemsTouched, True, 0);
}

/*----------------------------------------------------------------------------
 *
 */
const IconObj*
IconWin::findByName (const char* name)
{
	IconObj*					obj;
	int							len = strlen (name);

	for (obj = _list->GetFirst (); obj; obj = _list->GetNext ()) {
		if (strlen (obj->GetLabel ()) == len &&
			!(strncmp (name, obj->GetLabel (), len))) {
			return (obj); 
		}
	}
	return (0);
}

//--------------------------------------------------------------
// This function adds an item to the iconWin.
//--------------------------------------------------------------
Boolean
IconWin::AddIcon (IconObj* obj)
{
	DisplayItem*				displayItems = 0;
	DisplayItem*				newItem;
	Cardinal					cnt;
	Dimension					width;
	Dimension					height;
	XmString					str;
	DmFclassPtr 				ptr;

	XtVaGetValues (_iconBox,
				   XmNitems, &displayItems,
				   XmNnumItems, &cnt,
				   XmNwidth, &width,
				   XmNheight, &height,
				   0);

	newItem = displayItems;
	for (int i = 0;i < cnt;i++, newItem++) {
		newItem->set = False;
	}

	cnt++;	
	displayItems = (DisplayItem*)XtRealloc ((char*)displayItems, 
											cnt * sizeof (*displayItems));	

	newItem = displayItems + cnt - 1;
	str = XmStringCreateSimple (obj->GetLabel ()); 
	newItem->label = (XtArgVal)_XmStringCreate (str);
	XmStringFree (str);
	newItem->width = _cellWidth;
	newItem->height = _cellHeight;
	newItem->set = True;
	newItem->sensitive = True;
	newItem->managed = True;
	newItem->mappedWhenManaged = True;
	newItem->obj = (XtArgVal)XtMalloc (sizeof (DmObjectRec));
	memset ((void*)newItem->obj, 0, sizeof (DmObjectRec));
	ptr = (DmFclassPtr)XtMalloc (sizeof (DmFclassRec));
	memset ((void*)ptr, 0, sizeof (DmFclassRec));
	((DmObjectRec*)newItem->obj)->fcp = ptr;
	DmSetObjProperty ((DmObjectPtr)newItem->obj,
					  ICONLABEL,
					  obj->GetLabel (),
					  NULL);
	DmSetObjProperty ((DmObjectPtr)newItem->obj,
					  ICONFILE,
					  obj->GetPixmap (),
					  NULL);
	DmInitObjType (_w, (DmObjectPtr)newItem->obj);
//	ptr->cursor = NULL;
//	ptr->glyph = DmGetPixmap (_scrn, obj->GetPixmap());
//	Dm__CreateIconMask (_di->Screen(), ptr->glyph);
	newItem->userData = (XtArgVal)obj;

	LayoutIcons (displayItems, cnt);
	XtVaSetValues (_iconBox,
				   XmNitems, displayItems,
				   XmNnumItems, cnt,
				   XmNnumItemFields, XtNumber (fields),
				   XmNitemFields, fields,
				   XmNitemsTouched, True,
				   0);
//	ExmVaFlatSetValues (_iconBox, cnt -1, XmNuserData, obj, NULL);
	XtVaSetValues (_iconBox, XmNitemsTouched, True, NULL);

	_list->UpdateSelected (obj);
	NotifyParentOfSel (obj);

	return (True);
}

//--------------------------------------------------------------
// This function deletes an item from the iconWin.
//--------------------------------------------------------------
Boolean
IconWin::DeleteIcon (IconObj* obj)
{
	DisplayItem*				displayItems;
	Cardinal					cnt;
	Boolean						found;
	short						i;
	IconObj*					tmpObj;

	XtVaGetValues (_iconBox,
				   XmNitems, &displayItems,
				   XmNnumItems, &cnt, 
				   0);
	for (i = 0, found = False; i < cnt; i++) {
		tmpObj = (IconObj*)(displayItems[i].userData);	

		if (found && i < (cnt - 1)) {
			memcpy (&displayItems[i],
					&displayItems[i + 1],
					sizeof (DisplayItem)); 
		}
		else {
			if (obj == tmpObj) {
				if (i < (cnt - 1)) {
					memcpy (&displayItems[i],
							&displayItems[i+1],
							sizeof (DisplayItem));
				}
				found = True;
			}
		}
	}
	cnt--;
	LayoutIcons (displayItems, cnt);
	XtVaSetValues (	_iconBox,
				   XmNitems, displayItems,
				   XmNnumItems, cnt,
				   XmNitemsTouched, True,
				   0);
	return (True);
}

//--------------------------------------------------------------
// This function adds the x and y fields to a list
//		of objects to be displayed in an IconWin object.
//--------------------------------------------------------------
void
IconWin::LayoutIcons (DisplayItem* items, Cardinal cnt)
{
	DisplayItem*				cur;
	int							x,y;
	int							i;
	short						width = 540/*_di->Width()*/;
	
	x = y = 0;
	for (cur = items, i = cnt; --i >= 0; cur++) {
		if ((x + _cellWidth) > width) {
			x = 0;
			y += _cellHeight;
		}
		cur->x = x;
		cur->y = y;
		x += _cellWidth;	
	}
}

//--------------------------------------------------------------
//	This function is called when an icon in the iconWin is selected.
//--------------------------------------------------------------
void
IconWin::SelectCB (Widget, XtPointer clientData, XtPointer callData)
{
	IconWin*					obj = (IconWin*)clientData; 	

	obj->Select ((ExmFlatCallData*)callData);
}

//--------------------------------------------------------------
//	This function is the member function called from SelectCB.
//--------------------------------------------------------------
void
IconWin::Select (ExmFlatCallData* data)
{
	IconObj*					obj;

	obj = (IconObj*)(data->item_user_data);
	_list->UpdateSelected (obj);
	NotifyParentOfSel (obj);
}

//--------------------------------------------------------------
//	This function is called when an icon in the iconWin is selected.
//--------------------------------------------------------------
void
IconWin::UnselectCB (Widget, XtPointer clientData, XtPointer)
{
	IconWin*					obj = (IconWin*)clientData; 	

	obj->Unselect ();
}

//--------------------------------------------------------------
//	This function is the member function called from SelectCB.
//--------------------------------------------------------------
void
IconWin::Unselect ()
{
	_list->UpdateSelected (NULL);
	NotifyParentOfSel (NULL);
}

//--------------------------------------------------------------
//	This function is called when an icon in the iconWin is double clicked on.
//--------------------------------------------------------------
void
IconWin::DblSelectCB (Widget, XtPointer clientData, XtPointer callData)
{
	IconWin*					obj = (IconWin*)clientData; 	

	obj->DblSelect ((ExmFlatCallData*)callData);
}

//--------------------------------------------------------------
//	This is the member function called from DblSelectCB.
//--------------------------------------------------------------
void
IconWin::DblSelect (ExmFlatCallData* data)
{
	IconObj*					obj;

	obj = (IconObj*)(data->item_user_data);
	_list->UpdateSelected (obj);
	NotifyParentOfDblSel (obj);
}

/*----------------------------------------------------------------------------
 *
 */
void
IconWin::TriggerMsg (Widget w, XtPointer clientData, XtPointer callData)
{
	IconWin*					obj = (IconWin*)clientData;

	obj->Trigger (w, clientData, callData);
}

/*----------------------------------------------------------------------------
 *
 */
void
IconWin::Trigger (Widget w, XtPointer clientData, XtPointer callData)
{
	ExmFIconBoxTriggerMsgCD*	cd = (ExmFIconBoxTriggerMsgCD*)callData;

	_droppedIndex = cd->item_data.item_index;

	if ((DmDnDGetFileNames (w,
							0,
							(void(*)())&IconWin::TransferProc,
							clientData,
							callData)) == NULL) {
#ifdef DEBUG
		printf("ERROR\n");
#endif
	}
}

/*----------------------------------------------------------------------------
 *
 */
void
IconWin::TransferProc (Widget w, XtPointer clientData, XtPointer callData)
{
	IconWin*					obj = (IconWin*)clientData;
	DmDnDDstInfoPtr				dip = (DmDnDDstInfoPtr)callData;

	obj->Transfer (w, dip);
}

/*----------------------------------------------------------------------------
 *
 */
void
IconWin::DropProcCB (Widget w, XtPointer clientData, XtPointer callData)
{
	IconWin*					obj = (IconWin*)clientData;

	obj->DropProc (w, clientData, callData);
}

/*----------------------------------------------------------------------------
 *
 */
void
IconWin::CreateIconCursor (Widget w, XtPointer clientData, XtPointer callData)
{
	ExmFlatDragCursorCallData*	f_cursor = (ExmFlatDragCursorCallData*)callData;
	DisplayItem*				item = ((DisplayItem*)f_cursor->item_data.items)
											+ f_cursor->item_data.item_index;
	DmObjectPtr					op = (DmObjectPtr) (item->obj);
	DmGlyphPtr					glyph = op->fcp->glyph;
	Pixmap						pix;
	Pixmap						pixMask;
	Arg							args[15];
	int							n;
	int							depth;

	if (op->fcp->cursor) {
		pix = op->fcp->cursor->pix;
		pixMask = op->fcp->cursor->mask;
		depth = 1; 		
	}
	else {
		pix = glyph->pix;
		pixMask = glyph->mask;
		depth = glyph->depth;
	}
	// Position of the cursor's hotspot relative to the origin of the icon
	f_cursor->x_hot = item->width / 2;
	f_cursor->y_hot = glyph->height / (Dimension)2;

	n = 0;
	XtSetArg (args[n], XmNwidth, glyph->width); n++;
	XtSetArg (args[n], XmNheight, glyph->height); n++;
	XtSetArg (args[n], XmNdepth, depth); n++;
	XtSetArg (args[n], XmNpixmap, pix); n++;
	XtSetArg (args[n], XmNmask, pixMask); n++;
	XtSetArg (args[n], XmNattachment, XmATTACH_CENTER); n++;

	f_cursor->source_icon = XmCreateDragIcon (w, "drag_icon", args, n);
	f_cursor->static_icon = False;
}

/*----------------------------------------------------------------------------
 *
 */
void
IconWin::InitDnDIcons (Screen* s)
{
	Display*					dpy = DisplayOfScreen (s);
	Window						root = RootWindowOfScreen (s);
	Pixmap						no_src;
	Pixmap						yes_src;
	Pixmap						mask;
	Widget						xm_screen;
	Widget						no_w;
	Widget						yes_w;
	Arg							args[10];
	int							n;

	xm_screen = XmGetXmScreen (s);
	no_src = XCreateBitmapFromData (dpy,
									root,
									(_XmConst char*)nocur_bits,
									cur_width,
									cur_height);
	mask = XCreateBitmapFromData (dpy,
								  root,
								  (_XmConst char*)msk_bits,
								  cur_width,
								  cur_height);
	yes_src = XCreateBitmapFromData (dpy,
									 root,
									 (_XmConst char*)yescur_bits,
									 cur_width,
									 cur_height);
	n = 0;
	XtSetArg (args[n], XmNhotX, cur_x_hot); n++;
	XtSetArg (args[n], XmNhotY, cur_y_hot); n++;
	XtSetArg (args[n], XmNwidth, cur_width); n++;
	XtSetArg (args[n], XmNheight, cur_height); n++;
	XtSetArg (args[n], XmNattachment, XmATTACH_CENTER); n++;
	XtSetArg (args[n], XmNmask, mask); n++;
	XtSetArg (args[n], XmNpixmap, no_src); n++;

	no_w = XmCreateDragIcon (xm_screen, "no_icon", args, n);
	n -= 1;
	XtSetArg (args[n], XmNpixmap, yes_src); n++;
	yes_w = XmCreateDragIcon (xm_screen, "yes_icon", args, n);

	XtSetArg (args[0], XmNdefaultNoneCursorIcon, no_w);
	XtSetArg (args[1], XmNdefaultInvalidCursorIcon, no_w);
	XtSetArg (args[0], XmNdefaultValidCursorIcon, yes_w);
	XtSetValues (xm_screen,args, 3);
}

