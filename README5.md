# BÁO CÁO TIẾN ĐỘ ĐỒ ÁN MÔN HỌC: LẬP TRÌNH LINUX DEVICE DRIVER

**Tên dự án:** Thiết Kế Và Phát Triển Bộ Driver Nhúng Trên Linux Kernel (Network Device Driver, Platform Driver, USB Device Driver)  
**Học phần:** Hệ Thống Nhúng (Embedded Systems)  
**Hệ điều hành mục tiêu:** Linux Kernel (x86_64 Host & ARM32/ARM64 Target - Raspberry Pi)  

---

## I. THÔNG TIN NHÓM & PHÂN CHIA CÔNG VIỆC

Dự án được thực hiện bởi nhóm 03 thành viên với sự phân công chuyên môn hóa theo 3 mô hình Device Driver cốt lõi của Linux Kernel:

| STT | Thành viên | MSSV | Vai trò | Mô hình Driver phụ trách | Nội dung công việc chi tiết | Tiến độ |
| :---: | :--- | :---: | :---: | :--- | :--- | :---: |
| 1 | **Võ Trần Đăng Khoa** | **24119051** | **Nhóm trưởng** | **Network Device Driver** | <ul><li>Nghiên cứu Linux Network Subsystem, cấu trúc `net_device`, `net_device_ops` và `sk_buff`.</li><li>Lập trình driver mạng `my_net_driver.c`: khởi tạo giao diện `mynet0`, xử lý đóng/mở hàng đợi (`netif_start_queue`/`stop_queue`), truyền nhận gói tin (`ndo_start_xmit`).</li><li>Quản lý bộ đệm socket và giải phóng bộ nhớ `dev_kfree_skb()`.</li><li>Chủ trì việc đóng gói, biên dịch chéo và quản lý kho chứa GitHub.</li></ul> | **100%** |
| 2 | **Huỳnh Anh Tuấn** | **24119096** | **Thành viên** | **Platform Driver** | <ul><li>Nghiên cứu Platform Bus & Device Tree Model (`of_device_id`, `compatible` string).</li><li>Lập trình Platform Driver `platform_driver.c`: định nghĩa các hàm `probe()` và `remove()`.</li><li>Thực hiện ánh xạ bộ nhớ `ioremap()` để truy cập thanh ghi phần cứng (GPIO/PWM/I2C) của vi xử lý Broadcom (BCM2835/2711).</li><li>Thiết lập môi trường biên dịch chéo (Cross-compiler Toolchain) và cấu hình máy ảo Ubuntu.</li></ul> | **100%** |
| 3 | **Phạm Trần Huy Hoàng** | **24119039** | **Thành viên** | **USB Device Driver** | <ul><li>Nghiên cứu USB Core Subsystem, cấu trúc `usb_driver` và ma trận nhận diện thiết bị `usb_device_id` (Vendor ID, Product ID).</li><li>Lập trình USB Driver `usb_device_driver.c`: viết hàm `probe()` khi cắm thiết bị USB và `disconnect()` khi rút thiết bị.</li><li>Tương tác với các USB Endpoints (Control, Bulk, Interrupt) và quản lý truyền dữ liệu thông qua URB (USB Request Block).</li><li>Xây dựng kịch bản kiểm thử, kiểm tra rò rỉ bộ nhớ và viết báo cáo tiến độ.</li></ul> | **100%** |

---

## II. SƠ ĐỒ THIẾT KẾ TOÀN HỆ THỐNG (SYSTEM ARCHITECTURE)

Hệ thống được thiết kế dựa trên kiến trúc phân tầng chuẩn của Linux Kernel, phân định rõ ràng giữa User Space, Kernel Space và Hardware Layer:

```
+-----------------------------------------------------------------------------------+
|                                USER SPACE                                         |
|   +---------------------+   +-----------------------+   +---------------------+   |
|   | Socket Applications |   | Sysfs / Devfs Access  |   | USB Control Utilities|   |
|   | (ping, netcat, etc.)|   | (/sys/bus/platform)   |   | (libusb, custom app)|   |
|   +----------+----------+   +-----------+-----------+   +----------+----------+   |
+--------------|--------------------------|--------------------------|--------------+
               | (Socket API)             | (System Calls)           | (USB Core API)
+--------------v--------------------------v--------------------------v--------------+
|                                KERNEL SPACE                                       |
|  +-----------------------------------------------------------------------------+  |
|  |                     KERNEL SUBSYSTEMS / FRAMEWORKS                          |  |
|  |  +-------------------+    +--------------------+    +--------------------+  |  |
|  |  | Network Stack     |    | Platform Bus Core  |    | USB Core Subsystem |  |  |
|  |  | (TCP/IP, sk_buff) |    | (Device Tree Match)|    | (URB Manager)      |  |  |
|  |  +---------+---------+    +---------+----------+    +---------+----------+  |  |
|  +------------|------------------------|-------------------------|-------------+  |
|               |                        |                         |                |
|  +------------v------------------------v-------------------------v-------------+  |
|  |                         DEVICE DRIVER LAYER                                 |  |
|  |  +-------------------+    +--------------------+    +--------------------+  |  |
|  |  | Network Driver    |    | Platform Driver    |    | USB Device Driver  |  |  |
|  |  | (Võ Trần Đăng Khoa)|    | (Huỳnh Anh Tuấn)   |    | (Phạm T. Huy Hoàng)|  |  |
|  |  +---------+---------+    +---------+----------+    +---------+----------+  |  |
|  +------------|------------------------|-------------------------|-------------+  |
+---------------|------------------------|-------------------------|----------------+
                |                        |                         |
+---------------+------------------------+-------------------------+----------------+
|                                HARDWARE LAYER                                     |
|  +-------------------+    +--------------------+    +--------------------+        |
|  | Virtual / PHY     |    | Broadcom SoC       |    | USB Hardware       |        |
|  | Ethernet Interface|    | (GPIO/PWM Registers|    | Peripherals        |        |
|  +-------------------+    +--------------------+    +--------------------+        |
+-----------------------------------------------------------------------------------+
```

---

## III. TIẾN ĐỘ THỰC HIỆN DỰ ÁN TỪNG BƯỚC (STEP-BY-STEP PROGRESS)

### Bước 1: Cài đặt và chuẩn bị môi trường phát triển (Environment Setup)
1. **Thiết lập máy ảo Host**: Cài đặt Ubuntu OS (phiên bản 22.04/24.04 LTS) trên Oracle VM VirtualBox.
2. **Cài đặt các công cụ biên dịch nền tảng**:
   ```bash
   sudo apt update
   sudo apt install -y build-essential bc bison flex libncurses5-dev libssl-dev git
   ```
3. **Cài đặt bộ biên dịch chéo ARM (Cross-Compiler Toolchain)**:
   ```bash
   sudo apt install -y gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf
   ```
4. **Tải và cấu hình Kernel Source/Headers**: Tải mã nguồn nhân Linux Raspberry Pi phục vụ cho việc biên dịch module khớp phiên bản nhân.

---

### Bước 2: Thiết kế & Lập trình mã nguồn các Driver (Driver Implementation)

#### 1. Network Device Driver (`my_net_driver.c` - Võ Trần Đăng Khoa)
* Khai báo cấu trúc `net_device` và bảng điều khiển `net_device_ops`.
* Cài đặt callback `ndo_open()` để bật hàng đợi mạng (`netif_start_queue()`) và `ndo_stop()` để đóng hàng đợi (`netif_stop_queue()`).
* Cài đặt callback `ndo_start_xmit()` tiếp nhận gói tin `sk_buff` từ TCP/IP stack, cập nhật thống kê `tx_packets`/`tx_bytes` và giải phóng bộ đệm `dev_kfree_skb()`.
* Đăng ký thiết bị mạng ảo `mynet0` thông qua `register_netdev()`.

#### 2. Platform Driver (`platform_driver.c` - Huỳnh Anh Tuấn)
* Khai báo cấu trúc `platform_driver` và danh sách ghép nối Device Tree `of_device_id` với chuỗi `compatible = "custom,platform-dev"`.
* Lập trình hàm `probe()`: được Kernel tự động gọi khi tìm thấy thiết bị phù hợp trên Platform Bus, thực hiện lấy tài nguyên bộ nhớ (`platform_get_resource()`) và ánh xạ sang địa chỉ ảo (`ioremap()`).
* Lập trình hàm `remove()`: hủy ánh xạ bộ nhớ (`iounmap()`) và giải phóng tài nguyên khi gỡ driver.

#### 3. USB Device Driver (`usb_device_driver.c` - Phạm Trần Huy Hoàng)
* Khai báo cấu trúc `usb_driver` và ma trận thiết bị `usb_device_id` chứa Vendor ID và Product ID của thiết bị USB mục tiêu.
* Lập trình hàm `probe()`: được gọi khi thiết bị USB có ID tương ứng được cắm vào hệ thống, thực hiện nhận diện Endpoint (Control/Bulk/Interrupt) và khởi tạo kết nối.
* Lập trình hàm `disconnect()`: được gọi khi rút thiết bị USB, hủy các URB đang chờ và giải phóng bộ nhớ.

---

### Bước 3: Xây dựng Makefile đa mục tiêu (Multi-Target Makefile)

Hệ thống sử dụng tệp `Makefile` tối ưu hóa, hỗ trợ cả biên dịch cục bộ (x86_64) và biên dịch chéo (ARM32/ARM64):

```makefile
obj-m += my_net_driver.o
obj-m += platform_driver.o
obj-m += usb_device_driver.o

KDIR ?= /lib/modules/$(shell uname -r)/build

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean
```

Lệnh biên dịch chéo cho Raspberry Pi:
```bash
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- KDIR=~/workspace/linux
```

---

### Bước 4: Nghiệm thử & Kiểm thử Hệ thống (Testing & Verification)

Quá trình nghiệm thử được thực hiện qua các bước nghiêm ngặt trên môi trường máy ảo Ubuntu và Kernel log:

#### 1. Kiểm thử Network Device Driver:
* **Nạp module**: `sudo insmod my_net_driver.ko`
* **Xác nhận giao diện mạng**: `ip link show mynet0`
* **Bật giao diện & Gán IP**:
  ```bash
  sudo ip link set dev mynet0 up
  sudo ip addr add 192.168.100.1/24 dev mynet0
  ```
* **Kiểm thử truyền gói tin (TX Test)**: `ping -c 3 192.168.100.2`
* **Kiểm tra Kernel Log**: `sudo dmesg | grep my_net_driver` (xác nhận log xuất `Transmitting packet of length...`).

#### 2. Kiểm thử Platform Driver:
* **Nạp module**: `sudo insmod platform_driver.ko`
* **Xác nhận đăng ký driver trên Platform Bus**: `ls /sys/bus/platform/drivers/custom_platform_driver`
* **Kiểm tra Kernel Log**: `sudo dmesg | grep platform` (xác nhận hàm `probe()` chạy thành công và nhận diện tài nguyên bộ nhớ).

#### 3. Kiểm thử USB Device Driver:
* **Nạp module**: `sudo insmod usb_device_driver.ko`
* **Cắm thiết bị USB vào máy**: Kiểm tra Kernel log tự động kích hoạt hàm `probe()` nhận diện Vendor ID / Product ID.
* **Rút thiết bị USB**: Kiểm tra Kernel log tự động kích hoạt hàm `disconnect()`.

---

### Bước 5: Triển khai và Kiểm thử trên Phần cứng Thực tế (Hardware Execution)

Nhóm đã tiến hành đưa toàn bộ bộ driver lên bo mạch nhúng **Raspberry Pi Zero W / Raspberry Pi 4**:

1. **Truyền tệp nhị phân `.ko` sang phần cứng qua SSH/SCP**:
   ```bash
   scp my_net_driver.ko platform_driver.ko usb_device_driver.ko pi@192.168.1.150:~/drivers/
   ```
2. **Kiểm thử thực địa trên Raspberry Pi**:
   * Nạp trực tiếp các driver vào Kernel của Raspberry Pi OS.
   * Giao diện mạng `mynet0` hoạt động ổn định trên kiến trúc ARM.
   * Platform Driver điều khiển chính xác các chân GPIO/PWM thực tế trên bo mạch.
   * USB Driver nhận diện chính xác ngoại vi cắm vào cổng USB OTG của Raspberry Pi.
3. **Đánh giá độ ổn định**: Chạy liên tục trong 12 giờ, xác nhận không xảy ra hiện tượng Kernel Panic, rò rỉ bộ nhớ (Memory Leak) hoặc treo hệ thống.

---

## IV. TỔNG KẾT & HƯỚNG PHÁT TRIỂN

1. **Kết quả đạt được**:
   * Hoàn thành 100% mục tiêu thiết kế và cài đặt 3 loại Linux Device Driver chuẩn mực.
   * Kiểm thử thành công trên cả môi trường máy ảo Ubuntu x86_64 và phần cứng bo mạch nhúng Raspberry Pi.
   * Mã nguồn được tổ chức sạch sẽ, có đầy đủ ghi chú và tài liệu hướng dẫn trên kho chứa GitHub.
2. **Hướng phát triển tiếp theo**:
   * Tích hợp thêm cơ chế NAPI (New API) cho Network Driver để tối ưu hóa hiệu năng nhận gói tin khi lưu lượng mạng cao.
   * Bổ sung giao tiếp DMA (Direct Memory Access) cho Platform Driver để truyền dữ liệu tốc độ cao mà không gây tải CPU.
