/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)vga256:vga256/devices/etw32p/font.c	1.3"

/*
 *	Copyright (c) 1992, 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 */

/*
 * Copyright (c) 1992,1993 Pittsburgh Powercomputing Corporation (PPc).
 * All Rights Reserved.
 */

#include "Xmd.h"
#include "sidep.h"
#include "sys/types.h"
#include "sys/kd.h"
#include "vtio.h"
#include "v256.h"
#include "sys/inline.h"
#include <stdio.h>
#include "etw32p.h"
#include "font.h"

#define FREE(p)	(p ? free(p) : 0)

/* Information on downloaded fonts: */
etw32p_font_rec etw32p_fonts[FONT_COUNT];


/*
 *   Font packing is disabled by default
 *   terminal font and non terminal fonts are enabled by default
 */

int etw32p_font_pack=0;
int etw32p_terminal_font=1;
int etw32p_non_terminal_font=1;


/*
 *  etw32p_font_check(num, info)  : Check whether we can download a font
 *
 *    Input:
 *       int num	  : index of the font to be downloaded
 *       SIFontInfoP info : basic info about font
 */
SIBool
etw32p_font_check(num, info)
int num;
SIFontInfoP info;
{
    /*
	 * only when both terminal_font and non_terminal_font are disabled
	 * it will return SI_FAIL.  If terminal_font is diabled, still it
	 * can go through non-terminal-font-code.
	 */
    if (  
		!( etw32p_terminal_font || etw32p_non_terminal_font ) ||
	    OUTSIDE(info->SFnumglyph, 0, FONT_NUMGLYPHS) ||
	    OUTSIDE(info->SFmax.SFwidth, 1, FONT_GLYPH_MAXWIDTH) ||
	    OUTSIDE(info->SFlascent + info->SFldescent, 1, FONT_GLYPH_MAXHEIGHT)) {

#if (defined(__DEBUG__))
		fprintf( stderr, "no download: numglyph=%d width=%d height=%d\n",
				  info->SFnumglyph, info->SFmax.SFwidth,
				  info->SFlascent + info->SFldescent );
#endif

        return(SI_FAIL);
    }
    return(SI_SUCCEED);
}


/*
 *	etw32p_non_terminal font_download(num, info, glyphs):download the glyphs for
 *    a font, converting them to a format ready for blitting.
 *
 *    Input:
 *        int num	   : the index for the downloaded font
 *        SIFontInfoP info : basic info about font
 *        SIGlyphP glyphs  : the glyphs themselves
 */

SIBool
etw32p_non_terminal_font_download(num, info, glyphs)
int num;
SIFontInfoP info;
SIGlyphP glyphs;
{

    int bpitch, bw, w, h, g, gnum;
    register BLTDATA *data;
    char *bdata;
    register int i, j;
	etw32p_font_rec *glyph_ptr;


	/*
	 *   bpitch:  pitch for SI_BITMAP data for a glyph.
	 *   gnum  :  no. of glyphs in the font.
	 *   bw    :  no. of bytes for a glyph line. 
	 *   bdata :  contains si_bitmap data for a glyph.
	 *   data  :  glyph_data stored in blittable format. 
	 *glyph_ptr:  pointer to etw32p_font_rec, useful in storing glyph data.
	 *      w  :  maximum width of a glyph in 'pixels'.
	 *      h  :  maximum height of a glyph in 'pixles'.
	 *    
	 *   'maximum width' means any glyph in the font will have width 
	 *         less than or equal to this value. 
	 *   'maximum height' means any glyph in the font will have height 
	 *         less than or equal to this value. 
	 */


    w = info->SFmax.SFrbearing - info->SFmin.SFlbearing;

	/*
	 *  w should contain maximum width.
	 *  In case of overlaping fonts, rbearing can be > width.
	 *  so, take maximum of these two.
	 */
	if ( w < info->SFmax.SFwidth ) {

       w = info->SFmax.SFwidth;
	}

    glyph_ptr = &etw32p_fonts[num];
	/* 
	 *  use glyph_ptr instead of etw32p_fonts[num];
	 */

    h = glyph_ptr->max_height = info->SFlascent + info->SFldescent;
	if ( h < info->SFmax.SFascent + info->SFmax.SFdescent) {

	 	h = info->SFmax.SFascent + info->SFmax.SFdescent;
	}
	glyph_ptr->lascent = info->SFlascent;

    gnum = info->SFnumglyph;

    /* Convert each glyph bitmap to a blit-ready image: a simple sequence of
    ||   bytes.
    */

    bpitch = ((w+31) & ~31) >> 3;	/* bytes between lines of glyph */
    bw = (w+7) >> 3;			/* width in bytes of each line */
    glyph_ptr->lsize = bw;	/* # of BLTDATA elements per line */
    glyph_ptr->size  = bw * h;	/* # of BLTDATA elements per glyph */

    if ( glyph_ptr->data != NULL )
    	FREE( glyph_ptr->data );

    data = (BLTDATA *) malloc(bw * h * gnum * sizeof(BLTDATA));
    if (!data) return (SI_FAIL);

    glyph_ptr->data = data;

    for (g=0; g<gnum; ++g) {
    
          int glyph_height;
	  	  int glyph_width;

		  glyph_ptr->w[g] = glyphs[g].SFwidth;
		  glyph_width = glyphs[g].SFrbearing - glyphs[g].SFlbearing;
		  glyph_height =  ( glyphs[g].SFascent + glyphs[g].SFdescent );

          ASSERT( glyph_width <= w && glyph_height <= h );

		  glyph_ptr->ascent[g]   = glyphs[g].SFascent;
		  glyph_ptr->descent[g]  = glyphs[g].SFdescent;
		  glyph_ptr->lbearing[g] = glyphs[g].SFlbearing;
		  glyph_ptr->rbearing[g] = glyphs[g].SFrbearing;

		  ASSERT( glyph_ptr->rbearing[g] - glyph_ptr->lbearing[g]                   <= w );
		  ASSERT( glyph_ptr->ascent[g] + glyph_ptr->descent[g]                      <= h );

		  glyph_width  =  ( glyph_width + 7 ) >> 3;

          ASSERT( bpitch >= glyph_width );
          ASSERT( bw >= glyph_width );

		  bdata = (char *) glyphs[g].SFglyph.Bptr;
		  i = glyph_height;
		  while ( i-- ) {

            j = glyph_width;
			while ( j-- )  
		      *data++ = *bdata++;
 	        bdata += ( bpitch - glyph_width );
			data  += ( bw - glyph_width );
	     }
		 data += ( ( h-glyph_height ) * bw );
    }

    glyph_ptr->is_terminal_font=0;
	/*
	 * non-terminal font downloading suceeded
	 */
    return(SI_SUCCEED);
}


/*
 *  etw32p_font_download(num, info, glyphs)  : download the glyphs for a
 *	font, converting them to a format ready for blitting.
 *
 *    Input:
 *        int num	   : the index for the downloaded font
 *        SIFontInfoP info : basic info about font
 *        SIGlyphP glyphs  : the glyphs themselves
 */
SIBool
etw32p_font_download(num, info, glyphs)
int num;
SIFontInfoP info;
SIGlyphP glyphs;
{


    int bpitch, bw, w, h, g, gnum;
    register BLTDATA *data;
    char *bdata;
    register int i, j;

	/*
	 *   bpitch:  pitch for SI_BITMAP data for a glyph.
	 *   gnum  :  no. of glyphs in the font.
	 *   bw    :  no. of bytes for a glyph line. 
	 *   bdata :  contains si_bitmap data for a glyph.
	 *   data  :  glyph_data stored in blittable format. 
	 *      w  :  width of a (any) glyph in 'pixels'.
	 *      h  :  height of a (any) glyph in 'pixles'.
	 */

     if (   ( !(info->SFflag & SFTerminalFont) || (!etw32p_terminal_font)  )  ) {

		/*
		 *  come here if not a terminal font or hardware terminal font
		 *  drawing is disabled
		 */

		if (  etw32p_non_terminal_font )  {
			/*
			 * incase of terminal font and hardware terminal font drawing
			 * is disabled (using HWFUNCS environment variable),
			 * terminal font will be drawn using non-terminal font code.
			 * ( Note:  terminal font is subset of non-terminal font )
			 */
			return ( etw32p_non_terminal_font_download(num, info, glyphs) );

        }
		else   {
			 /*
			  *  if non-terminal font is disabled, ask si to take over
			  */

		    return ( SI_FAIL );
        }

    }

	h = etw32p_fonts[num].max_height = info->SFldescent + info->SFlascent;
	w = etw32p_fonts[num].w[0]   = info->SFmax.SFwidth;
    etw32p_fonts[num].lascent    = info->SFlascent;

    gnum = info->SFnumglyph;

    /* Convert each glyph bitmap to a blit-ready image: a simple sequence of
    ||   bytes.
    */

    bpitch = ((w+31) & ~31) >> 3; /* bytes between lines of glyph */
    bw = (w+7) >> 3;			  /* width in bytes of each line */

    if ( etw32p_fonts[num].data != NULL )
    	FREE(etw32p_fonts[num].data);

    etw32p_fonts[num].lsize = bw;	/* # of BLTDATA elements per line */
    etw32p_fonts[num].size  = bw * h;	/* # of BLTDATA elements per glyph */

    data = (BLTDATA *) malloc(bw * h * gnum * sizeof(BLTDATA));

    if (!data) return (SI_FAIL);
	/*    if memory allocation fails */

    etw32p_fonts[num].data = data;

    for (g=0; g<gnum; ++g) {

		bdata = (char *) glyphs[g].SFglyph.Bptr;

        ASSERT( glyphs[g].SFglyph.Bwidth == w );

        i=h;
		while ( i-- ) {

		  	j=bw;
		  	while ( j-- ) 
		 		 *data++ = *bdata++;
 	      	bdata += (bpitch - bw);
	    }

    }
	etw32p_fonts[num].is_terminal_font=1;
	/*
	 * terminal font successfully down loaded 
	 */
    return(SI_SUCCEED);

}

/*
 * etw32p_non_terminal_font_stpl(num, x, y, cnt, glyphs, type)
 *                : stipple glyphs in a downloaded font.
 *
 *    Input:
 *       int num	: font index to stipple from
 *       int x,y	: position to stipple to (at baseline of font)
 *       int count	: number of glyphs to stipple
 *       SIint16 *glyphs : list of glyph indices to stipple
 *       int forectype  : Opaque or regular stipple (if non-zero)
 */

SIBool
etw32p_non_terminal_font_stplblt(num, x, y, gcount, glyphs, forcetype)
SIint32 num, x, y, gcount, forcetype;
SIint16 *glyphs;
{

    int addr, lsize,  ch, cw=0, cx, cy, cumulate_cx;
    int size, overlap=0, left_over=0;
    int hidden_start;
    BLTDATA *data;
    register BLTDATA *gdata;
	register int j;
	int orginal_count;
	int glyph_index;
    etw32p_font_rec *glyph_ptr;
	int *glyph_width_ptr;


    /*
	 *  cx: x-coridinate from which font_blit starts.
	 *  cy: y-cordiante from which font_blit starts.
	 *  ch: height for the font_blit.
	 *  cw: width for font_blit.
     *
	 *  lsize:   no of bytes for a glyph_line.
	 *  csize:   no of bytes for a entire glyph.
	 *  goverlap: no. of glyphs which are outside the cliprectangle.
	 *  overlap:  temporary variable which holds, amount of width or height
	 *              outside cliprectangle.
	 *
	 *  hidden_start:  When only a portion of first glyph is to be drawn, this
	 *                  holds amount to be added to cx so as to get
	 *                  starting x-cordinate for the font_blit.
	 *
	 *  leftover:      When only a portion of last glyph is to be drawn, this
	 *                  holds the howmuch of width of the last glyph to be
	 *                  drawn.
	 *
	 *      addr    :  Holds addr in display memory  at which  font_blit should
	 *                  start.   
	 *  
	 *      gdata   :  contains  glyph data.      
	 *      data    :  pointer to glyph data.
     *
	 *    original_count:  contains original value of gcount (no. or glyphs)
	 *    glyph_index   :   temporary index variable (indexing glyphs array).
	 *       
	 *    glyph_width_ptr:  is etw32p_fonts[num].w
	 *    glyph_ptr      :  is pointer to etw32p_fonts[num]
	 *
	 *    cumulate_cx    :  cx + width of all glyphs to be drawn - 1
	 *                      Useful during opaque stippling.
	 */

	orginal_count=gcount;
	glyph_ptr =  &etw32p_fonts[num];
	glyph_width_ptr = etw32p_fonts[num].w;

#if (defined(__DEBUG__))
    fprintf( stderr, "\n\nnon_terminal_font_stplblt(x=%d,y=%d,gcount=%d){\n",
	x,y,gcount);

#endif

#if (defined(__DEBUG__))
{

    fprintf( stderr, "	gcount=%d x=%d y=%d v256_clip_x1=%d v256_clip_x2=%d v256_clip_y1=%d v256_clip_y2=%d\n", 
   		gcount, x, y, v256_clip_x1, v256_clip_x2, v256_clip_y1, v256_clip_y2);


    fprintf( stderr, "	glyph widths...\n	" );
	for ( j=0; j<gcount; ++j )
	   fprintf( stderr, "%d  ", *( glyph_width_ptr+*(glyphs+j) ) );
    fprintf( stderr, "\n" );

}
#endif

    glyph_index=0;
    data  = glyph_ptr->data;
    size  = glyph_ptr->size;
    lsize = glyph_ptr->lsize;


    ASSERT( lsize < 5 && lsize > 0 );

    cx = x ;


    if (cx < v256_clip_x1) {

           /*
			*   each glyph of a non-terminal font may be different.
			*   To calculate no. of glyphs totally clipped (left or right),
			*   we may need to parse through width array of the font
			*   record.
			*/

#if (defined(__DEBUG__))

		fprintf( stderr, "	clipping left edge ..\n       " );
#endif

		overlap  = v256_clip_x1 - cx ;
		while ( overlap >= *(glyph_width_ptr+*glyphs) ) {


              ASSERT( *(glyph_width_ptr+*glyphs) < 33 );
			  --gcount;
			  cx += *(glyph_width_ptr+*glyphs);
			  overlap -= *(glyph_width_ptr+*glyphs);
		  	  ++glyph_index;
              ++glyphs; 

		}

#if (defined(__DEBUG__))
        fprintf( stderr, "	no. of glyphs clipped left totally=%d\n",
						 glyph_index );
#endif

		hidden_start = overlap;
   	}
	else {

		hidden_start = 0;
    }

    ASSERT( overlap >= 0 );

	if (  gcount <= 0  || cx > v256_clip_x2 ) {

	   return SI_SUCCEED;
    }

    j = glyph_index; 
	cumulate_cx = cx;

#if (defined(__DEBUG__))

	fprintf( stderr, "	j=%d before looking for right clip\n", j );
	fprintf( stderr, "	cumulate_cx=%d \n", cumulate_cx );
	fprintf( stderr, "	gcount=%d \n", gcount );
#endif

	while ( j < orginal_count )  {

        cumulate_cx = cumulate_cx + *( glyph_width_ptr + *(glyphs+j) );   
        if ( cumulate_cx-1 > v256_clip_x2 )   {

			 /* 
			  * cx outside v256_clip_x2
			  * clip right all the glyphs after this.
			  */
             gcount    = j-glyph_index;
			 left_over = *( glyph_width_ptr + *(glyphs+j) ) - cumulate_cx +
						  v256_clip_x2+1; 

#if (defined(__DEBUG__))

		      fprintf( stderr, "	clipping right edge ..\n" );
			  fprintf( stderr,
				      "	no of glyphs totally clipped right=%d left_over=%d\n",
				      gcount-(j-glyph_index), left_over 
					 );
#endif

              ASSERT( gcount >=0 );
			  break;
		}
		++j;
   }

   ASSERT( left_over >= 0 );
   ASSERT( left_over <= * (glyph_width_ptr+*(glyphs+j)) ); 

   if ( j != orginal_count )  {

  	 cumulate_cx = v256_clip_x2;
   }
   else {

     cumulate_cx = cumulate_cx - 1;
   }

   ASSERT( cumulate_cx <= v256_clip_x2 );

   CHECK_ACL();
   ACL_DIR   = DIR_NORMAL;

   if ( forcetype != SGStipple  && (cumulate_cx-cx-hidden_start+1) > 0  ) {

	   /*
		*  Opaque stippling has to be done.
		*  Solid fill with bg first.
		*/

		/*
		 * calculate height and width for the solid fill.
		 *  Width can be calculated from cumulate_cx and cx, which are
		 *  the last and first x-cordinates (respectively) for the fontblit.
		 *  Height is taken as max_height, which is stored during download
		 *  time itself.  
		 *  Starting y co-ordinate can be calculated from logical-ascent
		 *  stored during download time itself.
		 */

		cy = y - glyph_ptr->lascent;
		ch = glyph_ptr->max_height;

    	if ( cy+ch-1 > v256_clip_y2 ) {

			overlap = cy+ch-1-v256_clip_y2;
			ch     -= overlap;
    	}

    	if ( cy < v256_clip_y1 ) {

			overlap = v256_clip_y1-cy;
			ch     -= overlap;
			cy     += overlap;
    	}
		if ( ch > 0  ) {

#if (defined(__DEBUG__))

        	fprintf( stderr, "	opqflag set.\n" );
        	fprintf( stderr, "	hence solid fill first.\n");
#endif

			/* write over text area with background color */
			SET_SRC_SOLID(v256_gs->bg);

			ACL_RO   = RO_ADAUTO | RO_DAAUTO;
			ACL_XCNT = cumulate_cx-(cx+hidden_start);
			ACL_YCNT = ch-1;
   		    ACL_DADDR  = ( cy * etw32p_disp_x ) + cx + hidden_start;
			WAIT_ON_ACL();
       }

    }

    SET_SRC_SOLID( v256_gs->fg );			/* set 'source' to fg */
    ACL_RO = RO_DAMIX | RO_ADAUTO;
    ACL_BGROP     =  ROP_NOOP;
    ACL_PROC_BYTE = PROC_BYTE_1;		/* write 4 bytes at a time */

	/*
	 *  For ms_stplblt, set these values
	 */
	ACL_MADDR     = 0;
	ACL_MYOFF     = 31;

   /*
    * Perform initial partial blit if necessary
    */
    if ( hidden_start ) {

        int hsize;

		gdata = data + size * *glyphs;
    	ch = glyph_ptr->ascent[*glyphs] + 
			 glyph_ptr->descent[*glyphs];
	    cy = y - glyph_ptr->ascent[*glyphs];
   	    if ( cy+ch-1 > v256_clip_y2 ) {

	 		overlap = cy+ch-1-v256_clip_y2;
	 		ch     -= overlap;
     	}
    	if ( cy < v256_clip_y1 ) {

			overlap = v256_clip_y1-cy;
			ch     -= overlap;
			cy     += overlap;
		    gdata += lsize * overlap;	/* glyph data */
    	}

   		if ( !gcount ) {

             if ( hidden_start <= glyph_ptr->lbearing[*glyphs] ) {

			    hidden_start=0;
				/*  
				 *  Now, we have to draw the full glyph.
				 *  so, don't do --gcount and ++glyphs.
				 */
			 }
			 else {

                hidden_start =  hidden_start - glyph_ptr->lbearing[*glyphs];
				if ( left_over > glyph_ptr->rbearing[*glyphs] ) {

					left_over =glyph_ptr->rbearing[*glyphs];

				}
   	            cw =left_over-hidden_start;
			 }

#if (defined(__DEBUG__))

			 fprintf( stderr, "		partial start and end blit: left_over=%d hidden_start=%d\n", left_over, hidden_start );

#endif

		} 
		else {

                 if ( hidden_start <= glyph_ptr->lbearing[*glyphs] ) {

					 hidden_start = 0;
					/*  
					 *  Now, we have to draw the full glyph.
					 *  so, don't do --gcount and ++glyphs
					 */

                 }
				 else {

                     hidden_start = hidden_start - glyph_ptr->lbearing[*glyphs];
	     	         cw = glyph_ptr->rbearing[*glyphs]
						 -glyph_ptr->lbearing[*glyphs]
						 -hidden_start;
				 }

#if (defined(__DEBUG__))

			 fprintf( stderr, "	partial start blit hidden_start=%d cw=%d\n",
					  hidden_start, cw);
#endif
		}

   		if ( ch <= 0 || cy < v256_clip_y1 ||  cw <= 0 ||
			 hidden_start == 0  || cy > v256_clip_y2 || 
			 (left_over <= hidden_start && !gcount) )  {
   
  			 /* 
			  * don't draw first glyph;
			  */

			  cw = 0;
			  /*
			   * if cw is negative, we need not draw.  But
			   * to avoid assertion violation later, make it 0.
			   */

#if (defined(__DEBUG__))

			fprintf( stderr, "	First glyph not drawn: ch=%d cy=%d\n", ch, cy );
#endif

        }
		else  {

            ASSERT( cy >= v256_clip_y1 );
            ASSERT( cy <= v256_clip_y2 );
            ASSERT( cx+hidden_start+glyph_ptr->lbearing[*glyphs] 
					<= v256_clip_x2 );
            ASSERT( cx+hidden_start+glyph_ptr->lbearing[*glyphs] 
					>= v256_clip_x1 );
			ASSERT( cw > 0 && ch > 0 );

			ACL_XCNT = cw - 1;
			ACL_YCNT = ch -1;
			hsize = (cw + 7) >> 3;			/* # of bytes per line */
			{
				int mask;
				mask = (~(unsigned int)0) >> (32-lsize*8);
   	      
   		       /*
			    *   mask  the additional (unwanted) leading bytes
			    *   Note:  lsize can be maximum of 4 bytes.
			    */
	
  		        ACL_DADDR    = ( cy * etw32p_disp_x ) + cx + hidden_start +
						   glyph_ptr->lbearing[*glyphs];

                while ( ch-- ) {

		   	  		int gword;
			  		char *gword_ptr;

			  		gword_ptr = (char *)&gword;
			  		gword = (*(int *) gdata) & mask;
              		gword >>= hidden_start;
		      		/*
			   		 * before first integer write, shift out hidden bits
			   		 */
			  		for (j=0; j< hsize; ++j ) {
               
			    		*(volatile char *)(MMU_AP0+j) = *(char *)gword_ptr++;
		      		}
		      		gdata += (lsize);
		   		}
       		}
	   		WAIT_ON_ACL();
	  	 /*
		  *  Hmm.., bad practice to use goto...
		  */
      }
	  if ( !gcount )  {

#if (defined(__DEBUG__))
                    fprintf( stderr, "	going to font_blit_succeed\n" );
#endif
					goto fontblit_succeed;
      }
	  if ( hidden_start ) {

	     --gcount;
         ASSERT( cw >= 0 );
		 ASSERT( cx + hidden_start + glyph_ptr->lbearing[*glyphs] == v256_clip_x1 );
 
	 	cx += *(glyph_width_ptr+*(glyphs));
	 	++glyphs;
	 }

 }	

  /*
  || Perform gcount full-width blits
  */

#if (defined(__DEBUG__))

	   fprintf( stderr, "	full width blit count=%d cx=%d\n", gcount, cx );
#endif

  while ( gcount-- ) {

		 register int glyph_width;

         glyph_width = glyph_ptr->rbearing[*glyphs] - 
					   glyph_ptr->lbearing[*glyphs];

	  	 gdata     = data + size * *glyphs;
    	 ACL_XCNT  = glyph_width - 1;

    	 ch = glyph_ptr->ascent[*glyphs] + glyph_ptr->descent[*glyphs];
	     cy = y - glyph_ptr->ascent[*glyphs];

		 if ( cx + glyph_width-1 > v256_clip_x2 ) {

			/*
			 *  OVERLAPPING glyph.  rbearing > width.
			 *  check whether it is going outside v256_clip_x2
			 */

 			glyph_width = v256_clip_x2 - cx;
			ASSERT( cx+glyph_width == v256_clip_x2 );

		 }

		 /*
		  *   each glyph might have different height.
		  *   We have to clip in y-direction for each glyph.
		  */

   	     if ( cy+ch-1 > v256_clip_y2 ) {

	 		overlap = cy+ch-1-v256_clip_y2;
	 		ch     -= overlap;
     	 }
    	 if ( cy < v256_clip_y1 ) {

			overlap = v256_clip_y1-cy;
			ch     -= overlap;
			gdata  += lsize * overlap;
			cy     += overlap;
    	 }
		 if ( glyph_width <= 0 || ch <= 0  ) {

#if (defined(__DEBUG__))
          fprintf( stderr, "	Glyph Need not be drawn: glyph_width=%d ch=%d\n",
				   glyph_width, ch );
          fprintf( stderr, "	*glyph=%d *glyph_ptr->lbearing[*glyph]=%d\n",
					 *glyphs, glyph_ptr->lbearing[*glyphs] );
#endif
		      cx += *(glyph_width_ptr+*glyphs);
			  ++glyphs;
              continue;
		 }

         ASSERT( cx+glyph_ptr->lbearing[*glyphs] >= v256_clip_x1 );
         ASSERT( cx+glyph_ptr->lbearing[*glyphs] <= v256_clip_x2 );
         ASSERT( cy >= v256_clip_y1 );
         ASSERT( cy <= v256_clip_y2 );

		 ACL_YCNT = ch-1;
		 glyph_width = (glyph_width+7) >> 3;
		 ACL_DADDR = cy * etw32p_disp_x + cx + glyph_ptr->lbearing[*glyphs];

		 while ( ch-- ) {

		 	for (j=0; j<glyph_width; ++j) {
	       	    *(volatile char *)(MMU_AP0+j) = *(char *)gdata++;
   	     	}
			gdata += ( lsize - glyph_width );

		 }
		 cx += *(glyph_width_ptr + *glyphs );
		 ++glyphs;
         WAIT_ON_ACL();
  }

   /*
   || Take care of partial-width ending blit
   */

    if ( left_over && left_over > glyph_ptr->lbearing[*glyphs] ) {

		int hsize;

		left_over = left_over - glyph_ptr->lbearing[*glyphs];
		cx = cx + glyph_ptr->lbearing[*glyphs];

#if (defined(__DEBUG__))

		fprintf( stderr, "	cx =%d left_over=%d in partial end\n", 
				cx,left_over );
#endif
        ASSERT( cx+left_over-1 == v256_clip_x2 );

		hsize = (left_over + 7) >> 3;	/* bytes to output per line */
		ACL_XCNT  = left_over - 1;
	  	gdata     = data + size * *glyphs;
    	ch = glyph_ptr->ascent[*glyphs] + glyph_ptr->descent[*glyphs];
	    cy = y - glyph_ptr->ascent[*glyphs];

   	    if ( cy+ch-1 > v256_clip_y2 ) {

	 		overlap = cy+ch-1-v256_clip_y2;
	 		ch     -= overlap;
     	 }
    	 if ( cy < v256_clip_y1 ) {

			overlap = v256_clip_y1-cy;
			ch     -= overlap;
			gdata  += overlap*lsize;
			cy     += overlap;
    	 }
		 if ( ch > 0 )  {

#if (defined(__DEBUG__))
	 		if ( cy < v256_clip_y1 || 
		  	 	 cy > v256_clip_y2 ||
		  		 cx > v256_clip_x2 ||
			     cx < v256_clip_x1 ) {

               fprintf( stderr, 
				  "cx=%d cy=%d v256_clip_x2=%d end blit Assertion violation\n",
				  cx, cy, v256_clip_x2 );

               fprintf( stderr, "hidden_start=%d left_over=%d gcount=%d\n", 
						hidden_start, left_over, gcount );
           }
#endif

		 ACL_YCNT = ch-1;
		 ACL_DADDR = cy * etw32p_disp_x + cx;
		 while ( ch-- ) {
		    for (j=0;j<hsize; ++j) {

				*(volatile char *)(MMU_AP0+j) = *(char *)gdata++;
	    	}
		    gdata += lsize - hsize;
	    }
	   WAIT_ON_ACL();
     }

  }

  fontblit_succeed:
  	WAIT_ON_ACL();

#if (defined(__DEBUG__))

    fprintf( stderr, "}\n" );
#endif

    return(SI_SUCCEED);
}

/*
 *  etw32p_font_stplblt(num, x, y, cnt, glyphs type)
 *                : stipple glyphs in a downloaded font.
 *
 *    Input:
 *       int num	: font index to stipple from
 *       int x,y	: position to stipple to (at baseline of font)
 *       int count	: number of glyphs to stipple
 *       SIint16 *glyphs : list of glyph indices to stipple
 *       int forectype  : Opaque or regular stipple (if non-zero)
 */

SIBool
etw32p_font_stplblt(num, x, y, gcount, glyphs, forcetype)
SIint32 num, x, y, gcount, forcetype;
SIint16 *glyphs;
{

    int addr, lsize, h, w, ch, cw, cx, cy, opqflag;
    int csize, size, overlap, goverlap, leftover_end;
    int hidden_start;
    BLTDATA *data;
    register BLTDATA *gdata;
	register int j;
	etw32p_font_rec *glyph_ptr;

    /*
	 *  cx: x-coridinate from which font_blit starts.
	 *  cy: y-cordiante from which font_blit starts.
	 *  ch: height for the font_blit.
	 *  cw: width for font_blit.
     *
	 *  lsize:   no of bytes for a glyph_line.
	 *  csize:   no of bytes for a entire glyph.
	 *  goverlap: no. of glyphs which are outside the cliprectangle.
	 *  overlap:  temporary variable which holds, amount of width or height
	 *              outside cliprectangle.
	 *
	 *  hidden_start:  When only a portion of first glyph is to be drawn, this
	 *                  holds amount to be added to cx so as to get
	 *                  starting x-cordinate for the font_blit.
	 *
	 *  leftover_end:  When only a portion of last glyph is to be drawn, this
	 *                  holds the howmuch of width of the last glyph to be
	 *                  drawn.
	 *
	 *      addr    :  Holds addr in display memory  at which  font_blit should
	 *                  start.   
	 *  
	 *      gdata   :  contains  glyph data.      
	 *      data    :  pointer to glyph data.
	 */


     glyph_ptr = &etw32p_fonts[num];
     if (    !glyph_ptr->is_terminal_font  ) {

     	 /*
	   	  * not a terminal font or hardware terminal font drawing is disabled.
	   	  */
			return ( etw32p_non_terminal_font_stplblt(num, x, y, gcount,
			     glyphs, forcetype) );

     }


#if (defined(__DEBUG__))
     fprintf(stderr, 
			"\netw32p_font_stplblt(x=%d,y=%d,gcount=%d){\n", x, y, gcount  );
#endif

    data  = glyph_ptr->data;
    size  = csize  = glyph_ptr->size;
    lsize = glyph_ptr->lsize;

	ASSERT ( lsize > 0 && lsize < 5 );
	ASSERT ( csize > 0 && csize <= 32 * 4 );

    ch = h = glyph_ptr->max_height;
    w  = glyph_ptr->w[0];
    cy = y - glyph_ptr->lascent;
    cx = x;

    /*
    || Check for y-clipping : these clipping adjustments affect every
    ||		glyph blit (data and csize may be modified).
    */

    if ( cy+ch-1 > v256_clip_y2 ) {

		overlap = cy+ch-1-v256_clip_y2;
		ch     -= overlap;
		csize  -= overlap*lsize;
    }
    if ( cy < v256_clip_y1 ) {

		overlap = v256_clip_y1-cy;
		ch     -= overlap;
		csize  -= overlap*lsize;
		data   += overlap*lsize;
		cy     += overlap;
    }

    if ( ch <= 0 ) {

#if (defined(__DEBUG__))
       fprintf( stderr, 
				"		nothing needs to be drawn :ch=%d\n", ch );
#endif

	   return SI_SUCCEED;
    }

    /*
    || Check for x-clipping : these affect start glyph & gcount, and set up
    ||	 variables for partial-width blits: leftover_env & hidden_start.
    */

    if (cx < v256_clip_x1) {

#if (defined(__DEBUG__))

        fprintf( stderr, "	cx is less than clip_x1, clipping..\n" );
        fprintf( stderr, "	before clipping: cx=%d", cx );
#endif

		overlap  = v256_clip_x1 - cx;
		goverlap = overlap / w;		    /* no. of glyphs TOTALLY clipped */
		gcount  -= goverlap;		  /* skip all totally-clipped glyphs */
		hidden_start = overlap - goverlap*w;   /* and this much of 1st glyph */

		glyphs += goverlap;
		cx     += goverlap * w;

#if (defined(__DEBUG__))

        fprintf( stderr, "	after clipping: cx=%d\n", cx );
#endif

   	 }
	 else {

		hidden_start = 0;
     }
 
	if ( cx > v256_clip_x2 || gcount < 0 ) {

#if (defined(__DEBUG__))
       fprintf( stderr, 
				"	nothing needs to be drawn:v256_clip_x2=%d cx=%d\n", 
				 v256_clip_x2, cx );
#endif

	   return SI_SUCCEED;
    }

    if ( cx + gcount*w - 1 > v256_clip_x2 ) {

		overlap  = cx + gcount*w - 1 - v256_clip_x2;
		goverlap = (overlap + w -1) / w;	/* # of glyphs completely clipped */
		gcount  -= goverlap;
		leftover_end = goverlap*w - overlap;/* pixel width left over */
    }
	else {

		leftover_end = 0;
    }

    if ( gcount < 0  ||
		 gcount == 0 && 
		 leftover_end <= hidden_start ) {

        /* 
		 * if gcount is zero, just skip.
		 * if gcount is 1, hidden start > leftover_end,  means nothings
		 * should be drawn. so, return ...
		 */
#if (defined(__DEBUG__))
       		fprintf( stderr, 
					 "	nothing needs to be drawn:gcount=%d leftover_end=%d\
					 hidden_start=%d\n", gcount, leftover_end, hidden_start );
#endif
		    return SI_SUCCEED;
    }
   
	ASSERT( (hidden_start && leftover_end && gcount >= 0) ||
			(!hidden_start ) || 
			!leftover_end );

#if (defined(__DEBUG__))
	fprintf( stderr, "	hidden_start=%d left_over=%d\n",
					 hidden_start, leftover_end );
#endif

    /*
    || Since the pattern pixmap doubles as pmask and background color, a
    || conflict occurs when pmask != 0xFF during an opaque stipple blit.
    || In that case, we first fill the area with the background color and
    || then stipple transparently.
    */

    CHECK_ACL();
    addr    = ( cy * etw32p_disp_x ) + cx + hidden_start;
    opqflag = ( (forcetype == SGStipple) ? 0 : 1 );

    ACL_DIR   = DIR_NORMAL;
    ACL_YCNT  = ch - 1;


    if ( opqflag && 
		 v256_gs->pmask != MASK_ALL ) {

#if (defined(__DEBUG__))
        fprintf( stderr, "	opqflag set and pmask=!MASK_ALL\n");
        fprintf( stderr, "	hence solid fill first.\n");
#endif
		/* write over text area with background color */
		SET_SRC_SOLID(v256_gs->bg);

		ACL_RO   = RO_ADAUTO | RO_DAAUTO;
		ACL_XCNT = gcount*w + leftover_end - hidden_start - 1;
		ACL_DADDR= addr;
		opqflag  = 0;			/* use pattern as pmask */
		WAIT_ON_ACL();
    }

    SET_SRC_SOLID(v256_gs->fg);			/* set 'source' to fg */

    if (opqflag) {

     	 SET_PAT_SOLID(v256_gs->bg);		/* set 'pattern' to bg */
    }

    ACL_RO = RO_DAMIX | RO_ADAUTO;

   /* 
	* Select transparent or opaque. (ROP_NOOP vs. ROP_PCOPY) 
	*/
    ACL_BGROP     = (opqflag ? etw32p_bg_rop : ROP_NOOP);
    ACL_PROC_BYTE = PROC_BYTE_1;		/* write 4 bytes at a time */

	/*
	 *  For ms_stplblt, set these values
	 */
	ACL_MADDR     = 0;
	ACL_MYOFF     = 31;
    /*
	 * background color is set by etw32p_select_state(), and never reset 
	 */

    /*
    || Perform initial partial blit if necessary
    */

    if ( hidden_start ) {

		int hsize;
		if ( !gcount ) {

		     cw = leftover_end-hidden_start;
#if (defined(__DEBUG__))
			 fprintf( stderr, "	cw=%d hidden_start=%d left_over=%d\n", 
					  cw, hidden_start, leftover_end );
#endif
		} 
		else {

	   		 cw =w-hidden_start;
#if (defined(__DEBUG__))
			 fprintf( stderr, "cw=%d hidden_start=%d \n", 
					  cw, hidden_start);
#endif

		}
		ASSERT( hidden_start > 0 );
		ASSERT( hidden_start < 32 );
		ASSERT( cw >= 1 );

		ACL_XCNT = cw - 1;
		hsize = (cw + 7) >> 3;			/* # of bytes per line */
		gdata = data + size * *glyphs++;	/* glyph data */
		{
			int mask;
			mask = (~(unsigned int)0) >> (32-lsize*8);
           
   	       /*
		    *   mask  the additional (unwanted) leading bytes
		    *   Note:  lsize can be maximum of 4 bytes.
		    */

			ACL_DADDR= addr;
			j = ch;
			while ( j-- ) {

		   	  int gword;
			  register int k;
			  char *gword_ptr;

			  gword_ptr = (char *)&gword;
			  gword = (*(int *) gdata) & mask;
              gword >>= hidden_start;
		      /*
			   *  before first integer write, shift out hidden bits
			   */
			  for (k=0; k< hsize; ++k ) {
               
			    *(volatile char *)(MMU_AP0+k) = *(char *)gword_ptr++;
		      }
		      gdata += (lsize);
		   }
       }
	   WAIT_ON_ACL();
	   /*
		*  Hmm.., bad practice to use goto...
		*/
	   if ( !gcount )  {

#if (defined(__DEBUG__))

                    fprintf( stderr, "	gcount is zero after start partial stplblt.  Going to fontblit_succeed\n" );
#endif
					goto fontblit_succeed;
       }
		--gcount;
		addr += (cw);
    }	

    /*
    || Perform gcount full-width blits
    */

	    ACL_XCNT = w - 1;
		if ( lsize > 1 ) {


#if (defined(__DEBUG__))

            fprintf( stderr, "	no of bytes per glyph line is more than 1\n" );
            fprintf( stderr, "	that is Width is greater than 8\n" );
#endif

            while ( gcount-- ) {

              register int	k;

			  ACL_DADDR = addr;
		  	  gdata     = data + size * *glyphs++;
			  j      	= ch;

              while ( j-- ) {

			  	for (k=0; k<lsize; ++k)
		       	 	*(volatile char *)(MMU_AP0+k) = *(char *)gdata++;
              }

	    	  addr += w;
	          WAIT_ON_ACL();
       	   }

     }
	 else {

#if (defined(__DEBUG__))
            fprintf( stderr, "	no of bytes per glyph line  <= 1\n" );
            fprintf( stderr, "	that is Width <= 8\n" );
#endif

            while ( gcount-- ) {

			  	ACL_DADDR=addr;
	  		  	gdata = data + size * *glyphs++;
			  	j = csize;
			  	while ( j-- )
	   		   	  *(volatile  char *)(MMU_AP0) = *(char *)gdata++;
	     	  	addr += w;
	     	  	WAIT_ON_ACL();
	    	}
     }

    /*
    || Take care of partial-width ending blit
    */

    if (leftover_end) {

		int hsize;
		hsize = (leftover_end + 7) >> 3;	/* bytes to output per line */
		ACL_XCNT  = leftover_end - 1;
		ACL_DADDR = addr;
		gdata     = data + size * *glyphs++;

        while ( ch-- ) {

		    for (j=0;j<hsize; ++j)
				*(volatile char *)(MMU_AP0+j) = *(char *)gdata++;
		    gdata += lsize - hsize;
	   }
	   WAIT_ON_ACL();

   }

   fontblit_succeed:
    if ( opqflag )  {
      SET_PAT_SOLID(v256_gs->pmask);	/* restore pattern pixmap */
    }
	WAIT_ON_ACL();

#if (defined(__DEBUG__))
	fprintf( stderr, "}\n\n" );
#endif

    return(SI_SUCCEED);
}




/*
 *    etw32p_font_free(num) : Free data structures associated with a
 *                downloaded font.
 *
 *    Input:
 *       int num : index of font
 */
SIBool
etw32p_font_free(num)
SIint32 num;
{
    /*fprintf(stderr, "etw32p_font_free(%d)\n", num); */

    FREE(etw32p_fonts[num].data);
    etw32p_fonts[num].data = 0;

    return(SI_SUCCEED);
}

