#!/bin/sh
#

command -v git > /dev/null 2>&1 || (echo "error: you need git to run this script" && exit 1)

findversion() {
	eval $(grep PACKAGE_VERSION $(dirname $0)/../configure | sed 1q)
	echo $PACKAGE_VERSION
}

VERSION=$(findversion)

DIR=$(git rev-parse --show-toplevel)/dist/gcli-${VERSION}
mkdir -p $DIR

echo "Making BZIP tarball"
git archive --format=tar --prefix=gcli-$VERSION/ @ \
	| bzip2 -v > $DIR/gcli-$VERSION.tar.bz2
echo "Making XZ tarball"
git archive --format=tar --prefix=gcli-$VERSION/ @ \
	| xz -v > $DIR/gcli-$VERSION.tar.xz
echo "Making GZIP tarball"
git archive --format=tar --prefix=gcli-$VERSION/ @ \
	| gzip -v > $DIR/gcli-$VERSION.tar.gz

OLDDIR=$(pwd)
cd $DIR
echo "Calculating SHA256SUMS"
sha256sum *.tar* > SHA256SUMS
cd $OLDDIR

echo "Release Tarballs are at $DIR"
