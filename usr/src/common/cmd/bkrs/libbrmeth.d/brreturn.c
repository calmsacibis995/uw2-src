/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:common/cmd/bkrs/libbrmeth.d/brreturn.c	1.5.5.3"
#ident  "$Header: brreturn.c 1.2 91/06/21 $"

#include	<sys/types.h>
#include	<time.h>
#include	<backup.h>
#include	<bkmsgs.h>
#include	<bkrs.h>

extern int brtype;
extern pid_t bkdaemonpid;

extern int bkm_send();
extern unsigned int strlen();
extern char *strcpy();
extern char *strchr();  

brreturncode( reason, nblocks, errortext )
int reason, nblocks;
char *errortext;
{
	bkdata_t msg;
        char *ptr, *str;   

	switch( brtype ) {

	case BACKUP_T:
		if( reason == BRSUCCESS ) {
			msg.done.nblocks = nblocks;
			if( bkm_send( bkdaemonpid, DONE, &msg ) == -1 )
				return( BRFATAL );
		} else {
			if( strlen( errortext ) > BKTEXT_SZ )
				return( BRTOOBIG );
			msg.failed.reason = reason;
			(void) strcpy( msg.failed.errmsg, errortext );
			if( bkm_send( bkdaemonpid, FAILED, &msg ) == -1 )
				return( BRFATAL );
		}
		break;

	case RESTORE_T:
                if( reason == BRSUCCESS ) {                                   
                        msg.done.nblocks = nblocks;                           
                        if( bkm_send( bkdaemonpid, DONE, &msg ) == -1 )       
                                return( BRFATAL );                            
                } else {                                                      
                        if( strlen( errortext ) > BKTEXT_SZ )                 
                                return( BRTOOBIG );                           
                        msg.rsret.retcode = reason;                           
                        msg.rsret.jobid[0] = '\0';                            
                        if (strncmp(errortext,"Job ID ",strlen("Job ID "))==0)
                        {                                                     
                                strncpy(msg.rsret.jobid,errortext+            
                                        strlen("Job ID "),                    
                                        sizeof(msg.rsret.jobid));             
                                str = strchr(msg.rsret.jobid,':');            
                                if (str) *str = '\0';                         
                        }                                                     
                        (void) strcpy( msg.rsret.errmsg, errortext );         
                        if (reason == BRMKFSFAILED)                           
                        {                                                     
                                if( bkm_send( bkdaemonpid, RSMKFSFAILED, &msg ) == -1 )                                                                         
                                        return( BRFATAL );                      
                        }                                                       
                        else                                                    
                        if( bkm_send( bkdaemonpid, RSRESULT, &msg ) == -1 )     
                                return( BRFATAL );                              
                }                                                               
		break;

	default:
		return( BRNOTINITIALIZED );
	}
	return( BRSUCCESS );
}
