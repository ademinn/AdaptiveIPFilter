cmake_minimum_required(VERSION 2.8)

project(AdaptiveIPFilter)

set(CMAKE_CXX_FLAGS "-Wall -std=c++11 -lnfnetlink -lnetfilter_queue -pthread")

set(NFQ_LIB
    packet.cpp
    nfq_packet.cpp
    nfqueue.cpp)

set(IPTABLES_LIB
    iptables.cpp)

set(RAW_SOCKET_LIB
    raw_socket.cpp)

set(PACKET_FILTER_LIB
    config.cpp
    packet_filter.cpp)

set(FILTER_EXE filter.cpp)

set(CHANNEL_EXE channel.cpp)

add_library(nfq STATIC ${NFQ_LIB})

add_library(iptables STATIC ${IPTABLES_LIB})

add_library(raw_socket STATIC ${RAW_SOCKET_LIB})

add_library(packet_filter STATIC ${PACKET_FILTER_LIB})

add_executable(filter ${FILTER_EXE})

add_executable(channel ${CHANNEL_EXE})

target_link_libraries(packet_filter nfq iptables raw_socket jsoncpp)

target_link_libraries(filter packet_filter)

target_link_libraries(channel packet_filter)
