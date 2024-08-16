#ident	"@(#)ktool:common/ktool/kstuff/klookup.awk	1.1"
#ident  "$Header:"

function normalize(root, mach) {
	if (root == "")
		return ""
	if (mach == "")
		mach = "i386at"

	cmd = "sh -ce \"cd " root "/" mach "; /bin/pwd\" 2>&1"
	cmd | getline dirname
	close cmd
	if (dirname ~ /ERROR|does not exist/)
		dirname = ""

	return dirname
}

function check_entry(root, mach, bundle, desc) {
	conf_root = normalize(root, mach)
	if  (conf_root !=  CONF_ROOT)
		return
	if (bundle == "")
		bundle = conf_root "/etc/conf/kstuff.d/bundle.Z"
	if (desc == "")
		desc = root
	print "PRE_BUILT=0"
	print "BUNDLE=" bundle
	print "CONF_ROOT=" conf_root
	print "DESC=\"" desc "\""
	exit
}

function scan_db_file(file) {
	while ((getline input < file) == 1) {
		split(input, field, ":")
		check_entry(field[1], field[2], field[3], field[5])
	}
	close file
}

NR == 1 {
	CONF_ROOT = normalize($1, $2)
	if (CONF_ROOT == "") {
		print "echo '$ROOT/$MACH: No such directory.' >&2"
		print "exit 1"
		exit
	}
}
NR == 2 {
	for (i = 1; i <= NF; ++i)
		scan_db_file($i)
	print "PRE_BUILT=0"
	print "BUNDLE=" CONF_ROOT "/etc/conf/kstuff.d/bundle.Z"
	print "CONF_ROOT=" CONF_ROOT
	print "DESC=\"" CONF_ROOT "\""
}
