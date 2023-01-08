#!/bin/sh

MAGIC=MAGIC_TTSIOD_RS
RSBEP='rsbep -B 255 -D 223 -R 4080'

if [ $# -ne 1 ]; then
    echo "Usage: $0 filename (... will output to stdout)"
    exit 1
fi

if [ ! -f "$1" ]; then
    echo "'$1' is not a regular file!"
    exit 1
fi

# stat -c doesn't work under OS/X
# SIZE=`stat -c '%s' "$1"`
SIZE=$(ls -l "$1" | awk '{print $5}')

{
    echo "$MAGIC"
    echo "$SIZE"
    cat "$1"
} | $RSBEP
