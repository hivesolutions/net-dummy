# Net Dummy Linux Kernel module

This is a simple tcp echo server implemented inside the TCP/IP Linux stack.
This driver takes controll of the complete sub network (any ip address from the network responds).

## Testing

There's currently no script for testing this module, but a simple python script is being created for such purposes.
It should be able to perform basic tasks for the testing of the device driver.

## Loading

In order to load the module execute `modprobe ./dummy.ko` or `insmod ./dummy.ko`.

* To check for log messages use `tail /var/log/syslog`.
* To start a new interface use `ifconfig dummy0 192.168.0.1 up`
* In order to unload the module use `rmmod net_dummy`

## Tricks

Keep in mind that a *different network* (from you local network) should be used to avoid any conflicts.

## Reference

http://lxr.free-electrons.com/source/drivers/net/dummy.c

## Issues

There are currently compiling issues with the newest version of the kernel (2.6.31+)