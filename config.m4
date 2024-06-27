# 'rayaop'サポートを有効にするかどうかの設定を追加
PHP_ARG_ENABLE(rayaop, whether to enable rayaop support,
[  --enable-rayaop           Enable rayaop support])

PHP_NEW_EXTENSION(rayaop, src/rayaop.c, $ext_shared)

