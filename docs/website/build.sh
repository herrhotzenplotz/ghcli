#!/bin/sh
#
# This script builds the website into a temporary directory
#

cd $(dirname $0)

# Build the tutorial
(
	cd tutorial
	./gen.sh
)

# Make a dist directory and copy over files
DISTDIR=$(mktemp -d)
mkdir -p ${DISTDIR}/tutorial
mkdir -p ${DISTDIR}/assets

cp -p index.html ${DISTDIR}/
cp -p \
	tutorial/0*.html \
	tutorial/index.html \
	${DISTDIR}/tutorial

cp -p \
	../screenshot.png \
	${DISTDIR}/assets/screenshot.png

echo "${DISTDIR}"
