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
parser.add_argument('--skip-flake', nargs='*', action='append', help='skip flake8 check on Python source files')
parser.add_argument('--extra-compiler', nargs='*', action='append', help='build configurations with additional custom compilers')

args = parser.parse_args()

BUILDROOT = "build.check"
root_dir = Path(os.path.realpath(__file__)).parent.parent
while True:
    next_parent = root_dir.parent
    if os.path.exists(next_parent / 'SConstruct'):
        root_dir = next_parent
    else:
        break

print('Running in top project directory', root_dir)
os.chdir(root_dir)


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


def buildConfig(config, env=None, label=''):
    try:
        shutil.rmtree(BUILDROOT)
    except FileNotFoundError:
        pass
    cmdline = ['scons', '-j5'] + config.split() + ['docs=0', f'buildroot={BUILDROOT}', '/']
    print(f'[{label}] ' if label else '', 'Running ', ' '.join(cmdline), sep='')
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


def addConfig(config):
    for compiler in compilers:
        this_config = config
        if compiler:
            this_config += f' compiler={compiler}'
        configurations.append(this_config)


addConfig('libtype=shared')

if not args.skip_static:
    addConfig('libtype=static')

configurations.append('sanitizer=1')

for config in configurations:
    buildConfig(config)

if not args.skip_32bit and platform.uname().machine == 'x86_64':
    env32 = os.environ.copy()
    for var in ('CFLAGS', 'CXXFLAGS', 'LDFLAGS'):
        env32[var] = '-m32'
    for config in ('libtype=shared', 'libtype=static'):
        buildConfig(config, env32, '32bit')
