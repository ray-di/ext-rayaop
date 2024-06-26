# 'rayaop'サポートを有効にするかどうかの設定を追加
PHP_ARG_ENABLE(rayaop, whether to enable rayaop support,
[  --enable-rayaop           Enable rayaop support])

# 'rayaop_advanced'の高度な機能を有効にするかどうかの設定を追加
PHP_ARG_ENABLE(rayaop_advanced, whether to enable advanced rayaop features,
[  --enable-rayaop-advanced  Enable advanced rayaop features], no, no)

# 'rayaop_advanced'の設定が有効になっているかを確認
if test "$PHP_RAYAOP_ADVANCED" = "yes"; then
  # 'rayaop_advanced'が有効ならば、定義を追加
  AC_DEFINE(RAYAOP_ADVANCED, 1, [Whether advanced rayaop features are enabled])
  # 'rayaop_advanced'用の新しい拡張機能を追加
  PHP_NEW_EXTENSION(rayaop, src/rayaop_advanced.c, $ext_shared,, -DRAYAOP_ADVANCED=1)
else
  # 'rayaop_advanced'が無効ならば、通常の'rayaop'拡張機能を追加
  PHP_NEW_EXTENSION(rayaop, src/rayaop.c, $ext_shared)
fi
