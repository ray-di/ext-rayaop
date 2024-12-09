#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php_rayaop.h"

/* Module globals initialization */
ZEND_DECLARE_MODULE_GLOBALS(rayaop)

/* Global variable declarations */
zend_class_entry *ray_aop_method_interceptor_interface_ce = NULL;
static void (*php_rayaop_original_execute_ex)(zend_execute_data *execute_data) = NULL;

/* Argument information for the interceptor method */
ZEND_BEGIN_ARG_INFO_EX(arginfo_method_intercept, 0, 0, 3)
    ZEND_ARG_TYPE_INFO(0, class_name, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, method_name, IS_STRING, 0)
    ZEND_ARG_OBJ_INFO(0, interceptor, Ray\\Aop\\MethodInterceptorInterface, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_method_intercept_init, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_method_intercept_enable, 0, 1, IS_VOID, 0)
    ZEND_ARG_TYPE_INFO(0, enable, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

static const zend_function_entry rayaop_functions[] = {
    PHP_FE(method_intercept, arginfo_method_intercept)
    PHP_FE(method_intercept_init, arginfo_method_intercept_init)
    PHP_FE(method_intercept_enable, arginfo_method_intercept_enable)
    PHP_FE_END
};

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_ray_aop_method_interceptor_intercept, 0, 3, IS_MIXED, 0)
    ZEND_ARG_TYPE_INFO(0, object, IS_OBJECT, 0)
    ZEND_ARG_TYPE_INFO(0, method, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, params, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

static const zend_function_entry ray_aop_method_interceptor_interface_methods[] = {
    PHP_ABSTRACT_ME(Ray_Aop_MethodInterceptorInterface, intercept, arginfo_ray_aop_method_interceptor_intercept)
    PHP_FE_END
};

PHP_RAYAOP_API void php_rayaop_handle_error(int error_code, const char *message) {
    int error_level = E_ERROR;
    const char *error_type = "Unknown Error";
    switch (error_code) {
        case RAYAOP_E_MEMORY_ALLOCATION:
            error_type = "Memory Allocation Error";
            break;
        case RAYAOP_E_HASH_UPDATE:
            error_type = "Hash Table Error";
            break;
        case RAYAOP_E_INVALID_HANDLER:
            error_type = "Invalid Handler";
            error_level = E_WARNING;
            break;
        case RAYAOP_E_MAX_DEPTH_EXCEEDED:
            error_type = "Max Depth Exceeded";
            error_level = E_WARNING;
            break;
        case RAYAOP_E_NULL_POINTER:
            error_type = "Null Pointer Error";
            break;
        case RAYAOP_E_INVALID_STATE:
            error_type = "Invalid State";
            break;
        default:
            break;
    }
    php_error_docref(NULL, error_level, "[RayAOP] %s: %s", error_type, message);
}

PHP_RAYAOP_API php_rayaop_intercept_info *php_rayaop_create_intercept_info(void) {
    php_rayaop_intercept_info *info = ecalloc(1, sizeof(php_rayaop_intercept_info));
    if (!info) {
        php_rayaop_handle_error(RAYAOP_E_MEMORY_ALLOCATION, "Failed to allocate intercept info");
        return NULL;
    }
    info->is_enabled = 1;
    ZVAL_UNDEF(&info->handler);
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

PHP_RAYAOP_API char *php_rayaop_generate_key(zend_string *class_name, zend_string *method_name, size_t *key_len) {
    if (!class_name || !method_name) {
        return NULL;
    }
    char *key;
    int len = spprintf(&key, 0, "%s::%s", ZSTR_VAL(class_name), ZSTR_VAL(method_name));
    if (key_len) {
        *key_len = (size_t)len;
    }
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

PHP_RAYAOP_API zend_bool php_rayaop_should_intercept(zend_execute_data *execute_data) {
    if (!RAYAOP_G(method_intercept_enabled)) {
        return 0;
    }

    if (RAYAOP_G(execution_depth) >= MAX_EXECUTION_DEPTH) {
        return 0;
    }

    /* If there's already an exception, do not intercept */
    if (EG(exception)) {
        return 0;
    }

    if (!execute_data || !execute_data->func || !execute_data->func->common.scope || !execute_data->func->common.function_name) {
        return 0;
    }

    if (RAYAOP_G(is_intercepting)) {
        return 0;
    }

    return 1;
}

static void rayaop_execute_ex(zend_execute_data *execute_data) {
    if (EG(exception)) {
        /* If exception is already set, just run original execute_ex */
        if (php_rayaop_original_execute_ex) {
            php_rayaop_original_execute_ex(execute_data);
        } else {
            zend_execute_ex(execute_data);
        }
        return;
    }

    if (!php_rayaop_should_intercept(execute_data)) {
        if (php_rayaop_original_execute_ex) {
            php_rayaop_original_execute_ex(execute_data);
        } else {
            zend_execute_ex(execute_data);
        }
        return;
    }

    RAYAOP_G(execution_depth)++;

    zend_function *func = execute_data->func;
    if (EG(exception)) {
        goto fallback;
    }

    size_t key_len = 0;
    char *key = php_rayaop_generate_key(func->common.scope->name, func->common.function_name, &key_len);
    if (!key) {
        goto fallback;
    }

    php_rayaop_intercept_info *info = php_rayaop_find_intercept_info(key, key_len);
    if (!info || !info->is_enabled) {
        efree(key);
        goto fallback;
    }

    if (Z_TYPE(info->handler) != IS_OBJECT) {
        efree(key);
        goto fallback;
    }

    if (EG(exception)) {
        efree(key);
        goto fallback;
    }

    zval retval;
    zval params[3];
    ZVAL_UNDEF(&retval);

    if (Z_TYPE(execute_data->This) != IS_OBJECT) {
        efree(key);
        goto fallback;
    }

    ZVAL_OBJ(&params[0], Z_OBJ(execute_data->This));
    Z_ADDREF_P(&params[0]);

    ZVAL_STR(&params[1], zend_string_copy(info->method_name));
    array_init(&params[2]);

    uint32_t arg_count = ZEND_CALL_NUM_ARGS(execute_data);
    if (arg_count > 0 && !EG(exception)) {
        zval *args = ZEND_CALL_ARG(execute_data, 1);
        if (args && !EG(exception)) {
            for (uint32_t i = 0; i < arg_count; i++) {
                zval *arg = &args[i];
                if (!Z_ISUNDEF_P(arg)) {
                    Z_TRY_ADDREF_P(arg);
                    add_next_index_zval(&params[2], arg);
                }
            }
        }
    }

    if (EG(exception)) {
        /* Exception occurred during arg processing */
        goto cleanup;
    }

    zval method_name;
    ZVAL_STRING(&method_name, "intercept");

    RAYAOP_G(is_intercepting) = 1;
    int call_result = call_user_function(NULL, &info->handler, &method_name, &retval, 3, params);
    if (call_result == SUCCESS && !EG(exception)) {
        if (execute_data->return_value) {
            if (!Z_ISUNDEF(retval)) {
                ZVAL_COPY(execute_data->return_value, &retval);  // Propagate explicit return values
            } else {
                ZVAL_NULL(execute_data->return_value);  // Handle void or empty returns
            }
        }
    } else if (call_result != SUCCESS) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
            "Interceptor call failed for %s::%s",
            ZSTR_VAL(info->class_name),
            ZSTR_VAL(info->method_name));
        php_rayaop_handle_error(RAYAOP_E_INVALID_HANDLER, error_msg);
    }

cleanup:
    zval_ptr_dtor(&retval);
    zval_ptr_dtor(&method_name);
    zval_ptr_dtor(&params[1]);
    zval_ptr_dtor(&params[2]);
    zval_ptr_dtor(&params[0]);
    RAYAOP_G(is_intercepting) = 0;
    efree(key);

    /* インターセプタ実行が完了し、例外が無い場合はここでreturnし、fallbackへ行かない */
    if (!EG(exception)) {
        RAYAOP_G(execution_depth)--;
        return;
    }

fallback:
    RAYAOP_G(execution_depth)--;

    if (EG(exception)) {
        /* If there's an exception now, just fallback to original or zend_execute_ex */
        if (php_rayaop_original_execute_ex) {
            php_rayaop_original_execute_ex(execute_data);
        } else {
            zend_execute_ex(execute_data);
        }
    } else {
        /* If no exception and no interception, call original */
        if (php_rayaop_original_execute_ex) {
            php_rayaop_original_execute_ex(execute_data);
        } else {
            zend_execute_ex(execute_data);
        }
    }
}

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
    if (!key) {
        zval tmp_zv;
        ZVAL_PTR(&tmp_zv, info);
        php_rayaop_free_intercept_info(&tmp_zv);
        RETURN_FALSE;
    }

    RAYAOP_G_LOCK();
    php_rayaop_intercept_info *existing = zend_hash_str_find_ptr(RAYAOP_G(intercept_ht), key, key_len);
    if (existing) {
        /* 上書き: 前のhandlerを解放して新しいhandlerを登録 */
        zval_ptr_dtor(&existing->handler);
        ZVAL_COPY(&existing->handler, interceptor);
        zend_string_release(existing->class_name);
        zend_string_release(existing->method_name);
        existing->class_name = info->class_name;
        existing->method_name = info->method_name;
        efree(info);
    } else {
        if (zend_hash_str_update_ptr(RAYAOP_G(intercept_ht), key, key_len, info) == NULL) {
            RAYAOP_G_UNLOCK();
            efree(key);
            zval tmp_zv;
            ZVAL_PTR(&tmp_zv, info);
            php_rayaop_free_intercept_info(&tmp_zv);
            php_rayaop_handle_error(RAYAOP_E_HASH_UPDATE, "Failed to update intercept hash table");
            RETURN_FALSE;
        }
    }
    RAYAOP_G_UNLOCK();

    efree(key);
    RETURN_TRUE;
}

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

PHP_FUNCTION(method_intercept_enable) {
    zend_bool enable;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_BOOL(enable)
    ZEND_PARSE_PARAMETERS_END();

    RAYAOP_G_LOCK();
    RAYAOP_G(method_intercept_enabled) = enable;
    if (enable) {
        zend_execute_ex = rayaop_execute_ex;
    } else {
        zend_execute_ex = php_rayaop_original_execute_ex;
    }
    RAYAOP_G_UNLOCK();
}

PHP_MINIT_FUNCTION(rayaop) {
#ifdef ZTS
    ts_allocate_id(&rayaop_globals_id, sizeof(zend_rayaop_globals), NULL, NULL);
    rayaop_mutex = tsrm_mutex_alloc();

    if (!rayaop_mutex) {
        ts_free_id(rayaop_globals_id);
        php_error_docref(NULL, E_ERROR, "Failed to allocate mutex for RayAOP");
        return FAILURE;
    }
#endif

    zend_class_entry ce;
    INIT_NS_CLASS_ENTRY(ce, "Ray\\Aop", "MethodInterceptorInterface", ray_aop_method_interceptor_interface_methods);
    ray_aop_method_interceptor_interface_ce = zend_register_internal_interface(&ce);

    RAYAOP_G(method_intercept_enabled) = 1;
    RAYAOP_G(debug_level) = 0;
    php_rayaop_original_execute_ex = zend_execute_ex;
    zend_execute_ex = rayaop_execute_ex;

    return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(rayaop) {
#ifdef ZTS
    if (rayaop_mutex) {
        tsrm_mutex_free(rayaop_mutex);
        rayaop_mutex = NULL;
    }
#endif

    if (php_rayaop_original_execute_ex) {
        zend_execute_ex = php_rayaop_original_execute_ex;
    }
    return SUCCESS;
}

PHP_RINIT_FUNCTION(rayaop) {
    if (!php_rayaop_original_execute_ex) {
        php_rayaop_original_execute_ex = zend_execute_ex;
    }

    RAYAOP_G_LOCK();
    if (!RAYAOP_G(intercept_ht)) {
        ALLOC_HASHTABLE(RAYAOP_G(intercept_ht));
        if (!RAYAOP_G(intercept_ht)) {
            RAYAOP_G_UNLOCK();
            php_rayaop_handle_error(RAYAOP_E_MEMORY_ALLOCATION, "Failed to allocate intercept hash table");
            return FAILURE;
        }
        zend_hash_init(RAYAOP_G(intercept_ht), 8, NULL, (dtor_func_t)php_rayaop_free_intercept_info, 0);
    }
    RAYAOP_G(is_intercepting) = 0;
    RAYAOP_G(execution_depth) = 0;

    if (RAYAOP_G(method_intercept_enabled)) {
        zend_execute_ex = rayaop_execute_ex;
    }

    RAYAOP_G_UNLOCK();
    return SUCCESS;
}

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

PHP_MINFO_FUNCTION(rayaop) {
    php_info_print_table_start();
    php_info_print_table_header(2, "RayAOP Support", "enabled");
    php_info_print_table_row(2, "Version", PHP_RAYAOP_VERSION);
    php_info_print_table_row(2, "Debug Level",
        RAYAOP_G(debug_level) == 0 ? "None" : "Basic");
    php_info_print_table_row(2, "Method Intercept",
        RAYAOP_G(method_intercept_enabled) ? "Enabled" : "Disabled");
    php_info_print_table_end();
}

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
