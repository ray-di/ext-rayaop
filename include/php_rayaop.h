#ifndef PHP_RAYAOP_H
#define PHP_RAYAOP_H

extern zend_module_entry rayaop_module_entry;
#define phpext_rayaop_ptr &rayaop_module_entry

#define PHP_RAYAOP_VERSION "1.0.0"  // バージョンを更新

#ifdef ZTS
#include "TSRM.h"
#endif

ZEND_BEGIN_MODULE_GLOBALS(rayaop)
    // グローバル変数をここに追加
ZEND_END_MODULE_GLOBALS(rayaop)

#define RAYAOP_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(rayaop, v)

#if defined(ZTS) && defined(COMPILE_DL_RAYAOP)
ZEND_TSRMLS_CACHE_EXTERN()
#endif

PHP_FUNCTION(method_intercept);

#endif /* PHP_RAYAOP_H */
