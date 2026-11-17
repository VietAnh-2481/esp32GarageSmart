#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <DHT.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Adafruit_NeoPixel.h>

// ===== WIFI: sua 2 dong nay theo mang Wi-Fi cua ban =====
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

// ===== CHAN PHAN CUNG theo file relayAutoOnOffFan.ino =====
#define DHT_PIN       4
#define DHT_TYPE      DHT22 // Wokwi hien mo phong DHT22; phan cung that van co the dung DHT11
#define LED_DO        25
#define LED_XANH      33
#define RELAY_PIN     26

// RC522 dung SPI VSPI mac dinh; PIR va buzzer dung chan rieng
#define RFID_SS_PIN   5
#define RFID_RST_PIN  27
#define PIR_PIN       32
#define BUZZER_PIN    14
// GIA DINH: vong LED la WS2812B 12 LED (5V, GND, DIN)
#define RING_LED_PIN  13
#define RING_LED_COUNT 12
// Cam bien anh sang 3 chan VCC/GND/DO: dung ngo ra so DO
#define LIGHT_SENSOR_PIN 34

U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);
DHT dht(DHT_PIN, DHT_TYPE);
WebServer server(80);
MFRC522 rfid(RFID_SS_PIN, RFID_RST_PIN);
Adafruit_NeoPixel ring(RING_LED_COUNT, RING_LED_PIN, NEO_GRB + NEO_KHZ800);

// QUAN TRONG: thay UID_THE_HOP_LE bang UID that doc duoc tren Serial Monitor.
// Vi du UID  DE AD BE EF  thi ghi "DE AD BE EF".
const char* UID_THE_HOP_LE = "1D 34 E9 00";
bool baoVeBat = false;
bool dangBaoDong = false;
bool coChuyenDong = false;
unsigned long batBaoVeLuc = 0;
bool choTuBatBaoVe = false;
unsigned long lucTatBaoVe = 0;
const unsigned long THOI_GIAN_TU_BAT_BAO_VE = 15000; // Tu bat lai bao ve sau 15 giay khi quet the de tat
unsigned long lanDocPIR = 0;
unsigned long lanBaoDong = 0;
unsigned long lanQuetThe = 0;
const unsigned long THOI_GIAN_CHO_BAO_VE = 10000;
const unsigned long THOI_GIAN_CHONG_NHAY_THE = 1200;
const unsigned long THOI_GIAN_CHONG_NHAY_PIR = 250;
const unsigned long THOI_GIAN_HU_TOI_DA = 30000;
const unsigned long THOI_GIAN_DEN_SAU_SU_KIEN = 15000; // Giu den 15 giay sau khi quet the/phat hien chuyen dong
// Doi thanh false neu Serial cho thay DO=HIGH khi toi.
const bool DO_LOW_KHI_TOI = false; // theo phep thu: che cam bien=1, sang=0
unsigned long denSuKienLuc = 0;
int giaTriAnhSang = 0;
bool troiToi = false;
bool denDangBat = false;
// AUTO: theo cam bien/su kien; ON/OFF: dieu khien tu website
String cheDoVongLED = "AUTO";
bool henGioLEDEnabled = false;
String gioBatLED = "18:00";
String gioTatLED = "23:00";
unsigned long lanGhiLogSensor = 0;
bool trangThaiHenGioTruoc = false;

String thongBaoThe = "Chua quet the";
unsigned long hetThongBaoThe = 0;

// Giu nguong theo file ban gui: bat 40C, tat 35C.
// Neu muon bat 33C/tat 32C, doi hai gia tri ben duoi.
const float NHiet_DO_BAT = 40.0;
const float NHiet_DO_TAT = 35.0;

// Relay trong file goc dang dung kich LOW.
const int RELAY_ON = LOW;
const int RELAY_OFF = HIGH;

bool quatDangBat = false;
bool cheDoTuDong = true;
bool camBienLoi = false;
float nhietDo = NAN;
float doAm = NAN;
unsigned long lanDocCuoi = 0;
const unsigned long CHU_KY_DOC = 2000;

const char PAGE[] PROGMEM = R"HTML(
<!doctype html><html lang="vi"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32 Smart Fan</title><style>
:root{--bg:#0a1020;--card:#131e33;--line:#293650;--muted:#91a0bb;--text:#edf4ff;--cyan:#53d9e8;--green:#35d49a;--orange:#ffb454;--red:#ff667a}
*{box-sizing:border-box}body{margin:0;background:radial-gradient(ellipse at 10% 0%,#19345a,transparent 42%),var(--bg);font:14px system-ui;color:var(--text)}main{max-width:1150px;margin:auto;padding:26px 18px}.top,.banner,.head,.metrichead,.device,.fanrow,.footer{display:flex;align-items:center;justify-content:space-between;gap:14px}.top{margin-bottom:22px;flex-wrap:wrap}.brand{display:flex;align-items:center;gap:12px}.logo{background:linear-gradient(135deg,var(--cyan),#6077ff);color:#061322;border-radius:14px;padding:12px;font-weight:900;font-size:22px}.eyebrow{color:var(--cyan);font-size:10px;letter-spacing:2px;font-weight:800}.brand h1{font-size:22px;margin:2px 0}.pill,.tag{border:1px solid var(--line);border-radius:99px;padding:8px 12px;color:var(--muted);font-size:11px}.dot{display:inline-block;width:7px;height:7px;border-radius:50%;background:var(--green);margin-right:7px}.banner{background:linear-gradient(110deg,#172b49,#111a2d);border:1px solid #2c4263;border-radius:20px;padding:22px;margin-bottom:16px;flex-wrap:wrap}.banner h2{font-size:26px;margin:0 0 7px}.banner p{color:var(--muted);line-height:1.6;margin:0;font-size:12px}.status{background:#10352f;border:1px solid #285d51;color:#75e6bd;border-radius:12px;padding:11px;font-size:11px;font-weight:800}.status.bad{background:#421d2a;color:#ffabb6;border-color:#743447}.grid{display:grid;grid-template-columns:repeat(4,1fr);gap:14px;margin-bottom:16px}.card{background:linear-gradient(145deg,#141f34,#101829);border:1px solid var(--line);border-radius:18px;padding:18px;min-width:0}.metrichead{font-size:11px;color:var(--muted)}.ico{font-size:20px;background:#26344e;border-radius:11px;padding:8px}.value{font-size:33px;font-weight:800;letter-spacing:-1px;margin:16px 0 6px}.unit{font-size:15px;color:var(--muted);font-weight:500}.sub,.muted{font-size:11px;color:var(--muted)}.content{display:grid;grid-template-columns:1.25fr 1fr;gap:16px}.head{margin-bottom:18px}.head h3{font-size:15px;margin:0}.chart{height:220px;width:100%;display:block}.legend{display:flex;gap:16px;color:var(--muted);font-size:11px}.legend i{display:inline-block;width:7px;height:7px;border-radius:50%;margin-right:5px}.mode{display:grid;grid-template-columns:1fr 1fr;background:#0b1221;padding:4px;border-radius:12px;gap:5px;margin-bottom:14px}.mode button,.action{border:0;border-radius:9px;padding:11px;cursor:pointer;font-weight:750;color:var(--muted);background:transparent}.mode button.active{background:#2a3b5b;color:white}.fanrow{border:1px solid var(--line);background:#0c1527;border-radius:15px;padding:15px;margin-bottom:14px}.fanicon{font-size:32px;color:var(--cyan);background:#172b3b;border-radius:16px;padding:14px}.spin{animation:spin 1.2s linear infinite}@keyframes spin{to{transform:rotate(360deg)}}.fanrow h4{margin:0 0 5px}.fanrow p{margin:0;color:var(--muted);font-size:11px;line-height:1.5}.switch{width:48px;height:27px;flex-shrink:0;border:0;border-radius:99px;background:#36435b;position:relative;cursor:pointer}.switch:after{content:"";position:absolute;left:4px;top:4px;width:19px;height:19px;background:#fff;border-radius:50%;transition:.2s}.switch.on{background:var(--green)}.switch.on:after{left:25px}.action{width:100%;background:#1a2b47;border:1px solid #344c70;color:#e4eeff;margin-bottom:13px}.threshold{display:flex;justify-content:space-between;gap:10px;background:#111d32;border:1px solid var(--line);border-radius:12px;padding:13px}.threshold span{display:block;color:var(--muted);font-size:10px;margin-top:4px}.threshold b{color:var(--orange);white-space:nowrap}.lower{display:grid;grid-template-columns:1fr 1fr;gap:16px;margin-top:16px}.device{padding:12px 0;border-bottom:1px solid #202d45}.device:last-child{border:0}.devleft{display:flex;align-items:center;gap:10px}.devicon{background:#1b2941;border-radius:10px;padding:9px}.devname{font-size:12px;font-weight:700}.devpin{font-size:10px;color:var(--muted);margin-top:4px}.tag{padding:5px 8px;background:#12382f;color:#75e6bd;border:0;font-weight:800}.tag.off{background:#2a3041;color:#9aa9c4}.log{display:grid;grid-template-columns:60px 1fr;gap:10px;font-size:11px;color:var(--muted);padding:10px 0;border-bottom:1px solid #202d45}.logtime{color:#70809c}.navbar{display:flex;gap:7px;flex-wrap:wrap;margin:0 0 16px;padding:8px;background:#101a2c;border:1px solid var(--line);border-radius:14px;position:sticky;top:6px;z-index:5}.navbar button{border:1px solid transparent;border-radius:9px;padding:10px 12px;background:transparent;color:var(--muted);font-weight:750;cursor:pointer}.navbar button.active{background:#243958;border-color:#3b567e;color:#fff}.tab-hidden{display:none!important}.logtools{display:grid;grid-template-columns:1fr 1fr;gap:8px;margin-bottom:10px}.logtools input,.logtools select,.schedule input{width:100%;min-width:0;padding:10px;border-radius:9px;border:1px solid var(--line);background:#0b1221;color:var(--text)}.logmeta{display:block;font-size:10px;color:var(--cyan);margin-bottom:4px}.loglevel{display:inline-block;border-radius:5px;padding:2px 5px;background:#243650;color:#c7d8f4;font-size:9px}.schedule{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin:12px 0}.schedule label{font-size:11px;color:var(--muted)}.schedule label input{display:block;margin-top:6px}.footer{color:#62728d;font-size:10px;margin-top:20px;flex-wrap:wrap}.toast{position:fixed;bottom:20px;left:50%;transform:translate(-50%,15px);opacity:0;background:#1a2b47;border:1px solid #3a5278;border-radius:10px;padding:12px 16px;transition:.2s;pointer-events:none}.toast.show{opacity:1;transform:translate(-50%,0)}@media(max-width:850px){.grid{grid-template-columns:repeat(2,1fr)}.content,.lower{grid-template-columns:1fr}}@media(max-width:450px){main{padding:14px 10px}.card{padding:14px}.value{font-size:28px}.banner h2{font-size:22px}}
</style></head><body><main>
<header class="top"><div class="brand"><div class="logo">⌁</div><div><div class="eyebrow">IOT CONTROL CENTER</div><h1>ESP32 Smart Fan</h1></div></div><div class="pill"><span class="dot"></span><span id="conn">Đang kết nối...</span> · <span id="clock">--:--:--</span></div></header>
<nav class="navbar" id="navbar"><button class="active" data-tab="overview">📊 Tổng quan</button><button data-tab="control">🌀 Điều khiển</button><button data-tab="light">💡 Đèn & hẹn giờ</button><button data-tab="security">🔐 An ninh</button><button data-tab="logs">📋 Nhật ký</button></nav>
<section class="banner" id="overview"><div><h2>Xin chào! 👋</h2><p>Giám sát môi trường và điều khiển quạt thông minh.<br>Dữ liệu trực tiếp từ ESP32 qua Wi-Fi.</p></div><div class="status" id="status">ĐANG KẾT NỐI ESP32</div></section>
<section class="grid">
<article class="card"><div class="metrichead">NHIỆT ĐỘ <span class="ico" style="color:var(--orange)">♨</span></div><div class="value"><span id="temp">--</span><span class="unit"> °C</span></div><div class="sub" id="tempstatus">DHT11</div></article>
<article class="card"><div class="metrichead">ĐỘ ẨM <span class="ico" style="color:var(--cyan)">◉</span></div><div class="value"><span id="hum">--</span><span class="unit"> %</span></div><div class="sub">Độ ẩm không khí · DHT22</div></article>
<article class="card"><div class="metrichead">TRẠNG THÁI QUẠT <span class="ico" style="color:var(--green)">✳</span></div><div class="value" style="font-size:25px" id="fanmetric">--</div><div class="sub" id="fansub">Relay GPIO26</div></article>
<article class="card"><div class="metrichead">CHẾ ĐỘ <span class="ico" style="color:#b5a2ff">⚙</span></div><div class="value" style="font-size:25px" id="modemetric">--</div><div class="sub" id="modesub">ESP32</div></article></section>
<section class="content" id="control"><article class="card"><div class="head"><h3>Lịch sử môi trường</h3><span class="muted">Dữ liệu trực tiếp</span></div><canvas id="chart" class="chart"></canvas><div class="legend"><span><i style="background:var(--orange)"></i>Nhiệt độ</span><span><i style="background:var(--cyan)"></i>Độ ẩm (thu nhỏ)</span></div></article>
<article class="card"><div class="head"><h3>Điều khiển thiết bị</h3><span class="muted">ESP32 WI-FI</span></div><div class="mode"><button id="auto">⚙ AUTO</button><button id="manual">☝ MANUAL</button></div>
<div class="fanrow"><div id="fanicon" class="fanicon">✳</div><div style="flex:1"><h4 id="fantitle">Đang chờ dữ liệu</h4><p id="fandesc">Kết nối vào mạng Wi-Fi của ESP32.</p></div><button id="switch" class="switch" aria-label="Bật hoặc tắt quạt"></button></div>
<button id="toggle" class="action">Bật quạt thủ công</button><div class="threshold"><div><b style="color:var(--text)">Ngưỡng nhiệt độ</b><span id="threshold">Bật ≥ 40°C · Tắt ≤ 35°C</span></div><b>AUTO</b></div><p class="muted" id="hint">Chọn MANUAL để điều khiển trực tiếp.</p></article></section>
<section class="lower" id="devices"><article class="card"><div class="head"><h3>Thiết bị trong hệ thống</h3><span class="muted">Trạng thái thật</span></div>
<div class="device"><div class="devleft"><span class="devicon">🌡</span><div><div class="devname">Cảm biến DHT11</div><div class="devpin">DATA · GPIO 4</div></div></div><span class="tag" id="dhtTag">--</span></div>
<div class="device"><div class="devleft"><span class="devicon">▤</span><div><div class="devname">OLED SH1106</div><div class="devpin">SDA 21 · SCL 22</div></div></div><span class="tag">LOCAL</span></div>
<div class="device"><div class="devleft"><span class="devicon">🔴</span><div><div class="devname">LED đỏ</div><div class="devpin">GPIO 25</div></div></div><span class="tag off" id="redtag">--</span></div>
<div class="device"><div class="devleft"><span class="devicon">🟢</span><div><div class="devname">LED xanh</div><div class="devpin">GPIO 33</div></div></div><span class="tag" id="greentag">--</span></div></article>
<article class="card" id="logsPanel"><div class="head"><h3>📋 Nhật ký hoạt động chi tiết</h3><span class="muted" id="logCount">0 sự kiện</span></div><div class="logtools"><input id="logSearch" placeholder="Tìm RFID, quạt, GPIO, lỗi..."><select id="logCategory"><option value="ALL">Tất cả nhóm</option><option value="HE_THONG">Hệ thống</option><option value="CAM_BIEN">Cảm biến</option><option value="QUAT">Quạt / relay</option><option value="LED">Vòng LED</option><option value="HEN_GIO">Hẹn giờ LED</option><option value="RFID">RFID</option><option value="AN_NINH">An ninh / PIR</option><option value="BUZZER">Còi</option><option value="WEB">Website</option><option value="LOI">Lỗi</option></select></div><button id="clearLogs" class="action" style="width:auto;padding:8px 10px">Xóa nhật ký trên web</button><div id="logs" style="max-height:480px;overflow:auto"><div class="log">Đang chờ sự kiện...</div></div></article></section>
<section class="lower" id="light"><article class="card"><div class="head"><h3>💡 Điều khiển vòng LED</h3><span class="muted">GPIO13 · 12 LED</span></div><p class="muted">Chọn AUTO để dùng cảm biến ánh sáng, hoặc bật/tắt vòng LED trực tiếp.</p><div class="mode"><button id="ringAuto" class="active">AUTO</button><button id="ringOn">BẬT LED</button></div><button id="ringOff" class="action">TẮT VÒNG LED</button><p class="muted" id="ringModeText">Chế độ tự động</p><div class="head" style="margin-top:14px;margin-bottom:8px"><h3>⏰ Hẹn giờ bật / tắt LED</h3><span class="muted" id="scheduleState">Chưa bật</span></div><p class="muted">ESP32 đồng bộ giờ Internet. Trong khung giờ này, lịch hẹn giờ ưu tiên hơn cảm biến ánh sáng.</p><div class="schedule"><label>Bật đèn lúc<input type="time" id="scheduleStart" value="18:00"></label><label>Tắt đèn lúc<input type="time" id="scheduleEnd" value="23:00"></label></div><div class="mode"><button id="scheduleEnable">Bật lịch</button><button id="scheduleDisable">Tắt lịch</button></div><button id="saveSchedule" class="action">Lưu lịch hẹn giờ</button><p class="muted" id="scheduleHint">Lịch chưa được cấu hình.</p><p class="muted" id="cardMessage">Thẻ NFC: chưa quét</p></article><article class="card" id="security"><div class="head"><h3>🔐 An ninh NFC</h3><span class="tag off" id="securityTag">CHƯA BẬT</span></div><p class="muted">Quét thẻ RC522 hợp lệ để bật/tắt bảo vệ. Khi phát hiện chuyển động, còi sẽ báo động.</p><div class="threshold"><div><b style="color:var(--text)">Chế độ bảo vệ</b><span id="securityText">Đang tắt</span></div><b id="motionText">PIR: --</b></div><button class="action" id="armBtn" style="margin-top:12px">Bật bảo vệ từ website</button><button class="action" id="silenceBtn">Tắt còi / tắt bảo vệ</button><p class="muted" id="alarmText">Trạng thái còi: --</p></article><article class="card"><div class="head"><h3>Thiết bị an ninh</h3><span class="muted">RC522 · PIR · Buzzer</span></div><div class="device"><div class="devleft"><span class="devicon">💳</span><div><div class="devname">Đầu đọc RC522</div><div class="devpin">SPI · SS GPIO5</div></div></div><span class="tag" id="rfidTag">SẴN SÀNG</span></div><div class="device"><div class="devleft"><span class="devicon">🕵️</span><div><div class="devname">Cảm biến PIR</div><div class="devpin">OUT · GPIO32</div></div></div><span class="tag off" id="pirTag">--</span></div><div class="device"><div class="devleft"><span class="devicon">🔊</span><div><div class="devname">Còi báo động</div><div class="devpin">GPIO14 · kiểm tra loại buzzer</div></div></div><span class="tag off" id="buzzerTag">--</span></div><div class="device"><div class="devleft"><span class="devicon">💡</span><div><div class="devname">Vòng LED 12 bóng</div><div class="devpin">WS2812B DIN · GPIO13</div></div></div><span class="tag off" id="ringTag">--</span></div><div class="device"><div class="devleft"><span class="devicon">🌙</span><div><div class="devname">Cảm biến ánh sáng</div><div class="devpin">DO · GPIO34</div></div></div><span class="tag off" id="lightTag">--</span></div><p class="muted" id="lightValue">ADC: --</p></article></section>
<footer class="footer"><span>ESP32 SMART FAN · REAL DEVICE MODE</span><span>Chỉ dùng trong mạng Wi-Fi tin cậy · Không mở cổng router ra Internet</span></footer></main><div class="toast" id="toast"></div>
<script>
const $=id=>document.getElementById(id);let history=[],lastState='',lastLog='',timer;
function time(){return new Date().toLocaleTimeString('vi-VN',{hour12:false})}
function toast(s){$('toast').textContent=s;$('toast').classList.add('show');clearTimeout(timer);timer=setTimeout(()=>$('toast').classList.remove('show'),2200)}
let eventLogs=[];function log(s,cat='WEB',level='THONG_TIN'){eventLogs.unshift({time:time(),cat,level,msg:s});if(eventLogs.length>150)eventLogs.length=150;renderLogs()}function renderLogs(){if(!$('logs'))return;let q=($('logSearch').value||'').toLowerCase(),cat=$('logCategory').value;let rows=eventLogs.filter(e=>(cat==='ALL'||cat===e.cat)&&(`${e.cat} ${e.level} ${e.msg}`).toLowerCase().includes(q));$('logCount').textContent=rows.length+' / '+eventLogs.length+' sự kiện';$('logs').innerHTML='';if(!rows.length){$('logs').textContent='Không có sự kiện khớp bộ lọc.';return}rows.forEach(e=>{let row=document.createElement('div');row.className='log';let t=document.createElement('span');t.className='logtime';t.textContent=e.time;let m=document.createElement('span');m.innerHTML='<span class="logmeta">'+e.cat+' · <b class="loglevel">'+e.level+'</b></span>';let msg=document.createElement('span');msg.textContent=e.msg;m.appendChild(msg);row.append(t,m);$('logs').appendChild(row)})}$('logSearch').addEventListener('input',renderLogs);$('logCategory').addEventListener('change',renderLogs);$('clearLogs').onclick=()=>{eventLogs=[];renderLogs();log('Đã xóa nhật ký trên giao diện web','WEB')}
async function command(obj){try{let r=await fetch('/api/control',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(obj)});let j=await r.json();if(!r.ok||!j.ok)throw Error(j.error||'Lệnh không thành công');toast('Đã gửi lệnh tới ESP32');await poll()}catch(e){toast('Không gửi được lệnh: '+e.message)}}
$('ringAuto').onclick=()=>{log('Chuyển vòng LED sang AUTO','LED');command({ringMode:'AUTO'})};$('ringOn').onclick=()=>{log('Bật vòng LED thủ công','LED');command({ringMode:'ON'})};$('ringOff').onclick=()=>{log('Tắt vòng LED thủ công','LED');command({ringMode:'OFF'})};$('saveSchedule').onclick=()=>{let a=$('scheduleStart').value,b=$('scheduleEnd').value;if(!a||!b||a===b){toast('Chọn hai giờ khác nhau');log('Lịch không hợp lệ: giờ bắt đầu và kết thúc phải khác nhau','HEN_GIO','CANH_BAO');return}log('Lưu lịch LED '+a+' đến '+b,'HEN_GIO');command({scheduleStart:a,scheduleEnd:b,scheduleEnabled:true})};$('scheduleEnable').onclick=()=>command({scheduleEnabled:true});$('scheduleDisable').onclick=()=>command({scheduleEnabled:false});
$('auto').onclick=()=>command({mode:'AUTO'});$('manual').onclick=()=>command({mode:'MANUAL'});$('switch').onclick=()=>{if(window.currentMode!=='MANUAL'){toast('Chuyển sang MANUAL trước');return}command({fan:!window.currentFan})};$('toggle').onclick=()=>{if(window.currentMode!=='MANUAL'){toast('Chuyển sang MANUAL trước');return}command({fan:!window.currentFan})};
async function securityCommand(armed){try{let r=await fetch('/api/security',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({armed})});let j=await r.json();if(!r.ok||!j.ok)throw Error(j.error||'Lệnh lỗi');toast(armed?'Đã bật bảo vệ':'Đã tắt bảo vệ');await poll()}catch(e){toast('Lỗi an ninh: '+e.message)}}$('armBtn').onclick=()=>securityCommand(true);$('silenceBtn').onclick=()=>securityCommand(false);
function draw(){let c=$('chart'),r=c.getBoundingClientRect();if(!r.width||!r.height)return;let d=window.devicePixelRatio||1;c.width=r.width*d;c.height=r.height*d;let x=c.getContext('2d');x.scale(d,d);let w=r.width,h=r.height,p={l:28,r:8,t:10,b:22},cw=w-p.l-p.r,ch=h-p.t-p.b;x.clearRect(0,0,w,h);x.font='10px system-ui';x.fillStyle='#72819c';x.strokeStyle='#28354d';for(let i=0;i<=4;i++){let y=p.t+ch*i/4;x.beginPath();x.moveTo(p.l,y);x.lineTo(w-p.r,y);x.stroke();x.fillText(40-i*5,1,y+3)}['old','-40s','-20s','now'].forEach((s,i)=>x.fillText(s,p.l+cw*i/3-8,h-4));function line(key,color,scale=1,offset=0){let vals=history.filter(v=>Number.isFinite(v[key]));if(vals.length<2)return;x.beginPath();vals.forEach((v,i)=>{let xx=p.l+cw*i/(vals.length-1),val=v[key]*scale+offset,yy=p.t+ch*(1-(val-20)/20);i?x.lineTo(xx,yy):x.moveTo(xx,yy)});x.strokeStyle=color;x.lineWidth=2.4;x.stroke()}line('t','#ffb454');line('h','#53d9e8',.25,15)}
async function poll(){try{let r=await fetch('/api/state',{cache:'no-store'});if(!r.ok)throw Error('HTTP '+r.status);let s=await r.json();window.currentMode=s.mode;window.currentFan=s.fan;
$('cardMessage').textContent='Thẻ NFC: '+(s.cardMessage||'chưa quét');$('ringModeText').textContent='Chế độ vòng LED: '+(s.ringMode||'AUTO');if(s.scheduleStart)$('scheduleStart').value=s.scheduleStart;if(s.scheduleEnd)$('scheduleEnd').value=s.scheduleEnd;$('scheduleState').textContent=s.scheduleEnabled?'ĐANG BẬT':'ĐANG TẮT';$('scheduleHint').textContent=s.scheduleEnabled?('Đèn sáng từ '+s.scheduleStart+' đến '+s.scheduleEnd):'Lịch tắt; AUTO dùng cảm biến ánh sáng.';$('ringAuto').classList.toggle('active',(s.ringMode||'AUTO')==='AUTO');$('ringOn').classList.toggle('active',s.ringMode==='ON');$('ringOff').classList.toggle('active',s.ringMode==='OFF');
$('securityTag').textContent=s.armed?(s.alarm?'BÁO ĐỘNG':'ĐÃ BẬT'): 'CHƯA BẬT';$('securityTag').classList.toggle('off',!s.armed&&!s.alarm);$('securityText').textContent=s.armed?'Đang giám sát':'Đang tắt';$('motionText').textContent=s.motion?'PHÁT HIỆN!':'PIR: bình thường';$('alarmText').textContent='Còi: '+(s.alarm?'ĐANG KÊU':'đang tắt');$('rfidTag').textContent='SẴN SÀNG';$('pirTag').textContent=s.motion?'CHUYỂN ĐỘNG':'BÌNH THƯỜNG';$('pirTag').classList.toggle('off',!s.motion);$('buzzerTag').textContent=s.alarm?'ĐANG KÊU':'TẮT';$('buzzerTag').classList.toggle('off',!s.alarm);$('armBtn').textContent=s.armed?'Bảo vệ đang bật':'Bật bảo vệ từ website';$('ringTag').textContent=s.ringOn?'ĐANG SÁNG':'TẮT';$('ringTag').classList.toggle('off',!s.ringOn);$('lightTag').textContent=s.dark?'TRỜI TỐI':'ĐỦ SÁNG';$('lightTag').classList.toggle('off',!s.dark);$('lightValue').textContent='Tín hiệu DO: '+s.lightRaw+' (0/1)';
$('conn').textContent='ESP32 đang trực tuyến';$('status').textContent='ĐÃ KẾT NỐI ESP32';$('status').classList.remove('bad');
$('temp').textContent=s.temp===null?'--':Number(s.temp).toFixed(1);$('hum').textContent=s.hum===null?'--':Math.round(s.hum);
$('fanmetric').textContent=s.fan?'ĐANG BẬT':'ĐANG TẮT';$('fansub').textContent='Relay GPIO26 · '+(s.fan?'ON':'OFF');$('modemetric').textContent=s.mode;$('modesub').textContent=s.mode==='AUTO'?'Điều khiển theo nhiệt độ':'Điều khiển thủ công';
$('auto').classList.toggle('active',s.mode==='AUTO');$('manual').classList.toggle('active',s.mode==='MANUAL');$('switch').classList.toggle('on',s.fan);$('fanicon').classList.toggle('spin',s.fan);$('fantitle').textContent=s.fan?'Quạt đang chạy':'Quạt đang tắt';
$('fandesc').textContent=s.mode==='AUTO'?(s.fan?'Quạt tự bật do nhiệt độ đạt ngưỡng.':'Quạt sẽ tự bật khi đạt ngưỡng nhiệt độ.'):(s.fan?'Quạt được bật thủ công.':'Quạt được tắt thủ công.');
$('toggle').textContent=s.fan?'Tắt quạt thủ công':'Bật quạt thủ công';$('toggle').disabled=s.mode!=='MANUAL';$('toggle').style.opacity=s.mode==='MANUAL'?1:.5;
$('hint').textContent=s.mode==='AUTO'?'AUTO: bật ở '+s.onTemp+'°C, tắt ở '+s.offTemp+'°C.':'MANUAL: điều khiển quạt bằng công tắc trên dashboard.';
$('threshold').textContent='Bật ≥ '+s.onTemp+'°C · Tắt ≤ '+s.offTemp+'°C';
$('tempstatus').textContent=s.sensorError?'LỖI CẢM BIẾN · DHT22':(s.temp>=s.onTemp?'● Nhiệt độ cao':s.temp<=s.offTemp?'● Trong ngưỡng':'● Đang theo dõi')+' · DHT22';
$('dhtTag').textContent=s.sensorError?'ERROR':'ONLINE';$('dhtTag').classList.toggle('off',s.sensorError);
$('redtag').textContent=s.fan?'ON':'OFF';$('redtag').classList.toggle('off',!s.fan);$('greentag').textContent=s.fan?'OFF':'ON';$('greentag').classList.toggle('off',s.fan);
if(s.temp!==null&&s.hum!==null){history.push({t:s.temp,h:s.hum});if(history.length>24)history.shift();draw()}
let key=s.mode+'-'+s.fan+'-'+s.sensorError;if(lastState&&lastState!==key)log('Trạng thái cập nhật: '+s.mode+' · Quạt '+(s.fan?'BẬT':'TẮT'));lastState=key;
}catch(e){$('conn').textContent='Mất kết nối';$('status').textContent='KHÔNG KẾT NỐI';$('status').classList.add('bad')}}
document.querySelectorAll('#navbar button').forEach(b=>b.addEventListener('click',()=>{document.querySelectorAll('#navbar button').forEach(x=>x.classList.toggle('active',x===b));let tab=b.dataset.tab;['overview','control','devices','light','security','logsPanel','tab-metrics'].forEach(id=>{let e=$(id);if(e)e.classList.add('tab-hidden')});if(tab==='overview'){['overview','tab-metrics','control','devices','light','security','logsPanel'].forEach(id=>$(id)?.classList.remove('tab-hidden'))}else if(tab==='control')$('control').classList.remove('tab-hidden');else if(tab==='light')$('light').classList.remove('tab-hidden');else if(tab==='security')$('security').classList.remove('tab-hidden');else if(tab==='logs')$('logsPanel').classList.remove('tab-hidden')}));
$('clock').textContent='';setInterval(()=>{$('clock').textContent=new Date().toLocaleTimeString('vi-VN',{hour12:false})},1000);window.addEventListener('resize',draw);poll();setInterval(poll,2000);
</script></body></html>
)HTML";

bool namTrongKhoangHenGio(const String& bat, const String& tat) {
  struct tm ti; if (!getLocalTime(&ti, 5)) return false;
  int a=bat.substring(0,2).toInt()*60+bat.substring(3,5).toInt();
  int b=tat.substring(0,2).toInt()*60+tat.substring(3,5).toInt();
  int n=ti.tm_hour*60+ti.tm_min;
  if(a<b)return n>=a&&n<b; if(a>b)return n>=a||n<b; return false;
}

void capNhatDenVong() {
  giaTriAnhSang = digitalRead(LIGHT_SENSOR_PIN);
  troiToi = DO_LOW_KHI_TOI ? (giaTriAnhSang == LOW) : (giaTriAnhSang == HIGH);
  bool suKienConHieuLuc = (millis() - denSuKienLuc) < THOI_GIAN_DEN_SAU_SU_KIEN;
  bool trongLich = henGioLEDEnabled && namTrongKhoangHenGio(gioBatLED, gioTatLED);
  bool canBatDen = henGioLEDEnabled ? trongLich : (troiToi || suKienConHieuLuc);
  if (cheDoVongLED == "ON") canBatDen = true;
  else if (cheDoVongLED == "OFF") canBatDen = false;
  if (trongLich != trangThaiHenGioTruoc) { trangThaiHenGioTruoc=trongLich; Serial.printf("[HEN_GIO] %s; lich %s-%s\n",trongLich?"BAT LED":"TAT LED",gioBatLED.c_str(),gioTatLED.c_str()); }
  denDangBat = canBatDen;
  uint32_t mau = dangBaoDong ? ring.Color(255, 0, 0) : ring.Color(255, 210, 120);
  for (int i = 0; i < RING_LED_COUNT; i++) ring.setPixelColor(i, canBatDen ? mau : 0);
  ring.show();
}

void ghiNhanSuKienDen() {
  denSuKienLuc = millis();
}

void capNhatTrangThai(bool batQuat) {
  if (quatDangBat != batQuat) Serial.printf("[QUAT][%s] Relay GPIO26; LED do GPIO25=%s; LED xanh GPIO33=%s\n",batQuat?"BAT":"TAT",batQuat?"ON":"OFF",batQuat?"OFF":"ON");
  quatDangBat = batQuat;
  digitalWrite(RELAY_PIN, batQuat ? RELAY_ON : RELAY_OFF);
  digitalWrite(LED_DO, batQuat ? HIGH : LOW);
  digitalWrite(LED_XANH, batQuat ? LOW : HIGH);
}

void veKhungTrangThai(const char* nhan) {
  // Tieu de ngan, khong chen lan voi du lieu ben duoi.
  oled.drawRFrame(0, 0, 128, 64, 4);
  oled.setFont(u8g2_font_5x8_tf);
  oled.drawStr(7, 10, "SMART FAN");
  oled.drawStr(77, 10, nhan);
  oled.drawHLine(5, 14, 118);
}

void hienThi(float t, float h) {
  oled.clearBuffer();
  oled.drawRFrame(0, 0, 128, 64, 3);
  oled.setFont(u8g2_font_6x10_tf);
  oled.drawStr(6, 10, "ESP32 SMART FAN");
  oled.drawHLine(5, 14, 118);
  oled.setFont(u8g2_font_5x8_tf);
  oled.drawStr(7, 25, "Nhiet do:");
  oled.setFont(u8g2_font_6x10_tf);
  if (isnan(t)) oled.drawStr(65, 25, "--.- C");
  else { oled.setCursor(65,25); oled.print(t,1); oled.print(" C"); }
  oled.setFont(u8g2_font_5x8_tf);
  oled.drawStr(7, 36, "Do am:");
  oled.setFont(u8g2_font_6x10_tf);
  if (isnan(h)) oled.drawStr(65,36,"-- %");
  else { oled.setCursor(65,36); oled.print((int)round(h)); oled.print(" %"); }
  oled.drawHLine(5, 40, 118);
  oled.setFont(u8g2_font_5x8_tf);
  oled.drawStr(7, 50, quatDangBat ? "QUAT: BAT" : "QUAT: TAT");
  oled.drawStr(76, 50, cheDoTuDong ? "AUTO" : "MANUAL");
  oled.drawStr(7, 60, baoVeBat ? (dangBaoDong ? "BAO DONG!" : "BAO VE: BAT") : "BAO VE: TAT");
  oled.drawStr(78, 60, denDangBat ? "LED: BAT" : "LED: TAT");
  oled.sendBuffer();
}

void hienThiLoiCamBien() {
  oled.clearBuffer();
  oled.drawRFrame(0, 0, 128, 64, 4);
  oled.setFont(u8g2_font_6x10_tf);
  oled.drawStr(8, 13, "CANH BAO CAM BIEN");
  oled.drawHLine(6, 18, 116);
  oled.setFont(u8g2_font_unifont_t_vietnamese2);
  oled.drawUTF8(8, 36, "Lỗi cảm biến");
  oled.setFont(u8g2_font_5x8_tf);
  oled.drawStr(8, 49, "Kiem tra DHT");
  oled.drawStr(8, 59, "DATA / VCC / GND");
  oled.sendBuffer();
}

void docCamBien() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  camBienLoi = isnan(t) || isnan(h);
  if (camBienLoi) {
    // An toan cho ban demo: neu mat cam bien, tat quat.
    capNhatTrangThai(false);
    hienThiLoiCamBien();
    Serial.println("[LOI][CAM_BIEN] Khong doc duoc DHT11; quat tat de an toan.");
    return;
  }
  nhietDo = t; doAm = h;
  if (cheDoTuDong) {
    if (!quatDangBat && nhietDo >= NHiet_DO_BAT) capNhatTrangThai(true);
    else if (quatDangBat && nhietDo <= NHiet_DO_TAT) capNhatTrangThai(false);
  }
  hienThi(nhietDo, doAm);
  if (millis()-lanGhiLogSensor >= 10000) { lanGhiLogSensor=millis(); Serial.printf("[CAM_BIEN] DHT22 T=%.1f C H=%.1f%%; QUAT=%s; MODE=%s; LED_RING=%s\n",nhietDo,doAm,quatDangBat?"BAT":"TAT",cheDoTuDong?"AUTO":"MANUAL",denDangBat?"BAT":"TAT"); }
  Serial.printf("Nhiet do: %.1f C | Do am: %.1f %% | Quat: %s | Mode: %s\n",
                nhietDo, doAm, quatDangBat ? "BAT" : "TAT",
                cheDoTuDong ? "AUTO" : "MANUAL");
}

void handleState() {
  String json = "{";
  json += "\"temp\":" + String(camBienLoi ? "null" : String(nhietDo, 1)) + ",";
  json += "\"hum\":" + String(camBienLoi ? "null" : String(doAm, 1)) + ",";
  json += "\"fan\":" + String(quatDangBat ? "true" : "false") + ",";
  json += "\"mode\":\"" + String(cheDoTuDong ? "AUTO" : "MANUAL") + "\",";
  json += "\"onTemp\":" + String(NHiet_DO_BAT, 1) + ",";
  json += "\"offTemp\":" + String(NHiet_DO_TAT, 1) + ",";
  json += "\"sensorError\":" + String(camBienLoi ? "true" : "false") + ",";
  json += "\"armed\":" + String(baoVeBat ? "true" : "false") + ",";
  json += "\"alarm\":" + String(dangBaoDong ? "true" : "false") + ",";
  json += "\"motion\":" + String(coChuyenDong ? "true" : "false") + ",";
  json += "\"lightRaw\":" + String(giaTriAnhSang) + ",";
  json += "\"dark\":" + String(troiToi ? "true" : "false") + ",";
  json += "\"ringOn\":" + String(denDangBat ? "true" : "false") + ",";
  json += "\"ringMode\":\"" + cheDoVongLED + "\",";
  json += "\"scheduleEnabled\":" + String(henGioLEDEnabled ? "true" : "false") + ",";
  json += "\"scheduleStart\":\"" + gioBatLED + "\",";
  json += "\"scheduleEnd\":\"" + gioTatLED + "\",";
  json += "\"cardMessage\":\"" + thongBaoThe + "\"";
  json += "}";
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}

void handleControl() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"Thieu JSON body\"}");
    return;
  }
  String body = server.arg("plain");
  if (body.indexOf("\"scheduleStart\"") >= 0 && body.indexOf("\"scheduleEnd\"") >= 0) {
    int p1=body.indexOf("\"scheduleStart\""); int c1=body.indexOf(':',p1); int q1=body.indexOf('"',c1+1); int q2=body.indexOf('"',q1+1);
    int p2=body.indexOf("\"scheduleEnd\""); int c2=body.indexOf(':',p2); int q3=body.indexOf('"',c2+1); int q4=body.indexOf('"',q3+1);
    if(q1>=0&&q2>q1&&q3>=0&&q4>q3){String a=body.substring(q1+1,q2),b=body.substring(q3+1,q4);if(a.length()==5&&b.length()==5&&a.charAt(2)==':'&&b.charAt(2)==':'){gioBatLED=a;gioTatLED=b;Serial.printf("[HEN_GIO] Da luu lich %s -> %s\n",gioBatLED.c_str(),gioTatLED.c_str());}}
  }
  if(body.indexOf("\"scheduleEnabled\":true")>=0){henGioLEDEnabled=true;Serial.println("[HEN_GIO] Bat lich LED");}
  if(body.indexOf("\"scheduleEnabled\":false")>=0){henGioLEDEnabled=false;Serial.println("[HEN_GIO] Tat lich LED");}
  if (body.indexOf("\"ringMode\":\"AUTO\"") >= 0) cheDoVongLED = "AUTO";
  else if (body.indexOf("\"ringMode\":\"ON\"") >= 0) { cheDoVongLED = "ON"; denSuKienLuc = millis(); }
  else if (body.indexOf("\"ringMode\":\"OFF\"") >= 0) cheDoVongLED = "OFF";
  if (body.indexOf("\"mode\":\"AUTO\"") >= 0) {
    cheDoTuDong = true;
    if (!camBienLoi) {
      if (nhietDo >= NHiet_DO_BAT) capNhatTrangThai(true);
      else if (nhietDo <= NHiet_DO_TAT) capNhatTrangThai(false);
    }
  } else if (body.indexOf("\"mode\":\"MANUAL\"") >= 0) {
    cheDoTuDong = false;
  }
  if (body.indexOf("\"fan\":true") >= 0) {
    cheDoTuDong = false;
    capNhatTrangThai(true);
  } else if (body.indexOf("\"fan\":false") >= 0) {
    cheDoTuDong = false;
    capNhatTrangThai(false);
  }
  hienThi(nhietDo, doAm);
  server.send(200, "application/json", "{\"ok\":true}");
}


String uidTheDangQuet() {
  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
    if (i + 1 < rfid.uid.size) uid += " ";
  }
  uid.toUpperCase();
  return uid;
}

void tatCoi() {
  // Arduino-ESP32 Core 3.x: dung PWM am thanh ngay lap tuc
  ledcWriteTone(BUZZER_PIN, 0);
  digitalWrite(BUZZER_PIN, LOW);
}

void tatBaoDong() {
  dangBaoDong = false;
  tatCoi();
}

void capNhatCoiHuy() {
  if (!dangBaoDong || millis() - lanBaoDong >= THOI_GIAN_HU_TOI_DA) {
    tatCoi();
    return;
  }

  // Huy len xuong cao do theo chu ky 1.6 giay, khong dung delay()
  // de website, RFID va PIR van duoc xu ly lien tuc.
  const unsigned long CHU_KY_HUY = 1600;
  const unsigned long nuaChuKy = CHU_KY_HUY / 2;
  unsigned long pha = (millis() - lanBaoDong) % CHU_KY_HUY;
  unsigned int tanSo;
  if (pha < nuaChuKy) {
    tanSo = map(pha, 0, nuaChuKy, 900, 2400);
  } else {
    tanSo = map(pha - nuaChuKy, 0, nuaChuKy, 2400, 900);
  }
  ledcWriteTone(BUZZER_PIN, tanSo);
}

void xuLyTheNFC() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;
  String uid = uidTheDangQuet();
  Serial.print("[RFID] UID: "); Serial.println(uid);
  if (millis() - lanQuetThe > THOI_GIAN_CHONG_NHAY_THE) {
    lanQuetThe = millis();
    if (String(UID_THE_HOP_LE) == uid) {
      ghiNhanSuKienDen();
      baoVeBat = !baoVeBat;
      if (baoVeBat) {
        thongBaoThe = "THE HOP LE - BAO VE BAT"; hetThongBaoThe = millis() + 5000;
        batBaoVeLuc = millis();
        tatBaoDong();
        Serial.println("The hop le: DA BAT BAO VE; cho 10 giay");
      } else {
        thongBaoThe = "THE HOP LE - TAT 15S"; hetThongBaoThe = millis() + 5000;
        tatBaoDong();
        choTuBatBaoVe = true;
        lucTatBaoVe = millis();
        Serial.println("The hop le: DA TAT BAO VE; se tu bat lai sau 15 giay");
      }
    } else {
      thongBaoThe = "THE KHONG HOP LE"; hetThongBaoThe = millis() + 5000;
      Serial.println("The khong hop le. Khong thay doi che do bao ve.");
    }
  }
  hienThi(nhietDo, doAm);
  oled.setDrawColor(0); oled.drawBox(2, 39, 124, 23); oled.setDrawColor(1);
  oled.drawRFrame(2, 39, 124, 23, 3); oled.setFont(u8g2_font_5x8_tf);
  if (uid == String(UID_THE_HOP_LE)) oled.drawStr(7, 49, "THE HOP LE - DA CAP NHAT");
  else oled.drawStr(7, 49, "THE KHONG HOP LE");
  oled.drawStr(7, 58, "Xem trang thai bao ve tren web"); oled.sendBuffer();
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

void xuLyAnNinh() {
  xuLyTheNFC();

  // Neu vua quet the de tat bao ve, tu bat lai sau 15 giay.
  if (choTuBatBaoVe && millis() - lucTatBaoVe >= THOI_GIAN_TU_BAT_BAO_VE) {
    choTuBatBaoVe = false;
    baoVeBat = true;
    batBaoVeLuc = millis();
    tatBaoDong();
    thongBaoThe = "TU DONG BAT BAO VE";
    hetThongBaoThe = millis() + 5000;
    Serial.println("Da tu dong BAT LAI BAO VE sau 15 giay; cho 10 giay truoc khi giam sat PIR");
  }

  if (millis() - lanDocPIR >= THOI_GIAN_CHONG_NHAY_PIR) {
    lanDocPIR = millis();
    bool chuyenDongCu = coChuyenDong;
    coChuyenDong = digitalRead(PIR_PIN) == HIGH;
    // Bat den 60 giay moi khi PIR phat hien chuyen dong, ke ca khi chua bat che do bao ve.
    if (coChuyenDong && !chuyenDongCu) ghiNhanSuKienDen();
  }
  if (baoVeBat && millis() - batBaoVeLuc >= THOI_GIAN_CHO_BAO_VE && coChuyenDong && !dangBaoDong) {
    dangBaoDong = true;
    ghiNhanSuKienDen();
    lanBaoDong = millis();
    Serial.println("CANH BAO: PHAT HIEN CHUYEN DONG!");
  }
  // Buzzer thu dong: huy thay doi cao do, toi da 30 giay moi lan bao dong.
  // Tat bao ve bang the/website se goi tatBaoDong() va dung coi ngay.
  capNhatCoiHuy();
}

void handleSecurity() {
  if (!server.hasArg("plain")) { server.send(400, "application/json", "{\"ok\":false}"); return; }
  String body = server.arg("plain");
  if (body.indexOf("\"armed\":true") >= 0) {
    choTuBatBaoVe = false;
    baoVeBat = true; batBaoVeLuc = millis(); tatBaoDong();
  } else if (body.indexOf("\"armed\":false") >= 0) {
    choTuBatBaoVe = false;
    baoVeBat = false; tatBaoDong();
  }
  server.send(200, "application/json", "{\"ok\":true}");
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_DO, OUTPUT);
  pinMode(LED_XANH, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_OFF);
  digitalWrite(LED_DO, LOW);
  digitalWrite(LED_XANH, HIGH);

  Wire.begin(21, 22);
  oled.begin();
  oled.enableUTF8Print();
  dht.begin();
  pinMode(PIR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  // Arduino-ESP32 Core 3.x: gan PWM cho buzzer thu dong GPIO14.
  // Dung ledcWriteTone(GPIO, 0) de tat tieng.
  if (!ledcAttach(BUZZER_PIN, 2000, 8)) {
    Serial.println("LOI: Khong the gan PWM cho buzzer GPIO14");
  }
  ledcWriteTone(BUZZER_PIN, 0);
  pinMode(LIGHT_SENSOR_PIN, INPUT);
  ring.begin();
  ring.setBrightness(45); // giam do sang va dong dien
  ring.clear();
  ring.show();
  SPI.begin(18, 19, 23, RFID_SS_PIN); // SCK, MISO, MOSI, SS
  rfid.PCD_Init();
  Serial.println("RC522 da khoi dong. Quet the de xem UID; them UID vao UID_THE_HOP_LE roi nap lai.");
  oled.clearBuffer();
  oled.drawRFrame(0, 0, 128, 64, 5);
  oled.setFont(u8g2_font_6x10_tf);
  oled.drawStr(17, 20, "ESP32 SMART FAN");
  oled.drawHLine(10, 27, 108);
  oled.setFont(u8g2_font_unifont_t_vietnamese2);
  oled.drawUTF8(13, 43, "Đang kết nối");
  oled.setFont(u8g2_font_5x8_tf);
  oled.drawStr(34, 56, "WI-FI...");
  oled.sendBuffer();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Dang ket noi Wi-Fi");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 25000) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    configTime(7 * 3600, 0, "pool.ntp.org", "time.google.com", "time.nist.gov");
    Serial.println("\nWi-Fi da ket noi; dang dong bo gio NTP");
    Serial.print("Mo dashboard tai: http://");
    Serial.println(WiFi.localIP());
    oled.clearBuffer();
    oled.drawRFrame(0, 0, 128, 64, 5);
    oled.setFont(u8g2_font_unifont_t_vietnamese2);
    oled.drawUTF8(8, 14, "Đã kết nối");
    oled.drawHLine(7, 18, 114);
    oled.setFont(u8g2_font_5x8_tf);
    oled.drawStr(9, 30, "DIA CHI WEBSITE");
    oled.setFont(u8g2_font_6x10_tf);
    oled.setCursor(9, 43); oled.print(WiFi.localIP());
    oled.setFont(u8g2_font_5x8_tf);
    oled.drawStr(9, 56, "MO IP TREN TRINH DUYET");
    oled.sendBuffer();
  } else {
    Serial.println("\nKhong ket noi duoc Wi-Fi. Kiem tra SSID/mat khau.");
    oled.clearBuffer();
    oled.drawRFrame(0, 0, 128, 64, 5);
    oled.setFont(u8g2_font_unifont_t_vietnamese2);
    oled.drawUTF8(8, 17, "Không kết nối");
    oled.drawHLine(7, 22, 114);
    oled.setFont(u8g2_font_unifont_t_vietnamese2);
    oled.drawUTF8(10, 40, "Kiểm tra Wi-Fi");
    oled.setFont(u8g2_font_5x8_tf);
    oled.drawStr(10, 54, "SSID / MAT KHAU");
    oled.sendBuffer();
  }

  server.on("/", HTTP_GET, []() { server.send_P(200, "text/html; charset=utf-8", PAGE); });
  server.on("/api/state", HTTP_GET, handleState);
  server.on("/api/control", HTTP_POST, handleControl);
  server.on("/api/security", HTTP_POST, handleSecurity);
  server.onNotFound([]() { server.send(404, "text/plain", "Not found"); });
  server.begin();
  Serial.println("HTTP server da khoi dong");
  lanDocCuoi = millis() - CHU_KY_DOC;
}

void loop() {
  server.handleClient();
  xuLyAnNinh();
  static unsigned long lanCapNhatDen = 0;
  if (millis() - lanCapNhatDen >= 250) { lanCapNhatDen = millis(); capNhatDenVong(); }
  if (millis() - lanDocCuoi >= CHU_KY_DOC) {
    lanDocCuoi = millis();
    docCamBien();
  }
}
