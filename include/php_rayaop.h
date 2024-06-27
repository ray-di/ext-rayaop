#ifndef PHP_RAYAOP_H
#define PHP_RAYAOP_H

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"

#define PHP_RAYAOP_VERSION "0.1.0"
#define PHP_RAYAOP_EXTNAME "rayaop"

// ここに新しい定義を追加
#if PHP_VERSION_ID >= 80300
#define RAYAOP_RETVAL_USED(opline) ((opline)->result_type != IS_UNUSED)
#else
#define RAYAOP_RETVAL_USED(opline) (!((opline)->result_type & EXT_TYPE_UNUSED))
#endif

// 他の既存の定義や宣言

PHP_MINIT_FUNCTION(rayaop);
PHP_MSHUTDOWN_FUNCTION(rayaop);
PHP_RINIT_FUNCTION(rayaop);
PHP_RSHUTDOWN_FUNCTION(rayaop);
PHP_MINFO_FUNCTION(rayaop);

extern zend_module_entry rayaop_module_entry;

#endif /* PHP_RAYAOP_H */