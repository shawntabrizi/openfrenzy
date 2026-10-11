#!/bin/sh
# Makes tools/diff-venv (git-ignored) with the Python packages of the differential test:
# Unicorn (x86 emulator) and Capstone (x86 disassembler), pinned. Needs network once.
#
#   sh tests/diff/setup_venv.sh
#
# Uses uv when it is installed (its cache goes to tools/.uv-cache), else python3 -m venv.
# Never installs into the system Python.
set -eu

root=$(cd "$(dirname "$0")/../.." && pwd)
venv="$root/tools/diff-venv"
packages="unicorn==2.1.1 capstone==5.0.3"

mkdir -p "$root/tools"
if command -v uv >/dev/null 2>&1; then
	export UV_CACHE_DIR="$root/tools/.uv-cache"
	uv venv -q "$venv"
	uv pip install -q --python "$venv/bin/python" $packages
else
	python3 -m venv "$venv"
	"$venv/bin/python" -m pip install -q $packages
fi
"$venv/bin/python" -I -c "import unicorn, capstone; print('diff-venv: unicorn', unicorn.__version__, 'capstone', capstone.__version__)"
