#ident	"@(#)unixtsa:common/cmd/unixtsa/tsaunix/tsamain.C	1.8"

/*
$Abstract: 
This is the entry point for the SMS responder. The responder is spawned off
from the tsa daemon which handles the tsa registration and connection
establishment. Before spawning the responder, tsa daemon redirects the standard
input and standard output to the connected endpoint.
$

$EndLog$ 
*/

#ifdef SYSV
#include	<sys/systeminfo.h>
#include	<netdb.h>
#include	<locale.h>
#include        <unistd.h>
#include        <tsaunix_msgs.h>
#include        <nl_types.h>
#endif

#include <tsad.h>
#include <smdr.h>
#include <smsdrerr.h>
#include "tsaglobl.h"


/* Extern variables */
extern int t_errno;
extern char *t_errlist[];
extern int t_nerr;
extern int CloseTransportEndpoint(int registerFD);
extern int SessionPipeParm ;
extern void ServiceRequest(void);

/* Function Prototypes */
void setMessageLocale(void) ;
#ifdef DEBUG
void show_buf(char *buf, int len) ;
void fshow_buf(FILE *outp, char *buf, int len) ;
#endif

/* Global variables */
CONNECTION   ConnectionHandle ;
TARGET_INFO  TargetInfo;
RESOURCE_LIST *resources ;
FILE *logfp ;
static char *logFile = LOG_FILE ;
static char     *optString = "l:L:t:e:" ;
nl_catd                 mCat ;  /* i18n message catalog */
char *tsaCodeset = NULL ;
char *engineCodeset = NULL ;
char *engineLocale = "" ;
char scratchBuffer[4096] ;
char tmpPath[MAXPATHLEN + 1] ;
int convertCodeset = 0 ;
NWSM_MODULE_VERSION_INFO unixtsaModule ;

static void SetupTargetInfo()
{
	struct hostent	*host_data = NULL ;
	char		short_name[MAXHOSTNAMELEN], *cptr ;

	memset((char *)&TargetInfo,'\0',sizeof(TargetInfo));

#ifdef SYSV
	(void) sysinfo(SI_HOSTNAME, short_name, MAXHOSTNAMELEN) ;
	host_data = gethostbyname(short_name) ;
	(void) strncpy(TargetInfo.targetName, 
					host_data->h_name, TSNAMELEN-1) ;
	if ( (cptr = strchr(TargetInfo.targetName, '.')) != NULL ) {
		*cptr = '\0' ;
	}
#else
	gethostname(TargetInfo.targetName,HOSTNAMELEN);
#endif

	strcpy(TargetInfo.targetType,TSAGetMessage(TSA_TARGET_SERVICE_TYPE));
	strcpy(TargetInfo.targetVersion, 
			ConvertTsaToEngine("1.8", NULL));
}

static void SetupModuleInfo(char *moduleFileName)
{
	char os[SI_SYSNAME_LEN+1], rel[SI_RELEASE_LEN+1] ;
	char *cptr ;

	if ( strlen(moduleFileName) > MODULE_FILE_NAME_LEN - 1 ) {
		if ( (cptr = strrchr(moduleFileName, '/')) != NULL ) {
			strncpy(unixtsaModule.moduleFileName,
							cptr+1, MODULE_FILE_NAME_LEN-1);
		}
		else {
			strncpy(unixtsaModule.moduleFileName,
							moduleFileName, MODULE_FILE_NAME_LEN-1);
		}
	}
	else {
		strcpy(unixtsaModule.moduleFileName, moduleFileName) ;
	}
	cptr = ConvertTsaToEngine(unixtsaModule.moduleFileName,NULL);
	if ( cptr != unixtsaModule.moduleFileName ) {
		strncpy(unixtsaModule.moduleFileName, cptr,
			MODULE_FILE_NAME_LEN-1);
	}
	(void) sysinfo(SI_SYSNAME, os, SI_SYSNAME_LEN) ;
	(void) sysinfo(SI_RELEASE, rel, SI_RELEASE_LEN) ;
	sprintf(unixtsaModule.baseOS,"%s %s",os,rel);
	cptr = ConvertTsaToEngine(unixtsaModule.baseOS,NULL);
	if ( cptr != unixtsaModule.baseOS ) {
		strncpy(unixtsaModule.baseOS, cptr, MODULE_OS_LEN-1);
	}
	unixtsaModule.moduleMajorVersion = 1 ;
	unixtsaModule.moduleMinorVersion = 0 ;
	unixtsaModule.moduleRevisionLevel = SwapUINT16(1);
	unixtsaModule.baseOSMajorVersion = 2 ;
	unixtsaModule.baseOSMinorVersion = 0 ;
	unixtsaModule.baseOSRevisionLevel = SwapUINT16(1);
}

void main(int argc, char *argv[])
{
	int optChar, err = 0 ;

	while ( (optChar = getopt(argc, argv, optString)) != EOF ) {
		switch ( optChar ) {
		case 'l' :
			logFile = optarg ;
			break ;
		case 't' :
			tsaCodeset = optarg ;
			break;
		case 'e' :
			engineCodeset = optarg ;
			break;
		case 'L' :
			engineLocale = optarg ;
			break;
		case '?' :
			err++ ;
			break ;
		}
	}

	setlocale(LC_ALL, engineLocale) ;         // set up for i18n
	mCat = catopen(MCAT, 0) ;
	setMessageLocale() ;		// i18n tsa message list

	logfp = fopen(logFile,"a");
	if ( logfp == NULL ) {
		if ( (logfp = fopen(CONSLOG,"w")) == NULL )
			logfp = stderr ; /* This should never happen. */
	}

	if ( err ) {
		logerror(PRIORITYERROR,TSAGetMessage(TSA_USAGE),argv[0]);
		fclose(logfp);
		exit(0);
	}
#ifdef DEBUG
	/* This logging mechanism may not work properly in non BSD system */
	/*
	openlog("unixtsa",LOG_PID,LOG_DAEMON);
	setlogmask(LOG_UPTO(LOG_DEBUG));
	*/
	fprintf(logfp, "Initializing TSAUNIX . . .\n") ;
#endif

	SessionPipeParm = dup(0) ;
	if ( SessionPipeParm == -1 ) {
		logerror(PRIORITYERROR,TSAGetMessage(DUP_FAILED),errno);
		fclose(logfp);
		exit(0);
	}

	close(0);
	close(1);
	close(2);

	while (1) {
		if ( t_sync(SessionPipeParm) == -1 ) {
			logerror(PRIORITYERROR, TSAGetMessage(T_SYNC_FAILED), 
									t_errno);
			if ( t_errno == TSTATECHNG ) {
				sleep(1);
				continue ;
			}
			close(SessionPipeParm) ;
			fclose(logfp);
			exit(1);
		}
		break ;
	}
	if ( t_nonblocking(SessionPipeParm) == -1 ) {
		logerror(PRIORITYERROR,
				TSAGetMessage(T_NONBLOCKING_FAILED),errno);
		close(SessionPipeParm) ;
		fclose(logfp);
		exit(1);
	}

	memset((char *)&ConnectionHandle,'\0',sizeof(ConnectionHandle));
	if ( ConvertInitialize(tsaCodeset,engineCodeset) == 0 ) {
		convertCodeset = 1 ;
	}
	SetupTargetInfo();
	SetupModuleInfo(argv[0]);
	tzset(); /* Initialize the timezone variables */
#ifdef DEBUG
	logerror(PRIORITYWARN,"Calling ServiceRequest()\n");
	FLUSHLOG ;
#endif
	ServiceRequest();
}
