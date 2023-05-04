#!/bin/bash
perf record -F 99 -a -g $1
perf script -i perf.data &> perf.unfold
~/workspace/FlameGraph/stackcollapse-perf.pl perf.unfold &> perf.folded
~/workspace/FlameGraph/flamegraph.pl perf.folded > perf.svg