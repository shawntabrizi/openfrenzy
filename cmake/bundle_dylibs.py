#!/usr/bin/env python3
"""Copies the host's non-system dylibs (Homebrew SDL3, FFmpeg and their own dependencies, found
recursively with otool) into OpenFrenzy.app/Contents/Frameworks, changes every reference to
@rpath/<name>, and copies each Homebrew formula's licence files to
Contents/Resources/licenses/<formula>/. Called by cmake/make_app.cmake before the signature.

Usage: bundle_dylibs.py <exe> <frameworks dir> <licenses dir>
"""
import os
import re
import shutil
import subprocess
import sys

SYSTEM_PREFIXES = ("/usr/lib/", "/System/")
LICENSE_FILE = re.compile(r"^(COPYING|LICENSE|NOTICE|PATENTS|AUTHORS)", re.IGNORECASE)


def run(*args):
    result = subprocess.run(args, capture_output=True, text=True)
    if result.returncode:
        sys.exit(f"bundle_dylibs: {' '.join(args)} failed:\n{result.stdout}{result.stderr}")
    return result.stdout


def load_commands(path):
    """(dependencies, rpaths) of one Mach-O file."""
    deps, rpaths, cmd = [], [], None
    for line in run("otool", "-l", path).splitlines():
        line = line.strip()
        if line.startswith("cmd "):
            cmd = line.split()[1]
        elif line.startswith("name ") and cmd in ("LC_LOAD_DYLIB", "LC_LOAD_WEAK_DYLIB", "LC_REEXPORT_DYLIB"):
            deps.append(line.split()[1])
        elif line.startswith("path ") and cmd == "LC_RPATH":
            rpaths.append(line.split()[1])
    return deps, rpaths


def resolve(dep, loader, rpaths):
    """The file a dependency names, or None for a system library or one already in the bundle."""
    if dep.startswith(SYSTEM_PREFIXES) or dep.startswith("@executable_path"):
        return None
    if dep.startswith("@loader_path/"):
        return os.path.realpath(os.path.join(os.path.dirname(loader), dep[len("@loader_path/"):]))
    if dep.startswith("@rpath/"):
        for rpath in rpaths:
            if rpath.startswith("@"):
                rpath = rpath.replace("@loader_path", os.path.dirname(loader))
                if rpath.startswith("@"):
                    continue
            path = os.path.join(rpath, dep[len("@rpath/"):])
            if os.path.exists(path):
                return os.path.realpath(path)
        return None  # ANGLE: already in Frameworks (make_app.cmake)
    return os.path.realpath(dep)


def formula_dir(real):
    """/opt/homebrew/Cellar/<formula>/<version> of a Homebrew library, else None."""
    match = re.match(r"^(.*/Cellar/[^/]+/[^/]+)/", real)
    return match.group(1) if match else None


def main():
    exe, frameworks, licenses = sys.argv[1:4]
    copied = {}  # basename -> source file
    pending = [exe]
    while pending:
        image = pending.pop()
        deps, rpaths = load_commands(image)
        source = copied.get(os.path.basename(image), image)
        for dep in deps:
            real = resolve(dep, source, rpaths)
            if real is None:
                continue
            name = os.path.basename(dep)
            target = os.path.join(frameworks, name)
            if name not in copied:
                copied[name] = real
                shutil.copyfile(real, target)
                os.chmod(target, 0o755)
                run("install_name_tool", "-id", f"@rpath/{name}", target)
                pending.append(target)
            elif os.path.realpath(copied[name]) != real:
                sys.exit(f"bundle_dylibs: two different files named {name}: {copied[name]}, {real}")
            run("install_name_tool", "-change", dep, f"@rpath/{name}", image)
        # Rpaths outside the bundle go; each copied dylib finds its neighbours with @loader_path.
        for rpath in rpaths:
            if not rpath.startswith("@"):
                run("install_name_tool", "-delete_rpath", rpath, image)
            elif image != exe and rpath != "@loader_path":
                run("install_name_tool", "-delete_rpath", rpath, image)
        if image != exe and "@loader_path" not in rpaths:
            run("install_name_tool", "-add_rpath", "@loader_path", image)

    summary = []
    for name, real in sorted(copied.items()):
        keg = formula_dir(real)
        if keg is None:
            sys.exit(f"bundle_dylibs: {real} is not a Homebrew formula; add its licence by hand")
        formula, version = keg.split("/")[-2:]
        out = os.path.join(licenses, formula)
        os.makedirs(out, exist_ok=True)
        files = [f for f in sorted(os.listdir(keg)) if LICENSE_FILE.match(f)]
        if not files:
            sys.exit(f"bundle_dylibs: no licence file in {keg}")
        for f in files:
            shutil.copyfile(os.path.join(keg, f), os.path.join(out, f))
        summary.append(f"{name}\t{formula} {version}\t{', '.join(files)}")
        print(f"bundled {name} ({formula} {version})")
    notes = []
    if "libavcodec" in " ".join(copied):
        avcodec = next(real for name, real in copied.items() if name.startswith("libavcodec"))
        with open(avcodec, "rb") as handle:
            config = re.search(rb"--prefix=[^\0]*", handle.read())
        config = config.group(0).decode() if config else "(not found)"
        notes.append(f"FFmpeg configuration: {config}")
        if "--enable-gpl" in config:
            licence = "GPL version 3 or later" if "--enable-version3" in config else "GPL version 2 or later"
            notes.append(f"This FFmpeg build is {licence} (--enable-gpl). "
                         "A copy of this app that includes it must meet the GPL terms.")
            print(f"bundle_dylibs: WARNING: the bundled FFmpeg is {licence} (--enable-gpl)")
    with open(os.path.join(licenses, "BUNDLED_LIBRARIES.txt"), "w") as handle:
        handle.write("Contents/Frameworks library\tHomebrew formula\tlicence files in this folder\n")
        handle.write("\n".join(summary) + "\n\n" + "\n".join(notes) + "\n")


main()
