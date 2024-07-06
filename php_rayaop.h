#ifndef PHP_RAYAOP_H
#define PHP_RAYAOP_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "zend_exceptions.h"
#include "zend_interfaces.h"

#ifdef ZTS
#include "TSRM.h"
#endif

#define PHP_RAYAOP_VERSION "1.0.0"
#define RAYAOP_NS "Ray\\Aop\\"

extern zend_module_entry rayaop_module_entry;
#define phpext_rayaop_ptr &rayaop_module_entry

#ifdef PHP_WIN32
#define PHP_RAYAOP_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#define PHP_RAYAOP_API __attribute__ ((visibility("default")))
#else
#define PHP_RAYAOP_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

// デバッグ出力用マクロ
#ifdef RAYAOP_DEBUG
#define RAYAOP_DEBUG_PRINT(fmt, ...) php_printf("RAYAOP DEBUG: " fmt "\n", ##__VA_ARGS__)
#else
#define RAYAOP_DEBUG_PRINT(fmt, ...)
#endif

// エラーコード
#define RAYAOP_ERROR_MEMORY_ALLOCATION 1
#define RAYAOP_ERROR_HASH_UPDATE 2

/**
 * インターセプト情報を保持する構造体
 * link: http//://www.phpinternalsbook.com/php5/classes_objects/internal_structures_and_implementation.html
 * link: http://php.adamharvey.name/manual/ja/internals2.variables.tables.php
 */
typedef struct _intercept_info {
    zend_string *class_name;     // インターセプト対象のクラス名
    zend_string *method_name;    // インターセプト対象のメソッド名
    zval handler;                // インターセプトハンドラー
} intercept_info;

// 関数宣言
PHP_MINIT_FUNCTION(rayaop);
PHP_MSHUTDOWN_FUNCTION(rayaop);
PHP_RINIT_FUNCTION(rayaop);
PHP_RSHUTDOWN_FUNCTION(rayaop);
PHP_MINFO_FUNCTION(rayaop);

PHP_FUNCTION(method_intercept);

// ユーティリティ関数の宣言
void rayaop_handle_error(const char *message);
bool rayaop_should_intercept(zend_execute_data *execute_data);
char* rayaop_generate_intercept_key(zend_string *class_name, zend_string *method_name, size_t *key_len);
intercept_info* rayaop_find_intercept_info(const char *key, size_t key_len);
void rayaop_execute_intercept(zend_execute_data *execute_data, intercept_info *info);
void rayaop_free_intercept_info(zval *zv);

#ifdef RAYAOP_DEBUG
void rayaop_debug_print_zval(zval *value);
void rayaop_dump_intercept_info(void);
#endif

ZEND_BEGIN_MODULE_GLOBALS(rayaop)
    HashTable *intercept_ht;
    zend_bool is_intercepting;
ZEND_END_MODULE_GLOBALS(rayaop)

#ifdef ZTS
#define RAYAOP_G(v) TSRMG(rayaop_globals_id, zend_rayaop_globals *, v)
#else
#define RAYAOP_G(v) (rayaop_globals.v)
#endif

#endif /* PHP_RAYAOP_H */