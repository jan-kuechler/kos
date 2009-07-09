# Version updater for kOS
export FILE=include/kos/version.h
export VER="kos_version = \"`git-describe`\";"

mv $FILE $FILE.old
sed "s/kos_version = \"[^\"]*\"\;/$VER/g" $FILE.old > $FILE
rm $FILE.old
