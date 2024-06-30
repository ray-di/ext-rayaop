dnl $Id$
dnl config.m4 for extension rayaop

dnl PECLの設定マクロをインクルード
dnl @link https://github.com/php/pecl-tools/blob/master/autoconf/pecl.m4
sinclude(./autoconf/pecl.m4)

dnl PHP実行ファイルの検出マクロをインクルード
dnl @link https://github.com/php/pecl-tools/blob/master/autoconf/php-executable.m4
sinclude(./autoconf/php-executable.m4)

dnl PECL拡張の初期化
dnl @link https://github.com/php/pecl-tools/blob/master/pecl.m4#L229
PECL_INIT([rayaop])

dnl 拡張機能を有効にするかどうかの設定オプションを追加
dnl @link https://www.gnu.org/software/autoconf/manual/autoconf-2.68/html_node/External-Shell-Variables.html
PHP_ARG_ENABLE(rayaop, whether to enable rayaop, [ --enable-rayaop   Enable rayaop])

dnl 拡張機能が有効な場合の処理
if test "$PHP_RAYAOP" != "no"; then
  dnl 拡張機能が有効かどうかを定義
  dnl @link https://www.gnu.org/software/autoconf/manual/autoconf-2.68/html_node/Defining-Variables.html
  AC_DEFINE(HAVE_RAYAOP, 1, [whether rayaop is enabled])

  dnl PHPの新しい拡張機能を追加
  dnl @link https://www.phpinternalsbook.com/build_system/build_system.html
  PHP_NEW_EXTENSION(rayaop, rayaop.c, $ext_shared)

  dnl Makefileフラグメントを追加
  dnl @link https://www.phpinternalsbook.com/build_system/build_system.html#php-add-makefile-fragment
  PHP_ADD_MAKEFILE_FRAGMENT

  dnl ヘッダーファイルのインストール指示を追加
  dnl @link https://www.phpinternalsbook.com/build_system/build_system.html#php-install-headers
  PHP_INSTALL_HEADERS([ext/rayaop], [php_rayaop.h])
fi
