# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_PREREQ([2.71])
AC_INIT([lux],[0.9.6a],[ramar@ramarcollins.com])
AC_CONFIG_SRCDIR([src/config.h.in])

# Define files to perform substitutions on
#AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile src/config.h share/package.Makefile etc/etc.systemd.system.lux.service share/config.server.lua])
#src/lua/tests/Makefile])
#src/config.h])
#                 example/Makefile
#                 example/c/Makefile
#                 tests/Makefile
#                 vendor/lua-5.4.3/Makefile
#                 vendor/lua-5.4.3/src/Makefile
#                 vendor/smtp-client/Makefile])

# Variables that would have a default variable in the Makefile
cc="gcc"
debug_flags=
default_access_file_name="access.log"
default_domain="localhost"
default_error_file_name="error.log"
disable_mysql=1
disable_oracle=1
disable_pgsql=1
disable_sqlite3=0
disable_sqlserver=1
disable_tls=0
examples="examples"
include_filter_c=1
include_filter_echo=1
include_filter_lua=1
include_filter_static=1
include_sendfile=1
initsys="systemd"
ld_flags="-ldl -lpthread -lm"
local_lua=0
local_sqlite3=0
local_tls=0
max_conn_count=128
max_simultaneous_connections=64
max_threads=1024
modelflags="HTHREAD_H"
objdeps=
poll_interval=1000000
preamble_size=2048
protocol=
read_max=8388608
read_size=4096
socket_backlog=4096
srvgroup="http"
srvuser="http"
stack_size=100000
svcpath=
systemd=yes
sysv=no
unit_file=${srcdir}/etc/etc.systemd.system.lux.service
with_json=1
with_msgpack=0
with_xml=0
write_size=8192
wwwport=2000

# Directories that have built-in values
# should default to /usr/local/etc, but is overriden with /etc for something like Debian
#confdir=${sysconfdir}/lux
confdir=
default_svcpath="/etc/systemd/system"
# Local state dir is the base for logging.  Should default to /usr/local/var, but this can also be /var
#logdir=${localstatedir}/log/lux
logdir=
# Data root dir is the "share dir" (which is /usr/local/share for docs, and ro files)
#sharedir=${datarootdir}/lux
sharedir=
sysvpath="/etc/init.d"
#wwwroot=${localstatedir}/www/lux
# wwwroot (or wwwrootdir) is the home for all sites.  This could probably also go in /var
wwwroot=${localstatedir}/www/lux

# Certtool/certificate stuff
default_organization="RWK Apps, LLC"
default_certificate_state="NC"
default_certificate_country="US"
default_common_name="Antonio R. Collins II"
default_expiration_days=365
default_server_name="localhost"
default_server_ip="127.0.0.1"

# Handle common easy options
AC_ARG_WITH([default-protocol], AS_HELP_STRING([--with-default-protocol=\[http,https\]],[Use this protocol by default when starting a new server]), [protocol=${withval}])
#AC_SUBST(protocol)
if test ! -z $protocol; then
	if ! test "${protocol}" == "https" && ! test "${protocol}" == "http"; then
		printf "Protocol value ${protocol} is invalid\n"
		exit 1
	fi
fi

AC_ARG_WITH([server-backlog], AS_HELP_STRING([--with-server-backlog=n],[Set server backlog to a different value]), [socket_backlog=${withval}])
AC_SUBST(socket_backlog)

AC_ARG_WITH([max-conn-per-context], AS_HELP_STRING([--with-max-conn-per-context=n],[Set maximum connection count per context]), [max_conn_count=${withval}])
AC_SUBST(max_conn_count)

AC_ARG_WITH([max-threads-per-context], AS_HELP_STRING([--with-max-threads-per-context=n],[Set maximum thread count per context (cannot be less than maximum connections)]), [max_threads=${withval}])
AC_SUBST(max_threads)

AC_ARG_WITH([max-write-buffer], AS_HELP_STRING([--with-max-write-buffer=n],[Set default server port for listening]), [write_size=${withval}])
AC_SUBST(write_size)

AC_ARG_WITH([www-port], AS_HELP_STRING([--with-www-port=PORT],[Set default server port for listening]), [wwwport=${withval}])
AC_SUBST(wwwport)

AC_ARG_WITH([www-user], AS_HELP_STRING([--with-www-user],[Set default user for managing the web server]), [srvuser=${withval}])
AC_SUBST(srvuser)

AC_ARG_WITH([www-group], AS_HELP_STRING([--with-www-group],[Set default group for managing the web server]),[srvgroup=${withval}])
AC_SUBST(srvgroup)


# Handle cases in which $prefix isn't specified at all
if test "x${prefix}" == "xNONE"; then
	prefix=/usr/local
fi

# Ascertain the configuration directory
#AC_ARG_WITH([conf-dir], AS_HELP_STRING([--with-conf-dir],[Set a different configuration directory]), [confdir=${withval}])
if test -z $confdir; then
	# Expand $prefix so substitutions work for header files.  
	# NOTE: this will break builds that bypass ./configure.
	if test "`echo ${sysconfdir} | cut -c1`" == "$"; then
		sysconfdir=`echo $sysconfdir | sed 's/${prefix}//' | sed "s;^;$(echo $prefix);"` 
	fi
	#echo $sysconfdir
	confdir=`echo ${sysconfdir}/lux`
fi
AC_SUBST(confdir)


# Ascertain the logging directory
#AC_ARG_WITH([log-dir], AS_HELP_STRING([--with-log-dir],[Set access and error log directory]), [logdir=${withval}])
if test -z $logdir; then
	# Expand $prefix so substitutions work for header files.  
	# NOTE: this will break builds that bypass ./configure.
	if test "`echo ${localstatedir} | cut -c1`" == "$"; then
		localstatedir=`echo $localstatedir | sed 's/${prefix}//' | sed "s;^;$(echo $prefix);"` 
	fi
	#echo $logdir
	logdir=`echo ${localstatedir}/log/lux`
fi
AC_SUBST(logdir)


# Ascertain the share directory
#AC_ARG_WITH([share-dir], AS_HELP_STRING([--with-share-dir],[Set a different configuration directory]), [sharedir=${withval}])
if test -z $sharedir; then
	# Expand $prefix so substitutions work for header files.  
	# NOTE: this will break builds that bypass ./configure.
	if test "`echo ${datarootdir} | cut -c1`" == "$"; then
		datarootdir=`echo $datarootdir | sed 's/${prefix}//' | sed "s;^;$(echo $prefix);"` 
	fi
	#echo $sharedir
	sharedir=`echo ${datarootdir}/lux`
fi
AC_SUBST(sharedir)


# Ascertain the wwwrootdir directory
AC_ARG_WITH([www-root], AS_HELP_STRING([--with-www-root=DIR],[Set default web directory for applications]), [wwwroot=${withval}])
AC_SUBST(wwwroot)


AC_ARG_WITH([cc], AS_HELP_STRING([--with-cc=CC],[Set a different C compiler as the default]), [cc=${withval}])
AC_SUBST(cc)

# Disable or enable examples
AC_ARG_ENABLE([examples], AS_HELP_STRING([--disable-examples],[Skip examples when installing]),[examples=none])
AC_SUBST(examples)

# Explicitly disable systemd 
AC_ARG_ENABLE([systemd], AS_HELP_STRING([--disable-systemd],[Do not use systemd units for daemon management]), [systemd=no;initsys=none])

# Use sysv init scripts
AC_ARG_ENABLE([sysv], AS_HELP_STRING([--enable-sysv],[Use sysv-style init scripts for daemon management]), [sysv=yes;systemd=no;initsys=sysv])

# Disable sendfile
AC_ARG_ENABLE([sendfile], AS_HELP_STRING([--disable-sendfile],[Do not use sendfile for static files]), [include_sendfile=0])


# Enable debugging
AC_ARG_ENABLE([debugging], AS_HELP_STRING([--enable-debugging],[Compile with debugging flags enabled]), [debug_flags='-g -DDEBUG_H'])

# If systemd is enabled, make sure that we actually found something...
#AS_IF([test "x$systemd" == xyes],
#	[
#	AC_ARG_WITH([systemd-unit-path],
#	AS_HELP_STRING([--with-systemd-unit-path],[Use an alternate systemd unit file path for installation]),
#	[svcpath=${withval}])
#	]
#)

# Check for Asan support and enable flags if so
if test "x${use_asan}" == xyes; then
	# Turn on debugging automatically...
	debug_flags+=" -O0 -fsanitize=address -fsanitize-undefined-trap-on-error"
fi

# Check for systemd support
if test "x${systemd}" == xyes; then
	AC_ARG_WITH([systemd-unit-path],
		AS_HELP_STRING([--with-systemd-unit-path],[Use an alternate systemd unit file path for installation]),
		[svcpath=${withval}]
	)
	initsys="systemd"
fi


# If sysv is in use instead, try to use those conventions
#AS_IF([test "x${sysv}" == xyes],[svcpath=${sysvpath}; initsys="sysv"])
if test "x${sysv}" == xyes; then
	svcpath=${sysvpath}
	initsys="sysv"
fi

# Finally, if there is no service file path, set with the basic one.
#AS_IF([test -z "${svcpath}"],[svcpath=${default_svcpath}])
if test -z "${svcpath}"; then
	svcpath=${default_svcpath}
fi

# Checks for programs.
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_PROG_SED

# Do a custom check for gzip
AC_CHECK_PROG([GZIP],[gzip],[yes],[no],,[/usr/bin/gzip])

# Do a custom check for gzip
AC_CHECK_PROG([TAR],[tar],[yes],[no],,[/usr/bin/tar])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h archive.h dlfcn.h fcntl.h float.h inttypes.h limits.h locale.h malloc.h math.h netdb.h netinet/in.h pthread.h stddef.h stdint.h stdlib.h string.h strings.h sys/file.h sys/ioctl.h sys/mount.h sys/param.h sys/socket.h sys/statvfs.h sys/time.h unistd.h utime.h])

# Checks for libraries.
# NOTE: Checks for -lARGUMENT_1 and that ARGUMENT_2 exists in ARGUMENT_1
AC_CHECK_LIB([dl], [dlopen])
AC_CHECK_LIB([m], [ceil])
AC_CHECK_LIB([pthread], [pthread_create])
AC_CHECK_LIB([archive], [archive_entry_new])
# TODO: You will need this, just not right now
#AC_CHECK_LIB([ssl], [main])
# TODO: Do you need this?
#AC_CHECK_LIB([gcov], [main])
# TODO: Do you need this?
#AC_CHECK_LIB([crypto], [main])

# Find Lua headers or use the local version
# FIXME: Add option for static vs dynamic
AC_ARG_ENABLE([local-lua], AS_HELP_STRING([--enable-local-lua],[Use the provided Lua libraries]), [local_lua=1])
if test ${local_lua} == 1; then
	objdeps+="lib/liblua.a "
else
	# Find Lua header and library
	LUA_HEADER_ERRMSG="Lua headers were not found and lux needs them to support TLS"
	AC_CHECK_HEADER([lua.h], [], AC_MSG_FAILURE([${LUA_HEADER_ERRMSG}]))
	LUA_LIB_ERRMSG="Lua library not found and lux needs this library for Lua support"
	# FIXME: Replace `main' with a function in `-lgnutls':
	AC_CHECK_LIB([lua], [main], [], AC_MSG_FAILURE(${LUA_LIB_ERRMSG}))
	ld_flags+=" -llua"
fi


# Find SQLite3 headers or use the local version
AC_ARG_ENABLE([local-sqlite3], AS_HELP_STRING([--enable-local-sqlite3],[Use the provided SQLite3 library]), [local_sqlite3=1])
if test ${local_sqlite3} == 1; then
	objdeps+="vendor/sqlite3.o "
else
	# Find SQLite3 header
	SQLITE3_HEADER_ERRMSG="SQLite3 headers were not found"
	AC_CHECK_HEADER([sqlite3.h], [], AC_MSG_FAILURE([${SQLITE3_HEADER_ERRMSG}]))
	SQLITE3_LIB_ERRMSG="SQLite3 library not found"
	# FIXME: Replace `main' with a function in `-lsqlite3':
	AC_CHECK_LIB([sqlite3], [main], [], AC_MSG_FAILURE(${SQLITE3_LIB_ERRMSG}))
	ld_flags+=" -lsqlite3"
fi


# Check for TLS support and find GnuTLS header and libs if present; and add flags
AC_ARG_ENABLE([tls-support], AS_HELP_STRING([--disable-tls-support],[Disable TLS support]), [disable_tls=1])
AC_ARG_ENABLE([local-tls], AS_HELP_STRING([--enable-local-tls],[Use the provided GnuTLS libraries]), [local_tls=1])
if test ${disable_tls} == 0; then
	if test -z $protocol; then
		protocol="https"
	fi
	if test ${local_tls} == 1; then
		#build the local one
		objdeps+="lib/libgnutls.a "
	else
		GNUTLS_HEADER_ERRMSG="GnuTLS headers were not found and lux needs them to support TLS"
		AC_CHECK_HEADER([gnutls/gnutls.h], [], AC_MSG_FAILURE([${GNUTLS_HEADER_ERRMSG}]))
		GNUTLS_LIB_ERRMSG="GnuTLS library not found and lux needs this library to support TLS"
		# FIXME: Replace `main' with a function in `-lgnutls':
		AC_CHECK_LIB([gnutls], [main], [], AC_MSG_FAILURE(${GNUTLS_LIB_ERRMSG}))
		ld_flags+=" -lgnutls"
	fi
else
	# TLS support won't be there, so force 'http'
	printf "Forcing protocol 'http', TLS support was not included.\n"
	protocol="http"
fi

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_UID_T
AC_TYPE_INT32_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_CHECK_MEMBERS([struct stat.st_blksize])
AC_STRUCT_ST_BLOCKS
AC_CHECK_MEMBERS([struct stat.st_rdev])
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_FUNC_STRCOLL
AC_FUNC_STRERROR_R
AC_FUNC_STRTOD
AC_CHECK_FUNCS([clock_gettime fdatasync floor getpagesize gettimeofday inet_ntoa localtime_r memchr memmove memset mkdir realpath select setenv setlocale socket strcasecmp strchr strcspn strdup strerror strpbrk strrchr strspn strstr strtoul utime])

cat << EOF

*** lux configured with: ***

Web root directory: $wwwroot
Default port: $wwwport
Default server user: $srvuser
Default server group: $srvgroup
Default logging directory: $logdir
Default configuration directory: $confdir
Default share directory: $sharedir
Default protocol: $protocol
Init system: $initsys
Daemon service file path: `test ${initsys} == "none" && echo "none" || echo $svcpath`
Chosen threading model: $modelflags
Default domain: $default_domain
Maximum thread usage (per context): $max_threads
Maximum connection count (per context): $max_conn_count
Server backlog count: $socket_backlog
TLS included: `test ${disable_tls} -eq 0 && echo "yes" || echo "no"`
Selected LD_FLAGS: $ld_flags
Using local SQLite3: `test ${local_sqlite3} -eq 1 && echo "yes" || echo "no"`
Using local Lua: `test ${local_lua} -eq 1 && echo "yes" || echo "no"`
Using local GnuTLS: `test ${local_tls} -eq 1 && echo "yes" || echo "no"`
Debugging flags: $debug_flags

EOF

# Replace all of these before generation
#AC_SUBST(default_svcpath)
AC_SUBST(debug_flags)
AC_SUBST(default_access_file_name)
AC_SUBST(default_domain)
AC_SUBST(default_error_file_name)
AC_SUBST(disable_mysql)
AC_SUBST(disable_oracle)
AC_SUBST(disable_pgsql)
AC_SUBST(disable_sqlite3)
AC_SUBST(disable_sqlserver)
AC_SUBST(disable_tls)
AC_SUBST(include_filter_c)
AC_SUBST(include_filter_echo)
AC_SUBST(include_filter_lua)
AC_SUBST(include_filter_static)
AC_SUBST(include_sendfile)
AC_SUBST(initsys)
AC_SUBST(ld_flags)
AC_SUBST(logdir)
AC_SUBST(max_conn_count)
AC_SUBST(max_simultaneous_connections)
AC_SUBST(max_threads)
AC_SUBST(modelflags)
AC_SUBST(objdeps)
AC_SUBST(poll_interval)
AC_SUBST(preamble_size)
AC_SUBST(protocol)
AC_SUBST(read_max)
AC_SUBST(read_size)
AC_SUBST(socket_backlog)
AC_SUBST(srvgroup)
AC_SUBST(srvuser)
AC_SUBST(stack_size)
AC_SUBST(svcpath)
AC_SUBST(systemd)
AC_SUBST(sysv)
AC_SUBST(sysvpath)
AC_SUBST(unit_file)
AC_SUBST(with_json)
AC_SUBST(with_msgpack)
AC_SUBST(with_xml)
AC_SUBST(write_size)
AC_SUBST(wwwport)
AC_SUBST(wwwroot)

AC_OUTPUT
