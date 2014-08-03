all: filter channel

filter: filter.cpp nfqueue.h nfqueue.cpp nfq_packet.h nfq_packet.cpp ring_buffer.h packet.h packet.cpp
	g++ -Wall -lnfnetlink -lnetfilter_queue -pthread -std=c++11 filter.cpp nfqueue.cpp nfq_packet.cpp packet.cpp -o filter

channel: channel.cpp nfqueue.h nfqueue.cpp nfq_packet.h nfq_packet.cpp ring_buffer.h packet.h packet.cpp
	g++ -Wall -lnfnetlink -lnetfilter_queue -pthread -std=c++11 channel.cpp nfqueue.cpp nfq_packet.cpp packet.cpp -o channel
