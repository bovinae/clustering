#! /bin/bash

for ((bar = 10; bar < 5000; bar += 1000)); do
    for ((topk = 10; topk < 5000; topk += 1000)); do
        echo "柱状图: $bar, 分词topk: $topk"
        ./build/ei $bar $topk
        echo ""
    done
done
