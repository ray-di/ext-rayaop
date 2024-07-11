/* If PHP_RAYAOP_H is not defined, then define PHP_RAYAOP_H (header guard) */
#ifndef PHP_RAYAOP_H
#define PHP_RAYAOP_H

/* If HAVE_CONFIG_H is defined, then include config.h */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"  /* Include PHP core header file */
#include "php_ini.h"  /* Include PHP INI related header file */
#include "ext/standard/info.h"  /* Include standard extension module information related header */
#include "zend_exceptions.h"  /* Include Zend exception handling related header */
#include "zend_interfaces.h"  /* Include Zend interface related header */

/* If in thread-safe mode, then include Thread Safe Resource Manager */
#ifdef ZTS
#include "TSRM.h"
#endif

/* Define RayAOP namespace */
#define RAYAOP_NS "Ray\\Aop\\"

/* Declare rayaop module entry as external reference */
extern zend_module_entry rayaop_module_entry;

/* Define pointer to rayaop module */
#define phpext_rayaop_ptr &rayaop_module_entry

/* If in Windows environment, specify DLL export */
#ifdef PHP_WIN32
#define PHP_RAYAOP_API __declspec(dllexport)
/* If using GCC 4 or later, specify default visibility */
#elif defined(__GNUC__) && __GNUC__ >= 4
#define PHP_RAYAOP_API __attribute__ ((visibility("default")))
/* For other environments, no specific definition is made */
#else
#define PHP_RAYAOP_API
#endif

/* If in thread-safe mode, include Thread Safe Resource Manager again (redundant but for safety) */
#ifdef ZTS
#include "TSRM.h"
#endif

/* Macro for debug output */
#ifdef RAYAOP_DEBUG  /* If debug mode is enabled */
#define PHP_RAYAOP_DEBUG_PRINT(fmt, ...) php_printf("RAYAOP DEBUG: " fmt "\n", ##__VA_ARGS__)  /* Define debug output macro */
#else  /* If debug mode is disabled */
#define PHP_RAYAOP_DEBUG_PRINT(fmt, ...)  /* Do nothing */
#endif

/* Structure to hold intercept information */
typedef struct _php_rayaop_intercept_info {
    zend_string *class_name; /* Class name to intercept */
    zend_string *method_name; /* Method name to intercept */
    zval handler; /* Intercept handler */
} php_rayaop_intercept_info;

/* Function declarations */
PHP_MINIT_FUNCTION(rayaop); /* Module initialization function */
PHP_MSHUTDOWN_FUNCTION(rayaop); /* Module shutdown function */
PHP_RINIT_FUNCTION(rayaop); /* Request initialization function */
PHP_RSHUTDOWN_FUNCTION(rayaop); /* Request shutdown function */
PHP_MINFO_FUNCTION(rayaop); /* Module information function */
PHP_FUNCTION(method_intercept); /* Method intercept function */

/* Utility function declarations */
void php_rayaop_handle_error(const char *message); /* Error handling function */
bool php_rayaop_should_intercept(zend_execute_data *execute_data); /* Function to determine if interception is necessary */
char *php_rayaop_generate_intercept_key(zend_string *class_name, zend_string *method_name, size_t *key_len); /* Function to generate intercept key */
php_rayaop_intercept_info *php_rayaop_find_intercept_info(const char *key, size_t key_len); /* Function to search for intercept information */
void php_rayaop_execute_intercept(zend_execute_data *execute_data, php_rayaop_intercept_info *info); /* Function to execute interception */
void php_rayaop_free_intercept_info(zval *zv); /* Function to free intercept information */

#ifdef RAYAOP_DEBUG  /* If debug mode is enabled */
void php_rayaop_debug_print_zval(zval *value);  /* Function to debug print zval value */
void php_rayaop_dump_intercept_info(void);  /* Function to dump intercept information */
#endif

ZEND_BEGIN_MODULE_GLOBALS(rayaop)
    /* Start of rayaop module global variables */
    HashTable *intercept_ht; /* Intercept hash table */
    zend_bool is_intercepting; /* Intercepting flag */
ZEND_END_MODULE_GLOBALS(rayaop) /* End of rayaop module global variables */

/* If in thread-safe mode, global variable access macro (thread-safe version) */
#ifdef ZTS
#define RAYAOP_G(v) TSRMG(rayaop_globals_id, zend_rayaop_globals *, v)
/* If in non-thread-safe mode, global variable access macro (non-thread-safe version) */
#else
#define RAYAOP_G(v) (rayaop_globals.v)
#endif

#endif /* End of header guard */