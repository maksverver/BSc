set xlabel "Iterations (x1000)"
set ylabel "Time (seconds)"
plot "aggregated/peterson-01_wctime" with lines title "Hash table", \
     "aggregated/peterson-02_wctime" with lines title "B-tree", \
     "aggregated/peterson-03_wctime" with lines title "Bender set";
