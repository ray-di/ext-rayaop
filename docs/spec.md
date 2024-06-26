### Ray.Aop PECL拡張仕様書

#### 概要
Ray.Aopは、PHPにアスペクト指向プログラミング（AOP）の機能を提供するPECL拡張です。本仕様書では、Ray.Aop拡張の機能、使用方法、インターフェイスについて説明します。

#### 機能
1. **インターセプション機能**:
   指定されたクラスとメソッドに対して実行時に指定した`intercept`メソッドがコールされます。

2. **インターセプトハンドラー**:
   インターセプトハンドラーがインターセプターを呼び出します。ここをユーザーが作成することでRay.Aop以外のインターセプターとの連携が可能です。

#### インターフェイスとクラス

##### InterceptHandlerInterface

Ray.Aopは、インターセプションを管理するためのインターフェイスを提供します。このインターフェイスは、PHPのユーザーが実装し、特定のクラスとメソッドに対してカスタムロジックを挿入するために使用されます。

- **Namespace**: `Ray\Aop`
- **メソッド**:
    - `intercept(object $object, string $method, array $params): mixed`

##### MethodInvocationクラス
インターセプターが呼び出されたときに、対象のメソッドを呼び出し、インターセプターのチェインを管理するためのクラスです。

- **Namespace**: `Ray\Aop`
- **メソッド**:
    - `proceed()`: インターセプターのチェインを進め、最終的にターゲットメソッドを実行します。

#### 関数

##### method_intercept
指定されたクラスとメソッドに対してインターセプトハンドラーを登録します。

- **関数名**: `method_intercept`
- **パラメータ**:
    - `string $className`: 対象クラス名
    - `string $methodName`: 対象メソッド名
    - `Ray\Aop\InterceptHandlerInterface $handler`: 登録するインターセプトハンドラー
- **戻り値**: なし (`void`)

#### 使用方法

1. **インターセプトハンドラーの登録**:
   `method_intercept` 関数を使用して、特定のクラスとメソッドにインターセプトハンドラーを登録します。

1.1 Ray.Aopの場合

```php
<?php

namespace Ray\Aop;

class InterceptHandler implements InterceptHandlerInterface {

    /** @var array<string, Interceptor> */
    public function __construct(private array $interceptors)
    {}

    public function intercept(object $object, string $method, array $params): mixed {
        // Ray.Aopの場合
        if (! isset($this->interceptors[get_class($object)][$method])) {
            throw new \LogicException('Interceptors not found');
        }
        $interceptors = $this->interceptors[get_class($object)][$method];
        $invocation = new ReflectiveMethodInvocation($object, $method, $params, $interceptors);

        return $invocation->proceed();
    }
}

method_intercept('MyClass', 'myMethod', $interceptHandler);
```

独自のインターセプトハンドラー

```php
class MyInterceptHandler implements InterceptHandlerInterface
{
    public function intercept(object $object, string $method, array $params): mixed
    {
        $hasSpecificAttribute = true; // Check special attribute for example
        if ($hasSpecificAttribute) {
            // do something before
            $result = call_user_func([$object, $method], ...$params);
            // do something after
            return $result;
        }
    }
}
```