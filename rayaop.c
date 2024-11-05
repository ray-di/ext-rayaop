#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php_rayaop.h"

/* Module globals initialization */
ZEND_DECLARE_MODULE_GLOBALS(rayaop)

/* Global variable declarations */
zend_class_entry *ray_aop_method_interceptor_interface_ce;
static void (*php_rayaop_original_execute_ex)(zend_execute_data *execute_data) = NULL;

/* Argument information for the interceptor method */
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_ray_aop_method_interceptor_intercept, 0, 3, IS_MIXED, 0)
    ZEND_ARG_TYPE_INFO(0, object, IS_OBJECT, 0)
    ZEND_ARG_TYPE_INFO(0, method, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, params, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

/* Interface methods */
static zend_function_entry ray_aop_method_interceptor_interface_methods[] = {
    PHP_ABSTRACT_ME(Ray_Aop_MethodInterceptorInterface, intercept, arginfo_ray_aop_method_interceptor_intercept)
    PHP_FE_END
};

/* Module function declarations */
static const zend_function_entry rayaop_functions[] = {
    PHP_FE(method_intercept, arginfo_method_intercept)
    PHP_FE(method_intercept_init, arginfo_method_intercept_init)
    PHP_FE(enable_method_intercept, arginfo_enable_method_intercept)
    PHP_FE_END
};



/* Module globals initializer */
static void php_rayaop_init_globals(zend_rayaop_globals *globals) {
    globals->intercept_ht = NULL;
    globals->is_intercepting = 0;
    globals->execution_depth = 0;
    globals->method_intercept_enabled = 0;
    globals->debug_level = 0;
}

/* Error handling function */
PHP_RAYAOP_API void php_rayaop_handle_error(int error_code, const char *message) {
    switch (error_code) {
        case RAYAOP_E_MEMORY_ALLOCATION:
            php_error_docref(NULL, E_ERROR, "Memory allocation failed: %s", message);
            break;
        case RAYAOP_E_HASH_UPDATE:
            php_error_docref(NULL, E_ERROR, "Hash table update failed: %s", message);
            break;
        case RAYAOP_E_INVALID_HANDLER:
            php_error_docref(NULL, E_WARNING, "Invalid handler: %s", message);
            break;
        case RAYAOP_E_MAX_DEPTH_EXCEEDED:
            php_error_docref(NULL, E_WARNING, "Maximum execution depth exceeded: %s", message);
            break;
        default:
            php_error_docref(NULL, E_ERROR, "RayAOP Error (%d): %s", error_code, message);
    }
}

/* Memory management functions */
PHP_RAYAOP_API php_rayaop_intercept_info *php_rayaop_create_intercept_info(void) {
    php_rayaop_intercept_info *info = ecalloc(1, sizeof(php_rayaop_intercept_info));
    if (!info) {
        php_rayaop_handle_error(RAYAOP_E_MEMORY_ALLOCATION, "Failed to allocate intercept info");
        return NULL;
    }
    info->is_enabled = 1;
    return info;
}

PHP_RAYAOP_API void php_rayaop_free_intercept_info(zval *zv) {
    RAYAOP_G_LOCK();
    php_rayaop_intercept_info *info = Z_PTR_P(zv);
    if (info) {
        if (info->class_name) {
            zend_string_release(info->class_name);
        }
        if (info->method_name) {
            zend_string_release(info->method_name);
        }
        zval_ptr_dtor(&info->handler);
        efree(info);
    }
    RAYAOP_G_UNLOCK();
}

/* Interception helper functions */
PHP_RAYAOP_API bool php_rayaop_should_intercept(zend_execute_data *execute_data) {
    if (!RAYAOP_G(method_intercept_enabled)) {
        return false;
    }

    if (RAYAOP_G(execution_depth) >= MAX_EXECUTION_DEPTH) {
        php_rayaop_handle_error(RAYAOP_E_MAX_DEPTH_EXCEEDED, "Maximum execution depth reached");
        return false;
    }

    return execute_data &&
           execute_data->func &&
           execute_data->func->common.scope &&
           execute_data->func->common.function_name &&
           !RAYAOP_G(is_intercepting);
}

PHP_RAYAOP_API char *php_rayaop_generate_key(zend_string *class_name, zend_string *method_name, size_t *key_len) {
    char *key = NULL;
    *key_len = spprintf(&key, 0, "%s::%s", ZSTR_VAL(class_name), ZSTR_VAL(method_name));
    PHP_RAYAOP_DEBUG_PRINT("Generated key: %s", key);
    return key;
}

PHP_RAYAOP_API php_rayaop_intercept_info *php_rayaop_find_intercept_info(const char *key, size_t key_len) {
    RAYAOP_G_LOCK();
    php_rayaop_intercept_info *info = NULL;
    if (RAYAOP_G(intercept_ht)) {
        info = zend_hash_str_find_ptr(RAYAOP_G(intercept_ht), key, key_len);
    }
    RAYAOP_G_UNLOCK();
    return info;
}

/* Parameter preparation and cleanup */
static void prepare_intercept_params(zend_execute_data *execute_data, zval *params, php_rayaop_intercept_info *info) {
    if (!execute_data->This.value.obj) {
        php_rayaop_handle_error(RAYAOP_E_INVALID_HANDLER, "Object instance is NULL");
        return;
    }

    ZVAL_OBJ(&params[0], execute_data->This.value.obj);
    ZVAL_STR_COPY(&params[1], info->method_name);

    array_init(&params[2]);
    uint32_t arg_count = ZEND_CALL_NUM_ARGS(execute_data);
    if (arg_count > 0) {
        zval *args = ZEND_CALL_ARG(execute_data, 1);
        for (uint32_t i = 0; i < arg_count; i++) {
            zval *arg = &args[i];
            if (!Z_ISUNDEF_P(arg)) {
                Z_TRY_ADDREF_P(arg);
                add_next_index_zval(&params[2], arg);
            }
        }
    }
}

static void cleanup_intercept_params(zval *params) {
    zval_ptr_dtor(&params[1]);
    zval_ptr_dtor(&params[2]);
}

/* Interception execution */
static bool execute_intercept_handler(zval *handler, zval *params, zval *retval) {
    zval method_name;
    ZVAL_STRING(&method_name, "intercept");

    bool success = (call_user_function(NULL, handler, &method_name, retval, 3, params) == SUCCESS);
    zval_ptr_dtor(&method_name);

    if (!success) {
        php_rayaop_handle_error(RAYAOP_E_INVALID_HANDLER, "Failed to execute intercept handler");
    }

    return success;
}

/* Fixed rayaop_execute_ex function */
static void rayaop_execute_ex(zend_execute_data *execute_data) {
    if (!php_rayaop_should_intercept(execute_data)) {
        if (php_rayaop_original_execute_ex) {
            php_rayaop_original_execute_ex(execute_data);
        } else {
            zend_execute_ex(execute_data);
        }
        return;
    }

    RAYAOP_G(execution_depth)++;
    PHP_RAYAOP_DEBUG_PRINT("Execution depth: %d", RAYAOP_G(execution_depth));

    zend_function *func = execute_data->func;
    size_t key_len;
    char *key = php_rayaop_generate_key(func->common.scope->name, func->common.function_name, &key_len);

    php_rayaop_intercept_info *info = php_rayaop_find_intercept_info(key, key_len);
    if (info && info->is_enabled) {
        PHP_RAYAOP_DEBUG_PRINT("Executing intercept for %s", key);

        if (Z_TYPE(info->handler) != IS_OBJECT) {
            php_rayaop_handle_error(RAYAOP_E_INVALID_HANDLER, "Invalid interceptor type");
            if (php_rayaop_original_execute_ex) {
                php_rayaop_original_execute_ex(execute_data);
            }
        } else {
            zval retval;
            zval params[3];

            prepare_intercept_params(execute_data, params, info);
            RAYAOP_G(is_intercepting) = 1;

            ZVAL_UNDEF(&retval);
            if (execute_intercept_handler(&info->handler, params, &retval)) {
                if (!Z_ISUNDEF(retval) && execute_data->return_value) {
                    ZVAL_COPY(execute_data->return_value, &retval);
                }
                zval_ptr_dtor(&retval);
            }

            cleanup_intercept_params(params);
            RAYAOP_G(is_intercepting) = 0;
        }
    } else {
        if (php_rayaop_original_execute_ex) {
            php_rayaop_original_execute_ex(execute_data);
        }
    }

    efree(key);
    RAYAOP_G(execution_depth)--;
}
/* Implementation of method_intercept function */
PHP_FUNCTION(method_intercept) {
    char *class_name, *method_name;
    size_t class_name_len, method_name_len;
    zval *interceptor;

    ZEND_PARSE_PARAMETERS_START(3, 3)
        Z_PARAM_STRING(class_name, class_name_len)
        Z_PARAM_STRING(method_name, method_name_len)
        Z_PARAM_OBJECT(interceptor)
    ZEND_PARSE_PARAMETERS_END();

    php_rayaop_intercept_info *info = php_rayaop_create_intercept_info();
    if (!info) {
        RETURN_FALSE;
    }

    info->class_name = zend_string_init(class_name, class_name_len, 0);
    info->method_name = zend_string_init(method_name, method_name_len, 0);
    ZVAL_COPY(&info->handler, interceptor);

    char *key;
    size_t key_len;
    key = php_rayaop_generate_key(info->class_name, info->method_name, &key_len);

    RAYAOP_G_LOCK();
    if (zend_hash_str_update_ptr(RAYAOP_G(intercept_ht), key, key_len, info) == NULL) {
        RAYAOP_G_UNLOCK();
        zend_string_release(info->class_name);
        zend_string_release(info->method_name);
        zval_ptr_dtor(&info->handler);
        efree(info);
        efree(key);
        php_rayaop_handle_error(RAYAOP_E_HASH_UPDATE, "Failed to update intercept hash table");
        RETURN_FALSE;
    }
    RAYAOP_G_UNLOCK();

    efree(key);
    RETURN_TRUE;
}

/* method_intercept_init function fix */
PHP_FUNCTION(method_intercept_init) {
    RAYAOP_G_LOCK();
    if (RAYAOP_G(intercept_ht)) {
        zend_hash_clean(RAYAOP_G(intercept_ht));
    } else {
        ALLOC_HASHTABLE(RAYAOP_G(intercept_ht));
        if (!RAYAOP_G(intercept_ht)) {
            RAYAOP_G_UNLOCK();
            php_rayaop_handle_error(RAYAOP_E_MEMORY_ALLOCATION, "Failed to allocate intercept hash table");
            RETURN_FALSE;
        }
        zend_hash_init(RAYAOP_G(intercept_ht), 8, NULL, (dtor_func_t)php_rayaop_free_intercept_info, 0);
    }
    RAYAOP_G_UNLOCK();
    RETURN_TRUE;
}

/* Implementation of enable_method_intercept function */
PHP_FUNCTION(enable_method_intercept) {
    zend_bool enable;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_BOOL(enable)
    ZEND_PARSE_PARAMETERS_END();

    RAYAOP_G_LOCK();
    RAYAOP_G(method_intercept_enabled) = enable;
    if (enable) {
        zend_execute_ex = rayaop_execute_ex;
        PHP_RAYAOP_DEBUG_PRINT("Method intercept enabled");
    } else {
        zend_execute_ex = php_rayaop_original_execute_ex;
        PHP_RAYAOP_DEBUG_PRINT("Method intercept disabled");
    }
    RAYAOP_G_UNLOCK();
}

/* Module initialization */
PHP_MINIT_FUNCTION(rayaop) {
#ifdef ZTS
    ts_allocate_id(&rayaop_globals_id, sizeof(zend_rayaop_globals),
                   (ts_allocate_ctor)php_rayaop_init_globals, NULL);
#else
    php_rayaop_init_globals(&rayaop_globals);
#endif

    // Register the interface
    zend_class_entry ce;
    INIT_NS_CLASS_ENTRY(ce, "Ray\\Aop", "MethodInterceptorInterface", ray_aop_method_interceptor_interface_methods);
    ray_aop_method_interceptor_interface_ce = zend_register_internal_interface(&ce);

    // Initialize other settings
    RAYAOP_G(method_intercept_enabled) = 1;
    RAYAOP_G(debug_level) = 0;
    php_rayaop_original_execute_ex = zend_execute_ex;
    zend_execute_ex = rayaop_execute_ex;

    return SUCCESS;
}


/* Module shutdown */
PHP_MSHUTDOWN_FUNCTION(rayaop) {
    if (php_rayaop_original_execute_ex) {
        zend_execute_ex = php_rayaop_original_execute_ex;
    }
    return SUCCESS;
}

/* Request initialization function fix */
PHP_RINIT_FUNCTION(rayaop) {
    RAYAOP_G_LOCK();
    if (!RAYAOP_G(intercept_ht)) {
        ALLOC_HASHTABLE(RAYAOP_G(intercept_ht));
        if (!RAYAOP_G(intercept_ht)) {
            RAYAOP_G_UNLOCK();
            php_rayaop_handle_error(RAYAOP_E_MEMORY_ALLOCATION, "Failed to initialize intercept hash table");
            return FAILURE;
        }
        zend_hash_init(RAYAOP_G(intercept_ht), 8, NULL, (dtor_func_t)php_rayaop_free_intercept_info, 0);
    }
    RAYAOP_G(is_intercepting) = 0;
    RAYAOP_G(execution_depth) = 0;
    RAYAOP_G_UNLOCK();
    return SUCCESS;
}

/* Request shutdown */
PHP_RSHUTDOWN_FUNCTION(rayaop) {
    RAYAOP_G_LOCK();
    if (RAYAOP_G(intercept_ht)) {
        zend_hash_destroy(RAYAOP_G(intercept_ht));
        FREE_HASHTABLE(RAYAOP_G(intercept_ht));
        RAYAOP_G(intercept_ht) = NULL;
    }
    RAYAOP_G_UNLOCK();
    return SUCCESS;
}

/* Module info */
PHP_MINFO_FUNCTION(rayaop) {
    php_info_print_table_start();
    php_info_print_table_header(2, "RayAOP Support", "enabled");
    php_info_print_table_row(2, "Version", PHP_RAYAOP_VERSION);
    php_info_print_table_row(2, "Debug Level",
        RAYAOP_G(debug_level) == 0 ? "None" :
        RAYAOP_G(debug_level) == 1 ? "Basic" : "Verbose");
    php_info_print_table_row(2, "Method Intercept",
        RAYAOP_G(method_intercept_enabled) ? "Enabled" : "Disabled");
    php_info_print_table_end();
}

#ifdef RAYAOP_DEBUG
/* Debug functions */
void php_rayaop_debug_print_zval(zval *value) {
    if (!value) {
        php_printf("NULL\n");
        return;
    }

    switch (Z_TYPE_P(value)) {
        case IS_NULL:
            php_printf("NULL\n");
            break;
        case IS_TRUE:
            php_printf("bool(true)\n");
            break;
        case IS_FALSE:
            php_printf("bool(false)\n");
            break;
        case IS_LONG:
            php_printf("int(%ld)\n", Z_LVAL_P(value));
            break;
        case IS_DOUBLE:
            php_printf("float(%g)\n", Z_DVAL_P(value));
            break;
        case IS_STRING:
            php_printf("string(%d) \"%s\"\n", Z_STRLEN_P(value), Z_STRVAL_P(value));
            break;
        case IS_ARRAY:
            php_printf("array(%d) {...}\n", zend_hash_num_elements(Z_ARRVAL_P(value)));
            break;
        case IS_OBJECT:
            php_printf("object(%s)#%d {...}\n",
                Z_OBJCE_P(value)->name->val, Z_OBJ_HANDLE_P(value));
            break;
        default:
            php_printf("unknown type(%d)\n", Z_TYPE_P(value));
    }
}

void php_rayaop_debug_dump_intercept_info(void) {
    RAYAOP_G_LOCK();
    if (RAYAOP_G(intercept_ht)) {
        php_printf("=== Intercept Information Dump ===\n");
        php_rayaop_intercept_info *info;
        zend_string *key;
        ZEND_HASH_FOREACH_STR_KEY_PTR(RAYAOP_G(intercept_ht), key, info) {
            if (key && info) {
                php_printf("Key: %s\n", ZSTR_VAL(key));
                php_printf("  Class: %s\n", ZSTR_VAL(info->class_name));
                php_printf("  Method: %s\n", ZSTR_VAL(info->method_name));
                php_printf("  Enabled: %d\n", info->is_enabled);
                php_printf("  Handler type: %d\n", Z_TYPE(info->handler));
            }
        } ZEND_HASH_FOREACH_END();
        php_printf("================================\n");
    } else {
        php_printf("No intercept information available\n");
    }
    RAYAOP_G_UNLOCK();
}
#endif

/* Module entry */
zend_module_entry rayaop_module_entry = {
    STANDARD_MODULE_HEADER,
    "rayaop",
    rayaop_functions,
    PHP_MINIT(rayaop),
    PHP_MSHUTDOWN(rayaop),
    PHP_RINIT(rayaop),
    PHP_RSHUTDOWN(rayaop),
    PHP_MINFO(rayaop),
    PHP_RAYAOP_VERSION,
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_RAYAOP
ZEND_GET_MODULE(rayaop)
#endif