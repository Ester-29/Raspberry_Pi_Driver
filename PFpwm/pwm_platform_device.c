#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Embedded System Group");
MODULE_DESCRIPTION("Mock Platform Device for VirtualBox Testing");

/* Giả lập tài nguyên vùng nhớ reg từ DTS */
static struct resource custom_pwm_resources[] = {
    [0] = {
        .start = 0x2020c000,
        .end   = 0x2020c000 + 0x28 - 1,
        .flags = IORESOURCE_MEM,
    },
};

static void custom_pwm_dev_release(struct device *dev)
{
    pr_info("[Platform Device] Released!\n");
}

static struct platform_device custom_pwm_device = {
    .name = "bcm2835-custom-pwm", /* Tên này BẮT BUỘC phải trùng với .name trong custom_pwm_driver */
    .id = -1,
    .num_resources = ARRAY_SIZE(custom_pwm_resources),
    .resource = custom_pwm_resources,
    .dev = {
        .release = custom_pwm_dev_release,
    },
};

static int __init custom_pwm_dev_init(void)
{
    pr_info("[Platform Device] Registering device...\n");
    return platform_device_register(&custom_pwm_device);
}

static void __exit custom_pwm_dev_exit(void)
{
    pr_info("[Platform Device] Unregistering device...\n");
    platform_device_unregister(&custom_pwm_device);
}

module_init(custom_pwm_dev_init);
module_exit(custom_pwm_dev_exit);