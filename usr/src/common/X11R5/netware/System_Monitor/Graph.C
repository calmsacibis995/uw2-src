#ident	"@(#)systemmon:Graph.C	1.9"
//////////////////////////////////////////////////////////////////////
// Graph.C:  Graph of counters 
/////////////////////////////////////////////////////////////////////
#include "Application.h"
#include "main.h"
#include "WorkArea.h"
#include "Graph.h"
#include "i18n.h"
#include <stdlib.h>
#include <Xm/DrawingA.h>
#include <Xm/BulletinB.h>
#include <Xm/ScrolledW.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <ctype.h>
#include "metrics.h" 

/*
 * 	GLOBAL DEFINES
 */
#define NEWLINE			"\n"	

#define  USER_TIME 		0
#define  SYS_TIME		1
#define  WIO_TIME		2
#define  IDLE_TIME		3
#define  FREEMEM		4
#define  FREEBLOCKS		5
#define  PGIN			6
#define  PGOUT			7	
#define  SWAPIN			8
#define  SWAPOUT		9
#define  BSWAPIN		10
#define  BSWAPOUT		11
#define  PSWTCH			12

enum { SINGLE_BYTE,  DOUBLE_BYTE, MULTI_BYTE };
#define	HUNDRED			"100"

/*
 *	defs for the top level functs
 */
extern "C" int	snap_mets( void ); 

Graph::Graph (Widget parent, WorkArea *w, char *name) : BasicComponent (name)
{
	XmFontList		fontlist;
	Widget			bulletin;

	/* Set all the variables in the ctor - initialize 
	 */
	_workArea = w;
	_procid = NULL;
	_drew_number = False;
	_maxy = _total = 0; _eachy = _lasty = _mark = 0;
	_graph_width = _graph_height = _ascent = _descent = 0, _eachx = 0;
	_gcmask = 0;

	/* create the drawing area widget to hold the graph
	 */
	_w = XtVaCreateManagedWidget (name, xmDrawingAreaWidgetClass, parent, 
					XmNtraversalOn, 		False,
					0);

	/* Create a dummy bulletin board in order to get the font
	 * list out of it
	 */
	bulletin = XtVaCreateWidget ("Dummybulletin", 
				xmBulletinBoardWidgetClass, _w, NULL, 0);
	XtVaGetValues (bulletin, XmNtextFontList, &fontlist, 0);	
	
	/* Use the fonlist to extract the XFontStruct * pointer and
	 * get the font details for calculation of text extents
	 */
	GetFontType (fontlist);

	/* Create the GC for clear lines to be drawn,
	 */
  	_cleargc =  _workArea->GetClearGC(_w);

	/* register all the callbacks for the graph - expose and resize 
	 * and input
	 */
	XtAddCallback (_w, XmNresizeCallback, &Graph::graph_resizeCB, 
							(XtPointer)this);
    	XtAddCallback (_w, XmNexposeCallback, &Graph::graph_exposeCB, 
							(XtPointer)this);
    	XtAddCallback (_w, XmNinputCallback, &Graph::graph_inputCB, 
							(XtPointer)this);
}

/*-------------------------------------------------------------
  **	graphbutton press/release
  **            graphbutton
  *------------------------------------------------------------*/
void  Graph::graph_inputCB(Widget, XtPointer client_data, XtPointer call_data)
{
	Graph *obj = (Graph *) client_data;
	XmDrawingAreaCallbackStruct 	*cb = (XmDrawingAreaCallbackStruct *)
						call_data;

	obj->graph_input(cb->event);
}

void Graph::graph_input (XEvent *event)
{
	switch (event->xany.type) {
		case ButtonRelease:
				/* If the number had been drawn clear the
				 * screen, redraw the old screen without 
				 * the number
				 */
				if (_drew_number) {
					XClearArea(XtDisplay (_w),XtWindow (_w),
							 0, 0, 0, 0, True);
					_drew_number = False;
				}
				break;
		case ButtonPress:
				graph_buttonpress (event);
				break;
	}
}

/*-------------------------------------------------------------
  **	Draw the value of the point where the button was clicked	
  ** 	displaying the actual value (based on scale) on the canvas
  *------------------------------------------------------------*/
void Graph::graph_buttonpress (XEvent *event)
{
	int 	i, j;
	Boolean	found = False;

	_drew_number = False; /* Set boolean variable to false */
	for (j = 0;  j < SAR_TOTAL; j++) { /* for all items in sar options */ 

		if (_workArea->_sar[j].selected == False)  
			continue; 	/* ignore unselected items */

		for (i = 0; i < _total; i++) { 

			if (!i) {
				if (event->xbutton.x >=  _maxx + PIXEL &&
				event->xbutton.x <= _workArea->_sar[j].x[i] +
					EACH_WIDTH) 			{
					if (event->xbutton.y >= 
						_workArea->_sar[j].y[i]-2 &&
						event->xbutton.y <= 
						_workArea->_sar[j].y[i]+2 ) {
						found = True;
						break;
					}
				}
			}
			else if (event->xbutton.x >= _workArea->_sar[j].x[i] &&
				event->xbutton.x <= _workArea->_sar[j].x[i] +
							EACH_WIDTH) {
				
				/* if y is > current y then if its < nexty */
				if (event->xbutton.y >= _workArea->_sar[j].y[i]
					&& event->xbutton.y <= 
						_workArea->_sar[j].y[i-1]) {
						found = True;
						break;
				}
				/* if y is < current y then if its > nexty */
				else if (event->xbutton.y <= 
						_workArea->_sar[j].y[i] && 
						event->xbutton.y >= 
						_workArea->_sar[j].y[i-1]) {
						found = True;
						break;
				} 	
			} 	/* if x is within previous x and current x */
		}		/* for all selected items */

		/* if value was found, set color, calc display value */
		if (found) { 
			char 	buf[10];
			int	ival, x, y;
			GC	localgc;
			double 	value;

			/* Use the GCValues that was set up in the beginning
			 * to get a new graphic context. All we need to do is 
			 * change the foreground color.
			 */
			_nameval.foreground = _workArea->_sar[j].color;
  			localgc = XCreateGC (XtDisplay(_w), 
					RootWindowOfScreen(XtScreen(_w)), 
					_gcmask, &_nameval);  

			/* re-calc the value based on default scale 
			 */
			value = _workArea->_sar[j].value[i] * (double)
				(_workArea->_sar[j].def_scale / PERCENT);

			ival = (int) value;
			sprintf (buf, "%d", ival);

			/* Draw the button number on the graph using
			 * a font based on the fonttype that was set
			 */
			x = event->xbutton.x;
			y = event->xbutton.y;
			switch (_fonttype) {
				case SINGLE_BYTE:	
	 				XDrawImageString (XtDisplay(_w), 
						XtWindow(_w), localgc, 
						x, y, buf, strlen (buf));
					break;
				case DOUBLE_BYTE:	
	 				XDrawImageString16 (XtDisplay(_w), 
						XtWindow(_w), localgc, x, y, 
						(XChar2b *)buf, strlen (buf));
					break;
				case MULTI_BYTE:	
	 				XmbDrawImageString (XtDisplay(_w), 
						XtWindow(_w), (XFontSet)
						(XFontSet) _fontptr, localgc, 
						x, y, buf, strlen (buf));
					break;
			}	
			/* Free the graphic context when we are done
			 * with displaying the number each time
			 */
			XFreeGC (XtDisplay(_w), localgc);
			_drew_number = True; /* set drawn boolean to true */
			break;
		}
	} /* for all sar options in the table */
}

/*-------------------------------------------------------------
  **	graph resize
  **            graph_resize
  *------------------------------------------------------------*/
void  Graph::graph_resizeCB(Widget, XtPointer client_data, XtPointer call_data)
{
//  XmDrawingAreaCallbackStruct
//	 *cb 	= (XmDrawingAreaCallbackStruct *) call_data;

	Graph *obj = (Graph *) client_data;
	obj->graph_resize ();

}

void Graph::graph_resize ()
{
	int 	width;

	width = _graph_width;

  	XtVaGetValues (_w, 
			XmNwidth, &_graph_width,
  			XmNheight, &_graph_height,
			0);

	/* clean out graph and restart plotting from the beginning
	 */
	if (_eachx >= width && _graph_width < width) {
		_eachx = _maxx + PIXEL;
		_total = 0;
	}
	//printf ("width height is %d %d\n", _graph_width, _graph_height);
	if (XtIsRealized (_w)) 
		XClearArea(XtDisplay (_w),XtWindow (_w), 0, 0, 0, 0, True);
}

/*-------------------------------------------------------------
  **	graph expose
  **            graph_expose
  *------------------------------------------------------------*/
void   Graph::graph_exposeCB(Widget, XtPointer client_data, XtPointer call_data)
{
	XmDrawingAreaCallbackStruct *cb = (XmDrawingAreaCallbackStruct *) 
					call_data;
	Graph *obj = (Graph *) client_data;
	obj->graph_expose (cb) ;
}

void Graph::graph_expose (XmDrawingAreaCallbackStruct *cb)
{
	int	i, j;
#if 0
  	static Region   	region;

	/*****************************************************
	 * Create the region add exposure and set the
	 * region for the exposed area
	 ***************************************************/
  	if (!region) 
	 	region = XCreateRegion ();
  	XtAddExposureToRegion (cb->event, region);
  	if (cb->event->xexpose.count != 0)
	 	return ;
  	XSetRegion (XtDisplay(_w), _namegc, region);

	/*****************************************************
	 * if the exposed rectangle falls out of the region
	 * then return
	 ***************************************************/
	if (XRectInRegion (region, 0, 0, _graph_width, _graph_height) 
			== RectangleOut)
		return;
#endif

	/*****************************************************
	 * Get the point of origin x,y from where the
	 * lines are going to be drawn. Calulate the height,
	 * the height of each mark and the last mark on screen
	 ***************************************************/
	_maxy = _graph_height - (_ascent + _descent + PIXEL); 	/* Y ORIGIN */
	_eachy = (_maxy - YOFFSET) / PERCENTAGE ;		/* Each y */
	_mark = floor((_eachy * UNIT) + 0.5);			/* MARK */
	_lasty = _maxy - (_mark * UNIT);

	/* DRAW THE X AND Y AXIS LINES */
	DrawXandYAxis (); 		
			
	/*****************************************************
	 * If the timeout has not been generated
	 * then create the routine first time
	 * else redraw the points on receiving
	 * an expose event
	 ***************************************************/
	if (_procid == NULL)  {
		_total = 0;
		_eachx = _maxx + PIXEL;
		plot_graph (this, NULL);
	}
	else {
		/* redraw the last updated time
	 	 */
		DrawUpdateTime (False);
		/*****************************************************
		 * for all the points that have been drawn for each of
	 	 * the lines draw them again on an expose event
	 	***************************************************/
		for (j = 0; j < SAR_TOTAL; j++) {
			int	x = _maxx + PIXEL;
			if (_workArea->_sar[j].selected == False) 
				continue;
			else { 
				for (i = 0; i < _total; i++) {
					DrawLine (i, j, x);
					x += EACH_WIDTH;
				}
			}
		}
	}

#if 0
	/*****************************************************
	 * Destroy the region
	 ***************************************************/
	XDestroyRegion (region);
	region = NULL;
#endif
}

/************************************************************
 * This routine draws the sar line.  It recalculates the value 
 * based on the scale and draws the line out.
 ************************************************************/
void Graph::DrawLine (int i, int j, int x)
{
	double 		y = (double) _maxy;
	double		sarvalue, value;
		
	/************************************************************
	 * If the scales are different from the default scale
	 * calculate the value to the new scale in terms of a 
	 * percentage, otherwise value is expressed as a percentage
	 ************************************************************/
	if  (_workArea->_sar[j].scale != _workArea->_sar[j].def_scale) {
		sarvalue = (double) (_workArea->_sar[j].def_scale / PERCENTAGE) 
					* _workArea->_sar[j].value[i] ;
		sarvalue = (sarvalue * PERCENTAGE )/ (double)
				_workArea->_sar[j].scale;
		if  (sarvalue  > PERCENTAGE) 
			sarvalue = PERCENTAGE;
	}
	else 
		sarvalue = _workArea->_sar[j].value[i]; 

	if (sarvalue != PERCENTAGE) {
		// eachy wont work because mark was rounded up and
		// the difference would be too much i.e if eachy was
		// 4.85 for e.g., and _mark was 49 then eachy is actually
		// 4.9 //value = floor ((sarvalue * _eachy) + 0.5); 
		value = floor ((sarvalue * (_mark/UNIT)));

		/************************************************************
		 * 	Calculate the y values 
	 	 ************************************************************/
		if (y - value < _lasty)  {
			_workArea->_sar[j].y[i] = (int)_lasty;
		}
		else	
			_workArea->_sar[j].y[i] = (int)(y - value);
	}
	else
		_workArea->_sar[j].y[i] = (int)_lasty;

	/************************************************************
	 * Draw the INFO line  if the item has been selected
	 * otherwise go on to the next item.
	 ************************************************************/
	if (_workArea->_sar[j].selected == True) {
		if (_eachx <= _graph_width)
			XDrawLine (theApplication->display(), XtWindow (_w), 
				_workArea->_sar[j].gc,  x, 
				i == 0 ? _workArea->_sar[j].y[i] : 
				_workArea->_sar[j].y[i-1], 
				x + EACH_WIDTH, _workArea->_sar[j].y[i]);
		else
			XDrawLine (theApplication->display(), XtWindow (_w), 
				_workArea->_sar[j].gc,  x, 
				i == 0 ? _workArea->_sar[j].y[_total] : 
				_workArea->_sar[j].y[i-1], 
				x + EACH_WIDTH, _workArea->_sar[j].y[i]);
	}
}
  	
/*
 *	per-processor metrics
 */
extern struct met *usr_time;
extern struct met *sys_time;
extern struct met *wio_time;
extern struct met *idl_time;
extern struct met *swpin_cnt;
extern struct met *swpout_cnt;
extern struct met *pswpin_cnt;
extern struct met *pswpout_cnt;
extern struct met *vpswpout_cnt;
extern struct met *pgin_cnt;
extern struct met *pgout_cnt;
extern struct met *pgpgin_cnt;
extern struct met *pgpgout_cnt;
extern struct met *pswtch_cnt;

/*
 *	global metrics supplied by system
 */
extern struct dblmet *freemem;
extern struct dblmet *freeswp;

/*
 * 	calculated global metrics
 */
extern struct met *cusr_time;
extern struct met *csys_time;
extern struct met *cwio_time;
extern struct met *cidl_time;
extern struct met *cswpin_cnt;
extern struct met *cswpout_cnt;
extern struct met *cpswpin_cnt;
extern struct met *cpswpout_cnt;
extern struct met *cvpswpout_cnt;
extern struct met *cpgin_cnt;
extern struct met *cpgout_cnt;
extern struct met *cpgpgin_cnt;
extern struct met *cpgpgout_cnt;
extern struct met *cpswtch_cnt;

void Graph::plot_graph (XtPointer client_data, XtIntervalId *)
{
	Graph			*obj = (Graph *) client_data;

//	double			y = (double) obj->_maxy;
	double 			masval;
	char			buf[BUFSIZ];
	int			i, j, index, xvalue;

	/************************************************************
	 * If the end of the chart is reached then clear area, drawx axis,
	 * move all pts to left, move the index back 1 and reduce xvalue
	 * by EACH_WIDTH else keep the index and xvalue the same.
	 ************************************************************/
	if (obj->_eachx > obj->_graph_width) {
		XClearWindow (XtDisplay (obj->_w), XtWindow (obj->_w));
		obj->DrawXandYAxis();
		obj->MovePointsOver ();

		index = obj->_total - 1;
		xvalue = obj->_eachx - EACH_WIDTH;
	}
	else {
		index = obj->_total;
		xvalue = obj->_eachx;
	}

	/************************************************************
	* Get the latest time and update the last time stamp on screen
	  ************************************************************/
	obj->DrawUpdateTime(True);

	/************************************************************
	 * Loop thru all the Sar Table entries and store the
	 * value to be drawn on the graph
	 ************************************************************/
	snap_mets();
	for (j = 0;  j < SAR_TOTAL; j++) {

		i = obj->_workArea->_which_cpu;

		switch (j) {
			case USER_TIME: 
					masval = (i == GLOBAL) ? 
					cusr_time[0].cooked :
					usr_time[i].cooked ;
					break;
			case SYS_TIME: 
					masval = (i == GLOBAL) ? 
					csys_time[0].cooked :
					sys_time[i].cooked ;
					break;
			case WIO_TIME: 
					masval = (i == GLOBAL) ? 
					cwio_time[0].cooked :
					wio_time[i].cooked ;
					break;
			case IDLE_TIME: 
					masval = (i == GLOBAL) ? 
					cidl_time[0].cooked :
					idl_time[i].cooked ;
					break;
			case FREEMEM: 	
					/* Free memory appears in units
					 * therefore it needs to be expressed
					 * just as sar does
					 */
					masval = freemem[0].cooked * 100;
					break;
			case FREEBLOCKS: 
					masval = freeswp[0].cooked ;
					break;
			case SWAPIN: 
					masval  = (i == GLOBAL) ? 
					cswpin_cnt[0].cooked :
					swpin_cnt[i].cooked ;
					break;
			case SWAPOUT: 
					masval = (i == GLOBAL) ? 
					cswpout_cnt[0].cooked :
					swpout_cnt[i].cooked ;
					break;
			case BSWAPIN: 
					masval = (i == GLOBAL) ? 
					cpswpin_cnt[0].cooked :
					pswpin_cnt[i].cooked ;
					break;
			case BSWAPOUT: 
					masval = (i == GLOBAL) ? 
					cpswpout_cnt[0].cooked :
					pswpout_cnt[i].cooked;
					break;
			case PSWTCH: 
					masval = (i == GLOBAL) ? 
					cpswtch_cnt[0].cooked :
					pswtch_cnt[i].cooked;
					break;
			case PGIN: 	
					masval = (i == GLOBAL ) ?
					cpgin_cnt[i].cooked :
					pgin_cnt[i].cooked ;
					break;
			case PGOUT: 	
					masval = ( i == GLOBAL) ? 
					cpgout_cnt[i].cooked :
					pgout_cnt[i].cooked;
					break;
		}

		/************************************************************
		 * integerise the string, round up to next value, 
	 	 * reduce value from height , calculate previous value
		 * then draw the point (in terms of percentages). 
	 	************************************************************/
		obj->_workArea->_sar[j].value[index] = masval  
		* (PERCENTAGE / (double)obj->_workArea->_sar[j].def_scale);

		/************************************************************
		 * Log data into file if the flag is set and set the alarm
	 	 ***********************************************************/
		if (obj->_workArea->_log) {
			sprintf (buf, ";%d", (int) 
				obj->_workArea->_sar[j].value[index]);
			obj->_workArea->WriteFile (buf);
			if (j == SAR_TOTAL - 1)
				obj->_workArea->WriteFile (NEWLINE);
		}
		obj->SetAlarm (masval, j);
	
		obj->_workArea->_sar[j].x[index] = xvalue;
		obj->DrawLine (index, j, xvalue);
	}

	/************************************************************
	 * INCREMENT THE COUNTERS  if end of screen is not reached
	 ************************************************************/
	if (obj->_eachx <= obj->_graph_width) {
		obj->_eachx += EACH_WIDTH;
		obj->_total++; 			/* add up the cycles */
	}

	/************************************************************
	 * create the timeout - so that this routine keeps getting
	 * called  again and again
	 ************************************************************/
	obj->_procid = XtAppAddTimeOut (theApplication->appContext (), 
			obj->_workArea->GetTimer(), (XtTimerCallbackProc) 
			&Graph::plot_graph, obj);
} 
	
/**************************************************************************
 * Move the points over to the left by 1.  Get rid of the 0-th item each 
 * time but store it in the last value _total for redraw
 ************************************************************************/
void Graph::MovePointsOver()
{
	int 		i, j, x;

	for (j = 0; j < SAR_TOTAL; j++) {

		/* all sar options need to start at the x axis
	 	 */
		x  = _maxx + PIXEL;

		/* if the item was unselected skip it
		 */ 
		if (!_workArea->_sar[j].selected) 
			continue;

		/* for all points that have already been drawn shift
		 * them over by one.  Make sure the first one is stored
		 * in the _total position (which is unused) for redraw
		 * purposes
		 */
		for (i = 1; i < _total; i++) {

			XDrawLine (theApplication->display(), XtWindow (_w), 
					_workArea->_sar[j].gc, x, 
					_workArea->_sar[j].y[i-1], 
					x + EACH_WIDTH,_workArea->_sar[j].y[i]);

			if (i == 1)
				_workArea->_sar[j].y[_total] =  
					_workArea->_sar[j].y[i-1];

			_workArea->_sar[j].y[i-1] = 
				_workArea->_sar[j].y[i];

			_workArea->_sar[j].value[i-1] = 
				_workArea->_sar[j].value[i];

			/* increment the x values for the redraw
			 */
			x += EACH_WIDTH;
		}
	}
}

/************************************************************
 * Set the alarm to go off if the item was selected and the 
 * value was in alarm range.  Sound the bell/flash if neccessary,
 * and set up the alarm icon
************************************************************/
extern Pixmap 	newpixmap; 
extern void 	InstallNewPixmap (Widget, Pixmap);
extern Boolean 	iconified;

void Graph::SetAlarm(double masvalue, int j)
{
	Boolean			alarm = False;
	char			timebuf[25], buf[BUFSIZ];
	struct	tm		*timeset;

	/************************************************************
	 * If the item was selected 
 	 ************************************************************/
	if (_workArea->_sar[j].selected) {

		/************************************************************
	 	 * if the value is between what is set in the alarm_over/
		 * alarm_under variables, then set alarm to true and create 
		 * the buffer that you want to store in alarm file
 	 	 ************************************************************/
		if (_workArea->_sar[j].is_alarm)  {

			if (_workArea->_sar[j].alarm_over <
                                _workArea->_sar[j].alarm_under) {

				if ((int)masvalue >= 
					_workArea->_sar[j].alarm_over &&
					(int)masvalue < 
					_workArea->_sar[j].alarm_under){

					sprintf (buf, "%s: %d > %f < %d ",  
					_workArea->_sar[j].title, 
					_workArea->_sar[j].alarm_over, masvalue,
					_workArea->_sar[j].alarm_under);
					alarm = True;
				} /* value is between over and under */

			} /* if alarm above is less that alarm below */
 
		/************************************************************
	 	 * if the value is > alarm_over or if value is < alarm_under 
		 * variables, then set alarm to true, create buffer that you 
		 * want to store in alarm file
 	 	 ************************************************************/
  			else if (_workArea->_sar[j].alarm_over >
                                _workArea->_sar[j].alarm_under) {

				if (((int)masvalue > 	
					_workArea->_sar[j].alarm_over) ||
                        		((int)masvalue < 
					_workArea->_sar[j].alarm_under)) {

					if ((int)masvalue >
					_workArea->_sar[j].alarm_over)
						sprintf (buf, "%s: %f > %d ",  
						_workArea->_sar[j].title, 
						masvalue,
						_workArea->_sar[j].alarm_over);
					else if ((int)masvalue < 
						_workArea->_sar[j].alarm_under)
						sprintf (buf, "%s: %f < %d ",  
						_workArea->_sar[j].title, 
						masvalue,
						_workArea->_sar[j].alarm_over);
					alarm = True;

				} /* if value > alarm over or < alarm under */

			} /* if alarm above is greater than alarm below */

	   		else if (_workArea->_sar[j].alarm_over == 0) {

				if ((int)masvalue >  0)	
					alarm = True;

			} /* if alarm over was set to zero */

		} /* if there was an alarm */

		/* if the alarm went off */
		if (alarm && _workArea->_sar[j].is_alarm) { 

			FILE		*fp = NULL;

			/***************************************************
	 	 	 *  Open the alarm log file write buffer, close it
			 *  store the time/date and the processor #. (mp mach)
 	 	 	 ***************************************************/
			fp = fopen (_workArea->_alarmfilename, "a");
			if (fp) { 
				timeset = localtime (&_clock);
				sprintf (timebuf, "@%02d:%02d:%02d-%d/%d/%d.",
					timeset->tm_hour, timeset->tm_min, 
					timeset->tm_sec, timeset->tm_mon,
					timeset->tm_mday, timeset->tm_year);
				strcat (buf, timebuf);

				strcat (buf, I18n::GetStr (TXT_proc));
				if (_workArea->_which_cpu == GLOBAL)
					sprintf(timebuf, "%s\n", 
						I18n::GetStr (TXT_global));
				else
					sprintf(timebuf, "%d\n",
						_workArea->_which_cpu);
				strcat (buf, timebuf);

				fputs (buf,fp);
				fclose (fp);
			}
					
			/***********************************************
	 	 	 * if the beep was set to true sound the bell
 	 	 	 ***************************************************/
			if (_workArea->_sar[j].beep)
				XBell (theApplication->display(), 100);

			/***********************************************
	 	 	 * if the flash was set to true flash the header
 	 	 	***************************************************/
			if (_workArea->_sar[j].flash) {
				_workArea->_sar[j].flashed = True;
				XtVaSetValues (_workArea->_sar[j].toggle,
						XmNbackground, 
						_workArea->_sar[j].foreground,
						XmNforeground,
						_workArea->_sar[j].background,
						0);
			} /* flash header */
			
			/***********************************************
	 	 	 * manage the alarm icon for the user to see  
 	 	 	***************************************************/
			XtManageChild (_workArea->_sar[j].alarmicon);
			if (iconified) { 
				InstallNewPixmap (XtParent
					(_workArea->baseWidget()), newpixmap);
			}
		} /* if the value is in alarm range */
		else { 
			/* if the value was not in alarm range 
			 * reset the flash, remove the alarm icon
			 */
			_workArea->ResetSarToggle(j);
		}
	} /* item was selected */
}

/************************************************************
 * This routine gets the latest time and updates the string
 * on screen under the X axis
 ***********************************************************/
void Graph::DrawUpdateTime(Boolean from_timeout)
{
	char		*tmp = NULL,buf[20];
	int 		y;
	int		interval = _workArea->GetTimer()/1000;
	struct tm 	*timeset = NULL;

	if (from_timeout)	
		_clock = time(0);

	timeset = localtime(&_clock);

	/*****************************************************
	 * Draw the time that the graph starts
	 ***************************************************/
	y = _graph_height - PIXEL;
	sprintf (buf, "%02d:%02d:%02d", timeset->tm_hour, timeset->tm_min, 
		timeset->tm_sec);

	tmp = XtMalloc(strlen(buf) + strlen(I18n::GetStr (TXT_lastupdate)) + 2);
	strcpy (tmp, I18n::GetStr (TXT_lastupdate));
	strcat (tmp, buf);
	switch (_fonttype) {
		case SINGLE_BYTE:	
  			XDrawImageString (XtDisplay(_w), XtWindow(_w), 
					_namegc, _maxx, y, tmp, strlen(tmp));
			break;
		case DOUBLE_BYTE:	
  			XDrawImageString16 (XtDisplay(_w), XtWindow(_w), 
					_namegc, _maxx, y, (XChar2b *)tmp, 
					strlen(tmp));
			break;
		case MULTI_BYTE:	
	 		XmbDrawImageString (XtDisplay(_w), XtWindow(_w), 
					(XFontSet)_fontptr, _namegc, _maxx, y,
					tmp, strlen(tmp));
			break;
	}
	XtFree (tmp);

	if (from_timeout && _workArea->_log) {
		sprintf (buf, "%d;%d", _clock, interval); 
		_workArea->WriteFile (buf);
	}
}

/************************************************************
 * Draw the time string across the x axis. 
 ************************************************************/
void Graph::DrawTimeString ()
{
	char 	*tmp, buf[10];
	int	x, y;

	/*****************************************************
	 * x axis  Draw the  TIME variable  string
	 * Position the Time in the middle of the xaxis
	 ***************************************************/
	y = _graph_height - PIXEL;
	sprintf (buf, " %d", _workArea->GetTimer()/1000);
	tmp = XtMalloc (strlen (I18n::GetStr(TXT_interval)) + strlen (buf) + 2);
	strcpy (tmp , I18n::GetStr (TXT_interval));
	strcat (tmp , buf);
	x = _graph_width / 2 ; 
	switch (_fonttype) {
		case SINGLE_BYTE:	
  			XDrawImageString (XtDisplay(_w), XtWindow(_w), 
					_namegc, x, y, tmp, strlen (tmp));
			break;
		case DOUBLE_BYTE:	
  			XDrawImageString16 (XtDisplay(_w), XtWindow(_w), 
					_namegc, x, y, (XChar2b *)tmp, 
					strlen (tmp));
			break;
		case MULTI_BYTE:	
	 		XmbDrawImageString (XtDisplay(_w), XtWindow(_w), 
					(XFontSet)_fontptr, _namegc, x, y, 
					tmp, strlen (tmp));
			break;
	}
	XtFree (tmp);
}

/*****************************************************
 * A generic draw x and y axis routine. Also draws
 * grid lines if necessary
 ***************************************************/
void Graph::DrawXandYAxis ()
{
  	int             	i, x, y;
	char			buf[10];
	static Boolean		drawn_hz =  False;
	static Boolean		drawn_vt =  False;
	
  	/* draw the x/y axis line */
  	XDrawLine (XtDisplay(_w), XtWindow(_w), _namegc, _maxx, _maxy, 
		_graph_width, _maxy);
  	XDrawLine(XtDisplay(_w), XtWindow(_w), _namegc, _maxx, _maxy, _maxx, 0);

  	/*****************************************************
	 * draw the y values for usage on the line 
	 * depending on the scale that is being used
	 ***************************************************/
	x = 0; 
	y = _maxy; 
  	for (i = 0; i <= PERCENT; i += (PERCENT / UNIT)) {

		sprintf (buf, "%d", i);
		DrawYValues (buf, y);

		if (i != PERCENT) {
	 		y -=(int) _mark; 		/* - y by eachy */

			/* DRAW the Horizontal Grid if necessary
		 	 */ 
			if (_workArea->_draw_HZ) {
				drawn_hz = True;
	 			XDrawLine (XtDisplay(_w),XtWindow(_w),_namegc, 
					_maxx + 1, y, _graph_width, y);
			}
			else {
				/* Clear the Horizontal Grid when asked to do so
		 	 	*/
				if (drawn_hz) { 
	 				XDrawLine (XtDisplay(_w),XtWindow(_w),
					_cleargc,_maxx + 1, y, _graph_width, y);
				}
			} 
		} /* DO not draw the horizontal grid beyond 100 */
  	}
	
	if (!_workArea->_draw_HZ && drawn_hz)
		drawn_hz = False;

	/* Vertical Grid needs to be drawn if necessary
	 */
	if (_workArea->_draw_VT) {
		drawn_vt = True;
  		for (x = _maxx + (int)_mark; x <= _graph_width; x +=(int)_mark)
	 		XDrawLine (XtDisplay(_w),XtWindow(_w),_namegc, 
					x, _maxy, x, 0);
	}
	else {
		/* Clear the vertical grid if asked to do so
	 	 */
		if (drawn_vt) {
			drawn_vt = False;
  			for (x = _maxx + (int)_mark; x <= _graph_width; 
				x += (int) _mark) 
	 			XDrawLine (XtDisplay(_w),XtWindow(_w), _cleargc,
						x, _maxy, x, 0);
		}
	}

	/* Draw the time string */
	DrawTimeString ();
}

/************************************************************
 *  Remove the timeout and set the procid  to NULL
 ************************************************************/
void Graph::RemoveTimeOut()
{
	XtRemoveTimeOut (_procid);
	_procid = NULL;
}

/************************************************************
 *  Reset the timeout to use the new timer 
 ************************************************************/
void Graph::ResetTimeOut()
{
	/************************************************************
	 * re-create the timeout - so that plot_graph keeps getting
	 * called  again and again.  Clear the time Interval area
	 * to draw the interval string and re-create the procid string
	 ************************************************************/
  	XClearArea(XtDisplay(_w), XtWindow(_w), _graph_width / 2, _maxy + 1,
			_graph_width, _graph_height, 0);
	DrawTimeString ();
	_procid = XtAppAddTimeOut (theApplication->appContext (),
			_workArea->GetTimer(), (XtTimerCallbackProc)
			plot_graph, this);
}

/************************************************************
 * Get the font type. If multibyte then use Xmb calls, if
 * two byte use X*16 calls else if single byte use plain X calls.
 ************************************************************/
void Graph::GetFontType (XmFontList fontlist)
{
	XCharStruct		overall;
	int			dir;
	XmFontListEntry		fontlistentry;
	XmFontType		ft;
	XmFontContext		context;
	XFontStruct		*font_struct;
	char			*textstring;
	int			len;

	/*******************************************************
	 * First get the font type and the fontptr off the 
	 * fontlist.  Get the fontptr out of the fontlist.  
	 * It may be a XFontSet or a XFontStruct *
	 *******************************************************/
	XmFontListInitFontContext (&context, fontlist);
	fontlistentry = XmFontListNextEntry (context);
	_fontptr = XmFontListEntryGetFont (fontlistentry, &ft);

	/* If it is a XFontSet get the fontstruct * out of it
	 * in order to get the fontid for setting up the graphic
	 * context.
	 */
	if (ft == XmFONT_IS_FONTSET) {
		_fonttype = MULTI_BYTE;
		XFontStruct 	**font_struct_list;
		char		**font_name_list;	
	
		if (XFontsOfFontSet ((XFontSet) _fontptr, &font_struct_list,
					&font_name_list))
			font_struct = font_struct_list[0];
		else
			font_struct = NULL;
	}
	/* If it is a XFontStruct * check to see if it is double
	 * or single byte
	 */
	else if (ft == XmFONT_IS_FONT) {
		font_struct = (XFontStruct *)_fontptr;
		if (font_struct->min_byte1 != 0 && font_struct->max_byte1 != 0)
			_fonttype = DOUBLE_BYTE;	
		else {
			_fonttype = SINGLE_BYTE;
		}
	}

	/* Free Font Context
	 */
	XmFontListFreeFontContext (context);

	/*******************************************************
	 * Second set the graphic context using the fontid of 
	 * the XFontStruct *, background/foreground of widget. 
	 *******************************************************/
	XtVaGetValues (_w, 
			XmNforeground, &_nameval.foreground,
			XmNbackground, &_nameval.background,
			0);	

	/* Set up the font on graphic context from the FontStruct *.
	 * If font_struct was not found use a mask without font.
	 * In this case the default font is picked up in the
	 * font struct * structure.
	 */
	if (font_struct) {
		_nameval.font = font_struct->fid;
		_gcmask = GCFont|GCForeground|GCBackground;  
	}
	else 
		_gcmask = GCForeground|GCBackground;  

	/* Setup the graphic context
	 */
  	_namegc = XCreateGC (XtDisplay(_w), RootWindowOfScreen(XtScreen(_w)), 
				_gcmask, &_nameval);  
	 
	/*********************************************************
	 * If font type is XFontSet use Xmb Calls else if it is 
	 * two-byte or single-byte, use X*16 or plain X calls.
	 * Set up _maxx and the height (ascent + descent). 
	 * _maxx is where the line is drawn from 0,0 on the graph
	 *******************************************************/
	textstring = I18n::GetStr (TXT_interval) ;
	len = strlen (I18n::GetStr (TXT_interval)) ;
	switch (_fonttype) {

		case SINGLE_BYTE:
				XTextExtents ((XFontStruct *)_fontptr,  
						textstring, len, &dir, 
						&_ascent, &_descent, &overall);
				_maxx = XTextWidth ((XFontStruct *)_fontptr, 
						HUNDRED, strlen (HUNDRED));
				break;

		case DOUBLE_BYTE:
				XTextExtents16 ((XFontStruct *)_fontptr,  
						(XChar2b *) textstring, len,
						&dir, &_ascent, &_descent, 
						&overall); 
				_maxx = XTextWidth16 ((XFontStruct *)_fontptr, 
							(XChar2b *) HUNDRED, 
							strlen (HUNDRED));
				break;

		/* In case of multi-byte there is only height
	 	 * no ascent or descent values. Therefore set
	 	 * ascent to height of the rectangle
		 */
		case MULTI_BYTE:
				XRectangle 	ink, logical;

				XmbTextExtents ((XFontSet)_fontptr, textstring,
						len, &ink, &logical);
				_ascent = logical.height;
				XmbTextExtents ((XFontSet)_fontptr,  
						HUNDRED, strlen (HUNDRED),
						&ink, &logical);
				_maxx = logical.width;
				break;
	}
}

/************************************************************
 *  Draw the values along the y axis based on the font type
 ************************************************************/
void Graph::DrawYValues (char *buf, int y)
{
	int			x, d, asc, desc;
	XCharStruct		all;

	 /* Get the width of each value and draw the co-ordinate strings 
	  * on the graph.
	  */
	switch (_fonttype) {
		case SINGLE_BYTE:
				XTextExtents ((XFontStruct *)_fontptr,  buf, 
						strlen (buf), &d, &asc, &desc, 
						&all); 

				x = _maxx - all.width;	/* width of number */
	 			XDrawImageString (XtDisplay(_w), XtWindow(_w),
						_namegc, x, y, buf,strlen(buf));
				break;
		case DOUBLE_BYTE:
				XTextExtents16 ((XFontStruct *)_fontptr,  
						(XChar2b *)buf, strlen (buf),  
						&d, &asc, &desc, &all); 

				x = _maxx - all.width;	/* width of number */
	 			XDrawImageString16 (XtDisplay(_w), XtWindow(_w),
						_namegc, x, y, (XChar2b *)buf, 
						strlen(buf));
				break;
		case MULTI_BYTE:
				XRectangle 	ink, logical;

				XmbTextExtents ((XFontSet) _fontptr,  buf, 
						strlen (buf),  &ink, &logical); 

				x = _maxx - logical.width; /* width of number */
	 			XmbDrawImageString (XtDisplay(_w), XtWindow(_w),
						(XFontSet) _fontptr, _namegc, 
						x, y, buf, strlen(buf));
				break;
	}
}
