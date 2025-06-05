#!/usr/bin/env bash

# 1. 记录启动时间
start_ts=$(date +%s.%6N)

# 2. 启动解释器并捕获输出
stdbuf -oL ./docs/src/lc3_o2 /root/repo/lc3-vm/docs/supplies/2048.obj \
| while IFS= read -r line; do
    # 3. 当前时间
    now_ts=$(date +%s.%6N)
    # 4. 计算时间差
    elapsed=$(awk -v n="$now_ts" -v s="$start_ts" 'BEGIN{printf "%.6f", n - s}')
    # 5. 打印带时间戳的输出
    printf "[%s] %s\n" "$elapsed" "$line"
done
