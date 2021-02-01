sudo valgrind --tool=memcheck --leak-check=full --show-reachable=yes --run-libc-freeres=yes --log-file=./valgrind_report.log \
./scenerunner slavenode_port1 $@ 
