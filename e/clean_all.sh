# 进入 command 目录并执行 make install
cd command
echo -e "stage1: clean command\n"
make clean >/dev/null 2>&1 # 将 make 的输出重定向到 null
echo -e "stage1: completed\n"

# 返回上级目录
cd ..

# 执行 make image
echo -e "stage2: clean os image\n"
make clean >/dev/null 2>&1 # 将 make 的输出重定向到 null
echo -e "stage2: completed\n"
