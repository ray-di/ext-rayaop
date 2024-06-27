#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_rayaop.h"
#include "zend_exceptions.h"

#ifdef ZTS
#include "TSRM.h"
#endif

#define RAYAOP_DEBUG

typedef struct _intercept_info {
    zend_string *class_name;
    zend_string *method_name;
    zval handler;
} intercept_info;

static HashTable *intercept_ht = NULL;
static void (*original_zend_execute_ex)(zend_execute_data *execute_data);

#ifdef ZTS
static THREAD_LOCAL zend_bool is_intercepting = 0;
#else
static zend_bool is_intercepting = 0;
#endif

#ifdef RAYAOP_DEBUG
#define RAYAOP_DEBUG_PRINT(fmt, ...) php_printf("RAYAOP DEBUG: " fmt "\n", ##__VA_ARGS__)
#else
#define RAYAOP_DEBUG_PRINT(fmt, ...)
#endif

#include <stdio.h>

static void dump_memory(const void *ptr, size_t size)
{
    const unsigned char *byte = ptr;
    for (size_t i = 0; i < size; i++) {
        printf("%02x ", byte[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    printf("\n");
}

static void dump_zend_string(zend_string *str)
{
    if (str) {
        printf("zend_string dump:\n");
        printf("  val: %s\n", ZSTR_VAL(str));
        printf("  len: %zu\n", ZSTR_LEN(str));
        printf("  h: %llu\n", (unsigned long long)str->h);
        printf("  refcounted: %d\n", GC_FLAGS(str) & IS_STR_PERSISTENT ? 0 : 1);
        dump_memory(str, sizeof(*str));
    }
}

static void dump_zval(zval *val)
{
    if (val) {
        printf("zval dump:\n");
        printf("  type: %d\n", Z_TYPE_P(val));
        printf("  refcount: %d\n", Z_REFCOUNT_P(val));
        dump_memory(val, sizeof(*val));
    }
}

static void free_intercept_info(zval *zv)
{
    intercept_info *info = Z_PTR_P(zv);
    if (info) {
        RAYAOP_DEBUG_PRINT("Freeing intercept info for %s::%s", ZSTR_VAL(info->class_name), ZSTR_VAL(info->method_name));

        if (info->class_name) {
            RAYAOP_DEBUG_PRINT("Before releasing class_name: %s", ZSTR_VAL(info->class_name));
            dump_zend_string(info->class_name);
            if (!ZSTR_IS_INTERNED(info->class_name)) {
                zend_string_release(info->class_name);
                RAYAOP_DEBUG_PRINT("After releasing class_name");
            } else {
                RAYAOP_DEBUG_PRINT("class_name is interned: %s", ZSTR_VAL(info->class_name));
            }
            info->class_name = NULL;
        }

        if (info->method_name) {
            RAYAOP_DEBUG_PRINT("Before releasing method_name: %s", ZSTR_VAL(info->method_name));
            dump_zend_string(info->method_name);
            if (!ZSTR_IS_INTERNED(info->method_name)) {
                zend_string_release(info->method_name);
                RAYAOP_DEBUG_PRINT("After releasing method_name");
            } else {
                RAYAOP_DEBUG_PRINT("method_name is interned: %s", ZSTR_VAL(info->method_name));
            }
            info->method_name = NULL;
        }

        RAYAOP_DEBUG_PRINT("Releasing handler");
        RAYAOP_DEBUG_PRINT("Before zval_ptr_dtor handler");
        dump_zval(&info->handler);
        zval_ptr_dtor(&info->handler);
        RAYAOP_DEBUG_PRINT("After zval_ptr_dtor handler");

        RAYAOP_DEBUG_PRINT("Before freeing info struct");
        dump_memory(info, sizeof(*info));
        efree(info);
        RAYAOP_DEBUG_PRINT("After freeing info struct");
        RAYAOP_DEBUG_PRINT("Memory freed for intercept info");
    }
}

static void rayaop_zend_execute_ex(zend_execute_data *execute_data)
{
    RAYAOP_DEBUG_PRINT("rayaop_zend_execute_ex called");

    if (is_intercepting || !intercept_ht) {
        RAYAOP_DEBUG_PRINT("Already intercepting or intercept_ht is NULL, calling original zend_execute_ex");
        original_zend_execute_ex(execute_data);
        return;
    }

    zend_function *current_function = execute_data->func;

    if (current_function->common.scope && current_function->common.function_name) {
        zend_string *key = zend_string_alloc(ZSTR_LEN(current_function->common.scope->name) + ZSTR_LEN(current_function->common.function_name) + 2, 0);
        sprintf(ZSTR_VAL(key), "%s::%s", ZSTR_VAL(current_function->common.scope->name), ZSTR_VAL(current_function->common.function_name));
        ZSTR_LEN(key) = strlen(ZSTR_VAL(key));
        RAYAOP_DEBUG_PRINT("Generated key: %s", ZSTR_VAL(key));

        intercept_info *info = zend_hash_find_ptr(intercept_ht, key);

        if (info) {
            RAYAOP_DEBUG_PRINT("Found intercept info for key: %s", ZSTR_VAL(key));

            if (Z_TYPE(info->handler) == IS_OBJECT) {
                zval retval, params[3];
                ZVAL_UNDEF(&retval);

                if (!Z_OBJ(execute_data->This)) {
                    RAYAOP_DEBUG_PRINT("execute_data->This is not an object, calling original zend_execute_ex");
                    zend_string_release(key);
                    original_zend_execute_ex(execute_data);
                    return;
                }

                ZVAL_OBJ(&params[0], Z_OBJ(execute_data->This));
                ZVAL_STR_COPY(&params[1], current_function->common.function_name);

                array_init(&params[2]);
                uint32_t arg_count = ZEND_CALL_NUM_ARGS(execute_data);
                zval *args = ZEND_CALL_ARG(execute_data, 1);
                for (uint32_t i = 0; i < arg_count; i++) {
                    zval *arg = &args[i];
                    Z_TRY_ADDREF_P(arg);
                    add_next_index_zval(&params[2], arg);
                }

                is_intercepting = 1;
                zval func_name;
                ZVAL_STRING(&func_name, "intercept");

                RAYAOP_DEBUG_PRINT("Calling interceptor for %s::%s", ZSTR_VAL(current_function->common.scope->name), ZSTR_VAL(current_function->common.function_name));

                if (call_user_function(NULL, &info->handler, &func_name, &retval, 3, params) == SUCCESS) {
                    if (!Z_ISUNDEF(retval)) {
                        ZVAL_COPY(execute_data->return_value, &retval);
                        zval_ptr_dtor(&retval);
                    }
                } else {
                    php_error_docref(NULL, E_WARNING, "Interception failed for %s::%s", ZSTR_VAL(current_function->common.scope->name), ZSTR_VAL(current_function->common.function_name));
                }

                zval_ptr_dtor(&func_name);
                zval_ptr_dtor(&params[1]);
                zval_ptr_dtor(&params[2]);

                is_intercepting = 0;
                zend_string_release(key);
                return;
            }
        } else {
            RAYAOP_DEBUG_PRINT("No intercept info found for key: %s", ZSTR_VAL(key));
        }

        zend_string_release(key);
    }

    original_zend_execute_ex(execute_data);
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_method_intercept, 0, 0, 3)
    ZEND_ARG_TYPE_INFO(0, class_name, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, method_name, IS_STRING, 0)
    ZEND_ARG_OBJ_INFO(0, interceptor, Ray\\Aop\\MethodInterceptorInterface, 0)
ZEND_END_ARG_INFO()

PHP_FUNCTION(method_intercept)
{
    RAYAOP_DEBUG_PRINT("method_intercept called");

    char *class_name, *method_name;
    size_t class_name_len, method_name_len;
    zval *intercepted;

    ZEND_PARSE_PARAMETERS_START(3, 3)
        Z_PARAM_STRING(class_name, class_name_len)
        Z_PARAM_STRING(method_name, method_name_len)
        Z_PARAM_OBJECT(intercepted)
    ZEND_PARSE_PARAMETERS_END();

    intercept_info *new_info = ecalloc(1, sizeof(intercept_info));
    if (!new_info) {
        php_error_docref(NULL, E_ERROR, "Memory allocation failed");
        RETURN_FALSE;
    }
    RAYAOP_DEBUG_PRINT("Allocated memory for intercept_info");

    new_info->class_name = zend_string_init(class_name, class_name_len, 0);
    new_info->method_name = zend_string_init(method_name, method_name_len, 0);
    ZVAL_COPY(&new_info->handler, intercepted);
    RAYAOP_DEBUG_PRINT("Initialized intercept_info for %s::%s", class_name, method_name);

    zend_string *key = zend_string_alloc(class_name_len + method_name_len + 2, 0);
    sprintf(ZSTR_VAL(key), "%s::%s", class_name, method_name);
    ZSTR_LEN(key) = strlen(ZSTR_VAL(key));
    RAYAOP_DEBUG_PRINT("Registered intercept info for key: %s", ZSTR_VAL(key));

    zend_hash_update_ptr(intercept_ht, key, new_info);
    // zend_string_release(key);

    RETURN_TRUE;
}

PHP_MINIT_FUNCTION(rayaop)
{
    RAYAOP_DEBUG_PRINT("PHP_MINIT_FUNCTION called");

    original_zend_execute_ex = zend_execute_ex;
    zend_execute_ex = rayaop_zend_execute_ex;

    intercept_ht = pemalloc(sizeof(HashTable), 1);
    if (intercept_ht) {
        zend_hash_init(intercept_ht, 8, NULL, free_intercept_info, 1);
        RAYAOP_DEBUG_PRINT("RayAOP extension initialized");
        return SUCCESS;
    }

    RAYAOP_DEBUG_PRINT("Failed to allocate memory for intercept_ht");
    return FAILURE;
}

PHP_MSHUTDOWN_FUNCTION(rayaop)
{
    RAYAOP_DEBUG_PRINT("PHP_MSHUTDOWN_FUNCTION called");

    zend_execute_ex = original_zend_execute_ex;

    if (intercept_ht) {
        RAYAOP_DEBUG_PRINT("Destroying intercept_ht");
        zend_hash_destroy(intercept_ht);
        RAYAOP_DEBUG_PRINT("Destroyed intercept_ht");
        pefree(intercept_ht, 1);
        intercept_ht = NULL;
        RAYAOP_DEBUG_PRINT("Intercept hash table destroyed and freed");
    }

    RAYAOP_DEBUG_PRINT("RayAOP extension shut down");
    return SUCCESS;
}

PHP_MINFO_FUNCTION(rayaop)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "rayaop support", "enabled");
    php_info_print_table_row(2, "Version", PHP_RAYAOP_VERSION);
    php_info_print_table_end();
}

static const zend_function_entry rayaop_functions[] = {
    PHP_FE(method_intercept, arginfo_method_intercept)
    PHP_FE_END
};

zend_module_entry rayaop_module_entry = {
    STANDARD_MODULE_HEADER,
    "rayaop",
    rayaop_functions,
    PHP_MINIT(rayaop),
    PHP_MSHUTDOWN(rayaop),
    NULL,
    NULL,
    PHP_MINFO(rayaop),
    PHP_RAYAOP_VERSION,
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_RAYAOP
ZEND_GET_MODULE(rayaop)
#endif
