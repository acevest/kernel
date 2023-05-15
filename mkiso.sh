CONTAINER_ID=78a
docker exec -it $CONTAINER_ID /bin/bash -c "cd /root/workspace/kernel && ./scripts/mkiso.sh"


MD5=md5sum

if [[ `uname` == 'Darwin' ]]; then
    MD5=md5
fi

$MD5 kernel.iso
