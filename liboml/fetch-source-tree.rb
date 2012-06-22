#!/usr/bin/env ruby

def arch_pkgbuild_version()
  f = File.new("../../PKGBUILD")
  begin
    while line = f.readline do
      if (line =~ /^pkgver=([0-9]+\.[0-9]+\..*)/ ) then
	return $1
      end
    end
    $stderr.print "ERROR:  PKGBUILD does not appear to have any version number in it.\n"
    Kernel.exit 1
  ensure
    f.close
  end
end

#
# Translate a version string into a git treeish.  If the version
# number is a release version number, such as '2.3.4', then the
# treeish will be a tag named 'v2.3.4'.  If the version number is a
# pre-release version number, intended for testing only, then the
# treeish will be the head of the master branch, i.e. 'master'.  The
# pre-release version numbers look like '2.3.pre4'.
#
# If the version is a release version, there must be a corresponding
# tag, otherwise this function exits the script with error status.
#
def treeish_from_version(version)
  if (version =~ /[0-9]+\.[0-9]+\.pre.*/) then
    return "master"
  else
    tags = `git tag -l`

    if (tags =~ /v#{version}/) then
      return "v#{version}"
    else
      $stderr.print "ERROR:  no tag found for v#{version}\n"
      Kernel.exit 1
    end
  end
end

#
# Check the source version (from configure.ac) against the specified
# version.  If there is a mis-match, exit the script with error
# status.
#
def check_source_version(version)
  File.open("configure.ac").each do |line|
    if (line =~ /^AC_INIT\(\[oml2\], \[(.*)\],.*/) then
      source_version = $1
      if (source_version != version) then
        $stderr.print "ERROR:  Package version from configure.ac (#{source_version}) does not match PKGBUILD version (#{version})\n"
        Kernel.exit 1
      else
        puts "Package version check OK"
        return true
      end
    end
  end
  $stderr.print "ERROR:  couldn't find a valid AC_INIT line in configure.ac\n"
  Kernel.exit 1
end

def run
  spec_version = arch_pkgbuild_version
  treeish = treeish_from_version(spec_version)

  puts "OML version #{spec_version}\n"
  puts "Building package from tree at '#{treeish}'\n"

  fetch_result = system("git archive #{treeish} | tar xf -")
  if not fetch_result or $? != 0 then
    $stderr.print "ERROR:  could not fetch tree from #{treeish}:  command failed (#{$?})\n"
    Kernel.exit 1
  end

  #
  # Check that the version number in configure.ac matches the version from PKGBUILD.
  #
  check_source_version(spec_version)
end


run if $PROGRAM_NAME == __FILE__
