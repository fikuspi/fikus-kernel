/*
 * Copyright 2004 Digi International (www.digi.com)
 *      Scott H Kilau <Scott_Kilau at digi dot com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 *      NOTE TO FIKUS KERNEL HACKERS:  DO NOT REFORMAT THIS CODE!
 *
 *      This is shared code between Digi's CVS archive and the
 *      Fikus Kernel sources.
 *      Changing the source just for reformatting needlessly breaks
 *      our CVS diff history.
 *
 *      Send any bug fixes/changes to:  Eng.Fikus at digi dot com.
 *      Thank you.
 *
 */


#include <fikus/kernel.h>
#include <fikus/module.h>
#include <fikus/ctype.h>
#include <fikus/string.h>
#include <fikus/serial_reg.h>
#include <fikus/device.h>
#include <fikus/pci.h>
#include <fikus/kdev_t.h>

#include "dgnc_driver.h"
#include "dgnc_mgmt.h"


static ssize_t dgnc_driver_version_show(struct device_driver *ddp, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", DG_PART);
}
static DRIVER_ATTR(version, S_IRUSR, dgnc_driver_version_show, NULL);


static ssize_t dgnc_driver_boards_show(struct device_driver *ddp, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", dgnc_NumBoards);
}
static DRIVER_ATTR(boards, S_IRUSR, dgnc_driver_boards_show, NULL);


static ssize_t dgnc_driver_maxboards_show(struct device_driver *ddp, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", MAXBOARDS);
}
static DRIVER_ATTR(maxboards, S_IRUSR, dgnc_driver_maxboards_show, NULL);


static ssize_t dgnc_driver_pollcounter_show(struct device_driver *ddp, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%ld\n", dgnc_poll_counter);
}
static DRIVER_ATTR(pollcounter, S_IRUSR, dgnc_driver_pollcounter_show, NULL);


static ssize_t dgnc_driver_state_show(struct device_driver *ddp, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", dgnc_driver_state_text[dgnc_driver_state]);
}
static DRIVER_ATTR(state, S_IRUSR, dgnc_driver_state_show, NULL);


static ssize_t dgnc_driver_debug_show(struct device_driver *ddp, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "0x%x\n", dgnc_debug);
}

static ssize_t dgnc_driver_debug_store(struct device_driver *ddp, const char *buf, size_t count)
{
	sscanf(buf, "0x%x\n", &dgnc_debug);
	return count;
}
static DRIVER_ATTR(debug, (S_IRUSR | S_IWUSR), dgnc_driver_debug_show, dgnc_driver_debug_store);


static ssize_t dgnc_driver_rawreadok_show(struct device_driver *ddp, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "0x%x\n", dgnc_rawreadok);
}

static ssize_t dgnc_driver_rawreadok_store(struct device_driver *ddp, const char *buf, size_t count)
{
	sscanf(buf, "0x%x\n", &dgnc_rawreadok);
	return count;
}
static DRIVER_ATTR(rawreadok, (S_IRUSR | S_IWUSR), dgnc_driver_rawreadok_show, dgnc_driver_rawreadok_store);


static ssize_t dgnc_driver_pollrate_show(struct device_driver *ddp, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%dms\n", dgnc_poll_tick);
}

static ssize_t dgnc_driver_pollrate_store(struct device_driver *ddp, const char *buf, size_t count)
{
	sscanf(buf, "%d\n", &dgnc_poll_tick);
	return count;
}
static DRIVER_ATTR(pollrate, (S_IRUSR | S_IWUSR), dgnc_driver_pollrate_show, dgnc_driver_pollrate_store);


void dgnc_create_driver_sysfiles(struct pci_driver *dgnc_driver)
{
	int rc = 0;
	struct device_driver *driverfs = &dgnc_driver->driver;

	rc |= driver_create_file(driverfs, &driver_attr_version);
	rc |= driver_create_file(driverfs, &driver_attr_boards);
	rc |= driver_create_file(driverfs, &driver_attr_maxboards);
	rc |= driver_create_file(driverfs, &driver_attr_debug);
	rc |= driver_create_file(driverfs, &driver_attr_rawreadok);
	rc |= driver_create_file(driverfs, &driver_attr_pollrate);
	rc |= driver_create_file(driverfs, &driver_attr_pollcounter);
	rc |= driver_create_file(driverfs, &driver_attr_state);
	if (rc) {
		printk(KERN_ERR "DGNC: sysfs driver_create_file failed!\n");
	}
}


void dgnc_remove_driver_sysfiles(struct pci_driver *dgnc_driver)
{
	struct device_driver *driverfs = &dgnc_driver->driver;
	driver_remove_file(driverfs, &driver_attr_version);
	driver_remove_file(driverfs, &driver_attr_boards);
	driver_remove_file(driverfs, &driver_attr_maxboards);
	driver_remove_file(driverfs, &driver_attr_debug);
	driver_remove_file(driverfs, &driver_attr_rawreadok);
	driver_remove_file(driverfs, &driver_attr_pollrate);
	driver_remove_file(driverfs, &driver_attr_pollcounter);
	driver_remove_file(driverfs, &driver_attr_state);
}


#define DGNC_VERIFY_BOARD(p, bd)			\
	if (!p)						\
		return (0);				\
							\
	bd = dev_get_drvdata(p);			\
	if (!bd || bd->magic != DGNC_BOARD_MAGIC)	\
		return (0);				\
	if (bd->state != BOARD_READY)			\
		return (0);				\



static ssize_t dgnc_vpd_show(struct device *p, struct device_attribute *attr, char *buf)
{
	struct board_t *bd;
	int count = 0;
	int i = 0;

	DGNC_VERIFY_BOARD(p, bd);

	count += sprintf(buf + count, "\n      0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F");
	for (i = 0; i < 0x40 * 2; i++) {
		if (!(i % 16))
			count += sprintf(buf + count, "\n%04X ", i * 2);
		count += sprintf(buf + count, "%02X ", bd->vpd[i]);
	}
	count += sprintf(buf + count, "\n");

	return count;
}
static DEVICE_ATTR(vpd, S_IRUSR, dgnc_vpd_show, NULL);

static ssize_t dgnc_serial_number_show(struct device *p, struct device_attribute *attr, char *buf)
{
	struct board_t *bd;
	int count = 0;

	DGNC_VERIFY_BOARD(p, bd);

	if (bd->serial_num[0] == '\0')
		count += sprintf(buf + count, "<UNKNOWN>\n");
	else
		count += sprintf(buf + count, "%s\n", bd->serial_num);

	return count;
}
static DEVICE_ATTR(serial_number, S_IRUSR, dgnc_serial_number_show, NULL);


static ssize_t dgnc_ports_state_show(struct device *p, struct device_attribute *attr, char *buf)
{
	struct board_t *bd;
	int count = 0;
	int i = 0;

	DGNC_VERIFY_BOARD(p, bd);

	for (i = 0; i < bd->nasync; i++) {
		count += snprintf(buf + count, PAGE_SIZE - count,
			"%d %s\n", bd->channels[i]->ch_portnum,
			bd->channels[i]->ch_open_count ? "Open" : "Closed");
	}
	return count;
}
static DEVICE_ATTR(ports_state, S_IRUSR, dgnc_ports_state_show, NULL);


static ssize_t dgnc_ports_baud_show(struct device *p, struct device_attribute *attr, char *buf)
{
	struct board_t *bd;
	int count = 0;
	int i = 0;

	DGNC_VERIFY_BOARD(p, bd);

	for (i = 0; i < bd->nasync; i++) {
		count +=  snprintf(buf + count, PAGE_SIZE - count,
			"%d %d\n", bd->channels[i]->ch_portnum, bd->channels[i]->ch_old_baud);
	}
	return count;
}
static DEVICE_ATTR(ports_baud, S_IRUSR, dgnc_ports_baud_show, NULL);


static ssize_t dgnc_ports_msignals_show(struct device *p, struct device_attribute *attr, char *buf)
{
	struct board_t *bd;
	int count = 0;
	int i = 0;

	DGNC_VERIFY_BOARD(p, bd);

	for (i = 0; i < bd->nasync; i++) {
		if (bd->channels[i]->ch_open_count) {
			count += snprintf(buf + count, PAGE_SIZE - count,
				"%d %s %s %s %s %s %s\n", bd->channels[i]->ch_portnum,
				(bd->channels[i]->ch_mostat & UART_MCR_RTS) ? "RTS" : "",
				(bd->channels[i]->ch_mistat & UART_MSR_CTS) ? "CTS" : "",
				(bd->channels[i]->ch_mostat & UART_MCR_DTR) ? "DTR" : "",
				(bd->channels[i]->ch_mistat & UART_MSR_DSR) ? "DSR" : "",
				(bd->channels[i]->ch_mistat & UART_MSR_DCD) ? "DCD" : "",
				(bd->channels[i]->ch_mistat & UART_MSR_RI)  ? "RI"  : "");
		} else {
			count += snprintf(buf + count, PAGE_SIZE - count,
				"%d\n", bd->channels[i]->ch_portnum);
		}
	}
	return count;
}
static DEVICE_ATTR(ports_msignals, S_IRUSR, dgnc_ports_msignals_show, NULL);


static ssize_t dgnc_ports_iflag_show(struct device *p, struct device_attribute *attr, char *buf)
{
	struct board_t *bd;
	int count = 0;
	int i = 0;

	DGNC_VERIFY_BOARD(p, bd);

	for (i = 0; i < bd->nasync; i++) {
		count += snprintf(buf + count, PAGE_SIZE - count, "%d %x\n",
			bd->channels[i]->ch_portnum, bd->channels[i]->ch_c_iflag);
	}
	return count;
}
static DEVICE_ATTR(ports_iflag, S_IRUSR, dgnc_ports_iflag_show, NULL);


static ssize_t dgnc_ports_cflag_show(struct device *p, struct device_attribute *attr, char *buf)
{
	struct board_t *bd;
	int count = 0;
	int i = 0;

	DGNC_VERIFY_BOARD(p, bd);

	for (i = 0; i < bd->nasync; i++) {
		count += snprintf(buf + count, PAGE_SIZE - count, "%d %x\n",
			bd->channels[i]->ch_portnum, bd->channels[i]->ch_c_cflag);
	}
	return count;
}
static DEVICE_ATTR(ports_cflag, S_IRUSR, dgnc_ports_cflag_show, NULL);


static ssize_t dgnc_ports_oflag_show(struct device *p, struct device_attribute *attr, char *buf)
{
	struct board_t *bd;
	int count = 0;
	int i = 0;

	DGNC_VERIFY_BOARD(p, bd);

	for (i = 0; i < bd->nasync; i++) {
		count += snprintf(buf + count, PAGE_SIZE - count, "%d %x\n",
			bd->channels[i]->ch_portnum, bd->channels[i]->ch_c_oflag);
	}
	return count;
}
static DEVICE_ATTR(ports_oflag, S_IRUSR, dgnc_ports_oflag_show, NULL);


static ssize_t dgnc_ports_lflag_show(struct device *p, struct device_attribute *attr, char *buf)
{
	struct board_t *bd;
	int count = 0;
	int i = 0;

	DGNC_VERIFY_BOARD(p, bd);

	for (i = 0; i < bd->nasync; i++) {
		count += snprintf(buf + count, PAGE_SIZE - count, "%d %x\n",
			bd->channels[i]->ch_portnum, bd->channels[i]->ch_c_lflag);
	}
	return count;
}
static DEVICE_ATTR(ports_lflag, S_IRUSR, dgnc_ports_lflag_show, NULL);


static ssize_t dgnc_ports_digi_flag_show(struct device *p, struct device_attribute *attr, char *buf)
{
	struct board_t *bd;
	int count = 0;
	int i = 0;

	DGNC_VERIFY_BOARD(p, bd);

	for (i = 0; i < bd->nasync; i++) {
		count += snprintf(buf + count, PAGE_SIZE - count, "%d %x\n",
			bd->channels[i]->ch_portnum, bd->channels[i]->ch_digi.digi_flags);
	}
	return count;
}
static DEVICE_ATTR(ports_digi_flag, S_IRUSR, dgnc_ports_digi_flag_show, NULL);


static ssize_t dgnc_ports_rxcount_show(struct device *p, struct device_attribute *attr, char *buf)
{
	struct board_t *bd;
	int count = 0;
	int i = 0;

	DGNC_VERIFY_BOARD(p, bd);

	for (i = 0; i < bd->nasync; i++) {
		count += snprintf(buf + count, PAGE_SIZE - count, "%d %ld\n",
			bd->channels[i]->ch_portnum, bd->channels[i]->ch_rxcount);
	}
	return count;
}
static DEVICE_ATTR(ports_rxcount, S_IRUSR, dgnc_ports_rxcount_show, NULL);


static ssize_t dgnc_ports_txcount_show(struct device *p, struct device_attribute *attr, char *buf)
{
	struct board_t *bd;
	int count = 0;
	int i = 0;

	DGNC_VERIFY_BOARD(p, bd);

	for (i = 0; i < bd->nasync; i++) {
		count += snprintf(buf + count, PAGE_SIZE - count, "%d %ld\n",
			bd->channels[i]->ch_portnum, bd->channels[i]->ch_txcount);
	}
	return count;
}
static DEVICE_ATTR(ports_txcount, S_IRUSR, dgnc_ports_txcount_show, NULL);


/* this function creates the sys files that will export each signal status
 * to sysfs each value will be put in a separate filename
 */
void dgnc_create_ports_sysfiles(struct board_t *bd)
{
	int rc = 0;

	dev_set_drvdata(&bd->pdev->dev, bd);
	rc |= device_create_file(&(bd->pdev->dev), &dev_attr_ports_state);
	rc |= device_create_file(&(bd->pdev->dev), &dev_attr_ports_baud);
	rc |= device_create_file(&(bd->pdev->dev), &dev_attr_ports_msignals);
	rc |= device_create_file(&(bd->pdev->dev), &dev_attr_ports_iflag);
	rc |= device_create_file(&(bd->pdev->dev), &dev_attr_ports_cflag);
	rc |= device_create_file(&(bd->pdev->dev), &dev_attr_ports_oflag);
	rc |= device_create_file(&(bd->pdev->dev), &dev_attr_ports_lflag);
	rc |= device_create_file(&(bd->pdev->dev), &dev_attr_ports_digi_flag);
	rc |= device_create_file(&(bd->pdev->dev), &dev_attr_ports_rxcount);
	rc |= device_create_file(&(bd->pdev->dev), &dev_attr_ports_txcount);
	rc |= device_create_file(&(bd->pdev->dev), &dev_attr_vpd);
	rc |= device_create_file(&(bd->pdev->dev), &dev_attr_serial_number);
	if (rc) {
		printk(KERN_ERR "DGNC: sysfs device_create_file failed!\n");
	}
}


/* removes all the sys files created for that port */
void dgnc_remove_ports_sysfiles(struct board_t *bd)
{
	device_remove_file(&(bd->pdev->dev), &dev_attr_ports_state);
	device_remove_file(&(bd->pdev->dev), &dev_attr_ports_baud);
	device_remove_file(&(bd->pdev->dev), &dev_attr_ports_msignals);
	device_remove_file(&(bd->pdev->dev), &dev_attr_ports_iflag);
	device_remove_file(&(bd->pdev->dev), &dev_attr_ports_cflag);
	device_remove_file(&(bd->pdev->dev), &dev_attr_ports_oflag);
	device_remove_file(&(bd->pdev->dev), &dev_attr_ports_lflag);
	device_remove_file(&(bd->pdev->dev), &dev_attr_ports_digi_flag);
	device_remove_file(&(bd->pdev->dev), &dev_attr_ports_rxcount);
	device_remove_file(&(bd->pdev->dev), &dev_attr_ports_txcount);
	device_remove_file(&(bd->pdev->dev), &dev_attr_vpd);
	device_remove_file(&(bd->pdev->dev), &dev_attr_serial_number);
}


static ssize_t dgnc_tty_state_show(struct device *d, struct device_attribute *attr, char *buf)
{
	struct board_t *bd;
	struct channel_t *ch;
	struct un_t *un;

	if (!d)
		return (0);
	un = (struct un_t *) dev_get_drvdata(d);
	if (!un || un->magic != DGNC_UNIT_MAGIC)
		return (0);
	ch = un->un_ch;
	if (!ch || ch->magic != DGNC_CHANNEL_MAGIC)
		return (0);
	bd = ch->ch_bd;
	if (!bd || bd->magic != DGNC_BOARD_MAGIC)
		return (0);
	if (bd->state != BOARD_READY)
		return (0);

	return snprintf(buf, PAGE_SIZE, "%s", un->un_open_count ? "Open" : "Closed");
}
static DEVICE_ATTR(state, S_IRUSR, dgnc_tty_state_show, NULL);


static ssize_t dgnc_tty_baud_show(struct device *d, struct device_attribute *attr, char *buf)
{
	struct board_t *bd;
	struct channel_t *ch;
	struct un_t *un;

	if (!d)
		return (0);
	un = (struct un_t *) dev_get_drvdata(d);
	if (!un || un->magic != DGNC_UNIT_MAGIC)
		return (0);
	ch = un->un_ch;
	if (!ch || ch->magic != DGNC_CHANNEL_MAGIC)
		return (0);
	bd = ch->ch_bd;
	if (!bd || bd->magic != DGNC_BOARD_MAGIC)
		return (0);
	if (bd->state != BOARD_READY)
		return (0);

	return snprintf(buf, PAGE_SIZE, "%d\n", ch->ch_old_baud);
}
static DEVICE_ATTR(baud, S_IRUSR, dgnc_tty_baud_show, NULL);


static ssize_t dgnc_tty_msignals_show(struct device *d, struct device_attribute *attr, char *buf)
{
	struct board_t *bd;
	struct channel_t *ch;
	struct un_t *un;

	if (!d)
		return (0);
	un = (struct un_t *) dev_get_drvdata(d);
	if (!un || un->magic != DGNC_UNIT_MAGIC)
		return (0);
	ch = un->un_ch;
	if (!ch || ch->magic != DGNC_CHANNEL_MAGIC)
		return (0);
	bd = ch->ch_bd;
	if (!bd || bd->magic != DGNC_BOARD_MAGIC)
		return (0);
	if (bd->state != BOARD_READY)
		return (0);

	if (ch->ch_open_count) {
		return snprintf(buf, PAGE_SIZE, "%s %s %s %s %s %s\n",
			(ch->ch_mostat & UART_MCR_RTS) ? "RTS" : "",
			(ch->ch_mistat & UART_MSR_CTS) ? "CTS" : "",
			(ch->ch_mostat & UART_MCR_DTR) ? "DTR" : "",
			(ch->ch_mistat & UART_MSR_DSR) ? "DSR" : "",
			(ch->ch_mistat & UART_MSR_DCD) ? "DCD" : "",
			(ch->ch_mistat & UART_MSR_RI)  ? "RI"  : "");
	}
	return 0;
}
static DEVICE_ATTR(msignals, S_IRUSR, dgnc_tty_msignals_show, NULL);


static ssize_t dgnc_tty_iflag_show(struct device *d, struct device_attribute *attr, char *buf)
{
	struct board_t *bd;
	struct channel_t *ch;
	struct un_t *un;

	if (!d)
		return (0);
	un = (struct un_t *) dev_get_drvdata(d);
	if (!un || un->magic != DGNC_UNIT_MAGIC)
		return (0);
	ch = un->un_ch;
	if (!ch || ch->magic != DGNC_CHANNEL_MAGIC)
		return (0);
	bd = ch->ch_bd;
	if (!bd || bd->magic != DGNC_BOARD_MAGIC)
		return (0);
	if (bd->state != BOARD_READY)
		return (0);

	return snprintf(buf, PAGE_SIZE, "%x\n", ch->ch_c_iflag);
}
static DEVICE_ATTR(iflag, S_IRUSR, dgnc_tty_iflag_show, NULL);


static ssize_t dgnc_tty_cflag_show(struct device *d, struct device_attribute *attr, char *buf)
{
	struct board_t *bd;
	struct channel_t *ch;
	struct un_t *un;

	if (!d)
		return (0);
	un = (struct un_t *) dev_get_drvdata(d);
	if (!un || un->magic != DGNC_UNIT_MAGIC)
		return (0);
	ch = un->un_ch;
	if (!ch || ch->magic != DGNC_CHANNEL_MAGIC)
		return (0);
	bd = ch->ch_bd;
	if (!bd || bd->magic != DGNC_BOARD_MAGIC)
		return (0);
	if (bd->state != BOARD_READY)
		return (0);

	return snprintf(buf, PAGE_SIZE, "%x\n", ch->ch_c_cflag);
}
static DEVICE_ATTR(cflag, S_IRUSR, dgnc_tty_cflag_show, NULL);


static ssize_t dgnc_tty_oflag_show(struct device *d, struct device_attribute *attr, char *buf)
{
	struct board_t *bd;
	struct channel_t *ch;
	struct un_t *un;

	if (!d)
		return (0);
	un = (struct un_t *) dev_get_drvdata(d);
	if (!un || un->magic != DGNC_UNIT_MAGIC)
		return (0);
	ch = un->un_ch;
	if (!ch || ch->magic != DGNC_CHANNEL_MAGIC)
		return (0);
	bd = ch->ch_bd;
	if (!bd || bd->magic != DGNC_BOARD_MAGIC)
		return (0);
	if (bd->state != BOARD_READY)
		return (0);

	return snprintf(buf, PAGE_SIZE, "%x\n", ch->ch_c_oflag);
}
static DEVICE_ATTR(oflag, S_IRUSR, dgnc_tty_oflag_show, NULL);


static ssize_t dgnc_tty_lflag_show(struct device *d, struct device_attribute *attr, char *buf)
{
	struct board_t *bd;
	struct channel_t *ch;
	struct un_t *un;

	if (!d)
		return (0);
	un = (struct un_t *) dev_get_drvdata(d);
	if (!un || un->magic != DGNC_UNIT_MAGIC)
		return (0);
	ch = un->un_ch;
	if (!ch || ch->magic != DGNC_CHANNEL_MAGIC)
		return (0);
	bd = ch->ch_bd;
	if (!bd || bd->magic != DGNC_BOARD_MAGIC)
		return (0);
	if (bd->state != BOARD_READY)
		return (0);

	return snprintf(buf, PAGE_SIZE, "%x\n", ch->ch_c_lflag);
}
static DEVICE_ATTR(lflag, S_IRUSR, dgnc_tty_lflag_show, NULL);


static ssize_t dgnc_tty_digi_flag_show(struct device *d, struct device_attribute *attr, char *buf)
{
	struct board_t *bd;
	struct channel_t *ch;
	struct un_t *un;

	if (!d)
		return (0);
	un = (struct un_t *) dev_get_drvdata(d);
	if (!un || un->magic != DGNC_UNIT_MAGIC)
		return (0);
	ch = un->un_ch;
	if (!ch || ch->magic != DGNC_CHANNEL_MAGIC)
		return (0);
	bd = ch->ch_bd;
	if (!bd || bd->magic != DGNC_BOARD_MAGIC)
		return (0);
	if (bd->state != BOARD_READY)
		return (0);

	return snprintf(buf, PAGE_SIZE, "%x\n", ch->ch_digi.digi_flags);
}
static DEVICE_ATTR(digi_flag, S_IRUSR, dgnc_tty_digi_flag_show, NULL);


static ssize_t dgnc_tty_rxcount_show(struct device *d, struct device_attribute *attr, char *buf)
{
	struct board_t *bd;
	struct channel_t *ch;
	struct un_t *un;

	if (!d)
		return (0);
	un = (struct un_t *) dev_get_drvdata(d);
	if (!un || un->magic != DGNC_UNIT_MAGIC)
		return (0);
	ch = un->un_ch;
	if (!ch || ch->magic != DGNC_CHANNEL_MAGIC)
		return (0);
	bd = ch->ch_bd;
	if (!bd || bd->magic != DGNC_BOARD_MAGIC)
		return (0);
	if (bd->state != BOARD_READY)
		return (0);

	return snprintf(buf, PAGE_SIZE, "%ld\n", ch->ch_rxcount);
}
static DEVICE_ATTR(rxcount, S_IRUSR, dgnc_tty_rxcount_show, NULL);


static ssize_t dgnc_tty_txcount_show(struct device *d, struct device_attribute *attr, char *buf)
{
	struct board_t *bd;
	struct channel_t *ch;
	struct un_t *un;

	if (!d)
		return (0);
	un = (struct un_t *) dev_get_drvdata(d);
	if (!un || un->magic != DGNC_UNIT_MAGIC)
		return (0);
	ch = un->un_ch;
	if (!ch || ch->magic != DGNC_CHANNEL_MAGIC)
		return (0);
	bd = ch->ch_bd;
	if (!bd || bd->magic != DGNC_BOARD_MAGIC)
		return (0);
	if (bd->state != BOARD_READY)
		return (0);

	return snprintf(buf, PAGE_SIZE, "%ld\n", ch->ch_txcount);
}
static DEVICE_ATTR(txcount, S_IRUSR, dgnc_tty_txcount_show, NULL);


static ssize_t dgnc_tty_name_show(struct device *d, struct device_attribute *attr, char *buf)
{
	struct board_t *bd;
	struct channel_t *ch;
	struct un_t *un;

	if (!d)
		return (0);
	un = (struct un_t *) dev_get_drvdata(d);
	if (!un || un->magic != DGNC_UNIT_MAGIC)
		return (0);
	ch = un->un_ch;
	if (!ch || ch->magic != DGNC_CHANNEL_MAGIC)
		return (0);
	bd = ch->ch_bd;
	if (!bd || bd->magic != DGNC_BOARD_MAGIC)
		return (0);
	if (bd->state != BOARD_READY)
		return (0);

	return snprintf(buf, PAGE_SIZE, "%sn%d%c\n",
		(un->un_type == DGNC_PRINT) ? "pr" : "tty",
		bd->boardnum + 1, 'a' + ch->ch_portnum);
}
static DEVICE_ATTR(custom_name, S_IRUSR, dgnc_tty_name_show, NULL);


static struct attribute *dgnc_sysfs_tty_entries[] = {
	&dev_attr_state.attr,
	&dev_attr_baud.attr,
	&dev_attr_msignals.attr,
	&dev_attr_iflag.attr,
	&dev_attr_cflag.attr,
	&dev_attr_oflag.attr,
	&dev_attr_lflag.attr,
	&dev_attr_digi_flag.attr,
	&dev_attr_rxcount.attr,
	&dev_attr_txcount.attr,
	&dev_attr_custom_name.attr,
	NULL
};


static struct attribute_group dgnc_tty_attribute_group = {
	.name = NULL,
	.attrs = dgnc_sysfs_tty_entries,
};


void dgnc_create_tty_sysfs(struct un_t *un, struct device *c)
{
	int ret;

	ret = sysfs_create_group(&c->kobj, &dgnc_tty_attribute_group);
	if (ret) {
		printk(KERN_ERR "dgnc: failed to create sysfs tty device attributes.\n");
		sysfs_remove_group(&c->kobj, &dgnc_tty_attribute_group);
		return;
	}

	dev_set_drvdata(c, un);

}


void dgnc_remove_tty_sysfs(struct device *c)
{
	sysfs_remove_group(&c->kobj, &dgnc_tty_attribute_group);
}

