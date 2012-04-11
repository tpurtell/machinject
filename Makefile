all: generate_inject inject library.dylib

clean:
	rm library.dylib inject generate_inject inject.inc trace.dylib

generate_inject: generate_inject.cpp
	$(CXX) -Wl,-no_pie generate_inject.cpp -o generate_inject

inject.inc: generate_inject
	./generate_inject > inject.inc

inject: inject.cpp inject.inc
	$(CXX) inject.cpp -o inject
	
sleep: sleep.cpp
	$(CXX) -Wl,-no_pie sleep.cpp -o sleep

library.dylib: library.cpp
	$(CXX) library.cpp -o library.dylib -dynamiclib

trace.dylib: trace.m 
	$(CXX) trace.m -o trace.dylib -dynamiclib -lobjc -framework Foundation
	install_name_tool -id `pwd`/trace.dylib trace.dylib

test: all sleep
	./sleep 1337 &
	@sleep 1
	sudo chgrp procmod inject
	sudo chmod 2755 inject
	@echo if you do not see something about poison, it did not work
	@echo -n "PID is " 
	@ps -ax | grep "sleep 1337" | grep -v grep | head -n 1 | awk '{print $$1}'
	
	exec ./inject `ps -ax | grep "sleep 1337" | grep -v grep | head -n 1 | awk '{print $$1}'` $(shell pwd)/library.dylib
	@sleep 2
	@kill -9 `ps -ax | grep "sleep 1337" | grep -v grep | head -n 1 | awk '{print $$1}'` &> /dev/null || echo "its gone..."
	
