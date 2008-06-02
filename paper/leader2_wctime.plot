set xlabel "Iterations (x1000)"
set ylabel "Time (seconds)"
set key left top
plot "aggregated/leader2-01_wctime" with lines title "Hash table", \
     "aggregated/leader2-02_wctime" with lines title "B-tree", \
     "aggregated/leader2-03_wctime" with lines title "Bender set";
