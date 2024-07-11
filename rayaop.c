#ifdef HAVE_CONFIG_H
#include "config.h"  /* Include configuration file */
#endif

#define PHP_RAYAOP_VERSION "0.1.0"

#ifndef RAYAOP_QUIET
#define RAYAOP_QUIET 0
#endif

// #define RAYAOP_DEBUG

#include "php_rayaop.h"  /* Include header file for RayAOP extension */

/* Declaration of module global variables */
ZEND_DECLARE_MODULE_GLOBALS(rayaop)

/* Declaration of static variable: pointer to the original zend_execute_ex function */
static void (*php_rayaop_original_execute_ex)(zend_execute_data *execute_data);

/* {{{ proto void php_rayaop_init_globals(zend_rayaop_globals *rayaop_globals)
   Global initialization function

   This function initializes the global variables for the RayAOP extension.

   @param zend_rayaop_globals *rayaop_globals Pointer to global variables
*/
static void php_rayaop_init_globals(zend_rayaop_globals *rayaop_globals) {
    rayaop_globals->intercept_ht = NULL; /* Initialize intercept hash table */
    rayaop_globals->is_intercepting = 0; /* Initialize intercept flag */
}
/* }}} */

/* Macro for debug output */
#ifdef RAYAOP_DEBUG
#define PHP_RAYAOP_DEBUG_PRINT(fmt, ...) php_printf("RAYAOP DEBUG: " fmt "\n", ##__VA_ARGS__)  /* Output debug information */
#else
#define PHP_RAYAOP_DEBUG_PRINT(fmt, ...)  /* Do nothing if not in debug mode */
#endif

/* Argument information for method_intercept function */
ZEND_BEGIN_ARG_INFO_EX(arginfo_method_intercept, 0, 0, 3)
    ZEND_ARG_TYPE_INFO(0, class_name, IS_STRING, 0) /* Argument information for class name */
    ZEND_ARG_TYPE_INFO(0, method_name, IS_STRING, 0) /* Argument information for method name */
    ZEND_ARG_OBJ_INFO(0, interceptor, Ray\\Aop\\MethodInterceptorInterface, 0) /* Argument information for intercept handler */
ZEND_END_ARG_INFO()

/* Argument information for method_intercept_init function */
ZEND_BEGIN_ARG_INFO(arginfo_method_intercept_init, 0)
ZEND_END_ARG_INFO()

/* {{{ proto void php_rayaop_handle_error(const char *message)
   Error handling function

   This function handles errors by outputting an error message.

   @param const char *message Error message to be displayed
*/
void php_rayaop_handle_error(const char *message) {
    php_error_docref(NULL, E_ERROR, "Memory error: %s", message); /* Output error message */
}
/* }}} */

/* {{{ proto bool php_rayaop_should_intercept(zend_execute_data *execute_data)
   Function to determine if interception is necessary

   This function checks if the current execution should be intercepted.

   @param zend_execute_data *execute_data Execution data
   @return bool Returns true if interception is necessary, false otherwise
*/
bool php_rayaop_should_intercept(zend_execute_data *execute_data) {
    return execute_data->func->common.scope && /* Scope exists */
           execute_data->func->common.function_name && /* Function name exists */
           !RAYAOP_G(is_intercepting); /* Not currently intercepting */
}
/* }}} */

/* {{{ proto char* php_rayaop_generate_intercept_key(zend_string *class_name, zend_string *method_name, size_t *key_len)
   Function to generate intercept key

   This function creates a key in the format "class_name::method_name" for use in the intercept hash table.

   @param zend_string *class_name The name of the class
   @param zend_string *method_name The name of the method
   @param size_t *key_len Pointer to store the length of the generated key
   @return char* The generated key, which must be freed by the caller using efree()
*/
char *php_rayaop_generate_intercept_key(zend_string *class_name, zend_string *method_name, size_t *key_len) {
    char *key = NULL;
    *key_len = spprintf(&key, 0, "%s::%s", ZSTR_VAL(class_name), ZSTR_VAL(method_name));
    /* Generate key in the format class_name::method_name */
    PHP_RAYAOP_DEBUG_PRINT("Generated key: %s", key); /* Output generated key for debugging */
    return key;
}
/* }}} */

/* {{{ proto php_rayaop_intercept_info* php_rayaop_find_intercept_info(const char *key, size_t key_len)
   Function to search for intercept information

   This function searches for intercept information in the hash table using the provided key.

   @param const char *key The key to search for
   @param size_t key_len The length of the key
   @return php_rayaop_intercept_info* Pointer to the intercept information if found, NULL otherwise
*/
php_rayaop_intercept_info *php_rayaop_find_intercept_info(const char *key, size_t key_len) {
    return zend_hash_str_find_ptr(RAYAOP_G(intercept_ht), key, key_len);
    /* Search for intercept information in hash table */
}
/* }}} */

/* {{{ Helper function to prepare intercept parameters */
static void prepare_intercept_params(zend_execute_data *execute_data, zval *params, php_rayaop_intercept_info *info) {
    ZVAL_OBJ(&params[0], execute_data->This.value.obj);
    ZVAL_STR(&params[1], info->method_name);

    array_init(&params[2]);
    uint32_t arg_count = ZEND_CALL_NUM_ARGS(execute_data);
    zval *args = ZEND_CALL_ARG(execute_data, 1);
    for (uint32_t i = 0; i < arg_count; i++) {
        zval *arg = &args[i];
        Z_TRY_ADDREF_P(arg);
        add_next_index_zval(&params[2], arg);
    }
}
/* }}} */

/* {{{ Helper function to call the intercept handler */
static bool call_intercept_handler(zval *handler, zval *params, zval *retval) {
    zval func_name;
    ZVAL_STRING(&func_name, "intercept");

    bool success = (call_user_function(NULL, handler, &func_name, retval, 3, params) == SUCCESS);

    zval_ptr_dtor(&func_name);
    return success;
}
/* }}} */

/* {{{ Helper function to clean up after interception */
static void cleanup_intercept(zval *params, zval *retval) {
    zval_ptr_dtor(&params[1]);
    zval_ptr_dtor(&params[2]);
    zval_ptr_dtor(retval);
}
/* }}} */

/* {{{ proto void php_rayaop_execute_intercept(zend_execute_data *execute_data, php_rayaop_intercept_info *info)
   Main function to execute method interception */
void php_rayaop_execute_intercept(zend_execute_data *execute_data, php_rayaop_intercept_info *info) {
    PHP_RAYAOP_DEBUG_PRINT("Executing intercept for %s::%s", ZSTR_VAL(info->class_name), ZSTR_VAL(info->method_name));

    if (Z_TYPE(info->handler) != IS_OBJECT) {
        PHP_RAYAOP_DEBUG_PRINT("Invalid handler, skipping interception");
        return;
    }

    if (execute_data->This.value.obj == NULL) {
        PHP_RAYAOP_DEBUG_PRINT("Object is NULL, calling original function");
        php_rayaop_original_execute_ex(execute_data);
        return;
    }

    zval retval, params[3];
    prepare_intercept_params(execute_data, params, info);

    RAYAOP_G(is_intercepting) = 1;

    ZVAL_UNDEF(&retval);
    if (call_intercept_handler(&info->handler, params, &retval)) {
        if (!Z_ISUNDEF(retval)) {
            ZVAL_COPY(execute_data->return_value, &retval);
        }
    } else {
        php_error_docref(NULL, E_WARNING, "Interception failed for %s::%s", ZSTR_VAL(info->class_name), ZSTR_VAL(info->method_name));
    }

    cleanup_intercept(params, &retval);
    RAYAOP_G(is_intercepting) = 0;

    PHP_RAYAOP_DEBUG_PRINT("Interception completed for %s::%s", ZSTR_VAL(info->class_name), ZSTR_VAL(info->method_name));
}
/* }}} */

/* {{{ proto void php_rayaop_free_intercept_info(zval *zv)
   Function to free intercept information

   This function frees the memory allocated for intercept information.

   @param zval *zv The zval containing the intercept information to be freed
*/
void php_rayaop_free_intercept_info(zval *zv) {
    php_rayaop_intercept_info *info = Z_PTR_P(zv); /* Get intercept information pointer from zval */
    if (info) {
        /* If intercept information exists */
        PHP_RAYAOP_DEBUG_PRINT("Freeing intercept info for %s::%s", ZSTR_VAL(info->class_name), ZSTR_VAL(info->method_name));
        /* Output debug information */
        zend_string_release(info->class_name); /* Free memory for class name */
        zend_string_release(info->method_name); /* Free memory for method name */
        zval_ptr_dtor(&info->handler); /* Free memory for handler */
        efree(info); /* Free memory for intercept information structure */
    }
}
/* }}} */

/* {{{ proto void php_rayaop_execute_ex(zend_execute_data *execute_data)
   Custom zend_execute_ex function

   This function replaces the original zend_execute_ex function to implement method interception.

   @param zend_execute_data *execute_data The execution data
*/
static void php_rayaop_execute_ex(zend_execute_data *execute_data) {
    PHP_RAYAOP_DEBUG_PRINT("php_rayaop_execute_ex called"); /* Output debug information */

    if (!php_rayaop_should_intercept(execute_data)) {
        /* If interception is not necessary */
        php_rayaop_original_execute_ex(execute_data); /* Call the original execution function */
        return; /* End processing */
    }

    zend_function *current_function = execute_data->func; /* Get current function information */
    zend_string *class_name = current_function->common.scope->name; /* Get class name */
    zend_string *method_name = current_function->common.function_name; /* Get method name */

    size_t key_len;
    char *key = php_rayaop_generate_intercept_key(class_name, method_name, &key_len); /* Generate intercept key */

    php_rayaop_intercept_info *info = php_rayaop_find_intercept_info(key, key_len); /* Search for intercept information */

    if (info) {
        /* If intercept information is found */
        PHP_RAYAOP_DEBUG_PRINT("Found intercept info for key: %s", key); /* Output debug information */
        php_rayaop_execute_intercept(execute_data, info); /* Execute interception */
    } else {
        /* If intercept information is not found */
        PHP_RAYAOP_DEBUG_PRINT("No intercept info found for key: %s", key); /* Output debug information */
        php_rayaop_original_execute_ex(execute_data); /* Call the original execution function */
    }

    efree(key); /* Free memory for key */
}
/* }}} */

/* {{{ proto void php_rayaop_hash_update_failed(php_rayaop_intercept_info *new_info, char *key)
   Handling for hash table update failure

   This function handles the case when updating the intercept hash table fails.

   @param php_rayaop_intercept_info *new_info The new intercept information that failed to be added
   @param char *key The key that was used for the failed update
*/
void php_rayaop_hash_update_failed(php_rayaop_intercept_info *new_info, char *key) {
    php_rayaop_handle_error("Failed to update intercept hash table"); /* Output error message */
    zend_string_release(new_info->class_name); /* Free memory for class name */
    zend_string_release(new_info->method_name); /* Free memory for method name */
    zval_ptr_dtor(&new_info->handler); /* Free memory for handler */
    efree(new_info); /* Free memory for intercept information structure */
    efree(key); /* Free memory for key */
}
/* }}} */

/* {{{ proto bool method_intercept(string class_name, string method_name, object intercepted)
   Function to register intercept method

   This function registers an interceptor for a specified class method.

   @param string class_name The name of the class
   @param string method_name The name of the method
   @param object intercepted The interceptor object
   @return bool Returns TRUE on success or FALSE on failure
*/
PHP_FUNCTION(method_intercept) {
    PHP_RAYAOP_DEBUG_PRINT("method_intercept called"); /* Output debug information */

    char *class_name, *method_name; /* Pointers for class name and method name */
    size_t class_name_len, method_name_len; /* Lengths of class name and method name */
    zval *intercepted; /* Intercept handler */

    ZEND_PARSE_PARAMETERS_START(3, 3)
        Z_PARAM_STRING(class_name, class_name_len) /* Parse class name parameter */
        Z_PARAM_STRING(method_name, method_name_len) /* Parse method name parameter */
        Z_PARAM_OBJECT(intercepted) /* Parse intercept handler parameter */
    ZEND_PARSE_PARAMETERS_END();

    php_rayaop_intercept_info *new_info = ecalloc(1, sizeof(php_rayaop_intercept_info)); /* Allocate memory for new intercept information */
    if (!new_info) {
        /* If memory allocation fails */
        php_rayaop_handle_error("Failed to allocate memory for intercept_info"); /* Output error message */
        RETURN_FALSE; /* Return false and end */
    }

    new_info->class_name = zend_string_init(class_name, class_name_len, 0); /* Initialize class name */
    new_info->method_name = zend_string_init(method_name, method_name_len, 0); /* Initialize method name */
    ZVAL_COPY(&new_info->handler, intercepted); /* Copy intercept handler */

    char *key = NULL;
    size_t key_len = spprintf(&key, 0, "%s::%s", class_name, method_name); /* Generate intercept key */

    if (zend_hash_str_update_ptr(RAYAOP_G(intercept_ht), key, key_len, new_info) == NULL) {
        /* Add to hash table */
        php_rayaop_hash_update_failed(new_info, key); /* Execute error handling if addition fails */
RETURN_FALSE; /* Return false and end */
    }

    efree(key); /* Free memory for key */
    PHP_RAYAOP_DEBUG_PRINT("Successfully registered intercept info"); /* Output debug information */
    RETURN_TRUE; /* Return true and end */
}
/* }}} */

/* {{{ proto void method_intercept_init()
   Function to reset intercept table

   This function clears the intercept hash table, ensuring it starts empty.

*/
PHP_FUNCTION(method_intercept_init) {
    PHP_RAYAOP_DEBUG_PRINT("method_intercept_init called"); /* Output debug information */
    if (RAYAOP_G(intercept_ht)) {
        zend_hash_clean(RAYAOP_G(intercept_ht)); /* Clear hash table */
    } else {
        ALLOC_HASHTABLE(RAYAOP_G(intercept_ht)); /* Allocate memory for hash table if not already allocated */
        zend_hash_init(RAYAOP_G(intercept_ht), 8, NULL, php_rayaop_free_intercept_info, 0); /* Initialize hash table */
    }
    RETURN_TRUE; /* Return true to indicate success */
}
/* }}} */

/* Interface definition */
zend_class_entry *ray_aop_method_interceptor_interface_ce;

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_ray_aop_method_interceptor_intercept, 0, 3, IS_MIXED, 0)
    ZEND_ARG_TYPE_INFO(0, object, IS_OBJECT, 0) /* 1st argument: object */
    ZEND_ARG_TYPE_INFO(0, method, IS_STRING, 0) /* 2nd argument: method name */
    ZEND_ARG_TYPE_INFO(0, params, IS_ARRAY, 0) /* 3rd argument: parameter array */
ZEND_END_ARG_INFO()

static const zend_function_entry ray_aop_method_interceptor_interface_methods[] = {
    ZEND_ABSTRACT_ME(Ray_Aop_MethodInterceptorInterface, intercept, arginfo_ray_aop_method_interceptor_intercept)
    /* Define intercept method */
    PHP_FE_END /* End of function entries */
};

/* {{{ proto int PHP_MINIT_FUNCTION(rayaop)
   Extension initialization function

   This function is called when the extension is loaded. It initializes the extension,
   registers the interface, and sets up the custom execute function.

   @return int Returns SUCCESS on successful initialization
*/
PHP_MINIT_FUNCTION(rayaop) {
    PHP_RAYAOP_DEBUG_PRINT("PHP_MINIT_FUNCTION called"); /* Output debug information */

#ifdef ZTS
    ts_allocate_id(&rayaop_globals_id, sizeof(zend_rayaop_globals), (ts_allocate_ctor) php_rayaop_init_globals, NULL);  /* Initialize global variables in thread-safe mode */
#else
    php_rayaop_init_globals(&rayaop_globals); /* Initialize global variables in non-thread-safe mode */
#endif

#if PHP_RAYAOP_EXPERIMENTAL && !RAYAOP_QUIET
    php_error_docref(NULL, E_NOTICE, "The Ray.Aop extension is experimental. Its functions may change or be removed in future releases.");
#endif

    zend_class_entry ce;
    INIT_CLASS_ENTRY(ce, "Ray\\Aop\\MethodInterceptorInterface", ray_aop_method_interceptor_interface_methods);
    /* Initialize class entry */
    ray_aop_method_interceptor_interface_ce = zend_register_internal_interface(&ce); /* Register interface */

    php_rayaop_original_execute_ex = zend_execute_ex; /* Save the original zend_execute_ex function */
    zend_execute_ex = php_rayaop_execute_ex; /* Set the custom zend_execute_ex function */

    PHP_RAYAOP_DEBUG_PRINT("RayAOP extension initialized"); /* Output debug information */
    return SUCCESS; /* Return success */
}
/* }}} */

/* {{{ proto int PHP_MSHUTDOWN_FUNCTION(rayaop)
   Extension shutdown function

   This function is called when the extension is unloaded. It restores the original
   execute function and performs any necessary cleanup.

   @return int Returns SUCCESS on successful shutdown
*/
PHP_MSHUTDOWN_FUNCTION(rayaop) {
    PHP_RAYAOP_DEBUG_PRINT("RayAOP PHP_MSHUTDOWN_FUNCTION called"); /* Output debug information */
    zend_execute_ex = php_rayaop_original_execute_ex; /* Restore the original zend_execute_ex function */
    php_rayaop_original_execute_ex = NULL; /* Clear the saved pointer */
    PHP_RAYAOP_DEBUG_PRINT("RayAOP PHP_MSHUTDOWN_FUNCTION shut down"); /* Output debug information */
    return SUCCESS; /* Return shutdown success */
}
/* }}} */

/* {{{ proto int PHP_RINIT_FUNCTION(rayaop)
   Request initialization function

   This function is called at the beginning of each request. It initializes
   the intercept hash table for the current request.

   @return int Returns SUCCESS on successful initialization
*/
PHP_RINIT_FUNCTION(rayaop) {
    PHP_RAYAOP_DEBUG_PRINT("PHP_RINIT_FUNCTION called"); /* Output debug information */
    if (RAYAOP_G(intercept_ht) == NULL) {
        /* If intercept hash table is not initialized */
        ALLOC_HASHTABLE(RAYAOP_G(intercept_ht)); /* Allocate memory for hash table */
        zend_hash_init(RAYAOP_G(intercept_ht), 8, NULL, php_rayaop_free_intercept_info, 0); /* Initialize hash table */
    }
    RAYAOP_G(is_intercepting) = 0; /* Initialize intercept flag */
    return SUCCESS; /* Return success */
}
/* }}} */

/* {{{ proto int PHP_RSHUTDOWN_FUNCTION(rayaop)
   Request shutdown function

   This function is called at the end of each request. It cleans up the
   intercept hash table and frees associated memory.

   @return int Returns SUCCESS on successful shutdown
*/
PHP_RSHUTDOWN_FUNCTION(rayaop) {
    PHP_RAYAOP_DEBUG_PRINT("RayAOP PHP_RSHUTDOWN_FUNCTION called"); /* Output debug information */
    if (RAYAOP_G(intercept_ht)) {
        /* If intercept hash table exists */
        zend_hash_destroy(RAYAOP_G(intercept_ht)); /* Destroy hash table */
        FREE_HASHTABLE(RAYAOP_G(intercept_ht)); /* Free memory for hash table */
        RAYAOP_G(intercept_ht) = NULL; /* Set hash table pointer to NULL */
    }
    PHP_RAYAOP_DEBUG_PRINT("RayAOP PHP_RSHUTDOWN_FUNCTION shut down"); /* Output debug information */
    return SUCCESS; /* Return shutdown success */
}
/* }}} */

/* {{{ proto void PHP_MINFO_FUNCTION(rayaop)
   Extension information display function

   This function is called to display information about the extension in phpinfo().
*/
PHP_MINFO_FUNCTION(rayaop) {
    php_info_print_table_start(); /* Start information table */
    php_info_print_table_header(2, "rayaop support", "enabled"); /* Display table header */
    php_info_print_table_row(2, "Version", PHP_RAYAOP_VERSION); /* Display version information */
    php_info_print_table_end(); /* End information table */
}
/* }}} */

#ifdef RAYAOP_DEBUG
/* {{{ proto void php_rayaop_dump_intercept_info(void)
   Function to dump intercept information (for debugging)

   This function outputs debug information about the current state of the intercept hash table.
*/
static void php_rayaop_dump_intercept_info(void)
{
    PHP_RAYAOP_DEBUG_PRINT("Dumping intercept information:");  /* Output debug information */
    if (RAYAOP_G(intercept_ht)) {  /* If intercept hash table exists */
        zend_string *key;
        php_rayaop_intercept_info *info;
        ZEND_HASH_FOREACH_STR_KEY_PTR(RAYAOP_G(intercept_ht), key, info) {  /* For each element in the hash table */
            if (key && info) {  /* If key and information exist */
                PHP_RAYAOP_DEBUG_PRINT("Key: %s", ZSTR_VAL(key));  /* Output key */
                PHP_RAYAOP_DEBUG_PRINT("  Class: %s", ZSTR_VAL(info->class_name));  /* Output class name */
                PHP_RAYAOP_DEBUG_PRINT("  Method: %s", ZSTR_VAL(info->method_name));  /* Output method name */
                PHP_RAYAOP_DEBUG_PRINT("  Handler type: %d", Z_TYPE(info->handler));  /* Output handler type */
            }
        } ZEND_HASH_FOREACH_END();
    } else {  /* If intercept hash table does not exist */
        PHP_RAYAOP_DEBUG_PRINT("Intercept hash table is not initialized");  /* Output that it's not initialized */
    }
}
/* }}} */
#endif

/* Definition of functions provided by the extension */
static const zend_function_entry rayaop_functions[] = {
    PHP_FE(method_intercept, arginfo_method_intercept) /* Register method_intercept function */
    PHP_FE(method_intercept_init, arginfo_method_intercept_init) /* Register method_intercept_init function */
    PHP_FE_END /* End of function entries */
};

/* Extension module entry */
zend_module_entry rayaop_module_entry = {
    STANDARD_MODULE_HEADER,
    "rayaop", /* Extension name */
    rayaop_functions, /* Functions provided by the extension */
    PHP_MINIT(rayaop), /* Extension initialization function */
    PHP_MSHUTDOWN(rayaop), /* Extension shutdown function */
    PHP_RINIT(rayaop), /* Function at request start */
    PHP_RSHUTDOWN(rayaop), /* Function at request end */
    PHP_MINFO(rayaop), /* Extension information display function */
    PHP_RAYAOP_VERSION, /* Extension version */
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_RAYAOP
ZEND_GET_MODULE(rayaop)  /* Function to get module for dynamic loading */
#endif
