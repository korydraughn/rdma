#! /bin/bash

# Client
g++ -std=c++17 -Wall -Wextra -o rdma_client client.cpp \
	-I/home/kory/dev/rdma-core/build/include \
	-L/home/kory/dev/rdma-core/build/lib \
	-lrdmacm \
	-libverbs

# Server
g++ -std=c++17 -Wall -Wextra -o rdma_server server.cpp \
	-I/home/kory/dev/rdma-core/build/include \
	-L/home/kory/dev/rdma-core/build/lib \
	-lrdmacm \
	-libverbs
