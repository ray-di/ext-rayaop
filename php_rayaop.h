#ifndef PHP_RAYAOP_H  // If PHP_RAYAOP_H is not defined
#define PHP_RAYAOP_H  // Define PHP_RAYAOP_H (header guard)

#ifdef HAVE_CONFIG_H  // If HAVE_CONFIG_H is defined
#include "config.h"  // Include config.h
#endif

#include "php.h"  // Include PHP core header file
#include "php_ini.h"  // Include PHP INI related header file
#include "ext/standard/info.h"  // Include standard extension module information related header
#include "zend_exceptions.h"  // Include Zend exception handling related header
#include "zend_interfaces.h"  // Include Zend interface related header

#ifdef ZTS  // If in thread-safe mode
#include "TSRM.h"  // Include Thread Safe Resource Manager
#endif

#define PHP_RAYAOP_VERSION "1.0.0"  // Define RayAOP extension version
#define RAYAOP_NS "Ray\\Aop\\"  // Define RayAOP namespace

extern zend_module_entry rayaop_module_entry;  // Declare rayaop module entry as external reference
#define phpext_rayaop_ptr &rayaop_module_entry  // Define pointer to rayaop module

#ifdef PHP_WIN32  // If in Windows environment
#define PHP_RAYAOP_API __declspec(dllexport)  // Specify DLL export
#elif defined(__GNUC__) && __GNUC__ >= 4  // If using GCC 4 or later
#define PHP_RAYAOP_API __attribute__ ((visibility("default")))  // Specify default visibility
#else  // For other environments
#define PHP_RAYAOP_API  // No specific definition
#endif

#ifdef ZTS  // If in thread-safe mode
#include "TSRM.h"  // Include Thread Safe Resource Manager again (redundant but for safety)
#endif

// Macro for debug output
#ifdef RAYAOP_DEBUG  // If debug mode is enabled
#define RAYAOP_DEBUG_PRINT(fmt, ...) php_printf("RAYAOP DEBUG: " fmt "\n", ##__VA_ARGS__)  // Define debug output macro
#else  // If debug mode is disabled
#define RAYAOP_DEBUG_PRINT(fmt, ...)  // Do nothing
#endif

// Error codes
#define RAYAOP_ERROR_MEMORY_ALLOCATION 1  // Error code for memory allocation
#define RAYAOP_ERROR_HASH_UPDATE 2  // Error code for hash update

/**
 * Structure to hold intercept information
 * link: http://www.phpinternalsbook.com/php5/classes_objects/internal_structures_and_implementation.html
 * link: http://php.adamharvey.name/manual/ja/internals2.variables.tables.php
 */
typedef struct _intercept_info {
    zend_string *class_name;     // Class name to intercept
    zend_string *method_name;    // Method name to intercept
    zval handler;                // Intercept handler
} intercept_info;

// Function declarations
PHP_MINIT_FUNCTION(rayaop);  // Module initialization function
PHP_MSHUTDOWN_FUNCTION(rayaop);  // Module shutdown function
PHP_RINIT_FUNCTION(rayaop);  // Request initialization function
PHP_RSHUTDOWN_FUNCTION(rayaop);  // Request shutdown function
PHP_MINFO_FUNCTION(rayaop);  // Module information function

PHP_FUNCTION(method_intercept);  // Method intercept function

// Utility function declarations
void rayaop_handle_error(const char *message);  // Error handling function
bool rayaop_should_intercept(zend_execute_data *execute_data);  // Function to determine if interception is necessary
char* rayaop_generate_intercept_key(zend_string *class_name, zend_string *method_name, size_t *key_len);  // Function to generate intercept key
intercept_info* rayaop_find_intercept_info(const char *key, size_t key_len);  // Function to search for intercept information
void rayaop_execute_intercept(zend_execute_data *execute_data, intercept_info *info);  // Function to execute interception
void rayaop_free_intercept_info(zval *zv);  // Function to free intercept information

#ifdef RAYAOP_DEBUG  // If debug mode is enabled
void rayaop_debug_print_zval(zval *value);  // Function to debug print zval value
void rayaop_dump_intercept_info(void);  // Function to dump intercept information
#endif

ZEND_BEGIN_MODULE_GLOBALS(rayaop)  // Start of rayaop module global variables
    HashTable *intercept_ht;  // Intercept hash table
    zend_bool is_intercepting;  // Intercepting flag
ZEND_END_MODULE_GLOBALS(rayaop)  // End of rayaop module global variables

#ifdef ZTS  // If in thread-safe mode
#define RAYAOP_G(v) TSRMG(rayaop_globals_id, zend_rayaop_globals *, v)  // Global variable access macro (thread-safe version)
#else  // If in non-thread-safe mode
#define RAYAOP_G(v) (rayaop_globals.v)  // Global variable access macro (non-thread-safe version)
#endif

#endif /* PHP_RAYAOP_H */  // End of header guard