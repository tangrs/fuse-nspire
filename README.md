FUSE filesystem for TI-Nspire calculators
=========================================

This is a FUSE filesystem for accessing files on a TI-Nspire calculator based on top of [libnspire](https://github.com/tangrs/libnspire) and uses pthreads for locking.

This is still a work in progress!

Support
-------

Although both FUSE and the implementation is (supposed) to be portable, the Makefile included with this currently only supports OSXFUSE. It shouldn't take long to hack the Makefile to get this to compile on a Linux machine.

In the long term, this project will be turned into an autoconf project.

Performance
-----------

Don't expect great performance from this filesystem. This more of a limitation of the USB protocol used to communicate with the TI-Nspire rather than this particular implementation.

Firstly, the TI-Nspire can only process one request at a time. A device lock needs to be held before every action. If you're saving to a large file, it can't do anything else until it's completely saved.

Secondly, the TI-Nspire sends and receives whole files at a time. This means that whole files have to be sent and received for file operations. This implementation grabs the whole file when open()'d and caches it until close()'d or asked to sync. This means read() and write() calls will mostly seem instant.

Many operating systems (OS X in particular) will look around the filesystem and try to open a file for preview purposes. If it happens to stumble upon a very large file, it will load the whole file from the device. This can cause the filesystem to look like it is hanging until the request is completed. For files larger than say 512KB, this could take a very long time (several minutes).

This is mitigated by the filesystem by preventing files larger than a certain size from being opened at all. The FUSE options ```allow_bigfile``` and ```thresh_bigfile=[size]``` can be used to override this behaviour.

Race conditions
---------------

This implementation doesn't (and can't) protect against race conditions. If the contents of a file is changed on the calculator while it is opened on the computer, whatever data is saved later will be the final one.

License
-------

GPLv3
