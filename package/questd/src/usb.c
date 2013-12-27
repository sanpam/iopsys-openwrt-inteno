/*
 * usb -- collects usb info for questd
 *
 * Copyright (C) 2012-2013 Inteno Broadband Technology AB. All rights reserved.
 *
 * Author: sukru.senli@inteno.se
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#include "questd.h"

static void
replace_space(char *buf)
{
	int i = 0;
	while (buf[i]) {
		if (buf[i] == ' ')
			buf[i] = '_';
		i++;
	}
}

static void
get_usb_infos(unsigned char **val, char *usbno, char *info) {
	FILE *in;
	char cmnd[64];
	char result[32];

	*val = "";

	sprintf(cmnd, "/sys/bus/usb/devices/%s/%s", usbno, info);
	if ((in = fopen(cmnd, "r"))) {
		fgets(result, sizeof(result), in);
		remove_newline(result);
		fclose(in);
		*val = strdup(result);
	}	
}

static void
get_usb_device(unsigned long *val, char *mount) {
	FILE *mounts;
	char line[128];
	char dev[16];
	char mnt[64];

	*val = NULL;

	if (mounts = fopen("/var/usbmounts", "r")) {
		while(fgets(line, sizeof(line), mounts) != NULL)
		{
			remove_newline(line);
			if (sscanf(line, "/dev/%s /mnt/%s", dev, mnt) == 2) {
				if (!strcmp(mnt, mount)) {
					*val = strdup(dev);
					break;
				}
			}
		}
		fclose(mounts);
	}
}

static void
get_usb_size(unsigned long *val, char *device) {
	FILE *in;
	char cmnd[64];
	char result[32];
	
	*val = 0;

	sprintf(cmnd, "/sys/class/block/%s/size", device);
	if ((in = fopen(cmnd, "r"))) {
		fgets(result, sizeof(result), in);
		remove_newline(result);
		fclose(in);
		*val = atoi(result);
	}
}

void
dump_usb_info(USB *usb, char *usbno)
{
	FILE *in;
	char cmnd[64];
	char result[32];

	sprintf(cmnd, "/sys/bus/usb/devices/%s/product", usbno);
	if ((in = fopen(cmnd, "r"))) {
		fgets(result, sizeof(result), in);
		remove_newline(result);
		fclose(in);

		strcpy(&usb->product, result);
		sprintf(&usb->no, "%s", usbno);
		sprintf(&usb->name, "USB%s", strndup(usbno+2, strlen(usbno)));
		get_usb_infos(&usb->vendor, usb->no, "manufacturer");
		get_usb_infos(&usb->serial, usb->no, "serial");
		//get_usb_infos(&usb->speed, usb->no, "speed");
		get_usb_infos(&usb->maxchild, usb->no, "maxchild");
		sprintf(&usb->mount, "%s%s", usb->vendor, usb->serial);
		replace_space(usb->mount);
		get_usb_device(&usb->device, usb->mount);
		//get_usb_size(&usb->size, usb->device);
	}
}