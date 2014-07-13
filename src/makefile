all: main.cpp nfqueue.h nfqueue.cpp
	g++ -Wall -lnfnetlink -lnetfilter_queue -pthread -std=c++11 main.cpp nfqueue.cpp -o filter
