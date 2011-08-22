#!/bin/bash

autoreconf --install
./configure
make dist

version=`grep "PACKAGE_VERSION =" Makefile | awk '{print $3;}'`
tmp=/tmp/oml2-rpmbuild

if [ -d $tmp ]; then rm -rf $tmp; fi

mkdir -p $tmp
cd $tmp
mkdir -p BUILD RPMS SOURCES SPECS SRPMS
cd -
ln -s `pwd`/oml2-$version.tar.gz $tmp/SOURCES/
rpmbuild -v -bb --clean oml2.spec --define "_topdir $tmp"
find $tmp -type f -name "*.rpm" -exec cp -v {} . \;
rm -rf $tmp

