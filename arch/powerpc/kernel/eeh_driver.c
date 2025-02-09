/*
 * PCI Error Recovery Driver for RPA-compliant PPC64 platform.
 * Copyright IBM Corp. 2004 2005
 * Copyright Linas Vepstas <linas@linas.org> 2004, 2005
 *
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, GOOD TITLE or
 * NON INFRINGEMENT.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Send comments and feedback to Linas Vepstas <linas@austin.ibm.com>
 */
#include <fikus/delay.h>
#include <fikus/interrupt.h>
#include <fikus/irq.h>
#include <fikus/module.h>
#include <fikus/pci.h>
#include <asm/eeh.h>
#include <asm/eeh_event.h>
#include <asm/ppc-pci.h>
#include <asm/pci-bridge.h>
#include <asm/prom.h>
#include <asm/rtas.h>

/**
 * eeh_pcid_name - Retrieve name of PCI device driver
 * @pdev: PCI device
 *
 * This routine is used to retrieve the name of PCI device driver
 * if that's valid.
 */
static inline const char *eeh_pcid_name(struct pci_dev *pdev)
{
	if (pdev && pdev->dev.driver)
		return pdev->dev.driver->name;
	return "";
}

/**
 * eeh_pcid_get - Get the PCI device driver
 * @pdev: PCI device
 *
 * The function is used to retrieve the PCI device driver for
 * the indicated PCI device. Besides, we will increase the reference
 * of the PCI device driver to prevent that being unloaded on
 * the fly. Otherwise, kernel crash would be seen.
 */
static inline struct pci_driver *eeh_pcid_get(struct pci_dev *pdev)
{
	if (!pdev || !pdev->driver)
		return NULL;

	if (!try_module_get(pdev->driver->driver.owner))
		return NULL;

	return pdev->driver;
}

/**
 * eeh_pcid_put - Dereference on the PCI device driver
 * @pdev: PCI device
 *
 * The function is called to do dereference on the PCI device
 * driver of the indicated PCI device.
 */
static inline void eeh_pcid_put(struct pci_dev *pdev)
{
	if (!pdev || !pdev->driver)
		return;

	module_put(pdev->driver->driver.owner);
}

#if 0
static void print_device_node_tree(struct pci_dn *pdn, int dent)
{
	int i;
	struct device_node *pc;

	if (!pdn)
		return;
	for (i = 0; i < dent; i++)
		printk(" ");
	printk("dn=%s mode=%x \tcfg_addr=%x pe_addr=%x \tfull=%s\n",
		pdn->node->name, pdn->eeh_mode, pdn->eeh_config_addr,
		pdn->eeh_pe_config_addr, pdn->node->full_name);
	dent += 3;
	pc = pdn->node->child;
	while (pc) {
		print_device_node_tree(PCI_DN(pc), dent);
		pc = pc->sibling;
	}
}
#endif

/**
 * eeh_disable_irq - Disable interrupt for the recovering device
 * @dev: PCI device
 *
 * This routine must be called when reporting temporary or permanent
 * error to the particular PCI device to disable interrupt of that
 * device. If the device has enabled MSI or MSI-X interrupt, we needn't
 * do real work because EEH should freeze DMA transfers for those PCI
 * devices encountering EEH errors, which includes MSI or MSI-X.
 */
static void eeh_disable_irq(struct pci_dev *dev)
{
	struct eeh_dev *edev = pci_dev_to_eeh_dev(dev);

	/* Don't disable MSI and MSI-X interrupts. They are
	 * effectively disabled by the DMA Stopped state
	 * when an EEH error occurs.
	 */
	if (dev->msi_enabled || dev->msix_enabled)
		return;

	if (!irq_has_action(dev->irq))
		return;

	edev->mode |= EEH_DEV_IRQ_DISABLED;
	disable_irq_nosync(dev->irq);
}

/**
 * eeh_enable_irq - Enable interrupt for the recovering device
 * @dev: PCI device
 *
 * This routine must be called to enable interrupt while failed
 * device could be resumed.
 */
static void eeh_enable_irq(struct pci_dev *dev)
{
	struct eeh_dev *edev = pci_dev_to_eeh_dev(dev);
	struct irq_desc *desc;

	if ((edev->mode) & EEH_DEV_IRQ_DISABLED) {
		edev->mode &= ~EEH_DEV_IRQ_DISABLED;

		desc = irq_to_desc(dev->irq);
		if (desc && desc->depth > 0)
			enable_irq(dev->irq);
	}
}

/**
 * eeh_report_error - Report pci error to each device driver
 * @data: eeh device
 * @userdata: return value
 *
 * Report an EEH error to each device driver, collect up and
 * merge the device driver responses. Cumulative response
 * passed back in "userdata".
 */
static void *eeh_report_error(void *data, void *userdata)
{
	struct eeh_dev *edev = (struct eeh_dev *)data;
	struct pci_dev *dev = eeh_dev_to_pci_dev(edev);
	enum pci_ers_result rc, *res = userdata;
	struct pci_driver *driver;

	/* We might not have the associated PCI device,
	 * then we should continue for next one.
	 */
	if (!dev) return NULL;
	dev->error_state = pci_channel_io_frozen;

	driver = eeh_pcid_get(dev);
	if (!driver) return NULL;

	eeh_disable_irq(dev);

	if (!driver->err_handler ||
	    !driver->err_handler->error_detected) {
		eeh_pcid_put(dev);
		return NULL;
	}

	rc = driver->err_handler->error_detected(dev, pci_channel_io_frozen);

	/* A driver that needs a reset trumps all others */
	if (rc == PCI_ERS_RESULT_NEED_RESET) *res = rc;
	if (*res == PCI_ERS_RESULT_NONE) *res = rc;

	eeh_pcid_put(dev);
	return NULL;
}

/**
 * eeh_report_mmio_enabled - Tell drivers that MMIO has been enabled
 * @data: eeh device
 * @userdata: return value
 *
 * Tells each device driver that IO ports, MMIO and config space I/O
 * are now enabled. Collects up and merges the device driver responses.
 * Cumulative response passed back in "userdata".
 */
static void *eeh_report_mmio_enabled(void *data, void *userdata)
{
	struct eeh_dev *edev = (struct eeh_dev *)data;
	struct pci_dev *dev = eeh_dev_to_pci_dev(edev);
	enum pci_ers_result rc, *res = userdata;
	struct pci_driver *driver;

	driver = eeh_pcid_get(dev);
	if (!driver) return NULL;

	if (!driver->err_handler ||
	    !driver->err_handler->mmio_enabled) {
		eeh_pcid_put(dev);
		return NULL;
	}

	rc = driver->err_handler->mmio_enabled(dev);

	/* A driver that needs a reset trumps all others */
	if (rc == PCI_ERS_RESULT_NEED_RESET) *res = rc;
	if (*res == PCI_ERS_RESULT_NONE) *res = rc;

	eeh_pcid_put(dev);
	return NULL;
}

/**
 * eeh_report_reset - Tell device that slot has been reset
 * @data: eeh device
 * @userdata: return value
 *
 * This routine must be called while EEH tries to reset particular
 * PCI device so that the associated PCI device driver could take
 * some actions, usually to save data the driver needs so that the
 * driver can work again while the device is recovered.
 */
static void *eeh_report_reset(void *data, void *userdata)
{
	struct eeh_dev *edev = (struct eeh_dev *)data;
	struct pci_dev *dev = eeh_dev_to_pci_dev(edev);
	enum pci_ers_result rc, *res = userdata;
	struct pci_driver *driver;

	if (!dev) return NULL;
	dev->error_state = pci_channel_io_normal;

	driver = eeh_pcid_get(dev);
	if (!driver) return NULL;

	eeh_enable_irq(dev);

	if (!driver->err_handler ||
	    !driver->err_handler->slot_reset) {
		eeh_pcid_put(dev);
		return NULL;
	}

	rc = driver->err_handler->slot_reset(dev);
	if ((*res == PCI_ERS_RESULT_NONE) ||
	    (*res == PCI_ERS_RESULT_RECOVERED)) *res = rc;
	if (*res == PCI_ERS_RESULT_DISCONNECT &&
	     rc == PCI_ERS_RESULT_NEED_RESET) *res = rc;

	eeh_pcid_put(dev);
	return NULL;
}

/**
 * eeh_report_resume - Tell device to resume normal operations
 * @data: eeh device
 * @userdata: return value
 *
 * This routine must be called to notify the device driver that it
 * could resume so that the device driver can do some initialization
 * to make the recovered device work again.
 */
static void *eeh_report_resume(void *data, void *userdata)
{
	struct eeh_dev *edev = (struct eeh_dev *)data;
	struct pci_dev *dev = eeh_dev_to_pci_dev(edev);
	struct pci_driver *driver;

	if (!dev) return NULL;
	dev->error_state = pci_channel_io_normal;

	driver = eeh_pcid_get(dev);
	if (!driver) return NULL;

	eeh_enable_irq(dev);

	if (!driver->err_handler ||
	    !driver->err_handler->resume) {
		eeh_pcid_put(dev);
		return NULL;
	}

	driver->err_handler->resume(dev);

	eeh_pcid_put(dev);
	return NULL;
}

/**
 * eeh_report_failure - Tell device driver that device is dead.
 * @data: eeh device
 * @userdata: return value
 *
 * This informs the device driver that the device is permanently
 * dead, and that no further recovery attempts will be made on it.
 */
static void *eeh_report_failure(void *data, void *userdata)
{
	struct eeh_dev *edev = (struct eeh_dev *)data;
	struct pci_dev *dev = eeh_dev_to_pci_dev(edev);
	struct pci_driver *driver;

	if (!dev) return NULL;
	dev->error_state = pci_channel_io_perm_failure;

	driver = eeh_pcid_get(dev);
	if (!driver) return NULL;

	eeh_disable_irq(dev);

	if (!driver->err_handler ||
	    !driver->err_handler->error_detected) {
		eeh_pcid_put(dev);
		return NULL;
	}

	driver->err_handler->error_detected(dev, pci_channel_io_perm_failure);

	eeh_pcid_put(dev);
	return NULL;
}

static void *eeh_rmv_device(void *data, void *userdata)
{
	struct pci_driver *driver;
	struct eeh_dev *edev = (struct eeh_dev *)data;
	struct pci_dev *dev = eeh_dev_to_pci_dev(edev);
	int *removed = (int *)userdata;

	/*
	 * Actually, we should remove the PCI bridges as well.
	 * However, that's lots of complexity to do that,
	 * particularly some of devices under the bridge might
	 * support EEH. So we just care about PCI devices for
	 * simplicity here.
	 */
	if (!dev || (dev->hdr_type & PCI_HEADER_TYPE_BRIDGE))
		return NULL;
	driver = eeh_pcid_get(dev);
	if (driver && driver->err_handler)
		return NULL;

	/* Remove it from PCI subsystem */
	pr_debug("EEH: Removing %s without EEH sensitive driver\n",
		 pci_name(dev));
	edev->bus = dev->bus;
	edev->mode |= EEH_DEV_DISCONNECTED;
	(*removed)++;

	pci_stop_and_remove_bus_device(dev);

	return NULL;
}

static void *eeh_pe_detach_dev(void *data, void *userdata)
{
	struct eeh_pe *pe = (struct eeh_pe *)data;
	struct eeh_dev *edev, *tmp;

	eeh_pe_for_each_dev(pe, edev, tmp) {
		if (!(edev->mode & EEH_DEV_DISCONNECTED))
			continue;

		edev->mode &= ~(EEH_DEV_DISCONNECTED | EEH_DEV_IRQ_DISABLED);
		eeh_rmv_from_parent_pe(edev);
	}

	return NULL;
}

/**
 * eeh_reset_device - Perform actual reset of a pci slot
 * @pe: EEH PE
 * @bus: PCI bus corresponding to the isolcated slot
 *
 * This routine must be called to do reset on the indicated PE.
 * During the reset, udev might be invoked because those affected
 * PCI devices will be removed and then added.
 */
static int eeh_reset_device(struct eeh_pe *pe, struct pci_bus *bus)
{
	struct pci_bus *frozen_bus = eeh_pe_bus_get(pe);
	struct timeval tstamp;
	int cnt, rc, removed = 0;

	/* pcibios will clear the counter; save the value */
	cnt = pe->freeze_count;
	tstamp = pe->tstamp;

	/*
	 * We don't remove the corresponding PE instances because
	 * we need the information afterwords. The attached EEH
	 * devices are expected to be attached soon when calling
	 * into pcibios_add_pci_devices().
	 */
	eeh_pe_state_mark(pe, EEH_PE_KEEP);
	if (bus)
		pcibios_remove_pci_devices(bus);
	else if (frozen_bus)
		eeh_pe_dev_traverse(pe, eeh_rmv_device, &removed);

	/* Reset the pci controller. (Asserts RST#; resets config space).
	 * Reconfigure bridges and devices. Don't try to bring the system
	 * up if the reset failed for some reason.
	 */
	rc = eeh_reset_pe(pe);
	if (rc)
		return rc;

	/* Restore PE */
	eeh_ops->configure_bridge(pe);
	eeh_pe_restore_bars(pe);

	/* Give the system 5 seconds to finish running the user-space
	 * hotplug shutdown scripts, e.g. ifdown for ethernet.  Yes,
	 * this is a hack, but if we don't do this, and try to bring
	 * the device up before the scripts have taken it down,
	 * potentially weird things happen.
	 */
	if (bus) {
		pr_info("EEH: Sleep 5s ahead of complete hotplug\n");
		ssleep(5);

		/*
		 * The EEH device is still connected with its parent
		 * PE. We should disconnect it so the binding can be
		 * rebuilt when adding PCI devices.
		 */
		eeh_pe_traverse(pe, eeh_pe_detach_dev, NULL);
		pcibios_add_pci_devices(bus);
	} else if (frozen_bus && removed) {
		pr_info("EEH: Sleep 5s ahead of partial hotplug\n");
		ssleep(5);

		eeh_pe_traverse(pe, eeh_pe_detach_dev, NULL);
		pcibios_add_pci_devices(frozen_bus);
	}
	eeh_pe_state_clear(pe, EEH_PE_KEEP);

	pe->tstamp = tstamp;
	pe->freeze_count = cnt;

	return 0;
}

/* The longest amount of time to wait for a pci device
 * to come back on line, in seconds.
 */
#define MAX_WAIT_FOR_RECOVERY 150

static void eeh_handle_normal_event(struct eeh_pe *pe)
{
	struct pci_bus *frozen_bus;
	int rc = 0;
	enum pci_ers_result result = PCI_ERS_RESULT_NONE;

	frozen_bus = eeh_pe_bus_get(pe);
	if (!frozen_bus) {
		pr_err("%s: Cannot find PCI bus for PHB#%d-PE#%x\n",
			__func__, pe->phb->global_number, pe->addr);
		return;
	}

	eeh_pe_update_time_stamp(pe);
	pe->freeze_count++;
	if (pe->freeze_count > EEH_MAX_ALLOWED_FREEZES)
		goto excess_failures;
	pr_warning("EEH: This PCI device has failed %d times in the last hour\n",
		pe->freeze_count);

	/* Walk the various device drivers attached to this slot through
	 * a reset sequence, giving each an opportunity to do what it needs
	 * to accomplish the reset.  Each child gets a report of the
	 * status ... if any child can't handle the reset, then the entire
	 * slot is dlpar removed and added.
	 */
	pr_info("EEH: Notify device drivers to shutdown\n");
	eeh_pe_dev_traverse(pe, eeh_report_error, &result);

	/* Get the current PCI slot state. This can take a long time,
	 * sometimes over 3 seconds for certain systems.
	 */
	rc = eeh_ops->wait_state(pe, MAX_WAIT_FOR_RECOVERY*1000);
	if (rc < 0 || rc == EEH_STATE_NOT_SUPPORT) {
		pr_warning("EEH: Permanent failure\n");
		goto hard_fail;
	}

	/* Since rtas may enable MMIO when posting the error log,
	 * don't post the error log until after all dev drivers
	 * have been informed.
	 */
	pr_info("EEH: Collect temporary log\n");
	eeh_slot_error_detail(pe, EEH_LOG_TEMP);

	/* If all device drivers were EEH-unaware, then shut
	 * down all of the device drivers, and hope they
	 * go down willingly, without panicing the system.
	 */
	if (result == PCI_ERS_RESULT_NONE) {
		pr_info("EEH: Reset with hotplug activity\n");
		rc = eeh_reset_device(pe, frozen_bus);
		if (rc) {
			pr_warning("%s: Unable to reset, err=%d\n",
				   __func__, rc);
			goto hard_fail;
		}
	}

	/* If all devices reported they can proceed, then re-enable MMIO */
	if (result == PCI_ERS_RESULT_CAN_RECOVER) {
		pr_info("EEH: Enable I/O for affected devices\n");
		rc = eeh_pci_enable(pe, EEH_OPT_THAW_MMIO);

		if (rc < 0)
			goto hard_fail;
		if (rc) {
			result = PCI_ERS_RESULT_NEED_RESET;
		} else {
			pr_info("EEH: Notify device drivers to resume I/O\n");
			result = PCI_ERS_RESULT_NONE;
			eeh_pe_dev_traverse(pe, eeh_report_mmio_enabled, &result);
		}
	}

	/* If all devices reported they can proceed, then re-enable DMA */
	if (result == PCI_ERS_RESULT_CAN_RECOVER) {
		pr_info("EEH: Enabled DMA for affected devices\n");
		rc = eeh_pci_enable(pe, EEH_OPT_THAW_DMA);

		if (rc < 0)
			goto hard_fail;
		if (rc)
			result = PCI_ERS_RESULT_NEED_RESET;
		else
			result = PCI_ERS_RESULT_RECOVERED;
	}

	/* If any device has a hard failure, then shut off everything. */
	if (result == PCI_ERS_RESULT_DISCONNECT) {
		pr_warning("EEH: Device driver gave up\n");
		goto hard_fail;
	}

	/* If any device called out for a reset, then reset the slot */
	if (result == PCI_ERS_RESULT_NEED_RESET) {
		pr_info("EEH: Reset without hotplug activity\n");
		rc = eeh_reset_device(pe, NULL);
		if (rc) {
			pr_warning("%s: Cannot reset, err=%d\n",
				   __func__, rc);
			goto hard_fail;
		}

		pr_info("EEH: Notify device drivers "
			"the completion of reset\n");
		result = PCI_ERS_RESULT_NONE;
		eeh_pe_dev_traverse(pe, eeh_report_reset, &result);
	}

	/* All devices should claim they have recovered by now. */
	if ((result != PCI_ERS_RESULT_RECOVERED) &&
	    (result != PCI_ERS_RESULT_NONE)) {
		pr_warning("EEH: Not recovered\n");
		goto hard_fail;
	}

	/* Tell all device drivers that they can resume operations */
	pr_info("EEH: Notify device driver to resume\n");
	eeh_pe_dev_traverse(pe, eeh_report_resume, NULL);

	return;

excess_failures:
	/*
	 * About 90% of all real-life EEH failures in the field
	 * are due to poorly seated PCI cards. Only 10% or so are
	 * due to actual, failed cards.
	 */
	pr_err("EEH: PHB#%d-PE#%x has failed %d times in the\n"
	       "last hour and has been permanently disabled.\n"
	       "Please try reseating or replacing it.\n",
		pe->phb->global_number, pe->addr,
		pe->freeze_count);
	goto perm_error;

hard_fail:
	pr_err("EEH: Unable to recover from failure from PHB#%d-PE#%x.\n"
	       "Please try reseating or replacing it\n",
		pe->phb->global_number, pe->addr);

perm_error:
	eeh_slot_error_detail(pe, EEH_LOG_PERM);

	/* Notify all devices that they're about to go down. */
	eeh_pe_dev_traverse(pe, eeh_report_failure, NULL);

	/* Shut down the device drivers for good. */
	if (frozen_bus)
		pcibios_remove_pci_devices(frozen_bus);
}

static void eeh_handle_special_event(void)
{
	struct eeh_pe *pe, *phb_pe;
	struct pci_bus *bus;
	struct pci_controller *hose, *tmp;
	unsigned long flags;
	int rc = 0;

	/*
	 * The return value from next_error() has been classified as follows.
	 * It might be good to enumerate them. However, next_error() is only
	 * supported by PowerNV platform for now. So it would be fine to use
	 * integer directly:
	 *
	 * 4 - Dead IOC           3 - Dead PHB
	 * 2 - Fenced PHB         1 - Frozen PE
	 * 0 - No error found
	 *
	 */
	rc = eeh_ops->next_error(&pe);
	if (rc <= 0)
		return;

	switch (rc) {
	case 4:
		/* Mark all PHBs in dead state */
		eeh_serialize_lock(&flags);
		list_for_each_entry_safe(hose, tmp,
				&hose_list, list_node) {
			phb_pe = eeh_phb_pe_get(hose);
			if (!phb_pe) continue;

			eeh_pe_state_mark(phb_pe,
				EEH_PE_ISOLATED | EEH_PE_PHB_DEAD);
		}
		eeh_serialize_unlock(flags);

		/* Purge all events */
		eeh_remove_event(NULL);
		break;
	case 3:
	case 2:
	case 1:
		/* Mark the PE in fenced state */
		eeh_serialize_lock(&flags);
		if (rc == 3)
			eeh_pe_state_mark(pe,
				EEH_PE_ISOLATED | EEH_PE_PHB_DEAD);
		else
			eeh_pe_state_mark(pe,
				EEH_PE_ISOLATED | EEH_PE_RECOVERING);
		eeh_serialize_unlock(flags);

		/* Purge all events of the PHB */
		eeh_remove_event(pe);
		break;
	default:
		pr_err("%s: Invalid value %d from next_error()\n",
		       __func__, rc);
		return;
	}

	/*
	 * For fenced PHB and frozen PE, it's handled as normal
	 * event. We have to remove the affected PHBs for dead
	 * PHB and IOC
	 */
	if (rc == 2 || rc == 1)
		eeh_handle_normal_event(pe);
	else {
		list_for_each_entry_safe(hose, tmp,
			&hose_list, list_node) {
			phb_pe = eeh_phb_pe_get(hose);
			if (!phb_pe || !(phb_pe->state & EEH_PE_PHB_DEAD))
				continue;

			bus = eeh_pe_bus_get(phb_pe);
			/* Notify all devices that they're about to go down. */
			eeh_pe_dev_traverse(pe, eeh_report_failure, NULL);
			pcibios_remove_pci_devices(bus);
		}
	}
}

/**
 * eeh_handle_event - Reset a PCI device after hard lockup.
 * @pe: EEH PE
 *
 * While PHB detects address or data parity errors on particular PCI
 * slot, the associated PE will be frozen. Besides, DMA's occurring
 * to wild addresses (which usually happen due to bugs in device
 * drivers or in PCI adapter firmware) can cause EEH error. #SERR,
 * #PERR or other misc PCI-related errors also can trigger EEH errors.
 *
 * Recovery process consists of unplugging the device driver (which
 * generated hotplug events to userspace), then issuing a PCI #RST to
 * the device, then reconfiguring the PCI config space for all bridges
 * & devices under this slot, and then finally restarting the device
 * drivers (which cause a second set of hotplug events to go out to
 * userspace).
 */
void eeh_handle_event(struct eeh_pe *pe)
{
	if (pe)
		eeh_handle_normal_event(pe);
	else
		eeh_handle_special_event();
}
