# BÁO CÁO TIẾN ĐỘ ĐỀ TÀI: XÂY DỰNG VÀ KIỂM THỬ PLATFORM DEVICE DRIVER TRÊN LINUX NHÚNG

---

## I. THÔNG TIN CHUNG
* **Tên đề tài / Hạng mục**: Lập trình Platform Device Driver điều khiển ngoại vi PWM (Pulse Width Modulation) và tích hợp Device Tree trên Linux Nhúng.
* **Môn học**: Hệ thống nhúng (Embedded Systems)
* **Giảng viên hướng dẫn**: Trương Ngọc Sơn
* **Môi trường thực thi & Kiểm thử**: Ubuntu 22.04 LTS (Oracle VirtualBox) / Raspberry Pi (SoC Broadcom BCM2835/BCM2711)
* **Ngày cập nhật**: 22/09/2026

---

## II. MỤC TIÊU & PHẠM VI CÔNG VIỆC

### 1. Mục tiêu đề tài
- Nắm vững kiến trúc **Linux Device Driver Architecture** và cơ chế hoạt động của tầng **Device Driver Layer**.
- Xây dựng hoàn chỉnh file **Device Tree Overlay (`.dts`)** mô tả tài nguyên phần cứng ngoại vi (địa chỉ thanh ghi vật lý `reg`, chuỗi định danh `compatible`).
- Viết mã nguồn C cho **Platform Driver** theo đúng mô hình quản lý vòng đời nhân Linux (`.probe()`, `.remove()`, `of_match_table`).
- Tạo giao diện giao tiếp **`sysfs`** để ứng dụng từ **Userspace** có thể đọc/ghi và điều khiển thông số thiết bị.
- Thực hiện kiểm thử (Runtest) thành công luồng logic trên máy ảo Ubuntu VirtualBox trước khi nạp lên bo mạch thực tế.

### 2. Phạm vi đã thực hiện
- [x] Thiết kế & biên dịch tệp Device Tree Overlay (`.dts` $\rightarrow$ `.dtbo`).
- [x] Lập trình mã nguồn Platform Driver C (`pwm_platform_driver.c`).
- [x] Tạo `Makefile` tự động hóa quá trình biên dịch Kernel Module và Device Tree.
- [x] Thực hiện Runtest 5 bước trên máy ảo Ubuntu thành công.
- [x] Chẩn đoán và khắc phục các sự cố về phân quyền `sysfs` và lỗi giải phóng tài nguyên `rmmod: Device or resource busy`.

---

## III. BẢNG TIẾN ĐỘ CHI TIẾT VÀ TRẠNG THÁI HẠNG MỤC

| STT | Hạng mục công việc | Mô tả chi tiết | Trạng thái | Tỷ lệ hoàn thành |
| :-: | :--- | :--- | :-: | :-: |
| **1** | **Khảo sát kiến trúc** | Nghiên cứu tài nguyên chip Broadcom (BCM2835/BCM2711) và cơ chế Platform Bus | Hoàn thành | 100% |
| **2** | **Xây dựng Device Tree** | Viết file `bcm2835_custom_pwm.dts` mô tả node PWM tại địa chỉ `0x2020C000` | Hoàn thành | 100% |
| **3** | **Lập trình Platform Driver** | Viết code C triển khai bảng so khớp `of_match_table`, hàm `probe()`, `remove()` và `sysfs` | Hoàn thành | 100% |
| **4** | **Biên dịch & Build Module** | Cấu hình `Makefile` biên dịch tạo ra tệp `.ko` và `.dtbo` | Hoàn thành | 100% |
| **5** | **Runtest trên Ubuntu** | Nạp module, kiểm tra log `dmesg`, kiểm thử tương tác đọc/ghi `duty_cycle` từ Userspace | Hoàn thành | 100% |
| **6** | **Xử lý Bug & Tối ưu** | Khắc phục lỗi kẹt module `EBUSY`, cấu hình ngắt liên kết thiết bị (unbind) an toàn | Hoàn thành | 100% |
| **7** | **Ứng dụng Userspace & Hardware Test** | Viết ứng dụng C điều khiển LED hô hấp và chuyển giao sang nạp bo mạch Raspberry Pi thật | Đang thực hiện | 60% |

---

## IV. KẾT QUẢ ĐẠT ĐƯỢC

### 1. Sản phẩm Mã nguồn đã Hoàn thành
1. **Tệp Device Tree Source (`bcm2835_custom_pwm.dts`)**:
   - Định nghĩa thành công Node `pwm@2020c000` thuộc phân vùng `/soc`.
   - Khai báo dải địa chỉ thanh ghi vật lý: `reg = <0x2020c000 0x28>`.
   - Khai báo chuỗi so khớp định danh: `compatible = "bcm2835,custom-pwm"`.

2. **Tệp Platform Driver C (`pwm_platform_driver.c`)**:
   - Khai báo bảng so khớp `custom_pwm_of_match[]` chứa chuỗi `compatible = "bcm2835,custom-pwm"`.
   - Hàm `custom_pwm_probe()`: Tự động kích hoạt khi Platform Bus so khớp thành công, trích xuất tài nguyên bộ nhớ từ DTS, thực hiện `ioremap` và bộc lộ thuộc tính `duty_cycle` trong `sysfs`.
   - Hàm `custom_pwm_remove()`: Thực hiện dọn dẹp thuộc tính `sysfs` và giải phóng tài nguyên an toàn khi gỡ module.

3. **Cấu hình Biên dịch (`Makefile`)**:
   - Tự động biên dịch tệp C thành Module nhị phân `.ko` bằng Kernel Headers của Ubuntu.
   - Sử dụng công cụ `dtc` biên dịch `.dts` thành `.dtbo`.

### 2. Kết quả Kiểm thử (Runtest Verification)
* **Kết quả nạp & So khớp (Matching)**:
  - Khi nạp `.dtbo` và `insmod pwm_platform_driver.ko`, lệnh `dmesg` ghi nhận hàm `.probe()` được kích hoạt ngay lập tức:
    ```text
    [Platform Driver] PROBE STARTED! Matched with Device Tree!
    [DTS Resource] Physical Base = 0x2020c000, Size = 0x2020c000-0x2020c027
    [Platform Driver] PROBE COMPLETED SUCCESSFULLY!
    ```
* **Kết quả tương tác Userspace (`sysfs`)**:
  - Hệ thống tự động tạo node thiết bị tại `/sys/bus/platform/devices/2020c000.pwm/`.
  - Thực hiện lệnh `cat duty_cycle` trả về giá trị mặc định: `512`.
  - Thực hiện lệnh `echo 750 | sudo tee duty_cycle` cập nhật thành công giá trị mới xuống Driver, log Kernel xác nhận: `[Platform Driver] Updated Duty Cycle = 750`.
* **Kết quả gỡ bỏ (Unload)**:
  - Lệnh `sudo rmmod pwm_platform_driver` thực thi mượt mà, hàm `remove()` dọn dẹp sạch tài nguyên, không gây rò rỉ bộ nhớ.

---

## V. CÁC VẤN ĐỀ ĐÃ XỬ LÝ & BÀI HỌC KINH NGHIỆM

1. **Sự cố 1: Lỗi `rmmod: ERROR: Device or resource busy` (Mã lỗi `EBUSY`)**
   - *Nguyên nhân*: Do việc khai báo `.owner = THIS_MODULE` trong cấu trúc driver khiến Platform Bus tự động tăng biến đếm tham chiếu (`refcount`), chặn không cho phép gỡ module khi thiết bị còn gán (bind).
   - *Giải pháp*: Thực hiện ngắt liên kết thủ công qua `unbind` (`echo "2020c000.pwm" | sudo tee /sys/bus/platform/drivers/bcm2835-custom-pwm/unbind`) hoặc bỏ khai báo `.owner = THIS_MODULE` trong mã C để quản lý vòng đời chuẩn.

2. **Sự cố 2: Lỗi phân quyền truy cập `sysfs` (`Permission Denied`)**
   - *Nguyên nhân*: File thuộc tính trong `/sys/...` do Kernel tạo ra thuộc quyền `root`.
   - *Giải pháp*: Sử dụng lệnh `sudo tee` để ghi dữ liệu từ không gian người dùng.

---

## VI. KẾ HOẠCH BƯỚC TIẾP THEO

1. **Hoàn thiện Ứng dụng C Userspace (Tuần tiếp theo)**:
   - Viết chương trình C tự động cập nhật liên tục giá trị `duty_cycle` để tạo hiệu ứng **Breathing LED (LED hô hấp)**.
2. **Thực hành Cross-Compile & Nạp bo mạch thật**:
   - Sử dụng toolchain `arm-linux-gnueabihf-gcc` để biên dịch module và nạp thử nghiệm trực tiếp trên phần cứng **Raspberry Pi 4 (SoC BCM2711)**.
3. **Hoàn thiện Báo cáo Tổng kết Cuối kỳ**:
   - Đóng gói toàn bộ mã nguồn, sơ đồ nguyên lý và video demo chạy thực tế nộp lên hệ thống LMS/GitHub.
