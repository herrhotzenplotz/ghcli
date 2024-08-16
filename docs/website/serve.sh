#!/bin/sh
#

cd $(dirname $0)

DISTDIR=$(./build.sh)

cleanup() {
	rm -fr ${DISTDIR}
}

trap cleanup EXIT TERM INT
python3.11 -m http.server -d ${DISTDIR} 8080
