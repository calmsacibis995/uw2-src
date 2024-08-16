#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libcrypt:libcrypt.mk	1.20.4.9"

#	Makefile for libcrypt

include $(LIBRULES)

.SUFFIXES: .P .P_d .P_i .o_d .o_i

LDFLAGS =

LFLAGS = -G -dy -ztext

HFLAGS = -h /usr/lib/libcrypt.so

LINTFLAGS = 

MAKEFILE = libcrypt.mk

LIBRARY = libcrypt.a
LIBRARY_I = libcrypt_i.a
LIBRARY_D = libcrypt_d.a

DOTSO = libcrypt.so
DOTSO_I = libcrypt_i.so
DOTSO_D = libcrypt_d.so

OBJECTS_O =  crypt.o cryptio.o des_encrypt.o cryptbuf.o

OBJECTS_P=$(OBJECTS_O:.o=.P)

LIBRARY_I_OBJS = des_crypt.o_i enigma.o $(OBJECTS_O)
DOTSO_I_OBJS = des_crypt.P_i enigma.P $(OBJECTS_P)

LIBRARY_D_OBJS = des_crypt.o_d des_decrypt.o enigma.o $(OBJECTS_O)
DOTSO_D_OBJS = des_crypt.P_d des_decrypt.P enigma.P $(OBJECTS_P)

all: intl
	if [ -s des_decrypt.c ] ;\
	then \
		$(MAKE) -f $(MAKEFILE) dom $(MAKEARGS) ;\
	fi

intl: $(LIBRARY_I) $(DOTSO_I)

dom: $(LIBRARY_D) $(DOTSO_D)

$(LIBRARY_I):	$(LIBRARY_I_OBJS)
	$(AR) $(ARFLAGS) $@ `$(LORDER) $(LIBRARY_I_OBJS) | $(TSORT)`

$(DOTSO_I):	$(DOTSO_I_OBJS)
	$(CC) $(LFLAGS) $(HFLAGS) -o $@ $(DOTSO_I_OBJS)

$(LIBRARY_D):	$(LIBRARY_D_OBJS)
	$(AR) $(ARFLAGS) $@ `$(LORDER) $(LIBRARY_D_OBJS) | $(TSORT)`

$(DOTSO_D):	$(DOTSO_D_OBJS)
	$(CC) $(LFLAGS) $(HFLAGS) -o $@ $(DOTSO_D_OBJS)

clean:
	rm -f $(OBJECTS_P) $(OBJECTS_O)
	rm -f des_crypt.o_d des_crypt.P_d des_crypt.P_i des_crypt.o_i
	rm -f des_decrypt.o des_decrypt.P
	[ ! -f enigma.c ] || rm -f enigma.P enigma.o

clobber: clean
	rm -f $(LIBRARY_I) $(LIBRARY_D) $(DOTSO_I) $(DOTSO_D)

install: all
	rm -f $(USRLIB)/$(LIBRARY)
	rm -f $(USRLIB)/$(DOTSO)
	$(INS) -f $(USRLIB) -m 644 $(LIBRARY_I)
	$(INS) -f $(USRLIB) -m 755 $(DOTSO_I)
	if [ -s des_decrypt.c ] ;\
	then \
		$(INS) -f $(USRLIB) -m 644 $(LIBRARY_D) ;\
		$(INS) -f $(USRLIB) -m 755 $(DOTSO_D) ;\
		ln -f $(USRLIB)/$(LIBRARY_D) $(USRLIB)/$(LIBRARY) ;\
		ln -f $(USRLIB)/$(DOTSO_D) $(USRLIB)/$(DOTSO) ;\
	else \
		ln -f $(USRLIB)/$(LIBRARY_I) $(USRLIB)/$(LIBRARY) ;\
		ln -f $(USRLIB)/$(DOTSO_I) $(USRLIB)/$(DOTSO) ;\
	fi

lintit:	
	$(LINT) $(LINTFLAGS) *.c
	
.c.P:
	$(CC) -Wa,-o,$*.P $(CFLAGS) $(DEFLIST) $(PICFLAG) -c $*.c

.c.P_i:
	$(CC) -Wa,-o,$*.P_i $(CFLAGS) $(DEFLIST) $(PICFLAG) -c $*.c -DINTERNATIONAL

.c.P_d:
	$(CC) -Wa,-o,$*.P_d $(CFLAGS) $(DEFLIST) $(PICFLAG) -c $*.c

.c.o_i:
	$(CC) -Wa,-o,$*.o_i $(CFLAGS) $(DEFLIST) -c $*.c -DINTERNATIONAL

.c.o_d:
	$(CC) -Wa,-o,$*.o_d $(CFLAGS) $(DEFLIST) -c $*.c
