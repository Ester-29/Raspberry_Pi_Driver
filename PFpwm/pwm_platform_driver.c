#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/device.h>
#include <linux/sysfs.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Embedded System Group");
MODULE_DESCRIPTION("Platform Driver with Device Tree Matching for PWM");
MODULE_VERSION("1.0");

/* Offset thanh ghi PWM BCM2835 */
#define PWM_CTL  0x00
#define PWM_RNG1 0x10
#define PWM_DAT1 0x14

/* Structure lưu trữ Context của Driver */
struct custom_pwm_dev {
    void __iomem *base_addr; /* Địa chỉ ảo sau khi ioremap */
    u32 duty_cycle;
    u32 period;
};

/* --- 1. GIAO DIỆN SYSFS TƯƠNG TÁC TỪ USERSPACE --- */
static ssize_t duty_cycle_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct custom_pwm_dev *pwm = dev_get_drvdata(dev);
    return sprintf(buf, "%u\n", pwm->duty_cycle);
}

static ssize_t duty_cycle_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct custom_pwm_dev *pwm = dev_get_drvdata(dev);
    u32 val;

    if (kstrtou32(buf, 10, &val) == 0) {
        pwm->duty_cycle = val;
        if (pwm->base_addr) {
            iowrite32(val, pwm->base_addr + PWM_DAT1); /* Ghi trực tiếp vào thanh ghi phần cứng */
        }
        dev_info(dev, "[Platform Driver] Updated Duty Cycle = %u\n", val);
    }
    return count;
}

static DEVICE_ATTR_RW(duty_cycle);

/* --- 2. BẢNG SO KHỚP DEVICE TREE (MATCHING TABLE) --- */
static const struct of_device_id custom_pwm_of_match[] = {
    { .compatible = "bcm2835,custom-pwm", }, /* TRÙNG KHỚP VỚI FILE .DTS */
    { /* Sentinel - Đánh dấu kết thúc mảng */ }
};
MODULE_DEVICE_TABLE(of, custom_pwm_of_match);

/* --- 3. HÀM PROBE: KÍCH HOẠT KHI MATCH THÀNH CÔNG --- */
static int custom_pwm_probe(struct platform_device *pdev)
{
    struct custom_pwm_dev *pwm;
    struct resource *res;
    int ret;

    dev_info(&pdev->dev, "===================================\n");
    dev_info(&pdev->dev, "[Platform Driver] PROBE STARTED! Matched with Device Tree!\n");

    /* Cấp phát bộ nhớ quản lý Device Context */
    pwm = devm_kzalloc(&pdev->dev, sizeof(*pwm), GFP_KERNEL);
    if (!pwm)
        return -ENOMEM;

    platform_set_drvdata(pdev, pwm);

    /* Trích xuất tài nguyên bộ nhớ 'reg' từ Device Tree */
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res) {
        dev_err(&pdev->dev, "Loi: Khong lay duoc tai nguyen IORESOURCE_MEM tu DTS\n");
        return -ENODEV;
    }
    dev_info(&pdev->dev, "[DTS Resource] Physical Base = 0x%pa, Size = %pr\n", &res->start, res);

    /* Ánh xạ địa chỉ vật lý sang địa chỉ ảo (ioremap) */
    pwm->base_addr = devm_ioremap_resource(&pdev->dev, res);
    if (IS_ERR(pwm->base_addr)) {
        dev_warn(&pdev->dev, "Loi ioremap (VirtualBox Mock Mode) -> Tiep tuc o che do gia lap\n");
        pwm->base_addr = NULL;
    } else {
        dev_info(&pdev->dev, "[ioremap Success] Virtual Base Address = %p\n", pwm->base_addr);
    }

    pwm->period = 1024;
    pwm->duty_cycle = 512;

    /* Tạo file giao tiếp sysfs trong /sys/bus/platform/devices/.../ */
    ret = device_create_file(&pdev->dev, &dev_attr_duty_cycle);
    if (ret) {
        dev_err(&pdev->dev, "Khong the tao file sysfs 'duty_cycle'\n");
        return ret;
    }

    dev_info(&pdev->dev, "[Platform Driver] PROBE COMPLETED SUCCESSFULLY!\n");
    dev_info(&pdev->dev, "===================================\n");
    return 0;
}

/* --- 4. HÀM REMOVE: GỠ BỎ DRIVER --- */
static void custom_pwm_remove(struct platform_device *pdev)
{
    dev_info(&pdev->dev, "[Platform Driver] REMOVE CALLED!\n");
    device_remove_file(&pdev->dev, &dev_attr_duty_cycle);
}

/* --- 5. CẤU TRÚC PLATFORM DRIVER --- */
static struct platform_driver custom_pwm_driver = {
    .probe = custom_pwm_probe,
    .remove = custom_pwm_remove,
    .driver = {
        .name = "bcm2835-custom-pwm",
        .of_match_table = custom_pwm_of_match,
    },
};

module_platform_driver(custom_pwm_driver);