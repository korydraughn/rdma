#! /bin/bash

# Client
g++ -std=c++17 -Wall -Wextra -o my_client client.cpp \
	-I/home/kory/dev/rdma-core/build/include \
	-L/home/kory/dev/rdma-core/build/lib \
	-libverbs

# Server
g++ -std=c++17 -Wall -Wextra -o my_server server.cpp \
	-I/home/kory/dev/rdma-core/build/include \
	-L/home/kory/dev/rdma-core/build/lib \
	-libverbs
