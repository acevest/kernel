#!/bin/bash
set -x

HDIMG=../HD.IMG
lodev=`losetup -f --show $HDIMG`

partx -a ${lodev}

PART=${lodev}p1

mount $PART /mnt/

while true; do
	read -p "Unmount?[y/n]" yn
	case $yn in
		[Yy]* ) break;;
		[Nn]* ) continue;;
		* ) echo "Please answer yes or no.";;
	esac
done

umount /mnt/

partx -d ${lodev}p1

losetup -d ${lodev}
