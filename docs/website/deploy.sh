#!/bin/sh -xe
#
# This script builds the website and then creates a tarball that
# is pulled regularly from the server
#

cd $(dirname $0)

DISTDIR=$(./build.sh)
tar -c -f - -C ${DISTDIR} \. | xz > website_dist.tar.xz
rm -fr ${DISTDIR}
