#!/bin/bash -e
# Create a new OML release based on [0].
#
# [0] http://oml.mytestbed.net/projects/oml/wiki/Release_Process
#
# Copyright 2013-2015 National ICT Australia (NICTA), Olivier Mehani
#
# This software may be used and distributed solely under the terms of
# the MIT license (License).  You should find a copy of the License in
# COPYING or at http://opensource.org/licenses/MIT. By downloading or
# using this software you accept the terms and the liability disclaimer
# in the License.
#
# Release the apps with:
# SOFTWARE_NAME="OML Applications" PACKAGE_NAME=oml2-apps REDMINE_PROJECT=omlapp ../oml/do-release.sh
# Don't forget to tag submodules before building the distribution tarball
SOFTWARE_NAME=${SOFTWARE_NAME:-OML}
PACKAGE_NAME=${PACKAGE_NAME:-oml2}
REDMINE_PROJECT=${REDMINE_PROJECT:-oml}

REDMINE_URL=http://oml.mytestbed.net

OBS_URL=https://build.opensuse.org
OBS_NS=devel:tools:mytestbed:stable
OBS_NS_STAGING= devel:tools:mytestbed:sid

# XXX: Linux-only
NPROC=$((`grep processor /proc/cpuinfo | wc -l`+1))

MAKE="make -j $NPROC"
GIT=git
DCH=dch
MKTEMP="mktemp --tmpdir"

TS=$(date +%Y-%m-%d_%H:%M)
if [ -e .gitmodules ]; then
	MODULES=`sed -n 's/^.*path = //p' .gitmodules`
fi

DEBPSQL=/usr/include/postgresql
test -d $DEBPSQL && export CFLAGS="$CFLAGS -I$DEBPSQL"

### HELPER FUNCTIONS ###

## VERSION STRING MANIPULATIONS ##

# Parse $1 as an OML2 version number from git-version-gen
parse_ver()
{
	OML=$1
	VER=$2
	echo $VER | \
		sed -n "s/^\([0-9]\+\)\.\([0-9]\+\)\(\.\([0-9]\+\)\~\?\([^-0-9+.]*\)\.\?\([0-9]*\)\(-\([^-0-9]*\)\([0-9]*\)\)\?\)\?.*/ \
		export ${OML}_MAJOR=\1 ${OML}_MINOR=\2 ${OML}_REV=\4 ${OML}_TYPE=\5 ${OML}_TYPEREV=\6 ${OML}_EXTRA=\8 ${OML}_EXTRAREV=\9/p"
	# XXX test that the mandatory fields are not empty
}

# Compare version strings $1 and $2 up to and including type, and return 255 ($1<$2), 0 ($1==$2) or 1 ($1>$2)
# 2.10.0 < 2.11.0				(255)
# 2.11.0pre < 2.11.0rc				(255)
# 2.11.0rc2 > 2.11.0rc1				(1)
# 2.11.0rc1-mytestbed1 == 2.11.0rc1-mytestbed2	(0)
compare_ver()
{
	eval `parse_ver FIRST "$1"`
	eval `parse_ver SECOND "$2"`

	# 2
	compare_ver_one "$FIRST_MAJOR" "$SECOND_MAJOR"
	case $? in
	1) return 1;;
	255) return 255;;
	esac

	# 11
	compare_ver_one "$FIRST_MINOR" "$SECOND_MINOR"
	case $? in
	1) return 1;;
	255) return 255;;
	esac

	# 0
	compare_ver_one "$FIRST_REV" "$SECOND_REV"
	case $? in
	1) return 1;;
	255) return 255;;
	esac

	# rc
	compare_ver_one "$FIRST_TYPE" "$SECOND_TYPE"
	case $? in
	1) return 1;;
	255) return 255;;
	esac

	# 2
	compare_ver_one "$FIRST_TYPEREV" "$SECOND_TYPEREV"
	case $? in
	1) return 1;;
	255) return 255;;
	esac

	# [mytestbed]
	# 2
	compare_ver_one "$FIRST_EXTRAREV" "$SECOND_EXTRAREV"
	case $? in
	1) return 1;;
	255) return 255;;
	esac

	return 0 # Nothing else is different
}

# Compare in two version component strings $1 and $2, and return 255 ($1<$2), 0 ($1==$2) or 1 ($1>$2)
# 10 < 11				(255)
# pre < rc				(255)
# 2 > 1					(1)
# 1 == 1				(0)
# "" < 1				(255)
compare_ver_one()
{
	case $1 in
		"pre")
			test "$2" = "" -o "$2" == "rc" && return 255
			return 0 # XXX: Everything else is the same as pre
			;;
		"rc")
			test "$2" = "" && return 255
			test "$2" = "rc" && return 0
			return 1 # Everything else is considered as pre
			;;
		"")
			test "$2" = "" && return 0
			test "$2" = "rc" -o "$2" = "pre" && return 255
			# XXX not version tags, try them as integers
			;;
	esac

	# XXX $1 and $2 should be checked to be integer at this stage
	test 0$1 -lt 0$2 && return 255
	test 0$1 -gt 0$2 && return 1
	return 0
}

## GIT HELPERS ##

# Check whether branch $1 exist in git, and output its name; $1 defaults to HEAD
check_branch()
{
	${GIT} rev-parse --abbrev-ref ${1:-HEAD} 2>>$LOG
}

# Check whether branch $1 (default to HEAD) has been created in this session
# (i.e., there are instructions to delete it on failure)
is_new_branch()
{
	grep -q "branch -D `check_branch $1`" ${REVERT}
}

# Cleanup workdir (incl. submodules)
workdir_cleanup()
{
	for dir in . ${MODULES}; do
		cd ${dir}; ${GIT} clean -fdx; cd -
	done >> $LOG 2>&1 || exit 1
}

## UI HELPERS ##

echolog()
{
	echo $* | tee -a $LOG
}

# Search for $1 in $*
is_in()
{
	_search=$1
	shift
	_list=$*
	for _item in $_list; do
		test "$_search" = "$_item" && return 0
	done
	return 1
}

# Loop $2 until the users types one of $3+ or just hits enter; any other input
# is stored in prompt_var, which is initialised to $1
prompt()
{
	_tmp=" " # Not blank, and impossible to expect in $3+
	prompt_var=$1
	shift
	_cmd=$1
	shift
	_stop="$*"
	until test -z "$_tmp" || is_in "$_tmp" $_stop; do
		test "$_tmp" != " " && prompt_var=$_tmp
		echo "PROMPT: '$_cmd' [prompt_var=$prompt_var]" >> $LOG
		eval $_cmd
		read _tmp
		echo "INPUT: '$_tmp'" >> $LOG
	done
	test -z "$_tmp" || prompt_var=$_tmp
}


### MAIN TASKS ###

test_build()
{
	echolog "Testing build of candidate $CURVER for $OML_VER..."
	(./autogen.sh && \
		./configure && \
		${MAKE} CFLAGS="-Wall" distcheck
	) >> $LOG 2>&1 || exit 1

	echolog "Checking out installation..."
	${MAKE} DESTDIR=$PWD/_inst install >> $LOG 2>&1 || exit 1
	(
	echolog "Please make sure everything has been installed where it should have:"
	tree _inst
	) | less
	prompt Y 'echo -n "Did that look alright? [Y/n] "' Y YES y yes N NO n no
	is_in $prompt_var "Y YES y yes" || exit 1
}

update_changelog()
{
	prompt Y 'echo -n "Did you properly merge ChangeLogs from previous stable branch release/2.$((OML_MINOR - 1)) (this is your time to do so!) [y/N] "' Y YES y yes N NO n no
	is_in $prompt_var "Y YES y yes" || exit 1
	export CL=`${MKTEMP} ${PACKAGE_NAME}.${TS}.ChangeLog.XXX`
	echolog "Creating ChangeLog entry stub (in $CL)..."
	# XXX: Find the real predecessor here; at the moment, the needed 
	# comparisons only happen when creating packages
	(echo "$(date +%Y-%m-%d)  `${GIT} config --get user.name` <`${GIT} config --get user.email`>"; \
		echo "	* OML: Version ${OML_VER}"; \
		${GIT} log v2.$((OML_MINOR - 1)).0..HEAD  --pretty="		%s";
	) > $CL
	prompt ${EDITOR:-vi} 'echo -n "Editor? [$prompt_var] "'
	EDITOR=$prompt_var
	prompt Y '${EDITOR} $CL; echo; cat $CL; echo; echo -n "Done? [Y/n] "' Y YES y yes

	echolog "Adding new entry to ChangeLog..."
	(
	cat $CL;
	echo; \
		cat ChangeLog
	) > ChangeLog.new; mv ChangeLog.new ChangeLog

	echolog "Committing to git..."
	${GIT} add ChangeLog >> $LOG 2>&1
	${GIT} commit -sm "Update ChangeLog for ${OML_VER}" >> $LOG 2>&1
	echo ${GIT} checkout -B `check_branch` `check_branch`^ >> ${REVERT}
}

update_git()
{
	echolog "Tagging `${GIT} rev-parse HEAD` as v${OML_VER}..."
	# XXX: Ask for PGP ID?
	${GIT} tag -as v${OML_VER} -m "${SOFTWARE_NAME} v${OML_VER}"
	echo ${GIT} tag -d v${OML_VER} >> ${REVERT}

	if check_branch release/${VERSION} >/dev/null; then
		echolog "Branch release/${VERSION} already exists"
		if [ `check_branch` != "release/${VERSION}" ]; then
			echolog "'***' ... but is not currently checked out!"
			exit 1
		fi
	else
		echolog "Creating branch release/${VERSION}..."
		${GIT} checkout -b release/${VERSION} >> $LOG 2>&1
		echo ${GIT} branch -D release/${VERSION} >> ${REVERT}
	fi
	workdir_cleanup
	if [ -n "$MODULES" ]; then
		echolog "'***' Remember to tag submodules (${MODULES}) as appropriate before continuing, e.g.,"
		echolog "	git tag -sm \"XXX for OML ${VERSION}\" vXXX+oml${VERSION}"
	fi
}

create_tarball()
{
	prompt Y 'echo -n "Ready to create release tarball? [Y/n] "' Y YES y yes N NO n no
	is_in $prompt_var "Y YES y yes" || exit 1
	workdir_cleanup
	(./autogen.sh && \
		${GIT} checkout m4/gnulib-cache.m4; rm -rf autom4te.cache/; autoreconf -I /usr/share/aclocal && \
		./configure && \
		eval "${MAKE} distcheck `test -d $DEBPSQL && echo DISTCHECK_CONFIGURE_FLAGS=--with-pgsql-inc=$DEBPSQL`"
	) >> $LOG 2>&1 || exit 1
	TARBALL=`ls ${PACKAGE_NAME}-${OML_VER}.tar.gz`
	echolog "Upload ${PWD}/${TARBALL} to ${REDMINE_URL}/projects/${REDMINE_PROJECT}/files/new for version ${VERSION}"
}

prepare_packages()
{
	prompt Y 'echo -n "Did you properly merge packaging code from previous stable branches {debian,rpm,archlinux}/release/2.$((OML_MINOR - 1)) [y/N] "' Y YES y yes N NO n no
	is_in $prompt_var "Y YES y yes" || exit 1
	prompt ' ' 'echo -n "Confirm REDMINEID for the uploaded ${TARBALL} (hint: ${REDMINE_URL}/attachments/download/REDMINEID/${TARBALL}): [$prompt_var] "'
	REDMINEID=$prompt_var

	package_wrap debian mytestbed
	package_wrap rpm
	package_wrap archlinux
}

build_packages()
{
	# Build package
	${GIT} checkout release/${VERSION} >> $LOG 2>&1
	echolog -e "\nBuilding packages from `check_branch`..."

	workdir_cleanup
	(./autogen.sh && \
		${GIT} checkout m4/gnulib-cache.m4; rm -rf autom4te.cache/; autoreconf -I /usr/share/aclocal && \
		./configure --enable-packaging `test -d $DEBPSQL && echo --with-pgsql-inc=$DEBPSQL` && \
		make pkg-all # XXX We do not want concurency here, as it seems to break package-building
	) >> $LOG 2>&1 || exit 1

	echolog "Checking out packages"
	(
	echolog "Please make sure every package has been created as they should:"
	tree p-*
	) | less
	prompt Y 'echo -n "Did that look alright? [Y/n] "' Y YES y yes N NO n no
	is_in $prompt_var "Y YES y yes" || exit 1
}

build_obs()
{
	prompt Y 'echo -n "Build special tarball for OBS? [Y/n] "' Y YES y yes N NO n no
	is_in $prompt_var "Y YES y yes" || return 0
	SRC=$PWD
	OBS=`${MKTEMP} -d ${PACKAGE_NAME}.${TS}.obs.XXX`
	SPECIAL_TARBALL=${OBS}/obs.tar.gz
	echolog "Working in $OBS (will not be deleted in case of failure)..."
	cd $OBS
	find $SRC/p-debian \(						\
		-name ${PACKAGE_NAME}_${VERSION}\*.dsc			\
		-o -name ${PACKAGE_NAME}_${VERSION}*.diff.gz		\
		-o -name ${PACKAGE_NAME}_${VERSION}\*.orig.tar.gz	\
		-o -name ${PACKAGE_NAME}_${VERSION}\*.debian.tar.gz	\
		\) -exec cp -H {} ${PWD} \; || exit 1
	find $SRC/p-rpm \(						\
		-name \*.spec						\
		-o -name \*.tar.gz					\
		\) -exec cp -H {} ${PWD} \; || exit 1
	find $SRC/p-arch \(						\
		-name PKGBUILD						\
		\) -exec cp -H {} ${PWD} \; || exit 1
	tar czvhf ${SPECIAL_TARBALL} * >> $LOG 2>&1 || exit 1
	cd $SRC

	if [ -z $OML_TYPE ]; then
		echolog "[RELEASE] Upload ${SPECIAL_TARBALL} to ${OBS_URL}/package/add_file/${OBS_NS}/${PACKAGE_NAME}"
	else
		echolog "[TEST] Upload ${SPECIAL_TARBALL} to ${OBS_URL}/package/add_file/${OBS_NS_STAGING}/${PACKAGE_NAME}"
	fi

	# XXX: This is not the best place to do it, but it should do the job...
	ARCHSRC=`ls p-arch/${PACKAGE_NAME}-${VERSION}*.src.tar.* 2>/dev/null`
	test -z "$ARCHSRC" || echolog "ArchLinux source ${ARCHSRC} should be uploaded to AUR at https://aur.archlinux.org/submit/ "
}

# package_wrap DISTRO-BRANCH PKG-SUFFIX
#   package_wrap debian mytestbed
package_wrap()
{
	DISTRO=$1
	SUFFIX=$2

	OLD_EXTRA=
	OLD_EXTRAREV=
	OML_PKGEXTRA=
	OML_PKGVER=

	# Work out version
	echolog -e "\nPreparing ${DISTRO} package..."
	# We're going to change branches
	if check_branch ${DISTRO}/release/${VERSION} >/dev/null; then
		echolog "Branch ${DISTRO}/release/${VERSION} already exists, advancing..."
		${GIT} checkout ${DISTRO}/release/${VERSION} >> $LOG 2>&1
		${GIT} pull >> $LOG 2>&1
	elif check_branch origin/${DISTRO}/release/${VERSION} >/dev/null; then
		echolog "Branch origin/${DISTRO}/release/${VERSION} exists, creating remote-tracking branch..."
		# FIXME: Check that we are on it; or fail
		${GIT} checkout -b ${DISTRO}/release/${VERSION} --track origin/${DISTRO}/release/${VERSION} >> $LOG 2>&1
		echo ${GIT} branch -D ${DISTRO}/release/${VERSION} >> ${REVERT}
	else
		echolog "Creating branch ${DISTRO}/release/${VERSION} from ${DISTRO}/master..."
		${GIT} checkout -b ${DISTRO}/release/${VERSION} ${DISTRO}/master >> $LOG 2>&1
		echo ${GIT} branch -D ${DISTRO}/release/${VERSION} >> ${REVERT}
	fi
	PKGBRANCHES+="${DISTRO}/master ${DISTRO}/release/${VERSION} "

	OML_PKGVER=`echo $OML_VER | sed "s/\(rc\|pre\)/\~\1/"`
	OLDPKGVER=`${GIT} describe | sed s#${DISTRO}/v##`
	echolog "Latest package buildable from `check_branch`: ${OLDPKGVER}"
	COMP=0
	compare_ver ${OLDPKGVER} ${OML_VER} || COMP=$? # Capture failures without killing the shell
	case $COMP in
		255)
			OML_PKGEXTRA="${SUFFIX}1"
			echolog "Creating new $DISTRO package version ${OML_PKGVER}-${OML_PKGEXTRA}..."
			;;
		0)
			parse_ver OLD ${OLDPKGVER}
			OLD_EXTRA=${OLD_EXTRA:-${SUFFIX}}
			OLD_EXTRAREV=${OLD_EXTRAREV:-1}
			OML_PKGEXTRA=$OLD_EXTRA$((++OLD_EXTRAREV))
			echolog "Creating new $DISTRO package revision ${OML_PKGVER}-${OML_PKGEXTRA}..."
			;;
		1)
			prompt N 'echo -n "Releasing an old package? [y/N] "' Y YES y yes N NO n no
			is_in $prompt_var "Y YES y yes" || exit 1
			;;
	esac
	# FIXME: Ask user for confirmation

	package_$DISTRO

	# Git stuff
	echolog "Committing to git..."
	${GIT} commit -as -m "${SOFTWARE_NAME} ${DISTRO} package ${OML_PKGVER}" >> $LOG 2>&1
	echo ${GIT} checkout -B `check_branch` `check_branch`^ >> ${REVERT}
	echolog "Tagging `${GIT} rev-parse HEAD` as ${DISTRO}/v${OML_VER}-${OML_PKGEXTRA}..." # Git tags cannot contain tildes
	${GIT} tag -as ${DISTRO}/v${OML_VER}-${OML_PKGEXTRA} -m "${SOFTWARE_NAME} ${DISTRO} package ${OML_PKGVER}-${OML_PKGEXTRA}" >> $LOG 2>&1
	echo ${GIT} tag -d ${DISTRO}/v${OML_VER}-${OML_PKGEXTRA} >> ${REVERT}
	PKGTAGS+="tag ${DISTRO}/v${OML_VER}-${OML_PKGEXTRA} "
}

package_debian()
{
	echolog "Updating Debian ChangeLog..."
	${DCH} -D testing -v $OML_PKGVER-${OML_PKGEXTRA} "New upstream release ${OML_VER}." >> $LOG 2>&1
	less debian/changelog
	prompt Y 'echo -n "Did that look alright? [Y/n] "' Y YES y yes N NO n no
	is_in $prompt_var "Y YES y yes" || exit 1
}

package_rpm()
{
	echolog "Updating RPM SPEC..."
	SPECFILE=SPECS/${PACKAGE_NAME}.spec
	sed -i "s/^\(%define srcver[[:blank:]]\).*/\1	${OML_VER}/; \
		s/^\(%define pkgver[[:blank:]]\).*/\1	${OML_PKGVER}/; \
		s/^\(%define pkgvernotilde[[:blank:]]\).*/\1	`echo "${OML_PKGVER}"|sed s/\~//`/; \
		s/^\(%define redmineid\).*/\1	${REDMINEID}/; \
		s/^\(Release:\).*/\1	${OML_PKGEXTRA}/" \
		${SPECFILE}
	less $SPECFILE
	prompt Y 'echo -n "Did that look alright? [Y/n] "' Y YES y yes N NO n no
	is_in $prompt_var "Y YES y yes" || exit 1
}

package_archlinux()
{
	echolog "Updating ArchLinux PKGBUILD..."
	PKGBUILD=PKGBUILD.proto
	sed -i "s/^\(pkgver\).*/\1=${OML_PKGVER}/; \
		s/^\(_redmineid\).*/\1=${REDMINEID}/; \
		s/^\(_srcver\).*/\1=${OML_VER}/; \
		s/^\(pkgrel\).*/\1=${OML_PKGEXTRA}/" \
		${PKGBUILD}
	less $PKGBUILD
	prompt Y 'echo -n "Did that look alright? [Y/n] "' Y YES y yes N NO n no
	is_in $prompt_var "Y YES y yes" || exit 1
}


### MAIN LOGIC ###

LOG=`${MKTEMP} ${PACKAGE_NAME}.${TS}.log.XXX`
echo ${TS} > $LOG
REVERT=`${MKTEMP} ${PACKAGE_NAME}.${TS}.revert.XXX`
echo "${GIT} checkout `check_branch`" > ${REVERT}

echolog "Verbose logfile: ${LOG}; Revert file: ${REVERT}"

trap '(test $? != 0 && echo -e "\n*** Something went wrong; check $LOG" && echo -e "*** Revert changes to this repo with:\n${GIT} checkout -f master\n`tac ${REVERT}`") || echo -e "*** Changes can be reverted with:\n${GIT} checkout master\n`tac ${REVERT}`" >> $LOG' EXIT SIGINT

workdir_cleanup
if [ ! -e build-aux/git-version-gen ]; then
	echolog "Regenerating build-aux/git-version-gen..."
	(./autogen.sh && \
		${GIT} checkout m4/gnulib-cache.m4 && \
		rm -rf autom4te.cache/ && \
		autoreconf -I /usr/share/aclocal
	) >> $LOG 2>&1 || exit 1
fi
CURVER=`build-aux/git-version-gen .`

# Get current version, and guess next version
eval `parse_ver OML $CURVER`
case "$OML_TYPE" in
	rc)
		RC=${OML_TYPEREV:-1}
		OML_TYPE=rc$((++RC))
		;;
	pre)
		OML_TYPE=rc
		;;
	"")
		# XXX: Untested branch
		OML_REV=$((OML_REV+1))
		OML_TYPE=rc
		;;
	*)
		echo "Unknown release type $OML_TYPE; ignoring..."
		;;
esac

OML_VER=${OML_MAJOR}.${OML_MINOR}.${OML_REV}${OML_TYPE}
prompt $OML_VER 'echo -n "Current version: $CURVER; release version? [$prompt_var] "'
eval `parse_ver OML "$prompt_var"`
export REPO=origin \
	OML_VER=$prompt_var \
	VERSION=${OML_MAJOR}.${OML_MINOR} # Useful for branches

test_build
update_changelog
update_git
create_tarball

prompt Y 'echo -n "Make distribution packages? [Y/n] "' Y YES y yes N NO n no
if is_in $prompt_var "Y YES y yes"; then
	prepare_packages
	build_packages || exit 1
	build_obs
fi

PUSH="${GIT} push ${REPO} master release/${VERSION} tag v${OML_VER} ${PKGBRANCHES} ${PKGTAGS}"
prompt N 'echo -n "Push changes upstream to $REPO? [y/N] "' Y YES y yes N NO n no
if is_in $prompt_var "Y YES y yes"; then
	eval $PUSH --set-upstream 2>&1 | tee -a $LOG
	echolog -e "'***' If ${REPO} is not he official MyTestbed repository, don't forget to push there too:\n${PUSH}"
else
	echolog -e "'***' Run the following when ready (changing ${REPO} for the official MyTestbed repository if need be):\n${PUSH}"
fi

echolog -e "All done! Now tell the world:\n - Finalise the Release Notes (using the ChangeLog at $CL);\n - Add a news item in the OML news (${REDMINE_URL}/projects/oml/news);\n - Send an email to the <oml-user@mytestbed.net> mailing list (containing the ChangeLog and a link to the source)."
echo "Logfile $LOG kept."
