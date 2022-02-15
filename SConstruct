import os

prefix = os.environ.get("SCONS_CROSS_PREFIX", None)

env_options = {}

if prefix:
    env_options = {
        "CC"    : "{prefix}-gcc",
        "CXX"   : "{prefix}-g++",
        "LD"    : "{prefix}-g++",
        "AR"    : "{prefix}-ar",
        "STRIP" : "{prefix}-strip",
    }

    for key in env_options:
        env_options[key] = env_options[key].format(prefix = prefix)

env = Environment(**env_options)

env.Append(CXXFLAGS = "-std=c++17")

if "CXXFLAGS" in os.environ:
    env.MergeFlags(os.environ["CXXFLAGS"])
env.Append(CCFLAGS = "-g -flto")
# automatically include the export header to create less noise
env.Append(CCFLAGS = "-include include/cosmos/dso_export.h")
env.Append(CCFLAGS = "-Wall -Wextra -Wno-unused-parameter -Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wshadow -Wformat=2 -Wdouble-promotion -Wnull-dereference")
env.Append(CPPPATH = "../../include")
env.Append(LINKFLAGS = "-Wl,--as-needed")
env.VariantDir("build", ".", duplicate=False)

Export("env")

env['libs'] = dict()

SConscript('build/src/SConstruct')
SConscript('build/test/SConstruct')
Default(env['libs']['libcosmos'])
