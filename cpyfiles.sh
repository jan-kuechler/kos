rm -rf tmp
mkdir tmp

mkid initrd bin/initrd

cp bin/kos.bin tmp
cp bin/initrd tmp

cp menu-$1.lst tmp/menu.lst
cp ../tools/grub/grldr tmp
