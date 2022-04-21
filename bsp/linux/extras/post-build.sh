#!/bin/sh

echo "#!/bin/sh
echo \"Content-type: text/html\"
echo \"\"
echo '<p>'
cat /proc/version
echo '<p>'
uname -m
echo '<p>'
/sbin/ifconfig
echo '<p>'
mount
echo '<p>'
ls -l /vda
" > $1/var/www/data/index.cgi
chmod +x $1/var/www/data/index.cgi



echo "
MJMN=\$(cat /sys/block/vda/dev | awk -F : '{print \$1\" \"\$2}')

mount -t debugfs none /sys/kernel/debug
/bin/mknod /dev/vda b \$MJMN
mkdir /vda
mount /dev/vda /vda
/sbin/ip link set eth0 up
/sbin/udhcpc -i eth0 -n -s /etc/udhcp/simple.script
/sbin/ifconfig eth0 10.0.2.15/8" > $1/etc/init.d/S00.hack
chmod +x $1/etc/init.d/S00.hack
