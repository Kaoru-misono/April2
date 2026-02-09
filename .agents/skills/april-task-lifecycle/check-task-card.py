#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import datetime as dt
import os
import re
import sys

try:
    import yaml
except ImportError:
    print("Missing dependency: pyyaml. Install it (pip install pyyaml).")
    sys.exit(2)

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
TASK_ROOT = os.path.join(ROOT, "agent", "sub-agent")

RE_ID = re.compile(r"^[A-Z][A-Z0-9-]*-\d{3,}$")
RE_DATE = re.compile(r"^\d{4}-\d{2}-\d{2}$")

REQUIRED_KEYS = {
    "id",
    "title",
    "status",
    "owner",
    "priority",
    "deps",
    "updated_at",
    "evidence",
}

ALLOWED_STATUS = {"todo", "in_progress", "blocked", "done"}
ALLOWED_PRIORITY = {"p0", "p1", "p2", "p3"}
REQUIRED_SECTIONS = {"## Goal", "## Acceptance Criteria", "## Test Plan"}


def normalize_path(path: str) -> str:
    return path.replace("\\", "/")


def iter_task_files() -> list[str]:
    files = []
    if not os.path.isdir(TASK_ROOT):
        return files
    for root, _, filenames in os.walk(TASK_ROOT):
        for name in filenames:
            if not name.endswith(".md"):
                continue
            full_path = os.path.join(root, name)
            norm = normalize_path(full_path)
            if "/tasks/" not in norm:
                continue
            files.append(full_path)
    return files


def read_text(path: str) -> str:
    with open(path, "r", encoding="utf-8") as f:
        return f.read()


def parse_frontmatter(text: str) -> tuple[dict, str, list[str]]:
    lines = text.splitlines()
    errors = []
    if not lines or lines[0].strip() != "---":
        errors.append("frontmatter must start with '---' on the first line")
        return {}, text, errors

    end_index = None
    for i in range(1, len(lines)):
        if lines[i].strip() == "---":
            end_index = i
            break
    if end_index is None:
        errors.append("frontmatter must end with '---' separator")
        return {}, text, errors

    frontmatter_text = "\n".join(lines[1:end_index])
    body_text = "\n".join(lines[end_index + 1 :])
    data = yaml.safe_load(frontmatter_text) or {}
    if not isinstance(data, dict):
        errors.append("frontmatter must be a YAML mapping")
        return {}, body_text, errors

    return data, body_text, errors


def validate_frontmatter(data: dict) -> list[str]:
    errors = []
    keys = set(data.keys())
    missing = REQUIRED_KEYS - keys
    extra = keys - REQUIRED_KEYS
    if missing:
        errors.append(f"missing required keys: {', '.join(sorted(missing))}")
    if extra:
        errors.append(f"unexpected keys: {', '.join(sorted(extra))}")

    if "id" in data:
        if not isinstance(data["id"], str) or not RE_ID.match(data["id"]):
            errors.append("id must match pattern MODULE-### (uppercase, dash, numeric suffix)")

    if "title" in data and not isinstance(data["title"], str):
        errors.append("title must be a string")

    if "owner" in data and not isinstance(data["owner"], str):
        errors.append("owner must be a string")

    if "status" in data:
        if data["status"] not in ALLOWED_STATUS:
            errors.append(f"status must be one of: {', '.join(sorted(ALLOWED_STATUS))}")

    if "priority" in data:
        if data["priority"] not in ALLOWED_PRIORITY:
            errors.append(f"priority must be one of: {', '.join(sorted(ALLOWED_PRIORITY))}")

    if "deps" in data:
        if not isinstance(data["deps"], list):
            errors.append("deps must be a list")
        else:
            for dep in data["deps"]:
                if not isinstance(dep, str):
                    errors.append("deps must contain only strings")
                    break

    if "updated_at" in data:
        if not isinstance(data["updated_at"], str) or not RE_DATE.match(data["updated_at"]):
            errors.append("updated_at must be YYYY-MM-DD")
        else:
            try:
                dt.date.fromisoformat(data["updated_at"])
            except ValueError:
                errors.append("updated_at must be a valid date")

    if "evidence" in data and not isinstance(data["evidence"], str):
        errors.append("evidence must be a string")

    return errors


def validate_body(body: str) -> list[str]:
    errors = []
    found = set()
    for line in body.splitlines():
        header = line.strip()
        if header in REQUIRED_SECTIONS:
            found.add(header)

    missing = REQUIRED_SECTIONS - found
    if missing:
        errors.append(f"missing required sections: {', '.join(sorted(missing))}")
    return errors


def validate_location(path: str, status: str | None) -> list[str]:
    errors = []
    norm = normalize_path(os.path.abspath(path))
    in_complete = "/tasks/complete/" in norm
    if status == "done" and not in_complete:
        errors.append("status done requires file to be under tasks/complete/")
    if status != "done" and in_complete:
        errors.append("only status done is allowed under tasks/complete/")
    return errors


def validate_file(path: str) -> list[str]:
    text = read_text(path)
    data, body, errors = parse_frontmatter(text)
    errors.extend(validate_frontmatter(data))
    errors.extend(validate_body(body))
    status = data.get("status") if isinstance(data, dict) else None
    errors.extend(validate_location(path, status))
    return errors


def resolve_path(path: str) -> str:
    if os.path.isabs(path):
        return path
    return os.path.abspath(os.path.join(ROOT, path))


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate April2 task card format.")
    parser.add_argument("--file", action="append", default=[], help="Task card file path to check.")
    args = parser.parse_args()

    files = []
    if args.file:
        for raw in args.file:
            path = resolve_path(raw)
            if not os.path.exists(path):
                print(f"Missing file: {raw}")
                return 2
            files.append(path)
    else:
        files = iter_task_files()

    if not files:
        print("No task cards found.")
        return 0

    failed = False
    for path in sorted(files):
        errors = validate_file(path)
        if errors:
            failed = True
            print(f"\nFAILED: {normalize_path(os.path.relpath(path, ROOT))}")
            for err in errors:
                print(f"- {err}")

    if failed:
        print("\nTask card validation FAILED.")
        return 1

    print(f"Task card validation OK. Files checked: {len(files)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
