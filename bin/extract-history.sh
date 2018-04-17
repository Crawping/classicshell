#!/bin/sh
# Get path of script itself (not links)
root="$(dirname "$(readlink -f "$(which "$0")")")/.."

# Directory where sources can be found
sources_dir="$1"
test -n "$sources_dir" || { echo "syntax error">&2; exit 1; }

while read source; do
	dest="$(mktemp -d)"
	rm -rf src

	# Old versions store the source in a directory called "ClassicShell"
	unzip -q -d "$dest" "$sources_dir/$source"
	if [ -d "$dest/ClassicShell" ]; then
		mv "$dest/ClassicShell" src
	else
		mv "$dest" src
	fi
	rm -rf "$dest"

	# Get metadata
	author="Ivo Beltchev <classicshell@ibeltchev.com>"
	version="$(basename "$(dirname "$sources_dir/$source")")"
	date="@$(stat -c %Y "$sources_dir/$source") +0000"

	# Commit the extracted source
	git add src
	git commit -q --no-gpg-sign -m "$version" --author="$author" --date="$date"
done < "$root/etc/sources.asc"
