#ifndef PHP_RAYAOP_H
#define PHP_RAYAOP_H

extern zend_module_entry rayaop_module_entry;  // 拡張機能のエントリポイント
#define phpext_rayaop_ptr &rayaop_module_entry  // 拡張機能のポインタ

#define PHP_RAYAOP_VERSION "1.0.0"  // 拡張機能のバージョン

#ifdef ZTS
#include "TSRM.h"  // スレッドセーフリソース管理ヘッダー
#endif

/**
 * 拡張機能のグローバル変数の宣言
 * @link https://www.php.net/manual/en/internals2.variables.globals.php
 */
ZEND_BEGIN_MODULE_GLOBALS(rayaop)
    // 拡張機能のグローバル変数をここに追加
ZEND_END_MODULE_GLOBALS(rayaop)

/**
 * グローバル変数アクセス用のマクロ
 * @link https://www.php.net/manual/en/internals2.variables.globals.php
 */
#define RAYAOP_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(rayaop, v)

/**
 * スレッドセーフリソース管理のキャッシュの外部宣言
 * @link https://www.php.net/manual/en/internals2.variables.globals.php
 */
#if defined(ZTS) && defined(COMPILE_DL_RAYAOP)
ZEND_TSRMLS_CACHE_EXTERN()
#endif

PHP_FUNCTION(method_intercept);  // method_intercept関数の宣言

#endif /* PHP_RAYAOP_H */
