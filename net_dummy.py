#!/usr/bin/python
# -*- coding: utf-8 -*-

# Hive Drivers
# Copyright (c) 2008-2015 Hive Solutions Lda.
#
# This file is part of Hive Drivers.
#
# Hive Drivers is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Hive Drivers is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Hive Drivers. If not, see <http://www.gnu.org/licenses/>.

__author__ = "João Magalhães <joamag@hive.pt>"
""" The author(s) of the module """

__version__ = "1.0.0"
""" The version of the module """

__revision__ = "$LastChangedRevision$"
""" The revision number of the module """

__date__ = "$LastChangedDate$"
""" The last change date of the module """

__copyright__ = "Copyright (c) 2008-2015 Hive Solutions Lda."
""" The copyright for the module """

__license__ = "GNU General Public License (GPL), Version 3"
""" The license for the module """

import socket

UDP_IP = "192.168.0.2"
""" The ip address of the target to be used
to send the udp packet """

UDP_PORT = 5005
""" The ip port of the target to be used
to send the udp packet """

MESSAGE = "Hello, World!"
""" The message to be sent in the udp packet,
not that it cannot exceed the maximum size of
the packet (maximum transmission unit) """

# prints a series of messages indicating the diagnostics
# information about the current operation
print "UDP target IP:", UDP_IP
print "UDP target port:", UDP_PORT
print "message:", MESSAGE

# creates a new socket for the internet protocol
# and configured with the datagram setting (udp)
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.sendto(MESSAGE, (UDP_IP, UDP_PORT))
