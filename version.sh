# Version updater for kOS
FILE=include/kos/version.h
VER="kos_version = \"`git describe`\";"
DATE="kos_builddate = \"`date +"%T - %d.%m.%y"`\";"

NAME="kos_buildname = \"$KOS_BUILDNAME\";"

if [ ! -e $FILE ]
then
	cat > $FILE << STOP 
#ifndef KOS_VERSION_H
#define KOS_VERSION_H

static const char *kos_version = "version";
static const char *kos_buildname = "name";
static const char *kos_builddate = "date";

#endif /*KOS_VERSION_H*/	
STOP
fi

mv $FILE $FILE.old
sed "s/kos_version = \"[^\"]*\"\;/$VER/g;s/kos_builddate = \"[^\"]*\"\;/$DATE/g;s/kos_buildname = \"[^\"]*\"\;/$NAME/g" $FILE.old > $FILE
rm $FILE.old
