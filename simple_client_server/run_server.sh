#! /bin/bash

port="${1:-9000}"

export LD_LIBRARY_PATH=/home/kory/dev/rdma-core/build/lib

./rdma_server $port
