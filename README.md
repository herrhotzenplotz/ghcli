# GCLI

Portable CLI tool for interacting with Git(Hub|Lab|Tea) from the command line.

![](docs/screenshot.png)

## Why?

The official GitHub CLI tool only supports GitHub. I wanted a simple
unified tool for various git forges such as GitHub and GitLab because
every forge does things differently yet all build on Git and
purposefully break with its philosophy.

Also, the official tool from Github is written in Go, which does manual
[DNS resolution](https://github.com/golang/go/blob/master/src/net/dnsclient_unix.go#L49)
which is a massive security vulnerability for people using Tor as it
leaks your IP to the DNS server. This program builds upon libcurl,
which obeys the operating system's DNS resolution mechanisms and thus
also works with Tor.

## Building

### Download

Recent tarballs can be downloaded here:

[https://herrhotzenplotz.de/gcli/releases/](https://herrhotzenplotz.de/gcli/releases/)

Please note that the tarballs provided by Github and Gitlab will not
work without you having autoconf and automake installed. So please
consider using the link above.

There are official packages available:

- [FreeBSD](https://freshports.org/devel/gcli)
- [Debian Testing](https://packages.debian.org/trixie/gcli)
- [ArchLinux AUR](https://aur.archlinux.org/packages/gcli)
- [NixPkgs Unstable](https://search.nixos.org/packages?channel=unstable&show=gcli&from=0&size=50&sort=relevance&type=packages&query=gcli)
- [NetBSD pkgsrc](https://pkgsrc.se/devel/gcli)
- [MacPorts](https://ports.macports.org/port/gcli/)

### Dependencies

Required dependencies:
- libcurl
- yacc (System V yacc, Berkeley Yacc or Bison should suffice)
- lex (flex is preferred)
- C99 Compiler and linker
- make

Optional dependencies:
- pkg-config

If you are building from Git you will also need:
- m4
- autoconf
- automake

The test suite requires:
- [Kyua](https://github.com/jmmv/kyua)
- [ATF](https://github.com/jmmv/atf)

### Compile
In order to perform a build, do:
```console
$ ./configure [--prefix=/usr/local]
$ make
# make [DESTDIR=/] install
```

You may leave out `DESTDIR` and `--prefix=`. The above is the default
value. The final installation destination is `$DESTDIR/$PREFIX/...`.

If you are unsure, consult the builtin configure help by running
`./configure --help`.

Also, if you are building from Git you need to generate the configure script
first:
```console
$ ./autogen.sh
```

For more details also see [HACKING.md](HACKING.md).

In case any of this does not work, please either report a bug, or
submit a patch in case you managed to fix it.

Tested Operating Systems so far:
- FreeBSD 13.0-RELEASE amd64 and arm64
- Solaris 10 and 11, sparc64
- SunOS 5.11 i86pc (OmniOS)
- Devuan GNU/Linux Chimaera x86_64
- Debian GNU/Linux ppc64, ppc64le
- Gentoo Linux sparc64, ia64
- Fedora 34 x86_64
- Haiku x86_64
- Minix 3.4.0 (GENERIC) i386
- OpenBSD 7.0 GENERIC amd64
- Alpine Linux 3.16 x86_64
- Darwin 22.2.0 arm64
- Windows 10 (MSYS2 mingw32-w64)
- NetBSD 9.3 amd64, sparc64 and VAX

Tested Compilers so far:
- LLVM Clang (various versions)
- GCC (various versions)
- Oracle DeveloperStudio 12.6
- IBM XL C/C++ V16.1.1 (Community Edition)

## Support

Please refer to the manual pages that come with gcli. You may want
to start at `gcli(1)`. For further questions refer to the issues
on Github and Gitlab or ask on IRC at #gcli on
[Libera.Chat](https://libera.chat/). Alternatively you may also use
the mailing list at
[https://lists.sr.ht/~herrhotzenplotz/gcli-discuss](https://lists.sr.ht/~herrhotzenplotz/gcli-discuss).

## Bugs and contributions

Please report bugs, issues and questions to
[~herrhotzenplotz/gcli-discuss@lists.sr.ht](mailto:~herrhotzenplotz/gcli-discuss@lists.sr.ht)
or on [GitLab](https://gitlab.com/herrhotzenplotz/gcli).

You can also submit patches using git-send-email or Mercurial
patchbomb to
[~herrhotzenplotz/gcli-devel@lists.sr.ht](mailto:~herrhotzenplotz/gcli-devl@lists.sr.ht).

## License

BSD-2 CLAUSE (aka. FreeBSD License). Please see the LICENSE file
attached.

## Credits

This program makes heavy use of both [libcurl](https://curl.haxx.se/)
and [pdjson](https://github.com/skeeto/pdjson).

herrhotzenplotz aka. Nico Sonack
October 2021
