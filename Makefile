all: generate_inject inject library.dylib

clean:
	rm library.dylib inject generate_inject inject.inc trace.dylib

generate_inject: generate_inject.cpp
	$(CXX) generate_inject.cpp -o generate_inject

inject.inc: generate_inject
	./generate_inject > inject.inc

inject: inject.cpp inject.inc
	$(CXX) inject.cpp -o inject

library.dylib: library.m library.cpp
	$(CXX) library.m library.cpp -o library.dylib -dynamiclib -lobjc -framework Foundation

trace.dylib: trace.m 
	$(CXX) trace.m -o trace.dylib -dynamiclib -lobjc -framework Foundation
	install_name_tool -id `pwd`/trace.dylib trace.dylib

test: all
	sudo chgrp procmod inject
	sudo chmod 2755 inject
	@echo if you do not see something about poison, it did not work
	exec ./inject $(shell ps | grep "sleep 100" | grep -v grep | head -n 1 | awk '{print $$1}') $(shell pwd)/library.dylib
	@sleep 1
