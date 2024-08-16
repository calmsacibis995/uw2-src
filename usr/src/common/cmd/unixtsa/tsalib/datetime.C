#ident	"@(#)unixtsa:common/cmd/unixtsa/tsalib/datetime.C	1.3"

#include <smsutapi.h>

#include <time.h>

#if defined(__TURBOC__) || defined(MSC)
	void _ConvertTimeToDOS( time_t calendarTime, UINT16 *filDatP, UINT16 *filTimP );
	time_t _ConvertDOSTimeToCalendar( UINT32 dateTime );
#endif

INT16  GenECMATimeZone(void) ;

#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "NWSMCheckDateAndTimeRange"
#endif
CCODE NWSMCheckDateAndTimeRange(
		UINT32 firstDateAndTime,
		UINT32 lastDateAndTime,
		UINT32 compareDateAndTime)
{
	CCODE ccode = TRUE;

	if (lastDateAndTime and compareDateAndTime > lastDateAndTime)
		goto Return;

	if (firstDateAndTime and compareDateAndTime and 
			(compareDateAndTime < firstDateAndTime))
		goto Return;

	ccode = FALSE;

Return:
	return (ccode);
}


#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "NWSMPackDate"
#endif
UINT16 NWSMPackDate(
		UINT16 year,
		UINT16 month,
		UINT16 day)
{
	//return ( ((year - 1980) << 9) |	(month << 5) | day);
	return ( (year << 9) |	(month << 5) | day);
}


#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "NWSMPackDateTime"
#endif
UINT32 NWSMPackDateTime(
		UINT16 year,
		UINT16 month,
		UINT16 day,
		UINT16 hours,
		UINT16 minutes,
		UINT16 seconds)
{
	UINT32 dateTime;
	dateTime = 0 ;

	dateTime = NWSMPackDate(year, month, day) << 16;
	dateTime  |= NWSMPackTime(hours, minutes, seconds);
	return (dateTime);
}


#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "NWSMPackTime"
#endif
UINT16 NWSMPackTime(
		UINT16 hours,
		UINT16 minutes,
		UINT16 seconds)
{
	return ( (hours << 11) | (minutes << 5) | (seconds >> 1) );
}

void NWSMUnPackDate(
		UINT16 date,
		UINT16 *year,
		UINT16 *month,
		UINT16 *day)
{
	if (year)
		*year = ((UINT16)(date & (UINT16)0xfe00) >> 9) + 1980;

	if (month)
		*month = (UINT16)(date & (UINT16)0x01e0) >> 5;

	if (day)
		*day = date & 0x001f;
}


void NWSMUnPackDateTime(
		UINT32 dateTime,
		UINT16 *year,
		UINT16 *month,
		UINT16 *day,
		UINT16 *hours,
		UINT16 *minutes,
		UINT16 *seconds)
{
	NWSMUnPackDate((UINT16)(dateTime >> 16) , year, month, day);
	NWSMUnPackTime((UINT16)(dateTime & 0xffff), hours, minutes, seconds);
}


#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "NWSMUnPackTime"
#endif
void NWSMUnPackTime(
		UINT16 time,
		UINT16 *hours,
		UINT16 *minutes,
		UINT16 *seconds)
{
	if (hours)
		*hours = (UINT16)(time & (UINT16)0xf800) >> 11;

	if (minutes)
		*minutes = (UINT16)(time & (UINT16)0x07e0) >> 5;

	if (seconds)
		*seconds = (time & 0x001f) << 1;
}


#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "NWSMGetCurrentDateAndTime"
#endif
UINT32 NWSMGetCurrentDateAndTime(void)
{
	struct tm *t;
	time_t timeOfDay;

	timeOfDay = time(NULL);
	t = localtime(&timeOfDay);

	if ( t->tm_year < 80 )
		t->tm_year = 80 ;

	return (NWSMPackDateTime((UINT16)t->tm_year - 80, (UINT16)t->tm_mon + 1,
			(UINT16)t->tm_mday, (UINT16)t->tm_hour, (UINT16)t->tm_min,
			(UINT16)t->tm_sec));
}



#if defined(__TURBOC__) || defined(MSC)

void _ConvertTimeToDOS( time_t calendarTime, UINT16 *filDatP, UINT16 *filTimP )
{
	struct tm *timeStruct;

	timeStruct = gmtime( &calendarTime );	// convert time_t to struct tm
	if ( !timeStruct )
		return;

	if (timeStruct->tm_mon  == 10 && // tm_mon is zero based(i.e. 10 is Nov)
		timeStruct->tm_mday == 30 && timeStruct->tm_year == 79) {
		*filDatP = NWSMPackDate( 0, 0, 0 );
	}
	else
	{
		*filDatP = NWSMPackDate(
			timeStruct->tm_year - 80,	// years since 1980
			timeStruct->tm_mon + 1,		// 1 indexed month
			timeStruct->tm_mday );
	}

	*filTimP = NWSMPackTime(
			timeStruct->tm_hour,
			timeStruct->tm_min,
			timeStruct->tm_sec >> 1 );	// bi-second
}


time_t _ConvertDOSTimeToCalendar( UINT32 dateTime )
{
	auto struct tm t;

	NWSMUnPackDateTime( dateTime, &(UINT16)(t.tm_year),
			&(UINT16)(t.tm_mon),
			&(UINT16)(t.tm_mday),
			&(UINT16)(t.tm_hour),
			&(UINT16)(t.tm_min),
			&(UINT16)(t.tm_sec) );

	t.tm_year -= 1900;		// adjust to years since 1900
	t.tm_isdst = -1;
	return mktime( &t );	// convert struct tm time to time_t
}

#endif

#if defined(UNIX)
void _ConvertTimeToDOS( time_t calendarTime, UINT16 *filDatP, UINT16 *filTimP )
{
	struct tm *timeStruct;

	timeStruct = gmtime( &calendarTime );	// convert time_t to struct tm
	if ( !timeStruct )
		return;

	if ( timeStruct->tm_year < 80 ) { // year before 1980
		timeStruct->tm_year = 80 ;
		timeStruct->tm_mon = 0 ;
		timeStruct->tm_mday = 1 ;
	}
	*filDatP = NWSMPackDate(
		timeStruct->tm_year - 80,	// years since 1980
		timeStruct->tm_mon + 1,		// 1 indexed month
		timeStruct->tm_mday );

	*filTimP = NWSMPackTime(
			timeStruct->tm_hour,
			timeStruct->tm_min,
			timeStruct->tm_sec );	// bi-second
}


time_t _ConvertDOSTimeToCalendar( UINT32 dateTime )
{
	auto struct tm t;
	time_t origin  = 0 ;
        UINT16 y, mon, d, h, m, s ;

	if ( dateTime == 0 ) {
		return(0);
	}
        y = mon = d = h = m = s = 0 ;

	memset((char *)&t,0,sizeof(t));

        NWSMUnPackDateTime( dateTime, &y, &mon, &d, &h, &m, &s);

        t.tm_year = y ; t.tm_mon = mon - 1 ; t.tm_mday = d ;
        t.tm_hour = h ; t.tm_min = m ; t.tm_sec = s ;

	t.tm_year -= 1900;		// adjust to years since 1900
	t.tm_isdst = -1  ;
#ifdef SYSV
	origin =  mktime( &t );	// convert struct tm time to time_t
	origin -= timezone ;
	
	return(origin);
#else
	return timegm( &t );	// convert struct tm time to time_t
#endif
}

#endif

CCODE NWSMUnixTimeToECMA(
               UINT32        unixTime,
               ECMATime     *ECMATime,
               NWBOOLEAN32   local)
{
	INT16      type, zone;
	time_t     uTime;
	struct tm *tmTime;

	/*The 'local' flag tells us whether parameter 1 is in local time or
	 *UTC. so that we know how to set the timeZone in the ECMA structure.  This
	 *	function does not convert from local to UTC or vice-versa. ***/

	uTime  = (time_t) unixTime;
	if ( local ) {
		tmTime = localtime( &uTime );
	}
	else {
		tmTime = gmtime( &uTime );
	}

	ECMATime->year    = tmTime->tm_year + 1900; /* might need to swap on HiLos */
	ECMATime->month   = tmTime->tm_mon  + 1;	/* tm is 0-based, ECMA 1-based */
	ECMATime->day     = tmTime->tm_mday;
	ECMATime->hour    = tmTime->tm_hour;
	ECMATime->minute  = tmTime->tm_min;
	ECMATime->second  = tmTime->tm_sec;

	ECMATime->centiSecond   = 0;
	ECMATime->hundredsOfMicroseconds  = 0;
	ECMATime->microSeconds  = 0;

	if (local)
	{
		type = 1;		/* ie local */
		tzset();		/* set the globals that are declared in time.h */

		zone = GenECMATimeZone();

		/*  If  zone  is a negative number, cannot simply shift  type  into the
			most significant 4 bits and add it.  Need to preserve the lower 12
			bits as they are.  In doing so, we also preserve the 'negative bit'
			because all bits above the numeric bits are set on when a number is
			negated, and we know that  timezone  in minutes is < 12 bits.   ***/

		/* might need to swap on HiLos */
		ECMATime->typeAndTimeZone = (zone & 0xfff) | (type << 12);
	}
	else
		ECMATime->typeAndTimeZone = 0;

	return(0);

}	/* end NWSMUnixTimeToECMA */

/*****************************************************************************/

/* This function converts the main variables in the ECMA time structure to
 *	the number of seconds since Jan 1 70  (if the ECMA time is later than 1970;
 *	if not, it sets unixTime to zero and returns non-0).  It then looks at the
 *	time zone and if not zero (which means UTC), it sets the parameter tzOffset
 *	to be the number of minutes from UTC, which is the ECMA timezone unit.	***/

CCODE NWSMECMAToUnixTime(
               ECMATime     *ECMATime,
               UINT32       *unixTime,
               INT32        *tzOffset )
{
	UINT16     type;
	struct tm  tmTime;

	if (ECMATime->year  <  1970)
	{
		*unixTime = 0;
		*tzOffset = ECMA_TIME_ZONE_UNKNOWN;
		return (1);
	}

	/*  Convert ECMA core variables to seconds-since-1970.	***/

	tmTime.tm_year = ECMATime->year  - 1900;
	tmTime.tm_mon  = ECMATime->month - 1;	/* tm is 0-based, ECMA 1-based */
	tmTime.tm_mday = ECMATime->day;
	tmTime.tm_hour = ECMATime->hour;
	tmTime.tm_min  = ECMATime->minute;
	tmTime.tm_sec  = ECMATime->second;
	tmTime.tm_isdst  = daylight ;

	*unixTime = mktime( &tmTime );

	if (ECMATime->typeAndTimeZone == 0) {
  		*tzOffset = 0;
		if ( daylight ) {
			*unixTime -= altzone ;
		}
		else {
			*unixTime -= timezone ;
		}
	}
	else
	{
		type = ECMATime->typeAndTimeZone >> 12;
		if (type != 1)
			*tzOffset = ECMA_TIME_ZONE_UNKNOWN;
		else
		{
			/*  Pull out the least significant 12 bits for the time zone;
			 *	then, if the value is negative (ie high bit is set), must
			 *	set all the remaining bits in the 32-bit value so that it
			 *	is negative also.											***/

			*tzOffset = ECMATime->typeAndTimeZone  &  0x0fff;
			if (*tzOffset != ECMA_TIME_ZONE_UNKNOWN  &&
				(*tzOffset &  0x800) )
					*tzOffset |= 0xfffff000;
		}
	}

	return(0);

}	/* end NWSMECMAToUnixTime */

/*****************************************************************************/

CCODE NWSMDOSTimeToECMA(
               UINT32       dosTime,
               ECMATime     *eTime)
{
	INT16  type, zone ;

   NWSMUnPackDateTime( dosTime, (UINT16 *) &eTime->year,
								 (UINT16 *) &eTime->month,
								 (UINT16 *) &eTime->day,
								 (UINT16 *) &eTime->hour,
								 (UINT16 *) &eTime->minute,
								 (UINT16 *) &eTime->second );

	eTime->centiSecond   = 0;
	eTime->hundredsOfMicroseconds  = 0;
	eTime->microSeconds  = 0;

	type = 1;		/* ie local */
	tzset();		/* set the globals that are declared in time.h */

	/*  See comments in  UnixTimeToECMA  about the next 2 lines. 			***/

	zone = GenECMATimeZone();
	eTime->typeAndTimeZone = (zone & 0xfff) | (type << 12);

	return(0);

}	/* end NWSMDOSTimeToECMA */

/*****************************************************************************/

/* Since DOS has no time zone info, we ignore that part of ECMA.
 *	If the year is < 1980, set dosTime to 0 and return non-zero.			***/

CCODE NWSMECMAToDOSTime(
               ECMATime     *ECMATime,
               UINT32       *dosTime  )
{
	if (ECMATime->year  <  1980)
	{
		*dosTime = 0;
		return (1);
	}

	*dosTime = NWSMPackDateTime(
		(UINT16)ECMATime->year,
		(UINT16)ECMATime->month,  
		(UINT16)ECMATime->day,    
		(UINT16)ECMATime->hour,   
		(UINT16)ECMATime->minute, 
		(UINT16)ECMATime->second  ); 

   return(0);

}	/* end NWSMECMAToDOSTime */

/*****************************************************************************/

/*  ECMATimeCompare will return:
 *                              0 = If the times are the same
 *                             -1 = If ECMATime1 is less than ECMATime2
 *                             +1 = If ECMATime1 is greater than ECMATime2
 *		    ECMA_TIME_ZONE_UNKNOWN = If this is true of either one			***/

int   NWSMECMATimeCompare(
               ECMATime     *ECMATime1,
               ECMATime     *ECMATime2)
{
   INT32   tzOffset1, tzOffset2;
   UINT32  utime1,    utime2;
	int     ccode;

	NWSMECMAToUnixTime( ECMATime1, &utime1,	&tzOffset1 );
	NWSMECMAToUnixTime( ECMATime2, &utime2,	&tzOffset2 );

	if (tzOffset1 == ECMA_TIME_ZONE_UNKNOWN   ||
		tzOffset2 == ECMA_TIME_ZONE_UNKNOWN)
	{
		ccode = ECMA_TIME_ZONE_UNKNOWN;
	}
	else
	{
		utime1 += tzOffset1 * 60;
		utime2 += tzOffset2 * 60;

		if (utime1 < utime2)
			ccode = -1;
		else if (utime1 > utime2)
			ccode = 1;
		else
			ccode = 0;
	}

	return (ccode);

}	/* end NWSMECMATimeCompare */

/*****************************************************************************/


/* The "daylight" vrbls are only available in 4.x, give more precise values. */

INT16  GenECMATimeZone(void)
{
#if defined(NLM)
   UINT32  nlmHandle;

   static long  *offsetPtr;
   static int   *onOffPtr;
static  NWBOOLEAN32 calledImport = FALSE;

	long  getzOffset;
	int   getzOnOff;
	INT16 zone;


   /* We can't get real time zone info out of a workstation, so don't try. */

	getzOffset = 3600;              /* standard daylight savings amount */

	getzOnOff  = daylight;          /* daylight variable in both 3.x and 4.x */
	if (!calledImport)
	{
		nlmHandle = GetNLMHandle();
		offsetPtr = ImportSymbol( nlmHandle, "daylightOffset" );
		onOffPtr  = ImportSymbol( nlmHandle, "daylightOnOff" );
		calledImport = TRUE;
	}

	if (offsetPtr)
		getzOffset = *offsetPtr;
	if (onOffPtr )
		getzOnOff  = *onOffPtr;

	/*  ECMA timezones are in minutes from UTC.  Novell timezone variables
	 *	are in secs _to_ UTC.  Ergo, must negate the value derived from
	 *	Novell.	 If DST is in effect, it decrements the difference between
	 *	local time and UTC.												***/

	zone = -( (timezone / 60) - (getzOnOff * (getzOffset / 60)) );

	return (zone);

#elif defined(UNIX)
	if ( daylight ) {
		return(altzone / 60) ;
	}
	else {
		return(timezone / 60) ;
	}
#else
	return(ECMA_TIME_ZONE_UNKNOWN);

#endif

}	/* end  GenECMATimeZone */

/*****************************************************************************/
/*****************************************************************************/
