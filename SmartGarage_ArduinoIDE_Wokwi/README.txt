ESP32 SMART GARAGE - WOKWI

Trong project nay da giu nguyen GPIO va logic chinh tu esp32_SmartGarage.ino.
Thay doi rieng de chay tren Wokwi:
- Wi-Fi: Wokwi-GUEST
- DHT11 that -> DHT22 mo phong (chan DATA van GPIO4)
- OLED SH1106 that -> SSD1306 mo phong (I2C SDA=21, SCL=22)
- Cam bien anh sang dung ngo ra so DO noi GPIO34.

CACH DUA LEN WOKWI:
1. Mo https://wokwi.com/projects/new/esp32
2. Tab diagram.json: thay toan bo bang file diagram.json trong thu muc nay.
3. Tab sketch.ino: thay toan bo bang file sketch.ino trong thu muc nay.
4. Mo Library Manager va cai cac thu vien trong libraries.txt neu Wokwi chua tu nhan.
5. Bam nut xanh Start the Simulation.
6. Bam Save va Share de lay URL project cong khai nop vao cot Link Wokwi.

LUU Y:
- Project Wokwi tren cloud can duoc Save trong tai khoan cua ban de tao URL nop bai.
- RC522 tren mo phong co UID mac dinh 1D 34 E9 00, khop UID_THE_HOP_LE trong code.
- Nguong quat giu nguyen code goc: BAT 40C, TAT 35C.
- Cac linh kien mo phong khong the thay the hoan toan phan cung vat ly; relay chi the hien dong/ngat.
- Code webserver giu nguyen; kha nang truy cap dashboard tu trinh duyet ben ngoai Wokwi phu thuoc co che mang/port forwarding cua Wokwi.
