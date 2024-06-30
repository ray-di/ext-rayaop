# 必要なCMakeのバージョンを指定
# @link https://cmake.org/cmake/help/latest/command/cmake_minimum_required.html
cmake_minimum_required(VERSION 3.8)

# プロジェクトの名前と使用する言語を指定
# @link https://cmake.org/cmake/help/latest/command/project.html
project(rayaop C)

# コンパイル時に定義するシンボルを指定
# @link https://cmake.org/cmake/help/latest/command/add_compile_definitions.html
add_compile_definitions(HAVE_RAYAOP)

# ソースファイルのリストを指定
# @link https://cmake.org/cmake/help/latest/command/set.html
set(SOURCE_FILES php_rayaop rayaop.c)

# `php-config` コマンドを使ってPHPのインクルードディレクトリを取得
# @link https://cmake.org/cmake/help/latest/command/execute_process.html
execute_process (
        COMMAND php-config --include-dir
        OUTPUT_VARIABLE PHP_SOURCE
)

# 取得したディレクトリの末尾にある改行を削除
# @link https://cmake.org/cmake/help/latest/command/string.html
string(REGEX REPLACE "\n$" "" PHP_SOURCE "${PHP_SOURCE}")

# 使用するソースディレクトリをメッセージとして表示
# @link https://cmake.org/cmake/help/latest/command/message.html
message("Using source directory: ${PHP_SOURCE}")

# インクルードディレクトリを追加
# @link https://cmake.org/cmake/help/latest/command/include_directories.html
include_directories(${PHP_SOURCE})
include_directories(${PHP_SOURCE}/main)
include_directories(${PHP_SOURCE}/Zend)
include_directories(${PHP_SOURCE}/TSRM)
include_directories(${PROJECT_SOURCE_DIR})

# カスタムターゲット `configure` を追加
# `phpize` と `./configure` を実行し、ソースファイルに依存させる
# @link https://cmake.org/cmake/help/latest/command/add_custom_target.html
add_custom_target(configure
        COMMAND phpize && ./configure
        DEPENDS ${SOURCE_FILES}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

# ソースファイルからライブラリを作成（ただし、ALLビルドからは除外）
# @link https://cmake.org/cmake/help/latest/command/add_library.html
add_library(___ EXCLUDE_FROM_ALL ${SOURCE_FILES})
