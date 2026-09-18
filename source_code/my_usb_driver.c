#include <linux/module.h>
#include <linux/usb.h>
#define VENDOR_ID 0x14cd
#define PRODUCT_ID 0x8125
static const struct usb_device_id my_usb_table [] ={
{ USB_DEVICE (VENDOR_ID, PRODUCT_ID) },
{}
};
MODULE_DEVICE_TABLE (usb, my_usb_table);

static int my_usb_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
struct usb_host_interface *iface_desc;
struct usb_endpoint_descriptor *endpoint;
int i;

iface_desc = interface -> cur_altsetting;
pr_info("usb_driver:Da ket noi thiet bi. So Endpoint: %d\n", iface_desc ->desc.bNumEndpoints);
for (i=0; i<iface_desc->desc.bNumEndpoints;i++) {
endpoint = &iface_desc->endpoint[i].desc;
 if (usb_endpoint_is_bulk_in(endpoint))
 pr_info("usb_driver:Bulk IN endpoint: 0x%02X\n",endpoint->bEndpointAddress);
 if (usb_endpoint_is_bulk_out(endpoint))
 pr_info("usb_driver: Bulk OUT endpoint: 0x%02X\n",endpoint->bEndpointAddress);
}
return 0;
}
static void my_usb_disconnect(struct usb_interface *interface)
{
pr_info("usb_driver: Thiet bi da bi ngat ket noi.\n");
}
static struct usb_driver my_usb_driver = {
 .name ="my_custom_usb_driver",
 .id_table = my_usb_table,
 .probe = my_usb_probe,
 .disconnect = my_usb_disconnect,
};
module_usb_driver (my_usb_driver);
MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("Pham Hoang");
MODULE_DESCRIPTION ("USB Driver Project");

