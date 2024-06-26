#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// PHPの主要なヘッダーファイルをインクルード
#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
// エクステンション固有のヘッダーファイルをインクルード
#include "include/php_rayaop.h"

/* モジュールグローバルを宣言します */
ZEND_DECLARE_MODULE_GLOBALS(rayaop)

/* INIエントリを定義します */
PHP_INI_BEGIN()
    /* 定義可能な挨拶メッセージを定義します */
    STD_PHP_INI_ENTRY("rayaop.greeting", "Hello AOP!", PHP_INI_ALL, OnUpdateString, greeting, zend_rayaop_globals, rayaop_globals)
PHP_INI_END()

// 関数の引数情報を指定（引数がないため空）
ZEND_BEGIN_ARG_INFO(arginfo_rayaop_advanced, 0)
ZEND_END_ARG_INFO()

// rayaop関数の実装
PHP_FUNCTION(rayaop)
{
    // 単純に "Hello AOP!" を出力
    php_printf("Hello AOP!\n");
}

/* rayaop_advanced関数の実装 */
PHP_FUNCTION(rayaop_advanced)
{
    ZEND_PARSE_PARAMETERS_NONE();

    /* 設定可能な挨拶メッセージを出力します */
    php_printf("%s\n", RAYAOP_G(greeting));
}


/* モジュール初期化関数 */
PHP_MINIT_FUNCTION(rayaop)
{
    /* INIエントリを登録します */
    REGISTER_INI_ENTRIES();
    return SUCCESS;
}

/* モジュールシャットダウン関数 */
PHP_MSHUTDOWN_FUNCTION(rayaop)
{
    /* INIエントリの登録を解除します */
    UNREGISTER_INI_ENTRIES();
    return SUCCESS;
}

// phpinfo()関数で表示される情報を設定
PHP_MINFO_FUNCTION(rayaop)
{
    // 情報テーブルの開始
    php_info_print_table_start();
    // テーブルヘッダーを追加（2列）
    php_info_print_table_header(2, "rayaop support", "enabled");
    // 情報テーブルの終了
    php_info_print_table_end();

    /* INIエントリを表示します */
    DISPLAY_INI_ENTRIES();
}

// 関数の引数情報を指定（引数がないため空）
ZEND_BEGIN_ARG_INFO(arginfo_rayaop, 0)
ZEND_END_ARG_INFO()

// エクステンションの関数エントリーを定義
const zend_function_entry rayaop_functions[] = {
    PHP_FE(rayaop, arginfo_rayaop)  // rayaop関数をPHPから呼び出し可能にする
    PHP_FE(rayaop_advanced, arginfo_rayaop_advanced) // rayaop_advanced関数をPHPから呼び出し可能にする
    PHP_FE_END  // 関数エントリーの終了を示す
};

// エクステンションのエントリーポイントを定義
zend_module_entry rayaop_module_entry = {
    STANDARD_MODULE_HEADER,  // 標準のモジュールヘッダー
    "rayaop",  // エクステンション名
    rayaop_functions,  // エクステンションの関数リスト
    PHP_MINIT(rayaop),  // PHPエクステンションの初期化関数
    PHP_MSHUTDOWN(rayaop),  // PHPエクステンションの終了関数
    NULL,  // リクエスト開始時の関数（ここでは使用しない）
    NULL,  // リクエスト終了時の関数（ここでは使用しない）
    PHP_MINFO(rayaop),  // phpinfo()用の関数
    "1.0",  // エクステンションのバージョン
    STANDARD_MODULE_PROPERTIES  // 標準のモジュールプロパティ
};

// 動的にロード可能なモジュールとしてコンパイルされる場合
#ifdef COMPILE_DL_RAYAOP
ZEND_GET_MODULE(rayaop)  // エクステンションを取得するためのマクロ
#endif
