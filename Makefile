.PHONY: clean
objects = sniffer.o workers.o named_pipes/* out/* monitor/*

ALL: clean sniffer workers run

sniffer:
	g++ -g -Wall -c sniffer.cpp
	g++ sniffer.o -o sniffer -lpthread -fsanitize=address -g3

workers:
	g++ -g -Wall -c workers.cpp
	g++ workers.o -o workers -lpthread -fsanitize=address -g3

run:
	./sniffer -p monitor/

clean:
	rm -f sniffer workers $(objects)