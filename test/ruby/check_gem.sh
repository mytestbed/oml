#!/bin/sh
echo "Testing that the oml4r-${PACKAGE_VERSION} Gem is properly built..."
EXPECTED_GEMFILES=`find ${TOPSRCDIR}/ruby/oml4r -type f | \
	sed "/\.gitignore/d;/pkg\//d;s-${TOPSRCDIR}/ruby/oml4r/--;s/\.in\$//" | sort | uniq`
ACTUAL_GEMFILES=`tar xf ${TOPBUILDDIR}/ruby/oml4r/pkg/oml4r-${PACKAGE_VERSION}.gem data.tar.gz -O  | \
	tar tzv | sed 's/.* //' | sort`
test -n "${EXPECTED_GEMFILES}" -a "${EXPECTED_GEMFILES}" = "${ACTUAL_GEMFILES}"
