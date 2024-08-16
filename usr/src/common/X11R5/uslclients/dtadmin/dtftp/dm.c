/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtftp:dm.c	1.1.1.2"
#endif

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/Olg.h>
#include <DtI.h>
#include "dm.h"
#include "shortcut.icon"

typedef struct _SmallIcons {
	DmGlyphPtr	icon;
	char *		name;
} SmallIcons;

static SmallIcons	smallIconTable[] = {
	NULL,	"sdir.icon",
	NULL,	"sexec.icon",
	NULL,	"sdatafile.icon",
	NULL,	"spipe.icon",
	NULL,	"schrdev.icon",
	NULL,	"sblkdev.icon",
	NULL,	"ssem.icon",
	NULL,	"sshmem.icon",
	NULL,	"sunk.icon",
	NULL,	"stoolbox.icon",
	NULL,	NULL
};

#define	POINT_SIZE	12	

static void
InitSmallIcons (Widget w)
{
	int	i;

	if (smallIconTable[0].icon == NULL) {
		for (i=0; smallIconTable[i].name != NULL; i++) {
			smallIconTable[i].icon = DmGetPixmap (
				XtScreen(w), smallIconTable[i].name
			);
		}
	}
}

static DmGlyphPtr
GetShortCut (Widget w)
{
	static DmGlyphPtr	SortCut = NULL; /* shortcut glyph */

	if (SortCut == NULL) {
		SortCut = DmCreateBitmapFromData (
			XtScreen (w),
			"\n/default shortcut icon\n",
			(unsigned char *)shortcut_bits,
			shortcut_width, shortcut_height
		);
	}
	return SortCut;
}

#define FontWidth(w,fontList)	OlFontWidth(GetDefaultFont(w), fontList)
#define FontHeight(w,fontList)	OlFontHeight(GetDefaultFont(w), fontList)

#define MAX(a, b)	(((int)(a) > (int)(b)) ? (int)(a) : (int)(b))

XFontStruct *
GetDefaultFont (Widget w)
{
	static XFontStruct *	deffont = NULL;

	if (deffont == NULL) {
		deffont = _OlGetDefaultFont (w, OlDefaultFont);
	}
	return deffont;
}

Dimension
NameRowHeight (Widget w, OlFontList *fontList)
{
	return MAX(
		FontHeight(w, fontList) + 2 * ICON_PADDING,
		smallIconTable[DM_FTYPE_DIR].icon->height
		+ 2 * ICON_PADDING + ICON_PADDING / 2
	);
}

Dimension
LongRowHeight (Widget w, OlFontList *fontList)
{
	return MAX(
		FontHeight(w, fontList) + 2 * ICON_PADDING,
		smallIconTable[DM_FTYPE_DIR].icon->height
		+ 2 * ICON_PADDING + ICON_PADDING / 2
	);
}

XFontStruct *
GetFixedFont (Widget w)
{
	static XFontStruct *	fixedfont = NULL;

	if (fixedfont == NULL) {
		fixedfont = _OlGetDefaultFont (w, OlDefaultFixedFont);
	}
	return fixedfont;
}

static void
ComputeItemSize (
	Widget w, DmItemPtr item, DmViewFormatType type,
	Dimension *width, Dimension *height, OlFontList *fontList
)
{
	DmGlyphPtr		gp;

	gp = smallIconTable[ITEM_OBJ(item)->ftype-1].icon;

	if (type == DM_ICONIC) {
		/* NOTE: DmSizeIcon will set the dimension in the item
		 * instead of just returning the width and height.
		 * There is currently no interface to just get the
		 * dimension of an item in ICONIC view.
		 */
		DmSizeIcon (item, fontList, GetDefaultFont(w));
		*width = (Dimension)item->icon_width;
		*height = (Dimension)item->icon_height;
	}
	else if (type == DM_NAME) {
		*width = (
			gp->width + ICON_LABEL_GAP + 2 * ICON_PADDING +
			DM_TextWidth (
				GetDefaultFont(w), fontList,
				ITEM_LABEL(item), strlen(ITEM_LABEL(item))
			)
		);
		*height = NameRowHeight (w, fontList);
	}
	else if (type == DM_LONG) {
		/* Since LONG view uses a fixed-width font, the
		 * optimization for calculating the label width is possible.
		 */
		*width = (
			gp->width + ICON_LABEL_GAP + 2 * ICON_PADDING +
			DM_TextWidth (
				GetFixedFont(w), fontList,
				ITEM_LABEL(item), strlen(ITEM_LABEL(item))
			)
		);
		*height = LongRowHeight (w, fontList);
	}
}

static int
ItemLabelsMaxLen (DmItemPtr item, int count, Boolean noIconLabel)
{
	int		i;
	int		maxl;

	maxl = 0;
	for (i=0; i<count; i++) {
		if (ITEM_MANAGED (item)) {
			maxl = MAX (
				strlen (
					noIconLabel ? (ITEM_OBJ(item))->name
					: DmGetObjectName(ITEM_OBJ(item))
				),
				maxl
			);
		}
		item += 1;
	}
	return maxl;
}

void
ComputeLayout(
	Widget w, DmItemPtr itemp, int count, int type,
	Dimension width,
	DtAttrs geomOptions,	/* size, icon position options */
	DtAttrs layoutOptions,	/* layout attributes */
	Dimension gridWidth, Dimension gridHeight,
	OlFontList *fontList,
	char *ShortName(), char *LongName()
)
{
	DmItemPtr	item;
	int		maxlen = 0;
	Dimension	margin = Ol_PointToPixel (OL_HORIZONTAL, ICON_MARGIN);
	Dimension	pad = Ol_PointToPixel (OL_HORIZONTAL, INTER_ICON_PAD);
	Position	x = margin;
	Position	y = margin;
	Position	centerx;
	Dimension	rowHeight;
	Dimension	itemWidth;
	Position	iconx;
	Position	icony;
	char *		label;

	InitSmallIcons (w);

	width = MAX (width, gridWidth);

	if (type == DM_NAME) {

		/* Compute row height and grid width outside of loop */
		rowHeight = (
			(geomOptions & DM_B_CALC_SIZE) ?
			NameRowHeight (w, fontList) : ITEM_HEIGHT (itemp)
		) + pad;

		for (item=itemp; item<itemp+count; item++) {
			if (!ITEM_MANAGED(item)) {
				continue;
			}
			if (geomOptions & DM_B_CALC_SIZE) {
				Dimension iconWidth;
				Dimension iconHeight;

				if (layoutOptions & UPDATE_LABEL) {
					FREE (ITEM_LABEL (item));
					label = (ShortName) (item);
					item->label = (XtArgVal) STRDUP(label);
				}
				ComputeItemSize (
					w, item, type,
					&iconWidth, &iconHeight,
					fontList
				);
				item->icon_width = iconWidth;
				item->icon_height = iconHeight;
			}
			itemWidth = ITEM_WIDTH (item);

			/* Wrap now if item will extend beyond "wrap" width */
			if (
				(x != margin) &&
				((Dimension)(x + itemWidth) > width)
			) {
				x = margin;
				y += rowHeight;
			}

			item->x = x;
			item->y = y;

			x += (
				(itemWidth <= gridWidth) ?
				gridWidth :
				((Dimension)(itemWidth+gridWidth-1)/
				gridWidth) * gridWidth
			);
		}
	}
	else if (type == DM_ICONIC) {
		/* Compute row height and grid width outside of loop */
		rowHeight = gridHeight;
		centerx = x + (gridWidth / 2);

		for (item=itemp; item<itemp+count; item++) {

			if (!ITEM_MANAGED(item)) {
				continue;
			}
			if (layoutOptions & UPDATE_LABEL) {
				FREE (ITEM_LABEL (item));
				label = (ShortName) (item);
				item->label = (XtArgVal) STRDUP (label);
			}

			if (geomOptions & DM_B_CALC_SIZE) {
				Dimension iconWidth;
				Dimension iconHeight;

				ComputeItemSize (
					w, item, type,
					&iconWidth, &iconHeight, fontList
				);
				item->icon_width = iconWidth;
				item->icon_height = iconHeight;
			}
			if (layoutOptions & RESTORE_ICON_POS) {
				if (
					((ITEM_OBJ(item))->x != 0) ||
					((ITEM_OBJ(item))->y != 0)
				) {
					item->x = ITEM_OBJ(item)->x;
					item->y = ITEM_OBJ(item)->y;
				}
			}
		}

		for (item=itemp; item<itemp+count; item++) {
			Position	nextx;

			if (!ITEM_MANAGED(item)) {
				continue;
			}

			itemWidth = ITEM_WIDTH(item);
	again:
			/* Horiz: centered */
			centerx = x + (gridWidth / 2);
			nextx = centerx - (itemWidth / 2);

			/* Wrap now if item will extend beyond "wrap" width */
			if (
				(x != margin) &&
				((Dimension)(x + gridWidth) > width)
			) {
				x = margin;
				y += rowHeight;
				goto again;
			}
			/* Vert: bottom justified */
			item->y = VertJustifyInGrid (
				y, ITEM_HEIGHT(item),
				rowHeight
			);

			item->x = nextx;

			x += gridWidth;
			centerx += gridWidth / 2;	/* next center */
		}
	}
	else if (type == DM_LONG) {
		Dimension	iconWidth;
		Dimension	iconHeight;

		/* Compute row height and max label length outside of loop */
		rowHeight = LongRowHeight(w, fontList) + pad;
		maxlen = ItemLabelsMaxLen (
			itemp, count, (layoutOptions & NO_ICONLABEL)
		);

		for (item=itemp; item<itemp+count; item++) {
			if (!ITEM_MANAGED (item)) {
				continue;
			}
			FREE (ITEM_LABEL(item));
			label = (LongName) (item, maxlen +3);
			item->label = (XtArgVal)STRDUP (label);
			ComputeItemSize (
				w, item, type, &iconWidth, &iconHeight,
				fontList
			);
			item->icon_width = iconWidth;
			item->icon_height = iconHeight;

			item->x = x;
			item->y = y;

			/* we always assume one column in long format */
			y += rowHeight;
		}
	}
}

/*
 *************************************************************************
 * DrawLinkIcon - draws an icon visual given an icon glyph and string.
 ****************************procedure*header*****************************
 */
void
DrawLinkIcon (Widget w, XtPointer clientData, OlFIconDrawPtr idp)
{
	Display *		dpy = XtDisplay (w);
	GC			gc = idp->label_gc;
	DmObjectPtr		op = (DmObjectPtr)idp->op;
	DmGlyphPtr		gp;
	DmGlyphPtr		sgp = GetShortCut (w); /* shortcut glyph */
	int			x;
	int			y;
	int			width;

	/* draw the standard icon first */
	DmDrawIcon (w, (XtPointer)NULL, (XtPointer)idp);

	if (op->attrs & DM_B_SYMLINK) {
		gp = op->fcp->glyph;

		y = idp->y + gp->height + ICON_PADDING * 2 +
		    ICON_PADDING / 2;
		width = (gp->width / sgp->width) * sgp->width;
		x = idp->x + ((int)idp->width - width) / 2;

		XSetStipple (dpy, gc, sgp->pix);
		XSetFillStyle (dpy, gc, FillStippled);
		XSetTSOrigin (dpy, gc, x, y);
		XFillRectangle (
			dpy, XtWindow (w), gc, x, y, width, sgp->height
		);
		XSetFillStyle (dpy, gc, FillSolid);
		XSetTSOrigin (dpy, gc, 0, 0);
	}
}

static void
DrawIcon (Widget w, OlFIconDrawPtr idp, DmViewFormatType type)
{
	DmObjectPtr		op = (DmObjectPtr)idp->op;
	int			yoffset;
	DmGlyphPtr		gp;
	OlgAttrs *		attrs;
	Pixel			pixel;
	GC			gc = idp->label_gc;
	Display *		dpy = XtDisplay(w);
	Window			win = XtWindow(w);
	unsigned		flags;

	if (idp->busy == True) {
		flags = RB_DIM;
	}
	else {
		flags = 0;
	}

	pixel = idp->focus ? idp->focus_color : idp->bg_color;
	attrs = OlgCreateAttrs (
		XtScreen(w), idp->fg_color, (OlgBG *)&(pixel),
		False, POINT_SIZE
	);
	InitSmallIcons (w);
	gp = smallIconTable[op->ftype-1].icon;

	/* Use offset to center glyph vertically */
	yoffset = ((int)idp->height - (int)(gp->height)) / 2;

	DmDrawIconGlyph (w, gc, gp, attrs, idp->x, idp->y + yoffset, flags);

	flags |= (idp->select) ? RB_SELECTED : RB_NOFRAME;

	/* Draw the label under the icon */
	if (idp->label != (String)NULL) {
		int			length = strlen (idp->label);
		int			i;	/* ignore this value	*/
		XFontStruct *		font;
		Dimension		textHeight;
		Dimension		xoffset;

		font = (type == DM_LONG) ? GetFixedFont(w) : idp->font;
		textHeight = OlFontHeight (font, idp->font_list);

		/* Use offset to center label vertically */
		yoffset = ((int)idp->height - (int)textHeight) / 2;

		/* Text offset is to the right of glyph */
		xoffset = gp->width + ICON_LABEL_GAP;

		if (!(idp->font_list)) {
			XSetFont (XtDisplay(w), gc, font->fid);
		}

		if (idp->focus) {
			if (idp->focus_color == idp->fg_color) {
				XSetBackground (dpy, gc, idp->fg_color);
				XSetForeground (dpy, gc, idp->bg_color);
			}
			else {
				XSetBackground (dpy, gc, idp->focus_color);
			}
		}
		DmDrawIconLabel (
			w, gc, idp->label,
			idp->font_list,
			font, attrs,
			idp->x + xoffset,
			idp->y + yoffset,
			flags
		);
		if (idp->focus_color == idp->fg_color) {
			XSetForeground (dpy, gc, idp->fg_color);
		}
		if (idp->focus) {
			XSetBackground (dpy, gc, idp->bg_color);
		}

		/* restore original font */
		if (!(idp->font_list)) {
		    XSetFont (XtDisplay (w), gc, idp->font->fid);
		}
	}

	if (op->attrs & DM_B_SYMLINK) {
		DmGlyphPtr	sgp = GetShortCut (w);	/* shortcut glyph */
		int		x;
		int		y;
		int		width;

		y = idp->y + gp->height + ICON_PADDING;
		width = (gp->width / sgp->width) * sgp->width;
		x = idp->x /* + ((int)idp->width - width) / 2 */ ;
		
		XSetStipple (dpy, gc, sgp->pix);
		XSetFillStyle (dpy, gc, FillStippled);
		XSetTSOrigin (dpy, gc, x, y);
		XFillRectangle (
			dpy, XtWindow (w), gc, x, y, width, sgp->height
		);
		XSetFillStyle (dpy, gc, FillSolid);
		XSetTSOrigin (dpy, gc, 0, 0);
	}
}

void 
DrawLongIcon (Widget w, XtPointer clientData, XtPointer callData)
{
	DrawIcon (w, (OlFIconDrawPtr)callData, DM_LONG);
}

void
DrawNameIcon (Widget w, XtPointer clientData, XtPointer callData)
{
	DrawIcon (w, (OlFIconDrawPtr)callData, DM_NAME);
}
