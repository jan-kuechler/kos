/*
 * Copyright (c) 2007-2009 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Antoine Kaufmann.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *     This product includes software developed by the tyndur Project
 *     and its contributors.
 * 4. Neither the name of the tyndur Project nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "cdi/storage.h"
#include "cdi/lists.h"
#include "cdi/pci.h"

#include "device.h"

static struct cdi_storage_driver driver_storage;
static struct cdi_scsi_driver driver_scsi;
static const char* driver_storage_name = "ata";
static const char* driver_scsi_name = "atapi";
static cdi_list_t controller_list = NULL;

static void ata_driver_init(int argc, char* argv[]);
static void ata_driver_destroy(struct cdi_driver* driver);
static void atapi_driver_destroy(struct cdi_driver* driver);

#ifdef CDI_STANDALONE
int main(int argc, char* argv[])
#else
int init_ata(int argc, char* argv[])
#endif
{
    cdi_init();
    ata_driver_init(argc, argv);
    cdi_storage_driver_register((struct cdi_storage_driver*) &driver_storage);
    cdi_scsi_driver_register((struct cdi_scsi_driver*) &driver_scsi);

#ifdef CDI_STANDALONE
    cdi_run_drivers();
#endif

    return 0;
}

/**
 * Initialisiert die Datenstrukturen fuer den sis900-Treiber
 */
static void ata_driver_init(int argc, char* argv[])
{
    struct ata_controller* controller;
    uint16_t busmaster_regbase = 0;
    struct cdi_pci_device* pci_dev;
    cdi_list_t pci_devices;
    int i;
    int j;

    // Konstruktor der Vaterklasse
    cdi_storage_driver_init((struct cdi_storage_driver*) &driver_storage);
    cdi_scsi_driver_init((struct cdi_scsi_driver*) &driver_scsi);

    // Namen setzen
    driver_storage.drv.name = driver_storage_name;
    driver_scsi.drv.name = driver_scsi_name;

    // Funktionspointer initialisieren
    driver_storage.drv.destroy          = ata_driver_destroy;
    driver_storage.drv.init_device      = ata_init_device;
    driver_storage.drv.remove_device    = ata_remove_device;
    driver_storage.read_blocks          = ata_read_blocks;
    driver_storage.write_blocks         = ata_write_blocks;

    driver_scsi.drv.destroy             = atapi_driver_destroy;
    driver_scsi.drv.init_device         = atapi_init_device;
    driver_scsi.drv.remove_device       = atapi_remove_device;
    driver_scsi.request                 = atapi_request;

    // Liste mit Controllern initialisieren
    controller_list = cdi_list_create();


    // PCI-Geraet fuer Controller finden
    pci_devices = cdi_list_create();
    cdi_pci_get_all_devices(pci_devices);
    for (i = 0; (pci_dev = cdi_list_get(pci_devices, i)) && !busmaster_regbase;
        i++)
    {
        struct cdi_pci_resource* res;

        if ((pci_dev->class_id != PCI_CLASS_ATA) ||
            (pci_dev->subclass_id != PCI_SUBCLASS_ATA))
        {
            continue;
        }

        // Jetzt noch die Ressource finden mit den Busmaster-Registern
        // TODO: Das funktioniert so vermutlich nicht ueberall, da es
        // warscheinlich auch Kontroller mit nur einem Kanal gibt und solche bei
        // denen die BM-Register im Speicher gemappt sind
        for (j = 0; (res = cdi_list_get(pci_dev->resources, j)); j++) {
            if ((res->type == CDI_PCI_IOPORTS) && (res->length == 16)) {
                busmaster_regbase = res->start;
                break;
            }
        }
    }

    // Kaputte VIA-Kontroller sollten nur PIO benutzen, da es bei DMA zu
    // haengern kommt. Dabei handelt es sich um den Kontroller 82C686B
    if (pci_dev && (pci_dev->vendor_id == PCI_VENDOR_VIA) &&
        (pci_dev->device_id == 0x686))
    {
        busmaster_regbase = 0;
    } else {
        // Wenn der nodma-Parameter uebergeben wurde, deaktivieren wir dma auch
        for (i = 1; i < argc; i++) {
            if (!strcmp(argv[i], "nodma")) {
                busmaster_regbase = 0;
                break;
            }
        }
    }

    // Primaeren Controller vorbereiten
    controller = kcalloc(1, sizeof(*controller));
    controller->port_cmd_base = ATA_PRIMARY_CMD_BASE;
    controller->port_ctl_base = ATA_PRIMARY_CTL_BASE;
    controller->port_bmr_base = busmaster_regbase;
    controller->irq = ATA_PRIMARY_IRQ;
    controller->id = 0;
    controller->storage = (struct cdi_storage_driver*) &driver_storage;
    controller->scsi = (struct cdi_scsi_driver*) &driver_scsi;
    ata_init_controller(controller);
    cdi_list_push(controller_list, controller);

    // Sekundaeren Controller vorbereiten
    controller = kcalloc(1, sizeof(*controller));
    controller->port_cmd_base = ATA_SECONDARY_CMD_BASE;
    controller->port_ctl_base = ATA_SECONDARY_CTL_BASE;
    controller->port_bmr_base = busmaster_regbase;
    controller->irq = ATA_SECONDARY_IRQ;
    controller->id = 1;
    controller->storage = (struct cdi_storage_driver*) &driver_storage;
    controller->scsi = (struct cdi_scsi_driver*) &driver_scsi;
    ata_init_controller(controller);
    cdi_list_push(controller_list, controller);
}

/**
 * Deinitialisiert die Datenstrukturen fuer den ata-Treiber
 */
static void ata_driver_destroy(struct cdi_driver* driver)
{
    cdi_storage_driver_destroy((struct cdi_storage_driver*) driver);

    // TODO Alle Karten deinitialisieren
}

static void atapi_driver_destroy(struct cdi_driver* driver)
{
    cdi_scsi_driver_destroy((struct cdi_scsi_driver*) driver);
}
