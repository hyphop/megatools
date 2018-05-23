megatools - command line client application for Mega [![Build Status](https://travis-ci.com/hyphop/megatools.svg?branch=master)](https://travis-ci.com/hyphop/megatools)
====================================================

Megatools is a collection of programs for accessing Mega service from
a command line of your desktop or server.

Megatools allow you to copy individual files as well as entire directory
trees to and from the cloud. You can also perform streaming downloads for
example to preview videos and audio files, without needing to download
the entire file.

Megatools are robust and optimized for fast operation - as fast as Mega
servers allow. Memory requirements and CPU utilization are kept at minimum.

You can register account using a `megareg` tool, with the benefit of having
true control of your encryption keys.

Mega website can be found at https://mega.nz.

Megatools can be downloaded at https://megatools.megous.com

You can support megatools via Patreon: https://www.patreon.com/megous
or PayPal: ahyechouchahquaachai@xff.cz.


Tools
=====

+  `megash`       one binary exec for everything like BusyBox, [more about megash](README.megash.md)
+  `megareg`      Register and verify a new mega account
+  `megadf`       Show your cloud storage space usage/quota
+  `megals`       List all remote files
+  `megamkdir`    Create remote directory
+  `megarm`       Remove remote file or directory
+  `megaput`      Upload individual files
+  `megaget`      Download individual files
+  `megadl`       Download file from a "public" Mega link (doesn't require login)
+  `megacopy`     Upload or download a directory tree


All of these tools do:

- Local caching of remote session/filesystem information
  for faster execution. Cache is encrypted with your password
  key.
- Support loading login credentials from a configuration file


Usage
=====

See man pages for how to use individual tools and how to configure them:

    man megatools

Man pages are also available online at:

  https://megatools.megous.com/man/megatools.html


Installation on Windows
=======================

Megatools is available on Windows via Chocolatey thanks to ERap320. See:

  https://chocolatey.org/packages/megatools/

You can contact the package maintainer here:

  https://github.com/megous/megatools/issues/347


Windows Quirks
==============

On Windows, if you see weird characters in your megals output, you'll need
to set correct CHARSET environment variable. For example on Czech Windows
this would mean executing this command in cmd before using the tools:

    set CHARSET=CP852

This is just a cosmetic issue. Internally, megatools always work with UTF-8
file names, and even if the tool's terminal output is corrupted, files names
of downloaded/uploaded files will be correct.


Installation on Mac OS X (Mountain Lion)
========================================

Thanks to Carl Moden, megatools is available in Homebrew (http://brew.sh/).

You can therefore install megatools with:

    brew install megatools


Installation on your favorite GNU/Linux distribution
====================================================

Megatools may already be pre-packaged in the package repository
of your distribution. It is already available at least in:

- Debian - https://packages.debian.org/stretch/megatools
- Ubuntu - https://launchpad.net/ubuntu/+source/megatools
- Fedora - https://admin.fedoraproject.org/pkgdb/package/rpms/megatools/
- Arch Linux (AUR) - https://aur.archlinux.org/packages/megatools/
- Gentoo - https://packages.gentoo.org/packages/net-misc/megatools

Be sure to check your distribution's package repository first.


Installation on FreeBSD
=======================

Megatools is available in ports thanks to Maxim V. Kostikov:

  http://www.freshports.org/net/megatools/


Building megatools from source code
===================================

You will need to install dependnencies first, to be able to do the build from
the official source code tarball, which is available only at
http://megatools.megous.com.

You can also check that the code was released by me, using PGP signatures
provided alongside the code.

Please don't report build issues against code that you downloaded from github.
These reports will be closed without explanation, with reference to this README
file. It is not expected to work on all distributions. Use the official tarball.


On Debian, Ubuntu:

    apt-get -y install build-essential libglib2.0-dev libssl-dev libcurl4-openssl-dev

Note: when you run into missing .so files, try building with 
`./configure --disable-shared --enable-static` instead of plain `./configure`.

On Fedora:

    yum -y install gcc make glib2-devel libcurl-devel openssl-devel gmp-devel tar

On OpenSUSE:

    zypper -n install gcc make glib2-devel libcurl-devel openssl-devel gmp-devel

On Arch Linux:

    pacman -Sy --noconfirm --needed pkg-config gcc make glib2 curl gmp nettle


Author
======

Megatools were written by Ondřej Jirman <megous@megous.com>, 2014-2018

My PGP key can be found at: https://megous.com/pgp
  (Fingerprint is: 9AB138B20691621CD4CF92026E6426C677CFEFF1)

Official website is: https://megatools.megous.com

If you'd like to donate, you can use:

PayPal: ahyechouchahquaachai@xff.cz
Patreon: https://www.patreon.com/megous


Contributors
============

- Chris Tarazi <tarazichris@gmail.com>
- Tom Maneiro <tomman@tsdx.net.ve>
- bAndie91 <bandie9100@gmail.com>
- Alberto Garcia <berto@igalia.com>
- David Guillen Fandos <david@davidgf.net>
- Erik Nordstrøm <erik@nordstroem.no>
- Johnathan Jenkins <john@nixheads.co.uk>
- Kagami Hiiragi <kagami@genshiken.org>
- Matthew Schultz <mattsch@gmail.com>
- Michael Ledin <mledin89@gmail.com>
- Michael Ripley <zkxs00@gmail.com>
- Palmer Dabbelt <palmer@dabbelt.com>
- RealDolos <dolos@cock.li>
- Viktor (Icon) VAD <vad.viktor@gmail.com>
- cyrozap <cyrozap@gmail.com>
- nyuszika7h <nyuszika7h@gmail.com>
- protomouse <root@protomou.se>
- strupo <sheeit@users.noreply.github.com>
- wdlkmpx <wdlkmpx@gmail.com>


Support and bug reports
=======================

If you think you've found bug in megatools, send a report including enough
information for recreating the issue to: megous@megous.com

IMPORTANT: I use personal e-mail server for my e-mail communication. I respond
to most e-mails, so if you are not getting a response within a few days, and
you're a user of a more aggressive e-mail provider (Outlook, iCloud, ...) it may
be because your provider rejected my e-mail as SPAM and did not deliver it at
all (not even to your SPAM folder), despite it being response to your e-mail and
my server using industry's best practices. You can either contact your e-mail
provider so that they fix their issue with rejecting legitimate responses to
your mail, or use different e-mail provider or contact channel to contact me.
(github issues) 


License
=======

Megatools are licensed under GPLv2 with OpenSSL exemption, see LICENSE
file for details.

This product includes software developed by the OpenSSL Project for use
in the OpenSSL Toolkit. (http://www.openssl.org/)
