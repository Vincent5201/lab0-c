declare -a traces=("1000" "10000" "50000" "100000" "500000")
for i in "${traces[@]}"
do
    perf stat --repeat 10 -o traces/compare_sort/k_sort/"$i"_report \
    -e branches,cycles,instructions,context-switches \
    ./qtest -v 3 -f traces/compare_sort/k_sort/"$i".cmd
done