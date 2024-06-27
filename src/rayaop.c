#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "zend_exceptions.h"
#include "zend_interfaces.h"
#include "php_rayaop.h"

#ifdef ZTS
#include "TSRM.h"
#endif

#define RAYAOP_DEBUG

typedef struct _intercept_info {
    zend_string *class_name;
    zend_string *method_name;
    zval handler;
    struct _intercept_info *next;
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

static zend_class_entry *zend_ce_ray_aop_interceptedinterface;

static void rayaop_zend_execute_ex(zend_execute_data *execute_data)
{
    RAYAOP_DEBUG_PRINT("rayaop_zend_execute_ex called");

    if (is_intercepting) {
        RAYAOP_DEBUG_PRINT("Already intercepting, calling original zend_execute_ex");
        original_zend_execute_ex(execute_data);
        return;
    }

    zend_function *current_function = execute_data->func;

    if (current_function->common.scope && current_function->common.function_name) {
        zend_string *class_name = current_function->common.scope->name;
        zend_string *method_name = current_function->common.function_name;

        char *key = NULL;
        size_t key_len = 0;
        key_len = spprintf(&key, 0, "%s::%s", ZSTR_VAL(class_name), ZSTR_VAL(method_name));
        RAYAOP_DEBUG_PRINT("Generated key: %s", key);

        intercept_info *info = zend_hash_str_find_ptr(intercept_ht, key, key_len);

        if (info) {
            RAYAOP_DEBUG_PRINT("Found intercept info for key: %s", key);

            // インターセプト処理のコード
            if (Z_TYPE(info->handler) == IS_OBJECT) {
                // ... (インターセプト処理のコード)
            }
        } else {
            RAYAOP_DEBUG_PRINT("No intercept info found for key: %s", key);
        }

        efree(key);  // keyの解放はすべてのデバッグ出力の後
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

    char *key = NULL;
    size_t key_len = 0;
    key_len = spprintf(&key, 0, "%s::%s", class_name, method_name);
    RAYAOP_DEBUG_PRINT("Registered intercept info for key: %s", key);

    zend_hash_str_update_ptr(intercept_ht, key, key_len, new_info);
    efree(key);

    RETURN_TRUE;
}

static int efree_intercept_info(zval *zv)
{
    intercept_info *info = Z_PTR_P(zv);
    if (info) {
        RAYAOP_DEBUG_PRINT("Freeing intercept info for %s::%s", ZSTR_VAL(info->class_name), ZSTR_VAL(info->method_name));

        zend_string_release(info->class_name);
        zend_string_release(info->method_name);

        RAYAOP_DEBUG_PRINT("class_name and method_name released");

        zval_ptr_dtor(&info->handler);
        RAYAOP_DEBUG_PRINT("handler released");

        efree(info);
        RAYAOP_DEBUG_PRINT("Memory freed for intercept info");
    }
    return ZEND_HASH_APPLY_REMOVE;
}

PHP_MINIT_FUNCTION(rayaop)
{
    RAYAOP_DEBUG_PRINT("PHP_MINIT_FUNCTION called");

    original_zend_execute_ex = zend_execute_ex;
    zend_execute_ex = rayaop_zend_execute_ex;

    intercept_ht = pemalloc(sizeof(HashTable), 1);
    zend_hash_init(intercept_ht, 8, NULL, NULL, 1);

    RAYAOP_DEBUG_PRINT("RayAOP extension initialized");
    return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(rayaop)
{
    RAYAOP_DEBUG_PRINT("PHP_MSHUTDOWN_FUNCTION called");

    zend_execute_ex = original_zend_execute_ex;

    if (intercept_ht) {
        zend_hash_destroy(intercept_ht);
        pefree(intercept_ht, 1);
        intercept_ht = NULL;
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
