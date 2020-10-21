#! /bin/bash

export LD_LIBRARY_PATH=/home/kory/dev/rdma-core/build/lib

./rdma_app "$@"
