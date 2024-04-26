from SCons.Script import Dir, File, SConscript, ARGUMENTS, Environment, Export
import os
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
            if file.startswith('_'):
                # skip leading underscores, these are headers included based
                # on #ifdef checks
                continue
            for suffix in suffixes:
                if file.endswith(suffix):
                    break
            else:
                continue

            relpath = os.path.join(root, file)[len(srcdir) + 1:]

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

    if self['libtype'] == 'shared':
        libs = flags.setdefault('LIBS', [])
        libs.append(name)
        libdir = Dir('.').abspath
        flags['LIBPATH'] = [libdir]

        if self['use_rpath']:
            flags['RPATH'] = [libdir]

    for env in (rootenv, self):
        env['libs'][name] = node
        env['libflags'][name] = flags
        env['libconfigs'][name] = config


def existsLib(self, name):
    return name in self['libs']


def configureForLib(self, name, sources):
    """This helper adds flags and other requirements to the environment to
    make it possible to build against the given library name."""

    libflags = self['libflags'][name]
    self.Append(**libflags)
    config = self['libconfigs'][name]

    if self['libtype'] == 'static':
        # for static linking, link the archive like an object file
        sources.append(self['libs'][name])

    for pkg in config.get('pkgs', []):
        self.ConfigureForPackage(pkg)


def configureRunForLib(self, name):
    """This helper adjust the runtime environment of the environment so that
    the given library is found for executing e.g. test programs."""
    lib = self['libs'][name]
    full_path = File(lib)[0].abspath
    libdir = str(Path(full_path).parent)

    ld_library_path = self['ENV'].get('LD_LIBRARY_PATH', '')
    paths = ld_library_path.split(':')
    if libdir not in paths:
        paths.append(libdir)
    self['ENV']['LD_LIBRARY_PATH'] = ':'.join(paths)


def configureForLibOrPackage(self, name, sources):
    """Depending on the use-system-pkgs setting either setup the environment
    for linking against a locally built library, or a library installed in the
    system."""

    if self['use_system_pkgs']:
        self.ConfigureForPackage(name)
    else:
        self.ConfigureForLib(name, sources)


def existsPackage(self, seq):
    """This helper checks whether the given package exists according to the
    pkg-config utility."""

    packages = sequencify(seq)

    res = subprocess.call(['pkg-config', '--exists'] + packages)

    return res == 0


def configureForPackage(self, seq):
    """This helper adds flags obtained from the pkg-config utility for the
    given system package."""

    rootenv = self['rootenv']

    for name in sequencify(seq):
        # cache pkg-config results in the environment to avoid multiple
        # pkg-config invocations for the same package
        pkgs = self['pkgs']
        flags = pkgs.get(name, None)

        if not flags:
            flags = subprocess.check_output(['pkg-config', '--cflags', '--libs', name])
            flags = flags.decode('utf8').strip().split()
            rootenv['pkgs'][name] = flags

        self.MergeFlags(flags)


def installHeaders(self, subdir):
    incdir = self.Dir('include').srcnode().abspath
    instroot = self['instroot']
    for root, _, files in os.walk(incdir):
        parts = root.split(os.path.sep)
        while parts.pop(0) != 'include':
            pass

        parts = [instroot, 'include', subdir] + parts
        target = os.path.sep.join(parts)

        for fil in files:
            src = os.path.join(root, fil)
            node = self.Install(target, src)
            self.Alias('install', node)


def addLocalLibrary(self, name):
    if self.ExistsLib(name) or self['use_system_pkgs']:
        # either already registered or we rely on system wide libs for this
        return

    lib_env = self.Clone()
    lib_env['buildroot'] = ''
    SConscript(f'{name}/SConstruct', duplicate=0,
               variant_dir=self['buildroot'] + f'{name}/',
               exports={'env': lib_env})


def enhanceEnv(env):
    env.AddMethod(gatherSources, 'GatherSources')
    env.AddMethod(registerLibConfig, 'RegisterLibConfig')
    env.AddMethod(configureForLib, 'ConfigureForLib')
    env.AddMethod(configureForLibOrPackage, 'ConfigureForLibOrPackage')
    env.AddMethod(configureRunForLib, 'ConfigureRunForLib')
    env.AddMethod(configureForPackage, 'ConfigureForPackage')
    env.AddMethod(existsPackage, 'ExistsPackage')
    env.AddMethod(existsLib, 'ExistsLib')
    env.AddMethod(installHeaders, 'InstallHeaders')
    env.AddMethod(addLocalLibrary, 'AddLocalLibrary')


def initSCons(project, rtti=True, deflibtype='shared'):
    """Initializes a generic C++ oriented SCons build environment.

    If the SCons environment is already setup then that one is returned.
    Otherwise a new one is created and env['project'] is set to the given
    project name.

    This allows separate projects to work together.
    """

    def evalBool(arg):
        return arg.lower() in ['1', 'true', 'yes']

    def getBuildroot():
        # support parallel (default) buildroots for certain different configurations
        if use_clang:
            flavour = 'clang'
        elif gcc_prefix:
            flavour = f'{gcc_prefix}'
        else:
            flavour = None

        defbuildroot = 'build'

        if flavour:
            defbuildroot += f'.{flavour}'
        return ARGUMENTS.get('buildroot', defbuildroot)

    def getInstroot():
        definstroot = 'install'

        ret = ARGUMENTS.get('instroot', definstroot)

        if ret.startswith('/'):
            return ret

        # relative to the src root
        return f'#{ret}'

    def getLibBaseDir():

        ret = 'lib'

        # trying to determine whether the compiler we use uses /lib64 lib dirs
        # or only /lib
        for line in subprocess.check_output([env['CC'], '-print-search-dirs']).splitlines():
            line = line.decode()
            if not line.startswith('libraries:'):
                continue

            if line.find('/lib64/') != -1:
                ret = 'lib64'
                break

        return ret

    compiler = ARGUMENTS.get('compiler', '')
    if compiler.endswith('-gcc') or compiler.endswith('-g++'):
        # some basic cross compilation support using GCC
        gcc_prefix = compiler[:-4]
    else:
        gcc_prefix = None
    use_clang = compiler in ('clang', 'clang++')
    use_gcc = not compiler or gcc_prefix
    clang_like = use_clang or compiler and compiler.find('clang') != -1

    # Whether we should add an rpath entry during linking executables to
    # automatically find shared libraries. This eases running executables
    # directly from the build tree, not so great for an install tree though.
    use_rpath = evalBool(ARGUMENTS.get('use-rpath', '1'))

    env_options = {
        'ENV': {
            # this is needed to get color output support in programs that SCons calls
            'TERM': os.environ['TERM'],
            'PATH': os.environ['PATH'],
            'HOME': os.environ['HOME']
        },
        'tools': ['default']
    }

    if gcc_prefix:
        env_options.update({
            'CC'    : f'{gcc_prefix}-gcc',
            'CXX'   : f'{gcc_prefix}-g++',
            'LD'    : f'{gcc_prefix}-g++',
            'AR'    : f'{gcc_prefix}-ar',
            'STRIP' : f'{gcc_prefix}-strip',
        })
    elif use_clang:
        env_options['tools'].extend(['clang', 'clangxx'])
    elif compiler:
        print(f"Warning: Unrecognized compiler '{compiler}'. Trying my best to continue anyway. Known values are `clang` or `some-arch-gcc`", file=sys.stderr)
        env_options.update({
            'CXX': compiler,
            'CC' : compiler
        })

    env = Environment(**env_options)
    # this entry can be used to add global entries visible by all other cloned
    # environments
    env['rootenv'] = env
    env['libflags'] = {}
    env['project'] = project
    env['use_rpath'] = use_rpath
    env['libtype'] = ARGUMENTS.get('libtype', deflibtype)
    env['compiler'] = 'clang' if use_clang else 'gcc'

    if env['libtype'] not in ('shared', 'static'):
        print(f"Invalid libtype {env['libtype']}: use 'shared' or 'static'", file=sys.stderr)
        sys.exit(1)

    # this switch control whether projects depending e.g. on libcosmos should
    # build a local libcosmos in a Git submodule via SCons or whether the
    # build system should look for these packages globally in the system
    # instead.
    # this is useful for distibution packaging where the necessary
    # dependencies are already installed in the OS.
    env['use_system_pkgs'] = evalBool(ARGUMENTS.get('use-system-pkgs', '0'))
    env['build_docs'] = evalBool(ARGUMENTS.get('docs', '1'))

    env.Append(CXXFLAGS=['-std=c++17'])
    env.Append(CCFLAGS=['-g', '-flto=auto', '-D_FILE_OFFSET_BITS=64', '-fdiagnostics-color=auto'])
    env.Append(LINKFLAGS=['-Wl,--as-needed', '-flto=auto'])

    if ARGUMENTS.get('sanitizer', 0):
        sanitizers = ['address', 'return', 'undefined', 'leak']
        sanitizers = ['-fsanitize={}'.format(f) for f in sanitizers]
        env.Append(CXXFLAGS=sanitizers)
        env.Append(LINKFLAGS=sanitizers)
        env.Append(LIBS=['asan', 'ubsan'])

    if evalBool(ARGUMENTS.get('debug', '0')):
        env.Append(CXXFLAGS=['-O0'])
    elif evalBool(ARGUMENTS.get('optforsize', '0')):
        env.Append(CXXFLAGS=['-Os'])
    else:
        env.Append(CXXFLAGS=['-O2'])

    if evalBool(ARGUMENTS.get('release', '0')):
        env.Append(CCFLAGS='-DNDEBUG')

    if not rtti:
        env.Append(CXXFLAGS=['-fno-rtti'])

    warnings = [
        'all', 'extra', 'shadow', 'format=2',
        'double-promotion', 'null-dereference'
    ]

    if clang_like:
        warnings.extend([
            'no-deprecated-copy'
        ])
    elif use_gcc:
        warnings.extend([
            'duplicated-cond',
            'duplicated-branches',
            'logical-op',
        ])

    env.Append(CCFLAGS=[f'-W{warning}' for warning in warnings])

    if 'CXXFLAGS' in os.environ:
        # add user specified flags
        env.Append(CXXFLAGS=[os.environ['CXXFLAGS']])

    if 'CFLAGS' in os.environ:
        # add user specified flags
        env.Append(CCFLAGS=[os.environ['CFLAGS']])

    if 'LDFLAGS' in os.environ:
        # add user specified linker flags
        env.Append(LINKFLAGS=[os.environ['LDFLAGS']])

    buildroot = Dir(getBuildroot()).srcnode().abspath + '/'

    env.VariantDir(buildroot, '.', duplicate=False)

    env['buildroot'] = buildroot
    env['instroot'] = getInstroot()
    env['lib_base_dir'] = getLibBaseDir()
    env['pkg_config_dir'] = getLibBaseDir() + '/pkgconfig'
    env['libs'] = dict()
    env['bins'] = dict()
    env['pkgs'] = dict()
    env['libconfigs'] = dict()
    # whether development artifacts like headers or static libraries should be
    # installed for the 'install' target. turn this off for pure application
    # projects.
    env['install_dev_files'] = True

    enhanceEnv(env)

    Export('env')

    return env
