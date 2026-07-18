#!/usr/bin/env python3
"""Validate release metadata and repository contracts without external packages."""

from __future__ import annotations

import json
import os
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(relative_path: str) -> str:
    return (ROOT / relative_path).read_text(encoding="utf-8")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def capture(pattern: str, text: str, label: str, flags: int = 0) -> str:
    match = re.search(pattern, text, flags)
    require(match is not None, f"Could not find {label}")
    return match.group(1)


def locale_keys(relative_path: str) -> set[str]:
    keys: set[str] = set()
    for raw_line in read(relative_path).splitlines():
        line = raw_line.strip()
        if not line or line.startswith(("#", ";")):
            continue
        require("=" in line, f"Malformed locale line in {relative_path}: {raw_line!r}")
        key, _ = line.split("=", 1)
        key = key.strip()
        require(key not in keys, f"Duplicate locale key {key!r} in {relative_path}")
        keys.add(key)
    return keys


def main() -> None:
    buildspec = json.loads(read("buildspec.json"))
    manifest = json.loads(read("docs/site.webmanifest"))
    version = str(buildspec["version"])
    require(re.fullmatch(r"\d+\.\d+\.\d+", version) is not None, f"Invalid semantic version: {version}")

    cmake_version = capture(
        r"project\(arvisual\s+VERSION\s+([0-9.]+)\s+LANGUAGES",
        read("CMakeLists.txt"),
        "standalone CMake version",
    )
    installer_version = capture(
        r'#define\s+AppVersion\s+"([0-9.]+)"',
        read("packaging/windows/arvisual.iss"),
        "Inno Setup fallback version",
    )
    package_version = capture(
        r'\[string\]\s+\$Version\s*=\s*"([0-9.]+)"',
        read("scripts/package-windows.ps1"),
        "Windows package fallback version",
    )
    docs_version = capture(
        r'"softwareVersion"\s*:\s*"([0-9.]+)"',
        read("docs/index.html"),
        "landing-page structured-data version",
    )
    docs_fallback_version = capture(
        r'data-release-version>v([0-9.]+)<',
        read("docs/index.html"),
        "landing-page release fallback version",
    )

    version_locations = {
        "buildspec.json": version,
        "CMakeLists.txt": cmake_version,
        "packaging/windows/arvisual.iss": installer_version,
        "scripts/package-windows.ps1": package_version,
        "docs/index.html structured data": docs_version,
        "docs/index.html release fallback": docs_fallback_version,
    }
    mismatches = {name: value for name, value in version_locations.items() if value != version}
    require(not mismatches, f"Version drift detected; expected {version}: {mismatches}")

    require(manifest.get("name") == "ArVisual Smart Color Enhancer for OBS", "Unexpected web-manifest name")
    require(buildspec.get("website") == "https://masarray.github.io/arvisual-obs/", "Unexpected project website")

    english_keys = locale_keys("data/locale/en-US.ini")
    indonesian_keys = locale_keys("data/locale/id-ID.ini")
    require(english_keys == indonesian_keys, f"Locale key mismatch: en-only={english_keys - indonesian_keys}, id-only={indonesian_keys - english_keys}")

    installer = read("packaging/windows/arvisual.iss")
    for marker in (
        "DirExistsWarning=no",
        "UsePreviousAppDir=yes",
        "GetSHA256OfFile",
        "bin\\64bit\\obs64.exe",
        "data\\obs-plugins\\arvisual\\locale\\id-ID.ini",
    ):
        require(marker in installer, f"Installer contract is missing {marker!r}")

    release_workflow = read(".github/workflows/release.yml")
    for marker in (
        "Validate release tag and metadata",
        "scripts/validate-repository.py",
        "scripts/validate-color-safety.py",
        "id-ID.ini",
        "$LASTEXITCODE",
        "if-no-files-found: error",
        "SHA256SUMS.txt",
        "actions/attest@v4",
    ):
        require(marker in release_workflow, f"Release workflow is missing {marker!r}")

    quality_workflow = read(".github/workflows/quality.yml")
    for marker in ("scripts/validate-repository.py", "scripts/validate-color-safety.py"):
        require(marker in quality_workflow, f"Quality workflow is missing {marker!r}")

    ref_type = os.environ.get("GITHUB_REF_TYPE", "")
    ref_name = os.environ.get("GITHUB_REF_NAME", "")
    if ref_type == "tag":
        require(ref_name == f"v{version}", f"Tag {ref_name!r} does not match buildspec version v{version}")

    print(f"ArVisual repository validation: PASS (v{version})")
    print(f"Locale keys checked: {len(english_keys)}")
    print("Installer and release workflow contracts checked")


if __name__ == "__main__":
    main()
