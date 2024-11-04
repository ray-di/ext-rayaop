/* Header guard */
#ifndef PHP_RAYAOP_H
#define PHP_RAYAOP_H

/* Configuration header */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* Required system headers first */
#include <stdint.h>

/* PHP Core headers in correct order */
#include "php.h"
#include "zend.h"
#include "zend_types.h"
#include "zend_API.h"
#include "zend_ini.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "zend_exceptions.h"
#include "zend_interfaces.h"

#ifdef ZTS
#include "TSRM.h"
#endif

/* Constants */
#define MAX_EXECUTION_DEPTH 100
#define PHP_RAYAOP_VERSION "1.0.0"
#define RAYAOP_NS "Ray\\Aop\\"

/* Error codes */
#define RAYAOP_E_MEMORY_ALLOCATION 1
#define RAYAOP_E_HASH_UPDATE      2
#define RAYAOP_E_INVALID_HANDLER  3
#define RAYAOP_E_MAX_DEPTH_EXCEEDED 4

/* Argument information declarations */
ZEND_BEGIN_ARG_INFO_EX(arginfo_method_intercept, 0, 0, 3)
    ZEND_ARG_TYPE_INFO(0, class_name, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, method_name, IS_STRING, 0)
    ZEND_ARG_OBJ_INFO(0, interceptor, Ray\\Aop\\MethodInterceptorInterface, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_method_intercept_init, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_enable_method_intercept, 0, 1, IS_VOID, 0)
    ZEND_ARG_TYPE_INFO(0, enable, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

/* Debug mode configuration */
#ifdef RAYAOP_DEBUG
#define PHP_RAYAOP_DEBUG_PRINT(fmt, ...) \
    do { \
        php_printf("RAYAOP DEBUG [%s:%d]: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
    } while (0)
#else
#define PHP_RAYAOP_DEBUG_PRINT(fmt, ...)
#endif

/* Thread safety macros */
#ifdef ZTS
#define RAYAOP_G_LOCK()   tsrm_mutex_lock(rayaop_globals_id)
#define RAYAOP_G_UNLOCK() tsrm_mutex_unlock(rayaop_globals_id)
#else
#define RAYAOP_G_LOCK()
#define RAYAOP_G_UNLOCK()
#endif

/* Module entry */
extern zend_module_entry rayaop_module_entry;
#define phpext_rayaop_ptr &rayaop_module_entry

/* Interface class entry */
extern zend_class_entry *ray_aop_method_interceptor_interface_ce;

/* Windows DLL export */
#ifdef PHP_WIN32
#define PHP_RAYAOP_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#define PHP_RAYAOP_API __attribute__ ((visibility("default")))
#else
#define PHP_RAYAOP_API
#endif

/* Intercept information structure */
typedef struct _php_rayaop_intercept_info {
    zend_string *class_name;    /* Class name to intercept */
    zend_string *method_name;   /* Method name to intercept */
    zval handler;               /* Intercept handler */
    zend_bool is_enabled;       /* Flag to enable/disable interception */
} php_rayaop_intercept_info;

/* Module globals structure */
ZEND_BEGIN_MODULE_GLOBALS(rayaop)
    HashTable *intercept_ht;     /* Intercept hash table */
    zend_bool is_intercepting;   /* Intercepting flag */
    uint32_t execution_depth;    /* Execution depth counter */
    zend_bool method_intercept_enabled;  /* Global interception enable flag */
    uint32_t debug_level;        /* Debug level */
ZEND_END_MODULE_GLOBALS(rayaop)

/* Globals access macro */
#ifdef ZTS
#define RAYAOP_G(v) TSRMG(rayaop_globals_id, zend_rayaop_globals *, v)
#else
#define RAYAOP_G(v) (rayaop_globals.v)
#endif

/* Module functions */
PHP_MINIT_FUNCTION(rayaop);
PHP_MSHUTDOWN_FUNCTION(rayaop);
PHP_RINIT_FUNCTION(rayaop);
PHP_RSHUTDOWN_FUNCTION(rayaop);
PHP_MINFO_FUNCTION(rayaop);

/* Extension functions */
PHP_FUNCTION(method_intercept);
PHP_FUNCTION(method_intercept_init);
PHP_FUNCTION(enable_method_intercept);

/* Utility functions */
PHP_RAYAOP_API void php_rayaop_handle_error(int error_code, const char *message);
PHP_RAYAOP_API bool php_rayaop_should_intercept(zend_execute_data *execute_data);
PHP_RAYAOP_API char *php_rayaop_generate_key(zend_string *class_name, zend_string *method_name, size_t *key_len);
PHP_RAYAOP_API php_rayaop_intercept_info *php_rayaop_find_intercept_info(const char *key, size_t key_len);

/* Memory management functions */
PHP_RAYAOP_API void php_rayaop_free_intercept_info(zval *zv);
PHP_RAYAOP_API php_rayaop_intercept_info *php_rayaop_create_intercept_info(void);

/* Debug functions */
#ifdef RAYAOP_DEBUG
void php_rayaop_debug_dump_intercept_info(void);
void php_rayaop_debug_print_zval(zval *value);
#endif

/* Module globals declaration */
ZEND_EXTERN_MODULE_GLOBALS(rayaop)

#endif /* PHP_RAYAOP_H */