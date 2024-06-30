#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"  // PHPのメインヘッダー
#include "php_ini.h"  // PHP設定関連のヘッダー
#include "ext/standard/info.h"  // 標準の情報関数用ヘッダー
#include "zend_exceptions.h"  // Zend例外用ヘッダー
#include "zend_interfaces.h"  // Zendインターフェース用ヘッダー
#include "php_rayaop.h"  // 拡張機能のヘッダー

#ifdef ZTS
#include "TSRM.h"  // スレッドセーフリソース管理ヘッダー
#endif

// デバッグ出力を有効にする場合はこのマクロをアンコメントしてください
// #define RAYAOP_DEBUG

/**
 * インターセプト情報を保持する構造体
 * link: http//://www.phpinternalsbook.com/php5/classes_objects/internal_structures_and_implementation.html
 * link: http://php.adamharvey.name/manual/ja/internals2.variables.tables.php
 */
typedef struct _intercept_info {
    zend_string *class_name;     // インターセプト対象のクラス名
    zend_string *method_name;    // インターセプト対象のメソッド名
    zval handler;                // インターセプトハンドラー
} intercept_info;

// グローバル変数の宣言
static HashTable *intercept_ht = NULL;  // インターセプト情報を格納するハッシュテーブル
static void (*original_zend_execute_ex)(zend_execute_data *execute_data);  // 元のzend_execute_ex関数へのポインタ

#ifdef ZTS
static THREAD_LOCAL zend_bool is_intercepting = 0;  // スレッドローカル変数（スレッドセーフビルド時用）
#else
static zend_bool is_intercepting = 0;  // インターセプト中かどうかのフラグ
#endif

// デバッグ出力用マクロ
#ifdef RAYAOP_DEBUG
#define RAYAOP_DEBUG_PRINT(fmt, ...) php_printf("RAYAOP DEBUG: " fmt "\n", ##__VA_ARGS__)
#else
#define RAYAOP_DEBUG_PRINT(fmt, ...)
#endif

// インターセプトされたインターフェースのクラスエントリ
static zend_class_entry *zend_ce_ray_aop_interceptedinterface;

// カスタム zend_execute_ex 関数
static void rayaop_zend_execute_ex(zend_execute_data *execute_data)
{
    RAYAOP_DEBUG_PRINT("rayaop_zend_execute_ex called");

    // 既にインターセプト中の場合は元の関数を呼び出す
    if (is_intercepting) {
        RAYAOP_DEBUG_PRINT("Already intercepting, calling original zend_execute_ex");
        original_zend_execute_ex(execute_data);  // 元のzend_execute_ex関数を呼び出す
        return;
    }

    zend_function *current_function = execute_data->func;  // 現在の関数情報を取得

    // クラスメソッドの場合のみ処理を行う
    if (current_function->common.scope && current_function->common.function_name) {
        zend_string *class_name = current_function->common.scope->name;  // クラス名を取得
        zend_string *method_name = current_function->common.function_name;  // メソッド名を取得

        char *key = NULL;  // ハッシュキー用の文字列ポインタ
        size_t key_len;  // ハッシュキーの長さ
        key_len = spprintf(&key, 0, "%s::%s", ZSTR_VAL(class_name), ZSTR_VAL(method_name));  // ハッシュキーを生成
        RAYAOP_DEBUG_PRINT("Generated key: %s", key);

        intercept_info *info = zend_hash_str_find_ptr(intercept_ht, key, key_len);  // ハッシュテーブルからインターセプト情報を取得

        if (info) {
            RAYAOP_DEBUG_PRINT("Found intercept info for key: %s", key);

            if (Z_TYPE(info->handler) == IS_OBJECT) {
                zval retval, params[3];  // 戻り値とパラメータ用のzvalを宣言

                // オブジェクトがNULLの場合は元の関数を実行
                if (execute_data->This.value.obj == NULL) {
                    RAYAOP_DEBUG_PRINT("Object is NULL, calling original function");
                    original_zend_execute_ex(execute_data);
                    efree(key);  // キーを解放
                    return;
                }

                ZVAL_OBJ(&params[0], execute_data->This.value.obj);  // オブジェクトを設定
                ZVAL_STR(&params[1], method_name);  // メソッド名を設定

                array_init(&params[2]);  // 引数の配列を初期化
                uint32_t arg_count = ZEND_CALL_NUM_ARGS(execute_data);  // 引数の数を取得
                zval *args = ZEND_CALL_ARG(execute_data, 1);  // 引数を取得
                for (uint32_t i = 0; i < arg_count; i++) {
                    zval *arg = &args[i];  // 各引数を取得
                    Z_TRY_ADDREF_P(arg);  // 引数の参照カウントを増やす
                    add_next_index_zval(&params[2], arg);  // 配列に引数を追加
                }

                is_intercepting = 1;  // インターセプト中フラグを設定
                zval func_name;
                ZVAL_STRING(&func_name, "intercept");  // インターセプトハンドラーのメソッド名を設定

                ZVAL_UNDEF(&retval);
                if (call_user_function(NULL, &info->handler, &func_name, &retval, 3, params) == SUCCESS) {
                    if (!Z_ISUNDEF(retval)) {
                        ZVAL_COPY(execute_data->return_value, &retval);  // 戻り値を設定
                    }
                } else {
                    php_error_docref(NULL, E_WARNING, "Interception failed for %s::%s", ZSTR_VAL(class_name), ZSTR_VAL(method_name));
                }
                zval_ptr_dtor(&retval);  // 戻り値を解放

                zval_ptr_dtor(&func_name);  // メソッド名のデストラクタを呼ぶ
                zval_ptr_dtor(&params[1]);  // メソッド名のデストラクタを呼ぶ
                zval_ptr_dtor(&params[2]);  // 引数配列のデストラクタを呼ぶ

                is_intercepting = 0;  // インターセプト中フラグを解除
                efree(key);  // キーを解放
                return;
            }
        } else {
            RAYAOP_DEBUG_PRINT("No intercept info found for key: %s", key);
        }

        efree(key);  // キーを解放
    }

    original_zend_execute_ex(execute_data);  // 元のzend_execute_ex関数を呼び出す
}

// method_intercept 関数の引数情報
ZEND_BEGIN_ARG_INFO_EX(arginfo_method_intercept, 0, 0, 3)
    ZEND_ARG_TYPE_INFO(0, class_name, IS_STRING, 0)  // クラス名の引数情報
    ZEND_ARG_TYPE_INFO(0, method_name, IS_STRING, 0)  // メソッド名の引数情報
    ZEND_ARG_OBJ_INFO(0, interceptor, Ray\\Aop\\MethodInterceptorInterface, 0)  // インターセプトハンドラーの引数情報
ZEND_END_ARG_INFO()

/**
 * インターセプトメソッドを登録する関数
 * link: https://www.phpinternalsbook.com/php7/extensions_design/php_functions.html
 */
PHP_FUNCTION(method_intercept)
{
    RAYAOP_DEBUG_PRINT("method_intercept called");

    char *class_name, *method_name;  // クラス名とメソッド名のポインタ
    size_t class_name_len, method_name_len;  // クラス名とメソッド名の長さ
    zval *intercepted;  // インターセプトハンドラー

    ZEND_PARSE_PARAMETERS_START(3, 3)
        Z_PARAM_STRING(class_name, class_name_len)  // クラス名のパラメータを解析
        Z_PARAM_STRING(method_name, method_name_len)  // メソッド名のパラメータを解析
        Z_PARAM_OBJECT(intercepted)  // インターセプトハンドラーのパラメータを解析
    ZEND_PARSE_PARAMETERS_END();

    intercept_info *new_info = emalloc(sizeof(intercept_info));
    if (!new_info) {
        php_error_docref(NULL, E_ERROR, "Memory allocation failed");  // メモリ確保に失敗した場合のエラー
        RETURN_FALSE;
    }
    RAYAOP_DEBUG_PRINT("Allocated memory for intercept_info");

    new_info->class_name = zend_string_init(class_name, class_name_len, 0);  // クラス名を初期化
    new_info->method_name = zend_string_init(method_name, method_name_len, 0);  // メソッド名を初期化
    ZVAL_COPY(&new_info->handler, intercepted);  // インターセプトハンドラーをコピー
    RAYAOP_DEBUG_PRINT("Initialized intercept_info for %s::%s", class_name, method_name);

    char *key = NULL;  // ハッシュキー用の文字列ポインタ
    size_t key_len = spprintf(&key, 0, "%s::%s", class_name, method_name);  // ハッシュキーを生成
    RAYAOP_DEBUG_PRINT("Generated key: %s", key);

    if (zend_hash_str_update_ptr(intercept_ht, key, key_len, new_info) == NULL) {
        php_error_docref(NULL, E_ERROR, "Failed to update hash table");  // ハッシュテーブルの更新に失敗した場合のエラー
        zend_string_release(new_info->class_name);  // クラス名を解放
        zend_string_release(new_info->method_name);  // メソッド名を解放
        zval_ptr_dtor(&new_info->handler);  // ハンドラーを解放
        efree(new_info);  // インターセプト情報を解放
        efree(key);  // キーを解放
        RETURN_FALSE;
    }

    efree(key);  // キーを解放
    RAYAOP_DEBUG_PRINT("Successfully registered intercept info");
    RETURN_TRUE;
}

/**
 * インターセプト情報を解放する関数
 * link: https://www.phpinternalsbook.com/php7/internal_types/strings/zend_strings.html
 */
static void efree_intercept_info(zval *zv)
{
    intercept_info *info = Z_PTR_P(zv);  // インターセプト情報を取得
    if (info) {
        RAYAOP_DEBUG_PRINT("Freeing intercept info for %s::%s", ZSTR_VAL(info->class_name), ZSTR_VAL(info->method_name));

        zend_string_release(info->class_name);  // クラス名を解放
        zend_string_release(info->method_name);  // メソッド名を解放
        RAYAOP_DEBUG_PRINT("class_name and method_name released");

        zval_ptr_dtor(&info->handler);  // ハンドラーを解放
        RAYAOP_DEBUG_PRINT("handler released");

        efree(info);  // インターセプト情報構造体を解放
        RAYAOP_DEBUG_PRINT("Memory freed for intercept info");
    }
}

/**
 * 拡張機能の初期化関数
 * link: https://www.phpinternalsbook.com/php7/extensions_design/hooks.html
 */
PHP_MINIT_FUNCTION(rayaop)
{
    RAYAOP_DEBUG_PRINT("PHP_MINIT_FUNCTION called");

    original_zend_execute_ex = zend_execute_ex;  // 元のzend_execute_ex関数を保存
    zend_execute_ex = rayaop_zend_execute_ex;  // カスタムzend_execute_ex関数を設定

    intercept_ht = pemalloc(sizeof(HashTable), 1);  // ハッシュテーブルを確保
    if (!intercept_ht) {
        php_error_docref(NULL, E_ERROR, "Failed to allocate memory for intercept hash table");
        return FAILURE;
    }
    zend_hash_init(intercept_ht, 8, NULL, (dtor_func_t)efree_intercept_info, 1);  // ハッシュテーブルを初期化

    RAYAOP_DEBUG_PRINT("RayAOP extension initialized");
    return SUCCESS;  // 初期化成功
}

/**
 * 拡張機能のシャットダウン関数
 * link: https://www.phpinternalsbook.com/php7/extensions_design/hooks.html
 */
PHP_MSHUTDOWN_FUNCTION(rayaop)
{
    RAYAOP_DEBUG_PRINT("PHP_MSHUTDOWN_FUNCTION called");

    zend_execute_ex = original_zend_execute_ex;  // 元のzend_execute_ex関数を復元

    if (intercept_ht) {
        zend_hash_destroy(intercept_ht);  // ハッシュテーブルを破棄
        pefree(intercept_ht, 1);  // ハッシュテーブルのメモリを解放
        intercept_ht = NULL;  // ハッシュテーブルポインタをNULLに設定
    }

    RAYAOP_DEBUG_PRINT("RayAOP extension shut down");
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
    NULL,  // リクエスト開始時の関数（未使用）
    NULL,  // リクエスト終了時の関数（未使用）
    PHP_MINFO(rayaop),  // 拡張機能の情報表示関数
    PHP_RAYAOP_VERSION,  // 拡張機能のバージョン
    STANDARD_MODULE_PROPERTIES
};

// 動的にロード可能なモジュールとしてコンパイルする場合の処理
#ifdef COMPILE_DL_RAYAOP
ZEND_GET_MODULE(rayaop)
#endif

// 以下は、必要に応じて追加の関数や定義を記述できます

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
    if (intercept_ht) {
        zend_string *key;
        intercept_info *info;
        ZEND_HASH_FOREACH_STR_KEY_PTR(intercept_ht, key, info) {
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
