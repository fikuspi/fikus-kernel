/*
 * File:	portdrv_pci.c
 * Purpose:	PCI Express Port Bus Driver
 *
 * Copyright (C) 2004 Intel
 * Copyright (C) Tom Long Nguyen (tom.l.nguyen@intel.com)
 */

#include <fikus/module.h>
#include <fikus/pci.h>
#include <fikus/kernel.h>
#include <fikus/errno.h>
#include <fikus/pm.h>
#include <fikus/pm_runtime.h>
#include <fikus/init.h>
#include <fikus/pcieport_if.h>
#include <fikus/aer.h>
#include <fikus/dmi.h>
#include <fikus/pci-aspm.h>

#include "portdrv.h"
#include "aer/aerdrv.h"

/*
 * Version Information
 */
#define DRIVER_VERSION "v1.0"
#define DRIVER_AUTHOR "tom.l.nguyen@intel.com"
#define DRIVER_DESC "PCIe Port Bus Driver"
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");

/* If this switch is set, PCIe port native services should not be enabled. */
bool pcie_ports_disabled;

/*
 * If this switch is set, ACPI _OSC will be used to determine whether or not to
 * enable PCIe port native services.
 */
bool pcie_ports_auto = true;

static int __init pcie_port_setup(char *str)
{
	if (!strncmp(str, "compat", 6)) {
		pcie_ports_disabled = true;
	} else if (!strncmp(str, "native", 6)) {
		pcie_ports_disabled = false;
		pcie_ports_auto = false;
	} else if (!strncmp(str, "auto", 4)) {
		pcie_ports_disabled = false;
		pcie_ports_auto = true;
	}

	return 1;
}
__setup("pcie_ports=", pcie_port_setup);

/* global data */

/**
 * pcie_clear_root_pme_status - Clear root port PME interrupt status.
 * @dev: PCIe root port or event collector.
 */
void pcie_clear_root_pme_status(struct pci_dev *dev)
{
	pcie_capability_set_dword(dev, PCI_EXP_RTSTA, PCI_EXP_RTSTA_PME);
}

static int pcie_portdrv_restore_config(struct pci_dev *dev)
{
	int retval;

	retval = pci_enable_device(dev);
	if (retval)
		return retval;
	pci_set_master(dev);
	return 0;
}

#ifdef CONFIG_PM
static int pcie_port_resume_noirq(struct device *dev)
{
	struct pci_dev *pdev = to_pci_dev(dev);

	/*
	 * Some BIOSes forget to clear Root PME Status bits after system wakeup
	 * which breaks ACPI-based runtime wakeup on PCI Express, so clear those
	 * bits now just in case (shouldn't hurt).
	 */
	if (pci_pcie_type(pdev) == PCI_EXP_TYPE_ROOT_PORT)
		pcie_clear_root_pme_status(pdev);
	return 0;
}

#ifdef CONFIG_PM_RUNTIME
struct d3cold_info {
	bool no_d3cold;
	unsigned int d3cold_delay;
};

static int pci_dev_d3cold_info(struct pci_dev *pdev, void *data)
{
	struct d3cold_info *info = data;

	info->d3cold_delay = max_t(unsigned int, pdev->d3cold_delay,
				   info->d3cold_delay);
	if (pdev->no_d3cold)
		info->no_d3cold = true;
	return 0;
}

static int pcie_port_runtime_suspend(struct device *dev)
{
	struct pci_dev *pdev = to_pci_dev(dev);
	struct d3cold_info d3cold_info = {
		.no_d3cold	= false,
		.d3cold_delay	= PCI_PM_D3_WAIT,
	};

	/*
	 * If any subordinate device disable D3cold, we should not put
	 * the port into D3cold.  The D3cold delay of port should be
	 * the max of that of all subordinate devices.
	 */
	pci_walk_bus(pdev->subordinate, pci_dev_d3cold_info, &d3cold_info);
	pdev->no_d3cold = d3cold_info.no_d3cold;
	pdev->d3cold_delay = d3cold_info.d3cold_delay;
	return 0;
}

static int pcie_port_runtime_resume(struct device *dev)
{
	return 0;
}

static int pci_dev_pme_poll(struct pci_dev *pdev, void *data)
{
	bool *pme_poll = data;

	if (pdev->pme_poll)
		*pme_poll = true;
	return 0;
}

static int pcie_port_runtime_idle(struct device *dev)
{
	struct pci_dev *pdev = to_pci_dev(dev);
	bool pme_poll = false;

	/*
	 * If any subordinate device needs pme poll, we should keep
	 * the port in D0, because we need port in D0 to poll it.
	 */
	pci_walk_bus(pdev->subordinate, pci_dev_pme_poll, &pme_poll);
	/* Delay for a short while to prevent too frequent suspend/resume */
	if (!pme_poll)
		pm_schedule_suspend(dev, 10);
	return -EBUSY;
}
#else
#define pcie_port_runtime_suspend	NULL
#define pcie_port_runtime_resume	NULL
#define pcie_port_runtime_idle		NULL
#endif

static const struct dev_pm_ops pcie_portdrv_pm_ops = {
	.suspend	= pcie_port_device_suspend,
	.resume		= pcie_port_device_resume,
	.freeze		= pcie_port_device_suspend,
	.thaw		= pcie_port_device_resume,
	.poweroff	= pcie_port_device_suspend,
	.restore	= pcie_port_device_resume,
	.resume_noirq	= pcie_port_resume_noirq,
	.runtime_suspend = pcie_port_runtime_suspend,
	.runtime_resume = pcie_port_runtime_resume,
	.runtime_idle	= pcie_port_runtime_idle,
};

#define PCIE_PORTDRV_PM_OPS	(&pcie_portdrv_pm_ops)

#else /* !PM */

#define PCIE_PORTDRV_PM_OPS	NULL
#endif /* !PM */

/*
 * pcie_portdrv_probe - Probe PCI-Express port devices
 * @dev: PCI-Express port device being probed
 *
 * If detected invokes the pcie_port_device_register() method for
 * this port device.
 *
 */
static int pcie_portdrv_probe(struct pci_dev *dev,
					const struct pci_device_id *id)
{
	int status;

	if (!pci_is_pcie(dev) ||
	    ((pci_pcie_type(dev) != PCI_EXP_TYPE_ROOT_PORT) &&
	     (pci_pcie_type(dev) != PCI_EXP_TYPE_UPSTREAM) &&
	     (pci_pcie_type(dev) != PCI_EXP_TYPE_DOWNSTREAM)))
		return -ENODEV;

	if (!dev->irq && dev->pin) {
		dev_warn(&dev->dev, "device [%04x:%04x] has invalid IRQ; "
			 "check vendor BIOS\n", dev->vendor, dev->device);
	}
	status = pcie_port_device_register(dev);
	if (status)
		return status;

	pci_save_state(dev);
	/*
	 * D3cold may not work properly on some PCIe port, so disable
	 * it by default.
	 */
	dev->d3cold_allowed = false;
	return 0;
}

static void pcie_portdrv_remove(struct pci_dev *dev)
{
	pcie_port_device_remove(dev);
}

static int error_detected_iter(struct device *device, void *data)
{
	struct pcie_device *pcie_device;
	struct pcie_port_service_driver *driver;
	struct aer_broadcast_data *result_data;
	pci_ers_result_t status;

	result_data = (struct aer_broadcast_data *) data;

	if (device->bus == &pcie_port_bus_type && device->driver) {
		driver = to_service_driver(device->driver);
		if (!driver ||
			!driver->err_handler ||
			!driver->err_handler->error_detected)
			return 0;

		pcie_device = to_pcie_device(device);

		/* Forward error detected message to service drivers */
		status = driver->err_handler->error_detected(
			pcie_device->port,
			result_data->state);
		result_data->result =
			merge_result(result_data->result, status);
	}

	return 0;
}

static pci_ers_result_t pcie_portdrv_error_detected(struct pci_dev *dev,
					enum pci_channel_state error)
{
	struct aer_broadcast_data data = {error, PCI_ERS_RESULT_CAN_RECOVER};

	/* get true return value from &data */
	device_for_each_child(&dev->dev, &data, error_detected_iter);
	return data.result;
}

static int mmio_enabled_iter(struct device *device, void *data)
{
	struct pcie_device *pcie_device;
	struct pcie_port_service_driver *driver;
	pci_ers_result_t status, *result;

	result = (pci_ers_result_t *) data;

	if (device->bus == &pcie_port_bus_type && device->driver) {
		driver = to_service_driver(device->driver);
		if (driver &&
			driver->err_handler &&
			driver->err_handler->mmio_enabled) {
			pcie_device = to_pcie_device(device);

			/* Forward error message to service drivers */
			status = driver->err_handler->mmio_enabled(
					pcie_device->port);
			*result = merge_result(*result, status);
		}
	}

	return 0;
}

static pci_ers_result_t pcie_portdrv_mmio_enabled(struct pci_dev *dev)
{
	pci_ers_result_t status = PCI_ERS_RESULT_RECOVERED;

	/* get true return value from &status */
	device_for_each_child(&dev->dev, &status, mmio_enabled_iter);
	return status;
}

static int slot_reset_iter(struct device *device, void *data)
{
	struct pcie_device *pcie_device;
	struct pcie_port_service_driver *driver;
	pci_ers_result_t status, *result;

	result = (pci_ers_result_t *) data;

	if (device->bus == &pcie_port_bus_type && device->driver) {
		driver = to_service_driver(device->driver);
		if (driver &&
			driver->err_handler &&
			driver->err_handler->slot_reset) {
			pcie_device = to_pcie_device(device);

			/* Forward error message to service drivers */
			status = driver->err_handler->slot_reset(
					pcie_device->port);
			*result = merge_result(*result, status);
		}
	}

	return 0;
}

static pci_ers_result_t pcie_portdrv_slot_reset(struct pci_dev *dev)
{
	pci_ers_result_t status = PCI_ERS_RESULT_RECOVERED;

	/* If fatal, restore cfg space for possible link reset at upstream */
	if (dev->error_state == pci_channel_io_frozen) {
		dev->state_saved = true;
		pci_restore_state(dev);
		pcie_portdrv_restore_config(dev);
		pci_enable_pcie_error_reporting(dev);
	}

	/* get true return value from &status */
	device_for_each_child(&dev->dev, &status, slot_reset_iter);
	return status;
}

static int resume_iter(struct device *device, void *data)
{
	struct pcie_device *pcie_device;
	struct pcie_port_service_driver *driver;

	if (device->bus == &pcie_port_bus_type && device->driver) {
		driver = to_service_driver(device->driver);
		if (driver &&
			driver->err_handler &&
			driver->err_handler->resume) {
			pcie_device = to_pcie_device(device);

			/* Forward error message to service drivers */
			driver->err_handler->resume(pcie_device->port);
		}
	}

	return 0;
}

static void pcie_portdrv_err_resume(struct pci_dev *dev)
{
	device_for_each_child(&dev->dev, NULL, resume_iter);
}

/*
 * FIKUS Device Driver Model
 */
static const struct pci_device_id port_pci_ids[] = { {
	/* handle any PCI-Express port */
	PCI_DEVICE_CLASS(((PCI_CLASS_BRIDGE_PCI << 8) | 0x00), ~0),
	}, { /* end: all zeroes */ }
};
MODULE_DEVICE_TABLE(pci, port_pci_ids);

static const struct pci_error_handlers pcie_portdrv_err_handler = {
	.error_detected = pcie_portdrv_error_detected,
	.mmio_enabled = pcie_portdrv_mmio_enabled,
	.slot_reset = pcie_portdrv_slot_reset,
	.resume = pcie_portdrv_err_resume,
};

static struct pci_driver pcie_portdriver = {
	.name		= "pcieport",
	.id_table	= &port_pci_ids[0],

	.probe		= pcie_portdrv_probe,
	.remove		= pcie_portdrv_remove,

	.err_handler 	= &pcie_portdrv_err_handler,

	.driver.pm 	= PCIE_PORTDRV_PM_OPS,
};

static int __init dmi_pcie_pme_disable_msi(const struct dmi_system_id *d)
{
	pr_notice("%s detected: will not use MSI for PCIe PME signaling\n",
			d->ident);
	pcie_pme_disable_msi();
	return 0;
}

static struct dmi_system_id __initdata pcie_portdrv_dmi_table[] = {
	/*
	 * Boxes that should not use MSI for PCIe PME signaling.
	 */
	{
	 .callback = dmi_pcie_pme_disable_msi,
	 .ident = "MSI Wind U-100",
	 .matches = {
		     DMI_MATCH(DMI_SYS_VENDOR,
		     		"MICRO-STAR INTERNATIONAL CO., LTD"),
		     DMI_MATCH(DMI_PRODUCT_NAME, "U-100"),
		     },
	 },
	 {}
};

static int __init pcie_portdrv_init(void)
{
	int retval;

	if (pcie_ports_disabled)
		return pci_register_driver(&pcie_portdriver);

	dmi_check_system(pcie_portdrv_dmi_table);

	retval = pcie_port_bus_register();
	if (retval) {
		printk(KERN_WARNING "PCIE: bus_register error: %d\n", retval);
		goto out;
	}
	retval = pci_register_driver(&pcie_portdriver);
	if (retval)
		pcie_port_bus_unregister();
 out:
	return retval;
}

module_init(pcie_portdrv_init);
