from SCons.Script import *
import os

def gatherSources(self, suffixes, path='.'):
    """Recursively walks through the given path (by default the current
    SConscript directory) and returns relative paths to
    matching sources that end in one of the given suffixes."""
    import bisect

    srcdir = self.Dir('.').srcnode().abspath

    # dynamically determine source list
    sources = []
    for root, _, files in os.walk(srcdir):
        for file in files:
            for suffix in suffixes:
                if file.endswith(suffix):
                    break
            else:
                continue

            relpath = os.path.join(root, file)[len(srcdir)+1:]

            # maintain a sorted list of sources for determinism
            bisect.insort(sources, relpath)

    return sources

def registerLibConfig(self, name, node, flags):
    """This helper takes care of recording the given library in the root
    environment object together with some metadata for other parts of the
    project being able to successfully build and link against it."""
    rootenv = self['rootenv']

    libs = flags.setdefault("LIBS", [])
    libs.append(name)
    flags["LIBPATH"] = [Dir(".").abspath]
    rootenv['libs'][name] = node
    rootenv['libflags'][name] = flags

def configureForLib(self, name):
    """This helper adds flags and other requirements to the environment to
    make it possible to build against the given library name."""

    libflags = self['libflags'][name]
    self.Append(**libflags)

def enhanceEnv(env):
    env.AddMethod(gatherSources, "GatherSources")
    env.AddMethod(registerLibConfig, "RegisterLibConfig")
    env.AddMethod(configureForLib, "ConfigureForLib")

def initSCons(project):
    """Initializes a generic C++ oriented SCons build environment.

    If the SCons environment is already setup then that one is returned.
    Otherwise a new one is created and env['project'] is set to the given
    project name.

    This allows separate projects to work together.
    """

    # some basic cross compilation support using GCC
    prefix = os.environ.get("SCONS_CROSS_PREFIX", None)

    env_options = {}

    if prefix:
        env_options = {
            "CC"    : f"{prefix}-gcc",
            "CXX"   : f"{prefix}-g++",
            "LD"    : f"{prefix}-g++",
            "AR"    : f"{prefix}-ar",
            "STRIP" : f"{prefix}-strip",
        }

    env = Environment(**env_options)
    # this entry can be used to add global entries visible by all other cloned
    # environments
    env['rootenv'] = env
    env['libflags'] = {}
    env['project'] = project
    env.Append(CXXFLAGS = ["-std=c++17"])
    if "CXXFLAGS" in os.environ:
        # add user specified flags
        env.MergeFlags(os.environ["CXXFLAGS"])
    env.Append(CCFLAGS = ["-g", "-flto"])
    env.Append(LINKFLAGS = "-Wl,--as-needed")

    warnings = (
        "all", "extra", "no-unused-parameter", "duplicated-cond",
        "duplicated-branches", "logical-op", "shadow", "format=2",
        "double-promotion", "null-dereference"
    )

    env.Append(CCFLAGS = [f"-W{warning}" for warning in warnings])

    buildroot = Dir("build").srcnode().abspath + "/"

    env.VariantDir(buildroot, ".", duplicate=False)

    env['buildroot'] = buildroot
    env['libs'] = dict()

    enhanceEnv(env)

    Export("env")

    return env
