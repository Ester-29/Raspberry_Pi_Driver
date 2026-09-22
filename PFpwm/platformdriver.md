# GIẢI THÍCH CHI TIẾT DRIVER PWM PLATFORM LINUX & DEVICE TREE

---

## GIẢI THÍCH CHI TIẾT CÁC KHỐI LỆNH TRONG

Dưới đây là phân tích chi tiết mục đích, vai trò kỹ thuật của từng khối lệnh trong mã nguồn Platform Driver PWM, cùng hệ quả xảy ra nếu thiếu khối lệnh đó:

### 1. Khai báo các tệp thư viện Kernel (Header Files)
```c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/device.h>
#include <linux/sysfs.h>
```
* **Mục đích**: Nạp các định nghĩa kiểu dữ liệu, cấu trúc hệ thống và hàm API của Linux Kernel (như `struct platform_driver`, `struct resource`, `devm_ioremap_resource`, `iowrite32`, `device_create_file`).
* **Khác biệt & Ảnh hưởng nếu thiếu**: Trình biên dịch C (`gcc` / `make`) sẽ báo lỗi cú pháp ngay lập tức (`unknown type name`, `undeclared identifier`) do không hiểu các kiểu dữ liệu và API đặc thụ của Kernel Space.

---

### 2. Cấu trúc Quản lý Ngữ cảnh Thiết bị (Device Context Structure)
```c
struct custom_pwm_dev {
    void __iomem *base_addr; /* Địa chỉ ảo sau khi ioremap */
    u32 duty_cycle;
    u32 period;
};
```
* **Mục đích**: Đóng gói toàn bộ biến trạng thái của thiết bị (con trỏ địa chỉ bộ nhớ ảo `base_addr`, chu kỳ `period`, độ rộng xung `duty_cycle`) vào một cấu trúc dữ liệu duy nhất.
* **Khác biệt & Ảnh hưởng nếu thiếu**: Không bị lỗi biên dịch nếu dùng biến toàn cục, nhưng việc dùng biến toàn cục là **vi phạm nguyên tắc thiết kế Kernel Driver**. Nếu không có struct này, Driver sẽ rất khó quản lý dữ liệu riêng cho từng Instance khi hệ thống có nhiều khối PWM chạy đồng thời.

---

### 3. Giao diện Giao tiếp Userspace qua Sysfs (`sysfs` Interface)
```c
static ssize_t duty_cycle_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t duty_cycle_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static DEVICE_ATTR_RW(duty_cycle);
```
* **Mục đích**: Bộc lộ thuộc tính `duty_cycle` thành một file văn bản trong hệ thống tệp ảo `/sys/bus/platform/devices/.../duty_cycle`. Cho phép ứng dụng ở Userspace đọc (hàm `show`) và ghi (hàm `store`) dữ liệu xuống Driver bằng các lệnh tiêu chuẩn như `cat` và `echo`.
* **Khác biệt & Ảnh hưởng nếu thiếu**: Driver vẫn nạp vào Kernel thành công, nhưng **Userspace hoàn toàn không có cách nào tương tác, cài đặt hay điều khiển thông số PWM** từ bên ngoài.

---

### 4. Bảng So khớp Device Tree (Matching Table)
```c
static const struct of_device_id custom_pwm_of_match[] = {
    { .compatible = "bcm2835,custom-pwm", },
    { /* Sentinel */ }
};
MODULE_DEVICE_TABLE(of, custom_pwm_of_match);
```
* **Mục đích**: Khai báo chuỗi định danh `"bcm2835,custom-pwm"`. Khi hệ thống khởi động hoặc khi nạp Overlay, **Platform Bus** sẽ so khớp chuỗi này với chuỗi `compatible` khai báo trong tệp Device Tree (`.dts`).
* **Khác biệt & Ảnh hưởng nếu thiếu**: Lỗi logic nghiêm trọng. Platform Bus không thể ghép nối Driver với Node phần cứng trong Device Tree, dẫn đến hàm `.probe()` **không bao giờ được hệ thống tự động kích hoạt**.

---

### 5. Hàm Khởi tạo Phần cứng (`custom_pwm_probe`)
```c
static int custom_pwm_probe(struct platform_device *pdev)
{
    /* 1. Cấp phát bộ nhớ context */
    pwm = devm_kzalloc(&pdev->dev, sizeof(*pwm), GFP_KERNEL);
    
    /* 2. Trích xuất tài nguyên reg từ Device Tree */
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    
    /* 3. Ánh xạ địa chỉ vật lý sang địa chỉ ảo */
    pwm->base_addr = devm_ioremap_resource(&pdev->dev, res);
    
    /* 4. Tạo tệp thuộc tính sysfs */
    device_create_file(&pdev->dev, &dev_attr_duty_cycle);
    return 0;
}
```
* **Mục đích**: Thực thi quy trình khởi tạo phần cứng khi So khớp (Matching) thành công:
  1. Cấp phát bộ nhớ tự động quản lý (`devm_kzalloc`).
  2. Đọc dải địa chỉ vật lý `reg` từ Device Tree thông qua `platform_get_resource`.
  3. Ánh xạ dải địa chỉ vật lý sang địa chỉ ảo bằng `devm_ioremap_resource` để CPU truy cập thanh ghi.
  4. Tạo file giao tiếp `sysfs`.
* **Khác biệt & Ảnh hưởng nếu thiếu**: Không có hàm `.probe()`, Driver không thể lấy địa chỉ thanh ghi vật lý hay khởi tạo tài nguyên. Driver trở nên vô dụng.

---

### 6. Hàm Dọn dẹp & Giải phóng (`custom_pwm_remove`)
```c
static int custom_pwm_remove(struct platform_device *pdev)
{
    struct custom_pwm_dev *pwm = platform_get_drvdata(pdev);
    device_remove_file(&pdev->dev, &dev_attr_duty_cycle);
    return 0;
}
```
* **Mục đích**: Được Platform Bus gọi khi gỡ bỏ module (`rmmod`). Hàm gỡ bỏ tệp giao tiếp `sysfs` và giải phóng các tài nguyên đã cấp phát.
* **Khác biệt & Ảnh hưởng nếu thiếu**: Gây rò rỉ tài nguyên hệ thống (resource leak). Tệp `sysfs` mồ côi vẫn tồn tại trong hệ thống tệp, khi Userspace truy cập vào file này sau khi đã `rmmod` sẽ gây lỗi **Kernel Panic** hoặc treo hệ thống.

---

### 7. Cấu trúc Platform Driver & Macro Đăng ký
```c
static struct platform_driver custom_pwm_driver = {
    .probe = custom_pwm_probe,
    .remove = custom_pwm_remove,
    .driver = {
        .name = "bcm2835-custom-pwm",
        .of_match_table = custom_pwm_of_match,
    },
};

module_platform_driver(custom_pwm_driver);
```
* **Mục đích**: Khai báo đối tượng `custom_pwm_driver` chứa các hàm vòng đời và liên kết bảng so khớp `of_match_table`. Macro `module_platform_driver()` tự động sinh ra hai hàm `module_init()` và `module_exit()` để đăng ký/hủy đăng ký driver với Platform Bus.
* **Khác biệt & Ảnh hưởng nếu thiếu**: Nhân Linux sẽ không biết sự tồn tại của Driver này trên Platform Bus. Lệnh `insmod` nạp file `.ko` vào bộ nhớ nhưng driver không được đăng ký vào bus ảo, khiến hàm `probe()` không thể vận hành.

---

## VAI TRÒ & Ý NGHĨA CỦA DEVICE TREE (.dts) TRONG DRIVER NÀY

### 1. Device Tree có ý nghĩa gì và Dùng để làm gì?
Trong hệ thống nhúng Linux, các khối phần cứng ngoại vi nằm bên trong SoC (như GPIO, PWM, Timer, I2C Controller...) là các thiết bị **không thể tự phát hiện (non-discoverable devices)**.

* **Bản vẽ mô tả phần cứng (Hardware Description)**: Device Tree (`.dts`) đóng vai trò là một cơ sở dữ liệu cấu trúc cây mô tả chi tiết thông số vật lý của phần cứng bao gồm: địa chỉ thanh ghi vật lý gốc (`reg = <0x2020c000 0x28>`), chuỗi so khớp (`compatible = "bcm2835,custom-pwm"`), chỉ số đường ngắt (IRQ), và xung đồng hồ (Clocks).
* **Tách biệt Mã nguồn C và Cấu hình Phần cứng**: Device Tree giúp loại bỏ hoàn toàn việc ghi cứng (hardcode) địa chỉ vật lý trong mã C. Driver chỉ thực hiện logic điều khiển, còn thông số địa chỉ do Device Tree cung cấp.
* **Tái sử dụng Mã nguồn (Reusability)**: Một mã nguồn C của Platform Driver có thể dùng chung cho nhiều bo mạch hoặc thế hệ chip khác nhau. Khi chuyển sang phần cứng mới, kỹ sư chỉ cần thay đổi file `.dts` tương ứng mà không phải sửa hay biên dịch lại code C của Driver.

---

### 2. Mất Device Tree có ảnh hưởng gì không?
Nếu thiếu hoặc mất file Device Tree (`.dts` / `.dtbo`):

1. **Platform Bus không thể So khớp (Matching Fail)**: Kernel không tìm thấy Node mô tả phần cứng mang chuỗi `compatible = "bcm2835,custom-pwm"`. Do đó, hàm `.probe()` của Driver **sẽ KHÔNG BAO GIỜ được kích hoạt**.
2. **Không trích xuất được tài nguyên bộ nhớ**: Hàm `platform_get_resource(pdev, IORESOURCE_MEM, 0)` trong driver sẽ trả về `NULL`, dẫn đến việc không thể ánh xạ địa chỉ ảo `ioremap` để điều khiển thanh ghi PWM.
3. **Driver hoàn toàn mất tác dụng**: Mã nguồn Driver tuy được nạp vào RAM (`insmod`) nhưng trôi tự do, không liên kết được với phần cứng vật lý.

---

### 📊 Bảng Tóm tắt Hệ quả khi thiếu các Thành phần

| Thành phần | Vai trò chính | Hệ quả nếu THIẾU |
| :--- | :--- | :--- |
| **`Header Files`** | Định nghĩa API & kiểu dữ liệu Kernel | Lỗi biên dịch (`make` thất bại). |
| **`sysfs Interface`** | Điểm giao tiếp Đọc/Ghi với Userspace | Không thể điều khiển hay đọc dữ liệu từ bên ngoài. |
| **`of_match_table`** | Bảng chuỗi `compatible` so khớp | Platform Bus không match được thiết bị, `probe()` không chạy. |
| **`custom_pwm_probe()`** | Cấp phát tài nguyên & ánh xạ `ioremap` | Không khởi tạo được phần cứng. |
| **`custom_pwm_remove()`** | Dọn dẹp tài nguyên khi `rmmod` | Rò rỉ bộ nhớ, nguy cơ Kernel Panic khi gỡ module. |
| **`Device Tree (.dts)`** | Bản vẽ mô tả địa chỉ vật lý phần cứng | Driver không nhận diện được phần cứng, `probe()` bị bỏ qua. |
