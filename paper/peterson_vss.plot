set xlabel "Iterations (x1000)"
set ylabel "Memory (MB)"
plot "aggregated/peterson-01_vss" with lines title "Hash table", \
     "aggregated/peterson-02_vss" with lines title "B-tree", \
     "aggregated/peterson-03_vss" with lines title "Bender set";
