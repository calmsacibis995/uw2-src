/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamuser:user/messages.c	1.6.12.12"
#ident  "$Header: $"

char *errmsgs[] = {
	":1276:uid %ld is reserved.\n",
	":1277:more than NGROUPS_MAX(%d) groups specified.\n",
	":1278:invalid syntax.\nusage: useradd [-u uid [-o] [-i] ] [-g group] [-G group[[,group]...]] [-d dir ]\n               [-s shell] [-c comment] [-m [-k skel_dir]] [-f inactive]\n               [-e expire] [-p passgen] %s%s login\n",
	":1279:Invalid syntax.\nusage:  userdel  [-r] [-n months] login\n",
	":1280:invalid syntax.\nusage: usermod [-u uid [-o] [-U]] [-g group] [-G group[[,group]...]]\n             [-d dir [-m]] [-s shell] [-c comment] [-l new_logname]\n             [-f inactive] [-e expire] [-p passgen] %s%s login\n",
	":1281:Unexpected failure.  Defaults unchanged.\n",
	":1282:Unable to remove files from home directory.\n",
	":1283:Unable to remove home directory.\n",
	":1284:Cannot update system files - login cannot be %s.\n",
	":1285:uid %ld is already in use.  Choose another.\n",
	":1286:%s is already in use.  Choose another.\n",
	":1287:%s does not exist.\n",
	":1288:%s is not a valid %s.  Choose another.\n",
	":1289:%s is in use.  Cannot %s it.\n",
	":1290:%s has no permissions to use %s.\n",
	":1291:There is not sufficient space to move %s home directory to %s\n",
	":1292:%s %s is too big.  Choose another.\n",
	":1293:group %s does not exist.  Choose another.\n",
	":1294:Unable to %s: %s.\n",
	":1295:%s is not a full path name.  Choose another.\n",
	":1296:invalid argument specified with -p flag\n",
	":1297:invalid audit event type or class specified.\n",
	":1298:invalid security level specified.\n",
	":1299:invalid default security level specified.\n",
	":1300:invalid option -a\n",
	":1301:invalid options -h\n",
	":1302:system service not installed.\n",
	":1303:cannot delete security level %s.\n                Current default security level will become invalid.\n",
	":1304:Invalid syntax.\nusage:  usermod [-u uid [-o] [-U]]  [-g group] [-G group[[,group]...]]\n             [-d dir [-m]] [-s shell] [-c comment] \n             [-l new_logname] [-f inactive] [-e expire]\n             [-h [operator]level[-h level]] [-v def_level] \n             [-a[operator]event[,..]]   login\n",
	":1305:%s is the primary group name.  Choose another.\n",
	":1306:invalid security level specified for user's home directory.\n",
	":1307:user's home directory already exists, -w ignored.\n",
	":1308:invalid months value specified for uid aging.\n",
	":1309:uid %d not aged sufficiently. Choose another.\n",
	":1310:unable to access ``%s''\n",
	":1311:The DATEMSK environment variable is null or undefined.\n",
	":1312:The /etc/datemsk file cannot be opened for reading.\n",
	":1313:Failed to get /etc/datemsk file status information.\n",
	":1314:The /etc/datemsk file is not a regular file.\n",
	":1315:An error is encountered while reading the /etc/datemsk file.\n",
	":1316:malloc failed (not enough memory is available).\n",
	":1317:login name ``%s'' may produce unexepected results when used with other commands on the system\n",
	":1318:invalid options -v\n",
	":1319:invalid options -w\n",
	":1320:more than NGROUPS_MAX(%d including basegid) groups specified.\n",
	":1321:ERROR: invalid option usage for NIS user\n",
	":1322:ERROR: unable to contact NIS \n",
	":1323:ERROR: unable to find user in NIS map \n",
	":1324:ERROR: unknown NIS error \n",
	":1325:WARNING:  unable to chown all files to new uid.\n"
};

int lasterrmsg = sizeof( errmsgs ) / sizeof( char * );
