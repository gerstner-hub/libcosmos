#!/usr/bin/python3

import os
import platform
import subprocess
import sys
from pathlib import Path

# this script checks the current code base for regressions.
# it compiles and runs tests in different configurations and stop on first
# error.

root_dir = Path(os.path.realpath(__file__)).parent.parent
while True:
    next_parent = root_dir.parent
    if os.path.exists(next_parent / "SConstruct"):
        root_dir = next_parent
    else:
        break

print("Running in top project directory", root_dir)
os.chdir(root_dir)


def flakePython():
    # performs flake8 check on all Python files
    python_sources = []
    scons_sources = []

    for line in subprocess.check_output(["git", "ls-tree", "--name-only", "-r", "master"]).splitlines():
        line = line.decode()
        if line.endswith(".py"):
            python_sources.append(line)
        elif line.endswith("SConstruct"):
            scons_sources.append(line)

    # 203: whitespace before ':': prevents pretty indenting
    FLAKE_ARGS = ["--max-line-length=300", "--extend-ignore=E203"]

    res = subprocess.run(["flake8"] + FLAKE_ARGS + python_sources)
    if res.returncode != 0:
        return False

    # 821: undefined names in SCons context
    res = subprocess.run(["flake8", "--ignore=F821"] + FLAKE_ARGS + scons_sources)
    return res.returncode == 0


def buildConfig(config, env=None, label=""):
    cmdline = ["scons", "-j5"] + config.split() + ["docs=0", "/"]
    print(f"[{label}] " if label else "", "Running ", ' '.join(cmdline), sep='')
    subprocess.check_call(cmdline, stdout=subprocess.DEVNULL, env=env)


if not flakePython():
    print("There are Python flake8 errors. Stopping check.", file=sys.stderr)
    sys.exit(1)

configurations = (
    "libtype=shared",
    "libtype=static",
    "libtype=shared compiler=clang",
    "libtype=static compiler=clang",
    "sanitizer=1"
)

for config in configurations:
    buildConfig(config)

if platform.uname().machine == 'x86_64':
    env32 = os.environ.copy()
    for var in ("CFLAGS", "CXXFLAGS", "LDFLAGS"):
        env32[var] = "-m32"
    for config in configurations[0:2]:
        buildConfig(config, env32, "32bit")
