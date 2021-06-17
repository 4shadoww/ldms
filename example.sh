#!/usr/sh

# example.sh
# Unmount the drive
umount /mnt
# Close drive
cryptsetup close drive
# Clear PageCache, dentries and inodes
sync; echo 3 > /proc/sys/vm/drop_caches
# And finally shut down
shutdown now
