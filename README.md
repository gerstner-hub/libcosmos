Introduction
============

This is *libcosmos*, a library providing a modern C++ API for the Linux
operating system. It is intended for low level systems programming on Linux,
while relying on a strong C++ type model for robustness and expressiveness.

Design Goals
------------

This library aims to be thin and free of any extra dependencies. It offers
native Linux features without losing functionality to abstraction. It is
implemented using modern C++ features with an emphasis on type safety by using
strongly typed classes and enums as well as an exception error system for
simplified error handling down to the system call level. It is supposed to map
the native APIs directly without unnecessarily changing the semantics.

Key Features / What makes it different?
---------------------------------------

- No restriction to the POSIX API or tradeoffs for compatibility with other
  operating systems.
- No legacy compatibility for older Linux versions with less functionality.
  Take advantage of modern Linux kernel features.
- Access to the full native Linux feature set, including lower level system
  calls, using a C++ API.
- Mostly no change of the API behaviour with respect to what you know from man
  pages.
- Modeling of the Linux API and data types using strong C++ types to foster
  safe, correct and expressive programs.
- Use of safe defaults where applicable, like close-on-exec file descriptors.

What's with the name?
---------------------

In ancient Greek _cosmos_ is the counterpart to _chaos_. So it is the order of
things, or the order of existence. I like order in programming.

Building the Library
====================

*libcosmos* currently uses the [SCons](https://www.scons.org) build system. You
can build it simply by invoking `scons`. Settings can be passed to the `scons`
command line in the form of `scons setting=value`. The following custom build
settings are supported:

|          Setting            |                     Description                         |
| --------------------------- | ------------------------------------------------------- |
|   `buildroot=somedir`       | Where to place the build tree. By default a sub-directory like `build` is used. |
|   `instroot=somedir`        | Where to install build artifacts and library files.  By default a sub-directory like `install` is used. |
|   `compiler=my-arch-gcc`    | Use the given compiler which can be a `gcc` based cross compiler or `clang`. By default the system gcc is used. |
|   `use-rpath=<bool>`        | Whether to add an RPATH to linked executables to find shared libraries automatically in the build tree. Good for development, not so good for packaging. |
|   `sanitizer=<bool>`        | Whether to build with address, leak and undefined sanitizers for detecting memory corruption, undefined behaviour or memory leaks during runtime. |
|   `debug=<bool>`            | Whether to build without compiler optimizations for simplified debugging. |
|   `optforsize=<bool>`       | Optimize for size instead for speed. |
|   `libtype=[shared\|static]` | Whether to build a shared or a static library. |
|   `libcosmos-soname=<soname>| Specify a custom SONAME for the shared library. |

You can also specify the `CXXFLAGS` environment variable to add additional
compiler switches to the build process.

Installing the Library
======================

The SCons based build system only supports a simple installation routine that
is invoked by building the `install` target. The `instroot` setting allows to
specify a custom installation directory. By default files are installed in an
`install` sub directory.

Hints for Library Users
=======================

Building against libcosmos
--------------------------

Since *libcosmos* has no additional dependencies you can simply build and link
against it using the `-lcosmos` linker switch, assuming that the *libcosmos*
library is found in the linker's search path. The installation tree also
contains a simple pkg-config configuration file. If installed correctly, or
using the `PKG_CONFIG_PATH` environment variable you can use `pkg-config
--cflags --libs libcosmos` for building against *libcosmos*.

Headers are placed in a `cosmos` sub directory. The path to this directory
needs to be in the compiler's include paths. `#include` directives should
always contain the `cosmos/` prefix component to avoid name clashes with any
other headers.

API documentation
-----------------

*libcosmos* uses Doxygen inline source comments that can either be viewed
as plaintext directly in the headers or can be generated by building `scons
doxygen`, provided you have the Doxygen program installed on your system.

Otherwise you can find the generated HTML version of the API documentation on
the related [GitHub Page](https://gerstner-hub.github.io/libcosmos).

Structure of the libcosmos API
------------------------------

Generally each class type is found in its dedicated header. Groups of free
functions as well a collections of basic types are found in headers starting
with a lower case letter. All classes are placed in the `cosmos` namespace.
For free functions that are grouped together more deeply nested namespaces
like `cosmos::fs` or `cosmos::proc` are used.

In many areas *libcosmos* offers a layer of more primitive types and free
functions that don't add much additional semantics. For example there is the
`FileDescriptor` type that represents an open file descriptor and offers a
couple of file descriptor specific operations on it. This type does not manage
the _ownership_ of the file descriptor, though. It is merely a thin wrapper
object. More complex, composed functionality is built on top of such layers
like the `File` object that supports opening files, streaming file I/O and
also manages the lifetime of its `FileDescriptor` member.

Similarly the header `fs/filesystem.hxx` offers a larger set of free functions
operating on primitive types like `cosmos::fs::unlink_file_at()`. The complex
type `cosmos::fs::Directory` offers the same operation via a member function
operating on the managed directory file descriptor it represents.

It is not an error to use the lower layers, if it justified for your use case
(like integrating with an existing code base, or implementing custom types).
*libcosmos* does not want to force you into a certain way of doing things. It
attempts to provide a typesafe and expressive systems programming
environment, but leaves open routes to do things differently by accessing the
internals.

Complex classes that manage the lifetime of a resource, like pretty much
everything that is based on a `FileDescriptor`, are implemented as move-only
types. This means they cannot be copied but can still be moved using C++ move
semantics.

The library API is roughly divided into different topics represented by
different sub-directories like `fs` for file system related API, `proc` for
process management or `error` for error handling.

Some types and helpers that are not directly related to the Linux API are
offered in top level includes that solve typical problems C++ programmers may
encounter. Like some memory or string operations, for example.

*libcosmos* avoids the C preprocessor and non-typesafe API constants as
far as possible. For each bitmask used on system call level there is a
dedicated `enum class` and `BitMask` type, for example. Similarly even
primitive file descriptors or `errno` values are wrapped in strong types.

Common Pitfalls
---------------

### Valgrind fails with `ApiError: clone3(): Function not implemented` or similar

*libcosmos* currently uses some newer systems calls like the `clone3()` system
call for creating child processes, which aren't yet fully supported neither
by `glibc` nor by `valgrind`. For this reason running *libcosmos* programs in
`valgrind` that use these features will fail. You can still try a build with
address sanitizer to achieve similar runtime error checking as with valgrind.

**Update**: Starting with library version 0.2 there is a transparent fallback
in the `ChildCloner` class to using `fork()` when Valgrind is detected during
runtime. This fallback has a couple of disadvantages in terms of efficiency
and difficult error handling situations, but typically allows to run Valgrind
on program using libcosmos without worrying to much.

The same issue can also happen when working with pidfds (`ProcessFile` class).
There is a transparent fallback in place for `pidfd_send_signal` as well.

State of Development
====================

ABI Stability
-------------

*libcosmos* currently has not concept of ABI stability or versioning. Commonly
providing ABI stability means that all implementation details are hidden from
headers and all object data is dynamically allocated on the heap. I want to
avoid this heap usage. The same can be achieved using abstract stack storage
these days, but it still increases the complexity of the library
implementation. More importantly, it also means that most if not all of the
inline code in headers cannot stay, reducing a lot of the available
optimization potential.

Once *libcosmos* reaches a stable feature set and test coverage, ABI stability
can be given another thought. It could also be provided by carefully deciding
about changes for ABI compatible versions - which is an error prone, human
backed process, of course.

For the moment I recommend using static library linking. Alternatively you can
use unique SONAMEs for each new version of *libcosmos*, which will at least
prevent programs from using incompatible shared library versions of *libcosmos*.
When using `libtype=shared` (the default) then the SCons build system
currently installs a versioned library involving some symlinks. This version
can be changed in `src/SConstruct`.

Starting with library version 0.3.0 the buildsystem now derives an SONAME from
the most recent Git tag. Micro versions are supposed to be compatible with
each other like 0.3.0 and 0.3.1. New minor versions get an incremented SONAME
so any `0.3.*` versions are not compatible with `0.4.*` versions.

Build System
------------

The current choice of SCons for building *libcosmos* is mostly for two reasons:

- autotools based build systems in my experience are a pain for developers if
  things get a bit more complex.
- I'm familiar with SCons. It also has its share of complexities, but since
  it is using Python is can easily be extended for all kinds of requirements
  without too much headache.

The downside of SCons is that is has a smaller user base and offers no
standards for things like installation procedures. I might consider switching
to a Meson based build system in the future, after looking more closely into
it.

Maturity
--------

While I try to add test coverage for all parts of *libcosmos* by way of unit
tests there is only limited real world integration testing of *libcosmos*,
yet. Thus a range of bugs is sure still to be found. At least nothing should
break just from looking at it.

There is also some automated test coverage by way of a GitHub action that
verifies that a couple of combinations build and test successfully. For
verifying yourself you can run the `scripts/check.py` script which builds
shared and static linking configurations, using the native gcc, clang and also
an address sanitizer build.

API History
===========

Since library version 0.2.0 a large chunk of API has been added mainly for the
net subsystem. There is now extensive support for sockets, concrete APIs for
TCP, UDP, Unix domain sockets and various infrastructure that allows to
integrate more socket types in the future. The `sendmsg()` and `recvmsg()` is
also supported and some more common ancillary message types like file
descriptor passing over UNIX domain sockets is implemented.

Furthermore memory mapping support has been added since library version 2.1.

Future Directions
=================

The Linux API is rich and a lot of stuff can still be modeled and added as the
need arises.

Contributing
============

Any bugfixes and improvements are welcome as pull requests. Please refer to
[the coding style](doc/coding_style.md) for a rough style guide. Before
working on larger changes it might be helpful to contact me first to reach
some common ground regarding the design etc.

By contributing you accept the same licensing conditions as the rest of the
library for your contribution. Your name will be added to an authors list in
the repository.
