#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// PHPの主要なヘッダーファイルをインクルード
#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
// エクステンション固有のヘッダーファイルをインクルード
#include "include/php_helloworld.h"

// helloworld関数の実装
PHP_FUNCTION(helloworld)
{
    // 単純に "Hello World!" を出力
    php_printf("Hello World!\n");
}

// phpinfo()関数で表示される情報を設定
PHP_MINFO_FUNCTION(helloworld)
{
    // 情報テーブルの開始
    php_info_print_table_start();
    // テーブルヘッダーを追加（2列）
    php_info_print_table_header(2, "helloworld support", "enabled");
    // 情報テーブルの終了
    php_info_print_table_end();
}

// 関数の引数情報を指定（引数がないため空）
ZEND_BEGIN_ARG_INFO(arginfo_helloworld, 0)
ZEND_END_ARG_INFO()

// エクステンションの関数エントリーを定義
const zend_function_entry helloworld_functions[] = {
    PHP_FE(helloworld, arginfo_helloworld)  // helloworld関数をPHPから呼び出し可能にする
    PHP_FE_END  // 関数エントリーの終了を示す
};

// エクステンションのエントリーポイントを定義
zend_module_entry helloworld_module_entry = {
    STANDARD_MODULE_HEADER,  // 標準のモジュールヘッダー
    "helloworld",  // エクステンション名
    helloworld_functions,  // エクステンションの関数リスト
    NULL,  // PHPエクステンションの初期化関数（ここでは使用しない）
    NULL,  // PHPエクステンションの終了関数（ここでは使用しない）
    NULL,  // リクエスト開始時の関数（ここでは使用しない）
    NULL,  // リクエスト終了時の関数（ここでは使用しない）
    PHP_MINFO(helloworld),  // phpinfo()用の関数
    "1.0",  // エクステンションのバージョン
    STANDARD_MODULE_PROPERTIES  // 標準のモジュールプロパティ
};

// 動的にロード可能なモジュールとしてコンパイルされる場合
#ifdef COMPILE_DL_HELLOWORLD
ZEND_GET_MODULE(helloworld)  // エクステンションを取得するためのマクロ
#endif
