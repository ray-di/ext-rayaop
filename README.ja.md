# Ray.Aop PHP拡張

[![Build and Test PHP Extension](https://github.com/ray-di/ext-rayaop/actions/workflows/build.yml/badge.svg)](https://github.com/ray-di/ext-rayaop/actions/workflows/build.yml)

<img src="https://ray-di.github.io/images/logo.svg" alt="ray-di logo" width="150px;">

[Ray.Aop](https://github.com/ray-di/Ray.Aop)のためのコアメソッドインターセプション機能を提供する低レベルのPHP拡張です。この拡張は単体での使用も可能ですが、Ray.Aopのより高度なAOP機能の基盤として設計されています。

## 特徴

- 効率的な低レベルメソッドインターセプション
- finalクラスおよびメソッドのインターセプトをサポート
- 引数および戻り値の完全な変更サポート
- `new`キーワードとのシームレスな互換性
- スレッドセーフな操作サポート

## 要件

- PHP 8.1以上
- Linux、macOS、またはWindowsでの適切なビルドツール
- マルチスレッド環境ではスレッドセーフなPHPビルドを推奨

## インストール

1. リポジトリをクローンします:
```bash
git clone https://github.com/ray-di/ext-rayaop.git
cd ext-rayaop
```

2. 拡張をビルドし、インストールします:
```bash
phpize
./configure
make
make install
```

3. php.iniファイルに次の行を追加します:
```ini
extension=rayaop.so  # Unix/Linuxの場合
extension=rayaop.dll # Windowsの場合
```

4. インストールを確認します:
```bash
php -m | grep rayaop
```

## 設計の決定

この拡張は、最小限で高パフォーマンスなメソッドインターセプション機能を提供します。

- メソッドごとに1つのインターセプター：各メソッドに対して最後に登録されたインターセプターが優先されます
- finalクラスサポート：純粋なPHP実装と異なり、finalクラスおよびメソッドをインターセプト可能
- 素のインターセプション：組み込みのマッチングや条件はありません（これらの機能はRay.Aopで提供されます）
- スレッドセーフ：PHP-FPMなどのマルチスレッド環境でも安全に使用可能

## Ray.Aopとの関係

この拡張は低レベルのメソッドインターセプションを提供し、[Ray.Aop](https://github.com/ray-di/Ray.Aop)は高レベルのAOP機能を提供します。

Ray.Aopが提供する機能：
- マッチャーを用いた条件付きインターセプション
- メソッドごとの複数インターセプター
- 属性/アノテーションベースのインターセプション
- 高度なAOP機能

両方を併用する場合：
- Ray.Aopが高レベルのAOPロジックを処理
- この拡張が低レベルのインターセプション機構を提供
- Ray.Aopは、パフォーマンス向上のためにこの拡張を自動的に利用

## 基本的な使い方

### シンプルなインターセプター
```php
class LoggingInterceptor implements Ray\Aop\MethodInterceptorInterface
{
    public function intercept(object $object, string $method, array $params): mixed
    {
        echo "Before {$method}\n";
        $result = $object->$method(...$params);
        echo "After {$method}\n";
        return $result;
    }
}

// インターセプターを登録
method_intercept(TestClass::class, 'testMethod', new LoggingInterceptor());
```

### メソッドインターセプションの設定
```php
// インターセプションシステムを初期化
method_intercept_init();

// メソッドインターセプションを有効化
method_intercept_enable(true);

// インターセプターを登録
method_intercept(MyClass::class, 'myMethod', new MyInterceptor());
```

## 開発

### ビルドスクリプト
```bash
./build.sh clean   # ビルド環境をクリーン
./build.sh prepare # ビルド環境を準備
./build.sh build   # 拡張をビルド
./build.sh run     # 拡張を実行
./build.sh all     # 全てのステップを実行
```

### テスト
```bash
make test
```

特定のテストを実行する場合:
```bash
make test TESTS="-v tests/your_specific_test.phpt"
```

## ライセンス

[MIT License](LICENSE)

## 作者

Akihito Koriyama

この拡張は、AIペアプログラミングの支援を受けて開発され、PHP拡張開発およびPECL基準に関する複雑さを克服するのに役立ちました。