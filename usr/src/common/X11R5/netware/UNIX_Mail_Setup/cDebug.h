/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)umailsetup:cDebug.h	1.3"
#ifndef CDEBUG_H
#define CDEBUG_H

#include	<iostream.h>		//  for cout
#include	<stdio.h>
#include	"setup.h"		//  for AppStruct definition

#ifdef CDEBUG_DEFINITION		// define storage
int	cDebugInit (int debugLevel, char *debugFileName, int argc, int maxLevel);
FILE	*log = stdout;	//  stdout used only if cDebugLevel is on && cLogFile is NULL.
#else					// simply declare 
extern FILE	*log;
extern int	cDebugInit (int debugLevel, char *debugFileName, int argc, int maxLevel);
#endif // CDEBUG_DEFINITION



extern AppStruct	app;		//  the struct containing app variables

//  The following logging macro means the cLog call in the code should look like:
//
//  cLog (level, (fprintf (log, "Blah=%d, %s, etc.\n", whateverInt, whatevercharstar)));
//	where "level" can be any of the debug levels defined below (1-4), and the
//	fprintf formatting string and args can be anything valid for fprintf. 
//	Note: "log" should always be "log", since cDebug.C sets it up.
//
//  log (level, "param1=", param1, ", param2=", param2);
//	where "level" can be any of the defined levels (1-5), and the following 
//	arguments are like you would use with cout.
//

#ifdef	SETUPAPP_DEBUG

#define	cLog(level, x)	((app.rData.cDebugLevel && (level == app.rData.cDebugLevel || app.rData.cDebugLevel == C_ALL))? x:0)
//#define	log(level, u,v,w,x,y,z)	((app.rData.cDebugLevel && (level == app.rData.cDebugLevel || app.rData.cDebugLevel == C_ALL))? cout<<u<<v<<w<<x<<y<<z<<"."<<endl:0)
//#define	log(level, u,v,w,x,y,z)	((app.rData.cDebugLevel && (level == app.rData.cDebugLevel || app.rData.cDebugLevel == C_ALL))? (v? (w? (x? (y? (z? (cout<<u<<v<<w<<x<<y<<z<<"."<<endl) : (cout<<u<<v<<w<<x<<y<<"."<<endl)) : (cout<<u<<v<<w<<x<<"."<<endl)) : (cout<<u<<v<<w<<"."<<endl)) : (cout<<u<<v<<"."<<endl)) : (cout<<u<<"."<<endl)):0)
#define	log1(level, u)	((app.rData.cDebugLevel && (level == app.rData.cDebugLevel || app.rData.cDebugLevel == C_ALL))? cout<<u<<"."<<endl:0)
#define	log2(level, u,v)	((app.rData.cDebugLevel && (level == app.rData.cDebugLevel || app.rData.cDebugLevel == C_ALL))? cout<<u<<v<<"."<<endl:0)
#define	log3(level, u,v,w)	((app.rData.cDebugLevel && (level == app.rData.cDebugLevel || app.rData.cDebugLevel == C_ALL))? cout<<u<<v<<w<<"."<<endl:0)
#define	log4(level, u,v,w,x)	((app.rData.cDebugLevel && (level == app.rData.cDebugLevel || app.rData.cDebugLevel == C_ALL))? cout<<u<<v<<w<<x<<"."<<endl:0)
#define	log5(level, u,v,w,x,y)	((app.rData.cDebugLevel && (level == app.rData.cDebugLevel || app.rData.cDebugLevel == C_ALL))? cout<<u<<v<<w<<x<<y<<"."<<endl:0)
#define	log6(level, u,v,w,x,y,z)	((app.rData.cDebugLevel && (level == app.rData.cDebugLevel || app.rData.cDebugLevel == C_ALL))? cout<<u<<v<<w<<x<<y<<z<<"."<<endl:0)

#else	//  SETUPAPP_DEBUG not defined

#define	cLog(level, x)
//#define	log(level, u,v,w,x,y,z)
#define	log1(level, u)
#define	log2(level, u,v)
#define	log3(level, u,v,w)
#define	log4(level, u,v,w,x)
#define	log5(level, u,v,w,x,y)
#define	log6(level, u,v,w,x,y,z)

#endif	SETUPAPP_DEBUG

//  DEBUG levels
#define	C_OFF		0		//  for no debugging (it is off by default)
#define	C_ERR		1		//  for capturing error messages only
#define	C_API		2		//  to trace client-side API usage only
#define	C_FUNC		3		//  to trace client function calls only
#define	C_PWD		4		//  to trace the password stuff
#define	C_ALL		5		//  to capture all debugging statements

#endif	//	CDEBUG_H
