C/C++ Call Graph Generator
==================

## install
*os install graphviz, graphviz-dev, pygraphviz
## compile
*cd pvtrace
*make
*ls
*libmyinstrument.so  pvtrace
*make install(cp $(LIB) /usr/lib cp $(EXE) /usr/bin)

## link libinstrument.so with your C/C++ application
gcc -g -finstrument-functions -finstrument-functions-exclude-file-list=/usr/include -lmyinstrument test.c -o test
## then run your application to generate the trace file
./test
ls
test.c           test           trace.txt
## pvtrace [exe] to generate the call graph
pvtrace test
ls
graph.dot        test           trace.txt
test.c
## dot to generate the graph.jpg
dot -Tjpg graph.dot -o graph.jpg
ls
graph.dot        test.c		   test
graph.jpg        trace.txt



