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

env = SConscript(env['buildroot'] + 'src/SConstruct')

is_main_project = env['project'] == "libcosmos"

if is_main_project:
    SConscript(env['buildroot'] + 'test/SConstruct')
    SConscript(env['buildroot'] + 'doc/SConstruct')
    Default(env['libs']['libcosmos'])

instroot = env['instroot']

install_dev_files = env['install_dev_files']

if install_dev_files or env['libtype'] == "shared":
    node = env.InstallVersionedLib(os.path.join(instroot, env['lib_base_dir']), env["libs"]["libcosmos"])
    env.Alias("install", node)

if install_dev_files:
    node = env.Install(Path(instroot) / env['pkg_config_dir'], "data/libcosmos.pc")
    env.Alias("install", node)

    env.InstallHeaders("cosmos")
