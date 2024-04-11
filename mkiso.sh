#!/bin/bash
#
if [ $# -ne 1 ]; then
    docker ps -a
    echo "input containerid "
    read CONTAINER_ID
else
    CONTAINER_ID=$1
fi

echo "container id ${CONTAINER_ID}"

docker exec -it $CONTAINER_ID /bin/bash -c "cd /root/workspace/kernel && ./scripts/mkiso.sh"


MD5=md5sum

if [[ `uname` == 'Darwin' ]]; then
    MD5=md5
fi

$MD5 kernel.iso
