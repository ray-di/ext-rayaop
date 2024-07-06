#ifndef PHP_RAYAOP_H  // PHP_RAYAOP_Hが定義されていない場合
#define PHP_RAYAOP_H  // PHP_RAYAOP_Hを定義（ヘッダーガード）

#ifdef HAVE_CONFIG_H  // HAVE_CONFIG_Hが定義されている場合
#include "config.h"  // config.hをインクルード
#endif

#include "php.h"  // PHP核のヘッダーファイルをインクルード
#include "php_ini.h"  // PHP INI関連のヘッダーファイルをインクルード
#include "ext/standard/info.h"  // 標準拡張モジュールの情報関連ヘッダーをインクルード
#include "zend_exceptions.h"  // Zend例外処理関連のヘッダーをインクルード
#include "zend_interfaces.h"  // Zendインターフェース関連のヘッダーをインクルード

#ifdef ZTS  // スレッドセーフモードの場合
#include "TSRM.h"  // Thread Safe Resource Managerをインクルード
#endif

#define PHP_RAYAOP_VERSION "1.0.0"  // RayAOP拡張機能のバージョンを定義
#define RAYAOP_NS "Ray\\Aop\\"  // RayAOPの名前空間を定義

extern zend_module_entry rayaop_module_entry;  // rayaopモジュールエントリを外部参照として宣言
#define phpext_rayaop_ptr &rayaop_module_entry  // rayaopモジュールへのポインタを定義

#ifdef PHP_WIN32  // Windows環境の場合
#define PHP_RAYAOP_API __declspec(dllexport)  // DLLエクスポート指定
#elif defined(__GNUC__) && __GNUC__ >= 4  // GCC 4以上の場合
#define PHP_RAYAOP_API __attribute__ ((visibility("default")))  // デフォルトの可視性を指定
#else  // その他の環境
#define PHP_RAYAOP_API  // 特に指定なし
#endif

#ifdef ZTS  // スレッドセーフモードの場合
#include "TSRM.h"  // Thread Safe Resource Managerを再度インクルード（冗長だが安全のため）
#endif

// デバッグ出力用マクロ
#ifdef RAYAOP_DEBUG  // デバッグモードが有効な場合
#define RAYAOP_DEBUG_PRINT(fmt, ...) php_printf("RAYAOP DEBUG: " fmt "\n", ##__VA_ARGS__)  // デバッグ出力マクロを定義
#else  // デバッグモードが無効な場合
#define RAYAOP_DEBUG_PRINT(fmt, ...)  // 何もしない
#endif

// エラーコード
#define RAYAOP_ERROR_MEMORY_ALLOCATION 1  // メモリ割り当てエラーのコード
#define RAYAOP_ERROR_HASH_UPDATE 2  // ハッシュ更新エラーのコード

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

// 関数宣言
PHP_MINIT_FUNCTION(rayaop);  // モジュール初期化関数
PHP_MSHUTDOWN_FUNCTION(rayaop);  // モジュールシャットダウン関数
PHP_RINIT_FUNCTION(rayaop);  // リクエスト初期化関数
PHP_RSHUTDOWN_FUNCTION(rayaop);  // リクエストシャットダウン関数
PHP_MINFO_FUNCTION(rayaop);  // モジュール情報関数

PHP_FUNCTION(method_intercept);  // メソッドインターセプト関数

// ユーティリティ関数の宣言
void rayaop_handle_error(const char *message);  // エラーハンドリング関数
bool rayaop_should_intercept(zend_execute_data *execute_data);  // インターセプトの必要性を判断する関数
char* rayaop_generate_intercept_key(zend_string *class_name, zend_string *method_name, size_t *key_len);  // インターセプトキーを生成する関数
intercept_info* rayaop_find_intercept_info(const char *key, size_t key_len);  // インターセプト情報を検索する関数
void rayaop_execute_intercept(zend_execute_data *execute_data, intercept_info *info);  // インターセプトを実行する関数
void rayaop_free_intercept_info(zval *zv);  // インターセプト情報を解放する関数

#ifdef RAYAOP_DEBUG  // デバッグモードが有効な場合
void rayaop_debug_print_zval(zval *value);  // zval値をデバッグ出力する関数
void rayaop_dump_intercept_info(void);  // インターセプト情報をダンプする関数
#endif

ZEND_BEGIN_MODULE_GLOBALS(rayaop)  // rayaopモジュールのグローバル変数の開始
    HashTable *intercept_ht;  // インターセプトハッシュテーブル
    zend_bool is_intercepting;  // インターセプト中フラグ
ZEND_END_MODULE_GLOBALS(rayaop)  // rayaopモジュールのグローバル変数の終了

#ifdef ZTS  // スレッドセーフモードの場合
#define RAYAOP_G(v) TSRMG(rayaop_globals_id, zend_rayaop_globals *, v)  // グローバル変数アクセスマクロ（スレッドセーフ版）
#else  // 非スレッドセーフモードの場合
#define RAYAOP_G(v) (rayaop_globals.v)  // グローバル変数アクセスマクロ（非スレッドセーフ版）
#endif

#endif /* PHP_RAYAOP_H */  // ヘッダーガードの終了