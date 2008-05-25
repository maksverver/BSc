set xlabel "Iterations (x1000)"
set ylabel "Memory (MB)"
plot "aggregated/leader2-01_vss" with lines title "Hash table", \
     "aggregated/leader2-02_vss" with lines title "B-tree", \
     "aggregated/leader2-03_vss" with lines title "Bender set";
