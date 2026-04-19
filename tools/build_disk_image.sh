#!/bin/bash

IMAGE_NAME=$1
DISK_CONTENTS=$2

if [ "$#" -eq 3 ]; then
    echo "Creating disk image with boot block from $3"
    BOOT_BLOCK=$3
    mkfs.cpm -f zen9 -b $BOOT_BLOCK $IMAGE_NAME
elif [ "$#" -eq 2 ]; then
    echo "Creating disk image without boot block"
    mkfs.cpm -f zen9 $IMAGE_NAME
else
    echo "Usage: $0 <image_name> <disk_contents_dir> [boot_block_file]"
    exit 1
fi

echo "Copying files to disk image..."
for name in `ls $DISK_CONTENTS`; do
    echo $name
    cpmcp -f zen9 $IMAGE_NAME $DISK_CONTENTS/$name 0:$name
done
