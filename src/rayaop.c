#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "zend_exceptions.h"
#include "zend_interfaces.h"
#include "php_rayaop.h"

// インターセプト情報を保持する構造体
typedef struct _intercept_info {
    char *class_name;
    char *method_name;
    zval *handler;
    struct _intercept_info *next;
} intercept_info;

// インターセプト情報のリスト
static intercept_info *intercept_list = NULL;

static void (*original_zend_execute_ex)(zend_execute_data *execute_data);

// 新しいzend_execute_ex関数の実装
static void rayaop_zend_execute_ex(zend_execute_data *execute_data)
{
    zend_function *current_function = execute_data->func;

    if (current_function->common.scope && current_function->common.function_name) {
        const char *class_name = ZSTR_VAL(current_function->common.scope->name);
        const char *method_name = ZSTR_VAL(current_function->common.function_name);

        // インターセプト情報を検索
        intercept_info *info = intercept_list;
        while (info) {
            if (strcmp(info->class_name, class_name) == 0 && strcmp(info->method_name, method_name) == 0) {
                zval retval, params[3];

                ZVAL_OBJ(&params[0], execute_data->This.value.obj);
                ZVAL_STRING(&params[1], method_name);

                array_init(&params[2]);
                uint32_t arg_count = ZEND_CALL_NUM_ARGS(execute_data);
                zval *args = ZEND_CALL_ARG(execute_data, 1);
                for (uint32_t i = 0; i < arg_count; i++) {
                    zval *arg = &args[i];
                    Z_TRY_ADDREF_P(arg);
                    add_next_index_zval(&params[2], arg);
                }

                zval func_name;
                ZVAL_STRING(&func_name, "intercept");
                call_user_function(NULL, info->handler, &func_name, &retval, 3, params);
                zval_ptr_dtor(&func_name);

                zval_ptr_dtor(&params[1]);
                zval_ptr_dtor(&params[2]);
                zval_ptr_dtor(&retval);
                break;
            }
            info = info->next;
        }
    }

    original_zend_execute_ex(execute_data);
}

PHP_FUNCTION(method_intercept)
{
    char *class_name, *method_name;
    size_t class_name_len, method_name_len;
    zval *intercepted;

    ZEND_PARSE_PARAMETERS_START(3, 3)
        Z_PARAM_STRING(class_name, class_name_len)
        Z_PARAM_STRING(method_name, method_name_len)
        Z_PARAM_OBJECT(intercepted)
    ZEND_PARSE_PARAMETERS_END();

    intercept_info *new_info = emalloc(sizeof(intercept_info));
    new_info->class_name = estrndup(class_name, class_name_len);
    new_info->method_name = estrndup(method_name, method_name_len);
    new_info->handler = emalloc(sizeof(zval));
    ZVAL_COPY(new_info->handler, intercepted);
    new_info->next = intercept_list;
    intercept_list = new_info;

    RETURN_NULL();
}

PHP_MINIT_FUNCTION(rayaop)
{
    original_zend_execute_ex = zend_execute_ex;
    zend_execute_ex = rayaop_zend_execute_ex;
    return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(rayaop)
{
    zend_execute_ex = original_zend_execute_ex;
    // インターセプト情報のクリーンアップ
    while (intercept_list) {
        intercept_info *temp = intercept_list;
        intercept_list = intercept_list->next;
        efree(temp->class_name);
        efree(temp->method_name);
        zval_ptr_dtor(temp->handler);
        efree(temp->handler);
        efree(temp);
    }
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
    PHP_FE(method_intercept, NULL)
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
