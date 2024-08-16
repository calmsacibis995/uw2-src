#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libNwCalls:libnwcalls.mk	1.13"
#ident	"$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/libnwcalls.mk,v 1.20 1994/09/30 23:01:14 rebekah Exp $"

include $(LIBRULES)

.SUFFIXES: .P

DOTSO = libNwCal.so
DOTA = libNwCal.a

LOCALINC = -I../../head/inc -I../../head/nw -I../../head

SHLIBLDFLAGS = -G -ztext
LFLAGS = -G -dy -ztext
LOCALDEF = -DUSL -DSVR4 -DI18N -DFUNCPROTO=15 -DNARROWPROTO -DN_PLAT_UNIX -D_INTELC32_ -DUNIX -DNATIVE -DN_USE_CRT  

HFLAGS = -h /usr/lib/libNwCal.so

SRCS1 = \
  abrtjob2.c \
  acctinst.c \
  addobj.c \
  addtrst.c \
  addtrust.c \
  aerasfil.c \
  afilesrc.c \
  afpdiren.c \
  afpsupp.c \
  alcprmdh.c \
  amovdent.c \
  ascndir.c \
  ascndir2.c \
  ascnentr.c \
  ascnfil2.c \
  ascntrst.c \
  asextfil.c \
  atmpdirh.c \
  atservqu.c \
  brdtocon.c \
  chgehndl.c \
  chjoben2.c \
  chjobpo2.c \
  chkaroot.c \
  chkconpr.c \
  chknwver.c \
  chsvrri2.c \
  cleanpth.c \
  closbind.c \
  closeea.c \
  closefil.c \
  closesem.c \
  clrconum.c \
  clrfllok.c \
  clrlgrec.c \
  clrlgset.c \
  clrlokst.c \
  clrphrec.c \
  clrphset.c \
  clsabtq2.c \
  clsstrt2.c \
  compath.c \
  compath2.c \
  conninfo.c \
  consolbr.c \
  conv2ent.c \
  convaug.c \
  convfhan.c \
  convhand.c \
  convspc.c \
  copyrite.c \
  creatdir.c \
  createdi.c \
  createfi.c \
  createqu.c \
  crtefil.c \
  crteobj.c \
  crteprop.c \
  crteqjo2.c \
  crttmphd.c \
  cvoldskr.c \
  datetime.c \
  deletdir.c \
  deletefi.c \
  delfile.c \
  delhndl.c \
  delobj.c \
  delobjst.c \
  delprop.c \
  deltrst.c \
  deltrust.c \
  destrque.c \
  detserqu.c \
  disfslog.c \
  distts.c \
  downfs.c \
  enbfslog.c \
  enbtts.c \
  encrypt.c \
  etridhan.c \
  examsem.c \
  ffirstea.c \
  filecon.c \
  filerdwr.c \
  finishj2.c \
  four2six.c \
  gcmpthl2.c \
  gconfobj.c \
  gdirbase.c \
  gdirspac.c \
  gdskrest.c \
  gefright.c \
  gentinfo.c \
  getacces.c \
  getaccst.c \
  getbasei.c \
  getchinf.c \
  getconid.c \
  getconus.c \
  getdate.c \
  getdirli.c \
  getdrvmt.c \
  getdskca.c \
  getdskch.c \
  getdsksp.c \
  getdskut.c \
  geteahan.c
  
  
SRCS2 = \
  getladdr.c \
  getext.c \
  getfilei.c \
  getfilss.c \
  getfsinf.c \
  getfslog.c \
  getfsver.c \
  gethndl.c \
  getisinf.c \
  getlancf.c \
  getlanio.c \
  getlmnlt.c \
  getlname.c \
  getmisc.c \
  getmmolt.c \
  getncpxl.c \
  getobjid.c \
  getown.c \
  getphdsk.c \
  getpstat.c \
  getpthde.c \
  getque.c \
  getticon.c \
  gettrust.c \
  gettts.c \
  getvolst.c \
  gextvol.c \
  gfileina.c \
  ggrsapi.c \
  gjoblst2.c \
  gjobsiz2.c \
  gksinfo.c \
  glancoci.c \
  glancuci.c \
  gncpxinf.c \
  gncpxinn.c \
  gnninfo.c \
  gnrinfo.c \
  gnrsinfo.c \
  gnumncpx.c \
  gpscoinf.c \
  gpscuinf.c \
  gpsnblbn.c \
  gserinfo.c \
  gsscinfo.c \
  gssetcat.c \
  gssinfo.c \
  gtaclbyt.c \
  gtactpst.c \
  gtalanbl.c \
  gtcmpthl.c \
  gtconnum.c \
  gtcpuinf.c \
  gtdrchin.c \
  gtfsmisc.c \
  gtfsver.c \
  gtgclinf.c \
  gtjobsiz.c \
  gtlancin.c \
  gtlslinf.c \
  gtlsllbs.c \
  gtmaxcon.c \
  gtmmoclt.c \
  gtmmoinf.c \
  gtmnbymn.c \
  gtnfsinf.c \
  gtnlminf.c \
  gtnlmlin.c \
  gtnlmrtl.c \
  gtnsinfo.c \
  gtnsload.c \
  gtnspath.c \
  gtobjnam.c \
  gtobjrts.c \
  gtosvinf.c \
  gtpkbinf.c \
  gtpsnbmn.c \
  gtpssinf.c \
  gtreqver.c \
  gtserial.c \
  gtsrvdes.c \
  gtsrvnam.c \
  gtusrinf.c \
  gtvlsinf.c \
  gtvseglt.c \
  gvolblvl.c \
  gvolinfo.c \
  indexpth.c \
  islnssup.c \
  ismgr.c \
  isobjset.c \
  jnumswap.c \
  lenstr.c \
  logfllok.c \
  login.c \
  loglgrec.c \
  logotsrv.c \
  logphrec.c \
  loklgset.c \
  loklokst.c \
  lokphset.c \
  mapvol.c \
  mapvol2.c \
  migdata.c \
  migfinfo.c \
  migget.c \
  migrdata.c \
  migset.c \
  migsinfo.c \
  migstat.c \
  migvolst.c \
  modmaxrt.c \
  newcon.c \
  newfile.c \
  nsrename.c
  
  
SRCS3 = \
  nwinit.c \
  objconn.c \
  objsrty.c \
  ocnsent.c \
  openbind.c \
  opendstr.c \
  openea.c \
  openfile.c \
  openfork.c \
  opensem.c \
  orderreq.c \
  propsrty.c \
  prsnwp2.c \
  prspath2.c \
  purgdel.c \
  purgefil.c \
  qryaccin.c \
  qutils.c \
  rdexinfo.c \
  rdnsinfo.c \
  readea.c \
  readprop.c \
  readqst2.c \
  readqsta.c \
  recdel.c \
  relfllok.c \
  rellgrec.c \
  rellgset.c \
  rellokst.c \
  relphrec.c \
  relphset.c \
  removjob.c \
  renamdir.c \
  rename.c \
  renamobj.c \
  renfile.c \
  req2all.c \
  request.c \
  rjobent2.c \
  rmovjob2.c \
  rstorfil.c \
  rsvrsta2.c \
  rtsvrrit.c \
  scanafpf.c \
  scandel.c \
  scanncpx.c \
  scanobj.c \
  scanprop.c \
  scnconuf.c \
  scnllcon.c \
  scnllnam.c \
  scnofco2.c \
  scnplcaf.c \
  scnplfil.c \
  scnsecon.c \
  scnsenam.c \
  scnsentr.c \
  scntrst.c \
  scntrst2.c \
  sendmsg.c \
  servcopy.c \
  servejo2.c \
  servinf1.c \
  setattr.c \
  setdir.c \
  setdire.c \
  setdrest.c \
  setext.c \
  setfile2.c \
  setfilei.c \
  setfsdat.c \
  setinfo.c \
  setlname.c \
  setqsta2.c \
  setsvrst.c \
  setvlim.c \
  signsem.c \
  sncpxreq.c \
  stcfsize.c \
  stripsrv.c \
  subaccch.c \
  subaccho.c \
  subaccno.c \
  svoldskr.c \
  tmphand2.c \
  tmphandl.c \
  ttsabort.c \
  ttsavail.c \
  ttsbegin.c \
  ttsend.c \
  ttsgetco.c \
  ttsgetfl.c \
  ttsgetpr.c \
  ttssetco.c \
  ttssetfl.c \
  ttssetpr.c \
  ttsstat.c \
  volinfo.c \
  waitsem.c \
  wildchk.c \
  wildpath.c \
  writeea.c \
  writeens.c \
  writens.c \
  wrteprop.c
  
 
 
OBJS_O_1 = \
  abrtjob2.o \
  acctinst.o \
  addobj.o \
  addtrst.o \
  addtrust.o \
  aerasfil.o \
  afilesrc.o \
  afpdiren.o \
  afpsupp.o \
  alcprmdh.o \
  amovdent.o \
  ascndir.o \
  ascndir2.o \
  ascnentr.o \
  ascnfil2.o \
  ascntrst.o \
  asextfil.o \
  atmpdirh.o \
  atservqu.o \
  brdtocon.o \
  chgehndl.o \
  chjoben2.o \
  chjobpo2.o \
  chkaroot.o \
  chkconpr.o \
  chknwver.o \
  chsvrri2.o \
  cleanpth.o \
  closbind.o \
  closeea.o \
  closefil.o \
  closesem.o \
  clrconum.o \
  clrfllok.o \
  clrlgrec.o \
  clrlgset.o \
  clrlokst.o \
  clrphrec.o \
  clrphset.o \
  clsabtq2.o \
  clsstrt2.o \
  compath.o \
  compath2.o \
  conninfo.o \
  consolbr.o \
  conv2ent.o \
  convaug.o \
  convfhan.o \
  convhand.o \
  convspc.o \
  copyrite.o \
  creatdir.o \
  createdi.o \
  createfi.o \
  createqu.o \
  crtefil.o \
  crteobj.o \
  crteprop.o \
  crteqjo2.o \
  crttmphd.o \
  cvoldskr.o \
  datetime.o \
  deletdir.o \
  deletefi.o \
  delfile.o \
  delhndl.o \
  delobj.o \
  delobjst.o \
  delprop.o \
  deltrst.o \
  deltrust.o \
  destrque.o \
  detserqu.o \
  disfslog.o \
  distts.o \
  downfs.o \
  enbfslog.o \
  enbtts.o \
  encrypt.o \
  etridhan.o \
  examsem.o \
  ffirstea.o \
  filecon.o \
  filerdwr.o \
  finishj2.o \
  four2six.o \
  gcmpthl2.o \
  gconfobj.o \
  gdirbase.o \
  gdirspac.o \
  gdskrest.o \
  gefright.o \
  gentinfo.o \
  getacces.o \
  getaccst.o \
  getbasei.o \
  getchinf.o \
  getconid.o \
  getconus.o \
  getdate.o \
  getdirli.o \
  getdrvmt.o \
  getdskca.o \
  getdskch.o \
  getdsksp.o \
  getdskut.o \
  geteahan.o
  
  
OBJS_O_2 = \
  getladdr.o \
  getext.o \
  getfilei.o \
  getfilss.o \
  getfsinf.o \
  getfslog.o \
  getfsver.o \
  gethndl.o \
  getisinf.o \
  getlancf.o \
  getlanio.o \
  getlmnlt.o \
  getlname.o \
  getmisc.o \
  getmmolt.o \
  getncpxl.o \
  getobjid.o \
  getown.o \
  getphdsk.o \
  getpstat.o \
  getpthde.o \
  getque.o \
  getticon.o \
  gettrust.o \
  gettts.o \
  getvolst.o \
  gextvol.o \
  gfileina.o \
  ggrsapi.o \
  gjoblst2.o \
  gjobsiz2.o \
  gksinfo.o \
  glancoci.o \
  glancuci.o \
  gncpxinf.o \
  gncpxinn.o \
  gnninfo.o \
  gnrinfo.o \
  gnrsinfo.o \
  gnumncpx.o \
  gpscoinf.o \
  gpscuinf.o \
  gpsnblbn.o \
  gserinfo.o \
  gsscinfo.o \
  gssetcat.o \
  gssinfo.o \
  gtaclbyt.o \
  gtactpst.o \
  gtalanbl.o \
  gtcmpthl.o \
  gtconnum.o \
  gtcpuinf.o \
  gtdrchin.o \
  gtfsmisc.o \
  gtfsver.o \
  gtgclinf.o \
  gtjobsiz.o \
  gtlancin.o \
  gtlslinf.o \
  gtlsllbs.o \
  gtmaxcon.o \
  gtmmoclt.o \
  gtmmoinf.o \
  gtmnbymn.o \
  gtnfsinf.o \
  gtnlminf.o \
  gtnlmlin.o \
  gtnlmrtl.o \
  gtnsinfo.o \
  gtnsload.o \
  gtnspath.o \
  gtobjnam.o \
  gtobjrts.o \
  gtosvinf.o \
  gtpkbinf.o \
  gtpsnbmn.o \
  gtpssinf.o \
  gtreqver.o \
  gtserial.o \
  gtsrvdes.o \
  gtsrvnam.o \
  gtusrinf.o \
  gtvlsinf.o \
  gtvseglt.o \
  gvolblvl.o \
  gvolinfo.o \
  indexpth.o \
  islnssup.o \
  ismgr.o \
  isobjset.o \
  jnumswap.o \
  lenstr.o \
  logfllok.o \
  login.o \
  loglgrec.o \
  logotsrv.o \
  logphrec.o \
  loklgset.o \
  loklokst.o \
  lokphset.o \
  mapvol.o \
  mapvol2.o \
  migdata.o \
  migfinfo.o \
  migget.o \
  migrdata.o \
  migset.o \
  migsinfo.o \
  migstat.o \
  migvolst.o \
  modmaxrt.o \
  newcon.o \
  newfile.o \
  nsrename.o
  

OBJS_O_3 = \
  nwinit.o \
  objconn.o \
  objsrty.o \
  ocnsent.o \
  openbind.o \
  opendstr.o \
  openea.o \
  openfile.o \
  openfork.o \
  opensem.o \
  orderreq.o \
  propsrty.o \
  prsnwp2.o \
  prspath2.o \
  purgdel.o \
  purgefil.o \
  qryaccin.o \
  qutils.o \
  rdexinfo.o \
  rdnsinfo.o \
  readea.o \
  readprop.o \
  readqst2.o \
  readqsta.o \
  recdel.o \
  relfllok.o \
  rellgrec.o \
  rellgset.o \
  rellokst.o \
  relphrec.o \
  relphset.o \
  removjob.o \
  renamdir.o \
  rename.o \
  renamobj.o \
  renfile.o \
  req2all.o \
  request.o \
  rjobent2.o \
  rmovjob2.o \
  rstorfil.o \
  rsvrsta2.o \
  rtsvrrit.o \
  scanafpf.o \
  scandel.o \
  scanncpx.o \
  scanobj.o \
  scanprop.o \
  scnconuf.o \
  scnllcon.o \
  scnllnam.o \
  scnofco2.o \
  scnplcaf.o \
  scnplfil.o \
  scnsecon.o \
  scnsenam.o \
  scnsentr.o \
  scntrst.o \
  scntrst2.o \
  sendmsg.o \
  servcopy.o \
  servejo2.o \
  servinf1.o \
  setattr.o \
  setdir.o \
  setdire.o \
  setdrest.o \
  setext.o \
  setfile2.o \
  setfilei.o \
  setfsdat.o \
  setinfo.o \
  setlname.o \
  setqsta2.o \
  setsvrst.o \
  setvlim.o \
  signsem.o \
  sncpxreq.o \
  stcfsize.o \
  stripsrv.o \
  subaccch.o \
  subaccho.o \
  subaccno.o \
  svoldskr.o \
  tmphand2.o \
  tmphandl.o \
  ttsabort.o \
  ttsavail.o \
  ttsbegin.o \
  ttsend.o \
  ttsgetco.o \
  ttsgetfl.o \
  ttsgetpr.o \
  ttssetco.o \
  ttssetfl.o \
  ttssetpr.o \
  ttsstat.o \
  volinfo.o \
  waitsem.o \
  wildchk.o \
  wildpath.o \
  writeea.o \
  writeens.o \
  writens.o \
  wrteprop.o
  

   
OBJS_P_1=$(OBJS_O_1:.o=.P)
OBJS_P_2=$(OBJS_O_2:.o=.P)
OBJS_P_3=$(OBJS_O_3:.o=.P)

.c.P:
	$(CC) -Wa,-o,$*.P $(CFLAGS) $(DEFLIST) $(PICFLAG) -c $*.c

all:	dota dotso

dota: $(OBJS_O_1) $(OBJS_O_2) $(OBJS_O_3) 
	$(AR) $(ARFLAGS) $(DOTA) $(OBJS_O_1) 
	$(AR) $(ARFLAGS) $(DOTA) $(OBJS_O_2) 
	$(AR) $(ARFLAGS) $(DOTA) $(OBJS_O_3) 

dotso: $(OBJS_P_1) $(OBJS_P_2) $(OBJS_P_3) 
	$(LD) -r -o $(DOTSO).r1 $(OBJS_P_1)
	$(LD) -r -o $(DOTSO).r2 $(OBJS_P_2)
	$(LD) -r -o $(DOTSO).r3 $(OBJS_P_3)
	$(LD) $(LFLAGS) $(HFLAGS) -o $(DOTSO) $(DOTSO).r[123] -lNwNcp -lNwClnt -lNwLoc

clean:
	rm -f *.o *.P *.log 

clobber: clean
	rm -f $(DOTA) $(DOTSO) $(DOTSO).r[123]

install: all
	$(INS) -f $(USRLIB) -m 644 $(DOTA)
	$(INS) -f $(USRLIB) -m 755 $(DOTSO)
