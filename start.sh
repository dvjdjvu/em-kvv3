sudo rmmod em_kvv3_mod.ko
sudo insmod ./em_kvv3_mod.ko ports0=0x330 count0=2 irq0=5 name0="/dev/em_kvv0" minor0=0

major=`sudo dmesg | grep "Djvu Init: Device registered. major number is" | tail -n 1 | awk '{size=split($0, arr, "-"); printf("%s\n", arr[2]);}' | tr -d " "`

echo "major number - " $major

sudo rm -rf /dev/em_kvv*
sudo mknod /dev/em_kvv0 c $major 0
sudo mknod /dev/em_kvv1 c $major 1
sudo mknod /dev/em_kvv2 c $major 2
sudo chmod 777 /dev/em_kvv*
