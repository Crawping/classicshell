#!/bin/sh
# Get path of script itself (not links)
root="$(dirname "$(readlink -f "$(which "$0")")")/.."

# For each URL in etc/download/urls.asc
while read url; do
	# form a path to save to based off of the URL
	path="download/$(echo "$url" | sed -e '
		s#\([a-zA-Z]\)://\(.*\)#\1/\2#;
		s/%20/ /g;
		s/%28/(/g;
		s/%29/)/g;
	')"
	dirname="$(dirname "$path")"

	# create the destination directory
	mkdir -p "$dirname"
	# and download the file, if we don't already have it
	test -f "$path" || wget "$url" -O "$path"
done < "$root/etc/download/urls.asc"
