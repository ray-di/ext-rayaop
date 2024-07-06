#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php_rayaop.h"

// モジュールグローバル変数の宣言
ZEND_DECLARE_MODULE_GLOBALS(rayaop)

// 静的変数の宣言
static void (*original_zend_execute_ex)(zend_execute_data *execute_data);

// グローバル初期化関数
static void php_rayaop_init_globals(zend_rayaop_globals *rayaop_globals)
{
    rayaop_globals->intercept_ht = NULL;
    rayaop_globals->is_intercepting = 0;
}

static void (*original_zend_execute_ex)(zend_execute_data *execute_data);

// デバッグ出力用マクロ
#ifdef RAYAOP_DEBUG
#define RAYAOP_DEBUG_PRINT(fmt, ...) php_printf("RAYAOP DEBUG: " fmt "\n", ##__VA_ARGS__)
#else
#define RAYAOP_DEBUG_PRINT(fmt, ...)
#endif

// method_intercept 関数の引数情報
ZEND_BEGIN_ARG_INFO_EX(arginfo_method_intercept, 0, 0, 3)
    ZEND_ARG_TYPE_INFO(0, class_name, IS_STRING, 0)  // クラス名の引数情報
    ZEND_ARG_TYPE_INFO(0, method_name, IS_STRING, 0)  // メソッド名の引数情報
    ZEND_ARG_OBJ_INFO(0, interceptor, Ray\\Aop\\MethodInterceptorInterface, 0)  // インターセプトハンドラーの引数情報
ZEND_END_ARG_INFO()

// 拡張機能が提供する関数の定義
// https://www.phpinternalsbook.com/php7/extensions_design/php_functions.html
static const zend_function_entry rayaop_functions[] = {
    PHP_FE(method_intercept, arginfo_method_intercept)  // method_intercept関数の登録
    PHP_FE_END  // 関数エントリの終了
};

// 拡張機能のモジュールエントリ
// link https://www.phpinternalsbook.com/php7/extensions_design/extension_infos.html
zend_module_entry rayaop_module_entry = {
    STANDARD_MODULE_HEADER,
    "rayaop",  // 拡張機能の名前
    rayaop_functions,  // 拡張機能が提供する関数
    PHP_MINIT(rayaop),  // 拡張機能の初期化関数
    PHP_MSHUTDOWN(rayaop),  // 拡張機能のシャットダウン関数
    PHP_RINIT(rayaop),  // リクエスト開始時の関数
    PHP_RSHUTDOWN(rayaop),  // リクエスト終了時の関数
    PHP_MINFO(rayaop),  // 拡張機能の情報表示関数
    PHP_RAYAOP_VERSION,  // 拡張機能のバージョン
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_RAYAOP
ZEND_GET_MODULE(rayaop)
#endif

// ユーティリティ関数の実装

void rayaop_handle_error(int error_code, const char *message) {
    switch (error_code) {
        case RAYAOP_ERROR_MEMORY_ALLOCATION:
            php_error_docref(NULL, E_ERROR, "Memory allocation failed: %s", message);
            break;
        case RAYAOP_ERROR_HASH_UPDATE:
            php_error_docref(NULL, E_ERROR, "Failed to update hash table: %s", message);
            break;
        default:
            php_error_docref(NULL, E_ERROR, "Unknown error: %s", message);
    }
}

bool rayaop_should_intercept(zend_execute_data *execute_data) {
    return execute_data->func->common.scope &&
           execute_data->func->common.function_name &&
           !RAYAOP_G(is_intercepting);
}

char* rayaop_generate_intercept_key(zend_string *class_name, zend_string *method_name, size_t *key_len) {
    char *key = NULL;
    *key_len = spprintf(&key, 0, "%s::%s", ZSTR_VAL(class_name), ZSTR_VAL(method_name));
    RAYAOP_DEBUG_PRINT("Generated key: %s", key);
    return key;
}

intercept_info* rayaop_find_intercept_info(const char *key, size_t key_len) {
    return zend_hash_str_find_ptr(RAYAOP_G(intercept_ht), key, key_len);
}

void rayaop_execute_intercept(zend_execute_data *execute_data, intercept_info *info) {
    if (Z_TYPE(info->handler) != IS_OBJECT) {
        return;
    }

    if (execute_data->This.value.obj == NULL) {
        RAYAOP_DEBUG_PRINT("Object is NULL, calling original function");
        original_zend_execute_ex(execute_data);
        return;
    }

    zval retval, params[3];
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

    RAYAOP_G(is_intercepting) = 1;
    zval func_name;
    ZVAL_STRING(&func_name, "intercept");

    ZVAL_UNDEF(&retval);
    if (call_user_function(NULL, &info->handler, &func_name, &retval, 3, params) == SUCCESS) {
        if (!Z_ISUNDEF(retval)) {
            ZVAL_COPY(execute_data->return_value, &retval);
        }
    } else {
        php_error_docref(NULL, E_WARNING, "Interception failed for %s::%s", ZSTR_VAL(info->class_name), ZSTR_VAL(info->method_name));
    }

    zval_ptr_dtor(&retval);
    zval_ptr_dtor(&func_name);
    zval_ptr_dtor(&params[1]);
    zval_ptr_dtor(&params[2]);

    RAYAOP_G(is_intercepting) = 0;
}

/**
 * インターセプト情報を解放する関数
 * link: https://www.phpinternalsbook.com/php7/internal_types/strings/zend_strings.html
 */
void rayaop_free_intercept_info(zval *zv) {
    intercept_info *info = Z_PTR_P(zv);
    if (info) {
        RAYAOP_DEBUG_PRINT("Freeing intercept info for %s::%s", ZSTR_VAL(info->class_name), ZSTR_VAL(info->method_name));
        zend_string_release(info->class_name);
        zend_string_release(info->method_name);
        zval_ptr_dtor(&info->handler);
        efree(info);
    }
}

// カスタム zend_execute_ex 関数
static void rayaop_zend_execute_ex(zend_execute_data *execute_data) {
    RAYAOP_DEBUG_PRINT("rayaop_zend_execute_ex called");

    if (!rayaop_should_intercept(execute_data)) {
        original_zend_execute_ex(execute_data);
        return;
    }

    zend_function *current_function = execute_data->func;
    zend_string *class_name = current_function->common.scope->name;
    zend_string *method_name = current_function->common.function_name;

    size_t key_len;
    char *key = rayaop_generate_intercept_key(class_name, method_name, &key_len);

    intercept_info *info = rayaop_find_intercept_info(key, key_len);

    if (info) {
        RAYAOP_DEBUG_PRINT("Found intercept info for key: %s", key);
        rayaop_execute_intercept(execute_data, info);
    } else {
        RAYAOP_DEBUG_PRINT("No intercept info found for key: %s", key);
        original_zend_execute_ex(execute_data);
    }

    efree(key);
}

/**
 * インターセプトメソッドを登録する関数
 * link: https://www.phpinternalsbook.com/php7/extensions_design/php_functions.html
 */
PHP_FUNCTION(method_intercept) {
    RAYAOP_DEBUG_PRINT("method_intercept called");

    char *class_name, *method_name;  // クラス名とメソッド名のポインタ
    size_t class_name_len, method_name_len;  // クラス名とメソッド名の長さ
    zval *intercepted;  // インターセプトハンドラー

    ZEND_PARSE_PARAMETERS_START(3, 3)
        Z_PARAM_STRING(class_name, class_name_len)  // クラス名のパラメータを解析
        Z_PARAM_STRING(method_name, method_name_len)  // メソッド名のパラメータを解析
        Z_PARAM_OBJECT(intercepted)  // インターセプトハンドラーのパラメータを解析
    ZEND_PARSE_PARAMETERS_END();

    intercept_info *new_info = ecalloc(1, sizeof(intercept_info));
    if (!new_info) {
        rayaop_handle_error(RAYAOP_ERROR_MEMORY_ALLOCATION, "Failed to allocate memory for intercept_info");
        RETURN_FALSE;
    }

    new_info->class_name = zend_string_init(class_name, class_name_len, 0);
    new_info->method_name = zend_string_init(method_name, method_name_len, 0);
    ZVAL_COPY(&new_info->handler, intercepted);

    char *key = NULL;
    size_t key_len = spprintf(&key, 0, "%s::%s", class_name, method_name);

    if (zend_hash_str_update_ptr(RAYAOP_G(intercept_ht), key, key_len, new_info) == NULL) {
        rayaop_handle_error(RAYAOP_ERROR_HASH_UPDATE, "Failed to update intercept hash table");
        zend_string_release(new_info->class_name);
        zend_string_release(new_info->method_name);
        zval_ptr_dtor(&new_info->handler);
        efree(new_info);
        efree(key);
        RETURN_FALSE;
    }

    efree(key);
    RAYAOP_DEBUG_PRINT("Successfully registered intercept info");
    RETURN_TRUE;
}

/**
 * 拡張機能の初期化関数
 * link: https://www.phpinternalsbook.com/php7/extensions_design/hooks.html
 */
PHP_MINIT_FUNCTION(rayaop)
{
    RAYAOP_DEBUG_PRINT("PHP_MINIT_FUNCTION called");

#ifdef ZTS
    ts_allocate_id(&rayaop_globals_id, sizeof(zend_rayaop_globals), (ts_allocate_ctor) php_rayaop_init_globals, NULL);
#else
    php_rayaop_init_globals(&rayaop_globals);
#endif

    original_zend_execute_ex = zend_execute_ex;
    zend_execute_ex = rayaop_zend_execute_ex;

    RAYAOP_DEBUG_PRINT("RayAOP extension initialized");
    return SUCCESS;
}

/**
 * 拡張機能のシャットダウン関数
 * link: https://www.phpinternalsbook.com/php7/extensions_design/hooks.html
 */
PHP_MSHUTDOWN_FUNCTION(rayaop)
{
    RAYAOP_DEBUG_PRINT("RayAOP PHP_MSHUTDOWN_FUNCTION called");

    zend_execute_ex = original_zend_execute_ex;  // 元のzend_execute_ex関数を復元
    original_zend_execute_ex = NULL;

    RAYAOP_DEBUG_PRINT("RayAOP PHP_MSHUTDOWN_FUNCTION shut down");
    return SUCCESS;  // シャットダウン成功
}

/**
 * リクエスト開始時の初期化関数
 */
PHP_RINIT_FUNCTION(rayaop)
{
    RAYAOP_DEBUG_PRINT("PHP_RINIT_FUNCTION called");

    if (RAYAOP_G(intercept_ht) == NULL) {
        ALLOC_HASHTABLE(RAYAOP_G(intercept_ht));
        zend_hash_init(RAYAOP_G(intercept_ht), 8, NULL, rayaop_free_intercept_info, 0);
    }
    RAYAOP_G(is_intercepting) = 0;

    return SUCCESS;
}

/**
 * 拡張機能のリクエストシャットダウン関数
 * link: https://www.phpinternalsbook.com/php7/extensions_design/hooks.html
 */
PHP_RSHUTDOWN_FUNCTION(rayaop)
{
    RAYAOP_DEBUG_PRINT("RayAOP PHP_RSHUTDOWN_FUNCTION called");
    if (RAYAOP_G(intercept_ht)) {
        zend_hash_destroy(RAYAOP_G(intercept_ht));  // ハッシュテーブルを破棄
        FREE_HASHTABLE(RAYAOP_G(intercept_ht));  // ハッシュテーブルのメモリを解放
        RAYAOP_G(intercept_ht) = NULL;  // ハッシュテーブルポインタをNULLに設定
    }

    RAYAOP_DEBUG_PRINT("RayAOP PHP_RSHUTDOWN_FUNCTION shut down");
    return SUCCESS;  // シャットダウン成功
}

/**
 * 拡張機能の情報表示関数
 * link: https://www.phpinternalsbook.com/php7/extensions_design/extension_infos.html
 */
PHP_MINFO_FUNCTION(rayaop)
{
php_info_print_table_start();  // 情報テーブルの開始
    php_info_print_table_header(2, "rayaop support", "enabled");  // テーブルヘッダーの表示
    php_info_print_table_row(2, "Version", PHP_RAYAOP_VERSION);  // バージョン情報の表示
    php_info_print_table_end();  // 情報テーブルの終了
}

// 例: デバッグ用のヘルパー関数
#ifdef RAYAOP_DEBUG
static void rayaop_debug_print_zval(zval *value)
{
    switch (Z_TYPE_P(value)) {
        case IS_NULL:
            RAYAOP_DEBUG_PRINT("(null)");
            break;
        case IS_TRUE:
            RAYAOP_DEBUG_PRINT("(bool) true");
            break;
        case IS_FALSE:
            RAYAOP_DEBUG_PRINT("(bool) false");
            break;
        case IS_LONG:
            RAYAOP_DEBUG_PRINT("(int) %ld", Z_LVAL_P(value));
            break;
        case IS_DOUBLE:
            RAYAOP_DEBUG_PRINT("(float) %f", Z_DVAL_P(value));
            break;
        case IS_STRING:
            RAYAOP_DEBUG_PRINT("(string) %s", Z_STRVAL_P(value));
            break;
        case IS_ARRAY:
            RAYAOP_DEBUG_PRINT("(array)");
            break;
        case IS_OBJECT:
            RAYAOP_DEBUG_PRINT("(object)");
            break;
        default:
            RAYAOP_DEBUG_PRINT("(unknown type)");
            break;
    }
}
#endif

// 例: インターセプト情報をダンプする関数
#ifdef RAYAOP_DEBUG
static void rayaop_dump_intercept_info(void)
{
    RAYAOP_DEBUG_PRINT("Dumping intercept information:");
    if (RAYAOP_G(intercept_ht)) {
        zend_string *key;
        intercept_info *info;
        ZEND_HASH_FOREACH_STR_KEY_PTR(RAYAOP_G(intercept_ht), key, info) {
            if (key && info) {
                RAYAOP_DEBUG_PRINT("Key: %s", ZSTR_VAL(key));
                RAYAOP_DEBUG_PRINT("  Class: %s", ZSTR_VAL(info->class_name));
                RAYAOP_DEBUG_PRINT("  Method: %s", ZSTR_VAL(info->method_name));
                RAYAOP_DEBUG_PRINT("  Handler type: %d", Z_TYPE(info->handler));
            }
        } ZEND_HASH_FOREACH_END();
    } else {
        RAYAOP_DEBUG_PRINT("Intercept hash table is not initialized");
    }
}
#endif