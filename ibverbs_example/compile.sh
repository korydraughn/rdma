#! /bin/bash

# Client
g++ -std=c++17 -Wall -Wextra -pthread -o my_client client.cpp \
	-I/home/kory/dev/rdma-core/build/include \
	-L/home/kory/dev/rdma-core/build/lib \
	-libverbs \
        -lboost_system

# Server
g++ -std=c++17 -Wall -Wextra -pthread -o my_server server.cpp \
	-I/home/kory/dev/rdma-core/build/include \
	-L/home/kory/dev/rdma-core/build/lib \
	-libverbs \
        -lboost_system
