/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5server:dix/colormap.c	1.2"

/*
 *	Copyright (c) 1991 1992 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */

/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
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

/* $XConsortium: colormap.c,v 5.21 91/07/12 20:23:41 rws Exp $ */

#include "X.h"
#define NEED_EVENTS
#include "Xproto.h"
#include "misc.h"
#include "dix.h"
#include "colormapst.h"
#include "os.h"
#include "scrnintstr.h"
#include "resource.h"
#include "windowstr.h"

extern XID clientErrorValue;

#ifdef __STDC__
/* Prototypes for some common functions */
static Pixel FindBestPixel(EntryPtr pentFirst,
			   int size,
			   xrgb *prgb,
			   int channel);

extern int FindColor (ColormapPtr pmap,
		      EntryPtr	pentFirst,
		      int	size,
		      xrgb	*prgb,
		      Pixel	*pPixel,
		      int	channel,
		      int	client,
		      int	(*comp) ());
#else /*__STDC__*/
static Pixel FindBestPixel();
#endif /*__STDC__*/
static void  CopyFree(), FreeCell(), FreePixels(), UpdateColors();
static int   AllComp(), RedComp(), GreenComp(), BlueComp();
static int   AllocDirect(), AllocPseudo(), FreeCo();
static Bool  AllocCP(), AllocShared();
static int   TellNoMap();

void FreeClientPixels();

#define NUMRED(vis) ((vis->redMask >> vis->offsetRed) + 1)
#define NUMGREEN(vis) ((vis->greenMask >> vis->offsetGreen) + 1)
#define NUMBLUE(vis) ((vis->blueMask >> vis->offsetBlue) + 1)
#define RGBMASK(vis) (vis->redMask | vis->greenMask | vis->blueMask)

/* GetNextBitsOrBreak(bits, mask, base)  -- 
 * (Suggestion: First read the macro, then read this explanation.
 *
 * Either generate the next value to OR in to a pixel or break out of this
 * while loop 
 *
 * This macro is used when we're trying to generate all 2^n combinations of
 * bits in mask.  What we're doing here is counting in binary, except that
 * the bits we use to count may not be contiguous.  This macro will be
 * called 2^n times, returning a different value in bits each time. Then
 * it will cause us to break out of a surrounding loop. (It will always be
 * called from within a while loop.)
 * On call: mask is the value we want to find all the combinations for
 * base has 1 bit set where the least significant bit of mask is set
 *
 * For example,if mask is 01010, base should be 0010 and we count like this:
 * 00010 (see this isn't so hard), 
 *     then we add base to bits and get 0100. (bits & ~mask) is (0100 & 0100) so
 *      we add that to bits getting (0100 + 0100) =
 * 01000 for our next value.
 *      then we add 0010 to get 
 * 01010 and we're done (easy as 1, 2, 3)
 */
#define GetNextBitsOrBreak(bits, mask, base)	\
	    if((bits) == (mask)) 		\
		break;		 		\
	    (bits) += (base);		 	\
	    while((bits) & ~(mask))		\
		(bits) += ((bits) & ~(mask));	
/* ID of server as client */
#define SERVER_ID	0

typedef struct 
{
	Colormap	mid;
	int		client;
	} colorResource;

/* Invariants:
 * refcnt == 0 means entry is empty
 * refcnt > 0 means entry is useable by many clients, so it can't be changed
 * refcnt == AllocPrivate means entry owned by one client only
 * fShared should only be set if refcnt == AllocPrivate, and only in red map
 */


/* Create and initialize the color map */
int 
CreateColormap (mid, pScreen, pVisual, ppcmap, alloc, client)
    Colormap	mid;		/* resource to use for this colormap */
    ScreenPtr	pScreen;
    VisualPtr	pVisual;
    ColormapPtr	*ppcmap;	
    int		alloc;		/* 1 iff all entries are allocated writeable */
    int		client;
{
    int		class, size;
    unsigned long sizebytes;
    ColormapPtr	pmap;
    register	EntryPtr	pent;
    int		i;
    register	Pixel	*ppix, **pptr;

    class = pVisual->class;
    if(!(class & DynamicClass) && (alloc != AllocNone) && (client != SERVER_ID))
	return (BadMatch);

    size = pVisual->ColormapEntries;
    sizebytes = (size * sizeof(Entry)) +
		(MAXCLIENTS * sizeof(Pixel *)) +
		(MAXCLIENTS * sizeof(int));
    if ((class | DynamicClass) == DirectColor)
	sizebytes *= 3;
    sizebytes += sizeof(ColormapRec);
    pmap = (ColormapPtr) xalloc(sizebytes);
    if (!pmap)
	return (BadAlloc);
    pmap->red = (EntryPtr)((char *)pmap + sizeof(ColormapRec));    
    sizebytes = size * sizeof(Entry);
    pmap->clientPixelsRed = (Pixel **)((char *)pmap->red + sizebytes);
    pmap->numPixelsRed = (int *)((char *)pmap->clientPixelsRed +
				 (MAXCLIENTS * sizeof(Pixel *)));
    pmap->mid = mid;
    pmap->flags = 0; 	/* start out with all flags clear */
    if(mid == pScreen->defColormap)
	pmap->flags |= IsDefault;
    pmap->pScreen = pScreen;
    pmap->pVisual = pVisual;
    pmap->class = class;
    if ((class | DynamicClass) == DirectColor)
	size = NUMRED(pVisual);
    pmap->freeRed = size;
    bzero ((char *) pmap->red, (int)sizebytes);
    bzero((char *) pmap->numPixelsRed, MAXCLIENTS * sizeof(int));
    for (pptr = &pmap->clientPixelsRed[MAXCLIENTS]; --pptr >= pmap->clientPixelsRed; )
	*pptr = (Pixel *)NULL;
    if (alloc == AllocAll)
    {
	if (class & DynamicClass)
	    pmap->flags |= AllAllocated;
	for (pent = &pmap->red[size - 1]; pent >= pmap->red; pent--)
	    pent->refcnt = AllocPrivate;
	pmap->freeRed = 0;
	ppix = (Pixel *)xalloc(size * sizeof(Pixel));
	if (!ppix)
	{
	    xfree(pmap);
	    return (BadAlloc);
	}
	pmap->clientPixelsRed[client] = ppix;
	for(i = 0; i < size; i++)
	    ppix[i] = i;
	pmap->numPixelsRed[client] = size;
    }

    if ((class | DynamicClass) == DirectColor)
    {
	pmap->freeGreen = NUMGREEN(pVisual);
	pmap->green = (EntryPtr)((char *)pmap->numPixelsRed +
				 (MAXCLIENTS * sizeof(int)));
	pmap->clientPixelsGreen = (Pixel **)((char *)pmap->green + sizebytes);
	pmap->numPixelsGreen = (int *)((char *)pmap->clientPixelsGreen +
				       (MAXCLIENTS * sizeof(Pixel *)));
	pmap->freeBlue = NUMBLUE(pVisual);
	pmap->blue = (EntryPtr)((char *)pmap->numPixelsGreen +
				(MAXCLIENTS * sizeof(int)));
	pmap->clientPixelsBlue = (Pixel **)((char *)pmap->blue + sizebytes);
	pmap->numPixelsBlue = (int *)((char *)pmap->clientPixelsBlue +
				      (MAXCLIENTS * sizeof(Pixel *)));

	bzero ((char *) pmap->green, (int)sizebytes);
	bzero ((char *) pmap->blue, (int)sizebytes);

	bcopy((char *) pmap->clientPixelsRed,
	      (char *) pmap->clientPixelsGreen,
	      MAXCLIENTS * sizeof(Pixel *));
	bcopy((char *) pmap->clientPixelsRed,
	      (char *) pmap->clientPixelsBlue,
	      MAXCLIENTS * sizeof(Pixel *));
	bzero((char *) pmap->numPixelsGreen, MAXCLIENTS * sizeof(int));
	bzero((char *) pmap->numPixelsBlue, MAXCLIENTS * sizeof(int));

	/* If every cell is allocated, mark its refcnt */
	if (alloc == AllocAll)
	{
	    size = pmap->freeGreen;
	    for(pent = &pmap->green[size-1]; pent >= pmap->green; pent--)
		pent->refcnt = AllocPrivate;
	    pmap->freeGreen = 0;
	    ppix = (Pixel *) xalloc(size * sizeof(Pixel));
	    if (!ppix)
	    {
		xfree(pmap->clientPixelsRed[client]);
		xfree(pmap);
		return(BadAlloc);
	    }
	    pmap->clientPixelsGreen[client] = ppix;
	    for(i = 0; i < size; i++)
		ppix[i] = i;
	    pmap->numPixelsGreen[client] = size;

	    size = pmap->freeBlue;
	    for(pent = &pmap->blue[size-1]; pent >= pmap->blue; pent--)
		pent->refcnt = AllocPrivate;
	    pmap->freeBlue = 0;
	    ppix = (Pixel *) xalloc(size * sizeof(Pixel));
	    if (!ppix)
	    {
		xfree(pmap->clientPixelsGreen[client]);
		xfree(pmap->clientPixelsRed[client]);
		xfree(pmap);
		return(BadAlloc);
	    }
	    pmap->clientPixelsBlue[client] = ppix;
	    for(i = 0; i < size; i++)
		ppix[i] = i;
	    pmap->numPixelsBlue[client] = size;
	}
    }
    if (!AddResource(mid, RT_COLORMAP, (pointer)pmap))
	return (BadAlloc);
    /* If the device wants a chance to initialize the colormap in any way,
     * this is it.  In specific, if this is a Static colormap, this is the
     * time to fill in the colormap's values */
    pmap->flags |= BeingCreated;
    if (!(*pScreen->CreateColormap)(pmap))
    {
	FreeResource (mid, RT_NONE);
	return BadAlloc;
    }
    pmap->flags &= ~BeingCreated;
    *ppcmap = pmap;
    return (Success);
}

int
FreeColormap (pmap, mid)
    ColormapPtr	pmap;
    Colormap	mid;
{
    int		i;
    register EntryPtr pent;

    if(CLIENT_ID(mid) != SERVER_ID)
    {
        (*pmap->pScreen->UninstallColormap) (pmap);
        WalkTree(pmap->pScreen, TellNoMap, (pointer) &mid);
    }

    /* This is the device's chance to undo anything it needs to, especially
     * to free any storage it allocated */
    (*pmap->pScreen->DestroyColormap)(pmap);

    if(pmap->clientPixelsRed)
    {
	for(i = 0; i < MAXCLIENTS; i++)
	    xfree(pmap->clientPixelsRed[i]);
    }

    if ((pmap->class == PseudoColor) || (pmap->class == GrayScale))
    {
	for(pent = &pmap->red[pmap->pVisual->ColormapEntries - 1];
	    pent >= pmap->red;
	    pent--)
	{
	    if(pent->fShared)
	    {
		if (--pent->co.shco.red->refcnt == 0)
		    xfree(pent->co.shco.red);
		if (--pent->co.shco.green->refcnt == 0)
		    xfree(pent->co.shco.green);
		if (--pent->co.shco.blue->refcnt == 0)
		    xfree(pent->co.shco.blue);
	    }
	}
    }
    if((pmap->class | DynamicClass) == DirectColor)
    {
        for(i = 0; i < MAXCLIENTS; i++)
	{
            xfree(pmap->clientPixelsGreen[i]);
            xfree(pmap->clientPixelsBlue[i]);
        }
    }
    xfree(pmap);
    return(Success);
}

/* Tell window that pmid has disappeared */
static int
TellNoMap (pwin, pmid)
    WindowPtr	pwin;
    Colormap 	*pmid;
{
    xEvent 	xE;
    if (wColormap(pwin) == *pmid)
    {
	/* This should be call to DeliverEvent */
	xE.u.u.type = ColormapNotify;
	xE.u.colormap.window = pwin->drawable.id;
	xE.u.colormap.colormap = None;
	xE.u.colormap.new = TRUE;
	xE.u.colormap.state = ColormapUninstalled;
	DeliverEvents(pwin, &xE, 1, (WindowPtr)NULL);
	if (pwin->optional) {
	    pwin->optional->colormap = None;
	    CheckWindowOptionalNeed (pwin);
	}
    }

    return (WT_WALKCHILDREN);
}

/* Tell window that pmid got uninstalled */
int
TellLostMap (pwin, pmid)
    WindowPtr	pwin;
    Colormap 	*pmid;
{
    xEvent 	xE;
    if (wColormap(pwin) == *pmid)
    {
	/* This should be call to DeliverEvent */
	xE.u.u.type = ColormapNotify;
	xE.u.colormap.window = pwin->drawable.id;
	xE.u.colormap.colormap = *pmid;
	xE.u.colormap.new = FALSE;
	xE.u.colormap.state = ColormapUninstalled;
	DeliverEvents(pwin, &xE, 1, (WindowPtr)NULL);
    }

    return (WT_WALKCHILDREN);
}

/* Tell window that pmid got installed */
int
TellGainedMap (pwin, pmid)
    WindowPtr	pwin;
    Colormap 	*pmid;
{
    xEvent 	xE;
    if (wColormap (pwin) == *pmid)
    {
	/* This should be call to DeliverEvent */
	xE.u.u.type = ColormapNotify;
	xE.u.colormap.window = pwin->drawable.id;
	xE.u.colormap.colormap = *pmid;
	xE.u.colormap.new = FALSE;
	xE.u.colormap.state = ColormapInstalled;
	DeliverEvents(pwin, &xE, 1, (WindowPtr)NULL);
    }

    return (WT_WALKCHILDREN);
}

  
int
CopyColormapAndFree (mid, pSrc, client)
    Colormap	mid;
    ColormapPtr	pSrc;
    int		client;
{
    ColormapPtr	pmap = (ColormapPtr) NULL;
    int		result, alloc, size;
    Colormap	midSrc;
    ScreenPtr	pScreen;
    VisualPtr	pVisual;

    pScreen = pSrc->pScreen;
    pVisual = pSrc->pVisual;
    midSrc = pSrc->mid;
    alloc = ((pSrc->flags & AllAllocated) && CLIENT_ID(midSrc) == client) ?
            AllocAll : AllocNone;
    size = pVisual->ColormapEntries;

    /* If the create returns non-0, it failed */
    result = CreateColormap (mid, pScreen, pVisual, &pmap, alloc, client);
    if(result != Success)
        return(result);
    if(alloc == AllocAll)
    {
	bcopy((char *)pSrc->red, (char *)pmap->red, size * sizeof(Entry));
	if((pmap->class | DynamicClass) == DirectColor)
	{
	    bcopy((char *)pSrc->green, (char *)pmap->green, size * sizeof(Entry));
	    bcopy((char *)pSrc->blue, (char *)pmap->blue, size * sizeof(Entry));
	}
	pSrc->flags &= ~AllAllocated;
	FreePixels(pSrc, client);
	UpdateColors(pmap);
	return(Success);
    }

    CopyFree(REDMAP, client, pSrc, pmap);
    if ((pmap->class | DynamicClass) == DirectColor)
    {
	CopyFree(GREENMAP, client, pSrc, pmap);
	CopyFree(BLUEMAP, client, pSrc, pmap);
    }
    if (pmap->class & DynamicClass)
	UpdateColors(pmap);
    /* XXX should worry about removing any RT_CMAPENTRY resource */
    return(Success);
}

/* Helper routine for freeing large numbers of cells from a map */
static void
CopyFree (channel, client, pmapSrc, pmapDst)
    int		channel, client;
    ColormapPtr	pmapSrc, pmapDst;
{
    int		z, npix, oldFree;
    EntryPtr	pentSrcFirst, pentDstFirst;
    EntryPtr	pentSrc, pentDst;
    Pixel	*ppix;
    int		nalloc;

    switch(channel)
    {
      case REDMAP:
	ppix = (pmapSrc->clientPixelsRed)[client];
	npix = (pmapSrc->numPixelsRed)[client];
	pentSrcFirst = pmapSrc->red;
	pentDstFirst = pmapDst->red;
	oldFree = pmapSrc->freeRed;
	break;
      case GREENMAP:
	ppix = (pmapSrc->clientPixelsGreen)[client];
	npix = (pmapSrc->numPixelsGreen)[client];
	pentSrcFirst = pmapSrc->green;
	pentDstFirst = pmapDst->green;
	oldFree = pmapSrc->freeGreen;
	break;
      case BLUEMAP:
	ppix = (pmapSrc->clientPixelsBlue)[client];
	npix = (pmapSrc->numPixelsBlue)[client];
	pentSrcFirst = pmapSrc->blue;
	pentDstFirst = pmapDst->blue;
	oldFree = pmapSrc->freeBlue;
	break;
    }
    nalloc = 0;
    if (pmapSrc->class & DynamicClass)
    {
	for(z = npix; --z >= 0; ppix++)
	{
	    /* Copy entries */
	    pentSrc = pentSrcFirst + *ppix;
	    pentDst = pentDstFirst + *ppix;
	    if (pentDst->refcnt > 0)
	    {
		pentDst->refcnt++;
	    }
	    else
	    {
		*pentDst = *pentSrc;
		nalloc++;
		if (pentSrc->refcnt > 0)
		    pentDst->refcnt = 1;
		else
		    pentSrc->fShared = FALSE;
	    }
	    FreeCell(pmapSrc, *ppix, channel);
	}
    }

    /* Note that FreeCell has already fixed pmapSrc->free{Color} */
    switch(channel)
    {
      case REDMAP:
        pmapDst->freeRed -= nalloc;
        (pmapDst->clientPixelsRed)[client] =
	    (pmapSrc->clientPixelsRed)[client];
        (pmapSrc->clientPixelsRed)[client] = (Pixel *) NULL;
        (pmapDst->numPixelsRed)[client] = (pmapSrc->numPixelsRed)[client];
        (pmapSrc->numPixelsRed)[client] = 0;
	break;
      case GREENMAP:
        pmapDst->freeGreen -= nalloc;
        (pmapDst->clientPixelsGreen)[client] =
	    (pmapSrc->clientPixelsGreen)[client];
        (pmapSrc->clientPixelsGreen)[client] = (Pixel *) NULL;
        (pmapDst->numPixelsGreen)[client] = (pmapSrc->numPixelsGreen)[client];
        (pmapSrc->numPixelsGreen)[client] = 0;
	break;
      case BLUEMAP:
        pmapDst->freeBlue -= nalloc;
        pmapDst->clientPixelsBlue[client] = pmapSrc->clientPixelsBlue[client];
        pmapSrc->clientPixelsBlue[client] = (Pixel *) NULL;
        pmapDst->numPixelsBlue[client] = pmapSrc->numPixelsBlue[client];
        pmapSrc->numPixelsBlue[client] = 0;
	break;
    }
}

/* Free the ith entry in a color map.  Must handle freeing of
 * colors allocated through AllocColorPlanes */
static void
FreeCell (pmap, i, channel)
    ColormapPtr pmap;
    Pixel i;
    int	channel;
{
    EntryPtr pent;
    int	*pCount;


    switch (channel)
    {
      case PSEUDOMAP:
      case REDMAP:
          pent = (EntryPtr) &pmap->red[i];
	  pCount = &pmap->freeRed;
	  break;
      case GREENMAP:
          pent = (EntryPtr) &pmap->green[i];
	  pCount = &pmap->freeGreen;
	  break;
      case BLUEMAP:
          pent = (EntryPtr) &pmap->blue[i];
	  pCount = &pmap->freeBlue;
	  break;
    }
    /* If it's not privately allocated and it's not time to free it, just
     * decrement the count */
    if (pent->refcnt > 1)
	pent->refcnt--;
    else
    {
        /* If the color type is shared, find the sharedcolor. If decremented
         * refcnt is 0, free the shared cell. */
        if (pent->fShared)
	{
	    if(--pent->co.shco.red->refcnt == 0)
		xfree(pent->co.shco.red);
	    if(--pent->co.shco.green->refcnt == 0)
		xfree(pent->co.shco.green);
	    if(--pent->co.shco.blue->refcnt == 0)
		xfree(pent->co.shco.blue);
	    pent->fShared = FALSE;
	}
	pent->refcnt = 0;
	*pCount += 1;
    }
}

static void
UpdateColors (pmap)
    ColormapPtr	pmap;
{
    xColorItem		*defs;
    register xColorItem *pdef;
    register EntryPtr 	pent;
    register VisualPtr	pVisual;
    int			i, n, size;

    pVisual = pmap->pVisual;
    size = pVisual->ColormapEntries;
    defs = (xColorItem *)ALLOCATE_LOCAL(size * sizeof(xColorItem));
    if (!defs)
	return;
    n = 0;
    pdef = defs;
    if (pmap->class == DirectColor)
    {
        for (i = 0; i < size; i++)
	{
	    if (!pmap->red[i].refcnt &&
		!pmap->green[i].refcnt &&
		!pmap->blue[i].refcnt)
		continue;
	    pdef->pixel = ((Pixel)i << pVisual->offsetRed) |
			  ((Pixel)i << pVisual->offsetGreen) |
			  ((Pixel)i << pVisual->offsetBlue);
	    pdef->red = pmap->red[i].co.local.red;
	    pdef->green = pmap->green[i].co.local.green;
	    pdef->blue = pmap->blue[i].co.local.blue;
	    pdef->flags = DoRed|DoGreen|DoBlue;
	    pdef++;
	    n++;
	}
    }
    else
    {
        for (i = 0, pent = pmap->red; i < size; i++, pent++)
	{
	    if (!pent->refcnt)
		continue;
	    pdef->pixel = i;
	    if(pent->fShared)
	    {
		pdef->red = pent->co.shco.red->color;
		pdef->green = pent->co.shco.green->color;
		pdef->blue = pent->co.shco.blue->color;
	    }
	    else
	    {
		pdef->red = pent->co.local.red;
		pdef->green = pent->co.local.green;
		pdef->blue = pent->co.local.blue;
	    }
	    pdef->flags = DoRed|DoGreen|DoBlue;
	    pdef++;
	    n++;
	}
    }
    if (n)
	(*pmap->pScreen->StoreColors)(pmap, n, defs);
    DEALLOCATE_LOCAL(defs);
}

/* Get a read-only color from a ColorMap (probably slow for large maps)
 * Returns by changing the value in pred, pgreen, pblue and pPix
 */
int
AllocColor (pmap, pred, pgreen, pblue, pPix, client)
    ColormapPtr		pmap;
    unsigned short 	*pred, *pgreen, *pblue;
    Pixel		*pPix;
    int			client;
{
    Pixel	pixR, pixG, pixB;
    int		entries;
    xrgb	rgb;
    int		class;
    VisualPtr	pVisual;
    int		npix;
    Pixel	*ppix;

    pVisual = pmap->pVisual;
    (*pmap->pScreen->ResolveColor) (pred, pgreen, pblue, pVisual);
    rgb.red = *pred;
    rgb.green = *pgreen;
    rgb.blue = *pblue;
    class = pmap->class;
    entries = pVisual->ColormapEntries;

    /* If the colormap is being created, then we want to be able to change
     * the colormap, even if it's a static type. Otherwise, we'd never be
     * able to initialize static colormaps
     */
    if(pmap->flags & BeingCreated)
	class |= DynamicClass;

    /* If this is one of the static storage classes, and we're not initializing
     * it, the best we can do is to find the closest color entry to the
     * requested one and return that.
     */
    switch (class) {
    case StaticColor:
    case StaticGray:
	/* Look up all three components in the same pmap */
	*pPix = pixR = FindBestPixel(pmap->red, entries, &rgb, PSEUDOMAP);
	*pred = pmap->red[pixR].co.local.red;
	*pgreen = pmap->red[pixR].co.local.green;
	*pblue = pmap->red[pixR].co.local.blue;
	npix = pmap->numPixelsRed[client];
	ppix = (Pixel *) xrealloc(pmap->clientPixelsRed[client],
				  (npix + 1) * sizeof(Pixel));
	if (!ppix)
	    return (BadAlloc);
	ppix[npix] = pixR;
	pmap->clientPixelsRed[client] = ppix;
	pmap->numPixelsRed[client]++;
	break;

    case TrueColor:
	/* Look up each component in its own map, then OR them together */
	pixR = FindBestPixel(pmap->red, NUMRED(pVisual), &rgb, REDMAP);
	pixG = FindBestPixel(pmap->green, NUMGREEN(pVisual), &rgb, GREENMAP);
	pixB = FindBestPixel(pmap->blue, NUMBLUE(pVisual), &rgb, BLUEMAP);
	*pPix = (pixR << pVisual->offsetRed) |
		(pixG << pVisual->offsetGreen) |
		(pixB << pVisual->offsetBlue);
	*pred = pmap->red[pixR].co.local.red;
	*pgreen = pmap->green[pixG].co.local.green;
	*pblue = pmap->blue[pixB].co.local.blue;
	npix = pmap->numPixelsRed[client];
	ppix = (Pixel *) xrealloc(pmap->clientPixelsRed[client],
				  (npix + 1) * sizeof(Pixel));
	if (!ppix)
	    return (BadAlloc);
	ppix[npix] = pixR;
	pmap->clientPixelsRed[client] = ppix;
	npix = pmap->numPixelsGreen[client];
	ppix = (Pixel *) xrealloc(pmap->clientPixelsGreen[client],
				  (npix + 1) * sizeof(Pixel));
	if (!ppix)
	    return (BadAlloc);
	ppix[npix] = pixG;
	pmap->clientPixelsGreen[client] = ppix;
	npix = pmap->numPixelsBlue[client];
	ppix = (Pixel *) xrealloc(pmap->clientPixelsBlue[client],
				  (npix + 1) * sizeof(Pixel));
	if (!ppix)
	    return (BadAlloc);
	ppix[npix] = pixB;
	pmap->clientPixelsBlue[client] = ppix;
	pmap->numPixelsRed[client]++;
	pmap->numPixelsGreen[client]++;
	pmap->numPixelsBlue[client]++;
	break;

    case GrayScale:
    case PseudoColor:
	if (FindColor(pmap, pmap->red, entries, &rgb, pPix, PSEUDOMAP,
		      client, AllComp) != Success)
	    return (BadAlloc);
        break;

    case DirectColor:
	pixR = (*pPix & pVisual->redMask) >> pVisual->offsetRed; 
	if (FindColor(pmap, pmap->red, NUMRED(pVisual), &rgb, &pixR, REDMAP,
		      client, RedComp) != Success)
	    return (BadAlloc);
	pixG = (*pPix & pVisual->greenMask) >> pVisual->offsetGreen; 
	if (FindColor(pmap, pmap->green, NUMGREEN(pVisual), &rgb, &pixG,
		      GREENMAP, client, GreenComp) != Success)
	{
	    (void)FreeCo(pmap, client, REDMAP, 1, &pixR, (Pixel)0);
	    return (BadAlloc);
	}
	pixB = (*pPix & pVisual->blueMask) >> pVisual->offsetBlue; 
	if (FindColor(pmap, pmap->blue, NUMBLUE(pVisual), &rgb, &pixB, BLUEMAP,
		      client, BlueComp) != Success)
	{
	    (void)FreeCo(pmap, client, GREENMAP, 1, &pixG, (Pixel)0);
	    (void)FreeCo(pmap, client, REDMAP, 1, &pixR, (Pixel)0);
	    return (BadAlloc);
	}
	*pPix = pixR | pixG | pixB;
	break;
    }

    /* if this is the client's first pixel in this colormap, tell the
     * resource manager that the client has pixels in this colormap which
     * should be freed when the client dies */
    if ((pmap->numPixelsRed[client] == 1) &&
	(CLIENT_ID(pmap->mid) != client) &&
	!(pmap->flags & BeingCreated))
    {
	colorResource	*pcr;

	pcr = (colorResource *) xalloc(sizeof(colorResource));
	if (!pcr)
	{
	    (void)FreeColors(pmap, client, 1, pPix, (Pixel)0);
	    return (BadAlloc);
	}
	pcr->mid = pmap->mid;
	pcr->client = client;
	if (!AddResource(FakeClientID(client), RT_CMAPENTRY, (pointer)pcr))
	    return (BadAlloc);
    }
    return (Success);
}

/*
 * FakeAllocColor -- fake an AllocColor request by
 * returning a free pixel if availible, otherwise returning
 * the closest matching pixel.  This is used by the mi
 * software sprite code to recolor cursors.  A nice side-effect
 * is that this routine will never return failure.
 */
void
FakeAllocColor (pmap, item)
    register ColormapPtr pmap;
    register xColorItem  *item;
{
    Pixel	pixR, pixG, pixB;
    int		entries;
    xrgb	rgb;
    int		class;
    register VisualPtr	pVisual;

    pVisual = pmap->pVisual;
    rgb.red = item->red;
    rgb.green = item->green;
    rgb.blue = item->blue;
    (*pmap->pScreen->ResolveColor) (&rgb.red, &rgb.green, &rgb.blue, pVisual);
    class = pmap->class;
    entries = pVisual->ColormapEntries;

    switch (class) {
    case GrayScale:
    case PseudoColor:
	if (FindColor(pmap, pmap->red, entries, &rgb, &item->pixel, PSEUDOMAP,
		      -1, AllComp) == Success)
	    break;
	/* FALLTHROUGH */
    case StaticColor:
    case StaticGray:
	item->pixel = FindBestPixel(pmap->red, entries, &rgb, PSEUDOMAP);
	break;

    case DirectColor:
	/* Look up each component in its own map, then OR them together */
	pixR = (item->pixel & pVisual->redMask) >> pVisual->offsetRed; 
	pixG = (item->pixel & pVisual->greenMask) >> pVisual->offsetGreen; 
	pixB = (item->pixel & pVisual->blueMask) >> pVisual->offsetBlue; 
	if (FindColor(pmap, pmap->red, NUMRED(pVisual), &rgb, &pixR, REDMAP,
		      -1, RedComp) != Success)
	    pixR = FindBestPixel(pmap->red, NUMRED(pVisual), &rgb, REDMAP)
			<< pVisual->offsetRed;
	if (FindColor(pmap, pmap->green, NUMGREEN(pVisual), &rgb, &pixG,
		      GREENMAP, -1, GreenComp) != Success)
	    pixG = FindBestPixel(pmap->green, NUMGREEN(pVisual), &rgb,
				 GREENMAP) << pVisual->offsetGreen;
	if (FindColor(pmap, pmap->blue, NUMBLUE(pVisual), &rgb, &pixB, BLUEMAP,
		      -1, BlueComp) != Success)
	    pixB = FindBestPixel(pmap->blue, NUMBLUE(pVisual), &rgb, BLUEMAP)
			<< pVisual->offsetBlue;
	item->pixel = pixR | pixG | pixB;
	break;

    case TrueColor:
	/* Look up each component in its own map, then OR them together */
	pixR = FindBestPixel(pmap->red, NUMRED(pVisual), &rgb, REDMAP);
	pixG = FindBestPixel(pmap->green, NUMGREEN(pVisual), &rgb, GREENMAP);
	pixB = FindBestPixel(pmap->blue, NUMBLUE(pVisual), &rgb, BLUEMAP);
	item->pixel = (pixR << pVisual->offsetRed) |
		      (pixG << pVisual->offsetGreen) |
		      (pixB << pVisual->offsetBlue);
	break;
    }
}

/* free a pixel value obtained from FakeAllocColor */
void
FakeFreeColor(pmap, pixel)
    register ColormapPtr pmap;
    Pixel pixel;
{
    register VisualPtr pVisual;
    Pixel pixR, pixG, pixB;

    switch (pmap->class) {
    case GrayScale:
    case PseudoColor:
	if (pmap->red[pixel].refcnt == AllocTemporary)
	    pmap->red[pixel].refcnt = 0;
	break;
    case DirectColor:
	pVisual = pmap->pVisual;
	pixR = (pixel & pVisual->redMask) >> pVisual->offsetRed; 
	pixG = (pixel & pVisual->greenMask) >> pVisual->offsetGreen; 
	pixB = (pixel & pVisual->blueMask) >> pVisual->offsetBlue; 
	if (pmap->red[pixR].refcnt == AllocTemporary)
	    pmap->red[pixR].refcnt = 0;
	if (pmap->green[pixG].refcnt == AllocTemporary)
	    pmap->green[pixG].refcnt = 0;
	if (pmap->blue[pixB].refcnt == AllocTemporary)
	    pmap->blue[pixB].refcnt = 0;
	break;
    }
}

typedef unsigned short	BigNumUpper;
typedef unsigned long	BigNumLower;

#define BIGNUMLOWERBITS	24
#define BIGNUMUPPERBITS	16
#define BIGNUMLOWER (1 << BIGNUMLOWERBITS)
#define BIGNUMUPPER (1 << BIGNUMUPPERBITS)
#define UPPERPART(i)	((i) >> BIGNUMLOWERBITS)
#define LOWERPART(i)	((i) & (BIGNUMLOWER - 1))

typedef struct _bignum {
    BigNumUpper	upper;
    BigNumLower	lower;
} BigNumRec, *BigNumPtr;

#define BigNumGreater(x,y) ((x)->upper > (y)->upper ||\
			    (x)->upper == (y)->upper && (x)->lower > (y)->lower)

#define UnsignedToBigNum(u,r)	(((r)->upper = UPPERPART(u)), \
				 ((r)->lower = LOWERPART(u)))

#define MaxBigNum(r)		(((r)->upper = BIGNUMUPPER-1), \
				 ((r)->lower = BIGNUMLOWER-1))

static void
BigNumAdd (x, y, r)
    BigNumPtr	x, y, r;
{
    BigNumLower	lower, carry = 0;

    lower = x->lower + y->lower;
    if (lower >= BIGNUMLOWER) {
	lower -= BIGNUMLOWER;
	carry = 1;
    }
    r->lower = lower;
    r->upper = x->upper + y->upper + carry;
}

static Pixel
FindBestPixel(pentFirst, size, prgb, channel)
    EntryPtr	pentFirst;
    int		size;
    xrgb	*prgb;
    int		channel;
{
    EntryPtr	pent;
    Pixel	pixel, final;
    long	dr, dg, db;
    unsigned long   sq;
    BigNumRec	minval, sum, temp;

    final = 0;
    MaxBigNum(&minval);
    /* look for the minimal difference */
    for (pent = pentFirst, pixel = 0; pixel < size; pent++, pixel++)
    {
	dr = dg = db = 0;
	switch(channel)
	{
	  case PSEUDOMAP:
	      dg = pent->co.local.green - prgb->green;
	      db = pent->co.local.blue - prgb->blue;
	      /* FALLTHROUGH */
	  case REDMAP:
	      dr = pent->co.local.red - prgb->red;
	      break;
	  case GREENMAP:
	      dg = pent->co.local.green - prgb->green;
	      break;
	  case BLUEMAP:
	      db = pent->co.local.blue - prgb->blue;
	      break;
	}
	sq = dr * dr;
	UnsignedToBigNum (sq, &sum);
	sq = dg * dg;
	UnsignedToBigNum (sq, &temp);
	BigNumAdd (&sum, &temp, &sum);
	sq = db * db;
	UnsignedToBigNum (sq, &temp);
	BigNumAdd (&sum, &temp, &sum);
	if (BigNumGreater (&minval, &sum))
	{
	    final = pixel;
	    minval = sum;
	}
    }
    return(final);
}

/* Tries to find a color in pmap that exactly matches the one requested in prgb
 * if it can't it allocates one.
 * Starts looking at pentFirst + *pPixel, so if you want a specific pixel,
 * load *pPixel with that value, otherwise set it to 0
 */
int
FindColor (pmap, pentFirst, size, prgb, pPixel, channel, client, comp)
    ColormapPtr	pmap;
    EntryPtr	pentFirst;
    int		size;
    xrgb	*prgb;
    Pixel	*pPixel;
    int		channel;
    int		client;
    int		(*comp) ();
{
    EntryPtr	pent;
    Bool	foundFree;
    Pixel	pixel, Free;
    int		npix, count, *nump;
    Pixel	**pixp, *ppix;
    xColorItem	def;

    foundFree = FALSE;

    if((pixel = *pPixel) >= size)
	pixel = 0;
    /* see if there is a match, and also look for a free entry */
    for (pent = pentFirst + pixel, count = size; --count >= 0; )
    {
        if (pent->refcnt > 0)
	{
    	    if ((*comp) (pent, prgb))
	    {
    	        pent->refcnt++;
		*pPixel = pixel;
		switch(channel)
		{
		  case REDMAP:
		    *pPixel <<= pmap->pVisual->offsetRed;
		    break;
		  case PSEUDOMAP:
		    break;
		  case GREENMAP:
		    *pPixel <<= pmap->pVisual->offsetGreen;
		    break;
		  case BLUEMAP:
		    *pPixel <<= pmap->pVisual->offsetBlue;
		    break;
		}
		goto gotit;
    	    }
        }
	else if (!foundFree && pent->refcnt == 0)
	{
	    Free = pixel;
	    foundFree = TRUE;
	    /* If we're initializing the colormap, then we are looking for
	     * the first free cell we can find, not to minimize the number
	     * of entries we use.  So don't look any further. */
	    if(pmap->flags & BeingCreated)
		break;
	}
	pixel++;
	if(pixel >= size)
	{
	    pent = pentFirst;
	    pixel = 0;
	}
	else
	    pent++;
    }

    /* If we got here, we didn't find a match.  If we also didn't find
     * a free entry, we're out of luck.  Otherwise, we'll usurp a free
     * entry and fill it in */
    if (!foundFree)
	return (BadAlloc);
    pent = pentFirst + Free;
    pent->fShared = FALSE;
    pent->refcnt = (client >= 0) ? 1 : AllocTemporary;

    switch (channel)
    {
      case PSEUDOMAP:
        pent->co.local.red = prgb->red;
        pent->co.local.green = prgb->green;
        pent->co.local.blue = prgb->blue;
        def.red = prgb->red;
	def.green = prgb->green;
	def.blue = prgb->blue;
	def.flags = (DoRed|DoGreen|DoBlue);
	if (client >= 0)
	    pmap->freeRed--;
	def.pixel = Free;
	break;

      case REDMAP:
        pent->co.local.red = prgb->red;
        def.red = prgb->red;
	def.green = pmap->green[0].co.local.green;
	def.blue = pmap->blue[0].co.local.blue;
	def.flags = DoRed;
	if (client >= 0)
	    pmap->freeRed--;
	def.pixel = Free << pmap->pVisual->offsetRed;
	break;

      case GREENMAP:
	pent->co.local.green = prgb->green;
	def.red = pmap->red[0].co.local.red;
        def.green = prgb->green;
	def.blue = pmap->blue[0].co.local.blue;
	def.flags = DoGreen;
	if (client >= 0)
	    pmap->freeGreen--;
	def.pixel = Free << pmap->pVisual->offsetGreen;
	break;

      case BLUEMAP:
	pent->co.local.blue = prgb->blue;
	def.red = pmap->red[0].co.local.red;
	def.green = pmap->green[0].co.local.green;
	def.blue = prgb->blue;
	def.flags = DoBlue;
	if (client >= 0)
	    pmap->freeBlue--;
	def.pixel = Free << pmap->pVisual->offsetBlue;
	break;
    }
    (*pmap->pScreen->StoreColors) (pmap, 1, &def);
    pixel = Free;	
    *pPixel = def.pixel;

gotit:
    if (pmap->flags & BeingCreated || client == -1)
	return(Success);
    /* Now remember the pixel, for freeing later */
    switch (channel)
    {
      case PSEUDOMAP:
      case REDMAP:
	nump = pmap->numPixelsRed;
	pixp = pmap->clientPixelsRed;
	break;

      case GREENMAP:
	nump = pmap->numPixelsGreen;
	pixp = pmap->clientPixelsGreen;
	break;

      case BLUEMAP:
	nump = pmap->numPixelsBlue;
	pixp = pmap->clientPixelsBlue;
	break;
    }
    npix = nump[client];
    ppix = (Pixel *) xrealloc (pixp[client], (npix + 1) * sizeof(Pixel));
    if (!ppix)
    {
	pent->refcnt--;
	if (!pent->fShared)
	    switch (channel)
	    {
	      case PSEUDOMAP:
	      case REDMAP:
		pmap->freeRed++;
		break;
	      case GREENMAP:
		pmap->freeGreen++;
		break;
	      case BLUEMAP:
		pmap->freeBlue++;
		break;
	    }
	return(BadAlloc);
    }
    ppix[npix] = pixel;
    pixp[client] = ppix;
    nump[client]++;

    return(Success);
}

/* Comparison functions -- passed to FindColor to determine if an
 * entry is already the color we're looking for or not */
static int
AllComp (pent, prgb)
    EntryPtr	pent;
    xrgb	*prgb;
{
    if((pent->co.local.red == prgb->red) &&
       (pent->co.local.green == prgb->green) &&
       (pent->co.local.blue == prgb->blue) )
       return (1);
    return (0);
}

static int
RedComp (pent, prgb)
    EntryPtr	pent;
    xrgb	*prgb;
{
    if (pent->co.local.red == prgb->red) 
	return (1);
    return (0);
}

static int
GreenComp (pent, prgb)
    EntryPtr	pent;
    xrgb	*prgb;
{
    if (pent->co.local.green == prgb->green) 
	return (1);
    return (0);
}

static int
BlueComp (pent, prgb)
    EntryPtr	pent;
    xrgb	*prgb;
{
    if (pent->co.local.blue == prgb->blue) 
	return (1);
    return (0);
}


/* Read the color value of a cell */

int
QueryColors (pmap, count, ppixIn, prgbList)
    ColormapPtr	pmap;
    int		count;
    Pixel	*ppixIn;
    xrgb	*prgbList;
{
    Pixel	*ppix, pixel;
    xrgb	*prgb;
    VisualPtr	pVisual;
    EntryPtr	pent;
    Pixel	i;
    int		errVal = Success;

    pVisual = pmap->pVisual;
    if ((pmap->class | DynamicClass) == DirectColor)
    {
	int numred, numgreen, numblue;
	Pixel rgbbad;

	numred = NUMRED(pVisual);
	numgreen = NUMGREEN(pVisual);
	numblue = NUMBLUE(pVisual);
	rgbbad = ~RGBMASK(pVisual);
	for( ppix = ppixIn, prgb = prgbList; --count >= 0; ppix++, prgb++)
	{
	    pixel = *ppix;
	    if (pixel & rgbbad) {
		clientErrorValue = pixel;
		errVal =  BadValue;
		continue;
	    }
	    i  = (pixel & pVisual->redMask) >> pVisual->offsetRed;
	    if (i >= numred)
	    {
		clientErrorValue = pixel;
		errVal =  BadValue;
		continue;
	    }
	    prgb->red = pmap->red[i].co.local.red;
	    i  = (pixel & pVisual->greenMask) >> pVisual->offsetGreen;
	    if (i >= numgreen)
	    {
		clientErrorValue = pixel;
		errVal =  BadValue;
		continue;
	    }
	    prgb->green = pmap->green[i].co.local.green;
	    i  = (pixel & pVisual->blueMask) >> pVisual->offsetBlue;
	    if (i >= numblue)
	    {
		clientErrorValue = pixel;
		errVal =  BadValue;
		continue;
	    }
	    prgb->blue = pmap->blue[i].co.local.blue;
	}
    }
    else
    {
	for( ppix = ppixIn, prgb = prgbList; --count >= 0; ppix++, prgb++)
	{
	    pixel = *ppix;
	    if (pixel >= pVisual->ColormapEntries)
	    {
		clientErrorValue = pixel;
		errVal = BadValue;
	    }
	    else
	    {
		pent = (EntryPtr)&pmap->red[pixel];
		if (pent->fShared)
		{
		    prgb->red = pent->co.shco.red->color;
		    prgb->green = pent->co.shco.green->color;
		    prgb->blue = pent->co.shco.blue->color;
		}
		else
		{
		    prgb->red = pent->co.local.red;
		    prgb->green = pent->co.local.green;
		    prgb->blue = pent->co.local.blue;
		}
	    }
	}
    }
    return (errVal);
}

static void
FreePixels(pmap, client)
    register ColormapPtr	pmap;
    register int 		client;
{
    register Pixel		*ppix, *ppixStart;
    register int 		n;
    int				class;

    class = pmap->class;
    ppixStart = pmap->clientPixelsRed[client];
    if (class & DynamicClass)
	for (ppix = ppixStart, n = pmap->numPixelsRed[client]; --n >= 0; )
	    FreeCell(pmap, *ppix++, REDMAP);
    xfree(ppixStart);
    pmap->clientPixelsRed[client] = (Pixel *) NULL;
    pmap->numPixelsRed[client] = 0;
    if ((class | DynamicClass) == DirectColor) 
    {
        ppixStart = pmap->clientPixelsGreen[client];
	if (class & DynamicClass)
	    for (ppix = ppixStart, n = pmap->numPixelsGreen[client]; --n >= 0;)
		FreeCell(pmap, *ppix++, GREENMAP);
	xfree(ppixStart);
	pmap->clientPixelsGreen[client] = (Pixel *) NULL;
	pmap->numPixelsGreen[client] = 0;

        ppixStart = pmap->clientPixelsBlue[client];
	if (class & DynamicClass)
	    for (ppix = ppixStart, n = pmap->numPixelsBlue[client]; --n >= 0; )
		FreeCell(pmap, *ppix++, BLUEMAP);
	xfree(ppixStart);
	pmap->clientPixelsBlue[client] = (Pixel *) NULL;
	pmap->numPixelsBlue[client] = 0;
    }
}

/* Free all of a client's colors and cells */
/*ARGSUSED*/
void
FreeClientPixels (pcr, fakeid)
    colorResource *pcr;
    XID	fakeid;
{
    ColormapPtr pmap;

    pmap = (ColormapPtr) LookupIDByType(pcr->mid, RT_COLORMAP);
    if (pmap)
	FreePixels(pmap, pcr->client);
    xfree(pcr);
}

int
AllocColorCells (client, pmap, colors, planes, contig, ppix, masks)
    int		client;
    ColormapPtr	pmap;
    int		colors, planes;
    Bool	contig;
    Pixel	*ppix;
    Pixel	*masks;
{
    Pixel	rmask, gmask, bmask, *ppixFirst, r, g, b;
    int		n, class;
    int		ok;
    int		oldcount;
    colorResource *pcr = (colorResource *)NULL;

    class = pmap->class;
    if (!(class & DynamicClass))
	return (BadAlloc); /* Shouldn't try on this type */
    oldcount = pmap->numPixelsRed[client];
    if (pmap->class == DirectColor)
	oldcount += pmap->numPixelsGreen[client] + pmap->numPixelsBlue[client];
    if (!oldcount && (CLIENT_ID(pmap->mid) != client))
    {
	pcr = (colorResource *) xalloc(sizeof(colorResource));
	if (!pcr)
	    return (BadAlloc);
    }

    if (pmap->class == DirectColor)
    {
        ok = AllocDirect (client, pmap, colors, planes, planes, planes,
			  contig, ppix, &rmask, &gmask, &bmask);
	if(ok == Success)
	{
	    for (r = g = b = 1, n = planes; --n >= 0; r += r, g += g, b += b)
	    {
		while(!(rmask & r))
		    r += r;
		while(!(gmask & g))
		    g += g;
		while(!(bmask & b))
		    b += b;
		*masks++ = r | g | b;
	    }
	}
    }
    else
    {
        ok = AllocPseudo (client, pmap, colors, planes, contig, ppix, &rmask,
			  &ppixFirst);
	if(ok == Success)
	{
	    for (r = 1, n = planes; --n >= 0; r += r)
	    {
		while(!(rmask & r))
		    r += r;
		*masks++ = r;
	    }
	}
    }

    /* if this is the client's first pixels in this colormap, tell the
     * resource manager that the client has pixels in this colormap which
     * should be freed when the client dies */
    if ((ok == Success) && pcr)
    {
	pcr->mid = pmap->mid;
	pcr->client = client;
	if (!AddResource(FakeClientID(client), RT_CMAPENTRY, (pointer)pcr))
	    ok = BadAlloc;
    } else if (pcr)
	xfree(pcr);

    return (ok);
}


int
AllocColorPlanes (client, pmap, colors, r, g, b, contig, pixels,
		  prmask, pgmask, pbmask)
    int		client;
    ColormapPtr	pmap;
    int		colors, r, g, b;
    Bool	contig;
    Pixel	*pixels;
    Pixel	*prmask, *pgmask, *pbmask;
{
    int		ok;
    Pixel	mask, *ppixFirst;
    register Pixel shift;
    register int i;
    int		class;
    int		oldcount;
    colorResource *pcr = (colorResource *)NULL;

    class = pmap->class;
    if (!(class & DynamicClass))
	return (BadAlloc); /* Shouldn't try on this type */
    oldcount = pmap->numPixelsRed[client];
    if (class == DirectColor)
	oldcount += pmap->numPixelsGreen[client] + pmap->numPixelsBlue[client];
    if (!oldcount && (CLIENT_ID(pmap->mid) != client))
    {
	pcr = (colorResource *) xalloc(sizeof(colorResource));
	if (!pcr)
	    return (BadAlloc);
    }

    if (class == DirectColor)
    {
        ok = AllocDirect (client, pmap, colors, r, g, b, contig, pixels,
			  prmask, pgmask, pbmask);
    }
    else
    {
	/* Allocate the proper pixels */
	/* XXX This is sort of bad, because of contig is set, we force all
	 * r + g + b bits to be contiguous.  Should only force contiguity
	 * per mask 
	 */
        ok = AllocPseudo (client, pmap, colors, r + g + b, contig, pixels,
			  &mask, &ppixFirst);

	if(ok == Success)
	{
	    /* now split that mask into three */
	    *prmask = *pgmask = *pbmask = 0;
	    shift = 1;
	    for (i = r; --i >= 0; shift += shift)
	    {
		while (!(mask & shift))
		    shift += shift;
		*prmask |= shift;
	    }
	    for (i = g; --i >= 0; shift += shift)
	    {
		while (!(mask & shift))
		    shift += shift;
		*pgmask |= shift;
	    }
	    for (i = b; --i >= 0; shift += shift)
	    {
		while (!(mask & shift))
		    shift += shift;
		*pbmask |= shift;
	    }

	    /* set up the shared color cells */
	    if (!AllocShared(pmap, pixels, colors, r, g, b,
			     *prmask, *pgmask, *pbmask, ppixFirst))
	    {
		(void)FreeColors(pmap, client, colors, pixels, mask);
		ok = BadAlloc;
	    }
	}
    }

    /* if this is the client's first pixels in this colormap, tell the
     * resource manager that the client has pixels in this colormap which
     * should be freed when the client dies */
    if ((ok == Success) && pcr)
    {
	pcr->mid = pmap->mid;
	pcr->client = client;
	if (!AddResource(FakeClientID(client), RT_CMAPENTRY, (pointer)pcr))
	    ok = BadAlloc;
    } else if (pcr)
	xfree(pcr);

    return (ok);
}

static int
AllocDirect (client, pmap, c, r, g, b, contig, pixels, prmask, pgmask, pbmask)
    int		client;
    ColormapPtr	pmap;
    int		c, r, g, b;
    Bool	contig;
    Pixel	*pixels;
    Pixel	*prmask, *pgmask, *pbmask;
{
    Pixel	*ppixRed, *ppixGreen, *ppixBlue;
    Pixel	*ppix, *pDst, *p;
    int		npix, npixR, npixG, npixB;
    Bool	okR, okG, okB;
    Pixel	*rpix, *gpix, *bpix;

    npixR = c << r;
    npixG = c << g;
    npixB = c << b;
    if ((r >= 32) || (g >= 32) || (b >= 32) ||
	(npixR > pmap->freeRed) || (npixR < c) ||
	(npixG > pmap->freeGreen) || (npixG < c) ||
	(npixB > pmap->freeBlue) || (npixB < c))
	return BadAlloc;

    /* start out with empty pixels */
    for(p = pixels; p < pixels + c; p++)
	*p = 0;

    ppixRed = (Pixel *)ALLOCATE_LOCAL(npixR * sizeof(Pixel));
    ppixGreen = (Pixel *)ALLOCATE_LOCAL(npixG * sizeof(Pixel));
    ppixBlue = (Pixel *)ALLOCATE_LOCAL(npixB * sizeof(Pixel));
    if (!ppixRed || !ppixGreen || !ppixBlue)
    {
	if (ppixBlue) DEALLOCATE_LOCAL(ppixBlue);
	if (ppixGreen) DEALLOCATE_LOCAL(ppixGreen);
	if (ppixRed) DEALLOCATE_LOCAL(ppixRed);
	return(BadAlloc);
    }

    okR = AllocCP(pmap, pmap->red, c, r, contig, ppixRed, prmask);
    okG = AllocCP(pmap, pmap->green, c, g, contig, ppixGreen, pgmask);
    okB = AllocCP(pmap, pmap->blue, c, b, contig, ppixBlue, pbmask);

    if (okR && okG && okB)
    {
	rpix = (Pixel *) xrealloc(pmap->clientPixelsRed[client],
				  (pmap->numPixelsRed[client] + (c << r)) *
				  sizeof(Pixel));
	if (rpix)
	    pmap->clientPixelsRed[client] = rpix;
	gpix = (Pixel *) xrealloc(pmap->clientPixelsGreen[client],
				  (pmap->numPixelsGreen[client] + (c << g)) *
				  sizeof(Pixel));
	if (gpix)
	    pmap->clientPixelsGreen[client] = gpix;
	bpix = (Pixel *) xrealloc(pmap->clientPixelsBlue[client],
				  (pmap->numPixelsBlue[client] + (c << b)) *
				  sizeof(Pixel));
	if (bpix)
	    pmap->clientPixelsBlue[client] = bpix;
    }

    if (!okR || !okG || !okB || !rpix || !gpix || !bpix)
    {
	if (okR)
	    for(ppix = ppixRed, npix = npixR; --npix >= 0; ppix++)
		pmap->red[*ppix].refcnt = 0;
	if (okG)
	    for(ppix = ppixGreen, npix = npixG; --npix >= 0; ppix++)
		pmap->green[*ppix].refcnt = 0;
	if (okB)
	    for(ppix = ppixBlue, npix = npixB; --npix >= 0; ppix++)
		pmap->blue[*ppix].refcnt = 0;
	DEALLOCATE_LOCAL(ppixBlue);
	DEALLOCATE_LOCAL(ppixGreen);
	DEALLOCATE_LOCAL(ppixRed);
	return(BadAlloc);
    }

    *prmask <<= pmap->pVisual->offsetRed;
    *pgmask <<= pmap->pVisual->offsetGreen;
    *pbmask <<= pmap->pVisual->offsetBlue;

    ppix = rpix + pmap->numPixelsRed[client];
    for (pDst = pixels, p = ppixRed; p < ppixRed + npixR; p++)
    {
	*ppix++ = *p;
	if(p < ppixRed + c)
	    *pDst++ |= *p << pmap->pVisual->offsetRed;
    }
    pmap->numPixelsRed[client] += npixR;
    pmap->freeRed -= npixR;

    ppix = gpix + pmap->numPixelsGreen[client];
    for (pDst = pixels, p = ppixGreen; p < ppixGreen + npixG; p++)
    {
	*ppix++ = *p;
	if(p < ppixGreen + c)
	    *pDst++ |= *p << pmap->pVisual->offsetGreen;
    }
    pmap->numPixelsGreen[client] += npixG;
    pmap->freeGreen -= npixG;

    ppix = bpix + pmap->numPixelsBlue[client];
    for (pDst = pixels, p = ppixBlue; p < ppixBlue + npixB; p++)
    {
	*ppix++ = *p;
	if(p < ppixBlue + c)
	    *pDst++ |= *p << pmap->pVisual->offsetBlue;
    }
    pmap->numPixelsBlue[client] += npixB;
    pmap->freeBlue -= npixB;

    DEALLOCATE_LOCAL(ppixBlue);
    DEALLOCATE_LOCAL(ppixGreen);
    DEALLOCATE_LOCAL(ppixRed);

    return (Success);
}

static int
AllocPseudo (client, pmap, c, r, contig, pixels, pmask, pppixFirst)
    int		client;
    ColormapPtr	pmap;
    int		c, r;
    Bool	contig;
    Pixel	*pixels;
    Pixel	*pmask;
    Pixel	**pppixFirst;
{
    Pixel	*ppix, *p, *pDst, *ppixTemp;
    int		npix;
    Bool	ok;

    npix = c << r;
    if ((r >= 32) || (npix > pmap->freeRed) || (npix < c))
	return(BadAlloc);
    if(!(ppixTemp = (Pixel *)ALLOCATE_LOCAL(npix * sizeof(Pixel))))
	return(BadAlloc);
    ok = AllocCP(pmap, pmap->red, c, r, contig, ppixTemp, pmask);

    if (ok)
    {

	/* all the allocated pixels are added to the client pixel list,
	 * but only the unique ones are returned to the client */
	ppix = (Pixel *)xrealloc(pmap->clientPixelsRed[client],
			 (pmap->numPixelsRed[client] + npix) * sizeof(Pixel));
	if (!ppix)
	{
	    for (p = ppixTemp; p < ppixTemp + npix; p++)
		pmap->red[*p].refcnt = 0;
	    return (BadAlloc);
	}
	pmap->clientPixelsRed[client] = ppix;
	ppix += pmap->numPixelsRed[client];
	*pppixFirst = ppix;
	pDst = pixels;
	for (p = ppixTemp; p < ppixTemp + npix; p++)
	{
	    *ppix++ = *p;
	    if(p < ppixTemp + c)
	        *pDst++ = *p;
	}
	pmap->numPixelsRed[client] += npix;
	pmap->freeRed -= npix;
    }
    DEALLOCATE_LOCAL(ppixTemp);
    return (ok ? Success : BadAlloc);
}

/* Allocates count << planes pixels from colormap pmap for client. If
 * contig, then the plane mask is made of consecutive bits.  Returns
 * all count << pixels in the array pixels. The first count of those
 * pixels are the unique pixels.  *pMask has the mask to Or with the
 * unique pixels to get the rest of them.
 *
 * Returns True iff all pixels could be allocated 
 * All cells allocated will have refcnt set to AllocPrivate and shared to FALSE
 * (see AllocShared for why we care)
 */
static Bool
AllocCP (pmap, pentFirst, count, planes, contig, pixels, pMask)
    ColormapPtr	pmap;
    EntryPtr	pentFirst;
    int		count, planes;
    Bool	contig;
    Pixel	*pixels, *pMask;
    
{
    EntryPtr	ent;
    Pixel	pixel, base, entries, maxp, save;
    int		dplanes, found;
    Pixel	*ppix;
    Pixel	mask;
    Pixel	finalmask;

    dplanes = pmap->pVisual->nplanes;

    /* Easy case.  Allocate pixels only */
    if (planes == 0)
    {
        /* allocate writable entries */
	ppix = pixels;
        ent = pentFirst;
        pixel = 0;
        while (--count >= 0)
	{
            /* Just find count unallocated cells */
    	    while (ent->refcnt)
	    {
    	        ent++;
    	        pixel++;
    	    }
    	    ent->refcnt = AllocPrivate;
    	    *ppix++ = pixel;
	    ent->fShared = FALSE;
        }
        *pMask = 0;
        return (TRUE);
    }
    else if (planes > dplanes)
    {
	return (FALSE);
    }

    /* General case count pixels * 2 ^ planes cells to be allocated */

    /* make room for new pixels */
    ent = pentFirst;

    /* first try for contiguous planes, since it's fastest */
    for (mask = (((Pixel)1) << planes) - 1, base = 1, dplanes -= (planes - 1);
         --dplanes >= 0;
         mask += mask, base += base)
    {
        ppix = pixels;
        found = 0;
        pixel = 0;
        entries = pmap->pVisual->ColormapEntries - mask;
        while (pixel < entries)
	{
    	    save = pixel;
    	    maxp = pixel + mask + base;
    	    /* check if all are free */
    	    while (pixel != maxp && ent[pixel].refcnt == 0)
    	        pixel += base;
	    if (pixel == maxp)
		{
		    /* this one works */
		    *ppix++ = save;
		    found++;
		    if (found == count)
		    {
			/* found enough, allocate them all */
			while (--count >= 0)
			{
			    pixel = pixels[count];
			    maxp = pixel + mask;
			    while (1)
			    {
				ent[pixel].refcnt = AllocPrivate;
				ent[pixel].fShared = FALSE;
				if (pixel == maxp)
				    break;
				pixel += base;
				*ppix++ = pixel;
			    }
			}
			*pMask = mask;
			return (TRUE);
		    }
		}
    	    pixel = save + 1;
    	    if (pixel & mask)
    	        pixel += mask;
        }
    }

    dplanes = pmap->pVisual->nplanes;
    if (contig || planes == 1 || dplanes < 3)
	return (FALSE);

    /* this will be very slow for large maps, need a better algorithm */

    /*
       we can generate the smallest and largest numbers that fits in dplanes
       bits and contain exactly planes bits set as follows. First, we need to
       check that it is possible to generate such a mask at all.
       (Non-contiguous masks need one more bit than contiguous masks). Then
       the smallest such mask consists of the rightmost planes-1 bits set, then
       a zero, then a one in position planes + 1. The formula is
         (3 << (planes-1)) -1
       The largest such masks consists of the leftmost planes-1 bits set, then
       a zero, then a one bit in position dplanes-planes-1. If dplanes is
       smaller than 32 (the number of bits in a word) then the formula is:
         (1<<dplanes) - (1<<(dplanes-planes+1) + (1<<dplanes-planes-1)
       If dplanes = 32, then we can't calculate (1<<dplanes) and we have
       to use:
         ( (1<<(planes-1)) - 1) << (dplanes-planes+1) + (1<<(dplanes-planes-1))
	  
	  << Thank you, Loretta>>>

    */

    finalmask =
        (((((Pixel)1)<<(planes-1)) - 1) << (dplanes-planes+1)) +
	  (((Pixel)1)<<(dplanes-planes-1));
    for (mask = (((Pixel)3) << (planes -1)) - 1; mask <= finalmask; mask++)
    {
        /* next 3 magic statements count number of ones (HAKMEM #169) */
        pixel = (mask >> 1) & 033333333333;
        pixel = mask - pixel - ((pixel >> 1) & 033333333333);
        if ((((pixel + (pixel >> 3)) & 030707070707) % 077) != planes)
    	    continue;
        ppix = pixels;
        found = 0;
        entries = pmap->pVisual->ColormapEntries - mask;
        base = lowbit (mask);
        for (pixel = 0; pixel < entries; pixel++)
	{
	    if (pixel & mask)
	        continue;
	    maxp = 0;
	    /* check if all are free */
	    while (ent[pixel + maxp].refcnt == 0)
	    {
		GetNextBitsOrBreak(maxp, mask, base);
	    }
	    if ((maxp < mask) || (ent[pixel + mask].refcnt != 0))
		continue;
	    /* this one works */
	    *ppix++ = pixel;
	    found++;
	    if (found < count)
		continue;
	    /* found enough, allocate them all */
	    while (--count >= 0)
	    {
		pixel = (pixels)[count];
		maxp = 0;
		while (1)
		{
		    ent[pixel + maxp].refcnt = AllocPrivate;
		    ent[pixel + maxp].fShared = FALSE;
		    GetNextBitsOrBreak(maxp, mask, base);
		    *ppix++ = pixel + maxp;
		}
	    }

	    *pMask = mask;
	    return (TRUE);
	}
    }
    return (FALSE);
}

static Bool
AllocShared (pmap, ppix, c, r, g, b, rmask, gmask, bmask, ppixFirst)
    ColormapPtr	pmap;
    Pixel	*ppix;
    int		c, r, g, b;
    Pixel	rmask, gmask, bmask;
    Pixel	*ppixFirst;	/* First of the client's new pixels */
{
    Pixel	*pptr, *cptr;
    int		npix, z, npixClientNew, npixShared;
    Pixel	basemask, base, bits, common;
    SHAREDCOLOR *pshared, **ppshared, **psharedList;

    npixClientNew = c << (r + g + b);
    npixShared = (c << r) + (c << g) + (c << b);
    psharedList = (SHAREDCOLOR **)ALLOCATE_LOCAL(npixShared *
						 sizeof(SHAREDCOLOR *));
    if (!psharedList)
	return FALSE;
    ppshared = psharedList;
    for (z = npixShared; --z >= 0; )
    {
	if (!(ppshared[z] = (SHAREDCOLOR *)xalloc(sizeof(SHAREDCOLOR))))
	{
	    for (z++ ; z < npixShared; z++)
		xfree(ppshared[z]);
	    return FALSE;
	}
    }
    for(pptr = ppix, npix = c; --npix >= 0; pptr++)
    {
	basemask = ~(gmask | bmask);
	common = *pptr & basemask;
	if (rmask)
	{
	    bits = 0;
	    base = lowbit (rmask);
	    while(1)
	    {
		pshared = *ppshared++;
		pshared->refcnt = 1 << (g + b);
		for (cptr = ppixFirst, z = npixClientNew; --z >= 0; cptr++)
		{
		    if ((*cptr & basemask) == (common | bits))
		    {
			pmap->red[*cptr].fShared = TRUE;
			pmap->red[*cptr].co.shco.red = pshared;
		    }
		}
		GetNextBitsOrBreak(bits, rmask, base);
	    }
	}
	else
	{
	    pshared = *ppshared++;
	    pshared->refcnt = 1 << (g + b);
	    for (cptr = ppixFirst, z = npixClientNew; --z >= 0; cptr++)
	    {
		if ((*cptr & basemask) == common)
		{
		    pmap->red[*cptr].fShared = TRUE;
		    pmap->red[*cptr].co.shco.red = pshared;
		}
	    }
	}
	basemask = ~(rmask | bmask);
	common = *pptr & basemask;
	if (gmask)
	{
	    bits = 0;
	    base = lowbit (gmask);
	    while(1)
	    {
		pshared = *ppshared++;
		pshared->refcnt = 1 << (r + b);
		for (cptr = ppixFirst, z = npixClientNew; --z >= 0; cptr++)
		{
		    if ((*cptr & basemask) == (common | bits))
		    {
			pmap->red[*cptr].co.shco.green = pshared;
		    }
		}
		GetNextBitsOrBreak(bits, gmask, base);
	    }
	}
	else
	{
	    pshared = *ppshared++;
	    pshared->refcnt = 1 << (g + b);
	    for (cptr = ppixFirst, z = npixClientNew; --z >= 0; cptr++)
	    {
		if ((*cptr & basemask) == common)
		{
		    pmap->red[*cptr].co.shco.green = pshared;
		}
	    }
	}
	basemask = ~(rmask | gmask);
	common = *pptr & basemask;
	if (bmask)
	{
	    bits = 0;
	    base = lowbit (bmask);
	    while(1)
	    {
		pshared = *ppshared++;
		pshared->refcnt = 1 << (r + g);
		for (cptr = ppixFirst, z = npixClientNew; --z >= 0; cptr++)
		{
		    if ((*cptr & basemask) == (common | bits))
		    {
			pmap->red[*cptr].co.shco.blue = pshared;
		    }
		}
		GetNextBitsOrBreak(bits, bmask, base);
	    }
	}
	else
	{
	    pshared = *ppshared++;
	    pshared->refcnt = 1 << (g + b);
	    for (cptr = ppixFirst, z = npixClientNew; --z >= 0; cptr++)
	    {
		if ((*cptr & basemask) == common)
		{
		    pmap->red[*cptr].co.shco.blue = pshared;
		}
	    }
	}
    }
    DEALLOCATE_LOCAL(psharedList);
    return TRUE;
}


/* Free colors and/or cells (probably slow for large numbers) */

int
FreeColors (pmap, client, count, pixels, mask)
    ColormapPtr	pmap;
    int		client, count;
    Pixel	*pixels;
    Pixel	mask;
{
    int		rval, result, class;
    Pixel	rmask;

    class = pmap->class;
    if (pmap->flags & AllAllocated)
	return(BadAccess);
    if ((class | DynamicClass) == DirectColor)
    {
	rmask = mask & RGBMASK(pmap->pVisual);
        result = FreeCo(pmap, client, REDMAP, count, pixels, rmask);
	/* If any of the three calls fails, we must report that, if more
	 * than one fails, it's ok that we report the last one */
        rval = FreeCo(pmap, client, GREENMAP, count, pixels, rmask);
	if(rval != Success)
	    result = rval;
	rval = FreeCo(pmap, client, BLUEMAP, count, pixels, rmask);
	if(rval != Success)
	    result = rval;
    }
    else
    {
	rmask = mask & ((((Pixel)1) << pmap->pVisual->nplanes) - 1);
        result = FreeCo(pmap, client, PSEUDOMAP, count, pixels, rmask);
    }
    if ((mask != rmask) && count)
    {
	clientErrorValue = *pixels | mask;
	result = BadValue;
    }
    /* XXX should worry about removing any RT_CMAPENTRY resource */
    return (result);
}

/* Helper for FreeColors -- frees all combinations of *newpixels and mask bits
 * which the client has allocated in channel colormap cells of pmap.
 * doesn't change newpixels if it doesn't need to */
static int
FreeCo (pmap, client, color, npixIn, ppixIn, mask)
    ColormapPtr	pmap;		/* which colormap head */
    int		client;		
    int		color;		/* which sub-map, eg RED, BLUE, PSEUDO */
    int		npixIn;		/* number of pixels passed in */
    Pixel	*ppixIn;	/* list of base pixels */
    Pixel	mask;		/* mask client gave us */ 
{

    Pixel	*ppixClient, pixTest;
    int		npixClient, npixNew, npix;
    Pixel	bits, base, cmask;
    Pixel	*pptr, *cptr;
    int 	n, zapped;
    int		errVal = Success;
    int		offset, numents;

    if (npixIn == 0)
        return (errVal);
    bits = 0;
    zapped = 0;
    base = lowbit (mask);

    switch(color)
    {
      case REDMAP:
	cmask = pmap->pVisual->redMask;
	offset = pmap->pVisual->offsetRed;
	numents = (cmask >> offset) + 1;
	ppixClient = pmap->clientPixelsRed[client];
	npixClient = pmap->numPixelsRed[client];
	break;
      case GREENMAP:
	cmask = pmap->pVisual->greenMask;
	offset = pmap->pVisual->offsetGreen;
	numents = (cmask >> offset) + 1;
	ppixClient = pmap->clientPixelsGreen[client];
	npixClient = pmap->numPixelsGreen[client];
	break;
      case BLUEMAP:
	cmask = pmap->pVisual->blueMask;
	offset = pmap->pVisual->offsetBlue;
	numents = (cmask >> offset) + 1;
	ppixClient = pmap->clientPixelsBlue[client];
	npixClient = pmap->numPixelsBlue[client];
	break;
      case PSEUDOMAP:
	cmask = ~((Pixel)0);
	offset = 0;
	numents = pmap->pVisual->ColormapEntries;
	ppixClient = pmap->clientPixelsRed[client];
	npixClient = pmap->numPixelsRed[client];
	break;
    }

    /* zap all pixels which match */
    while (1)
    {
        /* go through pixel list */
        for (pptr = ppixIn, n = npixIn; --n >= 0; pptr++)
	{
	    pixTest = ((*pptr | bits) & cmask) >> offset;
	    if (pixTest >= numents)
	    {
		clientErrorValue = *pptr | bits;
		errVal = BadValue;
		continue;
	    }

	    /* find match in client list */
	    for (cptr = ppixClient, npix = npixClient;
	         --npix >= 0 && *cptr != pixTest;
		 cptr++) ;

	    if (npix >= 0)
	    {
		if (pmap->class & DynamicClass)
		    FreeCell(pmap, pixTest, color);
		*cptr = ~((Pixel)0);
		zapped++;
	    }
	    else
		errVal = BadAccess;
	}
        /* generate next bits value */
	GetNextBitsOrBreak(bits, mask, base);
    }

    /* delete freed pixels from client pixel list */
    if (zapped)
    {
        npixNew = npixClient - zapped;
        if (npixNew)
	{
	    /* Since the list can only get smaller, we can do a copy in
	     * place and then realloc to a smaller size */
    	    pptr = cptr = ppixClient;

	    /* If we have all the new pixels, we don't have to examine the
	     * rest of the old ones */
	    for(npix = 0; npix < npixNew; cptr++)
	    {
    	        if (*cptr != ~((Pixel)0))
		{
    		    *pptr++ = *cptr;
		    npix++;
    	        }
    	    }
	    pptr = (Pixel *)xrealloc(ppixClient, npixNew * sizeof(Pixel));
	    if (pptr)
		ppixClient = pptr;
	    npixClient = npixNew;
        }
	else
	{
	    npixClient = 0;
	    xfree(ppixClient);
    	    ppixClient = (Pixel *)NULL;
	}
	switch(color)
	{
	  case PSEUDOMAP:
	  case REDMAP:
	    pmap->clientPixelsRed[client] = ppixClient;
	    pmap->numPixelsRed[client] = npixClient;
	    break;
	  case GREENMAP:
	    pmap->clientPixelsGreen[client] = ppixClient;
	    pmap->numPixelsGreen[client] = npixClient;
	    break;
	  case BLUEMAP:
	    pmap->clientPixelsBlue[client] = ppixClient;
	    pmap->numPixelsBlue[client] = npixClient;
	    break;
	}
    }
    return (errVal);
}



/* Redefine color values */
int
StoreColors (pmap, count, defs)
    ColormapPtr	pmap;
    int		count;
    xColorItem	*defs;
{
    register Pixel 	pix;
    register xColorItem *pdef;
    register EntryPtr 	pent, pentT, pentLast;
    register VisualPtr	pVisual;
    SHAREDCOLOR		*pred, *pgreen, *pblue;
    int			n, ChgRed, ChgGreen, ChgBlue, idef;
    int			class, errVal = Success;
    int			ok;


    class = pmap->class;
    if(!(class & DynamicClass) && !(pmap->flags & BeingCreated))
    {
	return(BadAccess);
    }
    pVisual = pmap->pVisual;

    idef = 0;
    if((class | DynamicClass) == DirectColor)
    {
	int numred, numgreen, numblue;
	Pixel rgbbad;

	numred = NUMRED(pVisual);
	numgreen = NUMGREEN(pVisual);
	numblue = NUMBLUE(pVisual);
	rgbbad = ~RGBMASK(pVisual);
        for (pdef = defs, n = 0; n < count; pdef++, n++)
	{
	    ok = TRUE;
            (*pmap->pScreen->ResolveColor)
	        (&pdef->red, &pdef->green, &pdef->blue, pmap->pVisual);

	    if (pdef->pixel & rgbbad)
	    {
		errVal = BadValue;
		ok = FALSE;
	    }
	    pix = (pdef->pixel & pVisual->redMask) >> pVisual->offsetRed;
	    if (pix >= numred)
	    {
		errVal = BadValue;
		ok = FALSE;
	    }
	    else if (pmap->red[pix].refcnt != AllocPrivate)
	    {
		errVal = BadAccess;
		ok = FALSE;
	    }
	    else if (pdef->flags & DoRed)
	    {
		pmap->red[pix].co.local.red = pdef->red;
	    }
	    else
	    {
		pdef->red = pmap->red[pix].co.local.red;
	    }

	    pix = (pdef->pixel & pVisual->greenMask) >> pVisual->offsetGreen;
	    if (pix >= numgreen)
	    {
		errVal = BadValue;
		ok = FALSE;
	    }
	    else if (pmap->green[pix].refcnt != AllocPrivate)
	    {
		errVal = BadAccess;
		ok = FALSE;
	    }
	    else if (pdef->flags & DoGreen)
	    {
		pmap->green[pix].co.local.green = pdef->green;
	    }
	    else
	    {
		pdef->green = pmap->green[pix].co.local.green;
	    }

	    pix = (pdef->pixel & pVisual->blueMask) >> pVisual->offsetBlue;
	    if (pix >= numblue)
	    {
		errVal = BadValue;
		ok = FALSE;
	    }
	    else if (pmap->blue[pix].refcnt != AllocPrivate)
	    {
		errVal = BadAccess;
		ok = FALSE;
	    }
	    else if (pdef->flags & DoBlue)
	    {
		pmap->blue[pix].co.local.blue = pdef->blue;
	    }
	    else
	    {
		pdef->blue = pmap->blue[pix].co.local.blue;
	    }
	    /* If this is an o.k. entry, then it gets added to the list
	     * to be sent to the hardware.  If not, skip it.  Once we've
	     * skipped one, we have to copy all the others.
	     */
	    if(ok)
	    {
		if(idef != n)
		    defs[idef] = defs[n];
		idef++;
	    } else
		clientErrorValue = pdef->pixel;
	}
    }
    else
    {
        for (pdef = defs, n = 0; n < count; pdef++, n++)
	{

	    ok = TRUE;
	    if (pdef->pixel >= pVisual->ColormapEntries)
	    {
		clientErrorValue = pdef->pixel;
	        errVal = BadValue;
		ok = FALSE;
	    }
	    else if (pmap->red[pdef->pixel].refcnt != AllocPrivate)
	    {
		errVal = BadAccess;
		ok = FALSE;
	    }

	    /* If this is an o.k. entry, then it gets added to the list
	     * to be sent to the hardware.  If not, skip it.  Once we've
	     * skipped one, we have to copy all the others.
	     */
	    if(ok)
	    {
		if(idef != n)
		    defs[idef] = defs[n];
		idef++;
	    }
	    else
		continue;

            (*pmap->pScreen->ResolveColor)
	        (&pdef->red, &pdef->green, &pdef->blue, pmap->pVisual);

	    pent = &pmap->red[pdef->pixel];

	    if(pdef->flags & DoRed)
	    {
		if(pent->fShared)
		{
		    pent->co.shco.red->color = pdef->red;
		    if (pent->co.shco.red->refcnt > 1)
			ok = FALSE;
		}
		else
		    pent->co.local.red = pdef->red;
	    }
	    else
	    {
		if(pent->fShared)
		    pdef->red = pent->co.shco.red->color;
		else
		    pdef->red = pent->co.local.red;
	    }
	    if(pdef->flags & DoGreen)
	    {
		if(pent->fShared)
		{
		    pent->co.shco.green->color = pdef->green;
		    if (pent->co.shco.green->refcnt > 1)
			ok = FALSE;
		}
		else
		    pent->co.local.green = pdef->green;
	    }
	    else
	    {
		if(pent->fShared)
		    pdef->green = pent->co.shco.green->color;
		else
		    pdef->green = pent->co.local.green;
	    }
	    if(pdef->flags & DoBlue)
	    {
		if(pent->fShared)
		{
		    pent->co.shco.blue->color = pdef->blue;
		    if (pent->co.shco.blue->refcnt > 1)
			ok = FALSE;
		}
		else
		    pent->co.local.blue = pdef->blue;
	    }
	    else
	    {
		if(pent->fShared)
		    pdef->blue = pent->co.shco.blue->color;
		else
		    pdef->blue = pent->co.local.blue;
	    }

	    if(!ok)
	    {
                /* have to run through the colormap and change anybody who
		 * shares this value */
	        pred = pent->co.shco.red;
	        pgreen = pent->co.shco.green;
	        pblue = pent->co.shco.blue;
	        ChgRed = pdef->flags & DoRed;
	        ChgGreen = pdef->flags & DoGreen;
	        ChgBlue = pdef->flags & DoBlue;
	        pentLast = pmap->red + pVisual->ColormapEntries;

	        for(pentT = pmap->red; pentT < pentLast; pentT++)
		{
		    if(pentT->fShared && (pentT != pent))
		    {
			xColorItem	defChg;

			/* There are, alas, devices in this world too dumb
			 * to read their own hardware colormaps.  Sick, but
			 * true.  So we're going to be really nice and load
			 * the xColorItem with the proper value for all the
			 * fields.  We will only set the flags for those
			 * fields that actually change.  Smart devices can
			 * arrange to change only those fields.  Dumb devices
			 * can rest assured that we have provided for them,
			 * and can change all three fields */

			defChg.flags = 0;
			if(ChgRed && pentT->co.shco.red == pred)
			{
			    defChg.flags |= DoRed;
			}
			if(ChgGreen && pentT->co.shco.green == pgreen)
			{
			    defChg.flags |= DoGreen;
			}
			if(ChgBlue && pentT->co.shco.blue == pblue)
			{
			    defChg.flags |= DoBlue;
			}
			if(defChg.flags != 0)
			{
			    defChg.pixel = pentT - pmap->red;
			    defChg.red = pentT->co.shco.red->color;
			    defChg.green = pentT->co.shco.green->color;
			    defChg.blue = pentT->co.shco.blue->color;
			    (*pmap->pScreen->StoreColors) (pmap, 1, &defChg);
			}
		    }
		}

	    }
	}
    }
    /* Note that we use idef, the count of acceptable entries, and not
     * count, the count of proposed entries */
    if (idef != 0)
	( *pmap->pScreen->StoreColors) (pmap, idef, defs);
    return (errVal);
}

int
IsMapInstalled(map, pWin)
    Colormap	map;
    WindowPtr	pWin;
{
    Colormap	*pmaps;
    int		imap, nummaps, found;

    pmaps = (Colormap *) ALLOCATE_LOCAL( 
             pWin->drawable.pScreen->maxInstalledCmaps * sizeof(Colormap));
    if(!pmaps)
	return(FALSE);
    nummaps = (*pWin->drawable.pScreen->ListInstalledColormaps)
        (pWin->drawable.pScreen, pmaps);
    found = FALSE;
    for(imap = 0; imap < nummaps; imap++)
    {
	if(pmaps[imap] == map)
	{
	    found = TRUE;
	    break;
	}
    }
    DEALLOCATE_LOCAL(pmaps);
    return (found);
}
