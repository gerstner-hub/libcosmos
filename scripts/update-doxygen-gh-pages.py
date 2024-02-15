#!/usr/bin/python3

from argparse import ArgumentParser
import os
import shutil
import subprocess
import tempfile

GH_PAGES_BRANCH = "gh-pages"
GH_PAGES_BRANCH_NEW = f"{GH_PAGES_BRANCH}.new"

parser = ArgumentParser(description=f"builds doxygen apidoc HTML and publishes it on the {GH_PAGES_BRANCH} branch")

parser.add_argument("REPOPATH", help="path to the repository where to build and publish doxygen apidoc")

args = parser.parse_args()

os.chdir(args.REPOPATH)
DOXYGEN_HTML_OUT = "build/doc/doxygen/html"
shutil.rmtree(DOXYGEN_HTML_OUT, ignore_errors=True)
print("Building doxygen")
print("-" * 80)
subprocess.check_call(["scons", "doxygen"], stdout=subprocess.DEVNULL)
print("-" * 80, "\n")

with tempfile.TemporaryDirectory() as worktree:
    subprocess.check_call(["git", "worktree", "add", worktree, GH_PAGES_BRANCH])
    try:
        for entry in os.listdir(worktree):
            if entry.startswith("google") and entry.endswith(".html"):
                print("Copying google cookie", entry)
                shutil.copy(f"{worktree}/{entry}", f"{DOXYGEN_HTML_OUT}/{entry}")

        subprocess.check_call(["git", "switch", "--orphan", GH_PAGES_BRANCH_NEW], cwd=worktree)

        print("Copying Doxygen HTML into pristine branch")
        shutil.copytree(DOXYGEN_HTML_OUT, worktree, dirs_exist_ok=True)

        print("Adding and commiting new Doxygen HTML")
        subprocess.check_call(["git", "add", "."], cwd=worktree)
        subprocess.check_call(["git", "commit", "-m", "automatically generated Doxygen HTML apidoc", "."], cwd=worktree)

        print(f"Replacing old {GH_PAGES_BRANCH} by {GH_PAGES_BRANCH_NEW}")
        subprocess.check_call(["git", "branch", "-D", GH_PAGES_BRANCH], cwd=worktree)
        subprocess.check_call(["git", "branch", "-m", GH_PAGES_BRANCH_NEW, GH_PAGES_BRANCH], cwd=worktree)

        print(f"Run 'git push --force {GH_PAGES_BRANCH}' to publish")
    except Exception:
        subprocess.check_call(["git", "worktree", "remove", worktree])
        raise
