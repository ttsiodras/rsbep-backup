#!/bin/sh

if [ $# -ne 1 ]; then
    echo "Usage: $0 filename (... will output to stdout)"
    exit 1
fi

if [ ! -f "$1" ]; then
    echo "'$1' is not a regular file!"
    exit 1
fi

fsize() {
    # stat -c doesn't work under OS/X
    # SIZE=`stat -c '%s' "$1"`

    set -- $(LC_ALL=C ls -ln "$1")
    echo "$5"
}

{
    echo MAGIC_TTSIOD_RS
    fsize "$1"
    cat "$1"
} | rsbep -B 255 -D 223 -R 4080
