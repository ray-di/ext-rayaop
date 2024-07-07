dnl $Id$
dnl config.m4 for extension rayaop

dnl Include PECL configuration macro
dnl link https://github.com/php/pecl-tools/blob/master/autoconf/pecl.m4
sinclude(./autoconf/pecl.m4)

dnl Include macro for detecting PHP executable
dnl link https://github.com/php/pecl-tools/blob/master/autoconf/php-executable.m4
sinclude(./autoconf/php-executable.m4)

dnl Initialize PECL extension
dnl link https://github.com/php/pecl-tools/blob/master/pecl.m4#L229
PECL_INIT([rayaop])

dnl Add configuration option to enable the extension
dnl link https://www.gnu.org/software/autoconf/manual/autoconf-2.68/html_node/External-Shell-Variables.html
PHP_ARG_ENABLE(rayaop, whether to enable rayaop, [ --enable-rayaop   Enable rayaop])

dnl Process if the extension is enabled
if test "$PHP_RAYAOP" != "no"; then
  dnl Define whether the extension is enabled
  dnl link https://www.gnu.org/software/autoconf/manual/autoconf-2.68/html_node/Defining-Variables.html
  AC_DEFINE(HAVE_RAYAOP, 1, [whether rayaop is enabled])

  dnl Add new PHP extension
  dnl link https://www.phpinternalsbook.com/build_system/build_system.html
  PHP_NEW_EXTENSION(rayaop, rayaop.c, $ext_shared)

  dnl Add Makefile fragment
  dnl link https://www.phpinternalsbook.com/build_system/build_system.html#php-add-makefile-fragment
  PHP_ADD_MAKEFILE_FRAGMENT

  dnl Add instruction to install header files
  dnl link https://www.phpinternalsbook.com/build_system/build_system.html#php-install-headers
  PHP_INSTALL_HEADERS([ext/rayaop], [php_rayaop.h])
fi
