all: main.cpp nfqueue.h nfqueue.cpp nfqueue.h nfq_packet.cpp ring_buffer.h
	g++ -Wall -lnfnetlink -lnetfilter_queue -pthread -std=c++11 main.cpp nfqueue.cpp nfq_packet.cpp -o filter
