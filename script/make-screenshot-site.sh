#!/usr/bin/env bash
# Collects system test screenshots into a static site (GitHub Pages).
# Usage: make-screenshot-site.sh <test-results-dir> <output-dir>
set -euo pipefail

src="${1:?test-results directory required}"
out="${2:?output directory required}"

rm -rf "$out"
mkdir -p "$out"
touch "$out/.nojekyll"

# Screenshot names are unique across the system tests.
find "$src" -type f -name '*.png' -exec cp {} "$out/" \;

{
  cat <<'EOF'
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Tupal screenshots</title>
<style>
  body { font-family: sans-serif; margin: 2rem; }
  figure { margin: 0 0 2rem; }
  img { max-width: 100%; border: 1px solid #ccc; }
</style>
</head>
<body>
<h1>Tupal application views</h1>
EOF
  for f in "$out"/*.png; do
    [ -e "$f" ] || continue
    name="$(basename "$f")"
    printf '<figure><figcaption><a href="%s">%s</a></figcaption><img src="%s" alt="%s" loading="lazy"></figure>\n' \
      "$name" "${name%.png}" "$name" "${name%.png}"
  done
  echo '</body></html>'
} > "$out/index.html"
