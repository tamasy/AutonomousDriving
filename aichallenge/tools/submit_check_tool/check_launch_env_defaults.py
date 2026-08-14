#!/usr/bin/env python3

from __future__ import annotations

import argparse
import sys
from dataclasses import dataclass
from pathlib import Path


@dataclass
class Finding:
    path: Path
    line: int
    col: int
    severity: str  # "ERROR" or "WARNING"
    snippet: str
    message: str


def mask_xml_comments(text: str) -> str:
    """Replace the contents of <!-- ... --> blocks with spaces (preserving
    length, line breaks, and every other character's offset) so comments
    that merely *mention* the dangerous pattern (e.g. explaining the bug in
    prose) are not mistaken for live code.
    """
    out = list(text)
    start = text.find("<!--")
    while start != -1:
        end = text.find("-->", start + 4)
        if end == -1:
            end = len(text)
        else:
            end += 3
        for i in range(start, end):
            if out[i] != "\n":
                out[i] = " "
        start = text.find("<!--", end)
    return "".join(out)


def find_dollar_paren_spans(text: str):
    """Yield (start, end, inner_text) for every $(...) span in text.

    `inner_text` is the content strictly between the matching '(' and ')'.
    Spans are found at every nesting level (i.e. both an outer $(eval "...")
    and any $(env ...)/$(var ...) substitutions nested inside it are each
    yielded as their own span), since a plain left-to-right scan for '$('
    is used rather than skipping past spans once matched.
    """
    n = len(text)
    i = 0
    while i < n:
        if text[i] == "$" and i + 1 < n and text[i + 1] == "(":
            depth = 0
            j = i + 1
            while j < n:
                if text[j] == "(":
                    depth += 1
                elif text[j] == ")":
                    depth -= 1
                    if depth == 0:
                        j += 1
                        break
                j += 1
            else:
                # Unbalanced parens from this '$(' onward; nothing more to find.
                break
            yield (i, j, text[i + 2 : j - 1])
        i += 1


def line_col(text: str, offset: int):
    line = text.count("\n", 0, offset) + 1
    last_nl = text.rfind("\n", 0, offset)
    col = offset - last_nl
    return line, col


def check_env_span(path: Path, text: str, start: int, end: int, inner: str):
    """inner is the content of a $(env ...) span, e.g. "env NAME default"."""
    tokens = inner.split()
    if not tokens or tokens[0] != "env":
        return None
    if len(tokens) < 3:
        # $(env NAME) with no default -- raises a clear error if unset,
        # not the bug this tool looks for.
        return None
    name = tokens[1]
    default = " ".join(tokens[2:])
    if "'" not in default and '"' not in default:
        return None

    line, col = line_col(text, start)
    snippet = text[start:end]
    looks_like_empty_string_typo = default in ("''", '""')
    if looks_like_empty_string_typo:
        hint = (
            f"This looks like an attempt to default to an empty string, but "
            f"$(env {name} {default}) does not mean \"empty string\" -- if "
            f"{name} is unset, the literal text {default} (the quote "
            f"characters themselves) is substituted."
        )
    else:
        hint = (
            f"The default value {default!r} contains a quote character. "
            f"$(env {name} ...) substitutes its default as raw text with no "
            f"quote handling, so an unset {name} will inject that quote "
            f"character literally."
        )
    message = (
        f"{hint}\n"
        f"    If this is unset in the real environment (e.g. production, "
        f"which typically does not run under docker-compose and so never "
        f"defines fallback env vars), the literal quote character(s) can "
        f"unbalance an enclosing $(eval \"...\") expression and crash the "
        f"entire launch tree before any node starts -- with zero log output.\n"
        f"    Suggested fix: use a quote-free sentinel default, e.g.\n"
        f"        $(env {name} UNSET)\n"
        f"    and compare explicitly where the value is used, e.g.\n"
        f"        $(eval \"'$(var some_arg)' in ('UNSET', '')\")"
    )
    return Finding(path, line, col, "ERROR", snippet, message)


def check_eval_span(path: Path, text: str, start: int, end: int, inner: str):
    """inner is the content of a $(eval ...) span, e.g. 'eval "...".

    Heuristic only: counts single-quote characters in the *authored* text.
    An odd count often (not always) indicates unbalanced string literals
    once the expression is evaluated as Python.
    """
    tokens = inner.split(None, 1)
    if not tokens or tokens[0] != "eval":
        return None
    if len(tokens) < 2:
        return None
    expr = tokens[1].strip()
    quote_count = expr.count("'")
    if quote_count % 2 == 0:
        return None

    line, col = line_col(text, start)
    snippet = text[start:end]
    message = (
        f"This $(eval ...) expression has an odd number of single-quote "
        f"characters ({quote_count}) as literally written. That can mean the "
        f"expression will fail to parse as Python once substitutions are "
        f"resolved -- often because a nested $(env ...)/$(var ...) "
        f"substitution injects (or fails to inject) a quote character.\n"
        f"    This is a heuristic, best-effort check: it can both miss real "
        f"problems and flag expressions that are actually fine (e.g. quotes "
        f"balanced across a nested substitution's *runtime* value, which "
        f"this tool cannot see). Treat it as a hint to double-check, not a "
        f"verdict -- the ERROR-level $(env ...) check above is the "
        f"authoritative one."
    )
    return Finding(path, line, col, "WARNING", snippet, message)


def check_file(path: Path):
    findings = []
    try:
        text = path.read_text(encoding="utf-8")
    except (OSError, UnicodeDecodeError) as exc:
        print(f"[skip] {path}: {exc}", file=sys.stderr)
        return findings

    masked = mask_xml_comments(text)
    for start, end, inner in find_dollar_paren_spans(masked):
        f = check_env_span(path, text, start, end, inner)
        if f is not None:
            findings.append(f)
            continue
        f = check_eval_span(path, text, start, end, inner)
        if f is not None:
            findings.append(f)

    return findings


def iter_target_files(paths, extensions):
    for raw in paths:
        p = Path(raw)
        if p.is_dir():
            for ext in extensions:
                yield from sorted(p.rglob(f"*{ext}"))
        elif p.is_file():
            yield p
        else:
            print(f"[warn] path not found, skipping: {p}", file=sys.stderr)


def main(argv=None):
    parser = argparse.ArgumentParser(
        description=(
            "Scan ROS2 launch XML files for the "
            "\"$(env NAME 'default')\" quote-substitution trap."
        )
    )
    parser.add_argument(
        "paths",
        nargs="*",
        default=["."],
        help="Files and/or directories to scan (default: current directory)",
    )
    parser.add_argument(
        "--ext",
        default=".xml",
        help="Comma-separated extensions to scan within directories (default: .xml)",
    )
    parser.add_argument(
        "--strict",
        action="store_true",
        help="Also fail (exit 1) on WARNING-level findings",
    )
    parser.add_argument(
        "--quiet",
        action="store_true",
        help="Print only the final summary line",
    )
    args = parser.parse_args(argv)

    extensions = [e if e.startswith(".") else f".{e}" for e in args.ext.split(",")]

    all_findings = []
    for path in iter_target_files(args.paths, extensions):
        all_findings.extend(check_file(path))

    errors = [f for f in all_findings if f.severity == "ERROR"]
    warnings = [f for f in all_findings if f.severity == "WARNING"]

    if not args.quiet:
        for f in sorted(all_findings, key=lambda f: (str(f.path), f.line)):
            print(f"[{f.severity}] {f.path}:{f.line}:{f.col}: {f.snippet}")
            for line in f.message.splitlines():
                print(f"    {line}")
            print()

    scanned = len(list(iter_target_files(args.paths, extensions)))
    print(
        f"Scanned {scanned} file(s): {len(errors)} error(s), "
        f"{len(warnings)} warning(s)."
    )

    if errors or (args.strict and warnings):
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
