#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// PHPの主要なヘッダーファイルをインクルード
#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "zend_exceptions.h"
#include "zend_interfaces.h"
// エクステンション固有のヘッダーファイルをインクルード
#include "php_rayaop.h"

// エクステンションのバージョンを定義
#define PHP_RAYAOP_VERSION "1.0.0"

// オリジナルのzend_execute_ex関数へのポインタ
static void (*original_zend_execute_ex)(zend_execute_data *execute_data);

// 新しいzend_execute_ex関数の実装
static void rayaop_zend_execute_ex(zend_execute_data *execute_data)
{
    // 現在実行中の関数情報を取得
    zend_function *current_function = execute_data->func;

    // メソッド名を取得
    const char *method_name = current_function->common.function_name
        ? ZSTR_VAL(current_function->common.function_name)
        : "unknown";

    // "Hello {method_name}" メッセージを出力
    php_printf("Hello %s\n", method_name);

    // オリジナルのzend_execute_ex関数を呼び出し
    original_zend_execute_ex(execute_data);
}

/* {{{ proto boolean rayaop_debug()
   Debug function for Ray.Aop */
PHP_FUNCTION(rayaop_debug)
{
    php_printf("Ray.Aop debug function called\n");
    RETURN_TRUE;
}
/* }}} */

// モジュール初期化時に呼ばれる関数
PHP_MINIT_FUNCTION(rayaop)
{
    // オリジナルのzend_execute_exを保存し、新しい関数に置き換え
    original_zend_execute_ex = zend_execute_ex;
    zend_execute_ex = rayaop_zend_execute_ex;
    php_printf("Ray.Aop extension initialized\n");
    return SUCCESS;
}

// モジュール終了時に呼ばれる関数
PHP_MSHUTDOWN_FUNCTION(rayaop)
{
    // オリジナルのzend_execute_exを復元
    zend_execute_ex = original_zend_execute_ex;
    return SUCCESS;
}

// phpinfo()関数で表示される情報を設定
PHP_MINFO_FUNCTION(rayaop)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "rayaop support", "enabled");
    php_info_print_table_row(2, "Version", PHP_RAYAOP_VERSION);
    php_info_print_table_end();
}

// rayaop_debug関数の引数情報を定義
ZEND_BEGIN_ARG_INFO_EX(arginfo_rayaop_debug, 0, 0, 0)
ZEND_END_ARG_INFO()

// エクステンションの関数エントリーを定義
static const zend_function_entry rayaop_functions[] = {
    PHP_FE(rayaop_debug, arginfo_rayaop_debug)
    PHP_FE_END  // 関数エントリーの終了を示す
};

// エクステンションのエントリーポイントを定義
zend_module_entry rayaop_module_entry = {
    STANDARD_MODULE_HEADER,
    "rayaop",                // エクステンション名
    rayaop_functions,        // 関数エントリー
    PHP_MINIT(rayaop),       // モジュール初期化関数
    PHP_MSHUTDOWN(rayaop),   // モジュール終了関数
    NULL,                    // リクエスト開始時の関数（未使用）
    NULL,                    // リクエスト終了時の関数（未使用）
    PHP_MINFO(rayaop),       // モジュール情報関数
    PHP_RAYAOP_VERSION,      // バージョン
    STANDARD_MODULE_PROPERTIES
};

// 動的にロード可能なモジュールとしてコンパイルされる場合
#ifdef COMPILE_DL_RAYAOP
ZEND_GET_MODULE(rayaop)
#endif