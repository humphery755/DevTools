C/C++ Call Graph Generator
==================

## install
os install graphviz, graphviz-dev, pygraphviz<br /><br />
cd pvtrace<br />
make<br />
ls<br />
libmyinstrument.so  pvtrace<br />
make install

## link libmyinstrument.so with your C/C++ application
gcc -g -finstrument-functions -finstrument-functions-exclude-file-list=/usr/include -lmyinstrument test.c -o test
## then run your application to generate the trace file
./test<br />
ls<br />
test.c           test           trace.txt<br />
## pvtrace [exe] to generate the call graph
pvtrace test<br />
ls<br />
graph.dot        test           trace.txt<br />
test.c<br />
## dot to generate the graph.svg
dot -T svg graph.dot -o graph.svg<br />
ls<br />
graph.dot        test.c		   test<br />
graph.svg        trace.txt<br />