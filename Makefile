all: compile

compile:
	g++ -o sniffer sniffer.cpp

run:
	./sniffer -p monitor

clean:
	rm -f *.o sniffer