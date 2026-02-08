#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import fnmatch
import os
import subprocess
import sys

try:
    import yaml
except ImportError:
    print("Missing dependency: pyyaml. Install it (pip install pyyaml).")
    sys.exit(2)

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
CONFIG = os.path.join(ROOT, ".docsync.yml")

def run(cmd: list[str]) -> str:
    return subprocess.check_output(cmd, cwd=ROOT).decode("utf-8", errors="ignore").strip()

def git_changed_files(base_ref: str, head_ref: str) -> list[str]:
    out = run(["git", "diff", "--name-only", f"{base_ref}...{head_ref}"])
    return [line.strip() for line in out.splitlines() if line.strip()]

def match_any(path: str, patterns: list[str]) -> bool:
    for p in patterns:
        if fnmatch.fnmatch(path, p):
            return True
    return False

def main():
    if not os.path.exists(CONFIG):
        print("No .docsync.yml found; skipping.")
        return 0

    base = os.environ.get("DOCSYNC_BASE", "origin/main")
    head = os.environ.get("DOCSYNC_HEAD", "HEAD")

    changed = git_changed_files(base, head)
    if not changed:
        print("No changes.")
        return 0

    with open(CONFIG, "r", encoding="utf-8") as f:
        cfg = yaml.safe_load(f) or {}

    rules = cfg.get("rules", [])
    failed = []
    changed_set = set(changed)

    for rule in rules:
        name = rule.get("name", "<unnamed>")
        when_changed = rule.get("when_changed", [])
        must_also = rule.get("must_also_change", [])

        if not when_changed or not must_also:
            continue

        triggered = any(match_any(p, when_changed) for p in changed)
        if not triggered:
            continue

        ok = any(req in changed_set for req in must_also)
        if not ok:
            failed.append((name, when_changed, must_also))

    if failed:
        print("\nDocSync FAILED: Public API/contract changed without updating required docs.\n")
        for name, when_changed, must_also in failed:
            print(f"- Rule: {name}")
            print("  Trigger patterns:")
            for p in when_changed:
                print(f"    - {p}")
            print("  Required doc(s) to update (change at least one):")
            for d in must_also:
                print(f"    - {d}")
            print()
        print("Fix: update module interfaces/changelog or integration contract docs in the same PR.")
        return 1

    print("DocSync OK.")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
