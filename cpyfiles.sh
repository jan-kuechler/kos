rm -rf tmp
mkdir tmp

cp bin/kos.bin tmp
cp bin/initrd tmp
cp bin/test.mod tmp

cp menu-$1.lst tmp/menu.lst
cp ../tools/grub/grldr tmp
