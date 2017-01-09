#!/usr/bin/env python

#    Copyright (C) 2001  Jeff Epler  <jepler@unpythonic.dhs.org>
#    Copyright (C) 2006  Csaba Henk  <csaba.henk@creo.hu>
#    Copyright (C) 2009  Thanassis Tsiodras  <ttsiodras at the famous gmail com>
#
#    This program can be distributed under the terms of the GNU LGPL.
#    See the file COPYING.
#
# ttsiodras comments:
#
#    Last summer an article of mine got Slashdotted (http://ttsiodras.googlepages.com/rsbep.html).
#    It described a simple technique that uses Reed-Solomon codes to make files resilient 
#    to data corruption. Using a python-fuse example (xmp.py) as a base, I added the
#    necessary code to have a FUSE-mounted directory automatically have these Reed-Solomon
#    "shields" calculated. In plain words, you can execute...
#
#        ./poorZFS.py -f /reed-solomoned-data /strong
#
#    This command will mount a FUSE-based filesystem in /strong (using the /reed-solomoned-data
#    directory to store the actual files and their "shielded" versions). Any file you create 
#    in /strong, will in fact exist under /reed-solomoned-data, and will also be shielded there. 
#    Upon reading any file in /strong, any data corruption is detected and it will be 
#    corrected using the Reed-Solomon "shielded" version (stored alongside the original file, 
#    and named originalFilename.frozen.RS file).
#
#    I coded this using Python-fuse in a couple of hours on a boring Sunday afternoon, so don't 
#    trust your bank account data with it... It's not supposed to be anything other than a proof 
#    of concept.

import os, sys, signal
from errno import *
from stat import *
import fcntl
from hashlib import md5
from subprocess import Popen, PIPE

import fuse
from fuse import Fuse

if not hasattr(fuse, '__version__'):
    raise RuntimeError, \
        "your fuse-py doesn't know of fuse.__version__, probably it's too old."

fuse.fuse_python_api = (0, 2)

fuse.feature_assert('stateful_files', 'has_init')

def CreateRS(path):
    print "Creating Reed-solomon version of file " + path + " ..."
    os.system("freeze.sh '" + path + "' > '" + path + ".frozen.RS'")
    print "Created."

def CheckCRC(path):
    print "Checking CRC of file " + path
    rsMD5 = md5()
    signal.signal(signal.SIGPIPE, signal.SIG_IGN)
    rsMD5.update(Popen("melt.sh '" + path + ".frozen.RS'", shell=True, stdout=PIPE).stdout.read())
    signal.signal(signal.SIGPIPE, signal.SIG_DFL)
    fileMD5 = md5()
    fileMD5.update(open(path, 'rb').read())
    if fileMD5.hexdigest() != rsMD5.hexdigest():
	print "Data corrupted inside file '" + path + "'... fixing..."
	os.system("melt.sh '" + path + ".frozen.RS' > '" + path + "'")
    else:
	print "CRC is OK"

def flag2mode(flags):
    md = {os.O_RDONLY: 'r', os.O_WRONLY: 'w', os.O_RDWR: 'w+'}
    m = md[flags & (os.O_RDONLY | os.O_WRONLY | os.O_RDWR)]

    if flags | os.O_APPEND:
        m = m.replace('w', 'a', 1)

    return m


class Xmp(Fuse):

    def __init__(self, *args, **kw):

        Fuse.__init__(self, *args, **kw)

        # do stuff to set up your filesystem here, if you want
        #import thread
        #thread.start_new_thread(self.mythread, ())
        self.root = None

#    def mythread(self):
#
#        """
#        The beauty of the FUSE python implementation is that with the python interp
#        running in foreground, you can have threads
#        """
#        print "mythread: started"
#        while 1:
#            time.sleep(120)
#            print "mythread: ticking"

    def getattr(self, path):
        return os.lstat("." + path)

    def readlink(self, path):
        return os.readlink("." + path)

    def readdir(self, path, offset):
        for e in os.listdir("." + path):
	    if not e.endswith(".frozen.RS"):
		yield fuse.Direntry(e)

    def unlink(self, path):
        os.unlink("." + path)
        os.unlink("." + path + ".frozen.RS")

    def rmdir(self, path):
        os.rmdir("." + path)

    def symlink(self, path, path1):
        os.symlink(path, "." + path1)
        os.symlink(path+".frozen.RS", "."+path1+".frozen.RS")

    def rename(self, path, path1):
        os.rename("." + path, "." + path1)
        os.rename("." + path + ".frozen.RS", "." + path1 + ".frozen.RS")

    def link(self, path, path1):
        os.link("." + path, "." + path1)
        os.link("." + path + ".frozen.RS", "." + path1 + ".frozen.RS")

    def chmod(self, path, mode):
        os.chmod("." + path, mode)
	if os.path.exists("." + path + ".frozen.RS"):
	    os.chmod("." + path + ".frozen.RS", mode)

    def chown(self, path, user, group):
        os.chown("." + path, user, group)
	if os.path.exists("." + path + ".frozen.RS"):
	    os.chown("." + path + ".frozen.RS", user, group)

    def truncate(self, path, len):
        f = open("." + path, "a")
        f.truncate(len)
        f.close()
	CreateRS("."+path)

    def mknod(self, path, mode, dev):
        os.mknod("." + path, mode, dev)

    def mkdir(self, path, mode):
        os.mkdir("." + path, mode)

    def utime(self, path, times):
        os.utime("." + path, times)

#    The following utimens method would do the same as the above utime method.
#    We can't make it better though as the Python stdlib doesn't know of
#    subsecond preciseness in acces/modify times.
#  
#    def utimens(self, path, ts_acc, ts_mod):
#      os.utime("." + path, (ts_acc.tv_sec, ts_mod.tv_sec))

    def access(self, path, mode):
        if not os.access("." + path, mode):
            return -EACCES

#    This is how we could add stub extended attribute handlers...
#    (We can't have ones which aptly delegate requests to the underlying fs
#    because Python lacks a standard xattr interface.)
#
#    def getxattr(self, path, name, size):
#        val = name.swapcase() + '@' + path
#        if size == 0:
#            # We are asked for size of the value.
#            return len(val)
#        return val
#
#    def listxattr(self, path, size):
#        # We use the "user" namespace to please XFS utils
#        aa = ["user." + a for a in ("foo", "bar")]
#        if size == 0:
#            # We are asked for size of the attr list, ie. joint size of attrs
#            # plus null separators.
#            return len("".join(aa)) + len(aa)
#        return aa

    def statfs(self):
        """
        Should return an object with statvfs attributes (f_bsize, f_frsize...).
        Eg., the return value of os.statvfs() is such a thing (since py 2.2).
        If you are not reusing an existing statvfs object, start with
        fuse.StatVFS(), and define the attributes.

        To provide usable information (ie., you want sensible df(1)
        output, you are suggested to specify the following attributes:

            - f_bsize - preferred size of file blocks, in bytes
            - f_frsize - fundamental size of file blcoks, in bytes
                [if you have no idea, use the same as blocksize]
            - f_blocks - total number of blocks in the filesystem
            - f_bfree - number of free blocks
            - f_files - total number of file inodes
            - f_ffree - nunber of free file inodes
        """

        return os.statvfs(".")

    def fsinit(self):
        os.chdir(self.root)

    class XmpFile(object):

        def __init__(self, path, flags, *mode):
	    self.mypath = "." + path
	    self.openflags = flag2mode(flags)
	    if ( self.openflags == 'r'):
		CheckCRC(self.mypath)
            self.file = os.fdopen(os.open("." + path, flags, *mode), self.openflags)
            self.fd = self.file.fileno()
	    self.direct_io = False
	    self.keep_cache = False

        def read(self, length, offset):
            self.file.seek(offset)
            return self.file.read(length)

        def write(self, buf, offset):
            self.file.seek(offset)
            self.file.write(buf)
            return len(buf)

        def release(self, flags):
            self.file.close()
	    if ( self.openflags != 'r'):
		CreateRS(self.mypath)

        def _fflush(self):
            if 'w' in self.file.mode or 'a' in self.file.mode:
                self.file.flush()

        def fsync(self, isfsyncfile):
            self._fflush()
            if isfsyncfile and hasattr(os, 'fdatasync'):
                os.fdatasync(self.fd)
            else:
                os.fsync(self.fd)

        def flush(self):
            self._fflush()
            # cf. xmp_flush() in fusexmp_fh.c
            os.close(os.dup(self.fd))

        def fgetattr(self):
            return os.fstat(self.fd)

        def ftruncate(self, len):
            self.file.truncate(len)
	    CreateRS(self.mypath)

        def lock(self, cmd, owner, **kw):
            # The code here is much rather just a demonstration of the locking
            # API than something which actually was seen to be useful.

            # Advisory file locking is pretty messy in Unix, and the Python
            # interface to this doesn't make it better.
            # We can't do fcntl(2)/F_GETLK from Python in a platfrom independent
            # way. The following implementation *might* work under Linux. 
            #
            # if cmd == fcntl.F_GETLK:
            #     import struct
            # 
            #     lockdata = struct.pack('hhQQi', kw['l_type'], os.SEEK_SET,
            #                            kw['l_start'], kw['l_len'], kw['l_pid'])
            #     ld2 = fcntl.fcntl(self.fd, fcntl.F_GETLK, lockdata)
            #     flockfields = ('l_type', 'l_whence', 'l_start', 'l_len', 'l_pid')
            #     uld2 = struct.unpack('hhQQi', ld2)
            #     res = {}
            #     for i in xrange(len(uld2)):
            #          res[flockfields[i]] = uld2[i]
            #  
            #     return fuse.Flock(**res)

            # Convert fcntl-ish lock parameters to Python's weird
            # lockf(3)/flock(2) medley locking API...
            op = { fcntl.F_UNLCK : fcntl.LOCK_UN,
                   fcntl.F_RDLCK : fcntl.LOCK_SH,
                   fcntl.F_WRLCK : fcntl.LOCK_EX }[kw['l_type']]
            if cmd == fcntl.F_GETLK:
                return -EOPNOTSUPP
            elif cmd == fcntl.F_SETLK:
                if op != fcntl.LOCK_UN:
                    op |= fcntl.LOCK_NB
            elif cmd == fcntl.F_SETLKW:
                pass
            else:
                return -EINVAL

            fcntl.lockf(self.fd, op, kw['l_start'], kw['l_len'])


    def main(self, *a, **kw):

        self.file_class = self.XmpFile

        return Fuse.main(self, *a, **kw)


def main():

    usage = """
Userspace nullfs-alike: mirror the filesystem tree from some point on.

""" + Fuse.fusage

    server = Xmp(version="%prog " + fuse.__version__,
                 usage=usage,
                 dash_s_do='setsingle')

    # Disable multithreading: if you want to use it, protect all method of
    # XmlFile class with locks, in order to prevent race conditions
    server.multithreaded = False

    server.parse(values=server, errex=1)

    if len(server.parser.largs) != 2:
	print "Usage:" + sys.argv[0] + " directoryWithShieldData mountDirectory"
	sys.exit(1)

    server.root = server.parser.largs[0]

    print "Shielding directory '"+server.root+"' (mounted in '" + server.parser.largs[1] + "')"

    try:
        if server.fuse_args.mount_expected():
            os.chdir(server.root)
    except OSError:
        print >> sys.stderr, "can't enter root of underlying filesystem"
        sys.exit(1)

    server.main()


if __name__ == '__main__':
    main()
