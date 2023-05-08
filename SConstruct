import os
import sys
from pathlib import Path

try:
    # if there is already an environment then simply use that, some other
    # level of the build system already initialized it
    Import('env')
except Exception:
    try:
        from buildsystem import initSCons
    except ImportError:
        cosmos_scripts = Path(Dir('.').abspath) / "scripts"
        sys.path.append(str(cosmos_scripts))
        from buildsystem import initSCons
    env = initSCons("libcosmos")

SConscript(env['buildroot'] + 'src/SConstruct')
if env['project'] == "libcosmos":
    SConscript(env['buildroot'] + 'test/SConstruct')
    SConscript(env['buildroot'] + 'doc/SConstruct')
    Default(env['libs']['libcosmos'])

    instroot = env['instroot']

    for root, _, files in os.walk("include"):
        for fil in files:
            src = os.path.join(root, fil)
            parts = root.split(os.path.sep)
            parts.insert(1, "cosmos")
            parts.insert(0, instroot)
            target = os.path.sep.join(parts)
            node = env.Install(target, src)
            env.Alias("install", node)

    node = env.InstallVersionedLib(os.path.join(instroot, "lib"), env["libs"]["libcosmos"])
    env.Alias("install", node)

    # TODO: we could also install a pkg-config libcosmos.pc file that explains
    # some things about libcosmos. But currently there's nothing beyond
    # -lcosmos that is really needed.
