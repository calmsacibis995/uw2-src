/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)oldattlib:XScale.c	1.2"
#endif
/*
 XScale.c (C source file)
	Acc: 575043158 Tue Mar 22 09:12:38 1988
	Mod: 571243147 Sun Feb  7 09:39:07 1988
	Sta: 573929825 Wed Mar  9 11:57:05 1988
	Owner: 2011
	Group: 1985
	Permissions: 666
*/
/*
	START USER STAMP AREA
*/
/*
	END USER STAMP AREA
*/
/* given an XY bitmap in least least 8 format scale it by percentage */

char * XScale(bitmap,width,length,percentage)
char bitmap[];
int width;
int length;
double percentage;
{
register i = 0;
register j = 0;

char * retmap;
#ifndef MEMUTIL
char * malloc();
#endif /* MEMUTIL */

int newwidth = width * percentage;
int newlength = length * percentage;
int newsize = ((newwidth + 7) / 8) * newlength;
int newbytesperline = ((newwidth + 7) / 8);
int bytesperline = ((width + 7) / 8);
int targetmask = 0;
int targetbyte = 0;
int sourcemask = 0;
int sourcebyte = 0;

retmap = (char *) malloc(newsize);
for (i = 0; i < newsize; i++) retmap[i] = 0;

if (percentage >= 1)
   {
   for (i = 0; i < newlength; i++)
      for (j = 0; j < newwidth; j++)
	 {
	 targetmask = j % 8;
	 targetbyte = i * newbytesperline + j / 8;
	 sourcemask = (int)(j / percentage) % 8;
	 sourcebyte = (int)(i / percentage) * bytesperline + 
		      (int)(j / percentage) / 8;
         retmap[targetbyte] |=(((bitmap[sourcebyte] >> sourcemask) & ~(~0 << 1))
	    << targetmask);
         /***************
	 printf("%2d: %2d   %2d:%2d -> %2d:%2d = %d\n",
	    i,j,sourcebyte,sourcemask,targetbyte,targetmask,(int)retmap[targetbyte]);
  	 retmap[targetbyte] |= ((( bitmap[sourcebyte] >> sourcemask ) & 1)
	    << targetmask);
	 ***************/
	 }
   }
else
   {

   /* not working yet !!! */

   for (i = 0; i < length; i++)
      for (j = 0; j < width; j++)
	 {
	 sourcemask = j % 8;
	 sourcebyte = i * bytesperline + j / 8;
	 targetmask = (int)(j * percentage) % 8;
	 targetbyte = (int)(i * percentage) * newbytesperline + 
		      (int)(j * percentage) / 8;
         retmap[targetbyte] |=(((bitmap[sourcebyte] >> sourcemask) & ~(~0 << 1))
	    << targetmask);

/*************
	 printf("%2d: %2d   %2d:%2d -> %2d:%2d\n",
	    i,j,sourcebyte,sourcemask,targetbyte,targetmask);
/*	 retm  [targetbyte] |= bitmap[sourcebyte] & sourcemask >> targetmask; 
*************/
	 }
   }
return retmap;
} /* end of XScale */
