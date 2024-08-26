# Hacking on GCLI

This document gives you hints to get started working with on source
code of [gcli](https://herrhotzenplotz.de/gcli/).

Please note that this document only captures the state of the code at
the some points in time and may be out of date. If you feel like this
is the case please submit bug reports or, even better, provide
patches.

## Building GCLI

We use handwritten Makefiles to build GCLI. This has a few advantages:

- Portability across many platforms, even many older ones
- I (Nico) know Makefiles fairly well
- Cross-Compilation can easily be done
- High flexibility
- Few to no dependencies
- Short compilation times

A few caveats are:

- The Makefile must work with at least 3 implementations of Make:
  - BSD Make (bmake)
  - Schily SMake (smake)
  - GNU Make (gmake)

  Some of these make implementations are very buggy (most notably GNU make)
- Getting target dependencies just right is not easy

For that reason I highly suggest testing with all three make implementations.

### General workflow

A hand written shell script called `configure` is run inside a
directory where to place build files.

This script checks the environment for various properties such as:

- The compiler to use
- Compiler options
- Target system properties
- Dependencies and Libraries
- Additional tooling that can be used

The script allows you to configure multiple build directories from
a single source directory (so called "out of tree builds").

The following example shows you how to configure a default build directory:

    $ mkdir build
    $ cd build/
    $ ../configure

Once the configure script has run you can run make to build gcli:

    $ make

To install gcli to the default prefix (`/usr/local`) you can run:

    $ make install

To run the test suite:

    $ make check

Note that running the test suite requires ATF and Kyua. More details
can be found below.

If you wish to change the compiler to be used you can set these in the
environment:

    $ env CC=/usr/local/bin/clang17 ../configure

To build a default release build with optimisations you can run:

    $ ../configure --release

Check the built-in help of the configure script for more details:

    $ ../configure --help

#### Full Debug build

The configure script comes with a `--debug` flag that configures a
directory for a build with no optimisations and full debug info.

I suggest you use it for development purposes:

    $ ../configure --debug

You can proceed as usual with make.

#### Sanitized Builds

TBD

#### Cross-Compilation

gcli supports cross compilation. A cross-compilation setup can be
achieved by setting at least the following environment variables:

- `CC` to the target system (aka. host) compiler
- `CC_FOR_BUILD` to the build system (aka. build) compiler
- `PKG_CONFIG_PATH` to the path where pkgconfig should look for `.pc` files

e.g. to compile from FreeBSD amd64 to a armv7l Linux system:

	$ CC=/opt/armv7-linux-gnueabihf-gcc/bin/armv7-linux-gnueabihf-gcc \
	> CC_FOR_BUILD=cc \
	> PKG_CONFIG_PATH=/opt/armv7-linux-gnueabihf-gcc/root/usr/lib/pkgconfig \
	> ../configure --debug
	Configuring gcli 2.4.0-devel
	Checking for realpath ... realpath
	Checking host compiler ... /opt/armv7-linux-gnueabihf-gcc/bin/armv7-linux-gnueabihf-gcc
	Checking host compiler type ... gcc
	Checking host compiler target ... armv7-linux-gnueabihf
	Checking for cross-compilation setup ... yes
	Checking build compiler type... clang
	Checking build compiler target ... amd64-unknown-freebsd14.1
	Checking for pkg-config ... pkg-config
	Checking for libcurl ... found
	Checking for atf-c ... found
	Checking for libedit ... found
	Checking for kyua ... kyua
	Checking for ccache ... ccache
	Checking for install ... install
	Writing config.h
	Configuration Summary:

	    Build system type: amd64-unknown-freebsd14.1
	     Host system type: armv7-linux-gnueabihf
	         optimise for: debug
	                   CC: /opt/armv7-linux-gnueabihf-gcc/bin/armv7-linux-gnueabihf-gcc
	         CC_FOR_BUILD: cc
	               CFLAGS: 
	     CFLAGS_FOR_BUILD: 
	       LIBCURL_CFLAGS: 
	         LIBCURL_LIBS: -lcurl
	       LIBATFC_CFLAGS: -I/usr/local/include
	         LIBATFC_LIBS: -L/usr/local/lib -latf-c
	 Using libedit:
	       LIBEDIT_CFLAGS: -I/usr/include/editline
	         LIBEDIT_LIBS: -ledit

	Configuration done. You may now run make.

When you now run make the compilers will be chosen appropriately.
The test suite will not work when cross-compiling.

## Tests

The test suite depends on [Kyua](https://github.com/jmmv/kyua) and
[libatf-c](https://github.com/jmmv/atf).

To run the test suite in a configured directory `build` run:

	$ make -C build check

# Code Style

Please use the BSD Style conventions for formatting your code. This means:

- Functions return type and name go on separate lines, no mixed code
  and declarations (except in for loops):

        void
        foo(int bar)
        {
            int x, y, z;

            x = bar;

            for (int i = 0; i < 10; ++i)
                z += i;

            return x;
        }

  This allows to search for the implementation of a function through a
  simple `grep -rn '^foo' .`.

- Use struct tags for structs, do not typedef them

        struct foo {
            int bar;
            char const *baz;
        };

        static void
        foodoo(struct foo const *const bar)
        {
        }

- Indent with tabs, align with spaces

  `»` denotes a TAB character, `.` indicates a whitespace:

        void
        foo(struct foo const *thefoo)
        {
        »   if (thefoo)
        »   »   printf("%s: %d\n"
        »   »   .......thefoo->wat,
        »   »   .......thefoo->count);
        }

- Try to have a max of 80 characters per line

  I know we're not using punchcards anymore, however it makes the code
  way more readable.

- Use C11

  Please don't use C17 or even more modern features. Reason being that
  I want gcli to be portable to older platforms where either no modern
  compilers are available or where we have to rely on old gcc versions
  and/or buggy vendor compilers. This also means that GNU extensions
  are forbidden. If you use the compiler flags I mentioned above
  you should get notified by the compiler.

There is a `.editorconfig` included in the source code that should
automatically provide you with all needed
options. [Editorconfig](https://editorconfig.org/#pre-installed) is a
plugin that is available for almost all notable editors out there. I
highly recommend you use it.

# Adding support for new forges

The starting point for adding forges is
[include/gcli/forges.h](include/gcli/forges.h). This file contains the
dispatch table for fetching data from various kinds of forges.

A pointer to the current dispatch table can be retrieved through a
call to `gcli_forge()`. You may have to adjust the routines called by
it to allow for automagic detection as well as overrides on the
command line for your new forge type. You should likely never call
`gcli_forge()` directly when adding a new forge type as there are
various frontend functions available that will do dispatching for the
caller.

## Parsing JSON

When you need to parse JSON Objects into C structs you likely want to
generate that code. Please see the [templates/](templates/) directory
for examples on how to do that. Currently the [PR Parser for
Github](templates/github/pulls.t) can act as an example for all
features available in the code generator.

The code generator is fully documented in [pgen.org](docs/pgen.org).

## Generating JSON

We not only need to parse JSON often, we also need to generate it
on the fly when submitting data to forge APIs.

For this the `gcli_jsongen_` family of functions exist. Since these
have been introduced quite late in the project their use is not
particularly wide-spread. However this may change in the future.

To use these, take a look at the header
[include/gcli/json_gen.h](include/gcli/json_gen.h) and also the use
in [src/gitlab/merge_requests.c](src/gitlab/merge_requests.c).

# User Frontend Features

The gcli command line tool links against libgcli. Through a context
structure it passes information like the forge type and user
credentials into the library.

All code for the command line frontend tool is found in the
[src/cmd/](src/cmd/) directory.

[src/cmd/gcli.c](src/cmd/gcli.c) is the entry point for the command
line tool. In this file you can find the dispatch table for all
subcommands of gcli.

## Subcommands

Subcommand implementations are found in separate C files in the
`src/cmd` subdirectory.

When parsing command line options please use `getopt_long`. Do not
forget to prefix your getopt string with a `+` as we do multiple calls
to `getopt_long` so it needs to reset some internal state.

## Output formatting

Output is usually formatted as a dictionary or a table. For these
cases gcli provides a few convenience functions and data structures.

The relevant header is [gcli/cmd/table.h](include/gcli/cmd/table.h).

Do not use these functions in the library code. It's only supposed
to be used from the command line tool.

### Tables

You can print tables by defining a list of columns first:

```C
gcli_tblcoldef cols[] = {
    { .... },
    { .... },
};
```

For a complete definition look at the header or uses of that interface
in e.g. [src/cmd/issues.c](src/cmd/issues.c).

You can then start adding rows to your table:

```C
gcli_tbl table = gcli_tbl_begin(cols, ARRAY_SIZE(cols));

for (int i = 0; i < list.whatever_size; ++i) {
    gcli_tbl_add_row(table, ...);
}
```

The variadic arguments you need to provide depends on the columns
defined. Most relevant is the flags and type field. Make sure you get
data type sizes correct.

To dump the table to stdout use the following call:

```C
gcli_tbl_end(table);
```

This will print the table and free all resources acquired by calls to
the tbl routines. You may no reuse the handle returned by
`gcli_tbl_begin()` after this call. Instead, call the begin routine
again to obtain a new handle.

### Dictionaries

The dictionary routines act almost the same way as tables except that
you don't define the columns. Instead you obtain a handle through
`gcli_dict_begin` and add entries to the dictionary by calling
`gcli_dict_add` or one of the specialized functions for
strings. `gcli_dict_add` is the most generic of them all and provides
a printf-like format and variadic argument list. You can dump the
dictionary and free resources through a call to `gcli_dict_end`.
