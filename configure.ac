# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_PREREQ([2.71])
AC_INIT([hypno],[0.3.7c],[ramar@ramar.work])
AC_CONFIG_SRCDIR([example/config.lua])

# Define files to perform substitutions on
#AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile src/config.h])
#src/lua/tests/Makefile])
#src/config.h])
#                 example/Makefile
#                 example/c/Makefile
#                 tests/Makefile
#                 vendor/lua-5.4.3/Makefile
#                 vendor/lua-5.4.3/src/Makefile
#                 vendor/smtp-client/Makefile])

# Variables that would have a default variable in the Makefile
wwwroot="/var/www"
wwwport=80
srvuser="http"
srvgroup="http"
logdir="/var/log/hypno"
svcpath=
default_svcpath="/etc/systemd/system"
initsys="systemd"
systemd="yes"
sysv="no"
sysvpath="/etc/init.d"
modelflags="HTHREAD_H"
default_domain="localhost"
poll_interval=1000000
preamble_size=2048
read_size=4096
read_max=8388608
write_size=8192
stack_size=100000
max_threads=1024
socket_backlog=4096
default_error_file_name="error.log"
default_access_file_name="access.log"
max_simultaneous_connections=64
disable_sqlite3=0
disable_mysql=1
disable_pgsql=1
disable_sqlserver=1
disable_oracle=1
with_xml=0
with_json=1
with_msgpack=0
disable_tls=0
include_filter_c=1
include_filter_lua=1
include_filter_static=1
include_filter_echo=1
ld_flags="-ldl -lpthread -lm"
cc="gcc"
debug_flags=
local_sqlite3=0
local_lua=0
local_tls=0
objdeps=

# CLI stuff
confdir="/etc/hypno"
sharedir="/usr/local/share/hypno"

# Certtool/certificate stuff
default_organization="Ironhead, Inc."
default_certificate_state="NC"
default_certificate_country="US"
default_common_name="Antonio R. Collins II"
default_expiration_days=365
default_server_name="localhost"
default_server_ip="127.0.0.1"

# Handle common easy options
AC_ARG_WITH([www-root], AS_HELP_STRING([--with-www-root=DIR],[Set default web directory for applications]), [wwwroot=${withval}])
AC_SUBST(wwwroot)

AC_ARG_WITH([www-port], AS_HELP_STRING([--with-www-port=PORT],[Set default server port for listening]), [wwwport=${withval}])
AC_SUBST(wwwport)

AC_ARG_WITH([server-user], AS_HELP_STRING([--with-server-user],[Set default user for managing the web server]), [srvuser=${withval}])
AC_SUBST(srvuser)

AC_ARG_WITH([server-group], AS_HELP_STRING([--with-server-group],[Set default group for managing the web server]),[srvgroup=${withval}])
AC_SUBST(srvgroup)

AC_ARG_WITH([log-dir], AS_HELP_STRING([--with-log-dir],[Set access and error log directory]), [logdir=${withval}])
AC_SUBST(logdir)

# Figure out the configuration directory
AC_ARG_WITH([conf-dir], AS_HELP_STRING([--with-conf-dir],[Set a different configuration directory]), [confdir={$withval}])
AC_SUBST(confdir)

# Figure out the dependency directory
AC_ARG_WITH([share-dir], AS_HELP_STRING([--with-share-dir],[Set a different configuration directory]), [sharedir={$withval}])
AC_SUBST(sharedir)

AC_ARG_WITH([cc], AS_HELP_STRING([--with-cc=CC],[Set a different C compiler as the default]), [cc=${withval}])
AC_SUBST(cc)

# Use enable systemd or enable sysv here
AC_ARG_ENABLE([systemd], AS_HELP_STRING([--enable-systemd],[Use systemd units for daemon management (default)]), [systemd=${enableval}])

AC_ARG_ENABLE([sysv], AS_HELP_STRING([--enable-sysv],[Use sysv-style init scripts for daemon management]), [sysv=yes], [systemd=no])


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

if test "x${systemd}" == xyes; then
	AC_ARG_WITH([systemd-unit-path], 
		AS_HELP_STRING([--with-systemd-unit-path],[Use an alternate systemd unit file path for installation]), 
		[svcpath=${withval}]
	)
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
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_PROG_CPP

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lcrypto':
#AC_CHECK_LIB([crypto], [main])
# FIXME: Replace `main' with a function in `-ldl':
AC_CHECK_LIB([dl], [main])
# FIXME: Replace `main' with a function in `-lgcov':
#AC_CHECK_LIB([gcov], [main])
# FIXME: Replace `main' with a function in `-lm':
AC_CHECK_LIB([m], [main])
# FIXME: Replace `main' with a function in `-lpthread':
AC_CHECK_LIB([pthread], [main])
# FIXME: Replace `main' with a function in `-lssl':
#AC_CHECK_LIB([ssl], [main])
# FIXME: Replace `main' with a function in `-lubsan':
#AC_CHECK_LIB([ubsan], [main])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h float.h inttypes.h limits.h locale.h malloc.h netdb.h netinet/in.h stddef.h stdint.h stdlib.h string.h strings.h sys/file.h sys/ioctl.h sys/mount.h sys/param.h sys/socket.h sys/statvfs.h sys/time.h unistd.h utime.h])

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

# Find Lua headers or use the local version 
# FIXME: Add option for static vs dynamic
AC_ARG_ENABLE([local-lua], AS_HELP_STRING([--enable-local-lua],[Use the provided Lua libraries]), [local_lua=1])
if test ${local_lua} == 1; then
	objdeps+="lib/liblua.a "
else
	# Find Lua header and library
	LUA_HEADER_ERRMSG="Lua headers were not found and Hypno needs them to support TLS"
	AC_CHECK_HEADER([lua.h], [], AC_MSG_FAILURE([${LUA_HEADER_ERRMSG}]))
	LUA_LIB_ERRMSG="Lua library not found and Hypno needs this library for Lua support"
	# FIXME: Replace `main' with a function in `-lgnutls':
	AC_CHECK_LIB([lua], [main], [], AC_MSG_FAILURE(${LUA_LIB_ERRMSG}))
	ld_flags+=" -llua"
fi

# Check for TLS support and find GnuTLS header and libs if present; and add flags
AC_ARG_ENABLE([tls-support], AS_HELP_STRING([--disable-tls-support],[Disable TLS support]), [disable_tls=1])
AC_ARG_ENABLE([local-tls], AS_HELP_STRING([--enable-local-tls],[Use the provided GnuTLS libraries]), [local_tls=1])
if test ${disable_tls} == 0; then 
	if test ${local_tls} == 1; then
		#build the local one
		objdeps+="lib/libgnutls.a "
	else
		GNUTLS_HEADER_ERRMSG="GnuTLS headers were not found and Hypno needs them to support TLS"
		AC_CHECK_HEADER([gnutls/gnutls.h], [], AC_MSG_FAILURE([${GNUTLS_HEADER_ERRMSG}]))
		GNUTLS_LIB_ERRMSG="GnuTLS library not found and Hypno needs this library to support TLS"
		# FIXME: Replace `main' with a function in `-lgnutls':
		AC_CHECK_LIB([gnutls], [main], [], AC_MSG_FAILURE(${GNUTLS_LIB_ERRMSG}))
		ld_flags+=" -lgnutls"
	fi
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

*** hypno configured with: ***

Path: $wwwroot
Default port: $wwwport
Default server user: $srvuser
Default server group: $srvgroup
Default logging directory: $logdir
Default configuration directory: $confdir
Default share directory: $sharedir
Init system: $initsys
Daemon service file path: $svcpath
Chosen threading model: $modelflags
Default domain: $default_domain
TLS included: `test ${disable_tls} -eq 0 && echo "yes" || echo "no"`
Selected LD_FLAGS: $ld_flags
Using local SQLite3: `test ${local_sqlite3} -eq 1 && echo "yes" || echo "no"`
Using local Lua: `test ${local_lua} -eq 1 && echo "yes" || echo "no"`
Using local GnuTLS: `test ${local_tls} -eq 1 && echo "yes" || echo "no"`
Debugging flags: $debug_flags

EOF

# Replace all of these before generation
AC_SUBST(wwwroot)
AC_SUBST(wwwport)
AC_SUBST(srvuser)
AC_SUBST(srvgroup)
AC_SUBST(logdir)
AC_SUBST(svcpath)
#AC_SUBST(default_svcpath)
AC_SUBST(initsys)
AC_SUBST(systemd)
AC_SUBST(sysv)
AC_SUBST(sysvpath)
AC_SUBST(modelflags)
AC_SUBST(default_domain)
AC_SUBST(poll_interval)
AC_SUBST(preamble_size)
AC_SUBST(read_size)
AC_SUBST(read_max)
AC_SUBST(write_size)
AC_SUBST(stack_size)
AC_SUBST(max_threads)
AC_SUBST(socket_backlog)
AC_SUBST(default_error_file_name)
AC_SUBST(default_access_file_name)
AC_SUBST(max_simultaneous_connections)
AC_SUBST(disable_sqlite3)
AC_SUBST(disable_mysql)
AC_SUBST(disable_pgsql)
AC_SUBST(disable_sqlserver)
AC_SUBST(disable_oracle)
AC_SUBST(with_xml)
AC_SUBST(with_json)
AC_SUBST(with_msgpack)
AC_SUBST(disable_tls)
AC_SUBST(include_filter_c)
AC_SUBST(include_filter_lua)
AC_SUBST(include_filter_static)
AC_SUBST(include_filter_echo)
AC_SUBST(ld_flags)
AC_SUBST(debug_flags)
AC_SUBST(objdeps)

AC_OUTPUT
