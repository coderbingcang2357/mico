#sudo valgrind --tool=memcheck --leak-check=yes --show-reachable=yes --run-libc-freeres=yes --log-file=./valgrind_report.log 
#spawn-fcgi fcgi-bin -a 0.0.0.0 -p 8089  $@
