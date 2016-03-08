OML: The O? Measurement Library
===============================

OML is an instrumentation tool that allows application writers to define
customisable measurement points inside new or per-existing applications.
Experimenters running the applications can then direct the *measurement
streams* (MS) from these *measurement points* (MP) to storage in a
remote measurement database.

The OML reporting chain consists in several elements:
1. the reporting library (C, Ruby or Python),
2. the collection server,
3. the optional proxy, for disconnected experiments.

This documentation mainly focuses on configuring the server, and
instrumenting applications. Please refer to file INSTALL for
instructions on building these programs.


Running the Server
------------------

The oml2-server(1) receives measurements streams in the OML protocol
(either binary marshalling or text mode), and stores them in a database
backend. Currently, SQLite3 is the default, and can be replaced
PostgreSQL.

### SQLite

SQLite [sqlite] is a lightweight SQL file-based database system. Each
database is stored as an independent file in the filesystem.  The
oml2-server(1) uses this backend by default.

Relevant command line arguments are the following:
* `-D DIR` or `--data-dir=DIR` specifies to create the database files in `DIR`;
* `--user=UID` and `--group=GID` specify as which user (resp. group) the
  server is run on the system and, consequently, who owns the created
  database files.

Running the server as

    $ oml2-server -D /path/to/my/databases --user=oml2 --group=oml2

will instruct it to drop privileges to user:group `oml2:oml2`, and
create and update databases in `/path/to/my/databases`.

### PostgreSQL

The database backend can also be PostgreSQL [pgsql]. In this case,
additional connection information must be passed to the server so it can
properly connect to the server, and create databases.

First, a user, say `oml`, with the appropriate permissions has to be
created within PostgreSQL. Note that this username has nothing to do
with the system user discussed in the previous section.

    $ createuser -U DBADMIN --pwprompt --no-superuser \
	    --createdb --no-createrole --no-replication oml

`DBADMIN` is any pre-existing user allowed to create roles in the
database.  PostgreSQL's default `postgres` will usually work.  The
`--no-replication` argument is only valid with PostgreSQL-9.2. In case
of error, simply remove it. A password for the newly created database
user will be requested. Alternatively, the role can be created through
and interactive console.

    $ psql -U DBADMIN
    psql (9.3.2)
    Type "help" for help.
    
    DBADMIN=# CREATE USER oml WITH PASSWORD 'oml';
    CREATE ROLE
    DBADMIN=# ALTER ROLE oml WITH CREATEDB;
    ALTER ROLE

It is now possible to start the server. Several Postgre-specific
options are available (see INSTALL for details on compiling the PostgreSQL
backend in).

* `--pg-user=USER` specifies the name of the user just created (default:
  `oml`);
* `--pg-pass=PASS` specifies the password of the user just created
  (default: empty);
* `--pg-host=HOST` specifies the host on which the database server runs
  (default: `localhost`;
* `--pg-port=PORT` specifies the service or port on which the database
  server expects connections (default: 5432).

Of course, it is also necessary to specify that a different database
backend is used. This is done with the `-b postgresql` option.
Altogether, the server can be run as

    $ oml2-server --user=oml2 --group=oml2 -b postgresql \
	    --pg-user=oml --pg-pass=correcthorsebatterystaple \
	    --pg-host=db.example.com --pg-port=postgresql

### Server Hook

The collection server has a rudimentary support for hook scripts to
perform some actions when specific conditions occurs. Currently, only
database-close and server-exit are supported. The former can be use for,
e.g., backup of measurement databases. Example script
server/oml2-server-hook.sh uses the `DBCLOSED` event to push experiment
databases to IRODS [irods].

### Integration in Distributions

Depending on your distribution, there are various ways to change the
oml2-server(1)'s command line arguments to fit it to your needs.

#### Debian

In Debian-based system, file `/etc/default/oml2-server` can be modified
to adjust the parameters of the server. The server can then be
(re)started with:

    $ sudo service oml2-server restart

#### ArchLinux

ArchLinux uses systemd(1) as its basic system process. A service unit
configuration is provided with the package in AUR [aur]. It is installed
in `/usr/lib/systemd/system/oml2-server.service` and uses the SQLite
backend in `/var/oml2`. If you want to override these parameters (e.g.,
the command line), you need to copy this file to `/etc/systemd/system/`,
and modify that copy.  Once done, you'll need to refresh systemd's
configuration to take the override into account:

     $ sudo systemctl reenable oml2-server.service

You can then (re)start the service with its new parameters.

    $ sudo systemctl (re)start oml2-server.service

Running the Applications
------------------------

liboml2(1) extends the command-line parameters of an instrumented
application with options specific to reporting. Most of these options
can also be passed as environment variables.

OML has the concept of an *experimental domain*, within which several
*applications* run on one or more *node*. They generate measurement
traffic which is collected and stored together by the oml2-server(1).
Each experimental domain has its own database, with measurements from
all the nodes and applications collected there. More details on these
concepts are available at [oml-gif].

The most important command line arguments (and environment variables) for an
instrumented application are the following.

* `--oml-id` (or `OML_ID`) is the identifier of the node;
* `--oml-domain` (or `OML_DOMAIN`) is the name of the experimental
  domain;
* `--oml-collect` (or `OML_COLLECT`) is a URI pointing to the collection
  server;
* each application also defines its own name (see next section).

If `--oml-collect` is not given, a file will be created in the current
directory containing text-mode OML protocol [oml-textmode], which can be
fed into a server at a later time.

Applications can also be configured more finely through the use of a
liboml2.conf(5) file.

More details on these topics are available at [oml-conf].


Instrumenting an Application
----------------------------

Instrumentation of an application (new or pre-existing) is done through
oml2-scaffold(1). Based on an concise description of the desired
measurement points, this helper generates the relevant header files and
appropriate functions to inject measurement samples into a measurement
stream.

Essentially, instrumenting an applications therefore consists in writing
the application description. A template for this file can be generated
for application APPNAME with

    $ oml2-scaffold --app APPNAME

After adjusting this template to contain the desired schemas for the
various MPs of the application, a header containing helper functions is
then generated with

    $ oml2-scaffold --oml APPNAME.rb

This creates file `APPNAME_oml.h` which can be directly included in the
application source to provide the oml_inject_MPNAME(3) helpers.

If writing an application from scratch, a template main() function and
popt(3)-based option parsing code can also be generated with the
`--main` and `--opts` options.

It is not recommended to manually change the `APPNAME_oml.h` nor
`APPNAME_popt.h` as they are automatically generated. It is best to
include rules for the build system to generate them dynamically as part
of the build (`BUILT_SOURCES`, in the Automake linguo). As a matter of
fact, oml2-scaffold(1) can also generate a rudimentary Makefile which,
based on APPNAME.rb and APPNAME.c, can generate all other necessary
files, and build the application.  This is done with the `--make`
option.

See the example, and its documentation in example/liboml2/README, as
well as the oml2-scaffold(1) and oml_inject_MPNAME(3) manpages for more
details.

Some native implementations of the text mode are also available. Though
they are built from this same source tree, it might be easier to install
them directly from the language-specific module repositories.
* Python: OML4Py [oml4py];
* Ruby: OML4R [oml4r];

Using the Proxy Server
----------------------

The oml2-proxy-server(1) can be used for situations where a connection
to the collection server is temporarily unavailable. It will cache
measurement streams until instructed to forward them to the actual
collection server.

The proxy has two main states: paused (at startup) and running. It
expects instructions on its standard input to change states:
`OMLPROXY-PAUSE`, `OMLPROXY-RESUME`.

Its most important command line arguments allow to specify the
collection server's address (`-a ADDR` or `--dstaddress=ADDR`) and port
(`-p PORT` or `--dstport=PORT`).

Some more details are available at [oml-proxy].


Reporting Bugs and Contributing to Development
----------------------------------------------

External contributions are most welcome. Be they in the form of bug
reports or feature request on the tracker [oml-tracker]. You can also
register to the mailing list [oml-ml], where you should direct all your
questions, comments or contributions. The archives can be consulted at
[oml-ml-archives].

The official source tree is stored in a Git repository [oml-git]. It can
be cloned without restrictions. To re-integrate patches (fixing bugs or
introducing new features), the best way is to make the Git clone
containing the changes public, and submit a pull request.

    $ git request-pull origin/release/2.x http://url/of/your/repo \
        # Assuming you started working from origin/release/2.x

Patches formatted by Git are also a good way to do so.

    $ git format-patch origin/release/2.x..HEAD \
        # Assuming you started working from origin/release/2.x

In either cases requests and patches should be sent to the mailing list
[oml-ml], or a ticket  created about them in the issue tracker
[oml-tracker].


Installing from Distribution Packages
-------------------------------------

Though you can build OML from source (see INSTALL), pre-built packages
for a few distributions are already available.

### Debian and Ubuntu

An archive is maintained on the OpenSuSE Build System, with packages for the
latest Debian and Ubuntu distributions. You can add it to your
`/etc/apt/sources.list` as

    deb http://download.opensuse.org/repositories/home:/cdwertmann:/oml/xUbuntu_12.10/ ./

then run

    sudo apt-get update
    sudo apt-get install oml2-server liboml2

to install the required components. You'll need `liboml2-dev` if you
want to build instrumented applications.

### ArchLinux

ArchLinux packages are available from the Arch User Repository [aur].
You can build and install the latest package directly with Yaourt
[aur-yaourt].

    $ sudo yaourt -S oml2

The stable package is available at [aur-oml2], while a development
version can be obtained from [aur-oml2-git].

### RPM-based (CentOS, Fedora)

RPM packages are also built on the OpenSuSE platform. They are available
there [opensuse-oml], as well as the instructions to install them on
your system.


License
-------

Copyright 2007-2016 National ICT Australia (NICTA), Australia

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


[sqlite]: http://sqlite.org/
[pgsql]: http://www.postgresql.org/
[irods]: https://www.irods.org/
[oml-gif]: http://oml.mytestbed.net/projects/oml/wiki/OML_Generic_Interface
[oml-textmode]: http://oml.mytestbed.net/projects/oml/wiki/Description_of_Text_Protocol
[oml-conf]: http://oml.mytestbed.net/projects/oml/wiki/Configuring_OML_Client_Applications
[oml-proxy]: http://oml.mytestbed.net/projects/oml/wiki/Proxy_Server
[oml-tracker]: http://oml.mytestbed.net/projects/oml/issues
[oml-ml]: mailto:oml-user@lists.nicta.com.au
[oml-ml-archives]: http://oml.mytestbed.net/tab/show/oml
[oml-git]: git://git.mytestbed.net/oml.git
[oml4py]: http://pypi.python.org/pypi/oml4py/
[oml4r]: https://rubygems.org/gems/oml4r/
[aur]: https://aur.archlinux.org/
[aur-yaourt]: https://aur.archlinux.org/packages/yaourt/
[aur-oml2]: https://aur.archlinux.org/packages/oml2/
[aur-oml2-git]: https://aur.archlinux.org/packages/oml2-git/
[opensuse-oml2]: http://software.opensuse.org/download.html?project=home:cdwertmann:oml&package=oml2
