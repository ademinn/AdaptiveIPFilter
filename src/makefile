all: main.cpp nfqueue.h nfqueue.cpp nfq_packet.h nfq_packet.cpp ring_buffer.h ethernet_socket.h ethernet_socket.cpp
	g++ -Wall -lnfnetlink -lnetfilter_queue -pthread -std=c++11 main.cpp nfqueue.cpp nfq_packet.cpp ethernet_socket.cpp -o filter
