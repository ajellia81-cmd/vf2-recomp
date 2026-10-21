#!/bin/sh
# Read-only preflight for a Mac before generating or archiving. It intentionally does not build.
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
repo=$(CDPATH= cd -- "$root/.." && pwd)
missing=0

need() {
	if [ ! -e "$1" ]; then
		printf 'missing: %s\n' "$2" >&2
		missing=1
	fi
}

command -v xcodegen >/dev/null 2>&1 || { printf 'missing: XcodeGen\n' >&2; missing=1; }
command -v xcodebuild >/dev/null 2>&1 || { printf 'missing: Xcode command-line tools\n' >&2; missing=1; }
need "$root/Dependencies/SDL3.xcframework" "ios/Dependencies/SDL3.xcframework"
need "$root/Dependencies/FreeType.xcframework" "ios/Dependencies/FreeType.xcframework"
need "$repo/src/rec/i960_native_stub.cpp" "interpreter-only native stub"

if [ "$missing" -ne 0 ]; then
	printf '%s\n' 'Preflight failed; no project was generated and no build was run.' >&2
	exit 1
fi

printf '%s\n' 'Preflight passed. Next: set PRODUCT_BUNDLE_IDENTIFIER and DEVELOPMENT_TEAM, then run xcodegen generate.'
