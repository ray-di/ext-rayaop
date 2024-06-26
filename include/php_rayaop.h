#ifndef PHP_RAYAOP_H  // ヘッダーファイルが既にインクルードされていないか確認
#define PHP_RAYAOP_H  // ヘッダーファイルをインクルードしたことをマーク

extern zend_module_entry rayaop_module_entry;  // エクステンションのエントリーポイントを宣言
#define phpext_rayaop_ptr &rayaop_module_entry  // エクステンションのエントリーポイントへのポインタを定義

#define PHP_RAYAOP_VERSION "1.0"  // エクステンションのバージョンを定義

typedef struct _zend_rayaop_globals {
    char *greeting;
} zend_rayaop_globals;

#ifdef PHP_WIN32  // Windows環境でコンパイルされる場合
#   define PHP_RAYAOP_API __declspec(dllexport)  // DLLエクスポートを指定
#elif defined(__GNUC__) && __GNUC__ >= 4  // GCC 4.0以上でコンパイルされる場合
#   define PHP_RAYAOP_API __attribute__ ((visibility("default")))  // シンボルの可視性をデフォルトに設定
#else  // それ以外の環境でコンパイルされる場合
#   define PHP_RAYAOP_API  // 空のマクロを定義
#endif

#ifdef ZTS  // スレッドセーフモードでコンパイルされる場合
#include "TSRM.h"  // スレッドセーフリソースマネージャをインクルード
#endif

PHP_FUNCTION(rayaop);  // rayaop関数を宣言

PHP_MINIT_FUNCTION(rayaop);  // エクステンションの初期化関数を宣言
PHP_MSHUTDOWN_FUNCTION(rayaop);  // エクステンションの終了関数を宣言
PHP_MINFO_FUNCTION(rayaop);  // phpinfo()用の関数を宣言

#ifdef ZTS  // スレッドセーフモードでコンパイルされる場合
#define RAYAOP_G(v) TSRMG(rayaop_globals_id, zend_rayaop_globals *, v)  // グローバル変数へのアクセスを定義
#else  // スレッドセーフモードでない場合
#define RAYAOP_G(v) (rayaop_globals.v)  // グローバル変数へのアクセスを定義
#endif

#endif  /* PHP_RAYAOP_H */  // ヘッダーファイルの終わり

