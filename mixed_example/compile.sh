#! /bin/bash

g++ -std=c++17 -Wall -Wextra -pthread -o rdma_app -lrdma_cma -libverbs -lboost_program_options -lboost_system

