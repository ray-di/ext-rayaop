# 'helloworld'サポートを有効にするかどうかの設定を追加
PHP_ARG_ENABLE(helloworld, whether to enable helloworld support,
[  --enable-helloworld           Enable helloworld support])

# 'helloworld_advanced'の高度な機能を有効にするかどうかの設定を追加
PHP_ARG_ENABLE(helloworld_advanced, whether to enable advanced helloworld features,
[  --enable-helloworld-advanced  Enable advanced helloworld features], no, no)

# 'helloworld_advanced'の設定が有効になっているかを確認
if test "$PHP_HELLOWORLD_ADVANCED" = "yes"; then
  # 'helloworld_advanced'が有効ならば、定義を追加
  AC_DEFINE(HELLOWORLD_ADVANCED, 1, [Whether advanced helloworld features are enabled])
  # 'helloworld_advanced'用の新しい拡張機能を追加
  PHP_NEW_EXTENSION(helloworld, src/helloworld_advanced.c, $ext_shared,, -DHELLOWORLD_ADVANCED=1)
else
  # 'helloworld_advanced'が無効ならば、通常の'helloworld'拡張機能を追加
  PHP_NEW_EXTENSION(helloworld, src/helloworld.c, $ext_shared)
fi
