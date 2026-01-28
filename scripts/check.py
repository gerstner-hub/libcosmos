#!/usr/bin/python3

import os
import platform
import shutil
import subprocess
import sys
import argparse
from pathlib import Path

# this script checks the current code base for regressions.
# it compiles and runs tests in different configurations and stop on first
# error.

parser = argparse.ArgumentParser(description='build and test various compiler configurations')
parser.add_argument('--skip-static', action='store_true', help='don\'t build static linking configurations')
parser.add_argument('--skip-32bit', action='store_true', help='don\'t build x86 32-bit configurations on x86_64')
parser.add_argument('--skip-clang', action='store_true', help='don\'t build clang configurations')
parser.add_argument('--skip-sanitizer', action='store_true', help='don\'t build saniizer=1 configuration')
parser.add_argument('--skip-flake', nargs='*', action='append', help='skip flake8 check on Python source files')
parser.add_argument('--extra-compiler', nargs='*', action='append', help='build configurations with additional custom compilers')
parser.add_argument('--keep-existing-build', action='store_true', help='don\'t delete existing build trees (to speed-up fixing cycles)')

args = parser.parse_args()

BUILDROOT_BASE = "build.check"
root_dir = Path(os.path.realpath(__file__)).parent.parent
while True:
    next_parent = root_dir.parent
    if os.path.exists(next_parent / 'SConstruct'):
        root_dir = next_parent
    else:
        break

print('Running in top project directory', root_dir)
os.chdir(root_dir)

if not args.keep_existing_build:
    try:
        shutil.rmtree(BUILDROOT_BASE)
    except FileNotFoundError:
        pass


def flakePython():
    # performs flake8 check on all Python files
    python_sources = []
    scons_sources = []

    for line in subprocess.check_output(['git', 'ls-tree', '--name-only', '-r', 'HEAD']).splitlines():
        line = line.decode()
        if line.endswith('.py'):
            python_sources.append(line)
        elif line.endswith('SConstruct'):
            scons_sources.append(line)

    # 203: whitespace before ':': prevents pretty indenting
    FLAKE_ARGS = ['--max-line-length=300', '--extend-ignore=E203']

    res = subprocess.run(['flake8'] + FLAKE_ARGS + python_sources)
    if res.returncode != 0:
        return False

    # 821: undefined names in SCons context
    res = subprocess.run(['flake8', '--ignore=F821'] + FLAKE_ARGS + scons_sources)
    return res.returncode == 0


def buildConfig(label, config, env=None):
    buildroot = f'{BUILDROOT_BASE}/{label}'
    cmdline = ['scons', '-j5'] + config.split() + ['docs=0', f'buildroot={buildroot}', '/']
    print(f'[{label.ljust(15)}] Running ', ' '.join(cmdline), sep='')
    subprocess.check_call(cmdline, stdout=subprocess.DEVNULL, env=env)


if not args.skip_flake and not flakePython():
    print('There are Python flake8 errors. Stopping check.', file=sys.stderr)
    sys.exit(1)

compilers = [None]
if not args.skip_clang:
    compilers.append('clang')

if args.extra_compiler:
    compilers.extend(args.extra_compiler)

configurations = []


def addConfig(label, config):
    for compiler in compilers:
        this_config = config
        if compiler:
            this_config += f' compiler={compiler}'
        else:
            compiler = 'default'
        configurations.append((f'{label}-{compiler}', this_config))


addConfig('shared', 'libtype=shared')

if not args.skip_static:
    addConfig('static', 'libtype=static')
if not args.skip_sanitizer:
    configurations.append(('sanitized', 'sanitizer=1'))

for label, config in configurations:
    buildConfig(label, config)

if not args.skip_32bit and platform.uname().machine == 'x86_64':
    env32 = os.environ.copy()
    for var in ('CFLAGS', 'CXXFLAGS', 'LDFLAGS'):
        env32[var] = '-m32'
    for config in ('libtype=shared', 'libtype=static'):
        label = config.split('=')[1]
        buildConfig(f'{label}-32', config, env32)
