#ifndef PHP_HELLOWORLD_H  // ヘッダーファイルが既にインクルードされていないか確認
#define PHP_HELLOWORLD_H  // ヘッダーファイルをインクルードしたことをマーク

extern zend_module_entry helloworld_module_entry;  // エクステンションのエントリーポイントを宣言
#define phpext_helloworld_ptr &helloworld_module_entry  // エクステンションのエントリーポイントへのポインタを定義

#define PHP_HELLOWORLD_VERSION "1.0"  // エクステンションのバージョンを定義

typedef struct _zend_helloworld_globals {
    char *greeting;
} zend_helloworld_globals;

#ifdef PHP_WIN32  // Windows環境でコンパイルされる場合
#   define PHP_HELLOWORLD_API __declspec(dllexport)  // DLLエクスポートを指定
#elif defined(__GNUC__) && __GNUC__ >= 4  // GCC 4.0以上でコンパイルされる場合
#   define PHP_HELLOWORLD_API __attribute__ ((visibility("default")))  // シンボルの可視性をデフォルトに設定
#else  // それ以外の環境でコンパイルされる場合
#   define PHP_HELLOWORLD_API  // 空のマクロを定義
#endif

#ifdef ZTS  // スレッドセーフモードでコンパイルされる場合
#include "TSRM.h"  // スレッドセーフリソースマネージャをインクルード
#endif

PHP_FUNCTION(helloworld);  // helloworld関数を宣言

PHP_MINIT_FUNCTION(helloworld);  // エクステンションの初期化関数を宣言
PHP_MSHUTDOWN_FUNCTION(helloworld);  // エクステンションの終了関数を宣言
PHP_MINFO_FUNCTION(helloworld);  // phpinfo()用の関数を宣言

#ifdef ZTS  // スレッドセーフモードでコンパイルされる場合
#define HELLOWORLD_G(v) TSRMG(helloworld_globals_id, zend_helloworld_globals *, v)  // グローバル変数へのアクセスを定義
#else  // スレッドセーフモードでない場合
#define HELLOWORLD_G(v) (helloworld_globals.v)  // グローバル変数へのアクセスを定義
#endif

#endif  /* PHP_HELLOWORLD_H */  // ヘッダーファイルの終わり

