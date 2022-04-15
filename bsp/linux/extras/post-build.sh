#!/bin/sh

echo xxx
echo $TARGET_DIR
echo $STAGING_DIR
echo xxx

echo "#!/bin/sh
echo \"Content-type: text/html\"
echo \"\"
echo '<p>'
cat /proc/version
echo '<p>'
uname -m
" > $1/var/www/data/index.cgi
chmod +x $1/var/www/data/index.cgi


echo "
/sbin/ip link set eth0 up
/sbin/udhcpc -i eth0 -n -s /etc/udhcp/simple.script
/sbin/ifconfig eth0 10.0.2.15/8" > $1/etc/init.d/S00.hack
chmod +x $1/etc/init.d/S00.hack
