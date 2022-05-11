# In order to execute this "Makefile" just type "make" or "make ALL"
.PHONY: clean
OBJS	= sniffer.o workers.o
OUT	= sniffer workers
DIRS	= named_pipes/* out/*
CPP	= g++
FLAGS	= -g -Wall -c

ALL: clean sniffer.o workers.o sniffer workers

sniffer.o: sniffer.cpp
	$(CPP) $(FLAGS) sniffer.cpp
	
workers.o: workers.cpp
	$(CPP) $(FLAGS) workers.cpp

sniffer:
	$(CPP) -g sniffer.o -o sniffer -fsanitize=address -g3

workers:
	$(CPP) -g workers.o -o workers -fsanitize=address -g3

# Clean things
clean:
	rm -f $(OBJS) $(OUT) $(DIRS)
