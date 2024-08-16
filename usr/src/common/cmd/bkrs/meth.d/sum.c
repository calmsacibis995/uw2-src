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


#ident	"@(#)bkrs:common/cmd/bkrs/meth.d/sum.c	1.9.5.3"
#ident  "$Header: sum.c 1.2 91/06/21 $"

#include	<limits.h>
#include	<sys/types.h>
#include	<stdio.h>
#include	<fcntl.h>
#include	<bktypes.h>
#include 	"libadmIO.h"
#include	<method.h>
#include	<errno.h>
#include        <stdlib.h>  
#include        <backup.h>  
#include        <brarc.h>   

/* code from cmd/sum.c to sum a single backup media */

extern int	brlog();
extern int	close();
extern int	g_close();
extern GFILE	*g_open();
extern int	g_read();
extern int      g_seek();         
extern int      archive_bufsize;  

void
sum(buf, nbytes, cur_sum)
register char	*buf;
register long	nbytes;
unsigned	*cur_sum;
{
	register unsigned	sump = *cur_sum;
	register long		i;
	register		c;

	for (i = 0; i < nbytes; i++) {
		c = *buf++;

		if (sump&01)
			sump = (sump >> 1) + 0x8000;
		else
			sump >>= 1;

		sump += c;

		sump &= 0xFFFF;
	}
	*cur_sum = sump;
} /* sum() */

static int                                          
sum_hdr(f, dev, chksum, nbytes)                     
GFILE **f;                                          
char *dev;                                          
unsigned *chksum;                                   
long    *nbytes;                                    
{                                                   
        char *hd = NULL;                            
        long hd_size = 0;                           
        register struct br_arc *a;                  
        register char *wrk;                         
        int ret, readsize = 0, i, savi;             
        int     buf_sz;                             
        unsigned        new_sum = *chksum;          
        long            hdrsize = 0;                
                                                    
                                                    
        hd = (char *) malloc((unsigned) 512);       
        if(hd == NULL)                              
        {                                           
                brlog("no memory for archive hdr"); 
                return(-1);                         
        }                                           
        else                                                        
        {                                                           
                hd_size = 512;                                      
        }                                                           
                                                                    
        a = (struct br_arc *) hd;                                   
                                                                    
        buf_sz = (*f)->_size;                                       
                                                                    
        if (g_flush((*f), 512) < 0)                                 
        {                                                           
                brlog("g_flush failed");                            
                return(-1);                                         
        }                                                           
        for(i=0; i<10; i++)                                         
        {                                                           
                ret = g_read((*f), hd, 512);                        
                sum( hd, 512L, &new_sum );                          
                hdrsize += 512;                                     
#ifdef TRACE                                                        
                if(ret != 512)                                      
                        brlog("sum_hdr: g_read returns %d f=%d %s", 
                                ret,(*f),SE);                      
#endif                                                                      
                if(a->br_magic == BR_MAGIC)                                 
                        break;                                              
#ifdef TRACE                                                                
                else                                                        
                {                                                           
                        brlog(" sum_hdr: a->br_magic != BR_MAGIC i=%d", i); 
                }                                                           
#endif                                                                      
        }                                                                   
        if((ret != 512) || (i >= 10))                                       
        {                                                                   
                if (g_seek((*f), 0l, 0) < 0)                                
                {                                                           
                        brlog("g_seek failed for archive hdr");             
                }                                                           
                brlog("sum_hdr: no BR_MAGIC");                              
                return(0);                                                  
        }                                                                   
        savi = i;                                                           
                                                                            
#ifdef TRACE                                                                
        brlog(" sum_hdr: ret=%d, i=%d, a->br_length=%d, hd_size=%d",       
                ret, i, a->br_length, hd_size);                              
#endif                                                                       
                                                                             
        if(a->br_length > hd_size)                                           
        {                                                                    
                new_sum = *chksum;                                           
                hdrsize = 0;                                                 
                hd_size = a->br_length;                                      
                hd = (char *) realloc(hd, (unsigned)(a->br_length));         
                if(hd == NULL)                                               
                {                                                            
                        brlog("sum_hdr: not enough memory for archive hdr"); 
                        hd_size = 0;                                         
                                                                             
                        (void) g_seek((*f), (hd_size - 512), 1);             
                        return(-1);                                          
                }                                                            
                else                                                         
                {                                                            
                        a = (struct br_arc *) hd;                            
                }                                                            
                                                                             
                readsize = a->br_length;                                     
                wrk = hd;                                                 
                g_close(*f);                                              
                (*f) = g_open(dev, O_RDONLY, 0);                          
                if((*f) == NULL)                                          
                {                                                         
                        brlog("sum_hdr: reopen of %s failed %s",dev,SE);  
                        return(-1);                                       
                }                                                         
                buf_sz = (*f)->_size;                                     
                                                                          
                if (g_flush((*f), 512) < 0)                               
                {                                                         
                        brlog("sum_hdr: g_flush failed");                 
                        return(-1);                                       
                }                                                         
                for(i=0; i<savi; i++)                                     
                {                                                         
                        ret = g_read((*f), hd, 512);                      
                        sum(hd, 512L, &new_sum);                          
                        hdrsize += 512;                                   
                }                                                         
                                                                          
                if(readsize > 0)                                            
                {                                                               
                        errno = 0;                                              
                        ret = g_read((*f), wrk, readsize);                      
                        sum(wrk, (long)readsize, &new_sum);                     
                        hdrsize += readsize;                                    
#ifdef TRACE                                                                    
                        brlog("sum_hdr: ret=%d", ret);                          
#endif                                                                          
                        if(ret != readsize)                                     
                        {                                                       
                                if(ret <= 0)                                    
                                {                                               
                                        brlog("sum_hdr: read of extended hdr failed %s",                                                                        
                                                SE);                            
                                                                                
                                                (void) g_seek((*f), (hd_size - 512), 1);                                                                        
                                        return(-1);                             
                                }                                               
                                else                                            
                                {                                               
                                        brlog("sum_hdr: read hdr incomplete");  
                                                                                
                                        (void) g_seek((*f), (hd_size - (512 + ret)), 1);
                                        return(-1);                             
                                }                                               
                        }                                                       
                }                                                               
        }                                                                       
                                                                                
        *chksum = new_sum;                                                      
        *nbytes -= hdrsize;                                                     
                                                                                
        if (g_flush((*f), buf_sz) < 0)                                          
        {                                                                       
                brlog("sum_hdr: g_flush failed");                               
                return(-1);                                                     
        }                                                                       
        free((void *)hd);                                                       
        return(1);                                                              
}                                           
                                    
chk_vol_sum(mp, f, nbytes, filename, old_sum)
m_info_t	*mp;
GFILE		**f;
long		nbytes;
char		*filename;
unsigned	old_sum;
{
	long		l;
        char            *buf;  
	int		n;
	int		i;
	unsigned	new_sum;

	(void) g_close(*f);

	(*f) = g_open (filename, O_RDONLY, 0);

	if ((*f) == NULL) {
		brlog("reopen of %s failed %s", filename, SE);
		return(1);
	}
	new_sum = 0;

#ifdef TRACE
	brlog("chk_vol_sum: nbytes=%d",nbytes);
#endif
        buf = (char *) malloc((unsigned) archive_bufsize);      
        if(buf == NULL)                                         
        {                                                       
                brlog("chk_vol_sum: no memory for check sum");  
                return(1);                                      
        }                                                       
        if ( mp->dtype != IS_DPART && mp->dtype != IS_FILE &&   
             mp->dtype != IS_DIR )                              
        {                                                       
                i = sum_hdr(f, filename, &new_sum, &nbytes);    
                if ( i != 1 )                                   
                {                                               
                        brlog("checksum failed");               
                        free((void *)buf);                      
                        return(1);                              
                }                                               
        }                                                       
	while (nbytes > 0) {
                n = (nbytes > archive_bufsize) ? archive_bufsize : nbytes;  

		if ((i = g_read(*f, buf, n)) != n) {
			brlog("chk_vol_sum: read %d returned %d nbytes=%d  %s",
					n, i,nbytes, SE);
                        free((void *)buf);   
	 		return(1);
		}
		sum(buf, (long) n, &new_sum);
		nbytes -= n;
	}
#ifdef TRACE
	brlog("chk_vol_sum: old_sum=%x new_sum=%x",old_sum,new_sum);
#endif
        free((void *)buf);        
	return(new_sum != old_sum);
} /* ckh_vol_sum() */
