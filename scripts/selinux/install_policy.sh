#!/bin/sh
if [ `id -u` -ne 0 ]; then
	echo "$0: must be root to install the sefikus policy"
	exit 1
fi
SF=`which setfiles`
if [ $? -eq 1 ]; then
	if [ -f /sbin/setfiles ]; then
		SF="/usr/setfiles"
	else
		echo "no sefikus tools installed: setfiles"
		exit 1
	fi
fi

cd mdp

CP=`which checkpolicy`
VERS=`$CP -V | awk '{print $1}'`

./mdp policy.conf file_contexts
$CP -o policy.$VERS policy.conf

mkdir -p /etc/sefikus/dummy/policy
mkdir -p /etc/sefikus/dummy/contexts/files

cp file_contexts /etc/sefikus/dummy/contexts/files
cp dbus_contexts /etc/sefikus/dummy/contexts
cp policy.$VERS /etc/sefikus/dummy/policy
FC_FILE=/etc/sefikus/dummy/contexts/files/file_contexts

if [ ! -d /etc/sefikus ]; then
	mkdir -p /etc/sefikus
fi
if [ ! -f /etc/sefikus/config ]; then
	cat > /etc/sefikus/config << EOF
SEFIKUS=enforcing
SEFIKUSTYPE=dummy
EOF
else
	TYPE=`cat /etc/sefikus/config | grep "^SEFIKUSTYPE" | tail -1 | awk -F= '{ print $2 '}`
	if [ "eq$TYPE" != "eqdummy" ]; then
		sefikusenabled
		if [ $? -eq 0 ]; then
			echo "SEFikus already enabled with a non-dummy policy."
			echo "Exiting.  Please install policy by hand if that"
			echo "is what you REALLY want."
			exit 1
		fi
		mv /etc/sefikus/config /etc/sefikus/config.mdpbak
		grep -v "^SEFIKUSTYPE" /etc/sefikus/config.mdpbak >> /etc/sefikus/config
		echo "SEFIKUSTYPE=dummy" >> /etc/sefikus/config
	fi
fi

cd /etc/sefikus/dummy/contexts/files
$SF file_contexts /

mounts=`cat /proc/$$/mounts | egrep "ext2|ext3|xfs|jfs|ext4|ext4dev|gfs2" | awk '{ print $2 '}`
$SF file_contexts $mounts


dodev=`cat /proc/$$/mounts | grep "/dev "`
if [ "eq$dodev" != "eq" ]; then
	mount --move /dev /mnt
	$SF file_contexts /dev
	mount --move /mnt /dev
fi

