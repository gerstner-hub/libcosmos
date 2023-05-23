from SCons.Script import *
import os
import shutil
import subprocess
import sys
from pathlib import Path

def sequencify(arg):
    for _type in (tuple, list):
        if isinstance(arg, _type):
            return arg

    return [arg,]

def gatherSources(self, suffixes, path='.', recursive=True):
    """Recursively walks through the given path (by default the current
    SConscript directory) and returns relative paths to
    matching sources that end in one of the given suffixes.

    If @param recursive is False then only the directory found in @param path
    itself is considered.
    """
    import bisect

    srcdir = self.Dir('.').srcnode().abspath

    # dynamically determine source list
    sources = []
    for root, _, files in os.walk(srcdir):
        for file in files:
            if file.startswith("_"):
                # skip leading underscores, these are headers included based
                # on #ifdef checks
                continue
            for suffix in suffixes:
                if file.endswith(suffix):
                    break
            else:
                continue

            relpath = os.path.join(root, file)[len(srcdir)+1:]

            # maintain a sorted list of sources for determinism
            bisect.insort(sources, relpath)

        if not recursive:
            break

    return sources

def registerLibConfig(self, name, node, flags, config={}):
    """This helper takes care of recording the given library in the root
    environment object together with some metadata for other parts of the
    project being able to successfully build and link against it."""
    rootenv = self['rootenv']

    libs = flags.setdefault("LIBS", [])
    libs.append(name)
    libdir = Dir(".").abspath
    flags["LIBPATH"] = [libdir]
    if self["use_rpath"]:
        flags["RPATH"] = [libdir]
    rootenv['libs'][name] = node
    rootenv['libflags'][name] = flags
    rootenv['libconfigs'][name] = config

def existsLib(self, name):
    return name in self['libs']

def configureForLib(self, name):
    """This helper adds flags and other requirements to the environment to
    make it possible to build against the given library name."""

    libflags = self['libflags'][name]
    self.Append(**libflags)
    config = self['libconfigs'][name]
    for pkg in config.get('pkgs', []):
        self.ConfigureForPackage(pkg)

def configureRunForLib(self, name):
    """This helper adjust the runtime environment of the environment so that
    the given library is found for execuring e.g. test programs."""
    lib = self['libs'][name]
    full_path = File(lib)[0].abspath
    libdir = str(Path(full_path).parent)

    ld_library_path = self['ENV'].get('LD_LIBRARY_PATH', '')
    paths = ld_library_path.split(':')
    if libdir not in paths:
        paths.append(libdir)
    self['ENV']['LD_LIBRARY_PATH'] = ':'.join(paths)

def existsPackage(self, seq):
    """This helper checks whether the given package exists according to the
    pkg-config utility."""

    packages = sequencify(seq)

    res = subprocess.call(["pkg-config", "--exists"] + packages)

    return res == 0

def configureForPackage(self, seq):
    """This helper adds flags obtained from the pkg-config utility for the
    given system package."""

    rootenv = self['rootenv']

    for name in sequencify(seq):
        # cache pkg-config results in the environment to avoid multiple
        # pkg-config invocations for the same package
        pkgs = self['pkgs']
        flags = self['pkgs'].get(name, None)

        if not flags:
            flags = subprocess.check_output(["pkg-config", "--cflags", "--libs", name])
            flags = flags.decode('utf8').strip().split()
            rootenv['pkgs'][name] = flags

        self.MergeFlags(flags)

def enhanceEnv(env):
    env.AddMethod(gatherSources, "GatherSources")
    env.AddMethod(registerLibConfig, "RegisterLibConfig")
    env.AddMethod(configureForLib, "ConfigureForLib")
    env.AddMethod(configureRunForLib, "ConfigureRunForLib")
    env.AddMethod(configureForPackage, "ConfigureForPackage")
    env.AddMethod(existsPackage, "ExistsPackage")
    env.AddMethod(existsLib, "ExistsLib")

def initSCons(project, rtti=True):
    """Initializes a generic C++ oriented SCons build environment.

    If the SCons environment is already setup then that one is returned.
    Otherwise a new one is created and env['project'] is set to the given
    project name.

    This allows separate projects to work together.
    """

    def evalBool(arg):
        return arg.lower() in ["1", "true", "yes"]

    def getBuildroot():
        # support parallel (default) buildroots for certain different configurations
        if use_clang:
            flavour = "clang"
        elif gcc_prefix:
            flavour = f"{gcc_prefix}"
        else:
            flavour = None

        defbuildroot = "build"

        if flavour:
            defbuildroot += f".{flavour}"
        return ARGUMENTS.get("buildroot", defbuildroot)

    def getInstroot():
        definstroot = "install"

        return ARGUMENTS.get("instroot", definstroot)

    def getLibBaseDir():

        ret = "lib"

        # trying to determine whether the compiler we use uses /lib64 lib dirs
        # or only /lib
        for line in subprocess.check_output([env["CC"], "-print-search-dirs"]).splitlines():
            line = line.decode()
            if not line.startswith("libraries:"):
                continue

            if line.find("/lib64/") != -1:
                ret = "lib64"
                break

        return ret

    compiler = ARGUMENTS.get("compiler", "")
    if compiler.endswith("-gcc") or compiler.endswith("-g++"):
        # some basic cross compilation support using GCC
        gcc_prefix = compiler[:-4]
    else:
        gcc_prefix = None
    use_clang = compiler in ("clang", "clang++")

    if compiler and not (gcc_prefix or use_clang):
        print(f"Unrecognized compiler '{compiler}': use `clang` or `some-arch-gcc`", file=sys.stderr)
        sys.exit(1)

    # Whether we should add an rpath entry during linking executables to
    # automatically find shared libraries. This eases running executables
    # directly from the build tree, not so great for an install tree though.
    use_rpath = evalBool(ARGUMENTS.get("use-rpath", "1"))

    env_options = {
            "ENV": {
                # this is needed to get color output support in programs that
                # SCons calls
                "TERM": os.environ["TERM"],
                "PATH": os.environ["PATH"],
                "HOME": os.environ["HOME"]
            },
            "tools": ['default']
    }

    if gcc_prefix:
        env_options.update({
            "CC"    : f"{gcc_prefix}-gcc",
            "CXX"   : f"{gcc_prefix}-g++",
            "LD"    : f"{gcc_prefix}-g++",
            "AR"    : f"{gcc_prefix}-ar",
            "STRIP" : f"{gcc_prefix}-strip",
        })
    elif use_clang:
        env_options['tools'].extend(["clang", "clangxx"])
    # use colorgcc wrapper if available
    elif shutil.which("colorgcc"):
        # TODO: how can we find out where exactly the symlinks are located
        # independently of the distribution?
        env_options.update({
            "CC"    : "/usr/lib/colorgcc/bin/gcc",
            "CXX"   : "/usr/lib/colorgcc/bin/g++"
        })

    env = Environment(**env_options)
    # this entry can be used to add global entries visible by all other cloned
    # environments
    env['rootenv'] = env
    env['libflags'] = {}
    env['project'] = project
    env['use_rpath'] = use_rpath
    env['libtype'] = ARGUMENTS.get("libtype", "shared")

    if env['libtype'] not in ("shared", "static"):
        print(f"Invalid libtype {env['libtype']}: use 'shared' or 'static'", file=sys.stderr)
        sys.exit(1)

    env.Append(CXXFLAGS = ["-std=c++17"])
    if "CXXFLAGS" in os.environ:
        # add user specified flags
        env.MergeFlags(os.environ["CXXFLAGS"])
    env.Append(CCFLAGS = ["-g", "-flto=auto"])
    env.Append(LINKFLAGS = ["-Wl,--as-needed", "-flto=auto"])

    if ARGUMENTS.get('sanitizer', 0):
        sanitizers = ["address", "return", "undefined", "leak"]
        sanitizers = ["-fsanitize={}".format(f) for f in sanitizers]
        env.Append(CXXFLAGS = sanitizers)
        env.Append(LIBS = ["asan", "ubsan"])

    if ARGUMENTS.get('debug', 0):
        env.Append(CXXFLAGS = ["-O0"])
    elif ARGUMENTS.get('optforsize', 0):
        env.Append(CXXFLAGS = ["-Os"])
    else:
        env.Append(CXXFLAGS = ["-O2"])

    if not rtti:
        env.Append(CXXFLAGS = ["-fno-rtti"])

    warnings = [
        "all", "extra", "shadow", "format=2",
        "double-promotion", "null-dereference"
    ]

    if use_clang:
        warnings.extend([
            "no-deprecated-copy"
        ])
    else:
        warnings.extend([
            "duplicated-cond",
            "duplicated-branches",
            "logical-op",
        ])

    env.Append(CCFLAGS = [f"-W{warning}" for warning in warnings])

    buildroot = Dir(getBuildroot()).srcnode().abspath + "/"

    env.VariantDir(buildroot, ".", duplicate=False)

    env['buildroot'] = buildroot
    env['instroot'] = getInstroot()
    env['lib_base_dir'] = getLibBaseDir()
    env['libs'] = dict()
    env['bins'] = dict()
    env['pkgs'] = dict()
    env['libconfigs'] = dict()

    enhanceEnv(env)

    Export("env")

    return env
