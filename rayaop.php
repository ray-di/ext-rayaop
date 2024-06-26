<?php

// デバッグ関数が利用可能か確認
if (function_exists('rayaop_debug')) {
    rayaop_debug();
} else {
    echo "rayaop_debug function not available\n";
}

// グローバル関数（インターセプトされない）
function sayHello($name) {
    echo "sayHello($name) called\n";
}

// テスト用クラス
class TestClass {
    public function __construct() {
        echo "TestClass constructed\n";
    }

    public function testMethod() {
        echo "TestClass::testMethod() called\n";
    }

    public static function staticMethod() {
        echo "TestClass::staticMethod() called\n";
    }
}

// グローバル関数呼び出し（インターセプトされない）
sayHello("World");

// クラスのインスタンス化とメソッド呼び出し
$test = new TestClass();
$test->testMethod();

// 静的メソッド呼び出し
TestClass::staticMethod();

echo "Script execution completed\n";
