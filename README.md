WHAT IS THIS?
=============

This is a slightly modified version of rsbep0.0.5 which I have
been using to create error-resilient backups of data (able to
survive hard disks' bad sectors, scratched CD/DVD backups, etc)

I've written a full blog post about it [here](https://www.thanassis.space/rsbep.html).

INSTALL
=======

    ./configure
    make
    make install

...will copy binaries and scripts into your /usr/local/bin/
directory. For reference, some additional docs (including this
README) will be copied inside a documentation directory:
/usr/local/share/doc/rsbep/. If you wish to change the prefix
from /usr/local/ to "/path/to/whatever", use...

    ./configure --prefix=/path/to/whatever
    make
    make install

...and remember to add /path/to/whatever/bin to your PATH.
Read INSTALL for more details, if you wish.

DATA RECOVERY
=============

Here's an example of data recovery via the "freeze.sh" and
"melt.sh" scripts:

    home:/var/tmp/recovery$ ls -la
    total 4108
    drwxr-xr-x 2 ttsiod ttsiod    4096 2008-07-30 22:21 .
    drwxrwxrwt 5 root   root      4096 2008-07-30 22:21 ..
    -rw-r--r-- 1 ttsiod ttsiod 4194304 2008-07-30 22:21 data

    home:/var/tmp/recovery$ freeze.sh data > data.shielded
    home:/var/tmp/recovery$ ls -la
    total 9204
    drwxr-xr-x 2 ttsiod ttsiod    4096 2008-07-30 22:21 .
    drwxrwxrwt 5 root   root      4096 2008-07-30 22:21 ..
    -rw-r--r-- 1 ttsiod ttsiod 4194304 2008-07-30 22:21 data
    -rw-r--r-- 1 ttsiod ttsiod 5202000 2008-07-30 22:21 data.shielded

    home:/var/tmp/recovery$ melt.sh data.shielded > data2
    home:/var/tmp/recovery$ md5sum data data2
    9440c7d2ff545de1ff340e7a81a53efb  data
    9440c7d2ff545de1ff340e7a81a53efb  data2

    home:/var/tmp/recovery$ # We just survived a 'round trip'.
    home:/var/tmp/recovery$ # Now let's create artificial corruption
    home:/var/tmp/recovery$ # of 127 times 512 which is 65024 bytes

    home:/var/tmp/recovery$ dd if=/dev/zero of=data.shielded bs=512 count=127 conv=notrunc
    127+0 records in
    127+0 records out
    65024 bytes (65 kB) copied, 0,00026734 seconds, 243 MB/s

    home:/var/tmp/recovery$ melt.sh data.shielded > data3

    rsbep: number of corrected failures   : 64764
    rsbep: number of uncorrectable blocks : 0

    home:/var/tmp/recovery$ md5sum data data2 data3
    9440c7d2ff545de1ff340e7a81a53efb  data
    9440c7d2ff545de1ff340e7a81a53efb  data2
    9440c7d2ff545de1ff340e7a81a53efb  data3

    home:/var/tmp/recovery$ # Look, ma! We recovered...

FUSE-based filesystem using this
================================

These tools allowed an easy implementation of a Reed-Solomon
protected filesystem, using Python FUSE bindings:

    bash$ poorZFS.py -f /reed-solomoned-data /strong

This command will mount a FUSE-based filesystem in "/strong" (using
the "/reed-solomoned-data" directory to store the actual files and
their "shielded" versions). Any file you create in /strong, will
in fact exist under "/reed-solomoned-data", and will also be shielded
there (via "freeze.sh"). When opening for reading any file in /strong,
data corruption is detected (via "melt.sh") and in case of corruption
the file will be corrected using the Reed-Solomon "shielded" version
of the file (which is stored alongside the original, and named as
originalFilename.frozen.RS).

I coded this using Python-FUSE in a couple of hours on a boring Sunday
afternoon, so don't trust your bank account data with it... It's just
a proof of concept (not to mention dog-slow). Still, if your machine
is only equipped with one drive, this will in fact transparently shield
you against any sudden appearance of your drive's bad sectors.

And just note that I coded this filesystem adding 20 or so lines of
Python (spawning my freeze/melt scripts) into the Python FUSE basic
example. Anyone who has ever coded a filesystem driver for Windows knows
why this justifies a heart attack - FUSE (and Python FUSE) rock!

CHANGES from original rsbep
===========================

- The original version wrote 3 parameters of the Reed-Solomon algorithm
  as a single line before the "shielded" data, and this made the stream
  fragile (if this information was lost, decoding failed...)

- It uses a default value of 16x255=4080 for parameter R,
  and it can thus tolerate 4080x16=65280 consecutive bytes
  to be lost anywhere in the stream, and still recover...

- It adds file size information in the shielded stream, so the recovery
  process re-creates an exact copy of the original.

- I added autoconf/automake support, to detect whether a fast 32bit x86
  asm version can be used and otherwise fall back to a plain C (slow)
  implementation. The tools thus compile and install cleanly on many
  operating systems (Linux, Mac OS/X, Free/Net/OpenBSD, even Windows
  with Cygwin).

- Python-FUSE support.

UPDATE, many years later: Streaming support, a hands-on corruption, and dd options
==================================================================================
While answering some questions I received about usage of rsbep for streaming processes,
 I realized I could demonstrate `dd`'s recovery from actual corruption at raw device level,
 via the functionality offered by dmsetup’s `error`. This is a mandatory part that `rsbep`
depends on, i.e. that when errors happen during reading, we still get *some* data,
*any* data for them. We - i.e. the algorithm - can then recover the lost data.

Let's first establish what are the correct options to use to recover via `dd`
while reading from bad devices.  The example below will create a device formed
from two loop devices, with an erroneous zone between them.

First, we create the two loop devices, serializing their data into two 1MB files:

    # mkdir example
    # cd example
    # truncate -s 1M a.img b.img
    # losetup -f a.img
    # losetup -f b.img
    # losetup -a

    /dev/loop1: [65024]:7984102 (/home/ttsiod/tmp/Milan/b.img)
    /dev/loop0: [65024]:7984101 (/home/ttsiod/tmp/Milan/a.img)

Now let's fill up the devices with data:

    # i=0; while printf 'A%06d' $i ; do i=$((i+1)) ; done > /dev/loop0
    -bash: printf: write error: No space left on device

    # i=0; while printf 'B%06d' $i ; do i=$((i+1)) ; done > /dev/loop1
    -bash: printf: write error: No space left on device

This wrote a series of counters in them, one after the other:

    # hexdump -C /dev/loop0 | head -3
    00000000  41 30 30 30 30 30 30 41  30 30 30 30 30 31 41 30  |A000000A000001A0|
    00000010  30 30 30 30 32 41 30 30  30 30 30 33 41 30 30 30  |00002A000003A000|
    00000020  30 30 34 41 30 30 30 30  30 35 41 30 30 30 30 30  |004A000005A00000|

Now let's create the joined-and-errored device:

    # dmsetup create DeviceWithBadSectors << EOF
    0 2000 linear /dev/loop0 0
    2000 96 error
    2096 2000 linear /dev/loop1 48
    EOF

    # blockdev --getsz /dev/mapper/DeviceWithBadSectors
    4096

This new device (`/dev/mapper/DeviceWithBadSectors`) is made of the first
2000 sectors of `/dev/loop0`, followed by 96 bad sectors; and then by the
last 2000 sectors from `/dev/loop1`. This, as we saw above, makes up
for a device with a total of 4096 sectors, 96 of which - in the middle - are
bad.

Now let's try reading the data of this device - first, with `ddrescue`, a tool
specifically made to read from bad devices:

    # ddrescue /dev/mapper/DeviceWithBadSectors recovered
    GNU ddrescue 1.25
    Press Ctrl-C to interrupt
         ipos:    1072 kB, non-trimmed:        0 B,  current rate:   2048 kB/s
         opos:    1072 kB, non-scraped:        0 B,  average rate:   2048 kB/s
    non-tried:        0 B,  bad-sector:    49152 B,    error rate:    139 kB/s
      rescued:    2048 kB,   bad areas:        1,        run time:          0s
    pct rescued:   97.65%, read errors:       98,  remaining time:          0s
                                  time since last successful read:         n/a
    Finished

    # ls -l recovered
    -rw-r--r-- 1 root root 2097152 Oct 10 13:41 recovered

    # blockdev --getsz recovered
    4096

Indeed, we got data for all 4096 sectors - including the 96 bad ones, for which
`ddrescue` will have placed zeroes in the output. This is exactly what `rsbep`
needs to happen for the Reed-Solomon algorithm to function properly; i.e. we
need the data from the bad sectors to be there - bad, but in their place.
We can't afford to miss them!

OK - but `ddrescue` writes into a file. What about streaming operations?
And also, most people won't have it installed - can't we use `dd` for the
same purpose?

Let's establish what is the output data we want - what is the MD5 checksum
of the recovered data?

    # md5sum recovered
    d2ae90b3a830d34c7e92717e8549b909  recovered

Now let's see what happens with `dd` - used with the proper options:

    # dd if=/dev/mapper/DeviceWithBadSectors conv=noerror,sync bs=512 > /dev/null
    ...
    dd: error reading '/dev/mapper/DeviceWithBadSectors': Input/output error
    2000+95 records in
    2095+0 records out
    1072640 bytes (1.1 MB, 1.0 MiB) copied, 0.00982337 s, 109 MB/s
    4000+96 records in
    4096+0 records out
    2097152 bytes (2.1 MB, 2.0 MiB) copied, 0.024313 s, 86.3 MB/s

    # dd if=/dev/mapper/DeviceWithBadSectors conv=noerror,sync bs=512 2>/dev/null | wc -c
    2097152

    # dd if=/dev/mapper/DeviceWithBadSectors conv=noerror,sync bs=512 2>/dev/null | md5sum
    d2ae90b3a830d34c7e92717e8549b909

So we see that `dd` read the same data as `ddrescue` - the MD5 checksum is good,
and we have indeed read 2097152 bytes - i.e. 4096 sectors of 512 bytes each.
This means `rsbep` will be able to perfectly recover, even though we are
streaming the data out.

The "magic" options for `dd`, are, as shown above:

- `noerror`: don't stop on hard drive read error, and
- `sync`: pad every input block with NULs to input block size

So, we now have all the ingredients to use `rsbep` in a streaming scenario...
e.g. sending data over SSH to another machine, or whatever.

Here's a standalone example via tar - first, creating a backup:

    # cd /path/to/backup
    # tar cpf - ./ | xz | \
        rsbep  -B 255 -D 223 -R 4080 > /var/tmp/shielded.data.xz.rsbep

And here's recovering:

    # cd /path/to/restore
    # dd if=/var/tmp/shielded.data.xz.rsbep conv=noerror,sync bs=512 | \
        rsbep -d -B 255 -D 223 -R 4080 | tar Jtvf -

FEEDBACK
========
Feel free to fork this.  It works as is for me, if you don't like it,
fix it to suit your needs!

It proved useful to me over the years; hope it is useful to you too...

Thanassis Tsiodras.
