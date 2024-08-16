#ident	"@(#)unixtsa:common/cmd/unixtsa/tsalib/tsalib.C	1.6"

#include "tsad.h"
#include <sys/filio.h>

int t_nonblocking(int fd);
int t_blocking(int fd);

int CloseTransportEndpoint(int skt)
{
	int	status ;
	char	rcv_buf[BUFSIZ];
	int	flags = 0;
	struct pollfd pollfd;

	/* initiate a closure */
	status = t_sndrel(skt);
	if(status == -1)
	{
		status = T_SNDREL_ERROR;
		goto goback;
	}
	pollfd.fd = skt;
	pollfd.events = POLLIN;	/* poll for reveive events */
	pollfd.revents = 0;
	for(;;)
	{
		status = poll(&pollfd, 1L, CLOSE_TIME_OUT);	/* timeout = 10 secs for now */
		if(status == -1)	/* if failure */
		{
			status = POLL_ERROR;
			goto goback;
		}
		else if(status == 0)	/* timeout */
		{
			status = POLL_TIME_OUT_ERROR;
			goto goback;
		}
		else if(pollfd.revents & POLLERR)
		{
			status = POLL_ERROR;
			goto goback;
		}
		else if(pollfd.revents & POLLHUP)
		{
			status = REMOTE_HUNGUP;
			goto goback;
		}
		else if(pollfd.revents & POLLNVAL)
		{
			status = INVALID_DESCRIPTOR;
			goto goback;
		}
		switch(t_look(skt))
		{
			case	T_DATA:
					t_nonblocking(skt);
					while(t_rcv(skt, rcv_buf, BUFSIZ, &flags) != -1)
						flags = 0;
					t_blocking(skt);
					continue; /* in the forever loop */

			case	T_ORDREL:
					if(t_rcvrel(skt) == -1)
					{
						status = T_RCVREL_ERROR;
						goto goback;
					}
					break;
					
			case	T_DISCONNECT:
					if(t_rcvdis(skt,(struct t_discon *)0) == -1)
					{
						status = T_RCVDIS_ERROR;
						goto goback;
					}
					break;

			default:
					break;
		}
		status = 0;
		break;
	} /* for(;;) */

goback:

	/* finally close the socket */
	t_close(skt);

	return(status);
}	/*CloseTransportEndpoint()*/

int t_blocking(int fd)
{
	int rc = 0 ;

	return( ioctl(fd,FIONBIO,(char *)&rc) );
}

int t_nonblocking(int fd)
{
	int rc = 1 ;

	return( ioctl(fd,FIONBIO,(char *)&rc) );
}

/*
 * PollTransportEndPoint - poll the endpoint for the given event.
 *
 * parameters :
 *    fd - end point to be polled.
 *    event - poll event of interest (only one of POLLIN and POLLOUT)
 *    polltimeout - timeout in secs.
 *    no_of_tries - number of times to try, if poll times out.
 *
 * returns : 0 - success (endpoint is readable(POLLIN) or 
 *               writable(POLLOUT)
 *          -1 - Either timeout and exhausted or error. errno is set.
 * errno values :
 *     ETIMEDOUT - timed out and exhausted
 *     EPROTO    - transport error.
 */

int PollTransportEndPoint(int fd,
	short event, int polltimeout, int no_of_tries)
{
	struct pollfd   fds[1] ;
	int timeout = polltimeout * 1000;
	int nd ;

	fds[0].fd = fd ;
	fds[0].events = event ;
	fds[0].revents = 0 ;

	while( no_of_tries > 0 ) {
		nd = poll(fds,1,timeout);
		if ( nd < 0 ) {
			if ( errno == EINTR ) {
				continue ;
			}
			return(-1);
		}
		if ( nd == 0 ) { /* poll timed out */
			no_of_tries-- ;
			continue ;
		}
		if ( fds[0].revents & POLLERR || fds[0].revents & POLLHUP ||
					fds[0].revents & POLLNVAL) {
			errno = EPROTO ;
			return(-1);
		}
		if ( fds[0].revents == event ) {
			return(0);
		}
		errno = EPROTO ;
		return(-1);
	}
	errno = ETIMEDOUT ;
	return(-1);
}

#ifdef DEBUG
void show_buf(char *buf, int len)
{
	unsigned char c;
	char *p1, *p2;
	int i, j, h;

	p1 = p2 = buf;
	i = 0;
	fprintf(stderr,"\n");
	while (i < len) {
		h = ((len - i) >= 16) ? 16 : (len - i);
		for (j=0; j < h; j++) {
			c = (*p1++) & 0x00FF;
			fprintf(stderr,"%02x ", c);
		}
		fprintf(stderr,"    ");
		for (j=0; j < h; j++) {
			c = (*p2++) & 0x00FF;
			if ((c > 0x1f) && (c < 0x7f))
				fprintf(stderr,"%c", c);
			else
				fprintf(stderr,".");
		}
		fprintf(stderr,"\n");
		i += h;
	}
	fprintf(stderr,"\n");
}

void fshow_buf(FILE *outfp, char *buf, int len)
{
	unsigned char c;
	char *p1, *p2;
	int i, j, h;

 	p2 = buf ;
	p1 = p2 ;
	i = 0;
	fprintf(outfp,"\nBlock of %d bytes\n", len);
	while (i < len) {
		h = ((len - i) >= 16) ? 16 : (len - i);
		for (j=0; j < h; j++) {
			c = (*p1++) & 0x00FF;
			fprintf(outfp,"%02x ", c);
		}
		fprintf(outfp,"    ");
		for (j=0; j < h; j++) {
			c = (*p2++) & 0x00FF;
			if ((c > 0x1f) && (c < 0x7f))
				fprintf(outfp,"%c", c);
			else
				fprintf(outfp,".");
		}
		fprintf(outfp,"\n");
		i += h;
	}
	fprintf(outfp,"\n");
	fflush(outfp) ;
}

DispalyMessage(char *message)
{
}
#endif

#define MAX_DUMPBUF_LEN	512

void dump_buf(char *buf, int len, char *dumpBuf)
{
	unsigned char c;
	char *p1;
	int i, j, h, idx = 0;
#ifdef DEBUG
	char *p2 ;
#endif

	p1 = buf;
	i = 0;
	if ( len >= MAX_DUMPBUF_LEN ) {
		len = MAX_DUMPBUF_LEN - 1 ;
	}
#ifdef DEBUG
	sprintf(dumpBuf+idx++,"\n");
#endif
	while (i < len) {
		h = ((len - i) >= 16) ? 16 : (len - i);
		for (j=0; j < h; j++) {
			c = (*p1++) & 0x00FF;
			sprintf(dumpBuf+idx,"%02x ", c);
			idx +=2 ;
		}
#ifdef DEBUG
		sprintf(dumpBuf+idx,"    ");
		idx += 4 ;
		for (j=0; j < h; j++) {
			c = (*p2++) & 0x00FF;
			if ((c > 0x1f) && (c < 0x7f))
				sprintf(dumpBuf+idx++,"%c", c);
			else
				sprintf(dumpBuf+idx++,".");
		}
		sprintf(dumpBuf+idx++,"\n");
#endif
		i += h;
	}
#ifdef DEBUG
	sprintf(dumpBuf+idx++,"\n");
#endif
}

