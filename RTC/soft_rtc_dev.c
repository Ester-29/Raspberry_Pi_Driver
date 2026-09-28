// SPDX-License-Identifier: GPL-2.0
/*
 * soft_rtc_dev.c - CHI dung tren may ao x86 (khong co Device Tree).
 *
 * Tao mot platform_device ten "soft-rtc". No dong vai tro ma node Device Tree
 * dam nhan tren Raspberry Pi: platform bus thay ten device trung voi
 * driver.name cua soft_rtc.ko -> goi probe() cua driver.
 */
#include <linux/module.h>
#include <linux/platform_device.h>

static struct platform_device *pdev;

static int __init soft_rtc_dev_init(void)
{
	/*
	 * PLATFORM_DEVID_NONE (-1): ten device la "soft-rtc", khong co hau to ".0".
	 * Khong truyen resource nao -> driver khong thay "reg" -> chay SW mode.
	 */
	pdev = platform_device_register_simple("soft-rtc", PLATFORM_DEVID_NONE,
					       NULL, 0);
	if (IS_ERR(pdev))
		return PTR_ERR(pdev);

	pr_info("soft_rtc_dev: platform_device '%s' registered\n",
		dev_name(&pdev->dev));
	return 0;
}

static void __exit soft_rtc_dev_exit(void)
{
	/* Go device -> neu driver dang gan vao, remove() cua driver duoc goi */
	platform_device_unregister(pdev);
	pr_info("soft_rtc_dev: platform_device removed\n");
}

module_init(soft_rtc_dev_init);
module_exit(soft_rtc_dev_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Creates a 'soft-rtc' platform_device (stand-in for Device Tree on x86)");
