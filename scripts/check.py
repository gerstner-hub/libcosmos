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
os.chdir(root_dir)

configurations = (
    "libtype=shared",
    "libtype=static",
    "libtype=shared compiler=clang",
    "libtype=static compiler=clang",
    "sanitizer=1"
)

def buildConfig(config, env=None, label=""):
    cmdline = ["scons", "-j5"] + config.split() + ["run_tests", "samples"]
    print(f"[{label}] " if label else "", "Running ", ' '.join(cmdline), sep='')
    subprocess.check_call(cmdline, stdout=subprocess.DEVNULL, env=env)

for config in configurations:
    buildConfig(config)

if platform.uname().machine == 'x86_64':
    env32 = os.environ.copy()
    for var in ("CFLAGS", "CXXFLAGS", "LDFLAGS"):
        env32[var] = "-m32"
    for config in configurations[0:2]:
        buildConfig(config, env32, "32bit")

