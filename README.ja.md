# Ray.Aop PHP拡張機能

[![Build and Test PHP Extension](https://github.com/ray-di/ext-rayaop/actions/workflows/build.yml/badge.svg)](https://github.com/ray-di/ext-rayaop/actions/workflows/build.yml)

<img src="https://ray-di.github.io/images/logo.svg" alt="ray-di logo" width="150px;">

[Ray.Aop](https://github.com/ray-di/Ray.Aop)のコアとなるメソッドインターセプション機能を提供する低レベルPHP拡張機能です。この拡張機能は単独でも使用できますが、Ray.Aopのより高度なAOP機能の基盤として設計されています。

## 特徴

- 効率的な低レベルメソッドインターセプション
- finalクラスやメソッドのインターセプトに対応
- パラメータと戻り値の完全な変更サポート
- `new`キーワードとのシームレスな連携
- スレッドセーフな動作のサポート
- 包括的なエラーハンドリングとデバッグ機能
- 適切なリソース管理による効率的なメモリ使用

## 要件

- PHP 8.1以上
- Linux、macOS、またはWindows（適切なビルドツールが必要）
- マルチスレッド環境では、スレッドセーフビルドのPHPを推奨

## インストール

1. リポジトリのクローン:
```bash
git clone https://github.com/ray-di/ext-rayaop.git
cd ext-rayaop
```

2. ビルドとインストール:
```bash
phpize
./configure
make
make install
```

3. php.iniに以下の行を追加:
```ini
extension=rayaop.so  # Unix/Linux用
extension=rayaop.dll # Windows用

; オプション: デバッグモードの有効化
; rayaop.debug_level = 1
```

4. インストールの確認:
```bash
php -m | grep rayaop
```

## API リファレンス

### コア関数

#### method_intercept_init()
インターセプションシステムを初期化します。インターセプターの登録前に必ず呼び出す必要があります。

```php
bool method_intercept_init()
```

成功時は`true`、失敗時は`false`を返します。

#### method_intercept()
特定のクラスメソッドにインターセプターを登録します。

```php
bool method_intercept(string $class_name, string $method_name, Ray\Aop\MethodInterceptorInterface $interceptor)
```

- すでにインターセプターが登録されている場合は上書きされます
- 成功時は`true`、失敗時は`false`を返します

#### method_intercept_enable()
メソッドインターセプションをグローバルに有効/無効にします。

```php
void method_intercept_enable(bool $enable)
```

### スレッドセーフティ

この拡張機能は完全にスレッドセーフで、以下の機構を使用しています：

- ミューテックスによるリソース保護
- グローバル状態用のスレッドローカルストレージ
- マルチスレッド環境での安全な初期化とクリーンアップ

マルチスレッド環境（PHP-FPMなど）での使用時の注意点：
- PHPでZTS（Zend Thread Safety）が有効になっていることを確認
- 各スレッドが独自のインターセプション状態を維持
- リソースのクリーンアップは自動的にスレッドごとに処理

## エラーハンドリング

### エラーコード

拡張機能は以下のエラーコードを定義しています：

- `RAYAOP_E_MEMORY_ALLOCATION (1)`: メモリ割り当て失敗
- `RAYAOP_E_HASH_UPDATE (2)`: ハッシュテーブル更新失敗
- `RAYAOP_E_INVALID_HANDLER (3)`: 無効なインターセプターハンドラー
- `RAYAOP_E_MAX_DEPTH_EXCEEDED (4)`: 最大インターセプション深度超過
- `RAYAOP_E_NULL_POINTER (5)`: NULLポインターエラー
- `RAYAOP_E_INVALID_STATE (6)`: 無効な内部状態

エラーはPHPのエラー報告システムを通じて報告されます。これらのエラーをキャッチして処理するには、エラー報告を有効にしてください。

## デバッグモード

php.iniでデバッグレベルを設定することでデバッグモードを有効にできます：

```ini
rayaop.debug_level = 1
```

デバッグ出力には以下が含まれます：
- インターセプター登録イベント
- メソッドインターセプションのトレース
- リソースの割り当て/解放
- エラー条件とスタックトレース

## パフォーマンスに関する考慮事項

### 実行深度

無限再帰を防ぐため、最大インターセプション深度に制限があります：
```php
#define MAX_EXECUTION_DEPTH 100
```

ネストされたインターセプターを設計する際は、この制限を考慮してください。

### メモリ管理

拡張機能は慎重なメモリ管理を実装しています：
- インターセプターリソースの自動クリーンアップ
- PHPオブジェクトの適切な参照カウント
- インターセプター置換時の即時リソース解放
- リクエスト終了時のクリーンアップ

### パフォーマンスへの影響

メソッドインターセプションによるオーバーヘッド：
- 直接のメソッド呼び出し：約0.1-0.2μsの追加オーバーヘッド
- 単純なインターセプターでの呼び出し：約1-2μsのオーバーヘッド
- 複雑なインターセプターは実装に応じてより多くのオーバーヘッドが発生する可能性があります

パフォーマンスに関するヒント：
- インターセプターはアプリケーション初期化時に登録
- インターセプターの頻繁な登録/解除を避ける
- パフォーマンスが重要なコードには単純なインターセプターを使用
- 必要のない場合はインターセプションを無効化

## 設計の決定

この拡張機能は、最小限の高性能メソッドインターセプション機能を提供します：

- メソッドあたり1つのインターセプター：最後に登録されたインターセプターが優先されます
    - 新しいインターセプターを登録すると、以前のものは自動的に解除されます
    - この設計により、予測可能な動作と最適なパフォーマンスを確保
- finalクラスのサポート：Pure PHP実装では不可能なfinalクラスやメソッドのインターセプトが可能
- 生のインターセプション：組み込みのマッチングや条件はなし（これらの機能にはRay.Aopを使用）
- スレッドセーフ：PHP-FPMなどのマルチスレッド環境で安全に使用可能

## Ray.Aopとの関係

この拡張機能は低レベルのメソッドインターセプションを提供し、[Ray.Aop](https://github.com/ray-di/Ray.Aop)は高レベルのAOP機能を提供します：

Ray.Aopの提供する機能：
- Matchersを使用した条件付きインターセプション
- メソッドあたり複数のインターセプター
- 属性/アノテーションベースのインターセプション
- 高度なAOP機能

両者を併用する場合：
- Ray.Aopが高レベルのAOPロジックを処理
- この拡張機能が低レベルのインターセプション機構を提供
- Ray.Aopは可能な場合、パフォーマンス向上のためにこの拡張機能を自動的に利用

## 基本的な使用方法

### シンプルなインターセプター
```php
class LoggingInterceptor implements Ray\Aop\MethodInterceptorInterface
{
    public function intercept(object $object, string $method, array $params): mixed
    {
        echo "実行前 {$method}\n";
        $result = $object->$method(...$params);
        echo "実行後 {$method}\n";
        return $result;
    }
}

// 初期化とインターセプションの有効化
method_intercept_init();
method_intercept_enable(true);

// インターセプターの登録
method_intercept(TestClass::class, 'testMethod', new LoggingInterceptor());
```

## 開発

### ビルドスクリプト
```bash
./build.sh clean   # ビルド環境のクリーン
./build.sh prepare # ビルド環境の準備
./build.sh build   # 拡張機能のビルド
./build.sh run     # 拡張機能の実行
./build.sh all     # すべての手順を実行
```

### テスト
```bash
make test
```

特定のテストの実行：
```bash
make test TESTS="-v tests/your_specific_test.phpt"
```

## 既知の制限事項

1. メソッドあたり1つのインターセプターのみ
2. 実行深度の最大値は100レベル
3. メソッドインターセプションのパターンマッチング機能なし
4. インターセプターはメソッドごとに個別に登録が必要

## ライセンス

[MITライセンス](LICENSE)

## 作者

Akihito Koriyama

この拡張機能は、PHP拡張機能開発とPECL標準の複雑さをナビゲートするのに役立つAIペアプログラミングの支援を受けて開発されました。

## 謝辞

このプロジェクトは[JetBrains CLion](https://www.jetbrains.com/clion/)を使用して作成されました。CLionは[オープンソースライセンス](https://www.jetbrains.com/community/opensource/)で無料で利用できます。

強力で使いやすい開発環境を提供してくださったJetBrainsに感謝いたします。