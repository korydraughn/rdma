#! /bin/bash

g++ -std=c++17 -Wall -Wextra -pthread -o rdma_app main.cpp \
	-I/home/kory/dev/rdma-core/build/include \
	-L/home/kory/dev/rdma-core/build/lib \
	-libverbs \
        -lboost_program_options \
        -lboost_system

