declare -a traces=("1000" "10000" "100000" "500000")
for i in "${traces[@]}"
do
    perf stat --repeat 100 -o traces/compare_sort/q_sort/"$i"_report                      \
    -e branches,cache-misses,cycles,instructions,context-switches,cpu-clock,branch-misses \
    ./qtest -v 3 -f traces/compare_sort/q_sort/"$i".cmd
done
# sudo sysctl -w kernel.perf_event_paranoid=-1