#!/usr/bin/python3

import os
import subprocess
import sys
from pathlib import Path

# this script checks the current code base for regressions.
# it compiles and runs tests in different configurations and stop on first
# error.

root_dir = Path(os.path.realpath(__file__)).parent.parent
os.chdir(root_dir)

configurations = (
    "libtype=static",
    "libtype=shared",
    "libtype=static compiler=clang",
    "libtype=shared compiler=clang",
    "sanitizer=1"
)

for config in configurations:
    cmdline = ["scons", "-j5"] + config.split() + ["run_tests", "samples"]
    print("Running", ' '.join(cmdline))
    subprocess.check_call(cmdline, stdout=subprocess.DEVNULL)
