#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// PHPの主要なヘッダーファイルをインクルード
#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "zend_exceptions.h"
// @link https://www.phpinternalsbook.com/php7/extensions_design/php_functions.html
#include "zend_interfaces.h"
// エクステンション固有のヘッダーファイルをインクルード
#include "php_rayaop.h"

// オリジナルのzend_execute_ex関数へのポインタ
static void (*original_zend_execute_ex)(zend_execute_data *execute_data);

// 新しいzend_execute_ex関数の実装
static void rayaop_zend_execute_ex(zend_execute_data *execute_data)
{
    // 現在実行中の関数情報を取得
    // @link https://www.phpinternalsbook.com/php7/internal_types/zend_execute_data.html
    zend_function *current_function = execute_data->func;

    // デバッグ情報を出力
    php_printf("Debug: Executing function\n");

    // クラスメソッドの場合
    if (current_function->common.scope && current_function->common.function_name) {
        const char *class_name = ZSTR_VAL(current_function->common.scope->name);
        const char *method_name = ZSTR_VAL(current_function->common.function_name);

        // インターセプトメッセージを出力
        php_printf("Intercepted: %s::%s\n", class_name, method_name);
    }
    // グローバル関数の場合
    else if (current_function->common.function_name) {
        php_printf("Debug: Global function: %s\n", ZSTR_VAL(current_function->common.function_name));
    }
    // その他の場合（匿名関数など）
    else {
        php_printf("Debug: Unknown function type\n");
    }

    // オリジナルのzend_execute_ex関数を呼び出し
    original_zend_execute_ex(execute_data);
}

// デバッグ用の関数を追加
PHP_FUNCTION(rayaop_debug)
{
    php_printf("Ray.Aop debug function called\n");
    RETURN_TRUE;
}

// モジュール初期化時に呼ばれる関数
// @link https://www.phpinternalsbook.com/php7/extensions_design/php_lifecycle.html
PHP_MINIT_FUNCTION(rayaop)
{
    // オリジナルのzend_execute_exを保存し、新しい関数に置き換え
    original_zend_execute_ex = zend_execute_ex;
    zend_execute_ex = rayaop_zend_execute_ex;
    php_printf("Debug: Ray.Aop extension initialized\n");
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

ZEND_BEGIN_ARG_INFO(arginfo_rayaop_debug, 0)
ZEND_END_ARG_INFO()

// エクステンションの関数エントリーを定義
const zend_function_entry rayaop_functions[] = {
    PHP_FE(rayaop_debug, NULL)  // デバッグ関数を追加
    PHP_FE_END  // 関数エントリーの終了を示す
};

// エクステンションのエントリーポイントを定義
zend_module_entry rayaop_module_entry = {
    STANDARD_MODULE_HEADER,
    "rayaop",
    rayaop_functions,
    PHP_MINIT(rayaop),      // 初期化関数
    PHP_MSHUTDOWN(rayaop),  // 終了関数
    NULL,
    NULL,
    PHP_MINFO(rayaop),
    PHP_RAYAOP_VERSION,
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_RAYAOP
ZEND_GET_MODULE(rayaop)
#endif
