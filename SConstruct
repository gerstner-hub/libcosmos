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
    Default(env['libs']['libcosmos'])
