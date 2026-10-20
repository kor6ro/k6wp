#!/usr/bin/env python3
"""Regenerate repomix.md - a single-file, AI-friendly packing of the repository.

Usage (from the repo root):

    python tools/gen_repomix.py

The output is written to repomix.md in the repo root. The scope mirrors the
previous hand-made repomix.md: first-party source, docs, tests, packaging and
the Remotion promo-video UI are included; build trees, agent state and
downloaded vendor binaries are not.
"""

from __future__ import annotations

import os
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUTPUT = ROOT / "repomix.md"

# Directories (relative to the repo root) whose contents are packed.
INCLUDE_DIRS = (
    "LICENSES",
    "brand",
    "compressor",
    "docs",
    "engine",
    "launcher",
    "packaging",
    "shadow-evidence",
    "shared",
    "studio",
    "tests",
    "tools",
    "vendor",
    "video",
)

# Directory names skipped anywhere in the tree.
EXCLUDE_DIR_NAMES = {
    ".git",
    ".github",
    ".omo",
    ".codegraph",
    ".opencode",
    ".kilo",
    ".ruff_cache",
    ".qt",
    ".vscode",
    "build",
    "cmake",
    "dist",
    "node_modules",
    "Testing",
    "attic",
}

# Path prefixes (relative to the repo root) skipped entirely.
EXCLUDE_PATH_PREFIXES = (
    "shared/thirdparty",
    "vendor/ffmpeg",
    "vendor/libmpv/bin",
    "vendor/libmpv/include",
)

# Individual file names skipped (repo-root scratch files).
EXCLUDE_FILE_NAMES = {
    "CMakeUserPresets.json",
    "aqtinstall.log",
    "planning.md",
    "repomix.md",
}

# File suffixes skipped (compiler/linker scratch output).
EXCLUDE_SUFFIXES = {".obj"}

# Extensions treated as binary without sniffing the content.
BINARY_SUFFIXES = {
    ".7z",
    ".cat",
    ".dll",
    ".exe",
    ".gif",
    ".ico",
    ".jpeg",
    ".jpg",
    ".lib",
    ".mp3",
    ".mp4",
    ".o",
    ".pdf",
    ".png",
    ".so",
    ".sys",
    ".ttf",
    ".webp",
    ".zip",
}

# Fence language per file extension. Anything unknown falls back to "text".
LANGUAGES = {
    ".bat": "text",
    ".c": "cpp",
    ".cc": "cpp",
    ".cpp": "cpp",
    ".cxx": "cpp",
    ".h": "cpp",
    ".hpp": "cpp",
    ".hxx": "cpp",
    ".in": "text",
    ".jsm": "javascript",
    ".js": "javascript",
    ".json": "json",
    ".jsx": "javascript",
    ".mjs": "javascript",
    ".manifest": "text",
    ".md": "markdown",
    ".mjs": "javascript",
    ".nsh": "text",
    ".nsi": "text",
    ".ps1": "powershell",
    ".py": "python",
    ".rc": "text",
    ".ts": "typescript",
    ".tsx": "tsx",
    ".txt": "text",
    ".xml": "xml",
    ".yaml": "yaml",
    ".yml": "yaml",
}


def language_for(path: Path) -> str:
    """Return the code-fence language used for the given file."""
    if path.name == "CMakeLists.txt" or path.suffix.lower() == ".cmake":
        return "cmake"
    return LANGUAGES.get(path.suffix.lower(), "text")


def fence_for(content: str) -> str:
    """Return a code fence long enough to survive backticks inside the content."""
    longest = 0
    run = 0
    for char in content:
        if char == "`":
            run += 1
            longest = max(longest, run)
        else:
            run = 0
    return "`" * max(3, longest + 1)


def is_binary(path: Path) -> bool:
    """True when the file must be skipped instead of embedded verbatim."""
    if path.suffix.lower() in BINARY_SUFFIXES:
        return True
    try:
        with path.open("rb") as handle:
            chunk = handle.read(8192)
    except OSError:
        return True
    return b"\x00" in chunk


def is_excluded(rel: Path) -> bool:
    """True when the repo-relative path must not be packed."""
    parts = rel.parts
    for part in parts[:-1]:
        if part in EXCLUDE_DIR_NAMES:
            return True
    if parts[-1] in EXCLUDE_FILE_NAMES:
        return True
    if rel.suffix.lower() in EXCLUDE_SUFFIXES:
        return True
    posix = rel.as_posix()
    return any(
        posix == prefix or posix.startswith(prefix + "/")
        for prefix in EXCLUDE_PATH_PREFIXES
    )


def collect_files() -> list[Path]:
    """Return every packed file, sorted by repo-relative path."""
    found: list[Path] = []

    for entry in sorted(ROOT.iterdir(), key=lambda p: p.name):
        if not entry.is_file() or entry.is_symlink():
            continue
        rel = entry.relative_to(ROOT)
        if not is_excluded(rel):
            found.append(rel)

    for dir_name in INCLUDE_DIRS:
        base = ROOT / dir_name
        if not base.is_dir():
            continue
        for current, dirnames, filenames in os.walk(base, followlinks=False):
            dirnames[:] = sorted(d for d in dirnames if d not in EXCLUDE_DIR_NAMES)
            for filename in sorted(filenames):
                rel = (Path(current) / filename).relative_to(ROOT)
                if not is_excluded(rel):
                    found.append(rel)

    return sorted(set(found), key=lambda p: p.as_posix())


def build_tree(files: list[Path]) -> list[str]:
    """Build the directory-structure section (directories only, sorted)."""
    directories: set[str] = set()
    for rel in files:
        parts = rel.parts[:-1]
        for index in range(len(parts)):
            directories.add("/".join(parts[: index + 1]))

    lines: list[str] = []
    for directory in sorted(directories):
        depth = directory.count("/")
        lines.append(f"{'  ' * depth}{directory}/")
    return lines


def read_content(path: Path) -> str:
    """Read a text file verbatim, dropping any UTF-8 BOM."""
    data = path.read_bytes()
    if data.startswith(b"\xef\xbb\xbf"):
        data = data[3:]
    return data.decode("utf-8", errors="replace")


def git_revision() -> str:
    """Short description of the revision the pack was generated from."""

    def run(*args: str) -> str:
        try:
            result = subprocess.run(
                ["git", *args],
                cwd=ROOT,
                capture_output=True,
                text=True,
                timeout=15,
            )
        except (OSError, subprocess.SubprocessError):
            return ""
        return result.stdout.strip() if result.returncode == 0 else ""

    sha = run("rev-parse", "--short", "HEAD")
    branch = run("rev-parse", "--abbrev-ref", "HEAD")
    dirty = " (dirty worktree)" if run("status", "--porcelain") else ""
    if not sha:
        return "unknown revision"
    return f"{sha} on {branch}{dirty}"


def main() -> int:
    files = collect_files()
    if not files:
        print("no files matched the include scope", file=sys.stderr)
        return 1

    revision = git_revision()
    stamp = datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M UTC")

    out: list[str] = []
    out.append(
        "This file is a merged representation of the codebase, containing all source code "
        "and UI/UX files combined into a single document."
    )
    out.append("")
    out.append(
        "Regenerated with full source code (comments preserved) from all code and UI/UX files."
    )
    out.append("")
    out.append(f"Source revision: {revision}")
    out.append(f"Generated: {stamp} - {len(files)} files")
    out.append("")
    out.append("# File Summary")
    out.append("")
    out.append("## Purpose")
    out.append(
        "This file contains a packed representation of the repository's source code and UI/UX files."
    )
    out.append(
        "It is designed to be easily consumable by AI systems for analysis, code review,"
    )
    out.append("or other automated processes.")
    out.append("")
    out.append("## File Format")
    out.append("1. This summary section")
    out.append("2. Repository information")
    out.append("3. Directory structure")
    out.append("4. Multiple file entries, each consisting of:")
    out.append("   a. A header with the file path (## File: path/to/file)")
    out.append("   b. The full contents of the file in a code block")
    out.append("")
    out.append("## Notes")
    out.append("- All source code files are included with comments preserved.")
    out.append("- Binary files are listed but not embedded.")
    out.append("- Files are sorted alphabetically by path.")
    out.append("")
    out.append("# Directory Structure")
    out.append("```")
    out.extend(build_tree(files))
    out.append("```")
    out.append("")
    out.append("# Files")
    out.append("")

    chunks: list[bytes] = ["\n".join(out).encode("utf-8")]

    for rel in files:
        path = ROOT / rel
        posix = rel.as_posix()
        if is_binary(path):
            body = "(binary file, skipped)\n"
            fence = "```"
        else:
            body = read_content(path)
            if body and not body.endswith("\n"):
                body += "\n"
            fence = fence_for(body)
        header = f"## File: {posix}\n{fence}{language_for(path)}\n"
        chunks.append(header.encode("utf-8"))
        chunks.append(body.encode("utf-8"))
        chunks.append(f"{fence}\n\n".encode("utf-8"))

    OUTPUT.write_bytes(b"".join(chunks))
    print(
        f"wrote {OUTPUT.relative_to(ROOT)} ({len(files)} files, {OUTPUT.stat().st_size} bytes)"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
