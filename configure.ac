#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_PREREQ([2.69])
AC_INIT([hypno], [0.3.1a], [ramar@ramar.work])
AC_CONFIG_SRCDIR([example/c/dylib.c])

# Define files to perform substitutions on
#AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
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
svcpath=`pkg-config systemd --variable=systemduserunitdir 2>/dev/null`
default_svcpath="/usr/lib/systemd/user"
initsys="systemd"
systemd="yes"
sysv="no"
sysvpath="/etc/init.d"
modelflags="HTHREAD_H"
default_domain="localhost"

AC_ARG_WITH([www-root], AC_HELP_STRING([--with-www-root=DIR],[Set default web directory for applications]), [wwwroot=${withval}])
AC_SUBST(wwwroot)

AC_ARG_WITH([www-port], AC_HELP_STRING([--with-www-port=PORT],[Set default server port for listening]), [wwwport=${withval}])
AC_SUBST(wwwport)

AC_ARG_WITH([server-user], AC_HELP_STRING([--with-server-user],[Set default user for managing the web server]), [srvuser=${withval}])
AC_SUBST(srvuser)

AC_ARG_WITH([server-group], AC_HELP_STRING([--with-server-group],[Set default group for managing the web server]),[srvgroup=${withval}])
AC_SUBST(srvgroup)

AC_ARG_WITH([log-dir], AC_HELP_STRING([--with-log-dir],[Set access and error log directory]), [logdir=${withval}])
AC_SUBST(logdir)

# Use enable systemd or enable sysv here
AC_ARG_ENABLE([systemd], AC_HELP_STRING([--enable-systemd],[Use systemd units for daemon management (default)]), [systemd=${enableval}])

AC_ARG_ENABLE([sysv], AC_HELP_STRING([--enable-sysv],[Use sysv-style init scripts for daemon management]), [sysv=yes], [systemd=no])

# If systemd is enabled, make sure that we actually found something...
AS_IF([test "x$systemd" == xyes],
	[
	AC_ARG_WITH([systemd-unit-path], 
	AC_HELP_STRING([--with-systemd-unit-path],[Use an alternate systemd unit file path for installation]), 
	[svcpath=${withval}])
	]
)

# If sysv is in use instead, try to use those conventions
AS_IF([test "x${sysv}" == xyes],[svcpath=${sysvpath}; initsys="sysv"])

# Finally, if there is no service file path, set with the basic one.
AS_IF([test -z "${svcpath}"],[svcpath=${default_svcpath}])

# Replace these paths
AC_SUBST(initsys)
AC_SUBST(svcpath)

# Checks for programs.
AC_PROG_AWK
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_PROG_CPP

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lcrypto':
AC_CHECK_LIB([crypto], [main])
# FIXME: Replace `main' with a function in `-ldl':
AC_CHECK_LIB([dl], [main])
# FIXME: Replace `main' with a function in `-lgcov':
AC_CHECK_LIB([gcov], [main])
# FIXME: Replace `main' with a function in `-lgnutls':
AC_CHECK_LIB([gnutls], [main])
# FIXME: Replace `main' with a function in `-lm':
AC_CHECK_LIB([m], [main])
# FIXME: Replace `main' with a function in `-lpthread':
AC_CHECK_LIB([pthread], [main])
# FIXME: Replace `main' with a function in `-lssl':
AC_CHECK_LIB([ssl], [main])
# FIXME: Replace `main' with a function in `-lubsan':
AC_CHECK_LIB([ubsan], [main])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h float.h inttypes.h limits.h locale.h malloc.h netdb.h netinet/in.h stddef.h stdint.h stdlib.h string.h strings.h sys/file.h sys/ioctl.h sys/mount.h sys/param.h sys/socket.h sys/statvfs.h sys/time.h unistd.h utime.h])

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

hypno configured with:
Path: $wwwroot
Default port: $wwwport
Default server user: $srvuser
Default server group: $srvgroup
Default logging directory: $logdir
Init system: $initsys
Daemon service file path: $svcpath
Chosen threading model: $modelflags
Default domain: $default_domain
EOF

AC_OUTPUT
