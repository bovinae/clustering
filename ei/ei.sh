#! /bin/bash

bar_start=100
topk_start=100
for ((bar = bar_start; bar <= 100000+bar_start; bar += 1000)); do
    for ((topk = topk_start; topk <= 100000+topk_start; topk += 1000)); do
        echo "柱状图: $bar, 分词topk: $topk"
        ./build/ei $bar $topk
        echo ""
    done
done
