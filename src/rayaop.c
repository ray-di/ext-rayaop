#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "zend_exceptions.h"

// @link https://www.phpinternalsbook.com/php7/extensions_design/php_functions.html
#include "zend_interfaces.h"

// エクステンション固有のヘッダーファイルをインクルード
#include "php_rayaop.h"

// オリジナルのハンドラーを保存するための変数
// @link https://www.phpinternalsbook.com/php7/internal_types/zend_function.html
static void (*original_zend_execute_ex)(zend_execute_data *execute_data);

// 新しい zend_execute_ex 関数
static void rayaop_zend_execute_ex(zend_execute_data *execute_data)
{
    // 現在実行中の関数名を取得
    // @link https://www.phpinternalsbook.com/php7/internal_types/zend_execute_data.html
    zend_function *current_function = execute_data->func;

    // クラスメソッドの場合のみインターセプト
    if (current_function->common.scope && current_function->common.function_name) {
        // クラス名とメソッド名を取得
        const char *class_name = ZSTR_VAL(current_function->common.scope->name);
        const char *method_name = ZSTR_VAL(current_function->common.function_name);

        // メッセージを出力
        php_printf("Hello %s::%s\n", class_name, method_name);
    }

    // オリジナルの zend_execute_ex を呼び出し
    original_zend_execute_ex(execute_data);
}

// モジュール初期化時に呼ばれる関数
// @link https://www.phpinternalsbook.com/php7/extensions_design/php_lifecycle.html
PHP_MINIT_FUNCTION(rayaop)
{
    // オリジナルの zend_execute_ex を保存し、新しい関数に置き換え
    original_zend_execute_ex = zend_execute_ex;
    zend_execute_ex = rayaop_zend_execute_ex;

    return SUCCESS;
}

// モジュール終了時に呼ばれる関数
PHP_MSHUTDOWN_FUNCTION(rayaop)
{
    // オリジナルの zend_execute_ex を復元
    zend_execute_ex = original_zend_execute_ex;

    return SUCCESS;
}

// phpinfo() 関数で表示される情報を設定
PHP_MINFO_FUNCTION(rayaop)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "rayaop support", "enabled");
    php_info_print_table_end();
}

// エクステンションの関数エントリーを定義
const zend_function_entry rayaop_functions[] = {
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

// 動的にロード可能なモジュールとしてコンパイルされる場合
#ifdef COMPILE_DL_RAYAOP
ZEND_GET_MODULE(rayaop)
#endif
