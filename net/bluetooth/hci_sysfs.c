/* Bluetooth HCI driver model support. */

#include <fikus/debugfs.h>
#include <fikus/module.h>
#include <asm/unaligned.h>

#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>

static struct class *bt_class;

struct dentry *bt_debugfs;
EXPORT_SYMBOL_GPL(bt_debugfs);

static inline char *link_typetostr(int type)
{
	switch (type) {
	case ACL_LINK:
		return "ACL";
	case SCO_LINK:
		return "SCO";
	case ESCO_LINK:
		return "eSCO";
	case LE_LINK:
		return "LE";
	default:
		return "UNKNOWN";
	}
}

static ssize_t show_link_type(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	struct hci_conn *conn = to_hci_conn(dev);
	return sprintf(buf, "%s\n", link_typetostr(conn->type));
}

static ssize_t show_link_address(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct hci_conn *conn = to_hci_conn(dev);
	return sprintf(buf, "%pMR\n", &conn->dst);
}

static ssize_t show_link_features(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct hci_conn *conn = to_hci_conn(dev);

	return sprintf(buf, "0x%02x%02x%02x%02x%02x%02x%02x%02x\n",
		       conn->features[0][0], conn->features[0][1],
		       conn->features[0][2], conn->features[0][3],
		       conn->features[0][4], conn->features[0][5],
		       conn->features[0][6], conn->features[0][7]);
}

#define LINK_ATTR(_name, _mode, _show, _store) \
struct device_attribute link_attr_##_name = __ATTR(_name, _mode, _show, _store)

static LINK_ATTR(type, S_IRUGO, show_link_type, NULL);
static LINK_ATTR(address, S_IRUGO, show_link_address, NULL);
static LINK_ATTR(features, S_IRUGO, show_link_features, NULL);

static struct attribute *bt_link_attrs[] = {
	&link_attr_type.attr,
	&link_attr_address.attr,
	&link_attr_features.attr,
	NULL
};

static struct attribute_group bt_link_group = {
	.attrs = bt_link_attrs,
};

static const struct attribute_group *bt_link_groups[] = {
	&bt_link_group,
	NULL
};

static void bt_link_release(struct device *dev)
{
	struct hci_conn *conn = to_hci_conn(dev);
	kfree(conn);
}

static struct device_type bt_link = {
	.name    = "link",
	.groups  = bt_link_groups,
	.release = bt_link_release,
};

/*
 * The rfcomm tty device will possibly retain even when conn
 * is down, and sysfs doesn't support move zombie device,
 * so we should move the device before conn device is destroyed.
 */
static int __match_tty(struct device *dev, void *data)
{
	return !strncmp(dev_name(dev), "rfcomm", 6);
}

void hci_conn_init_sysfs(struct hci_conn *conn)
{
	struct hci_dev *hdev = conn->hdev;

	BT_DBG("conn %p", conn);

	conn->dev.type = &bt_link;
	conn->dev.class = bt_class;
	conn->dev.parent = &hdev->dev;

	device_initialize(&conn->dev);
}

void hci_conn_add_sysfs(struct hci_conn *conn)
{
	struct hci_dev *hdev = conn->hdev;

	BT_DBG("conn %p", conn);

	dev_set_name(&conn->dev, "%s:%d", hdev->name, conn->handle);

	if (device_add(&conn->dev) < 0) {
		BT_ERR("Failed to register connection device");
		return;
	}

	hci_dev_hold(hdev);
}

void hci_conn_del_sysfs(struct hci_conn *conn)
{
	struct hci_dev *hdev = conn->hdev;

	if (!device_is_registered(&conn->dev))
		return;

	while (1) {
		struct device *dev;

		dev = device_find_child(&conn->dev, NULL, __match_tty);
		if (!dev)
			break;
		device_move(dev, NULL, DPM_ORDER_DEV_LAST);
		put_device(dev);
	}

	device_del(&conn->dev);

	hci_dev_put(hdev);
}

static inline char *host_bustostr(int bus)
{
	switch (bus) {
	case HCI_VIRTUAL:
		return "VIRTUAL";
	case HCI_USB:
		return "USB";
	case HCI_PCCARD:
		return "PCCARD";
	case HCI_UART:
		return "UART";
	case HCI_RS232:
		return "RS232";
	case HCI_PCI:
		return "PCI";
	case HCI_SDIO:
		return "SDIO";
	default:
		return "UNKNOWN";
	}
}

static inline char *host_typetostr(int type)
{
	switch (type) {
	case HCI_BREDR:
		return "BR/EDR";
	case HCI_AMP:
		return "AMP";
	default:
		return "UNKNOWN";
	}
}

static ssize_t show_bus(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct hci_dev *hdev = to_hci_dev(dev);
	return sprintf(buf, "%s\n", host_bustostr(hdev->bus));
}

static ssize_t show_type(struct device *dev,
			 struct device_attribute *attr, char *buf)
{
	struct hci_dev *hdev = to_hci_dev(dev);
	return sprintf(buf, "%s\n", host_typetostr(hdev->dev_type));
}

static ssize_t show_name(struct device *dev,
			 struct device_attribute *attr, char *buf)
{
	struct hci_dev *hdev = to_hci_dev(dev);
	char name[HCI_MAX_NAME_LENGTH + 1];
	int i;

	for (i = 0; i < HCI_MAX_NAME_LENGTH; i++)
		name[i] = hdev->dev_name[i];

	name[HCI_MAX_NAME_LENGTH] = '\0';
	return sprintf(buf, "%s\n", name);
}

static ssize_t show_class(struct device *dev,
			  struct device_attribute *attr, char *buf)
{
	struct hci_dev *hdev = to_hci_dev(dev);
	return sprintf(buf, "0x%.2x%.2x%.2x\n", hdev->dev_class[2],
		       hdev->dev_class[1], hdev->dev_class[0]);
}

static ssize_t show_address(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	struct hci_dev *hdev = to_hci_dev(dev);
	return sprintf(buf, "%pMR\n", &hdev->bdaddr);
}

static ssize_t show_features(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	struct hci_dev *hdev = to_hci_dev(dev);

	return sprintf(buf, "0x%02x%02x%02x%02x%02x%02x%02x%02x\n",
		       hdev->features[0][0], hdev->features[0][1],
		       hdev->features[0][2], hdev->features[0][3],
		       hdev->features[0][4], hdev->features[0][5],
		       hdev->features[0][6], hdev->features[0][7]);
}

static ssize_t show_manufacturer(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct hci_dev *hdev = to_hci_dev(dev);
	return sprintf(buf, "%d\n", hdev->manufacturer);
}

static ssize_t show_hci_version(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct hci_dev *hdev = to_hci_dev(dev);
	return sprintf(buf, "%d\n", hdev->hci_ver);
}

static ssize_t show_hci_revision(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct hci_dev *hdev = to_hci_dev(dev);
	return sprintf(buf, "%d\n", hdev->hci_rev);
}

static ssize_t show_idle_timeout(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct hci_dev *hdev = to_hci_dev(dev);
	return sprintf(buf, "%d\n", hdev->idle_timeout);
}

static ssize_t store_idle_timeout(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct hci_dev *hdev = to_hci_dev(dev);
	unsigned int val;
	int rv;

	rv = kstrtouint(buf, 0, &val);
	if (rv < 0)
		return rv;

	if (val != 0 && (val < 500 || val > 3600000))
		return -EINVAL;

	hdev->idle_timeout = val;

	return count;
}

static ssize_t show_sniff_max_interval(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	struct hci_dev *hdev = to_hci_dev(dev);
	return sprintf(buf, "%d\n", hdev->sniff_max_interval);
}

static ssize_t store_sniff_max_interval(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct hci_dev *hdev = to_hci_dev(dev);
	u16 val;
	int rv;

	rv = kstrtou16(buf, 0, &val);
	if (rv < 0)
		return rv;

	if (val == 0 || val % 2 || val < hdev->sniff_min_interval)
		return -EINVAL;

	hdev->sniff_max_interval = val;

	return count;
}

static ssize_t show_sniff_min_interval(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	struct hci_dev *hdev = to_hci_dev(dev);
	return sprintf(buf, "%d\n", hdev->sniff_min_interval);
}

static ssize_t store_sniff_min_interval(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct hci_dev *hdev = to_hci_dev(dev);
	u16 val;
	int rv;

	rv = kstrtou16(buf, 0, &val);
	if (rv < 0)
		return rv;

	if (val == 0 || val % 2 || val > hdev->sniff_max_interval)
		return -EINVAL;

	hdev->sniff_min_interval = val;

	return count;
}

static DEVICE_ATTR(bus, S_IRUGO, show_bus, NULL);
static DEVICE_ATTR(type, S_IRUGO, show_type, NULL);
static DEVICE_ATTR(name, S_IRUGO, show_name, NULL);
static DEVICE_ATTR(class, S_IRUGO, show_class, NULL);
static DEVICE_ATTR(address, S_IRUGO, show_address, NULL);
static DEVICE_ATTR(features, S_IRUGO, show_features, NULL);
static DEVICE_ATTR(manufacturer, S_IRUGO, show_manufacturer, NULL);
static DEVICE_ATTR(hci_version, S_IRUGO, show_hci_version, NULL);
static DEVICE_ATTR(hci_revision, S_IRUGO, show_hci_revision, NULL);

static DEVICE_ATTR(idle_timeout, S_IRUGO | S_IWUSR,
		   show_idle_timeout, store_idle_timeout);
static DEVICE_ATTR(sniff_max_interval, S_IRUGO | S_IWUSR,
		   show_sniff_max_interval, store_sniff_max_interval);
static DEVICE_ATTR(sniff_min_interval, S_IRUGO | S_IWUSR,
		   show_sniff_min_interval, store_sniff_min_interval);

static struct attribute *bt_host_attrs[] = {
	&dev_attr_bus.attr,
	&dev_attr_type.attr,
	&dev_attr_name.attr,
	&dev_attr_class.attr,
	&dev_attr_address.attr,
	&dev_attr_features.attr,
	&dev_attr_manufacturer.attr,
	&dev_attr_hci_version.attr,
	&dev_attr_hci_revision.attr,
	&dev_attr_idle_timeout.attr,
	&dev_attr_sniff_max_interval.attr,
	&dev_attr_sniff_min_interval.attr,
	NULL
};

static struct attribute_group bt_host_group = {
	.attrs = bt_host_attrs,
};

static const struct attribute_group *bt_host_groups[] = {
	&bt_host_group,
	NULL
};

static void bt_host_release(struct device *dev)
{
	struct hci_dev *hdev = to_hci_dev(dev);
	kfree(hdev);
	module_put(THIS_MODULE);
}

static struct device_type bt_host = {
	.name    = "host",
	.groups  = bt_host_groups,
	.release = bt_host_release,
};

static int inquiry_cache_show(struct seq_file *f, void *p)
{
	struct hci_dev *hdev = f->private;
	struct discovery_state *cache = &hdev->discovery;
	struct inquiry_entry *e;

	hci_dev_lock(hdev);

	list_for_each_entry(e, &cache->all, all) {
		struct inquiry_data *data = &e->data;
		seq_printf(f, "%pMR %d %d %d 0x%.2x%.2x%.2x 0x%.4x %d %d %u\n",
			   &data->bdaddr,
			   data->pscan_rep_mode, data->pscan_period_mode,
			   data->pscan_mode, data->dev_class[2],
			   data->dev_class[1], data->dev_class[0],
			   __le16_to_cpu(data->clock_offset),
			   data->rssi, data->ssp_mode, e->timestamp);
	}

	hci_dev_unlock(hdev);

	return 0;
}

static int inquiry_cache_open(struct inode *inode, struct file *file)
{
	return single_open(file, inquiry_cache_show, inode->i_private);
}

static const struct file_operations inquiry_cache_fops = {
	.open		= inquiry_cache_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int blacklist_show(struct seq_file *f, void *p)
{
	struct hci_dev *hdev = f->private;
	struct bdaddr_list *b;

	hci_dev_lock(hdev);

	list_for_each_entry(b, &hdev->blacklist, list)
		seq_printf(f, "%pMR\n", &b->bdaddr);

	hci_dev_unlock(hdev);

	return 0;
}

static int blacklist_open(struct inode *inode, struct file *file)
{
	return single_open(file, blacklist_show, inode->i_private);
}

static const struct file_operations blacklist_fops = {
	.open		= blacklist_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static void print_bt_uuid(struct seq_file *f, u8 *uuid)
{
	u32 data0, data5;
	u16 data1, data2, data3, data4;

	data5 = get_unaligned_le32(uuid);
	data4 = get_unaligned_le16(uuid + 4);
	data3 = get_unaligned_le16(uuid + 6);
	data2 = get_unaligned_le16(uuid + 8);
	data1 = get_unaligned_le16(uuid + 10);
	data0 = get_unaligned_le32(uuid + 12);

	seq_printf(f, "%.8x-%.4x-%.4x-%.4x-%.4x%.8x\n",
		   data0, data1, data2, data3, data4, data5);
}

static int uuids_show(struct seq_file *f, void *p)
{
	struct hci_dev *hdev = f->private;
	struct bt_uuid *uuid;

	hci_dev_lock(hdev);

	list_for_each_entry(uuid, &hdev->uuids, list)
		print_bt_uuid(f, uuid->uuid);

	hci_dev_unlock(hdev);

	return 0;
}

static int uuids_open(struct inode *inode, struct file *file)
{
	return single_open(file, uuids_show, inode->i_private);
}

static const struct file_operations uuids_fops = {
	.open		= uuids_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int auto_accept_delay_set(void *data, u64 val)
{
	struct hci_dev *hdev = data;

	hci_dev_lock(hdev);

	hdev->auto_accept_delay = val;

	hci_dev_unlock(hdev);

	return 0;
}

static int auto_accept_delay_get(void *data, u64 *val)
{
	struct hci_dev *hdev = data;

	hci_dev_lock(hdev);

	*val = hdev->auto_accept_delay;

	hci_dev_unlock(hdev);

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(auto_accept_delay_fops, auto_accept_delay_get,
			auto_accept_delay_set, "%llu\n");

void hci_init_sysfs(struct hci_dev *hdev)
{
	struct device *dev = &hdev->dev;

	dev->type = &bt_host;
	dev->class = bt_class;

	__module_get(THIS_MODULE);
	device_initialize(dev);
}

int hci_add_sysfs(struct hci_dev *hdev)
{
	struct device *dev = &hdev->dev;
	int err;

	BT_DBG("%p name %s bus %d", hdev, hdev->name, hdev->bus);

	dev_set_name(dev, "%s", hdev->name);

	err = device_add(dev);
	if (err < 0)
		return err;

	if (!bt_debugfs)
		return 0;

	hdev->debugfs = debugfs_create_dir(hdev->name, bt_debugfs);
	if (!hdev->debugfs)
		return 0;

	debugfs_create_file("inquiry_cache", 0444, hdev->debugfs,
			    hdev, &inquiry_cache_fops);

	debugfs_create_file("blacklist", 0444, hdev->debugfs,
			    hdev, &blacklist_fops);

	debugfs_create_file("uuids", 0444, hdev->debugfs, hdev, &uuids_fops);

	debugfs_create_file("auto_accept_delay", 0444, hdev->debugfs, hdev,
			    &auto_accept_delay_fops);
	return 0;
}

void hci_del_sysfs(struct hci_dev *hdev)
{
	BT_DBG("%p name %s bus %d", hdev, hdev->name, hdev->bus);

	debugfs_remove_recursive(hdev->debugfs);

	device_del(&hdev->dev);
}

int __init bt_sysfs_init(void)
{
	bt_debugfs = debugfs_create_dir("bluetooth", NULL);

	bt_class = class_create(THIS_MODULE, "bluetooth");

	return PTR_ERR_OR_ZERO(bt_class);
}

void bt_sysfs_cleanup(void)
{
	class_destroy(bt_class);

	debugfs_remove_recursive(bt_debugfs);
}
