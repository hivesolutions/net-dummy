/*
 Hive Drivers
 Copyright (C) 2008 Hive Solutions Lda.

 This file is part of Hive Drivers.

 Hive Drivers is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Hive Drivers is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Hive Drivers. If not, see <http://www.gnu.org/licenses/>.

 __author__    = João Magalhães <joamag@hive.pt>
 __version__   = 1.0.0
 __revision__  = $LastChangedRevision$
 __date__      = $LastChangedDate$
 __copyright__ = Copyright (c) 2008-2012 Hive Solutions Lda.
 __license__   = GNU General Public License (GPL), Version 3
*/

/**
 * Function called to set the address, in this case only the mac
 * address to the device once the initialization is complete.
 *
 * The parameters argument is assumet to be a socket structure.
 *
 * @param dev The device to be used for the setting of the address.
 * @param parameters The parameters to the setting of the address, this
 * value is assumed to be a socket structure.
 * @return The result of the setting of the address.
 */
static int dummy_set_address(struct net_device *dev, void *parameters);

/**
 * Function called to set the multicast address in the provided
 * device.
 *
 * Under the current module this call as no effect.
 *
 * @param dev The device to be used for the setting of the address.
 */
static void dummy_set_multicast(struct net_device *dev);
static struct rtnl_link_stats64 *dummy_get_stats64(struct net_device *dev, struct rtnl_link_stats64 *stats);
static netdev_tx_t dummy_xmit(struct sk_buff *skb, struct net_device *dev);
static int dummy_dev_init(struct net_device *dev);
static void dummy_dev_uninit(struct net_device *dev);

/**
 * Runs the setup operation in the current device, after
 * this operation the default and base configuration should
 * be set on the device.
 *
 * @param dev The pointer to the device to be configured.
 */
static void dummy_setup(struct net_device *dev);
static int dummy_validate(struct nlattr *tb[], struct nlattr *data[]);
static int __init dummy_init_one(void);
static int __init dummy_init_module(void);
static void __exit dummy_cleanup_module(void);
