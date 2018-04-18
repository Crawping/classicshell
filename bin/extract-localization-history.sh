#!/bin/sh
# Get path of script itself (not links)
root="$(dirname "$(readlink -f "$(which "$0")")")/.."

# Directory where sources can be found
sources_dir="$1"
test -n "$sources_dir" || { echo "syntax error">&2; exit 1; }

while read source; do
	dest="$(mktemp -d)"
	rm -rf src

	unzip -q -d "$dest" "$sources_dir/$source"
	mv "$dest" src
	rm -rf "$dest"

	# Get metadata
	author="Ivo Beltchev <classicshell@ibeltchev.com>"
	version="$(basename "$(dirname "$sources_dir/$source")")"
	date="@$(stat -c %Y "$sources_dir/$source") +0000"

	# Commit the extracted source
	git add src
	git commit -q --no-gpg-sign -m "$version" --author="$author" --date="$date"
done < "$root/etc/localization-sources.asc"
