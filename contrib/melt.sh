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

dd if="$1" conv=noerror,sync 2>/dev/null | $RSBEP -d | {
    read -r SIG

    if [ "$SIG" != "$MAGIC" ]; then
        echo 'Beyond capacity to correct... Sorry.' >&2
        exit 1
    fi

    read -r SIZE
    rsbep_chopper "$SIZE"
}
