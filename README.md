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

FEEDBACK
========
Feel free to fork.  It works as is for me, if you don't like it, 
fix it to suit your needs!

It proved useful to me over the years; hope it is useful to you too...

Thanassis Tsiodras.
