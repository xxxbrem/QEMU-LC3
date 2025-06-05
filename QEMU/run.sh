#!/usr/bin/env bash

# 1. 记录起始时间（秒.微秒）
start_ts=$(date +%s.%6N)

# 2. 启动 QEMU-LC3 并实时读取 stdout
stdbuf -oL ./build/qemu-system-lc3 \
    -bios ./bios/2048.obj \
    -machine mylc3-v1 \
    -nographic \
    -monitor telnet::11236,server,nowait \
| while IFS= read -r line; do
    # 3. 每读取到一行输出，就计算当前时间戳
    now_ts=$(date +%s.%6N)
    # 4. 计算 elapsed = now_ts - start_ts
    elapsed=$(awk -v n="$now_ts" -v s="$start_ts" 'BEGIN{printf "%.6f", n - s}')
    # 5. 打印 elapsed 及原始输出行
    printf "[%s] %s\n" "$elapsed" "$line"
done
