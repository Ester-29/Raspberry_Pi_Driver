// SPDX-License-Identifier: GPL-2.0
/*
 * soft_rtc.c - Platform driver RTC minh hoa
 *
 * Mot driver, hai che do (tu chon trong probe()):
 *  - HW mode: device duoc tao tu Device Tree va co thuoc tinh "reg"
 *    (Raspberry Pi Zero W). Driver doc bo dem 64 bit cua System Timer
 *    trong SoC BCM2835 (1 MHz) qua MMIO va dung no lam nhip dem giay.
 *  - SW mode: device khong co "reg" (Ubuntu x86 trong VirtualBox, khong co
 *    Device Tree, device do soft_rtc_dev.ko tao). Nhip dem = ktime cua kernel.
 *
 * Luu y: day KHONG phai RTC co pin nuoi. Mat dien / reboot la mat gio.
 * User-space giao tiep qua /dev/rtcN do RTC core tao san (hwclock,
 * timedatectl, /sys/class/rtc/rtcN/...). Driver chi can cai rtc_class_ops,
 * khong phai tu viet file_operations nhu character device.
 */
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>	/* struct of_device_id */
#include <linux/property.h>		/* device_property_read_u32() */
#include <linux/rtc.h>			/* RTC core API */
#include <linux/io.h>			/* devm_ioremap(), readl() */
#include <linux/delay.h>		/* udelay() */
#include <linux/ktime.h>		/* ktime_get_*() */
#include <linux/math64.h>		/* div_u64() */
#include <linux/version.h>		/* LINUX_VERSION_CODE */

/* Thanh ghi System Timer (BCM2835 ARM Peripherals, chuong 12) */
#define ST_CLO	0x04	/* 32 bit thap cua bo dem tu do */
#define ST_CHI	0x08	/* 32 bit cao cua bo dem tu do */

/* Tham so module: bat log moi lan read_time() de chung minh lenh di qua driver */
static bool trace;
module_param(trace, bool, 0644);
MODULE_PARM_DESC(trace, "Log every read_time() call (rate-limited)");

/* Du lieu rieng cua MOT device (probe chay bao nhieu lan thi co bay nhieu ban) */
struct soft_rtc {
	struct rtc_device *rtc;
	void __iomem *base;	/* != NULL: HW mode, NULL: SW mode */
	u32 tick_hz;		/* so tick moi giay cua nguon dem */
	u64 base_ticks;		/* gia tri nguon dem tai lan dat gio gan nhat */
	time64_t base_sec;	/* gio RTC (giay tu 1970, UTC) tai base_ticks */
};

/* Doc nguon dem: HW = bo dem 64 bit cua SoC, SW = so ns tu luc boot */
static u64 soft_rtc_ticks(struct soft_rtc *priv)
{
	u32 hi, lo;

	if (!priv->base)
		return ktime_get_boottime_ns();

	/*
	 * Doc CHI -> CLO -> CHI. Neu CLO vua tran (moi ~71,6 phut) giua hai lan
	 * doc thi CHI doi gia tri -> doc lai, tranh ghep sai hai nua 32 bit.
	 */
	do {
		hi = readl(priv->base + ST_CHI);
		lo = readl(priv->base + ST_CLO);
	} while (hi != readl(priv->base + ST_CHI));

	return ((u64)hi << 32) | lo;
}

/* Duoc RTC core goi khi user doc gio: ioctl(RTC_RD_TIME), hwclock -r, sysfs... */
static int soft_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	struct soft_rtc *priv = dev_get_drvdata(dev);
	/* div_u64: chia 64 bit an toan tren ARM 32 bit ('/' se loi __aeabi_uldivmod) */
	u64 elapsed = div_u64(soft_rtc_ticks(priv) - priv->base_ticks,
			      priv->tick_hz);

	rtc_time64_to_tm(priv->base_sec + (time64_t)elapsed, tm);
	if (trace)
		dev_info_ratelimited(dev, "read_time -> %ptR UTC\n", tm);
	return 0;
}

/* Duoc RTC core goi khi user dat gio: ioctl(RTC_SET_TIME), hwclock --set / -w */
static int soft_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	struct soft_rtc *priv = dev_get_drvdata(dev);

	/* Khong can mutex rieng: RTC core da giu rtc->ops_lock khi goi ops */
	priv->base_ticks = soft_rtc_ticks(priv);
	priv->base_sec = rtc_tm_to_time64(tm);
	dev_info(dev, "set_time <- %ptR UTC\n", tm);
	return 0;
}

/* Cac ham driver cung cap cho RTC core. Khong co .set_alarm -> core tu tat alarm */
static const struct rtc_class_ops soft_rtc_ops = {
	.read_time = soft_rtc_read_time,
	.set_time  = soft_rtc_set_time,
};

/* probe(): platform bus goi khi tim thay device khop voi driver nay */
static int soft_rtc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct soft_rtc *priv;
	struct resource *res;
	int ret;

	dev_info(dev, "probe(): matched by %s\n",
		 dev->of_node ? "Device Tree (compatible)" : "name (no Device Tree)");

	/* devm_*: tai nguyen gan voi vong doi device, tu giai phong khi go device */
	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	/* Vung thanh ghi lay tu thuoc tinh "reg" cua node Device Tree */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res) {
		u64 t0;

		/*
		 * Vung System Timer dang duoc clocksource cua kernel dung chung,
		 * nen chi ioremap (khong request_mem_region) va CHI DOC CLO/CHI.
		 * Khong bao gio ghi CS/C0..C3 (GPU dung C0/C2, kernel dung C3).
		 */
		priv->base = devm_ioremap(dev, res->start, resource_size(res));
		if (!priv->base)
			return -ENOMEM;

		/* Tan so bo dem lay tu DT; thieu thi mac dinh 1 MHz */
		if (device_property_read_u32(dev, "clock-frequency", &priv->tick_hz) ||
		    !priv->tick_hz) {
			dev_warn(dev, "no clock-frequency in DT, assuming 1 MHz\n");
			priv->tick_hz = 1000000;
		}

		/* Tu kiem tra phan cung: bo dem phai chay, neu khong la "reg" sai */
		t0 = soft_rtc_ticks(priv);
		udelay(20);
		if (soft_rtc_ticks(priv) == t0) {
			dev_err(dev, "counter at %pR is not running - wrong 'reg'?\n", res);
			return -ENODEV;
		}
		dev_info(dev, "HW mode: counter @ %pR, %u Hz\n", res, priv->tick_hz);
	} else {
		priv->tick_hz = NSEC_PER_SEC;
		dev_info(dev, "SW mode: no 'reg' resource, time base = ktime\n");
	}

	/*
	 * Gio ban dau = gio he thong. Tren Pi, driver thanh rtc0 va kernel co the
	 * dat gio he thong theo rtc0 ngay luc dang ky (hctosys) -> khong bi nhay gio.
	 */
	priv->base_ticks = soft_rtc_ticks(priv);
	priv->base_sec = ktime_get_real_seconds();

	/* Phai set drvdata TRUOC khi dang ky: core co the goi read_time ngay luc do */
	platform_set_drvdata(pdev, priv);

	priv->rtc = devm_rtc_allocate_device(dev);
	if (IS_ERR(priv->rtc))
		return PTR_ERR(priv->rtc);

	priv->rtc->ops = &soft_rtc_ops;
	priv->rtc->range_min = RTC_TIMESTAMP_BEGIN_2000;	/* 2000-01-01 00:00:00 */
	priv->rtc->range_max = RTC_TIMESTAMP_END_2099;		/* 2099-12-31 23:59:59 */

	/* Dang ky voi RTC core -> sinh /dev/rtcN va /sys/class/rtc/rtcN */
	ret = devm_rtc_register_device(priv->rtc);
	if (ret)
		return ret;

	dev_info(dev, "probe() done -> /dev/%s\n", dev_name(&priv->rtc->dev));
	return 0;
}

/*
 * remove(): goi khi rmmod driver, khi device bi go (rmmod soft_rtc_dev,
 * dtoverlay -r) hoac khi unbind. Moi tai nguyen deu la devm_* nen sau khi
 * ham nay tra ve, kernel tu huy /dev/rtcN, iounmap, kfree theo thu tu nguoc.
 * Tu kernel 6.11, remove() tra ve void; kernel cu hon tra ve int.
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 11, 0)
static void soft_rtc_remove(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "remove() called\n");
}
#else
static int soft_rtc_remove(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "remove() called\n");
	return 0;
}
#endif

/* Bang so khop voi Device Tree: chuoi phai GIONG HET thuoc tinh compatible */
static const struct of_device_id soft_rtc_of_match[] = {
	{ .compatible = "demo,soft-rtc" },
	{ /* sentinel: phan tu rong danh dau het bang */ }
};
MODULE_DEVICE_TABLE(of, soft_rtc_of_match);	/* sinh alias de udev tu nap module */

static struct platform_driver soft_rtc_driver = {
	.probe  = soft_rtc_probe,
	.remove = soft_rtc_remove,
	.driver = {
		.name           = "soft-rtc",	/* dung de match theo ten khi khong co DT */
		.of_match_table = soft_rtc_of_match,
	},
};

/* Macro sinh module_init/module_exit goi platform_driver_register/unregister */
module_platform_driver(soft_rtc_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Demo RTC platform driver (BCM2835 system timer / ktime)");
