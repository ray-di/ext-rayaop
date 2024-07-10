#ifdef HAVE_CONFIG_H
#include "config.h"  /* Include configuration file */
#endif

#include "php_rayaop.h"  /* Include header file for RayAOP extension */

/* Declaration of module global variables */
ZEND_DECLARE_MODULE_GLOBALS(rayaop)

/* Declaration of static variable: pointer to the original zend_execute_ex function */
static void (*original_zend_execute_ex)(zend_execute_data *execute_data);

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
#define RAYAOP_DEBUG_PRINT(fmt, ...) php_printf("RAYAOP DEBUG: " fmt "\n", ##__VA_ARGS__)  /* Output debug information */
#else
#define RAYAOP_DEBUG_PRINT(fmt, ...)  /* Do nothing if not in debug mode */
#endif

/* Argument information for method_intercept function */
ZEND_BEGIN_ARG_INFO_EX(arginfo_method_intercept, 0, 0, 3)
    ZEND_ARG_TYPE_INFO(0, class_name, IS_STRING, 0) /* Argument information for class name */
    ZEND_ARG_TYPE_INFO(0, method_name, IS_STRING, 0) /* Argument information for method name */
    ZEND_ARG_OBJ_INFO(0, interceptor, Ray\\Aop\\MethodInterceptorInterface, 0) /* Argument information for intercept handler */
ZEND_END_ARG_INFO()

/* {{{ proto void rayaop_handle_error(const char *message)
   Error handling function
   
   This function handles errors by outputting an error message.
   
   @param const char *message Error message to be displayed
*/
void rayaop_handle_error(const char *message) {
    php_error_docref(NULL, E_ERROR, "Memory error: %s", message); /* Output error message */
}
/* }}} */

/* {{{ proto bool rayaop_should_intercept(zend_execute_data *execute_data)
   Function to determine if interception is necessary
   
   This function checks if the current execution should be intercepted.
   
   @param zend_execute_data *execute_data Execution data
   @return bool Returns true if interception is necessary, false otherwise
*/
bool rayaop_should_intercept(zend_execute_data *execute_data) {
    return execute_data->func->common.scope && /* Scope exists */
           execute_data->func->common.function_name && /* Function name exists */
           !RAYAOP_G(is_intercepting); /* Not currently intercepting */
}
/* }}} */

/* {{{ proto char* rayaop_generate_intercept_key(zend_string *class_name, zend_string *method_name, size_t *key_len)
   Function to generate intercept key
   
   This function creates a key in the format "class_name::method_name" for use in the intercept hash table.
   
   @param zend_string *class_name The name of the class
   @param zend_string *method_name The name of the method
   @param size_t *key_len Pointer to store the length of the generated key
   @return char* The generated key, which must be freed by the caller using efree()
*/
char *rayaop_generate_intercept_key(zend_string *class_name, zend_string *method_name, size_t *key_len) {
    char *key = NULL;
    *key_len = spprintf(&key, 0, "%s::%s", ZSTR_VAL(class_name), ZSTR_VAL(method_name));
    /* Generate key in the format class_name::method_name */
    RAYAOP_DEBUG_PRINT("Generated key: %s", key); /* Output generated key for debugging */
    return key;
}
/* }}} */

/* {{{ proto intercept_info* rayaop_find_intercept_info(const char *key, size_t key_len)
   Function to search for intercept information
   
   This function searches for intercept information in the hash table using the provided key.
   
   @param const char *key The key to search for
   @param size_t key_len The length of the key
   @return intercept_info* Pointer to the intercept information if found, NULL otherwise
*/
intercept_info *rayaop_find_intercept_info(const char *key, size_t key_len) {
    return zend_hash_str_find_ptr(RAYAOP_G(intercept_ht), key, key_len);
    /* Search for intercept information in hash table */
}
/* }}} */

/* {{{ proto void rayaop_execute_intercept(zend_execute_data *execute_data, intercept_info *info)
   Function to execute interception
   
   This function executes the interception logic for a given method call.
   
   @param zend_execute_data *execute_data The execution data
   @param intercept_info *info The intercept information
*/
void rayaop_execute_intercept(zend_execute_data *execute_data, intercept_info *info) {
    if (Z_TYPE(info->handler) != IS_OBJECT) {
        /* If handler is not an object */
        return; /* Abort processing */
    }

    if (execute_data->This.value.obj == NULL) {
        /* If target object is NULL */
        RAYAOP_DEBUG_PRINT("Object is NULL, calling original function"); /* Output debug information */
        original_zend_execute_ex(execute_data); /* Execute original function */
        return; /* Abort processing */
    }

    zval retval, params[3]; /* Prepare variables to store return value and arguments */
    ZVAL_OBJ(&params[0], execute_data->This.value.obj); /* 1st argument: target object */
    ZVAL_STR(&params[1], info->method_name); /* 2nd argument: method name */

    array_init(&params[2]); /* Initialize array to store method arguments as 3rd argument */
    uint32_t arg_count = ZEND_CALL_NUM_ARGS(execute_data); /* Get number of arguments */
    zval *args = ZEND_CALL_ARG(execute_data, 1); /* Get argument array */
    for (uint32_t i = 0; i < arg_count; i++) {
        /* For each argument */
        zval *arg = &args[i];
        Z_TRY_ADDREF_P(arg); /* Increase reference count */
        add_next_index_zval(&params[2], arg); /* Add to array */
    }

    RAYAOP_G(is_intercepting) = 1; /* Set intercept flag */
    zval func_name;
    ZVAL_STRING(&func_name, "intercept"); /* Set function name to call */

    ZVAL_UNDEF(&retval); /* Initialize return value as undefined */
    if (call_user_function(NULL, &info->handler, &func_name, &retval, 3, params) == SUCCESS) {
        /* Call intercept function */
        if (!Z_ISUNDEF(retval)) {
            /* If return value is defined */
            ZVAL_COPY(execute_data->return_value, &retval); /* Copy return value */
        }
    } else {
        /* If interception fails */
        php_error_docref(NULL, E_WARNING, "Interception failed for %s::%s", ZSTR_VAL(info->class_name),
                         ZSTR_VAL(info->method_name)); /* Output warning */
    }

    zval_ptr_dtor(&retval); /* Free memory for return value */
    zval_ptr_dtor(&func_name); /* Free memory for function name */
    zval_ptr_dtor(&params[1]); /* Free memory for method name */
    zval_ptr_dtor(&params[2]); /* Free memory for argument array */

    RAYAOP_G(is_intercepting) = 0; /* Reset intercept flag */
}
/* }}} */

/* {{{ proto void rayaop_free_intercept_info(zval *zv)
   Function to free intercept information
   
   This function frees the memory allocated for intercept information.
   
   @param zval *zv The zval containing the intercept information to be freed
*/
void rayaop_free_intercept_info(zval *zv) {
    intercept_info *info = Z_PTR_P(zv); /* Get intercept information pointer from zval */
    if (info) {
        /* If intercept information exists */
        RAYAOP_DEBUG_PRINT("Freeing intercept info for %s::%s", ZSTR_VAL(info->class_name), ZSTR_VAL(info->method_name));
        /* Output debug information */
        zend_string_release(info->class_name); /* Free memory for class name */
        zend_string_release(info->method_name); /* Free memory for method name */
        zval_ptr_dtor(&info->handler); /* Free memory for handler */
        efree(info); /* Free memory for intercept information structure */
    }
}
/* }}} */

/* {{{ proto void rayaop_zend_execute_ex(zend_execute_data *execute_data)
   Custom zend_execute_ex function
   
   This function replaces the original zend_execute_ex function to implement method interception.
   
   @param zend_execute_data *execute_data The execution data
*/
static void rayaop_zend_execute_ex(zend_execute_data *execute_data) {
    RAYAOP_DEBUG_PRINT("rayaop_zend_execute_ex called"); /* Output debug information */

    if (!rayaop_should_intercept(execute_data)) {
        /* If interception is not necessary */
        original_zend_execute_ex(execute_data); /* Call the original execution function */
        return; /* End processing */
    }

    zend_function *current_function = execute_data->func; /* Get current function information */
    zend_string *class_name = current_function->common.scope->name; /* Get class name */
    zend_string *method_name = current_function->common.function_name; /* Get method name */

    size_t key_len;
    char *key = rayaop_generate_intercept_key(class_name, method_name, &key_len); /* Generate intercept key */

    intercept_info *info = rayaop_find_intercept_info(key, key_len); /* Search for intercept information */

    if (info) {
        /* If intercept information is found */
        RAYAOP_DEBUG_PRINT("Found intercept info for key: %s", key); /* Output debug information */
        rayaop_execute_intercept(execute_data, info); /* Execute interception */
    } else {
        /* If intercept information is not found */
        RAYAOP_DEBUG_PRINT("No intercept info found for key: %s", key); /* Output debug information */
        original_zend_execute_ex(execute_data); /* Call the original execution function */
    }

    efree(key); /* Free memory for key */
}
/* }}} */

/* {{{ proto void hash_update_failed(intercept_info *new_info, char *key)
   Handling for hash table update failure
   
   This function handles the case when updating the intercept hash table fails.
   
   @param intercept_info *new_info The new intercept information that failed to be added
   @param char *key The key that was used for the failed update
*/
void hash_update_failed(intercept_info *new_info, char *key) {
    rayaop_handle_error("Failed to update intercept hash table"); /* Output error message */
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
    RAYAOP_DEBUG_PRINT("method_intercept called"); /* Output debug information */

    char *class_name, *method_name; /* Pointers for class name and method name */
    size_t class_name_len, method_name_len; /* Lengths of class name and method name */
    zval *intercepted; /* Intercept handler */

    ZEND_PARSE_PARAMETERS_START(3, 3)
        Z_PARAM_STRING(class_name, class_name_len) /* Parse class name parameter */
        Z_PARAM_STRING(method_name, method_name_len) /* Parse method name parameter */
        Z_PARAM_OBJECT(intercepted) /* Parse intercept handler parameter */
    ZEND_PARSE_PARAMETERS_END();

    intercept_info *new_info = ecalloc(1, sizeof(intercept_info)); /* Allocate memory for new intercept information */
    if (!new_info) {
        /* If memory allocation fails */
        rayaop_handle_error("Failed to allocate memory for intercept_info"); /* Output error message */
        RETURN_FALSE; /* Return false and end */
    }

    new_info->class_name = zend_string_init(class_name, class_name_len, 0); /* Initialize class name */
    new_info->method_name = zend_string_init(method_name, method_name_len, 0); /* Initialize method name */
    ZVAL_COPY(&new_info->handler, intercepted); /* Copy intercept handler */

    char *key = NULL;
    size_t key_len = spprintf(&key, 0, "%s::%s", class_name, method_name); /* Generate intercept key */

    if (zend_hash_str_update_ptr(RAYAOP_G(intercept_ht), key, key_len, new_info) == NULL) {
        /* Add to hash table */
        hash_update_failed(new_info, key); /* Execute error handling if addition fails */
        RETURN_FALSE; /* Return false and end */
    }

    efree(key); /* Free memory for key */
    RAYAOP_DEBUG_PRINT("Successfully registered intercept info"); /* Output debug information */
    RETURN_TRUE; /* Return true and end */
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
    RAYAOP_DEBUG_PRINT("PHP_MINIT_FUNCTION called"); /* Output debug information */

#ifdef ZTS
    ts_allocate_id(&rayaop_globals_id, sizeof(zend_rayaop_globals), (ts_allocate_ctor) php_rayaop_init_globals, NULL);  /* Initialize global variables in thread-safe mode */
#else
    php_rayaop_init_globals(&rayaop_globals); /* Initialize global variables in non-thread-safe mode */
#endif

    zend_class_entry ce;
    INIT_CLASS_ENTRY(ce, "Ray\\Aop\\MethodInterceptorInterface", ray_aop_method_interceptor_interface_methods);
    /* Initialize class entry */
    ray_aop_method_interceptor_interface_ce = zend_register_internal_interface(&ce); /* Register interface */

    original_zend_execute_ex = zend_execute_ex; /* Save the original zend_execute_ex function */
    zend_execute_ex = rayaop_zend_execute_ex; /* Set the custom zend_execute_ex function */

    RAYAOP_DEBUG_PRINT("RayAOP extension initialized"); /* Output debug information */
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
    RAYAOP_DEBUG_PRINT("RayAOP PHP_MSHUTDOWN_FUNCTION called"); /* Output debug information */
    zend_execute_ex = original_zend_execute_ex; /* Restore the original zend_execute_ex function */
    original_zend_execute_ex = NULL; /* Clear the saved pointer */
    RAYAOP_DEBUG_PRINT("RayAOP PHP_MSHUTDOWN_FUNCTION shut down"); /* Output debug information */
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
    RAYAOP_DEBUG_PRINT("PHP_RINIT_FUNCTION called"); /* Output debug information */
    if (RAYAOP_G(intercept_ht) == NULL) {
        /* If intercept hash table is not initialized */
        ALLOC_HASHTABLE(RAYAOP_G(intercept_ht)); /* Allocate memory for hash table */
        zend_hash_init(RAYAOP_G(intercept_ht), 8, NULL, rayaop_free_intercept_info, 0); /* Initialize hash table */
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
    RAYAOP_DEBUG_PRINT("RayAOP PHP_RSHUTDOWN_FUNCTION called"); /* Output debug information */
    if (RAYAOP_G(intercept_ht)) {
        /* If intercept hash table exists */
        zend_hash_destroy(RAYAOP_G(intercept_ht)); /* Destroy hash table */
        FREE_HASHTABLE(RAYAOP_G(intercept_ht)); /* Free memory for hash table */
        RAYAOP_G(intercept_ht) = NULL; /* Set hash table pointer to NULL */
    }
    RAYAOP_DEBUG_PRINT("RayAOP PHP_RSHUTDOWN_FUNCTION shut down"); /* Output debug information */
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
/* {{{ proto void rayaop_dump_intercept_info(void)
   Function to dump intercept information (for debugging)

   This function outputs debug information about the current state of the intercept hash table.
*/
static void rayaop_dump_intercept_info(void)
{
    RAYAOP_DEBUG_PRINT("Dumping intercept information:");  /* Output debug information */
    if (RAYAOP_G(intercept_ht)) {  /* If intercept hash table exists */
        zend_string *key;
        intercept_info *info;
        ZEND_HASH_FOREACH_STR_KEY_PTR(RAYAOP_G(intercept_ht), key, info) {  /* For each element in the hash table */
            if (key && info) {  /* If key and information exist */
                RAYAOP_DEBUG_PRINT("Key: %s", ZSTR_VAL(key));  /* Output key */
                RAYAOP_DEBUG_PRINT("  Class: %s", ZSTR_VAL(info->class_name));  /* Output class name */
                RAYAOP_DEBUG_PRINT("  Method: %s", ZSTR_VAL(info->method_name));  /* Output method name */
                RAYAOP_DEBUG_PRINT("  Handler type: %d", Z_TYPE(info->handler));  /* Output handler type */
            }
        } ZEND_HASH_FOREACH_END();
    } else {  /* If intercept hash table does not exist */
        RAYAOP_DEBUG_PRINT("Intercept hash table is not initialized");  /* Output that it's not initialized */
    }
}
/* }}} */
#endif

/* Definition of functions provided by the extension */
static const zend_function_entry rayaop_functions[] = {
    PHP_FE(method_intercept, arginfo_method_intercept) /* Register method_intercept function */
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