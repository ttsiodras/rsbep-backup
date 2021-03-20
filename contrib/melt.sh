#!/bin/sh
if [ $# -ne 1 ] ; then
	echo Usage: $0 filename   \(...will output to stdout\)
	exit 1
fi
if [ ! -f "$1" ] ; then echo $1 is not a regular file\! ; exit 1 ; fi

dd if="$1" conv=noerror,sync 2>/dev/null | rsbep -d -B 255 -D 223 -R 4080 | {
	read SIG
	if [ "$SIG" != "MAGIC_TTSIOD_RS" ] ; then
		echo "Beyond capacity to correct... Sorry." > /dev/stderr
		exit 1
	fi
	read SIZE
	rsbep_chopper $SIZE
}
