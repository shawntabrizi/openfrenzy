#!/usr/bin/env python3
"""Copies the host's non-system dylibs (Homebrew SDL3 and its dependencies, the FFmpeg built in
deps/, found recursively with otool) into OpenFrenzy.app/Contents/Frameworks, changes every
reference to @rpath/<name>, and copies the licence files of each Homebrew formula or local
package to Contents/Resources/licenses/<name>/. Called by cmake/make_app.cmake before the
signature.

Usage: bundle_dylibs.py <exe> <frameworks dir> <licenses dir> [--local <prefix>]...

A local package (cmake/Dependencies.cmake) is a prefix with lib/, a PACKAGE file ("<name>
<version>") and a LICENSES/ folder.
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


def local_package(real, prefixes):
    """(name, version, licence folder) of a library in a local package, else None."""
    for prefix in prefixes:
        if real.startswith(os.path.realpath(prefix) + "/"):
            with open(os.path.join(prefix, "PACKAGE")) as handle:
                name, version = handle.read().split()
            return name, version, os.path.join(prefix, "LICENSES")
    return None


def main():
    exe, frameworks, licenses = sys.argv[1:4]
    rest = sys.argv[4:]
    if len(rest) % 2 or any(flag != "--local" for flag in rest[::2]):
        sys.exit("usage: bundle_dylibs.py <exe> <frameworks dir> <licenses dir> [--local <prefix>]...")
    local = rest[1::2]
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
        package = local_package(real, local)
        if package:
            formula, version, folder = package
            files = sorted(os.listdir(folder))
            origin = "built from source"
        else:
            keg = formula_dir(real)
            if keg is None:
                sys.exit(f"bundle_dylibs: {real} is not a Homebrew formula or local package; "
                         "add its licence by hand")
            formula, version = keg.split("/")[-2:]
            folder = keg
            files = [f for f in sorted(os.listdir(keg)) if LICENSE_FILE.match(f)]
            origin = "Homebrew"
        if not files:
            sys.exit(f"bundle_dylibs: no licence file in {folder}")
        out = os.path.join(licenses, formula)
        os.makedirs(out, exist_ok=True)
        for f in files:
            shutil.copyfile(os.path.join(folder, f), os.path.join(out, f))
        summary.append(f"{name}\t{formula} {version} ({origin})\t{', '.join(files)}")
        print(f"bundled {name} ({formula} {version}, {origin})")
    notes = []
    if "libavcodec" in " ".join(copied):
        avcodec = next(real for name, real in copied.items() if name.startswith("libavcodec"))
        with open(avcodec, "rb") as handle:
            config = re.search(rb"--prefix=[^\0]*", handle.read())
        config = config.group(0).decode() if config else "(not found)"
        notes.append(f"FFmpeg configuration: {config}")
        # The app must hold no GPL code: an FFmpeg with GPL, version 3 or nonfree parts stops the build.
        bad = [flag for flag in ("--enable-gpl", "--enable-version3", "--enable-nonfree") if flag in config]
        if bad:
            sys.exit(f"bundle_dylibs: the FFmpeg to bundle is configured with {', '.join(bad)}; "
                     "the app takes only an LGPL FFmpeg (cmake/Dependencies.cmake)")
        notes.append("FFmpeg licence: LGPL version 2.1 or later (no --enable-gpl, --enable-version3 "
                     "or --enable-nonfree). Source, configure line and how to replace the "
                     "libraries: ffmpeg/SOURCE.txt.")
    with open(os.path.join(licenses, "BUNDLED_LIBRARIES.txt"), "w") as handle:
        handle.write("Contents/Frameworks library\tsource\tlicence files in this folder\n")
        handle.write("\n".join(summary) + "\n\n" + "\n".join(notes) + "\n")


main()
