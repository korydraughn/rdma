#! /bin/bash

host="$1"
port="$2"
msg="$3"

export LD_LIBRARY_PATH=/home/kory/dev/rdma-core/build/lib

./rdma_client "$host" "$port" "$msg"
