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


#ident	"@(#)bkrs:common/cmd/bkrs/meth.d/bknewvol.c	1.6.5.3"
#ident  "$Header: bknewvol.c 1.2 91/06/21 $"

#include	<limits.h> 	/* get PATH_MAX from here, not stdio */
#include	<sys/types.h>
#include	<stdio.h>
#include	<string.h>
#include	<signal.h>
#include	<backup.h>
#include	<bkrs.h>
#include	<method.h>
#include	<setjmp.h>
#include      <fcntl.h>      
#include      <brarc.h>      
#include      <bktypes.h>    
#include      <sys/errno.h>  
#include      <errno.h>      
#include      "libadmIO.h"   
                             
extern int    g_close();     
extern GFILE  *g_open();     
extern int    g_read();      
extern int    g_seek();      
                             
extern char *sys_errlist[];  

extern void	br_state();
extern int	brinvlbl();
extern int	brgetvolume();
extern int	brlog();
extern int	get_media_name();
extern void	newlabel();

extern int	bklevels;

static int    getvolname();         
static int    read_hdr();           
                                    
static int    first_vol = TRUE;     
static char   *hd = NULL;           
static long   hd_size = 0;          

int
bknewvol(mp, fname, reuse, envp, mi)
m_info_t	*mp;
char		fname[];
short		*reuse;
jmp_buf		*envp;
media_info_t	*mi;
{
	int		isfile = 0;
	int		ret;
	char		*dmname = NULL;
	char		label[81];
	media_list_t	*m_info;
#ifdef TRACE
brlog("bknewvol: dtype=%d", mp->dtype);
#endif
	switch(mp->dtype) {
	case IS_DIR:
		if (get_media_name(reuse, &m_info, mi, mp)) {
			return(-1);
		}
		(void) sprintf(fname,"%s/%s",mp->dname,m_info->label);
		isfile++;
		break;
	case IS_FILE:
		if (!strcmp(fname, mp->dname)) {	/* already used */
			brlog("bknewvol: %s has been used",fname);
			sprintf(ME(mp), "Job ID %s: %s has been used", mp->jobid, fname);
			return(-1);
		}
		(void) strcpy(fname, mp->dname);
		isfile++;
		break;
	case IS_DPART:
		if ((mp->blks_per_vol) <= 0) {
			brlog("bknewvol: size of partition %s not specified",
					mp->dname);
			sprintf(ME(mp), "Job ID %s: size of partition %s not specified", mp->jobid, mp->dname);
			return(-1);
		}
		if (!strcmp(fname, mp->dname)) {	/* already used */
			brlog("bknewvol: %s has been used",fname);
			sprintf(ME(mp), "Job ID %s: %s has been used", mp->jobid, fname);
			return(-1);
		}
		(void) strcpy(fname, mp->dname);
		break;
	default:
		if (mp->volpromt) {
			if (get_media_name(reuse, &m_info, mi, mp)) {
				return(-1);
			}
			dmname = m_info->label;
trylab:
                        if ( AUTOM(mp) )                                        
                        {                                                       
                                if (first_vol) first_vol = FALSE;               
                                else                                            
                                {                                               
                                        brlog("Job ID %s: \nOnly one tape allowed in automatic mode, job terminated",                                           
                                                mp->jobid);                     
                                        sprintf(ME(mp), "Job ID %s: \nOnly one tape allowed in automatic mode, job terminated",                                 
                                                mp->jobid);                     
                                        return(-1);                             
                                }                                               
                                                                                
                                if (!getvolname( mp->dname, mp->dchar, label, 81 ))                                                                             
                                {                                               
                                        brlog("Job ID %s: failed to find volume name %s on tape",                                                               
                                                mp->jobid, dmname);             
                                        sprintf(ME(mp), "Job ID %s: failed to find volume name %s on tape",                                                     
                                                mp->jobid, dmname);             
                                        return(-1);                             
                                }                                               
                                else if ( strcmp(dmname,label) )                
                                {                                               
                                        brlog("Job ID %s: tape is labelled %s, should be %s",                                                                   
                                                mp->jobid, label, dmname);      
                                        sprintf(ME(mp), "Job ID %s: tape is labelled %s, should be %s",                                                         
                                                mp->jobid, label, dmname);      
                                        return(-1);                             
                                }                                               
                                else ret = BRSUCCESS;                           
                        }                                                       
                        else                                                    
                        {                                                       
#ifdef TRACE                                                                    
                        brlog("call brgetvolume for %s",dmname);                
#endif                                                                          
                        ret = brgetvolume(dmname, OVRIDE(mp), AUTOM(mp), label);
                        }                                                      
 
			if (ret != BRSUCCESS) {
				brlog("brgetvol returned %d", ret);
				if (ret == BRFATAL)
					sprintf(ME(mp), "Job ID %s: get volume failed for %s", mp->jobid, dmname);
				br_state(mp, envp);
				goto trylab;	/* longjmped out if fatal */
			}
			if (strcmp(dmname, label)) {	/* not what asked for */
				newlabel(mp, m_info, label, envp);
			}
			(void) brinvlbl (label);
		}
		else {
			if (!strcmp(fname, mp->dname)) {	/* already used */
				brlog("bknewvol: %s has been used",fname);
				sprintf(ME(mp), "Job ID %s: %s has been used", mp->jobid, fname);
				return(-1);
			}
		}
		(void) strcpy(fname, mp->dname);
	}
#ifdef TRACE
	if (isfile) {
		brlog("bknewvol: create %s ",fname);
	}
	else if (dmname != NULL) {
		brlog("bknewvol: dev %s vol=%s",fname,dmname);
	}
	else {
		brlog("bknewvol: dev %s ",fname);
	}
#endif
	return(isfile);
} /* bknewvol() */
                                                                   
static int                                                         
getvolname( device, dchar, result, result_sz )                     
char *device, *dchar, *result;                                     
int result_sz;                                                     
{                                                                  
        struct archive_info ai;                                    
        GFILE   *f;                                                
        register rc = 0;                                           
                                                                   
        if( (f = g_open( device, O_RDONLY, 0)) != NULL ) {         
                                                                   
                if( read_hdr( &f, &ai, device ) > 0 ) {            
                        strncpy( result, ai.br_mname, result_sz ); 
                        result[ result_sz-1 ] = '\0';              
                        rc = 1;                                    
                }                                                  
                                                                   
                (void) g_close( f );                               
                                                                   
        } else return( -1 );                                       
                                                                   
        return( rc );                                              
}                                                           
                                                            
static int                                                  
read_hdr(f, ai, dev)                                        
GFILE **f;                                                  
register struct archive_info *ai;                           
char *dev;                                                  
{                                                           
        register struct br_arc *a;                          
        register char *wrk;                                 
        int ret, readsize = 0, i, savi;                     
        int     buf_sz;                                     
        char    volname[81];    /* as result in bknewvol */ 
                                                            
#ifdef TRACE                                                
        brlog(" read_hdr: dev=%s", dev);                    
#endif                                                      
                                                            
        if(hd == NULL) {                                    
                hd = (char *) malloc((unsigned) 512);       
                if(hd == NULL) {                            
                        brlog("no memory for archive hdr"); 
                        return(-1);                        
                }                                        
                else {                                   
                        hd_size = 512;                   
                }                                        
        }                                                
#ifdef TRACE                                             
        brlog(" read_hdr: (*f)->_file=%d", (*f)->_file); 
#endif                                                   
                                                         
        a = (struct br_arc *) hd;                        
                                                         
        buf_sz = (*f)->_size;                            
                                                         
        if (g_flush((*f), 512) < 0) {                    
                brlog("g_flush failed");                 
                return(-1);                              
        }                                                
                                                         
        volname[0] = '\0';                               
        for(i=0; i<10; i++) {                            
                ret = g_read((*f), hd, 512);             
#ifdef TRACE                                             
                if(ret != 512)                          
                        brlog("read_hdr: g_read returns %d f=%d %s",         
                                ret,(*f),sys_errlist[errno]);                
#endif                                                                       
                if ( !strncmp(hd,"Volcopy", 7) )                             
                {                                                            
                        strcpy(volname,hd+8);                                
                                                                             
                        /* copy it in case archive header is missing */      
                        ai->br_mname = volname;                              
                }                                                            
                if(a->br_magic == BR_MAGIC)                                  
                        break;                                               
#ifdef TRACE                                                                 
                else {                                                       
                        brlog(" read_hdr: a->br_magic != BR_MAGIC i=%d", i); 
                }                                                            
#endif                                                                       
        }                                                                    
        if((ret != 512) || (i >= 10)) {                                      
                if ( strlen(volname) == 0 ) /* no labelit information */     
                {                                                            
                        if (g_seek((*f), 0l, 0) < 0) {                       
                                brlog("g_seek failed for archive hdr");     
                        }                                            
                        brlog("read_hdr no BR_MAGIC");               
                        return(-1);                                  
                }                                                    
                return(1);                                           
        }                                                            
        savi = i;                                                    
                                                                     
#ifdef TRACE                                                         
        brlog(" read_hdr: ret=%d, i=%d, a->br_length=%d, hd_size=%d",
                ret, i, a->br_length, hd_size);                      
#endif                                                               
                                                                     
        if(a->br_length > hd_size) {                                 
                hd_size = a->br_length;                              
                hd = (char *) realloc(hd, (unsigned)(a->br_length)); 
                if(hd == NULL) {                                     
                        brlog("not enough memory for archive hdr");  
                        hd_size = 0;                                 
                        (void) g_seek((*f), (hd_size - 512), 1);     
                        return(-1);                                  
                }                                                    
                else {                                              
                        a = (struct br_arc *) hd;                              
                }                                                              
                                                                               
                readsize = a->br_length;                                       
                wrk = hd;                                                      
                g_close(*f);                                                   
                (*f) = g_open(dev, O_RDONLY, 0);                               
                if((*f) == NULL) {                                             
                        brlog("reopen of %s failed %s",dev,sys_errlist[errno]);
                        return(-1);                                            
                }                                                              
                buf_sz = (*f)->_size;                                          
                                                                               
                if (g_flush((*f), 512) < 0) {                                  
                        brlog("g_flush failed");                               
                        return(-1);                                            
                }                                                              
                for(i=0; i<savi; i++) {                                        
                        ret = g_read((*f), hd, 512);                           
#ifdef TRACE                                                                   
                brlog(" read_hdr: ret=%d, i=%d", ret, i);                      
#endif                                                                         
                }                                                             
#ifdef TRACE                                                                    
        brlog(" read_hdr: readsize=%d", readsize);                              
#endif                                                                          
                if(readsize > 0) {                                              
                        errno = 0;                                              
                        ret = g_read((*f), wrk, readsize);                      
        #ifdef TRACE                                                            
                        brlog(" read_hdr: ret=%d", ret);                        
        #endif                                                                  
                        if(ret != readsize) {                                   
                                if(ret <= 0) {                                  
                                        brlog("read of extended hdr failed %s", 
                                                sys_errlist[errno]);            
                                        (void) g_seek((*f), (hd_size - 512), 1);
                                        return(-1);                             
                                }                                               
                                else {                                          
                                        brlog("read hdr incomplete");           
                                        (void) g_seek((*f), (hd_size - (512 + ret)), 1);                                                                        
                                        return(-1);                             
                                }                                               
                        }                                
                }                                                               
        }                                                                       
        wrk = (char *) (&(a->br_data));                                         
                                                                                
        ai->br_seqno = a->br_seqno;     /* sequence num of this vol */          
        ai->br_blk_est = a->br_blk_est; /* num of blks this archive */          
        ai->br_flags = a->br_flags;                                             
        ai->br_media_cap = a->br_media_cap;                                     
        ai->br_length = a->br_length;                                           
                                                                                
        ai->br_sysname = (wrk + a->br_sysname_off);     /* system originating */
        ai->br_method = (wrk + a->br_method_off);       /* method name */       
        ai->br_fsname = (wrk + a->br_fsname_off);       /* orginating fs */     
        ai->br_dev = (wrk + a->br_dev_off);             /* originating device */
        ai->br_fstype = (wrk + a->br_fstype_off);       /* fstype string */     
        ai->br_mname = (wrk + a->br_mname_off);         /* media name */        
                                                                                
#if _not_icldrs                                                                 
#else /* not _not_icldrs */                                                     
        if ( a->br_sysname_off == 0 )                   /* default block size */
        {                                                                       
                ai->br_blksize = 0;                                             
        }                                                                               else                                                   
        {                                                      
                ai->br_blksize = ( (int)(*wrk) & 0xFF ) << 8 ; 
                wrk++;                                         
                ai->br_blksize |= (int)(*wrk) & 0xFF;          
        }                                                      
#ifdef TRACE                                                   
brlog("read_hdr(): blksize %d",ai->br_blksize);                
#endif                                                         
#endif /* not _not_icldrs */                                   
                                                               
        if (g_flush((*f), buf_sz) < 0) {                       
                brlog("g_flush failed");                       
                return(-1);                                    
        }                                                      
#ifdef TRACE                                                   
        brlog(" read_hdr: returning=%d", a->br_length);        
#endif                                                         
        return(a->br_length);                                  
}                                                               
     
