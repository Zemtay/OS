#!/bin/bash

# 进入 command 目录并执行 make install
cd command
echo -e "stage1: install software\n"
make install #>/dev/null 2>&1 # 将 make 的输出重定向到 null; 丢弃make install命令的标准输出
echo -e "stage1: completed\n"

# 返回上级目录
cd ..

# 执行 make image
echo -e "stage2: build image\n"
make image >/dev/null 2>&1 # 将 make 的输出重定向到 null
echo -e "stage2: completed\n"
