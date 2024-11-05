dnl config.m4 for extension rayaop

PHP_ARG_ENABLE(rayaop, whether to enable rayaop,
[ --enable-rayaop   Enable rayaop])

if test "$PHP_RAYAOP" != "no"; then
  dnl Define whether the extension is enabled
  AC_DEFINE(HAVE_RAYAOP, 1, [whether rayaop is enabled])

  dnl Add new PHP extension
  PHP_NEW_EXTENSION(rayaop, rayaop.c, $ext_shared)

  dnl Add Makefile fragment
  PHP_ADD_MAKEFILE_FRAGMENT

  dnl Add instruction to install header files
  PHP_INSTALL_HEADERS([ext/rayaop], [php_rayaop.h])

  dnl Add quiet mode option
  PHP_ARG_ENABLE(rayaop-quiet, whether to suppress experimental notices,
    [ --enable-rayaop-quiet   Suppress experimental notices], no, yes)

  if test "$PHP_RAYAOP_QUIET" != "no"; then
    AC_DEFINE(RAYAOP_QUIET, 1, [Whether to suppress experimental notices])
  fi
fi