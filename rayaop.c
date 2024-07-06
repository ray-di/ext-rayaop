#ifdef HAVE_CONFIG_H
#include "config.h"  // 設定ファイルをインクルード
#endif

#include "php_rayaop.h"  // RayAOP拡張機能のヘッダーファイルをインクルード

// モジュールグローバル変数の宣言
ZEND_DECLARE_MODULE_GLOBALS(rayaop)

// 静的変数の宣言：元のzend_execute_ex関数へのポインタ
static void (*original_zend_execute_ex)(zend_execute_data *execute_data);

/**
 * グローバル初期化関数
 * @param zend_rayaop_globals *rayaop_globals グローバル変数へのポインタ
 */
static void php_rayaop_init_globals(zend_rayaop_globals *rayaop_globals)
{
    rayaop_globals->intercept_ht = NULL;  // インターセプトハッシュテーブルを初期化
    rayaop_globals->is_intercepting = 0;  // インターセプトフラグを初期化
}

// デバッグ出力用マクロ
#ifdef RAYAOP_DEBUG
#define RAYAOP_DEBUG_PRINT(fmt, ...) php_printf("RAYAOP DEBUG: " fmt "\n", ##__VA_ARGS__)  // デバッグ情報を出力
#else
#define RAYAOP_DEBUG_PRINT(fmt, ...)  // デバッグモードでない場合は何もしない
#endif

// method_intercept 関数の引数情報
ZEND_BEGIN_ARG_INFO_EX(arginfo_method_intercept, 0, 0, 3)
    ZEND_ARG_TYPE_INFO(0, class_name, IS_STRING, 0)  // クラス名の引数情報
    ZEND_ARG_TYPE_INFO(0, method_name, IS_STRING, 0)  // メソッド名の引数情報
    ZEND_ARG_OBJ_INFO(0, interceptor, Ray\\Aop\\MethodInterceptorInterface, 0)  // インターセプトハンドラーの引数情報
ZEND_END_ARG_INFO()

// ユーティリティ関数の実装

/**
 * エラーハンドリング関数
 * @param const char *message エラーメッセージ
 */
void rayaop_handle_error(const char *message) {
    php_error_docref(NULL, E_ERROR, "Memory error: %s", message);  // エラーメッセージを出力
}

/**
 * インターセプトが必要かどうかを判断する関数
 * @param zend_execute_data *execute_data 実行データ
 * @return bool インターセプトが必要な場合はtrue
 */
bool rayaop_should_intercept(zend_execute_data *execute_data) {
    return execute_data->func->common.scope &&  // スコープが存在し
           execute_data->func->common.function_name &&  // 関数名が存在し
           !RAYAOP_G(is_intercepting);  // 現在インターセプト中でない
}

/**
 * インターセプトキーを生成する関数
 * @param zend_string *class_name クラス名
 * @param zend_string *method_name メソッド名
 * @param size_t *key_len キーの長さを格納するポインタ
 * @return char* 生成されたキー
 */
char* rayaop_generate_intercept_key(zend_string *class_name, zend_string *method_name, size_t *key_len) {
    char *key = NULL;
    *key_len = spprintf(&key, 0, "%s::%s", ZSTR_VAL(class_name), ZSTR_VAL(method_name));  // クラス名::メソッド名 の形式でキーを生成
    RAYAOP_DEBUG_PRINT("Generated key: %s", key);  // 生成されたキーをデバッグ出力
    return key;
}

/**
 * インターセプト情報を検索する関数
 * @param const char *key 検索キー
 * @param size_t key_len キーの長さ
 * @return intercept_info* 見つかったインターセプト情報、見つからない場合はNULL
 */
intercept_info* rayaop_find_intercept_info(const char *key, size_t key_len) {
    return zend_hash_str_find_ptr(RAYAOP_G(intercept_ht), key, key_len);  // ハッシュテーブルからインターセプト情報を検索
}

/**
 * インターセプトを実行する関数
 * @param zend_execute_data *execute_data 実行データ
 * @param intercept_info *info インターセプト情報
 */
void rayaop_execute_intercept(zend_execute_data *execute_data, intercept_info *info) {
    if (Z_TYPE(info->handler) != IS_OBJECT) {  // ハンドラーがオブジェクトでない場合
        return;  // 処理を中断
    }

    if (execute_data->This.value.obj == NULL) {  // 対象オブジェクトがNULLの場合
        RAYAOP_DEBUG_PRINT("Object is NULL, calling original function");  // デバッグ情報を出力
        original_zend_execute_ex(execute_data);  // 元の関数を実行
        return;  // 処理を中断
    }

    zval retval, params[3];  // 戻り値と引数を格納する変数を準備
    ZVAL_OBJ(&params[0], execute_data->This.value.obj);  // 第1引数：対象オブジェクト
    ZVAL_STR(&params[1], info->method_name);  // 第2引数：メソッド名

    array_init(&params[2]);  // 第3引数：メソッドの引数を格納する配列を初期化
    uint32_t arg_count = ZEND_CALL_NUM_ARGS(execute_data);  // 引数の数を取得
    zval *args = ZEND_CALL_ARG(execute_data, 1);  // 引数の配列を取得
    for (uint32_t i = 0; i < arg_count; i++) {  // 各引数に対して
        zval *arg = &args[i];
        Z_TRY_ADDREF_P(arg);  // 参照カウントを増やす
        add_next_index_zval(&params[2], arg);  // 配列に追加
    }

    RAYAOP_G(is_intercepting) = 1;  // インターセプトフラグを設定
    zval func_name;
    ZVAL_STRING(&func_name, "intercept");  // 呼び出す関数名を設定

    ZVAL_UNDEF(&retval);  // 戻り値を未定義に初期化
    if (call_user_function(NULL, &info->handler, &func_name, &retval, 3, params) == SUCCESS) {  // インターセプト関数を呼び出し
        if (!Z_ISUNDEF(retval)) {  // 戻り値が定義されている場合
            ZVAL_COPY(execute_data->return_value, &retval);  // 戻り値をコピー
        }
    } else {  // インターセプトに失敗した場合
        php_error_docref(NULL, E_WARNING, "Interception failed for %s::%s", ZSTR_VAL(info->class_name), ZSTR_VAL(info->method_name));  // 警告を出力
    }

    zval_ptr_dtor(&retval);  // 戻り値のメモリを解放
    zval_ptr_dtor(&func_name);  // 関数名のメモリを解放
    zval_ptr_dtor(&params[1]);  // メソッド名のメモリを解放
    zval_ptr_dtor(&params[2]);  // 引数配列のメモリを解放

    RAYAOP_G(is_intercepting) = 0;  // インターセプトフラグをリセット
}

/**
 * インターセプト情報を解放する関数
 * @param zval *zv 解放するインターセプト情報を含むzval
 */
void rayaop_free_intercept_info(zval *zv) {
    intercept_info *info = Z_PTR_P(zv);  // zvalからインターセプト情報ポインタを取得
    if (info) {  // インターセプト情報が存在する場合
        RAYAOP_DEBUG_PRINT("Freeing intercept info for %s::%s", ZSTR_VAL(info->class_name), ZSTR_VAL(info->method_name));  // デバッグ情報を出力
        zend_string_release(info->class_name);  // クラス名のメモリを解放
        zend_string_release(info->method_name);  // メソッド名のメモリを解放
        zval_ptr_dtor(&info->handler);  // ハンドラーのメモリを解放
        efree(info);  // インターセプト情報構造体のメモリを解放
    }
}

/**
 * カスタム zend_execute_ex 関数
 * @param zend_execute_data *execute_data 実行データ
 */
static void rayaop_zend_execute_ex(zend_execute_data *execute_data) {
    RAYAOP_DEBUG_PRINT("rayaop_zend_execute_ex called");  // デバッグ情報を出力

    if (!rayaop_should_intercept(execute_data)) {  // インターセプトが不要な場合
        original_zend_execute_ex(execute_data);  // 元の実行関数を呼び出し
        return;  // 処理を終了
    }

    zend_function *current_function = execute_data->func;  // 現在の関数情報を取得
    zend_string *class_name = current_function->common.scope->name;  // クラス名を取得
    zend_string *method_name = current_function->common.function_name;  // メソッド名を取得

    size_t key_len;
    char *key = rayaop_generate_intercept_key(class_name, method_name, &key_len);  // インターセプトキーを生成

    intercept_info *info = rayaop_find_intercept_info(key, key_len);  // インターセプト情報を検索

    if (info) {  // インターセプト情報が見つかった場合
        RAYAOP_DEBUG_PRINT("Found intercept info for key: %s", key);  // デバッグ情報を出力
        rayaop_execute_intercept(execute_data, info);  // インターセプトを実行
    } else {  // インターセプト情報が見つからなかった場合
        RAYAOP_DEBUG_PRINT("No intercept info found for key: %s", key);  // デバッグ情報を出力
        original_zend_execute_ex(execute_data);  // 元の実行関数を呼び出し
    }

    efree(key);  // キーのメモリを解放
}

/**
 * ハッシュテーブル更新失敗時の処理
 * @param intercept_info *new_info 新しいインターセプト情報
 * @param char *key 更新に使用されたキー
 */
void hash_update_failed(intercept_info *new_info, char *key) {
    rayaop_handle_error("Failed to update intercept hash table");  // エラーメッセージを出力
    zend_string_release(new_info->class_name);  // クラス名のメモリを解放
    zend_string_release(new_info->method_name);  // メソッド名のメモリを解放
    zval_ptr_dtor(&new_info->handler);  // ハンドラーのメモリを解放
    efree(new_info);  // インターセプト情報構造体のメモリを解放
    efree(key);  // キーのメモリを解放
}

/**
 * インターセプトメソッドを登録する関数
 * link: https://www.phpinternalsbook.com/php7/extensions_design/php_functions.html
 *
 * @param INTERNAL_FUNCTION_PARAMETERS 内部関数パラメータ
 */
PHP_FUNCTION(method_intercept) {
    RAYAOP_DEBUG_PRINT("method_intercept called");  // デバッグ情報を出力

    char *class_name, *method_name;  // クラス名とメソッド名のポインタ
    size_t class_name_len, method_name_len;  // クラス名とメソッド名の長さ
    zval *intercepted;  // インターセプトハンドラー

    ZEND_PARSE_PARAMETERS_START(3, 3)
        Z_PARAM_STRING(class_name, class_name_len)  // クラス名のパラメータを解析
        Z_PARAM_STRING(method_name, method_name_len)  // メソッド名のパラメータを解析
        Z_PARAM_OBJECT(intercepted)  // インターセプトハンドラーのパラメータを解析
    ZEND_PARSE_PARAMETERS_END();

    intercept_info *new_info = ecalloc(1, sizeof(intercept_info));  // 新しいインターセプト情報のメモリを確保
    if (!new_info) {  // メモリ確保に失敗した場合
        rayaop_handle_error("Failed to allocate memory for intercept_info");  // エラーメッセージを出力
        RETURN_FALSE;  // falseを返して終了
    }

new_info->class_name = zend_string_init(class_name, class_name_len, 0);  // クラス名を初期化
    new_info->method_name = zend_string_init(method_name, method_name_len, 0);  // メソッド名を初期化
    ZVAL_COPY(&new_info->handler, intercepted);  // インターセプトハンドラーをコピー

    char *key = NULL;
    size_t key_len = spprintf(&key, 0, "%s::%s", class_name, method_name);  // インターセプトキーを生成

    if (zend_hash_str_update_ptr(RAYAOP_G(intercept_ht), key, key_len, new_info) == NULL) {  // ハッシュテーブルに追加
        hash_update_failed(new_info, key);  // 追加に失敗した場合、エラー処理を実行
        RETURN_FALSE;  // falseを返して終了
    }

    efree(key);  // キーのメモリを解放
    RAYAOP_DEBUG_PRINT("Successfully registered intercept info");  // デバッグ情報を出力
    RETURN_TRUE;  // trueを返して終了
}

// インターフェースの定義
zend_class_entry *ray_aop_method_interceptor_interface_ce;

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_ray_aop_method_interceptor_intercept, 0, 3, IS_MIXED, 0)
    ZEND_ARG_TYPE_INFO(0, object, IS_OBJECT, 0)  // 第1引数：オブジェクト
    ZEND_ARG_TYPE_INFO(0, method, IS_STRING, 0)  // 第2引数：メソッド名
    ZEND_ARG_TYPE_INFO(0, params, IS_ARRAY, 0)  // 第3引数：パラメータ配列
ZEND_END_ARG_INFO()

static const zend_function_entry ray_aop_method_interceptor_interface_methods[] = {
    ZEND_ABSTRACT_ME(Ray_Aop_MethodInterceptorInterface, intercept, arginfo_ray_aop_method_interceptor_intercept)  // interceptメソッドを定義
    PHP_FE_END  // 関数エントリの終了
};

/**
 * 拡張機能の初期化関数
 * @param INIT_FUNC_ARGS 初期化関数の引数
 * @return int 初期化の成功・失敗
 */
PHP_MINIT_FUNCTION(rayaop)
{
    RAYAOP_DEBUG_PRINT("PHP_MINIT_FUNCTION called");  // デバッグ情報を出力

#ifdef ZTS
    ts_allocate_id(&rayaop_globals_id, sizeof(zend_rayaop_globals), (ts_allocate_ctor) php_rayaop_init_globals, NULL);  // スレッドセーフモードでのグローバル変数の初期化
#else
    php_rayaop_init_globals(&rayaop_globals);  // 非スレッドセーフモードでのグローバル変数の初期化
#endif

    zend_class_entry ce;
    INIT_CLASS_ENTRY(ce, "Ray\\Aop\\MethodInterceptorInterface", ray_aop_method_interceptor_interface_methods);  // クラスエントリを初期化
    ray_aop_method_interceptor_interface_ce = zend_register_internal_interface(&ce);  // インターフェースを登録

    original_zend_execute_ex = zend_execute_ex;  // 元のzend_execute_ex関数を保存
    zend_execute_ex = rayaop_zend_execute_ex;  // カスタムのzend_execute_ex関数を設定

    RAYAOP_DEBUG_PRINT("RayAOP extension initialized");  // デバッグ情報を出力
    return SUCCESS;  // 成功を返す
}

/**
 * 拡張機能のシャットダウン関数
 * @param SHUTDOWN_FUNC_ARGS シャットダウン関数の引数
 * @return int シャットダウンの成功・失敗
 */
PHP_MSHUTDOWN_FUNCTION(rayaop)
{
    RAYAOP_DEBUG_PRINT("RayAOP PHP_MSHUTDOWN_FUNCTION called");  // デバッグ情報を出力

    zend_execute_ex = original_zend_execute_ex;  // 元のzend_execute_ex関数を復元
    original_zend_execute_ex = NULL;  // 保存していたポインタをクリア

    RAYAOP_DEBUG_PRINT("RayAOP PHP_MSHUTDOWN_FUNCTION shut down");  // デバッグ情報を出力
    return SUCCESS;  // シャットダウン成功
}

/**
 * リクエスト開始時の初期化関数
 * @param INIT_FUNC_ARGS 初期化関数の引数
 * @return int 初期化の成功・失敗
 */
PHP_RINIT_FUNCTION(rayaop)
{
    RAYAOP_DEBUG_PRINT("PHP_RINIT_FUNCTION called");  // デバッグ情報を出力

    if (RAYAOP_G(intercept_ht) == NULL) {  // インターセプトハッシュテーブルが未初期化の場合
        ALLOC_HASHTABLE(RAYAOP_G(intercept_ht));  // ハッシュテーブルのメモリを確保
        zend_hash_init(RAYAOP_G(intercept_ht), 8, NULL, rayaop_free_intercept_info, 0);  // ハッシュテーブルを初期化
    }
    RAYAOP_G(is_intercepting) = 0;  // インターセプトフラグを初期化

    return SUCCESS;  // 成功を返す
}

/**
 * リクエスト終了時のシャットダウン関数
 * @param SHUTDOWN_FUNC_ARGS シャットダウン関数の引数
 * @return int シャットダウンの成功・失敗
 */
PHP_RSHUTDOWN_FUNCTION(rayaop)
{
    RAYAOP_DEBUG_PRINT("RayAOP PHP_RSHUTDOWN_FUNCTION called");  // デバッグ情報を出力
    if (RAYAOP_G(intercept_ht)) {  // インターセプトハッシュテーブルが存在する場合
        zend_hash_destroy(RAYAOP_G(intercept_ht));  // ハッシュテーブルを破棄
        FREE_HASHTABLE(RAYAOP_G(intercept_ht));  // ハッシュテーブルのメモリを解放
        RAYAOP_G(intercept_ht) = NULL;  // ハッシュテーブルポインタをNULLに設定
    }

    RAYAOP_DEBUG_PRINT("RayAOP PHP_RSHUTDOWN_FUNCTION shut down");  // デバッグ情報を出力
    return SUCCESS;  // シャットダウン成功
}

/**
 * 拡張機能の情報表示関数
 * @param ZEND_MODULE_INFO_FUNC_ARGS 情報表示関数の引数
 */
PHP_MINFO_FUNCTION(rayaop)
{
    php_info_print_table_start();  // 情報テーブルの開始
    php_info_print_table_header(2, "rayaop support", "enabled");  // テーブルヘッダーの表示
    php_info_print_table_row(2, "Version", PHP_RAYAOP_VERSION);  // バージョン情報の表示
    php_info_print_table_end();  // 情報テーブルの終了
}

// デバッグ用：インターセプト情報をダンプする関数
#ifdef RAYAOP_DEBUG
/**
 * インターセプト情報をダンプする関数（デバッグ用）
 */
static void rayaop_dump_intercept_info(void)
{
    RAYAOP_DEBUG_PRINT("Dumping intercept information:");  // デバッグ情報を出力
    if (RAYAOP_G(intercept_ht)) {  // インターセプトハッシュテーブルが存在する場合
        zend_string *key;
        intercept_info *info;
        ZEND_HASH_FOREACH_STR_KEY_PTR(RAYAOP_G(intercept_ht), key, info) {  // ハッシュテーブルの各要素に対して
            if (key && info) {  // キーと情報が存在する場合
                RAYAOP_DEBUG_PRINT("Key: %s", ZSTR_VAL(key));  // キーを出力
                RAYAOP_DEBUG_PRINT("  Class: %s", ZSTR_VAL(info->class_name));  // クラス名を出力
                RAYAOP_DEBUG_PRINT("  Method: %s", ZSTR_VAL(info->method_name));  // メソッド名を出力
                RAYAOP_DEBUG_PRINT("  Handler type: %d", Z_TYPE(info->handler));  // ハンドラーの型を出力
            }
        } ZEND_HASH_FOREACH_END();
    } else {  // インターセプトハッシュテーブルが存在しない場合
        RAYAOP_DEBUG_PRINT("Intercept hash table is not initialized");  // 初期化されていないことを出力
    }
}
#endif

// 拡張機能が提供する関数の定義
static const zend_function_entry rayaop_functions[] = {
    PHP_FE(method_intercept, arginfo_method_intercept)  // method_intercept関数の登録
    PHP_FE_END  // 関数エントリの終了
};

// 拡張機能のモジュールエントリ
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
ZEND_GET_MODULE(rayaop)  // 動的ロード時のモジュール取得関数
#endif
