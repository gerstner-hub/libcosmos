#!/usr/bin/python3

import argparse
import os
import subprocess
import sys
import tempfile
from pathlib import Path


parser = argparse.ArgumentParser(
        description="This script creates a release tarball from a complete project including sub-modules and places fixed version numbers and SONAME information inside for the SCons build system to use.",
)

parser.add_argument("--outdir", required=True, help="Directory where to create the output tarball")
parser.add_argument("--root", required=True, help="Root of the project to create a release tarball for")
parser.add_argument("--include-submodules", action='store_true', help="Also add submodules to the archive")
args = parser.parse_args()

os.chdir(args.root)
project = os.path.basename(os.getcwd())
version = subprocess.check_output("git describe --abbrev=0 --tags".split()).decode().strip()
prjbase = f"{project}-{version}"
tarball = Path(args.outdir) / f"{prjbase}.tar"

if os.path.exists(tarball):
    print(tarball, "already exists. Refusing to overwrite")
    sys.exit(1)

submodules = []

if args.include_submodules:
    # git config below exits with status code 1 if no matching key is found,
    # thus it is a bit hard to differentiate between other errors and "just no
    # submodules present". Thus first check whether any submodules are
    # configured at all
    substatus = subprocess.check_output(["git", "submodule"])
    have_submodules = len(substatus) != 0

    if have_submodules:
        # find out the sub-paths for all sub-modules
        lines = subprocess.check_output(
                "git config --local --name-only --get-regexp submodule.*url".split()).decode().splitlines()
        for line in lines:
            parts = line.split('.')
            if len(parts) != 3:
                continue
            submodules.append(parts[1])

print("Creating main archive")
subprocess.check_call(["git", "archive", "--prefix", f"{prjbase}/", "-o", tarball, "HEAD"])

for module in submodules:
    module_root = f"{args.root}/{module}"
    print("Adding", module, "archive")
    with tempfile.NamedTemporaryFile() as tf:
        subprocess.check_call(
                ["git", "archive", "--prefix", f"{prjbase}/{module}/", "-o", tf.name, "HEAD"],
                cwd=module_root)
        subprocess.check_call(["tar", "-A", "-f", tarball.absolute(), tf.name])

subprocess.check_call(["xz", tarball])
print("You can find the archive at", f"{tarball}.xz")
