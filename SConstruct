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
        cosmos_scripts = Path(Dir('.').abspath) / 'scripts'
        sys.path.append(str(cosmos_scripts))
        from buildsystem import initSCons
    env = initSCons('libcosmos')

env['project_root'] = str(Dir('.').srcnode().abspath)

env = SConscript(env['buildroot'] + 'src/SConstruct', exports=['env'])

is_main_project = env['project'] == 'libcosmos'

if is_main_project:
    SConscript(env['buildroot'] + 'test/SConstruct')
    SConscript(env['buildroot'] + 'sample/SConstruct')
    SConscript(env['buildroot'] + 'doc/SConstruct')
    Default(env['libs']['libcosmos'])

instroot = env['instroot']

install_dev_files = env['install_dev_files']

if install_dev_files or env['libtype'] == 'shared':
    node = env.InstallVersionedLib(os.path.join(instroot, env['lib_base_dir']), env['libs']['libcosmos'])
    env.Alias('install', node)

if install_dev_files:
    env.InstallHeaders('cosmos')
