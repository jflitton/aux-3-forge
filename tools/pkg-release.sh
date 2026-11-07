#!/bin/sh
# Release pkg: make release (or make release-dry-run) in pkg/ runs this.
#
#     tools/pkg-release.sh [-n | --github]
#
# For the PKG_VERSION in pkg/src/pkg.h, committed on main, it checks everything it can
# first (pkg/ and tools/ committed, the version new on GitHub and on the server, the key,
# the AWS CLI and gh signed in), runs make test and make test-aux, then tags the commit
# pkg-vVERSION, makes the package (make package), adds it to the default server
# (tools/pkg-publish.py), pushes main and the tag, and makes the GitHub release. If the
# package doesn't get onto the server, the tag is taken off again and nothing is pushed.
#
# -n does it all short of the server and GitHub: the checks (a missing sign-in is only a
# warning), the tests, the package and the new list (pkg-publish.py -n, in build/site/),
# then takes the tag off again.
#
# --github only makes the GitHub release, for a version already on the server and tagged
# on GitHub (if pushing failed, say): pkg and pkg-VERSION.pkg as the server has them,
# which pkg/README.md's "Getting pkg" links to, and the commits since the last release.
#
# Environment:
#   PKG_S3        the server's bucket, s3://BUCKET[/DIR]: PKG_S3=... in ../.env, which
#                 direnv's ../.envrc exports (kept out of the repository)
#   PKG_KEYFILE   the key that signs the list; ../.env (the repository's) if not set
set -e
mode=release
case $1 in
	-n) mode=dry; shift ;;
	--github) mode=github; shift ;;
esac
[ $# -eq 0 ] || { echo "usage: pkg-release.sh [-n | --github]" >&2; exit 2; }
cd "$(dirname "$0")/../pkg"
PY="python3 -I"
die() { echo "release: $*" >&2; exit 1; }
warn() { echo "release: $*" >&2; }
need() { if [ $mode = dry ]; then warn "$* (a release would stop here)"; else die "$*"; fi; }

version=$(sed -n 's/^#define[ 	]*PKG_VERSION[ 	]*"\(.*\)"/\1/p' src/pkg.h)
url=$(sed -n 's/^#define[ 	]*DEFAULT_SERVER[ 	]*"\(.*\)"/\1/p' src/defaults.h)
[ -n "$version" ] || die "no PKG_VERSION in src/pkg.h"
[ -n "$url" ] || die "no DEFAULT_SERVER in src/defaults.h"
tag=pkg-v$version
key=${PKG_KEYFILE:-../.env}
pkgfile=build/packages/pkg-$version.pkg

# The GitHub release: pkg and its package as the server has them, with notes
github() {
	gh auth status > /dev/null 2>&1 || die "gh isn't signed in: gh auth login"
	git ls-remote --exit-code --tags origin "$tag" > /dev/null || die "$tag isn't on GitHub"
	! gh release view "$tag" > /dev/null 2>&1 || die "GitHub has a release $tag already"
	line=$(curl -sf "${url}index" | grep "^pkg	$version	") || die "pkg $version isn't on $url"
	size=$(echo "$line" | cut -f3) sha=$(echo "$line" | cut -f4) path=$(echo "$line" | cut -f5)
	rm -rf build/github
	mkdir -p build/github
	curl -sf "$url$path" -o "build/github/pkg-$version.pkg" || die "can't fetch $url$path"
	[ "$(wc -c < "build/github/pkg-$version.pkg" | tr -d ' ')" = "$size" ] &&
		[ "$($PY -c 'import hashlib, sys; print(hashlib.sha256(open(sys.argv[1], "rb").read()).hexdigest())' "build/github/pkg-$version.pkg")" = "$sha" ] ||
		die "$url$path isn't the file the server's list names"
	tar -xzf "build/github/pkg-$version.pkg" -O usr/local/bin/pkg > build/github/pkg
	prev=$(git describe --tags --match 'pkg-v*' --abbrev=0 "$tag^" 2>/dev/null) || prev=
	{
		echo "pkg $version, the package manager for A/UX 3."
		echo
		echo "On a Mac that has pkg, \`pkg upgrade\` installs it. On one that doesn't, follow [Getting pkg](https://github.com/jflitton/aux-3/blob/main/pkg/README.md#getting-pkg) with \`pkg\` below. \`pkg-$version.pkg\` is the same pkg as a package file."
		if [ -n "$prev" ]; then
			echo
			echo "Since ${prev#pkg-v}:"
			echo
			git log --no-merges --format='- %s' "$prev..$tag" -- .
		fi
	} > build/github/notes.md
	gh release create "$tag" build/github/pkg "build/github/pkg-$version.pkg" \
		--title "pkg $version" --notes-file build/github/notes.md --latest
}

if [ $mode = github ]; then
	github
	exit 0
fi

[ "$(git rev-parse --abbrev-ref HEAD)" = main ] || die "not on main"
[ -z "$(git status --porcelain -- . ../tools)" ] || die "pkg/ or tools/ has changes not committed"
! git rev-parse -q --verify "refs/tags/$tag" > /dev/null ||
	die "$tag is tagged already: a new pkg needs a new PKG_VERSION in src/pkg.h"
git fetch -q origin
! git ls-remote --exit-code --tags origin "$tag" > /dev/null || die "$tag is on GitHub already"
git merge-base --is-ancestor origin/main HEAD || die "main is behind GitHub's: pull first"
if curl -sf "${url}index" | grep -q "^pkg	$version	"; then
	die "pkg $version is on $url already"
fi
[ -f "$key" ] || die "no $key: the key that signs the package list (set PKG_KEYFILE)"
[ -n "$PKG_S3" ] || need "PKG_S3 isn't set: PKG_S3=s3://BUCKET[/DIR] in ../.env, and direnv allow"
aws sts get-caller-identity > /dev/null 2>&1 || need "the AWS CLI isn't signed in: aws login"
gh auth status > /dev/null 2>&1 || need "gh isn't signed in: gh auth login"

make test
make test-aux

git tag -a "$tag" -m "pkg $version"
trap 'git tag -d "$tag" > /dev/null; echo "release: $tag taken off again" >&2' EXIT
make package
if [ $mode = dry ]; then
	$PY ../tools/pkg-publish.py -n "$key" "${PKG_S3:-s3://unset}" "$url" "$pkgfile"
	echo "release: pkg $version would be released (-n: not published, not pushed)"
	exit 0
fi
$PY ../tools/pkg-publish.py "$key" "$PKG_S3" "$url" "$pkgfile"
trap - EXIT

git push --atomic origin main "$tag" ||
	die "pkg $version is on the server, but pushing failed: git push --atomic origin main $tag, then make release-github"
github
echo "release: pkg $version is on $url and GitHub"
echo "release: if a patch needs it, bring --pkg up to date: patches/pkg/update.sh"
