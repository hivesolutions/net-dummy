# Net Dummy Linux Kernel module

This is a simple tcp echo server implemented inside the TCP/IP Linux stack.

## Testing

Tehre's currently no script for testing this module, but a simple python script is
being created for such purposes.

## Loading

In order to load the module execute `modprobe ./net_dummy.ko` or `insmod ./net_dummy.ko`.

To check for log messages use `tail /var/log/syslog`.

To start a new device use `ifconfig dummy0 192.168.1.232 up`

In order to unload the module use `rmmod net_dummy`

## Reference

http://lxr.free-electrons.com/source/drivers/net/dummy.c

## Issues

There are currently compiling issues with the newest version of the kernel (2.6.31+)