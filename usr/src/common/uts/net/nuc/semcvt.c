/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/semcvt.c	1.13"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/semcvt.c,v 2.53.2.3 1994/12/29 20:16:26 hashem Exp $"

/*
 *  Netware Unix Client
 *
 *	  MODULE:   semcvt.c
 *	ABSTRACT:   DOS/UNIX semantic conversio library module
 *
 *    This module contains functions that are utilized by the dos name space
 *    entry functions to convert UNIX to DOS semantics on the way in, and DOS
 *    to UNIX on the way out.
 *
 *	Rules of conversion are defined in the specific functions
 *
 *	Functions declared in this module:
 *    NCPspcUNIXtoDOSTime         - Convert UNIX time field to NetWare date
 *                                  and time.
 *    NCPspcDOStoUNIXName         - Convert DOS filename to UNIX.
 *    ConvertDOSTimeDateToSeconds - Convert NetWare time and date to UNIX.
 *                                  time (seconds since 01 Jan 1970).
 *    EncodeDOSDate               - combine year, month and day into a NetWare
 *                                  date.
 *    EncodeDOSTime               - combine hour, minute and second into a
 *                                  NetWare time.
 *    ConvertSecondsToFields      - converts time in seconds since midnight
 *                                  01Jan70 to year, month, day, hour, minute,
 *                                  and seconds.
 *
 */ 

#include <net/tiuser.h>
#include <net/nuc/nuctool.h>
#include <net/nuc/nwctypes.h>
#include <net/nuc/slstruct.h>
#include <net/nw/nwportable.h>
#include <net/nuc/nwctypes.h>
#include <net/nuc/spilcommon.h>
#include <util/cmn_err.h>
#include <net/nuc/ncpconst.h>
#include <net/nuc/nucmachine.h>
#include <net/nuc/nucerror.h>
#include <net/nuc/nuc_prototypes.h>

#include <io/ddi.h>

ccode_t	EncodeDOSDate (	uint8, uint8, uint8, uint16 * );
ccode_t EncodeDOSTime (	uint8, uint8, uint8, uint16	* );

/*
 *	Manifest constants used only here
 *
 *	Number of seconds between the Base UNIX timestamp to
 *	NetWare/DOS timestamp base.
 */
#define SECOND1970TO1980 			315532800L
#define	DOS_INITIAL_YEAR			80		/* 1980 */

/*
 *	Macro version of Upcase
 */
#define upCase( lc ) ( ((lc>0x60) && (lc < 0x7b)) ? lc-0x20 : lc )
#define downCase( lc ) ( ((lc>0x40) && (lc < 0x5b)) ? lc+0x20 : lc )

/*
 *	Function version
 */
uint32 ConvertDOSTimeDateToSeconds();
ccode_t ConvertSecondsToFields();

/*
 *	variable showMunge used to enable the munging message in function
 *	NCPpcUNIXtoDOSFileName.  Set it to zero (FALSE) to turn the message
 *	off, set it to non-zero to enable the message.  
 */
#ifdef DEBUG_TRACE
int showMunge = 0;
#endif

/*
 *	dosSuffixTable is an array of pointers to NetWare-style suffixes (3-char,
 *	lowercase, valid DOS characters, etc).  For each entry, there exists a
 *	corresponding entry in the unixSuffixTable which is its UNIX mapping.
 *	These suffixes are used in mapping DOS file/directory suffixes and
 *	their UNIX equivalent.
 */
char *dosSuffixes[64] = { "FRA", "SF1", "SF2", "SF3", "SF4", "SF5" };
char **dosSuffixTable = dosSuffixes;
char *unixSuffixes[64] = { "frame", "sf1\0\0\0\0\0", "sf2\0\0\0\0\0", "sf3\0\0\0\0\0", "sf4\0\0\0\0\0", "sf5\0\0\0\0\0" };
char **unixSuffixTable = unixSuffixes;


/*
 * BEGIN_MANUAL_ENTRY(NCPspcUNIXtoDOSTime.3k)
 * NAME
 *    NCPspcUNIXtoDOSTime - Convert UNIX time field to DOS format for
 *                          transmission over the wire.
 *
 * SYNOPSIS
 *    ccode_t
 *    NCPspcUNIXtoDOSTime ( uint32   unixTime,
 *                          int32    timeZoneOffset,
 *                          uint32   *dosDate,
 *                          uint32   *dosTime )
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPspcUNIXtoDOSTime (	uint32	unixTime,
						int32	timeZoneOffset,
						uint32	*dosDate,
						uint32	*dosTime )
{
	uint8	year, month, day, hour, minutes, seconds;

	ConvertSecondsToFields (unixTime, timeZoneOffset, &year, &month, &day,
		&hour, &minutes, &seconds);

	EncodeDOSDate (year, month, day, (uint16 *)dosDate);
	EncodeDOSTime (hour, minutes, seconds, (uint16 *)dosTime);

	return (SUCCESS);
}

/*
 * BEGIN_MANUAL_ENTRY(NCPspcDOStoUNIXName.3k)
 * NAME
 *    NCPspcDOStoUNIXName - Convert DOS names to UNIX names
 *
 * SYNOPSIS
 *    void_t
 *    NCPspcDOStoUNIXName (	char  *dosName,
 *                          char  *unixName )
 *
 * END_MANUAL_ENTRY
 */
void_t
NCPspcDOStoUNIXName (	char	*dosName,
						char	*unixName )
{

	/*
	 *	No need to check valid characters, as UNIX will allow (almost) anything.
	 */
	while (*dosName) {
		switch ( *dosName ) {
		
			case '\\':
				*unixName = '/';
				break;
			
			default:
				*unixName = downCase( *dosName );
				break;
		}

		unixName++;
		dosName++;
	}
	*unixName = '\0';
}

/*
 * BEGIN_MANUAL_ENTRY(ConvertDOSTimeDateToSeconds.p3k)
 * NAME
 *    ConvertDOSTimeDateToSeconds - Convert DOS/NetWare time date stamp to
 *                                  host format.
 *
 * SYNOPSIS
 *    uint32 
 *    ConvertDOSTimeDateToSeconds ( uint32  nwtime,
 *                                  uint32  nwdate,
 *                                  int32   timezoneAdjust )
 *
 * RETURN VALUES
 *		Unsigned 32-bit integer set to the number of seconds since 1970.
 *
 * DESCRIPTION
 *		Converts DOS/NetWare time/date stamp format to seconds since 1970.
 *
 * END_MANUAL_ENTRY
 */
uint32 
ConvertDOSTimeDateToSeconds (	uint32	nwtime,
								uint32	nwdate,
								int32	timezoneAdjust )
{
	uint32 hosttime;
	uint16 tmpInt;
	uint32 year, month;
	static uint16 
		dayMonth[12]={0,31,59,90,120,151,181,212,243,273,304,334};
	
	hosttime = SECOND1970TO1980;
	hosttime += ((nwtime & 0x1F) << 1);		/* convert from 2 seconds */
	hosttime += ((uint16)(nwtime & 0x7E0) >> 5) * 60;	/* minutes */
	hosttime +=  (nwtime >> 11) * 3600;			/* hours */
	if((tmpInt = (nwdate & 0x1F)) > 0)			/* days */
		hosttime += (tmpInt - 1) * 3600 * 24;
	if((month = (uint32)((uint16)(nwdate & 0x1E0) >> 5)) > 0)	/* month */
		hosttime += dayMonth[ month - 1 ] * 24 * 3600;
	if(((year = nwdate >> 9) % 4) == 0 && month >= 2)/* leap year ? */
		hosttime += 24 * 3600;					/* yes -- add 1 day */
	for(tmpInt = 0; tmpInt < year; tmpInt++) {
		if(tmpInt % 4)
			hosttime += 365 * 24 * 3600;
		else
			hosttime += 366 * 24 * 3600;
	}

	/*
	 *	Need to get the adjustment in hours between GMT and 
	 *	local time (including Daylight savings time (if active)
	 *	and multiply it to get seconds
	 */
	hosttime += ((timezoneAdjust*60)*60);	

	return (hosttime);
}

/*
 *   EncodeDOSDate - combine year, month and day into a NetWare date.
 */
static ccode_t
EncodeDOSDate (	uint8	year,
				uint8	month,
				uint8	day,
				uint16	*nwDate )
{
	/*
	 *	Perform a boundary check to make sure something funny
	 *	doesn't happen here.
	 */
	if (year < DOS_INITIAL_YEAR)
		year = DOS_INITIAL_YEAR;

	*nwDate = ( ((year - DOS_INITIAL_YEAR) & 0x7F) << 9 ) |
			((month & 0x0F) << 5 ) |
			(day & 0x1F);

	return (SUCCESS);
}

/*
 *    EncodeDOSTime - combine hour, minute and second into a NetWare time.
 */
static ccode_t
EncodeDOSTime (	uint8	hour,
				uint8	minute,
				uint8	second,
				uint16	*nwTime )
{
	*nwTime = ((hour & 0x1F) << 11 ) |
			((minute & 0x3F) << 5 ) |
			((uint8)(second & 0x3F) >> 1);/* convert to 2 seconds */

	return (SUCCESS);
}

/*
 *	Quick and dirty macro for deriving the number of days
 *	in the year (leap or otherwise)
 */
#define	DAYS_IN_YEAR(YEAR)	(((YEAR)%4) ? 365 : 366 )
#define	IS_LEAP_YEAR(Y)		(DAYS_IN_YEAR(Y)==366 ? -1 : 0)

/*
 *	Devisor for separating time from date 
 */
#define UNIX_TIME_CONVERT_FACTOR 	86400L
#define SYSV_INITIAL_YEAR			70		/* 1970 */


/*
 *	Globals
 */
static uint8 daysInMonth[12] = {31,28,31,30,31,30,31,31,30,31,30,31 };

/*
 * BEGIN_MANUAL_ENTRY(ConvertSecondsToFields.p3k)
 * NAME
 *    ConvertSecondsToFields - Convert GMT seconds since the epoch to 
 *                             year, month, day, hour, minute and seconds.
 *
 * SYNOPSIS
 *
 * RETURN VALUES
 *		Success
 *
 * DESCRIPTION
 *		Converts seconds since Midnight, 01Jan70 to year, month, day, hour,
 *		minute, and seconds.  Year is actual year, month, day, hour, minute
 *		and second fields are one-based.
 *
 * END_MANUAL_ENTRY
 */
ccode_t
ConvertSecondsToFields (	int32	GMTTime,
							int32	timeZoneOffset,
							uint8	*year,
							uint8	*month,
							uint8	*day,
							uint8	*hour,
							uint8	*minutes,
							uint8	*seconds )
{
	int32			hourMinuteSec, numberOfDays;
	register int32	i,j;

	/*
	 *
	 *	Adjust from GMT to localTime
	 */
	GMTTime -= (timeZoneOffset * 3600);
		

	/*
	 *	Convert UNIX time to year/month/day hour/minute/second format.
	 */
	hourMinuteSec = GMTTime % UNIX_TIME_CONVERT_FACTOR;
	numberOfDays = GMTTime / UNIX_TIME_CONVERT_FACTOR;

	if (hourMinuteSec < 0) {
		hourMinuteSec += UNIX_TIME_CONVERT_FACTOR;
		--numberOfDays;
	}

	*seconds = (uint8)(hourMinuteSec % 60);
	j = hourMinuteSec/60;
	*minutes = (uint8)(j % 60);
	*hour = (uint8)(j / 60);


	/*
	 *	Calculate the number of years and the number of days remaining...
	 */
	if (numberOfDays >= 0) {
		for (i = SYSV_INITIAL_YEAR; numberOfDays >=DAYS_IN_YEAR(i); i++) {
			numberOfDays -= DAYS_IN_YEAR(i);
		}
	} else {
		for (i = SYSV_INITIAL_YEAR; numberOfDays < 0 ; i--)
			numberOfDays += DAYS_IN_YEAR(i-1);
	}

	*year = (uint8)i;
	j = numberOfDays;	/* Number of days into this year */

	/*
	 *	Temporarily change February because this year happens
	 *	to be a leap one...
	 */
	if (IS_LEAP_YEAR(i))
		daysInMonth[1] = 29;

	for ( i = 0; j >= (int32)daysInMonth[i]; i++ )
		j -= daysInMonth[i];

	/*
	 *	reset the arrary to initial value
	 */
	daysInMonth[1] = 28;

	/*
	 *	Set the month and day for caller
	 */
	*day = (uint8)j+1;		/* Make the day 1 based */
	*month = (uint8)i+1;	/* Make the month 1 based */

	/*
	 *	Need to place code to adjust for daylight savings time
	 *	here...
	 *		Currently, this is just stubbed as part of the timezone
	 *
	 */

	return (SUCCESS);
}
