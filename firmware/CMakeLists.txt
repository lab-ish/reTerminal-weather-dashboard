# reTerminal E1002 天気ダッシュボード ファームウェア
# ビルド: docker run --rm -v $PWD/firmware:/project -w /project \
#          espressif/idf:release-v6.1 idf.py build
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(reterminal_weather)
