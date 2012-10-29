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
