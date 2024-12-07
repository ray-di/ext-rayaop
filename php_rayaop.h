#ifndef PHP_RAYAOP_H
#define PHP_RAYAOP_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "zend.h"
#include "zend_API.h"
#include "zend_types.h"
#include "zend_ini.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "zend_exceptions.h"
#include "zend_interfaces.h"

#ifdef ZTS
#include "TSRM.h"
#endif

/* Export macros */
#ifdef PHP_WIN32
# define PHP_RAYAOP_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
# define PHP_RAYAOP_API __attribute__ ((visibility("default")))
#else
# define PHP_RAYAOP_API
#endif

/* Constants */
#define MAX_EXECUTION_DEPTH 100
#define PHP_RAYAOP_VERSION "1.0.0"
#define RAYAOP_NS "Ray\\Aop\\"

/* Error codes */
#define RAYAOP_E_MEMORY_ALLOCATION   1
#define RAYAOP_E_HASH_UPDATE         2
#define RAYAOP_E_INVALID_HANDLER     3
#define RAYAOP_E_MAX_DEPTH_EXCEEDED  4
#define RAYAOP_E_NULL_POINTER        5
#define RAYAOP_E_INVALID_STATE       6

#ifdef ZTS
extern MUTEX_T rayaop_mutex;
#define RAYAOP_G_LOCK() tsrm_mutex_lock(rayaop_mutex)
#define RAYAOP_G_UNLOCK() tsrm_mutex_unlock(rayaop_mutex)
#else
#define RAYAOP_G_LOCK()
#define RAYAOP_G_UNLOCK()
#endif

typedef struct _php_rayaop_intercept_info {
    zend_string *class_name;     /* Class name */
    zend_string *method_name;    /* Method name */
    zval handler;                /* Intercept handler object */
    zend_bool is_enabled;        /* Enabled flag */
} php_rayaop_intercept_info;

ZEND_BEGIN_MODULE_GLOBALS(rayaop)
    HashTable *intercept_ht;      /* Hash table for intercept information */
    zend_bool is_intercepting;    /* Flag indicating if currently intercepting */
    uint32_t execution_depth;     /* Nesting depth of interception */
    zend_bool method_intercept_enabled; /* Global enable/disable flag */
    uint32_t debug_level;         /* Debug level */
ZEND_END_MODULE_GLOBALS(rayaop)

#ifdef ZTS
#define RAYAOP_G(v) TSRMG(rayaop_globals_id, zend_rayaop_globals *, v)
#else
#define RAYAOP_G(v) (rayaop_globals.v)
#endif

extern zend_module_entry rayaop_module_entry;
#define phpext_rayaop_ptr &rayaop_module_entry

extern zend_class_entry *ray_aop_method_interceptor_interface_ce;

/* PHP lifecycle hooks */
PHP_MINIT_FUNCTION(rayaop);
PHP_MSHUTDOWN_FUNCTION(rayaop);
PHP_RINIT_FUNCTION(rayaop);
PHP_RSHUTDOWN_FUNCTION(rayaop);
PHP_MINFO_FUNCTION(rayaop);

/* PHP functions */
PHP_FUNCTION(method_intercept);
PHP_FUNCTION(method_intercept_init);
PHP_FUNCTION(method_intercept_enable);

/* API functions */
PHP_RAYAOP_API void php_rayaop_handle_error(int error_code, const char *message);
PHP_RAYAOP_API zend_bool php_rayaop_should_intercept(zend_execute_data *execute_data);
PHP_RAYAOP_API char *php_rayaop_generate_key(zend_string *class_name, zend_string *method_name, size_t *key_len);
PHP_RAYAOP_API php_rayaop_intercept_info *php_rayaop_find_intercept_info(const char *key, size_t key_len);
PHP_RAYAOP_API void php_rayaop_free_intercept_info(zval *zv);
PHP_RAYAOP_API php_rayaop_intercept_info *php_rayaop_create_intercept_info(void);

#endif /* PHP_RAYAOP_H */
