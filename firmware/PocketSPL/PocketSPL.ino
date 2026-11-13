/*
  PocketSPL - DIY pocket SPL meter (Feather + phone)
  ---------------------------------------------------------
  Board:    Adafruit ESP32-S3 Feather 8MB Flash No PSRAM (Core ADA5323)      <- production meters
            Adafruit ESP32-S3 Feather 4MB Flash / 2MB PSRAM (Core ADA5477)  <- also works (e.g. your own)
            Same pins, same code. Pick the matching Board + Partition Scheme below when you load it.
  Display:  1.5" 128x128 colour OLED, SSD1351      (Jaycar XC3726)
  Mic:      Adafruit ICS-43434 I2S MEMS mic          (Core ADA6049)
  Power:    2000mAh LiPo in the Feather's battery socket, slide switch EN -> GND = off

  Arduino IDE setup
    - Boards Manager: "esp32" by Espressif (3.x)
    - 8MB board: Board "Adafruit Feather ESP32-S3 No PSRAM",
                 Tools > Partition Scheme "Default (3MB APP/1.5MB SPIFFS)"   (3 MB for software, 1.5 MB of logs)
    - 4MB board: Board "Adafruit Feather ESP32-S3 2MB PSRAM",
                 Tools > Partition Scheme "Minimal SPIFFS (1.9MB APP with OTA/128KB SPIFFS)"
    Both leave room for updates from the phone. Each update file is made for one board size:
    the meter refuses a file for the other size, so a wrong file can't stop it starting.
    - Library Manager: "Adafruit SSD1351 library" (installs Adafruit GFX too)
    Nothing else to install: WiFi, the web server and the battery gauge use built-in code.

  WIRING (16 jumper wires, each from a breadboard hole beside a Feather pin to a part)
    Seen from the front, USB end at the bottom: row 1 = the row at the USB end.
    The Feather's 16-pin side is on the RIGHT (pins in the 3rd hole from the right edge),
    its 12-pin side on the LEFT (2nd hole from the left edge).
     1  row 2  (3V)   far-right       -> screen VCC
     2  row 2  (3V)   second-right    -> mic 3V
     3  row 4  (GND)  far-right       -> screen GND
     4  row 4  (GND)  second-right    -> switch outer leg
     5  row 5  (A0)   far-right       -> mic GND   (held LOW in code, acts as the mic's ground)
     6  row 6  (A1)   far-right       -> mic BCLK
     7  row 7  (A2)   far-right       -> mic LRCL
     8  row 8  (A3)   far-right       -> screen CS
     9  row 9  (A4)   far-right       -> screen DC
    10  row 10 (A5)   far-right       -> screen RES
    11  row 11 (SCK)  far-right       -> screen SCL
    12  row 12 (MO)   far-right       -> screen SDA
    13  row 13 (MI)   far-right       -> mic DOUT
    14  row 14 (RX)   far-right       -> button leg
    15  row 15 (TX)   far-right       -> button leg diagonally opposite (held HIGH in code)
    16  row 6  (EN)   far-left        -> switch middle leg
    Mic SEL: nothing (the mic board pulls it to ground itself).

  GIG FEATURES
    - Rolling LAeq over the last 1, 5 and 15 minutes (also in dB C)
    - Limit alarm: default 100 dB(A) LAeq over 15 min (WHO safe listening venues). Amber border when within 3 dB, red when over.
    - Gig log: every minute LAeq, LCeq, LAFmax and LCpeak, up to 24 hours, with markers.
      Download it from the phone as a spreadsheet (CSV). The log lives in memory: download it
      before you switch the meter off.
    - Hearing dose: % of the Australian workplace daily limit (85 dB(A) over 8 hours).
    - Screen dims after a while with no button press (first press just wakes it).
    - GIG screen on the meter (hold 1 s to step to it).

  SCREEN LOOK
    The "Clean" look is the new one: black screen, colour only for the level, smooth text and
    shapes, and a reading that glides instead of jumping (the screen only: MAX, Leq, the limit,
    alerts and the log always use the true value). Studio and Bold are still on the phone
    (Settings > General > Look). Loading this version switches the meter to Clean once.

  CONTROLS (one button)
    Double-click, then tap : dB A -> dB C -> dB Z   (locks again 3 s after the last tap)
    Hold 1 s, let go       : FAST -> SLOW -> LEQ -> RTA -> GIG   (LEQ restarts LEQ and MAX)
    Hold 3 s               : WiFi on / off
    A single tap on its own does nothing, so a knock can't change anything.

  MODEL, SOFTWARE VERSION, SERIAL NUMBER
    MODEL_NO   (below) is the model number, "PS-1". It stays the same for every meter of this design.
    Two versions share this one software file and the same case:
      PS-1   battery model (2000mAh LiPo)
      PS-1W  wired model: no battery, powered from a USB plug pack, made to run nonstop in a venue.
             Set it once per meter in the Serial Monitor:  model wired   (or  model battery  to go back).
             It's kept through factory reset. The wired model:
               - hides the battery icon and never switches itself off for a flat battery
               - switches the screen OFF after 10 minutes without a press (an OLED left on the same
                 picture for weeks burns in). Any press wakes it. It also wakes by itself while the
                 level is over the limit alarm. Screen dimming set to Never keeps it on.
               - saves the running log every 5 minutes (not every minute), so the flash lasts for years
               - when a night is over (at least an hour of 70 dB(A)+, then 2 hours under 60 dB(A)),
                 saves that night as its own log and starts a fresh one. Up to 30 are kept (8MB board).
               - ON/OFF SCHEDULE (phone: Settings > General > On/off schedule), e.g.
                 Fri 6:30 pm - 2:00 am and Sat 8:55 pm - 3:00 am. Outside those times it goes to
                 standby: no logging, screen dark, WiFi stays on so the phone can still reach it.
                 A press shows when it comes on next. Each session is saved as its own log.
                 The clock: set by the phone every time the page opens, and from the internet
                 (pool.ntp.org) when it's on the venue's WiFi. Daylight saving is handled.
                 No clock yet (power cut, no phone, no internet) = it stays ON, so nothing is missed.

  ALERTS (both models; phone: Settings > General > Alerts)
    Sends a notification when the level stays over the limit alarm (its level, 1/5/15 min average and
    LAeq/LCeq), after it has been over for a set time, repeating every 15/30/60 min while it stays over,
    and optionally when it's back under. Needs the meter on a WiFi network with internet.
    Sent through ntfy.sh (free, no account): install the ntfy app, add the meter's topic, done.
    FW_VERSION (below) is the software version: change it for every release (e.g. "1.0.1").
    Serial number: made automatically from the chip (PSPL- + 6 characters, different on every meter),
    or set your own with "sn 0001" in the Serial Monitor.
  SOFTWARE VERSION: change FW_VERSION below for every release (e.g. "1.0.1"). See RELEASING in the
    build guide. Tools > Partition Scheme must be the one above for the board so the meter
    has room to update itself from the phone.

  PHONE
    WiFi network "PocketSPL", password "pocketspl", then open http://192.168.4.1
    The name and password can be changed on the phone (Settings > Meter's own WiFi).
    Forgot them? Switch off, hold the button, switch on and keep holding for 5 seconds:
    the meter's WiFi goes back to PocketSPL / pocketspl.
    Better range: in the phone page's Network section, add your own WiFi (home, studio, travel
    router). From then on the meter joins it at switch-on and shows its address, e.g. 192.168.1.47
    (an iPhone can also use http://pocketspl.local). No saved network in range = its own network again.
    Networks with no DHCP server: set a fixed address for that network (Network > Address) on the phone.
    Shows the live level, 60 s graph, RTA (31 bands, 1/3 octave), MAX and Leq, and every setting.
    Tip: in Safari, Share > Add to Home Screen makes it open like an app.

  SERIAL MONITOR (115200), type a command and press Enter:
    test          production check: screen, button, mic, battery gauge, WiFi, storage, update space
    cal94         94 dB calibration with a calibrator on the mic
    cal 2.5       set the offset by hand        cal?   show it
    factory cal   save the current offset as this unit's factory calibration
    sn 0007       set the serial number to PSPL-0007   (sn auto = use the chip's own number)
    help          list the commands
*/

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <Update.h>
#include <LittleFS.h>
#include "esp_wifi.h"
#include "esp_ota_ops.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <math.h>
#include <complex>
#include "driver/i2s.h"
#ifdef ARDUINO_ARCH_ESP32
#include <NetworkClientSecure.h>           // alerts: HTTPS to ntfy.sh (built-in root certificates)
#include <HTTPClient.h>
#endif

// ---------------- Pins (Feather ESP32-S3 GPIO numbers) ----------------
#define PIN_MIC_GND   18   // A0  - held LOW, mic ground
#define PIN_I2S_BCLK  17   // A1
#define PIN_I2S_WS    16   // A2
#define PIN_I2S_DIN   37   // MI
#define PIN_OLED_CS   15   // A3
#define PIN_OLED_DC   14   // A4
#define PIN_OLED_RST   8   // A5
#define PIN_OLED_SCK  36   // SCK
#define PIN_OLED_MOSI 35   // MO
#define PIN_BTN       38   // RX  - button input, internal pull-down
#define PIN_BTN_HIGH  39   // TX  - held HIGH, feeds the other side of the button
#define PIN_NEO_POWER 21   // on-board NeoPixel power (switched off to save battery)
#define OLED_SPI_HZ  8000000   // screen speed: 8 MHz runs clean on the new screen (drop to 4000000 if it ever scrambles)

// ---------------- Settings ----------------
#define FW_VERSION "1.1.1"                 // software (firmware) version: change this for every release
#define MODEL_NO   "PS-1"                  // model number: only change it for a new model with different hardware
const char FW_TAG[] = "POCKETSPL-FW " MODEL_NO " " FW_VERSION;   // marks this file as PocketSPL software for this model
#if defined(ARDUINO_ADAFRUIT_FEATHER_ESP32S3_NOPSRAM)
#define BOARD_FLASH "8MB"                   // built for the 8MB Feather (ADA5323)
#else
#define BOARD_FLASH "4MB"                   // built for the 4MB / 2MB PSRAM Feather (ADA5477)
#endif
const char FLASH_TAG[] = "POCKETSPL-FLASH " BOARD_FLASH ";";   // update files carry this: the wrong size is refused
bool wiredModel = false;                    // PS-1W: no battery, USB powered nonstop (set with "model wired", kept in prefs)
#define modelName() (wiredModel ? MODEL_NO "W" : MODEL_NO)   // macro, not a function: keeps Arduino's prototypes after the types
const uint32_t WIRED_SCREEN_OFF_MS = 10UL * 60 * 1000;     // wired model: screen off after 10 min without a press
const uint32_t WIRED_LOG_SAVE_MS   = 5UL * 60 * 1000;      // wired model: save the running log every 5 min
const int NIGHT_LOUD_DB10 = 700, NIGHT_LOUD_MIN = 60;      // a night = at least 60 minutes at 70 dB(A)+ ...
const int NIGHT_QUIET_DB10 = 600, NIGHT_QUIET_MIN = 120;   // ... and it's over after 120 minutes under 60 dB(A)
// clock + on/off schedule (wired model), see "Clock + on/off schedule" further down
int64_t clockBaseMs = 0; uint32_t clockBaseMillis = 0; bool clockSet = false;
String tzPosix = "AEST-10";                 // time zone rule (POSIX), from the phone; default Queensland
const int SCH_MAX = 8;                      // up to 8 sessions a week
int schN = 0; uint8_t schDay[SCH_MAX]; uint16_t schOnM[SCH_MAX], schOffM[SCH_MAX];   // day 0 = Sunday, minutes from midnight
bool schEnabled = false;
volatile bool standby = false;              // outside the schedule: no logging, screen dark
uint32_t standbyShowUntil = 0;              // a press in standby shows the standby screen for a few seconds
int weekMinuteNow();
void fmtWeekMinute(int wm, char* out, size_t n);
String scheduleString();
bool scheduleActive();
int nextChange(int wm);
bool parseSchedule(const String& in);
// alerts (see "Alerts" further down)
bool alOn = false, alBack = false;
String alTopic, alLast;                     // ntfy topic, result of the last send
int alHold = 5, alRep = 30;                 // minutes over before the first alert / minutes between repeats
uint32_t alOverSince = 0, alLastAt = 0; bool alAlerted = false;
volatile int alResult = 0;                  // set by the sending task: HTTP status, or <0 = failed
volatile bool alBusy = false;
bool netOnline();
bool validTopic(const String& t);
String newTopic();
void alertWhen(char* out, size_t n);
bool sendAlert(const char* title, const char* body, const char* prio);
const char*   DEFAULT_WIFI_NAME = "PocketSPL";
const char*   DEFAULT_WIFI_PASS = "pocketspl";     // 8+ characters
String apName = DEFAULT_WIFI_NAME, apPass = DEFAULT_WIFI_PASS;   // the meter's own WiFi (changeable on the phone)
String serialNo;                            // e.g. PSPL-0007 (set with "sn" in the Serial Monitor) or from the chip
float factoryCal = 0; bool factoryCalSet = false;   // this unit's calibration from the bench, kept through factory reset
String lockPin;                             // settings lock PIN, "" = not locked
String latestFw;                            // newest version the phone has seen online ("" = not known)
bool fsOk = false;                          // flash storage for saved gig logs is working
volatile uint32_t micFlatSince = 0;         // when the mic signal went completely flat (0 = it's fine)
bool lastDenied = false, lastPinOk = false; // for the next reply to the phone
const float   DEFAULT_CAL_OFFSET = 0.0f;
const uint8_t DEFAULT_BRIGHTNESS = 10;     // 1..15
const uint8_t DEFAULT_THEME = 0;           // 0 = Clean (the new look), 1 = Studio, 2 = Bold (changeable on the phone)
const int     DEFAULT_AMBER = 85, DEFAULT_RED = 95;
const float   BAR_MIN_DB = 60.0f, BAR_MAX_DB = 110.0f;
const int     BATT_LOW_PCT = 15, BATT_CRIT_PCT = 5;
const int     BATT_CHARGE_RED_PCT = 20;   // while charging: red below this, green from here up
const bool    USE_MIC_EQ = true;

// If the reading sits near the bottom (~30 dB) whatever you do, change to I2S_CHANNEL_FMT_ONLY_RIGHT.
#define MIC_CHANNEL_FMT I2S_CHANNEL_FMT_ONLY_LEFT

// ---------------- Constants ----------------
const int   FS = 48000;
const int   BUF_LEN = 256;
const float MIC_REF_DB = 123.01f;   // ICS-43434: -26 dBFS @ 94 dB SPL, +3.01 for sine RMS
const float TAU_FAST = 0.125f, TAU_SLOW = 1.0f;
const float OVERLOAD_DB = 119.5f;

enum Weighting { W_A = 0, W_C = 1, W_Z = 2 };
enum Response  { R_FAST = 0, R_SLOW = 1, R_LEQ = 2, R_RTA = 3, R_GIG = 4 };   // RTA = spectrum, GIG = limit / dose screen
const char* W_NAME[] = {"dB(A)", "dB(C)", "dB(Z)"};
const char* R_NAME[] = {"Fast", "Slow", "Leq", "RTA", "Gig"};
const int   NUM_R = 5;

// Declared up here so the Arduino IDE can see it before any function uses it
struct Button { uint8_t pin; bool down = false; uint32_t downAt = 0; uint32_t changedAt = 0; bool longDone = false; };

// ---------------- Biquad + filter design ----------------
struct Biquad {
  float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
  inline float process(float x) {
    float y = b0 * x + z1;
    z1 = b1 * x - a1 * y + z2;
    z2 = b2 * x - a2 * y;
    return y;
  }
};

static double prewarp(double f) { return 2.0 * FS * tan(M_PI * f / FS); }

static Biquad bilinear(double nb0, double nb1, double nb2, double da0, double da1, double da2) {
  double K = 2.0 * FS, K2 = K * K;
  double B0 = nb0 * K2 + nb1 * K + nb2, B1 = 2 * nb2 - 2 * nb0 * K2, B2 = nb0 * K2 - nb1 * K + nb2;
  double A0 = da0 * K2 + da1 * K + da2, A1 = 2 * da2 - 2 * da0 * K2, A2 = da0 * K2 - da1 * K + da2;
  Biquad q;
  q.b0 = B0 / A0; q.b1 = B1 / A0; q.b2 = B2 / A0; q.a1 = A1 / A0; q.a2 = A2 / A0;
  return q;
}

static void normalise1k(Biquad* s, int n) {
  std::complex<double> z1 = std::polar(1.0, -2 * M_PI * 1000.0 / FS), z2 = z1 * z1, h(1, 0);
  for (int i = 0; i < n; i++)
    h *= ((double)s[i].b0 + (double)s[i].b1 * z1 + (double)s[i].b2 * z2) /
         (1.0 + (double)s[i].a1 * z1 + (double)s[i].a2 * z2);
  double g = std::abs(h);
  s[0].b0 /= g; s[0].b1 /= g; s[0].b2 /= g;
}

Biquad filtA[3], filtC[2], micEq[2];

void designFilters() {
  double w1 = prewarp(20.598997), w2 = prewarp(107.65265);
  double w3 = prewarp(737.86223), w4 = prewarp(12194.217);
  filtA[0] = bilinear(1, 0, 0, 1, 2 * w1, w1 * w1);
  filtA[1] = bilinear(1, 0, 0, 1, w2 + w3, w2 * w3);
  filtA[2] = bilinear(0, 0, 1, 1, 2 * w4, w4 * w4);
  normalise1k(filtA, 3);
  filtC[0] = bilinear(1, 0, 0, 1, 2 * w1, w1 * w1);
  filtC[1] = bilinear(0, 0, 1, 1, 2 * w4, w4 * w4);
  normalise1k(filtC, 2);
  // ICS-43434 response correction at 48 kHz (from the esp32-i2s-slm project)
  micEq[0].b0 = 1; micEq[0].b1 = 0.96986791463971267f; micEq[0].b2 = 0.23515976355743193f;
  micEq[0].a1 = 0.06681948004769928f; micEq[0].a2 = 0.00111521990688725f;
  micEq[1].b0 = 1; micEq[1].b1 = -1.98905931743624453f; micEq[1].b2 = 0.98908924206960169f;
  micEq[1].a1 = -1.99755331853906037f; micEq[1].a2 = 0.99755481510122113f;
  const float g = 0.477326418836803f;
  micEq[0].b0 *= g; micEq[0].b1 *= g; micEq[0].b2 *= g;
  normalise1k(micEq, 2);
}

// ---------------- Display ----------------
Adafruit_SSD1351 oled(128, 128, &SPI, PIN_OLED_CS, PIN_OLED_DC, PIN_OLED_RST);
GFXcanvas16 canvas(128, 128);

static inline uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}
const uint16_t C_BG     = rgb(0, 0, 0);
// Colours: set by the theme (see applyTheme), green / amber / red always mean level.
uint16_t C_TEXT, C_DIM, C_FAINT, C_PANEL, C_ACCENT, C_GREEN, C_AMBER, C_RED;

// ---------------- Fonts ----------------
// Three looks (themes), each with its own fonts, the same as the phone page uses:
//   Clean  = Inter,  Studio = Barlow + DSEG7 (7-segment digits),  Bold = Space Grotesk.
// All SIL Open Font License. Converted to bitmaps for this screen. Barlow is also the start-up logo.
// BarlowLogo: generated from barlow-semi-condensed-600.ttf at 25px
const uint8_t BarlowLogoBitmaps[] PROGMEM = {
  0x00, 0xEE, 0xEE, 0xEE, 0x66, 0x66, 0x66, 0x00, 0x6F, 0xE6, 0xE6, 0xCD, 0x9B, 0x36, 0x6C, 0xC0,
  0x0E, 0x38, 0x38, 0xC1, 0xC7, 0x07, 0x1C, 0x7F, 0xFD, 0xFF, 0xF7, 0xFF, 0xC7, 0x1C, 0x18, 0x60,
  0xE3, 0x8F, 0xFF, 0xBF, 0xFE, 0xFF, 0xF8, 0xE3, 0x83, 0x8E, 0x1C, 0x70, 0x0C, 0x01, 0x80, 0x78,
  0x1F, 0xC7, 0xFD, 0xE3, 0xB8, 0x37, 0x07, 0xE0, 0x0F, 0x01, 0xF8, 0x0F, 0x80, 0xF8, 0x07, 0x80,
  0x7F, 0x07, 0xE0, 0xFE, 0x39, 0xFF, 0x1F, 0x80, 0xC0, 0x18, 0x00, 0x38, 0x1C, 0x7E, 0x0C, 0x31,
  0x8E, 0x10, 0xC6, 0x08, 0x66, 0x06, 0x33, 0x03, 0xF3, 0x00, 0xF1, 0x80, 0x01, 0x80, 0x00, 0xC0,
  0x00, 0xC7, 0x00, 0x67, 0xE0, 0x66, 0x30, 0x33, 0x08, 0x31, 0x86, 0x38, 0xC2, 0x18, 0x33, 0x1C,
  0x0F, 0x00, 0x1F, 0x00, 0xFE, 0x07, 0xFC, 0x1C, 0x70, 0x71, 0xC1, 0xC7, 0x03, 0xBC, 0x0F, 0xE0,
  0x1F, 0x00, 0xF8, 0x07, 0xF1, 0x3C, 0xEE, 0xE3, 0xFB, 0x87, 0xCE, 0x0F, 0x3F, 0xFC, 0x7F, 0xF8,
  0xFC, 0xF0, 0xED, 0xB6, 0x1C, 0xE3, 0x9C, 0x71, 0xCF, 0x38, 0xE3, 0x8E, 0x38, 0xE3, 0x8F, 0x1C,
  0x71, 0xC7, 0x8E, 0x1C, 0xE7, 0x1C, 0xE7, 0x9C, 0xE7, 0x39, 0xCE, 0x73, 0x9C, 0xE7, 0x7B, 0x9D,
  0xCE, 0x00, 0x18, 0x18, 0xDF, 0xFE, 0x3C, 0xFE, 0xDF, 0x18, 0x18, 0x0C, 0x03, 0x00, 0xC3, 0xFF,
  0xFF, 0xFF, 0xF0, 0xC0, 0x30, 0x0C, 0x03, 0x00, 0x6F, 0xFD, 0x80, 0xFF, 0xFF, 0xF8, 0x6F, 0xF6,
  0x07, 0x07, 0x0E, 0x0E, 0x0E, 0x0C, 0x1C, 0x1C, 0x18, 0x38, 0x38, 0x30, 0x70, 0x70, 0x70, 0xE0,
  0xE0, 0xE0, 0x1F, 0x0F, 0xF1, 0xFF, 0x70, 0xEE, 0x0F, 0xC1, 0xF8, 0x3F, 0x07, 0xE0, 0xFC, 0x1F,
  0x83, 0xF0, 0x7E, 0x0F, 0xC1, 0xF8, 0x73, 0xFE, 0x7F, 0x83, 0xE0, 0x3F, 0xFF, 0x31, 0x8C, 0x63,
  0x18, 0xC6, 0x31, 0x8C, 0x63, 0x18, 0xC0, 0x1E, 0x1F, 0xEF, 0xFB, 0x87, 0xE1, 0xF8, 0x70, 0x1C,
  0x0E, 0x07, 0x81, 0xC0, 0xF0, 0x78, 0x3C, 0x0E, 0x07, 0x03, 0xFF, 0xFF, 0xFF, 0xF0, 0xFF, 0xFF,
  0xFF, 0xFC, 0x1E, 0x0F, 0x03, 0x81, 0xE0, 0xFC, 0x1F, 0x80, 0x60, 0x1C, 0x07, 0xE1, 0xF8, 0x7E,
  0x1F, 0xFE, 0x7F, 0x0F, 0x80, 0x0E, 0x03, 0xC0, 0x70, 0x0E, 0x03, 0x80, 0x70, 0x0E, 0x03, 0x8E,
  0x71, 0xDE, 0x3B, 0x87, 0x7F, 0xFF, 0xFF, 0xFF, 0xC0, 0x70, 0x0E, 0x01, 0xC0, 0x38, 0xFF, 0xFF,
  0xFF, 0xFB, 0x80, 0xE0, 0x38, 0x0E, 0x03, 0xBC, 0xFF, 0xBF, 0xFE, 0x1C, 0x07, 0x01, 0xF8, 0x7E,
  0x1D, 0xFF, 0x7F, 0x87, 0xC0, 0x1E, 0x1F, 0xE7, 0xFB, 0x87, 0xE1, 0xF8, 0x0E, 0x43, 0xFC, 0xFF,
  0xBC, 0xFE, 0x1F, 0x87, 0xE1, 0xF8, 0x7E, 0x1D, 0xFF, 0x7F, 0x87, 0xC0, 0xFF, 0xFF, 0xFF, 0xFF,
  0x0E, 0x03, 0x80, 0xE0, 0x70, 0x1C, 0x07, 0x03, 0x80, 0xE0, 0x38, 0x1C, 0x07, 0x01, 0xC0, 0xE0,
  0x38, 0x0E, 0x00, 0x1E, 0x1F, 0xE7, 0xFB, 0x87, 0xE1, 0xF8, 0x7E, 0x1D, 0xFE, 0x3F, 0x1F, 0xEF,
  0x3F, 0x87, 0xE1, 0xF8, 0x7E, 0x1D, 0xFE, 0x7F, 0x87, 0x80, 0x3E, 0x1F, 0xEF, 0xFB, 0x87, 0xE1,
  0xF8, 0x7E, 0x1F, 0x87, 0xF3, 0xDF, 0xF3, 0xFC, 0x27, 0x01, 0xF8, 0x7E, 0x1D, 0xFE, 0x7F, 0x87,
  0x80, 0x6E, 0xF6, 0x00, 0x00, 0x6E, 0xF6, 0x7F, 0xF7, 0x00, 0x00, 0x76, 0x66, 0x66, 0x00, 0x40,
  0x70, 0x7C, 0xFE, 0xFC, 0x3C, 0x0F, 0x03, 0xF0, 0x3F, 0x01, 0xF0, 0x1C, 0x01, 0xFF, 0xFF, 0xF0,
  0x00, 0x00, 0x00, 0x3F, 0xFF, 0xFC, 0x80, 0x38, 0x0F, 0x81, 0xF8, 0x1F, 0x80, 0xF0, 0x3C, 0x3E,
  0x7E, 0x3E, 0x0E, 0x02, 0x00, 0x3E, 0x1F, 0xEF, 0xFB, 0x8F, 0xE1, 0xC0, 0x70, 0x38, 0x1E, 0x0F,
  0x07, 0x81, 0xC0, 0x70, 0x00, 0x00, 0x01, 0x80, 0xF0, 0x1C, 0x06, 0x00, 0x01, 0xF8, 0x01, 0xFF,
  0x81, 0xE0, 0x70, 0xE0, 0x0E, 0x31, 0xF1, 0x98, 0xFC, 0x66, 0x73, 0x1F, 0x18, 0xC3, 0xC6, 0x31,
  0xF1, 0x8C, 0x6C, 0x67, 0x1B, 0x1F, 0xFE, 0xE7, 0xFF, 0x18, 0xE7, 0x87, 0x00, 0x00, 0xFF, 0xE0,
  0x1F, 0xF0, 0x03, 0xF0, 0x00, 0x07, 0x80, 0x3C, 0x01, 0xE0, 0x1F, 0x80, 0xFC, 0x06, 0xE0, 0x73,
  0x03, 0x9C, 0x1C, 0xE0, 0xE7, 0x0E, 0x38, 0x70, 0xE3, 0x87, 0x1F, 0xF9, 0xFF, 0xCE, 0x07, 0x70,
  0x3B, 0x81, 0xC0, 0xFF, 0x1F, 0xF3, 0xFF, 0x70, 0xEE, 0x1D, 0xC3, 0xB8, 0x77, 0x0E, 0xFF, 0x9F,
  0xF3, 0xFF, 0x70, 0xEE, 0x0D, 0xC1, 0xF8, 0x77, 0xFE, 0xFF, 0x9F, 0xE0, 0x1F, 0x07, 0xF9, 0xFF,
  0xF8, 0x7E, 0x0F, 0xC1, 0xF8, 0x07, 0x00, 0xE0, 0x1C, 0x03, 0x80, 0x70, 0x0E, 0x0F, 0xC1, 0xFC,
  0x3B, 0xFF, 0x3F, 0xC3, 0xE0, 0xFF, 0x3F, 0xEF, 0xFF, 0x87, 0xE1, 0xF8, 0x3E, 0x0F, 0x83, 0xE0,
  0xF8, 0x3E, 0x0F, 0x83, 0xE0, 0xF8, 0x7E, 0x1F, 0xFF, 0xFF, 0xBF, 0xC0, 0xFF, 0xFF, 0xFF, 0xFF,
  0x80, 0xE0, 0x38, 0x0E, 0x03, 0x80, 0xFE, 0x3F, 0x8F, 0xE3, 0x80, 0xE0, 0x38, 0x0E, 0x03, 0xFF,
  0xFF, 0xFF, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0x80, 0xE0, 0x38, 0x0E, 0x03, 0x80, 0xFE, 0x3F, 0x8F,
  0xE3, 0x80, 0xE0, 0x38, 0x0E, 0x03, 0x80, 0xE0, 0x38, 0x00, 0x1F, 0x07, 0xF9, 0xFF, 0xF8, 0x7E,
  0x0F, 0xC1, 0xF8, 0x07, 0x00, 0xE3, 0xFC, 0x7F, 0x8F, 0xF0, 0x7E, 0x0F, 0xC1, 0xFC, 0x3B, 0xFF,
  0x3F, 0xC3, 0xE0, 0xE1, 0xF8, 0x7E, 0x1F, 0x87, 0xE1, 0xF8, 0x7E, 0x1F, 0x87, 0xFF, 0xFF, 0xFF,
  0xFF, 0x87, 0xE1, 0xF8, 0x7E, 0x1F, 0x87, 0xE1, 0xF8, 0x70, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFC, 0x00, 0xE0, 0x1C, 0x03, 0x80, 0x70, 0x0E, 0x01, 0xC0, 0x38, 0x07, 0x00, 0xE0, 0x1C, 0x03,
  0xF0, 0x7E, 0x0E, 0xC1, 0xDC, 0x3B, 0xFF, 0x3F, 0xC3, 0xE0, 0xE0, 0xFC, 0x3B, 0x8F, 0x71, 0xCE,
  0x71, 0xDE, 0x3B, 0x87, 0xE0, 0xFC, 0x1F, 0xC3, 0xF8, 0x73, 0x8E, 0x79, 0xC7, 0x38, 0xF7, 0x0E,
  0xE1, 0xFC, 0x1C, 0xE0, 0x38, 0x0E, 0x03, 0x80, 0xE0, 0x38, 0x0E, 0x03, 0x80, 0xE0, 0x38, 0x0E,
  0x03, 0x80, 0xE0, 0x38, 0x0E, 0x03, 0xFF, 0xFF, 0xFF, 0xF0, 0xE0, 0x7E, 0x0F, 0xF0, 0xFF, 0x9F,
  0xFB, 0xFF, 0xFF, 0xDF, 0x7C, 0xF7, 0xCE, 0x7C, 0x67, 0xC0, 0x7C, 0x07, 0xC0, 0x7C, 0x07, 0xC0,
  0x7C, 0x07, 0xC0, 0x7C, 0x07, 0xE0, 0xFC, 0x1F, 0xC3, 0xF8, 0x7F, 0x8F, 0xF1, 0xF7, 0x3E, 0xE7,
  0xCE, 0xF9, 0xDF, 0x1B, 0xE3, 0xFC, 0x3F, 0x87, 0xF0, 0xFE, 0x0F, 0xC1, 0xF8, 0x1C, 0x1F, 0x07,
  0xF9, 0xFF, 0xF8, 0x7E, 0x0F, 0xC1, 0xF8, 0x3F, 0x07, 0xE0, 0xFC, 0x1F, 0x83, 0xF0, 0x7E, 0x0F,
  0xC1, 0xFC, 0x3B, 0xFF, 0x3F, 0xC3, 0xE0, 0xFF, 0x9F, 0xFB, 0xFF, 0xF0, 0x7E, 0x0F, 0xC0, 0xF8,
  0x3F, 0x07, 0xFF, 0xFF, 0xFB, 0xFE, 0x70, 0x0E, 0x01, 0xC0, 0x38, 0x07, 0x00, 0xE0, 0x1C, 0x00,
  0x1F, 0x07, 0xF1, 0xFF, 0x78, 0xFE, 0x0F, 0xC1, 0xF8, 0x3F, 0x07, 0xE0, 0xFC, 0x1F, 0x83, 0xF0,
  0x7E, 0x0F, 0xC1, 0xFC, 0x73, 0xFE, 0x3F, 0x81, 0xC0, 0x38, 0x07, 0x00, 0xFF, 0x1F, 0xF3, 0xFF,
  0x70, 0xEE, 0x0D, 0xC1, 0xF8, 0x77, 0x0E, 0xFF, 0xDF, 0xF3, 0xFC, 0x73, 0x8E, 0x39, 0xC7, 0x38,
  0xE7, 0x0E, 0xE1, 0xDC, 0x3C, 0x1F, 0x0F, 0xF1, 0xFF, 0x70, 0xFE, 0x0F, 0xC1, 0xFC, 0x03, 0xE0,
  0x3E, 0x01, 0xF0, 0x1F, 0x00, 0xFE, 0x0F, 0xC1, 0xF8, 0x3F, 0xFE, 0x7F, 0xC3, 0xE0, 0xFF, 0xFF,
  0xFF, 0xFF, 0x87, 0x00, 0xE0, 0x1C, 0x03, 0x80, 0x70, 0x0E, 0x01, 0xC0, 0x38, 0x07, 0x00, 0xE0,
  0x1C, 0x03, 0x80, 0x70, 0x0E, 0x01, 0xC0, 0xE0, 0x7C, 0x0F, 0x81, 0xF0, 0x3E, 0x07, 0xC0, 0xF8,
  0x1F, 0x03, 0xE0, 0x7C, 0x0F, 0x81, 0xF0, 0x3E, 0x06, 0xC1, 0xDC, 0x3B, 0xFF, 0x3F, 0xC3, 0xE0,
  0xE0, 0x7E, 0x0E, 0xE0, 0xEF, 0x0E, 0x70, 0xE7, 0x0C, 0x71, 0xC3, 0x9C, 0x39, 0xC3, 0x98, 0x3B,
  0x81, 0xB8, 0x1F, 0x81, 0xF0, 0x1F, 0x00, 0xF0, 0x0F, 0x00, 0xE0, 0xE1, 0xC1, 0xF8, 0x70, 0xEE,
  0x1E, 0x3B, 0x87, 0x8E, 0x63, 0xE3, 0x9C, 0xF8, 0xE7, 0x3E, 0x71, 0xCD, 0xDC, 0x73, 0x77, 0x0F,
  0xDD, 0xC3, 0xF3, 0x60, 0xFC, 0xF8, 0x3E, 0x3E, 0x07, 0x8F, 0x81, 0xE3, 0xE0, 0x78, 0x70, 0x1E,
  0x1C, 0x03, 0x07, 0x00, 0xE0, 0x7E, 0x0E, 0x70, 0xE7, 0x1C, 0x39, 0xC3, 0xB8, 0x1F, 0x81, 0xF0,
  0x0F, 0x00, 0xF0, 0x1F, 0x01, 0xF8, 0x3B, 0x83, 0x9C, 0x71, 0xC7, 0x0E, 0xE0, 0xEE, 0x07, 0xE0,
  0xFC, 0x1D, 0xC7, 0xB8, 0xE3, 0x9C, 0x77, 0x0E, 0xE0, 0xF8, 0x1F, 0x01, 0xE0, 0x38, 0x07, 0x00,
  0xE0, 0x1C, 0x03, 0x80, 0x70, 0x0E, 0x01, 0xC0, 0xFF, 0xFF, 0xFF, 0xFC, 0x0F, 0x03, 0x81, 0xC0,
  0x70, 0x38, 0x0E, 0x07, 0x01, 0xC0, 0xE0, 0x78, 0x1C, 0x0E, 0x03, 0xFF, 0xFF, 0xFF, 0xF0, 0xFF,
  0xFF, 0xCE, 0x73, 0x9C, 0xE7, 0x39, 0xCE, 0x73, 0x9C, 0xE7, 0x39, 0xCF, 0xFF, 0xE0, 0xE0, 0x38,
  0x1C, 0x0E, 0x03, 0x81, 0xC0, 0xE0, 0x38, 0x1C, 0x0E, 0x03, 0x01, 0xC0, 0xE0, 0x30, 0x1C, 0x0E,
  0x03, 0x01, 0xC0, 0xFF, 0xFE, 0x73, 0x9C, 0xE7, 0x39, 0xCE, 0x73, 0x9C, 0xE7, 0x39, 0xCE, 0x7F,
  0xFF, 0xE0, 0x1C, 0x0E, 0x0F, 0x86, 0xC3, 0x63, 0xB9, 0x8C, 0xC6, 0xE3, 0xE0, 0xC0, 0x7F, 0xFF,
  0xFF, 0xFF, 0x80, 0xC6, 0x70, 0x3E, 0x1F, 0xEF, 0xFB, 0x87, 0x01, 0xCF, 0xF7, 0xFF, 0xFF, 0xE1,
  0xF8, 0x7F, 0xFF, 0xFF, 0x7D, 0xC0, 0xE0, 0x38, 0x0E, 0x03, 0x80, 0xE0, 0x3B, 0xCF, 0xFB, 0xFF,
  0xF1, 0xF8, 0x7E, 0x1F, 0x87, 0xE1, 0xF8, 0x7F, 0x1F, 0xFF, 0xFF, 0xBB, 0xC0, 0x1F, 0x1F, 0xE7,
  0xFF, 0x87, 0xE1, 0xF8, 0x0E, 0x03, 0x80, 0xE1, 0xB8, 0x77, 0xFD, 0xFE, 0x1F, 0x00, 0x01, 0xC0,
  0x70, 0x1C, 0x07, 0x01, 0xCF, 0x77, 0xFD, 0xFF, 0xF1, 0xF8, 0x7E, 0x1F, 0x87, 0xE1, 0xF8, 0x7F,
  0x1D, 0xFF, 0x7F, 0xCF, 0x70, 0x1E, 0x1F, 0xE7, 0xFF, 0x87, 0xE1, 0xFF, 0xFF, 0xFF, 0xFF, 0xE0,
  0x38, 0x77, 0xFD, 0xFE, 0x1F, 0x00, 0x1E, 0x7D, 0xF3, 0x87, 0x1F, 0xFF, 0xFE, 0x70, 0xE1, 0xC3,
  0x87, 0x0E, 0x1C, 0x38, 0x70, 0xE0, 0x3D, 0xDF, 0xF7, 0xFF, 0x87, 0xE1, 0xF8, 0x7E, 0x1F, 0x87,
  0xE1, 0xF8, 0x7F, 0xFD, 0xFF, 0x3D, 0xC0, 0x70, 0x3C, 0x7E, 0x1F, 0x8F, 0x80, 0xE0, 0x38, 0x0E,
  0x03, 0x80, 0xE0, 0x3B, 0xCF, 0xFB, 0xFF, 0xF1, 0xF8, 0x7E, 0x1F, 0x87, 0xE1, 0xF8, 0x7E, 0x1F,
  0x87, 0xE1, 0xF8, 0x70, 0x4F, 0xFE, 0x40, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xE0, 0x10, 0xE3,
  0xCE, 0x10, 0x03, 0x8E, 0x38, 0xE3, 0x8E, 0x38, 0xE3, 0x8E, 0x38, 0xE3, 0x8E, 0x3B, 0xEF, 0xB8,
  0xE0, 0x38, 0x0E, 0x03, 0x80, 0xE0, 0x38, 0x7E, 0x3B, 0x9C, 0xEF, 0x3B, 0x8F, 0xE3, 0xFC, 0xF7,
  0x38, 0xEE, 0x3B, 0x8E, 0xE1, 0xF8, 0x70, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0xEF, 0x1E,
  0xFF, 0xBF, 0xFF, 0xFF, 0xE1, 0xC7, 0xE1, 0xC3, 0xE1, 0xC3, 0xE1, 0xC3, 0xE1, 0xC3, 0xE1, 0xC3,
  0xE1, 0xC3, 0xE1, 0xC3, 0xE1, 0xC3, 0xE1, 0xC3, 0xEF, 0x3F, 0xEF, 0xFF, 0xC7, 0xE1, 0xF8, 0x7E,
  0x1F, 0x87, 0xE1, 0xF8, 0x7E, 0x1F, 0x87, 0xE1, 0xC0, 0x1F, 0x1F, 0xE7, 0xFF, 0x87, 0xE1, 0xF8,
  0x7E, 0x1F, 0x87, 0xE1, 0xF8, 0x77, 0xFD, 0xFE, 0x1F, 0x00, 0xEF, 0x3F, 0xEF, 0xFF, 0xC7, 0xF1,
  0xF8, 0x7E, 0x1F, 0x87, 0xF1, 0xFC, 0x7F, 0xFF, 0xFE, 0xEF, 0x38, 0x0E, 0x03, 0x80, 0xE0, 0x38,
  0x00, 0x3D, 0xDF, 0xF7, 0xFF, 0xC7, 0xE1, 0xF8, 0x7E, 0x1F, 0x87, 0xE1, 0xFC, 0x77, 0xFD, 0xFF,
  0x3D, 0xC0, 0x70, 0x1C, 0x07, 0x01, 0xC0, 0x70, 0xEF, 0xFF, 0xFF, 0x8E, 0x1C, 0x38, 0x70, 0xE1,
  0xC3, 0x87, 0x0E, 0x00, 0x3E, 0x3F, 0xBF, 0xFC, 0x7E, 0x03, 0xC0, 0xFC, 0x1F, 0x03, 0xF1, 0xFF,
  0xEF, 0xF3, 0xE0, 0x71, 0xC7, 0x3F, 0xFF, 0xF7, 0x1C, 0x71, 0xC7, 0x1C, 0x71, 0xF7, 0xCF, 0xE1,
  0xF8, 0x7E, 0x1F, 0x87, 0xE1, 0xF8, 0x7E, 0x1F, 0x87, 0xE1, 0xF8, 0x7F, 0x3D, 0xFF, 0x3D, 0xC0,
  0xE1, 0xF8, 0xFE, 0x3B, 0x8E, 0x73, 0x9C, 0xC7, 0x70, 0xDC, 0x3F, 0x0F, 0x83, 0xE0, 0x78, 0x1C,
  0x00, 0xC3, 0x8F, 0xC7, 0x1F, 0x8E, 0x3F, 0x3E, 0x76, 0x7C, 0xCE, 0xDB, 0x9D, 0xB7, 0x3F, 0x6E,
  0x3E, 0xF8, 0x79, 0xF0, 0xF1, 0xE1, 0xE3, 0xC1, 0xC7, 0x00, 0x70, 0xEE, 0x38, 0xE7, 0x1F, 0xC1,
  0xF8, 0x1E, 0x03, 0xC0, 0x78, 0x1F, 0x87, 0xF0, 0xE7, 0x38, 0xEF, 0x0E, 0xF0, 0xEE, 0x39, 0xC7,
  0x38, 0xE3, 0x98, 0x77, 0x0E, 0xE0, 0xDC, 0x1B, 0x03, 0xE0, 0x7C, 0x07, 0x00, 0xE0, 0x1C, 0x03,
  0x81, 0xE0, 0x7C, 0x0E, 0x00, 0xFF, 0xFF, 0xFF, 0xC1, 0xE0, 0xE0, 0xE0, 0xF0, 0x70, 0x70, 0x78,
  0x3F, 0xDF, 0xFF, 0xF8, 0x1E, 0x7D, 0xF3, 0x87, 0x0E, 0x1C, 0x38, 0x70, 0xE3, 0xC7, 0x0F, 0x0E,
  0x1C, 0x38, 0x70, 0xE1, 0xC3, 0x87, 0xC7, 0xC7, 0x80, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFE, 0xF1, 0xF3, 0xE1, 0xC3, 0x87, 0x0E, 0x1C, 0x38, 0x70, 0xF8, 0xF3, 0xC7, 0x0E, 0x1C, 0x38,
  0x70, 0xE1, 0xCF, 0x9F, 0x3C, 0x00, 0x78, 0x5F, 0xFF, 0xFF, 0x21, 0xC0,
};
const GFXglyph BarlowLogoGlyphs[] PROGMEM = {
  {0, 1, 1, 5, 0, 0},   // 0x20  
  {1, 4, 18, 8, 2, -17},   // 0x21 !
  {10, 7, 6, 8, 0, -17},   // 0x22 "
  {16, 14, 16, 16, 1, -16},   // 0x23 #
  {44, 11, 22, 13, 1, -19},   // 0x24 $
  {75, 17, 18, 20, 2, -17},   // 0x25 %
  {114, 14, 18, 16, 1, -17},   // 0x26 &
  {146, 3, 5, 4, 0, -17},   // 0x27 '
  {148, 6, 21, 7, 0, -18},   // 0x28 (
  {164, 5, 21, 7, 2, -18},   // 0x29 )
  {178, 8, 9, 10, 1, -18},   // 0x2A *
  {187, 10, 10, 12, 1, -12},   // 0x2B +
  {200, 3, 6, 6, 1, -3},   // 0x2C ,
  {203, 7, 3, 9, 1, -8},   // 0x2D -
  {206, 4, 4, 6, 1, -3},   // 0x2E .
  {208, 8, 18, 10, 1, -17},   // 0x2F /
  {226, 11, 18, 13, 1, -17},   // 0x30 0
  {251, 5, 18, 8, 1, -17},   // 0x31 1
  {263, 10, 18, 12, 1, -17},   // 0x32 2
  {286, 10, 18, 12, 1, -17},   // 0x33 3
  {309, 11, 18, 13, 1, -17},   // 0x34 4
  {334, 10, 18, 12, 1, -17},   // 0x35 5
  {357, 10, 18, 12, 1, -17},   // 0x36 6
  {380, 10, 18, 11, 1, -17},   // 0x37 7
  {403, 10, 18, 12, 1, -17},   // 0x38 8
  {426, 10, 18, 12, 1, -17},   // 0x39 9
  {449, 4, 12, 8, 2, -12},   // 0x3A :
  {455, 4, 14, 7, 1, -11},   // 0x3B ;
  {462, 10, 12, 12, 1, -12},   // 0x3C <
  {477, 10, 7, 12, 1, -11},   // 0x3D =
  {486, 10, 12, 12, 1, -12},   // 0x3E >
  {501, 10, 18, 11, 1, -17},   // 0x3F ?
  {524, 18, 18, 20, 1, -15},   // 0x40 @
  {565, 13, 18, 14, 0, -17},   // 0x41 A
  {595, 11, 18, 14, 2, -17},   // 0x42 B
  {620, 11, 18, 13, 1, -17},   // 0x43 C
  {645, 10, 18, 14, 2, -17},   // 0x44 D
  {668, 10, 18, 13, 2, -17},   // 0x45 E
  {691, 10, 18, 12, 2, -17},   // 0x46 F
  {714, 11, 18, 13, 1, -17},   // 0x47 G
  {739, 10, 18, 14, 2, -17},   // 0x48 H
  {762, 3, 18, 6, 2, -17},   // 0x49 I
  {769, 11, 18, 13, 0, -17},   // 0x4A J
  {794, 11, 18, 14, 2, -17},   // 0x4B K
  {819, 10, 18, 12, 2, -17},   // 0x4C L
  {842, 12, 18, 16, 2, -17},   // 0x4D M
  {869, 11, 18, 15, 2, -17},   // 0x4E N
  {894, 11, 18, 14, 1, -17},   // 0x4F O
  {919, 11, 18, 13, 1, -17},   // 0x50 P
  {944, 11, 20, 13, 1, -17},   // 0x51 Q
  {972, 11, 18, 13, 2, -17},   // 0x52 R
  {997, 11, 18, 13, 1, -17},   // 0x53 S
  {1022, 11, 18, 13, 1, -17},   // 0x54 T
  {1047, 11, 18, 14, 1, -17},   // 0x55 U
  {1072, 12, 18, 14, 1, -17},   // 0x56 V
  {1099, 18, 18, 19, 1, -17},   // 0x57 W
  {1140, 12, 18, 13, 1, -17},   // 0x58 X
  {1167, 11, 18, 13, 1, -17},   // 0x59 Y
  {1192, 10, 18, 12, 1, -17},   // 0x5A Z
  {1215, 5, 23, 9, 1, -19},   // 0x5B [
  {1230, 9, 18, 10, 1, -17},   // 0x5C backslash
  {1251, 5, 23, 9, 3, -19},   // 0x5D ]
  {1266, 9, 10, 11, 1, -17},   // 0x5E ^
  {1278, 11, 3, 11, 0, -2},   // 0x5F _
  {1283, 4, 3, 5, 1, -16},   // 0x60 `
  {1285, 10, 13, 12, 1, -12},   // 0x61 a
  {1302, 10, 18, 12, 1, -17},   // 0x62 b
  {1325, 10, 13, 12, 1, -12},   // 0x63 c
  {1342, 10, 18, 12, 1, -17},   // 0x64 d
  {1365, 10, 13, 12, 1, -12},   // 0x65 e
  {1382, 7, 18, 8, 1, -17},   // 0x66 f
  {1398, 10, 18, 12, 1, -12},   // 0x67 g
  {1421, 10, 18, 12, 1, -17},   // 0x68 h
  {1444, 4, 19, 6, 1, -18},   // 0x69 i
  {1454, 6, 24, 6, -1, -18},   // 0x6A j
  {1472, 10, 18, 12, 1, -17},   // 0x6B k
  {1495, 3, 18, 5, 1, -17},   // 0x6C l
  {1502, 16, 13, 19, 1, -12},   // 0x6D m
  {1528, 10, 13, 12, 1, -12},   // 0x6E n
  {1545, 10, 13, 12, 1, -12},   // 0x6F o
  {1562, 10, 18, 13, 1, -12},   // 0x70 p
  {1585, 10, 18, 13, 1, -12},   // 0x71 q
  {1608, 7, 13, 9, 1, -12},   // 0x72 r
  {1620, 9, 13, 11, 1, -12},   // 0x73 s
  {1635, 6, 16, 8, 1, -15},   // 0x74 t
  {1647, 10, 13, 12, 1, -12},   // 0x75 u
  {1664, 10, 13, 12, 1, -12},   // 0x76 v
  {1681, 15, 13, 17, 1, -12},   // 0x77 w
  {1706, 11, 13, 12, 0, -12},   // 0x78 x
  {1724, 11, 18, 11, 0, -12},   // 0x79 y
  {1749, 9, 13, 10, 1, -12},   // 0x7A z
  {1764, 7, 23, 8, 1, -19},   // 0x7B {
  {1785, 3, 21, 5, 1, -18},   // 0x7C |
  {1793, 7, 23, 8, 1, -19},   // 0x7D }
  {1814, 11, 4, 13, 1, -8},   // 0x7E ~
};
const GFXfont BarlowLogo PROGMEM = {(uint8_t*)BarlowLogoBitmaps, (GFXglyph*)BarlowLogoGlyphs, 0x20, 0x7E, 30};

// CleanSmall: generated from inter-600.ttf at 12px (cursor y = top of the text)
const uint8_t CleanSmallBitmaps[] PROGMEM = {
  0x00, 0xFF, 0xF3, 0xC0, 0xDD, 0xDD, 0x36, 0x26, 0x25, 0x7E, 0x24, 0x24, 0xFE, 0x4C, 0x48, 0x21,
  0xEF, 0xE9, 0xE1, 0xE2, 0xE9, 0xED, 0xE2, 0x00, 0x61, 0xA4, 0x49, 0x23, 0xD0, 0x04, 0x02, 0x61,
  0x24, 0xC9, 0x61, 0x80, 0x71, 0xB3, 0x63, 0x86, 0x1E, 0xA7, 0x46, 0xF6, 0xFF, 0x6B, 0x6D, 0xB6,
  0x4C, 0x80, 0xCD, 0x92, 0x49, 0x6B, 0x00, 0x25, 0xDD, 0xF2, 0x00, 0x30, 0xCF, 0xCC, 0x30, 0xEA,
  0xF0, 0xF0, 0x11, 0x32, 0x26, 0x64, 0x4C, 0x73, 0xFC, 0xE1, 0x86, 0x18, 0xF3, 0x78, 0x7E, 0x92,
  0x49, 0x20, 0x73, 0x68, 0xC2, 0x18, 0xC6, 0x30, 0xFC, 0x73, 0x68, 0xC2, 0x30, 0x30, 0xF3, 0x78,
  0x18, 0x62, 0x9A, 0x4B, 0x2F, 0xC2, 0x08, 0xFB, 0xE8, 0x2C, 0xD8, 0x30, 0xF2, 0x78, 0x3B, 0xFC,
  0xEC, 0xFE, 0x38, 0x73, 0x78, 0xFF, 0xF0, 0xC2, 0x18, 0x43, 0x08, 0x60, 0x73, 0x78, 0xF2, 0x73,
  0x38, 0xF3, 0x78, 0x73, 0x68, 0xE3, 0xCD, 0xD0, 0xF2, 0x78, 0xF0, 0xF0, 0xF0, 0xEA, 0x04, 0x77,
  0x30, 0x70, 0x70, 0x40, 0xFC, 0x0F, 0xFF, 0x83, 0x83, 0x83, 0x3B, 0x88, 0x00, 0x39, 0xBC, 0xC3,
  0x18, 0xC0, 0x0C, 0x30, 0x0C, 0x0F, 0xE4, 0x0E, 0x7D, 0x93, 0x68, 0xDB, 0x36, 0x4D, 0xDD, 0x90,
  0x03, 0xF0, 0x18, 0x1C, 0x14, 0x34, 0x26, 0x22, 0x7F, 0x43, 0xC1, 0xFB, 0xF8, 0xE3, 0xFA, 0x38,
  0x63, 0xF8, 0x38, 0xFF, 0x1C, 0x08, 0x10, 0x30, 0xF3, 0x7C, 0xF1, 0xFA, 0x1C, 0x38, 0x30, 0xE1,
  0xC6, 0xF8, 0xFF, 0xF8, 0x20, 0xFA, 0x08, 0x20, 0xFC, 0xFF, 0xE1, 0x0F, 0xFE, 0x10, 0x80, 0x38,
  0xFF, 0x1C, 0x08, 0xF1, 0xF0, 0xF3, 0x7C, 0x87, 0x0E, 0x1C, 0x3F, 0xF0, 0xE1, 0xC3, 0x86, 0xFF,
  0x80, 0x04, 0x10, 0x41, 0x04, 0x1C, 0x53, 0x78, 0x87, 0x1A, 0x65, 0x8B, 0x1B, 0x22, 0x46, 0x86,
  0x84, 0x21, 0x08, 0x42, 0x10, 0xF8, 0xC1, 0xF1, 0xE8, 0xB4, 0x5B, 0x6C, 0xA6, 0x53, 0x39, 0x88,
  0x80, 0xC3, 0x86, 0x8D, 0x99, 0x33, 0x62, 0xC3, 0x86, 0x38, 0x7E, 0xC6, 0x83, 0x83, 0x83, 0xC2,
  0xE6, 0x7C, 0xFB, 0xF8, 0x61, 0x8F, 0xE8, 0x20, 0x80, 0x38, 0x7E, 0xC6, 0x83, 0x83, 0x83, 0xDA,
  0xEE, 0x7C, 0x02, 0xF9, 0xFA, 0x14, 0x2F, 0xDF, 0x26, 0x46, 0x86, 0x7B, 0xF8, 0x70, 0x78, 0x38,
  0x73, 0x78, 0xFF, 0xFC, 0x60, 0xC1, 0x83, 0x06, 0x0C, 0x18, 0x87, 0x0E, 0x1C, 0x38, 0x70, 0xF1,
  0xE7, 0x7C, 0xC1, 0x43, 0x63, 0x62, 0x26, 0x36, 0x14, 0x1C, 0x1C, 0xC6, 0x34, 0x63, 0x67, 0x26,
  0xF6, 0x6D, 0x62, 0x96, 0x29, 0x43, 0x9C, 0x18, 0xC0, 0x63, 0x66, 0x36, 0x1C, 0x18, 0x1C, 0x36,
  0x66, 0xC3, 0xC3, 0x63, 0x26, 0x34, 0x1C, 0x18, 0x08, 0x08, 0x08, 0xFF, 0xF0, 0x86, 0x31, 0x84,
  0x30, 0xFC, 0xFB, 0x6D, 0xB6, 0xDF, 0x80, 0xC4, 0x46, 0x62, 0x23, 0x11, 0xD5, 0x55, 0x7C, 0x31,
  0x97, 0x90, 0xFC, 0xD0, 0x31, 0xF0, 0x4F, 0x47, 0x37, 0x40, 0x82, 0x0B, 0x3E, 0xCE, 0x38, 0xF3,
  0xB8, 0x33, 0xE8, 0xA0, 0x83, 0x37, 0x80, 0x0C, 0x36, 0xFF, 0x8E, 0x38, 0xF3, 0x7C, 0x33, 0xE8,
  0xBF, 0x83, 0x37, 0x80, 0x3B, 0x3D, 0xE6, 0x31, 0x8C, 0x60, 0x6F, 0xF8, 0xE3, 0x8F, 0x77, 0xC3,
  0xF8, 0xC0, 0x82, 0x0B, 0x3E, 0x8E, 0x38, 0xE3, 0x8C, 0xFF, 0x80, 0x55, 0x55, 0x5C, 0x82, 0x08,
  0xE6, 0xB3, 0x8F, 0x26, 0x8C, 0xFF, 0x80, 0xB3, 0x7F, 0xE2, 0x71, 0x38, 0x9C, 0x4E, 0x26, 0xB3,
  0xE8, 0xE3, 0x8E, 0x38, 0xC0, 0x33, 0xE8, 0xE3, 0x8F, 0x27, 0x80, 0xB3, 0xEC, 0xE3, 0x8F, 0x3B,
  0xA0, 0x80, 0x6F, 0xF8, 0xE3, 0x8F, 0x37, 0xC3, 0x0C, 0xBF, 0x88, 0x88, 0x80, 0x67, 0xE1, 0xE1,
  0xCF, 0xC0, 0x6F, 0xF6, 0x66, 0x67, 0x8E, 0x38, 0xE3, 0x8F, 0x3F, 0xC0, 0xC6, 0x89, 0x93, 0x62,
  0x85, 0x0E, 0x00, 0xCC, 0xD3, 0x34, 0xC9, 0xBE, 0x73, 0x8C, 0xC3, 0x30, 0xC4, 0xD8, 0xA1, 0x83,
  0x8D, 0xB1, 0x80, 0xC6, 0x89, 0x93, 0x62, 0x85, 0x06, 0x08, 0x70, 0x80, 0xFF, 0xC4, 0x46, 0x63,
  0xE0, 0x36, 0x66, 0xCC, 0x66, 0x67, 0x10, 0xFF, 0xFF, 0xFF, 0xFC, 0xC4, 0x44, 0x73, 0x44, 0x4C,
  0x80, 0x67, 0xF9, 0x80,
};
const GFXglyph CleanSmallGlyphs[] PROGMEM = {
  {0, 1, 1, 3, 0, 11},   // 0x20  
  {1, 2, 9, 4, 1, 3},   // 0x21 !
  {4, 4, 4, 6, 1, 3},   // 0x22 "
  {6, 8, 9, 8, 0, 3},   // 0x23 #
  {15, 6, 11, 8, 1, 2},   // 0x24 $
  {24, 10, 9, 12, 1, 3},   // 0x25 %
  {36, 7, 9, 8, 1, 3},   // 0x26 &
  {44, 2, 4, 4, 1, 3},   // 0x27 '
  {45, 3, 11, 4, 1, 3},   // 0x28 (
  {50, 3, 11, 4, 0, 3},   // 0x29 )
  {55, 5, 5, 6, 1, 3},   // 0x2A *
  {59, 6, 5, 8, 1, 6},   // 0x2B +
  {63, 2, 4, 4, 1, 10},   // 0x2C ,
  {64, 4, 1, 6, 1, 8},   // 0x2D -
  {65, 2, 2, 4, 1, 10},   // 0x2E .
  {66, 4, 10, 5, 0, 3},   // 0x2F /
  {71, 6, 9, 8, 1, 3},   // 0x30 0
  {78, 3, 9, 5, 1, 3},   // 0x31 1
  {82, 6, 9, 7, 1, 3},   // 0x32 2
  {89, 6, 9, 8, 1, 3},   // 0x33 3
  {96, 6, 9, 8, 1, 3},   // 0x34 4
  {103, 6, 9, 7, 1, 3},   // 0x35 5
  {110, 6, 9, 8, 1, 3},   // 0x36 6
  {117, 6, 9, 7, 0, 3},   // 0x37 7
  {124, 6, 9, 8, 1, 3},   // 0x38 8
  {131, 6, 9, 8, 1, 3},   // 0x39 9
  {138, 2, 6, 4, 1, 6},   // 0x3A :
  {140, 2, 8, 4, 1, 6},   // 0x3B ;
  {142, 6, 7, 8, 1, 5},   // 0x3C <
  {148, 6, 4, 8, 1, 7},   // 0x3D =
  {151, 6, 7, 8, 1, 5},   // 0x3E >
  {157, 6, 9, 7, 0, 3},   // 0x3F ?
  {164, 10, 11, 12, 1, 3},   // 0x40 @
  {178, 8, 9, 9, 0, 3},   // 0x41 A
  {187, 6, 9, 8, 1, 3},   // 0x42 B
  {194, 7, 9, 9, 1, 3},   // 0x43 C
  {202, 7, 9, 9, 1, 3},   // 0x44 D
  {210, 6, 9, 7, 1, 3},   // 0x45 E
  {217, 5, 9, 7, 1, 3},   // 0x46 F
  {223, 7, 9, 9, 1, 3},   // 0x47 G
  {231, 7, 9, 9, 1, 3},   // 0x48 H
  {239, 1, 9, 3, 1, 3},   // 0x49 I
  {241, 6, 9, 7, 0, 3},   // 0x4A J
  {248, 7, 9, 8, 1, 3},   // 0x4B K
  {256, 5, 9, 7, 1, 3},   // 0x4C L
  {262, 9, 9, 11, 1, 3},   // 0x4D M
  {273, 7, 9, 9, 1, 3},   // 0x4E N
  {281, 8, 9, 9, 1, 3},   // 0x4F O
  {290, 6, 9, 8, 1, 3},   // 0x50 P
  {297, 8, 10, 9, 1, 3},   // 0x51 Q
  {307, 7, 9, 8, 1, 3},   // 0x52 R
  {315, 6, 9, 8, 1, 3},   // 0x53 S
  {322, 7, 9, 8, 0, 3},   // 0x54 T
  {330, 7, 9, 9, 1, 3},   // 0x55 U
  {338, 8, 9, 9, 0, 3},   // 0x56 V
  {347, 12, 9, 12, 0, 3},   // 0x57 W
  {361, 8, 9, 9, 0, 3},   // 0x58 X
  {370, 8, 9, 9, 0, 3},   // 0x59 Y
  {379, 6, 9, 8, 1, 3},   // 0x5A Z
  {386, 3, 11, 4, 1, 3},   // 0x5B [
  {391, 4, 10, 5, 0, 3},   // 0x5C backslash
  {396, 2, 11, 4, 1, 3},   // 0x5D ]
  {399, 5, 4, 6, 0, 3},   // 0x5E ^
  {402, 6, 1, 6, 0, 12},   // 0x5F _
  {403, 2, 2, 4, 1, 3},   // 0x60 `
  {404, 6, 7, 7, 0, 5},   // 0x61 a
  {410, 6, 9, 7, 1, 3},   // 0x62 b
  {417, 6, 7, 7, 1, 5},   // 0x63 c
  {423, 6, 9, 7, 1, 3},   // 0x64 d
  {430, 6, 7, 7, 1, 5},   // 0x65 e
  {436, 5, 9, 5, 0, 3},   // 0x66 f
  {442, 6, 10, 8, 1, 5},   // 0x67 g
  {450, 6, 9, 7, 1, 3},   // 0x68 h
  {457, 1, 9, 3, 1, 3},   // 0x69 i
  {459, 2, 11, 3, 0, 3},   // 0x6A j
  {462, 6, 9, 7, 1, 3},   // 0x6B k
  {469, 1, 9, 3, 1, 3},   // 0x6C l
  {471, 9, 7, 11, 1, 5},   // 0x6D m
  {479, 6, 7, 7, 1, 5},   // 0x6E n
  {485, 6, 7, 7, 1, 5},   // 0x6F o
  {491, 6, 9, 7, 1, 5},   // 0x70 p
  {498, 6, 9, 7, 1, 5},   // 0x71 q
  {505, 4, 7, 5, 1, 5},   // 0x72 r
  {509, 5, 7, 7, 1, 5},   // 0x73 s
  {514, 4, 8, 4, 0, 4},   // 0x74 t
  {518, 6, 7, 7, 1, 5},   // 0x75 u
  {524, 7, 7, 7, 0, 5},   // 0x76 v
  {531, 10, 7, 10, 0, 5},   // 0x77 w
  {540, 7, 7, 7, 0, 5},   // 0x78 x
  {547, 7, 10, 7, 0, 5},   // 0x79 y
  {556, 5, 7, 7, 1, 5},   // 0x7A z
  {561, 4, 11, 5, 1, 3},   // 0x7B {
  {567, 2, 15, 4, 1, 0},   // 0x7C |
  {571, 4, 11, 5, 1, 3},   // 0x7D }
  {577, 6, 3, 8, 1, 7},   // 0x7E ~
};
const GFXfont CleanSmall PROGMEM = {(uint8_t*)CleanSmallBitmaps, (GFXglyph*)CleanSmallGlyphs, 0x20, 0x7E, 15};

// CleanLabel: generated from inter-700.ttf at 15px
const uint8_t CleanLabelBitmaps[] PROGMEM = {
  0x00, 0xFF, 0xFF, 0xFB, 0x1F, 0x80, 0xED, 0xB4, 0xD3, 0x4C, 0x19, 0x84, 0x41, 0x31, 0xFF, 0x33,
  0x0C, 0xC3, 0x23, 0xFE, 0x26, 0x19, 0x86, 0x60, 0x10, 0x3C, 0xFF, 0xD3, 0xD0, 0xF0, 0x7E, 0x0F,
  0x13, 0xD3, 0xFF, 0x7E, 0x10, 0x70, 0x66, 0xC2, 0x32, 0x21, 0xB3, 0x07, 0x90, 0x01, 0x00, 0x19,
  0xC1, 0x9B, 0x08, 0xD8, 0xC6, 0xCC, 0x1C, 0x38, 0x3E, 0x11, 0x0D, 0x87, 0x83, 0x83, 0xED, 0x9E,
  0xCF, 0x7F, 0x9F, 0xE0, 0xED, 0x24, 0x33, 0x66, 0x6E, 0xEC, 0xEE, 0x66, 0x73, 0xE6, 0x73, 0x33,
  0x33, 0x33, 0x37, 0x6E, 0x31, 0x5F, 0xDE, 0xDC, 0xC0, 0x18, 0x18, 0x18, 0xFF, 0xFF, 0x18, 0x18,
  0xFB, 0x68, 0xF8, 0xFC, 0x1C, 0x61, 0x86, 0x30, 0xC3, 0x08, 0x61, 0x86, 0x10, 0xC0, 0x3C, 0x7E,
  0xE7, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xE7, 0x7E, 0x3C, 0x3F, 0xFB, 0x33, 0x33, 0x33, 0x30, 0x7C,
  0xFE, 0xC6, 0x06, 0x06, 0x0E, 0x1C, 0x38, 0x70, 0xFF, 0xFF, 0x3C, 0xFE, 0xC7, 0x07, 0x0E, 0x1C,
  0x07, 0x03, 0xC7, 0xFF, 0x7C, 0x0E, 0x1E, 0x1E, 0x36, 0x36, 0x66, 0xC6, 0xFF, 0xFF, 0x06, 0x06,
  0xFE, 0xFE, 0xC0, 0xC0, 0xFC, 0xEE, 0x07, 0x07, 0xC6, 0xFE, 0x7C, 0x3C, 0x7E, 0xE7, 0xC0, 0xDE,
  0xFF, 0xC3, 0xC3, 0xC3, 0x7E, 0x3C, 0xFF, 0xFC, 0x18, 0x60, 0xC3, 0x06, 0x18, 0x30, 0xC3, 0x80,
  0x3C, 0xFE, 0xC7, 0xC7, 0x6E, 0x3C, 0xE7, 0xC3, 0xC3, 0xFF, 0x7E, 0x7C, 0xFE, 0xC7, 0xC3, 0xC7,
  0xEF, 0x7B, 0x03, 0xC6, 0xFE, 0x7C, 0xFC, 0x01, 0xF8, 0xEC, 0x01, 0xF6, 0xD0, 0x01, 0x07, 0x1F,
  0x78, 0xE0, 0xF8, 0x3E, 0x07, 0x01, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x80, 0xE0, 0xF8, 0x1E, 0x07,
  0x0F, 0x7C, 0xF0, 0x80, 0x79, 0xFB, 0x38, 0x71, 0xC7, 0x0C, 0x18, 0x00, 0x60, 0xC0, 0x0F, 0x80,
  0xFF, 0x87, 0x03, 0x18, 0x86, 0xCF, 0xDB, 0x63, 0x2D, 0x8C, 0xF6, 0x32, 0xD8, 0xCB, 0x3F, 0xEC,
  0x77, 0x18, 0x00, 0x3F, 0xC0, 0x7F, 0x00, 0x0E, 0x01, 0xE0, 0x6C, 0x0D, 0x81, 0xB8, 0x73, 0x0C,
  0x61, 0xFE, 0x70, 0xCC, 0x1B, 0x83, 0x80, 0xFC, 0xFF, 0xC7, 0xC3, 0xC6, 0xFE, 0xC7, 0xC3, 0xC3,
  0xFF, 0xFE, 0x3E, 0x3F, 0xB8, 0xF8, 0x3C, 0x06, 0x03, 0x01, 0x83, 0xE1, 0xBF, 0x8F, 0x80, 0xFC,
  0x7F, 0xB0, 0xD8, 0x3C, 0x1E, 0x0F, 0x07, 0x83, 0xC3, 0x7F, 0xBF, 0x00, 0xFF, 0xFF, 0x06, 0x0C,
  0x1F, 0xF0, 0x60, 0xC1, 0xFF, 0xF8, 0xFF, 0xFF, 0x06, 0x0C, 0x1F, 0xFF, 0xE0, 0xC1, 0x83, 0x00,
  0x3E, 0x1F, 0xCE, 0x3B, 0x06, 0xC0, 0x31, 0xFC, 0x7F, 0x06, 0xE1, 0x9F, 0xE3, 0xE0, 0xC1, 0xE0,
  0xF0, 0x78, 0x3F, 0xFF, 0xFF, 0x07, 0x83, 0xC1, 0xE0, 0xF0, 0x60, 0xFF, 0xFF, 0xFC, 0x03, 0x03,
  0x03, 0x03, 0x03, 0x03, 0x03, 0xE3, 0x67, 0x7E, 0x3E, 0xC3, 0xE3, 0xB1, 0x99, 0x8D, 0x87, 0xE3,
  0xB1, 0xDC, 0xC7, 0x61, 0xB0, 0xE0, 0xC1, 0x83, 0x06, 0x0C, 0x18, 0x30, 0x60, 0xC1, 0xFF, 0xF8,
  0xF0, 0xFF, 0x0F, 0xF0, 0xFD, 0x9B, 0xD9, 0xBD, 0x9B, 0xDF, 0xBC, 0xF3, 0xCF, 0x3C, 0x63, 0xC6,
  0x30, 0xE1, 0xF0, 0xFC, 0x7F, 0x3D, 0x9E, 0x6F, 0x37, 0x8F, 0xC7, 0xE1, 0xF0, 0x60, 0x3E, 0x1F,
  0xCE, 0x3B, 0x07, 0xC0, 0xF0, 0x3C, 0x0F, 0x07, 0xE1, 0x9F, 0xE3, 0xE0, 0xFC, 0xFE, 0xC7, 0xC3,
  0xC3, 0xC7, 0xFE, 0xC0, 0xC0, 0xC0, 0xC0, 0x3E, 0x1F, 0xCE, 0x3B, 0x07, 0xC0, 0xF0, 0x3C, 0x0F,
  0x37, 0xE7, 0x9F, 0xE3, 0xF8, 0x07, 0xFC, 0x7F, 0xB1, 0xD8, 0x6C, 0x37, 0xFB, 0xF9, 0x9C, 0xC6,
  0x63, 0xB0, 0xE0, 0x3C, 0xFF, 0xC3, 0xC0, 0xF0, 0x7E, 0x0F, 0x03, 0xC3, 0xFF, 0x7E, 0xFF, 0xFF,
  0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0xC1, 0xE0, 0xF0, 0x78, 0x3C, 0x1E, 0x0F,
  0x07, 0x83, 0xE3, 0xBF, 0x8F, 0x80, 0xE0, 0xEC, 0x19, 0xC3, 0x18, 0xE3, 0x18, 0x73, 0x06, 0xE0,
  0xD8, 0x1B, 0x01, 0xE0, 0x38, 0x00, 0xE1, 0x86, 0xC7, 0x1D, 0x8F, 0x3B, 0x9E, 0x63, 0x2C, 0xC6,
  0xDB, 0x8D, 0x9E, 0x1B, 0x3C, 0x1C, 0x78, 0x38, 0xF0, 0x70, 0xC0, 0x70, 0xC6, 0x30, 0xEE, 0x0D,
  0x80, 0xE0, 0x1C, 0x03, 0xC0, 0xD8, 0x3B, 0x8E, 0x3B, 0x83, 0x80, 0xE0, 0xEE, 0x38, 0xC6, 0x1D,
  0xC1, 0xB0, 0x1C, 0x03, 0x80, 0x70, 0x0E, 0x01, 0xC0, 0x38, 0x00, 0xFF, 0xFF, 0x06, 0x0E, 0x1C,
  0x18, 0x38, 0x70, 0x60, 0xFF, 0xFF, 0xFF, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xFF, 0xC1, 0x86, 0x18,
  0x20, 0xC3, 0x0C, 0x18, 0x61, 0x86, 0x0C, 0xFD, 0xB6, 0xDB, 0x6D, 0xBF, 0xC0, 0x18, 0x70, 0xB3,
  0x6C, 0x60, 0xFF, 0xFC, 0xCC, 0x80, 0x3E, 0x67, 0x03, 0x3F, 0x73, 0xE3, 0x67, 0x7B, 0xC0, 0xC0,
  0xC0, 0xDE, 0xFE, 0xC7, 0xC3, 0xC3, 0xC7, 0xFE, 0xDC, 0x7D, 0xDF, 0x1E, 0x0C, 0x18, 0xFF, 0xBE,
  0x07, 0x07, 0x07, 0x7F, 0xEF, 0xC7, 0xC7, 0xC7, 0xC7, 0xFF, 0x7F, 0x7D, 0xDF, 0x1F, 0xFC, 0x18,
  0x3F, 0xBE, 0x3C, 0xC3, 0x3F, 0x30, 0xC3, 0x0C, 0x30, 0xC3, 0x00, 0x7F, 0xEF, 0xC7, 0xC7, 0xC7,
  0xC7, 0xFF, 0x77, 0x07, 0xEE, 0x7C, 0xC1, 0x83, 0x06, 0xFF, 0xF8, 0xF1, 0xE3, 0xC7, 0x8F, 0x18,
  0x7C, 0xFF, 0xFF, 0x2D, 0x86, 0xDB, 0x6D, 0xB7, 0xF0, 0xC0, 0xC0, 0xC0, 0xCE, 0xCC, 0xD8, 0xF8,
  0xF8, 0xDC, 0xCE, 0xC7, 0xFF, 0xFF, 0xFC, 0xDD, 0xEF, 0xFF, 0xC6, 0x3C, 0x63, 0xC6, 0x3C, 0x63,
  0xC6, 0x3C, 0x63, 0xDF, 0xFF, 0x1E, 0x3C, 0x78, 0xF1, 0xE3, 0x7C, 0xEE, 0xC6, 0xC7, 0xC7, 0xC6,
  0xFE, 0x7C, 0xDE, 0xFE, 0xC7, 0xC3, 0xC3, 0xC7, 0xFE, 0xDC, 0xC0, 0xC0, 0xC0, 0x7F, 0xEF, 0xC7,
  0xC7, 0xC7, 0xC7, 0xFF, 0x7F, 0x07, 0x07, 0x07, 0xDF, 0xF1, 0x8C, 0x63, 0x18, 0x7D, 0x9F, 0x07,
  0xC3, 0xE8, 0xFB, 0xBE, 0x63, 0x3E, 0xC6, 0x31, 0x8C, 0x79, 0xC0, 0xC7, 0x8F, 0x1E, 0x3C, 0x78,
  0xFF, 0xBB, 0xE3, 0xB1, 0x98, 0xC6, 0xC3, 0x61, 0xB0, 0x70, 0x38, 0xE7, 0x3B, 0x39, 0x99, 0xCC,
  0xDA, 0xC3, 0xDE, 0x1E, 0xF0, 0xE7, 0x87, 0x18, 0xE3, 0x36, 0x3E, 0x1C, 0x1C, 0x36, 0x76, 0xE3,
  0xE3, 0xB1, 0x98, 0xC6, 0xC3, 0x61, 0xB0, 0x70, 0x38, 0x18, 0x3C, 0x1C, 0x00, 0xFF, 0xF8, 0x71,
  0xC3, 0x0C, 0x3F, 0xFF, 0x09, 0xCC, 0x63, 0x1B, 0x9C, 0x71, 0x8C, 0x63, 0x8C, 0xFF, 0xFF, 0xFF,
  0xFF, 0xF0, 0x87, 0x18, 0xC6, 0x38, 0xE3, 0x73, 0x18, 0xCE, 0x60, 0x73, 0xFF, 0xCE,
};
const GFXglyph CleanLabelGlyphs[] PROGMEM = {
  {0, 1, 1, 4, 0, 0},   // 0x20  
  {1, 3, 11, 5, 1, -10},   // 0x21 !
  {6, 6, 5, 8, 1, -10},   // 0x22 "
  {10, 10, 11, 10, 0, -10},   // 0x23 #
  {24, 8, 13, 10, 1, -11},   // 0x24 $
  {37, 13, 11, 15, 1, -10},   // 0x25 %
  {55, 9, 11, 10, 1, -10},   // 0x26 &
  {68, 3, 5, 5, 1, -10},   // 0x27 '
  {70, 4, 14, 6, 1, -11},   // 0x28 (
  {77, 4, 14, 6, 0, -11},   // 0x29 )
  {84, 6, 6, 8, 1, -10},   // 0x2A *
  {89, 8, 7, 10, 1, -7},   // 0x2B +
  {96, 3, 5, 5, 1, -1},   // 0x2C ,
  {98, 5, 1, 7, 1, -4},   // 0x2D -
  {99, 3, 2, 5, 1, -1},   // 0x2E .
  {100, 6, 13, 6, 0, -10},   // 0x2F /
  {110, 8, 11, 10, 1, -10},   // 0x30 0
  {121, 4, 11, 6, 1, -10},   // 0x31 1
  {127, 8, 11, 9, 1, -10},   // 0x32 2
  {138, 8, 11, 10, 1, -10},   // 0x33 3
  {149, 8, 11, 10, 1, -10},   // 0x34 4
  {160, 8, 11, 9, 1, -10},   // 0x35 5
  {171, 8, 11, 10, 1, -10},   // 0x36 6
  {182, 7, 11, 9, 1, -10},   // 0x37 7
  {192, 8, 11, 10, 1, -10},   // 0x38 8
  {203, 8, 11, 10, 1, -10},   // 0x39 9
  {214, 3, 7, 5, 1, -6},   // 0x3A :
  {217, 3, 10, 5, 1, -6},   // 0x3B ;
  {221, 8, 9, 10, 1, -8},   // 0x3C <
  {230, 8, 5, 10, 1, -6},   // 0x3D =
  {235, 8, 9, 10, 1, -8},   // 0x3E >
  {244, 7, 11, 8, 1, -10},   // 0x3F ?
  {254, 14, 14, 15, 1, -10},   // 0x40 @
  {279, 11, 11, 11, 0, -10},   // 0x41 A
  {295, 8, 11, 10, 1, -10},   // 0x42 B
  {306, 9, 11, 11, 1, -10},   // 0x43 C
  {319, 9, 11, 11, 1, -10},   // 0x44 D
  {332, 7, 11, 9, 1, -10},   // 0x45 E
  {342, 7, 11, 9, 1, -10},   // 0x46 F
  {352, 10, 11, 11, 1, -10},   // 0x47 G
  {366, 9, 11, 11, 1, -10},   // 0x48 H
  {379, 2, 11, 4, 1, -10},   // 0x49 I
  {382, 8, 11, 9, 0, -10},   // 0x4A J
  {393, 9, 11, 11, 1, -10},   // 0x4B K
  {406, 7, 11, 8, 1, -10},   // 0x4C L
  {416, 12, 11, 14, 1, -10},   // 0x4D M
  {433, 9, 11, 11, 1, -10},   // 0x4E N
  {446, 10, 11, 12, 1, -10},   // 0x4F O
  {460, 8, 11, 10, 1, -10},   // 0x50 P
  {471, 10, 12, 12, 1, -10},   // 0x51 Q
  {486, 9, 11, 10, 1, -10},   // 0x52 R
  {499, 8, 11, 10, 1, -10},   // 0x53 S
  {510, 8, 11, 10, 1, -10},   // 0x54 T
  {521, 9, 11, 11, 1, -10},   // 0x55 U
  {534, 11, 11, 11, 0, -10},   // 0x56 V
  {550, 15, 11, 16, 0, -10},   // 0x57 W
  {571, 11, 11, 11, 0, -10},   // 0x58 X
  {587, 11, 11, 11, 0, -10},   // 0x59 Y
  {603, 8, 11, 10, 1, -10},   // 0x5A Z
  {614, 4, 14, 6, 1, -11},   // 0x5B [
  {621, 6, 13, 6, 0, -10},   // 0x5C backslash
  {631, 3, 14, 6, 1, -11},   // 0x5D ]
  {637, 7, 5, 7, 0, -10},   // 0x5E ^
  {642, 7, 2, 7, 0, 1},   // 0x5F _
  {644, 3, 3, 5, 1, -11},   // 0x60 `
  {646, 8, 8, 9, 0, -7},   // 0x61 a
  {654, 8, 11, 9, 1, -10},   // 0x62 b
  {665, 7, 8, 9, 1, -7},   // 0x63 c
  {672, 8, 11, 9, 1, -10},   // 0x64 d
  {683, 7, 8, 9, 1, -7},   // 0x65 e
  {690, 6, 11, 6, 0, -10},   // 0x66 f
  {699, 8, 11, 9, 1, -7},   // 0x67 g
  {710, 7, 11, 9, 1, -10},   // 0x68 h
  {720, 2, 12, 4, 1, -11},   // 0x69 i
  {723, 3, 15, 4, 0, -11},   // 0x6A j
  {729, 8, 11, 9, 1, -10},   // 0x6B k
  {740, 2, 11, 4, 1, -10},   // 0x6C l
  {743, 12, 8, 14, 1, -7},   // 0x6D m
  {755, 7, 8, 9, 1, -7},   // 0x6E n
  {762, 8, 8, 9, 1, -7},   // 0x6F o
  {770, 8, 11, 9, 1, -7},   // 0x70 p
  {781, 8, 11, 9, 1, -7},   // 0x71 q
  {792, 5, 8, 6, 1, -7},   // 0x72 r
  {797, 7, 8, 8, 1, -7},   // 0x73 s
  {804, 5, 10, 6, 0, -9},   // 0x74 t
  {811, 7, 8, 9, 1, -7},   // 0x75 u
  {818, 9, 8, 9, 0, -7},   // 0x76 v
  {827, 13, 8, 13, 0, -7},   // 0x77 w
  {840, 8, 8, 9, 0, -7},   // 0x78 x
  {848, 9, 11, 9, 0, -7},   // 0x79 y
  {861, 7, 8, 9, 1, -7},   // 0x7A z
  {868, 5, 14, 7, 1, -11},   // 0x7B {
  {877, 2, 18, 6, 2, -13},   // 0x7C |
  {882, 5, 14, 7, 1, -11},   // 0x7D }
  {891, 8, 3, 10, 1, -5},   // 0x7E ~
};
const GFXfont CleanLabel PROGMEM = {(uint8_t*)CleanLabelBitmaps, (GFXglyph*)CleanLabelGlyphs, 0x20, 0x7E, 18};

// CleanReadout: generated from inter-800.ttf at 25px
const uint8_t CleanReadoutBitmaps[] PROGMEM = {
  0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0xE0, 0x01, 0xDF, 0xFB, 0x80, 0xF3, 0xDC, 0xF7,
  0x3D, 0xCF, 0x73, 0xDC, 0x77, 0x1D, 0xC7, 0x07, 0x1C, 0x07, 0x1C, 0x0F, 0x1C, 0x0E, 0x1C, 0x3F,
  0xFF, 0x3F, 0xFF, 0x7F, 0xFF, 0x0E, 0x38, 0x1E, 0x38, 0x1C, 0x38, 0x7F, 0xFE, 0x7F, 0xFE, 0xFF,
  0xFE, 0xFF, 0xFE, 0x3C, 0x70, 0x3C, 0x70, 0x38, 0x70, 0x38, 0xF0, 0x01, 0x00, 0x02, 0x00, 0x04,
  0x00, 0xFF, 0x83, 0xFF, 0x8F, 0xFF, 0x9F, 0x5F, 0x7C, 0x9E, 0xF9, 0x00, 0xFA, 0x01, 0xFF, 0x01,
  0xFF, 0x81, 0xFF, 0x80, 0x3F, 0x80, 0x4F, 0x78, 0x9F, 0xF9, 0x3F, 0xFB, 0xF9, 0xFF, 0xF1, 0xFF,
  0xC1, 0xFE, 0x00, 0x20, 0x00, 0x40, 0x00, 0x7E, 0x01, 0xC1, 0xFC, 0x06, 0x0E, 0x70, 0x38, 0x39,
  0xC1, 0xC0, 0xE7, 0x0E, 0x03, 0x9C, 0x38, 0x0F, 0xF1, 0xC0, 0x1F, 0x8E, 0x00, 0x3C, 0x30, 0x00,
  0x01, 0xC3, 0x80, 0x0E, 0x3F, 0x80, 0x30, 0xFE, 0x01, 0xC7, 0x1C, 0x0E, 0x1C, 0x70, 0x30, 0x71,
  0xC1, 0xC0, 0xEE, 0x0E, 0x03, 0xF8, 0x70, 0x07, 0xC0, 0x1F, 0xC0, 0x3F, 0xE0, 0x3F, 0xE0, 0x38,
  0xE0, 0x38, 0xE0, 0x38, 0xE0, 0x3F, 0xE0, 0x3F, 0xC0, 0x1F, 0x80, 0x3F, 0x8E, 0x7F, 0x9E, 0xFB,
  0xDE, 0xF1, 0xFC, 0xF0, 0xFC, 0xF8, 0xFC, 0xFF, 0xFC, 0x7F, 0xFE, 0x1F, 0xDF, 0xF7, 0x77, 0x77,
  0x77, 0x3E, 0x78, 0xF3, 0xC7, 0x8F, 0x3E, 0x7C, 0xF1, 0xE3, 0xC7, 0x8F, 0x1E, 0x3C, 0x7C, 0xF8,
  0xF1, 0xE3, 0xE3, 0xC7, 0xC0, 0xF1, 0xF1, 0xE3, 0xC7, 0xC7, 0x8F, 0x1E, 0x3E, 0x7C, 0xF9, 0xF3,
  0xE7, 0xCF, 0x9E, 0x3C, 0xF9, 0xE3, 0xCF, 0x9E, 0x00, 0x0E, 0x13, 0x26, 0xDF, 0xFF, 0x3F, 0x0F,
  0xEF, 0xFD, 0xB7, 0x4C, 0x43, 0x80, 0x07, 0x00, 0x38, 0x01, 0xC0, 0x0E, 0x00, 0x70, 0x7F, 0xFF,
  0xFF, 0xFF, 0xFF, 0x07, 0x00, 0x38, 0x01, 0xC0, 0x0E, 0x00, 0x70, 0x00, 0x7D, 0xE7, 0x9E, 0x71,
  0xC7, 0x38, 0xFF, 0xFF, 0xFF, 0x77, 0xFE, 0xE0, 0x03, 0xC0, 0xE0, 0x78, 0x1E, 0x07, 0x81, 0xC0,
  0xF0, 0x3C, 0x0E, 0x03, 0x81, 0xE0, 0x78, 0x1C, 0x0F, 0x03, 0xC0, 0xF0, 0x38, 0x1E, 0x07, 0x81,
  0xC0, 0x70, 0x3C, 0x00, 0x0F, 0xE0, 0x7F, 0xF0, 0xFF, 0xF3, 0xE3, 0xE7, 0x83, 0xFF, 0x07, 0xFE,
  0x07, 0xFC, 0x0F, 0xF0, 0x1F, 0xE0, 0x3F, 0xE0, 0x7F, 0xC0, 0xFF, 0x83, 0xEF, 0x07, 0xDF, 0x9F,
  0x1F, 0xFE, 0x1F, 0xF8, 0x1F, 0xC0, 0x1F, 0x9F, 0xFF, 0xFF, 0xFE, 0xFC, 0x7C, 0x3E, 0x1F, 0x0F,
  0x87, 0xC3, 0xE1, 0xF0, 0xF8, 0x7C, 0x3E, 0x1F, 0x0F, 0x87, 0xC0, 0x1F, 0xE0, 0xFF, 0xC7, 0xFF,
  0xBE, 0x3E, 0xF0, 0x7B, 0xC1, 0xF0, 0x07, 0x80, 0x1E, 0x00, 0xF0, 0x0F, 0xC0, 0x7E, 0x03, 0xE0,
  0x1F, 0x00, 0xF8, 0x07, 0xC0, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x1F, 0xE0, 0xFF, 0xC7, 0xFF,
  0x9E, 0x1F, 0xF8, 0x3C, 0x00, 0xF0, 0x07, 0x81, 0xFE, 0x07, 0xC0, 0x1F, 0xE0, 0x0F, 0xC0, 0x0F,
  0x00, 0x3F, 0xE0, 0xFF, 0xCF, 0xDF, 0xFF, 0x3F, 0xF8, 0x7F, 0x80, 0x01, 0xF8, 0x03, 0xF0, 0x0F,
  0xE0, 0x3F, 0xC0, 0x7F, 0x81, 0xEF, 0x07, 0xDE, 0x0F, 0x3C, 0x3C, 0x78, 0xF8, 0xF1, 0xE1, 0xE7,
  0x83, 0xCF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x80, 0x3C, 0x00, 0x78, 0x00, 0xF0, 0x7F, 0xF9, 0xFF,
  0xE7, 0xFF, 0x9E, 0x00, 0x78, 0x01, 0xE0, 0x07, 0x3E, 0x1D, 0xFC, 0xFF, 0xFB, 0xE3, 0xE0, 0x07,
  0xC0, 0x0F, 0x00, 0x3F, 0xC1, 0xFF, 0x8F, 0x9F, 0xFE, 0x3F, 0xF0, 0x7F, 0x00, 0x0F, 0xE0, 0x3F,
  0xF0, 0xFF, 0xE3, 0xE7, 0xE7, 0x83, 0xDF, 0x00, 0x3C, 0x78, 0x7B, 0xFC, 0xFF, 0xFD, 0xF8, 0xFB,
  0xE0, 0xF7, 0xC1, 0xFF, 0x83, 0xEF, 0x07, 0x9F, 0x1F, 0x1F, 0xFC, 0x1F, 0xF8, 0x1F, 0xC0, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFE, 0x01, 0xF0, 0x0F, 0x00, 0xF8, 0x07, 0x80, 0x7C, 0x03, 0xC0, 0x3E, 0x01,
  0xE0, 0x1F, 0x00, 0xF0, 0x0F, 0x80, 0x78, 0x07, 0xC0, 0x3C, 0x03, 0xE0, 0x00, 0x1F, 0xE0, 0x7F,
  0xF1, 0xFF, 0xE3, 0xC3, 0xE7, 0x83, 0xCF, 0x07, 0x9E, 0x1E, 0x1F, 0xFC, 0x0F, 0xE0, 0x7F, 0xF1,
  0xF3, 0xF7, 0xC1, 0xEF, 0x83, 0xFF, 0x07, 0xFE, 0x1F, 0x3F, 0xFE, 0x3F, 0xF8, 0x3F, 0xC0, 0x1F,
  0xE0, 0x7F, 0xE1, 0xFF, 0xE7, 0xC3, 0xEF, 0x03, 0xDE, 0x07, 0xBC, 0x0F, 0x7C, 0x3F, 0xFC, 0xFE,
  0xFF, 0xFC, 0xFE, 0xF0, 0x71, 0xE0, 0x03, 0xDF, 0x0F, 0x9F, 0xBE, 0x3F, 0xFC, 0x3F, 0xF0, 0x3F,
  0x80, 0x77, 0xFE, 0xE0, 0x00, 0x00, 0x03, 0xBF, 0xF7, 0x00, 0x39, 0xF7, 0xCE, 0x00, 0x00, 0x00,
  0x00, 0x17, 0x9E, 0x79, 0xE7, 0x1C, 0x73, 0x80, 0x00, 0x08, 0x01, 0xC0, 0x3E, 0x07, 0xF1, 0xFF,
  0xBF, 0xE3, 0xF8, 0x1F, 0x00, 0xFC, 0x07, 0xFC, 0x0F, 0xFC, 0x0F, 0xF0, 0x1F, 0x80, 0x3C, 0x00,
  0x20, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xF8,
  0x80, 0x07, 0x00, 0x3E, 0x01, 0xFE, 0x0F, 0xFC, 0x0F, 0xF8, 0x0F, 0xE0, 0x1F, 0x01, 0xF8, 0x7F,
  0xDF, 0xF9, 0xFE, 0x0F, 0xC0, 0x78, 0x02, 0x00, 0x00, 0x3F, 0xC7, 0xFE, 0xFF, 0xFF, 0x9F, 0xF0,
  0xF0, 0x0F, 0x01, 0xF0, 0x3E, 0x07, 0xC0, 0xF8, 0x0F, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x0E, 0x01,
  0xF0, 0x1F, 0x00, 0xF0, 0x01, 0xFF, 0x80, 0x07, 0xFF, 0xE0, 0x0F, 0xFF, 0xF0, 0x1F, 0x80, 0xF8,
  0x3E, 0x00, 0x3C, 0x7C, 0x30, 0x1E, 0x78, 0xFD, 0xCE, 0x71, 0xFF, 0xCF, 0xF1, 0xFF, 0xCF, 0xF3,
  0xC3, 0xCF, 0xF3, 0x83, 0xC7, 0xF3, 0x81, 0xCF, 0xF3, 0x81, 0xCF, 0xF3, 0x83, 0xCF, 0xF3, 0xC7,
  0xCE, 0xF1, 0xFF, 0xFE, 0x78, 0xFE, 0xFC, 0x78, 0x78, 0x70, 0x3C, 0x00, 0x00, 0x3F, 0x00, 0x80,
  0x1F, 0xFF, 0xC0, 0x07, 0xFF, 0xC0, 0x01, 0xFF, 0xC0, 0x07, 0xF0, 0x01, 0xFC, 0x00, 0x7F, 0x00,
  0x3D, 0xE0, 0x0F, 0x78, 0x03, 0xDE, 0x01, 0xF7, 0xC0, 0x78, 0xF0, 0x1E, 0x3C, 0x0F, 0x8F, 0x83,
  0xE3, 0xE0, 0xFF, 0xF8, 0x7F, 0xFF, 0x1F, 0xFF, 0xC7, 0x80, 0xF3, 0xE0, 0x3E, 0xF8, 0x0F, 0xBE,
  0x01, 0xF0, 0xFF, 0xF1, 0xFF, 0xF3, 0xFF, 0xF7, 0xC7, 0xEF, 0x83, 0xDF, 0x07, 0xBE, 0x1F, 0x7F,
  0xFC, 0xFF, 0xE1, 0xFF, 0xF3, 0xE1, 0xF7, 0xC1, 0xFF, 0x83, 0xFF, 0x07, 0xFE, 0x3F, 0xFF, 0xFE,
  0xFF, 0xF9, 0xFF, 0xC0, 0x07, 0xF8, 0x0F, 0xFE, 0x0F, 0xFF, 0x8F, 0xE7, 0xE7, 0xC1, 0xF7, 0xC0,
  0x7F, 0xE0, 0x01, 0xF0, 0x00, 0xF0, 0x00, 0x78, 0x00, 0x3E, 0x00, 0x1F, 0x00, 0x1F, 0x80, 0xF3,
  0xE0, 0xF9, 0xFF, 0xFC, 0x7F, 0xFC, 0x1F, 0xFC, 0x03, 0xF8, 0x00, 0xFF, 0xE0, 0xFF, 0xF8, 0xFF,
  0xFC, 0xFF, 0xFE, 0xF8, 0x3E, 0xF8, 0x1F, 0xF8, 0x1F, 0xF8, 0x0F, 0xF8, 0x0F, 0xF8, 0x0F, 0xF8,
  0x0F, 0xF8, 0x1F, 0xF8, 0x1F, 0xF8, 0x3E, 0xFF, 0xFE, 0xFF, 0xFC, 0xFF, 0xF8, 0xFF, 0xC0, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x0F, 0x80, 0x7C, 0x03, 0xE0, 0x1F, 0xFE, 0xFF, 0xF7, 0xFF, 0xBF,
  0xFD, 0xF0, 0x0F, 0x80, 0x7C, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC0, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xF0, 0x0F, 0x80, 0x7C, 0x03, 0xE0, 0x1F, 0x00, 0xFF, 0xF7, 0xFF, 0xBF, 0xFD, 0xF0,
  0x0F, 0x80, 0x7C, 0x03, 0xE0, 0x1F, 0x00, 0xF8, 0x07, 0xC0, 0x00, 0x07, 0xF8, 0x0F, 0xFE, 0x0F,
  0xFF, 0x8F, 0xE7, 0xE7, 0xC0, 0xF7, 0xC0, 0x7F, 0xE0, 0x01, 0xF0, 0x00, 0xF0, 0x7F, 0xF8, 0x3F,
  0xFE, 0x1F, 0xFF, 0x00, 0xFF, 0x80, 0xFB, 0xE0, 0x79, 0xFF, 0xFC, 0x7F, 0xFC, 0x1F, 0xFC, 0x03,
  0xF8, 0x00, 0xF8, 0x0F, 0xF8, 0x0F, 0xF8, 0x0F, 0xF8, 0x0F, 0xF8, 0x0F, 0xF8, 0x0F, 0xF8, 0x0F,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF8, 0x0F, 0xF8, 0x0F, 0xF8, 0x0F, 0xF8, 0x0F, 0xF8, 0x0F,
  0xF8, 0x0F, 0xF8, 0x0F, 0xF8, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xC0, 0x00, 0xF0, 0x0F, 0x00, 0xF0, 0x0F, 0x00, 0xF0, 0x0F, 0x00, 0xF0, 0x0F, 0x00, 0xF0,
  0x0F, 0x00, 0xFF, 0x0F, 0xF0, 0xFF, 0x0F, 0xF9, 0xFF, 0xFF, 0x7F, 0xE3, 0xFC, 0xF8, 0x1F, 0xFC,
  0x1F, 0x3E, 0x1F, 0x1F, 0x1F, 0x8F, 0x8F, 0x87, 0xCF, 0x83, 0xEF, 0x81, 0xFF, 0x80, 0xFF, 0xE0,
  0x7F, 0xF0, 0x3F, 0xFC, 0x1F, 0x9F, 0x0F, 0x8F, 0x87, 0xC3, 0xE3, 0xE0, 0xF9, 0xF0, 0x7C, 0xF8,
  0x1F, 0x7C, 0x0F, 0xC0, 0xF8, 0x0F, 0x80, 0xF8, 0x0F, 0x80, 0xF8, 0x0F, 0x80, 0xF8, 0x0F, 0x80,
  0xF8, 0x0F, 0x80, 0xF8, 0x0F, 0x80, 0xF8, 0x0F, 0x80, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE,
  0x03, 0xFF, 0xF8, 0x1F, 0xFF, 0xC0, 0xFF, 0xFE, 0x0F, 0xFF, 0xF8, 0x7F, 0xFF, 0xC3, 0xBF, 0xFE,
  0x3D, 0xFF, 0x79, 0xEF, 0xFB, 0xCF, 0x7F, 0xDE, 0x73, 0xFE, 0xF7, 0x9F, 0xF3, 0xFC, 0xFF, 0x9F,
  0xC7, 0xFC, 0xFE, 0x3F, 0xE3, 0xF1, 0xFF, 0x1F, 0x0F, 0xF8, 0xF8, 0x7F, 0xC3, 0xC3, 0xC0, 0xFC,
  0x0F, 0xFE, 0x07, 0xFF, 0x83, 0xFF, 0xC1, 0xFF, 0xF0, 0xFF, 0xFC, 0x7F, 0xFE, 0x3F, 0xF7, 0x9F,
  0xFB, 0xEF, 0xFC, 0xF7, 0xFE, 0x7F, 0xFF, 0x1F, 0xFF, 0x87, 0x7F, 0xC3, 0xFF, 0xE0, 0xFF, 0xF0,
  0x7F, 0xF8, 0x1F, 0xFC, 0x07, 0xC0, 0x07, 0xF8, 0x0F, 0xFE, 0x0F, 0xFF, 0x8F, 0xE7, 0xE7, 0xC0,
  0xF7, 0xC0, 0x7F, 0xE0, 0x1F, 0xF0, 0x0F, 0xF0, 0x07, 0xF8, 0x03, 0xFE, 0x01, 0xFF, 0x00, 0xFF,
  0x80, 0xFB, 0xE0, 0xF9, 0xFF, 0xFC, 0x7F, 0xFC, 0x1F, 0xFC, 0x03, 0xF8, 0x00, 0xFF, 0xE3, 0xFF,
  0xEF, 0xFF, 0xBE, 0x3F, 0xF8, 0x3F, 0xE0, 0xFF, 0x83, 0xFE, 0x0F, 0xF8, 0x7F, 0xFF, 0xFF, 0xFF,
  0xBF, 0xF8, 0xF8, 0x03, 0xE0, 0x0F, 0x80, 0x3E, 0x00, 0xF8, 0x03, 0xE0, 0x00, 0x07, 0xF8, 0x0F,
  0xFE, 0x0F, 0xFF, 0x8F, 0xE7, 0xE7, 0xC0, 0xF7, 0xC0, 0x7F, 0xE0, 0x1F, 0xF0, 0x0F, 0xF0, 0x07,
  0xF8, 0x03, 0xFE, 0x01, 0xFF, 0x20, 0xFF, 0x8F, 0xFB, 0xE3, 0xF9, 0xFF, 0xFC, 0x7F, 0xFC, 0x1F,
  0xFE, 0x03, 0xFF, 0x80, 0x01, 0xE0, 0xFF, 0xE1, 0xFF, 0xF3, 0xFF, 0xF7, 0xC7, 0xEF, 0x83, 0xDF,
  0x07, 0xBE, 0x0F, 0x7C, 0x3E, 0xFF, 0xFD, 0xFF, 0xF3, 0xFF, 0xC7, 0xFF, 0x8F, 0x9F, 0x1F, 0x1F,
  0x3E, 0x1E, 0x7C, 0x3E, 0xF8, 0x3D, 0xF0, 0x7C, 0x1F, 0xF0, 0x7F, 0xF1, 0xFF, 0xF3, 0xE3, 0xEF,
  0x83, 0xDF, 0x00, 0x1F, 0x00, 0x3F, 0xE0, 0x3F, 0xF0, 0x3F, 0xF0, 0x07, 0xF0, 0x01, 0xEF, 0x03,
  0xFF, 0x07, 0xFF, 0x3F, 0x3F, 0xFE, 0x3F, 0xF8, 0x3F, 0xE0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF8,
  0x3E, 0x00, 0x7C, 0x00, 0xF8, 0x01, 0xF0, 0x03, 0xE0, 0x07, 0xC0, 0x0F, 0x80, 0x1F, 0x00, 0x3E,
  0x00, 0x7C, 0x00, 0xF8, 0x01, 0xF0, 0x03, 0xE0, 0x07, 0xC0, 0x0F, 0x80, 0xF8, 0x1F, 0xF8, 0x1F,
  0xF8, 0x1F, 0xF8, 0x1F, 0xF8, 0x1F, 0xF8, 0x1F, 0xF8, 0x1F, 0xF8, 0x1F, 0xF8, 0x1F, 0xF8, 0x1F,
  0xF8, 0x1F, 0xF8, 0x1F, 0xF8, 0x1F, 0x7C, 0x1F, 0x7F, 0xFE, 0x3F, 0xFE, 0x1F, 0xFC, 0x0F, 0xF0,
  0xF8, 0x07, 0xFE, 0x03, 0xEF, 0x80, 0xF9, 0xF0, 0x3C, 0x7C, 0x1F, 0x0F, 0x07, 0xC3, 0xE1, 0xE0,
  0xF8, 0x78, 0x1E, 0x3E, 0x07, 0x8F, 0x01, 0xF3, 0xC0, 0x3D, 0xF0, 0x0F, 0x78, 0x03, 0xFE, 0x00,
  0x7F, 0x80, 0x1F, 0xC0, 0x07, 0xF0, 0x00, 0xFC, 0x00, 0xF8, 0x3E, 0x0F, 0xFC, 0x1F, 0x07, 0xBE,
  0x1F, 0x83, 0xCF, 0x0F, 0xC3, 0xE7, 0x87, 0xF1, 0xF3, 0xE3, 0xF8, 0xF1, 0xF3, 0xFC, 0x78, 0x79,
  0xEE, 0x7C, 0x3C, 0xF7, 0x3E, 0x1E, 0x73, 0xDE, 0x0F, 0xB9, 0xEF, 0x03, 0xFC, 0x77, 0x81, 0xFE,
  0x3B, 0xC0, 0xFE, 0x1F, 0xC0, 0x7F, 0x0F, 0xE0, 0x1F, 0x83, 0xF0, 0x0F, 0xC1, 0xF0, 0x07, 0xC0,
  0xF8, 0x00, 0xFC, 0x1F, 0xBE, 0x0F, 0x8F, 0x8F, 0x83, 0xE7, 0x81, 0xF7, 0xC0, 0x7F, 0xC0, 0x1F,
  0xC0, 0x0F, 0xE0, 0x03, 0xE0, 0x01, 0xF8, 0x01, 0xFC, 0x01, 0xFF, 0x01, 0xF7, 0xC0, 0xFB, 0xE0,
  0xF8, 0xF8, 0xF8, 0x3E, 0x7C, 0x1F, 0x7C, 0x07, 0xC0, 0xF8, 0x0F, 0xBC, 0x0F, 0x9F, 0x07, 0xC7,
  0xC7, 0xC3, 0xE3, 0xE0, 0xFB, 0xE0, 0x7D, 0xE0, 0x1E, 0xF0, 0x07, 0xF0, 0x03, 0xF8, 0x00, 0xF8,
  0x00, 0x7C, 0x00, 0x3E, 0x00, 0x1F, 0x00, 0x0F, 0x80, 0x07, 0xC0, 0x03, 0xE0, 0x01, 0xF0, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF8, 0x03, 0xE0, 0x0F, 0x80, 0x1E, 0x00, 0x7C, 0x01, 0xF0, 0x07,
  0xC0, 0x0F, 0x00, 0x3E, 0x00, 0xF8, 0x01, 0xE0, 0x07, 0x80, 0x1F, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFC, 0xFF, 0xFF, 0xFC, 0xF3, 0xCF, 0x3C, 0xF3, 0xCF, 0x3C, 0xF3, 0xCF, 0x3C, 0xF3, 0xCF,
  0x3F, 0xFF, 0xF0, 0xF0, 0x1C, 0x07, 0x81, 0xE0, 0x78, 0x0E, 0x03, 0xC0, 0xF0, 0x1C, 0x07, 0x01,
  0xE0, 0x78, 0x0E, 0x03, 0xC0, 0xF0, 0x3C, 0x07, 0x01, 0xE0, 0x78, 0x0E, 0x03, 0x80, 0xF0, 0xFF,
  0xFF, 0xF9, 0xF3, 0xE7, 0xCF, 0x9F, 0x3E, 0x7C, 0xF9, 0xF3, 0xE7, 0xCF, 0x9F, 0x3E, 0x7C, 0xFF,
  0xFF, 0xFF, 0xC0, 0x1E, 0x03, 0xC0, 0xFC, 0x1F, 0x87, 0x38, 0xE7, 0x1C, 0x77, 0x0F, 0xFF, 0xFF,
  0xFF, 0xFF, 0xF0, 0xF3, 0xCE, 0x30, 0x0F, 0x83, 0xFE, 0x7F, 0xFF, 0x9F, 0x10, 0xF0, 0x1F, 0x3F,
  0xF7, 0xFF, 0xF8, 0xFF, 0x0F, 0xF0, 0xFF, 0xFF, 0xFF, 0xF7, 0xEF, 0xF8, 0x03, 0xE0, 0x0F, 0x80,
  0x3E, 0x00, 0xF9, 0xC3, 0xFF, 0xCF, 0xFF, 0xBF, 0xFE, 0xF8, 0x7F, 0xE1, 0xFF, 0x83, 0xFE, 0x0F,
  0xF8, 0x3F, 0xE1, 0xFF, 0xCF, 0xFF, 0xFE, 0xFF, 0xF3, 0xEF, 0x80, 0x0F, 0x81, 0xFF, 0x1F, 0xFC,
  0xFF, 0xFF, 0x87, 0xF8, 0x23, 0xC0, 0x1E, 0x00, 0xF0, 0x07, 0xC3, 0xFE, 0x3E, 0xFF, 0xE3, 0xFF,
  0x0F, 0xE0, 0x00, 0x7C, 0x01, 0xF0, 0x07, 0xC0, 0x1F, 0x0E, 0x7C, 0xFD, 0xF7, 0xFF, 0xDF, 0xFF,
  0xF8, 0x7F, 0xC1, 0xFF, 0x07, 0xFC, 0x1F, 0xF0, 0x7F, 0xE1, 0xFF, 0xCF, 0xDF, 0xFF, 0x3F, 0xFC,
  0x7D, 0xF0, 0x0F, 0x01, 0xFF, 0x1F, 0xFC, 0xF9, 0xEF, 0x07, 0xF8, 0x3F, 0xFF, 0xFF, 0xFF, 0xF0,
  0x07, 0x80, 0x3E, 0x1E, 0xFF, 0xF3, 0xFF, 0x0F, 0xE0, 0x07, 0xC7, 0xF1, 0xFC, 0x7C, 0x1E, 0x3F,
  0xFF, 0xFF, 0xFF, 0xFF, 0xC7, 0x81, 0xE0, 0x78, 0x1E, 0x07, 0x81, 0xE0, 0x78, 0x1E, 0x07, 0x81,
  0xE0, 0x0E, 0x7C, 0xFD, 0xF7, 0xFF, 0xDF, 0xFF, 0xF8, 0x7F, 0xC1, 0xFF, 0x07, 0xFC, 0x1F, 0xF0,
  0x7F, 0xE1, 0xFF, 0xDF, 0xDF, 0xFF, 0x3F, 0xFC, 0x79, 0xF0, 0x07, 0xEC, 0x1E, 0x7F, 0xF9, 0xFF,
  0xC1, 0xFE, 0x00, 0xF8, 0x07, 0xC0, 0x3E, 0x01, 0xF0, 0x0F, 0x9C, 0x7D, 0xFB, 0xFF, 0xFF, 0xFF,
  0xF8, 0x7F, 0xC3, 0xFE, 0x1F, 0xF0, 0xFF, 0x87, 0xFC, 0x3F, 0xE1, 0xFF, 0x0F, 0xF8, 0x7F, 0xC3,
  0xC0, 0x77, 0xFE, 0x60, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0x1C, 0x7C, 0xF8,
  0x60, 0x07, 0xCF, 0x9F, 0x3E, 0x7C, 0xF9, 0xF3, 0xE7, 0xCF, 0x9F, 0x3E, 0x7C, 0xF9, 0xF3, 0xFF,
  0xBF, 0x7C, 0xF8, 0x03, 0xE0, 0x0F, 0x80, 0x3E, 0x00, 0xF8, 0x7F, 0xE3, 0xEF, 0x9F, 0x3E, 0xF8,
  0xFF, 0xC3, 0xFE, 0x0F, 0xF8, 0x3F, 0xF0, 0xFF, 0xC3, 0xEF, 0x8F, 0x9F, 0x3E, 0x7C, 0xF8, 0xFB,
  0xE1, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC0, 0xF1, 0xC1,
  0xC7, 0xBF, 0x3F, 0xBD, 0xFF, 0xFD, 0xFF, 0xFF, 0xFF, 0x8F, 0x8F, 0xFC, 0x3C, 0x7F, 0xE1, 0xE1,
  0xFF, 0x0F, 0x0F, 0xF8, 0x78, 0x7F, 0xC3, 0xC3, 0xFE, 0x1E, 0x1F, 0xF0, 0xF0, 0xFF, 0x87, 0x87,
  0xFC, 0x3C, 0x3C, 0xF1, 0xC7, 0x9F, 0xBD, 0xFF, 0xFF, 0xFF, 0x87, 0xFC, 0x3F, 0xE1, 0xFF, 0x0F,
  0xF8, 0x7F, 0xC3, 0xFE, 0x1F, 0xF0, 0xFF, 0x87, 0xFC, 0x3C, 0x0F, 0x80, 0xFF, 0x87, 0xFF, 0x1F,
  0xFE, 0xF8, 0x7B, 0xC1, 0xEF, 0x07, 0xFC, 0x1F, 0xF0, 0x7F, 0xE1, 0xEF, 0x8F, 0x9F, 0xFC, 0x3F,
  0xF0, 0x7F, 0x00, 0xF9, 0xC3, 0xFF, 0xCF, 0xFF, 0xBF, 0xFE, 0xF8, 0x7F, 0xE1, 0xFF, 0x83, 0xFE,
  0x0F, 0xF8, 0x3F, 0xE1, 0xFF, 0xCF, 0xFF, 0xFE, 0xFF, 0xF3, 0xEF, 0x8F, 0x80, 0x3E, 0x00, 0xF8,
  0x03, 0xE0, 0x0F, 0x80, 0x00, 0x0E, 0x7C, 0xFD, 0xF7, 0xFF, 0xDF, 0xFF, 0xF8, 0x7F, 0xC1, 0xFF,
  0x07, 0xFC, 0x1F, 0xF0, 0x7F, 0xE1, 0xFF, 0xCF, 0xDF, 0xFF, 0x3F, 0xFC, 0x7D, 0xF0, 0x07, 0xC0,
  0x1F, 0x00, 0x7C, 0x01, 0xF0, 0x07, 0xC0, 0xFB, 0xFF, 0xFF, 0xFF, 0xFF, 0xC7, 0xC3, 0xE1, 0xF0,
  0xF8, 0x7C, 0x3E, 0x1F, 0x0F, 0x87, 0xC0, 0x0F, 0x01, 0xFF, 0x1F, 0xFD, 0xF1, 0xEF, 0x88, 0x7E,
  0x01, 0xFF, 0x07, 0xFE, 0x0F, 0xF0, 0x87, 0xFC, 0x3D, 0xFF, 0xE7, 0xFE, 0x1F, 0xE0, 0x3C, 0x1E,
  0x0F, 0x1F, 0xFF, 0xFF, 0xFF, 0xFE, 0x78, 0x3C, 0x1E, 0x0F, 0x07, 0x83, 0xC1, 0xE0, 0xFE, 0x3F,
  0x0F, 0x80, 0xF8, 0x7F, 0xC3, 0xFE, 0x1F, 0xF0, 0xFF, 0x87, 0xFC, 0x3F, 0xE1, 0xFF, 0x0F, 0xF8,
  0x7F, 0xC3, 0xDF, 0x3E, 0xFF, 0xF7, 0xFF, 0x8F, 0xBC, 0xF8, 0x1E, 0xF0, 0x7D, 0xE0, 0xF3, 0xE1,
  0xE3, 0xC7, 0xC7, 0x8F, 0x0F, 0x9E, 0x0F, 0x3C, 0x1E, 0xF0, 0x1D, 0xE0, 0x3F, 0x80, 0x7F, 0x00,
  0x7E, 0x00, 0xF8, 0x00, 0xF8, 0x78, 0x7B, 0xC3, 0xC3, 0xDE, 0x3E, 0x3E, 0xF1, 0xF1, 0xE7, 0xCF,
  0xCF, 0x1E, 0x7E, 0x78, 0xF7, 0xF3, 0xC7, 0xBB, 0xBC, 0x1D, 0xDF, 0xE0, 0xFE, 0x7F, 0x07, 0xF3,
  0xF0, 0x3F, 0x1F, 0x80, 0xF8, 0x7C, 0x07, 0xC3, 0xE0, 0x78, 0x3D, 0xF1, 0xF3, 0xC7, 0x87, 0xBC,
  0x1E, 0xF0, 0x3F, 0x80, 0xFE, 0x03, 0xF0, 0x0F, 0xE0, 0x7B, 0xC3, 0xEF, 0x0F, 0x1E, 0x7C, 0x7F,
  0xE0, 0xF0, 0xF8, 0x1E, 0xF0, 0x7D, 0xE0, 0xF3, 0xE1, 0xE3, 0xC7, 0xC7, 0x8F, 0x07, 0x9E, 0x0F,
  0x78, 0x1E, 0xF0, 0x1D, 0xE0, 0x3F, 0x80, 0x7F, 0x00, 0x7E, 0x00, 0xF8, 0x01, 0xF0, 0x03, 0xC0,
  0x3F, 0x80, 0x7E, 0x01, 0xF8, 0x00, 0x7F, 0xF7, 0xFF, 0x7F, 0xF7, 0xFF, 0x03, 0xE0, 0x7C, 0x07,
  0x80, 0xF0, 0x1F, 0x03, 0xE0, 0x7C, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x8F, 0xC7, 0xE7, 0xC3,
  0xC1, 0xE0, 0xF0, 0x78, 0x3C, 0x7E, 0x3C, 0x1E, 0x0F, 0xC1, 0xE0, 0xF0, 0x78, 0x3C, 0x1E, 0x0F,
  0x83, 0xF1, 0xF8, 0x7C, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xF8, 0x7E, 0x3F, 0x07, 0xC1, 0xE0, 0xF0, 0x78, 0x3C, 0x1E, 0x0F, 0xC1, 0xE0,
  0xF1, 0xF8, 0xF0, 0x78, 0x3C, 0x1E, 0x0F, 0x0F, 0x9F, 0x8F, 0xC7, 0xC0, 0x7C, 0x3B, 0xF1, 0xFF,
  0xFF, 0xEF, 0xFE, 0x3F, 0x00, 0x20,
};
const GFXglyph CleanReadoutGlyphs[] PROGMEM = {
  {0, 1, 1, 5, 0, 0},   // 0x20  
  {1, 5, 18, 9, 2, -17},   // 0x21 !
  {13, 10, 8, 15, 2, -17},   // 0x22 "
  {23, 16, 18, 16, 0, -17},   // 0x23 #
  {59, 15, 23, 17, 1, -20},   // 0x24 $
  {103, 22, 18, 26, 2, -17},   // 0x25 %
  {153, 16, 18, 17, 1, -17},   // 0x26 &
  {189, 4, 8, 9, 2, -17},   // 0x27 '
  {193, 7, 22, 10, 2, -18},   // 0x28 (
  {213, 7, 22, 10, 1, -18},   // 0x29 )
  {233, 10, 10, 15, 2, -17},   // 0x2A *
  {246, 13, 13, 17, 2, -13},   // 0x2B +
  {268, 6, 8, 9, 1, -2},   // 0x2C ,
  {274, 8, 3, 12, 2, -8},   // 0x2D -
  {277, 5, 4, 9, 2, -3},   // 0x2E .
  {280, 10, 22, 10, 0, -18},   // 0x2F /
  {308, 15, 18, 17, 1, -17},   // 0x30 0
  {342, 9, 18, 11, 1, -17},   // 0x31 1
  {363, 14, 18, 16, 1, -17},   // 0x32 2
  {395, 14, 18, 16, 1, -17},   // 0x33 3
  {427, 15, 18, 17, 1, -17},   // 0x34 4
  {461, 14, 18, 16, 1, -17},   // 0x35 5
  {493, 15, 18, 17, 1, -17},   // 0x36 6
  {527, 13, 18, 15, 1, -17},   // 0x37 7
  {557, 15, 18, 17, 1, -17},   // 0x38 8
  {591, 15, 18, 17, 1, -17},   // 0x39 9
  {625, 5, 13, 9, 2, -12},   // 0x3A :
  {634, 6, 18, 9, 1, -12},   // 0x3B ;
  {648, 13, 15, 17, 2, -14},   // 0x3C <
  {673, 13, 9, 17, 2, -11},   // 0x3D =
  {688, 13, 15, 17, 2, -14},   // 0x3E >
  {713, 12, 18, 14, 1, -17},   // 0x3F ?
  {740, 24, 23, 26, 1, -17},   // 0x40 @
  {809, 18, 18, 19, 1, -17},   // 0x41 A
  {850, 15, 18, 17, 1, -17},   // 0x42 B
  {884, 17, 18, 19, 1, -17},   // 0x43 C
  {923, 16, 18, 18, 1, -17},   // 0x44 D
  {959, 13, 18, 15, 1, -17},   // 0x45 E
  {989, 13, 18, 15, 1, -17},   // 0x46 F
  {1019, 17, 18, 19, 1, -17},   // 0x47 G
  {1058, 16, 18, 19, 1, -17},   // 0x48 H
  {1094, 5, 18, 7, 1, -17},   // 0x49 I
  {1106, 12, 18, 15, 1, -17},   // 0x4A J
  {1133, 17, 18, 18, 1, -17},   // 0x4B K
  {1172, 12, 18, 14, 1, -17},   // 0x4C L
  {1199, 21, 18, 24, 1, -17},   // 0x4D M
  {1247, 17, 18, 19, 1, -17},   // 0x4E N
  {1286, 17, 18, 19, 1, -17},   // 0x4F O
  {1325, 14, 18, 16, 1, -17},   // 0x50 P
  {1357, 17, 19, 20, 1, -17},   // 0x51 Q
  {1398, 15, 18, 17, 1, -17},   // 0x52 R
  {1432, 15, 18, 17, 1, -17},   // 0x53 S
  {1466, 15, 18, 17, 1, -17},   // 0x54 T
  {1500, 16, 18, 18, 1, -17},   // 0x55 U
  {1536, 18, 18, 19, 1, -17},   // 0x56 V
  {1577, 25, 18, 26, 1, -17},   // 0x57 W
  {1634, 17, 18, 19, 1, -17},   // 0x58 X
  {1673, 17, 18, 19, 1, -17},   // 0x59 Y
  {1712, 15, 18, 17, 1, -17},   // 0x5A Z
  {1746, 6, 22, 10, 2, -18},   // 0x5B [
  {1763, 10, 22, 10, 0, -18},   // 0x5C backslash
  {1791, 7, 22, 10, 1, -18},   // 0x5D ]
  {1811, 11, 8, 12, 1, -17},   // 0x5E ^
  {1822, 12, 3, 12, 0, 1},   // 0x5F _
  {1827, 5, 4, 10, 2, -18},   // 0x60 `
  {1830, 12, 14, 15, 1, -13},   // 0x61 a
  {1851, 14, 18, 16, 1, -17},   // 0x62 b
  {1883, 13, 14, 15, 1, -13},   // 0x63 c
  {1906, 14, 18, 16, 1, -17},   // 0x64 d
  {1938, 13, 14, 15, 1, -13},   // 0x65 e
  {1961, 10, 19, 10, 0, -18},   // 0x66 f
  {1985, 14, 19, 16, 1, -13},   // 0x67 g
  {2019, 13, 18, 16, 1, -17},   // 0x68 h
  {2049, 5, 19, 7, 1, -18},   // 0x69 i
  {2061, 7, 24, 7, -1, -18},   // 0x6A j
  {2082, 14, 18, 15, 1, -17},   // 0x6B k
  {2114, 5, 18, 7, 1, -17},   // 0x6C l
  {2126, 21, 14, 23, 1, -13},   // 0x6D m
  {2163, 13, 14, 16, 1, -13},   // 0x6E n
  {2186, 14, 14, 15, 1, -13},   // 0x6F o
  {2211, 14, 19, 16, 1, -13},   // 0x70 p
  {2245, 14, 19, 16, 1, -13},   // 0x71 q
  {2279, 9, 14, 11, 1, -13},   // 0x72 r
  {2295, 13, 14, 14, 1, -13},   // 0x73 s
  {2318, 9, 17, 10, 0, -16},   // 0x74 t
  {2338, 13, 14, 16, 1, -13},   // 0x75 u
  {2361, 15, 14, 15, 0, -13},   // 0x76 v
  {2388, 21, 14, 22, 0, -13},   // 0x77 w
  {2425, 14, 14, 15, 0, -13},   // 0x78 x
  {2450, 15, 19, 15, 0, -13},   // 0x79 y
  {2486, 12, 14, 15, 1, -13},   // 0x7A z
  {2507, 9, 22, 12, 2, -18},   // 0x7B {
  {2532, 4, 30, 10, 3, -23},   // 0x7C |
  {2547, 9, 22, 12, 1, -18},   // 0x7D }
  {2572, 13, 6, 17, 2, -9},   // 0x7E ~
};
const GFXfont CleanReadout PROGMEM = {(uint8_t*)CleanReadoutBitmaps, (GFXglyph*)CleanReadoutGlyphs, 0x20, 0x7E, 30};

// CleanBig: generated from inter-700.ttf at 21px
const uint8_t CleanBigBitmaps[] PROGMEM = {
  0x00, 0xFF, 0xFF, 0xFF, 0xFC, 0x0F, 0xF8, 0xE7, 0xE6, 0xE6, 0xE6, 0xE6, 0xE6, 0x0E, 0x70, 0x63,
  0x83, 0x18, 0x18, 0xC7, 0xFF, 0xBF, 0xFC, 0x63, 0x83, 0x1C, 0x18, 0xCB, 0xFF, 0xBF, 0xFC, 0x71,
  0x83, 0x1C, 0x18, 0xE0, 0xC6, 0x00, 0x06, 0x00, 0x60, 0x3F, 0xC7, 0xFE, 0x76, 0xEF, 0x67, 0xF6,
  0x07, 0xE0, 0x7F, 0x03, 0xFC, 0x07, 0xE0, 0x6F, 0xE6, 0x7E, 0x6F, 0xFF, 0xE7, 0xFE, 0x3F, 0xC0,
  0x60, 0x06, 0x00, 0x10, 0x00, 0x3E, 0x07, 0x3B, 0x03, 0x19, 0xC3, 0x0C, 0xE3, 0x06, 0x61, 0x83,
  0xF1, 0x80, 0xF1, 0x80, 0x01, 0xC0, 0x00, 0xC7, 0x80, 0xC7, 0xE0, 0xE3, 0x30, 0x61, 0x98, 0x60,
  0xCC, 0x60, 0x7E, 0x70, 0x1E, 0x3F, 0x01, 0xFC, 0x1C, 0xE0, 0xE3, 0x07, 0x38, 0x1F, 0x80, 0xF8,
  0x07, 0x80, 0x7E, 0x77, 0x3B, 0xB8, 0xFD, 0xC3, 0xCF, 0x3E, 0x3F, 0xF8, 0xFD, 0xE0, 0xFF, 0xFF,
  0xC0, 0x3B, 0xDC, 0xEF, 0x73, 0x9C, 0xE7, 0x39, 0xCE, 0x73, 0xCE, 0x71, 0xCE, 0xE7, 0x1C, 0xE7,
  0x1C, 0xE7, 0x39, 0xCE, 0x73, 0x9D, 0xCE, 0x77, 0x38, 0x18, 0x9A, 0xFF, 0x7E, 0x7C, 0xFF, 0x9A,
  0x18, 0x0C, 0x03, 0x00, 0xC0, 0x30, 0xFF, 0xFF, 0xF0, 0xC0, 0x30, 0x0C, 0x03, 0x00, 0x77, 0x66,
  0xEC, 0xFF, 0xFF, 0xF8, 0xFF, 0x80, 0x07, 0x06, 0x06, 0x0E, 0x0E, 0x0C, 0x1C, 0x1C, 0x1C, 0x18,
  0x38, 0x38, 0x38, 0x30, 0x70, 0x70, 0x60, 0xE0, 0x1F, 0x83, 0xFC, 0x79, 0xE7, 0x0F, 0xE0, 0x7E,
  0x07, 0xE0, 0x7E, 0x07, 0xE0, 0x7E, 0x07, 0xF0, 0x77, 0x0F, 0x79, 0xE3, 0xFC, 0x1F, 0x80, 0x3E,
  0xFF, 0xBE, 0x78, 0xE1, 0xC3, 0x87, 0x0E, 0x1C, 0x38, 0x70, 0xE1, 0xC3, 0x80, 0x3F, 0x8F, 0xFB,
  0xC7, 0xF0, 0x70, 0x0E, 0x01, 0xC0, 0x70, 0x1C, 0x07, 0x81, 0xC0, 0x70, 0x1C, 0x07, 0xFF, 0xFF,
  0xFF, 0xF8, 0x3F, 0x87, 0xFC, 0x71, 0xEE, 0x0E, 0x00, 0xE0, 0x1E, 0x0F, 0xC0, 0xF8, 0x01, 0xE0,
  0x0E, 0x00, 0xFE, 0x0F, 0xF9, 0xE7, 0xFC, 0x1F, 0x80, 0x03, 0xC0, 0x7C, 0x07, 0xC0, 0xFC, 0x1D,
  0xC1, 0xDC, 0x39, 0xC7, 0x9C, 0x71, 0xCE, 0x1C, 0xFF, 0xFF, 0xFF, 0x01, 0xC0, 0x1C, 0x01, 0xC0,
  0x7F, 0xCF, 0xF9, 0xC0, 0x30, 0x06, 0x01, 0xDF, 0x3F, 0xF7, 0x8F, 0x00, 0xE0, 0x1C, 0x03, 0xF0,
  0x7F, 0x1C, 0xFF, 0x8F, 0xC0, 0x1F, 0x83, 0xFE, 0x79, 0xE7, 0x0F, 0xE0, 0x0E, 0x78, 0xEF, 0xCF,
  0x9E, 0xF0, 0xFE, 0x07, 0xE0, 0x77, 0x0E, 0x79, 0xE3, 0xFC, 0x1F, 0x80, 0xFF, 0xFF, 0xF0, 0x1C,
  0x07, 0x03, 0x80, 0xE0, 0x70, 0x1C, 0x0E, 0x03, 0x81, 0xC0, 0x70, 0x38, 0x0E, 0x07, 0x00, 0x3F,
  0x87, 0xFC, 0x71, 0xE7, 0x0E, 0x70, 0xE7, 0x1E, 0x3F, 0xC3, 0xFC, 0x79, 0xEF, 0x0E, 0xE0, 0xFE,
  0x0F, 0xF1, 0xE7, 0xFE, 0x3F, 0x80, 0x04, 0x03, 0xF8, 0x7F, 0xCF, 0x1E, 0xE0, 0xEE, 0x0E, 0xE0,
  0xFF, 0x0F, 0x7F, 0xF7, 0xF7, 0x1C, 0xE0, 0x0E, 0xF0, 0xE7, 0xBC, 0x7F, 0xC1, 0xF0, 0xFF, 0x80,
  0x00, 0xFF, 0x80, 0x77, 0x70, 0x00, 0x00, 0x07, 0x76, 0x6E, 0xC0, 0x00, 0x40, 0x70, 0x7C, 0xFE,
  0xFC, 0x3C, 0x0F, 0x03, 0xF8, 0x1F, 0x81, 0xF0, 0x1C, 0x01, 0xFF, 0xFF, 0xFF, 0xFC, 0x00, 0x00,
  0x3F, 0xFF, 0xFF, 0xFF, 0x80, 0x1E, 0x03, 0xF0, 0x3F, 0x80, 0xFC, 0x07, 0xC0, 0xF8, 0xFE, 0x7F,
  0x1F, 0x03, 0x80, 0x40, 0x00, 0x7F, 0x3F, 0xEE, 0x3F, 0x87, 0x03, 0xC0, 0xE0, 0xF8, 0x78, 0x1C,
  0x07, 0x00, 0x00, 0x00, 0x1C, 0x07, 0x01, 0xC0, 0x03, 0xFC, 0x01, 0xFF, 0xE0, 0x78, 0x1E, 0x1C,
  0x00, 0xE7, 0x04, 0x0C, 0xC7, 0xF9, 0xF8, 0xFF, 0x1F, 0x38, 0xE3, 0xE6, 0x0C, 0x7C, 0xC1, 0x8F,
  0x98, 0x31, 0xF3, 0x86, 0x3E, 0x7B, 0xEE, 0xC7, 0xEF, 0x9C, 0x38, 0xE3, 0xC0, 0x00, 0x3E, 0x04,
  0x03, 0xFF, 0xC0, 0x1F, 0xF8, 0x00, 0x07, 0xC0, 0x0F, 0x80, 0x1F, 0x80, 0x77, 0x00, 0xEF, 0x01,
  0xCE, 0x07, 0x9C, 0x0E, 0x3C, 0x1C, 0x38, 0x7F, 0xF0, 0xFF, 0xF1, 0xC0, 0xE7, 0x81, 0xCE, 0x03,
  0xFC, 0x03, 0x80, 0xFF, 0xCF, 0xFE, 0xF1, 0xEF, 0x0E, 0xF0, 0xEF, 0x0E, 0xFF, 0xCF, 0xFC, 0xF0,
  0xEF, 0x07, 0xF0, 0x7F, 0x07, 0xFF, 0xFF, 0xFE, 0xFF, 0xC0, 0x1F, 0xE0, 0xFF, 0xC7, 0xCF, 0x9C,
  0x0E, 0xF0, 0x3F, 0x80, 0x0E, 0x00, 0x38, 0x00, 0xE0, 0x03, 0x80, 0x0F, 0x03, 0xDE, 0x0E, 0x7F,
  0xF8, 0xFF, 0xC0, 0xFC, 0x00, 0xFF, 0x87, 0xFF, 0x3C, 0x7D, 0xE0, 0xEF, 0x03, 0xF8, 0x1F, 0xC0,
  0xFE, 0x07, 0xF0, 0x3F, 0x81, 0xFC, 0x1F, 0xE0, 0xEF, 0xFF, 0x7F, 0xF3, 0xFE, 0x00, 0xFF, 0xFF,
  0xFF, 0xC0, 0x78, 0x0F, 0x01, 0xE0, 0x3F, 0xF7, 0xFE, 0xFF, 0xDE, 0x03, 0xC0, 0x78, 0x0F, 0xFF,
  0xFF, 0xFF, 0xF8, 0xFF, 0xFF, 0xFF, 0x03, 0xC0, 0xF0, 0x3C, 0x0F, 0x03, 0xFF, 0xFF, 0xFC, 0x0F,
  0x03, 0xC0, 0xF0, 0x3C, 0x0F, 0x00, 0x1F, 0xE0, 0xFF, 0xC7, 0xCF, 0x9C, 0x0E, 0xF0, 0x3F, 0x80,
  0x0E, 0x00, 0x38, 0x7F, 0xE1, 0xFF, 0x80, 0x7F, 0x03, 0xDE, 0x0E, 0x7F, 0xF8, 0xFF, 0xC0, 0xFC,
  0x00, 0xF0, 0x3F, 0x81, 0xFC, 0x0F, 0xE0, 0x7F, 0x03, 0xF8, 0x1F, 0xFF, 0xFF, 0xFF, 0xF0, 0x3F,
  0x81, 0xFC, 0x0F, 0xE0, 0x7F, 0x03, 0xF8, 0x1F, 0xC0, 0xE0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xF0, 0x01, 0xC0, 0x70, 0x1C, 0x07, 0x01, 0xC0, 0x70, 0x1C, 0x07, 0x01, 0xC0, 0x7E, 0x1F,
  0x87, 0xF3, 0xDF, 0xE3, 0xF0, 0xF0, 0x7B, 0xC3, 0xCF, 0x1E, 0x3C, 0xF0, 0xF3, 0x83, 0x9C, 0x0E,
  0xF0, 0x3F, 0xE0, 0xFB, 0x83, 0xEF, 0x0F, 0x1E, 0x3C, 0x38, 0xF0, 0xF3, 0xC1, 0xEF, 0x03, 0xC0,
  0xF0, 0x3C, 0x0F, 0x03, 0xC0, 0xF0, 0x3C, 0x0F, 0x03, 0xC0, 0xF0, 0x3C, 0x0F, 0x03, 0xC0, 0xFF,
  0xFF, 0xFF, 0xFC, 0xF8, 0x0F, 0xFE, 0x07, 0xFF, 0x07, 0xFD, 0xC3, 0xFE, 0xE1, 0xFF, 0x71, 0xDF,
  0xBC, 0xEF, 0xCE, 0x77, 0xE7, 0x73, 0xF1, 0xB9, 0xF8, 0xFC, 0xFC, 0x7C, 0x7E, 0x1E, 0x3F, 0x0F,
  0x1F, 0x87, 0x0E, 0xF0, 0x3F, 0xE0, 0xFF, 0xC3, 0xFF, 0x0F, 0xEE, 0x3F, 0xB8, 0xFF, 0x73, 0xFD,
  0xEF, 0xF3, 0xBF, 0xC7, 0x7F, 0x1D, 0xFC, 0x3F, 0xF0, 0x7F, 0xC1, 0xFF, 0x03, 0xC0, 0x1F, 0xE0,
  0xFF, 0xC7, 0xCF, 0x9C, 0x0E, 0xF0, 0x1F, 0x80, 0x7E, 0x01, 0xF8, 0x07, 0xE0, 0x1F, 0x80, 0x7F,
  0x03, 0xDE, 0x0E, 0x7F, 0xF8, 0xFF, 0xC0, 0xFC, 0x00, 0xFF, 0xCF, 0xFE, 0xF1, 0xEF, 0x0F, 0xF0,
  0x7F, 0x07, 0xF0, 0xFF, 0xFE, 0xFF, 0xCF, 0xF8, 0xF0, 0x0F, 0x00, 0xF0, 0x0F, 0x00, 0xF0, 0x00,
  0x1F, 0xE0, 0xFF, 0xC7, 0xCF, 0x9C, 0x0E, 0xF0, 0x1F, 0x80, 0x7E, 0x01, 0xF8, 0x07, 0xE0, 0x1F,
  0x80, 0x7F, 0x3B, 0xDE, 0x7E, 0x7F, 0xF8, 0xFF, 0xC0, 0xFF, 0x80, 0x0F, 0xFF, 0xCF, 0xFE, 0xF1,
  0xEF, 0x0F, 0xF0, 0x7F, 0x0F, 0xF0, 0xFF, 0xFE, 0xFF, 0xCF, 0xF8, 0xF3, 0xCF, 0x1C, 0xF0, 0xEF,
  0x0F, 0xF0, 0x70, 0x3F, 0xC7, 0xFE, 0x70, 0xEF, 0x07, 0xF0, 0x07, 0x80, 0x7F, 0x03, 0xFC, 0x07,
  0xE0, 0x0F, 0xE0, 0x7E, 0x0F, 0xF9, 0xE7, 0xFE, 0x3F, 0xC0, 0xFF, 0xFF, 0xFF, 0x0F, 0x00, 0xF0,
  0x0F, 0x00, 0xF0, 0x0F, 0x00, 0xF0, 0x0F, 0x00, 0xF0, 0x0F, 0x00, 0xF0, 0x0F, 0x00, 0xF0, 0x0F,
  0x00, 0xF0, 0x3F, 0x81, 0xFC, 0x0F, 0xE0, 0x7F, 0x03, 0xF8, 0x1F, 0xC0, 0xFE, 0x07, 0xF0, 0x3F,
  0x81, 0xFC, 0x0E, 0xE0, 0xF7, 0xFF, 0x1F, 0xF0, 0x7F, 0x00, 0xF0, 0x1E, 0xF0, 0x3D, 0xE0, 0x71,
  0xC0, 0xE3, 0xC3, 0xC3, 0x87, 0x07, 0x0E, 0x0F, 0x3C, 0x0E, 0x70, 0x1C, 0xE0, 0x3B, 0x80, 0x3F,
  0x00, 0x7E, 0x00, 0xF8, 0x00, 0xF0, 0x00, 0xF0, 0x78, 0x3B, 0x83, 0xC3, 0xDE, 0x1E, 0x1E, 0x71,
  0xF0, 0xE3, 0x8F, 0xC7, 0x1C, 0x7E, 0x38, 0xF3, 0x73, 0x83, 0xB9, 0x9C, 0x1D, 0xCE, 0xE0, 0xEE,
  0x77, 0x07, 0x63, 0xF0, 0x1B, 0x0F, 0x80, 0xF8, 0x7C, 0x07, 0xC3, 0xE0, 0x3C, 0x1E, 0x00, 0xF0,
  0x7D, 0xE1, 0xC3, 0x8F, 0x07, 0x38, 0x1F, 0xC0, 0x3F, 0x00, 0x78, 0x01, 0xE0, 0x0F, 0xC0, 0x7F,
  0x01, 0xDE, 0x0F, 0x3C, 0x78, 0x71, 0xC1, 0xEE, 0x03, 0xC0, 0xF8, 0x1E, 0x70, 0x78, 0xF0, 0xE0,
  0xE3, 0xC1, 0xE7, 0x01, 0xDC, 0x01, 0xF8, 0x03, 0xE0, 0x03, 0xC0, 0x07, 0x00, 0x0E, 0x00, 0x1C,
  0x00, 0x38, 0x00, 0x70, 0x00, 0xE0, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xE0, 0x1C, 0x03, 0xC0, 0x38,
  0x07, 0x00, 0xF0, 0x1E, 0x01, 0xC0, 0x38, 0x07, 0x80, 0x7F, 0xFF, 0xFF, 0xFF, 0xF0, 0xFF, 0xF9,
  0xCE, 0x73, 0x9C, 0xE7, 0x39, 0xCE, 0x73, 0x9C, 0xE7, 0xFE, 0xE0, 0x60, 0x70, 0x70, 0x70, 0x30,
  0x38, 0x38, 0x18, 0x1C, 0x1C, 0x1C, 0x0C, 0x0E, 0x0E, 0x0E, 0x06, 0x07, 0xFF, 0xCE, 0x73, 0x9C,
  0xE7, 0x39, 0xCE, 0x73, 0x9C, 0xE7, 0x3F, 0xFE, 0x1C, 0x1E, 0x0F, 0x8C, 0xC6, 0x67, 0x1B, 0x0E,
  0xFF, 0xFF, 0xF0, 0xE6, 0x70, 0x0C, 0x1F, 0xEF, 0xFB, 0x87, 0x01, 0xC7, 0xF7, 0xFF, 0x87, 0xE1,
  0xF8, 0xFF, 0xFD, 0xF7, 0xE0, 0x1C, 0x03, 0x80, 0x71, 0x8E, 0xFD, 0xFF, 0xBC, 0x7F, 0x87, 0xE0,
  0xFC, 0x1F, 0x83, 0xF8, 0x7F, 0x9F, 0xDF, 0xB9, 0xE0, 0x0E, 0x07, 0xF1, 0xFF, 0x78, 0xEE, 0x0B,
  0xC0, 0x38, 0x07, 0x00, 0xE1, 0xEE, 0x39, 0xFF, 0x1F, 0x80, 0x00, 0xE0, 0x1C, 0x03, 0x8C, 0x73,
  0xEE, 0xFF, 0xFC, 0x7F, 0x07, 0xE0, 0xFC, 0x1F, 0x83, 0xF0, 0xFF, 0x3E, 0xFF, 0xCF, 0xB8, 0x0E,
  0x07, 0xF1, 0xFF, 0x78, 0xEE, 0x0F, 0xFF, 0xFF, 0xFF, 0x00, 0xE0, 0x0E, 0x3D, 0xFF, 0x1F, 0xC0,
  0x0F, 0x1F, 0x3D, 0x38, 0x38, 0xFF, 0xFF, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38,
  0x0C, 0x07, 0xDD, 0xFF, 0xF8, 0xFE, 0x0F, 0xC1, 0xF8, 0x3F, 0x07, 0xF1, 0xEF, 0xFD, 0xFB, 0x8E,
  0x70, 0x0F, 0xC3, 0xDF, 0xF1, 0xFC, 0x04, 0x00, 0xE0, 0x1C, 0x03, 0x80, 0x71, 0x8E, 0xFD, 0xFF,
  0xBC, 0x7F, 0x07, 0xE0, 0xFC, 0x1F, 0x83, 0xF0, 0x7E, 0x0F, 0xC1, 0xF8, 0x38, 0xEF, 0x60, 0x0E,
  0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0x38, 0xF1, 0x80, 0x00, 0xE3, 0x8E, 0x38, 0xE3, 0x8E, 0x38, 0xE3,
  0x8E, 0x38, 0xEF, 0xBC, 0xE0, 0x1C, 0x03, 0x80, 0x70, 0x1E, 0x3D, 0xC7, 0x39, 0xC7, 0x70, 0xFE,
  0x1F, 0xC3, 0xFC, 0x73, 0xCE, 0x39, 0xC7, 0xB8, 0x78, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF8, 0x03,
  0x0C, 0x77, 0xDF, 0xBF, 0xEF, 0xFE, 0x3C, 0xFE, 0x1C, 0x3F, 0x0E, 0x1F, 0x87, 0x0F, 0xC3, 0x87,
  0xE1, 0xC3, 0xF0, 0xE1, 0xF8, 0x70, 0xFC, 0x38, 0x70, 0x03, 0x1D, 0xFB, 0xFF, 0x78, 0xFE, 0x0F,
  0xC1, 0xF8, 0x3F, 0x07, 0xE0, 0xFC, 0x1F, 0x83, 0xF0, 0x70, 0x0E, 0x07, 0xF1, 0xFF, 0x78, 0xFE,
  0x0F, 0xC1, 0xF8, 0x3F, 0x07, 0xE1, 0xEE, 0x39, 0xFF, 0x1F, 0x80, 0x03, 0x1D, 0xFB, 0xFF, 0x78,
  0xFF, 0x0F, 0xC1, 0xF8, 0x3F, 0x07, 0xF0, 0xFF, 0x3F, 0xBF, 0x73, 0xCE, 0x01, 0xC0, 0x38, 0x07,
  0x00, 0x18, 0x07, 0xDD, 0xFF, 0xF8, 0xFE, 0x0F, 0xC1, 0xF8, 0x3F, 0x07, 0xE1, 0xFE, 0x7D, 0xFF,
  0x9F, 0x70, 0x0E, 0x01, 0xC0, 0x38, 0x07, 0x07, 0xDF, 0xFF, 0x8F, 0x1C, 0x38, 0x70, 0xE1, 0xC3,
  0x87, 0x00, 0x0C, 0x1F, 0xEF, 0xFB, 0x87, 0xF0, 0x3F, 0x87, 0xF8, 0x1F, 0x21, 0xF8, 0x77, 0xF8,
  0xFC, 0x38, 0x70, 0xE7, 0xFF, 0xE7, 0x0E, 0x1C, 0x38, 0x70, 0xE1, 0xC3, 0xE3, 0xC0, 0xE0, 0xFC,
  0x1F, 0x83, 0xF0, 0x7E, 0x0F, 0xC1, 0xF8, 0x3F, 0x8F, 0x7B, 0xEF, 0xDC, 0xF3, 0x80, 0xF0, 0x77,
  0x0F, 0x78, 0xE3, 0x8E, 0x39, 0xC1, 0xDC, 0x1D, 0xC1, 0xD8, 0x0F, 0x80, 0xF8, 0x0F, 0x00, 0xF1,
  0xE3, 0xDC, 0x78, 0xE7, 0x1E, 0x39, 0xC7, 0x9C, 0x3B, 0xF7, 0x0E, 0xDD, 0xC3, 0xB3, 0x60, 0x6C,
  0xF8, 0x1F, 0x3E, 0x07, 0x87, 0x80, 0xE1, 0xC0, 0x78, 0xF3, 0x9C, 0x3D, 0xC1, 0xF8, 0x0F, 0x00,
  0xF0, 0x1F, 0x81, 0xFC, 0x39, 0xC7, 0x8E, 0xF0, 0xF0, 0xF0, 0x77, 0x0F, 0x78, 0xE3, 0x8E, 0x39,
  0xC1, 0xDC, 0x1D, 0xC1, 0xF8, 0x0F, 0x80, 0xF8, 0x0F, 0x00, 0x70, 0x0E, 0x03, 0xE0, 0x7C, 0x00,
  0xFF, 0xFF, 0xF0, 0x38, 0x1C, 0x0E, 0x07, 0x83, 0xC0, 0xE0, 0x70, 0x3F, 0xFF, 0xFC, 0x0F, 0x1F,
  0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x38, 0xF8, 0xE0, 0xF8, 0x3C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1E, 0x1F,
  0x07, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xE0, 0xF1, 0xF0, 0xE1, 0xC3, 0x87,
  0x0E, 0x1E, 0x1E, 0x1C, 0x79, 0xC3, 0x87, 0x0E, 0x1C, 0x79, 0xE3, 0x80, 0x78, 0xFF, 0x9F, 0x3F,
  0x63, 0xC0,
};
const GFXglyph CleanBigGlyphs[] PROGMEM = {
  {0, 1, 1, 5, 0, 0},   // 0x20  
  {1, 3, 15, 7, 2, -14},   // 0x21 !
  {7, 8, 6, 12, 2, -14},   // 0x22 "
  {13, 13, 15, 14, 0, -14},   // 0x23 #
  {38, 12, 19, 14, 1, -16},   // 0x24 $
  {67, 17, 16, 21, 2, -15},   // 0x25 %
  {101, 13, 15, 14, 1, -14},   // 0x26 &
  {126, 3, 6, 7, 2, -14},   // 0x27 '
  {129, 5, 19, 8, 2, -15},   // 0x28 (
  {141, 5, 19, 8, 1, -15},   // 0x29 )
  {153, 8, 8, 12, 2, -14},   // 0x2A *
  {161, 10, 10, 14, 2, -10},   // 0x2B +
  {174, 4, 6, 7, 1, -1},   // 0x2C ,
  {177, 7, 3, 10, 1, -7},   // 0x2D -
  {180, 3, 3, 7, 2, -2},   // 0x2E .
  {182, 8, 18, 8, 0, -15},   // 0x2F /
  {200, 12, 15, 14, 1, -14},   // 0x30 0
  {223, 7, 15, 9, 1, -14},   // 0x31 1
  {237, 11, 15, 13, 1, -14},   // 0x32 2
  {258, 12, 15, 14, 1, -14},   // 0x33 3
  {281, 12, 15, 14, 1, -14},   // 0x34 4
  {304, 11, 15, 13, 1, -14},   // 0x35 5
  {325, 12, 15, 14, 1, -14},   // 0x36 6
  {348, 10, 15, 12, 1, -14},   // 0x37 7
  {367, 12, 15, 14, 1, -14},   // 0x38 8
  {390, 12, 16, 14, 1, -15},   // 0x39 9
  {414, 3, 11, 7, 2, -10},   // 0x3A :
  {419, 4, 15, 7, 1, -10},   // 0x3B ;
  {427, 10, 12, 14, 2, -11},   // 0x3C <
  {442, 10, 8, 14, 2, -9},   // 0x3D =
  {452, 11, 12, 14, 2, -11},   // 0x3E >
  {469, 10, 15, 12, 1, -14},   // 0x3F ?
  {488, 19, 19, 21, 1, -14},   // 0x40 @
  {534, 15, 15, 16, 0, -14},   // 0x41 A
  {563, 12, 15, 14, 1, -14},   // 0x42 B
  {586, 14, 15, 16, 1, -14},   // 0x43 C
  {613, 13, 15, 15, 1, -14},   // 0x44 D
  {638, 11, 15, 13, 1, -14},   // 0x45 E
  {659, 10, 15, 12, 1, -14},   // 0x46 F
  {678, 14, 15, 16, 1, -14},   // 0x47 G
  {705, 13, 15, 16, 1, -14},   // 0x48 H
  {730, 4, 15, 6, 1, -14},   // 0x49 I
  {738, 10, 15, 12, 1, -14},   // 0x4A J
  {757, 14, 15, 15, 1, -14},   // 0x4B K
  {784, 10, 15, 12, 1, -14},   // 0x4C L
  {803, 17, 15, 20, 1, -14},   // 0x4D M
  {835, 14, 15, 16, 1, -14},   // 0x4E N
  {862, 14, 15, 16, 1, -14},   // 0x4F O
  {889, 12, 15, 14, 1, -14},   // 0x50 P
  {912, 14, 16, 16, 1, -14},   // 0x51 Q
  {940, 12, 15, 14, 1, -14},   // 0x52 R
  {963, 12, 15, 14, 1, -14},   // 0x53 S
  {986, 12, 15, 14, 1, -14},   // 0x54 T
  {1009, 13, 15, 15, 1, -14},   // 0x55 U
  {1034, 15, 15, 16, 0, -14},   // 0x56 V
  {1063, 21, 15, 22, 0, -14},   // 0x57 W
  {1103, 14, 15, 16, 1, -14},   // 0x58 X
  {1130, 15, 15, 15, 0, -14},   // 0x59 Y
  {1159, 12, 15, 14, 1, -14},   // 0x5A Z
  {1182, 5, 19, 8, 2, -15},   // 0x5B [
  {1194, 8, 18, 8, 0, -15},   // 0x5C backslash
  {1212, 5, 19, 8, 1, -15},   // 0x5D ]
  {1224, 9, 7, 10, 1, -14},   // 0x5E ^
  {1232, 10, 2, 10, 0, 1},   // 0x5F _
  {1235, 4, 3, 8, 2, -15},   // 0x60 `
  {1237, 10, 12, 12, 1, -11},   // 0x61 a
  {1252, 11, 15, 13, 1, -14},   // 0x62 b
  {1273, 11, 12, 12, 1, -11},   // 0x63 c
  {1290, 11, 15, 13, 1, -14},   // 0x64 d
  {1311, 11, 12, 13, 1, -11},   // 0x65 e
  {1328, 8, 16, 8, 0, -15},   // 0x66 f
  {1344, 11, 17, 13, 1, -11},   // 0x67 g
  {1368, 11, 15, 13, 1, -14},   // 0x68 h
  {1389, 4, 16, 6, 1, -15},   // 0x69 i
  {1397, 6, 20, 6, -1, -15},   // 0x6A j
  {1412, 11, 15, 12, 1, -14},   // 0x6B k
  {1433, 3, 15, 6, 1, -14},   // 0x6C l
  {1439, 17, 12, 19, 1, -11},   // 0x6D m
  {1465, 11, 12, 13, 1, -11},   // 0x6E n
  {1482, 11, 12, 13, 1, -11},   // 0x6F o
  {1499, 11, 16, 13, 1, -11},   // 0x70 p
  {1521, 11, 16, 13, 1, -11},   // 0x71 q
  {1543, 7, 12, 9, 1, -11},   // 0x72 r
  {1554, 10, 12, 12, 1, -11},   // 0x73 s
  {1569, 7, 14, 8, 0, -13},   // 0x74 t
  {1582, 11, 11, 13, 1, -10},   // 0x75 u
  {1598, 12, 11, 13, 0, -10},   // 0x76 v
  {1615, 18, 11, 18, 0, -10},   // 0x77 w
  {1640, 12, 11, 12, 0, -10},   // 0x78 x
  {1657, 12, 15, 13, 0, -10},   // 0x79 y
  {1680, 10, 11, 12, 1, -10},   // 0x7A z
  {1694, 8, 19, 10, 1, -15},   // 0x7B {
  {1713, 3, 25, 8, 2, -19},   // 0x7C |
  {1723, 7, 19, 10, 1, -15},   // 0x7D }
  {1740, 11, 4, 14, 2, -7},   // 0x7E ~
};
const GFXfont CleanBig PROGMEM = {(uint8_t*)CleanBigBitmaps, (GFXglyph*)CleanBigGlyphs, 0x20, 0x7E, 25};

// StudioSmall: generated from barlow-semi-condensed-600.ttf at 13px (cursor y = top of the text)
const uint8_t StudioSmallBitmaps[] PROGMEM = {
  0x00, 0xFF, 0xF0, 0xC0, 0xFD, 0xD0, 0x64, 0xDB, 0xFA, 0x64, 0x89, 0x3F, 0x64, 0xD8, 0x10, 0xE4,
  0xD1, 0x61, 0xC3, 0x83, 0xC5, 0x37, 0x84, 0x62, 0x5A, 0x25, 0x1D, 0x00, 0x80, 0xB8, 0x56, 0x4B,
  0x67, 0x00, 0x38, 0x6C, 0x6C, 0x38, 0x30, 0x7A, 0xCE, 0x4E, 0x7B, 0xFC, 0x6B, 0x6D, 0xB6, 0xD9,
  0x30, 0xD9, 0x36, 0xDB, 0x6B, 0x60, 0x27, 0xFE, 0x40, 0x30, 0xC3, 0x3F, 0x30, 0xC0, 0xE0, 0xF0,
  0x80, 0x18, 0x8C, 0x62, 0x31, 0x88, 0xC0, 0x76, 0xE7, 0x39, 0xCE, 0x7B, 0x70, 0x7C, 0x92, 0x49,
  0x20, 0x79, 0x3C, 0xC2, 0x18, 0xC2, 0x18, 0x7C, 0x7C, 0x21, 0x8C, 0x08, 0x3C, 0xD2, 0x78, 0x30,
  0xC2, 0x1B, 0x4F, 0xF0, 0xC3, 0x0C, 0xFC, 0x21, 0xE9, 0x8E, 0x7B, 0x70, 0xF6, 0xE1, 0xED, 0xCE,
  0x7B, 0x70, 0xFF, 0x20, 0x86, 0x10, 0x43, 0x08, 0x60, 0xF4, 0xE7, 0xED, 0x4E, 0x7A, 0x70, 0x79,
  0x2C, 0xD3, 0x7C, 0x34, 0xDB, 0x38, 0xC0, 0xC0, 0xF0, 0x2A, 0x04, 0x77, 0x30, 0x70, 0x30, 0xFC,
  0x0F, 0xC0, 0x83, 0x83, 0x83, 0x3B, 0x80, 0x79, 0x2C, 0xC2, 0x18, 0xC3, 0x00, 0x30, 0x3E, 0x31,
  0xB0, 0x53, 0x9B, 0x4D, 0x26, 0xB5, 0x7E, 0xC0, 0x3E, 0x00, 0x18, 0x70, 0xA1, 0x46, 0xCC, 0x9F,
  0x23, 0xC6, 0xF2, 0x68, 0xA2, 0xF2, 0x28, 0xE6, 0xF8, 0x76, 0xE3, 0x08, 0x42, 0x3B, 0x70, 0xF4,
  0xE3, 0x18, 0xC6, 0x33, 0xF0, 0xFC, 0x21, 0x0F, 0x42, 0x10, 0xF8, 0xFC, 0x21, 0x0F, 0x42, 0x10,
  0x80, 0x76, 0xE3, 0x0B, 0xC6, 0x3B, 0x70, 0x8C, 0x63, 0x1F, 0xC6, 0x31, 0x88, 0xFF, 0x80, 0x0C,
  0x30, 0xC3, 0x0F, 0x3C, 0xF3, 0x78, 0x8E, 0x6B, 0x28, 0xF3, 0xC9, 0xA6, 0x8C, 0x84, 0x21, 0x08,
  0x42, 0x10, 0xF8, 0xC7, 0x3F, 0xED, 0xB6, 0x18, 0x61, 0x84, 0xCF, 0x3E, 0xEB, 0xBE, 0x79, 0xE3,
  0x8C, 0x76, 0xE3, 0x18, 0xC6, 0x3B, 0x70, 0xFC, 0xE3, 0x1F, 0xC2, 0x10, 0x80, 0x76, 0xE3, 0x18,
  0xC6, 0x31, 0xFB, 0x08, 0xFA, 0x68, 0xE2, 0xFA, 0x49, 0xA6, 0x8C, 0x79, 0xB4, 0x58, 0x38, 0x3C,
  0x5B, 0x78, 0xFC, 0x41, 0x04, 0x10, 0x41, 0x04, 0x10, 0x8C, 0x63, 0x18, 0xC6, 0x3B, 0x70, 0xC6,
  0x89, 0x93, 0x62, 0x85, 0x0A, 0x1C, 0x10, 0xCC, 0xD3, 0x24, 0xC9, 0xB2, 0x7F, 0x8E, 0xE3, 0x30,
  0xCC, 0x33, 0x00, 0xC6, 0xD8, 0xA1, 0xC1, 0x07, 0x0A, 0x36, 0xC6, 0xC5, 0xB6, 0x8A, 0x38, 0x41,
  0x04, 0x10, 0xFC, 0x31, 0x86, 0x30, 0x86, 0x30, 0xFC, 0xFB, 0x6D, 0xB6, 0xDB, 0x70, 0x8C, 0xC4,
  0x66, 0x23, 0x30, 0xE4, 0x92, 0x49, 0x24, 0xF0, 0x21, 0x8C, 0xA4, 0xE4, 0xFC, 0xC0, 0x79, 0xA0,
  0xDF, 0xCF, 0x37, 0xC0, 0x84, 0x3D, 0xB9, 0xCE, 0x7B, 0xF0, 0x76, 0xE5, 0x08, 0x6D, 0xC0, 0x18,
  0xFF, 0xB9, 0xCE, 0x7B, 0xF8, 0x76, 0xE7, 0xF8, 0x6D, 0xC0, 0x36, 0xF6, 0x66, 0x66, 0x60, 0xFE,
  0xE7, 0x39, 0xEF, 0xE3, 0x33, 0x80, 0x84, 0x3D, 0xB9, 0xCE, 0x73, 0x98, 0xBF, 0x80, 0x20, 0x92,
  0x49, 0x25, 0xF0, 0x84, 0x27, 0x6E, 0x7A, 0xD2, 0x98, 0xFF, 0x80, 0xF6, 0xDB, 0x99, 0x99, 0x99,
  0x99, 0x99, 0xF6, 0xE7, 0x39, 0xCE, 0x60, 0x76, 0xE7, 0x39, 0xED, 0xC0, 0xF6, 0xE7, 0x39, 0xEF,
  0xD0, 0x84, 0x00, 0xFE, 0xE7, 0x39, 0xEF, 0xE3, 0x18, 0xC0, 0xFA, 0x49, 0x20, 0x7A, 0x50, 0xE0,
  0xE5, 0xE0, 0x46, 0xF6, 0x66, 0x66, 0x70, 0x9C, 0xE7, 0x39, 0xCF, 0xE0, 0xCD, 0x25, 0x9E, 0x70,
  0xC3, 0x00, 0xC9, 0xAC, 0x97, 0xCE, 0xE7, 0x61, 0xB0, 0xD8, 0xCD, 0xE3, 0x0C, 0x31, 0xEC, 0xC0,
  0xCD, 0x25, 0x9E, 0x70, 0xC3, 0x0C, 0x21, 0x80, 0xF8, 0xCC, 0x66, 0x63, 0xE0, 0x36, 0x66, 0x6E,
  0xC6, 0x66, 0x67, 0xFF, 0xFF, 0xFC, 0xE6, 0x22, 0x23, 0x32, 0x22, 0x6E, 0x67, 0x70,
};
const GFXglyph StudioSmallGlyphs[] PROGMEM = {
  {0, 1, 1, 3, 0, 10},   // 0x20  
  {1, 2, 9, 4, 1, 2},   // 0x21 !
  {4, 4, 3, 4, 0, 2},   // 0x22 "
  {6, 7, 9, 8, 1, 2},   // 0x23 #
  {14, 6, 12, 7, 0, 0},   // 0x24 $
  {23, 9, 9, 10, 1, 2},   // 0x25 %
  {34, 8, 9, 8, 0, 2},   // 0x26 &
  {43, 2, 3, 2, 0, 2},   // 0x27 '
  {44, 3, 12, 4, 0, 0},   // 0x28 (
  {49, 3, 12, 4, 1, 0},   // 0x29 )
  {54, 5, 4, 5, 0, 2},   // 0x2A *
  {57, 6, 6, 6, 0, 4},   // 0x2B +
  {62, 1, 3, 3, 1, 9},   // 0x2C ,
  {63, 4, 1, 5, 0, 6},   // 0x2D -
  {64, 1, 1, 3, 1, 10},   // 0x2E .
  {65, 5, 9, 5, 0, 2},   // 0x2F /
  {71, 5, 9, 7, 1, 2},   // 0x30 0
  {77, 3, 9, 4, 0, 2},   // 0x31 1
  {81, 6, 9, 6, 0, 2},   // 0x32 2
  {88, 6, 9, 6, 0, 2},   // 0x33 3
  {95, 6, 9, 7, 0, 2},   // 0x34 4
  {102, 5, 9, 6, 1, 2},   // 0x35 5
  {108, 5, 9, 6, 1, 2},   // 0x36 6
  {114, 6, 9, 6, 0, 2},   // 0x37 7
  {121, 5, 9, 6, 1, 2},   // 0x38 8
  {127, 6, 9, 6, 0, 2},   // 0x39 9
  {134, 2, 5, 4, 1, 5},   // 0x3A :
  {136, 2, 8, 3, 1, 4},   // 0x3B ;
  {138, 6, 6, 6, 0, 4},   // 0x3C <
  {143, 6, 3, 6, 0, 5},   // 0x3D =
  {146, 6, 6, 6, 0, 4},   // 0x3E >
  {151, 6, 9, 6, 0, 2},   // 0x3F ?
  {158, 9, 10, 10, 1, 2},   // 0x40 @
  {170, 7, 9, 7, 0, 2},   // 0x41 A
  {178, 6, 9, 7, 1, 2},   // 0x42 B
  {185, 5, 9, 7, 1, 2},   // 0x43 C
  {191, 5, 9, 7, 1, 2},   // 0x44 D
  {197, 5, 9, 7, 1, 2},   // 0x45 E
  {203, 5, 9, 6, 1, 2},   // 0x46 F
  {209, 5, 9, 7, 1, 2},   // 0x47 G
  {215, 5, 9, 7, 1, 2},   // 0x48 H
  {221, 1, 9, 3, 1, 2},   // 0x49 I
  {223, 6, 9, 7, 0, 2},   // 0x4A J
  {230, 6, 9, 7, 1, 2},   // 0x4B K
  {237, 5, 9, 6, 1, 2},   // 0x4C L
  {243, 6, 9, 8, 1, 2},   // 0x4D M
  {250, 6, 9, 8, 1, 2},   // 0x4E N
  {257, 5, 9, 7, 1, 2},   // 0x4F O
  {263, 5, 9, 7, 1, 2},   // 0x50 P
  {269, 5, 11, 7, 1, 2},   // 0x51 Q
  {276, 6, 9, 7, 1, 2},   // 0x52 R
  {283, 6, 9, 7, 0, 2},   // 0x53 S
  {290, 6, 9, 7, 0, 2},   // 0x54 T
  {297, 5, 9, 7, 1, 2},   // 0x55 U
  {303, 7, 9, 7, 0, 2},   // 0x56 V
  {311, 10, 9, 10, 0, 2},   // 0x57 W
  {323, 7, 9, 7, 0, 2},   // 0x58 X
  {331, 6, 9, 7, 0, 2},   // 0x59 Y
  {338, 6, 9, 6, 0, 2},   // 0x5A Z
  {345, 3, 12, 5, 0, 1},   // 0x5B [
  {350, 4, 9, 5, 1, 2},   // 0x5C backslash
  {355, 3, 12, 5, 1, 1},   // 0x5D ]
  {360, 5, 6, 6, 0, 1},   // 0x5E ^
  {364, 6, 1, 6, 0, 10},   // 0x5F _
  {365, 2, 1, 3, 0, 2},   // 0x60 `
  {366, 6, 7, 6, 0, 4},   // 0x61 a
  {372, 5, 9, 6, 1, 2},   // 0x62 b
  {378, 5, 7, 6, 1, 4},   // 0x63 c
  {383, 5, 9, 6, 1, 2},   // 0x64 d
  {389, 5, 7, 6, 1, 4},   // 0x65 e
  {394, 4, 9, 4, 0, 2},   // 0x66 f
  {399, 5, 10, 6, 1, 4},   // 0x67 g
  {406, 5, 9, 6, 1, 2},   // 0x68 h
  {412, 1, 9, 3, 1, 2},   // 0x69 i
  {414, 3, 12, 3, -1, 2},   // 0x6A j
  {419, 5, 9, 6, 1, 2},   // 0x6B k
  {425, 1, 9, 3, 1, 2},   // 0x6C l
  {427, 8, 7, 10, 1, 4},   // 0x6D m
  {434, 5, 7, 6, 1, 4},   // 0x6E n
  {439, 5, 7, 6, 1, 4},   // 0x6F o
  {444, 5, 10, 7, 1, 4},   // 0x70 p
  {451, 5, 10, 7, 1, 4},   // 0x71 q
  {458, 3, 7, 4, 1, 4},   // 0x72 r
  {461, 5, 7, 6, 0, 4},   // 0x73 s
  {466, 4, 9, 4, 0, 2},   // 0x74 t
  {471, 5, 7, 6, 1, 4},   // 0x75 u
  {476, 6, 7, 6, 0, 4},   // 0x76 v
  {482, 9, 7, 9, 0, 4},   // 0x77 w
  {490, 6, 7, 6, 0, 4},   // 0x78 x
  {496, 6, 10, 6, 0, 4},   // 0x79 y
  {504, 5, 7, 5, 0, 4},   // 0x7A z
  {509, 4, 12, 4, 0, 1},   // 0x7B {
  {515, 2, 11, 2, 0, 1},   // 0x7C |
  {518, 4, 12, 4, 0, 1},   // 0x7D }
  {524, 6, 2, 7, 0, 6},   // 0x7E ~
};
const GFXfont StudioSmall PROGMEM = {(uint8_t*)StudioSmallBitmaps, (GFXglyph*)StudioSmallGlyphs, 0x20, 0x7E, 16};

// StudioLabel: generated from barlow-semi-condensed-600.ttf at 16px
const uint8_t StudioLabelBitmaps[] PROGMEM = {
  0x00, 0xC9, 0x24, 0x90, 0x1F, 0x00, 0xDE, 0xF6, 0x33, 0x32, 0xFF, 0xFF, 0x66, 0x66, 0xFF, 0xFF,
  0x64, 0x4C, 0x30, 0xCF, 0xF3, 0x87, 0x07, 0x0E, 0x0E, 0x1C, 0xFF, 0x78, 0xC0, 0x71, 0x9F, 0x22,
  0x2C, 0x7D, 0x07, 0x60, 0x08, 0x03, 0x70, 0x5F, 0x12, 0x26, 0x7C, 0x87, 0x00, 0x78, 0x7C, 0xCC,
  0x4C, 0x78, 0x70, 0xF9, 0xCF, 0x8E, 0xFE, 0x7B, 0xFC, 0x36, 0x6C, 0xCC, 0xCC, 0xCC, 0x66, 0x30,
  0xC6, 0x66, 0x33, 0x33, 0x36, 0x66, 0xC0, 0x33, 0xF3, 0x3F, 0x30, 0x10, 0x23, 0xFB, 0xF1, 0x02,
  0x00, 0xFA, 0x7F, 0xC0, 0xF0, 0x0C, 0x21, 0x86, 0x10, 0xC3, 0x08, 0x61, 0x8C, 0x00, 0x7B, 0xFC,
  0xF1, 0xC7, 0x1C, 0x71, 0xCF, 0xF7, 0x80, 0x7F, 0x33, 0x33, 0x33, 0x33, 0x30, 0x7B, 0xFC, 0xE3,
  0x0C, 0x63, 0x1C, 0xE3, 0xFF, 0xC0, 0xFF, 0xF1, 0x8C, 0x78, 0x30, 0xE3, 0xCF, 0xE7, 0x80, 0x18,
  0x18, 0x30, 0x30, 0x66, 0x66, 0x7F, 0xFF, 0x06, 0x06, 0x06, 0xFF, 0xFC, 0x30, 0xFB, 0xF0, 0xC3,
  0xCF, 0xF7, 0x80, 0x7B, 0xFC, 0xF0, 0xFB, 0xFC, 0xF3, 0xCF, 0xF7, 0x80, 0xFF, 0xFC, 0x10, 0x60,
  0xC1, 0x06, 0x0C, 0x10, 0x60, 0xC0, 0x7B, 0xFC, 0xF3, 0xFD, 0xEC, 0xF3, 0xCF, 0xF7, 0x80, 0x7B,
  0xEC, 0xE3, 0xCF, 0xF7, 0xC3, 0xCF, 0xF7, 0x80, 0xF8, 0x01, 0xF0, 0xF0, 0x3F, 0x80, 0x06, 0x3D,
  0xE6, 0x07, 0x83, 0xC1, 0x80, 0xFF, 0xFC, 0x07, 0xFF, 0xE0, 0xC0, 0xE0, 0x70, 0x31, 0xEE, 0x30,
  0x00, 0x3C, 0xFB, 0x1E, 0x30, 0xE3, 0x86, 0x18, 0x00, 0x20, 0xE0, 0x06, 0x03, 0xF1, 0xC3, 0x23,
  0xBD, 0xE3, 0x24, 0x64, 0x9C, 0x93, 0x9F, 0xD9, 0x21, 0xFC, 0x1F, 0x00, 0x1C, 0x1C, 0x1C, 0x34,
  0x36, 0x36, 0x26, 0x63, 0x7F, 0x63, 0xC3, 0xFD, 0xFF, 0x1E, 0x3F, 0xDF, 0xB1, 0xE3, 0xC7, 0xFF,
  0xF0, 0x79, 0xFB, 0x1E, 0x0C, 0x18, 0x30, 0x63, 0xC7, 0xF9, 0xE0, 0xFD, 0xFF, 0x1E, 0x3C, 0x78,
  0xF1, 0xE3, 0xC7, 0xFF, 0xF0, 0xFF, 0xFF, 0x06, 0x0C, 0x1F, 0x3E, 0x60, 0xC1, 0xFF, 0xF8, 0xFF,
  0xFF, 0x06, 0x0C, 0x1F, 0x3E, 0x60, 0xC1, 0x83, 0x00, 0x79, 0xFB, 0x1E, 0x3C, 0x1B, 0xF7, 0xE3,
  0xC7, 0xF9, 0xE0, 0xC7, 0x8F, 0x1E, 0x3C, 0x7F, 0xFF, 0xE3, 0xC7, 0x8F, 0x18, 0xFF, 0xFF, 0xFC,
  0x06, 0x0C, 0x18, 0x30, 0x60, 0xC1, 0xE3, 0xC6, 0xFC, 0xF0, 0xC7, 0x9B, 0x36, 0xCF, 0x1F, 0x3E,
  0x66, 0xCD, 0x8F, 0x18, 0xC1, 0x83, 0x06, 0x0C, 0x18, 0x30, 0x60, 0xC1, 0xFB, 0xF8, 0xC3, 0xE7,
  0xE7, 0xFF, 0xDB, 0xDB, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC7, 0x8F, 0x9F, 0x3B, 0x76, 0xE7,
  0xCF, 0x8F, 0x18, 0x79, 0xFB, 0x1E, 0x3C, 0x78, 0xF1, 0xE3, 0xC7, 0xF9, 0xE0, 0xFD, 0xFF, 0x1E,
  0x3C, 0x7F, 0xBC, 0x60, 0xC1, 0x83, 0x00, 0x79, 0xFB, 0x16, 0x3C, 0x78, 0xF1, 0xE3, 0xC7, 0x99,
  0xF1, 0x83, 0x00, 0xFD, 0xFF, 0x1E, 0x3C, 0x7F, 0xBE, 0x66, 0xCD, 0x8B, 0x18, 0x79, 0xFB, 0x16,
  0x3E, 0x0F, 0x07, 0x43, 0xC7, 0xF9, 0xE0, 0xFF, 0x7F, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
  0x18, 0x18, 0xC7, 0x8F, 0x1E, 0x3C, 0x78, 0xF1, 0xE3, 0xC7, 0xF9, 0xE0, 0xE3, 0x63, 0x63, 0x66,
  0x26, 0x36, 0x34, 0x34, 0x1C, 0x1C, 0x18, 0xC6, 0x36, 0x63, 0x67, 0x26, 0x76, 0x6F, 0x62, 0xD6,
  0x3D, 0x63, 0x9C, 0x39, 0xC1, 0x9C, 0x18, 0xC0, 0xE3, 0x62, 0x36, 0x34, 0x1C, 0x18, 0x1C, 0x34,
  0x36, 0x62, 0x63, 0xE3, 0x66, 0x26, 0x34, 0x3C, 0x1C, 0x18, 0x18, 0x18, 0x18, 0x18, 0xFE, 0xFC,
  0x18, 0x61, 0x83, 0x0C, 0x18, 0x60, 0xFF, 0xF8, 0x7F, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCF, 0xF0,
  0xC6, 0x10, 0xC6, 0x10, 0xC6, 0x10, 0xC6, 0xFC, 0x92, 0x49, 0x24, 0x93, 0xF8, 0x23, 0x95, 0xBD,
  0xC4, 0xFF, 0xFC, 0xCC, 0x3C, 0xFC, 0x19, 0xF7, 0xF8, 0xDF, 0xBF, 0xC3, 0x0C, 0x3E, 0xFF, 0x3C,
  0xF3, 0xCF, 0xFF, 0x80, 0x7B, 0xFC, 0xF0, 0xC3, 0x3F, 0xDE, 0x0C, 0x30, 0xDF, 0xFF, 0x3C, 0xF3,
  0xCF, 0xF7, 0xC0, 0x7B, 0xFC, 0xFF, 0xFF, 0x2F, 0xDE, 0x39, 0xD9, 0xFF, 0xB1, 0x8C, 0x63, 0x18,
  0x7F, 0xFC, 0xF3, 0xCF, 0x3F, 0xDF, 0x0D, 0xF7, 0x80, 0xC3, 0x0C, 0x3E, 0xFF, 0x3C, 0xF3, 0xCF,
  0x3C, 0xC0, 0xF3, 0xFF, 0xFC, 0x33, 0x03, 0x33, 0x33, 0x33, 0x33, 0xFE, 0xC3, 0x0C, 0x33, 0xDB,
  0xCF, 0x36, 0xDB, 0x3C, 0xC0, 0xFF, 0xFF, 0xFC, 0xFB, 0xBF, 0xFC, 0xCF, 0x33, 0xCC, 0xF3, 0x3C,
  0xCF, 0x33, 0xFB, 0xFC, 0xF3, 0xCF, 0x3C, 0xF3, 0x7B, 0xFC, 0xF3, 0xCF, 0x3F, 0xDE, 0xFB, 0xFC,
  0xF3, 0xCF, 0x3F, 0xFE, 0xC3, 0x0C, 0x00, 0x7F, 0xFC, 0xF3, 0xCF, 0x3F, 0xDF, 0x0C, 0x30, 0xC0,
  0xFF, 0xCC, 0xCC, 0xCC, 0x7B, 0xE8, 0xF8, 0x3A, 0x3F, 0xDE, 0x63, 0x3F, 0xF6, 0x31, 0x8C, 0x71,
  0xC0, 0xCF, 0x3C, 0xF3, 0xCF, 0x3D, 0xDF, 0xC6, 0xCD, 0xB3, 0x62, 0xC7, 0x0E, 0x0C, 0xCC, 0x49,
  0xD9, 0xBB, 0x35, 0x66, 0xA8, 0x77, 0x0E, 0xE1, 0x9C, 0xE6, 0xD8, 0xF1, 0xC3, 0x87, 0x9B, 0x73,
  0xC6, 0xC9, 0xB3, 0x62, 0xC7, 0x0E, 0x0C, 0x30, 0xE1, 0x80, 0xFD, 0xF1, 0x86, 0x31, 0x8F, 0xFF,
  0x19, 0xCC, 0x42, 0x11, 0x9C, 0x61, 0x08, 0x42, 0x1C, 0xE0, 0xFF, 0xF8, 0x47, 0x8C, 0x63, 0x18,
  0xC3, 0x31, 0x8C, 0x63, 0x3B, 0x80, 0x20, 0x7E, 0x4E,
};
const GFXglyph StudioLabelGlyphs[] PROGMEM = {
  {0, 1, 1, 3, 0, 0},   // 0x20  
  {1, 3, 11, 5, 1, -10},   // 0x21 !
  {6, 5, 3, 5, 0, -10},   // 0x22 "
  {8, 8, 10, 10, 1, -9},   // 0x23 #
  {18, 6, 14, 8, 1, -12},   // 0x24 $
  {29, 11, 11, 13, 1, -10},   // 0x25 %
  {45, 8, 11, 10, 1, -10},   // 0x26 &
  {56, 2, 3, 2, 0, -10},   // 0x27 '
  {57, 4, 13, 5, 0, -11},   // 0x28 (
  {64, 4, 13, 5, 1, -11},   // 0x29 )
  {71, 6, 5, 6, 0, -10},   // 0x2A *
  {75, 7, 6, 7, 0, -7},   // 0x2B +
  {81, 2, 4, 4, 1, -2},   // 0x2C ,
  {82, 5, 2, 6, 0, -5},   // 0x2D -
  {84, 2, 2, 4, 1, -1},   // 0x2E .
  {85, 6, 11, 7, 0, -10},   // 0x2F /
  {94, 6, 11, 8, 1, -10},   // 0x30 0
  {103, 4, 11, 5, 0, -10},   // 0x31 1
  {109, 6, 11, 8, 1, -10},   // 0x32 2
  {118, 6, 11, 8, 1, -10},   // 0x33 3
  {127, 8, 11, 8, 0, -10},   // 0x34 4
  {138, 6, 11, 8, 1, -10},   // 0x35 5
  {147, 6, 11, 8, 1, -10},   // 0x36 6
  {156, 7, 11, 7, 0, -10},   // 0x37 7
  {166, 6, 11, 8, 1, -10},   // 0x38 8
  {175, 6, 11, 8, 1, -10},   // 0x39 9
  {184, 3, 7, 5, 1, -6},   // 0x3A :
  {187, 2, 9, 4, 1, -7},   // 0x3B ;
  {190, 7, 7, 7, 0, -7},   // 0x3C <
  {197, 7, 5, 7, 0, -7},   // 0x3D =
  {202, 7, 7, 7, 0, -7},   // 0x3E >
  {209, 7, 11, 7, 0, -10},   // 0x3F ?
  {219, 11, 12, 13, 1, -10},   // 0x40 @
  {236, 8, 11, 9, 0, -10},   // 0x41 A
  {247, 7, 11, 9, 1, -10},   // 0x42 B
  {257, 7, 11, 9, 1, -10},   // 0x43 C
  {267, 7, 11, 9, 1, -10},   // 0x44 D
  {277, 7, 11, 8, 1, -10},   // 0x45 E
  {287, 7, 11, 8, 1, -10},   // 0x46 F
  {297, 7, 11, 9, 1, -10},   // 0x47 G
  {307, 7, 11, 9, 1, -10},   // 0x48 H
  {317, 2, 11, 4, 1, -10},   // 0x49 I
  {320, 7, 11, 8, 0, -10},   // 0x4A J
  {330, 7, 11, 9, 1, -10},   // 0x4B K
  {340, 7, 11, 8, 1, -10},   // 0x4C L
  {350, 8, 11, 10, 1, -10},   // 0x4D M
  {361, 7, 11, 9, 1, -10},   // 0x4E N
  {371, 7, 11, 9, 1, -10},   // 0x4F O
  {381, 7, 11, 8, 1, -10},   // 0x50 P
  {391, 7, 13, 8, 1, -10},   // 0x51 Q
  {403, 7, 11, 9, 1, -10},   // 0x52 R
  {413, 7, 11, 8, 1, -10},   // 0x53 S
  {423, 8, 11, 8, 0, -10},   // 0x54 T
  {434, 7, 11, 9, 1, -10},   // 0x55 U
  {444, 8, 11, 9, 0, -10},   // 0x56 V
  {455, 12, 11, 12, 0, -10},   // 0x57 W
  {472, 8, 11, 9, 0, -10},   // 0x58 X
  {483, 8, 11, 8, 0, -10},   // 0x59 Y
  {494, 7, 11, 8, 0, -10},   // 0x5A Z
  {504, 4, 15, 6, 0, -12},   // 0x5B [
  {512, 5, 11, 7, 1, -10},   // 0x5C backslash
  {519, 3, 15, 6, 2, -12},   // 0x5D ]
  {525, 5, 6, 7, 1, -10},   // 0x5E ^
  {529, 7, 2, 7, 0, -1},   // 0x5F _
  {531, 3, 2, 3, 0, -10},   // 0x60 `
  {532, 7, 8, 8, 0, -7},   // 0x61 a
  {539, 6, 11, 8, 1, -10},   // 0x62 b
  {548, 6, 8, 8, 1, -7},   // 0x63 c
  {554, 6, 11, 8, 1, -10},   // 0x64 d
  {563, 6, 8, 8, 1, -7},   // 0x65 e
  {569, 5, 11, 5, 0, -10},   // 0x66 f
  {576, 6, 11, 8, 1, -7},   // 0x67 g
  {585, 6, 11, 8, 1, -10},   // 0x68 h
  {594, 2, 11, 4, 1, -10},   // 0x69 i
  {597, 4, 14, 4, -1, -10},   // 0x6A j
  {604, 6, 11, 8, 1, -10},   // 0x6B k
  {613, 2, 11, 4, 1, -10},   // 0x6C l
  {616, 10, 8, 12, 1, -7},   // 0x6D m
  {626, 6, 8, 8, 1, -7},   // 0x6E n
  {632, 6, 8, 8, 1, -7},   // 0x6F o
  {638, 6, 11, 8, 1, -7},   // 0x70 p
  {647, 6, 11, 8, 1, -7},   // 0x71 q
  {656, 4, 8, 5, 1, -7},   // 0x72 r
  {660, 6, 8, 7, 1, -7},   // 0x73 s
  {666, 5, 10, 5, 0, -9},   // 0x74 t
  {673, 6, 8, 8, 1, -7},   // 0x75 u
  {679, 7, 8, 7, 0, -7},   // 0x76 v
  {686, 11, 8, 11, 0, -7},   // 0x77 w
  {697, 7, 8, 7, 0, -7},   // 0x78 x
  {704, 7, 11, 7, 0, -7},   // 0x79 y
  {714, 6, 8, 7, 0, -7},   // 0x7A z
  {720, 5, 15, 5, 0, -12},   // 0x7B {
  {730, 1, 13, 3, 1, -11},   // 0x7C |
  {732, 5, 15, 5, 0, -12},   // 0x7D }
  {742, 8, 3, 8, 0, -5},   // 0x7E ~
};
const GFXfont StudioLabel PROGMEM = {(uint8_t*)StudioLabelBitmaps, (GFXglyph*)StudioLabelGlyphs, 0x20, 0x7E, 19};

// StudioReadout: generated from dseg7.ttf at 30px
const uint8_t StudioReadoutBitmaps[] PROGMEM = {
  0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C,
  0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1F, 0xFF,
  0xFF, 0xFF, 0xFF, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0,
  0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1F, 0xFF, 0xFF, 0xFF,
  0xFF, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01,
  0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1F, 0xFF, 0xFF, 0xFF, 0xFF, 0xC0,
  0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C,
  0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1F, 0xFF, 0xFF, 0xFF, 0xFF, 0xC0, 0x1C, 0x01,
  0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0,
  0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1F, 0xFF, 0xFF, 0xFF, 0xFF, 0xC0, 0x1C, 0x01, 0xC0, 0x1C,
  0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01,
  0xC0, 0x1C, 0x01, 0xC0, 0x1F, 0xFF, 0xFF, 0xFF, 0xFF, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0,
  0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C,
  0x01, 0xC0, 0x1F, 0xFF, 0xFF, 0xFF, 0xFF, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01,
  0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0,
  0x1F, 0xFF, 0xFF, 0xFF, 0xFF, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C,
  0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1F, 0xFF,
  0xFF, 0xFF, 0xFF, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0,
  0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1F, 0xFF, 0xFF, 0xFF,
  0xFF, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01,
  0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1F, 0xFF, 0x3F, 0xF9, 0xFF, 0xF7,
  0xFF, 0xCF, 0xFE, 0x6F, 0xF6, 0xFF, 0xFF, 0xFF, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C,
  0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01,
  0xC0, 0x1F, 0xFF, 0x3F, 0xFF, 0x87, 0xFF, 0xF3, 0x7F, 0xFC, 0x77, 0xFF, 0x3F, 0x00, 0x0F, 0xE0,
  0x03, 0xFC, 0x00, 0x7F, 0x80, 0x0F, 0xF0, 0x01, 0xFE, 0x00, 0x3F, 0xC0, 0x07, 0xF8, 0x00, 0xFF,
  0x00, 0x1F, 0xC0, 0x03, 0xF0, 0x00, 0x7E, 0x00, 0x04, 0xE0, 0x00, 0x9E, 0x00, 0x3F, 0xC0, 0x07,
  0xF8, 0x00, 0xFF, 0x00, 0x1F, 0xE0, 0x03, 0xFC, 0x00, 0x7F, 0x80, 0x0F, 0xF0, 0x01, 0xFE, 0x00,
  0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0xFF, 0xF7, 0x7F, 0xFF, 0x80, 0x27, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0x70, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xF7, 0x20, 0x3F, 0xFF, 0x87, 0xFF, 0xF8, 0x7F, 0xFF,
  0x87, 0xFF, 0xF0, 0x00, 0x1E, 0x00, 0x03, 0xC0, 0x00, 0x78, 0x00, 0x0F, 0x00, 0x01, 0xE0, 0x00,
  0x3C, 0x00, 0x07, 0x80, 0x00, 0xF0, 0xFF, 0xEE, 0x3F, 0xFE, 0xF7, 0xFF, 0xC7, 0x7F, 0xF0, 0xF0,
  0x00, 0x1E, 0x00, 0x03, 0xC0, 0x00, 0x78, 0x00, 0x0F, 0x00, 0x01, 0xE0, 0x00, 0x3C, 0x00, 0x07,
  0x80, 0x00, 0xF0, 0x00, 0x1E, 0x00, 0x03, 0xFF, 0xF8, 0x7F, 0xFF, 0x8F, 0xFF, 0xF9, 0x7F, 0xFF,
  0x00, 0x7F, 0xFF, 0x1F, 0xFF, 0xE3, 0xFF, 0xFC, 0x7F, 0xFF, 0x00, 0x03, 0xC0, 0x00, 0xF0, 0x00,
  0x3C, 0x00, 0x0F, 0x00, 0x03, 0xC0, 0x00, 0xF0, 0x00, 0x3C, 0x00, 0x0F, 0x1F, 0xFD, 0xCF, 0xFF,
  0xB3, 0xFF, 0xFC, 0x7F, 0xFF, 0x00, 0x03, 0xC0, 0x00, 0xF0, 0x00, 0x3C, 0x00, 0x0F, 0x00, 0x03,
  0xC0, 0x00, 0xF0, 0x00, 0x3C, 0x00, 0x0F, 0x00, 0x03, 0xC0, 0x00, 0xF1, 0xFF, 0xDC, 0xFF, 0xFF,
  0x7F, 0xFF, 0xDF, 0xFF, 0xE0, 0x00, 0x00, 0x58, 0x00, 0x1B, 0x80, 0x07, 0xF8, 0x00, 0xFF, 0x00,
  0x1F, 0xE0, 0x03, 0xFC, 0x00, 0x7F, 0x80, 0x0F, 0xF0, 0x01, 0xFE, 0x00, 0x3F, 0xC0, 0x07, 0xF8,
  0x00, 0xFE, 0xFF, 0xEF, 0xBF, 0xFE, 0xC7, 0xFF, 0xD8, 0x7F, 0xF7, 0x00, 0x01, 0xE0, 0x00, 0x3C,
  0x00, 0x07, 0x80, 0x00, 0xF0, 0x00, 0x1E, 0x00, 0x03, 0xC0, 0x00, 0x78, 0x00, 0x0F, 0x00, 0x01,
  0xE0, 0x00, 0x3C, 0x00, 0x03, 0x80, 0x00, 0x70, 0x00, 0x06, 0x00, 0x00, 0x80, 0x3F, 0xFF, 0x9F,
  0xFF, 0xF3, 0xFF, 0xFC, 0x7F, 0xFF, 0x0F, 0x00, 0x01, 0xE0, 0x00, 0x3C, 0x00, 0x07, 0x80, 0x00,
  0xF0, 0x00, 0x1E, 0x00, 0x03, 0xC0, 0x00, 0x78, 0x00, 0x0E, 0xFF, 0xE1, 0xBF, 0xFE, 0x07, 0xFF,
  0xD8, 0x7F, 0xF3, 0x00, 0x00, 0xE0, 0x00, 0x3C, 0x00, 0x07, 0x80, 0x00, 0xF0, 0x00, 0x1E, 0x00,
  0x03, 0xC0, 0x00, 0x78, 0x00, 0x0F, 0x00, 0x01, 0xE0, 0x00, 0x3C, 0x3F, 0xFF, 0x8F, 0xFF, 0x73,
  0xFF, 0xF6, 0x7F, 0xFF, 0x80, 0x3F, 0xFF, 0x9B, 0xFF, 0xF3, 0xFF, 0xFC, 0x7F, 0xFF, 0x0F, 0x00,
  0x01, 0xE0, 0x00, 0x3C, 0x00, 0x07, 0x80, 0x00, 0xF0, 0x00, 0x1E, 0x00, 0x03, 0xC0, 0x00, 0x78,
  0x00, 0x0E, 0xFF, 0xE1, 0xBF, 0xFE, 0x37, 0xFF, 0xC7, 0x7F, 0xF0, 0xF0, 0x00, 0x9E, 0x00, 0x3F,
  0xC0, 0x07, 0xF8, 0x00, 0xFF, 0x00, 0x1F, 0xE0, 0x03, 0xFC, 0x00, 0x7F, 0x80, 0x0F, 0xF0, 0x01,
  0xFE, 0x00, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0xFF, 0xF7, 0x7F, 0xFF, 0x80, 0x3F, 0xFF, 0x87,
  0xFF, 0xF3, 0x7F, 0xFD, 0x77, 0xFF, 0x7F, 0x00, 0x1F, 0xE0, 0x03, 0xFC, 0x00, 0x7F, 0x80, 0x0F,
  0xF0, 0x01, 0xFE, 0x00, 0x3F, 0xC0, 0x07, 0xF8, 0x00, 0xFF, 0x00, 0x1F, 0xC0, 0x03, 0xF0, 0x00,
  0x38, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x00, 0x3C, 0x00, 0x07, 0x80, 0x00, 0xF0, 0x00, 0x1E, 0x00,
  0x03, 0xC0, 0x00, 0x78, 0x00, 0x0F, 0x00, 0x01, 0xE0, 0x00, 0x3C, 0x00, 0x07, 0x80, 0x00, 0x70,
  0x00, 0x06, 0x00, 0x00, 0x80, 0x3F, 0xFF, 0x9B, 0xFF, 0xF3, 0xFF, 0xFC, 0xFF, 0xFF, 0x3F, 0x00,
  0x0F, 0xE0, 0x03, 0xFC, 0x00, 0x7F, 0x80, 0x0F, 0xF0, 0x01, 0xFE, 0x00, 0x3F, 0xC0, 0x07, 0xF8,
  0x00, 0xFE, 0xFF, 0xFF, 0xBF, 0xFF, 0xF7, 0xFF, 0xE7, 0x7F, 0xF4, 0xF0, 0x00, 0x9E, 0x00, 0x3F,
  0xC0, 0x07, 0xF8, 0x00, 0xFF, 0x00, 0x1F, 0xE0, 0x03, 0xFC, 0x00, 0x7F, 0x80, 0x0F, 0xF0, 0x01,
  0xFE, 0x00, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0xFF, 0x80, 0x3F, 0xFF, 0xDF,
  0xFF, 0xFB, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x1F, 0xE0, 0x03, 0xFC, 0x00, 0x7F, 0x80, 0x0F,
  0xF0, 0x01, 0xFE, 0x00, 0x3F, 0xC0, 0x07, 0xF8, 0x00, 0xFE, 0xFF, 0xEF, 0xBF, 0xFE, 0xC7, 0xFF,
  0xD8, 0x7F, 0xF3, 0x00, 0x00, 0xE0, 0x00, 0x3C, 0x00, 0x07, 0x80, 0x00, 0xF0, 0x00, 0x1E, 0x00,
  0x03, 0xC0, 0x00, 0x78, 0x00, 0x0F, 0x00, 0x01, 0xE0, 0x00, 0x3C, 0x3F, 0xFF, 0x8F, 0xFF, 0xF3,
  0xFF, 0xFE, 0x7F, 0xFF, 0x80, 0x6F, 0xF6, 0x00, 0x00, 0x00, 0x00, 0x6F, 0xF6, 0xFF, 0xFF, 0xFF,
  0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0,
  0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1F, 0xFF, 0xFF, 0xFF, 0xFF, 0xC0, 0x1C,
  0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01,
  0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1F, 0xFF, 0xFF, 0xFF, 0xFF, 0xC0, 0x1C, 0x01, 0xC0,
  0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C,
  0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1F, 0xFF, 0xFF, 0xFF, 0xFF, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01,
  0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0,
  0x1C, 0x01, 0xC0, 0x1F, 0xFF, 0xFF, 0xFF, 0xFF, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C,
  0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01,
  0xC0, 0x1F, 0xFF, 0xFF, 0xFF, 0xFF, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0,
  0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1F,
  0xFF, 0x3F, 0xFF, 0x9B, 0xFF, 0xF3, 0xFF, 0xFC, 0xFF, 0xFF, 0x3F, 0x00, 0x0F, 0xE0, 0x03, 0xFC,
  0x00, 0x7F, 0x80, 0x0F, 0xF0, 0x01, 0xFE, 0x00, 0x3F, 0xC0, 0x07, 0xF8, 0x00, 0xFE, 0xFF, 0xFF,
  0xBF, 0xFF, 0xF7, 0xFF, 0xE7, 0x7F, 0xF4, 0xF0, 0x00, 0x9E, 0x00, 0x3F, 0xC0, 0x07, 0xF8, 0x00,
  0xFF, 0x00, 0x1F, 0xE0, 0x03, 0xFC, 0x00, 0x7F, 0x80, 0x0F, 0xF0, 0x01, 0xFE, 0x00, 0x3F, 0xC0,
  0x07, 0xF0, 0x00, 0x7C, 0x00, 0x07, 0x00, 0x00, 0x80, 0x00, 0x00, 0x18, 0x00, 0x03, 0x80, 0x00,
  0x78, 0x00, 0x0F, 0x00, 0x01, 0xE0, 0x00, 0x3C, 0x00, 0x07, 0x80, 0x00, 0xF0, 0x00, 0x1E, 0x00,
  0x03, 0xC0, 0x00, 0x78, 0x00, 0x0E, 0xFF, 0xE1, 0xBF, 0xFE, 0x37, 0xFF, 0xFF, 0x7F, 0xF7, 0xF0,
  0x01, 0xFE, 0x00, 0x3F, 0xC0, 0x07, 0xF8, 0x00, 0xFF, 0x00, 0x1F, 0xE0, 0x03, 0xFC, 0x00, 0x7F,
  0x80, 0x0F, 0xF0, 0x01, 0xFE, 0x00, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0xFF,
  0x80, 0x0F, 0xFE, 0x0F, 0xFF, 0xB7, 0xFF, 0xDD, 0xFF, 0xCF, 0x00, 0x07, 0x80, 0x03, 0xC0, 0x01,
  0xE0, 0x00, 0xF0, 0x00, 0x78, 0x00, 0x3C, 0x00, 0x1E, 0x00, 0x0F, 0x00, 0x07, 0x80, 0x03, 0xFF,
  0xF9, 0xFF, 0xFC, 0xFF, 0xFF, 0x5F, 0xFF, 0xC0, 0x00, 0x00, 0x40, 0x00, 0x18, 0x00, 0x07, 0x80,
  0x00, 0xF0, 0x00, 0x1E, 0x00, 0x03, 0xC0, 0x00, 0x78, 0x00, 0x0F, 0x00, 0x01, 0xE0, 0x00, 0x3C,
  0x00, 0x07, 0x80, 0x00, 0xF0, 0xFF, 0xEE, 0x3F, 0xFE, 0xF7, 0xFF, 0xFF, 0x7F, 0xF7, 0xF0, 0x01,
  0xFE, 0x00, 0x3F, 0xC0, 0x07, 0xF8, 0x00, 0xFF, 0x00, 0x1F, 0xE0, 0x03, 0xFC, 0x00, 0x7F, 0x80,
  0x0F, 0xF0, 0x01, 0xFE, 0x00, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0xFF, 0x80,
  0x3F, 0xFF, 0xEF, 0xFF, 0xFF, 0xFF, 0xDF, 0xFF, 0xCF, 0x00, 0x07, 0x80, 0x03, 0xC0, 0x01, 0xE0,
  0x00, 0xF0, 0x00, 0x78, 0x00, 0x3C, 0x00, 0x1E, 0x00, 0x0E, 0xFF, 0xE6, 0xFF, 0xFB, 0x7F, 0xFD,
  0xDF, 0xFC, 0xF0, 0x00, 0x78, 0x00, 0x3C, 0x00, 0x1E, 0x00, 0x0F, 0x00, 0x07, 0x80, 0x03, 0xC0,
  0x01, 0xE0, 0x00, 0xF0, 0x00, 0x78, 0x00, 0x3F, 0xFF, 0x9F, 0xFF, 0xEF, 0xFF, 0xFD, 0xFF, 0xFC,
  0x3F, 0xFF, 0xEF, 0xFF, 0xFF, 0xFF, 0xDF, 0xFF, 0xCF, 0x00, 0x07, 0x80, 0x03, 0xC0, 0x01, 0xE0,
  0x00, 0xF0, 0x00, 0x78, 0x00, 0x3C, 0x00, 0x1E, 0x00, 0x0E, 0xFF, 0xE6, 0xFF, 0xFB, 0x7F, 0xFD,
  0xDF, 0xFC, 0xF0, 0x00, 0x78, 0x00, 0x3C, 0x00, 0x1E, 0x00, 0x0F, 0x00, 0x07, 0x80, 0x03, 0xC0,
  0x01, 0xE0, 0x00, 0xF0, 0x00, 0x78, 0x00, 0x3C, 0x00, 0x1C, 0x00, 0x0C, 0x00, 0x04, 0x00, 0x00,
  0x3F, 0xFF, 0x87, 0xFF, 0xF3, 0x7F, 0xFC, 0x77, 0xFF, 0x0F, 0x00, 0x01, 0xE0, 0x00, 0x3C, 0x00,
  0x07, 0x80, 0x00, 0xF0, 0x00, 0x1E, 0x00, 0x03, 0xC0, 0x00, 0x78, 0x00, 0x0F, 0x00, 0x01, 0xC0,
  0x00, 0x30, 0x00, 0x06, 0x00, 0x00, 0xE0, 0x00, 0x9E, 0x00, 0x3F, 0xC0, 0x07, 0xF8, 0x00, 0xFF,
  0x00, 0x1F, 0xE0, 0x03, 0xFC, 0x00, 0x7F, 0x80, 0x0F, 0xF0, 0x01, 0xFE, 0x00, 0x3F, 0xFF, 0xFF,
  0xFF, 0xFF, 0x7F, 0xFF, 0xF7, 0x7F, 0xFF, 0x80, 0x00, 0x00, 0x18, 0x00, 0x03, 0x80, 0x00, 0x78,
  0x00, 0x0F, 0x00, 0x01, 0xE0, 0x00, 0x3C, 0x00, 0x07, 0x80, 0x00, 0xF0, 0x00, 0x1E, 0x00, 0x03,
  0xC0, 0x00, 0x78, 0x00, 0x0E, 0xFF, 0xE1, 0xBF, 0xFE, 0x37, 0xFF, 0xFF, 0x7F, 0xF7, 0xF0, 0x01,
  0xFE, 0x00, 0x3F, 0xC0, 0x07, 0xF8, 0x00, 0xFF, 0x00, 0x1F, 0xE0, 0x03, 0xFC, 0x00, 0x7F, 0x80,
  0x0F, 0xF0, 0x01, 0xFE, 0x00, 0x3F, 0xC0, 0x07, 0xF0, 0x00, 0xFC, 0x00, 0x0F, 0x00, 0x00, 0x80,
  0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xF7, 0x20, 0x00, 0x00, 0x40, 0x00, 0x18, 0x00, 0x07, 0x80, 0x00,
  0xF0, 0x00, 0x1E, 0x00, 0x03, 0xC0, 0x00, 0x78, 0x00, 0x0F, 0x00, 0x01, 0xE0, 0x00, 0x3C, 0x00,
  0x07, 0x80, 0x00, 0xF0, 0x00, 0x0E, 0x00, 0x00, 0xF0, 0x00, 0x3F, 0x00, 0x0F, 0xF0, 0x01, 0xFE,
  0x00, 0x3F, 0xC0, 0x07, 0xF8, 0x00, 0xFF, 0x00, 0x1F, 0xE0, 0x03, 0xFC, 0x00, 0x7F, 0x80, 0x0F,
  0xF0, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFB, 0xFF, 0xFC, 0x3F, 0xFF, 0x9B, 0xFF,
  0xF3, 0xFF, 0xFC, 0x7F, 0xFF, 0x0F, 0x00, 0x01, 0xE0, 0x00, 0x3C, 0x00, 0x07, 0x80, 0x00, 0xF0,
  0x00, 0x1E, 0x00, 0x03, 0xC0, 0x00, 0x78, 0x00, 0x0E, 0xFF, 0xE1, 0xBF, 0xFE, 0x37, 0xFF, 0xC7,
  0x7F, 0xF0, 0xF0, 0x00, 0x9E, 0x00, 0x3F, 0xC0, 0x07, 0xF8, 0x00, 0xFF, 0x00, 0x1F, 0xE0, 0x03,
  0xFC, 0x00, 0x7F, 0x80, 0x0F, 0xF0, 0x01, 0xFE, 0x00, 0x3F, 0xC0, 0x07, 0xF0, 0x00, 0x7C, 0x00,
  0x07, 0x00, 0x00, 0x80, 0xC0, 0x00, 0x70, 0x00, 0x3C, 0x00, 0x1E, 0x00, 0x0F, 0x00, 0x07, 0x80,
  0x03, 0xC0, 0x01, 0xE0, 0x00, 0xF0, 0x00, 0x78, 0x00, 0x3C, 0x00, 0x1C, 0x00, 0x0C, 0x00, 0x06,
  0x00, 0x03, 0x80, 0x01, 0xE0, 0x00, 0xF0, 0x00, 0x78, 0x00, 0x3C, 0x00, 0x1E, 0x00, 0x0F, 0x00,
  0x07, 0x80, 0x03, 0xC0, 0x01, 0xE0, 0x00, 0xFF, 0xFE, 0x7F, 0xFF, 0x3F, 0xFF, 0xD7, 0xFF, 0xF0,
  0x3F, 0xFF, 0x8F, 0xFF, 0xF3, 0xFF, 0xFC, 0x77, 0xFF, 0x3F, 0x00, 0x0F, 0xE0, 0x03, 0xFC, 0x00,
  0x7F, 0x80, 0x0F, 0xF0, 0x01, 0xFE, 0x00, 0x3F, 0xC0, 0x07, 0xF8, 0x00, 0xFF, 0x00, 0x1F, 0xC0,
  0x03, 0xF0, 0x00, 0x78, 0x00, 0x04, 0xC0, 0x00, 0x9C, 0x00, 0x3F, 0xC0, 0x07, 0xF8, 0x00, 0xFF,
  0x00, 0x1F, 0xE0, 0x03, 0xFC, 0x00, 0x7F, 0x80, 0x0F, 0xF0, 0x01, 0xFE, 0x00, 0x3F, 0xC0, 0x07,
  0xF0, 0x00, 0x7C, 0x00, 0x07, 0x00, 0x00, 0x80, 0x0F, 0xFE, 0x03, 0xFF, 0xE3, 0x7F, 0xFF, 0xF7,
  0xFF, 0x7F, 0x00, 0x1F, 0xE0, 0x03, 0xFC, 0x00, 0x7F, 0x80, 0x0F, 0xF0, 0x01, 0xFE, 0x00, 0x3F,
  0xC0, 0x07, 0xF8, 0x00, 0xFF, 0x00, 0x1F, 0xE0, 0x03, 0xFC, 0x00, 0x7F, 0x00, 0x0F, 0xC0, 0x00,
  0xF0, 0x00, 0x08, 0x0F, 0xFE, 0x03, 0xFF, 0xE3, 0x7F, 0xFF, 0xF7, 0xFF, 0x7F, 0x00, 0x1F, 0xE0,
  0x03, 0xFC, 0x00, 0x7F, 0x80, 0x0F, 0xF0, 0x01, 0xFE, 0x00, 0x3F, 0xC0, 0x07, 0xF8, 0x00, 0xFF,
  0x00, 0x1F, 0xE0, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF7, 0xFF, 0xF8, 0x3F, 0xFF,
  0x9B, 0xFF, 0xFB, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x1F, 0xE0, 0x03, 0xFC, 0x00, 0x7F, 0x80,
  0x0F, 0xF0, 0x01, 0xFE, 0x00, 0x3F, 0xC0, 0x07, 0xF8, 0x00, 0xFE, 0xFF, 0xFF, 0xBF, 0xFF, 0xF7,
  0xFF, 0xC7, 0x7F, 0xF0, 0xF0, 0x00, 0x1E, 0x00, 0x03, 0xC0, 0x00, 0x78, 0x00, 0x0F, 0x00, 0x01,
  0xE0, 0x00, 0x3C, 0x00, 0x07, 0x80, 0x00, 0xF0, 0x00, 0x1E, 0x00, 0x03, 0xC0, 0x00, 0x70, 0x00,
  0x0C, 0x00, 0x01, 0x00, 0x00, 0x00, 0x3F, 0xFF, 0xDF, 0xFF, 0xFB, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x1F, 0xE0, 0x03, 0xFC, 0x00, 0x7F, 0x80, 0x0F, 0xF0, 0x01, 0xFE, 0x00, 0x3F, 0xC0, 0x07,
  0xF8, 0x00, 0xFE, 0xFF, 0xEF, 0xBF, 0xFE, 0xC7, 0xFF, 0xD8, 0x7F, 0xF3, 0x00, 0x00, 0xE0, 0x00,
  0x3C, 0x00, 0x07, 0x80, 0x00, 0xF0, 0x00, 0x1E, 0x00, 0x03, 0xC0, 0x00, 0x78, 0x00, 0x0F, 0x00,
  0x01, 0xE0, 0x00, 0x3C, 0x00, 0x07, 0x80, 0x00, 0x70, 0x00, 0x06, 0x00, 0x00, 0x80, 0x0F, 0xFE,
  0x1F, 0xFF, 0xDF, 0xFF, 0xEF, 0xFE, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00,
  0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xE0, 0x00, 0xC0, 0x00,
  0x80, 0x00, 0x00, 0x00, 0x18, 0x00, 0x03, 0x80, 0x00, 0x78, 0x00, 0x0F, 0x00, 0x01, 0xE0, 0x00,
  0x3C, 0x00, 0x07, 0x80, 0x00, 0xF0, 0x00, 0x1E, 0x00, 0x03, 0xC0, 0x00, 0x78, 0x00, 0x0E, 0xFF,
  0xE1, 0xBF, 0xFE, 0x07, 0xFF, 0xD8, 0x7F, 0xF7, 0x00, 0x01, 0xE0, 0x00, 0x3C, 0x00, 0x07, 0x80,
  0x00, 0xF0, 0x00, 0x1E, 0x00, 0x03, 0xC0, 0x00, 0x78, 0x00, 0x0F, 0x00, 0x01, 0xE0, 0x00, 0x3C,
  0x3F, 0xFB, 0x8F, 0xFF, 0x73, 0xFF, 0xF6, 0x7F, 0xFF, 0x80, 0x00, 0x00, 0x60, 0x00, 0x38, 0x00,
  0x1E, 0x00, 0x0F, 0x00, 0x07, 0x80, 0x03, 0xC0, 0x01, 0xE0, 0x00, 0xF0, 0x00, 0x78, 0x00, 0x3C,
  0x00, 0x1E, 0x00, 0x0E, 0xFF, 0xE6, 0xFF, 0xFB, 0x7F, 0xFD, 0xDF, 0xFC, 0xF0, 0x00, 0x78, 0x00,
  0x3C, 0x00, 0x1E, 0x00, 0x0F, 0x00, 0x07, 0x80, 0x03, 0xC0, 0x01, 0xE0, 0x00, 0xF0, 0x00, 0x78,
  0x00, 0x3F, 0xFF, 0x9F, 0xFF, 0xCF, 0xFF, 0xF5, 0xFF, 0xFC, 0xC0, 0x00, 0xFC, 0x00, 0x3F, 0xC0,
  0x07, 0xF8, 0x00, 0xFF, 0x00, 0x1F, 0xE0, 0x03, 0xFC, 0x00, 0x7F, 0x80, 0x0F, 0xF0, 0x01, 0xFE,
  0x00, 0x3F, 0xC0, 0x07, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0xFF, 0xF0, 0x00, 0x00,
  0x58, 0x00, 0x1B, 0x80, 0x07, 0xF8, 0x00, 0xFF, 0x00, 0x1F, 0xE0, 0x03, 0xFC, 0x00, 0x7F, 0x80,
  0x0F, 0xF0, 0x01, 0xFE, 0x00, 0x3F, 0xC0, 0x07, 0xF8, 0x00, 0xFE, 0x00, 0x0F, 0x80, 0x00, 0xF0,
  0x00, 0x3F, 0x00, 0x0F, 0xF0, 0x01, 0xFE, 0x00, 0x3F, 0xC0, 0x07, 0xF8, 0x00, 0xFF, 0x00, 0x1F,
  0xE0, 0x03, 0xFC, 0x00, 0x7F, 0x80, 0x0F, 0xF0, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFB, 0xFF, 0xFC, 0x00, 0x00, 0x58, 0x00, 0x1B, 0x80, 0x07, 0xF8, 0x00, 0xFF, 0x00, 0x1F, 0xE0,
  0x03, 0xFC, 0x00, 0x7F, 0x80, 0x0F, 0xF0, 0x01, 0xFE, 0x00, 0x3F, 0xC0, 0x07, 0xF8, 0x00, 0xFE,
  0xFF, 0xEF, 0xBF, 0xFE, 0xF7, 0xFF, 0xFF, 0x7F, 0xF7, 0xF0, 0x01, 0xFE, 0x00, 0x3F, 0xC0, 0x07,
  0xF8, 0x00, 0xFF, 0x00, 0x1F, 0xE0, 0x03, 0xFC, 0x00, 0x7F, 0x80, 0x0F, 0xF0, 0x01, 0xFE, 0x00,
  0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0xFF, 0x80, 0x00, 0x00, 0x58, 0x00, 0x1B,
  0x80, 0x07, 0xF8, 0x00, 0xFF, 0x00, 0x1F, 0xE0, 0x03, 0xFC, 0x00, 0x7F, 0x80, 0x0F, 0xF0, 0x01,
  0xFE, 0x00, 0x3F, 0xC0, 0x07, 0xF8, 0x00, 0xFE, 0xFF, 0xEF, 0xBF, 0xFE, 0xF7, 0xFF, 0xFF, 0x7F,
  0xF7, 0xF0, 0x01, 0xFE, 0x00, 0x3F, 0xC0, 0x07, 0xF8, 0x00, 0xFF, 0x00, 0x1F, 0xE0, 0x03, 0xFC,
  0x00, 0x7F, 0x80, 0x0F, 0xF0, 0x01, 0xFE, 0x00, 0x3F, 0xC0, 0x07, 0xF0, 0x00, 0xFC, 0x00, 0x0F,
  0x00, 0x00, 0x80, 0x00, 0x00, 0x58, 0x00, 0x1B, 0x80, 0x07, 0xF8, 0x00, 0xFF, 0x00, 0x1F, 0xE0,
  0x03, 0xFC, 0x00, 0x7F, 0x80, 0x0F, 0xF0, 0x01, 0xFE, 0x00, 0x3F, 0xC0, 0x07, 0xF8, 0x00, 0xFE,
  0xFF, 0xEF, 0xBF, 0xFE, 0xC7, 0xFF, 0xD8, 0x7F, 0xF7, 0x00, 0x01, 0xE0, 0x00, 0x3C, 0x00, 0x07,
  0x80, 0x00, 0xF0, 0x00, 0x1E, 0x00, 0x03, 0xC0, 0x00, 0x78, 0x00, 0x0F, 0x00, 0x01, 0xE0, 0x00,
  0x3C, 0x3F, 0xFB, 0x8F, 0xFF, 0xF3, 0xFF, 0xFE, 0x7F, 0xFF, 0x80, 0x3F, 0xFF, 0x87, 0xFF, 0xF8,
  0x7F, 0xFF, 0x87, 0xFF, 0xF0, 0x00, 0x1E, 0x00, 0x03, 0xC0, 0x00, 0x78, 0x00, 0x0F, 0x00, 0x01,
  0xE0, 0x00, 0x3C, 0x00, 0x07, 0x80, 0x00, 0xF0, 0x00, 0x1E, 0x00, 0x01, 0xC0, 0x00, 0x1E, 0x00,
  0x00, 0xE0, 0x00, 0x1E, 0x00, 0x03, 0xC0, 0x00, 0x78, 0x00, 0x0F, 0x00, 0x01, 0xE0, 0x00, 0x3C,
  0x00, 0x07, 0x80, 0x00, 0xF0, 0x00, 0x1E, 0x00, 0x03, 0xFF, 0xF8, 0x7F, 0xFF, 0x0F, 0xFF, 0xF1,
  0x7F, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0,
  0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1F,
  0xFF, 0xFF, 0xFF, 0xFF, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01,
  0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1F, 0xFF, 0xFF,
  0xFF, 0xFF, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C,
  0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1F, 0xFF, 0xFF, 0xFF, 0xFF,
  0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0,
  0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1F, 0xFF, 0x1F, 0xFC, 0x3F, 0xFE, 0x7F,
  0xFF, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0,
  0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1F,
  0xFF, 0x3F, 0xFF, 0x9B, 0xFF, 0xF3, 0xFF, 0xFC, 0xFF, 0xFF, 0x3F, 0x00, 0x0F, 0xE0, 0x03, 0xFC,
  0x00, 0x7F, 0x80, 0x0F, 0xF0, 0x01, 0xFE, 0x00, 0x3F, 0xC0, 0x07, 0xF8, 0x00, 0xFE, 0xFF, 0xFF,
  0xBF, 0xFF, 0xF7, 0xFF, 0xE7, 0x7F, 0xF4, 0xF0, 0x00, 0x9E, 0x00, 0x3F, 0xC0, 0x07, 0xF8, 0x00,
  0xFF, 0x00, 0x1F, 0xE0, 0x03, 0xFC, 0x00, 0x7F, 0x80, 0x0F, 0xF0, 0x01, 0xFE, 0x00, 0x3F, 0xC0,
  0x07, 0xF0, 0x00, 0x7C, 0x00, 0x07, 0x00, 0x00, 0x80, 0x00, 0x00, 0x18, 0x00, 0x03, 0x80, 0x00,
  0x78, 0x00, 0x0F, 0x00, 0x01, 0xE0, 0x00, 0x3C, 0x00, 0x07, 0x80, 0x00, 0xF0, 0x00, 0x1E, 0x00,
  0x03, 0xC0, 0x00, 0x78, 0x00, 0x0E, 0xFF, 0xE1, 0xBF, 0xFE, 0x37, 0xFF, 0xFF, 0x7F, 0xF7, 0xF0,
  0x01, 0xFE, 0x00, 0x3F, 0xC0, 0x07, 0xF8, 0x00, 0xFF, 0x00, 0x1F, 0xE0, 0x03, 0xFC, 0x00, 0x7F,
  0x80, 0x0F, 0xF0, 0x01, 0xFE, 0x00, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0xFF,
  0x80, 0x0F, 0xFE, 0x0F, 0xFF, 0xB7, 0xFF, 0xDD, 0xFF, 0xCF, 0x00, 0x07, 0x80, 0x03, 0xC0, 0x01,
  0xE0, 0x00, 0xF0, 0x00, 0x78, 0x00, 0x3C, 0x00, 0x1E, 0x00, 0x0F, 0x00, 0x07, 0x80, 0x03, 0xFF,
  0xF9, 0xFF, 0xFC, 0xFF, 0xFF, 0x5F, 0xFF, 0xC0, 0x00, 0x00, 0x40, 0x00, 0x18, 0x00, 0x07, 0x80,
  0x00, 0xF0, 0x00, 0x1E, 0x00, 0x03, 0xC0, 0x00, 0x78, 0x00, 0x0F, 0x00, 0x01, 0xE0, 0x00, 0x3C,
  0x00, 0x07, 0x80, 0x00, 0xF0, 0xFF, 0xEE, 0x3F, 0xFE, 0xF7, 0xFF, 0xFF, 0x7F, 0xF7, 0xF0, 0x01,
  0xFE, 0x00, 0x3F, 0xC0, 0x07, 0xF8, 0x00, 0xFF, 0x00, 0x1F, 0xE0, 0x03, 0xFC, 0x00, 0x7F, 0x80,
  0x0F, 0xF0, 0x01, 0xFE, 0x00, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0xFF, 0x80,
  0x3F, 0xFF, 0xEF, 0xFF, 0xFF, 0xFF, 0xDF, 0xFF, 0xCF, 0x00, 0x07, 0x80, 0x03, 0xC0, 0x01, 0xE0,
  0x00, 0xF0, 0x00, 0x78, 0x00, 0x3C, 0x00, 0x1E, 0x00, 0x0E, 0xFF, 0xE6, 0xFF, 0xFB, 0x7F, 0xFD,
  0xDF, 0xFC, 0xF0, 0x00, 0x78, 0x00, 0x3C, 0x00, 0x1E, 0x00, 0x0F, 0x00, 0x07, 0x80, 0x03, 0xC0,
  0x01, 0xE0, 0x00, 0xF0, 0x00, 0x78, 0x00, 0x3F, 0xFF, 0x9F, 0xFF, 0xEF, 0xFF, 0xFD, 0xFF, 0xFC,
  0x3F, 0xFF, 0xEF, 0xFF, 0xFF, 0xFF, 0xDF, 0xFF, 0xCF, 0x00, 0x07, 0x80, 0x03, 0xC0, 0x01, 0xE0,
  0x00, 0xF0, 0x00, 0x78, 0x00, 0x3C, 0x00, 0x1E, 0x00, 0x0E, 0xFF, 0xE6, 0xFF, 0xFB, 0x7F, 0xFD,
  0xDF, 0xFC, 0xF0, 0x00, 0x78, 0x00, 0x3C, 0x00, 0x1E, 0x00, 0x0F, 0x00, 0x07, 0x80, 0x03, 0xC0,
  0x01, 0xE0, 0x00, 0xF0, 0x00, 0x78, 0x00, 0x3C, 0x00, 0x1C, 0x00, 0x0C, 0x00, 0x04, 0x00, 0x00,
  0x3F, 0xFF, 0x87, 0xFF, 0xF3, 0x7F, 0xFC, 0x77, 0xFF, 0x0F, 0x00, 0x01, 0xE0, 0x00, 0x3C, 0x00,
  0x07, 0x80, 0x00, 0xF0, 0x00, 0x1E, 0x00, 0x03, 0xC0, 0x00, 0x78, 0x00, 0x0F, 0x00, 0x01, 0xC0,
  0x00, 0x30, 0x00, 0x06, 0x00, 0x00, 0xE0, 0x00, 0x9E, 0x00, 0x3F, 0xC0, 0x07, 0xF8, 0x00, 0xFF,
  0x00, 0x1F, 0xE0, 0x03, 0xFC, 0x00, 0x7F, 0x80, 0x0F, 0xF0, 0x01, 0xFE, 0x00, 0x3F, 0xFF, 0xFF,
  0xFF, 0xFF, 0x7F, 0xFF, 0xF7, 0x7F, 0xFF, 0x80, 0x00, 0x00, 0x18, 0x00, 0x03, 0x80, 0x00, 0x78,
  0x00, 0x0F, 0x00, 0x01, 0xE0, 0x00, 0x3C, 0x00, 0x07, 0x80, 0x00, 0xF0, 0x00, 0x1E, 0x00, 0x03,
  0xC0, 0x00, 0x78, 0x00, 0x0E, 0xFF, 0xE1, 0xBF, 0xFE, 0x37, 0xFF, 0xFF, 0x7F, 0xF7, 0xF0, 0x01,
  0xFE, 0x00, 0x3F, 0xC0, 0x07, 0xF8, 0x00, 0xFF, 0x00, 0x1F, 0xE0, 0x03, 0xFC, 0x00, 0x7F, 0x80,
  0x0F, 0xF0, 0x01, 0xFE, 0x00, 0x3F, 0xC0, 0x07, 0xF0, 0x00, 0xFC, 0x00, 0x0F, 0x00, 0x00, 0x80,
  0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xF7, 0x20, 0x00, 0x00, 0x40, 0x00, 0x18, 0x00, 0x07, 0x80, 0x00,
  0xF0, 0x00, 0x1E, 0x00, 0x03, 0xC0, 0x00, 0x78, 0x00, 0x0F, 0x00, 0x01, 0xE0, 0x00, 0x3C, 0x00,
  0x07, 0x80, 0x00, 0xF0, 0x00, 0x0E, 0x00, 0x00, 0xF0, 0x00, 0x3F, 0x00, 0x0F, 0xF0, 0x01, 0xFE,
  0x00, 0x3F, 0xC0, 0x07, 0xF8, 0x00, 0xFF, 0x00, 0x1F, 0xE0, 0x03, 0xFC, 0x00, 0x7F, 0x80, 0x0F,
  0xF0, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFB, 0xFF, 0xFC, 0x3F, 0xFF, 0x9B, 0xFF,
  0xF3, 0xFF, 0xFC, 0x7F, 0xFF, 0x0F, 0x00, 0x01, 0xE0, 0x00, 0x3C, 0x00, 0x07, 0x80, 0x00, 0xF0,
  0x00, 0x1E, 0x00, 0x03, 0xC0, 0x00, 0x78, 0x00, 0x0E, 0xFF, 0xE1, 0xBF, 0xFE, 0x37, 0xFF, 0xC7,
  0x7F, 0xF0, 0xF0, 0x00, 0x9E, 0x00, 0x3F, 0xC0, 0x07, 0xF8, 0x00, 0xFF, 0x00, 0x1F, 0xE0, 0x03,
  0xFC, 0x00, 0x7F, 0x80, 0x0F, 0xF0, 0x01, 0xFE, 0x00, 0x3F, 0xC0, 0x07, 0xF0, 0x00, 0x7C, 0x00,
  0x07, 0x00, 0x00, 0x80, 0xC0, 0x00, 0x70, 0x00, 0x3C, 0x00, 0x1E, 0x00, 0x0F, 0x00, 0x07, 0x80,
  0x03, 0xC0, 0x01, 0xE0, 0x00, 0xF0, 0x00, 0x78, 0x00, 0x3C, 0x00, 0x1C, 0x00, 0x0C, 0x00, 0x06,
  0x00, 0x03, 0x80, 0x01, 0xE0, 0x00, 0xF0, 0x00, 0x78, 0x00, 0x3C, 0x00, 0x1E, 0x00, 0x0F, 0x00,
  0x07, 0x80, 0x03, 0xC0, 0x01, 0xE0, 0x00, 0xFF, 0xFE, 0x7F, 0xFF, 0x3F, 0xFF, 0xD7, 0xFF, 0xF0,
  0x3F, 0xFF, 0x8F, 0xFF, 0xF3, 0xFF, 0xFC, 0x77, 0xFF, 0x3F, 0x00, 0x0F, 0xE0, 0x03, 0xFC, 0x00,
  0x7F, 0x80, 0x0F, 0xF0, 0x01, 0xFE, 0x00, 0x3F, 0xC0, 0x07, 0xF8, 0x00, 0xFF, 0x00, 0x1F, 0xC0,
  0x03, 0xF0, 0x00, 0x78, 0x00, 0x04, 0xC0, 0x00, 0x9C, 0x00, 0x3F, 0xC0, 0x07, 0xF8, 0x00, 0xFF,
  0x00, 0x1F, 0xE0, 0x03, 0xFC, 0x00, 0x7F, 0x80, 0x0F, 0xF0, 0x01, 0xFE, 0x00, 0x3F, 0xC0, 0x07,
  0xF0, 0x00, 0x7C, 0x00, 0x07, 0x00, 0x00, 0x80, 0x0F, 0xFE, 0x03, 0xFF, 0xE3, 0x7F, 0xFF, 0xF7,
  0xFF, 0x7F, 0x00, 0x1F, 0xE0, 0x03, 0xFC, 0x00, 0x7F, 0x80, 0x0F, 0xF0, 0x01, 0xFE, 0x00, 0x3F,
  0xC0, 0x07, 0xF8, 0x00, 0xFF, 0x00, 0x1F, 0xE0, 0x03, 0xFC, 0x00, 0x7F, 0x00, 0x0F, 0xC0, 0x00,
  0xF0, 0x00, 0x08, 0x0F, 0xFE, 0x03, 0xFF, 0xE3, 0x7F, 0xFF, 0xF7, 0xFF, 0x7F, 0x00, 0x1F, 0xE0,
  0x03, 0xFC, 0x00, 0x7F, 0x80, 0x0F, 0xF0, 0x01, 0xFE, 0x00, 0x3F, 0xC0, 0x07, 0xF8, 0x00, 0xFF,
  0x00, 0x1F, 0xE0, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF7, 0xFF, 0xF8, 0x3F, 0xFF,
  0x9B, 0xFF, 0xFB, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x1F, 0xE0, 0x03, 0xFC, 0x00, 0x7F, 0x80,
  0x0F, 0xF0, 0x01, 0xFE, 0x00, 0x3F, 0xC0, 0x07, 0xF8, 0x00, 0xFE, 0xFF, 0xFF, 0xBF, 0xFF, 0xF7,
  0xFF, 0xC7, 0x7F, 0xF0, 0xF0, 0x00, 0x1E, 0x00, 0x03, 0xC0, 0x00, 0x78, 0x00, 0x0F, 0x00, 0x01,
  0xE0, 0x00, 0x3C, 0x00, 0x07, 0x80, 0x00, 0xF0, 0x00, 0x1E, 0x00, 0x03, 0xC0, 0x00, 0x70, 0x00,
  0x0C, 0x00, 0x01, 0x00, 0x00, 0x00, 0x3F, 0xFF, 0xDF, 0xFF, 0xFB, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x1F, 0xE0, 0x03, 0xFC, 0x00, 0x7F, 0x80, 0x0F, 0xF0, 0x01, 0xFE, 0x00, 0x3F, 0xC0, 0x07,
  0xF8, 0x00, 0xFE, 0xFF, 0xEF, 0xBF, 0xFE, 0xC7, 0xFF, 0xD8, 0x7F, 0xF3, 0x00, 0x00, 0xE0, 0x00,
  0x3C, 0x00, 0x07, 0x80, 0x00, 0xF0, 0x00, 0x1E, 0x00, 0x03, 0xC0, 0x00, 0x78, 0x00, 0x0F, 0x00,
  0x01, 0xE0, 0x00, 0x3C, 0x00, 0x07, 0x80, 0x00, 0x70, 0x00, 0x06, 0x00, 0x00, 0x80, 0x0F, 0xFE,
  0x1F, 0xFF, 0xDF, 0xFF, 0xEF, 0xFE, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00,
  0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xE0, 0x00, 0xC0, 0x00,
  0x80, 0x00, 0x00, 0x00, 0x18, 0x00, 0x03, 0x80, 0x00, 0x78, 0x00, 0x0F, 0x00, 0x01, 0xE0, 0x00,
  0x3C, 0x00, 0x07, 0x80, 0x00, 0xF0, 0x00, 0x1E, 0x00, 0x03, 0xC0, 0x00, 0x78, 0x00, 0x0E, 0xFF,
  0xE1, 0xBF, 0xFE, 0x07, 0xFF, 0xD8, 0x7F, 0xF7, 0x00, 0x01, 0xE0, 0x00, 0x3C, 0x00, 0x07, 0x80,
  0x00, 0xF0, 0x00, 0x1E, 0x00, 0x03, 0xC0, 0x00, 0x78, 0x00, 0x0F, 0x00, 0x01, 0xE0, 0x00, 0x3C,
  0x3F, 0xFB, 0x8F, 0xFF, 0x73, 0xFF, 0xF6, 0x7F, 0xFF, 0x80, 0x00, 0x00, 0x60, 0x00, 0x38, 0x00,
  0x1E, 0x00, 0x0F, 0x00, 0x07, 0x80, 0x03, 0xC0, 0x01, 0xE0, 0x00, 0xF0, 0x00, 0x78, 0x00, 0x3C,
  0x00, 0x1E, 0x00, 0x0E, 0xFF, 0xE6, 0xFF, 0xFB, 0x7F, 0xFD, 0xDF, 0xFC, 0xF0, 0x00, 0x78, 0x00,
  0x3C, 0x00, 0x1E, 0x00, 0x0F, 0x00, 0x07, 0x80, 0x03, 0xC0, 0x01, 0xE0, 0x00, 0xF0, 0x00, 0x78,
  0x00, 0x3F, 0xFF, 0x9F, 0xFF, 0xCF, 0xFF, 0xF5, 0xFF, 0xFC, 0xC0, 0x00, 0xFC, 0x00, 0x3F, 0xC0,
  0x07, 0xF8, 0x00, 0xFF, 0x00, 0x1F, 0xE0, 0x03, 0xFC, 0x00, 0x7F, 0x80, 0x0F, 0xF0, 0x01, 0xFE,
  0x00, 0x3F, 0xC0, 0x07, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0xFF, 0xF0, 0x00, 0x00,
  0x58, 0x00, 0x1B, 0x80, 0x07, 0xF8, 0x00, 0xFF, 0x00, 0x1F, 0xE0, 0x03, 0xFC, 0x00, 0x7F, 0x80,
  0x0F, 0xF0, 0x01, 0xFE, 0x00, 0x3F, 0xC0, 0x07, 0xF8, 0x00, 0xFE, 0x00, 0x0F, 0x80, 0x00, 0xF0,
  0x00, 0x3F, 0x00, 0x0F, 0xF0, 0x01, 0xFE, 0x00, 0x3F, 0xC0, 0x07, 0xF8, 0x00, 0xFF, 0x00, 0x1F,
  0xE0, 0x03, 0xFC, 0x00, 0x7F, 0x80, 0x0F, 0xF0, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFB, 0xFF, 0xFC, 0x00, 0x00, 0x58, 0x00, 0x1B, 0x80, 0x07, 0xF8, 0x00, 0xFF, 0x00, 0x1F, 0xE0,
  0x03, 0xFC, 0x00, 0x7F, 0x80, 0x0F, 0xF0, 0x01, 0xFE, 0x00, 0x3F, 0xC0, 0x07, 0xF8, 0x00, 0xFE,
  0xFF, 0xEF, 0xBF, 0xFE, 0xF7, 0xFF, 0xFF, 0x7F, 0xF7, 0xF0, 0x01, 0xFE, 0x00, 0x3F, 0xC0, 0x07,
  0xF8, 0x00, 0xFF, 0x00, 0x1F, 0xE0, 0x03, 0xFC, 0x00, 0x7F, 0x80, 0x0F, 0xF0, 0x01, 0xFE, 0x00,
  0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0xFF, 0x80, 0x00, 0x00, 0x58, 0x00, 0x1B,
  0x80, 0x07, 0xF8, 0x00, 0xFF, 0x00, 0x1F, 0xE0, 0x03, 0xFC, 0x00, 0x7F, 0x80, 0x0F, 0xF0, 0x01,
  0xFE, 0x00, 0x3F, 0xC0, 0x07, 0xF8, 0x00, 0xFE, 0xFF, 0xEF, 0xBF, 0xFE, 0xF7, 0xFF, 0xFF, 0x7F,
  0xF7, 0xF0, 0x01, 0xFE, 0x00, 0x3F, 0xC0, 0x07, 0xF8, 0x00, 0xFF, 0x00, 0x1F, 0xE0, 0x03, 0xFC,
  0x00, 0x7F, 0x80, 0x0F, 0xF0, 0x01, 0xFE, 0x00, 0x3F, 0xC0, 0x07, 0xF0, 0x00, 0xFC, 0x00, 0x0F,
  0x00, 0x00, 0x80, 0x00, 0x00, 0x58, 0x00, 0x1B, 0x80, 0x07, 0xF8, 0x00, 0xFF, 0x00, 0x1F, 0xE0,
  0x03, 0xFC, 0x00, 0x7F, 0x80, 0x0F, 0xF0, 0x01, 0xFE, 0x00, 0x3F, 0xC0, 0x07, 0xF8, 0x00, 0xFE,
  0xFF, 0xEF, 0xBF, 0xFE, 0xC7, 0xFF, 0xD8, 0x7F, 0xF7, 0x00, 0x01, 0xE0, 0x00, 0x3C, 0x00, 0x07,
  0x80, 0x00, 0xF0, 0x00, 0x1E, 0x00, 0x03, 0xC0, 0x00, 0x78, 0x00, 0x0F, 0x00, 0x01, 0xE0, 0x00,
  0x3C, 0x3F, 0xFB, 0x8F, 0xFF, 0xF3, 0xFF, 0xFE, 0x7F, 0xFF, 0x80, 0x3F, 0xFF, 0x87, 0xFF, 0xF8,
  0x7F, 0xFF, 0x87, 0xFF, 0xF0, 0x00, 0x1E, 0x00, 0x03, 0xC0, 0x00, 0x78, 0x00, 0x0F, 0x00, 0x01,
  0xE0, 0x00, 0x3C, 0x00, 0x07, 0x80, 0x00, 0xF0, 0x00, 0x1E, 0x00, 0x01, 0xC0, 0x00, 0x1E, 0x00,
  0x00, 0xE0, 0x00, 0x1E, 0x00, 0x03, 0xC0, 0x00, 0x78, 0x00, 0x0F, 0x00, 0x01, 0xE0, 0x00, 0x3C,
  0x00, 0x07, 0x80, 0x00, 0xF0, 0x00, 0x1E, 0x00, 0x03, 0xFF, 0xF8, 0x7F, 0xFF, 0x0F, 0xFF, 0xF1,
  0x7F, 0xFF, 0x00,
};
const GFXglyph StudioReadoutGlyphs[] PROGMEM = {
  {0, 1, 1, 6, 0, 0},   // 0x20  
  {1, 1, 1, 24, 0, 0},   // 0x21 !
  {2, 12, 20, 15, 1, -19},   // 0x22 "
  {32, 12, 20, 15, 1, -19},   // 0x23 #
  {62, 12, 20, 15, 1, -19},   // 0x24 $
  {92, 12, 20, 15, 1, -19},   // 0x25 %
  {122, 12, 20, 15, 1, -19},   // 0x26 &
  {152, 12, 20, 15, 1, -19},   // 0x27 '
  {182, 12, 20, 15, 1, -19},   // 0x28 (
  {212, 12, 20, 15, 1, -19},   // 0x29 )
  {242, 12, 20, 15, 1, -19},   // 0x2A *
  {272, 12, 20, 15, 1, -19},   // 0x2B +
  {302, 12, 20, 15, 1, -19},   // 0x2C ,
  {332, 14, 4, 24, 5, -17},   // 0x2D -
  {339, 4, 4, 0, -2, -3},   // 0x2E .
  {341, 12, 20, 15, 1, -19},   // 0x2F /
  {371, 19, 30, 24, 3, -29},   // 0x30 0
  {443, 4, 27, 24, 18, -26},   // 0x31 1
  {457, 19, 30, 24, 3, -29},   // 0x32 2
  {529, 18, 30, 24, 4, -29},   // 0x33 3
  {597, 19, 30, 24, 3, -29},   // 0x34 4
  {669, 19, 30, 24, 3, -29},   // 0x35 5
  {741, 19, 30, 24, 3, -29},   // 0x36 6
  {813, 19, 30, 24, 3, -29},   // 0x37 7
  {885, 19, 30, 24, 3, -29},   // 0x38 8
  {957, 19, 30, 24, 3, -29},   // 0x39 9
  {1029, 4, 16, 6, 1, -22},   // 0x3A :
  {1037, 12, 20, 15, 1, -19},   // 0x3B ;
  {1067, 12, 20, 15, 1, -19},   // 0x3C <
  {1097, 12, 20, 15, 1, -19},   // 0x3D =
  {1127, 12, 20, 15, 1, -19},   // 0x3E >
  {1157, 12, 20, 15, 1, -19},   // 0x3F ?
  {1187, 12, 20, 15, 1, -19},   // 0x40 @
  {1217, 19, 30, 24, 3, -29},   // 0x41 A
  {1289, 19, 30, 24, 3, -29},   // 0x42 B
  {1361, 17, 18, 24, 3, -17},   // 0x43 C
  {1400, 19, 30, 24, 3, -29},   // 0x44 D
  {1472, 17, 30, 24, 3, -29},   // 0x45 E
  {1536, 17, 30, 24, 3, -29},   // 0x46 F
  {1600, 19, 30, 24, 3, -29},   // 0x47 G
  {1672, 19, 30, 24, 3, -29},   // 0x48 H
  {1744, 4, 13, 24, 18, -12},   // 0x49 I
  {1751, 19, 29, 24, 3, -28},   // 0x4A J
  {1820, 19, 30, 24, 3, -29},   // 0x4B K
  {1892, 17, 28, 24, 3, -27},   // 0x4C L
  {1952, 19, 30, 24, 3, -29},   // 0x4D M
  {2024, 19, 18, 24, 3, -17},   // 0x4E N
  {2067, 19, 18, 24, 3, -17},   // 0x4F O
  {2110, 19, 30, 24, 3, -29},   // 0x50 P
  {2182, 19, 30, 24, 3, -29},   // 0x51 Q
  {2254, 16, 18, 24, 3, -17},   // 0x52 R
  {2290, 19, 30, 24, 3, -29},   // 0x53 S
  {2362, 17, 30, 24, 3, -29},   // 0x54 T
  {2426, 19, 15, 24, 3, -14},   // 0x55 U
  {2462, 19, 29, 24, 3, -28},   // 0x56 V
  {2531, 19, 30, 24, 3, -29},   // 0x57 W
  {2603, 19, 30, 24, 3, -29},   // 0x58 X
  {2675, 19, 30, 24, 3, -29},   // 0x59 Y
  {2747, 19, 30, 24, 3, -29},   // 0x5A Z
  {2819, 12, 20, 15, 1, -19},   // 0x5B [
  {2849, 12, 20, 15, 1, -19},   // 0x5C backslash
  {2879, 12, 20, 15, 1, -19},   // 0x5D ]
  {2909, 12, 20, 15, 1, -19},   // 0x5E ^
  {2939, 16, 4, 24, 4, -3},   // 0x5F _
  {2947, 12, 20, 15, 1, -19},   // 0x60 `
  {2977, 19, 30, 24, 3, -29},   // 0x61 a
  {3049, 19, 30, 24, 3, -29},   // 0x62 b
  {3121, 17, 18, 24, 3, -17},   // 0x63 c
  {3160, 19, 30, 24, 3, -29},   // 0x64 d
  {3232, 17, 30, 24, 3, -29},   // 0x65 e
  {3296, 17, 30, 24, 3, -29},   // 0x66 f
  {3360, 19, 30, 24, 3, -29},   // 0x67 g
  {3432, 19, 30, 24, 3, -29},   // 0x68 h
  {3504, 4, 13, 24, 18, -12},   // 0x69 i
  {3511, 19, 29, 24, 3, -28},   // 0x6A j
  {3580, 19, 30, 24, 3, -29},   // 0x6B k
  {3652, 17, 28, 24, 3, -27},   // 0x6C l
  {3712, 19, 30, 24, 3, -29},   // 0x6D m
  {3784, 19, 18, 24, 3, -17},   // 0x6E n
  {3827, 19, 18, 24, 3, -17},   // 0x6F o
  {3870, 19, 30, 24, 3, -29},   // 0x70 p
  {3942, 19, 30, 24, 3, -29},   // 0x71 q
  {4014, 16, 18, 24, 3, -17},   // 0x72 r
  {4050, 19, 30, 24, 3, -29},   // 0x73 s
  {4122, 17, 30, 24, 3, -29},   // 0x74 t
  {4186, 19, 15, 24, 3, -14},   // 0x75 u
  {4222, 19, 29, 24, 3, -28},   // 0x76 v
  {4291, 19, 30, 24, 3, -29},   // 0x77 w
  {4363, 19, 30, 24, 3, -29},   // 0x78 x
  {4435, 19, 30, 24, 3, -29},   // 0x79 y
  {4507, 19, 30, 24, 3, -29},   // 0x7A z
};
const GFXfont StudioReadout PROGMEM = {(uint8_t*)StudioReadoutBitmaps, (GFXglyph*)StudioReadoutGlyphs, 0x20, 0x7A, 33};

// StudioBig: generated from barlow-semi-condensed-600.ttf at 22px
const uint8_t StudioBigBitmaps[] PROGMEM = {
  0x00, 0xFF, 0xFF, 0xB6, 0xDB, 0x01, 0xBE, 0xEF, 0xD9, 0xB3, 0x66, 0xC0, 0x1C, 0xE1, 0xCE, 0x18,
  0xE1, 0x8E, 0x7F, 0xF7, 0xFF, 0x38, 0xC3, 0x9C, 0x39, 0xCF, 0xFE, 0xFF, 0xE3, 0x18, 0x71, 0x87,
  0x18, 0x73, 0x80, 0x1C, 0x0C, 0x0F, 0x8F, 0xEE, 0x3E, 0x0F, 0x07, 0xC0, 0x78, 0x3E, 0x07, 0xC0,
  0xF0, 0x3E, 0x0F, 0x07, 0xC7, 0x7F, 0x1F, 0x07, 0x03, 0x80, 0x38, 0x18, 0xF8, 0x61, 0x18, 0xC6,
  0x13, 0x04, 0x66, 0x0F, 0x98, 0x0E, 0x30, 0x00, 0xC0, 0x01, 0x00, 0x06, 0x78, 0x09, 0xF8, 0x33,
  0x30, 0x44, 0x21, 0x88, 0x46, 0x19, 0x8C, 0x1E, 0x1E, 0x07, 0xF0, 0x73, 0x86, 0x38, 0x63, 0x87,
  0x38, 0x3F, 0x01, 0xE0, 0x3C, 0x07, 0xE2, 0xE7, 0x7C, 0x3E, 0xC1, 0xCE, 0x7C, 0x7F, 0xE3, 0xE7,
  0xFD, 0xB6, 0x39, 0x9C, 0xE6, 0x73, 0x9C, 0xE7, 0x39, 0xCE, 0x73, 0x8C, 0x73, 0x8C, 0x70, 0xC7,
  0x18, 0xE7, 0x39, 0xC6, 0x39, 0xCE, 0x73, 0x39, 0xCE, 0x77, 0x39, 0x80, 0x31, 0x67, 0xFB, 0xE7,
  0x9F, 0xEC, 0x98, 0x18, 0x0C, 0x06, 0x1F, 0xFF, 0xF8, 0xC0, 0x60, 0x30, 0x18, 0x00, 0x7F, 0x6D,
  0x80, 0xFF, 0xF0, 0xFF, 0x80, 0x07, 0x07, 0x06, 0x0E, 0x0E, 0x0C, 0x1C, 0x1C, 0x18, 0x18, 0x38,
  0x30, 0x30, 0x70, 0x60, 0x60, 0x3E, 0x3F, 0xBC, 0xFC, 0x7E, 0x1F, 0x0F, 0x87, 0xC3, 0xE1, 0xF0,
  0xF8, 0x7C, 0x3E, 0x3F, 0x9D, 0xFC, 0x7C, 0x7F, 0xFE, 0x73, 0x9C, 0xE7, 0x39, 0xCE, 0x73, 0x9C,
  0xE7, 0x3C, 0x3F, 0xB9, 0xDC, 0x7C, 0x38, 0x1C, 0x1C, 0x0E, 0x0E, 0x0E, 0x0F, 0x07, 0x07, 0x07,
  0x03, 0xFF, 0xFF, 0xFF, 0x7F, 0x81, 0xC1, 0xC1, 0xE0, 0xE0, 0xF8, 0x3E, 0x03, 0x01, 0x80, 0xF8,
  0x7E, 0x37, 0x39, 0xF8, 0x78, 0x1C, 0x07, 0x03, 0x80, 0xE0, 0x30, 0x1C, 0x07, 0x39, 0x8E, 0xE3,
  0xB8, 0xEF, 0xFF, 0xFF, 0x03, 0x80, 0xE0, 0x38, 0x0E, 0xFF, 0x7F, 0xB8, 0x1C, 0x0E, 0x07, 0x03,
  0xF9, 0xFE, 0xE3, 0x81, 0xC0, 0xF8, 0x7E, 0x3F, 0xB9, 0xFC, 0x7C, 0x3E, 0x3F, 0xB9, 0xDC, 0x6E,
  0x3F, 0x03, 0xF9, 0xFE, 0xE3, 0x71, 0xF8, 0xFC, 0x7E, 0x3B, 0xB9, 0xFC, 0x7C, 0xFF, 0xFF, 0xF8,
  0x68, 0x70, 0x38, 0x18, 0x1C, 0x0E, 0x06, 0x07, 0x03, 0x81, 0x81, 0xC0, 0xE0, 0x60, 0x70, 0x3E,
  0x3F, 0xB9, 0xDC, 0x7E, 0x3F, 0x19, 0xFC, 0xFC, 0xF7, 0x71, 0xF8, 0xFC, 0x7E, 0x3F, 0xB9, 0xFC,
  0x78, 0x3C, 0x3F, 0xB9, 0xDC, 0x6C, 0x3E, 0x1F, 0x8F, 0xCF, 0x7F, 0x9D, 0xC0, 0xFC, 0x7E, 0x3F,
  0x39, 0xFC, 0x7C, 0xFF, 0x80, 0x00, 0xFF, 0x80, 0x6F, 0xF6, 0x00, 0x06, 0x66, 0x6C, 0xC0, 0x01,
  0x83, 0xCF, 0xDF, 0x0E, 0x07, 0x81, 0xF0, 0x3E, 0x07, 0x80, 0xC0, 0xFF, 0xFF, 0xC0, 0x00, 0x0F,
  0xFF, 0xFC, 0xC0, 0x78, 0x1F, 0x03, 0xE0, 0x78, 0x3C, 0xF9, 0xF0, 0xE0, 0x40, 0x00, 0x3C, 0x7F,
  0xE7, 0xC3, 0xC3, 0x03, 0x07, 0x0E, 0x1C, 0x38, 0x38, 0x00, 0x00, 0x38, 0x38, 0x38, 0x03, 0xE0,
  0x1F, 0xF0, 0x78, 0xF1, 0xC0, 0x77, 0x3E, 0x6C, 0xFC, 0xF1, 0x98, 0xE6, 0x21, 0xCC, 0x47, 0x99,
  0x8F, 0x33, 0x1E, 0x7F, 0xEC, 0x7F, 0xCC, 0x00, 0x1E, 0x18, 0x1F, 0xF0, 0x0F, 0x80, 0x0F, 0x00,
  0xF0, 0x0F, 0x00, 0xF0, 0x1F, 0x81, 0x98, 0x19, 0x81, 0x9C, 0x39, 0xC3, 0x8C, 0x30, 0xC3, 0xFE,
  0x7F, 0xE7, 0x0E, 0x60, 0x6E, 0x07, 0xFF, 0x3F, 0xEE, 0x3F, 0x87, 0xE1, 0xF8, 0x7E, 0x3B, 0xFC,
  0xFF, 0xB8, 0x7E, 0x1F, 0x87, 0xE1, 0xF8, 0xFF, 0xFB, 0xFC, 0x1E, 0x1F, 0xE7, 0x3B, 0x87, 0xE1,
  0xF8, 0x0E, 0x03, 0x80, 0xE0, 0x38, 0x0E, 0x03, 0x87, 0xE1, 0xDC, 0xE7, 0xF8, 0x78, 0xFF, 0x3F,
  0xEE, 0x3F, 0x87, 0xE1, 0xF8, 0x7E, 0x1F, 0x87, 0xE1, 0xF8, 0x7E, 0x1F, 0x87, 0xE1, 0xF8, 0xFF,
  0xFB, 0xFC, 0xFF, 0xFF, 0xF8, 0x1C, 0x0E, 0x07, 0x03, 0x81, 0xFC, 0xFE, 0x70, 0x38, 0x1C, 0x0E,
  0x07, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xF8, 0x1C, 0x0E, 0x07, 0x03, 0x81, 0xFC, 0xFE, 0x70, 0x38,
  0x1C, 0x0E, 0x07, 0x03, 0x81, 0xC0, 0x1E, 0x1F, 0xE7, 0x3B, 0x87, 0xE1, 0xF8, 0x0E, 0x03, 0x9F,
  0xE7, 0xF8, 0x7E, 0x1F, 0x87, 0xE1, 0xDC, 0xE7, 0xF8, 0x78, 0xE1, 0xF8, 0x7E, 0x1F, 0x87, 0xE1,
  0xF8, 0x7E, 0x1F, 0xFF, 0xFF, 0xF8, 0x7E, 0x1F, 0x87, 0xE1, 0xF8, 0x7E, 0x1F, 0x87, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0xC0, 0x70, 0x1C, 0x07, 0x01, 0xC0, 0x70, 0x1C, 0x07, 0x01, 0xC0,
  0x7E, 0x1F, 0x87, 0x61, 0xDC, 0xE7, 0xF8, 0x78, 0xE1, 0xDC, 0x3B, 0x8E, 0x73, 0x8E, 0x71, 0xDC,
  0x3F, 0x07, 0xE0, 0xFE, 0x1E, 0xC3, 0x9C, 0x71, 0x8E, 0x39, 0xC3, 0x38, 0x77, 0x07, 0xE0, 0x70,
  0x38, 0x1C, 0x0E, 0x07, 0x03, 0x81, 0xC0, 0xE0, 0x70, 0x38, 0x1C, 0x0E, 0x07, 0x03, 0xFF, 0xFF,
  0xE0, 0x7E, 0x1F, 0xC7, 0xFC, 0xFF, 0xFF, 0xFE, 0xFB, 0xDF, 0x33, 0xE6, 0x7C, 0x0F, 0x81, 0xF0,
  0x3E, 0x07, 0xC0, 0xF8, 0x1F, 0x03, 0xE0, 0xFE, 0x1F, 0xC3, 0xFC, 0x7F, 0x8F, 0xF9, 0xFF, 0x3F,
  0x67, 0xEE, 0xFC, 0xDF, 0x9F, 0xF1, 0xFE, 0x3F, 0xC3, 0xF8, 0x7F, 0x07, 0x1E, 0x1F, 0xE7, 0x3B,
  0x87, 0xE1, 0xF8, 0x7E, 0x1F, 0x87, 0xE1, 0xF8, 0x7E, 0x1F, 0x87, 0xE1, 0xDC, 0xE7, 0xF8, 0x78,
  0xFF, 0x3F, 0xEE, 0x3F, 0x87, 0xE1, 0xF8, 0x7E, 0x1F, 0xFE, 0xFF, 0x38, 0x0E, 0x03, 0x80, 0xE0,
  0x38, 0x0E, 0x03, 0x80, 0x3E, 0x3F, 0x9C, 0xFC, 0x3E, 0x1F, 0x0F, 0x87, 0xC3, 0xE1, 0xF0, 0xF8,
  0x7C, 0x3E, 0x1F, 0x1D, 0xFE, 0x7E, 0x1C, 0x0E, 0x07, 0x00, 0xFF, 0x3F, 0xEE, 0x3F, 0x87, 0xE1,
  0xF8, 0x7E, 0x1F, 0xFE, 0xFF, 0x39, 0xCE, 0x73, 0x8C, 0xE3, 0xB8, 0x6E, 0x1F, 0x87, 0x3E, 0x3F,
  0xB8, 0xFC, 0x3C, 0x1F, 0x01, 0xC0, 0x78, 0x1F, 0x03, 0xC0, 0xF8, 0x3E, 0x1F, 0x9D, 0xFE, 0x7C,
  0xFF, 0xFF, 0xF1, 0xC0, 0x70, 0x1C, 0x07, 0x01, 0xC0, 0x70, 0x1C, 0x07, 0x01, 0xC0, 0x70, 0x1C,
  0x07, 0x01, 0xC0, 0x70, 0xE1, 0xF8, 0x7E, 0x1F, 0x87, 0xE1, 0xF8, 0x7E, 0x1F, 0x87, 0xE1, 0xF8,
  0x7E, 0x1F, 0x87, 0xE1, 0xDC, 0xE7, 0xF8, 0x78, 0xC0, 0xF8, 0x7E, 0x1F, 0x86, 0x61, 0x9C, 0xE7,
  0x39, 0xCC, 0x33, 0x0F, 0xC3, 0xF0, 0xF8, 0x1E, 0x07, 0x81, 0xE0, 0x70, 0xC3, 0x87, 0x87, 0x0F,
  0x8E, 0x3F, 0x1C, 0x7E, 0x7C, 0xEC, 0xD9, 0x99, 0xB3, 0x3B, 0x6E, 0x76, 0xFC, 0x6D, 0xF0, 0xF1,
  0xE1, 0xE3, 0xC3, 0xC7, 0x83, 0x8F, 0x07, 0x1C, 0x0E, 0x18, 0xC1, 0xF8, 0x76, 0x39, 0xCE, 0x37,
  0x0F, 0xC1, 0xE0, 0x78, 0x1E, 0x07, 0x83, 0xF0, 0xDC, 0x73, 0x98, 0xEE, 0x1F, 0x87, 0xC1, 0xF8,
  0x66, 0x39, 0xCE, 0x73, 0x0D, 0xC3, 0xE0, 0x78, 0x1C, 0x03, 0x00, 0xC0, 0x30, 0x0C, 0x03, 0x00,
  0xC0, 0x30, 0xFF, 0xFF, 0xC0, 0xE0, 0xE0, 0x70, 0x70, 0x30, 0x38, 0x18, 0x1C, 0x1C, 0x0E, 0x0E,
  0x07, 0x03, 0xFF, 0xFF, 0xFF, 0xF1, 0x8C, 0x63, 0x18, 0xC6, 0x31, 0x8C, 0x63, 0x18, 0xC6, 0x31,
  0xFF, 0x80, 0xE0, 0x60, 0x60, 0x70, 0x30, 0x30, 0x38, 0x38, 0x18, 0x1C, 0x1C, 0x0C, 0x0E, 0x0E,
  0x06, 0x07, 0xFF, 0xC6, 0x31, 0x8C, 0x63, 0x18, 0xC6, 0x31, 0x8C, 0x63, 0x18, 0xC7, 0xFF, 0x80,
  0x18, 0x38, 0x3C, 0x3C, 0x6C, 0x66, 0x66, 0xC6, 0xC3, 0xFF, 0xFF, 0xF0, 0xCC, 0x3C, 0x3F, 0xB9,
  0xDC, 0x60, 0x3B, 0xFF, 0xFF, 0xC7, 0xC3, 0xF3, 0xFF, 0xEF, 0x70, 0xE0, 0x70, 0x38, 0x1C, 0x0F,
  0xE7, 0xFB, 0xDF, 0xC7, 0xE3, 0xF1, 0xF8, 0xFC, 0x7E, 0x3F, 0xBB, 0xFD, 0xFC, 0x3E, 0x3F, 0xBD,
  0xDC, 0x7E, 0x07, 0x03, 0x81, 0xC0, 0xE3, 0xFB, 0x9F, 0xC7, 0xC0, 0x03, 0x81, 0xC0, 0xE0, 0x73,
  0xBB, 0xFF, 0xDF, 0xC7, 0xE3, 0xF1, 0xF8, 0xFC, 0x7E, 0x3B, 0xBD, 0xFE, 0x7F, 0x3E, 0x3F, 0xBD,
  0xDC, 0x7E, 0x3F, 0xFF, 0xFF, 0xC0, 0xE2, 0x7B, 0xDF, 0xC7, 0xC0, 0x3D, 0xF7, 0x1C, 0xFF, 0xF7,
  0x1C, 0x71, 0xC7, 0x1C, 0x71, 0xC7, 0x1C, 0x3B, 0xBF, 0xFD, 0xFC, 0x7E, 0x3E, 0x1F, 0x0F, 0xC7,
  0xE3, 0xFB, 0xDF, 0xE7, 0x70, 0x38, 0x38, 0xFC, 0x78, 0xE0, 0x70, 0x38, 0x1C, 0x0E, 0xE7, 0xFB,
  0xDF, 0xC7, 0xE3, 0xF1, 0xF8, 0xFC, 0x7E, 0x3F, 0x1F, 0x8F, 0xC7, 0x5F, 0xA1, 0xFF, 0xFF, 0xFF,
  0xFF, 0xE0, 0x11, 0xCE, 0x20, 0x1C, 0xE7, 0x39, 0xCE, 0x73, 0x9C, 0xE7, 0x39, 0xDF, 0xEE, 0x00,
  0xE0, 0x70, 0x38, 0x1C, 0x0E, 0x3F, 0x3B, 0xB9, 0xF8, 0xFC, 0x7F, 0x3F, 0x9C, 0xCE, 0x77, 0x1B,
  0x8D, 0xC7, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0x7B, 0xFF, 0xFF, 0x79, 0xF8, 0xE3, 0xE3,
  0x8F, 0x8E, 0x3E, 0x38, 0xF8, 0xE3, 0xE3, 0x8F, 0x8E, 0x3E, 0x38, 0xF8, 0xE3, 0xFE, 0x7F, 0xBD,
  0xFC, 0x7E, 0x3F, 0x1F, 0x8F, 0xC7, 0xE3, 0xF1, 0xF8, 0xFC, 0x70, 0x3E, 0x3F, 0xBD, 0xDC, 0x7E,
  0x3E, 0x1F, 0x0F, 0xC7, 0xE3, 0xFB, 0x9F, 0xC7, 0xC0, 0xEE, 0x7F, 0xBD, 0xFC, 0x7E, 0x3F, 0x1F,
  0x8F, 0xC7, 0xE3, 0xFB, 0xFF, 0xDD, 0xCE, 0x07, 0x03, 0x81, 0xC0, 0x3B, 0xBF, 0xFD, 0xFC, 0x7E,
  0x3F, 0x1F, 0x8F, 0xC7, 0xE3, 0xFB, 0xDF, 0xE7, 0xF0, 0x38, 0x1C, 0x0E, 0x07, 0xFF, 0xFF, 0x38,
  0xE3, 0x8E, 0x38, 0xE3, 0x8E, 0x38, 0x3C, 0xFE, 0xE7, 0xC3, 0xE0, 0x7C, 0x3E, 0x07, 0xC3, 0xE7,
  0xFF, 0x3C, 0x30, 0x60, 0xC7, 0xFF, 0xE6, 0x0C, 0x18, 0x30, 0x60, 0xC1, 0x83, 0x87, 0x87, 0x00,
  0xE3, 0xF1, 0xF8, 0xFC, 0x7E, 0x3F, 0x1F, 0x8F, 0xC7, 0xE3, 0xF3, 0xDF, 0xE7, 0x70, 0xE1, 0xDC,
  0x67, 0x39, 0xCE, 0x33, 0x0E, 0xC3, 0xF0, 0xFC, 0x1E, 0x07, 0x81, 0xE0, 0x38, 0xE3, 0x8D, 0x8E,
  0x37, 0x39, 0xDD, 0xE7, 0x77, 0xD8, 0xDB, 0x63, 0x6F, 0x8F, 0xBE, 0x3E, 0xF0, 0x73, 0xC1, 0xC7,
  0x07, 0x1C, 0xE1, 0xDC, 0xE3, 0xB8, 0xFC, 0x1E, 0x07, 0x81, 0xE0, 0x78, 0x3F, 0x0E, 0xE7, 0x3B,
  0x87, 0xE1, 0xB1, 0xDC, 0xEE, 0x63, 0x31, 0x98, 0xFC, 0x7C, 0x1E, 0x0F, 0x07, 0x01, 0x81, 0xC0,
  0xE1, 0xE0, 0xE0, 0xFF, 0xFC, 0x38, 0x71, 0xC7, 0x0E, 0x38, 0xE1, 0xC3, 0xFF, 0xF0, 0x3D, 0xF7,
  0x1C, 0x71, 0xC7, 0x1C, 0x73, 0x8E, 0x38, 0x71, 0xC7, 0x1C, 0x71, 0xC7, 0x1F, 0x3C, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFC, 0xF3, 0xC7, 0x0C, 0x30, 0xC3, 0x0C, 0x30, 0xF1, 0xCE, 0x30, 0xC3, 0x0C, 0x30,
  0xC7, 0x3C, 0xE0, 0x70, 0x7E, 0xF3, 0xE0, 0xE0,
};
const GFXglyph StudioBigGlyphs[] PROGMEM = {
  {0, 1, 1, 4, 0, 0},   // 0x20  
  {1, 3, 16, 7, 2, -15},   // 0x21 !
  {7, 7, 5, 7, 0, -15},   // 0x22 "
  {12, 12, 15, 14, 1, -15},   // 0x23 #
  {35, 9, 20, 11, 1, -17},   // 0x24 $
  {58, 15, 16, 18, 1, -15},   // 0x25 %
  {88, 12, 16, 14, 1, -15},   // 0x26 &
  {112, 3, 5, 3, 0, -15},   // 0x27 '
  {114, 5, 20, 7, 0, -17},   // 0x28 (
  {127, 5, 20, 7, 2, -17},   // 0x29 )
  {140, 7, 8, 8, 1, -16},   // 0x2A *
  {147, 9, 9, 10, 1, -11},   // 0x2B +
  {158, 3, 6, 5, 1, -3},   // 0x2C ,
  {161, 6, 2, 8, 1, -7},   // 0x2D -
  {163, 3, 3, 5, 1, -2},   // 0x2E .
  {165, 8, 16, 9, 0, -15},   // 0x2F /
  {181, 9, 16, 11, 1, -15},   // 0x30 0
  {199, 5, 16, 7, 1, -15},   // 0x31 1
  {209, 9, 16, 11, 1, -15},   // 0x32 2
  {227, 9, 16, 11, 1, -15},   // 0x33 3
  {245, 10, 16, 11, 1, -15},   // 0x34 4
  {265, 9, 16, 11, 1, -15},   // 0x35 5
  {283, 9, 16, 11, 1, -15},   // 0x36 6
  {301, 9, 16, 10, 0, -15},   // 0x37 7
  {319, 9, 16, 11, 1, -15},   // 0x38 8
  {337, 9, 16, 11, 1, -15},   // 0x39 9
  {355, 3, 11, 7, 2, -10},   // 0x3A :
  {360, 4, 13, 6, 1, -10},   // 0x3B ;
  {367, 9, 10, 10, 1, -10},   // 0x3C <
  {379, 9, 6, 10, 1, -9},   // 0x3D =
  {386, 9, 10, 10, 1, -10},   // 0x3E >
  {398, 8, 16, 10, 1, -15},   // 0x3F ?
  {414, 15, 17, 17, 1, -14},   // 0x40 @
  {446, 12, 16, 12, 0, -15},   // 0x41 A
  {470, 10, 16, 12, 1, -15},   // 0x42 B
  {490, 10, 16, 12, 1, -15},   // 0x43 C
  {510, 10, 16, 12, 1, -15},   // 0x44 D
  {530, 9, 16, 11, 1, -15},   // 0x45 E
  {548, 9, 16, 11, 1, -15},   // 0x46 F
  {566, 10, 16, 12, 1, -15},   // 0x47 G
  {586, 10, 16, 12, 1, -15},   // 0x48 H
  {606, 3, 16, 5, 1, -15},   // 0x49 I
  {612, 10, 16, 11, 0, -15},   // 0x4A J
  {632, 11, 16, 12, 1, -15},   // 0x4B K
  {654, 9, 16, 11, 1, -15},   // 0x4C L
  {672, 11, 16, 14, 1, -15},   // 0x4D M
  {694, 11, 16, 13, 1, -15},   // 0x4E N
  {716, 10, 16, 12, 1, -15},   // 0x4F O
  {736, 10, 16, 12, 1, -15},   // 0x50 P
  {756, 9, 19, 11, 1, -15},   // 0x51 Q
  {778, 10, 16, 12, 1, -15},   // 0x52 R
  {798, 9, 16, 11, 1, -15},   // 0x53 S
  {816, 10, 16, 11, 1, -15},   // 0x54 T
  {836, 10, 16, 12, 1, -15},   // 0x55 U
  {856, 10, 16, 12, 1, -15},   // 0x56 V
  {876, 15, 16, 17, 1, -15},   // 0x57 W
  {906, 10, 16, 12, 1, -15},   // 0x58 X
  {926, 10, 16, 12, 1, -15},   // 0x59 Y
  {946, 9, 16, 11, 1, -15},   // 0x5A Z
  {964, 5, 21, 8, 1, -17},   // 0x5B [
  {978, 8, 16, 9, 1, -15},   // 0x5C backslash
  {994, 5, 21, 8, 2, -17},   // 0x5D ]
  {1008, 8, 9, 10, 1, -15},   // 0x5E ^
  {1017, 10, 2, 10, 0, -1},   // 0x5F _
  {1020, 3, 2, 5, 1, -14},   // 0x60 `
  {1021, 9, 12, 11, 1, -11},   // 0x61 a
  {1035, 9, 16, 11, 1, -15},   // 0x62 b
  {1053, 9, 12, 11, 1, -11},   // 0x63 c
  {1067, 9, 16, 11, 1, -15},   // 0x64 d
  {1085, 9, 12, 11, 1, -11},   // 0x65 e
  {1099, 6, 16, 7, 1, -15},   // 0x66 f
  {1111, 9, 16, 11, 1, -11},   // 0x67 g
  {1129, 9, 16, 11, 1, -15},   // 0x68 h
  {1147, 3, 17, 5, 1, -16},   // 0x69 i
  {1154, 5, 21, 5, -1, -16},   // 0x6A j
  {1168, 9, 16, 11, 1, -15},   // 0x6B k
  {1186, 3, 16, 5, 1, -15},   // 0x6C l
  {1192, 14, 12, 16, 1, -11},   // 0x6D m
  {1213, 9, 12, 11, 1, -11},   // 0x6E n
  {1227, 9, 12, 11, 1, -11},   // 0x6F o
  {1241, 9, 16, 11, 1, -11},   // 0x70 p
  {1259, 9, 16, 11, 1, -11},   // 0x71 q
  {1277, 6, 12, 8, 1, -11},   // 0x72 r
  {1286, 8, 12, 10, 1, -11},   // 0x73 s
  {1298, 7, 15, 7, 0, -14},   // 0x74 t
  {1312, 9, 12, 11, 1, -11},   // 0x75 u
  {1326, 10, 12, 10, 0, -11},   // 0x76 v
  {1341, 14, 12, 15, 0, -11},   // 0x77 w
  {1362, 10, 12, 10, 0, -11},   // 0x78 x
  {1377, 9, 16, 10, 0, -11},   // 0x79 y
  {1395, 7, 12, 9, 1, -11},   // 0x7A z
  {1406, 6, 21, 7, 1, -17},   // 0x7B {
  {1422, 2, 19, 4, 1, -16},   // 0x7C |
  {1427, 6, 21, 7, 1, -17},   // 0x7D }
  {1443, 9, 4, 11, 1, -8},   // 0x7E ~
};
const GFXfont StudioBig PROGMEM = {(uint8_t*)StudioBigBitmaps, (GFXglyph*)StudioBigGlyphs, 0x20, 0x7E, 26};

// BoldSmall: generated from sg-500.ttf at 12px (cursor y = top of the text)
const uint8_t BoldSmallBitmaps[] PROGMEM = {
  0x00, 0xAA, 0x8B, 0xBF, 0x5A, 0x24, 0x4B, 0xF9, 0x22, 0x5F, 0xC9, 0x12, 0x10, 0x21, 0xF2, 0xB5,
  0x67, 0x07, 0xE9, 0x56, 0xF8, 0x40, 0x21, 0x28, 0xA4, 0x8E, 0x80, 0x80, 0xD8, 0x52, 0x49, 0x47,
  0x00, 0x71, 0x81, 0x13, 0xF4, 0x58, 0x91, 0x3E, 0xB6, 0x7C, 0x88, 0x88, 0x88, 0x88, 0xC3, 0xC2,
  0x31, 0x11, 0x11, 0x13, 0x2C, 0xB3, 0x3F, 0x62, 0x00, 0x21, 0x3E, 0x42, 0x00, 0xB6, 0xE0, 0xB0,
  0x18, 0x8C, 0x42, 0x11, 0x08, 0x44, 0x00, 0x33, 0xE8, 0x61, 0x86, 0x18, 0x63, 0x78, 0x35, 0x59,
  0x11, 0x11, 0x23, 0xE8, 0xC3, 0x09, 0xCC, 0x20, 0xFC, 0x7E, 0x18, 0xE1, 0xE0, 0x78, 0xD1, 0xBE,
  0x1C, 0x68, 0x92, 0x24, 0x5F, 0xC1, 0x02, 0xFA, 0x0B, 0x36, 0x0E, 0x38, 0xBE, 0x33, 0xE8, 0xE0,
  0xFA, 0x38, 0x63, 0x78, 0xFC, 0x10, 0xC2, 0x10, 0xC2, 0x08, 0x10, 0xF9, 0x1A, 0x33, 0xC8, 0xF0,
  0xA3, 0x7C, 0x23, 0xE8, 0xE3, 0x8D, 0xF0, 0xE2, 0xF8, 0xB0, 0xB0, 0xB0, 0xB6, 0x09, 0xF1, 0xC1,
  0x80, 0xF8, 0x3E, 0x83, 0x86, 0x7E, 0x00, 0x11, 0xFC, 0x41, 0x0C, 0x40, 0x04, 0x30, 0x04, 0x0F,
  0xE4, 0x0F, 0x75, 0x93, 0x6C, 0x5B, 0x16, 0x4D, 0xDD, 0xD8, 0x03, 0xF0, 0x38, 0x50, 0xB1, 0x26,
  0x4F, 0xD0, 0xE1, 0xFD, 0x14, 0x5E, 0x4D, 0x14, 0x7F, 0x33, 0xF8, 0x61, 0x82, 0x08, 0x73, 0x78,
  0xFD, 0x14, 0x51, 0x45, 0x14, 0xFE, 0xFC, 0x21, 0xF8, 0x42, 0x1F, 0xFC, 0x21, 0xF8, 0x42, 0x10,
  0x33, 0xF8, 0x61, 0x82, 0x78, 0x73, 0x7C, 0x86, 0x18, 0x7F, 0x86, 0x18, 0x61, 0xFF, 0x1E, 0x18,
  0x30, 0x60, 0xD9, 0x93, 0x3C, 0x8E, 0x4B, 0x38, 0xE2, 0xC9, 0xA3, 0x84, 0x21, 0x08, 0x42, 0x1F,
  0xE3, 0xD1, 0xE9, 0xF4, 0xBB, 0x5C, 0xAE, 0x77, 0x33, 0xE6, 0x9A, 0x6D, 0x96, 0x59, 0x67, 0x33,
  0xF8, 0x61, 0x86, 0x18, 0x73, 0x78, 0xFA, 0x38, 0x63, 0xFA, 0x08, 0x20, 0x33, 0xF8, 0x61, 0x86,
  0x18, 0x73, 0x78, 0xC1, 0x80, 0xFE, 0x18, 0x66, 0xFE, 0x38, 0xE3, 0x10, 0xF9, 0x1A, 0x37, 0x81,
  0xF0, 0xA3, 0x7C, 0xFE, 0x20, 0x40, 0x81, 0x02, 0x04, 0x08, 0x86, 0x18, 0x61, 0x86, 0x1C, 0xDE,
  0xC2, 0x8D, 0x13, 0x22, 0x45, 0x8A, 0x1C, 0xCE, 0x52, 0x94, 0xA5, 0x2B, 0x5A, 0xD4, 0xA7, 0x38,
  0xC6, 0xC6, 0x48, 0xF0, 0xC3, 0x85, 0x99, 0x61, 0xC6, 0xC8, 0xB1, 0xC1, 0x82, 0x04, 0x08, 0xF8,
  0x44, 0x66, 0x62, 0x1F, 0xF2, 0x49, 0x24, 0x92, 0x70, 0x86, 0x10, 0x86, 0x10, 0x86, 0x10, 0xC0,
  0xE4, 0x92, 0x49, 0x24, 0xF0, 0x71, 0x68, 0xC0, 0xFC, 0xC0, 0xF2, 0x67, 0xA2, 0x9B, 0xB0, 0x82,
  0x0B, 0xB3, 0x86, 0x1C, 0xEE, 0x7A, 0x28, 0x60, 0x8D, 0xE0, 0x04, 0x17, 0x63, 0x86, 0x18, 0xDD,
  0x74, 0x7F, 0x08, 0xBC, 0x39, 0x3E, 0x42, 0x10, 0x84, 0x76, 0x38, 0x61, 0x8D, 0xD0, 0x5F, 0x82,
  0x0B, 0xB2, 0x8E, 0x38, 0xE3, 0xBF, 0x45, 0x55, 0x70, 0x84, 0x27, 0x6E, 0x72, 0xD3, 0xFF, 0xF7,
  0x99, 0x89, 0x89, 0x89, 0x89, 0xBB, 0x28, 0xE3, 0x8E, 0x30, 0x7A, 0x28, 0xE3, 0x89, 0xE0, 0xBB,
  0x38, 0x61, 0xCE, 0xE8, 0x20, 0x76, 0x38, 0x61, 0x8D, 0xD0, 0x41, 0xFA, 0x49, 0x00, 0xF4, 0xF8,
  0x78, 0xF8, 0x21, 0x3E, 0x42, 0x10, 0x87, 0x8C, 0x63, 0x18, 0xBC, 0xC5, 0x34, 0x9A, 0x28, 0xC0,
  0x99, 0xB9, 0xAB, 0xAA, 0xA6, 0xE6, 0xC6, 0x58, 0xE1, 0xC2, 0xD8, 0xC0, 0x8C, 0x63, 0x18, 0xBC,
  0x2F, 0xF8, 0xCC, 0x88, 0x7C, 0x39, 0x08, 0x42, 0x73, 0x84, 0x21, 0x08, 0x70, 0xFF, 0xF8, 0xE1,
  0x08, 0x42, 0x1C, 0xE4, 0x21, 0x09, 0xC0, 0xEE, 0xC0,
};
const GFXglyph BoldSmallGlyphs[] PROGMEM = {
  {0, 1, 1, 3, 0, 9},   // 0x20  
  {1, 2, 8, 3, 1, 2},   // 0x21 !
  {3, 4, 4, 6, 1, 1},   // 0x22 "
  {5, 7, 8, 8, 0, 2},   // 0x23 #
  {12, 7, 11, 7, 0, 0},   // 0x24 $
  {22, 9, 9, 9, 0, 1},   // 0x25 %
  {33, 7, 8, 7, 0, 2},   // 0x26 &
  {40, 2, 4, 3, 1, 1},   // 0x27 '
  {41, 4, 12, 5, 1, 1},   // 0x28 (
  {47, 4, 12, 5, 0, 1},   // 0x29 )
  {53, 5, 5, 6, 1, 2},   // 0x2A *
  {57, 5, 5, 7, 1, 3},   // 0x2B +
  {61, 2, 4, 3, 1, 8},   // 0x2C ,
  {62, 3, 1, 5, 1, 6},   // 0x2D -
  {63, 2, 2, 3, 1, 8},   // 0x2E .
  {64, 5, 10, 5, 0, 2},   // 0x2F /
  {71, 6, 9, 8, 1, 1},   // 0x30 0
  {78, 4, 8, 5, 0, 2},   // 0x31 1
  {82, 6, 9, 7, 1, 1},   // 0x32 2
  {89, 7, 8, 7, 0, 2},   // 0x33 3
  {96, 7, 8, 8, 0, 2},   // 0x34 4
  {103, 6, 8, 7, 1, 2},   // 0x35 5
  {109, 6, 9, 7, 1, 1},   // 0x36 6
  {116, 6, 8, 7, 0, 2},   // 0x37 7
  {122, 7, 9, 7, 0, 1},   // 0x38 8
  {130, 6, 9, 7, 1, 1},   // 0x39 9
  {137, 2, 6, 3, 1, 4},   // 0x3A :
  {139, 2, 8, 3, 1, 4},   // 0x3B ;
  {141, 5, 5, 7, 1, 3},   // 0x3C <
  {145, 5, 3, 7, 1, 4},   // 0x3D =
  {147, 5, 5, 7, 1, 3},   // 0x3E >
  {151, 6, 9, 7, 0, 1},   // 0x3F ?
  {158, 10, 11, 12, 1, 1},   // 0x40 @
  {172, 7, 8, 8, 0, 2},   // 0x41 A
  {179, 6, 8, 8, 1, 2},   // 0x42 B
  {185, 6, 9, 8, 1, 1},   // 0x43 C
  {192, 6, 8, 8, 1, 2},   // 0x44 D
  {198, 5, 8, 7, 1, 2},   // 0x45 E
  {203, 5, 8, 6, 1, 2},   // 0x46 F
  {208, 6, 9, 8, 1, 1},   // 0x47 G
  {215, 6, 8, 8, 1, 2},   // 0x48 H
  {221, 1, 8, 3, 1, 2},   // 0x49 I
  {222, 7, 8, 7, 0, 2},   // 0x4A J
  {229, 6, 8, 7, 1, 2},   // 0x4B K
  {235, 5, 8, 7, 1, 2},   // 0x4C L
  {240, 9, 8, 10, 1, 2},   // 0x4D M
  {249, 6, 8, 8, 1, 2},   // 0x4E N
  {255, 6, 9, 8, 1, 1},   // 0x4F O
  {262, 6, 8, 7, 1, 2},   // 0x50 P
  {268, 6, 11, 8, 1, 1},   // 0x51 Q
  {277, 6, 8, 8, 1, 2},   // 0x52 R
  {283, 7, 9, 7, 0, 1},   // 0x53 S
  {291, 7, 8, 7, 0, 2},   // 0x54 T
  {298, 6, 8, 8, 1, 2},   // 0x55 U
  {304, 7, 8, 7, 0, 2},   // 0x56 V
  {311, 10, 8, 11, 0, 2},   // 0x57 W
  {321, 7, 8, 8, 0, 2},   // 0x58 X
  {328, 7, 8, 7, 0, 2},   // 0x59 Y
  {335, 5, 8, 7, 1, 2},   // 0x5A Z
  {340, 3, 12, 4, 1, 1},   // 0x5B [
  {345, 5, 10, 5, 0, 2},   // 0x5C backslash
  {352, 3, 12, 4, 0, 1},   // 0x5D ]
  {357, 6, 3, 7, 1, 2},   // 0x5E ^
  {360, 6, 1, 7, 1, 11},   // 0x5F _
  {361, 2, 1, 3, 1, 2},   // 0x60 `
  {362, 6, 6, 7, 1, 4},   // 0x61 a
  {367, 6, 8, 8, 1, 2},   // 0x62 b
  {373, 6, 6, 7, 1, 4},   // 0x63 c
  {378, 6, 8, 8, 1, 2},   // 0x64 d
  {384, 5, 6, 7, 1, 4},   // 0x65 e
  {388, 5, 8, 5, 0, 2},   // 0x66 f
  {393, 6, 8, 8, 1, 4},   // 0x67 g
  {399, 6, 8, 7, 1, 2},   // 0x68 h
  {405, 1, 8, 3, 1, 2},   // 0x69 i
  {406, 2, 10, 3, 0, 2},   // 0x6A j
  {409, 5, 8, 7, 1, 2},   // 0x6B k
  {414, 1, 8, 3, 1, 2},   // 0x6C l
  {415, 8, 6, 10, 1, 4},   // 0x6D m
  {421, 6, 6, 7, 1, 4},   // 0x6E n
  {426, 6, 6, 7, 1, 4},   // 0x6F o
  {431, 6, 8, 8, 1, 4},   // 0x70 p
  {437, 6, 8, 8, 1, 4},   // 0x71 q
  {443, 3, 6, 5, 1, 4},   // 0x72 r
  {446, 5, 6, 6, 1, 4},   // 0x73 s
  {450, 5, 8, 6, 0, 2},   // 0x74 t
  {455, 5, 6, 7, 1, 4},   // 0x75 u
  {459, 6, 6, 7, 0, 4},   // 0x76 v
  {464, 8, 6, 10, 1, 4},   // 0x77 w
  {470, 7, 6, 7, 0, 4},   // 0x78 x
  {476, 5, 8, 7, 1, 4},   // 0x79 y
  {481, 5, 6, 6, 1, 4},   // 0x7A z
  {485, 5, 12, 5, 0, 1},   // 0x7B {
  {493, 1, 13, 3, 1, 0},   // 0x7C |
  {495, 5, 12, 5, 0, 1},   // 0x7D }
  {503, 6, 2, 7, 1, 5},   // 0x7E ~
};
const GFXfont BoldSmall PROGMEM = {(uint8_t*)BoldSmallBitmaps, (GFXglyph*)BoldSmallGlyphs, 0x20, 0x7E, 15};

// BoldLabel: generated from sg-700.ttf at 16px
const uint8_t BoldLabelBitmaps[] PROGMEM = {
  0x00, 0xDB, 0x6D, 0xB0, 0xDF, 0x80, 0xCF, 0xFE, 0xC9, 0xEC, 0x66, 0x33, 0x19, 0x9F, 0xFF, 0xFB,
  0x33, 0xFF, 0xFF, 0x66, 0x33, 0x19, 0x80, 0x18, 0x18, 0x7E, 0xFF, 0xDB, 0xDB, 0xF8, 0x7E, 0x1F,
  0xDB, 0xDB, 0xFF, 0x7E, 0x18, 0x18, 0x78, 0x6C, 0xCC, 0xCD, 0x87, 0x98, 0x33, 0x00, 0x60, 0x0C,
  0xC1, 0x9E, 0x19, 0x33, 0x13, 0x61, 0xE0, 0x3C, 0x38, 0x18, 0x0C, 0x67, 0xF9, 0xFD, 0x8D, 0x86,
  0xE3, 0x3F, 0x8F, 0xC0, 0xDF, 0x9E, 0x3B, 0xF9, 0x8C, 0x63, 0x18, 0xC6, 0x31, 0x8C, 0x71, 0xE3,
  0xE7, 0x86, 0x31, 0x8C, 0x63, 0x18, 0xC6, 0x31, 0x9F, 0xDC, 0x55, 0xB8, 0xE7, 0xF7, 0x9A, 0x84,
  0x00, 0x30, 0xCF, 0xFF, 0x30, 0xC3, 0x00, 0xDF, 0x9E, 0xFF, 0xC0, 0xDF, 0x80, 0x0C, 0x30, 0xC6,
  0x18, 0x63, 0x0C, 0x31, 0x86, 0x18, 0xC3, 0x00, 0x7E, 0x73, 0xB0, 0xD8, 0x7C, 0x1E, 0x0F, 0x07,
  0x87, 0xC3, 0x7F, 0x9F, 0x80, 0x3C, 0xF6, 0xDB, 0xCC, 0x30, 0xC3, 0x0C, 0x30, 0xC0, 0x7E, 0xE7,
  0xC3, 0xC3, 0x07, 0x1E, 0x7C, 0xE0, 0xC0, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x3C, 0x30, 0x3E, 0x03,
  0xC3, 0xC3, 0xFF, 0x7E, 0x0F, 0x06, 0xC1, 0xB0, 0xCC, 0x33, 0x18, 0xCC, 0x33, 0xFF, 0xFF, 0xC0,
  0xC0, 0x30, 0xFF, 0xFF, 0xC0, 0xC0, 0xFE, 0xE7, 0x03, 0xC3, 0xC3, 0xFF, 0x7E, 0x7E, 0xE7, 0xC3,
  0xC0, 0xFE, 0xE7, 0xC3, 0xC3, 0xC3, 0xFF, 0x7E, 0xFF, 0xFF, 0x03, 0x07, 0x06, 0x0E, 0x0C, 0x1C,
  0x18, 0x30, 0x30, 0x3F, 0x39, 0xD8, 0x6C, 0x37, 0xF1, 0xF9, 0x8F, 0xC3, 0xE1, 0xBF, 0xCF, 0xC0,
  0x7E, 0xE7, 0xC3, 0xC3, 0xC3, 0xFF, 0x7B, 0x03, 0xC3, 0xFE, 0x7E, 0xDF, 0x81, 0xBF, 0xDF, 0x81,
  0xBF, 0x3C, 0x0C, 0xFF, 0x30, 0xF8, 0xF0, 0x40, 0xFF, 0xF0, 0x3F, 0xFC, 0x83, 0xC3, 0xC3, 0x7F,
  0xC8, 0x00, 0x7C, 0xEE, 0xC7, 0x07, 0x0E, 0x1C, 0x38, 0x00, 0x10, 0x38, 0x38, 0x1F, 0xE0, 0xF1,
  0xC7, 0x01, 0x99, 0xDB, 0xCF, 0xEF, 0x31, 0xBD, 0x86, 0xF6, 0x1B, 0xCC, 0x6F, 0x3F, 0xF6, 0x77,
  0x9C, 0x00, 0x3F, 0xE0, 0x3F, 0x80, 0x1E, 0x07, 0x81, 0xB0, 0xCC, 0x33, 0x0C, 0xC7, 0x19, 0xFE,
  0x7F, 0x98, 0x6E, 0x0C, 0xFF, 0x7F, 0xD8, 0x6C, 0x37, 0xF3, 0xF1, 0x8E, 0xC3, 0x61, 0xFF, 0xFF,
  0xC0, 0x7E, 0x7B, 0xB0, 0xD8, 0x3C, 0x06, 0x03, 0x01, 0x83, 0xC3, 0x3F, 0x8F, 0x80, 0xFE, 0x7F,
  0x98, 0xEC, 0x36, 0x1B, 0x0D, 0x86, 0xC3, 0x63, 0xFF, 0xBF, 0x80, 0xFF, 0xFF, 0x06, 0x0F, 0xFF,
  0xF0, 0x60, 0xC1, 0xFF, 0xF8, 0xFF, 0xFF, 0x06, 0x0F, 0xFF, 0xF0, 0x60, 0xC1, 0x83, 0x00, 0x7E,
  0x7B, 0xB0, 0xF8, 0x3C, 0x06, 0x7F, 0x3F, 0x83, 0xC3, 0xBF, 0xCF, 0x60, 0xC3, 0xC3, 0xC3, 0xC3,
  0xFF, 0xFF, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xFF, 0xFF, 0xFC, 0x1F, 0x1F, 0x06, 0x06, 0x06, 0x06,
  0x06, 0xC6, 0xC6, 0xFC, 0x7C, 0xC3, 0xE3, 0x33, 0x9B, 0x8F, 0x87, 0x83, 0xE1, 0xB8, 0xCE, 0x63,
  0xB0, 0xE0, 0xC1, 0x83, 0x06, 0x0C, 0x18, 0x30, 0x60, 0xC1, 0xFF, 0xF8, 0xF0, 0xFF, 0x0F, 0xD8,
  0xBD, 0x9B, 0xD9, 0xBD, 0x9B, 0xD9, 0xBC, 0x93, 0xCF, 0x3C, 0xF3, 0xCF, 0x30, 0xF1, 0xF8, 0xF6,
  0x7B, 0x3D, 0x9E, 0xCF, 0x37, 0x9B, 0xCD, 0xE6, 0xF1, 0xE0, 0x7E, 0x7B, 0xB0, 0x78, 0x3C, 0x1E,
  0x0F, 0x07, 0x83, 0xC3, 0xBF, 0x8F, 0x80, 0xFE, 0xFF, 0xC3, 0xC3, 0xC3, 0xFF, 0xFC, 0xC0, 0xC0,
  0xC0, 0xC0, 0x7E, 0x7B, 0xB0, 0x78, 0x3C, 0x1E, 0x0F, 0x07, 0x83, 0xC3, 0xBF, 0x8F, 0x83, 0x01,
  0xE0, 0x70, 0xFE, 0xFF, 0xC3, 0xC3, 0xC3, 0xFE, 0xFF, 0xC3, 0xC3, 0xC3, 0xC3, 0x7E, 0xEF, 0xC3,
  0xC3, 0xF8, 0x7E, 0x07, 0xC3, 0xC3, 0xFF, 0x7E, 0xFF, 0xFF, 0xC3, 0x01, 0x80, 0xC0, 0x60, 0x30,
  0x18, 0x0C, 0x06, 0x03, 0x00, 0xC1, 0xE0, 0xF0, 0x78, 0x3C, 0x1E, 0x0F, 0x07, 0x83, 0xC3, 0xBF,
  0x8F, 0x80, 0xE1, 0xD8, 0x66, 0x19, 0x86, 0x73, 0x0C, 0xC3, 0x30, 0xCC, 0x1E, 0x07, 0x81, 0xE0,
  0xE7, 0x8D, 0x9E, 0x36, 0x69, 0xD9, 0xB6, 0x66, 0xD9, 0x93, 0x66, 0xCD, 0x9B, 0x36, 0x3C, 0xD8,
  0xF1, 0x63, 0xC7, 0x00, 0xE1, 0xDC, 0x63, 0x30, 0x7C, 0x1E, 0x03, 0x81, 0xE0, 0xEC, 0x33, 0x1C,
  0x6E, 0x1C, 0xE1, 0xD8, 0x63, 0x30, 0xCC, 0x1E, 0x07, 0x80, 0xC0, 0x30, 0x0C, 0x03, 0x00, 0xC0,
  0xFE, 0xFE, 0x06, 0x0E, 0x1C, 0x38, 0x70, 0xE0, 0xC0, 0xFF, 0xFF, 0xFF, 0xCC, 0xCC, 0xCC, 0xCC,
  0xCC, 0xCC, 0xF7, 0xC3, 0x06, 0x18, 0x60, 0xC3, 0x0C, 0x18, 0x61, 0x83, 0x0C, 0x30, 0xF7, 0xC6,
  0x31, 0x8C, 0x63, 0x18, 0xC6, 0x31, 0x8F, 0xFE, 0x3C, 0x3C, 0x66, 0xE7, 0xFF, 0xFF, 0xCC, 0x7C,
  0xEE, 0x46, 0x7E, 0xFE, 0xC6, 0xCF, 0xFF, 0xC0, 0xC0, 0xC0, 0xDE, 0xFF, 0xC3, 0xC3, 0xC3, 0xC3,
  0xFF, 0xDE, 0x7C, 0xFE, 0xC3, 0xC0, 0xC0, 0xC3, 0xFE, 0x7C, 0x03, 0x03, 0x03, 0x7B, 0xFF, 0xC3,
  0xC3, 0xC3, 0xC3, 0xFF, 0x7B, 0x7C, 0xFE, 0xC6, 0xFF, 0xFF, 0xC4, 0xFE, 0x7C, 0x1C, 0x70, 0xC7,
  0xFF, 0xE6, 0x0C, 0x18, 0x30, 0x60, 0xC0, 0x7B, 0xFF, 0xC3, 0xC3, 0xC3, 0xC3, 0xFF, 0x7B, 0x03,
  0x7F, 0x7F, 0xC0, 0xC0, 0xC0, 0xDE, 0xFE, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xF3, 0xFF, 0xFC,
  0x6C, 0x36, 0xDB, 0x6D, 0xBF, 0xC0, 0xC0, 0xC0, 0xC0, 0xC7, 0xCC, 0xD8, 0xF0, 0xF8, 0xD8, 0xCC,
  0xC7, 0xFF, 0xFF, 0xFC, 0xDD, 0xEF, 0xFE, 0xC6, 0x3C, 0x63, 0xC6, 0x3C, 0x63, 0xC6, 0x3C, 0x63,
  0xDE, 0xFE, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0x7C, 0xFE, 0xC3, 0xC3, 0xC3, 0xC3, 0xFE, 0x7C,
  0xDE, 0xFF, 0xC3, 0xC3, 0xC3, 0xC3, 0xFF, 0xDE, 0xC0, 0xC0, 0xC0, 0x7B, 0xFF, 0xC3, 0xC3, 0xC3,
  0xC3, 0xFF, 0x7B, 0x03, 0x03, 0x03, 0xDF, 0xF1, 0x8C, 0x63, 0x18, 0x79, 0xDB, 0x27, 0xC3, 0xD8,
  0xFB, 0xBE, 0x30, 0x63, 0xFF, 0xF3, 0x06, 0x0C, 0x18, 0x3C, 0x38, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3,
  0xC7, 0xFF, 0x7B, 0xE3, 0xB1, 0x98, 0xCC, 0xC3, 0x61, 0xB0, 0xD0, 0x38, 0xCE, 0x7B, 0xCF, 0x69,
  0xED, 0x2D, 0xBD, 0xA7, 0x94, 0xF3, 0x9E, 0xE3, 0xB9, 0x8D, 0x83, 0xC1, 0xE1, 0xB1, 0xCD, 0xC7,
  0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC7, 0xFF, 0x7B, 0x03, 0x7F, 0x7E, 0xFD, 0xF8, 0x31, 0xC7, 0x1C,
  0x3F, 0xFF, 0x3C, 0xF3, 0x0C, 0x30, 0xC3, 0x38, 0xF0, 0xC3, 0x0C, 0x30, 0xC3, 0xCF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xC0, 0xF1, 0xF0, 0x60, 0xC1, 0x83, 0x06, 0x0F, 0x1E, 0x30, 0x60, 0xC1, 0x83, 0x3E,
  0x78, 0x7B, 0xFF, 0xCC,
};
const GFXglyph BoldLabelGlyphs[] PROGMEM = {
  {0, 1, 1, 4, 0, 0},   // 0x20  
  {1, 3, 11, 5, 1, -10},   // 0x21 !
  {6, 6, 5, 8, 1, -11},   // 0x22 "
  {10, 9, 11, 10, 1, -10},   // 0x23 #
  {23, 8, 15, 10, 1, -12},   // 0x24 $
  {38, 12, 11, 12, 0, -10},   // 0x25 %
  {55, 9, 11, 9, 0, -10},   // 0x26 &
  {68, 3, 5, 5, 1, -11},   // 0x27 '
  {70, 5, 16, 6, 1, -11},   // 0x28 (
  {80, 5, 16, 6, 0, -11},   // 0x29 )
  {90, 7, 7, 9, 1, -10},   // 0x2A *
  {97, 6, 7, 10, 2, -8},   // 0x2B +
  {103, 3, 5, 5, 1, -2},   // 0x2C ,
  {105, 5, 2, 7, 1, -4},   // 0x2D -
  {107, 3, 3, 5, 1, -2},   // 0x2E .
  {109, 6, 14, 6, 0, -10},   // 0x2F /
  {120, 9, 11, 10, 1, -10},   // 0x30 0
  {133, 6, 11, 7, 0, -10},   // 0x31 1
  {142, 8, 11, 10, 1, -10},   // 0x32 2
  {153, 8, 11, 10, 1, -10},   // 0x33 3
  {164, 10, 11, 10, 0, -10},   // 0x34 4
  {178, 8, 11, 10, 1, -10},   // 0x35 5
  {189, 8, 11, 10, 1, -10},   // 0x36 6
  {200, 8, 11, 9, 0, -10},   // 0x37 7
  {211, 9, 11, 10, 0, -10},   // 0x38 8
  {224, 8, 11, 10, 1, -10},   // 0x39 9
  {235, 3, 8, 5, 1, -7},   // 0x3A :
  {238, 3, 10, 5, 1, -7},   // 0x3B ;
  {242, 6, 7, 10, 2, -8},   // 0x3C <
  {248, 6, 5, 10, 2, -7},   // 0x3D =
  {252, 6, 7, 10, 2, -8},   // 0x3E >
  {258, 8, 11, 9, 1, -10},   // 0x3F ?
  {269, 14, 14, 16, 1, -10},   // 0x40 @
  {294, 10, 11, 10, 0, -10},   // 0x41 A
  {308, 9, 11, 11, 1, -10},   // 0x42 B
  {321, 9, 11, 10, 1, -10},   // 0x43 C
  {334, 9, 11, 11, 1, -10},   // 0x44 D
  {347, 7, 11, 9, 1, -10},   // 0x45 E
  {357, 7, 11, 9, 1, -10},   // 0x46 F
  {367, 9, 11, 11, 1, -10},   // 0x47 G
  {380, 8, 11, 11, 1, -10},   // 0x48 H
  {391, 2, 11, 4, 1, -10},   // 0x49 I
  {394, 8, 11, 10, 1, -10},   // 0x4A J
  {405, 9, 11, 10, 1, -10},   // 0x4B K
  {418, 7, 11, 9, 1, -10},   // 0x4C L
  {428, 12, 11, 14, 1, -10},   // 0x4D M
  {445, 9, 11, 11, 1, -10},   // 0x4E N
  {458, 9, 11, 11, 1, -10},   // 0x4F O
  {471, 8, 11, 10, 1, -10},   // 0x50 P
  {482, 9, 14, 11, 1, -10},   // 0x51 Q
  {498, 8, 11, 10, 1, -10},   // 0x52 R
  {509, 8, 11, 10, 1, -10},   // 0x53 S
  {520, 9, 11, 9, 0, -10},   // 0x54 T
  {533, 9, 11, 11, 1, -10},   // 0x55 U
  {546, 10, 11, 10, 0, -10},   // 0x56 V
  {560, 14, 11, 14, 0, -10},   // 0x57 W
  {580, 10, 11, 10, 0, -10},   // 0x58 X
  {594, 10, 11, 10, 0, -10},   // 0x59 Y
  {608, 8, 11, 9, 1, -10},   // 0x5A Z
  {619, 4, 16, 6, 1, -11},   // 0x5B [
  {627, 6, 14, 6, 0, -10},   // 0x5C backslash
  {638, 5, 16, 6, 0, -11},   // 0x5D ]
  {648, 8, 4, 10, 1, -10},   // 0x5E ^
  {652, 8, 2, 10, 1, 2},   // 0x5F _
  {654, 3, 2, 5, 1, -10},   // 0x60 `
  {655, 8, 8, 9, 1, -7},   // 0x61 a
  {663, 8, 11, 10, 1, -10},   // 0x62 b
  {674, 8, 8, 9, 1, -7},   // 0x63 c
  {682, 8, 11, 10, 1, -10},   // 0x64 d
  {693, 8, 8, 9, 1, -7},   // 0x65 e
  {701, 7, 11, 7, 0, -10},   // 0x66 f
  {711, 8, 11, 10, 1, -7},   // 0x67 g
  {722, 8, 11, 10, 1, -10},   // 0x68 h
  {733, 2, 11, 4, 1, -10},   // 0x69 i
  {736, 3, 14, 4, 0, -10},   // 0x6A j
  {742, 8, 11, 9, 1, -10},   // 0x6B k
  {753, 2, 11, 4, 1, -10},   // 0x6C l
  {756, 12, 8, 14, 1, -7},   // 0x6D m
  {768, 8, 8, 10, 1, -7},   // 0x6E n
  {776, 8, 8, 10, 1, -7},   // 0x6F o
  {784, 8, 11, 10, 1, -7},   // 0x70 p
  {795, 8, 11, 10, 1, -7},   // 0x71 q
  {806, 5, 8, 6, 1, -7},   // 0x72 r
  {811, 7, 8, 8, 1, -7},   // 0x73 s
  {818, 7, 10, 7, 0, -9},   // 0x74 t
  {827, 8, 8, 10, 1, -7},   // 0x75 u
  {835, 9, 8, 9, 0, -7},   // 0x76 v
  {844, 11, 8, 13, 1, -7},   // 0x77 w
  {855, 9, 8, 9, 0, -7},   // 0x78 x
  {864, 8, 11, 10, 1, -7},   // 0x79 y
  {875, 7, 8, 8, 1, -7},   // 0x7A z
  {882, 6, 16, 7, 1, -11},   // 0x7B {
  {894, 2, 17, 4, 1, -12},   // 0x7C |
  {899, 7, 16, 7, 0, -11},   // 0x7D }
  {913, 8, 3, 10, 1, -6},   // 0x7E ~
};
const GFXfont BoldLabel PROGMEM = {(uint8_t*)BoldLabelBitmaps, (GFXglyph*)BoldLabelGlyphs, 0x20, 0x7E, 20};

// BoldReadout: generated from sg-700.ttf at 44px
const uint8_t BoldReadoutBitmaps[] PROGMEM = {
  0x00, 0x3E, 0x1F, 0x0F, 0x87, 0xC3, 0xE1, 0xF0, 0xF8, 0x7C, 0x3E, 0x1F, 0x0F, 0x87, 0xC3, 0xE1,
  0xF0, 0xF8, 0x7C, 0x3E, 0x1F, 0x0F, 0x87, 0xC3, 0xE0, 0x00, 0x00, 0x38, 0x7F, 0x3F, 0xBF, 0xFF,
  0xFF, 0xFB, 0xF8, 0xFC, 0x30, 0x08, 0x06, 0x1F, 0x87, 0xE7, 0xF3, 0xFF, 0xFC, 0xFF, 0xFF, 0xBF,
  0xDF, 0xEF, 0xF7, 0xF9, 0xFC, 0xFE, 0x3F, 0x07, 0x81, 0xC1, 0xE0, 0x70, 0x78, 0x1D, 0xFC, 0x7F,
  0x7F, 0x1F, 0xDF, 0x87, 0xE0, 0x07, 0xC3, 0xE0, 0x07, 0xC3, 0xE0, 0x07, 0xC3, 0xE0, 0x07, 0xC3,
  0xE0, 0x07, 0xC3, 0xE0, 0x07, 0xC3, 0xE0, 0x07, 0xC3, 0xE0, 0x07, 0xC3, 0xE0, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x07, 0xC3, 0xE0, 0x07,
  0xC3, 0xE0, 0x07, 0xC3, 0xE0, 0x07, 0xC3, 0xE0, 0x07, 0xC3, 0xE0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x07, 0xC3, 0xE0, 0x07, 0xC3, 0xE0,
  0x07, 0xC3, 0xE0, 0x07, 0xC3, 0xE0, 0x07, 0xC3, 0xE0, 0x07, 0xC3, 0xE0, 0x07, 0xC3, 0xE0, 0x07,
  0xC3, 0xE0, 0x00, 0x3E, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x3E,
  0x00, 0x01, 0xFF, 0xC0, 0x07, 0xFF, 0xF0, 0x0F, 0xFF, 0xF8, 0x1F, 0xFF, 0xFC, 0x3F, 0xFF, 0xFC,
  0x3F, 0xBE, 0xFE, 0x7F, 0x3E, 0x7E, 0x7E, 0x3E, 0x7E, 0x7E, 0x3E, 0x3E, 0x7E, 0x3E, 0x3F, 0x7F,
  0x3E, 0x3F, 0x3F, 0xBE, 0x00, 0x3F, 0xFE, 0x00, 0x1F, 0xFF, 0x00, 0x0F, 0xFF, 0xE0, 0x07, 0xFF,
  0xF8, 0x01, 0xFF, 0xFC, 0x00, 0x3F, 0xFE, 0x00, 0x3F, 0xFF, 0x00, 0x3E, 0x7F, 0xFC, 0x3E, 0x3F,
  0x7C, 0x3E, 0x3F, 0x7C, 0x3E, 0x1F, 0x7E, 0x3E, 0x3F, 0x7E, 0x3E, 0x3F, 0x7F, 0x3E, 0x7F, 0x3F,
  0xFF, 0xFE, 0x1F, 0xFF, 0xFC, 0x0F, 0xFF, 0xFC, 0x07, 0xFF, 0xF0, 0x01, 0xFF, 0xE0, 0x00, 0x3E,
  0x00, 0x00, 0x3E, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x3E, 0x00, 0x0F, 0xE0, 0x00,
  0x60, 0x7F, 0xE0, 0x01, 0xE1, 0xFF, 0xE0, 0x07, 0xE3, 0xF7, 0xE0, 0x0F, 0x8F, 0x83, 0xC0, 0x3E,
  0x1F, 0x07, 0x80, 0xFC, 0x3E, 0x0F, 0x03, 0xF0, 0x7C, 0x1E, 0x0F, 0xC0, 0xF8, 0x3C, 0x1F, 0x00,
  0xF8, 0xF8, 0x7C, 0x01, 0xFF, 0xE1, 0xF8, 0x01, 0xFF, 0xC7, 0xE0, 0x01, 0xFE, 0x1F, 0x80, 0x00,
  0x00, 0x3E, 0x00, 0x00, 0x00, 0xF8, 0x00, 0x00, 0x03, 0xF0, 0x00, 0x00, 0x0F, 0xC0, 0x00, 0x00,
  0x3F, 0x00, 0x00, 0x00, 0x7C, 0x3F, 0xC0, 0x01, 0xF0, 0xFF, 0xC0, 0x07, 0xC3, 0xFF, 0xC0, 0x1F,
  0x8F, 0xEF, 0xC0, 0x7E, 0x1F, 0x0F, 0x80, 0xF8, 0x3C, 0x0F, 0x03, 0xE0, 0x78, 0x1E, 0x0F, 0x80,
  0xF0, 0x3C, 0x3F, 0x01, 0xF0, 0xF8, 0xFC, 0x03, 0xF3, 0xF3, 0xF0, 0x03, 0xFF, 0xC3, 0xC0, 0x03,
  0xFF, 0x03, 0x00, 0x03, 0xFC, 0x02, 0x00, 0x00, 0x00, 0x01, 0xFC, 0x00, 0x0F, 0xFC, 0x00, 0x1F,
  0xFC, 0x00, 0x3F, 0xFC, 0x00, 0x7F, 0xFC, 0x00, 0x7F, 0x00, 0x00, 0x7E, 0x00, 0x00, 0xFC, 0x00,
  0x00, 0xFC, 0x00, 0x00, 0xFC, 0x01, 0xF0, 0x7C, 0x01, 0xF0, 0x7E, 0x01, 0xF0, 0x3F, 0x01, 0xF0,
  0x1F, 0xFF, 0xFF, 0x0F, 0xFF, 0xFF, 0x07, 0xFF, 0xFF, 0x0F, 0xFF, 0xFF, 0x3F, 0xFF, 0xFF, 0x7F,
  0x01, 0xF0, 0x7C, 0x01, 0xF0, 0xFC, 0x01, 0xF0, 0xFC, 0x01, 0xF0, 0xFC, 0x01, 0xF0, 0xFC, 0x01,
  0xF0, 0xFC, 0x01, 0xF0, 0x7E, 0x01, 0xF0, 0x7F, 0xFF, 0xF0, 0x3F, 0xFF, 0xF0, 0x3F, 0xFF, 0xF0,
  0x0F, 0xFF, 0xF0, 0x03, 0xFF, 0xF0, 0x08, 0x3F, 0x1F, 0xDF, 0xEF, 0xFB, 0xFD, 0xFE, 0x7F, 0x07,
  0x83, 0xC1, 0xEF, 0xE7, 0xF3, 0xF0, 0x00, 0x7E, 0x03, 0xFC, 0x1F, 0xF8, 0x7F, 0xF1, 0xFF, 0xE3,
  0xF8, 0x0F, 0xE0, 0x1F, 0x80, 0x7E, 0x00, 0xFC, 0x01, 0xF0, 0x07, 0xE0, 0x0F, 0xC0, 0x1F, 0x80,
  0x3F, 0x00, 0x7E, 0x00, 0xFC, 0x01, 0xF8, 0x03, 0xF0, 0x07, 0xE0, 0x0F, 0xC0, 0x1F, 0x80, 0x3F,
  0x00, 0x7E, 0x00, 0xFC, 0x01, 0xF8, 0x03, 0xF0, 0x07, 0xE0, 0x0F, 0xC0, 0x1F, 0x80, 0x3F, 0x00,
  0x7E, 0x00, 0x7C, 0x00, 0xF8, 0x01, 0xF8, 0x03, 0xF0, 0x03, 0xF0, 0x07, 0xF0, 0x07, 0xF8, 0x07,
  0xFF, 0x07, 0xFE, 0x07, 0xFC, 0x03, 0xF8, 0x00, 0x70, 0xFC, 0x01, 0xFE, 0x03, 0xFF, 0x07, 0xFF,
  0x0F, 0xFF, 0x00, 0xFF, 0x00, 0xFE, 0x00, 0xFE, 0x00, 0xFC, 0x00, 0xF8, 0x01, 0xF0, 0x03, 0xF0,
  0x07, 0xE0, 0x0F, 0xC0, 0x0F, 0x80, 0x1F, 0x00, 0x3E, 0x00, 0x7C, 0x00, 0xF8, 0x01, 0xF0, 0x03,
  0xE0, 0x07, 0xC0, 0x0F, 0x80, 0x1F, 0x00, 0x3E, 0x00, 0x7C, 0x00, 0xF8, 0x01, 0xF0, 0x03, 0xE0,
  0x0F, 0xC0, 0x1F, 0x80, 0x3F, 0x00, 0x7E, 0x00, 0xF8, 0x03, 0xF0, 0x07, 0xE0, 0x1F, 0x80, 0x7F,
  0x03, 0xFC, 0x7F, 0xF8, 0xFF, 0xE1, 0xFF, 0x03, 0xF8, 0x07, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x0F,
  0x00, 0x18, 0xF1, 0x03, 0xCF, 0x38, 0x3E, 0xE7, 0xC1, 0xE6, 0x78, 0x0F, 0x6F, 0x00, 0x7F, 0xE0,
  0x01, 0xF8, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x9F, 0x9F, 0x03, 0xFC, 0x00, 0xF6, 0xF0, 0x1E,
  0x6F, 0x83, 0xEE, 0x7C, 0x3C, 0xE3, 0xC1, 0x8F, 0x18, 0x00, 0xF0, 0x00, 0x0F, 0x00, 0x01, 0xF0,
  0x00, 0x3E, 0x00, 0x07, 0xC0, 0x00, 0xF8, 0x00, 0x1F, 0x00, 0x03, 0xE0, 0x00, 0x7C, 0x07, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x1F, 0x00, 0x03, 0xE0, 0x00,
  0x7C, 0x00, 0x0F, 0x80, 0x01, 0xF0, 0x00, 0x3E, 0x00, 0x07, 0xC0, 0x00, 0x08, 0x3F, 0x1F, 0xDF,
  0xEF, 0xFB, 0xFD, 0xFE, 0x7F, 0x07, 0x83, 0xC1, 0xEF, 0xE7, 0xF3, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0x80, 0x1C, 0x3F, 0x9F, 0xDF, 0xFF, 0xFF, 0xFD, 0xFC, 0x7E, 0x18, 0x00,
  0x00, 0x07, 0xE0, 0x00, 0xF8, 0x00, 0x3F, 0x00, 0x07, 0xE0, 0x00, 0xF8, 0x00, 0x3F, 0x00, 0x07,
  0xE0, 0x00, 0xF8, 0x00, 0x1F, 0x00, 0x07, 0xE0, 0x00, 0xF8, 0x00, 0x1F, 0x00, 0x07, 0xE0, 0x00,
  0xF8, 0x00, 0x1F, 0x00, 0x07, 0xE0, 0x00, 0xF8, 0x00, 0x1F, 0x00, 0x07, 0xE0, 0x00, 0xF8, 0x00,
  0x1F, 0x00, 0x07, 0xE0, 0x00, 0xFC, 0x00, 0x1F, 0x00, 0x07, 0xE0, 0x00, 0xFC, 0x00, 0x1F, 0x00,
  0x07, 0xE0, 0x00, 0xFC, 0x00, 0x1F, 0x00, 0x07, 0xE0, 0x00, 0xFC, 0x00, 0x1F, 0x00, 0x07, 0xE0,
  0x00, 0xFC, 0x00, 0x1F, 0x00, 0x07, 0xE0, 0x00, 0xFC, 0x00, 0x1F, 0x00, 0x07, 0xE0, 0x00, 0x01,
  0xFF, 0xC0, 0x07, 0xFF, 0xF0, 0x0F, 0xFF, 0xF8, 0x1F, 0xFF, 0xFC, 0x3F, 0xFF, 0xFE, 0x7F, 0x80,
  0xFE, 0x7E, 0x00, 0x7E, 0x7E, 0x00, 0x3F, 0xFC, 0x00, 0x3F, 0xFC, 0x00, 0x3F, 0xFC, 0x00, 0x1F,
  0xFC, 0x00, 0x1F, 0xFC, 0x00, 0x1F, 0xFC, 0x00, 0x1F, 0xFC, 0x00, 0x1F, 0xFC, 0x00, 0x1F, 0xFC,
  0x00, 0x1F, 0xFC, 0x00, 0x1F, 0xFC, 0x00, 0x1F, 0xFC, 0x00, 0x1F, 0xFC, 0x00, 0x1F, 0xFC, 0x00,
  0x1F, 0xFC, 0x00, 0x3F, 0x7E, 0x00, 0x3F, 0x7E, 0x00, 0x7F, 0x7F, 0x00, 0xFE, 0x3F, 0xF7, 0xFE,
  0x3F, 0xFF, 0xFC, 0x1F, 0xFF, 0xF8, 0x07, 0xFF, 0xF0, 0x03, 0xFF, 0xC0, 0x00, 0x1C, 0x00, 0x03,
  0xFF, 0x07, 0xFF, 0x07, 0xFF, 0x0F, 0xFF, 0x0F, 0xFF, 0x1F, 0xBF, 0x1F, 0xBF, 0x1F, 0x3F, 0x3F,
  0x3F, 0x3E, 0x3F, 0x7E, 0x3F, 0x7E, 0x3F, 0xFC, 0x3F, 0xFC, 0x3F, 0x00, 0x3F, 0x00, 0x3F, 0x00,
  0x3F, 0x00, 0x3F, 0x00, 0x3F, 0x00, 0x3F, 0x00, 0x3F, 0x00, 0x3F, 0x00, 0x3F, 0x00, 0x3F, 0x00,
  0x3F, 0x00, 0x3F, 0x00, 0x3F, 0x00, 0x3F, 0x00, 0x3F, 0x00, 0x3F, 0x00, 0x3F, 0x03, 0xFF, 0x00,
  0x3F, 0xFF, 0x01, 0xFF, 0xFF, 0x0F, 0xFF, 0xFC, 0x7F, 0xFF, 0xF9, 0xFC, 0x07, 0xFF, 0xE0, 0x0F,
  0xFF, 0x00, 0x3F, 0xFC, 0x00, 0x7F, 0xF0, 0x01, 0xFF, 0xC0, 0x07, 0xC0, 0x00, 0x3F, 0x00, 0x00,
  0xFC, 0x00, 0x07, 0xF0, 0x00, 0x3F, 0x80, 0x07, 0xFE, 0x00, 0xFF, 0xF0, 0x0F, 0xFF, 0x80, 0xFF,
  0xF8, 0x07, 0xFF, 0x80, 0x3F, 0xF0, 0x01, 0xFE, 0x00, 0x07, 0xE0, 0x00, 0x3F, 0x00, 0x00, 0xFC,
  0x00, 0x03, 0xF0, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xC0, 0x7F, 0xFF, 0xFE, 0x7F, 0xFF, 0xFE, 0x7F, 0xFF, 0xFE, 0x7F, 0xFF, 0xFE, 0x7F,
  0xFF, 0xFE, 0x00, 0x00, 0x7E, 0x00, 0x00, 0xFE, 0x00, 0x03, 0xFE, 0x00, 0x0F, 0xFC, 0x00, 0x3F,
  0xF0, 0x00, 0xFF, 0xC0, 0x01, 0xFF, 0x00, 0x01, 0xFC, 0x00, 0x01, 0xFF, 0x80, 0x01, 0xFF, 0xF0,
  0x01, 0xFF, 0xFC, 0x01, 0xFF, 0xFE, 0x00, 0x03, 0xFE, 0x00, 0x00, 0x7F, 0x00, 0x00, 0x3F, 0x00,
  0x00, 0x3F, 0xFC, 0x00, 0x3F, 0x7C, 0x00, 0x3F, 0x7E, 0x00, 0x3F, 0x7E, 0x00, 0x7F, 0x7F, 0x00,
  0xFE, 0x3F, 0xF7, 0xFE, 0x3F, 0xFF, 0xFC, 0x1F, 0xFF, 0xF8, 0x07, 0xFF, 0xF0, 0x03, 0xFF, 0xC0,
  0x00, 0x1E, 0x00, 0x00, 0x1F, 0xF8, 0x00, 0x0F, 0xFE, 0x00, 0x07, 0xFF, 0x80, 0x01, 0xFF, 0xE0,
  0x00, 0xFD, 0xF8, 0x00, 0x3F, 0x7E, 0x00, 0x1F, 0x9F, 0x80, 0x0F, 0xC7, 0xE0, 0x03, 0xF1, 0xF8,
  0x01, 0xF8, 0x7E, 0x00, 0x7E, 0x1F, 0x80, 0x3F, 0x07, 0xE0, 0x0F, 0x81, 0xF8, 0x07, 0xE0, 0x7E,
  0x03, 0xF0, 0x1F, 0x80, 0xFC, 0x07, 0xE0, 0x7E, 0x01, 0xF8, 0x1F, 0x00, 0x7E, 0x0F, 0xC0, 0x1F,
  0x83, 0xF0, 0x07, 0xE0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xC0, 0x00, 0x7E, 0x00, 0x00, 0x1F, 0x80, 0x00, 0x07, 0xE0, 0x00, 0x01,
  0xF8, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x1F, 0x80, 0x7F, 0xFF, 0xFC, 0xFF, 0xFF, 0xF9, 0xFF, 0xFF,
  0xF3, 0xFF, 0xFF, 0xE7, 0xFF, 0xFF, 0xCF, 0x80, 0x00, 0x1F, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x7C,
  0x00, 0x00, 0xF8, 0x00, 0x01, 0xF0, 0x20, 0x03, 0xE3, 0xFE, 0x07, 0xDF, 0xFE, 0x0F, 0xFF, 0xFE,
  0x1F, 0xFF, 0xFE, 0x3F, 0xE1, 0xFE, 0x7F, 0x00, 0xFC, 0x00, 0x01, 0xFC, 0x00, 0x01, 0xF8, 0x00,
  0x03, 0xF0, 0x00, 0x07, 0xFF, 0x00, 0x0F, 0xFF, 0x00, 0x1F, 0xFE, 0x00, 0x3F, 0xFC, 0x00, 0xFD,
  0xFC, 0x03, 0xF9, 0xFF, 0xBF, 0xF1, 0xFF, 0xFF, 0xC3, 0xFF, 0xFF, 0x01, 0xFF, 0xFC, 0x01, 0xFF,
  0xE0, 0x00, 0x1A, 0x00, 0x01, 0xFF, 0x80, 0x0F, 0xFF, 0xC0, 0x7F, 0xFF, 0xE1, 0xFF, 0xFF, 0xC3,
  0xFF, 0xFF, 0xCF, 0xE0, 0x1F, 0xDF, 0x80, 0x1F, 0xBE, 0x00, 0x3F, 0xFC, 0x00, 0x7F, 0xF8, 0x00,
  0x03, 0xF0, 0x00, 0x07, 0xE1, 0xF8, 0x0F, 0xCF, 0xFE, 0x1F, 0xBF, 0xFE, 0x3F, 0xFF, 0xFE, 0x7F,
  0xFF, 0xFE, 0xFF, 0x01, 0xFF, 0xFC, 0x00, 0xFF, 0xF0, 0x01, 0xFF, 0xE0, 0x01, 0xFF, 0xC0, 0x03,
  0xFF, 0x80, 0x07, 0xFF, 0x00, 0x0F, 0xFE, 0x00, 0x3F, 0x7E, 0x00, 0x7E, 0xFE, 0x01, 0xFC, 0xFF,
  0xBF, 0xF1, 0xFF, 0xFF, 0xC1, 0xFF, 0xFF, 0x81, 0xFF, 0xFC, 0x00, 0xFF, 0xF0, 0x00, 0x0C, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0x00, 0x01,
  0xF0, 0x00, 0x0F, 0xC0, 0x00, 0x3F, 0x00, 0x01, 0xF8, 0x00, 0x0F, 0xE0, 0x00, 0x3F, 0x00, 0x01,
  0xFC, 0x00, 0x07, 0xE0, 0x00, 0x3F, 0x80, 0x00, 0xFC, 0x00, 0x07, 0xF0, 0x00, 0x1F, 0x80, 0x00,
  0xFC, 0x00, 0x07, 0xF0, 0x00, 0x1F, 0x80, 0x00, 0xFE, 0x00, 0x03, 0xF0, 0x00, 0x1F, 0xC0, 0x00,
  0x7E, 0x00, 0x03, 0xF0, 0x00, 0x1F, 0xC0, 0x00, 0x7E, 0x00, 0x01, 0xF8, 0x00, 0x07, 0xE0, 0x00,
  0x1F, 0x00, 0x00, 0x7C, 0x00, 0x00, 0x03, 0xFF, 0xC0, 0x0F, 0xFF, 0xF0, 0x1F, 0xFF, 0xF8, 0x3F,
  0xFF, 0xFC, 0x7F, 0xFF, 0xFE, 0x7F, 0x00, 0xFE, 0x7E, 0x00, 0x7F, 0x7C, 0x00, 0x3F, 0x7C, 0x00,
  0x3F, 0x7C, 0x00, 0x3F, 0x7E, 0x00, 0x3E, 0x7F, 0x00, 0x7E, 0x3F, 0xC3, 0xFC, 0x1F, 0xFF, 0xFC,
  0x0F, 0xFF, 0xF0, 0x07, 0xFF, 0xE0, 0x1F, 0xFF, 0xF8, 0x3F, 0xFF, 0xFE, 0x7F, 0x00, 0xFE, 0x7E,
  0x00, 0x3F, 0xFC, 0x00, 0x3F, 0xFC, 0x00, 0x3F, 0xFC, 0x00, 0x1F, 0xFC, 0x00, 0x3F, 0xFE, 0x00,
  0x3F, 0x7F, 0x00, 0x7F, 0x7F, 0xE7, 0xFE, 0x3F, 0xFF, 0xFE, 0x1F, 0xFF, 0xFC, 0x0F, 0xFF, 0xF0,
  0x03, 0xFF, 0xE0, 0x00, 0x1C, 0x00, 0x03, 0xFF, 0x80, 0x1F, 0xFF, 0xC0, 0x7F, 0xFF, 0xC1, 0xFF,
  0xFF, 0xC7, 0xFF, 0xFF, 0xCF, 0xE0, 0x3F, 0xBF, 0x80, 0x3F, 0x7E, 0x00, 0x3F, 0xFC, 0x00, 0x7F,
  0xF0, 0x00, 0xFF, 0xE0, 0x01, 0xFF, 0xC0, 0x03, 0xFF, 0xC0, 0x07, 0xFF, 0x80, 0x1F, 0xDF, 0x80,
  0x7F, 0xBF, 0xE7, 0xFF, 0x3F, 0xFF, 0xFE, 0x3F, 0xFF, 0xFC, 0x3F, 0xFD, 0xF8, 0x1F, 0xE3, 0xF0,
  0x00, 0x07, 0xE0, 0x00, 0x0F, 0xFF, 0x00, 0x1F, 0xFE, 0x00, 0x3F, 0xFC, 0x00, 0xFD, 0xFC, 0x03,
  0xF9, 0xFF, 0xBF, 0xF1, 0xFF, 0xFF, 0xC3, 0xFF, 0xFF, 0x01, 0xFF, 0xFC, 0x01, 0xFF, 0xE0, 0x00,
  0x18, 0x00, 0x1C, 0x3F, 0x9F, 0xDF, 0xFF, 0xFF, 0xFD, 0xFC, 0x7E, 0x18, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x38, 0x7F, 0x3F, 0xBF, 0xFF, 0xFF, 0xFB, 0xF8, 0xFC, 0x30, 0x1C, 0x3F, 0x9F,
  0xDF, 0xFF, 0xFF, 0xFD, 0xFC, 0x7E, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x3F,
  0x3F, 0x9F, 0xFF, 0xF7, 0xFB, 0xFC, 0xFE, 0x0F, 0x07, 0x83, 0xCF, 0xC7, 0xE3, 0xE0, 0x00, 0x00,
  0x60, 0x00, 0x7C, 0x00, 0x7F, 0x80, 0x7F, 0xF0, 0x7F, 0xFE, 0x7F, 0xFF, 0xBF, 0xFF, 0x87, 0xFF,
  0x00, 0xFF, 0x00, 0x1F, 0x80, 0x03, 0xF8, 0x00, 0x7F, 0xF0, 0x0F, 0xFF, 0xC0, 0x7F, 0xFF, 0x01,
  0xFF, 0xF8, 0x07, 0xFF, 0x00, 0x1F, 0xE0, 0x00, 0x7C, 0x00, 0x01, 0x80, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xC0, 0x00, 0x1F, 0x00, 0x03,
  0xFC, 0x00, 0x7F, 0xF0, 0x0F, 0xFF, 0xC0, 0xFF, 0xFF, 0x01, 0xFF, 0xF8, 0x07, 0xFF, 0x00, 0x1F,
  0xE0, 0x00, 0x7C, 0x00, 0x3F, 0x80, 0x3F, 0xF0, 0x7F, 0xFE, 0x7F, 0xFF, 0x3F, 0xFF, 0x87, 0xFF,
  0x80, 0xFF, 0x80, 0x1F, 0x80, 0x03, 0x80, 0x00, 0x00, 0x01, 0xFF, 0x80, 0x0F, 0xFF, 0xC0, 0x7F,
  0xFF, 0xC1, 0xFF, 0xFF, 0xC3, 0xFF, 0xFF, 0xCF, 0xE0, 0x3F, 0x9F, 0x80, 0x3F, 0x3E, 0x00, 0x3F,
  0x7C, 0x00, 0x7F, 0xF8, 0x00, 0xFC, 0x00, 0x03, 0xF0, 0x00, 0x07, 0xE0, 0x00, 0x3F, 0xC0, 0x01,
  0xFF, 0x00, 0x07, 0xFC, 0x00, 0x1F, 0xF0, 0x00, 0x7F, 0xC0, 0x01, 0xFE, 0x00, 0x03, 0xF0, 0x00,
  0x07, 0xE0, 0x00, 0x0F, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1C, 0x00, 0x00, 0xFC,
  0x00, 0x03, 0xFC, 0x00, 0x07, 0xF8, 0x00, 0x0F, 0xF0, 0x00, 0x1F, 0xE0, 0x00, 0x1F, 0xC0, 0x00,
  0x3F, 0x00, 0x00, 0x08, 0x00, 0x00, 0x07, 0xFF, 0xE0, 0x00, 0x00, 0x7F, 0xFF, 0xF0, 0x00, 0x03,
  0xFF, 0xFF, 0xF8, 0x00, 0x0F, 0xFF, 0xFF, 0xFC, 0x00, 0x7F, 0xFF, 0xFF, 0xFC, 0x00, 0xFF, 0x80,
  0x07, 0xFC, 0x03, 0xFC, 0x00, 0x03, 0xF8, 0x0F, 0xE0, 0x00, 0x03, 0xF8, 0x3F, 0x80, 0x00, 0x03,
  0xF8, 0x7E, 0x07, 0xF1, 0xF3, 0xF1, 0xFC, 0x1F, 0xFB, 0xE3, 0xF3, 0xF0, 0x7F, 0xF7, 0xC7, 0xE7,
  0xC1, 0xFF, 0xFF, 0x87, 0xDF, 0x83, 0xFF, 0xFF, 0x0F, 0xBF, 0x0F, 0xE0, 0xFE, 0x1F, 0xFE, 0x1F,
  0x80, 0xFC, 0x3F, 0xFC, 0x3E, 0x00, 0xF8, 0x7F, 0xF0, 0xFC, 0x01, 0xF0, 0xFF, 0xE1, 0xF8, 0x03,
  0xE1, 0xFF, 0xC3, 0xF0, 0x07, 0xC3, 0xFF, 0x87, 0xE0, 0x0F, 0x87, 0xFF, 0x0F, 0xC0, 0x1F, 0x0F,
  0xFE, 0x1F, 0x80, 0x3E, 0x1F, 0xFE, 0x1F, 0x00, 0x7C, 0x3F, 0xFC, 0x3F, 0x01, 0xF8, 0x7F, 0xF8,
  0x7F, 0x07, 0xF9, 0xF9, 0xF0, 0x7F, 0xFF, 0xFF, 0xF3, 0xF0, 0xFF, 0xFF, 0xFF, 0xE7, 0xE0, 0xFF,
  0xE7, 0xFF, 0x87, 0xE0, 0xFF, 0xCF, 0xFE, 0x0F, 0xE0, 0x7F, 0x0F, 0xF0, 0x0F, 0xE0, 0x00, 0x00,
  0x00, 0x0F, 0xE0, 0x00, 0x00, 0x00, 0x1F, 0xF0, 0x00, 0x00, 0x00, 0x1F, 0xFE, 0x00, 0x00, 0x00,
  0x1F, 0xFF, 0xFF, 0xE0, 0x00, 0x0F, 0xFF, 0xFF, 0xC0, 0x00, 0x0F, 0xFF, 0xFF, 0x80, 0x00, 0x03,
  0xFF, 0xFF, 0x00, 0x00, 0x00, 0x7F, 0xFE, 0x00, 0x00, 0xFF, 0xC0, 0x00, 0x3F, 0xF0, 0x00, 0x1F,
  0xFE, 0x00, 0x07, 0xFF, 0x80, 0x01, 0xF7, 0xE0, 0x00, 0xFC, 0xF8, 0x00, 0x3F, 0x3F, 0x00, 0x0F,
  0xCF, 0xC0, 0x03, 0xE3, 0xF0, 0x01, 0xF8, 0x7C, 0x00, 0x7E, 0x1F, 0x80, 0x1F, 0x87, 0xE0, 0x07,
  0xC1, 0xF8, 0x03, 0xF0, 0x3F, 0x00, 0xFC, 0x0F, 0xC0, 0x3F, 0x03, 0xF0, 0x0F, 0x80, 0xFC, 0x07,
  0xE0, 0x1F, 0x81, 0xF8, 0x07, 0xE0, 0x7F, 0xFF, 0xF8, 0x1F, 0xFF, 0xFE, 0x0F, 0xFF, 0xFF, 0xC3,
  0xFF, 0xFF, 0xF0, 0xFF, 0xFF, 0xFC, 0x7E, 0x00, 0x1F, 0x1F, 0x80, 0x07, 0xE7, 0xE0, 0x01, 0xF9,
  0xF8, 0x00, 0x7E, 0xFC, 0x00, 0x0F, 0xBF, 0x00, 0x03, 0xFF, 0xC0, 0x00, 0xFC, 0xFF, 0xFF, 0xE0,
  0x7F, 0xFF, 0xFC, 0x3F, 0xFF, 0xFF, 0x9F, 0xFF, 0xFF, 0xEF, 0xFF, 0xFF, 0xF0, 0x7E, 0x0F, 0xFC,
  0x3F, 0x00, 0xFE, 0x1F, 0x80, 0x3F, 0x0F, 0xC0, 0x1F, 0x87, 0xE0, 0x0F, 0xC3, 0xF0, 0x07, 0xE1,
  0xF8, 0x03, 0xF0, 0xFC, 0x07, 0xF0, 0x7F, 0xFF, 0xF0, 0x3F, 0xFF, 0xF0, 0x1F, 0xFF, 0xE0, 0x0F,
  0xFF, 0xFC, 0x07, 0xFF, 0xFF, 0x03, 0xF0, 0x1F, 0xC1, 0xF8, 0x03, 0xF0, 0xFC, 0x01, 0xF8, 0x7E,
  0x00, 0x7C, 0x3F, 0x00, 0x3E, 0x1F, 0x80, 0x3F, 0x0F, 0xC0, 0x1F, 0x87, 0xE0, 0x3F, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0xFF, 0xFF, 0xE7, 0xFF, 0xFF, 0xE3, 0xFF, 0xFF, 0xC0, 0x01, 0xFF,
  0xC0, 0x07, 0xFF, 0xF0, 0x0F, 0xFF, 0xF8, 0x1F, 0xFF, 0xFC, 0x3F, 0xFF, 0xFE, 0x3F, 0x80, 0xFE,
  0x7F, 0x00, 0x7F, 0x7E, 0x00, 0x3F, 0xFE, 0x00, 0x3F, 0xFC, 0x00, 0x1F, 0xFC, 0x00, 0x1F, 0xFC,
  0x00, 0x00, 0xFC, 0x00, 0x00, 0xFC, 0x00, 0x00, 0xFC, 0x00, 0x00, 0xFC, 0x00, 0x00, 0xFC, 0x00,
  0x00, 0xFC, 0x00, 0x00, 0xFC, 0x00, 0x00, 0xFC, 0x00, 0x1F, 0xFC, 0x00, 0x1F, 0xFC, 0x00, 0x1F,
  0xFE, 0x00, 0x3F, 0x7E, 0x00, 0x3F, 0x7F, 0x00, 0x7F, 0x7F, 0x80, 0xFE, 0x3F, 0xFF, 0xFE, 0x1F,
  0xFF, 0xFC, 0x0F, 0xFF, 0xF8, 0x07, 0xFF, 0xF0, 0x01, 0xFF, 0xE0, 0x00, 0x1C, 0x00, 0xFF, 0xFF,
  0x00, 0x7F, 0xFF, 0xF0, 0x3F, 0xFF, 0xFE, 0x1F, 0xFF, 0xFF, 0x8F, 0xFF, 0xFF, 0xE0, 0x7E, 0x3F,
  0xF8, 0x3F, 0x01, 0xFC, 0x1F, 0x80, 0x7F, 0x0F, 0xC0, 0x1F, 0x87, 0xE0, 0x0F, 0xC3, 0xF0, 0x07,
  0xE1, 0xF8, 0x01, 0xF0, 0xFC, 0x00, 0xF8, 0x7E, 0x00, 0x7C, 0x3F, 0x00, 0x3E, 0x1F, 0x80, 0x1F,
  0x0F, 0xC0, 0x0F, 0x87, 0xE0, 0x07, 0xC3, 0xF0, 0x03, 0xE1, 0xF8, 0x01, 0xF0, 0xFC, 0x01, 0xF8,
  0x7E, 0x00, 0xFC, 0x3F, 0x00, 0x7E, 0x1F, 0x80, 0x7F, 0x0F, 0xC0, 0x7F, 0x07, 0xE1, 0xFF, 0xBF,
  0xFF, 0xFF, 0x9F, 0xFF, 0xFF, 0xCF, 0xFF, 0xFF, 0x87, 0xFF, 0xFF, 0x83, 0xFF, 0xFE, 0x00, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC0, 0x00, 0xFC, 0x00,
  0x0F, 0xC0, 0x00, 0xFC, 0x00, 0x0F, 0xC0, 0x00, 0xFC, 0x00, 0x0F, 0xC0, 0x00, 0xFC, 0x00, 0x0F,
  0xFF, 0xFC, 0xFF, 0xFF, 0xCF, 0xFF, 0xFC, 0xFF, 0xFF, 0xCF, 0xFF, 0xFC, 0xFC, 0x00, 0x0F, 0xC0,
  0x00, 0xFC, 0x00, 0x0F, 0xC0, 0x00, 0xFC, 0x00, 0x0F, 0xC0, 0x00, 0xFC, 0x00, 0x0F, 0xC0, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF8, 0x00, 0x3F, 0x00, 0x07, 0xE0, 0x00,
  0xFC, 0x00, 0x1F, 0x80, 0x03, 0xF0, 0x00, 0x7E, 0x00, 0x0F, 0xC0, 0x01, 0xFF, 0xFF, 0xBF, 0xFF,
  0xF7, 0xFF, 0xFE, 0xFF, 0xFF, 0xDF, 0xFF, 0xFB, 0xF0, 0x00, 0x7E, 0x00, 0x0F, 0xC0, 0x01, 0xF8,
  0x00, 0x3F, 0x00, 0x07, 0xE0, 0x00, 0xFC, 0x00, 0x1F, 0x80, 0x03, 0xF0, 0x00, 0x7E, 0x00, 0x0F,
  0xC0, 0x01, 0xF8, 0x00, 0x3F, 0x00, 0x00, 0x01, 0xFF, 0xC0, 0x07, 0xFF, 0xF0, 0x0F, 0xFF, 0xF8,
  0x1F, 0xFF, 0xFC, 0x3F, 0xFF, 0xFE, 0x3F, 0x80, 0xFE, 0x7F, 0x00, 0x7F, 0x7E, 0x00, 0x3F, 0xFE,
  0x00, 0x3F, 0xFC, 0x00, 0x1F, 0xFC, 0x00, 0x1F, 0xFC, 0x00, 0x00, 0xFC, 0x00, 0x00, 0xFC, 0x00,
  0x00, 0xFC, 0x00, 0x00, 0xFC, 0x1F, 0xFF, 0xFC, 0x1F, 0xFF, 0xFC, 0x1F, 0xFF, 0xFC, 0x1F, 0xFF,
  0xFC, 0x1F, 0xFF, 0xFC, 0x00, 0x1F, 0xFC, 0x00, 0x1F, 0xFE, 0x00, 0x1F, 0x7E, 0x00, 0x3F, 0x7F,
  0x00, 0x3F, 0x7F, 0x80, 0x7F, 0x3F, 0xFF, 0xFF, 0x1F, 0xFF, 0xFF, 0x0F, 0xFF, 0xDF, 0x07, 0xFF,
  0x9F, 0x03, 0xFF, 0x1F, 0x00, 0x18, 0x00, 0xFC, 0x00, 0x7F, 0xF8, 0x00, 0xFF, 0xF0, 0x01, 0xFF,
  0xE0, 0x03, 0xFF, 0xC0, 0x07, 0xFF, 0x80, 0x0F, 0xFF, 0x00, 0x1F, 0xFE, 0x00, 0x3F, 0xFC, 0x00,
  0x7F, 0xF8, 0x00, 0xFF, 0xF0, 0x01, 0xFF, 0xE0, 0x03, 0xFF, 0xC0, 0x07, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x01, 0xFF, 0xE0, 0x03,
  0xFF, 0xC0, 0x07, 0xFF, 0x80, 0x0F, 0xFF, 0x00, 0x1F, 0xFE, 0x00, 0x3F, 0xFC, 0x00, 0x7F, 0xF8,
  0x00, 0xFF, 0xF0, 0x01, 0xFF, 0xE0, 0x03, 0xFF, 0xC0, 0x07, 0xFF, 0x80, 0x0F, 0xFF, 0x00, 0x1F,
  0x80, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC0, 0x00, 0x7F, 0xFF, 0x00, 0x7F, 0xFF, 0x00,
  0x7F, 0xFF, 0x00, 0x7F, 0xFF, 0x00, 0x7F, 0xFF, 0x00, 0x01, 0xF0, 0x00, 0x01, 0xF0, 0x00, 0x01,
  0xF0, 0x00, 0x01, 0xF0, 0x00, 0x01, 0xF0, 0x00, 0x01, 0xF0, 0x00, 0x01, 0xF0, 0x00, 0x01, 0xF0,
  0x00, 0x01, 0xF0, 0x00, 0x01, 0xF0, 0x00, 0x01, 0xF0, 0x00, 0x01, 0xF0, 0x00, 0x01, 0xF0, 0x00,
  0x01, 0xF0, 0xFC, 0x01, 0xF0, 0xFC, 0x01, 0xF0, 0xFC, 0x01, 0xF0, 0xFC, 0x01, 0xF0, 0xFC, 0x03,
  0xF0, 0x7E, 0x03, 0xF0, 0x7F, 0x07, 0xF0, 0x7F, 0xFF, 0xF0, 0x3F, 0xFF, 0xE0, 0x1F, 0xFF, 0xC0,
  0x0F, 0xFF, 0x80, 0x07, 0xFF, 0x00, 0x00, 0x70, 0x00, 0xFC, 0x00, 0xFF, 0xFC, 0x01, 0xFE, 0xFC,
  0x01, 0xFC, 0xFC, 0x03, 0xF8, 0xFC, 0x07, 0xF0, 0xFC, 0x0F, 0xE0, 0xFC, 0x1F, 0xE0, 0xFC, 0x1F,
  0xC0, 0xFC, 0x3F, 0x80, 0xFC, 0x7F, 0x00, 0xFC, 0xFE, 0x00, 0xFD, 0xFC, 0x00, 0xFF, 0xFC, 0x00,
  0xFF, 0xF8, 0x00, 0xFF, 0xF0, 0x00, 0xFF, 0xE0, 0x00, 0xFF, 0xF0, 0x00, 0xFF, 0xF8, 0x00, 0xFF,
  0xFC, 0x00, 0xFD, 0xFE, 0x00, 0xFC, 0xFE, 0x00, 0xFC, 0x7F, 0x00, 0xFC, 0x3F, 0x80, 0xFC, 0x1F,
  0xC0, 0xFC, 0x1F, 0xE0, 0xFC, 0x0F, 0xF0, 0xFC, 0x07, 0xF8, 0xFC, 0x03, 0xF8, 0xFC, 0x01, 0xFC,
  0xFC, 0x01, 0xFE, 0xFC, 0x00, 0xFF, 0xFC, 0x00, 0x0F, 0xC0, 0x00, 0xFC, 0x00, 0x0F, 0xC0, 0x00,
  0xFC, 0x00, 0x0F, 0xC0, 0x00, 0xFC, 0x00, 0x0F, 0xC0, 0x00, 0xFC, 0x00, 0x0F, 0xC0, 0x00, 0xFC,
  0x00, 0x0F, 0xC0, 0x00, 0xFC, 0x00, 0x0F, 0xC0, 0x00, 0xFC, 0x00, 0x0F, 0xC0, 0x00, 0xFC, 0x00,
  0x0F, 0xC0, 0x00, 0xFC, 0x00, 0x0F, 0xC0, 0x00, 0xFC, 0x00, 0x0F, 0xC0, 0x00, 0xFC, 0x00, 0x0F,
  0xC0, 0x00, 0xFC, 0x00, 0x0F, 0xC0, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xF0, 0xFF, 0xE0, 0x03, 0xFF, 0xFF, 0xF0, 0x01, 0xFF, 0xFF, 0xF8, 0x00, 0xFF,
  0xFF, 0xFC, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x7D, 0xFF, 0xEF, 0x80, 0x3E, 0xFF, 0xF7, 0xC0, 0x1F,
  0x7F, 0xFB, 0xE0, 0x0F, 0xBF, 0xFD, 0xF0, 0x0F, 0xDF, 0xFE, 0xFC, 0x07, 0xEF, 0xFF, 0x3E, 0x03,
  0xE7, 0xFF, 0x9F, 0x01, 0xF3, 0xFF, 0xCF, 0x80, 0xF9, 0xFF, 0xE7, 0xC0, 0xFC, 0xFF, 0xF3, 0xF0,
  0x7E, 0x7F, 0xF8, 0xF8, 0x3E, 0x3F, 0xFC, 0x7C, 0x1F, 0x1F, 0xFE, 0x3E, 0x0F, 0x8F, 0xFF, 0x1F,
  0x0F, 0xC7, 0xFF, 0x8F, 0xC7, 0xE3, 0xFF, 0xC3, 0xE3, 0xE1, 0xFF, 0xE1, 0xF1, 0xF0, 0xFF, 0xF0,
  0xF8, 0xF8, 0x7F, 0xF8, 0x7C, 0xFC, 0x3F, 0xFC, 0x3F, 0x7E, 0x1F, 0xFE, 0x0F, 0xBE, 0x0F, 0xFF,
  0x07, 0xDF, 0x07, 0xFF, 0x83, 0xFF, 0x83, 0xFF, 0xC1, 0xFF, 0xC1, 0xFF, 0xE0, 0xFF, 0xE0, 0xFF,
  0xF0, 0x3F, 0xE0, 0x7E, 0xFF, 0xE0, 0x3F, 0xFF, 0xE0, 0x3F, 0xFF, 0xE0, 0x3F, 0xFF, 0xF0, 0x3F,
  0xFF, 0xF0, 0x3F, 0xFD, 0xF0, 0x3F, 0xFD, 0xF0, 0x3F, 0xFD, 0xF8, 0x3F, 0xFD, 0xF8, 0x3F, 0xFC,
  0xF8, 0x3F, 0xFC, 0xF8, 0x3F, 0xFC, 0xFC, 0x3F, 0xFC, 0xFC, 0x3F, 0xFC, 0x7C, 0x3F, 0xFC, 0x7C,
  0x3F, 0xFC, 0x7C, 0x3F, 0xFC, 0x7E, 0x3F, 0xFC, 0x7E, 0x3F, 0xFC, 0x3E, 0x3F, 0xFC, 0x3E, 0x3F,
  0xFC, 0x3F, 0x3F, 0xFC, 0x3F, 0x3F, 0xFC, 0x1F, 0x3F, 0xFC, 0x1F, 0x3F, 0xFC, 0x1F, 0x3F, 0xFC,
  0x1F, 0xBF, 0xFC, 0x0F, 0xBF, 0xFC, 0x0F, 0xFF, 0xFC, 0x0F, 0xFF, 0xFC, 0x0F, 0xFF, 0xFC, 0x0F,
  0xFF, 0x01, 0xFF, 0xC0, 0x01, 0xFF, 0xFC, 0x00, 0xFF, 0xFF, 0xC0, 0x7F, 0xFF, 0xF8, 0x3F, 0xFF,
  0xFF, 0x0F, 0xE0, 0x3F, 0xC7, 0xF0, 0x03, 0xF9, 0xF8, 0x00, 0x7E, 0xFE, 0x00, 0x1F, 0xBF, 0x00,
  0x07, 0xEF, 0xC0, 0x00, 0xFB, 0xF0, 0x00, 0x3F, 0xFC, 0x00, 0x0F, 0xFF, 0x00, 0x03, 0xFF, 0xC0,
  0x00, 0xFF, 0xF0, 0x00, 0x3F, 0xFC, 0x00, 0x0F, 0xFF, 0x00, 0x03, 0xFF, 0xC0, 0x00, 0xFF, 0xF0,
  0x00, 0x3F, 0xFC, 0x00, 0x0F, 0xFF, 0x00, 0x07, 0xEF, 0xE0, 0x01, 0xF9, 0xF8, 0x00, 0x7E, 0x7F,
  0x00, 0x3F, 0x9F, 0xE0, 0x1F, 0xC3, 0xFF, 0xFF, 0xF0, 0x7F, 0xFF, 0xF8, 0x0F, 0xFF, 0xFC, 0x01,
  0xFF, 0xFE, 0x00, 0x1F, 0xFE, 0x00, 0x00, 0x70, 0x00, 0xFF, 0xFE, 0x03, 0xFF, 0xFF, 0x0F, 0xFF,
  0xFE, 0x3F, 0xFF, 0xFC, 0xFF, 0xFF, 0xFB, 0xF0, 0x3F, 0xFF, 0xC0, 0x1F, 0xFF, 0x00, 0x3F, 0xFC,
  0x00, 0xFF, 0xF0, 0x01, 0xFF, 0xC0, 0x07, 0xFF, 0x00, 0x3F, 0xFC, 0x00, 0xFF, 0xF0, 0x07, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xF3, 0xFF, 0xFF, 0x8F, 0xFF, 0xFC, 0x3F, 0xFF, 0x80,
  0xFC, 0x00, 0x03, 0xF0, 0x00, 0x0F, 0xC0, 0x00, 0x3F, 0x00, 0x00, 0xFC, 0x00, 0x03, 0xF0, 0x00,
  0x0F, 0xC0, 0x00, 0x3F, 0x00, 0x00, 0xFC, 0x00, 0x03, 0xF0, 0x00, 0x0F, 0xC0, 0x00, 0x00, 0x01,
  0xFF, 0xC0, 0x01, 0xFF, 0xFC, 0x00, 0xFF, 0xFF, 0xC0, 0x7F, 0xFF, 0xF8, 0x3F, 0xFF, 0xFF, 0x0F,
  0xE0, 0x3F, 0xC7, 0xF0, 0x03, 0xF9, 0xF8, 0x00, 0x7E, 0xFE, 0x00, 0x1F, 0xBF, 0x00, 0x07, 0xEF,
  0xC0, 0x00, 0xFB, 0xF0, 0x00, 0x3F, 0xFC, 0x00, 0x0F, 0xFF, 0x00, 0x03, 0xFF, 0xC0, 0x00, 0xFF,
  0xF0, 0x00, 0x3F, 0xFC, 0x00, 0x0F, 0xFF, 0x00, 0x03, 0xFF, 0xC0, 0x00, 0xFF, 0xF0, 0x00, 0x3F,
  0xFC, 0x00, 0x0F, 0xFF, 0x00, 0x07, 0xEF, 0xE0, 0x01, 0xF9, 0xF8, 0x00, 0x7E, 0x7F, 0x00, 0x3F,
  0x9F, 0xE0, 0x1F, 0xC3, 0xFF, 0xFF, 0xF0, 0x7F, 0xFF, 0xF8, 0x0F, 0xFF, 0xFC, 0x01, 0xFF, 0xFE,
  0x00, 0x1F, 0xFE, 0x00, 0x00, 0xFC, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x0F, 0xC0, 0x00, 0x03, 0xFF,
  0x00, 0x00, 0xFF, 0xC0, 0x00, 0x1F, 0xF0, 0x00, 0x07, 0xFC, 0x00, 0x00, 0x7F, 0x00, 0xFF, 0xFF,
  0x01, 0xFF, 0xFF, 0xC3, 0xFF, 0xFF, 0xC7, 0xFF, 0xFF, 0xCF, 0xFF, 0xFF, 0xDF, 0x80, 0xFF, 0xBF,
  0x00, 0x3F, 0xFE, 0x00, 0x3F, 0xFC, 0x00, 0x7F, 0xF8, 0x00, 0xFF, 0xF0, 0x01, 0xFF, 0xE0, 0x03,
  0xFF, 0xC0, 0x0F, 0xDF, 0x80, 0x3F, 0xBF, 0xFF, 0xFE, 0x7F, 0xFF, 0xF0, 0xFF, 0xFF, 0xC1, 0xFF,
  0xFF, 0xF3, 0xFF, 0xFF, 0xE7, 0xE0, 0x1F, 0xEF, 0xC0, 0x0F, 0xDF, 0x80, 0x1F, 0xBF, 0x00, 0x3F,
  0x7E, 0x00, 0x7E, 0xFC, 0x00, 0xFD, 0xF8, 0x01, 0xFB, 0xF0, 0x03, 0xF7, 0xE0, 0x07, 0xEF, 0xC0,
  0x0F, 0xDF, 0x80, 0x1F, 0xBF, 0x00, 0x3F, 0x00, 0x01, 0xFF, 0x80, 0x07, 0xFF, 0xE0, 0x0F, 0xFF,
  0xF8, 0x1F, 0xFF, 0xFC, 0x3F, 0xFF, 0xFC, 0x3F, 0x00, 0xFE, 0x7E, 0x00, 0x7E, 0x7E, 0x00, 0x7E,
  0x7E, 0x00, 0x3E, 0x7E, 0x00, 0x3E, 0x7F, 0x00, 0x3E, 0x3F, 0x80, 0x00, 0x3F, 0xF0, 0x00, 0x1F,
  0xFF, 0x00, 0x0F, 0xFF, 0xE0, 0x07, 0xFF, 0xF8, 0x01, 0xFF, 0xFC, 0x00, 0x3F, 0xFE, 0x00, 0x03,
  0xFE, 0x00, 0x00, 0x7F, 0xFC, 0x00, 0x3F, 0x7C, 0x00, 0x3F, 0x7C, 0x00, 0x1F, 0x7E, 0x00, 0x3F,
  0x7E, 0x00, 0x3F, 0x7F, 0x00, 0x7F, 0x3F, 0xF7, 0xFE, 0x1F, 0xFF, 0xFC, 0x0F, 0xFF, 0xFC, 0x07,
  0xFF, 0xF0, 0x01, 0xFF, 0xE0, 0x00, 0x1C, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x7E, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x7E, 0x00,
  0x00, 0x7E, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x7E, 0x00, 0x00,
  0x7E, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x7E,
  0x00, 0x00, 0x7E, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x7E, 0x00,
  0x00, 0x7E, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x7E, 0x00, 0x00,
  0x7E, 0x00, 0x00, 0x7E, 0x00, 0xFC, 0x00, 0x3F, 0xFC, 0x00, 0x3F, 0xFC, 0x00, 0x3F, 0xFC, 0x00,
  0x3F, 0xFC, 0x00, 0x3F, 0xFC, 0x00, 0x3F, 0xFC, 0x00, 0x3F, 0xFC, 0x00, 0x3F, 0xFC, 0x00, 0x3F,
  0xFC, 0x00, 0x3F, 0xFC, 0x00, 0x3F, 0xFC, 0x00, 0x3F, 0xFC, 0x00, 0x3F, 0xFC, 0x00, 0x3F, 0xFC,
  0x00, 0x3F, 0xFC, 0x00, 0x3F, 0xFC, 0x00, 0x3F, 0xFC, 0x00, 0x3F, 0xFC, 0x00, 0x3F, 0xFC, 0x00,
  0x3F, 0xFC, 0x00, 0x3F, 0xFC, 0x00, 0x3F, 0xFC, 0x00, 0x3F, 0xFC, 0x00, 0x7E, 0xFE, 0x00, 0x7E,
  0x7F, 0x00, 0xFE, 0x7F, 0xFF, 0xFC, 0x3F, 0xFF, 0xFC, 0x1F, 0xFF, 0xF8, 0x0F, 0xFF, 0xE0, 0x03,
  0xFF, 0xC0, 0x00, 0x18, 0x00, 0xFC, 0x00, 0x1F, 0xFE, 0x00, 0x0F, 0xFF, 0x00, 0x07, 0xEF, 0xC0,
  0x03, 0xF7, 0xE0, 0x03, 0xF3, 0xF0, 0x01, 0xF9, 0xF8, 0x00, 0xFC, 0x7E, 0x00, 0x7E, 0x3F, 0x00,
  0x7E, 0x1F, 0x80, 0x3F, 0x0F, 0xC0, 0x1F, 0x83, 0xF0, 0x0F, 0xC1, 0xF8, 0x0F, 0xC0, 0xFC, 0x07,
  0xE0, 0x7E, 0x03, 0xF0, 0x1F, 0x01, 0xF8, 0x0F, 0xC1, 0xF8, 0x07, 0xE0, 0xFC, 0x03, 0xF0, 0x7E,
  0x00, 0xF8, 0x3F, 0x00, 0x7E, 0x1F, 0x00, 0x3F, 0x1F, 0x80, 0x1F, 0x8F, 0xC0, 0x07, 0xC7, 0xC0,
  0x03, 0xF3, 0xE0, 0x01, 0xFB, 0xF0, 0x00, 0xFD, 0xF8, 0x00, 0x3F, 0xF8, 0x00, 0x1F, 0xFC, 0x00,
  0x0F, 0xFE, 0x00, 0x07, 0xFF, 0x00, 0xFC, 0x03, 0xFF, 0x01, 0xFF, 0xE0, 0x1F, 0xF8, 0x0F, 0xDF,
  0x01, 0xFF, 0xC0, 0x7E, 0xF8, 0x0F, 0xFE, 0x03, 0xF7, 0xE0, 0x7D, 0xF0, 0x1F, 0xBF, 0x03, 0xEF,
  0xC0, 0xFD, 0xF8, 0x1F, 0x3E, 0x07, 0xCF, 0xC0, 0xF9, 0xF0, 0x3E, 0x7E, 0x07, 0xCF, 0x83, 0xF1,
  0xF0, 0x7E, 0x7C, 0x1F, 0x8F, 0x83, 0xE3, 0xE0, 0xFC, 0x7C, 0x1F, 0x1F, 0x07, 0xE3, 0xE0, 0xF8,
  0xFC, 0x3F, 0x1F, 0x87, 0xC3, 0xE1, 0xF0, 0xFC, 0x3E, 0x1F, 0x0F, 0x87, 0xE1, 0xF0, 0xF8, 0x7C,
  0x3F, 0x1F, 0x87, 0xC3, 0xE0, 0xF8, 0xF8, 0x3E, 0x3F, 0x07, 0xC7, 0xC1, 0xF1, 0xF8, 0x3E, 0x3E,
  0x0F, 0xCF, 0xC1, 0xF1, 0xF0, 0x3E, 0x7E, 0x0F, 0x8F, 0x81, 0xF3, 0xE0, 0x7E, 0xFC, 0x0F, 0x9F,
  0x03, 0xF7, 0xE0, 0x7C, 0xF8, 0x0F, 0xBE, 0x03, 0xE7, 0xC0, 0x7D, 0xF0, 0x1F, 0x3E, 0x03, 0xEF,
  0x80, 0xFF, 0xF0, 0x1F, 0xFC, 0x03, 0xFF, 0x80, 0xFF, 0xE0, 0x1F, 0xF8, 0x07, 0xFF, 0x00, 0xFF,
  0xC0, 0x3F, 0xF0, 0x07, 0xFE, 0x00, 0xFE, 0x00, 0x0F, 0xDF, 0x80, 0x07, 0xF7, 0xF0, 0x03, 0xF8,
  0xFC, 0x00, 0xFC, 0x1F, 0x80, 0x7F, 0x07, 0xF0, 0x1F, 0x80, 0xFC, 0x0F, 0xE0, 0x3F, 0x83, 0xF0,
  0x07, 0xE1, 0xF8, 0x01, 0xFC, 0x7E, 0x00, 0x3F, 0x3F, 0x00, 0x07, 0xFF, 0xC0, 0x01, 0xFF, 0xE0,
  0x00, 0x3F, 0xF8, 0x00, 0x0F, 0xFC, 0x00, 0x01, 0xFE, 0x00, 0x00, 0xFF, 0xC0, 0x00, 0x3F, 0xF8,
  0x00, 0x1F, 0xFE, 0x00, 0x07, 0xFF, 0xC0, 0x03, 0xF3, 0xF0, 0x01, 0xFC, 0x7E, 0x00, 0x7E, 0x1F,
  0x80, 0x3F, 0x83, 0xF0, 0x0F, 0xC0, 0xFE, 0x07, 0xF0, 0x1F, 0x81, 0xF8, 0x07, 0xF0, 0xFC, 0x00,
  0xFC, 0x7F, 0x00, 0x3F, 0x9F, 0x80, 0x07, 0xFF, 0xE0, 0x01, 0xFC, 0xFE, 0x00, 0x0F, 0xEF, 0xC0,
  0x01, 0xF8, 0xFC, 0x00, 0x7F, 0x1F, 0x80, 0x0F, 0xC1, 0xF8, 0x01, 0xF8, 0x3F, 0x00, 0x7E, 0x03,
  0xF0, 0x0F, 0xC0, 0x7E, 0x03, 0xF0, 0x07, 0xE0, 0x7E, 0x00, 0xFC, 0x1F, 0x80, 0x0F, 0xC3, 0xF0,
  0x01, 0xF8, 0xFC, 0x00, 0x1F, 0x9F, 0x80, 0x03, 0xFF, 0xE0, 0x00, 0x3F, 0xFC, 0x00, 0x07, 0xFF,
  0x00, 0x00, 0x7F, 0xE0, 0x00, 0x0F, 0xF8, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x1F, 0xC0, 0x00, 0x01,
  0xF8, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x07, 0xE0, 0x00, 0x00, 0xFC, 0x00, 0x00, 0x1F, 0x80, 0x00,
  0x03, 0xF0, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x0F, 0xC0, 0x00, 0x01, 0xF8, 0x00, 0x00, 0x3F, 0x00,
  0x00, 0x07, 0xE0, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0x80, 0x00, 0xFC, 0x00, 0x07, 0xE0, 0x00, 0x7F, 0x00, 0x07, 0xF8, 0x00, 0x7F, 0x80, 0x07,
  0xF8, 0x00, 0x3F, 0x80, 0x03, 0xF8, 0x00, 0x3F, 0x80, 0x03, 0xF8, 0x00, 0x3F, 0x80, 0x03, 0xFC,
  0x00, 0x3F, 0xC0, 0x03, 0xFC, 0x00, 0x1F, 0xC0, 0x01, 0xFC, 0x00, 0x1F, 0xC0, 0x01, 0xFC, 0x00,
  0x1F, 0xE0, 0x00, 0xFE, 0x00, 0x07, 0xE0, 0x00, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xE0, 0x3F, 0xF7, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0x0F, 0x80, 0xF8,
  0x0F, 0x80, 0xF8, 0x0F, 0x80, 0xF8, 0x0F, 0x80, 0xF8, 0x0F, 0x80, 0xF8, 0x0F, 0x80, 0xF8, 0x0F,
  0x80, 0xF8, 0x0F, 0x80, 0xF8, 0x0F, 0x80, 0xF8, 0x0F, 0x80, 0xF8, 0x0F, 0x80, 0xF8, 0x0F, 0x80,
  0xF8, 0x0F, 0x80, 0xF8, 0x0F, 0x80, 0xF8, 0x0F, 0x80, 0xF8, 0x0F, 0x80, 0xF8, 0x0F, 0x80, 0xFC,
  0x0F, 0xFF, 0xFF, 0xF7, 0xFF, 0x7F, 0xF1, 0xFF, 0xFC, 0x00, 0x0F, 0x80, 0x01, 0xF8, 0x00, 0x3F,
  0x00, 0x03, 0xE0, 0x00, 0x7E, 0x00, 0x0F, 0xC0, 0x00, 0xF8, 0x00, 0x1F, 0x80, 0x03, 0xF0, 0x00,
  0x3E, 0x00, 0x07, 0xE0, 0x00, 0xFC, 0x00, 0x0F, 0x80, 0x01, 0xF8, 0x00, 0x3F, 0x00, 0x03, 0xE0,
  0x00, 0x7C, 0x00, 0x0F, 0xC0, 0x00, 0xF8, 0x00, 0x1F, 0x00, 0x03, 0xF0, 0x00, 0x3E, 0x00, 0x07,
  0xC0, 0x00, 0xFC, 0x00, 0x0F, 0x80, 0x01, 0xF0, 0x00, 0x3F, 0x00, 0x03, 0xE0, 0x00, 0x7C, 0x00,
  0x0F, 0xC0, 0x00, 0xF8, 0x00, 0x1F, 0x00, 0x03, 0xF0, 0x00, 0x7E, 0x00, 0x07, 0xC0, 0x00, 0xFC,
  0x00, 0x1F, 0x80, 0x01, 0xF0, 0x00, 0x3F, 0xFF, 0xCF, 0xFE, 0xFF, 0xEF, 0xFF, 0x07, 0xF0, 0x3F,
  0x03, 0xF0, 0x3F, 0x03, 0xF0, 0x3F, 0x03, 0xF0, 0x3F, 0x03, 0xF0, 0x3F, 0x03, 0xF0, 0x3F, 0x03,
  0xF0, 0x3F, 0x03, 0xF0, 0x3F, 0x03, 0xF0, 0x3F, 0x03, 0xF0, 0x3F, 0x03, 0xF0, 0x3F, 0x03, 0xF0,
  0x3F, 0x03, 0xF0, 0x3F, 0x03, 0xF0, 0x3F, 0x03, 0xF0, 0x3F, 0x03, 0xF0, 0x3F, 0x03, 0xF0, 0x3F,
  0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xCF, 0xF8, 0x03, 0xFE, 0x00, 0x1F, 0xF0, 0x01, 0xFF,
  0xC0, 0x0F, 0xFF, 0x00, 0xFD, 0xF8, 0x07, 0xC7, 0xE0, 0x7E, 0x3F, 0x03, 0xF0, 0xFC, 0x3F, 0x07,
  0xE1, 0xF8, 0x1F, 0x9F, 0x80, 0xFD, 0xFC, 0x03, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x80, 0xFC, 0x3F, 0x1F, 0x87, 0xE1, 0xF0, 0x7C, 0x3E, 0x07,
  0xFE, 0x00, 0x7F, 0xFE, 0x03, 0xFF, 0xFC, 0x0F, 0xFF, 0xF0, 0x7F, 0x0F, 0xE1, 0xF8, 0x0F, 0x81,
  0xC0, 0x3E, 0x00, 0x00, 0xF8, 0x00, 0x03, 0xE0, 0x3F, 0xFF, 0x83, 0xFF, 0xFE, 0x1F, 0xFF, 0xF8,
  0xFF, 0xFF, 0xE3, 0xF0, 0x0F, 0x8F, 0x80, 0x3E, 0x3E, 0x00, 0xF8, 0xF8, 0x07, 0xF3, 0xF0, 0x3F,
  0xFF, 0xFF, 0xFF, 0xDF, 0xFF, 0x7F, 0x3F, 0xF9, 0xFC, 0x7F, 0xC1, 0xF0, 0x18, 0x00, 0x00, 0xFC,
  0x00, 0x01, 0xF8, 0x00, 0x03, 0xF0, 0x00, 0x07, 0xE0, 0x00, 0x0F, 0xC0, 0x00, 0x1F, 0x80, 0x00,
  0x3F, 0x00, 0x00, 0x7E, 0x00, 0x00, 0xFC, 0x00, 0x01, 0xF8, 0xFF, 0x03, 0xF7, 0xFF, 0x87, 0xEF,
  0xFF, 0x8F, 0xFF, 0xFF, 0x9F, 0xFD, 0xFF, 0xBF, 0xC0, 0x7F, 0x7F, 0x00, 0x7E, 0xFC, 0x00, 0x7F,
  0xF8, 0x00, 0xFF, 0xF0, 0x01, 0xFF, 0xE0, 0x03, 0xFF, 0xC0, 0x07, 0xFF, 0x80, 0x0F, 0xFF, 0x00,
  0x1F, 0xFE, 0x00, 0x3F, 0xFE, 0x00, 0xFF, 0xFE, 0x03, 0xFB, 0xFF, 0x1F, 0xF7, 0xFF, 0xFF, 0xCF,
  0xFF, 0xFF, 0x1F, 0xBF, 0xFC, 0x3F, 0x3F, 0xF0, 0x00, 0x06, 0x00, 0x03, 0xFF, 0x00, 0x1F, 0xFF,
  0x01, 0xFF, 0xFE, 0x0F, 0xFF, 0xFC, 0x3F, 0xFF, 0xF9, 0xFC, 0x07, 0xE7, 0xE0, 0x0F, 0xFF, 0x00,
  0x3F, 0xFC, 0x00, 0x73, 0xF0, 0x00, 0x0F, 0xC0, 0x00, 0x3F, 0x00, 0x00, 0xFC, 0x00, 0x03, 0xF0,
  0x01, 0x8F, 0xC0, 0x0F, 0xDF, 0x80, 0x3F, 0x7F, 0x01, 0xFC, 0xFF, 0x1F, 0xE3, 0xFF, 0xFF, 0x07,
  0xFF, 0xF8, 0x0F, 0xFF, 0xC0, 0x0F, 0xFE, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x7E, 0x00, 0x00,
  0xFC, 0x00, 0x01, 0xF8, 0x00, 0x03, 0xF0, 0x00, 0x07, 0xE0, 0x00, 0x0F, 0xC0, 0x00, 0x1F, 0x80,
  0x00, 0x3F, 0x00, 0x00, 0x7E, 0x07, 0xF8, 0xFC, 0x3F, 0xFD, 0xF8, 0xFF, 0xFF, 0xF3, 0xFF, 0xFF,
  0xEF, 0xFD, 0xFF, 0xDF, 0xC0, 0x7F, 0xBF, 0x00, 0x7F, 0xFC, 0x00, 0x7F, 0xF8, 0x00, 0xFF, 0xF0,
  0x01, 0xFF, 0xE0, 0x01, 0xFF, 0xC0, 0x03, 0xFF, 0x80, 0x0F, 0xFF, 0x00, 0x1F, 0xFE, 0x00, 0x3F,
  0x7E, 0x00, 0xFE, 0xFE, 0x03, 0xFD, 0xFF, 0x1F, 0xF9, 0xFF, 0xFF, 0xF1, 0xFF, 0xFB, 0xE1, 0xFF,
  0xE7, 0xC1, 0xFF, 0x8F, 0x80, 0x30, 0x00, 0x03, 0xFF, 0x00, 0x3F, 0xFE, 0x01, 0xFF, 0xFC, 0x0F,
  0xFF, 0xF8, 0x3F, 0xCF, 0xF1, 0xF8, 0x0F, 0xC7, 0xC0, 0x1F, 0xBF, 0x00, 0x3E, 0xFC, 0x00, 0xFB,
  0xFF, 0xFF, 0xEF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0x0F, 0xC0, 0x00,
  0x1F, 0x80, 0x30, 0x7E, 0x01, 0xF0, 0xFF, 0x1F, 0xE3, 0xFF, 0xFF, 0x07, 0xFF, 0xF8, 0x0F, 0xFF,
  0xC0, 0x0F, 0xFC, 0x00, 0x03, 0x00, 0x00, 0x00, 0x7F, 0x00, 0xFF, 0x80, 0xFF, 0xC0, 0x7F, 0xE0,
  0x3F, 0xF0, 0x1F, 0x00, 0x0F, 0x80, 0x07, 0xC0, 0x03, 0xE0, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0x0F, 0x80, 0x07, 0xC0, 0x03, 0xE0, 0x01, 0xF0, 0x00, 0xF8, 0x00,
  0x7C, 0x00, 0x3E, 0x00, 0x1F, 0x00, 0x0F, 0x80, 0x07, 0xC0, 0x03, 0xE0, 0x01, 0xF0, 0x00, 0xF8,
  0x00, 0x7C, 0x00, 0x3E, 0x00, 0x1F, 0x00, 0x0F, 0x80, 0x03, 0xFC, 0x3E, 0x1F, 0xFE, 0x7C, 0x7F,
  0xFE, 0xF9, 0xFF, 0xFF, 0xF7, 0xFE, 0xFF, 0xEF, 0xE0, 0x3F, 0xDF, 0x80, 0x3F, 0xFE, 0x00, 0x3F,
  0xFC, 0x00, 0x7F, 0xF8, 0x00, 0xFF, 0xF0, 0x00, 0xFF, 0xE0, 0x03, 0xFF, 0xC0, 0x07, 0xFF, 0x80,
  0x0F, 0xFF, 0x00, 0x1F, 0xBF, 0x00, 0x7F, 0x7F, 0x01, 0xFE, 0x7F, 0xFF, 0xFC, 0xFF, 0xFF, 0xF8,
  0xFF, 0xFB, 0xF0, 0x7F, 0xE7, 0xE0, 0x7F, 0x8F, 0xC0, 0x00, 0x1F, 0x80, 0x00, 0x3F, 0x00, 0x00,
  0x7E, 0x00, 0x00, 0xFC, 0x3F, 0xFF, 0xF8, 0x7F, 0xFF, 0xF0, 0xFF, 0xFF, 0xC1, 0xFF, 0xFF, 0x83,
  0xFF, 0xFC, 0x00, 0xFC, 0x00, 0x07, 0xE0, 0x00, 0x3F, 0x00, 0x01, 0xF8, 0x00, 0x0F, 0xC0, 0x00,
  0x7E, 0x00, 0x03, 0xF0, 0x00, 0x1F, 0x80, 0x00, 0xFC, 0x00, 0x07, 0xE3, 0xF8, 0x3F, 0x7F, 0xF1,
  0xFB, 0xFF, 0xCF, 0xFF, 0xFF, 0x7F, 0xFF, 0xFB, 0xFC, 0x0F, 0xFF, 0xC0, 0x3F, 0xFC, 0x01, 0xFF,
  0xE0, 0x07, 0xFF, 0x00, 0x3F, 0xF8, 0x01, 0xFF, 0xC0, 0x0F, 0xFE, 0x00, 0x7F, 0xF0, 0x03, 0xFF,
  0x80, 0x1F, 0xFC, 0x00, 0xFF, 0xE0, 0x07, 0xFF, 0x00, 0x3F, 0xF8, 0x01, 0xFF, 0xC0, 0x0F, 0xFE,
  0x00, 0x7F, 0xF0, 0x03, 0xE0, 0x7C, 0xFF, 0xFF, 0xF7, 0xEF, 0xC6, 0x00, 0x00, 0xFD, 0xFB, 0xF7,
  0xEF, 0xDF, 0xBF, 0x7E, 0xFD, 0xFB, 0xF7, 0xEF, 0xDF, 0xBF, 0x7E, 0xFD, 0xFB, 0xF7, 0xEF, 0xDF,
  0x80, 0x0F, 0x81, 0xF8, 0x7F, 0x0F, 0xF0, 0xFC, 0x1F, 0x80, 0xC0, 0x00, 0x00, 0x01, 0xF8, 0x3F,
  0x07, 0xE0, 0xFC, 0x1F, 0x83, 0xF0, 0x7E, 0x0F, 0xC1, 0xF8, 0x3F, 0x07, 0xE0, 0xFC, 0x1F, 0x83,
  0xF0, 0x7E, 0x0F, 0xC1, 0xF8, 0x3F, 0x07, 0xE0, 0xFC, 0x1F, 0x83, 0xF0, 0x7E, 0x0F, 0xC1, 0xF8,
  0x3F, 0x7F, 0xEF, 0xFD, 0xFF, 0x3F, 0xC7, 0xF0, 0xFC, 0x00, 0x07, 0xE0, 0x00, 0x3F, 0x00, 0x01,
  0xF8, 0x00, 0x0F, 0xC0, 0x00, 0x7E, 0x00, 0x03, 0xF0, 0x00, 0x1F, 0x80, 0x00, 0xFC, 0x00, 0x07,
  0xE0, 0x3F, 0xFF, 0x03, 0xFD, 0xF8, 0x1F, 0xCF, 0xC1, 0xFC, 0x7E, 0x1F, 0xC3, 0xF1, 0xFC, 0x1F,
  0x9F, 0xC0, 0xFD, 0xFC, 0x07, 0xFF, 0xC0, 0x3F, 0xFC, 0x01, 0xFF, 0xC0, 0x0F, 0xFE, 0x00, 0x7F,
  0xF8, 0x03, 0xF7, 0xE0, 0x1F, 0xBF, 0x80, 0xFC, 0xFE, 0x07, 0xE3, 0xF8, 0x3F, 0x0F, 0xE1, 0xF8,
  0x3F, 0x8F, 0xC0, 0xFE, 0x7E, 0x03, 0xFB, 0xF0, 0x0F, 0xE0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xC0, 0xFC, 0xFE, 0x07, 0xF0, 0xFD, 0xFF, 0x0F, 0xF8, 0xFF, 0xFF, 0x9F, 0xFC, 0xFF, 0xFF,
  0xFF, 0xFE, 0xFF, 0xBF, 0xFD, 0xFE, 0xFE, 0x0F, 0xF0, 0x7E, 0xFC, 0x07, 0xE0, 0x3F, 0xFC, 0x07,
  0xE0, 0x3F, 0xFC, 0x07, 0xE0, 0x3F, 0xFC, 0x07, 0xE0, 0x3F, 0xFC, 0x07, 0xE0, 0x3F, 0xFC, 0x07,
  0xE0, 0x3F, 0xFC, 0x07, 0xE0, 0x3F, 0xFC, 0x07, 0xE0, 0x3F, 0xFC, 0x07, 0xE0, 0x3F, 0xFC, 0x07,
  0xE0, 0x3F, 0xFC, 0x07, 0xE0, 0x3F, 0xFC, 0x07, 0xE0, 0x3F, 0xFC, 0x07, 0xE0, 0x3F, 0xFC, 0x07,
  0xE0, 0x3F, 0xFC, 0x07, 0xE0, 0x3F, 0xFC, 0x07, 0xE0, 0x3F, 0xFC, 0x7F, 0x07, 0xEF, 0xFE, 0x3F,
  0x7F, 0xF9, 0xFF, 0xFF, 0xEF, 0xFF, 0xFF, 0x7F, 0x81, 0xFF, 0xF8, 0x07, 0xFF, 0x80, 0x3F, 0xFC,
  0x00, 0xFF, 0xE0, 0x07, 0xFF, 0x00, 0x3F, 0xF8, 0x01, 0xFF, 0xC0, 0x0F, 0xFE, 0x00, 0x7F, 0xF0,
  0x03, 0xFF, 0x80, 0x1F, 0xFC, 0x00, 0xFF, 0xE0, 0x07, 0xFF, 0x00, 0x3F, 0xF8, 0x01, 0xFF, 0xC0,
  0x0F, 0xFE, 0x00, 0x7C, 0x03, 0xFF, 0x00, 0x0F, 0xFF, 0x80, 0x7F, 0xFF, 0xC1, 0xFF, 0xFF, 0xC3,
  0xFF, 0xFF, 0x8F, 0xE0, 0x3F, 0x9F, 0x80, 0x3F, 0x7E, 0x00, 0x3F, 0xFC, 0x00, 0x7F, 0xF8, 0x00,
  0xFF, 0xF0, 0x01, 0xFF, 0xE0, 0x03, 0xFF, 0xC0, 0x07, 0xFF, 0x80, 0x0F, 0xFF, 0x00, 0x1F, 0xBF,
  0x00, 0x7E, 0x7F, 0x01, 0xFC, 0x7F, 0x8F, 0xF0, 0xFF, 0xFF, 0xE0, 0xFF, 0xFF, 0x80, 0xFF, 0xFE,
  0x00, 0x7F, 0xF0, 0x00, 0x1E, 0x00, 0x00, 0xFC, 0x7F, 0x81, 0xFB, 0xFF, 0xC3, 0xF7, 0xFF, 0xC7,
  0xFF, 0xFF, 0xCF, 0xFE, 0xFF, 0xDF, 0xE0, 0x3F, 0xBF, 0x80, 0x3F, 0x7E, 0x00, 0x3F, 0xFC, 0x00,
  0x7F, 0xF8, 0x00, 0xFF, 0xF0, 0x01, 0xFF, 0xE0, 0x03, 0xFF, 0xC0, 0x07, 0xFF, 0x80, 0x0F, 0xFF,
  0x00, 0x1F, 0xFF, 0x00, 0x7F, 0xFF, 0x01, 0xFD, 0xFF, 0x8F, 0xFB, 0xFF, 0xFF, 0xE7, 0xFF, 0xFF,
  0x8F, 0xDF, 0xFE, 0x1F, 0x9F, 0xF8, 0x3F, 0x03, 0x00, 0x7E, 0x00, 0x00, 0xFC, 0x00, 0x01, 0xF8,
  0x00, 0x03, 0xF0, 0x00, 0x07, 0xE0, 0x00, 0x0F, 0xC0, 0x00, 0x1F, 0x80, 0x00, 0x3F, 0x00, 0x00,
  0x00, 0x03, 0xFC, 0x3E, 0x1F, 0xFE, 0x7C, 0x7F, 0xFE, 0xF9, 0xFF, 0xFF, 0xF7, 0xFE, 0xFF, 0xEF,
  0xE0, 0x3F, 0xDF, 0x80, 0x3F, 0xFE, 0x00, 0x3F, 0xFC, 0x00, 0x7F, 0xF8, 0x00, 0xFF, 0xF0, 0x00,
  0xFF, 0xE0, 0x01, 0xFF, 0xC0, 0x07, 0xFF, 0x80, 0x0F, 0xFF, 0x00, 0x1F, 0xBF, 0x00, 0x7F, 0x7F,
  0x01, 0xFE, 0xFF, 0x8F, 0xFC, 0xFF, 0xFF, 0xF8, 0xFF, 0xFF, 0xF0, 0xFF, 0xF7, 0xE0, 0xFF, 0xCF,
  0xC0, 0x18, 0x1F, 0x80, 0x00, 0x3F, 0x00, 0x00, 0x7E, 0x00, 0x00, 0xFC, 0x00, 0x01, 0xF8, 0x00,
  0x03, 0xF0, 0x00, 0x07, 0xE0, 0x00, 0x0F, 0xC0, 0x00, 0x1F, 0x80, 0xFC, 0xFF, 0xEF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0x83, 0xF8, 0x1F, 0x80, 0xFC, 0x07, 0xE0, 0x3F, 0x01, 0xF8, 0x0F, 0xC0,
  0x7E, 0x03, 0xF0, 0x1F, 0x80, 0xFC, 0x07, 0xE0, 0x3F, 0x01, 0xF8, 0x0F, 0xC0, 0x7E, 0x00, 0x0F,
  0xFC, 0x03, 0xFF, 0xE0, 0xFF, 0xFE, 0x3F, 0xFF, 0xE7, 0xE0, 0xFC, 0xF8, 0x0F, 0xDF, 0x01, 0x83,
  0xF0, 0x00, 0x7F, 0xC0, 0x07, 0xFF, 0x80, 0xFF, 0xFE, 0x07, 0xFF, 0xE0, 0x3F, 0xFE, 0x00, 0x3F,
  0xC2, 0x01, 0xFF, 0xC0, 0x1F, 0xFC, 0x03, 0xFF, 0xC0, 0xFD, 0xFF, 0xFF, 0x9F, 0xFF, 0xE1, 0xFF,
  0xF8, 0x1F, 0xFE, 0x00, 0x18, 0x00, 0x03, 0xE0, 0x01, 0xF0, 0x00, 0xF8, 0x00, 0x7C, 0x00, 0x3E,
  0x00, 0x1F, 0x00, 0x0F, 0x81, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0,
  0x3E, 0x00, 0x1F, 0x00, 0x0F, 0x80, 0x07, 0xC0, 0x03, 0xE0, 0x01, 0xF0, 0x00, 0xF8, 0x00, 0x7C,
  0x00, 0x3E, 0x00, 0x1F, 0x00, 0x0F, 0x80, 0x07, 0xC0, 0x03, 0xFF, 0x81, 0xFF, 0xC0, 0xFF, 0xE0,
  0x3F, 0xF0, 0x0F, 0xF8, 0xF8, 0x01, 0xFF, 0xC0, 0x0F, 0xFE, 0x00, 0x7F, 0xF0, 0x03, 0xFF, 0x80,
  0x1F, 0xFC, 0x00, 0xFF, 0xE0, 0x07, 0xFF, 0x00, 0x3F, 0xF8, 0x01, 0xFF, 0xC0, 0x0F, 0xFE, 0x00,
  0x7F, 0xF0, 0x03, 0xFF, 0x80, 0x1F, 0xFC, 0x00, 0xFF, 0xF0, 0x07, 0xFF, 0x80, 0x7F, 0xFE, 0x03,
  0xFB, 0xFC, 0xFF, 0xDF, 0xFF, 0xFE, 0x7F, 0xFD, 0xF1, 0xFF, 0xCF, 0x87, 0xF8, 0x7C, 0xFC, 0x00,
  0xFF, 0xF0, 0x03, 0xFF, 0xC0, 0x0F, 0xDF, 0x80, 0x3E, 0x7E, 0x01, 0xF9, 0xF8, 0x07, 0xE3, 0xE0,
  0x1F, 0x0F, 0xC0, 0xFC, 0x3F, 0x03, 0xF0, 0x7C, 0x0F, 0x81, 0xF8, 0x3E, 0x07, 0xE1, 0xF8, 0x0F,
  0x87, 0xC0, 0x3E, 0x1F, 0x00, 0xFC, 0xFC, 0x01, 0xF3, 0xF0, 0x07, 0xCF, 0x80, 0x1F, 0xBE, 0x00,
  0x3F, 0xF8, 0x00, 0xFF, 0xC0, 0x03, 0xFF, 0x00, 0x0F, 0xFC, 0x00, 0xF8, 0x1F, 0xF0, 0x7F, 0xF0,
  0x3F, 0xE0, 0xFF, 0xF0, 0xFF, 0xC1, 0xF7, 0xE1, 0xFF, 0x83, 0xEF, 0xC3, 0xEF, 0x07, 0xCF, 0x87,
  0xDF, 0x0F, 0x9F, 0x0F, 0x3E, 0x1F, 0x3E, 0x1E, 0x7C, 0x7E, 0x7C, 0x7C, 0xF8, 0xFC, 0xF8, 0xF8,
  0xF1, 0xF1, 0xF1, 0xF1, 0xE3, 0xE3, 0xE3, 0xE3, 0xC7, 0xC7, 0xE7, 0xC7, 0xCF, 0x87, 0xCF, 0x0F,
  0x9F, 0x0F, 0x9E, 0x1F, 0x3E, 0x1F, 0x7C, 0x3E, 0x7C, 0x3E, 0xF8, 0x3D, 0xF0, 0x7D, 0xF0, 0x7B,
  0xE0, 0xFF, 0xE0, 0xFF, 0xC1, 0xFF, 0x81, 0xFF, 0x81, 0xFF, 0x03, 0xFF, 0x03, 0xFE, 0x07, 0xFE,
  0x00, 0xFE, 0x00, 0x7F, 0x7F, 0x00, 0xFE, 0x3F, 0x00, 0xFC, 0x1F, 0x81, 0xF8, 0x1F, 0xC3, 0xF8,
  0x0F, 0xC3, 0xF0, 0x07, 0xE7, 0xE0, 0x03, 0xFF, 0xE0, 0x03, 0xFF, 0xC0, 0x01, 0xFF, 0x80, 0x00,
  0xFF, 0x00, 0x00, 0xFF, 0x00, 0x01, 0xFF, 0x80, 0x03, 0xFF, 0xC0, 0x03, 0xFF, 0xE0, 0x07, 0xE7,
  0xE0, 0x0F, 0xC3, 0xF0, 0x1F, 0xC3, 0xF8, 0x1F, 0x81, 0xF8, 0x3F, 0x00, 0xFC, 0x7F, 0x00, 0xFE,
  0xFE, 0x00, 0x7F, 0xF8, 0x01, 0xFF, 0xC0, 0x0F, 0xFE, 0x00, 0x7F, 0xF0, 0x03, 0xFF, 0x80, 0x1F,
  0xFC, 0x00, 0xFF, 0xE0, 0x07, 0xFF, 0x00, 0x3F, 0xF8, 0x01, 0xFF, 0xC0, 0x0F, 0xFE, 0x00, 0x7F,
  0xF0, 0x03, 0xFF, 0x80, 0x1F, 0xFC, 0x00, 0xFF, 0xF0, 0x07, 0xFF, 0x80, 0x7F, 0xFE, 0x03, 0xFB,
  0xFC, 0xFF, 0xDF, 0xFF, 0xFE, 0x7F, 0xFD, 0xF1, 0xFF, 0xCF, 0x87, 0xFC, 0x7C, 0x00, 0x03, 0xE0,
  0x00, 0x1F, 0x00, 0x00, 0xF8, 0x00, 0x0F, 0xCF, 0xFF, 0xFE, 0x7F, 0xFF, 0xF3, 0xFF, 0xFF, 0x1F,
  0xFF, 0xF8, 0xFF, 0xFF, 0x00, 0x7F, 0xFF, 0xCF, 0xFF, 0xF9, 0xFF, 0xFF, 0x3F, 0xFF, 0xE7, 0xFF,
  0xFC, 0x00, 0x1F, 0x80, 0x07, 0xF0, 0x01, 0xFC, 0x00, 0x7F, 0x00, 0x3F, 0xC0, 0x0F, 0xF0, 0x03,
  0xFC, 0x00, 0xFE, 0x00, 0x3F, 0x80, 0x1F, 0xE0, 0x07, 0xF8, 0x00, 0xFE, 0x00, 0x1F, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC0, 0x00, 0xFF, 0xC0, 0x7F, 0xF0, 0x3F,
  0xFC, 0x0F, 0xFF, 0x03, 0xF0, 0x00, 0xF8, 0x00, 0x3E, 0x00, 0x0F, 0x80, 0x03, 0xE0, 0x00, 0xF8,
  0x00, 0x3E, 0x00, 0x0F, 0x80, 0x03, 0xE0, 0x00, 0xF8, 0x00, 0x3E, 0x00, 0x0F, 0x80, 0x03, 0xE0,
  0x00, 0xF8, 0x00, 0x3E, 0x03, 0xFF, 0x80, 0xFF, 0xC0, 0x3F, 0x80, 0x0F, 0xF8, 0x03, 0xFF, 0x80,
  0x07, 0xE0, 0x00, 0xF8, 0x00, 0x3E, 0x00, 0x0F, 0x80, 0x03, 0xE0, 0x00, 0xF8, 0x00, 0x3E, 0x00,
  0x0F, 0x80, 0x03, 0xE0, 0x00, 0xF8, 0x00, 0x3E, 0x00, 0x0F, 0x80, 0x03, 0xE0, 0x00, 0xF8, 0x00,
  0x3E, 0x00, 0x0F, 0xFF, 0x03, 0xFF, 0xC0, 0xFF, 0xF0, 0x1F, 0xFC, 0x03, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0xFF, 0xC0, 0x3F, 0xF8, 0x0F, 0xFE,
  0x03, 0xFF, 0xC0, 0x07, 0xF0, 0x00, 0xFC, 0x00, 0x3F, 0x00, 0x0F, 0xC0, 0x03, 0xF0, 0x00, 0xFC,
  0x00, 0x3F, 0x00, 0x0F, 0xC0, 0x03, 0xF0, 0x00, 0xFC, 0x00, 0x3F, 0x00, 0x0F, 0xC0, 0x03, 0xF0,
  0x00, 0xFC, 0x00, 0x3F, 0x00, 0x07, 0xFF, 0x01, 0xFF, 0xC0, 0x0F, 0xF0, 0x0F, 0xFC, 0x07, 0xFF,
  0x03, 0xF8, 0x00, 0xFC, 0x00, 0x3F, 0x00, 0x0F, 0xC0, 0x03, 0xF0, 0x00, 0xFC, 0x00, 0x3F, 0x00,
  0x0F, 0xC0, 0x03, 0xF0, 0x00, 0xFC, 0x00, 0x3F, 0x00, 0x0F, 0xC0, 0x03, 0xF0, 0x00, 0xFC, 0x00,
  0x3F, 0x03, 0xFF, 0xC0, 0xFF, 0xF0, 0x3F, 0xF8, 0x0F, 0xFC, 0x03, 0xFE, 0x00, 0x01, 0xF0, 0x18,
  0x0F, 0xF0, 0x78, 0x3F, 0xE3, 0xF0, 0xFF, 0xFF, 0xF7, 0xFF, 0xFF, 0xDF, 0xFF, 0xFF, 0x1F, 0x8F,
  0xFC, 0x3E, 0x1F, 0xE0, 0x30, 0x1F, 0x80, 0x40, 0x00, 0x00,
};
const GFXglyph BoldReadoutGlyphs[] PROGMEM = {
  {0, 1, 1, 11, 0, 0},   // 0x20  
  {1, 9, 32, 13, 2, -30},   // 0x21 !
  {37, 18, 14, 23, 2, -31},   // 0x22 "
  {69, 24, 31, 28, 2, -30},   // 0x23 #
  {162, 24, 41, 27, 1, -35},   // 0x24 $
  {285, 31, 32, 33, 1, -30},   // 0x25 %
  {409, 24, 31, 26, 1, -30},   // 0x26 &
  {502, 9, 14, 13, 2, -31},   // 0x27 '
  {518, 15, 44, 18, 2, -32},   // 0x28 (
  {601, 15, 44, 17, 0, -32},   // 0x29 )
  {684, 20, 20, 24, 2, -30},   // 0x2A *
  {734, 19, 19, 27, 4, -24},   // 0x2B +
  {780, 9, 14, 13, 2, -7},   // 0x2C ,
  {796, 13, 5, 19, 3, -13},   // 0x2D -
  {805, 9, 9, 13, 2, -7},   // 0x2E .
  {816, 19, 40, 17, -1, -30},   // 0x2F /
  {911, 24, 32, 29, 2, -30},   // 0x30 0
  {1007, 16, 31, 20, 1, -30},   // 0x31 1
  {1069, 22, 31, 26, 2, -30},   // 0x32 2
  {1155, 24, 32, 27, 1, -30},   // 0x33 3
  {1251, 26, 31, 28, 1, -30},   // 0x34 4
  {1352, 23, 32, 26, 2, -30},   // 0x35 5
  {1444, 23, 32, 27, 2, -30},   // 0x36 6
  {1536, 22, 31, 24, 1, -30},   // 0x37 7
  {1622, 24, 32, 26, 1, -30},   // 0x38 8
  {1718, 23, 32, 27, 2, -30},   // 0x39 9
  {1810, 9, 24, 13, 2, -22},   // 0x3A :
  {1837, 9, 29, 13, 2, -22},   // 0x3B ;
  {1870, 19, 19, 27, 4, -24},   // 0x3C <
  {1916, 19, 13, 27, 4, -21},   // 0x3D =
  {1947, 19, 19, 27, 4, -24},   // 0x3E >
  {1993, 23, 32, 25, 1, -30},   // 0x3F ?
  {2085, 39, 40, 45, 3, -30},   // 0x40 @
  {2280, 26, 31, 28, 1, -30},   // 0x41 A
  {2381, 25, 31, 29, 2, -30},   // 0x42 B
  {2478, 24, 32, 28, 2, -30},   // 0x43 C
  {2574, 25, 31, 29, 2, -30},   // 0x44 D
  {2671, 20, 31, 24, 3, -30},   // 0x45 E
  {2749, 19, 31, 24, 3, -30},   // 0x46 F
  {2823, 24, 32, 29, 2, -30},   // 0x47 G
  {2919, 23, 31, 29, 3, -30},   // 0x48 H
  {3009, 6, 31, 12, 3, -30},   // 0x49 I
  {3033, 24, 32, 27, 1, -30},   // 0x4A J
  {3129, 24, 31, 28, 3, -30},   // 0x4B K
  {3222, 20, 31, 24, 3, -30},   // 0x4C L
  {3300, 33, 31, 39, 3, -30},   // 0x4D M
  {3428, 24, 31, 29, 3, -30},   // 0x4E N
  {3521, 26, 32, 30, 2, -30},   // 0x4F O
  {3625, 22, 31, 27, 3, -30},   // 0x50 P
  {3711, 26, 39, 30, 2, -30},   // 0x51 Q
  {3838, 23, 31, 28, 3, -30},   // 0x52 R
  {3928, 24, 32, 27, 1, -30},   // 0x53 S
  {4024, 24, 31, 26, 1, -30},   // 0x54 T
  {4117, 24, 32, 30, 3, -30},   // 0x55 U
  {4213, 25, 31, 27, 1, -30},   // 0x56 V
  {4310, 37, 31, 40, 1, -30},   // 0x57 W
  {4454, 26, 31, 28, 1, -30},   // 0x58 X
  {4555, 27, 31, 27, 0, -30},   // 0x59 Y
  {4660, 21, 31, 25, 2, -30},   // 0x5A Z
  {4742, 12, 44, 16, 3, -32},   // 0x5B [
  {4808, 19, 40, 17, -1, -30},   // 0x5C backslash
  {4903, 12, 44, 16, 1, -32},   // 0x5D ]
  {4969, 21, 12, 27, 3, -30},   // 0x5E ^
  {5001, 21, 5, 27, 3, 3},   // 0x5F _
  {5015, 9, 7, 13, 2, -30},   // 0x60 `
  {5023, 22, 23, 25, 2, -21},   // 0x61 a
  {5087, 23, 32, 28, 3, -30},   // 0x62 b
  {5179, 22, 23, 26, 2, -21},   // 0x63 c
  {5243, 23, 32, 28, 2, -30},   // 0x64 d
  {5335, 22, 23, 25, 2, -21},   // 0x65 e
  {5399, 17, 31, 19, 1, -30},   // 0x66 f
  {5465, 23, 31, 28, 2, -21},   // 0x67 g
  {5555, 21, 31, 27, 3, -30},   // 0x68 h
  {5637, 7, 31, 12, 2, -30},   // 0x69 i
  {5665, 11, 40, 12, -1, -30},   // 0x6A j
  {5720, 21, 31, 25, 3, -30},   // 0x6B k
  {5802, 6, 31, 12, 3, -30},   // 0x6C l
  {5826, 32, 22, 38, 3, -21},   // 0x6D m
  {5914, 21, 22, 27, 3, -21},   // 0x6E n
  {5972, 23, 23, 27, 2, -21},   // 0x6F o
  {6039, 23, 31, 28, 3, -21},   // 0x70 p
  {6129, 23, 31, 28, 2, -21},   // 0x71 q
  {6219, 13, 22, 17, 3, -21},   // 0x72 r
  {6255, 19, 23, 23, 2, -21},   // 0x73 s
  {6310, 17, 29, 20, 1, -28},   // 0x74 t
  {6372, 21, 22, 27, 3, -21},   // 0x75 u
  {6430, 22, 22, 24, 1, -21},   // 0x76 v
  {6491, 31, 22, 35, 2, -21},   // 0x77 w
  {6577, 24, 22, 26, 1, -21},   // 0x78 x
  {6643, 21, 31, 27, 3, -21},   // 0x79 y
  {6725, 19, 22, 23, 2, -21},   // 0x7A z
  {6778, 18, 44, 21, 2, -32},   // 0x7B {
  {6877, 5, 46, 11, 3, -34},   // 0x7C |
  {6906, 18, 44, 21, 1, -32},   // 0x7D }
  {7005, 23, 10, 27, 2, -19},   // 0x7E ~
};
const GFXfont BoldReadout PROGMEM = {(uint8_t*)BoldReadoutBitmaps, (GFXglyph*)BoldReadoutGlyphs, 0x20, 0x7E, 56};

// BoldBig: generated from sg-700.ttf at 23px
const uint8_t BoldBigBitmaps[] PROGMEM = {
  0x00, 0x73, 0x9C, 0xE7, 0x39, 0xCE, 0x73, 0x9C, 0x07, 0x7F, 0xEE, 0x73, 0xBD, 0xFF, 0xFD, 0xEF,
  0x18, 0xC4, 0x37, 0x38, 0x38, 0xC1, 0xC6, 0x0E, 0x30, 0x71, 0x8F, 0xFF, 0xFF, 0xFF, 0xFF, 0xE7,
  0x18, 0x38, 0xC7, 0xFF, 0xFF, 0xFF, 0xFF, 0xF3, 0x8C, 0x1C, 0x60, 0xE3, 0x07, 0x18, 0x06, 0x00,
  0x60, 0x06, 0x03, 0xFC, 0x7F, 0xE7, 0x7E, 0xE6, 0x7E, 0x67, 0xF6, 0x77, 0xE0, 0x3F, 0xC0, 0xFE,
  0x07, 0xFE, 0x67, 0xE6, 0x7E, 0x67, 0xFF, 0xF7, 0xFE, 0x1F, 0xC0, 0x60, 0x06, 0x00, 0x60, 0x7C,
  0x0C, 0xFE, 0x1E, 0xC6, 0x1C, 0xC6, 0x38, 0xCE, 0x70, 0xFC, 0xE0, 0x39, 0xC0, 0x01, 0xC0, 0x03,
  0x80, 0x07, 0x3C, 0x0E, 0x7E, 0x1C, 0x67, 0x18, 0xE3, 0x38, 0xE7, 0x70, 0x7E, 0x60, 0x3E, 0x3E,
  0x07, 0xE0, 0xF0, 0x0E, 0x00, 0xE0, 0x0E, 0x1C, 0xF1, 0xC3, 0xFF, 0x7F, 0xFE, 0x1C, 0xE1, 0xCE,
  0x1C, 0xE1, 0xCF, 0x1C, 0x7F, 0xC3, 0xFC, 0x77, 0xBE, 0xF1, 0x89, 0xC0, 0x0F, 0x3F, 0x7C, 0x70,
  0x70, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0x60, 0x70, 0x78,
  0x3F, 0x1F, 0x03, 0xF0, 0xFC, 0x3E, 0x0E, 0x0E, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
  0x07, 0x07, 0x07, 0x07, 0x06, 0x0E, 0x1E, 0xFC, 0xF8, 0xC0, 0x0C, 0x1B, 0x27, 0x58, 0xFC, 0xFF,
  0xFF, 0xF1, 0xF0, 0xD6, 0x6D, 0x83, 0x00, 0x0C, 0x03, 0x00, 0xC0, 0x30, 0xFF, 0xFF, 0xF0, 0xC0,
  0x30, 0x0C, 0x03, 0x00, 0x77, 0xBE, 0xF1, 0x89, 0xC0, 0xFF, 0xF0, 0x77, 0xFE, 0xE0, 0x03, 0x81,
  0xC0, 0xC0, 0xE0, 0x70, 0x30, 0x38, 0x1C, 0x0C, 0x0E, 0x07, 0x03, 0x83, 0x81, 0xC0, 0xE0, 0xE0,
  0x70, 0x38, 0x38, 0x1C, 0x0E, 0x00, 0x1F, 0xC1, 0xFF, 0x1E, 0x3D, 0xE0, 0xEE, 0x03, 0xF0, 0x1F,
  0x80, 0xFC, 0x07, 0xE0, 0x3F, 0x01, 0xF8, 0x0F, 0xC0, 0x7F, 0x07, 0x3C, 0x78, 0xFF, 0x83, 0xF8,
  0x1F, 0x8F, 0xCE, 0xE7, 0x77, 0x3B, 0x9F, 0xCE, 0x07, 0x03, 0x81, 0xC0, 0xE0, 0x70, 0x38, 0x1C,
  0x0E, 0x07, 0x3F, 0x87, 0xFE, 0x71, 0xEE, 0x07, 0xE0, 0x70, 0x07, 0x00, 0xF0, 0x1E, 0x0F, 0xC3,
  0xF8, 0x7E, 0x0F, 0x00, 0xE0, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x1F,
  0x07, 0xC1, 0xF0, 0x1E, 0x01, 0xFC, 0x1F, 0xE0, 0x0F, 0x00, 0x7E, 0x07, 0xE0, 0x7F, 0x9F, 0x7F,
  0xE3, 0xFC, 0x07, 0xC0, 0x7E, 0x03, 0xB0, 0x39, 0x83, 0x8C, 0x1C, 0x61, 0xC3, 0x0E, 0x18, 0xE0,
  0xC7, 0x06, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0x80, 0x60, 0x03, 0x00, 0x18, 0xFF, 0xEF, 0xFE, 0xFF,
  0xEE, 0x00, 0xE0, 0x0E, 0x70, 0xFF, 0xCF, 0xFE, 0xF0, 0xF0, 0x07, 0x00, 0x7E, 0x07, 0xE0, 0x7F,
  0x9E, 0x7F, 0xE3, 0xF8, 0x1F, 0xC7, 0xFE, 0x78, 0xFF, 0x07, 0xE0, 0x7E, 0x00, 0xEF, 0xCF, 0xFE,
  0xF0, 0xFE, 0x07, 0xE0, 0x7E, 0x07, 0xF0, 0x77, 0x8F, 0x7F, 0xE1, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF,
  0x80, 0x70, 0x1E, 0x03, 0x80, 0xE0, 0x1C, 0x07, 0x01, 0xE0, 0x38, 0x0F, 0x01, 0xC0, 0x78, 0x0E,
  0x01, 0xC0, 0x3F, 0xC7, 0xFE, 0xF0, 0xFE, 0x07, 0xE0, 0x7E, 0x07, 0x7F, 0xE3, 0xFC, 0x7F, 0xEF,
  0x0F, 0xE0, 0x7E, 0x07, 0xE0, 0x7F, 0x1F, 0x7F, 0xE3, 0xFC, 0x3F, 0xC7, 0xFE, 0xF0, 0xFE, 0x07,
  0xE0, 0x7E, 0x07, 0xE0, 0x7F, 0x0F, 0x7F, 0xF3, 0xF7, 0x00, 0x7E, 0x07, 0xE0, 0x7F, 0x9E, 0x7F,
  0xE3, 0xF8, 0x77, 0xFE, 0xE0, 0x00, 0x00, 0x77, 0xFE, 0xE0, 0x77, 0xFE, 0xE0, 0x00, 0x00, 0x77,
  0xBE, 0xF1, 0x8D, 0xC0, 0x00, 0xC1, 0xF3, 0xFF, 0xF8, 0xF0, 0x3C, 0x0F, 0xF0, 0x7F, 0x07, 0xC0,
  0x30, 0xFF, 0xFF, 0xF0, 0x00, 0x00, 0xFF, 0xFF, 0xF0, 0xC0, 0x3E, 0x0F, 0xF0, 0x7F, 0x03, 0xC0,
  0xF1, 0xFF, 0xFC, 0xF8, 0x30, 0x00, 0x3F, 0x8F, 0xFB, 0xC7, 0xF0, 0x7E, 0x0E, 0x01, 0xC0, 0x78,
  0x3E, 0x0F, 0x81, 0xC0, 0x38, 0x00, 0x00, 0xE0, 0x3E, 0x07, 0xC0, 0x70, 0x07, 0xFE, 0x01, 0xFF,
  0xF0, 0x3E, 0x07, 0xC7, 0x80, 0x1C, 0x70, 0xCE, 0xEE, 0x3F, 0xEE, 0xE7, 0xFE, 0x7E, 0x71, 0xE7,
  0xC7, 0x0E, 0x7C, 0xE0, 0xE7, 0xCE, 0x0E, 0x7C, 0x60, 0xE7, 0xE7, 0x0E, 0x7E, 0x79, 0xFF, 0xE3,
  0xFF, 0xE7, 0x1E, 0x7C, 0x78, 0x00, 0x03, 0xE0, 0x00, 0x1F, 0xFE, 0x00, 0x7F, 0xE0, 0x00, 0xFE,
  0x00, 0x0F, 0xC0, 0x3F, 0x00, 0xEE, 0x03, 0xB8, 0x1C, 0xE0, 0x73, 0x81, 0xC7, 0x07, 0x1C, 0x38,
  0x70, 0xE1, 0xC3, 0xFF, 0x8F, 0xFE, 0x70, 0x39, 0xC0, 0xF7, 0x01, 0xFC, 0x07, 0xFF, 0xE7, 0xFF,
  0xBF, 0xFE, 0x70, 0x73, 0x83, 0x9C, 0x1C, 0xE3, 0xC7, 0xFC, 0x3F, 0xF1, 0xC3, 0xCE, 0x0E, 0x70,
  0x73, 0x83, 0xFF, 0xFF, 0xFF, 0xDF, 0xFC, 0x1F, 0xC1, 0xFF, 0x1E, 0x3C, 0xE0, 0xFE, 0x03, 0xF0,
  0x1F, 0x80, 0x1C, 0x00, 0xE0, 0x07, 0x00, 0x38, 0x0F, 0xC0, 0x77, 0x07, 0xBE, 0x78, 0xFF, 0x83,
  0xF8, 0xFF, 0xC7, 0xFF, 0xBF, 0xFC, 0x70, 0xF3, 0x83, 0x9C, 0x1C, 0xE0, 0xE7, 0x07, 0x38, 0x39,
  0xC1, 0xCE, 0x0E, 0x70, 0x73, 0x87, 0xFF, 0xFB, 0xFF, 0x9F, 0xF8, 0xFF, 0xFF, 0xFF, 0xFF, 0x80,
  0xE0, 0x38, 0x0F, 0xFB, 0xFE, 0xFF, 0xB8, 0x0E, 0x03, 0x80, 0xE0, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0x80, 0xE0, 0x38, 0x0E, 0x03, 0xFE, 0xFF, 0xB8, 0x0E, 0x03, 0x80, 0xE0, 0x38,
  0x0E, 0x03, 0x80, 0x1F, 0xC1, 0xFF, 0x9E, 0x3C, 0xE0, 0xFE, 0x03, 0xF0, 0x1F, 0x80, 0x1C, 0x00,
  0xE3, 0xFF, 0x1F, 0xF8, 0x0F, 0xC0, 0x77, 0x07, 0xBE, 0x7C, 0xFF, 0xE3, 0xF7, 0xE0, 0x7E, 0x07,
  0xE0, 0x7E, 0x07, 0xE0, 0x7E, 0x07, 0xE0, 0x7F, 0xFF, 0xFF, 0xFE, 0x07, 0xE0, 0x7E, 0x07, 0xE0,
  0x7E, 0x07, 0xE0, 0x7E, 0x07, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0xF0, 0xFF, 0x0F, 0xF0,
  0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0xE1, 0xCE, 0x1C, 0xE1, 0xCF, 0xFC,
  0x7F, 0x83, 0xF0, 0xE0, 0xFE, 0x1E, 0xE3, 0xCE, 0x78, 0xEF, 0x0E, 0xE0, 0xFC, 0x0F, 0xC0, 0xFC,
  0x0F, 0xE0, 0xEF, 0x0E, 0x70, 0xE7, 0x8E, 0x3C, 0xE1, 0xEE, 0x0F, 0xE0, 0x38, 0x0E, 0x03, 0x80,
  0xE0, 0x38, 0x0E, 0x03, 0x80, 0xE0, 0x38, 0x0E, 0x03, 0x80, 0xE0, 0x3F, 0xFF, 0xFF, 0xFF, 0xF8,
  0x1F, 0xFC, 0x0F, 0xF7, 0x06, 0xFB, 0x87, 0x7D, 0xC3, 0xBE, 0x61, 0xDF, 0x30, 0xEF, 0x9C, 0x67,
  0xCE, 0x73, 0xE7, 0x39, 0xF1, 0x9C, 0xF8, 0xCE, 0x7C, 0x76, 0x3E, 0x3F, 0x1F, 0x1F, 0x8F, 0x87,
  0xC7, 0xF8, 0x7F, 0xC7, 0xFC, 0x7F, 0xC7, 0xFC, 0x7E, 0xE7, 0xEE, 0x7E, 0xE7, 0xEE, 0x7E, 0x67,
  0xE7, 0x7E, 0x77, 0xE7, 0x7E, 0x37, 0xE3, 0xFE, 0x3F, 0x1F, 0xE1, 0xFF, 0x9E, 0x3E, 0xE0, 0x7E,
  0x03, 0xF0, 0x1F, 0x80, 0xFC, 0x07, 0xE0, 0x3F, 0x01, 0xF8, 0x0F, 0xC0, 0x77, 0x03, 0xBE, 0x7C,
  0xFF, 0xC3, 0xF8, 0xFF, 0x9F, 0xFB, 0xFF, 0xF0, 0x7E, 0x0F, 0xC1, 0xF8, 0x3F, 0xFF, 0xFF, 0xDF,
  0xF3, 0x80, 0x70, 0x0E, 0x01, 0xC0, 0x38, 0x07, 0x00, 0x1F, 0xE1, 0xFF, 0x9E, 0x3E, 0xE0, 0x7E,
  0x03, 0xF0, 0x1F, 0x80, 0xFC, 0x07, 0xE0, 0x3F, 0x01, 0xF8, 0x0F, 0xC0, 0x77, 0x03, 0xBE, 0x7C,
  0xFF, 0xC3, 0xF8, 0x07, 0x00, 0x38, 0x01, 0xF0, 0x07, 0x80, 0xFF, 0x8F, 0xFE, 0xFF, 0xEE, 0x0E,
  0xE0, 0xFE, 0x0E, 0xE0, 0xEF, 0xFC, 0xFF, 0x8F, 0xFE, 0xE0, 0xEE, 0x0E, 0xE0, 0xEE, 0x0E, 0xE0,
  0xEE, 0x0E, 0x3F, 0xC7, 0xFE, 0xF1, 0xEE, 0x07, 0xE0, 0x7F, 0x07, 0x7E, 0x03, 0xFC, 0x1F, 0xE0,
  0x1F, 0xE0, 0x7E, 0x07, 0xE0, 0x7F, 0x9F, 0x7F, 0xE1, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0xE0,
  0x0E, 0x00, 0xE0, 0x0E, 0x00, 0xE0, 0x0E, 0x00, 0xE0, 0x0E, 0x00, 0xE0, 0x0E, 0x00, 0xE0, 0x0E,
  0x00, 0xE0, 0xE0, 0x3F, 0x01, 0xF8, 0x0F, 0xC0, 0x7E, 0x03, 0xF0, 0x1F, 0x80, 0xFC, 0x07, 0xE0,
  0x3F, 0x01, 0xF8, 0x0E, 0xE0, 0x77, 0x07, 0xBE, 0x78, 0xFF, 0xC3, 0xF8, 0xF0, 0x1D, 0xC0, 0xE7,
  0x03, 0x9C, 0x0E, 0x38, 0x38, 0xE1, 0xC3, 0x87, 0x0E, 0x1C, 0x1C, 0x70, 0x73, 0x81, 0xCE, 0x07,
  0x38, 0x0E, 0xE0, 0x3F, 0x00, 0xFC, 0x03, 0xF0, 0xE1, 0xF0, 0xFC, 0x7E, 0x1F, 0x8E, 0xC3, 0xF1,
  0xD8, 0x6E, 0x3B, 0x9D, 0xC7, 0x73, 0x98, 0xCE, 0x73, 0x99, 0xCE, 0x77, 0x19, 0xCE, 0xE3, 0x39,
  0xDC, 0x67, 0x3B, 0x8E, 0xC7, 0x71, 0xD8, 0xEC, 0x3F, 0x0F, 0x87, 0xE1, 0xF0, 0x7C, 0x70, 0x3D,
  0xE0, 0xE3, 0x87, 0x87, 0x1C, 0x1E, 0xF0, 0x3B, 0x80, 0xFC, 0x01, 0xF0, 0x07, 0xC0, 0x3F, 0x80,
  0xEE, 0x07, 0x3C, 0x1C, 0x70, 0xE1, 0xE7, 0x83, 0xBC, 0x07, 0xF0, 0x3D, 0xC0, 0xE3, 0x83, 0x8E,
  0x1C, 0x1C, 0x70, 0x73, 0x80, 0xEE, 0x03, 0xF0, 0x07, 0xC0, 0x1E, 0x00, 0x38, 0x00, 0xE0, 0x03,
  0x80, 0x0E, 0x00, 0x38, 0x00, 0xE0, 0xFF, 0xFF, 0xFF, 0xFF, 0x80, 0x70, 0x1E, 0x07, 0x81, 0xE0,
  0x78, 0x1E, 0x07, 0x81, 0xE0, 0x38, 0x0F, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0x30, 0xC3,
  0x0C, 0x30, 0xC3, 0x0C, 0x30, 0xC3, 0x0C, 0x30, 0xC3, 0x0C, 0x30, 0xFF, 0xF7, 0xC0, 0xE0, 0x70,
  0x38, 0x0E, 0x07, 0x03, 0x80, 0xE0, 0x70, 0x38, 0x0E, 0x07, 0x03, 0x80, 0xE0, 0x70, 0x38, 0x0E,
  0x07, 0x03, 0x80, 0xE0, 0x70, 0x38, 0xFD, 0xFC, 0x38, 0x70, 0xE1, 0xC3, 0x87, 0x0E, 0x1C, 0x38,
  0x70, 0xE1, 0xC3, 0x87, 0x0E, 0x1C, 0x38, 0x7F, 0xFF, 0xBE, 0x00, 0x1F, 0x07, 0xE0, 0xEE, 0x39,
  0xC7, 0x1D, 0xC3, 0xC0, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0xE3, 0x8E, 0x1E, 0x03, 0xF8, 0x7F, 0xCE,
  0x1C, 0x01, 0xC3, 0xFC, 0x7F, 0xCE, 0x1C, 0xE1, 0xCE, 0x3C, 0xFE, 0xF7, 0xEF, 0xE0, 0x0E, 0x00,
  0xE0, 0x0E, 0x00, 0xE7, 0x0F, 0xFC, 0xFF, 0xEF, 0x0E, 0xE0, 0xEE, 0x07, 0xC0, 0x7E, 0x07, 0xE0,
  0xEF, 0x1E, 0xFF, 0xCC, 0xF8, 0x0F, 0x03, 0xFC, 0x7F, 0xE7, 0x0E, 0xE0, 0x7E, 0x00, 0xE0, 0x0E,
  0x04, 0xF0, 0xF7, 0x9E, 0x7F, 0xC1, 0xF8, 0x00, 0x70, 0x07, 0x00, 0x70, 0x07, 0x0E, 0x73, 0xF7,
  0x7F, 0xF7, 0x0F, 0xE0, 0x7E, 0x07, 0xE0, 0x7E, 0x07, 0xE0, 0x77, 0x9F, 0x7F, 0xF3, 0xF7, 0x0E,
  0x07, 0xF1, 0xFF, 0x38, 0x7E, 0x0F, 0xFF, 0xFF, 0xFF, 0x00, 0xE0, 0x8F, 0x3D, 0xFF, 0x0F, 0xC0,
  0x1F, 0x1F, 0x3C, 0x38, 0x38, 0xFF, 0xFF, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38,
  0x0E, 0x03, 0xF7, 0x7F, 0xFF, 0x0F, 0xE0, 0x7E, 0x07, 0xE0, 0x7E, 0x07, 0xF0, 0xF7, 0xFF, 0x3F,
  0xF1, 0xF7, 0x00, 0x70, 0x07, 0x3F, 0xF3, 0xFF, 0x3F, 0xC0, 0xE0, 0x1C, 0x03, 0x80, 0x70, 0x0E,
  0x61, 0xFF, 0x3F, 0xF7, 0x0E, 0xE0, 0xFC, 0x1F, 0x83, 0xF0, 0x7E, 0x0F, 0xC1, 0xF8, 0x3F, 0x07,
  0xFF, 0x70, 0x07, 0x77, 0x77, 0x77, 0x77, 0x77, 0x3C, 0xF1, 0xC0, 0x00, 0x71, 0xC7, 0x1C, 0x71,
  0xC7, 0x1C, 0x71, 0xC7, 0x1C, 0x7F, 0xFE, 0xF0, 0xE0, 0x1C, 0x03, 0x80, 0x70, 0x0E, 0x01, 0xC7,
  0xB9, 0xE7, 0x78, 0xFE, 0x1F, 0x83, 0xF0, 0x7E, 0x0E, 0xE1, 0xCE, 0x38, 0xE7, 0x1F, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0x06, 0x18, 0xDF, 0x3E, 0xFF, 0xFF, 0xE3, 0xC7, 0xE3, 0x87, 0xE3, 0x87,
  0xE3, 0x87, 0xE3, 0x87, 0xE3, 0x87, 0xE3, 0x87, 0xE3, 0x87, 0xE3, 0x87, 0x06, 0x1B, 0xF3, 0xFF,
  0x70, 0xEE, 0x0F, 0xC1, 0xF8, 0x3F, 0x07, 0xE0, 0xFC, 0x1F, 0x83, 0xF0, 0x70, 0x0F, 0x03, 0xFC,
  0x7F, 0xE7, 0x0F, 0xE0, 0x7E, 0x07, 0xE0, 0x7E, 0x07, 0xF0, 0x77, 0x9E, 0x7F, 0xE1, 0xF8, 0x07,
  0x0D, 0xFC, 0xFF, 0xEF, 0x0E, 0xE0, 0xEE, 0x07, 0xC0, 0x7E, 0x07, 0xE0, 0xEF, 0x1E, 0xFF, 0xCE,
  0xF8, 0xE0, 0x0E, 0x00, 0xE0, 0x0E, 0x00, 0xE0, 0x00, 0x0E, 0x03, 0xF7, 0x7F, 0xF7, 0x0F, 0xE0,
  0x7E, 0x07, 0xE0, 0x7E, 0x07, 0xE0, 0x77, 0x8F, 0x7F, 0xF3, 0xF7, 0x00, 0x70, 0x07, 0x00, 0x70,
  0x07, 0x00, 0x70, 0xDF, 0xFF, 0x38, 0xE3, 0x8E, 0x38, 0xE3, 0x8E, 0x00, 0x1E, 0x1F, 0xE7, 0xFB,
  0x87, 0xF0, 0x1F, 0xC3, 0xF8, 0x1F, 0xE1, 0xFC, 0x77, 0xFC, 0xFE, 0x38, 0x38, 0x38, 0x38, 0xFF,
  0xFF, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x3C, 0x1F, 0x1F, 0xC0, 0xF8, 0x1F, 0x03, 0xE0, 0x7C,
  0x0F, 0x81, 0xF0, 0x3F, 0x0F, 0xF3, 0xFF, 0xFC, 0xF3, 0x80, 0xF0, 0x77, 0x07, 0x70, 0xE3, 0x8E,
  0x38, 0xE3, 0x9C, 0x19, 0xC1, 0xDC, 0x1D, 0x80, 0xF8, 0x0F, 0x80, 0xE3, 0xC7, 0xE7, 0xE7, 0xE7,
  0xE7, 0xE7, 0x67, 0x66, 0x66, 0x66, 0x66, 0x76, 0x6E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7C,
  0x3E, 0xF0, 0xF7, 0x1E, 0x39, 0xC3, 0xF8, 0x1F, 0x00, 0xF0, 0x1F, 0x83, 0xB8, 0x79, 0xC7, 0x0E,
  0xE0, 0xF0, 0xC0, 0xF8, 0x1F, 0x03, 0xE0, 0x7C, 0x0F, 0x81, 0xF0, 0x3F, 0x0F, 0xF3, 0xFF, 0xFC,
  0xF3, 0x80, 0x70, 0x0E, 0xFF, 0xDF, 0xF3, 0xFC, 0xFF, 0xFF, 0xF0, 0x3C, 0x1E, 0x0F, 0x07, 0x87,
  0x83, 0xC0, 0xE0, 0x3F, 0xFF, 0xFC, 0x0F, 0x8F, 0xC7, 0x03, 0x81, 0xC0, 0xE0, 0x70, 0x38, 0x1C,
  0x0E, 0x3E, 0x1F, 0x0F, 0xC0, 0xE0, 0x70, 0x38, 0x1C, 0x0E, 0x07, 0x03, 0x81, 0xF8, 0xFC, 0x1E,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC0, 0xFC, 0x3F, 0x80, 0xE0, 0x38, 0x0E, 0x03, 0x80, 0xE0,
  0x38, 0x0E, 0x03, 0x80, 0xFC, 0x1F, 0x0F, 0xC3, 0x80, 0xE0, 0x38, 0x0E, 0x03, 0x80, 0xE0, 0x38,
  0xFE, 0x3F, 0x0F, 0x80, 0x08, 0x21, 0xE7, 0x7F, 0xFF, 0xFE, 0x67, 0xC4, 0x00,
};
const GFXglyph BoldBigGlyphs[] PROGMEM = {
  {0, 1, 1, 6, 0, 0},   // 0x20  
  {1, 5, 16, 7, 1, -15},   // 0x21 !
  {11, 10, 7, 12, 1, -15},   // 0x22 "
  {20, 13, 16, 15, 1, -15},   // 0x23 #
  {46, 12, 22, 14, 1, -18},   // 0x24 $
  {79, 16, 16, 17, 1, -15},   // 0x25 %
  {111, 12, 16, 14, 1, -15},   // 0x26 &
  {135, 5, 7, 7, 1, -15},   // 0x27 '
  {140, 8, 23, 9, 1, -16},   // 0x28 (
  {163, 8, 23, 9, 0, -16},   // 0x29 )
  {186, 10, 10, 12, 1, -15},   // 0x2A *
  {199, 10, 10, 14, 2, -12},   // 0x2B +
  {212, 5, 7, 7, 1, -3},   // 0x2C ,
  {217, 6, 2, 10, 2, -6},   // 0x2D -
  {219, 5, 4, 7, 1, -3},   // 0x2E .
  {222, 9, 21, 9, 0, -15},   // 0x2F /
  {246, 13, 16, 15, 1, -15},   // 0x30 0
  {272, 9, 16, 10, 0, -15},   // 0x31 1
  {290, 12, 16, 14, 1, -15},   // 0x32 2
  {314, 12, 16, 14, 1, -15},   // 0x33 3
  {338, 13, 16, 15, 1, -15},   // 0x34 4
  {364, 12, 16, 14, 1, -15},   // 0x35 5
  {388, 12, 16, 14, 1, -15},   // 0x36 6
  {412, 11, 16, 13, 1, -15},   // 0x37 7
  {434, 12, 16, 14, 1, -15},   // 0x38 8
  {458, 12, 16, 14, 1, -15},   // 0x39 9
  {482, 5, 12, 7, 1, -11},   // 0x3A :
  {490, 5, 15, 7, 1, -11},   // 0x3B ;
  {500, 10, 10, 14, 2, -12},   // 0x3C <
  {513, 10, 6, 14, 2, -10},   // 0x3D =
  {521, 10, 10, 14, 2, -12},   // 0x3E >
  {534, 11, 16, 13, 1, -15},   // 0x3F ?
  {556, 20, 21, 23, 2, -15},   // 0x40 @
  {609, 14, 16, 15, 0, -15},   // 0x41 A
  {637, 13, 16, 15, 1, -15},   // 0x42 B
  {663, 13, 16, 15, 1, -15},   // 0x43 C
  {689, 13, 16, 15, 1, -15},   // 0x44 D
  {715, 10, 16, 13, 2, -15},   // 0x45 E
  {735, 10, 16, 12, 2, -15},   // 0x46 F
  {755, 13, 16, 15, 1, -15},   // 0x47 G
  {781, 12, 16, 15, 2, -15},   // 0x48 H
  {805, 3, 16, 6, 2, -15},   // 0x49 I
  {811, 12, 16, 14, 1, -15},   // 0x4A J
  {835, 12, 16, 14, 2, -15},   // 0x4B K
  {859, 10, 16, 12, 2, -15},   // 0x4C L
  {879, 17, 16, 20, 2, -15},   // 0x4D M
  {913, 12, 16, 15, 2, -15},   // 0x4E N
  {937, 13, 16, 16, 1, -15},   // 0x4F O
  {963, 11, 16, 14, 2, -15},   // 0x50 P
  {985, 13, 20, 16, 1, -15},   // 0x51 Q
  {1018, 12, 16, 15, 2, -15},   // 0x52 R
  {1042, 12, 16, 14, 1, -15},   // 0x53 S
  {1066, 12, 16, 14, 1, -15},   // 0x54 T
  {1090, 13, 16, 15, 1, -15},   // 0x55 U
  {1116, 14, 16, 14, 0, -15},   // 0x56 V
  {1144, 19, 16, 21, 1, -15},   // 0x57 W
  {1182, 14, 16, 15, 0, -15},   // 0x58 X
  {1210, 14, 16, 14, 0, -15},   // 0x59 Y
  {1238, 11, 16, 13, 1, -15},   // 0x5A Z
  {1260, 6, 23, 8, 2, -16},   // 0x5B [
  {1278, 9, 21, 9, 0, -15},   // 0x5C backslash
  {1302, 7, 23, 8, 0, -16},   // 0x5D ]
  {1323, 11, 6, 14, 2, -15},   // 0x5E ^
  {1332, 12, 3, 14, 1, 2},   // 0x5F _
  {1337, 5, 3, 7, 1, -15},   // 0x60 `
  {1339, 12, 12, 13, 1, -11},   // 0x61 a
  {1357, 12, 16, 15, 2, -15},   // 0x62 b
  {1381, 12, 12, 13, 1, -11},   // 0x63 c
  {1399, 12, 16, 15, 1, -15},   // 0x64 d
  {1423, 11, 12, 13, 1, -11},   // 0x65 e
  {1440, 8, 16, 10, 1, -15},   // 0x66 f
  {1456, 12, 17, 15, 1, -11},   // 0x67 g
  {1482, 11, 16, 14, 2, -15},   // 0x68 h
  {1504, 4, 16, 6, 1, -15},   // 0x69 i
  {1512, 6, 21, 6, -1, -15},   // 0x6A j
  {1528, 11, 16, 13, 2, -15},   // 0x6B k
  {1550, 3, 16, 6, 2, -15},   // 0x6C l
  {1556, 16, 12, 20, 2, -11},   // 0x6D m
  {1580, 11, 12, 14, 2, -11},   // 0x6E n
  {1597, 12, 12, 14, 1, -11},   // 0x6F o
  {1615, 12, 17, 15, 2, -11},   // 0x70 p
  {1641, 12, 17, 15, 1, -11},   // 0x71 q
  {1667, 6, 11, 9, 2, -10},   // 0x72 r
  {1676, 10, 12, 12, 1, -11},   // 0x73 s
  {1691, 8, 15, 10, 1, -14},   // 0x74 t
  {1706, 11, 11, 14, 2, -10},   // 0x75 u
  {1722, 12, 11, 13, 0, -10},   // 0x76 v
  {1739, 16, 11, 18, 1, -10},   // 0x77 w
  {1761, 12, 11, 14, 1, -10},   // 0x78 x
  {1778, 11, 16, 14, 2, -10},   // 0x79 y
  {1800, 10, 11, 12, 1, -10},   // 0x7A z
  {1814, 9, 23, 11, 1, -16},   // 0x7B {
  {1840, 2, 25, 6, 2, -18},   // 0x7C |
  {1847, 10, 23, 11, 0, -16},   // 0x7D }
  {1876, 12, 6, 14, 1, -10},   // 0x7E ~
};
const GFXfont BoldBig PROGMEM = {(uint8_t*)BoldBigBitmaps, (GFXglyph*)BoldBigGlyphs, 0x20, 0x7E, 29};

// ---------------- Themes ----------------
enum { TH_CLEAN = 0, TH_STUDIO = 1, TH_BOLD = 2, NUM_THEMES = 3 };
const char* THEME_NAME[NUM_THEMES] = {"Clean", "Studio", "Bold"};
uint8_t theme = DEFAULT_THEME;
struct Look { const GFXfont *small, *label, *readout, *big; };
Look L;
void applyTheme() {
  if (theme >= NUM_THEMES) theme = DEFAULT_THEME;
  if (theme == TH_CLEAN) {
    L = {&CleanSmall, &CleanLabel, &CleanReadout, &CleanBig};
    C_TEXT = 0xFFFF; C_DIM = 0x8C72; C_FAINT = 0x2965; C_PANEL = 0x18E3;           // the new look's palette
    C_ACCENT = 0x0C3F; C_GREEN = 0x368B; C_AMBER = 0xFCE1; C_RED = 0xFA27;
  } else if (theme == TH_STUDIO) {
    L = {&StudioSmall, &StudioLabel, &StudioReadout, &StudioBig};
    C_TEXT = rgb(207, 214, 221); C_DIM = rgb(109, 117, 128); C_FAINT = rgb(18, 34, 22); C_PANEL = rgb(10, 20, 12);
    C_ACCENT = rgb(98, 255, 142); C_GREEN = rgb(77, 255, 124); C_AMBER = rgb(255, 194, 61); C_RED = rgb(255, 74, 61);
  } else {
    L = {&BoldSmall, &BoldLabel, &BoldReadout, &BoldBig};
    C_TEXT = rgb(255, 255, 255); C_DIM = rgb(160, 160, 160); C_FAINT = rgb(45, 45, 45); C_PANEL = rgb(30, 30, 30);
    C_ACCENT = C_TEXT; C_GREEN = rgb(31, 191, 95); C_AMBER = rgb(245, 165, 36); C_RED = rgb(235, 68, 58);
  }
}
uint16_t darker(uint16_t c) {                // the same colour at 70%, for tracks on a coloured screen
  int r = (c >> 11) * 7 / 10, g = ((c >> 5) & 63) * 7 / 10, b = (c & 31) * 7 / 10;
  return (r << 11) | (g << 5) | b;
}
// Bold theme: the meter and gig screens fill with the level colour. While drawing those,
// labels and lines switch to white so they read on the colour.
bool onColourBg = false;                    // drawing on the Bold theme's level colour: battery goes white
struct SavedInk { uint16_t text, dim, faint, accent, panel; };
SavedInk inkOnColour(uint16_t bg);          // (declared here so Arduino keeps them after the type)
void restoreInk(const SavedInk& s);
SavedInk inkOnColour(uint16_t bg) {
  SavedInk s = {C_TEXT, C_DIM, C_FAINT, C_ACCENT, C_PANEL};
  C_TEXT = rgb(255, 255, 255); C_DIM = rgb(255, 255, 255); C_ACCENT = C_TEXT; C_FAINT = darker(bg); C_PANEL = rgb(0, 0, 0);
  onColourBg = true;
  return s;
}
void restoreInk(const SavedInk& s) { C_TEXT = s.text; C_DIM = s.dim; C_FAINT = s.faint; C_ACCENT = s.accent; C_PANEL = s.panel; onColourBg = false; }




// ---------------- Settings / state ----------------
Preferences prefs;
volatile int wMode = W_A, rMode = R_FAST;
float calOffset = DEFAULT_CAL_OFFSET;
int amberFrom = DEFAULT_AMBER, redFrom = DEFAULT_RED;
uint8_t brightness = DEFAULT_BRIGHTNESS;
bool wifiOn = true;
// gig mode settings
bool limOn = false;          // limit alarm
int  limDb = 100;            // limit, dB (WHO safe listening venues: 100 dB LAeq over 15 min)
int  limWin = 2;             // 0 = 1 min, 1 = 5 min, 2 = 15 min
int  limW = W_A;             // weighting the limit is measured in (A or C)
const int LIM_WARN_DB = 3;   // amber this many dB below the limit
int  dimSecs = 60;           // dim the screen after this many seconds without a press (0 = never)

uint16_t levelColour(float db) {
  if (db >= redFrom) return C_RED;
  if (db >= amberFrom) return C_AMBER;
  return C_GREEN;
}

void textCentered(const char* s, int cx, int baseline) {
  int16_t x1, y1; uint16_t w, h;
  canvas.getTextBounds(s, 0, baseline, &x1, &y1, &w, &h);
  canvas.setCursor(cx - (int)w / 2 - x1, baseline);
  canvas.print(s);
}

void textRight(const char* s, int right, int baseline) {
  int16_t x1, y1; uint16_t w, h;
  canvas.getTextBounds(s, 0, baseline, &x1, &y1, &w, &h);
  canvas.setCursor(right - (int)w - x1, baseline);
  canvas.print(s);
}

void pushCanvas();                         // sends the whole frame (defined further down)

void setBrightness(uint8_t level) {
  uint8_t v = level > 15 ? 15 : level;
  oled.sendCommand(0xC7, &v, 1);   // SSD1351 master contrast
}

// ---------------- Audio state (written by the audio task) ----------------
portMUX_TYPE audioLock = portMUX_INITIALIZER_UNLOCKED;
float msqF[3] = {0}, msqS[3] = {0};
double leqSum[3] = {0};
uint64_t leqN = 0;
float maxF[3] = {0};
float dcX = 0, dcY = 0;
const float DC_R = 0.9995f;
float aFast, aSlow;
volatile long warmup = FS / 2;
bool seedAverages = true;
volatile bool resetRequested = false;
volatile uint32_t clipAt = 0;        // millis() when the mic last hit full scale
bool micClipping() { uint32_t c = clipAt; return c && millis() - c < 2000; }
bool micFaulty() { uint32_t c = micFlatSince; return c && millis() - c > 2000; }   // flat for 2 s: mic or wiring problem

enum CalState { CAL_IDLE, CAL_SETTLE, CAL_MEASURE };
volatile int calState = CAL_IDLE;
uint32_t calT0 = 0;
double calSum = 0; uint64_t calN = 0;
int32_t i2sBuf[BUF_LEN];

// Snapshot the UI and the phone read (copied under the lock)
// ---- gig mode: rolling windows, log, dose (written by the audio task, under audioLock) ----
const int ROLL_SECS = 900;                 // keep 15 minutes of 1-second mean-squares
const int ROLL_W[3] = {60, 300, 900};      // 1, 5, 15 minute windows
float  rollSec[ROLL_SECS][3];
int    rollHead = 0; uint32_t rollCount = 0;
double rollSum[3][3];                      // [window][weighting]
double secSum[3] = {0, 0, 0}; uint32_t secN = 0;
double minSum[3] = {0, 0, 0}; int minSecs = 0; float minMaxFA = 0, minPeakC = 0;
double doseE = 0; double doseSecs = 0;     // A-weighted energy (mean-square x seconds) since the log started
struct LogRec { int16_t laeq, lceq, lafmax, lcpeak; };   // dB x 10
const int LOG_MAX = 1440;                  // 24 hours of minutes
LogRec logRec[LOG_MAX];
uint32_t logTotal = 0;                     // minutes logged since the log started (keeps counting past 24 h)
struct Marker { uint32_t sec; char text[25]; };
const int MARK_MAX = 40;
Marker marks[MARK_MAX]; int markCount = 0;
int copyLog(LogRec* out, uint32_t& first, Marker* mk, int& nmk);   // (declared here so Arduino keeps it after these types)
uint32_t logSecs = 0;                      // seconds since the log started
volatile bool newLogRequested = false;
int64_t logStartEpochMs = 0;               // wall-clock time the log started (from the phone), 0 = unknown
int tzOffsetMin = 0;                       // phone's time zone, minutes east of UTC

struct Snapshot { float msqF[3], msqS[3], maxF[3]; double leqSum[3]; uint64_t leqN;
                  double roll[3][3]; uint32_t rollCount; double doseE, doseSecs; uint32_t logTotal, logSecs; };
Snapshot snap;

void takeSnapshot() {
  portENTER_CRITICAL(&audioLock);
  for (int w = 0; w < 3; w++) { snap.msqF[w] = msqF[w]; snap.msqS[w] = msqS[w]; snap.maxF[w] = maxF[w]; snap.leqSum[w] = leqSum[w]; }
  snap.leqN = leqN;
  for (int k = 0; k < 3; k++) for (int w = 0; w < 3; w++) snap.roll[k][w] = rollSum[k][w];
  snap.rollCount = rollCount; snap.doseE = doseE; snap.doseSecs = doseSecs;
  snap.logTotal = logTotal; snap.logSecs = logSecs;
  portEXIT_CRITICAL(&audioLock);
}

float toDb(double ms) {
  if (ms < 1e-20) ms = 1e-20;
  return 10.0f * log10f((float)ms) + MIC_REF_DB + calOffset;
}
float snapFastDb() { return toDb(snap.msqF[wMode]); }
float snapLeqDb()  { return snap.leqN ? toDb(snap.leqSum[wMode] / (double)snap.leqN) : 0; }
float snapDb() {
  if (rMode == R_FAST || rMode == R_RTA || rMode == R_GIG) return toDb(snap.msqF[wMode]);
  if (rMode == R_SLOW) return toDb(snap.msqS[wMode]);
  return snapLeqDb();
}
unsigned long snapLeqSecs() { return (unsigned long)(snap.leqN / FS); }
// rolling Leq: window k (0 = 1 min, 1 = 5 min, 2 = 15 min), weighting w. Until the window has
// filled it averages over the seconds so far. Returns false before the first full second.
bool rollDb(int k, int w, float& out) {
  uint32_t n = snap.rollCount < (uint32_t)ROLL_W[k] ? snap.rollCount : (uint32_t)ROLL_W[k];
  if (!n) return false;
  out = toDb(snap.roll[k][w] / n);
  return true;
}
bool rollFull(int k) { return snap.rollCount >= (uint32_t)ROLL_W[k]; }
// ---- limit alarm ----
int limState = 0;          // 0 fine, 1 within LIM_WARN_DB of the limit, 2 over
float limVal = 0;
bool limReady = false;
const char* LIM_WIN_NAME[3] = {"1 min", "5 min", "15 min"};
void toast(const char* big, const char* small, uint32_t ms);
bool otaReady();
bool validVersion(const String& v);
String makeSerial();
void saveCurrentLog(bool force);
void archiveCurrentLog();
void factoryReset();
void drawUpdateScreen(const char* big, const char* small, int pct, uint16_t colour);
void handleUpdateUpload();
void handleUpdateDone();
void wake(uint32_t now);
void evalLimit(uint32_t now) {             // call after takeSnapshot()
  float v;
  limReady = !standby && limOn && snap.rollCount >= 10 && rollDb(limWin, limW, v);
  if (!limReady) { limState = 0; return; }
  limVal = v;
  int st = v >= limDb ? 2 : v >= limDb - LIM_WARN_DB ? 1 : 0;
  if (st > limState) {
    char sm[24];
    snprintf(sm, sizeof(sm), "%.1f dB, limit %d", v, limDb);
    toast(st == 2 ? "Over limit" : "Near limit", sm, 3000);
    wake(now);
  }
  limState = st;
}

// dose: % of 85 dB(A) for 8 hours (3 dB exchange rate, as in the Australian WHS code)
float dosePct() {
  double ref = pow(10.0, (85.0 - MIC_REF_DB - calOffset) / 10.0) * 8 * 3600;
  return (float)(100.0 * snap.doseE / ref);
}

float peakDb = 0; uint32_t peakAt = 0;   // bar peak hold (2 s)

// ---------------- Toast ----------------
char toastBig[16] = "", toastSmall[24] = "";
uint32_t toastUntil = 0;
uint32_t toastAt = 0, toastFadeAt = 0;     // Clean look: when the card appeared / started to fade out
void toast(const char* big, const char* small, uint32_t ms) {
  if (!toastUntil || (int32_t)(millis() - toastUntil) >= 0) toastAt = millis();   // a new card fades in
  toastFadeAt = 0;
  strncpy(toastBig, big, sizeof(toastBig) - 1); toastBig[sizeof(toastBig) - 1] = 0;
  strncpy(toastSmall, small, sizeof(toastSmall) - 1); toastSmall[sizeof(toastSmall) - 1] = 0;
  toastUntil = millis() + ms;
}

// ---------------- Button ----------------
Button btn{PIN_BTN};
const uint32_t DEBOUNCE_MS = 25;
const uint32_t HOLD_RESPONSE_MS = 1000, HOLD_WIFI_MS = 3000;
const uint32_t DOUBLE_CLICK_MS = 800, EDIT_TIMEOUT_MS = 3000;
uint32_t editUntil = 0;
bool tapPending = false;
uint32_t lastTapAt = 0;

// 0 nothing, 1 tap, 2 released after 1-3 s, 3 reached 3 s (fires once while held)
// ---- screen dimming ----
bool dimmed = false;
bool screenOff = false;                    // wired model: screen switched off to stop burn-in
uint32_t lastActivity = 0;
void wake(uint32_t now) {
  lastActivity = now;
  if (standby) standbyShowUntil = now + 8000;     // standby: show when it comes on next
  if (screenOff) { screenOff = false; oled.sendCommand(0xAF); }   // display on
  if (dimmed) { dimmed = false; setBrightness(brightness); }
}

int pollButton(Button& b) {
  bool pressed = digitalRead(b.pin) == HIGH;    // other side of the button is held HIGH
  uint32_t now = millis();
  if (pressed != b.down) {
    if (now - b.changedAt < DEBOUNCE_MS) return 0;
    b.changedAt = now;
    b.down = pressed;
    if (pressed) {
      b.downAt = now; b.longDone = false;
      if (dimmed) { wake(now); b.longDone = true; }   // first press only wakes the screen
      else lastActivity = now;
      return 0;
    }
    uint32_t held = now - b.downAt;
    if (b.longDone) return 0;
    if (held >= HOLD_RESPONSE_MS) return 2;
    return 1;
  }
  if (b.down && !b.longDone && now - b.downAt >= HOLD_WIFI_MS) { b.longDone = true; return 3; }
  return 0;
}

// ---------------- Battery (Feather's built-in fuel gauge) ----------------
enum Gauge { G_NONE, G_MAX17048, G_LC709203 };
Gauge gauge = G_NONE;
float battV = 0; int battPct = 100; bool charging = false;
uint32_t lastBatt = 0, critSince = 0;

bool i2cPresent(uint8_t addr) { Wire.beginTransmission(addr); return Wire.endTransmission() == 0; }

bool readReg16BE(uint8_t addr, uint8_t reg, uint16_t& v) {      // MAX17048: big-endian
  Wire.beginTransmission(addr); Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(addr, (uint8_t)2) != 2) return false;
  v = ((uint16_t)Wire.read() << 8) | Wire.read();
  return true;
}

uint8_t crc8(const uint8_t* d, int n) {
  uint8_t c = 0;
  for (int i = 0; i < n; i++) { c ^= d[i]; for (int k = 0; k < 8; k++) c = (c & 0x80) ? (uint8_t)((c << 1) ^ 0x07) : (uint8_t)(c << 1); }
  return c;
}
bool lcWrite(uint8_t reg, uint16_t v) {                           // LC709203F: little-endian + CRC
  uint8_t d[4] = {(uint8_t)(0x0B << 1), reg, (uint8_t)(v & 0xFF), (uint8_t)(v >> 8)};
  Wire.beginTransmission(0x0B); Wire.write(reg); Wire.write(d[2]); Wire.write(d[3]); Wire.write(crc8(d, 4));
  return Wire.endTransmission() == 0;
}
bool lcRead(uint8_t reg, uint16_t& v) {
  Wire.beginTransmission(0x0B); Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((uint8_t)0x0B, (uint8_t)3) != 3) return false;
  uint8_t lo = Wire.read(), hi = Wire.read(); Wire.read();
  v = ((uint16_t)hi << 8) | lo;
  return true;
}

void gaugeInit() {
  Wire.begin();                  // Feather SDA/SCL (GPIO 3/4); its I2C power is switched on by the board
  delay(10);
  if (i2cPresent(0x36)) gauge = G_MAX17048;
  else if (i2cPresent(0x0B)) {
    gauge = G_LC709203;
    lcWrite(0x15, 0x0001);       // operational mode
    lcWrite(0x0B, 0x002D);       // APA for a 2000 mAh pack
    lcWrite(0x12, 0x0001);       // battery profile (4.2 V)
    lcWrite(0x16, 0x0000);       // temperature from I2C (no thermistor)
    lcWrite(0x08, 0x0BA6);       // 25 C
    lcWrite(0x07, 0xAA55);       // start RSOC estimate
  }
  Serial.printf("Battery gauge: %s\n", gauge == G_MAX17048 ? "MAX17048" : gauge == G_LC709203 ? "LC709203F" : "not found");
}

void readBattery() {
  uint16_t v;
  if (gauge == G_MAX17048) {
    if (readReg16BE(0x36, 0x02, v)) battV = v * 78.125e-6f;
    if (readReg16BE(0x36, 0x04, v)) battPct = constrain((int)(v / 256.0f + 0.5f), 0, 100);
    if (readReg16BE(0x36, 0x16, v)) charging = ((int16_t)v) * 0.208f > 0.5f;   // %/hour
  } else if (gauge == G_LC709203) {
    if (lcRead(0x09, v)) battV = v / 1000.0f;
    if (lcRead(0x0D, v)) battPct = constrain((int)v, 0, 100);
    charging = false;
  }
}

// ---------------- Prefs ----------------
void savePrefs() {
  prefs.putUChar("w", wMode); prefs.putUChar("r", rMode); prefs.putFloat("cal", calOffset);
  prefs.putUChar("amb", amberFrom); prefs.putUChar("red", redFrom);
  prefs.putUChar("bri", brightness); prefs.putBool("wifi", wifiOn);
  prefs.putBool("limOn", limOn); prefs.putUChar("limDb", limDb); prefs.putUChar("limWin", limWin); prefs.putUChar("limW", limW);
  prefs.putUShort("dim", dimSecs);
  prefs.putUChar("theme", theme);
}

// ---------------- Actions (from the button or the phone) ----------------
void setWeighting(int w) { wMode = constrain(w, 0, 2); savePrefs(); }
void setResponse(int r) {
  rMode = constrain(r, 0, NUM_R - 1); savePrefs();
  if (rMode == R_LEQ) resetRequested = true;
}
void startCal94() {
  if (calState != CAL_IDLE) return;
  calT0 = millis(); calState = CAL_SETTLE;
  Serial.println("Calibrating, keep the 94 dB calibrator on...");
}

// ---------------- RTA (1/3-octave real-time analyser) ----------------
// 31 bands, 20 Hz - 20 kHz, FAST (125 ms) averaging. Two 8192-point FFTs:
//   bands 250 Hz - 20 kHz from the full 48 kHz signal   (5.9 Hz resolution, every ~43 ms)
//   bands  20 Hz - 200 Hz from a copy slowed to 6 kHz   (0.7 Hz resolution, every ~85 ms)
// so even the narrow bottom bands read correctly. Bands are measured flat; the dB A / dB C
// curve is added at each band centre when shown.
const int RTA_N = 8192, RTA_BANDS = 31, RTA_SPLIT = 11;   // bands 0..10 use the slow copy
const int RTA_HOP_HI = 2048, RTA_HOP_LO = 512, RTA_DECIM = 8;
const char* RTA_LABEL[RTA_BANDS] = {"20","25","31.5","40","50","63","80","100","125","160","200","250","315","400","500",
  "630","800","1k","1.25k","1.6k","2k","2.5k","3.15k","4k","5k","6.3k","8k","10k","12.5k","16k","20k"};
float *rtaRingHi = nullptr, *rtaRingLo = nullptr;          // newest RTA_N samples of each stream
volatile uint32_t rtaWriteHi = 0, rtaWriteLo = 0;           // samples written so far
float *rtaRe, *rtaIm, *rtaWin, *rtaCos, *rtaSin;
float rtaWinPow = 0;
float rtaMs[RTA_BANDS];                                     // smoothed band mean-square (flat)
bool rtaDoneHi = false, rtaDoneLo = false;
volatile bool rtaReady = false;
Biquad decim[2];                                            // 1 kHz low-pass before slowing to 6 kHz
int decimCount = 0;

float bandCentre(int b) { return 1000.0f * powf(2.0f, (b - 17) / 3.0f); }

float weightDb(float f, int w) {          // IEC 61672 A / C curves
  double f2 = (double)f * f;
  if (w == W_A) {
    double ra = (148693636.0 * f2 * f2) / ((f2 + 424.36) * sqrt((f2 + 11599.29) * (f2 + 544496.41)) * (f2 + 148693636.0));
    return (float)(20.0 * log10(ra) + 2.00);
  }
  if (w == W_C) {
    double rc = (148693636.0 * f2) / ((f2 + 424.36) * (f2 + 148693636.0));
    return (float)(20.0 * log10(rc) + 0.06);
  }
  return 0;
}

float rtaBandDb(int b) {                  // band level in the current weighting, as shown
  float ms = rtaMs[b];
  if (ms < 1e-20f) ms = 1e-20f;
  return 10.0f * log10f(ms) + MIC_REF_DB + calOffset + weightDb(bandCentre(b), wMode);
}

static float* bigAlloc(size_t n) {
  float* p = nullptr;
  if (psramFound()) p = (float*)ps_malloc(n * sizeof(float));
  if (!p) p = (float*)malloc(n * sizeof(float));
  return p;
}

static Biquad lowpass(double fc, double q) {   // RBJ cookbook low-pass at FS
  double w0 = 2 * M_PI * fc / FS, c = cos(w0), al = sin(w0) / (2 * q), a0 = 1 + al;
  Biquad b;
  b.b0 = (1 - c) / 2 / a0; b.b1 = (1 - c) / a0; b.b2 = (1 - c) / 2 / a0;
  b.a1 = -2 * c / a0; b.a2 = (1 - al) / a0;
  return b;
}

inline void rtaPush(float e) {             // called by the audio task for every sample
  if (!rtaRingHi) return;
  rtaRingHi[rtaWriteHi & (RTA_N - 1)] = e; rtaWriteHi = rtaWriteHi + 1;
  float l = decim[1].process(decim[0].process(e));
  if (++decimCount >= RTA_DECIM) { decimCount = 0; rtaRingLo[rtaWriteLo & (RTA_N - 1)] = l; rtaWriteLo = rtaWriteLo + 1; }
}

void fft(float* re, float* im) {          // in-place radix-2, N = RTA_N
  for (int i = 1, j = 0; i < RTA_N; i++) {
    int bit = RTA_N >> 1;
    for (; j & bit; bit >>= 1) j ^= bit;
    j ^= bit;
    if (i < j) { float t = re[i]; re[i] = re[j]; re[j] = t; t = im[i]; im[i] = im[j]; im[j] = t; }
  }
  for (int len = 2; len <= RTA_N; len <<= 1) {
    int half = len >> 1, step = RTA_N / len;
    for (int i = 0; i < RTA_N; i += len) {
      for (int k = 0; k < half; k++) {
        float c = rtaCos[k * step], s = rtaSin[k * step];
        int a = i + k, b = a + half;
        float tr = re[b] * c + im[b] * s, ti = im[b] * c - re[b] * s;
        re[b] = re[a] - tr; im[b] = im[a] - ti;
        re[a] += tr;        im[a] += ti;
      }
    }
  }
}

bool rtaInit() {
  rtaRingHi = bigAlloc(RTA_N); rtaRingLo = bigAlloc(RTA_N); rtaRe = bigAlloc(RTA_N); rtaIm = bigAlloc(RTA_N);
  rtaWin = bigAlloc(RTA_N); rtaCos = bigAlloc(RTA_N / 2); rtaSin = bigAlloc(RTA_N / 2);
  if (!rtaRingHi || !rtaRingLo || !rtaRe || !rtaIm || !rtaWin || !rtaCos || !rtaSin) { rtaRingHi = nullptr; return false; }
  for (int i = 0; i < RTA_N; i++) {
    rtaRingHi[i] = rtaRingLo[i] = 0;
    rtaWin[i] = 0.5f - 0.5f * cosf(2 * M_PI * i / RTA_N); rtaWinPow += rtaWin[i] * rtaWin[i];
  }
  for (int k = 0; k < RTA_N / 2; k++) { rtaCos[k] = cosf(2 * M_PI * k / RTA_N); rtaSin[k] = sinf(2 * M_PI * k / RTA_N); }
  for (int b = 0; b < RTA_BANDS; b++) rtaMs[b] = 0;
  decim[0] = lowpass(1000, 0.5411961); decim[1] = lowpass(1000, 1.3065630);   // 4th-order Butterworth
  return true;
}

// one analysis frame of the newest RTA_N samples of a stream, for bands bFrom..bTo-1
void rtaAnalyse(const float* ring, uint32_t end, float fs, int hop, int bFrom, int bTo, bool first) {
  for (int i = 0; i < RTA_N; i++) { rtaRe[i] = ring[(end + i) & (RTA_N - 1)] * rtaWin[i]; rtaIm[i] = 0; }
  fft(rtaRe, rtaIm);
  const float binHz = fs / RTA_N, norm = 2.0f / (RTA_N * rtaWinPow);
  const float alpha = 1.0f - expf(-(float)hop / fs / TAU_FAST);
  for (int b = bFrom; b < bTo; b++) {
    float fc = bandCentre(b), lo = fc * 0.8908987f, hi = fc * 1.1224620f;   // fc * 2^(-+1/6)
    int k0 = (int)floorf(lo / binHz), k1 = (int)ceilf(hi / binHz);
    if (k0 < 1) k0 = 1;
    if (k1 > RTA_N / 2 - 1) k1 = RTA_N / 2 - 1;
    double ms = 0;
    for (int k = k0; k <= k1; k++) {       // each bin counts by how much of it falls in the band
      float bl = (k - 0.5f) * binHz, bh = (k + 0.5f) * binHz;
      float ov = fminf(bh, hi) - fmaxf(bl, lo);
      if (ov > 0) ms += (double)(rtaRe[k] * rtaRe[k] + rtaIm[k] * rtaIm[k]) * (ov / binHz);
    }
    float v = (float)(ms * norm);
    rtaMs[b] = first ? v : rtaMs[b] + alpha * (v - rtaMs[b]);
  }
}

void rtaStep() {                           // run whichever analysis is due
  static uint32_t lastHi = 0, lastLo = 0;
  uint32_t wh = rtaWriteHi, wl = rtaWriteLo;
  if (wh >= (uint32_t)RTA_N && wh - lastHi >= (uint32_t)RTA_HOP_HI) {
    lastHi = wh; rtaAnalyse(rtaRingHi, wh, (float)FS, RTA_HOP_HI, RTA_SPLIT, RTA_BANDS, !rtaDoneHi); rtaDoneHi = true;
  }
  if (wl >= (uint32_t)RTA_N && wl - lastLo >= (uint32_t)RTA_HOP_LO) {
    lastLo = wl; rtaAnalyse(rtaRingLo, wl, (float)FS / RTA_DECIM, RTA_HOP_LO, 0, RTA_SPLIT, !rtaDoneLo); rtaDoneLo = true;
  }
  rtaReady = rtaDoneHi && rtaDoneLo;
}

void rtaTask(void*) {
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(10));
    if (warmup <= 0) rtaStep();
  }
}

// ---------------- WiFi + phone page ----------------
WebServer server(80);
bool serverStarted = false;
uint32_t wifiOffAt = 0;
uint32_t apRestartAt = 0;                 // apply a new WiFi name / password shortly after replying
uint32_t restartAt = 0;                   // restart after a software update

// The phone page (built in from phone_gig.html)
const char PAGE[] PROGMEM =
"<!DOCTYPE html>\n"
"<html lang=\"en\">\n"
"<head>\n"
"<meta charset=\"utf-8\">\n"
"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1, viewport-fit=cover\">\n"
"<meta name=\"apple-mobile-web-app-capable\" content=\"yes\">\n"
"<meta name=\"apple-mobile-web-app-status-bar-style\" content=\"black-translucent\">\n"
"<meta name=\"apple-mobile-web-app-title\" content=\"PocketSPL\">\n"
"<meta name=\"theme-color\" content=\"#17191b\">\n"
"<title>PocketSPL</title>\n"
"<style>\n"
"@font-face{font-family:\"Barlow SC\";font-weight:500;font-display:block;src:url(data:font/woff2;base64,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) format(\"woff2\")}\n"
"@font-face{font-family:\"Barlow SC\";font-weight:600;font-display:block;src:url(data:font/woff2;base64,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) format(\"woff2\")}\n"
"/* PocketSPL: graphite panel, silkscreen-white type, LED colours only where they mean level. */\n"
":root{\n"
"  --body:#17191b; --panel:#202225; --recess:#121416; --raised:#2c2f33; --rule:#34383d;\n"
"  --ink:#edebe6; --mute:#94989e; --faint:#5d6166;\n"
"  --green:#5bd483; --amber:#f4b23e; --red:#f45a4f;\n"
"  --accent:var(--ink);\n"
"  --dim:var(--mute); --text:var(--ink); --line:var(--rule); --card:var(--panel);\n"
"  --r-group:14px; --r-ctl:9px;\n"
"}\n"
"*{box-sizing:border-box;-webkit-tap-highlight-color:transparent}\n"
"[hidden]{display:none!important}\n"
"html,body{margin:0;background:var(--body);color:var(--ink);font:500 16px/1.4 \"Barlow SC\",-apple-system,\"Segoe UI\",sans-serif;-webkit-font-smoothing:antialiased;font-variant-numeric:tabular-nums}\n"
"body{padding:max(12px,env(safe-area-inset-top)) 16px calc(88px + env(safe-area-inset-bottom));min-height:100vh;box-sizing:border-box}\n"
".wrap{max-width:460px;margin:0 auto}\n"
"button,input,a.btn,label.btn{font:inherit}\n"
":focus-visible{outline:2px solid var(--ink);outline-offset:2px}\n"
"a{color:var(--ink);text-underline-offset:3px}\n"
"\n"
"/* top bar */\n"
"header{display:flex;align-items:baseline;justify-content:space-between;padding:4px 0 12px;border-bottom:1px solid var(--rule);margin-bottom:14px}\n"
"header h1{font-size:19px;font-weight:600;margin:0;letter-spacing:.2px}\n"
".status{display:flex;gap:14px;align-items:center;font-size:14px;color:var(--mute)}\n"
".dot{width:7px;height:7px;border-radius:50%;background:var(--red);display:inline-block;margin-right:6px;vertical-align:1px}\n"
".dot.ok{background:var(--green);box-shadow:0 0 6px rgba(91,212,131,.6)}\n"
".bat{display:inline-flex;align-items:center;gap:4px;font-weight:600}\n"
".bat svg{display:block}\n"
"\n"
"/* sections */\n"
".card{background:var(--panel);border-radius:var(--r-group);padding:16px;margin-bottom:12px}\n"
"h2{font-size:18px;font-weight:600;margin:0 0 12px;color:var(--ink)}\n"
"h3{font-size:14px;font-weight:600;color:var(--mute);margin:20px 0 8px}\n"
".note{font-size:14px;color:var(--mute);line-height:1.45;margin:8px 0}\n"
".row{display:flex;align-items:center;justify-content:space-between;gap:12px;margin:12px 0}\n"
".row label{font-size:16px}\n"
".row output{font-weight:600}\n"
".off{opacity:.45}\n"
"\n"
"/* the reading */\n"
"#meter{background:transparent;padding:6px 0 4px;border-radius:0}\n"
".modes{display:flex;align-items:center;justify-content:space-between;font-size:17px;font-weight:600}\n"
".modes #wLbl{color:var(--ink)} .modes .r{color:var(--mute);margin-right:auto;margin-left:10px}\n"
".big{font:600 128px/0.95 var(--font-num),sans-serif;letter-spacing:-1px;text-align:right;margin:10px 0 2px;white-space:nowrap;overflow:hidden;transition:color .25s}\n"
".big.word{font-size:72px;line-height:1.3;text-align:left}\n"
".unit{color:var(--mute);font-size:15px;text-align:right;margin-bottom:14px;min-height:21px}\n"
".ladder{display:grid;grid-template-columns:repeat(40,1fr);gap:2px;height:22px;padding:4px;background:var(--recess);border-radius:6px}\n"
".ladder i{background:#26292d;border-radius:1px;transition:background-color .08s}\n"
".ladder i.g{background:var(--green)} .ladder i.a{background:var(--amber)} .ladder i.r{background:var(--red)}\n"
".ladder i.pk{background:var(--ink)}\n"
".scale{position:relative;height:18px;margin:5px 4px 0;font-size:12px;color:var(--faint)}\n"
".scale span{position:absolute;transform:translateX(-50%)}\n"
".scale span:first-child{transform:none}.scale span.end{transform:translateX(-100%)}\n"
"#amberMark{color:var(--amber)}\n"
".stats{display:grid;grid-template-columns:repeat(3,1fr);margin:18px 0 6px;border-top:1px solid var(--rule);border-bottom:1px solid var(--rule)}\n"
".stat{padding:10px 0 10px 12px;border-left:1px solid var(--rule)}\n"
".stat:first-child{border-left:0;padding-left:0}\n"
".stat span{display:block;font-size:13px;color:var(--mute);order:-1}\n"
".stat b{display:block;font:600 26px/1.15 var(--font-num),sans-serif}\n"
".stat{display:flex;flex-direction:column}\n"
".ctl{display:grid;grid-template-columns:92px 1fr;align-items:center;gap:10px;margin:12px 0}\n"
".ctl > span{font-size:14px;color:var(--mute)}\n"
"\n"
"/* controls */\n"
".seg{display:flex;background:var(--recess);border-radius:var(--r-ctl);padding:3px;gap:3px;width:100%}\n"
".seg button{flex:1;border:0;background:transparent;color:var(--mute);font-weight:600;font-size:15px;padding:9px 0;border-radius:7px;cursor:pointer}\n"
".seg button.on{background:var(--ink);color:#17191b}\n"
".btn{display:inline-block;border:0;border-radius:var(--r-ctl);padding:11px 16px;font-weight:600;font-size:16px;background:var(--raised);color:var(--ink);box-shadow:inset 0 1px 0 rgba(255,255,255,.05);cursor:pointer;text-align:center;text-decoration:none}\n"
".btn:active{background:#34383d}\n"
".btn.wide{width:100%}\n"
".btn.small{padding:7px 12px;font-size:14px}\n"
".btn.primary{background:var(--ink);color:#17191b;box-shadow:none}\n"
".btn.danger{color:var(--red)}\n"
".btn.danger.solid{background:var(--red);color:#fff}\n"
".btn.on,.btn.small.on{background:var(--ink);color:#17191b}\n"
"#whoStd.on{background:transparent;box-shadow:inset 0 0 0 1px var(--rule);color:var(--green)}\n"
"#holdBtn.on{background:var(--amber);color:#17191b}\n"
".btn:disabled{opacity:.4}\n"
".step{display:flex;align-items:center;gap:4px}\n"
".step button{min-width:42px;height:40px;border:0;border-radius:var(--r-ctl);background:var(--raised);color:var(--ink);font-size:17px;font-weight:600;padding:0 8px;cursor:pointer}\n"
".step output{min-width:66px;text-align:center;font-weight:600;font-size:17px}\n"
"input[type=range]{width:55%;accent-color:var(--ink)}\n"
".check{display:flex;gap:10px;align-items:center;font-size:16px;margin:12px 0}\n"
".schrow{display:grid;grid-template-columns:auto 1fr auto 1fr auto;gap:8px;align-items:center;padding:10px 0;border-bottom:1px solid var(--rule)}\n"
".schrow select{grid-column:1/5}\n"
".schrow select,.schrow input{background:var(--recess);border:1px solid var(--rule);color:var(--ink);border-radius:var(--r-ctl);padding:10px 6px;font:500 15px \"Barlow SC\",sans-serif;width:100%;min-width:0}\n"
".schrow span{font-size:14px;color:var(--mute)}\n"
".schrow .x{padding:8px 11px}\n"
".schbtns{display:flex;gap:8px}.schbtns .btn{flex:1}\n"
"#schState{font-weight:600}\n"
"body:not(.wired) .wiredonly{display:none!important}\n"
".check input{width:20px;height:20px;accent-color:var(--ink)}\n"
"input[type=text],input[type=password],input:not([type]),.joinform input,.markform input,.addr input,.pinrow input{background:var(--recess);border:1px solid var(--rule);color:var(--ink);border-radius:var(--r-ctl);padding:11px 12px;font:500 16px \"Barlow SC\",sans-serif;width:100%;min-width:0}\n"
"input:focus{outline:none;border-color:var(--mute)}\n"
"\n"
"/* messages */\n"
".toast{position:fixed;left:50%;bottom:calc(92px + env(safe-area-inset-bottom));transform:translateX(-50%);background:var(--ink);color:#17191b;font-weight:600;padding:10px 16px;border-radius:var(--r-ctl);font-size:15px;opacity:0;transition:opacity .2s;pointer-events:none;max-width:calc(100% - 32px);text-align:center;z-index:30}\n"
".toast.show{opacity:1}\n"
".limbanner{border-radius:var(--r-ctl);padding:10px 12px;font-weight:600;font-size:15px;margin:0 0 12px;color:#17191b}\n"
".limbanner.amber{background:var(--amber)}.limbanner.red{background:var(--red);color:#fff}\n"
".updbanner{background:var(--panel);border-left:3px solid var(--ink);border-radius:6px;padding:10px 12px;margin:0 0 12px;font-size:15px;font-weight:600}\n"
".updbanner.good{border-left-color:var(--green)}.updbanner.bad{border-left-color:var(--red)}\n"
".updbanner .progress{margin:8px 0 0}\n"
".lockbar{border-left:3px solid var(--amber);background:var(--panel);border-radius:6px;padding:10px 12px;font-size:15px;margin:0}\n"
".confirm{margin-top:12px;border-left:3px solid var(--red);background:var(--recess);border-radius:6px;padding:12px;display:flex;flex-direction:column;gap:10px}\n"
".confirm p{margin:0;font-size:15px;line-height:1.45}\n"
".confirmbtns{display:flex;gap:8px}.confirmbtns .btn{flex:1}\n"
".done{border-left:3px solid var(--green);background:var(--recess);border-radius:6px;padding:12px;margin:12px 0}\n"
".done ol{margin:8px 0;padding-left:20px;line-height:1.5;font-size:15px}\n"
".progress{height:6px;background:var(--recess);border-radius:3px;overflow:hidden;margin:8px 0 4px}\n"
".progress div{height:100%;width:0;background:var(--ink);transition:width .2s}\n"
"\n"
"/* gig */\n"
"canvas{width:100%;height:auto;aspect-ratio:3/1;display:block}\n"
".rtac{aspect-ratio:900/560}\n"
".rtahead{display:flex;justify-content:space-between;align-items:center;margin-bottom:10px}\n"
".rtahead h2{margin:0}\n"
".rollgrid{display:grid;grid-template-columns:52px repeat(3,1fr);align-items:baseline;margin:4px 0 14px;border-bottom:1px solid var(--rule)}\n"
".rollgrid b{font-size:13px;color:var(--mute);font-weight:500;text-align:right;padding-bottom:4px}\n"
".rollgrid span{font-size:14px;color:var(--mute);padding:6px 0;border-top:1px solid var(--rule)}\n"
".rollgrid output{text-align:right;font:600 28px/1.2 var(--font-num),sans-serif;padding:4px 0;border-top:1px solid var(--rule)}\n"
".dose{display:grid;grid-template-columns:auto 1fr;gap:2px 14px;align-items:center;margin:0 0 6px}\n"
".dose b{font:600 40px/1 var(--font-num),sans-serif;grid-row:span 2}\n"
".dose span{font-weight:600}.dose small{color:var(--mute);font-size:13px;line-height:1.35}\n"
".dose > div{display:contents}\n"
"#gigChart{aspect-ratio:3/1;margin-top:4px}\n"
".loginfo{display:flex;justify-content:space-between;align-items:center;gap:10px;margin:10px 0 12px;font-size:15px;color:var(--mute);flex-wrap:wrap}\n"
".loginfo .acts,.acts{display:flex;gap:6px;flex:none}\n"
".markform{display:flex;gap:6px;margin:0 0 12px}\n"
".logrow{display:flex;align-items:center;justify-content:space-between;gap:8px;padding:11px 0;border-top:1px solid var(--rule)}\n"
".logrow div{min-width:0}.logrow small{display:block;color:var(--mute);font-size:14px}\n"
".csvview{position:fixed;inset:0;background:rgba(0,0,0,.7);display:flex;align-items:center;justify-content:center;padding:16px;z-index:40}\n"
".csvview pre{background:var(--panel);color:var(--ink);max-width:100%;max-height:80vh;overflow:auto;padding:12px;border-radius:10px;font-size:12px}\n"
"\n"
"/* network + lists */\n"
".netnow{display:flex;gap:10px;align-items:center;padding:4px 0 12px;border-bottom:1px solid var(--rule)}\n"
".netnow b{display:block;font-size:17px;font-weight:600}.netnow span{font-size:14px;color:var(--mute)}\n"
".netnow .dot{margin:0;flex:none}\n"
".list{display:flex;flex-direction:column;margin-top:4px}\n"
".item{display:flex;align-items:center;justify-content:space-between;gap:8px;padding:11px 0;border-bottom:1px solid var(--rule);font-size:16px;min-width:0;background:none}\n"
".item span{overflow:hidden;text-overflow:ellipsis;white-space:nowrap;min-width:0}\n"
".item small{color:var(--mute);font-size:14px;white-space:nowrap}\n"
"button.item{border:0;border-bottom:1px solid var(--rule);color:var(--ink);width:100%;text-align:left;cursor:pointer;padding:12px 0}\n"
".item .meta{display:flex;flex-direction:column;min-width:0}\n"
".savedwrap{display:flex;flex-direction:column}\n"
".editor{background:var(--recess);border-radius:var(--r-ctl);padding:12px;display:flex;flex-direction:column;gap:8px;margin:6px 0}\n"
".joinform{display:flex;flex-direction:column;gap:6px;margin-top:10px}\n"
".joinform label{font-size:14px;color:var(--mute);margin-top:4px}\n"
".joinform .btn.primary{margin-top:8px}\n"
".pw,.pinrow{display:flex;gap:6px}\n"
".pinrow input{letter-spacing:4px;font-size:18px}\n"
".pinrow input#alTopic{letter-spacing:normal;font-size:16px}\n"
".addr{display:flex;flex-direction:column;gap:6px}\n"
".addr .fixed{display:flex;flex-direction:column;gap:6px}\n"
".addr small{font-size:13px;color:var(--mute)}\n"
".updbox{display:flex;flex-direction:column;gap:10px;padding:12px 0;border-top:1px solid var(--rule);border-bottom:1px solid var(--rule);margin:4px 0 12px}\n"
".updbox b{font-size:18px;font-weight:600}.updbox .notes{color:var(--mute);font-size:14px;line-height:1.45;white-space:pre-line;margin-top:2px}\n"
".fileBtn{display:block}\n"
"body.locked [data-lock]{opacity:.4;pointer-events:none;user-select:none}\n"
".about .row{margin:0;padding:11px 0;border-bottom:1px solid var(--rule)}\n"
".about .row label{color:var(--mute)}\n"
"\n"
"/* settings pages */\n"
".card.subnav{background:transparent;padding:0;margin-bottom:12px}\n"
".card.subnav .lockbar{margin-top:10px}\n"
"#segSub button{position:relative}\n"
"#segSub button.hasupd::after{content:\"\";position:absolute;top:7px;right:10px;width:7px;height:7px;border-radius:50%;background:var(--green)}\n"
"\n"
"/* bottom bar */\n"
".tabs{position:fixed;left:0;right:0;bottom:0;z-index:10;display:flex;justify-content:center;background:rgba(23,25,27,.96);backdrop-filter:blur(10px);-webkit-backdrop-filter:blur(10px);border-top:1px solid var(--rule);padding:0 max(10px,env(safe-area-inset-left)) env(safe-area-inset-bottom) max(10px,env(safe-area-inset-right))}\n"
".tabs button{position:relative;flex:1;max-width:140px;border:0;background:transparent;color:var(--faint);display:flex;flex-direction:column;align-items:center;gap:3px;font-size:13px;font-weight:600;padding:9px 0 8px;cursor:pointer}\n"
".tabs button::before{content:\"\";position:absolute;top:-1px;left:30%;right:30%;height:2px;background:transparent}\n"
".tabs button.on{color:var(--ink)}\n"
".tabs button.on::before{background:var(--ink)}\n"
".tabs button .badge{position:absolute;top:7px;left:calc(50% + 9px);width:7px;height:7px;border-radius:50%;background:var(--green)}\n"
"\n"
"/* iPad and wider: two columns */\n"
"@media (min-width:760px){\n"
"  .wrap{max-width:1040px;display:grid;grid-template-columns:1fr 1fr;gap:12px 20px;align-items:start}\n"
"  header,.limbanner,.updbanner,.card.subnav,.card[data-tab=\"rta\"]{grid-column:1/-1}\n"
"  .card{margin-bottom:0}\n"
"  .big{font-size:150px}\n"
"}\n"
"@media (prefers-reduced-motion:reduce){*{transition:none!important}}\n"
"/* ===================== Themes: Clean, Studio, Bold (Settings > General > Look) ===================== */\n"
"@font-face{font-family:Inter;font-weight:400;font-display:block;src:url(data:font/woff2;base64,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) format(\"woff2\")}\n"
"@font-face{font-family:Inter;font-weight:600;font-display:block;src:url(data:font/woff2;base64,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) format(\"woff2\")}\n"
"@font-face{font-family:Inter;font-weight:800;font-display:block;src:url(data:font/woff2;base64,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) format(\"woff2\")}\n"
"@font-face{font-family:SG;font-weight:500;font-display:block;src:url(data:font/woff2;base64,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) format(\"woff2\")}\n"
"@font-face{font-family:SG;font-weight:700;font-display:block;src:url(data:font/woff2;base64,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) format(\"woff2\")}\n"
"@font-face{font-family:DSEG;font-weight:400;font-display:block;src:url(data:font/woff2;base64,d09GMk9UVE8AAATAAAkAAAAACaAAAAR6AAB1wgAAAAAAAAAAAAAAAAAAAAAAAAAADYtlBmAARAE2AiQDPAQGBYM4ByAb/AgRVZxrAH4e2LaatUJa3qRGjfdxQsgKj8XDABzUYhOvQDdNtHgqY1c5de9kB2RAFVAhUgjJDhaQVSSwC0wvuEujaQmD260PiAOC96y8iPbhrHp9rMfN3RyLdbHv//b31U/uOoKlgTG+O493Zs0GKxSeLiQkuvn0bRsjU/jL2ARrVq3FV9whKVCkTypMTg29OqM+77YimG/mSDsC7YRoZ0y7MNQoyxZbzH9HyUmT5nAn2Sk96acDmUjeY9R0PyEo5Bm1/TIbB3br3tPcx/0vnoonGn+saE8C8T/U96gNC+s3oK8H6xL5nWYu6zWguZEPEPx4ZihI8tEmTLEwsOkjJczD1ml8OXzvVwQirPBYuAIJw4eJnotRHlqN3uNwvAQ/Y5KYjcwZ6qX+o6YZRIvoA3qZ41m1IBItyiZaxmrlxIXao+YBUpwBFDQD0G8LRKVRIcxQMtkuVDRspiMdUGNyAS9IqGpdBovK9ppFZmTBYvMqb1ugFok2f8jaIbRpBQ8WkSgmWv1PE7V5rkF2lgTTJZwDa8g7209q5BzZo391nAGddWLpIAmpcj4i7qwLu5k2ca4sR6UbW/OPpsW4s+6s+obA6ryha4ZaNwr19MPQcX2LEx3OefAobCGboHRex0T6GS1W5GpfH5z2hrTX0nypooM6a0GNnDz8Wq/q9TCJ6gSDFSyAUX92mTSJ9uC8iJYWCxYjlq7joaVsmN1h3LXSwBWcJ2uQj5MCLtnSKQNE2ueNIzjRy2HCRo4zmARasGn7MbSWM8BrS1Wm3SyVajjDOJOzrCzxxi94riHNlJLJ4fVwDbzzszYanGs/7qTgbUzGjJuyznWVvecNckoZTCqTxxzj0XU0/UP6Eg05tLwM7SNahoS0pHNerOQnK5+mFo+eZTTDpkt8U7Plv9JeVLJEWx2iIJqZSiXRKlXDp6lbLho/c4cNoBRpuvpGJ+WWmfuOnfGz9bOYnW3NJl9Fi0WusFpEsl/43KuXKAVcEuWKz5/3Ncu2TJ17iQARBWLlM03hdqmpqsnOEVv3I5pt327HP782CULBAWsOBMyZukXgcgmfWyMEYXsvKldDdXKZfD2jTe2v/bSv7kkiPloB6c74pISc3TFj3Qdg5h3o4x8MgNkNwCw5cmXeldwAa/Ei5jOAil6m47YBvHk8xQ0s0QgMgVvGL8iOHdyaJ4howeXD7wD9+/aBdJ86ePc+OvGr9qJ+g0JfwG33vfpIgE+aDGMX4LtnSLLIKVqn+FGYumQYPgvQ5JcCCwBVBewBAOuh9eOH8y0wd9Tlh1jAFeQBDQBbFRjotAeACWA7DOlgQhhmZ2UYo8rGMBnnQJhcuzhfZcMxUZBogEZmg4aoDXh1hmPmcNVA3XTXk1kfrKEZDtobxEvRlw5aB35shqi8AAI/dKfCQfOgfiL4pjbRG5uaCwZqImCwEf7nHlhJCiVLFYqXqBezgQbqrl6Cvnpp0LrUCLyUXB8gUhf6C96PgUuvAH7j568t0WgAhtiyF8+PIEhINrZKPLPX+o8CzAAAAA==) format(\"woff2\")}\n"
":root{--font-ui:\"Barlow SC\";--font-num:\"Barlow SC\";--num-w:600}\n"
"html,body{font-family:var(--font-ui),-apple-system,\"Segoe UI\",sans-serif}\n"
".big,.stat b,.rollgrid output,.dose b{font-family:var(--font-num),var(--font-ui),sans-serif;font-weight:var(--num-w)}\n"
"input,button{font-family:inherit!important}\n"
".dial{position:relative}\n"
".ring{display:none}\n"
".mood{display:none}\n"
".card.meterctl{margin-top:4px}\n"
"\n"
"/* ---- Clean: like a phone app; light, or dark when the phone is in dark mode ---- */\n"
"body.th-clean{--body:#f2f2f7;--panel:#fff;--recess:#e8e8ed;--raised:#fff;--rule:#e5e5ea;--ink:#111;--mute:#8a8a8e;--faint:#c7c7cc;\n"
"  --green:#34c759;--amber:#ff9f0a;--red:#ff3b30;--accent:#007aff;--segon:#fff;--font-ui:Inter;--font-num:Inter;--num-w:800}\n"
"body.th-clean.dark{--body:#000;--panel:#1c1c1e;--recess:#2c2c2e;--raised:#2c2c2e;--rule:#38383a;--ink:#fff;--mute:#8e8e93;--faint:#48484a;--accent:#0a84ff;--segon:#636366}\n"
"body.th-clean{background:var(--body);font-size:16px}\n"
"body.th-clean header{border:0;margin-bottom:4px}\n"
"body.th-clean header h1{font-weight:800;font-size:20px}\n"
"body.th-clean .card{border-radius:16px}\n"
"body.th-clean #meter{text-align:center}\n"
"body.th-clean .modes{justify-content:center;gap:8px}\n"
"body.th-clean .modes .r{margin:0}\n"
"body.th-clean #holdBtn{position:absolute;right:0}\n"
"body.th-clean #meter .modes{position:relative}\n"
"body.th-clean .dial{width:min(300px,78vw);aspect-ratio:1;margin:8px auto 0}\n"
"body.th-clean .ring{display:block;position:absolute;inset:0;width:100%;height:100%}\n"
"body.th-clean .ring path{fill:none;stroke-width:22;stroke-linecap:round}\n"
"body.th-clean .ring .track{stroke:var(--recess)}\n"
"body.th-clean .ring circle{fill:var(--ink)}\n"
"body.th-clean .big{position:absolute;left:0;right:0;top:36%;transform:translateY(-50%);text-align:center;font-size:84px;letter-spacing:-3px;margin:0;line-height:1}\n"
"body.th-clean .big.word{font-size:40px;letter-spacing:-1px}\n"
"body.th-clean .unit{position:absolute;left:0;right:0;top:56%;text-align:center;margin:0}\n"
"body.th-clean .ladder,body.th-clean .scale{display:none}\n"
"body.th-clean .stats{border:0;gap:8px;margin:0 0 6px}\n"
"body.th-clean .stat{border:0;padding:10px 12px;background:var(--recess);border-radius:12px}\n"
"body.th-clean .stat:first-child{padding-left:12px}\n"
"body.th-clean .stat b{font-size:22px;font-weight:800}\n"
"body.th-clean .seg{border-radius:10px;padding:2px}\n"
"body.th-clean .seg button{border-radius:8px;color:var(--ink);font-weight:600}\n"
"body.th-clean .seg button.on{background:var(--segon);color:var(--ink);box-shadow:0 1px 3px rgba(0,0,0,.14)}\n"
"body.th-clean .btn{background:var(--raised);box-shadow:0 0 0 1px var(--rule);border-radius:12px;font-weight:600}\n"
"body.th-clean .btn.primary,body.th-clean .btn.on{background:var(--accent);color:#fff;box-shadow:none}\n"
"body.th-clean .btn.danger{color:var(--red)}\n"
"body.th-clean .step button{background:var(--recess)}\n"
"body.th-clean .tabs{background:color-mix(in srgb,var(--panel) 92%,transparent);border-top:1px solid var(--rule)}\n"
"body.th-clean .tabs button{color:var(--mute)} body.th-clean .tabs button.on{color:var(--accent)} body.th-clean .tabs button::before{display:none}\n"
"body.th-clean .toast{background:var(--ink);color:var(--body)}\n"
"body.th-clean a{color:var(--accent)}\n"
"body.th-clean h2{font-weight:800}\n"
"\n"
"/* ---- Studio: rack gear; glowing 7-segment digits, LED meter, backlit buttons ---- */\n"
"body.th-studio{--body:#0c0e10;--panel:#15181c;--recess:#0b0d0f;--raised:#22262b;--rule:#262b31;--ink:#d8dde3;--mute:#6d7580;--faint:#3b4249;\n"
"  --green:#4dff7c;--amber:#ffc23d;--red:#ff4a3d;--accent:#62ff8e;--font-ui:\"Barlow SC\";--font-num:DSEG;--num-w:400;\n"
"  background:radial-gradient(120% 60% at 50% 0%,#20242a 0%,#0c0e10 70%) fixed #0c0e10}\n"
"body.th-studio header{border-color:#23272c}\n"
"body.th-studio header h1,body.th-studio h2,body.th-studio h3,body.th-studio .modes,body.th-studio .stat span,body.th-studio .ctl>span,\n"
"body.th-studio .tabs button,body.th-studio .seg button,body.th-studio .btn,body.th-studio .rollgrid b,body.th-studio .rollgrid span{text-transform:uppercase;letter-spacing:1.6px}\n"
"body.th-studio .card{background:linear-gradient(#181b20,#13161a);box-shadow:inset 0 1px 0 rgba(255,255,255,.04)}\n"
"body.th-studio #meter{background:linear-gradient(#071008,#0b160c);border-radius:14px;padding:16px;box-shadow:inset 0 2px 12px rgba(0,0,0,.85),0 0 0 1px #1d2a1f}\n"
"body.th-studio .modes,body.th-studio .modes .r,body.th-studio .modes #wLbl,body.th-studio .unit{color:#4fdc7a}\n"
"body.th-studio .modes{font-size:14px}\n"
"body.th-studio .big{position:relative;font-size:84px;letter-spacing:0;margin:18px 0 8px;text-shadow:0 0 14px currentColor}\n"
"body.th-studio .big::before{content:\"888.8\";position:absolute;right:0;color:rgba(98,255,142,.07);text-shadow:none}\n"
"body.th-studio .big.word{font-family:\"Barlow SC\";font-size:56px;text-shadow:none}\n"
"body.th-studio .big.word::before{content:none}\n"
"body.th-studio .ladder{background:transparent;padding:0;gap:3px;height:16px}\n"
"body.th-studio .ladder i{background:#132016;border-radius:2px}\n"
"body.th-studio .ladder i.g{box-shadow:0 0 6px var(--green)} body.th-studio .ladder i.a{box-shadow:0 0 6px var(--amber)} body.th-studio .ladder i.r{box-shadow:0 0 6px var(--red)}\n"
"body.th-studio .scale{color:#3f7c52}\n"
"body.th-studio .stats{border:0;gap:8px}\n"
"body.th-studio .stat{border:0;padding:10px;background:#15181c;border-radius:10px;box-shadow:inset 0 1px 0 rgba(255,255,255,.04)}\n"
"body.th-studio .stat:first-child{padding-left:10px}\n"
"body.th-studio .stat span{font-size:11px}\n"
"body.th-studio .stat b{font-size:22px;margin-top:6px;color:#cfd6dd}\n"
"body.th-studio .seg{background:transparent;gap:8px;padding:0}\n"
"body.th-studio .seg button,body.th-studio .btn,body.th-studio .step button{background:linear-gradient(#262a30,#1a1d21);box-shadow:inset 0 1px 0 rgba(255,255,255,.07),0 2px 3px rgba(0,0,0,.5);color:#aab2bb;border-radius:9px}\n"
"body.th-studio .seg button.on,body.th-studio .btn.on,body.th-studio .btn.primary{color:var(--accent);background:linear-gradient(#20302a,#17211b);box-shadow:inset 0 1px 0 rgba(255,255,255,.07),0 0 0 1px #2c5a37,0 0 12px rgba(98,255,142,.15)}\n"
"body.th-studio .btn.danger{color:var(--red)}\n"
"body.th-studio .rollgrid output,body.th-studio .dose b{font-size:22px}\n"
"body.th-studio .tabs{background:rgba(12,14,16,.96);border-color:#23272c}\n"
"body.th-studio .tabs button.on{color:var(--accent)} body.th-studio .tabs button.on::before{background:var(--accent);box-shadow:0 0 8px var(--accent)}\n"
"body.th-studio .toast{background:#0b160c;color:var(--accent);box-shadow:0 0 0 1px #2c5a37}\n"
"body.th-studio .check input,body.th-studio input[type=range]{accent-color:var(--accent)}\n"
"\n"
"/* ---- Bold: the whole page turns green, amber or red with the level ---- */\n"
"body.th-bold{--lvl:#1fbf5f;--panel:#fff;--recess:#f1f1f1;--raised:#f1f1f1;--rule:#e8e8e8;--ink:#111;--mute:#6b6b6b;--faint:#b9b9b9;\n"
"  --green:#1fbf5f;--amber:#f5a524;--red:#eb443a;--accent:#111;--font-ui:SG;--font-num:SG;--num-w:700;background:var(--lvl);transition:background-color .4s}\n"
"body.th-bold[data-level=a]{--lvl:#f5a524} body.th-bold[data-level=r]{--lvl:#eb443a} body.th-bold[data-level=off]{--lvl:#4a4d52}\n"
"body.th-bold header{border-color:rgba(255,255,255,.3)}\n"
"body.th-bold header h1,body.th-bold .status,body.th-bold .status > span:not(.bat){color:#fff!important}\n"
"body.th-bold .bat{background:rgba(255,255,255,.92);border-radius:99px;padding:2px 7px}\n"
"body.th-bold .dot{box-shadow:none;background:#fff}\n"
"body.th-bold #meter{color:#fff}\n"
"body.th-bold .modes,body.th-bold .modes .r,body.th-bold .modes #wLbl,body.th-bold .unit{color:rgba(255,255,255,.92)}\n"
"body.th-bold .modes{font-size:20px;font-weight:500}\n"
"body.th-bold #holdBtn{background:rgba(255,255,255,.22);color:#fff;box-shadow:none}\n"
"body.th-bold #holdBtn.on{background:#fff;color:#111}\n"
"body.th-bold .big{color:#fff!important;text-align:left;font-size:140px;letter-spacing:-6px;line-height:.9;margin:16px 0 6px}\n"
"body.th-bold .big.word{font-size:64px;letter-spacing:-2px}\n"
"body.th-bold .unit{text-align:left;font-size:18px}\n"
"body.th-bold .mood{display:block;font-size:20px;color:#fff;margin:2px 0 18px;min-height:28px}\n"
"body.th-bold .ladder{gap:0;padding:0;height:10px;border-radius:99px;background:rgba(0,0,0,.18);overflow:hidden}\n"
"body.th-bold .ladder i,body.th-bold .ladder i.pk{background:transparent;border-radius:0}\n"
"body.th-bold .ladder i.g,body.th-bold .ladder i.a,body.th-bold .ladder i.r{background:#fff}\n"
"body.th-bold .scale{color:rgba(255,255,255,.75)}\n"
"body.th-bold .card{background:#fff;color:#111;border-radius:24px}\n"
"body.th-bold #meter{background:transparent}\n"
"body.th-bold .card.subnav{background:transparent}\n"
"body.th-bold .stats{border:0}\n"
"body.th-bold .stat{border:0}\n"
"body.th-bold .stat b{font-size:28px}\n"
"body.th-bold .seg{background:#f1f1f1;border-radius:99px;padding:4px}\n"
"body.th-bold .seg button{border-radius:99px;color:#666;font-weight:700}\n"
"body.th-bold .seg button.on{background:#111;color:#fff}\n"
"body.th-bold .card.subnav .seg{background:rgba(255,255,255,.25)}\n"
"body.th-bold .card.subnav .seg button{color:#fff}\n"
"body.th-bold .card.subnav .seg button.on{background:#fff;color:#111}\n"
"body.th-bold .btn{background:#f1f1f1;color:#111;box-shadow:none;border-radius:99px;font-weight:700}\n"
"body.th-bold .btn.primary,body.th-bold .btn.on{background:#111;color:#fff}\n"
"body.th-bold .btn.danger{color:var(--red)}\n"
"body.th-bold .step button{background:#f1f1f1;border-radius:99px}\n"
"body.th-bold .tabs{background:#fff;border-top:0;box-shadow:0 -1px 0 rgba(0,0,0,.06)}\n"
"body.th-bold .tabs button{color:#999} body.th-bold .tabs button.on{color:#111} body.th-bold .tabs button::before{display:none}\n"
"body.th-bold .toast{background:#111;color:#fff}\n"
"body.th-bold .limbanner{box-shadow:0 0 0 2px #fff}\n"
"body.th-bold .lockbar,body.th-bold .updbanner{background:#fff;color:#111}\n"
"body.th-bold .check input,body.th-bold input[type=range]{accent-color:#111}\n"
"\n"
"\n"
"/* ---- Studio, light: a pale aluminium chassis; the LCD stays dark like real gear ---- */\n"
"body.th-studio.light{--body:#d4d7db;--panel:#eceef0;--recess:#d3d6da;--raised:#f2f3f5;--rule:#c4c8cd;--ink:#1d2126;--mute:#646a71;--faint:#9aa0a7;\n"
"  background:linear-gradient(#e7e9eb,#cdd1d5) fixed #d4d7db}\n"
"body.th-studio.light header{border-color:#c4c8cd}\n"
"body.th-studio.light .card{background:linear-gradient(#f4f5f6,#e6e8eb);box-shadow:inset 0 1px 0 #fff,0 1px 2px rgba(0,0,0,.08)}\n"
"body.th-studio.light #meter{background:linear-gradient(#071008,#0b160c)}\n"
"body.th-studio.light .stat{background:#e2e5e8;box-shadow:inset 0 1px 2px rgba(0,0,0,.08)}\n"
"body.th-studio.light .stat b{color:#1d2126}\n"
"body.th-studio.light .seg button,body.th-studio.light .btn,body.th-studio.light .step button{background:linear-gradient(#fbfbfc,#e3e6e9);box-shadow:inset 0 1px 0 #fff,0 1px 2px rgba(0,0,0,.18);color:#3a3f45}\n"
"body.th-studio.light .seg button.on,body.th-studio.light .btn.on,body.th-studio.light .btn.primary{color:#127a35;background:linear-gradient(#f3fbf5,#dcefe2);box-shadow:inset 0 1px 0 #fff,0 0 0 1px #6fbf88}\n"
"body.th-studio.light .tabs{background:rgba(232,234,236,.96);border-color:#c4c8cd}\n"
"body.th-studio.light .tabs button{color:#7d838a} body.th-studio.light .tabs button.on{color:#127a35} body.th-studio.light .tabs button.on::before{background:#127a35;box-shadow:none}\n"
"body.th-studio.light .check input,body.th-studio.light input[type=range]{accent-color:#127a35}\n"
"body.th-studio.light .looks button.on{box-shadow:0 0 0 2px #127a35}\n"
"\n"
"/* ---- Bold, dark: the level colour stays, the panels go dark ---- */\n"
"body.th-bold.dark{--panel:#1c1c1e;--recess:#2c2c2e;--raised:#2c2c2e;--rule:#38383a;--ink:#fff;--mute:#a1a1a6;--faint:#5a5a5e;--accent:#fff}\n"
"body.th-bold.dark[data-level=g]{--lvl:#15904a} body.th-bold.dark[data-level=a]{--lvl:#c9831a} body.th-bold.dark[data-level=r]{--lvl:#c0352d} body.th-bold.dark[data-level=off]{--lvl:#2a2c30}\n"
"body.th-bold.dark .card{background:#1c1c1e;color:#fff}\n"
"body.th-bold.dark #meter,body.th-bold.dark .card.subnav{background:transparent}\n"
"body.th-bold.dark .seg{background:#2c2c2e}\n"
"body.th-bold.dark .seg button{color:#a1a1a6}\n"
"body.th-bold.dark .seg button.on{background:#fff;color:#111}\n"
"body.th-bold.dark .btn,body.th-bold.dark .step button{background:#2c2c2e;color:#fff}\n"
"body.th-bold.dark .btn.primary,body.th-bold.dark .btn.on{background:#fff;color:#111}\n"
"body.th-bold.dark .btn.danger{color:var(--red)}\n"
"body.th-bold.dark .tabs{background:#1c1c1e;box-shadow:none}\n"
"body.th-bold.dark .tabs button{color:#8e8e93} body.th-bold.dark .tabs button.on{color:#fff}\n"
"body.th-bold.dark .toast{background:#fff;color:#111}\n"
"body.th-bold.dark .lockbar,body.th-bold.dark .updbanner{background:#1c1c1e;color:#fff}\n"
"body.th-bold.dark .check input,body.th-bold.dark input[type=range]{accent-color:#fff}\n"
"body.th-bold.dark .looks button.on{box-shadow:0 0 0 2px #fff}\n"
"\n"
"/* look picker */\n"
".looks{display:grid;grid-template-columns:repeat(3,1fr);gap:8px}\n"
".looks button{border:0;border-radius:12px;padding:0 0 8px;background:var(--recess);color:var(--ink);font-weight:600;font-size:14px;cursor:pointer;overflow:hidden;box-shadow:0 0 0 2px transparent}\n"
".looks button.on{box-shadow:0 0 0 2px var(--accent)}\n"
".looks button i{display:flex;align-items:center;justify-content:center;height:54px;margin-bottom:6px;font-style:normal;font-size:24px}\n"
".looks .lk0 i{background:#f2f2f7;color:#111;font:800 24px Inter}\n"
".looks .lk1 i{background:#071008;color:#62ff8e;font:24px DSEG;text-shadow:0 0 8px rgba(98,255,142,.6)}\n"
".looks .lk2 i{background:#f5a524;color:#fff;font:700 26px SG}\n"
"</style>\n"
"</head>\n"
"<body>\n"
"<div class=\"wrap\">\n"
"  <header>\n"
"    <h1>PocketSPL</h1>\n"
"    <div class=\"status\"><span class=\"bat\" id=\"batWrap\"><svg id=\"batBolt\" viewBox=\"0 0 8 12\" width=\"8\" height=\"12\" aria-hidden=\"true\" style=\"display:none\"><path d=\"M5 0 0 7h3l-1 5 6-8H5l1-4z\" fill=\"currentColor\"/></svg><span id=\"batt\">--%</span><svg viewBox=\"0 0 24 12\" width=\"24\" height=\"12\" aria-hidden=\"true\"><rect x=\".75\" y=\".75\" width=\"19.5\" height=\"10.5\" rx=\"2.5\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"1.5\"/><rect x=\"21\" y=\"4\" width=\"2\" height=\"4\" rx=\"1\" fill=\"currentColor\"/><rect id=\"batFill\" x=\"2.5\" y=\"2.5\" width=\"16\" height=\"7\" rx=\"1.2\" fill=\"currentColor\"/></svg></span><span><i class=\"dot\" id=\"dot\"></i><span id=\"conn\">Connecting</span></span></div>\n"
"  </header>\n"
"  <div class=\"limbanner\" id=\"limBanner\" hidden></div>\n"
"  <div class=\"updbanner\" id=\"updBanner\" hidden><span id=\"updBanText\"></span><div class=\"progress\" id=\"updBanProg\"><div id=\"updBanBar\"></div></div></div>\n"
"\n"
"  <div class=\"card hero\" data-tab=\"meter\" id=\"meter\">\n"
"    <div class=\"modes\"><span id=\"wLbl\">dB(A)</span><span class=\"r\" id=\"rLbl\">Fast</span><button class=\"btn small\" id=\"holdBtn\" type=\"button\">Hold</button></div>\n"
"    <div class=\"dial\">\n"
"      <svg class=\"ring\" viewBox=\"0 0 300 300\" aria-hidden=\"true\"><path class=\"track\" d=\"M64 236 A121 121 0 1 1 236 236\"/><path class=\"lit\" id=\"ringLit\" d=\"M64 236 A121 121 0 1 1 236 236\" stroke-dasharray=\"570\" stroke-dashoffset=\"570\"/><circle id=\"ringPk\" r=\"7\" cx=\"-20\" cy=\"-20\"/></svg>\n"
"      <div class=\"big\" id=\"db\">--</div>\n"
"      <div class=\"unit\" id=\"unit\">dB SPL</div>\n"
"    </div>\n"
"    <div class=\"mood\" id=\"mood\"></div>\n"
"    <div class=\"ladder\" id=\"ladder\" aria-hidden=\"true\"></div>\n"
"    <div class=\"scale\" id=\"scale\" aria-hidden=\"true\"></div>\n"
"  </div>\n"
"\n"
"  <div class=\"card meterctl\" data-tab=\"meter\">\n"
"    <div class=\"stats\">\n"
"      <div class=\"stat\"><b id=\"max\">--</b><span>Max</span></div>\n"
"      <div class=\"stat\"><b id=\"leq\">--</b><span>Leq</span></div>\n"
"      <div class=\"stat\"><b id=\"time\">--</b><span>Leq time</span></div>\n"
"    </div>\n"
"    <div class=\"ctl\"><span>Weighting</span><div class=\"seg\" id=\"segW\"><button data-v=\"0\">A</button><button data-v=\"1\">C</button><button data-v=\"2\">Z</button></div></div>\n"
"    <div class=\"ctl\"><span>Response</span><div class=\"seg\" id=\"segR\"><button data-v=\"0\">Fast</button><button data-v=\"1\">Slow</button><button data-v=\"2\">Leq</button></div></div>\n"
"    <div class=\"ctl\"><span>Meter screen</span><div class=\"seg\" id=\"segScr\"><button data-v=\"lvl\">Level</button><button data-v=\"3\">RTA</button><button data-v=\"4\">Gig</button></div></div>\n"
"    <button class=\"btn wide\" id=\"reset\">Reset Max and Leq</button>\n"
"  </div>\n"
"\n"
"  <div class=\"card\" data-tab=\"rta\">\n"
"    <div class=\"rtahead\"><h2 id=\"rtaTitle\">RTA</h2><button class=\"btn small\" id=\"freeze\">Freeze</button></div>\n"
"    <canvas id=\"rta\" width=\"900\" height=\"560\" class=\"rtac\"></canvas>\n"
"  </div>\n"
"\n"
"  <div class=\"card\" data-tab=\"gig\" id=\"gigCard\">\n"
"    <div class=\"rtahead\"><h2>Gig</h2><button class=\"btn small\" id=\"markBtn\" type=\"button\">Add marker</button></div>\n"
"    <div class=\"markform\" id=\"markForm\" hidden>\n"
"      <input id=\"markText\" maxlength=\"24\" placeholder=\"e.g. Doors, Headliner on\" autocomplete=\"off\">\n"
"      <button class=\"btn small primary\" id=\"markSave\" type=\"button\">Add</button>\n"
"    </div>\n"
"    <div class=\"rollgrid\">\n"
"      <span></span><b>1 min</b><b>5 min</b><b>15 min</b>\n"
"      <span>LAeq</span><output id=\"ra0\">--</output><output id=\"ra1\">--</output><output id=\"ra2\">--</output>\n"
"      <span>LCeq</span><output id=\"rc0\">--</output><output id=\"rc1\">--</output><output id=\"rc2\">--</output>\n"
"    </div>\n"
"    <div class=\"dose\"><div><b id=\"dose\">--%</b> <span>Hearing dose</span></div><small>Share of the daily limit at this spot: 85 dB(A) over 8 hours, the Australian workplace standard.</small></div>\n"
"    <h3>LAeq per minute</h3>\n"
"    <canvas id=\"gigChart\" width=\"900\" height=\"300\"></canvas>\n"
"    <div class=\"loginfo\"><span id=\"logInfo\">Log starting…</span>\n"
"      <div class=\"acts\"><a class=\"btn small\" id=\"dlCsv\" href=\"/api/log.csv\" download=\"PocketSPL-log.csv\">Download</a></div></div>\n"
"    <button class=\"btn wide danger\" id=\"resetGig\" type=\"button\" data-lock>Reset gig</button>\n"
"    <div class=\"confirm\" id=\"resetConfirm\" hidden>\n"
"      <b>Start the gig again?</b>\n"
"      <p>This starts a new log and clears the markers, hearing dose, rolling LAeq, MAX and Leq. The current log is kept under <b>Saved gigs</b>.</p>\n"
"      <div class=\"confirmbtns\"><button class=\"btn\" id=\"resetNo\" type=\"button\">Cancel</button><button class=\"btn danger solid\" id=\"resetYes\" type=\"button\">Yes, reset</button></div>\n"
"    </div>\n"
"    <p class=\"note\" id=\"logSaveNote\">Saved in the meter every minute (up to 24 hours), so switching off doesn't lose it.</p>\n"
"    <h3>Saved gigs</h3>\n"
"    <div id=\"savedLogs\"><p class=\"note\">Loading…</p></div>\n"
"    <p class=\"note\">A gig lands here when you reset it or switch the meter off. When storage is full the oldest goes first.</p>\n"
"  </div>\n"
"\n"
"  <div class=\"card\" data-tab=\"meter\">\n"
"    <h2>Last 60 seconds</h2>\n"
"    <canvas id=\"graph\" width=\"900\" height=\"300\"></canvas>\n"
"  </div>\n"
"\n"
"  <div class=\"card subnav\" data-tab=\"settings\">\n"
"    <div class=\"seg\" id=\"segSub\"><button type=\"button\" data-s=\"general\">General</button><button type=\"button\" data-s=\"wifi\">WiFi</button><button type=\"button\" data-s=\"software\" id=\"subSoftware\">Software</button></div>\n"
"    <div class=\"lockbar\" id=\"lockBar\" hidden>Settings are locked. To change them, enter the PIN on the <b>General</b> page under <b>Lock</b>.</div>\n"
"  </div>\n"
"\n"
"  <div class=\"card\" data-tab=\"settings\" data-sub=\"general\">\n"
"    <h2>Look</h2>\n"
"    <div class=\"looks\" id=\"looks\"><button type=\"button\" class=\"lk0\" data-v=\"0\"><i>89.7</i>Clean</button><button type=\"button\" class=\"lk1\" data-v=\"1\"><i>89.7</i>Studio</button><button type=\"button\" class=\"lk2\" data-v=\"2\"><i>89.7</i>Bold</button></div>\n"
"    <div class=\"ctl\"><span>Appearance</span><div class=\"seg\" id=\"segMode\"><button type=\"button\" data-v=\"auto\">Auto</button><button type=\"button\" data-v=\"light\">Light</button><button type=\"button\" data-v=\"dark\">Dark</button></div></div>\n"
"    <p class=\"note\">The look changes the phone and the meter's screen. Appearance is just for this phone: Auto follows the phone's light or dark setting.</p>\n"
"  </div>\n"
"\n"
"  <div class=\"card\" data-tab=\"settings\" data-sub=\"general\">\n"
"    <h2>Alarm and screen</h2>\n"
"    <div data-lock>\n"
"    <label class=\"check\"><input type=\"checkbox\" id=\"limOn\"> Limit alarm</label>\n"
"    <div class=\"row\"><label>Limit</label><div class=\"step\"><button data-l=\"-1\" type=\"button\">−</button><output id=\"limDb\">--</output><button data-l=\"1\" type=\"button\">+</button></div></div>\n"
"    <div class=\"seg\" id=\"segLimWin\"><button data-v=\"0\">1 min</button><button data-v=\"1\">5 min</button><button data-v=\"2\">15 min</button></div>\n"
"    <div style=\"height:8px\"></div>\n"
"    <div class=\"seg\" id=\"segLimW\"><button data-v=\"0\">LAeq</button><button data-v=\"1\">LCeq</button></div>\n"
"    <div class=\"row\"><button class=\"btn wide\" id=\"whoStd\" type=\"button\">Use venue standard: 100 dB(A) over 15 min</button></div>\n"
"    <p class=\"note\">The WHO standard for venues is 100 dB LAeq over 15 minutes. The meter turns amber within 3 dB of the limit and red over it.</p>\n"
"    <h3>Screen</h3>\n"
"    <div class=\"seg\" id=\"segDim\"><button data-v=\"0\">Never dim</button><button data-v=\"30\">30 s</button><button data-v=\"60\">1 min</button><button data-v=\"300\">5 min</button></div>\n"
"    <p class=\"note\">Dims the meter's screen when the button isn't used. The first press wakes it.</p>\n"
"    <div class=\"row\"><label>Brightness</label><input type=\"range\" id=\"bright\" min=\"1\" max=\"15\" step=\"1\"></div>\n"
"    <div class=\"row\"><label>Amber from</label><div class=\"step\"><button data-k=\"amber\" data-d=\"-1\">−</button><output id=\"amber\">--</output><button data-k=\"amber\" data-d=\"1\">+</button></div></div>\n"
"    <div class=\"row\"><label>Red from</label><div class=\"step\"><button data-k=\"red\" data-d=\"-1\">−</button><output id=\"red\">--</output><button data-k=\"red\" data-d=\"1\">+</button></div></div>\n"
"    </div>\n"
"  </div>\n"
"\n"
"  <div class=\"card\" data-tab=\"settings\" data-sub=\"general\">\n"
"    <h2>Alerts</h2>\n"
"    <p class=\"note\" id=\"alState\"></p>\n"
"    <div data-lock>\n"
"    <label class=\"check\"><input type=\"checkbox\" id=\"alOn\"> Send an alert when it's over the limit</label>\n"
"    <div class=\"ctl\"><span>Over for</span><div class=\"seg\" id=\"segAlHold\"><button type=\"button\" data-v=\"0\">Right away</button><button type=\"button\" data-v=\"2\">2 min</button><button type=\"button\" data-v=\"5\">5 min</button><button type=\"button\" data-v=\"10\">10 min</button></div></div>\n"
"    <div class=\"ctl\"><span>Repeat every</span><div class=\"seg\" id=\"segAlRep\"><button type=\"button\" data-v=\"15\">15 min</button><button type=\"button\" data-v=\"30\">30 min</button><button type=\"button\" data-v=\"60\">1 hour</button></div></div>\n"
"    <label class=\"check\"><input type=\"checkbox\" id=\"alBack\"> Also when it's back under</label>\n"
"    <h3>Phone</h3>\n"
"    <p class=\"note\">Install the free <b>ntfy</b> app (App Store or Google Play), tap <b>+</b> and add this topic. Keep it private: anyone with the topic can see the alerts.</p>\n"
"    <div class=\"pinrow\"><input id=\"alTopic\" autocomplete=\"off\" autocapitalize=\"off\" spellcheck=\"false\" aria-label=\"ntfy topic\"><button class=\"btn\" id=\"alCopy\" type=\"button\">Copy</button></div>\n"
"    <div class=\"row\"><button class=\"btn\" id=\"alNew\" type=\"button\">New topic</button></div>\n"
"    <div class=\"schbtns\" style=\"margin-top:12px\"><button class=\"btn\" id=\"alSave\" type=\"button\">Save</button><button class=\"btn\" id=\"alTest\" type=\"button\">Send a test</button></div>\n"
"    </div>\n"
"    <p class=\"note\">Uses the limit, the average (1, 5 or 15 min) and LAeq or LCeq from <b>Alarm and screen</b>. The meter needs to be on a WiFi network with internet (Settings → WiFi).</p>\n"
"  </div>\n"
"\n"
"  <div class=\"card\" data-tab=\"settings\" data-sub=\"general\" data-lock>\n"
"    <h2>Calibration</h2>\n"
"    <div class=\"row\"><label>Offset</label><div class=\"step\"><button data-c=\"-1\">−1</button><button data-c=\"-0.1\">−.1</button><output id=\"cal\">--</output><button data-c=\"0.1\">+.1</button><button data-c=\"1\">+1</button></div></div>\n"
"    <div class=\"row\"><button class=\"btn wide\" id=\"cal94\">Calibrate with 94 dB calibrator</button></div>\n"
"    <p class=\"note\">Or set it beside a trusted meter and nudge the offset until they agree.</p>\n"
"    <div id=\"fcalBox\" hidden><p class=\"note\">Factory calibration for this meter: <b id=\"fcalVal\">--</b></p>\n"
"      <button class=\"btn wide\" id=\"fcalBack\" type=\"button\">Back to factory calibration</button></div>\n"
"  </div>\n"
"\n"
"  <div class=\"card\" data-tab=\"settings\" data-sub=\"wifi\" data-lock>\n"
"    <h2>Network</h2>\n"
"    <div class=\"netnow\"><i class=\"dot\" id=\"netDot\"></i><div><b id=\"netName\">--</b><span id=\"netAddr\"></span></div></div>\n"
"    <p class=\"note\" id=\"netMsg\"></p>\n"
"    <div class=\"done\" id=\"netDone\" hidden>\n"
"      <b>Joined <span id=\"doneName\"></span>.</b>\n"
"      <ol><li>On your phone, join <b id=\"doneName2\"></b> in WiFi settings.</li>\n"
"      <li>Open <b id=\"doneIp\"></b> (on iPhone, <b id=\"doneMdns\">pocketspl.local</b> also works).</li></ol>\n"
"      <p class=\"note\">The meter's own network (<span id=\"doneAp\">PocketSPL</span>) switches off in 2 minutes.</p>\n"
"      <button class=\"btn wide\" id=\"netFinish\">Switch it off now</button>\n"
"    </div>\n"
"\n"
"    <h3>Saved networks</h3>\n"
"    <div id=\"savedList\" class=\"list\"></div>\n"
"    <label class=\"check\"><input type=\"checkbox\" id=\"joinSaved\"> Join a saved network when the meter switches on</label>\n"
"\n"
"    <h3>Add a network</h3>\n"
"    <button class=\"btn wide\" id=\"scanBtn\">Find networks</button>\n"
"    <div id=\"scanList\" class=\"list\"></div>\n"
"    <div id=\"joinForm\" class=\"joinform\" hidden>\n"
"      <label for=\"ssid\">Network name</label>\n"
"      <input id=\"ssid\" autocomplete=\"off\" autocapitalize=\"off\" spellcheck=\"false\">\n"
"      <label for=\"pass\">Password</label>\n"
"      <div class=\"pw\"><input id=\"pass\" type=\"password\" autocomplete=\"off\" autocapitalize=\"off\" spellcheck=\"false\"><button class=\"btn small\" id=\"showPw\" type=\"button\">Show</button></div>\n"
"      <label>IP address</label>\n"
"      <div class=\"addr\" id=\"jAddr\"></div>\n"
"      <button class=\"btn wide primary\" id=\"joinBtn\">Join this network</button>\n"
"      <p class=\"note\">Your phone may drop off the meter for a few seconds while it joins. It comes back by itself.</p>\n"
"    </div>\n"
"    <p class=\"note\"><a href=\"#\" id=\"manual\">Type a network name instead</a></p>\n"
"\n"
"    <h3>WiFi power</h3>\n"
"    <div class=\"row\"><button class=\"btn wide danger\" id=\"wifiOff\">Turn WiFi off (saves battery)</button></div>\n"
"    <p class=\"note\">To turn WiFi back on, hold the meter's button for 3 seconds.</p>\n"
"  </div>\n"
"\n"
"  <div class=\"card\" data-tab=\"settings\" data-sub=\"wifi\" data-lock>\n"
"    <h2>Meter's own WiFi</h2>\n"
"    <p class=\"note\">The network the meter makes itself when it isn't on another WiFi. Now: <b id=\"apNow\">PocketSPL</b><span id=\"apDefNote\"></span></p>\n"
"    <div class=\"joinform\">\n"
"      <label for=\"apNameIn\">Name</label>\n"
"      <input id=\"apNameIn\" maxlength=\"32\" autocomplete=\"off\" autocapitalize=\"off\" spellcheck=\"false\">\n"
"      <label for=\"apPassIn\">New password</label>\n"
"      <div class=\"pw\"><input id=\"apPassIn\" type=\"password\" maxlength=\"63\" placeholder=\"8 or more characters\" autocomplete=\"new-password\" autocapitalize=\"off\" spellcheck=\"false\"><button class=\"btn small\" id=\"apShow\" type=\"button\">Show</button></div>\n"
"      <p class=\"note\">Leave the password empty to keep the current one.</p>\n"
"      <button class=\"btn wide primary\" id=\"apSave\" type=\"button\">Save name and password</button>\n"
"      <button class=\"btn wide\" id=\"apDefault\" type=\"button\">Back to PocketSPL / pocketspl</button>\n"
"    </div>\n"
"    <p class=\"note\">If your phone is on the meter's own WiFi, it drops off after saving. Join the new name, then open http://192.168.4.1 again.</p>\n"
"    <p class=\"note\"><b>Forgot the password?</b> Switch the meter off, hold its button, switch it on and keep holding for 5 seconds. It goes back to PocketSPL / pocketspl.</p>\n"
"  </div>\n"
"\n"
"  <div class=\"card\" data-tab=\"settings\" data-sub=\"software\" data-lock>\n"
"    <h2>Software</h2>\n"
"    <div class=\"row\"><label>Version on the meter</label><output id=\"fwVer\">--</output></div>\n"
"    <div class=\"updbox\" id=\"updBox\">\n"
"      <span id=\"updMsg\">Checking for updates…</span>\n"
"      <div id=\"updAvail\" hidden><b id=\"updVer\"></b><div class=\"notes\" id=\"updNotes\"></div></div>\n"
"      <button class=\"btn wide primary\" id=\"updInstall\" type=\"button\" hidden>Install update</button>\n"
"      <div id=\"updProg\" hidden><div class=\"progress\"><div id=\"updBar\"></div></div><small id=\"updStep\"></small></div>\n"
"      <button class=\"btn wide\" id=\"updCheck\" type=\"button\">Check again</button>\n"
"    </div>\n"
"    <label class=\"btn wide fileBtn\" for=\"updFile\" id=\"updFileBtn\">Install from a file…</label>\n"
"    <input type=\"file\" id=\"updFile\" accept=\".bin,application/octet-stream\" hidden>\n"
"    <p class=\"note\" id=\"updNoOta\" hidden><b>This meter can't update itself yet.</b> Load the software once by USB with the <b>Partition Scheme</b> from the build guide. After that, updates work from here.</p>\n"
"    <p class=\"note\">Updates keep your settings, calibration, saved networks and saved gigs. The meter restarts and starts a new gig log. Checking online needs the phone to have internet.</p>\n"
"  </div>\n"
"\n"
"  <div class=\"card\" data-tab=\"settings\" data-sub=\"general\">\n"
"    <h2>Lock</h2>\n"
"    <div id=\"lockOff\">\n"
"      <p class=\"note\">With a PIN, anyone can read the meter but only you can change the limit, calibration, WiFi or reset the gig.</p>\n"
"      <div class=\"pinrow\"><input id=\"pinNew\" type=\"password\" inputmode=\"numeric\" pattern=\"[0-9]*\" maxlength=\"8\" placeholder=\"4 to 8 numbers\" autocomplete=\"off\"><button class=\"btn primary\" id=\"lockBtn\" type=\"button\">Lock</button></div>\n"
"    </div>\n"
"    <div id=\"lockOn\" hidden>\n"
"      <p class=\"note\" id=\"lockState\">Locked.</p>\n"
"      <div class=\"pinrow\" id=\"unlockRow\"><input id=\"pinIn\" type=\"password\" inputmode=\"numeric\" pattern=\"[0-9]*\" maxlength=\"8\" placeholder=\"PIN\" autocomplete=\"off\"><button class=\"btn primary\" id=\"unlockBtn\" type=\"button\">Unlock</button></div>\n"
"      <div id=\"unlockedActs\" hidden>\n"
"        <div class=\"row\"><button class=\"btn wide\" id=\"relockBtn\" type=\"button\">Lock this phone again</button></div>\n"
"        <div class=\"row\"><button class=\"btn wide danger\" id=\"lockRemove\" type=\"button\">Remove the lock</button></div>\n"
"      </div>\n"
"    </div>\n"
"    <p class=\"note\">Forgot the PIN? Switch the meter off, hold its button, switch it on and keep holding for 5 seconds. The lock comes off and the WiFi goes back to PocketSPL / pocketspl.</p>\n"
"  </div>\n"
"\n"
"  <div class=\"card wiredonly\" data-tab=\"settings\" data-sub=\"general\" id=\"schCard\">\n"
"    <h2>On/off schedule</h2>\n"
"    <p class=\"note\" id=\"schState\"></p>\n"
"    <div data-lock>\n"
"    <label class=\"check\"><input type=\"checkbox\" id=\"schOn\"> Turn on and off by itself</label>\n"
"    <div id=\"schList\"></div>\n"
"    <div class=\"schbtns\"><button class=\"btn\" id=\"schAdd\" type=\"button\">Add a time</button><button class=\"btn\" id=\"schSave\" type=\"button\">Save schedule</button></div>\n"
"    </div>\n"
"    <p class=\"note\">An off time earlier than the on time finishes the next morning: Friday 6:30 pm to 2:00 am runs into Saturday. Outside these times the meter goes to standby: no logging, screen dark, WiFi still on. A press shows when it comes on next. Each session is saved as its own log.</p>\n"
"    <p class=\"note\" id=\"schClock\"></p>\n"
"  </div>\n"
"\n"
"  <div class=\"card about\" data-tab=\"settings\" data-sub=\"general\">\n"
"    <h2>About and help</h2>\n"
"    <div class=\"row\"><label>Model</label><output id=\"modelVal\">--</output></div>\n"
"    <div class=\"row\"><label>Serial number</label><output id=\"snVal\">--</output></div>\n"
"    <div class=\"row\"><label>Software</label><output id=\"fwVer2\">--</output></div>\n"
"    <p class=\"note\">PocketSPL is made by Overdrive Music Studio. Help, updates and USB repair: <a href=\"https://overdrive-studios.github.io/pocketspl/\" target=\"_blank\" rel=\"noopener\">overdrive-studios.github.io/pocketspl</a></p>\n"
"    <p class=\"note\"><a href=\"https://www.instagram.com/studio_overdrive/\" target=\"_blank\" rel=\"noopener\">Instagram</a>&emsp;<a href=\"https://www.facebook.com/profile.php?id=61558215059242\" target=\"_blank\" rel=\"noopener\">Facebook</a></p>\n"
"    <p class=\"note\">A sound level indicator for live sound. Not a certified Class 1 or Class 2 sound level meter.</p>\n"
"    <h3>Factory reset</h3>\n"
"    <button class=\"btn wide danger\" id=\"factoryBtn\" type=\"button\" data-lock>Factory reset</button>\n"
"    <div class=\"confirm\" id=\"factoryConfirm\" hidden>\n"
"      <b>Reset everything?</b>\n"
"      <p>Clears all settings, saved networks, the WiFi name and password, the lock and saved gigs. Keeps the factory calibration and serial number. The meter restarts.</p>\n"
"      <div class=\"confirmbtns\"><button class=\"btn\" id=\"factoryNo\" type=\"button\">Cancel</button><button class=\"btn danger solid\" id=\"factoryYes\" type=\"button\">Yes, reset</button></div>\n"
"    </div>\n"
"  </div>\n"
"</div>\n"
"<nav class=\"tabs\" id=\"tabs\"><button type=\"button\" data-t=\"meter\"><svg viewBox=\"0 0 24 24\" width=\"24\" height=\"24\" aria-hidden=\"true\"><path d=\"M4 17a8 8 0 1 1 16 0\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\"/><path d=\"M12 17l4-5\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\"/></svg><span>Meter</span></button><button type=\"button\" data-t=\"rta\"><svg viewBox=\"0 0 24 24\" width=\"24\" height=\"24\" aria-hidden=\"true\"><path d=\"M5 19V13M9 19V9M13 19V5M17 19V11M21 19V15M1 19V16\" stroke=\"currentColor\" stroke-width=\"2.2\" stroke-linecap=\"round\"/></svg><span>RTA</span></button><button type=\"button\" data-t=\"gig\"><svg viewBox=\"0 0 24 24\" width=\"24\" height=\"24\" aria-hidden=\"true\"><path d=\"M3 17l5-6 4 3 5-7 4 4\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\"/><path d=\"M3 21h18\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\"/></svg><span>Gig</span></button><button type=\"button\" data-t=\"settings\"><svg viewBox=\"0 0 24 24\" width=\"24\" height=\"24\" aria-hidden=\"true\"><circle cx=\"12\" cy=\"12\" r=\"3.2\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\"/><path d=\"M12 2.5v3M12 18.5v3M2.5 12h3M18.5 12h3M5.3 5.3l2.1 2.1M16.6 16.6l2.1 2.1M5.3 18.7l2.1-2.1M16.6 7.4l2.1-2.1\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\"/></svg><span>Settings</span></button></nav>\n"
"<div class=\"toast\" id=\"toast\"></div>\n"
"\n"
"<script>\n"
"const $ = id => document.getElementById(id);\n"
"const W = [\"dB(A)\",\"dB(C)\",\"dB(Z)\"], R = [\"Fast\",\"Slow\",\"Leq\",\"RTA\",\"Gig\"];\n"
"const BAR_MIN = 60, BAR_MAX = 110;\n"
"let st = null, ok = false, hist = [], busy = false, lastOk = 0;\n"
"\n"
"/* ---- looks (themes) ---- */\n"
"const LOOKS=[\"clean\",\"studio\",\"bold\"]; let look=-1, CVc={};\n"
"function cv(n){ return CVc[n] || (CVc[n]=getComputedStyle(document.body).getPropertyValue(n).trim() || \"#888\"); }\n"
"function cvFont(){ return (cv(\"--font-ui\")||\"sans-serif\")+\", sans-serif\"; }\n"
"function applyLook(i){\n"
"  i = (i>=0 && i<3) ? i : 1; if(i===look) return; look=i;\n"
"  LOOKS.forEach((n,k)=>document.body.classList.toggle(\"th-\"+n, k===i));\n"
"  CVc={}; try{ localStorage.setItem(\"pocketspl-look\", i); }catch(e){}\n"
"  document.querySelectorAll(\"#looks button\").forEach(b=>b.classList.toggle(\"on\", +b.dataset.v===i));\n"
"}\n"
"try{ applyLook(+(localStorage.getItem(\"pocketspl-look\") ?? 1)); }catch(e){ applyLook(1); }\n"
"/* Appearance (light / dark / auto), saved on this phone */\n"
"let mode=\"auto\"; try{ mode=localStorage.getItem(\"pocketspl-mode\")||\"auto\"; }catch(e){}\n"
"const darkQ = window.matchMedia ? matchMedia(\"(prefers-color-scheme: dark)\") : null;\n"
"function applyMode(){\n"
"  const dark = mode===\"dark\" || (mode===\"auto\" && darkQ && darkQ.matches);\n"
"  document.body.classList.toggle(\"dark\", dark); document.body.classList.toggle(\"light\", !dark);\n"
"  document.querySelectorAll(\"#segMode button\").forEach(b=>b.classList.toggle(\"on\", b.dataset.v===mode));\n"
"  CVc={}; if(st) render();\n"
"}\n"
"if(darkQ) (darkQ.addEventListener ? darkQ.addEventListener(\"change\",applyMode) : darkQ.addListener(applyMode));\n"
"document.querySelectorAll(\"#segMode button\").forEach(b=>b.onclick=()=>{ mode=b.dataset.v; try{ localStorage.setItem(\"pocketspl-mode\",mode); }catch(e){} applyMode(); toast(mode===\"auto\"?\"Appearance follows the phone\":mode===\"light\"?\"Light appearance\":\"Dark appearance\"); });\n"
"applyMode();\n"
"document.querySelectorAll(\"#looks button\").forEach(b=>b.onclick=()=>{ const v=+b.dataset.v; applyLook(v); lookPending=Date.now(); set(\"theme=\"+v, [\"Clean\",\"Studio\",\"Bold\"][v]+\" look\"); });\n"
"let lookPending=0;\n"
"const RING_LEN=570;\n"
"function drawRing(){\n"
"  const lit=$(\"ringLit\"), pk=$(\"ringPk\"); if(!lit) return;\n"
"  const off=st.warm||st.micfault, frac=off?0:Math.max(0,Math.min(1,(st.fast-BAR_MIN)/(BAR_MAX-BAR_MIN)));\n"
"  lit.setAttribute(\"stroke-dashoffset\", RING_LEN*(1-frac)); lit.style.stroke = off?\"transparent\":col(st.fast).replace(\"var(--ink)\",cv(\"--green\"));\n"
"  const pf=off?-1:Math.max(0,Math.min(1,(st.peak-BAR_MIN)/(BAR_MAX-BAR_MIN))), a=(135+270*pf)*Math.PI/180;\n"
"  if(pf>0){ pk.setAttribute(\"cx\",150+121*Math.cos(a)); pk.setAttribute(\"cy\",150+121*Math.sin(a)); } else { pk.setAttribute(\"cx\",-20); pk.setAttribute(\"cy\",-20); }\n"
"}\n"
"function levelState(){\n"
"  const off=st.warm||st.micfault||!ok;\n"
"  document.body.dataset.level = off?\"off\":st.fast>=st.red?\"r\":st.fast>=st.amber?\"a\":\"g\";\n"
"  let m = off?\"\":st.fast>=st.red?\"Very loud\":st.fast>=st.amber?\"Getting loud\":\"Comfortable\";\n"
"  if(!off && st.lim && st.lim.on && st.lim.val!==null){ const d=st.lim.db-st.lim.val; m += d>=0 ? \", \"+d.toFixed(1)+\" dB under your limit\" : \", \"+(-d).toFixed(1)+\" dB over your limit\"; }\n"
"  $(\"mood\").textContent=m;\n"
"}\n"
"\n"
"/* LED ladder: 40 segments from 60 to 110 dB, the same as the meter's own screen */\n"
"const SEGS=40; let lastResp=0;\n"
"(function buildLadder(){ const L=document.getElementById(\"ladder\"); for(let i=0;i<SEGS;i++) L.appendChild(document.createElement(\"i\"));\n"
"  const S=document.getElementById(\"scale\"); [60,70,80,90,100,110].forEach((v,k,a)=>{ const s=document.createElement(\"span\"); s.textContent=v; s.style.left=((v-60)/50*100)+\"%\"; if(k===a.length-1) s.className=\"end\"; S.appendChild(s); }); })();\n"
"function drawLadder(){\n"
"  const segs=document.getElementById(\"ladder\").children, lvl=st.warm||st.micfault?-1:st.fast, pk=st.warm||st.micfault?-1:st.peak;\n"
"  const pkI = pk>=BAR_MIN ? Math.min(SEGS-1, Math.floor((pk-BAR_MIN)/(BAR_MAX-BAR_MIN)*SEGS)) : -1;\n"
"  for(let i=0;i<SEGS;i++){ const lo=BAR_MIN+i*(BAR_MAX-BAR_MIN)/SEGS;\n"
"    const c = lvl>=lo ? (lo>=st.red?\"r\":lo>=st.amber?\"a\":\"g\") : (i===pkI?\"pk\":\"\");\n"
"    if(segs[i].className!==c) segs[i].className=c; }\n"
"}\n"
"function col(db){ const calm = look===1 ? \"var(--green)\" : \"var(--ink)\"; if(!st) return calm; return db>=st.red?\"var(--red)\":db>=st.amber?\"var(--amber)\":calm; }\n"
"function f1(v){ return (v===null||v===undefined)?\"--\":v.toFixed(1); }\n"
"function pct(db){ return Math.max(0,Math.min(100,(db-BAR_MIN)/(BAR_MAX-BAR_MIN)*100)); }\n"
"function hms(s){ s=Math.floor(s); const h=Math.floor(s/3600),m=Math.floor(s/60)%60,x=s%60;\n"
"  return h?`${h}h${String(m).padStart(2,\"0\")}m`:`${String(m).padStart(2,\"0\")}:${String(x).padStart(2,\"0\")}`; }\n"
"function toast(t){ const e=$(\"toast\"); e.textContent=t; e.classList.add(\"show\"); clearTimeout(e._t); e._t=setTimeout(()=>e.classList.remove(\"show\"),1800); }\n"
"\n"
"let held=false;\n"
"function render(){\n"
"  if(st){ renderUpd(); renderLock(); renderSchedule(); renderAlerts(); }\n"
"  $(\"dot\").classList.toggle(\"ok\", ok);\n"
"  $(\"conn\").textContent = ok ? (st&&st.standby ? \"Standby until \"+(st.next||\"--\") : \"Live\") : \"Not connected\";\n"
"  $(\"meter\").classList.toggle(\"off\", !ok);\n"
"  if(!st) return;\n"
"  renderGig();\n"
"  if(!held){\n"
"  $(\"wLbl\").textContent = W[st.w]; $(\"rLbl\").textContent = st.r>=3 ? \"Fast\" : R[st.r];\n"
"  const big = $(\"db\"); const isNum = !st.calst && !st.micfault && !st.warm && !st.over; big.classList.toggle(\"word\", !isNum && !st.warm);\n"
"  if(st.calst){ big.textContent = \"CAL\"; big.style.color=\"var(--ink)\"; $(\"unit\").textContent = st.calst==1?\"Fit the 94 dB calibrator…\":\"Measuring…\"; }\n"
"  else if(st.micfault){ big.textContent=\"MIC FAULT\"; big.style.color=\"var(--red)\"; $(\"unit\").textContent=\"No signal from the mic. Check its 5 wires.\"; }\n"
"  else if(st.warm){ big.textContent=\"--\"; big.style.color=\"var(--faint)\"; $(\"unit\").textContent=\"Starting…\"; }\n"
"  else if(st.over){ big.textContent=\"OVER\"; big.style.color=\"var(--red)\"; $(\"unit\").textContent=\"Above the mic's range (~120 dB)\"; }\n"
"  else { big.textContent=f1(st.db); big.style.color=col(st.db); $(\"unit\").textContent = \"dB SPL\"; }\n"
"  const clip=st.clip&&!st.over&&!st.calst&&!st.warm&&!st.micfault; $(\"unit\").style.color=clip?\"var(--red)\":\"\";\n"
"  if(clip) $(\"unit\").textContent=\"Mic clipping: peaks above ~120 dB read low\";\n"
"  drawLadder(); drawRing(); levelState();\n"
"  $(\"max\").textContent = f1(st.max); $(\"leq\").textContent = f1(st.leq); $(\"time\").textContent = st.leqs?hms(st.leqs):\"--\";\n"
"  }\n"
"  if(st.theme!==undefined && Date.now()-lookPending>3000) applyLook(st.theme);\n"
"  // battery: green with a bolt while charging (red until 20%), otherwise grey, amber under 30%, red when low\n"
"  $(\"batWrap\").style.display = st.wired ? \"none\" : \"\";   // wired model: no battery\n"
"  $(\"batt\").textContent = st.batt + \"%\";\n"
"  $(\"batBolt\").style.display = st.chg ? \"\" : \"none\";\n"
"  $(\"batFill\").setAttribute(\"width\", Math.max(1, 16*Math.min(100,Math.max(0,st.batt))/100));\n"
"  $(\"batWrap\").style.color = st.chg ? (st.batt<20?\"var(--red)\":\"var(--green)\") : st.batt<15?\"var(--red)\":st.batt<30?\"var(--amber)\":\"var(--mute)\";\n"
"  $(\"batWrap\").title = st.chg ? \"Charging\" : \"Battery\";\n"
"  document.querySelectorAll(\"#segW button\").forEach(b=>b.classList.toggle(\"on\", +b.dataset.v===st.w));\n"
"  if(st.r<3) lastResp=st.r;\n"
"  document.querySelectorAll(\"#segR button\").forEach(b=>b.classList.toggle(\"on\", +b.dataset.v===(st.r<3?st.r:lastResp)));\n"
"  document.querySelectorAll(\"#segScr button\").forEach(b=>b.classList.toggle(\"on\", st.r<3 ? b.dataset.v===\"lvl\" : +b.dataset.v===st.r));\n"
"  if(document.activeElement!==$(\"bright\")) $(\"bright\").value = st.bright;\n"
"  $(\"amber\").textContent = st.amber+\" dB\"; $(\"red\").textContent = st.red+\" dB\";\n"
"  $(\"cal\").textContent = (st.cal>=0?\"+\":\"\")+st.cal.toFixed(1);\n"
"  if(!held){ drawGraph(); drawRta(); }\n"
"  renderNet();\n"
"}\n"
"\n"
"const RTA_LBL=[\"20\",\"25\",\"31.5\",\"40\",\"50\",\"63\",\"80\",\"100\",\"125\",\"160\",\"200\",\"250\",\"315\",\"400\",\"500\",\"630\",\"800\",\"1k\",\"1.25k\",\"1.6k\",\"2k\",\"2.5k\",\"3.15k\",\"4k\",\"5k\",\"6.3k\",\"8k\",\"10k\",\"12.5k\",\"16k\",\"20k\"];\n"
"let rtaShown=null, rtaPeak=[], rtaPeakT=[], frozen=false;\n"
"function drawRta(){\n"
"  const c=$(\"rta\"), g=c.getContext(\"2d\"), w=c.width, h=c.height;\n"
"  $(\"rtaTitle\").textContent = \"RTA in \" + (st?W[st.w]:\"dB\");\n"
"  if(!frozen && st && st.rta && st.rta[0]!==null){\n"
"    rtaShown=st.rta.slice(); const now=Date.now();\n"
"    rtaShown.forEach((v,i)=>{ if(rtaPeak[i]===undefined||v>=rtaPeak[i]){rtaPeak[i]=v; rtaPeakT[i]=now;}\n"
"      else if(now-rtaPeakT[i]>2000){ rtaPeak[i]=Math.max(v, rtaPeak[i]-1.5); } });\n"
"  }\n"
"  g.clearRect(0,0,w,h);\n"
"  const L=70, B=h-54, T=14, lo=20, hi=110, n=31, bw=(w-L-6)/n;\n"
"  const y=v=>B-(Math.max(lo,Math.min(hi,v))-lo)/(hi-lo)*(B-T);\n"
"  g.font=\"500 32px \"+cvFont(); g.textAlign=\"right\"; g.fillStyle=cv(\"--mute\"); g.strokeStyle=cv(\"--rule\"); g.lineWidth=1;\n"
"  for(let v=20; v<=100; v+=20){ g.beginPath(); g.moveTo(L,y(v)); g.lineTo(w,y(v)); g.stroke(); g.fillText(v,L-10,y(v)+10); }\n"
"  g.textAlign=\"center\";\n"
"  [5,11,17,23,29].forEach(i=>g.fillText(RTA_LBL[i]+(i===5?\" Hz\":\"\"), L+bw*i+bw/2, h-14));\n"
"  if(!rtaShown) return;\n"
"  rtaShown.forEach((v,i)=>{ const x=L+bw*i+2, top=y(v);\n"
"    g.fillStyle = v>=st.red?cv(\"--red\"):v>=st.amber?cv(\"--amber\"):cv(\"--green\");\n"
"    g.fillRect(x, top, bw-4, B-top);\n"
"    if(rtaPeak[i]!==undefined){ g.fillStyle=cv(\"--ink\"); g.fillRect(x, y(rtaPeak[i])-2, bw-4, 3); } });\n"
"}\n"
"/* ---- gig mode ---- */\n"
"const WIN=[\"1 min\",\"5 min\",\"15 min\"];\n"
"function renderGig(){\n"
"  const L=st.lim, ban=$(\"limBanner\");\n"
"  if(L && L.on && L.st>0 && L.val!==null){\n"
"    ban.hidden=false; ban.className=\"limbanner \"+(L.st===2?\"red\":\"amber\");\n"
"    ban.textContent=(L.st===2?\"Over the limit: \":\"Near the limit: \")+(L.w===1?\"LCeq \":\"LAeq \")+WIN[L.win]+\" \"+L.val.toFixed(1)+\" dB (limit \"+L.db+\")\";\n"
"  } else ban.hidden=true;\n"
"  if(!held && st.roll){ for(let k=0;k<3;k++){ $(\"ra\"+k).textContent=f1(st.roll[k][0])+(st.roll[k][0]!==null&&!st.rollfull[k]?\"*\":\"\"); $(\"rc\"+k).textContent=f1(st.roll[k][1])+(st.roll[k][1]!==null&&!st.rollfull[k]?\"*\":\"\"); } }\n"
"  if(!held && st.dose!==undefined){ const d=$(\"dose\"); d.textContent=st.dose.toFixed(st.dose<10?1:0)+\"%\"; d.style.color=st.dose>=100?\"var(--red)\":st.dose>=50?\"var(--amber)\":\"var(--text)\"; }\n"
"  if(st.log){ const sec=st.log.secs, h=Math.floor(sec/3600), m=Math.floor(sec/60)%60, s2=sec%60;\n"
"    $(\"logInfo\").textContent = \"Logging for \"+(h? h+\" h \"+m+\" min\" : m? m+\" min \"+s2+\" s\" : s2+\" s\")+(st.log.marks?\", \"+st.log.marks+\" marker\"+(st.log.marks>1?\"s\":\"\"):\"\"); }\n"
"  if(L){ if(document.activeElement!==$(\"limOn\")) $(\"limOn\").checked=L.on; $(\"limDb\").textContent=L.db+\" dB\";\n"
"    document.querySelectorAll(\"#segLimWin button\").forEach(b=>b.classList.toggle(\"on\",+b.dataset.v===L.win));\n"
"    document.querySelectorAll(\"#segLimW button\").forEach(b=>b.classList.toggle(\"on\",(+b.dataset.v===1)===(L.w===1)));\n"
"    const who=L.on&&L.db===100&&L.win===2&&L.w!==1; $(\"whoStd\").textContent=who?\"✓ Venue standard, 100 dB(A) over 15 min\":\"Use the venue standard (100 dB(A), 15 min)\"; $(\"whoStd\").classList.toggle(\"on\",who); }\n"
"  if(st.dim!==undefined) document.querySelectorAll(\"#segDim button\").forEach(b=>b.classList.toggle(\"on\",+b.dataset.v===st.dim));\n"
"}\n"
"let gigLog=null;\n"
"async function loadLog(){\n"
"  try{ const r=await fetch(\"/api/log.json\",{cache:\"no-store\"}); gigLog=await r.json(); drawGigChart(); }catch(e){}\n"
"}\n"
"function drawGigChart(){\n"
"  const c=$(\"gigChart\"), g=c.getContext(\"2d\"), w=c.width, h=c.height; g.clearRect(0,0,w,h);\n"
"  const X0=62, lo=50, hi=120, y=v=>h-30-(Math.max(lo,Math.min(hi,v))-lo)/(hi-lo)*(h-44);\n"
"  g.font=\"500 30px \"+cvFont(); g.fillStyle=cv(\"--mute\"); g.strokeStyle=cv(\"--rule\"); g.lineWidth=1;\n"
"  for(const v of [60,80,100]){ g.beginPath(); g.moveTo(X0,y(v)); g.lineTo(w,y(v)); g.stroke(); g.fillText(v,4,y(v)+10); }\n"
"  const pts=gigLog?gigLog.laeq.slice():[]; const live=gigLog&&gigLog.cur!==null&&gigLog.cur!==undefined;\n"
"  if(live) pts.push(gigLog.cur);                       // the minute that's still building\n"
"  if(!pts.length){ g.fillText(\"Logging has started. The chart appears in a second or two.\",50,h/2); return; }\n"
"  const n=pts.length, span=Math.max(n-1,10), x=i=>X0+i/span*(w-X0-10);   // at least a 10-minute-wide view so early points start on the left\n"
"  if(st&&st.lim&&st.lim.on){ g.setLineDash([8,8]); g.strokeStyle=cv(\"--red\"); g.beginPath(); g.moveTo(X0,y(st.lim.db)); g.lineTo(w,y(st.lim.db)); g.stroke(); g.setLineDash([]); }\n"
"  g.lineWidth=4; g.lineJoin=\"round\";\n"
"  for(let i=1;i<n;i++){ const a=pts[i-1]/10, b=pts[i]/10;\n"
"    g.strokeStyle=st&&st.lim&&st.lim.on?(b>=st.lim.db?cv(\"--red\"):b>=st.lim.db-3?cv(\"--amber\"):cv(\"--green\")):(b>=st.red?cv(\"--red\"):b>=st.amber?cv(\"--amber\"):cv(\"--green\"));\n"
"    g.beginPath(); g.moveTo(x(i-1),y(a)); g.lineTo(x(i),y(b)); g.stroke(); }\n"
"  if(live){ const v=pts[n-1]/10; g.fillStyle=cv(\"--ink\"); g.beginPath(); g.arc(x(n-1),y(v),7,0,7); g.fill(); }\n"
"  g.fillStyle=cv(\"--ink\"); g.font=\"500 28px \"+cvFont();\n"
"  (gigLog.marks||[]).forEach(([sec,t],j)=>{ const i=Math.floor(sec/60)-gigLog.first; if(i<0||i>=n) return; const xx=x(Math.min(i,n-1));\n"
"    g.fillStyle=cv(\"--faint\"); g.fillRect(xx-1,10,2,h-40); g.fillStyle=cv(\"--ink\"); g.fillText(t,Math.min(xx+6,w-g.measureText(t).width-2),32+(j%2)*28); });\n"
"  const hrs=n/60; g.fillStyle=cv(\"--mute\"); g.font=\"500 28px \"+cvFont(); g.textAlign=\"right\"; g.fillText(hrs>=1?hrs.toFixed(1)+\" h\":n+\" min\",w-4,h-2); g.textAlign=\"left\";\n"
"}\n"
"let timeSent=false;\n"
"function tzPosix(){                         // this phone's time zone as a rule the meter understands (daylight saving included)\n"
"  const z=(Intl.DateTimeFormat().resolvedOptions().timeZone||\"\");\n"
"  const E=\"AEST-10AEDT,M10.1.0,M4.1.0/3\", C=\"ACST-9:30ACDT,M10.1.0,M4.1.0/3\";\n"
"  const AU={\"Australia/Brisbane\":\"AEST-10\",\"Australia/Lindeman\":\"AEST-10\",\"Australia/Sydney\":E,\"Australia/Melbourne\":E,\"Australia/Canberra\":E,\n"
"    \"Australia/ACT\":E,\"Australia/NSW\":E,\"Australia/Victoria\":E,\"Australia/Hobart\":E,\"Australia/Tasmania\":E,\"Australia/Currie\":E,\n"
"    \"Australia/Adelaide\":C,\"Australia/South\":C,\"Australia/Broken_Hill\":C,\"Australia/Darwin\":\"ACST-9:30\",\"Australia/North\":\"ACST-9:30\",\n"
"    \"Australia/Perth\":\"AWST-8\",\"Australia/West\":\"AWST-8\",\"Australia/Eucla\":\"<+0845>-8:45\",\"Australia/Lord_Howe\":\"<+1030>-10:30<+11>-11,M10.1.0,M4.1.0\",\n"
"    \"Pacific/Auckland\":\"NZST-12NZDT,M9.5.0,M4.1.0/3\"};\n"
"  if(AU[z]) return AU[z];\n"
"  const o=-new Date().getTimezoneOffset(), a=Math.abs(o), hh=Math.floor(a/60), mm=a%60, p2=n=>String(n).padStart(2,\"0\");\n"
"  return \"<\"+(o>=0?\"+\":\"-\")+p2(hh)+p2(mm)+\">\"+(o>=0?\"-\":\"+\")+hh+(mm?\":\"+p2(mm):\"\");\n"
"}\n"
"async function sendTime(){ if(timeSent||!ok) return; timeSent=true; try{ await fetch(\"/api/time?t=\"+Date.now()+\"&tz=\"+(-new Date().getTimezoneOffset())+\"&tzp=\"+encodeURIComponent(tzPosix()),{cache:\"no-store\"}); }catch(e){ timeSent=false; } }\n"
"setInterval(()=>{ sendTime(); if(!document.querySelector('.card[data-tab=\"gig\"]').hidden) loadLog(); },5000); setTimeout(()=>{ sendTime(); loadLog(); },1500);\n"
"$(\"holdBtn\").onclick=()=>{ held=!held; $(\"holdBtn\").classList.toggle(\"on\",held); $(\"holdBtn\").textContent=held?\"Held\":\"Hold\"; render(); };\n"
"$(\"markBtn\").onclick=()=>{ const f=$(\"markForm\"); f.hidden=!f.hidden; if(!f.hidden) $(\"markText\").focus(); };\n"
"$(\"markSave\").onclick=async()=>{ const t=$(\"markText\").value.trim(); await set(\"mark=\"+encodeURIComponent(t||\"Marker\"),\"Marker added\"+(t?\": \"+t:\"\")); $(\"markText\").value=\"\"; $(\"markForm\").hidden=true; loadLog(); };\n"
"$(\"resetGig\").onclick=()=>{ $(\"resetConfirm\").hidden=false; $(\"resetGig\").hidden=true; $(\"resetConfirm\").scrollIntoView({block:\"nearest\",behavior:\"smooth\"}); };\n"
"$(\"resetNo\").onclick=()=>{ $(\"resetConfirm\").hidden=true; $(\"resetGig\").hidden=false; };\n"
"$(\"resetYes\").onclick=async()=>{ $(\"resetConfirm\").hidden=true; $(\"resetGig\").hidden=false;\n"
"  hist=[]; gigLog=null; await set(\"newlog=1&reset=1\",\"Gig reset. Logging again now\"); timeSent=false; drawGigChart(); setTimeout(()=>{ sendTime(); loadLog(); logsKey=\"\"; loadSavedLogs(); },1200); };\n"
"$(\"dlCsv\").onclick=async(e)=>{ const P=window.parent!==window&&window.parent.PocketSim; if(!P) return; e.preventDefault();\n"
"  const txt=P.csv(); const v=document.createElement(\"div\"); v.className=\"csvview\"; const pre=document.createElement(\"pre\"); pre.textContent=txt+\"\\n(Simulator: on the real meter this downloads as a spreadsheet file. Tap to close.)\"; v.append(pre); v.onclick=()=>v.remove(); document.body.append(v); };\n"
"$(\"limOn\").onchange=e=>set(\"limOn=\"+(e.target.checked?1:0), e.target.checked?\"Limit alarm on\":\"Limit alarm off\");\n"
"document.querySelectorAll(\"[data-l]\").forEach(b=>b.onclick=()=>{ if(!st) return; const v=st.lim.db+(+b.dataset.l); set(\"limDb=\"+v,\"Limit \"+v+\" dB\"); });\n"
"document.querySelectorAll(\"#segLimWin button\").forEach(b=>b.onclick=()=>set(\"limWin=\"+b.dataset.v,\"Limit over \"+WIN[+b.dataset.v]));\n"
"$(\"whoStd\").onclick=()=>set(\"limOn=1&limDb=100&limWin=2&limW=0\",\"Limit: 100 dB(A) over 15 min\");\n"
"document.querySelectorAll(\"#segLimW button\").forEach(b=>b.onclick=()=>set(\"limW=\"+b.dataset.v,b.dataset.v===\"1\"?\"Limit in dB C\":\"Limit in dB A\"));\n"
"document.querySelectorAll(\"#segDim button\").forEach(b=>b.onclick=()=>set(\"dim=\"+b.dataset.v,b.dataset.v===\"0\"?\"Screen won't dim\":\"Dims after \"+b.textContent));\n"
"\n"
"$(\"freeze\").onclick=()=>{ frozen=!frozen; $(\"freeze\").textContent=frozen?\"Frozen\":\"Freeze\"; $(\"freeze\").classList.toggle(\"on\",frozen); render(); };\n"
"\n"
"function drawGraph(){\n"
"  const c=$(\"graph\"), g=c.getContext(\"2d\"), w=c.width, h=c.height, now=Date.now();\n"
"  g.clearRect(0,0,w,h);\n"
"  const lo=40, hi=120, y=v=>h-(Math.max(lo,Math.min(hi,v))-lo)/(hi-lo)*h;\n"
"  g.font=\"500 30px \"+cvFont(); g.fillStyle=cv(\"--mute\"); g.strokeStyle=cv(\"--rule\"); g.lineWidth=1;\n"
"  for(const v of [60,80,100]){ g.beginPath(); g.moveTo(0,y(v)); g.lineTo(w,y(v)); g.stroke(); g.fillText(v,6,y(v)-6); }\n"
"  if(st){ for(const [v,cl] of [[st.amber,cv(\"--amber\")],[st.red,cv(\"--red\")]]){ g.setLineDash([8,8]); g.strokeStyle=cl; g.globalAlpha=.6;\n"
"    g.beginPath(); g.moveTo(0,y(v)); g.lineTo(w,y(v)); g.stroke(); g.setLineDash([]); g.globalAlpha=1; } }\n"
"  const pts=hist.filter(p=>now-p.t<=60000);\n"
"  if(pts.length<2) return;\n"
"  g.lineWidth=4; g.lineJoin=\"round\";\n"
"  for(let i=1;i<pts.length;i++){\n"
"    const a=pts[i-1], b=pts[i]; if(b.t-a.t>2000) continue;\n"
"    g.strokeStyle = b.v>=st.red?cv(\"--red\"):b.v>=st.amber?cv(\"--amber\"):cv(\"--green\");\n"
"    g.beginPath(); g.moveTo(w-(now-a.t)/60000*w, y(a.v)); g.lineTo(w-(now-b.t)/60000*w, y(b.v)); g.stroke();\n"
"  }\n"
"}\n"
"\n"
"async function call(path){\n"
"  try{\n"
"    const ctl=new AbortController(), t=setTimeout(()=>ctl.abort(),4000);\n"
"    const r=await fetch(path,{cache:\"no-store\",signal:ctl.signal}); clearTimeout(t);\n"
"    if(!r.ok) throw 0;\n"
"    st=await r.json(); ok=true; lastOk=Date.now();\n"
"    if(!st.warm && !st.calst && !st.over) { hist.push({t:Date.now(), v:st.fast}); }\n"
"    const cut=Date.now()-61000; while(hist.length && hist[0].t<cut) hist.shift();\n"
"    return true;\n"
"  }catch(e){ if(Date.now()-lastOk>6000) ok=false; return false; }\n"
"}\n"
"\n"
"async function poll(){\n"
"  if(!busy && !upd.sending){ busy=true; await call(\"/api/state\"); busy=false; render(); }   // pause while the update is being sent\n"
"  setTimeout(poll, 200);\n"
"}\n"
"\n"
"function getPin(){ try{ return sessionStorage.getItem(\"pocketspl-pin\")||\"\"; }catch(e){ return window.__pin||\"\"; } }\n"
"function savePin(v){ try{ if(v) sessionStorage.setItem(\"pocketspl-pin\",v); else sessionStorage.removeItem(\"pocketspl-pin\"); }catch(e){} window.__pin=v; }\n"
"function pinQ(){ const v=getPin(); return v?\"&pin=\"+encodeURIComponent(v):\"\"; }\n"
"async function set(q, msg){ const good=await call(\"/api/set?\"+q+pinQ()); render();\n"
"  if(good && st.denied){ toast(\"Locked: enter the PIN in Settings → General → Lock\"); return false; }\n"
"  toast(good?(msg||\"Saved\"):\"Not connected\"); return good; }\n"
"\n"
"document.querySelectorAll(\"#segW button\").forEach(b=>b.onclick=()=>set(\"w=\"+b.dataset.v, W[b.dataset.v]));\n"
"document.querySelectorAll(\"#segR button\").forEach(b=>b.onclick=()=>set(\"r=\"+b.dataset.v, R[b.dataset.v]+(b.dataset.v===\"2\"?\" (Leq restarted)\":\"\")));\n"
"document.querySelectorAll(\"#segScr button\").forEach(b=>b.onclick=()=>{ const v=b.dataset.v===\"lvl\"?lastResp:+b.dataset.v; set(\"r=\"+v, v===3?\"Meter shows the RTA\":v===4?\"Meter shows the Gig screen\":\"Meter shows the level\"); });\n"
"$(\"reset\").onclick=()=>{ hist=[]; set(\"reset=1\",\"MAX and Leq reset\"); };\n"
"$(\"bright\").onchange=e=>set(\"bright=\"+e.target.value,\"Brightness \"+e.target.value);\n"
"document.querySelectorAll(\"[data-k]\").forEach(b=>b.onclick=()=>{ if(!st) return; const k=b.dataset.k; set(k+\"=\"+(st[k]+(+b.dataset.d)), (k===\"amber\"?\"Amber\":\"Red\")+\" from \"+(st[k]+(+b.dataset.d))+\" dB\"); });\n"
"document.querySelectorAll(\"[data-c]\").forEach(b=>b.onclick=()=>{ if(!st) return; const v=Math.round((st.cal+(+b.dataset.c))*10)/10; set(\"cal=\"+v,\"Offset \"+(v>=0?\"+\":\"\")+v.toFixed(1)+\" dB\"); });\n"
"$(\"cal94\").onclick=()=>{ if(confirm(\"Fit the 94 dB / 1 kHz calibrator over the mic grille, switch it on, then press OK. Takes 7 seconds.\")) set(\"cal94=1\",\"Calibrating…\"); };\n"
"/* ---- network ---- */\n"
"let savedKey=\"\";\n"
"function bars(r){ return r>-55?\"▂▄▆█\":r>-67?\"▂▄▆\":r>-78?\"▂▄\":\"▂\"; }\n"
"function renderNet(){\n"
"  if(!st||!st.net) return; const n=st.net;\n"
"  const sta=n.mode===\"sta\", joining=n.mode===\"joining\";\n"
"  $(\"netDot\").classList.toggle(\"ok\", sta||n.mode===\"ap\");\n"
"  $(\"netName\").textContent = sta ? n.ssid : joining ? \"Joining \"+n.ssid+\"…\" : n.mode===\"ap\" ? \"Meter's own WiFi (\"+n.apname+\")\" : \"WiFi off\";\n"
"  $(\"netAddr\").textContent = n.ip ? \"http://\"+n.ip+(sta?\" or \"+n.mdns+\".local\":\"\") : \"\";\n"
"  $(\"netMsg\").textContent = n.msg||\"\";\n"
"  const showDone = sta && n.ap && n.phone;\n"
"  $(\"netDone\").hidden=!showDone;\n"
"  if(showDone){ $(\"doneName\").textContent=n.ssid; $(\"doneName2\").textContent=n.ssid; $(\"doneIp\").textContent=\"http://\"+n.ip; $(\"doneMdns\").textContent=n.mdns+\".local\"; $(\"doneAp\").textContent=n.apname; }\n"
"  $(\"apNow\").textContent=n.apname; $(\"apDefNote\").textContent=n.apdef?\" (password: pocketspl)\":\"\";\n"
"  if(document.activeElement!==$(\"apNameIn\") && !$(\"apNameIn\").dataset.edited) $(\"apNameIn\").value=n.apname;\n"
"  if(document.activeElement!==$(\"joinSaved\")) $(\"joinSaved\").checked=n.join;\n"
"  const key=JSON.stringify(n.saved);\n"
"  if(key!==savedKey){ savedKey=key; const L=$(\"savedList\"); L.innerHTML=\"\";\n"
"    if(!n.saved.length){ L.innerHTML='<p class=\"note\">None yet. Add your home or studio WiFi below.</p>'; }\n"
"    n.saved.forEach((net,i)=>{\n"
"      const wrap=document.createElement(\"div\"); wrap.className=\"savedwrap\";\n"
"      const d=document.createElement(\"div\"); d.className=\"item\";\n"
"      const meta=document.createElement(\"div\"); meta.className=\"meta\";\n"
"      const sp=document.createElement(\"span\"); sp.textContent=net.s;\n"
"      const sm=document.createElement(\"small\"); sm.textContent=net.ip?\"Fixed address \"+net.ip:\"Automatic address (DHCP)\";\n"
"      meta.append(sp,sm);\n"
"      const acts=document.createElement(\"div\"); acts.className=\"acts\";\n"
"      const ba=document.createElement(\"button\"); ba.className=\"btn small\"; ba.textContent=\"Address\";\n"
"      const bf=document.createElement(\"button\"); bf.className=\"btn small\"; bf.textContent=\"Forget\";\n"
"      bf.onclick=()=>netCall(\"forget=\"+i,\"Forgot \"+net.s);\n"
"      ba.onclick=()=>{ if(wrap.querySelector(\".editor\")){ wrap.querySelector(\".editor\").remove(); return; }\n"
"        const ed=document.createElement(\"div\"); ed.className=\"editor\";\n"
"        const box=document.createElement(\"div\"); box.className=\"addr\"; ed.append(box);\n"
"        const get=addrForm(box,\"e\"+i,net);\n"
"        const sv=document.createElement(\"button\"); sv.className=\"btn wide primary\"; sv.textContent=\"Save address\";\n"
"        sv.onclick=async()=>{ const a=get(); if(a===null) return;\n"
"          await call(\"/api/net?setip=\"+i+\"&ip=\"+encodeURIComponent(a.ip)+\"&mask=\"+encodeURIComponent(a.mask)+\"&gw=\"+encodeURIComponent(a.gw)+pinQ());\n"
"          render(); toast(st&&st.net&&st.net.msg?st.net.msg:\"Saved\"); };\n"
"        const nt=document.createElement(\"p\"); nt.className=\"note\";\n"
"        nt.textContent=\"If the meter is on this network now, it rejoins with the new address. Your phone may need the new address too.\";\n"
"        ed.append(sv,nt); wrap.append(ed); };\n"
"      acts.append(ba,bf); d.append(meta,acts); wrap.append(d); L.append(wrap); }); }\n"
"}\n"
"\n"
"/* fixed / automatic address fields; returns a getter that gives {ip,mask,gw} or null if something's wrong */\n"
"function validIp(v){ const p=v.split(\".\"); return p.length===4 && p.every(x=>/^\\d{1,3}$/.test(x) && +x<=255); }\n"
"function addrForm(box,id,cur){\n"
"  cur=cur||{};\n"
"  box.innerHTML='<div class=\"seg\"><button type=\"button\" data-m=\"auto\">Automatic</button><button type=\"button\" data-m=\"fixed\">Fixed</button></div>'+\n"
"    '<div class=\"fixed\">'+\n"
"    '<label for=\"'+id+'ip\">Meter address</label><input id=\"'+id+'ip\" inputmode=\"decimal\" autocomplete=\"off\" placeholder=\"192.168.1.50\">'+\n"
"    '<label for=\"'+id+'mask\">Subnet mask</label><input id=\"'+id+'mask\" inputmode=\"decimal\" autocomplete=\"off\" placeholder=\"255.255.255.0\">'+\n"
"    '<label for=\"'+id+'gw\">Router (leave blank if there isn\\'t one)</label><input id=\"'+id+'gw\" inputmode=\"decimal\" autocomplete=\"off\" placeholder=\"192.168.1.1\">'+\n"
"    '<small>Pick an address no other device on that network is using. Your phone needs one in the same range too.</small></div>'+\n"
"    '<small class=\"autonote\">Automatic works on most WiFi. Use Fixed when nothing on the network hands out addresses (no DHCP server).</small>';\n"
"  const ip=box.querySelector(\"#\"+id+\"ip\"), mask=box.querySelector(\"#\"+id+\"mask\"), gw=box.querySelector(\"#\"+id+\"gw\"), fx=box.querySelector(\".fixed\");\n"
"  ip.value=cur.ip||\"\"; mask.value=cur.mask||\"255.255.255.0\"; gw.value=cur.gw||\"\";\n"
"  let mode=cur.ip?\"fixed\":\"auto\";\n"
"  const btns=box.querySelectorAll(\".seg button\");\n"
"  function show(){ btns.forEach(b=>b.classList.toggle(\"on\",b.dataset.m===mode)); fx.hidden=mode!==\"fixed\"; box.querySelector(\".autonote\").hidden=mode===\"fixed\"; }\n"
"  btns.forEach(b=>b.onclick=()=>{ mode=b.dataset.m; show(); if(mode===\"fixed\") ip.focus(); });\n"
"  show();\n"
"  return ()=>{\n"
"    if(mode===\"auto\") return {ip:\"\",mask:\"\",gw:\"\"};\n"
"    const a={ip:ip.value.trim(),mask:mask.value.trim()||\"255.255.255.0\",gw:gw.value.trim()};\n"
"    if(!validIp(a.ip)){ toast(\"Type the meter address like 192.168.1.50\"); ip.focus(); return null; }\n"
"    if(!validIp(a.mask)){ toast(\"Type the subnet mask like 255.255.255.0\"); mask.focus(); return null; }\n"
"    if(a.gw && !validIp(a.gw)){ toast(\"Type the router like 192.168.1.1, or leave it blank\"); gw.focus(); return null; }\n"
"    if(a.ip.startsWith(\"192.168.4.\")){ toast(\"Use an address outside 192.168.4.x\"); ip.focus(); return null; }\n"
"    return a;\n"
"  };\n"
"}\n"
"const joinAddr=addrForm($(\"jAddr\"),\"j\",{});\n"
"async function netCall(q,msg){ await call(\"/api/net?\"+q+pinQ()); render(); toast(st&&st.denied?\"Locked: enter the PIN in Settings → General → Lock\":msg); }\n"
"function pick(name){ $(\"joinForm\").hidden=false; $(\"ssid\").value=name; $(\"pass\").value=\"\"; $(\"pass\").focus(); }\n"
"$(\"scanBtn\").onclick=async()=>{ const b=$(\"scanBtn\"); b.textContent=\"Looking…\"; b.disabled=true;\n"
"  try{ const r=await fetch(\"/api/scan\",{cache:\"no-store\"}); const j=await r.json(); const L=$(\"scanList\"); L.innerHTML=\"\";\n"
"    if(!j.nets.length) L.innerHTML='<p class=\"note\">Nothing found. Move closer to the router and try again.</p>';\n"
"    j.nets.forEach(x=>{ const it=document.createElement(\"button\"); it.className=\"item\"; const sp=document.createElement(\"span\"); sp.textContent=x.s;\n"
"      const sm=document.createElement(\"small\"); sm.textContent=(x.l?\"🔒 \":\"\")+bars(x.r); it.append(sp,sm); it.onclick=()=>pick(x.s); L.append(it); });\n"
"  }catch(e){ toast(\"Couldn't search. Try again.\"); }\n"
"  b.textContent=\"Find networks\"; b.disabled=false; };\n"
"$(\"manual\").onclick=e=>{ e.preventDefault(); pick(\"\"); $(\"ssid\").focus(); };\n"
"$(\"showPw\").onclick=()=>{ const p=$(\"pass\"); p.type=p.type===\"password\"?\"text\":\"password\"; $(\"showPw\").textContent=p.type===\"password\"?\"Show\":\"Hide\"; };\n"
"$(\"joinBtn\").onclick=async()=>{ const s=$(\"ssid\").value.trim(); if(!s){ toast(\"Type the network name\"); return; }\n"
"  const a=joinAddr(); if(a===null) return;\n"
"  await call(\"/api/join?ssid=\"+encodeURIComponent(s)+\"&pass=\"+encodeURIComponent($(\"pass\").value)+\"&ip=\"+encodeURIComponent(a.ip)+\"&mask=\"+encodeURIComponent(a.mask)+\"&gw=\"+encodeURIComponent(a.gw)+pinQ());\n"
"  render(); toast(\"Joining \"+s+(a.ip?\" at \"+a.ip:\"\")+\"…\");\n"
"  $(\"joinForm\").hidden=true; $(\"scanList\").innerHTML=\"\"; };\n"
"$(\"joinSaved\").onchange=e=>netCall(\"join=\"+(e.target.checked?1:0), e.target.checked?\"Will join saved networks\":\"Will use its own WiFi\");\n"
"$(\"netFinish\").onclick=()=>netCall(\"finish=1\",\"Meter's own WiFi switching off\");\n"
"\n"
"$(\"wifiOff\").onclick=()=>{ if(confirm(\"Turn the meter's WiFi off? This page will disconnect. Hold the meter's button 3 s to turn it back on.\")) set(\"wifi=0\",\"WiFi off\"); };\n"
"/* ---- meter's own WiFi ---- */\n"
"$(\"apNameIn\").oninput=()=>{ $(\"apNameIn\").dataset.edited=\"1\"; };\n"
"$(\"apShow\").onclick=()=>{ const p=$(\"apPassIn\"); p.type=p.type===\"password\"?\"text\":\"password\"; $(\"apShow\").textContent=p.type===\"password\"?\"Show\":\"Hide\"; };\n"
"$(\"apSave\").onclick=async()=>{\n"
"  const name=$(\"apNameIn\").value.trim(), pw=$(\"apPassIn\").value;\n"
"  if(name.length<1||name.length>32){ toast(\"Name: 1 to 32 characters\"); return; }\n"
"  if(pw && (pw.length<8||pw.length>63)){ toast(\"Password: 8 to 63 characters\"); return; }\n"
"  if(/[^\\x20-\\x7e]/.test(name+pw)){ toast(\"Use plain letters, numbers and symbols\"); return; }\n"
"  const onAp = st && st.net && st.net.mode!==\"sta\";\n"
"  if(onAp && !confirm(\"Your phone will drop off the meter. Then join \\\"\"+name+\"\\\"\"+(pw?\" with the new password\":\"\")+\" and open http://192.168.4.1 again. Save?\")) return;\n"
"  await set(\"apname=\"+encodeURIComponent(name)+(pw?\"&appass=\"+encodeURIComponent(pw):\"\"), \"Saved: \"+name);\n"
"  $(\"apPassIn\").value=\"\"; delete $(\"apNameIn\").dataset.edited;\n"
"};\n"
"$(\"apDefault\").onclick=async()=>{\n"
"  if(!confirm(\"Put the meter's own WiFi back to PocketSPL / pocketspl?\")) return;\n"
"  await set(\"apdefault=1\",\"Back to PocketSPL / pocketspl\"); delete $(\"apNameIn\").dataset.edited;\n"
"};\n"
"\n"
"/* ---- software updates ---- */\n"
"const UPDATE_URL=\"https://overdrive-studios.github.io/pocketspl/latest.json\";   // where new versions are published\n"
"let upd={state:\"idle\", info:null, checkedAt:0, busy:false, note:\"\", cached:null, fetching:false};\n"
"/* the downloaded update is kept on the phone, so Install works later without internet */\n"
"const fwCache={ mem:{},\n"
"  db(){ return new Promise((res,rej)=>{ try{ const r=indexedDB.open(\"pocketspl\",1); r.onupgradeneeded=()=>r.result.createObjectStore(\"fw\"); r.onsuccess=()=>res(r.result); r.onerror=()=>rej(); }catch(e){ rej(); } }); },\n"
"  async get(v){ try{ const d=await this.db(); const b=await new Promise(res=>{ const q=d.transaction(\"fw\").objectStore(\"fw\").get(v); q.onsuccess=()=>res(q.result||null); q.onerror=()=>res(null); }); return b||this.mem[v]||null; }catch(e){ return this.mem[v]||null; } },\n"
"  async put(v,b){ this.mem={[v]:b}; try{ const d=await this.db(); const tx=d.transaction(\"fw\",\"readwrite\"); const s=tx.objectStore(\"fw\"); s.clear(); s.put(b,v); }catch(e){} },\n"
"  async clear(){ this.mem={}; try{ const d=await this.db(); d.transaction(\"fw\",\"readwrite\").objectStore(\"fw\").clear(); }catch(e){} } };\n"
"function verCmp(a,b){ const pa=String(a).split(\".\").map(Number), pb=String(b).split(\".\").map(Number); for(let i=0;i<3;i++){ const d=(pa[i]||0)-(pb[i]||0); if(d) return d; } return 0; }\n"
"function newestKnown(){   // the newest version we know of: from the internet check, or remembered by the meter\n"
"  if(!st||!st.fw) return null;\n"
"  let v=null; if(upd.info && verCmp(upd.info.version, st.fw)>0) v=upd.info.version;\n"
"  if(st.latest && verCmp(st.latest, st.fw)>0 && (!v || verCmp(st.latest, v)>0)) v=st.latest;\n"
"  return v;\n"
"}\n"
"function renderUpd(){\n"
"  if(!st) return;\n"
"  $(\"fwVer\").textContent = st.fw ? \"v\"+st.fw : \"--\";\n"
"  $(\"updNoOta\").hidden = st.ota!==false;\n"
"  const v=newestKnown(), ready = !!v && upd.cached===v, online = !!(v && upd.info && upd.info.version===v);\n"
"  $(\"updAvail\").hidden = !v || upd.busy;\n"
"  $(\"updInstall\").hidden = !v || upd.busy || st.ota===false || !(ready || online);\n"
"  $(\"updInstall\").textContent = ready ? \"Install update\" : \"Download and install\";\n"
"  if(v){ $(\"updVer\").textContent=\"Version \"+v+\" is available\";\n"
"    $(\"updNotes\").textContent = (online && upd.info.notes ? upd.info.notes+\"\\n\" : \"\") +\n"
"      (ready ? \"Downloaded to this phone and ready. No internet needed to install.\"\n"
"       : upd.fetching ? \"Downloading to this phone…\"\n"
"       : online ? \"\" : \"Your phone needs internet once to download it: mobile data, or join the meter to a WiFi with internet (Settings → WiFi). After that you can install it any time.\"); }\n"
"  $(\"updCheck\").hidden = upd.busy; $(\"updFileBtn\").hidden = upd.busy;\n"
"  if(!upd.busy){\n"
"    $(\"updMsg\").textContent = upd.state===\"checking\" ? \"Checking for updates…\" : upd.note ? upd.note :\n"
"      upd.state===\"offline\" && !v ? \"Couldn't check online: the phone needs internet. Try with mobile data on, or join the meter to a WiFi with internet (Settings → WiFi).\" :\n"
"      v ? \"\" : upd.state===\"ok\" ? \"Up to date.\" : \"\";\n"
"    $(\"updMsg\").hidden = !!v && !upd.note;\n"
"  }\n"
"  document.querySelectorAll('#tabs button[data-t=\"settings\"] .badge').forEach(b=>b.remove());\n"
"  if(v && !upd.busy){ const b=document.createElement(\"i\"); b.className=\"badge\"; document.querySelector('#tabs button[data-t=\"settings\"]').appendChild(b); }\n"
"  $(\"subSoftware\").classList.toggle(\"hasupd\", !!v && !upd.busy);\n"
"}\n"
"const ck=v=>v+(st&&st.board==='4MB'?'-4mb':'');   // the phone keeps each board size's file separately\n"
"async function refreshCached(){ const v=newestKnown(); upd.cached = v && await fwCache.get(ck(v)) ? v : null; renderUpd(); }\n"
"async function fetchFirmware(i, onProg){   // download from the update site, checking the size\n"
"  const url=new URL(i.url, UPDATE_URL).href;\n"
"  const r=await fetch(url,{cache:\"no-store\"}); if(!r.ok) throw 0;\n"
"  const total=+r.headers.get(\"content-length\")||i.size||0;\n"
"  let blob;\n"
"  if(r.body && total && onProg){ const rd=r.body.getReader(), parts=[]; let got=0;\n"
"    for(;;){ const {done,value}=await rd.read(); if(done) break; parts.push(value); got+=value.length; onProg(got/total); }\n"
"    blob=new Blob(parts); }\n"
"  else blob=await r.blob();\n"
"  if(i.size && blob.size!==i.size) throw 0;\n"
"  return blob;\n"
"}\n"
"async function prefetch(){   // get the new version onto the phone in the background, while there's internet\n"
"  const i=upd.info; if(!i || !st || verCmp(i.version, st.fw)<=0 || upd.fetching || upd.cached===i.version) return;\n"
"  if(await fwCache.get(ck(i.version))){ upd.cached=i.version; renderUpd(); return; }\n"
"  upd.fetching=true; renderUpd();\n"
"  try{ const b=await fetchFirmware(i,null); await fwCache.put(ck(i.version),b); upd.cached=i.version; }catch(e){}\n"
"  upd.fetching=false; renderUpd();\n"
"}\n"
"async function checkUpdate(force){\n"
"  if(upd.busy || !st || !st.fw) return;\n"
"  if(!force && Date.now()-upd.checkedAt<30*60000){ refreshCached(); return; }\n"
"  upd.checkedAt=Date.now(); upd.state=\"checking\"; upd.note=\"\"; renderUpd();\n"
"  try{\n"
"    const ctl=new AbortController(), tm=setTimeout(()=>ctl.abort(),8000);\n"
"    const r=await fetch(UPDATE_URL+\"?t=\"+Date.now(),{cache:\"no-store\",signal:ctl.signal}); clearTimeout(tm);\n"
"    if(!r.ok) throw 0;\n"
"    let j=await r.json(); if(!j.version||!j.url) throw 0;\n"
"    if(st.board===\"4MB\"){ if(!j.url_4mb){ upd.info=null; upd.state=\"ok\"; await refreshCached(); return; }   // no file for this board size yet\n"
"      j=Object.assign({},j,{url:j.url_4mb, size:j.size_4mb||0}); }\n"
"    upd.info=j; upd.state=\"ok\";\n"
"    if(j.version!==st.latest) fetch(\"/api/set?latest=\"+encodeURIComponent(j.version),{cache:\"no-store\"}).catch(()=>{});   // the meter remembers it and shows it on its screen\n"
"  }catch(e){ upd.state=\"offline\"; }\n"
"  await refreshCached(); prefetch();\n"
"}\n"
"function updStep(text,pct){ const w=Math.max(0,Math.min(100,pct))+\"%\";\n"
"  $(\"updProg\").hidden=false; $(\"updStep\").textContent=text; $(\"updBar\").style.width=w; $(\"updMsg\").hidden=false; $(\"updMsg\").textContent=\"\";\n"
"  const b=$(\"updBanner\"); b.hidden=false; b.className=\"updbanner\"; clearTimeout(b._t);   // shows on every page while updating\n"
"  $(\"updBanText\").textContent=\"Software update: \"+text; $(\"updBanProg\").hidden=false; $(\"updBanBar\").style.width=w; }\n"
"function sendFirmware(blob, onProg){\n"
"  if(window.__simUpload) return window.__simUpload(blob,onProg);\n"
"  return new Promise((res)=>{\n"
"    const x=new XMLHttpRequest(); x.open(\"POST\",\"/api/update?size=\"+blob.size+(upd.toVer?\"&ver=\"+encodeURIComponent(upd.toVer):\"\")+pinQ());\n"
"    x.upload.onprogress=e=>{ if(e.lengthComputable) onProg(e.loaded/e.total); };\n"
"    x.onload=()=>{ try{ res(JSON.parse(x.responseText)); }catch(e){ res({ok:false,err:\"The meter gave an odd answer.\"}); } };\n"
"    x.onerror=()=>res({ok:false,err:\"Lost the connection to the meter. Nothing was changed unless the meter shows UPDATED.\"});\n"
"    x.timeout=180000; x.ontimeout=x.onerror;\n"
"    const fd=new FormData(); fd.append(\"firmware\",blob,\"PocketSPL.bin\"); x.send(fd);\n"
"  });\n"
"}\n"
"async function installBlob(blob, label){\n"
"  upd.toVer = /^[0-9.]+$/.test(label||\"\") ? label : \"\";\n"
"  if(blob.size<100000 || blob.size>1966080){ updDone(false,\"That doesn't look like PocketSPL software (wrong size).\"); return; }\n"
"  const head=new Uint8Array(await blob.slice(0,1).arrayBuffer());\n"
"  if(head[0]!==0xE9){ updDone(false,\"That doesn't look like PocketSPL software.\"); return; }\n"
"  updStep(\"Sending to the meter… keep this page open\",0);\n"
"  upd.sending=true;\n"
"  const r=await sendFirmware(blob,f=>updStep(\"Sending to the meter… \"+Math.round(f*100)+\"%\",f*100));\n"
"  upd.sending=false;\n"
"  if(!r.ok){ updDone(false,r.err||\"Not updated.\"); return; }\n"
"  updStep(\"Installed. The meter is restarting… (about 15 seconds)\",100);\n"
"  const was=st.fw, t0=Date.now();\n"
"  let wentAway=false;\n"
"  const wait=async()=>{\n"
"    await new Promise(z=>setTimeout(z,2000));\n"
"    const up=await call(\"/api/state\");\n"
"    if(!up) wentAway=true;\n"
"    else if(st.fw && st.fw!==was){ fwCache.clear(); upd.cached=null; updDone(true,\"Updated to version \"+st.fw+\".\"); return; }\n"
"    else if(wentAway){ updDone(true,\"The meter restarted. It's on version \"+st.fw+\".\"); return; }\n"
"    if(Date.now()-t0>90000){ updDone(true,\"Installed. If this page doesn't come back, rejoin the meter's WiFi and reload.\"); return; }\n"
"    wait();\n"
"  };\n"
"  wait();\n"
"}\n"
"function updDone(good,msg){\n"
"  upd.busy=false; upd.note=msg; $(\"updProg\").hidden=true;\n"
"  const b=$(\"updBanner\"); b.hidden=false; b.className=\"updbanner \"+(good?\"good\":\"bad\"); $(\"updBanText\").textContent=msg; $(\"updBanProg\").hidden=true;\n"
"  clearTimeout(b._t); b._t=setTimeout(()=>{ b.hidden=true; },good?8000:15000);\n"
"  toast(good?\"Updated\":\"Not updated\"); renderUpd();\n"
"}\n"
"$(\"updCheck\").onclick=()=>checkUpdate(true);\n"
"$(\"updInstall\").onclick=async()=>{\n"
"  const v=newestKnown(); if(!v) return;\n"
"  if(!confirm(\"Install version \"+v+\"? The meter restarts (about 30 seconds).\")) return;\n"
"  upd.busy=true; renderUpd();\n"
"  let blob=await fwCache.get(ck(v));\n"
"  if(!blob){\n"
"    if(!upd.info || upd.info.version!==v){ updDone(false,\"Your phone needs internet to download it first.\"); return; }\n"
"    updStep(\"Downloading version \"+v+\"…\",0);\n"
"    try{ blob=await fetchFirmware(upd.info,f=>updStep(\"Downloading… \"+Math.round(f*100)+\"%\",f*100)); await fwCache.put(ck(v),blob); upd.cached=v; }\n"
"    catch(e){ updDone(false,\"Couldn't download the update. Check the phone has internet and try again.\"); return; }\n"
"  }\n"
"  await installBlob(blob,v);\n"
"};\n"
"$(\"updFile\").onchange=async e=>{\n"
"  const f=e.target.files[0]; e.target.value=\"\"; if(!f) return;\n"
"  if(!confirm(\"Install \"+f.name+\"? The meter restarts (about 30 seconds). The gig log is cleared, so download it first if you need it.\")) return;\n"
"  upd.busy=true; renderUpd(); await installBlob(f,f.name);\n"
"};\n"
"/* ---- lock, about, factory calibration, saved gigs ---- */\n"
"let pinVerified=false;\n"
"// ---- alerts ----\n"
"let alDirty=false;\n"
"function renderAlerts(){\n"
"  $(\"alOn\").checked=!!st.alon; $(\"alBack\").checked=!!st.alback;\n"
"  document.querySelectorAll(\"#segAlHold button\").forEach(b=>b.classList.toggle(\"on\",+b.dataset.v===st.alhold));\n"
"  document.querySelectorAll(\"#segAlRep button\").forEach(b=>b.classList.toggle(\"on\",+b.dataset.v===st.alrep));\n"
"  if(!alDirty && document.activeElement!==$(\"alTopic\")){ $(\"alTopic\").value=st.altopic||\"\"; }\n"
"  let t = !st.alon ? \"Off.\" : !st.online ? \"On, but the meter isn't on a WiFi network with internet, so alerts can't go out.\" : \"On. Alerts go out when it's over \"+(st.lim?st.lim.db:\"--\")+\" dB\"+(st.alhold?\" for \"+st.alhold+\" min\":\"\")+\".\";\n"
"  if(st.allast) t += \" Last: \"+st.allast+\".\";\n"
"  $(\"alState\").textContent=t;\n"
"}\n"
"$(\"alTopic\").addEventListener(\"input\",()=>alDirty=true);\n"
"$(\"alOn\").onchange=()=>set(\"alon=\"+($(\"alOn\").checked?1:0), $(\"alOn\").checked?\"Alerts on\":\"Alerts off\");\n"
"$(\"alBack\").onchange=()=>set(\"alback=\"+($(\"alBack\").checked?1:0));\n"
"document.querySelectorAll(\"#segAlHold button\").forEach(b=>b.onclick=()=>set(\"alhold=\"+b.dataset.v));\n"
"document.querySelectorAll(\"#segAlRep button\").forEach(b=>b.onclick=()=>set(\"alrep=\"+b.dataset.v));\n"
"$(\"alSave\").onclick=async()=>{\n"
"  const t=$(\"alTopic\").value.trim();\n"
"  if(!/^[A-Za-z0-9_-]{1,64}$/.test(t)){ toast(\"Topic: letters, numbers, - and _ only\"); return; }\n"
"  alDirty=false; await set(\"altopic=\"+encodeURIComponent(t), \"Topic saved\");\n"
"};\n"
"$(\"alTest\").onclick=()=>set(\"altest=1\", \"Sending a test\");\n"
"$(\"alNew\").onclick=()=>{ alDirty=false; set(\"alnew=1\", \"New topic: add it in the ntfy app\"); };\n"
"$(\"alCopy\").onclick=async()=>{ try{ await navigator.clipboard.writeText($(\"alTopic\").value); toast(\"Copied\"); }catch(e){ $(\"alTopic\").select(); toast(\"Select and copy it\"); } };\n"
"\n"
"// ---- on/off schedule (wired model) ----\n"
"const SDAYS=[\"Sun\",\"Mon\",\"Tue\",\"Wed\",\"Thu\",\"Fri\",\"Sat\"], SORDER=[1,2,3,4,5,6,0];\n"
"let schRows=null, schDirty=false;\n"
"const m2t=m=>String(Math.floor(m/60)).padStart(2,\"0\")+\":\"+String(m%60).padStart(2,\"0\");\n"
"const t2m=t=>{ const p=/^(\\d{1,2}):(\\d{2})/.exec(t||\"\"); return p? (+p[1])*60+(+p[2]) : -1; };\n"
"function schParse(str){ return (str||\"\").split(\";\").filter(Boolean).map(x=>{ const v=x.split(\",\").map(Number); return {d:v[0],on:v[1],off:v[2]}; }); }\n"
"function schDraw(){\n"
"  const box=$(\"schList\"); box.innerHTML=\"\";\n"
"  schRows.forEach((r,i)=>{\n"
"    const row=document.createElement(\"div\"); row.className=\"schrow\";\n"
"    const sel=document.createElement(\"select\"); sel.setAttribute(\"aria-label\",\"Day\");\n"
"    SORDER.forEach(d=>{ const o=document.createElement(\"option\"); o.value=d; o.textContent=SDAYS[d]; if(d===r.d) o.selected=true; sel.appendChild(o); });\n"
"    const on=document.createElement(\"input\"); on.type=\"time\"; on.value=m2t(r.on); on.setAttribute(\"aria-label\",\"On at\");\n"
"    const onL=document.createElement(\"span\"); onL.textContent=\"On\";\n"
"    const to=document.createElement(\"span\"); to.textContent=\"Off\";\n"
"    const off=document.createElement(\"input\"); off.type=\"time\"; off.value=m2t(r.off); off.setAttribute(\"aria-label\",\"Off at\");\n"
"    const x=document.createElement(\"button\"); x.type=\"button\"; x.className=\"btn x\"; x.textContent=\"×\"; x.setAttribute(\"aria-label\",\"Remove\");\n"
"    sel.onchange=()=>{ r.d=+sel.value; schDirty=true; };\n"
"    on.onchange=()=>{ r.on=t2m(on.value); schDirty=true; };\n"
"    off.onchange=()=>{ r.off=t2m(off.value); schDirty=true; };\n"
"    x.onclick=()=>{ schRows.splice(i,1); schDirty=true; schDraw(); };\n"
"    row.append(sel,x,onL,on,to,off); box.appendChild(row);\n"
"  });\n"
"  $(\"schAdd\").disabled = schRows.length>=8;\n"
"}\n"
"function renderSchedule(){\n"
"  document.body.classList.toggle(\"wired\",!!st.wired); $(\"logSaveNote\").textContent = st.wired ? \"Saved in the meter every 5 minutes (up to 24 hours), so a power cut loses at most 5 minutes.\" : \"Saved in the meter every minute (up to 24 hours), so switching off doesn't lose it.\";\n"
"  if(!st.wired) return;\n"
"  if(!schDirty){ schRows=schParse(st.sch); schDraw(); }\n"
"  $(\"schOn\").checked=!!st.schon;\n"
"  let t=\"\";\n"
"  if(st.schon && st.sch){ t = !st.clk ? \"Waiting for the time: it stays on until it knows it.\" : st.standby ? \"Standby now. Comes on \"+(st.next||\"--\")+\".\" : \"On now\"+(st.next?\". Goes to standby \"+st.next+\".\":\".\"); }\n"
"  else t = \"Off: the meter runs all the time.\";\n"
"  $(\"schState\").textContent=t;\n"
"  $(\"schClock\").textContent = st.clk ? \"Meter clock: \"+st.clock+\". Set by this phone, and from the internet when it's on the venue's WiFi.\" : \"The meter doesn't know the time yet. It gets it from this phone, or from the internet on the venue's WiFi.\";\n"
"}\n"
"$(\"schAdd\").onclick=()=>{ if(!schRows) schRows=[]; const last=schRows[schRows.length-1]; schRows.push(last?{d:(last.d+1)%7,on:last.on,off:last.off}:{d:5,on:1110,off:120}); schDirty=true; schDraw(); };\n"
"$(\"schSave\").onclick=async()=>{\n"
"  for(const r of schRows){ if(r.on<0||r.off<0){ toast(\"Fill in every time\"); return; } if(r.on===r.off){ toast(\"On and off can't be the same time\"); return; } }\n"
"  const q=schRows.map(r=>r.d+\",\"+r.on+\",\"+r.off).join(\";\");\n"
"  schDirty=false;\n"
"  await set(\"sch=\"+encodeURIComponent(q)+\"&schon=\"+($(\"schOn\").checked||schRows.length?1:0), \"Schedule saved\");\n"
"};\n"
"$(\"schOn\").onchange=()=>set(\"schon=\"+($(\"schOn\").checked?1:0), $(\"schOn\").checked?\"Schedule on\":\"Schedule off\");\n"
"function renderLock(){\n"
"  if(!st) return;\n"
"  const locked=!!st.lock, open=!locked||pinVerified;\n"
"  document.body.classList.toggle(\"locked\", !open);\n"
"  $(\"lockBar\").hidden=open;\n"
"  $(\"lockOff\").hidden=locked; $(\"lockOn\").hidden=!locked;\n"
"  $(\"unlockRow\").hidden=open; $(\"unlockedActs\").hidden=!(locked&&pinVerified);\n"
"  $(\"lockState\").textContent = pinVerified ? \"Locked, but unlocked on this phone until you close the page.\" : \"Locked. Enter the PIN to change settings.\";\n"
"  $(\"snVal\").textContent=st.sn||\"--\"; $(\"modelVal\").textContent=st.model?\"PocketSPL \"+st.model+(st.board?\" · \"+st.board+\" board\":\"\"):\"--\"; $(\"fwVer2\").textContent=st.fw?\"v\"+st.fw:\"--\";\n"
"  const fc=st.fcal; $(\"fcalBox\").hidden = fc===null||fc===undefined;\n"
"  if(fc!==null&&fc!==undefined){ $(\"fcalVal\").textContent=(fc>=0?\"+\":\"\")+fc.toFixed(1)+\" dB\"; $(\"fcalBack\").disabled=Math.abs(fc-st.cal)<0.005; }\n"
"}\n"
"$(\"lockBtn\").onclick=async()=>{ const v=$(\"pinNew\").value.trim();\n"
"  if(!/^[0-9]{4,8}$/.test(v)){ toast(\"PIN: 4 to 8 numbers\"); return; }\n"
"  if(!confirm(\"Lock settings with PIN \"+v+\"? Write it down.\")) return;\n"
"  if(await set(\"lockset=\"+v,\"Settings locked\")){ savePin(v); pinVerified=true; $(\"pinNew\").value=\"\"; } renderLock(); };\n"
"$(\"unlockBtn\").onclick=async()=>{ const v=$(\"pinIn\").value.trim(); if(!v) return;\n"
"  const good=await call(\"/api/set?pincheck=1&pin=\"+encodeURIComponent(v));\n"
"  if(good && st.pinok){ savePin(v); pinVerified=true; $(\"pinIn\").value=\"\"; toast(\"Unlocked on this phone\"); } else toast(\"Wrong PIN\");\n"
"  renderLock(); };\n"
"$(\"relockBtn\").onclick=()=>{ savePin(\"\"); pinVerified=false; renderLock(); toast(\"Locked again\"); };\n"
"$(\"lockRemove\").onclick=async()=>{ if(!confirm(\"Remove the lock? Anyone on the meter's WiFi can then change the settings.\")) return;\n"
"  if(await set(\"lockoff=1\",\"Lock removed\")){ savePin(\"\"); pinVerified=false; } renderLock(); };\n"
"$(\"fcalBack\").onclick=()=>set(\"fcalback=1\",\"Back to factory calibration\");\n"
"$(\"factoryBtn\").onclick=()=>{ $(\"factoryConfirm\").hidden=false; $(\"factoryBtn\").hidden=true; };\n"
"$(\"factoryNo\").onclick=()=>{ $(\"factoryConfirm\").hidden=true; $(\"factoryBtn\").hidden=false; };\n"
"$(\"factoryYes\").onclick=async()=>{ $(\"factoryConfirm\").hidden=true; $(\"factoryBtn\").hidden=false;\n"
"  if(await set(\"factory=1\",\"Factory reset. The meter is restarting\")){ savePin(\"\"); pinVerified=false; } };\n"
"// a PIN remembered from earlier in this session still works after a reload\n"
"if(getPin()) pinVerified=true;\n"
"\n"
"let logsKey=\"\";\n"
"async function loadSavedLogs(){\n"
"  let j=null;\n"
"  if(window.parent!==window && window.parent.PocketSim) j=window.parent.PocketSim.logs();\n"
"  else { try{ const r=await fetch(\"/api/logs\",{cache:\"no-store\"}); j=await r.json(); }catch(e){ return; } }\n"
"  const key=JSON.stringify(j); if(key===logsKey) return; logsKey=key;\n"
"  const L=$(\"savedLogs\"); L.innerHTML=\"\";\n"
"  if(!j.fs){ L.innerHTML='<p class=\"note\">Saved gigs need the meter loaded with the Partition Scheme from the build guide.</p>'; return; }\n"
"  if(!j.logs.length){ L.innerHTML='<p class=\"note\">None yet.</p>'; return; }\n"
"  j.logs.forEach(g=>{\n"
"    const row=document.createElement(\"div\"); row.className=\"logrow\";\n"
"    const meta=document.createElement(\"div\"); const b=document.createElement(\"span\");\n"
"    b.textContent = g.t0 ? new Date(g.t0).toLocaleString([], {weekday:\"short\", day:\"numeric\", month:\"short\", hour:\"numeric\", minute:\"2-digit\"}) : \"Gig \"+g.id;\n"
"    const sm=document.createElement(\"small\"); const h=Math.floor(g.mins/60), m=g.mins%60;\n"
"    sm.textContent=(h?h+\" h \":\"\")+m+\" min\"+(g.marks?\", \"+g.marks+\" marker\"+(g.marks>1?\"s\":\"\"):\"\");\n"
"    meta.append(b,sm);\n"
"    const acts=document.createElement(\"div\"); acts.className=\"acts\";\n"
"    const dl=document.createElement(\"a\"); dl.className=\"btn small\"; dl.textContent=\"Download\"; dl.href=\"/api/oldlog.csv?id=\"+g.id; dl.download=\"PocketSPL-log-\"+g.id+\".csv\";\n"
"    dl.onclick=e=>{ const P=window.parent!==window&&window.parent.PocketSim; if(!P) return; e.preventDefault(); toast(\"Simulator: downloads on the real meter\"); };\n"
"    const del=document.createElement(\"button\"); del.className=\"btn small\"; del.textContent=\"Delete\"; del.setAttribute(\"data-lock\",\"\");\n"
"    del.onclick=async()=>{ if(!confirm(\"Delete this saved gig?\")) return;\n"
"      if(window.parent!==window && window.parent.PocketSim){ window.parent.PocketSim.delLog(g.id); }\n"
"      else { try{ const r=await fetch(\"/api/logs?del=\"+g.id+pinQ(),{cache:\"no-store\"}); const k=await r.json(); if(k.denied){ toast(\"Locked: enter the PIN in Settings → General → Lock\"); return; } }catch(e){} }\n"
"      logsKey=\"\"; loadSavedLogs(); toast(\"Deleted\"); };\n"
"    acts.append(dl,del); row.append(meta,acts); L.append(row);\n"
"  });\n"
"}\n"
"/* ---- pages ---- */\n"
"const TABS=[\"meter\",\"rta\",\"gig\",\"settings\"], SUBS=[\"general\",\"wifi\",\"software\"];\n"
"let sub=\"general\"; try{ sub=localStorage.getItem(\"pocketspl-sub\")||\"general\"; }catch(e){} if(!SUBS.includes(sub)) sub=\"general\";\n"
"function showSub(s){\n"
"  sub=SUBS.includes(s)?s:\"general\";\n"
"  try{ localStorage.setItem(\"pocketspl-sub\",sub); }catch(e){}\n"
"  document.querySelectorAll(\"#segSub button\").forEach(b=>b.classList.toggle(\"on\",b.dataset.s===sub));\n"
"  showTab(\"settings\");\n"
"}\n"
"document.querySelectorAll(\"#segSub button\").forEach(b=>b.onclick=()=>showSub(b.dataset.s));\n"
"function showTab(t){\n"
"  if(!TABS.includes(t)) t=\"meter\";\n"
"  document.querySelectorAll(\".card[data-tab]\").forEach(c=>c.hidden = c.dataset.tab!==t || (c.dataset.sub && c.dataset.sub!==sub));\n"
"  document.querySelectorAll(\"#tabs button\").forEach(b=>{ b.classList.toggle(\"on\",b.dataset.t===t); b.setAttribute(\"aria-current\",b.dataset.t===t?\"page\":\"false\"); });\n"
"  try{ localStorage.setItem(\"pocketspl-tab\",t); }catch(e){}\n"
"  window.scrollTo(0,0);\n"
"  if(t===\"gig\"){ loadLog(); loadSavedLogs(); }\n"
"  if(t===\"settings\"){ checkUpdate(false); document.querySelectorAll(\"#segSub button\").forEach(b=>b.classList.toggle(\"on\",b.dataset.s===sub)); }\n"
"  render();\n"
"}\n"
"document.querySelectorAll(\"#tabs button\").forEach(b=>b.onclick=()=>showTab(b.dataset.t));\n"
"let startTab=\"meter\"; try{ startTab=localStorage.getItem(\"pocketspl-tab\")||\"meter\"; }catch(e){}\n"
"showTab(startTab);\n"
"\n"
"poll();\n"
"setTimeout(()=>checkUpdate(false),4000);\n"
"</script>\n"
"</body>\n"
"</html>\n"
"\n";

// ---------------- Networks ----------------
// The meter can join up to 3 saved WiFi networks (home, studio, travel router). At switch-on it
// joins the strongest one it can see; if none are around it makes its own "PocketSPL" network.
const int MAX_NETS = 3;
const uint32_t JOIN_TIMEOUT_MS = 12000, NET_LOST_MS = 30000, AP_LINGER_MS = 120000;
String netSsid[MAX_NETS], netPass[MAX_NETS];
String netFixIp[MAX_NETS], netMask[MAX_NETS], netGw[MAX_NETS];   // fixed address (blank = automatic / DHCP)
bool joinSaved = true;                     // try saved networks at switch-on
enum NetMode { NET_OFF, NET_JOINING, NET_STA, NET_AP };
NetMode netMode = NET_OFF;
bool apOn = false;                         // the meter's own network is running
int joinOrder[MAX_NETS]; int joinCount = 0, joinPos = 0;
uint32_t joinT0 = 0, staLostAt = 0, apOffAt = 0;
bool joinFromPhone = false;
String joinSsid, joinPass, netMsg;

String jsonEsc(const String& in) {
  String o;
  for (unsigned i = 0; i < in.length(); i++) {
    char c = in[i];
    if (c == '"' || c == '\\') { o += '\\'; o += c; }
    else if ((uint8_t)c >= 0x20) o += c;
  }
  return o;
}

void loadNets() {
  char k[6];
  for (int i = 0; i < MAX_NETS; i++) {
    snprintf(k, sizeof(k), "ns%d", i); netSsid[i] = prefs.getString(k, "");
    snprintf(k, sizeof(k), "np%d", i); netPass[i] = prefs.getString(k, "");
    snprintf(k, sizeof(k), "ni%d", i); netFixIp[i] = prefs.getString(k, "");
    snprintf(k, sizeof(k), "nm%d", i); netMask[i] = prefs.getString(k, "");
    snprintf(k, sizeof(k), "ng%d", i); netGw[i] = prefs.getString(k, "");
  }
  joinSaved = prefs.getBool("join", true);
}
void saveNets() {
  char k[6];
  for (int i = 0; i < MAX_NETS; i++) {
    snprintf(k, sizeof(k), "ns%d", i); prefs.putString(k, netSsid[i]);
    snprintf(k, sizeof(k), "np%d", i); prefs.putString(k, netPass[i]);
    snprintf(k, sizeof(k), "ni%d", i); prefs.putString(k, netFixIp[i]);
    snprintf(k, sizeof(k), "nm%d", i); prefs.putString(k, netMask[i]);
    snprintf(k, sizeof(k), "ng%d", i); prefs.putString(k, netGw[i]);
  }
  prefs.putBool("join", joinSaved);
}
void copyNet(int to, int from) {
  netSsid[to] = netSsid[from]; netPass[to] = netPass[from];
  netFixIp[to] = netFixIp[from]; netMask[to] = netMask[from]; netGw[to] = netGw[from];
}
void rememberNet(const String& ss, const String& pw, const String& ip, const String& mask, const String& gw) {   // newest first
  int at = MAX_NETS - 1;
  for (int i = 0; i < MAX_NETS; i++) if (netSsid[i] == ss) { at = i; break; }
  for (int i = at; i > 0; i--) copyNet(i, i - 1);
  netSsid[0] = ss; netPass[0] = pw; netFixIp[0] = ip; netMask[0] = mask; netGw[0] = gw;
  saveNets();
}
void forgetNet(int i) {
  if (i < 0 || i >= MAX_NETS) return;
  for (int k = i; k < MAX_NETS - 1; k++) copyNet(k, k + 1);
  netSsid[MAX_NETS - 1] = ""; netPass[MAX_NETS - 1] = ""; netFixIp[MAX_NETS - 1] = ""; netMask[MAX_NETS - 1] = ""; netGw[MAX_NETS - 1] = "";
  saveNets();
}

void wifiMode(wifi_mode_t m) {              // set the WiFi mode, always with Australian channel / power rules
  WiFi.mode(m);
  if (m != WIFI_OFF) esp_wifi_set_country_code("AU", true);
}
void startAP(bool keepSta) {
  wifiMode(keepSta ? WIFI_AP_STA : WIFI_AP);
  WiFi.softAP(apName.c_str(), apPass.c_str(), 6, 0, 2);
  WiFi.setSleep(false);                              // radio always awake: no dropped phones
  if (!keepSta) WiFi.setTxPower(WIFI_POWER_15dBm);    // own network: strong enough to hold the phone through the case
  apOn = true;
}
void stopAP() { WiFi.softAPdisconnect(true); wifiMode(WIFI_STA); apOn = false; }

bool validApName(const String& n) {         // 1-32 plain characters
  if (n.length() < 1 || n.length() > 32) return false;
  for (unsigned i = 0; i < n.length(); i++) if ((uint8_t)n[i] < 0x20 || (uint8_t)n[i] > 0x7E) return false;
  return true;
}
bool validApPass(const String& p) {         // 8-63 plain characters (WPA2 rule)
  if (p.length() < 8 || p.length() > 63) return false;
  for (unsigned i = 0; i < p.length(); i++) if ((uint8_t)p[i] < 0x20 || (uint8_t)p[i] > 0x7E) return false;
  return true;
}
void loadApWifi() {
  apName = prefs.getString("apn", DEFAULT_WIFI_NAME);
  apPass = prefs.getString("app", DEFAULT_WIFI_PASS);
  if (!validApName(apName) || !validApPass(apPass)) { apName = DEFAULT_WIFI_NAME; apPass = DEFAULT_WIFI_PASS; }
}
void saveApWifi() { prefs.putString("apn", apName); prefs.putString("app", apPass); }
String mdnsName() {                         // "PocketSPL FOH" -> pocketspl-foh  (http://pocketspl-foh.local)
  String o;
  for (unsigned i = 0; i < apName.length() && o.length() < 30; i++) {
    char c = tolower(apName[i]);
    if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) o += c;
    else if (o.length() && o[o.length() - 1] != '-') o += '-';
  }
  while (o.length() && o[o.length() - 1] == '-') o.remove(o.length() - 1);
  return o.length() ? o : String("pocketspl");
}

String joinIp, joinMask, joinGw;

// checks a fixed-address setting; returns "" if fine, or what's wrong
String checkAddress(const String& ip, const String& mask, const String& gw) {
  IPAddress a, m, g;
  if (!ip.length()) return "";                              // automatic
  if (!a.fromString(ip)) return "The address isn't valid (it should look like 192.168.1.50)";
  if (!m.fromString(mask.length() ? mask : String("255.255.255.0"))) return "The subnet mask isn't valid";
  if (gw.length() && !g.fromString(gw)) return "The router address isn't valid";
  if (a[0] == 192 && a[1] == 168 && a[2] == 4) return "Use an address outside 192.168.4.x (the meter's own WiFi uses it)";
  return "";
}

void tryJoin(const String& ss, const String& pw, const String& ip, const String& mask, const String& gw) {
  WiFi.setTxPower(WIFI_POWER_15dBm);       // reach the router from across the room
  if (ip.length()) {                        // fixed address for networks with no DHCP server
    IPAddress a, m, g((uint32_t)0);
    a.fromString(ip); m.fromString(mask.length() ? mask : String("255.255.255.0"));
    if (gw.length()) g.fromString(gw);
    WiFi.config(a, g, m);
  } else {
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);     // automatic (DHCP)
  }
  WiFi.begin(ss.c_str(), pw.length() ? pw.c_str() : nullptr);
  joinSsid = ss; joinPass = pw; joinIp = ip; joinMask = mask; joinGw = gw;
  joinT0 = millis(); netMode = NET_JOINING;
}
void tryJoinSaved(int i) { tryJoin(netSsid[i], netPass[i], netFixIp[i], netMask[i], netGw[i]); }

bool beginJoinSaved() {                    // scan, then try the saved networks that are in range, strongest first
  bool any = false;
  for (int i = 0; i < MAX_NETS; i++) if (netSsid[i].length()) any = true;
  if (!joinSaved || !any) return false;
  wifiMode(WIFI_STA);
  int n = WiFi.scanNetworks();
  int best[MAX_NETS];
  joinCount = 0;
  for (int i = 0; i < MAX_NETS; i++) {
    best[i] = -999;
    if (!netSsid[i].length()) continue;
    for (int k = 0; k < n; k++) if (WiFi.SSID(k) == netSsid[i] && WiFi.RSSI(k) > best[i]) best[i] = WiFi.RSSI(k);
    if (best[i] > -999) joinOrder[joinCount++] = i;
  }
  WiFi.scanDelete();
  for (int a = 1; a < joinCount; a++)      // strongest first
    for (int b = a; b > 0 && best[joinOrder[b]] > best[joinOrder[b - 1]]; b--) { int t = joinOrder[b]; joinOrder[b] = joinOrder[b - 1]; joinOrder[b - 1] = t; }
  if (!joinCount) { netMsg = "No saved network in range"; return false; }
  joinPos = 0; joinFromPhone = false;
  tryJoinSaved(joinOrder[0]);
  netMsg = "Joining " + joinSsid + "...";
  return true;
}

String netIp() {
  if (netMode == NET_STA && WiFi.status() == WL_CONNECTED) return WiFi.localIP().toString();
  if (apOn) return "192.168.4.1";
  return "";
}

bool phoneLinked() {                       // for the little WiFi symbol: blue = a phone can reach the meter
  if (netMode == NET_STA && WiFi.status() == WL_CONNECTED) return true;
  return apOn && WiFi.softAPgetStationNum() > 0;
}

// writes a number, or null while there isn't one yet
static void jnum(String& s, const char* key, bool valid, float v) {
  char t[24];
  if (valid) snprintf(t, sizeof(t), "\"%s\":%.1f,", key, v); else snprintf(t, sizeof(t), "\"%s\":null,", key);
  s += t;
}

void sendState() {
  takeSnapshot();
  bool warm = warmup > 0;
  float fast = snapFastDb();
  String s = "{";
  jnum(s, "db", !warm, snapDb());
  jnum(s, "fast", !warm, fast);
  jnum(s, "peak", !warm, peakDb);
  jnum(s, "max", !warm && snap.maxF[wMode] > 0, toDb(snap.maxF[wMode]));
  jnum(s, "leq", !warm && snap.leqN > 0, snapLeqDb());
  s += "\"rta\":[";
  for (int b = 0; b < RTA_BANDS; b++) {
    char t2[12];
    if (rtaReady && !warm) snprintf(t2, sizeof(t2), "%s%.1f", b ? "," : "", rtaBandDb(b)); else snprintf(t2, sizeof(t2), "%snull", b ? "," : "");
    s += t2;
  }
  s += "],";
  char t[200];
  snprintf(t, sizeof(t),
    "\"leqs\":%lu,\"w\":%d,\"r\":%d,\"warm\":%s,\"over\":%s,\"clip\":%s,\"calst\":%d,\"cal\":%.2f,"
    "\"amber\":%d,\"red\":%d,\"bright\":%d,\"batt\":%d,\"chg\":%s,",
    snapLeqSecs(), (int)wMode, (int)rMode, warm ? "true" : "false",
    (!warm && fast > OVERLOAD_DB) ? "true" : "false", (!warm && micClipping()) ? "true" : "false", (int)calState, calOffset,
    amberFrom, redFrom, (int)brightness, battPct, charging ? "true" : "false");
  s += t;
  s += "\"theme\":"; s += String((int)theme); s += ",";
  s += "\"fw\":\"" FW_VERSION "\",\"model\":\""; s += modelName(); s += "\",\"ota\":"; s += otaReady() ? "true," : "false,";
  s += "\"wired\":"; s += wiredModel ? "true," : "false,";
  s += "\"board\":\"" BOARD_FLASH "\",";
  s += "\"sn\":\""; s += jsonEsc(serialNo); s += "\",";
  s += "\"latest\":\""; s += latestFw; s += "\",";
  s += "\"lock\":"; s += lockPin.length() ? "true," : "false,";
  s += "\"micfault\":"; s += micFaulty() ? "true," : "false,";
  s += "\"fs\":"; s += fsOk ? "true," : "false,";
  if (factoryCalSet) { char fc[32]; snprintf(fc, sizeof(fc), "\"fcal\":%.2f,", factoryCal); s += fc; } else s += "\"fcal\":null,";
  {                                          // clock + on/off schedule
    char cb[32]; int wm = weekMinuteNow();
    s += "\"clk\":"; s += wm >= 0 ? "true," : "false,";
    if (wm >= 0) { fmtWeekMinute(wm, cb, sizeof(cb)); s += "\"clock\":\""; s += cb; s += "\","; }
    s += "\"schon\":"; s += schEnabled ? "true," : "false,";
    s += "\"sch\":\""; s += scheduleString(); s += "\",";
    s += "\"standby\":"; s += standby ? "true," : "false,";
    int nx = (wm >= 0 && scheduleActive()) ? nextChange(wm) : -1;
    if (nx >= 0) { fmtWeekMinute(nx, cb, sizeof(cb)); s += "\"next\":\""; s += cb; s += "\","; }
  }
  s += "\"alon\":"; s += alOn ? "true," : "false,";
  s += "\"alback\":"; s += alBack ? "true," : "false,";
  s += "\"alhold\":"; s += String(alHold); s += ",\"alrep\":"; s += String(alRep); s += ",";
  s += "\"altopic\":\""; s += jsonEsc(alTopic); s += "\",";
  s += "\"allast\":\""; s += jsonEsc(alLast); s += "\",\"online\":"; s += netOnline() ? "true," : "false,";
  if (lastDenied) s += "\"denied\":true,";
  if (lastPinOk) s += "\"pinok\":true,";
  lastDenied = false; lastPinOk = false;
  const char* mode = netMode == NET_STA ? "sta" : netMode == NET_JOINING ? "joining" : netMode == NET_AP ? "ap" : "off";
  {                                          // gig mode
    char g[48];
    s += "\"roll\":[";
    for (int k = 0; k < 3; k++) {
      s += k ? ",[" : "[";
      for (int w2 = 0; w2 < 3; w2++) {
        float v;
        if (!warm && rollDb(k, w2, v)) snprintf(g, sizeof(g), "%s%.1f", w2 ? "," : "", v); else snprintf(g, sizeof(g), "%snull", w2 ? "," : "");
        s += g;
      }
      s += "]";
    }
    snprintf(g, sizeof(g), "],\"rollfull\":[%s,%s,%s],", rollFull(0) ? "true" : "false", rollFull(1) ? "true" : "false", rollFull(2) ? "true" : "false");
    s += g;
    snprintf(g, sizeof(g), "\"lim\":{\"on\":%s,\"db\":%d,\"win\":%d,\"w\":%d,", limOn ? "true" : "false", limDb, limWin, limW); s += g;
    snprintf(g, sizeof(g), "\"st\":%d,\"val\":", limState); s += g;
    if (limReady) { snprintf(g, sizeof(g), "%.1f},", limVal); s += g; } else s += "null},";
    snprintf(g, sizeof(g), "\"dose\":%.1f,\"dim\":%d,\"dimmed\":%s,", warm ? 0.0f : dosePct(), dimSecs, dimmed ? "true" : "false"); s += g;
    snprintf(g, sizeof(g), "\"log\":{\"mins\":%lu,\"secs\":%lu,\"marks\":%d,\"t0\":%lld},",
             (unsigned long)snap.logTotal, (unsigned long)snap.logSecs, markCount, (long long)logStartEpochMs); s += g;
  }
  s += "\"net\":{\"mode\":\""; s += mode;
  s += "\",\"ssid\":\""; s += jsonEsc(netMode == NET_AP ? apName : joinSsid);
  s += "\",\"apname\":\""; s += jsonEsc(apName);
  s += "\",\"apdef\":"; s += (apName == DEFAULT_WIFI_NAME && apPass == DEFAULT_WIFI_PASS) ? "true" : "false";
  s += ",\"mdns\":\""; s += mdnsName();
  s += "\",\"ip\":\""; s += netIp();
  s += "\",\"ap\":"; s += apOn ? "true" : "false";
  s += ",\"phone\":"; s += joinFromPhone ? "true" : "false";
  s += ",\"join\":"; s += joinSaved ? "true" : "false";
  s += ",\"msg\":\""; s += jsonEsc(netMsg);
  s += "\",\"saved\":[";
  bool first = true;
  for (int i = 0; i < MAX_NETS; i++) if (netSsid[i].length()) {
    s += first ? "{" : ",{"; first = false;
    s += "\"s\":\""; s += jsonEsc(netSsid[i]);
    s += "\",\"ip\":\""; s += jsonEsc(netFixIp[i]);
    s += "\",\"mask\":\""; s += jsonEsc(netMask[i]);
    s += "\",\"gw\":\""; s += jsonEsc(netGw[i]); s += "\"}";
  }
  s += "]}}";
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", s);
}

// Settings lock: these need the PIN when the meter is locked. Weighting, response, Reset MAX + Leq and markers don't.
const char* const PROTECTED_ARGS[] = {"bright", "amber", "red", "cal", "cal94", "fcalback", "wifi", "limOn", "limDb", "limWin", "limW",
                                      "dim", "apname", "apdefault", "newlog", "lockset", "lockoff", "factory", "sch", "schon", "alon", "altopic", "alhold", "alrep", "alback", "altest", "alnew"};
int verCmp(const String& a, const String& b) {   // compares "1.0.10" with "1.0.9" the right way
  int pa[3] = {0, 0, 0}, pb[3] = {0, 0, 0};
  sscanf(a.c_str(), "%d.%d.%d", &pa[0], &pa[1], &pa[2]); sscanf(b.c_str(), "%d.%d.%d", &pb[0], &pb[1], &pb[2]);
  for (int i = 0; i < 3; i++) if (pa[i] != pb[i]) return pa[i] < pb[i] ? -1 : 1;
  return 0;
}
bool validVersion(const String& v) {
  if (v.length() < 1 || v.length() > 11) return false;
  for (unsigned i = 0; i < v.length(); i++) if (!(v[i] == '.' || (v[i] >= '0' && v[i] <= '9'))) return false;
  return true;
}
bool updateWaiting() { return latestFw.length() && verCmp(latestFw, FW_VERSION) > 0; }
bool pinOk() { return lockPin.length() == 0 || server.arg("pin") == lockPin; }
bool validPin(const String& p) {
  if (p.length() < 4 || p.length() > 8) return false;
  for (unsigned i = 0; i < p.length(); i++) if (p[i] < '0' || p[i] > '9') return false;
  return true;
}

void handleSet() {
  if (server.hasArg("w")) { setWeighting(server.arg("w").toInt()); toast(W_NAME[wMode], "from phone", 900); }
  if (server.hasArg("r")) { setResponse(server.arg("r").toInt()); toast(R_NAME[rMode], rMode == R_LEQ ? "Leq and Max reset" : "from phone", 900); }
  if (server.hasArg("reset")) { resetRequested = true; toast("Reset", "Max and Leq", 900); }
  if (server.hasArg("mark")) {
    String t = server.arg("mark"); t.trim();
    portENTER_CRITICAL(&audioLock);
    if (markCount < MARK_MAX) {
      marks[markCount].sec = logSecs;
      strncpy(marks[markCount].text, t.length() ? t.c_str() : "Marker", 24); marks[markCount].text[24] = 0;
      markCount++;
    }
    portEXIT_CRITICAL(&audioLock);
    toast("Marker", t.length() ? t.substring(0, 23).c_str() : "added", 1500);
  }
  if (server.hasArg("latest")) {                                // the phone found this version online
    String v = server.arg("latest");
    if (validVersion(v) && v != latestFw) {
      latestFw = v; prefs.putString("latest", latestFw);
      if (updateWaiting()) toast("Update", ("v" + latestFw + " available").c_str(), 3000);
    }
  }
  if (server.hasArg("theme")) { theme = constrain(server.arg("theme").toInt(), 0, NUM_THEMES - 1); applyTheme(); savePrefs(); toast(THEME_NAME[theme], "Look", 1200); }
  if (server.hasArg("pincheck")) lastPinOk = lockPin.length() && server.arg("pin") == lockPin;
  wake(millis());

  bool wantsProtected = false;
  for (const char* k : PROTECTED_ARGS) if (server.hasArg(k)) wantsProtected = true;
  if (wantsProtected && !pinOk()) { lastDenied = true; toast("Locked", "Unlock on the phone", 1500); sendState(); return; }

  if (server.hasArg("bright")) { brightness = constrain(server.arg("bright").toInt(), 1, 15); setBrightness(brightness); savePrefs(); }
  if (server.hasArg("amber")) { amberFrom = constrain(server.arg("amber").toInt(), 50, 120); if (redFrom <= amberFrom) redFrom = amberFrom + 1; savePrefs(); }
  if (server.hasArg("red"))   { redFrom = constrain(server.arg("red").toInt(), 51, 125); if (amberFrom >= redFrom) amberFrom = redFrom - 1; savePrefs(); }
  if (server.hasArg("cal"))   { calOffset = constrain(server.arg("cal").toFloat(), -20.0f, 20.0f); savePrefs(); }
  if (server.hasArg("fcalback") && factoryCalSet) { calOffset = factoryCal; savePrefs(); toast("Calibration", "Factory setting", 1500); }
  if (server.hasArg("cal94")) startCal94();
  if (server.hasArg("wifi") && server.arg("wifi").toInt() == 0) wifiOffAt = millis() + 600;   // after the reply goes out
  if (server.hasArg("sch")) {
    if (parseSchedule(server.arg("sch"))) { prefs.putString("sch", scheduleString()); toast("Schedule", "Saved", 1500); }
    else toast("Schedule", "Not saved", 1500);
  }
  if (server.hasArg("altopic")) { String t = server.arg("altopic"); t.trim(); if (validTopic(t)) { alTopic = t; prefs.putString("alTopic", alTopic); } else toast("Alerts", "Topic not saved", 1500); }
  if (server.hasArg("alnew"))   { alTopic = newTopic(); prefs.putString("alTopic", alTopic); }
  if (server.hasArg("alhold"))  { int v = server.arg("alhold").toInt(); if (v == 0 || v == 2 || v == 5 || v == 10) { alHold = v; prefs.putUChar("alHold", v); } }
  if (server.hasArg("alrep"))   { int v = server.arg("alrep").toInt(); if (v == 15 || v == 30 || v == 60) { alRep = v; prefs.putUChar("alRep", v); } }
  if (server.hasArg("alback"))  { alBack = server.arg("alback").toInt() != 0; prefs.putBool("alBack", alBack); }
  if (server.hasArg("alon"))    { alOn = server.arg("alon").toInt() != 0; prefs.putBool("alOn", alOn); alOverSince = 0; alAlerted = false;
                                  if (alOn && !limOn) { limOn = true; limState = 0; savePrefs(); } }   // alerts use the limit alarm
  if (server.hasArg("altest")) {
    char t[64], b[120], w[32]; alertWhen(w, sizeof(w));
    snprintf(t, sizeof(t), "%s: test alert", apName.c_str());
    snprintf(b, sizeof(b), "Alerts are working. You'll get one when it's over %d dB%s.", limDb, w);
    if (sendAlert(t, b, "default")) alLast = "Sending a test...";
    toast("Alerts", "Test", 1200);
  }
  if (server.hasArg("schon")) { schEnabled = server.arg("schon").toInt() != 0; prefs.putBool("schon", schEnabled); }
  if (server.hasArg("limOn"))  { limOn = server.arg("limOn").toInt() != 0; limState = 0; savePrefs(); }
  if (server.hasArg("limDb"))  { limDb = constrain(server.arg("limDb").toInt(), 60, 125); limState = 0; savePrefs(); }
  if (server.hasArg("limWin")) { limWin = constrain(server.arg("limWin").toInt(), 0, 2); limState = 0; savePrefs(); }
  if (server.hasArg("limW"))   { limW = server.arg("limW").toInt() == W_C ? W_C : W_A; limState = 0; savePrefs(); }
  if (server.hasArg("dim"))    { dimSecs = constrain(server.arg("dim").toInt(), 0, 3600); savePrefs(); }
  if (server.hasArg("apname") || server.hasArg("apdefault")) {   // the meter's own WiFi name / password
    bool def = server.hasArg("apdefault");
    String n = def ? String(DEFAULT_WIFI_NAME) : server.arg("apname");
    String p = def ? String(DEFAULT_WIFI_PASS) : server.arg("appass");
    n.trim();
    if (!def && p.length() == 0) p = apPass;                     // password left blank: keep the old one
    if (!validApName(n)) netMsg = "The name needs 1 to 32 characters (letters, numbers, spaces, symbols).";
    else if (!validApPass(p)) netMsg = "The password needs 8 to 63 characters.";
    else {
      apName = n; apPass = p; saveApWifi();
      apRestartAt = millis() + 1500;                             // after the reply has gone out
      netMsg = "Meter's WiFi is now \"" + apName + "\"";
      toast("WiFi name", apName.substring(0, 21).c_str(), 3000);
    }
  }
  if (server.hasArg("newlog")) {                                 // keep the old log in storage, then start again
    saveCurrentLog(true); archiveCurrentLog();
    newLogRequested = true; toast("New log", "Log + dose restarted", 1500);
  }
  if (server.hasArg("lockset")) {
    String p = server.arg("lockset");
    if (validPin(p)) { lockPin = p; prefs.putString("pin", lockPin); toast("Locked", "Settings locked", 1500); }
    else netMsg = "The PIN needs 4 to 8 numbers.";
  }
  if (server.hasArg("lockoff")) { lockPin = ""; prefs.putString("pin", ""); toast("Unlocked", "Lock removed", 1500); }
  if (server.hasArg("factory")) factoryReset();
  sendState();
}

void handleScan() {                        // list nearby networks for the phone
  if (!(WiFi.getMode() & WIFI_STA)) wifiMode(apOn ? WIFI_AP_STA : WIFI_STA);
  int n = WiFi.scanNetworks();
  String o = "{\"nets\":[";
  int shown = 0;
  bool used[64] = {false};
  for (int pick = 0; pick < 20; pick++) {   // strongest first, each name once
    int bi = -1;
    for (int k = 0; k < n && k < 64; k++) {
      if (used[k] || WiFi.SSID(k).length() == 0) continue;
      if (bi < 0 || WiFi.RSSI(k) > WiFi.RSSI(bi)) bi = k;
    }
    if (bi < 0) break;
    for (int k = 0; k < n && k < 64; k++) if (WiFi.SSID(k) == WiFi.SSID(bi)) used[k] = true;
    char t[24];
    o += shown ? ",{\"s\":\"" : "{\"s\":\""; o += jsonEsc(WiFi.SSID(bi));
    snprintf(t, sizeof(t), "\",\"r\":%d,\"l\":%s}", (int)WiFi.RSSI(bi), WiFi.encryptionType(bi) == WIFI_AUTH_OPEN ? "false" : "true");
    o += t; shown++;
  }
  o += "]}";
  WiFi.scanDelete();
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", o);
}

void phoneJoin(const String& ss, const String& pw, const String& ip, const String& mask, const String& gw) {
  if (!apOn) startAP(true); else wifiMode(WIFI_AP_STA);   // keep a way back in while it tries
  joinFromPhone = true; apOffAt = 0;
  tryJoin(ss, pw, ip, mask, gw);
}

void handleJoin() {                        // phone asked the meter to join a network
  if (!pinOk()) { lastDenied = true; sendState(); return; }
  String ss = server.arg("ssid"), pw = server.arg("pass");
  String ip = server.arg("ip"), mask = server.arg("mask"), gw = server.arg("gw");
  ip.trim(); mask.trim(); gw.trim();
  if (!ss.length()) { sendState(); return; }
  String bad = checkAddress(ip, mask, gw);
  if (bad.length()) { netMsg = bad; sendState(); return; }
  phoneJoin(ss, pw, ip, mask, gw);
  netMsg = "Joining " + ss + "...";
  toast("Joining", ss.c_str(), 2500);
  sendState();
}

void handleNet() {                         // forget a network, start-up preference, finish
  if (!pinOk()) { lastDenied = true; sendState(); return; }
  if (server.hasArg("forget")) forgetNet(server.arg("forget").toInt());
  if (server.hasArg("join")) { joinSaved = server.arg("join").toInt() != 0; saveNets(); }
  if (server.hasArg("finish") && netMode == NET_STA && apOn) apOffAt = millis() + 1500;
  if (server.hasArg("setip")) {            // change a saved network's address (blank ip = automatic)
    int i = server.arg("setip").toInt();
    String ip = server.arg("ip"), mask = server.arg("mask"), gw = server.arg("gw");
    ip.trim(); mask.trim(); gw.trim();
    String bad = checkAddress(ip, mask, gw);
    if (bad.length()) netMsg = bad;
    else if (i >= 0 && i < MAX_NETS && netSsid[i].length()) {
      netFixIp[i] = ip; netMask[i] = ip.length() ? mask : String(""); netGw[i] = ip.length() ? gw : String("");
      saveNets();
      netMsg = "Saved the address for " + netSsid[i] + ".";
      if ((netMode == NET_STA || netMode == NET_JOINING) && joinSsid == netSsid[i]) {   // in use now: rejoin with it
        netMsg += ip.length() ? " Rejoining at " + ip + "..." : String(" Rejoining...");
        WiFi.disconnect();
        phoneJoin(netSsid[i], netPass[i], netFixIp[i], netMask[i], netGw[i]);
      }
    }
  }
  sendState();
}

// ---------------- Alerts (ntfy.sh push to the ntfy phone app) ----------------
bool netOnline() { return netMode == NET_STA && WiFi.status() == WL_CONNECTED; }
bool validTopic(const String& t) {
  if (t.length() < 1 || t.length() > 64) return false;
  for (unsigned i = 0; i < t.length(); i++) { char c = t[i]; if (!(isalnum(c) || c == '-' || c == '_')) return false; }
  return true;
}
uint32_t rnd32() {
#ifdef ARDUINO_ARCH_ESP32
  return esp_random();
#else
  return (uint32_t)rand();
#endif
}
String newTopic() {                         // hard to guess: anyone who knows the topic can read the alerts
  const char* k = "abcdefghjkmnpqrstuvwxyz23456789";
  String t = "pocketspl-";
  for (int i = 0; i < 12; i++) t += k[rnd32() % 31];
  return t;
}
void loadAlerts() {
  alOn = prefs.getBool("alOn", false); alBack = prefs.getBool("alBack", false);
  alTopic = prefs.getString("alTopic", ""); if (!validTopic(alTopic)) { alTopic = newTopic(); prefs.putString("alTopic", alTopic); }
  alHold = prefs.getUChar("alHold", 5); alRep = prefs.getUChar("alRep", 30);
}

#ifdef ARDUINO_ARCH_ESP32
struct AlertMsg { char topic[68]; char title[64]; char body[200]; char prio[8]; };
void alertTask(void* arg) {                 // runs on its own so the meter never stalls while sending
  AlertMsg* m = (AlertMsg*)arg;
  int code = -1;
  {
    NetworkClientSecure c; c.useBuiltinCACertBundle();
    HTTPClient h; h.setTimeout(8000);
    if (h.begin(c, String("https://ntfy.sh/") + m->topic)) {
      h.addHeader("Title", m->title); h.addHeader("Priority", m->prio); h.addHeader("Tags", "loud_sound");
      code = h.POST((uint8_t*)m->body, strlen(m->body));
      h.end();
    }
  }
  delete m;
  alResult = code ? code : -1; alBusy = false;
  vTaskDelete(NULL);
}
#else
String hostAlertTitle, hostAlertBody; int hostAlertsSent = 0;   // host tests read these
#endif

bool sendAlert(const char* title, const char* body, const char* prio) {
  if (!netOnline()) { alLast = "Not sent: the meter isn't on a WiFi network with internet"; return false; }
  if (alBusy) { alLast = "Not sent: still sending the last one"; return false; }
  alBusy = true; alResult = 0;
#ifdef ARDUINO_ARCH_ESP32
  AlertMsg* m = new AlertMsg();
  strncpy(m->topic, alTopic.c_str(), sizeof(m->topic) - 1);
  strncpy(m->title, title, sizeof(m->title) - 1); strncpy(m->body, body, sizeof(m->body) - 1); strncpy(m->prio, prio, sizeof(m->prio) - 1);
  if (xTaskCreate(alertTask, "alert", 12288, m, 1, NULL) != pdPASS) { delete m; alBusy = false; alLast = "Not sent: out of memory"; return false; }
#else
  hostAlertTitle = title; hostAlertBody = body; hostAlertsSent++; alResult = 200; alBusy = false;
#endif
  return true;
}
void alertWhen(char* out, size_t n) {       // " at Fri 9:42 pm" when the clock is known
  int wm = weekMinuteNow(); char b[24];
  if (wm >= 0) { fmtWeekMinute(wm, b, sizeof(b)); snprintf(out, n, " at %s", b); } else out[0] = 0;
}
void alertTick(uint32_t now) {              // once a second
  static uint32_t last = 0;
  if (now - last < 1000) return;
  last = now;
  if (!alBusy && alResult) {                // the last send finished
    char w[32]; alertWhen(w, sizeof(w));
    alLast = alResult >= 200 && alResult < 300 ? String("Sent") + w
           : alResult == 429 ? String("Not sent: too many alerts today (ntfy limit)") : String("Not sent: no answer from ntfy.sh (") + String(alResult) + ")";
    alResult = 0;
  }
  if (!alOn || !limOn) { alOverSince = 0; alAlerted = false; return; }
  static const char* WN[] = {"LAeq", "LCeq"};
  static const char* WINS[] = {"1 min", "5 min", "15 min"};
  char title[64], body[200], w[32];
  if (limReady && limState == 2) {
    if (!alOverSince) alOverSince = now;
    bool due = now - alOverSince >= (uint32_t)alHold * 60000UL && (!alAlerted || now - alLastAt >= (uint32_t)alRep * 60000UL);
    if (due) {
      alertWhen(w, sizeof(w));
      snprintf(title, sizeof(title), "%s: over the limit", apName.c_str());
      snprintf(body, sizeof(body), "%.1f dB %s (%s), limit %d dB. Over for %lu min%s.",
               limVal, WN[limW == W_C ? 1 : 0], WINS[constrain(limWin, 0, 2)], limDb, (unsigned long)((now - alOverSince) / 60000UL), w);
      sendAlert(title, body, "high");
      alAlerted = true; alLastAt = now;
    }
  } else {
    if (alAlerted && alBack && limReady) {
      alertWhen(w, sizeof(w));
      snprintf(title, sizeof(title), "%s: back under the limit", apName.c_str());
      snprintf(body, sizeof(body), "%.1f dB %s (%s), limit %d dB%s.", limVal, WN[limW == W_C ? 1 : 0], WINS[constrain(limWin, 0, 2)], limDb, w);
      sendAlert(title, body, "default");
    }
    alOverSince = 0; alAlerted = false;
  }
}

// ---------------- Clock + on/off schedule (wired model) ----------------
// The Feather has no clock chip. The time comes from the phone (every time the page opens) and from
// the internet when the meter is on a WiFi network with internet. Once set it keeps counting for as
// long as it's powered (re-based every call, so millis() wrapping every 49.7 days doesn't matter).
const char* const DAY_NAME[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

void setClockMs(int64_t ms) { clockBaseMs = ms; clockBaseMillis = millis(); clockSet = true; }
int64_t clockNowMs() {
  uint32_t m = millis();
  clockBaseMs += (int64_t)(uint32_t)(m - clockBaseMillis); clockBaseMillis = m;
  return clockBaseMs;
}
bool validTz(const String& z) {
  if (z.length() < 3 || z.length() > 47) return false;
  for (unsigned i = 0; i < z.length(); i++) { char c = z[i]; if (!(isalnum(c) || strchr("<>+-:,./", c))) return false; }
  return true;
}
void applyTz() { setenv("TZ", tzPosix.c_str(), 1); tzset(); }
int weekMinuteNow() {                       // minutes since Sunday 00:00 local time, -1 = clock not set
  if (!clockSet) return -1;
  time_t t = (time_t)(clockNowMs() / 1000);
  struct tm lt; localtime_r(&t, &lt);
  return lt.tm_wday * 1440 + lt.tm_hour * 60 + lt.tm_min;
}
bool inSession(int i, int wm) {             // session i covers week-minute wm? (off before on = ends next morning)
  int st = schDay[i] * 1440 + schOnM[i];
  int en = schDay[i] * 1440 + schOffM[i] + (schOffM[i] <= schOnM[i] ? 1440 : 0);
  return (wm >= st && wm < en) || (wm + 10080 >= st && wm + 10080 < en);
}
bool inSchedule(int wm) { for (int i = 0; i < schN; i++) if (inSession(i, wm)) return true; return false; }
bool scheduleActive() { return wiredModel && schEnabled && schN > 0; }
int nextChange(int wm) {                    // week-minute when the on/off state next changes (-1 = never)
  bool on = inSchedule(wm);
  for (int d = 1; d <= 10080; d++) { int w = (wm + d) % 10080; if (inSchedule(w) != on) return w; }
  return -1;
}
void fmtWeekMinute(int wm, char* out, size_t n) {   // e.g. "Fri 6:30 pm"
  int h = (wm % 1440) / 60, m = wm % 60;
  snprintf(out, n, "%s %d:%02d %s", DAY_NAME[(wm / 1440) % 7], h % 12 == 0 ? 12 : h % 12, m, h < 12 ? "am" : "pm");
}
String scheduleString() {                   // "5,1110,120;6,1255,180"
  String r;
  for (int i = 0; i < schN; i++) { if (i) r += ";"; r += String(schDay[i]) + "," + String(schOnM[i]) + "," + String(schOffM[i]); }
  return r;
}
bool parseSchedule(const String& in) {     // checks it all first, then replaces the schedule
  uint8_t d[SCH_MAX]; uint16_t on[SCH_MAX], off[SCH_MAX]; int n = 0;
  const char* p = in.c_str();
  if (in.length() > 120) return false;
  while (*p) {
    const char* semi = strchr(p, ';'); size_t len = semi ? (size_t)(semi - p) : strlen(p);
    if (len) {
      char part[24]; if (len >= sizeof(part)) return false;
      memcpy(part, p, len); part[len] = 0;
      int a, b, c; char extra;
      if (n >= SCH_MAX || sscanf(part, "%d,%d,%d%c", &a, &b, &c, &extra) != 3) return false;
      if (a < 0 || a > 6 || b < 0 || b > 1439 || c < 0 || c > 1439 || b == c) return false;
      d[n] = a; on[n] = b; off[n] = c; n++;
    }
    p += len; if (*p == ';') p++;
  }
  schN = n;
  for (int i = 0; i < n; i++) { schDay[i] = d[i]; schOnM[i] = on[i]; schOffM[i] = off[i]; }
  return true;
}

#ifdef ARDUINO_ARCH_ESP32
bool ntpStarted = false; uint32_t ntpLastSet = 0;
void ntpPoll() {                            // on a network with internet: take the time from pool.ntp.org
  if (!(netMode == NET_STA && WiFi.status() == WL_CONNECTED)) return;
  if (!ntpStarted) { configTime(0, 0, "pool.ntp.org", "time.google.com"); applyTz(); ntpStarted = true; }
  time_t t = time(nullptr);
  if (t > 1700000000 && (!clockSet || millis() - ntpLastSet > 3600000UL)) { setClockMs((int64_t)t * 1000); ntpLastSet = millis(); }
}
#else
void ntpPoll() {}
#endif

void scheduleTick(uint32_t now) {           // once a second: go to standby / come back on
  static uint32_t last = 0;
  if (now - last < 1000) return;
  last = now;
  ntpPoll();
  int wm = weekMinuteNow();
  bool want = scheduleActive() && wm >= 0 && !inSchedule(wm);   // no clock = stay on
  if (want && !standby) {
    Serial.println("Schedule: standby (session saved)");
    saveCurrentLog(true); archiveCurrentLog();
    newLogRequested = true; standby = true; limState = 0;
  } else if (!want && standby) {
    Serial.println("Schedule: on");
    standby = false; newLogRequested = true;
    wake(now); toast("On", "Schedule", 2500);
  }
}

void handleTime() {                        // the phone tells the meter the time (it has no clock of its own)
  int64_t t = atoll(server.arg("t").c_str());
  tzOffsetMin = server.arg("tz").toInt();
  if (t > 1600000000000LL) setClockMs(t);
  if (server.hasArg("tzp")) {
    String z = server.arg("tzp");
    if (validTz(z) && z != tzPosix) { tzPosix = z; prefs.putString("tzp", tzPosix); applyTz(); }
  }
  if (t > 1600000000000LL && logStartEpochMs == 0) logStartEpochMs = t - (int64_t)logSecs * 1000;
  server.send(200, "application/json", "{}");
}

// copy the log out under the lock, oldest first (up to 24 h)
double curMinMs = 0; int curMinSecs = 0;   // the minute that's still building (copied with the log)
int copyLog(LogRec* out, uint32_t& first, Marker* mk, int& nmk) {
  portENTER_CRITICAL(&audioLock);
  curMinSecs = minSecs; curMinMs = minSecs ? minSum[W_A] / minSecs : 0;
  uint32_t total = logTotal;
  int kept = total < (uint32_t)LOG_MAX ? (int)total : LOG_MAX;
  first = total - kept;
  for (int i = 0; i < kept; i++) out[i] = logRec[(first + i) % LOG_MAX];
  nmk = markCount;
  for (int i = 0; i < nmk; i++) mk[i] = marks[i];
  portEXIT_CRITICAL(&audioLock);
  return kept;
}
LogRec logCopy[LOG_MAX];
Marker markCopy[MARK_MAX];

void handleLogJson() {                     // for the chart on the phone
  uint32_t first; int nmk;
  int kept = copyLog(logCopy, first, markCopy, nmk);
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", "");
  char b[64];
  snprintf(b, sizeof(b), "{\"first\":%lu,\"laeq\":[", (unsigned long)first);
  String chunk = b;
  for (int i = 0; i < kept; i++) {
    snprintf(b, sizeof(b), "%s%d", i ? "," : "", logCopy[i].laeq);
    chunk += b;
    if (chunk.length() > 1200) { server.sendContent(chunk); chunk = ""; }
  }
  if (curMinSecs) { snprintf(b, sizeof(b), "],\"cur\":%d,\"curSecs\":%d", (int)lround(10.0 * toDb(curMinMs)), curMinSecs); chunk += b; }
  else chunk += "],\"cur\":null,\"curSecs\":0";
  chunk += ",\"marks\":[";
  for (int i = 0; i < nmk; i++) {
    snprintf(b, sizeof(b), "%s[%lu,\"", i ? "," : "", (unsigned long)markCopy[i].sec);
    chunk += b; chunk += jsonEsc(String(markCopy[i].text)); chunk += "\"]";
  }
  chunk += "]}";
  server.sendContent(chunk);
  server.sendContent("");
}

static void csvField(String& o, const char* t) {   // quote text so commas can't break the columns
  o += '"';
  for (const char* p = t; *p; p++) { if (*p == '"') o += '"'; o += *p; }
  o += '"';
}

// Sends logCopy / markCopy as a spreadsheet. liveMarks: also list markers in the minute that's still building.
void sendLogCsv(const char* filename, int64_t t0, uint32_t first, int kept, int nmk, bool liveMarks) {
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.sendHeader("Content-Disposition", String("attachment; filename=\"") + filename + "\"");
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "text/csv", "");
  String o = "Minute ending,Elapsed (min),LAeq 1 min (dB),LCeq 1 min (dB),LAFmax (dB),LCpeak (dB),Marker\r\n";
  char b[96];
  for (int i = 0; i < kept; i++) {
    uint32_t m = first + i;                  // minute number since the log started (0 = first)
    if (t0) {
      time_t t = (time_t)((t0 + (int64_t)(m + 1) * 60000 + (int64_t)tzOffsetMin * 60000) / 1000);
      struct tm tmv; gmtime_r(&t, &tmv);
      strftime(b, sizeof(b), "%Y-%m-%d %H:%M", &tmv);
      o += b;
    }
    const LogRec& r = logCopy[i];
    snprintf(b, sizeof(b), ",%lu,%.1f,%.1f,%.1f,%.1f,", (unsigned long)(m + 1), r.laeq / 10.0, r.lceq / 10.0, r.lafmax / 10.0, r.lcpeak / 10.0);
    o += b;
    String mt;
    for (int k = 0; k < nmk; k++) if (markCopy[k].sec / 60 == m) { if (mt.length()) mt += "; "; mt += markCopy[k].text; }
    if (mt.length()) csvField(o, mt.c_str());
    o += "\r\n";
    if (o.length() > 1200) { server.sendContent(o); o = ""; }
  }
  for (int k = 0; k < nmk; k++) {           // markers after the last full minute
    uint32_t m = markCopy[k].sec / 60;
    if (m < first + kept) continue;
    if (!liveMarks && kept) continue;
    if (t0) {
      time_t t = (time_t)((t0 + (int64_t)markCopy[k].sec * 1000 + (int64_t)tzOffsetMin * 60000) / 1000);
      struct tm tmv; gmtime_r(&t, &tmv);
      strftime(b, sizeof(b), "%Y-%m-%d %H:%M", &tmv);
      o += b;
    }
    snprintf(b, sizeof(b), ",%lu,,,,,", (unsigned long)(m + 1));
    o += b; csvField(o, markCopy[k].text); o += "\r\n";
  }
  if (!kept && !nmk) o += "No full minutes logged yet\r\n";
  server.sendContent(o);
  server.sendContent("");
}

void handleLogCsv() {                      // the gig log as a spreadsheet
  uint32_t first; int nmk;
  int kept = copyLog(logCopy, first, markCopy, nmk);
  String fn = "PocketSPL-" + serialNo + "-log.csv";
  sendLogCsv(fn.c_str(), logStartEpochMs, first, kept, nmk, true);
}

// ---------------- Saved gig logs (kept in the Feather's flash, so they survive switch-off) ----------------
// The log that's running is saved every minute to /cur.bin. At switch-on, or when the gig is reset, it's
// kept as /logN.bin. The newest SAVED_LOGS are kept.
const int SAVED_LOGS = 30;                 // the oldest go first when storage is full (about 5 on a 4MB board)
const uint32_t LOG_MAGIC = 0x314C5350;      // "PSL1"
struct LogFileHdr { uint32_t magic, first, kept, logSecs; int64_t t0; uint16_t nmk, pad; };
bool readLogHdr(uint32_t id, LogFileHdr& h, bool loadData);
int listLogs(uint32_t* ids, int maxN);
void makeRoom(size_t need);
uint32_t savedTotal = 0xFFFFFFFF; int savedMarks = -1; int64_t savedT0 = -1;
uint32_t lastLogSave = 0;

void saveCurrentLog(bool force) {
  if (!fsOk) return;
  portENTER_CRITICAL(&audioLock); uint32_t tot = logTotal; int nm = markCount; portEXIT_CRITICAL(&audioLock);
  if (!force && tot == savedTotal && nm == savedMarks && logStartEpochMs == savedT0) return;
  savedTotal = tot; savedMarks = nm; savedT0 = logStartEpochMs;
  if (tot == 0 && nm == 0) return;
  uint32_t first; int nmk;
  int kept = copyLog(logCopy, first, markCopy, nmk);
  LogFileHdr h = {LOG_MAGIC, first, (uint32_t)kept, (uint32_t)logSecs, logStartEpochMs, (uint16_t)nmk, 0};
  size_t bytes = sizeof(h) + sizeof(LogRec) * kept + sizeof(Marker) * nmk;
  makeRoom(2 * bytes + 12288);              // room for this file twice over (old + new copy) plus spare
  File fl = LittleFS.open("/cur.tmp", FILE_WRITE);
  if (!fl) return;
  bool good = fl.write((const uint8_t*)&h, sizeof(h)) == sizeof(h)
           && fl.write((const uint8_t*)logCopy, sizeof(LogRec) * kept) == sizeof(LogRec) * kept
           && fl.write((const uint8_t*)markCopy, sizeof(Marker) * nmk) == sizeof(Marker) * nmk;
  fl.close();
  if (!good) { LittleFS.remove("/cur.tmp"); return; }
  LittleFS.remove("/cur.bin");
  LittleFS.rename("/cur.tmp", "/cur.bin");
}

int listLogs(uint32_t* ids, int maxN) {     // saved log numbers, newest first
  int n = 0;
  File dir = LittleFS.open("/");
  if (!dir) return 0;
  for (File e = dir.openNextFile(); e; e = dir.openNextFile()) {
    String nm = e.name();
    if (nm.startsWith("/")) nm = nm.substring(1);
    unsigned long id;
    if (sscanf(nm.c_str(), "log%lu.bin", &id) == 1 && n < maxN) ids[n++] = id;
  }
  for (int i = 1; i < n; i++) for (int j = i; j > 0 && ids[j] > ids[j - 1]; j--) { uint32_t x = ids[j]; ids[j] = ids[j - 1]; ids[j - 1] = x; }
  return n;
}

void makeRoom(size_t need) {                // delete the oldest saved gigs until there's enough free space
  uint32_t ids[40]; int n = listLogs(ids, 40);
  while (n > 0 && LittleFS.totalBytes() - LittleFS.usedBytes() < need) {
    char p[24]; snprintf(p, sizeof(p), "/log%lu.bin", (unsigned long)ids[--n]); LittleFS.remove(p);
  }
}

// Wired model, running nonstop in a venue: when the night's over (an hour or more of 70 dB(A)+, then
// 2 hours under 60 dB(A)) that night is saved as its own log and a fresh log starts. Checked once a minute.
uint32_t nightCheckedAt = 0xFFFFFFFF;
void checkNightOver() {
  portENTER_CRITICAL(&audioLock); uint32_t tot = logTotal; portEXIT_CRITICAL(&audioLock);
  if (tot == nightCheckedAt || newLogRequested) return;
  nightCheckedAt = tot;
  if (tot < (uint32_t)(NIGHT_LOUD_MIN + NIGHT_QUIET_MIN)) return;
  uint32_t first; int nmk;
  int kept = copyLog(logCopy, first, markCopy, nmk);
  if (kept < NIGHT_LOUD_MIN + NIGHT_QUIET_MIN) return;
  for (int i = kept - NIGHT_QUIET_MIN; i < kept; i++) if (logCopy[i].laeq >= NIGHT_QUIET_DB10) return;   // not quiet yet
  int loud = 0;
  for (int i = 0; i < kept - NIGHT_QUIET_MIN; i++) if (logCopy[i].laeq >= NIGHT_LOUD_DB10) loud++;
  if (loud < NIGHT_LOUD_MIN) return;                                                                    // no show in this log
  Serial.println("Night over: saving it and starting a new log");
  saveCurrentLog(true); archiveCurrentLog();
  newLogRequested = true;
}

void archiveCurrentLog() {                 // the running log becomes a saved log
  if (!fsOk || !LittleFS.exists("/cur.bin")) return;
  uint32_t seq = prefs.getUInt("lseq", 0) + 1; prefs.putUInt("lseq", seq);
  char p[24]; snprintf(p, sizeof(p), "/log%lu.bin", (unsigned long)seq);
  LittleFS.rename("/cur.bin", p);
  uint32_t ids[40]; int n = listLogs(ids, 40);
  for (int i = SAVED_LOGS; i < n; i++) { snprintf(p, sizeof(p), "/log%lu.bin", (unsigned long)ids[i]); LittleFS.remove(p); }
  savedTotal = 0xFFFFFFFF; savedMarks = -1; savedT0 = -1;
}

bool readLogHdr(uint32_t id, LogFileHdr& h, bool loadData) {
  char p[24]; snprintf(p, sizeof(p), "/log%lu.bin", (unsigned long)id);
  File fl = LittleFS.open(p, FILE_READ);
  if (!fl) return false;
  bool good = fl.read((uint8_t*)&h, sizeof(h)) == sizeof(h) && h.magic == LOG_MAGIC && h.kept <= (uint32_t)LOG_MAX && h.nmk <= MARK_MAX;
  if (good && loadData)
    good = fl.read((uint8_t*)logCopy, sizeof(LogRec) * h.kept) == sizeof(LogRec) * h.kept
        && fl.read((uint8_t*)markCopy, sizeof(Marker) * h.nmk) == sizeof(Marker) * h.nmk;
  fl.close();
  return good;
}

void handleLogs() {                        // list saved logs, or delete one (?del=N)
  if (server.hasArg("del")) {
    if (!pinOk()) { server.send(200, "application/json", "{\"denied\":true}"); return; }
    char p[24]; snprintf(p, sizeof(p), "/log%lu.bin", (unsigned long)server.arg("del").toInt());
    if (fsOk) LittleFS.remove(p);
  }
  String s = "{\"fs\":"; s += fsOk ? "true" : "false"; s += ",\"logs\":[";
  uint32_t ids[40]; int n = fsOk ? listLogs(ids, 40) : 0;
  bool firstOut = true;
  for (int i = 0; i < n; i++) {
    LogFileHdr h;
    if (!readLogHdr(ids[i], h, false)) continue;
    char b[120];
    snprintf(b, sizeof(b), "%s{\"id\":%lu,\"mins\":%lu,\"secs\":%lu,\"t0\":%lld,\"marks\":%u}", firstOut ? "" : ",",
             (unsigned long)ids[i], (unsigned long)(h.first + h.kept), (unsigned long)h.logSecs, (long long)h.t0, (unsigned)h.nmk);
    s += b; firstOut = false;
  }
  s += "]}";
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", s);
}

void handleOldLogCsv() {                   // a saved log as a spreadsheet
  LogFileHdr h;
  uint32_t id = server.arg("id").toInt();
  if (!fsOk || !readLogHdr(id, h, true)) { server.send(404, "text/plain", "That log isn't there any more."); return; }
  char fn[48]; snprintf(fn, sizeof(fn), "PocketSPL-%s-log-%lu.csv", serialNo.c_str(), (unsigned long)id);
  sendLogCsv(fn, h.t0, h.first, (int)h.kept, h.nmk, true);
}

// ---------------- Factory reset ----------------
// Clears every setting, saved network, the WiFi name, the lock and saved logs.
// Keeps this unit's factory calibration and serial number, then restarts.
void factoryReset() {
  String sn = prefs.getString("sn", "");
  bool wired = prefs.getBool("wired", false);
  uint32_t lseq = prefs.getUInt("lseq", 0);
  prefs.clear();
  if (factoryCalSet) { prefs.putFloat("fcal", factoryCal); prefs.putBool("fcalok", true); prefs.putFloat("cal", factoryCal); }
  if (sn.length()) prefs.putString("sn", sn);
  if (wired) prefs.putBool("wired", true);
  prefs.putUInt("lseq", lseq);
  if (fsOk) {
    uint32_t ids[40]; int n = listLogs(ids, 40);
    char p[24];
    for (int i = 0; i < n; i++) { snprintf(p, sizeof(p), "/log%lu.bin", (unsigned long)ids[i]); LittleFS.remove(p); }
    LittleFS.remove("/cur.bin"); LittleFS.remove("/cur.tmp");
  }
  fsOk = false;                             // nothing more gets saved before the restart
  drawUpdateScreen("Reset", "Restarting", -1, C_ACCENT);
  restartAt = millis() + 1500;
}

void startServer() {
  if (!serverStarted) {
    server.on("/", []() { server.send_P(200, "text/html", PAGE); });
    server.on("/api/state", sendState);
    server.on("/api/set", handleSet);
    server.on("/api/scan", handleScan);
    server.on("/api/join", handleJoin);
    server.on("/api/net", handleNet);
    server.on("/api/time", handleTime);
    server.on("/api/log.json", handleLogJson);
    server.on("/api/log.csv", handleLogCsv);
    server.on("/api/update", HTTP_POST, handleUpdateDone, handleUpdateUpload);
    server.on("/api/logs", handleLogs);
    server.on("/api/oldlog.csv", handleOldLogCsv);
    server.onNotFound([]() { server.sendHeader("Location", "/"); server.send(302, "text/plain", ""); });
    serverStarted = true;
  }
  server.begin();
}

void startMdns() {                         // lets an iPhone use http://pocketspl.local (name follows the meter's WiFi name)
  MDNS.end();
  if (MDNS.begin(mdnsName().c_str())) MDNS.addService("http", "tcp", 80);
}

void wifiStart() {
  wifiOn = true; staLostAt = 0; apOffAt = 0; joinFromPhone = false;
  WiFi.persistent(false);                  // passwords live in our own settings, not the WiFi chip's
  if (!beginJoinSaved()) { startAP(false); netMode = NET_AP; }
  startServer();
}

void wifiStop() {
  server.stop();
  MDNS.end();
  WiFi.disconnect(true);
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  wifiOn = false; apOn = false; netMode = NET_OFF;
}

void netService(uint32_t now) {            // runs every loop: joining, losing and getting back the network
  if (!wifiOn) return;
  bool up = WiFi.status() == WL_CONNECTED;
  if (netMode == NET_JOINING) {
    if (up) {
      netMode = NET_STA; staLostAt = 0;
      if (joinFromPhone) { rememberNet(joinSsid, joinPass, joinIp, joinMask, joinGw); apOffAt = now + AP_LINGER_MS; }
      else if (apOn) apOffAt = now + AP_LINGER_MS;
      startMdns();
      netMsg = "Joined " + joinSsid;
      toast("Joined", WiFi.localIP().toString().c_str(), 5000);
      Serial.printf("Joined %s, open http://%s\n", joinSsid.c_str(), WiFi.localIP().toString().c_str());
    } else if (now - joinT0 > JOIN_TIMEOUT_MS) {
      if (joinFromPhone) {                 // wrong password or out of range: stay on the meter's own network
        netMsg = "Couldn't join " + joinSsid + ". Check the password and that it's in range." +
                 (joinIp.length() ? String(" With a fixed address, also check the address and subnet mask.") : String(""));
        joinFromPhone = false; WiFi.disconnect(); netMode = NET_AP;
        toast("Not joined", "Check password", 2500);
      } else if (++joinPos < joinCount) {
        tryJoinSaved(joinOrder[joinPos]);
        netMsg = "Joining " + joinSsid + "...";
      } else {
        WiFi.disconnect(); startAP(false); netMode = NET_AP;
        netMsg = "Couldn't join a saved network, using the meter's own WiFi";
        toast("WiFi on", apName.substring(0, 21).c_str(), 2500);
      }
    }
  } else if (netMode == NET_STA) {
    if (!up) {
      if (!staLostAt) staLostAt = now;
      if (!apOn && now - staLostAt > NET_LOST_MS) {    // network gone: open the meter's own as well
        startAP(true); apOffAt = 0;
        netMsg = "Lost " + joinSsid + ", meter's own WiFi is on too";
        toast("Network lost", (apName.substring(0, 18) + " on").c_str(), 2500);
      }
    } else {
      if (staLostAt && apOn) apOffAt = now + AP_LINGER_MS;   // back again: close our own network later
      staLostAt = 0;
      if (apOffAt && (int32_t)(now - apOffAt) >= 0) { apOffAt = 0; joinFromPhone = false; stopAP(); netMsg = "Joined " + joinSsid; }
    }
  }
}

// ---------------- Audio task ----------------
void i2sInit() {
  i2s_config_t cfg = {};
  cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);
  cfg.sample_rate = FS;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT;
  cfg.channel_format = MIC_CHANNEL_FMT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  cfg.dma_buf_count = 8;
  cfg.dma_buf_len = BUF_LEN;
  cfg.use_apll = false;

  i2s_pin_config_t pins = {};
  pins.mck_io_num = I2S_PIN_NO_CHANGE;
  pins.bck_io_num = PIN_I2S_BCLK;
  pins.ws_io_num = PIN_I2S_WS;
  pins.data_out_num = I2S_PIN_NO_CHANGE;
  pins.data_in_num = PIN_I2S_DIN;

  i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pins);
  i2s_zero_dma_buffer(I2S_NUM_0);
}

// called by the audio task (inside audioLock) for every block after warm-up
void gigReset() {
  for (int k = 0; k < 3; k++) for (int w = 0; w < 3; w++) rollSum[k][w] = 0;
  rollHead = 0; rollCount = 0;
  for (int w = 0; w < 3; w++) { secSum[w] = 0; minSum[w] = 0; }
  secN = 0; minSecs = 0; minMaxFA = 0; minPeakC = 0;
  doseE = 0; doseSecs = 0; logTotal = 0; logSecs = 0; markCount = 0;
  logStartEpochMs = 0;
}
void gigSecond(const float ms[3]) {        // one finished second
  for (int k = 0; k < 3; k++)
    if (rollCount >= (uint32_t)ROLL_W[k]) {
      int old = (rollHead - ROLL_W[k] + ROLL_SECS) % ROLL_SECS;
      for (int w = 0; w < 3; w++) rollSum[k][w] -= rollSec[old][w];
    }
  for (int w = 0; w < 3; w++) { rollSec[rollHead][w] = ms[w]; for (int k = 0; k < 3; k++) rollSum[k][w] += ms[w]; }
  rollHead = (rollHead + 1) % ROLL_SECS; rollCount++;
  if (rollCount % 600 == 0)                 // every 10 min, re-add from scratch so rounding can't creep in
    for (int k = 0; k < 3; k++) {
      uint32_t n = rollCount < (uint32_t)ROLL_W[k] ? rollCount : (uint32_t)ROLL_W[k];
      for (int w = 0; w < 3; w++) {
        double t = 0;
        for (uint32_t j = 1; j <= n; j++) t += rollSec[(rollHead - (int)j + ROLL_SECS) % ROLL_SECS][w];
        rollSum[k][w] = t;
      }
    }
  for (int w = 0; w < 3; w++) minSum[w] += ms[w];
  logSecs++;
  if (++minSecs >= 60) {                    // one finished minute -> log record
    LogRec r;
    auto d10 = [](double v) { return (int16_t)lround(10.0 * (v < 1e-20 ? -77.0 : 10.0 * log10(v) + MIC_REF_DB + calOffset)); };
    r.laeq = d10(minSum[W_A] / 60); r.lceq = d10(minSum[W_C] / 60); r.lafmax = d10(minMaxFA);
    r.lcpeak = (int16_t)lround(10.0 * (minPeakC > 0 ? 20.0 * log10(minPeakC) + MIC_REF_DB + calOffset : -77.0));
    logRec[logTotal % LOG_MAX] = r; logTotal++;
    for (int w = 0; w < 3; w++) minSum[w] = 0;
    minSecs = 0; minMaxFA = 0; minPeakC = 0;
  }
}
void gigBlock(const float blockSum[3], int n, float peakC) {
  if (newLogRequested) { gigReset(); newLogRequested = false; }
  if (standby) return;                     // outside the on/off schedule: nothing is logged
  for (int w = 0; w < 3; w++) secSum[w] += blockSum[w];
  secN += n;
  doseE += blockSum[W_A] / FS; doseSecs += (double)n / FS;
  if (msqF[W_A] > minMaxFA) minMaxFA = msqF[W_A];
  if (peakC > minPeakC) minPeakC = peakC;
  if (secN >= (uint32_t)FS) {
    float ms[3];
    for (int w = 0; w < 3; w++) { ms[w] = (float)(secSum[w] / secN); secSum[w] = 0; }
    secN = 0;
    gigSecond(ms);
  }
}

void processAudio() {
  size_t bytes = 0;
  i2s_read(I2S_NUM_0, i2sBuf, sizeof(i2sBuf), &bytes, portMAX_DELAY);
  int n = bytes / 4;
  if (n <= 0) return;

  float blockSum[3] = {0, 0, 0};
  float blockRaw = 0, blockPeakC = 0, rawPk = 0, rawMin = 2, rawMax = -2;
  float f[3], s[3];
  for (int w = 0; w < 3; w++) { f[w] = msqF[w]; s[w] = msqS[w]; }
  for (int i = 0; i < n; i++) {
    float x = (float)(i2sBuf[i] >> 8) * (1.0f / 8388608.0f);   // 24-bit sample
    float ax = fabsf(x); if (ax > rawPk) rawPk = ax;
    if (x < rawMin) rawMin = x; if (x > rawMax) rawMax = x;
    float d = x - dcX + DC_R * dcY; dcX = x; dcY = d;            // DC blocker
    blockRaw += d * d;
    float e = d;
    if (USE_MIC_EQ) { e = micEq[0].process(e); e = micEq[1].process(e); }
    rtaPush(e);
    float a = e; for (int k = 0; k < 3; k++) a = filtA[k].process(a);
    float c = e; for (int k = 0; k < 2; k++) c = filtC[k].process(c);
    float s2[3] = {a * a, c * c, e * e};
    float ac = fabsf(c); if (ac > blockPeakC) blockPeakC = ac;
    for (int w = 0; w < 3; w++) {
      f[w] += aFast * (s2[w] - f[w]);
      s[w] += aSlow * (s2[w] - s[w]);
      blockSum[w] += s2[w];
    }
  }

  if (rawPk >= 0.99f && warmup <= 0) { uint32_t m = millis(); clipAt = m ? m : 1; }   // mic at full scale: peaks are being cut off
  if (rawMax - rawMin < 6e-7f) { if (!micFlatSince) { uint32_t m = millis(); micFlatSince = m ? m : 1; } }   // no signal at all
  else micFlatSince = 0;
  portENTER_CRITICAL(&audioLock);
  for (int w = 0; w < 3; w++) { msqF[w] = f[w]; msqS[w] = s[w]; }
  if (warmup > 0) { warmup -= n; portEXIT_CRITICAL(&audioLock); return; }
  if (seedAverages) {
    for (int w = 0; w < 3; w++) msqF[w] = msqS[w] = blockSum[w] / n;
    seedAverages = false; resetRequested = true;
  }
  if (resetRequested) {
    for (int w = 0; w < 3; w++) { leqSum[w] = 0; maxF[w] = 0; }
    leqN = 0; resetRequested = false;
  }
  for (int w = 0; w < 3; w++) {
    leqSum[w] += blockSum[w];
    if (msqF[w] > maxF[w]) maxF[w] = msqF[w];
  }
  leqN += n;
  if (calState == CAL_MEASURE) { calSum += blockRaw; calN += n; }
  gigBlock(blockSum, n, blockPeakC);
  portEXIT_CRITICAL(&audioLock);
}

void audioTask(void*) {
  for (;;) processAudio();
}

// ---------------- Screens ----------------

bool battLow() { return !wiredModel && battPct < BATT_LOW_PCT && !charging; }   // wired model has no battery
// Battery colour: charging = green (red while still under 20%), otherwise grey, amber under 30%, red when low
uint16_t batteryColour() {
  if (charging) return battPct < BATT_CHARGE_RED_PCT ? C_RED : C_GREEN;
  if (battPct < BATT_LOW_PCT) return C_RED;
  return battPct < 30 ? C_AMBER : C_DIM;
}
void drawBolt(int x, int y, uint16_t col) {            // lightning bolt, 6 x 10 px
  canvas.fillTriangle(x + 4, y, x, y + 6, x + 4, y + 5, col);
  canvas.fillTriangle(x + 2, y + 4, x + 6, y + 4, x + 1, y + 10, col);
}
void drawBatteryIcon(int x, int y, int pct, uint16_t col) {
  canvas.drawRect(x, y, 20, 10, col);
  canvas.fillRect(x + 20, y + 3, 2, 4, col);
  int w = constrain(pct, 0, 100) * 16 / 100;
  if (w > 0) canvas.fillRect(x + 2, y + 2, w, 6, col);
}

void drawWifiIcon(int cx, int y, uint16_t col) {      // dot + two arcs, 15 px wide
  canvas.fillCircle(cx, y + 9, 1, col);
  canvas.drawCircleHelper(cx, y + 9, 4, 0x3, col);
  canvas.drawCircleHelper(cx, y + 9, 7, 0x3, col);
}

// ---- anti-aliased Inter, pre-rendered (8-bit coverage). Generated. ----
struct AAGlyph { uint8_t cp, w, h, adv; int8_t dx, dy; uint16_t off; };
struct AAFont { uint8_t count; const AAGlyph* g; const uint8_t* bmp; };
// F54: Inter-Bold.otf 54px, 12 glyphs, 13819 bytes
const uint8_t F54_bmp[] PROGMEM = {
0,0,0,0,0,0,0,0,0,0,0,0,4,69,149,187,212,237,246,222,196,169,100,21,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,19,148,237,255,255,255,255,255,255,255,255,255,255,252,190,60,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,98,236,255,255,255,255,255,255,255,255,255,255,255,255,255,255,254,158,20,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,161,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,225,34,0,0,0,0,0,0,0,0,0,0,0,0,0,2,161,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,226,35,0,0,0,0,0,0,0,0,0,0,0,0,137,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,223,12,0,0,0,0,0,0,0,0,0,0,50,252,255,255,255,255,255,255,255,255,255,230,190,179,211,254,255,255,255,
255,255,255,255,255,255,145,0,0,0,0,0,0,0,0,0,3,206,255,255,255,255,255,255,255,255,216,75,1,0,0,0,35,159,255,255,255,255,255,255,
255,255,253,54,0,0,0,0,0,0,0,0,88,255,255,255,255,255,255,255,255,193,15,0,0,0,0,0,0,0,109,255,255,255,255,255,255,255,255,186,
0,0,0,0,0,0,0,0,188,255,255,255,255,255,255,255,232,23,0,0,0,0,0,0,0,0,0,154,255,255,255,255,255,255,255,254,32,0,0,0,
0,0,0,33,254,255,255,255,255,255,255,255,101,0,0,0,0,0,0,0,0,0,0,14,241,255,255,255,255,255,255,255,129,0,0,0,0,0,0,122,
255,255,255,255,255,255,255,233,7,0,0,0,0,0,0,0,0,0,0,0,138,255,255,255,255,255,255,255,221,0,0,0,0,0,0,179,255,255,255,255,
255,255,255,152,0,0,0,0,0,0,0,0,0,0,0,0,51,255,255,255,255,255,255,255,255,21,0,0,0,0,0,232,255,255,255,255,255,255,255,81,
0,0,0,0,0,0,0,0,0,0,0,0,1,234,255,255,255,255,255,255,255,74,0,0,0,0,29,255,255,255,255,255,255,255,255,25,0,0,0,0,
0,0,0,0,0,0,0,0,0,180,255,255,255,255,255,255,255,127,0,0,0,0,71,255,255,255,255,255,255,255,244,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,142,255,255,255,255,255,255,255,169,0,0,0,0,88,255,255,255,255,255,255,255,209,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,107,255,255,255,255,255,255,255,185,0,0,0,0,104,255,255,255,255,255,255,255,193,0,0,0,0,0,0,0,0,0,0,0,0,0,0,91,255,255,
255,255,255,255,255,202,0,0,0,0,119,255,255,255,255,255,255,255,182,0,0,0,0,0,0,0,0,0,0,0,0,0,0,80,255,255,255,255,255,255,
255,218,0,0,0,0,132,255,255,255,255,255,255,255,174,0,0,0,0,0,0,0,0,0,0,0,0,0,0,71,255,255,255,255,255,255,255,231,0,0,
0,0,120,255,255,255,255,255,255,255,181,0,0,0,0,0,0,0,0,0,0,0,0,0,0,79,255,255,255,255,255,255,255,219,0,0,0,0,105,255,
255,255,255,255,255,255,192,0,0,0,0,0,0,0,0,0,0,0,0,0,0,90,255,255,255,255,255,255,255,203,0,0,0,0,89,255,255,255,255,255,
255,255,207,0,0,0,0,0,0,0,0,0,0,0,0,0,0,105,255,255,255,255,255,255,255,187,0,0,0,0,73,255,255,255,255,255,255,255,241,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,140,255,255,255,255,255,255,255,171,0,0,0,0,33,255,255,255,255,255,255,255,255,22,0,0,0,0,
0,0,0,0,0,0,0,0,0,177,255,255,255,255,255,255,255,131,0,0,0,0,0,236,255,255,255,255,255,255,255,75,0,0,0,0,0,0,0,0,
0,0,0,0,0,231,255,255,255,255,255,255,255,79,0,0,0,0,0,184,255,255,255,255,255,255,255,146,0,0,0,0,0,0,0,0,0,0,0,0,
47,255,255,255,255,255,255,255,255,26,0,0,0,0,0,128,255,255,255,255,255,255,255,227,5,0,0,0,0,0,0,0,0,0,0,0,134,255,255,255,
255,255,255,255,228,0,0,0,0,0,0,40,255,255,255,255,255,255,255,255,93,0,0,0,0,0,0,0,0,0,0,12,239,255,255,255,255,255,255,255,
139,0,0,0,0,0,0,0,196,255,255,255,255,255,255,255,227,19,0,0,0,0,0,0,0,0,0,149,255,255,255,255,255,255,255,255,40,0,0,0,
0,0,0,0,96,255,255,255,255,255,255,255,255,186,12,0,0,0,0,0,0,0,103,255,255,255,255,255,255,255,255,196,0,0,0,0,0,0,0,0,
6,215,255,255,255,255,255,255,255,255,211,69,1,0,0,0,33,155,255,255,255,255,255,255,255,255,255,67,0,0,0,0,0,0,0,0,0,60,254,255,
255,255,255,255,255,255,255,255,228,188,178,209,253,255,255,255,255,255,255,255,255,255,159,0,0,0,0,0,0,0,0,0,0,0,150,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,232,18,0,0,0,0,0,0,0,0,0,0,0,5,173,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,234,45,0,0,0,0,0,0,0,0,0,0,0,0,0,4,171,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,233,43,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,109,240,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,171,28,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,23,155,241,255,255,255,255,255,255,255,255,255,255,254,198,69,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,6,74,153,190,215,239,247,224,199,173,106,26,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,7,142,254,255,255,255,255,255,255,255,187,0,0,0,0,0,0,0,0,0,0,0,47,211,255,255,255,
255,255,255,255,255,255,187,0,0,0,0,0,0,0,0,0,2,116,249,255,255,255,255,255,255,255,255,255,255,187,0,0,0,0,0,0,0,0,29,191,
255,255,255,255,255,255,255,255,255,255,255,255,187,0,0,0,0,0,0,0,89,239,255,255,255,255,255,255,255,255,255,255,255,255,255,187,0,0,0,0,
0,16,166,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,187,0,0,0,0,0,142,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,187,0,0,0,0,0,143,255,255,255,255,255,255,255,190,110,255,255,255,255,255,255,255,187,0,0,0,0,0,143,255,255,255,255,255,248,115,1,75,
255,255,255,255,255,255,255,187,0,0,0,0,0,143,255,255,255,255,209,45,0,0,75,255,255,255,255,255,255,255,187,0,0,0,0,0,143,255,255,254,
140,7,0,0,0,75,255,255,255,255,255,255,255,187,0,0,0,0,0,143,255,226,65,0,0,0,0,0,75,255,255,255,255,255,255,255,187,0,0,0,
0,0,143,166,16,0,0,0,0,0,0,75,255,255,255,255,255,255,255,187,0,0,0,0,0,27,0,0,0,0,0,0,0,0,75,255,255,255,255,255,
255,255,187,0,0,0,0,0,0,0,0,0,0,0,0,0,0,75,255,255,255,255,255,255,255,187,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
75,255,255,255,255,255,255,255,187,0,0,0,0,0,0,0,0,0,0,0,0,0,0,75,255,255,255,255,255,255,255,187,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,75,255,255,255,255,255,255,255,187,0,0,0,0,0,0,0,0,0,0,0,0,0,0,75,255,255,255,255,255,255,255,187,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,75,255,255,255,255,255,255,255,187,0,0,0,0,0,0,0,0,0,0,0,0,0,0,75,255,255,255,255,
255,255,255,187,0,0,0,0,0,0,0,0,0,0,0,0,0,0,75,255,255,255,255,255,255,255,187,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,75,255,255,255,255,255,255,255,187,0,0,0,0,0,0,0,0,0,0,0,0,0,0,75,255,255,255,255,255,255,255,187,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,75,255,255,255,255,255,255,255,187,0,0,0,0,0,0,0,0,0,0,0,0,0,0,75,255,255,255,255,255,255,255,187,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,75,255,255,255,255,255,255,255,187,0,0,0,0,0,0,0,0,0,0,0,0,0,0,75,255,255,255,
255,255,255,255,187,0,0,0,0,0,0,0,0,0,0,0,0,0,0,75,255,255,255,255,255,255,255,187,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,75,255,255,255,255,255,255,255,187,0,0,0,0,0,0,0,0,0,0,0,0,0,0,75,255,255,255,255,255,255,255,187,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,75,255,255,255,255,255,255,255,187,0,0,0,0,0,0,0,0,0,0,0,0,0,0,75,255,255,255,255,255,255,255,187,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,75,255,255,255,255,255,255,255,187,0,0,0,0,0,0,0,0,0,0,0,0,0,0,75,255,255,
255,255,255,255,255,187,0,0,0,0,0,0,0,0,0,0,0,0,0,0,75,255,255,255,255,255,255,255,187,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,75,255,255,255,255,255,255,255,187,0,0,0,0,0,0,0,0,0,0,0,0,0,0,75,255,255,255,255,255,255,255,187,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,75,255,255,255,255,255,255,255,187,0,0,0,0,0,0,0,0,0,0,0,0,0,1,53,116,179,211,231,250,239,220,
199,147,86,22,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,6,98,214,255,255,255,255,255,255,255,255,255,255,255,249,163,52,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,46,213,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,254,149,11,0,0,0,0,
0,0,0,0,0,0,0,0,0,89,248,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,217,27,0,0,0,0,0,0,0,0,0,
0,0,72,250,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,216,22,0,0,0,0,0,0,0,0,0,27,243,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,177,0,0,0,0,0,0,0,0,0,162,255,255,255,255,255,255,255,255,255,203,
160,123,147,187,246,255,255,255,255,255,255,255,255,255,78,0,0,0,0,0,0,0,38,254,255,255,255,255,255,255,255,216,64,0,0,0,0,0,28,164,
255,255,255,255,255,255,255,255,190,0,0,0,0,0,0,0,120,255,255,255,255,255,255,255,226,13,0,0,0,0,0,0,0,0,149,255,255,255,255,255,
255,255,253,23,0,0,0,0,0,0,197,255,255,255,255,255,255,255,73,0,0,0,0,0,0,0,0,0,8,218,255,255,255,255,255,255,255,81,0,0,
0,0,0,0,237,255,255,255,255,255,255,235,0,0,0,0,0,0,0,0,0,0,0,146,255,255,255,255,255,255,255,108,0,0,0,0,0,6,255,255,
255,255,255,255,255,190,0,0,0,0,0,0,0,0,0,0,0,95,255,255,255,255,255,255,255,130,0,0,0,0,0,9,103,103,103,103,103,103,103,65,
0,0,0,0,0,0,0,0,0,0,0,96,255,255,255,255,255,255,255,116,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,139,255,255,255,255,255,255,255,80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,215,
255,255,255,255,255,255,253,22,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,97,255,255,255,255,255,255,255,
180,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,30,232,255,255,255,255,255,255,255,63,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,11,208,255,255,255,255,255,255,255,171,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,8,184,255,255,255,255,255,255,255,230,22,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,9,186,255,255,255,255,255,255,255,251,64,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,12,189,255,
255,255,255,255,255,255,255,98,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,21,203,255,255,255,255,255,255,255,255,
126,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,217,255,255,255,255,255,255,255,255,133,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,45,229,255,255,255,255,255,255,255,255,134,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,60,239,255,255,255,255,255,255,255,255,122,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,78,246,255,255,255,255,255,255,255,253,103,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,98,252,255,255,255,
255,255,255,255,250,85,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,120,255,255,255,255,255,255,255,255,247,75,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,143,255,255,255,255,255,255,255,255,244,66,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,5,165,255,255,255,255,255,255,255,255,240,58,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,11,184,255,255,255,255,255,255,255,255,236,50,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,19,
201,255,255,255,255,255,255,255,255,231,43,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,30,215,255,255,255,255,255,255,
255,255,255,201,155,155,155,155,155,155,155,155,155,155,155,155,155,155,155,155,7,0,0,0,0,0,182,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,11,0,0,0,0,0,187,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,11,0,0,0,0,0,187,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,11,0,0,0,0,0,187,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,11,0,0,0,
0,0,187,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,11,0,0,0,0,0,187,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,11,0,0,0,0,0,0,0,0,0,0,0,0,0,11,74,
134,190,216,236,250,235,215,188,129,67,6,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,37,145,240,255,255,255,255,255,255,
255,255,255,255,255,230,125,19,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,129,250,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,235,84,0,0,0,0,0,0,0,0,0,0,0,0,0,0,21,206,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,144,
0,0,0,0,0,0,0,0,0,0,0,0,24,214,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,141,0,0,0,0,
0,0,0,0,0,0,0,184,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,75,0,0,0,0,0,0,0,0,
0,99,255,255,255,255,255,255,255,255,255,241,178,144,121,151,192,253,255,255,255,255,255,255,255,255,226,6,0,0,0,0,0,0,0,0,213,255,255,255,
255,255,255,255,246,119,15,0,0,0,0,0,46,190,255,255,255,255,255,255,255,255,80,0,0,0,0,0,0,0,55,255,255,255,255,255,255,255,249,72,
0,0,0,0,0,0,0,0,9,207,255,255,255,255,255,255,255,167,0,0,0,0,0,0,0,107,255,255,255,255,255,255,255,144,0,0,0,0,0,0,
0,0,0,0,58,255,255,255,255,255,255,255,207,0,0,0,0,0,0,0,141,255,255,255,255,255,255,255,70,0,0,0,0,0,0,0,0,0,0,6,
248,255,255,255,255,255,255,235,0,0,0,0,0,0,0,7,11,11,11,11,11,11,11,1,0,0,0,0,0,0,0,0,0,0,2,240,255,255,255,255,
255,255,223,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,47,255,255,255,255,255,255,255,181,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,168,255,255,255,255,255,255,255,97,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,143,255,255,255,255,255,255,255,197,2,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,24,58,116,229,255,255,255,255,255,255,255,246,41,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
248,251,251,253,255,255,255,255,255,255,255,255,255,255,217,48,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,251,255,255,255,255,
255,255,255,255,255,255,255,239,148,12,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,251,255,255,255,255,255,255,255,255,255,
215,77,6,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,251,255,255,255,255,255,255,255,255,255,255,235,181,95,12,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,251,255,255,255,255,255,255,255,255,255,255,255,255,255,235,104,1,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,251,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,169,7,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,2,27,59,95,192,255,255,255,255,255,255,255,255,255,159,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,55,219,255,255,255,255,255,255,255,255,75,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,37,235,255,255,255,255,255,255,255,197,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,98,255,255,255,255,255,255,255,254,20,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
26,255,255,255,255,255,255,255,255,68,0,0,0,0,28,75,75,75,75,75,75,75,56,0,0,0,0,0,0,0,0,0,0,0,0,22,255,255,255,255,
255,255,255,255,88,0,0,0,0,73,255,255,255,255,255,255,255,237,3,0,0,0,0,0,0,0,0,0,0,0,80,255,255,255,255,255,255,255,255,70,
0,0,0,0,40,255,255,255,255,255,255,255,255,74,0,0,0,0,0,0,0,0,0,0,0,175,255,255,255,255,255,255,255,255,39,0,0,0,0,4,
231,255,255,255,255,255,255,255,239,50,0,0,0,0,0,0,0,0,0,129,255,255,255,255,255,255,255,255,231,4,0,0,0,0,0,137,255,255,255,255,
255,255,255,255,243,143,32,0,0,0,0,0,64,192,255,255,255,255,255,255,255,255,255,136,0,0,0,0,0,0,30,242,255,255,255,255,255,255,255,255,
255,254,228,195,176,206,242,255,255,255,255,255,255,255,255,255,255,241,29,0,0,0,0,0,0,0,100,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,95,0,0,0,0,0,0,0,0,0,147,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,138,0,0,0,0,0,0,0,0,0,0,0,131,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,253,
119,0,0,0,0,0,0,0,0,0,0,0,0,0,63,216,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,204,52,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,6,96,198,255,255,255,255,255,255,255,255,255,255,255,255,253,184,83,2,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,37,99,156,203,222,240,251,236,218,196,145,89,27,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,62,254,255,255,255,255,255,255,255,255,255,47,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,8,217,255,255,255,255,255,255,255,255,255,255,47,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,133,255,255,255,255,255,255,255,255,255,255,255,47,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,46,
250,255,255,255,255,255,255,255,255,255,255,255,47,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,202,255,255,255,
255,255,255,255,255,255,255,255,255,47,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,113,255,255,255,255,255,255,255,
255,255,255,255,255,255,47,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,244,255,255,255,255,255,255,255,255,255,255,
255,255,255,47,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,184,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
47,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,92,255,255,255,255,255,255,255,243,255,255,255,255,255,255,255,47,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,20,235,255,255,255,255,255,255,218,108,255,255,255,255,255,255,255,47,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,164,255,255,255,255,255,255,254,63,99,255,255,255,255,255,255,255,47,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,72,255,255,255,255,255,255,255,152,0,99,255,255,255,255,255,255,255,47,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,11,224,255,255,255,255,255,255,228,15,0,99,255,255,255,255,255,255,255,47,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,144,255,255,255,255,255,255,255,79,0,0,99,255,255,255,255,255,255,255,47,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,54,
253,255,255,255,255,255,255,170,0,0,0,99,255,255,255,255,255,255,255,47,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,210,255,255,255,
255,255,255,238,23,0,0,0,99,255,255,255,255,255,255,255,47,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,123,255,255,255,255,255,255,255,
96,0,0,0,0,99,255,255,255,255,255,255,255,47,0,0,0,0,0,0,0,0,0,0,0,0,0,0,39,248,255,255,255,255,255,255,188,0,0,0,
0,0,99,255,255,255,255,255,255,255,47,0,0,0,0,0,0,0,0,0,0,0,0,0,1,194,255,255,255,255,255,255,245,34,0,0,0,0,0,99,
255,255,255,255,255,255,255,47,0,0,0,0,0,0,0,0,0,0,0,0,0,103,255,255,255,255,255,255,255,114,0,0,0,0,0,0,99,255,255,255,
255,255,255,255,47,0,0,0,0,0,0,0,0,0,0,0,0,26,240,255,255,255,255,255,255,203,3,0,0,0,0,0,0,99,255,255,255,255,255,255,
255,47,0,0,0,0,0,0,0,0,0,0,0,0,175,255,255,255,255,255,255,251,46,0,0,0,0,0,0,0,99,255,255,255,255,255,255,255,47,0,
0,0,0,0,0,0,0,0,0,0,82,255,255,255,255,255,255,255,132,0,0,0,0,0,0,0,0,99,255,255,255,255,255,255,255,47,0,0,0,0,
0,0,0,0,0,0,16,230,255,255,255,255,255,255,216,7,0,0,0,0,0,0,0,0,99,255,255,255,255,255,255,255,47,0,0,0,0,0,0,0,
0,0,0,154,255,255,255,255,255,255,254,61,0,0,0,0,0,0,0,0,0,99,255,255,255,255,255,255,255,47,0,0,0,0,0,0,0,0,0,53,
254,255,255,255,255,255,255,230,123,123,123,123,123,123,123,123,123,123,175,255,255,255,255,255,255,255,148,123,123,123,123,9,0,0,0,0,87,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,19,0,0,0,0,87,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,19,0,0,0,0,87,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,19,0,0,0,0,87,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,19,0,0,0,0,87,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,19,0,0,0,0,87,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,19,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,139,255,255,
255,255,255,255,255,47,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,139,255,255,255,255,255,
255,255,47,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,139,255,255,255,255,255,255,255,47,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,139,255,255,255,255,255,255,255,47,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,139,255,255,255,255,255,255,255,47,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,139,255,255,255,255,255,255,255,47,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,139,255,255,255,255,255,255,255,47,0,0,0,0,0,0,0,0,0,0,0,0,
0,240,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,219,0,0,0,0,0,0,0,0,0,0,8,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,219,0,0,0,0,0,0,0,0,0,0,31,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,219,0,0,0,0,0,0,0,0,0,0,55,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,219,0,0,0,0,0,0,0,0,0,0,79,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,219,0,0,0,0,0,0,0,0,0,0,103,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,219,
0,0,0,0,0,0,0,0,0,0,127,255,255,255,255,255,255,235,167,167,167,167,167,167,167,167,167,167,167,167,167,167,167,167,144,0,0,0,0,0,
0,0,0,0,0,151,255,255,255,255,255,255,176,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
174,255,255,255,255,255,255,148,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,198,255,255,255,255,
255,255,121,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,222,255,255,255,255,255,255,94,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,246,255,255,255,255,255,255,66,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,14,255,255,255,255,255,255,255,39,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,38,255,255,255,255,255,255,255,12,0,0,47,126,192,219,244,231,206,168,93,19,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,62,255,255,255,255,255,255,240,0,41,184,255,255,255,255,255,255,255,255,255,248,142,19,0,0,0,0,0,0,0,0,
0,0,0,0,85,255,255,255,255,255,255,213,69,246,255,255,255,255,255,255,255,255,255,255,255,255,237,67,0,0,0,0,0,0,0,0,0,0,0,109,
255,255,255,255,255,255,238,246,255,255,255,255,255,255,255,255,255,255,255,255,255,255,251,96,0,0,0,0,0,0,0,0,0,0,133,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,251,62,0,0,0,0,0,0,0,0,0,157,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,232,17,0,0,0,0,0,0,0,0,181,255,255,255,255,255,255,255,255,230,133,58,22,6,28,85,
193,255,255,255,255,255,255,255,255,255,133,0,0,0,0,0,0,0,0,68,127,170,214,251,255,255,255,142,11,0,0,0,0,0,0,1,104,242,255,255,
255,255,255,255,255,243,16,0,0,0,0,0,0,0,0,0,0,0,6,44,87,86,0,0,0,0,0,0,0,0,0,0,84,255,255,255,255,255,255,255,
255,88,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,173,255,255,255,255,255,255,255,161,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,45,255,255,255,255,255,255,255,212,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,236,255,255,255,255,255,255,235,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,209,255,255,255,255,255,255,254,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,229,255,255,255,255,255,255,243,0,0,0,0,0,0,53,91,91,91,91,91,91,91,24,0,0,0,0,0,0,0,0,
0,0,0,19,255,255,255,255,255,255,255,217,0,0,0,0,0,0,126,255,255,255,255,255,255,255,117,0,0,0,0,0,0,0,0,0,0,0,74,255,
255,255,255,255,255,255,184,0,0,0,0,0,0,92,255,255,255,255,255,255,255,197,0,0,0,0,0,0,0,0,0,0,6,211,255,255,255,255,255,255,
255,111,0,0,0,0,0,0,30,253,255,255,255,255,255,255,255,128,0,0,0,0,0,0,0,0,0,135,255,255,255,255,255,255,255,255,32,0,0,0,
0,0,0,0,187,255,255,255,255,255,255,255,255,147,23,0,0,0,0,0,28,180,255,255,255,255,255,255,255,255,176,0,0,0,0,0,0,0,0,67,
255,255,255,255,255,255,255,255,255,245,172,133,106,141,187,244,255,255,255,255,255,255,255,255,251,38,0,0,0,0,0,0,0,0,0,155,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,125,0,0,0,0,0,0,0,0,0,0,12,195,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,159,0,0,0,0,0,0,0,0,0,0,0,0,12,190,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,152,5,0,0,0,0,0,0,0,0,0,0,0,0,0,2,116,247,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
239,89,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,31,140,238,255,255,255,255,255,255,255,255,255,255,255,234,121,14,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,10,73,134,191,218,237,251,235,214,189,129,61,5,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,44,131,180,206,233,249,230,209,171,105,38,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,2,105,214,255,255,255,255,255,255,255,255,255,255,255,188,73,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,45,
199,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,177,23,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,85,251,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,235,50,0,0,0,0,0,0,0,0,0,0,0,0,0,82,251,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,238,51,0,0,0,0,0,0,0,0,0,0,0,64,250,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,225,15,0,0,0,0,0,0,0,0,0,9,220,255,255,255,255,255,255,255,255,255,255,255,248,255,255,255,255,255,255,255,255,255,255,255,
161,0,0,0,0,0,0,0,0,0,135,255,255,255,255,255,255,255,255,231,125,42,7,0,22,67,172,252,255,255,255,255,255,255,255,253,37,0,0,0,
0,0,0,0,32,251,255,255,255,255,255,255,255,181,19,0,0,0,0,0,0,0,73,247,255,255,255,255,255,255,255,147,0,0,0,0,0,0,0,132,
255,255,255,255,255,255,255,197,8,0,0,0,0,0,0,0,0,0,96,255,255,255,255,255,255,255,220,0,0,0,0,0,0,3,229,255,255,255,255,255,
255,250,35,0,0,0,0,0,0,0,0,0,0,4,198,219,219,219,219,219,219,219,18,0,0,0,0,0,75,255,255,255,255,255,255,255,153,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,144,255,255,255,255,255,255,255,61,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,198,255,255,255,255,255,255,236,2,0,0,0,0,38,124,160,192,216,198,173,123,45,
0,0,0,0,0,0,0,0,0,0,0,0,5,247,255,255,255,255,255,255,188,0,0,0,76,192,255,255,255,255,255,255,255,255,255,210,74,0,0,0,
0,0,0,0,0,0,0,49,255,255,255,255,255,255,255,147,0,6,145,255,255,255,255,255,255,255,255,255,255,255,255,255,176,13,0,0,0,0,0,0,
0,0,76,255,255,255,255,255,255,255,120,5,195,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,210,26,0,0,0,0,0,0,0,92,255,255,
255,255,255,255,255,108,140,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,200,6,0,0,0,0,0,0,108,255,255,255,255,255,255,255,
183,254,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,145,0,0,0,0,0,0,124,255,255,255,255,255,255,255,255,255,255,255,246,
131,51,14,12,45,120,239,255,255,255,255,255,255,255,255,251,34,0,0,0,0,0,130,255,255,255,255,255,255,255,255,255,255,198,37,0,0,0,0,0,
0,27,183,255,255,255,255,255,255,255,255,156,0,0,0,0,0,112,255,255,255,255,255,255,255,255,255,209,5,0,0,0,0,0,0,0,0,2,197,255,
255,255,255,255,255,255,237,3,0,0,0,0,94,255,255,255,255,255,255,255,255,251,50,0,0,0,0,0,0,0,0,0,0,41,248,255,255,255,255,255,
255,255,57,0,0,0,0,76,255,255,255,255,255,255,255,255,162,0,0,0,0,0,0,0,0,0,0,0,0,158,255,255,255,255,255,255,255,105,0,0,
0,0,58,255,255,255,255,255,255,255,255,90,0,0,0,0,0,0,0,0,0,0,0,0,92,255,255,255,255,255,255,255,129,0,0,0,0,19,254,255,
255,255,255,255,255,255,60,0,0,0,0,0,0,0,0,0,0,0,0,64,255,255,255,255,255,255,255,149,0,0,0,0,0,213,255,255,255,255,255,255,
255,67,0,0,0,0,0,0,0,0,0,0,0,0,73,255,255,255,255,255,255,255,132,0,0,0,0,0,151,255,255,255,255,255,255,255,109,0,0,0,
0,0,0,0,0,0,0,0,0,114,255,255,255,255,255,255,255,107,0,0,0,0,0,90,255,255,255,255,255,255,255,197,0,0,0,0,0,0,0,0,
0,0,0,0,199,255,255,255,255,255,255,255,72,0,0,0,0,0,15,242,255,255,255,255,255,255,255,99,0,0,0,0,0,0,0,0,0,0,102,255,
255,255,255,255,255,255,244,8,0,0,0,0,0,0,139,255,255,255,255,255,255,255,240,41,0,0,0,0,0,0,0,0,42,242,255,255,255,255,255,255,
255,174,0,0,0,0,0,0,0,27,249,255,255,255,255,255,255,255,246,110,1,0,0,0,0,2,114,246,255,255,255,255,255,255,255,255,60,0,0,0,
0,0,0,0,0,137,255,255,255,255,255,255,255,255,255,216,140,107,108,143,218,255,255,255,255,255,255,255,255,255,173,0,0,0,0,0,0,0,0,0,
4,195,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,232,30,0,0,0,0,0,0,0,0,0,0,25,230,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,245,52,0,0,0,0,0,0,0,0,0,0,0,0,31,207,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,230,61,0,0,0,0,0,0,0,0,0,0,0,0,0,0,10,162,254,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,185,23,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,47,172,254,255,255,255,255,255,255,255,255,255,255,255,187,
63,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,25,92,158,204,225,245,246,225,204,165,97,29,0,0,0,0,0,0,
0,0,0,0,0,0,7,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,107,0,0,7,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,107,0,0,7,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,107,0,0,7,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,107,0,0,7,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,107,0,
0,7,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,107,0,0,5,167,167,167,167,167,167,167,
167,167,167,167,167,167,167,167,167,167,167,168,255,255,255,255,255,255,255,255,103,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,77,255,255,255,255,255,255,255,244,21,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,206,255,255,255,255,255,255,
255,135,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,82,255,255,255,255,255,255,255,242,19,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,1,210,255,255,255,255,255,255,255,130,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,87,255,255,255,255,255,255,255,240,16,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,214,255,255,255,255,
255,255,255,126,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,91,255,255,255,255,255,255,255,237,14,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,218,255,255,255,255,255,255,255,121,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,96,255,255,255,255,255,255,255,235,12,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,221,255,255,
255,255,255,255,255,117,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,101,255,255,255,255,255,255,255,232,10,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,6,225,255,255,255,255,255,255,255,112,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,106,255,255,255,255,255,255,255,230,8,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,228,
255,255,255,255,255,255,255,108,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,110,255,255,255,255,255,255,255,227,7,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,231,255,255,255,255,255,255,255,103,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,115,255,255,255,255,255,255,255,224,6,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
11,234,255,255,255,255,255,255,255,99,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,120,255,255,255,255,255,255,255,221,
4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,13,236,255,255,255,255,255,255,255,95,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,124,255,255,255,255,255,255,255,217,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,15,239,255,255,255,255,255,255,255,90,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,129,255,255,255,255,255,255,
255,214,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,18,241,255,255,255,255,255,255,255,86,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,134,255,255,255,255,255,255,255,210,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,20,243,255,255,255,255,255,255,255,81,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,139,255,255,255,255,
255,255,255,207,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,23,245,255,255,255,255,255,255,255,77,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,143,255,255,255,255,255,255,255,202,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,26,247,255,255,255,255,255,255,255,72,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,148,255,255,
255,255,255,255,255,199,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,29,249,255,255,255,255,255,255,255,68,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,76,133,189,217,235,250,237,218,193,137,79,12,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,34,144,238,255,255,255,255,255,255,255,255,255,255,255,242,151,41,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,6,124,247,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,250,136,10,0,0,0,0,0,0,0,0,
0,0,0,0,0,14,192,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,206,22,0,0,0,0,0,0,0,0,0,0,0,9,
204,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,217,18,0,0,0,0,0,0,0,0,0,0,147,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,172,0,0,0,0,0,0,0,0,0,48,254,255,255,255,255,255,255,255,255,221,121,
80,50,77,116,212,255,255,255,255,255,255,255,255,255,74,0,0,0,0,0,0,0,0,149,255,255,255,255,255,255,255,255,125,6,0,0,0,0,0,3,
106,254,255,255,255,255,255,255,255,177,0,0,0,0,0,0,0,0,226,255,255,255,255,255,255,255,154,0,0,0,0,0,0,0,0,0,131,255,255,255,
255,255,255,255,251,4,0,0,0,0,0,0,5,253,255,255,255,255,255,255,255,21,0,0,0,0,0,0,0,0,0,4,246,255,255,255,255,255,255,255,
33,0,0,0,0,0,0,21,255,255,255,255,255,255,255,223,0,0,0,0,0,0,0,0,0,0,0,196,255,255,255,255,255,255,255,52,0,0,0,0,
0,0,3,252,255,255,255,255,255,255,214,0,0,0,0,0,0,0,0,0,0,0,187,255,255,255,255,255,255,255,29,0,0,0,0,0,0,0,199,255,
255,255,255,255,255,253,14,0,0,0,0,0,0,0,0,0,1,240,255,255,255,255,255,255,228,0,0,0,0,0,0,0,0,100,255,255,255,255,255,255,
255,121,0,0,0,0,0,0,0,0,0,96,255,255,255,255,255,255,255,129,0,0,0,0,0,0,0,0,2,206,255,255,255,255,255,255,252,75,0,0,
0,0,0,0,0,57,246,255,255,255,255,255,255,225,10,0,0,0,0,0,0,0,0,0,57,247,255,255,255,255,255,255,255,172,60,18,0,14,55,159,
253,255,255,255,255,255,255,252,78,0,0,0,0,0,0,0,0,0,0,0,55,232,255,255,255,255,255,255,255,255,255,242,255,255,255,255,255,255,255,255,
255,241,72,0,0,0,0,0,0,0,0,0,0,0,0,0,30,171,250,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,253,183,43,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,20,131,255,255,255,255,255,255,255,255,255,255,255,255,255,159,30,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,9,102,200,252,255,255,255,255,255,255,255,255,255,255,255,255,255,254,209,115,15,0,0,0,0,0,0,0,0,0,0,0,0,0,
47,221,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,232,63,0,0,0,0,0,0,0,0,0,0,0,94,246,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,251,114,0,0,0,0,0,0,0,0,0,103,255,255,255,255,255,255,255,255,252,157,81,
41,10,38,78,148,250,255,255,255,255,255,255,255,255,130,0,0,0,0,0,0,0,41,248,255,255,255,255,255,255,255,203,43,0,0,0,0,0,0,0,
36,191,255,255,255,255,255,255,255,254,64,0,0,0,0,0,0,171,255,255,255,255,255,255,255,231,25,0,0,0,0,0,0,0,0,0,15,219,255,255,
255,255,255,255,255,203,0,0,0,0,0,18,252,255,255,255,255,255,255,255,85,0,0,0,0,0,0,0,0,0,0,0,55,255,255,255,255,255,255,255,
255,49,0,0,0,0,86,255,255,255,255,255,255,255,255,19,0,0,0,0,0,0,0,0,0,0,0,0,223,255,255,255,255,255,255,255,122,0,0,0,
0,118,255,255,255,255,255,255,255,226,0,0,0,0,0,0,0,0,0,0,0,0,0,194,255,255,255,255,255,255,255,156,0,0,0,0,125,255,255,255,
255,255,255,255,251,9,0,0,0,0,0,0,0,0,0,0,0,0,233,255,255,255,255,255,255,255,164,0,0,0,0,94,255,255,255,255,255,255,255,255,
63,0,0,0,0,0,0,0,0,0,0,0,36,255,255,255,255,255,255,255,255,131,0,0,0,0,49,255,255,255,255,255,255,255,255,207,9,0,0,0,
0,0,0,0,0,0,3,186,255,255,255,255,255,255,255,255,87,0,0,0,0,0,207,255,255,255,255,255,255,255,255,174,20,0,0,0,0,0,0,0,
13,154,255,255,255,255,255,255,255,255,236,8,0,0,0,0,0,92,255,255,255,255,255,255,255,255,255,242,130,63,26,1,22,58,119,235,255,255,255,255,
255,255,255,255,255,128,0,0,0,0,0,0,0,172,255,255,255,255,255,255,255,255,255,255,255,255,252,255,255,255,255,255,255,255,255,255,255,255,255,201,
5,0,0,0,0,0,0,0,16,201,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,219,30,0,0,0,0,0,
0,0,0,0,10,177,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,198,20,0,0,0,0,0,0,0,0,0,0,0,
1,93,231,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,239,110,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,13,109,206,
255,255,255,255,255,255,255,255,255,255,255,255,255,214,118,20,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,41,100,154,201,221,
238,251,240,223,204,159,105,48,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,25,91,160,202,222,242,246,223,201,158,89,22,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,53,176,254,255,255,255,255,255,255,255,255,255,255,254,171,45,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,16,171,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,254,161,10,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,49,221,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,208,31,0,0,0,0,0,0,0,0,0,0,0,0,38,
237,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,233,29,0,0,0,0,0,0,0,0,0,0,19,221,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,200,6,0,0,0,0,0,0,0,0,0,152,255,255,255,255,255,255,255,255,255,237,165,
127,122,153,224,255,255,255,255,255,255,255,255,255,145,0,0,0,0,0,0,0,0,41,252,255,255,255,255,255,255,255,254,146,11,0,0,0,0,1,111,
246,255,255,255,255,255,255,255,252,34,0,0,0,0,0,0,0,153,255,255,255,255,255,255,255,252,69,0,0,0,0,0,0,0,0,39,232,255,255,255,
255,255,255,255,149,0,0,0,0,0,0,1,230,255,255,255,255,255,255,255,133,0,0,0,0,0,0,0,0,0,0,82,255,255,255,255,255,255,255,246,
21,0,0,0,0,0,51,255,255,255,255,255,255,255,221,7,0,0,0,0,0,0,0,0,0,0,0,177,255,255,255,255,255,255,255,104,0,0,0,0,
0,88,255,255,255,255,255,255,255,137,0,0,0,0,0,0,0,0,0,0,0,0,85,255,255,255,255,255,255,255,165,0,0,0,0,0,113,255,255,255,
255,255,255,255,93,0,0,0,0,0,0,0,0,0,0,0,0,41,255,255,255,255,255,255,255,228,0,0,0,0,0,130,255,255,255,255,255,255,255,83,
0,0,0,0,0,0,0,0,0,0,0,0,32,255,255,255,255,255,255,255,255,33,0,0,0,0,112,255,255,255,255,255,255,255,109,0,0,0,0,0,
0,0,0,0,0,0,0,62,255,255,255,255,255,255,255,255,76,0,0,0,0,89,255,255,255,255,255,255,255,174,0,0,0,0,0,0,0,0,0,0,
0,0,134,255,255,255,255,255,255,255,255,93,0,0,0,0,43,255,255,255,255,255,255,255,251,51,0,0,0,0,0,0,0,0,0,0,31,242,255,255,
255,255,255,255,255,255,112,0,0,0,0,0,225,255,255,255,255,255,255,255,209,5,0,0,0,0,0,0,0,0,0,188,255,255,255,255,255,255,255,255,
255,130,0,0,0,0,0,142,255,255,255,255,255,255,255,255,190,32,0,0,0,0,0,0,27,182,255,255,255,255,255,255,255,255,255,255,149,0,0,0,
0,0,25,247,255,255,255,255,255,255,255,255,243,125,46,12,10,44,120,239,255,255,255,255,255,255,255,255,255,255,255,144,0,0,0,0,0,0,134,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,186,255,255,255,255,255,255,255,127,0,0,0,0,0,0,4,192,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,168,86,255,255,255,255,255,255,255,110,0,0,0,0,0,0,0,22,207,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,213,14,99,255,255,255,255,255,255,255,94,0,0,0,0,0,0,0,0,11,174,255,255,255,255,255,255,255,255,255,255,255,255,255,
167,13,0,124,255,255,255,255,255,255,255,67,0,0,0,0,0,0,0,0,0,0,74,210,255,255,255,255,255,255,255,255,255,208,94,0,0,0,166,255,
255,255,255,255,255,254,16,0,0,0,0,0,0,0,0,0,0,0,0,49,126,180,205,225,204,172,138,53,0,0,0,0,0,216,255,255,255,255,255,255,
216,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,39,255,255,255,255,255,255,255,162,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,132,255,255,255,255,255,255,255,94,0,0,0,0,0,10,225,227,227,
227,227,227,227,219,15,0,0,0,0,0,0,0,0,0,0,21,242,255,255,255,255,255,255,240,10,0,0,0,0,0,0,208,255,255,255,255,255,255,255,
125,0,0,0,0,0,0,0,0,0,3,179,255,255,255,255,255,255,255,149,0,0,0,0,0,0,0,133,255,255,255,255,255,255,255,252,94,0,0,0,
0,0,0,0,12,164,255,255,255,255,255,255,255,254,47,0,0,0,0,0,0,0,26,250,255,255,255,255,255,255,255,255,182,71,21,0,3,32,113,222,
255,255,255,255,255,255,255,255,152,0,0,0,0,0,0,0,0,0,146,255,255,255,255,255,255,255,255,255,255,255,245,252,255,255,255,255,255,255,255,255,
255,255,231,16,0,0,0,0,0,0,0,0,0,10,217,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,253,80,0,0,
0,0,0,0,0,0,0,0,0,42,233,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,254,97,0,0,0,0,0,0,0,0,
0,0,0,0,0,41,228,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,253,101,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,18,167,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,209,55,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,66,182,
254,255,255,255,255,255,255,255,255,255,255,220,116,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,34,103,168,209,230,
249,236,210,183,138,52,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,63,193,243,244,195,66,0,0,0,0,0,0,0,0,0,0,0,98,254,255,255,255,255,254,104,0,0,0,0,0,0,0,0,0,26,243,255,
255,255,255,255,255,245,30,0,0,0,0,0,0,0,0,91,255,255,255,255,255,255,255,255,98,0,0,0,0,0,0,0,0,127,255,255,255,255,255,255,
255,255,135,0,0,0,0,0,0,0,0,88,255,255,255,255,255,255,255,255,95,0,0,0,0,0,0,0,0,22,239,255,255,255,255,255,255,241,26,0,
0,0,0,0,0,0,0,0,88,251,255,255,255,255,252,93,0,0,0,0,0,0,0,0,0,0,0,51,179,231,232,180,54,0,0,0,0,0,0,0,
0,0,26,83,83,83,83,83,83,83,83,83,83,83,83,83,83,83,83,83,47,0,0,0,0,0,0,79,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,143,0,0,0,0,0,0,79,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,143,0,0,0,0,0,0,79,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,143,0,0,0,0,0,0,79,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
143,0,0,0,0,0,0,79,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,143,0,0,0,0,0,0,79,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,143,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
const AAGlyph F54_g[] PROGMEM = {
{48,36,39,36,0,39,0},
{49,23,39,23,0,39,1404},
{50,34,39,34,0,39,2301},
{51,35,39,35,0,39,3627},
{52,37,39,37,0,39,4992},
{53,35,39,35,0,39,6435},
{54,35,39,35,0,39,7800},
{55,31,39,31,0,39,9165},
{56,35,39,35,0,39,10374},
{57,35,40,35,0,40,11739},
{46,18,10,18,0,10,13139},
{45,25,20,25,0,20,13319}
};
const AAFont F54 = { 12, F54_g, F54_bmp };
// F46: Inter-Bold.otf 46px, 12 glyphs, 10282 bytes
const uint8_t F46_bmp[] PROGMEM = {
0,0,0,0,0,0,0,0,0,0,18,96,173,204,230,249,229,204,171,94,16,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,20,
159,251,255,255,255,255,255,255,255,255,255,250,156,19,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,86,237,255,255,255,255,255,255,255,255,255,
255,255,255,255,235,84,0,0,0,0,0,0,0,0,0,0,0,0,0,93,253,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,253,92,0,0,
0,0,0,0,0,0,0,0,0,80,253,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,252,79,0,0,0,0,0,0,0,0,0,16,
232,255,255,255,255,255,255,255,251,205,180,206,252,255,255,255,255,255,255,255,232,16,0,0,0,0,0,0,0,0,152,255,255,255,255,255,255,255,166,29,
0,0,0,31,169,255,255,255,255,255,255,255,151,0,0,0,0,0,0,0,25,252,255,255,255,255,255,255,148,0,0,0,0,0,0,0,150,255,255,255,
255,255,255,252,25,0,0,0,0,0,0,119,255,255,255,255,255,255,213,7,0,0,0,0,0,0,0,7,213,255,255,255,255,255,255,119,0,0,0,0,
0,0,216,255,255,255,255,255,255,92,0,0,0,0,0,0,0,0,0,92,255,255,255,255,255,255,217,0,0,0,0,0,29,255,255,255,255,255,255,242,
5,0,0,0,0,0,0,0,0,0,5,241,255,255,255,255,255,255,30,0,0,0,0,80,255,255,255,255,255,255,178,0,0,0,0,0,0,0,0,0,
0,0,177,255,255,255,255,255,255,81,0,0,0,0,132,255,255,255,255,255,255,120,0,0,0,0,0,0,0,0,0,0,0,119,255,255,255,255,255,255,
133,0,0,0,0,173,255,255,255,255,255,255,83,0,0,0,0,0,0,0,0,0,0,0,82,255,255,255,255,255,255,174,0,0,0,0,189,255,255,255,
255,255,255,52,0,0,0,0,0,0,0,0,0,0,0,50,255,255,255,255,255,255,190,0,0,0,0,204,255,255,255,255,255,255,38,0,0,0,0,0,
0,0,0,0,0,0,37,255,255,255,255,255,255,206,0,0,0,0,220,255,255,255,255,255,255,28,0,0,0,0,0,0,0,0,0,0,0,26,255,255,
255,255,255,255,222,0,0,0,0,221,255,255,255,255,255,255,28,0,0,0,0,0,0,0,0,0,0,0,25,255,255,255,255,255,255,224,0,0,0,0,
205,255,255,255,255,255,255,38,0,0,0,0,0,0,0,0,0,0,0,36,255,255,255,255,255,255,208,0,0,0,0,190,255,255,255,255,255,255,51,0,
0,0,0,0,0,0,0,0,0,0,49,255,255,255,255,255,255,192,0,0,0,0,174,255,255,255,255,255,255,81,0,0,0,0,0,0,0,0,0,0,
0,80,255,255,255,255,255,255,177,0,0,0,0,135,255,255,255,255,255,255,118,0,0,0,0,0,0,0,0,0,0,0,117,255,255,255,255,255,255,137,
0,0,0,0,84,255,255,255,255,255,255,174,0,0,0,0,0,0,0,0,0,0,0,174,255,255,255,255,255,255,86,0,0,0,0,32,255,255,255,255,
255,255,240,4,0,0,0,0,0,0,0,0,0,4,239,255,255,255,255,255,255,35,0,0,0,0,0,221,255,255,255,255,255,255,87,0,0,0,0,0,
0,0,0,0,89,255,255,255,255,255,255,224,1,0,0,0,0,0,126,255,255,255,255,255,255,209,5,0,0,0,0,0,0,0,6,210,255,255,255,255,
255,255,128,0,0,0,0,0,0,31,254,255,255,255,255,255,255,143,0,0,0,0,0,0,0,147,255,255,255,255,255,255,254,33,0,0,0,0,0,0,
0,161,255,255,255,255,255,255,255,162,27,0,0,0,29,166,255,255,255,255,255,255,255,164,0,0,0,0,0,0,0,0,20,236,255,255,255,255,255,255,
255,250,203,179,205,252,255,255,255,255,255,255,255,238,22,0,0,0,0,0,0,0,0,0,88,254,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,254,91,0,0,0,0,0,0,0,0,0,0,0,103,254,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,105,0,0,0,0,0,
0,0,0,0,0,0,0,0,94,240,255,255,255,255,255,255,255,255,255,255,255,255,255,240,96,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
24,165,252,255,255,255,255,255,255,255,255,255,252,164,24,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,20,100,175,207,232,250,231,
206,174,99,20,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,81,236,255,255,255,255,255,255,207,0,0,0,0,0,0,0,0,0,
12,156,255,255,255,255,255,255,255,255,207,0,0,0,0,0,0,0,0,56,220,255,255,255,255,255,255,255,255,255,207,0,0,0,0,0,0,4,127,252,
255,255,255,255,255,255,255,255,255,255,207,0,0,0,0,0,36,200,255,255,255,255,255,255,255,255,255,255,255,255,207,0,0,0,0,0,228,255,255,255,
255,255,255,255,255,255,255,255,255,255,207,0,0,0,0,0,235,255,255,255,255,255,224,69,255,255,255,255,255,255,207,0,0,0,0,0,235,255,255,255,
255,167,15,7,255,255,255,255,255,255,207,0,0,0,0,0,235,255,255,243,96,0,0,7,255,255,255,255,255,255,207,0,0,0,0,0,235,255,203,37,
0,0,0,7,255,255,255,255,255,255,207,0,0,0,0,0,234,137,5,0,0,0,0,7,255,255,255,255,255,255,207,0,0,0,0,0,55,0,0,0,
0,0,0,7,255,255,255,255,255,255,207,0,0,0,0,0,0,0,0,0,0,0,0,7,255,255,255,255,255,255,207,0,0,0,0,0,0,0,0,0,
0,0,0,7,255,255,255,255,255,255,207,0,0,0,0,0,0,0,0,0,0,0,0,7,255,255,255,255,255,255,207,0,0,0,0,0,0,0,0,0,
0,0,0,7,255,255,255,255,255,255,207,0,0,0,0,0,0,0,0,0,0,0,0,7,255,255,255,255,255,255,207,0,0,0,0,0,0,0,0,0,
0,0,0,7,255,255,255,255,255,255,207,0,0,0,0,0,0,0,0,0,0,0,0,7,255,255,255,255,255,255,207,0,0,0,0,0,0,0,0,0,
0,0,0,7,255,255,255,255,255,255,207,0,0,0,0,0,0,0,0,0,0,0,0,7,255,255,255,255,255,255,207,0,0,0,0,0,0,0,0,0,
0,0,0,7,255,255,255,255,255,255,207,0,0,0,0,0,0,0,0,0,0,0,0,7,255,255,255,255,255,255,207,0,0,0,0,0,0,0,0,0,
0,0,0,7,255,255,255,255,255,255,207,0,0,0,0,0,0,0,0,0,0,0,0,7,255,255,255,255,255,255,207,0,0,0,0,0,0,0,0,0,
0,0,0,7,255,255,255,255,255,255,207,0,0,0,0,0,0,0,0,0,0,0,0,7,255,255,255,255,255,255,207,0,0,0,0,0,0,0,0,0,
0,0,0,7,255,255,255,255,255,255,207,0,0,0,0,0,0,0,0,0,0,0,0,7,255,255,255,255,255,255,207,0,0,0,0,0,0,0,0,0,
0,0,0,7,255,255,255,255,255,255,207,0,0,0,0,0,0,0,0,0,0,0,0,7,255,255,255,255,255,255,207,0,0,0,0,0,0,0,0,0,
0,0,0,7,255,255,255,255,255,255,207,0,0,0,0,0,0,0,0,0,0,0,0,7,255,255,255,255,255,255,207,0,0,0,0,0,0,0,0,0,
0,0,0,7,255,255,255,255,255,255,207,0,0,0,0,0,0,0,0,0,0,0,0,59,127,190,220,240,249,230,210,160,100,23,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,82,205,255,255,255,255,255,255,255,255,255,255,249,160,33,0,0,0,0,0,0,0,0,0,0,0,0,12,162,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,243,99,0,0,0,0,0,0,0,0,0,0,7,192,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,113,0,0,0,0,0,0,0,0,0,160,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,74,0,0,0,0,0,
0,0,57,255,255,255,255,255,255,255,253,198,153,139,179,237,255,255,255,255,255,255,255,221,8,0,0,0,0,0,0,184,255,255,255,255,255,255,212,54,
0,0,0,0,16,146,255,255,255,255,255,255,255,87,0,0,0,0,0,15,250,255,255,255,255,255,240,25,0,0,0,0,0,0,0,157,255,255,255,255,
255,255,172,0,0,0,0,0,78,255,255,255,255,255,255,116,0,0,0,0,0,0,0,0,12,250,255,255,255,255,255,217,0,0,0,0,0,106,255,255,
255,255,255,255,56,0,0,0,0,0,0,0,0,0,206,255,255,255,255,255,244,0,0,0,0,0,95,191,191,191,191,191,191,14,0,0,0,0,0,0,
0,0,0,176,255,255,255,255,255,233,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,212,255,255,255,255,255,197,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,28,251,255,255,255,255,255,158,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,152,255,255,255,255,255,255,73,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,66,251,255,
255,255,255,255,209,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,33,236,255,255,255,255,255,254,67,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,28,221,255,255,255,255,255,255,143,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,26,219,255,255,255,255,255,255,197,7,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,220,255,255,255,255,255,255,221,20,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,41,228,255,255,255,255,255,255,228,34,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,50,235,255,255,255,255,255,255,233,39,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,61,241,255,255,255,255,255,
255,229,40,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,72,246,255,255,255,255,255,255,224,34,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,84,249,255,255,255,255,255,255,220,29,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,97,252,255,
255,255,255,255,255,219,28,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,111,254,255,255,255,255,255,255,217,26,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,126,255,255,255,255,255,255,255,215,25,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,141,255,255,255,255,255,255,255,214,24,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,155,255,255,255,255,255,255,255,255,190,159,
159,159,159,159,159,159,159,159,159,159,159,159,72,0,0,0,0,55,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,115,0,0,0,0,55,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,115,0,0,0,0,55,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,115,0,0,0,0,55,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,115,0,0,0,0,55,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,115,0,0,
0,0,0,0,0,0,0,0,0,14,92,152,206,228,247,243,223,195,135,70,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,25,151,243,
255,255,255,255,255,255,255,255,255,255,220,104,2,0,0,0,0,0,0,0,0,0,0,0,0,0,94,239,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,193,28,0,0,0,0,0,0,0,0,0,0,0,120,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,220,27,0,0,0,0,0,
0,0,0,0,90,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,202,2,0,0,0,0,0,0,0,21,237,255,255,255,255,255,
255,255,229,172,137,149,189,252,255,255,255,255,255,255,255,106,0,0,0,0,0,0,0,119,255,255,255,255,255,255,244,101,6,0,0,0,0,43,198,255,
255,255,255,255,255,211,0,0,0,0,0,0,0,209,255,255,255,255,255,252,73,0,0,0,0,0,0,0,21,229,255,255,255,255,255,255,34,0,0,0,
0,0,2,250,255,255,255,255,255,193,0,0,0,0,0,0,0,0,0,134,255,255,255,255,255,255,68,0,0,0,0,0,13,147,147,147,147,147,147,80,
0,0,0,0,0,0,0,0,0,87,255,255,255,255,255,255,78,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,121,
255,255,255,255,255,255,33,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,200,255,255,255,255,255,238,1,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,149,255,255,255,255,255,255,112,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,11,46,98,216,255,255,255,255,255,255,202,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,231,255,255,255,255,255,255,255,255,
255,255,255,170,13,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,231,255,255,255,255,255,255,255,255,251,183,87,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,231,255,255,255,255,255,255,255,255,136,37,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,231,255,255,255,255,255,255,255,255,255,255,221,123,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,231,255,255,255,255,255,255,255,255,
255,255,255,255,198,34,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,25,27,27,39,72,107,202,255,255,255,255,255,255,255,229,13,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,77,245,255,255,255,255,255,255,143,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,83,255,255,255,255,255,255,252,25,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
218,255,255,255,255,255,255,72,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,162,255,255,255,255,255,255,116,0,0,
0,0,149,203,203,203,203,203,203,45,0,0,0,0,0,0,0,0,0,0,186,255,255,255,255,255,255,137,0,0,0,0,157,255,255,255,255,255,255,124,
0,0,0,0,0,0,0,0,0,12,241,255,255,255,255,255,255,107,0,0,0,0,108,255,255,255,255,255,255,242,53,0,0,0,0,0,0,0,1,173,
255,255,255,255,255,255,255,60,0,0,0,0,20,250,255,255,255,255,255,255,239,121,16,0,0,0,0,56,191,255,255,255,255,255,255,255,221,1,0,0,
0,0,0,148,255,255,255,255,255,255,255,255,249,215,183,197,234,255,255,255,255,255,255,255,255,255,97,0,0,0,0,0,0,12,221,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,178,0,0,0,0,0,0,0,0,33,222,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,184,10,0,0,0,0,0,0,0,0,0,25,180,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,251,137,6,0,0,0,0,0,
0,0,0,0,0,0,0,83,198,255,255,255,255,255,255,255,255,255,255,255,248,165,45,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,44,
115,172,215,233,250,243,226,203,151,94,19,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,28,243,255,255,255,
255,255,255,255,219,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,173,255,255,255,255,255,255,255,255,219,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,76,255,255,255,255,255,255,255,255,255,219,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,10,224,255,255,255,255,255,255,255,255,255,219,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,137,255,255,
255,255,255,255,255,255,255,255,219,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,44,251,255,255,255,255,255,255,255,255,255,255,219,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,196,255,255,255,255,255,255,255,255,255,255,255,219,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,100,255,255,255,255,255,252,213,255,255,255,255,255,219,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,21,237,
255,255,255,255,255,142,151,255,255,255,255,255,219,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,161,255,255,255,255,255,228,13,151,255,255,
255,255,255,219,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,64,255,255,255,255,255,255,84,0,151,255,255,255,255,255,219,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,6,215,255,255,255,255,255,184,0,0,151,255,255,255,255,255,219,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
124,255,255,255,255,255,247,36,0,0,151,255,255,255,255,255,219,0,0,0,0,0,0,0,0,0,0,0,0,0,35,247,255,255,255,255,255,126,0,0,
0,151,255,255,255,255,255,219,0,0,0,0,0,0,0,0,0,0,0,0,0,185,255,255,255,255,255,219,7,0,0,0,151,255,255,255,255,255,219,0,
0,0,0,0,0,0,0,0,0,0,0,88,255,255,255,255,255,255,70,0,0,0,0,151,255,255,255,255,255,219,0,0,0,0,0,0,0,0,0,0,
0,15,231,255,255,255,255,255,168,0,0,0,0,0,151,255,255,255,255,255,219,0,0,0,0,0,0,0,0,0,0,0,149,255,255,255,255,255,242,26,
0,0,0,0,0,151,255,255,255,255,255,219,0,0,0,0,0,0,0,0,0,0,53,253,255,255,255,255,255,111,0,0,0,0,0,0,151,255,255,255,
255,255,219,0,0,0,0,0,0,0,0,0,3,206,255,255,255,255,255,207,3,0,0,0,0,0,0,151,255,255,255,255,255,219,0,0,0,0,0,0,
0,0,0,112,255,255,255,255,255,254,56,0,0,0,0,0,0,0,151,255,255,255,255,255,219,0,0,0,0,0,0,0,0,27,242,255,255,255,255,255,
153,0,0,0,0,0,0,0,0,151,255,255,255,255,255,219,0,0,0,0,0,0,0,0,160,255,255,255,255,255,255,156,131,131,131,131,131,131,131,131,
205,255,255,255,255,255,238,131,131,131,131,4,0,0,0,187,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,7,0,0,0,187,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,7,0,0,0,187,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,7,0,0,0,187,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,7,0,0,0,187,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,7,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,183,255,255,255,255,255,219,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,183,255,255,255,255,255,219,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,183,255,255,255,255,255,219,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,183,
255,255,255,255,255,219,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,183,255,255,255,255,255,219,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,183,255,255,255,255,255,219,0,0,0,0,0,0,0,0,0,0,0,216,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,111,0,0,0,0,0,0,0,0,239,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,111,0,0,0,0,0,0,0,8,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,111,0,0,
0,0,0,0,0,31,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,111,0,0,0,0,0,0,0,55,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,111,0,0,0,0,0,0,0,79,255,255,255,255,255,245,171,171,171,171,171,171,171,171,171,171,
171,171,171,171,75,0,0,0,0,0,0,0,103,255,255,255,255,255,204,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,127,255,255,255,255,255,176,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,150,255,255,255,255,255,149,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,174,255,255,255,255,255,121,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,198,255,255,255,255,255,94,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,222,255,255,
255,255,255,66,0,19,105,182,221,247,237,212,159,82,6,0,0,0,0,0,0,0,0,0,0,0,246,255,255,255,255,255,39,105,238,255,255,255,255,255,
255,255,255,221,84,0,0,0,0,0,0,0,0,0,14,255,255,255,255,255,255,148,255,255,255,255,255,255,255,255,255,255,255,255,143,2,0,0,0,0,
0,0,0,38,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,149,0,0,0,0,0,0,0,61,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,88,0,0,0,0,0,0,85,255,255,255,255,255,255,255,232,136,66,37,64,114,207,255,255,255,255,
255,255,255,227,6,0,0,0,0,0,30,110,157,204,247,255,255,165,15,0,0,0,0,0,5,133,254,255,255,255,255,255,255,100,0,0,0,0,0,0,
0,0,0,3,42,78,2,0,0,0,0,0,0,0,0,158,255,255,255,255,255,255,181,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,20,238,255,255,255,255,255,245,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,175,255,255,255,255,255,
255,32,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,131,255,255,255,255,255,255,55,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,103,255,255,255,255,255,255,64,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,140,255,255,255,255,255,255,40,0,0,0,11,251,251,251,251,251,251,174,0,0,0,0,0,0,0,0,0,0,183,255,255,255,255,255,255,15,0,0,
0,0,235,255,255,255,255,255,240,4,0,0,0,0,0,0,0,0,42,250,255,255,255,255,255,208,0,0,0,0,0,179,255,255,255,255,255,255,139,0,
0,0,0,0,0,0,2,192,255,255,255,255,255,255,133,0,0,0,0,0,87,255,255,255,255,255,255,255,136,12,0,0,0,0,32,194,255,255,255,255,
255,255,249,33,0,0,0,0,0,6,213,255,255,255,255,255,255,255,233,165,124,136,186,244,255,255,255,255,255,255,255,145,0,0,0,0,0,0,0,59,
252,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,211,13,0,0,0,0,0,0,0,0,88,252,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,228,31,0,0,0,0,0,0,0,0,0,0,74,231,255,255,255,255,255,255,255,255,255,255,255,255,255,255,190,23,0,0,0,
0,0,0,0,0,0,0,0,0,18,143,241,255,255,255,255,255,255,255,255,255,255,218,96,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
12,90,152,206,229,248,243,222,196,134,66,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,71,157,198,225,249,236,215,170,104,
26,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,109,235,255,255,255,255,255,255,255,255,255,249,157,29,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,35,197,255,255,255,255,255,255,255,255,255,255,255,255,255,239,87,0,0,0,0,0,0,0,0,0,0,0,0,32,227,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,254,101,0,0,0,0,0,0,0,0,0,0,24,221,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,253,71,0,0,0,0,0,0,0,0,0,171,255,255,255,255,255,255,255,255,255,245,255,255,255,255,255,255,255,255,255,226,14,0,0,0,
0,0,0,0,70,255,255,255,255,255,255,255,206,86,14,0,14,66,189,255,255,255,255,255,255,255,116,0,0,0,0,0,0,0,212,255,255,255,255,255,
255,162,6,0,0,0,0,0,0,150,255,255,255,255,255,255,221,0,0,0,0,0,0,56,255,255,255,255,255,255,201,5,0,0,0,0,0,0,0,2,
216,255,255,255,255,255,255,36,0,0,0,0,0,151,255,255,255,255,255,255,58,0,0,0,0,0,0,0,0,0,97,199,199,199,199,199,199,67,0,0,
0,0,1,238,255,255,255,255,255,200,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,39,255,255,255,255,255,255,121,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,90,255,255,255,255,255,255,58,0,0,0,41,140,194,225,248,229,201,
133,50,0,0,0,0,0,0,0,0,0,0,142,255,255,255,255,255,255,18,0,29,187,255,255,255,255,255,255,255,255,255,177,36,0,0,0,0,0,0,
0,0,175,255,255,255,255,255,242,0,68,236,255,255,255,255,255,255,255,255,255,255,255,240,71,0,0,0,0,0,0,0,190,255,255,255,255,255,228,34,
241,255,255,255,255,255,255,255,255,255,255,255,255,255,250,61,0,0,0,0,0,0,205,255,255,255,255,255,231,202,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,231,20,0,0,0,0,0,220,255,255,255,255,255,255,255,255,255,209,95,44,13,56,113,235,255,255,255,255,255,255,255,139,0,0,0,
0,0,219,255,255,255,255,255,255,255,255,137,6,0,0,0,0,0,22,185,255,255,255,255,255,255,246,14,0,0,0,0,200,255,255,255,255,255,255,255,
175,0,0,0,0,0,0,0,0,17,231,255,255,255,255,255,255,81,0,0,0,0,182,255,255,255,255,255,255,248,23,0,0,0,0,0,0,0,0,0,
97,255,255,255,255,255,255,146,0,0,0,0,164,255,255,255,255,255,255,201,0,0,0,0,0,0,0,0,0,0,34,255,255,255,255,255,255,173,0,0,
0,0,129,255,255,255,255,255,255,152,0,0,0,0,0,0,0,0,0,0,0,242,255,255,255,255,255,193,0,0,0,0,67,255,255,255,255,255,255,170,
0,0,0,0,0,0,0,0,0,0,11,253,255,255,255,255,255,176,0,0,0,0,11,251,255,255,255,255,255,219,0,0,0,0,0,0,0,0,0,0,
58,255,255,255,255,255,255,151,0,0,0,0,0,194,255,255,255,255,255,255,68,0,0,0,0,0,0,0,0,0,159,255,255,255,255,255,255,97,0,0,
0,0,0,82,255,255,255,255,255,255,223,13,0,0,0,0,0,0,0,73,255,255,255,255,255,255,253,21,0,0,0,0,0,2,217,255,255,255,255,255,
255,219,58,0,0,0,0,3,116,247,255,255,255,255,255,255,172,0,0,0,0,0,0,0,81,255,255,255,255,255,255,255,253,199,148,121,165,222,255,255,
255,255,255,255,255,250,36,0,0,0,0,0,0,0,0,142,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,110,0,0,0,0,
0,0,0,0,0,4,178,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,139,0,0,0,0,0,0,0,0,0,0,0,2,134,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,246,99,0,0,0,0,0,0,0,0,0,0,0,0,0,0,59,185,255,255,255,255,255,255,255,255,255,255,
250,158,36,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,45,116,182,218,238,250,232,211,166,98,26,0,0,0,0,0,0,0,0,0,
0,83,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,15,0,0,83,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,15,0,0,83,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,15,
0,0,83,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,15,0,0,83,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,15,0,0,56,171,171,171,171,171,171,171,171,171,171,171,171,171,171,171,181,255,255,255,255,255,255,255,
13,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,103,255,255,255,255,255,255,178,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,5,224,255,255,255,255,255,255,52,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,101,255,255,255,255,255,255,181,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,223,255,255,255,255,255,255,55,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,99,255,255,255,255,255,255,183,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,221,255,255,255,255,255,255,57,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,96,255,255,255,255,255,255,186,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,3,219,255,255,255,255,255,255,60,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,94,255,255,255,255,255,255,189,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,217,255,255,255,255,255,255,62,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,92,255,255,255,255,255,255,191,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,216,255,255,255,255,255,255,65,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,90,255,255,255,255,255,255,194,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,2,214,255,255,255,255,255,255,67,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,87,255,255,255,255,255,255,197,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,212,255,255,255,255,255,255,70,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,85,255,255,255,255,255,255,200,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,210,255,255,255,255,255,255,73,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,83,255,255,255,255,255,255,202,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,1,208,255,255,255,255,255,255,75,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,80,255,255,255,255,255,255,205,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,206,255,255,255,255,255,255,78,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,78,255,255,255,255,255,255,207,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,204,255,255,255,255,255,255,81,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,76,255,255,255,255,255,255,209,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,202,255,255,255,255,255,255,83,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,74,255,255,255,255,255,255,212,1,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,200,255,255,255,255,255,255,86,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,12,91,150,204,228,247,245,226,200,144,83,7,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,21,149,242,255,255,
255,255,255,255,255,255,255,255,236,134,12,0,0,0,0,0,0,0,0,0,0,0,0,0,84,237,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
227,67,0,0,0,0,0,0,0,0,0,0,0,103,253,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,249,82,0,0,0,0,0,0,0,
0,0,60,253,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,246,40,0,0,0,0,0,0,0,1,216,255,255,255,255,255,255,255,
204,118,78,82,122,215,255,255,255,255,255,255,255,193,0,0,0,0,0,0,0,65,255,255,255,255,255,255,255,110,1,0,0,0,0,4,133,255,255,255,
255,255,255,255,42,0,0,0,0,0,0,133,255,255,255,255,255,255,157,0,0,0,0,0,0,0,1,182,255,255,255,255,255,255,111,0,0,0,0,0,
0,163,255,255,255,255,255,255,59,0,0,0,0,0,0,0,0,85,255,255,255,255,255,255,142,0,0,0,0,0,0,148,255,255,255,255,255,255,17,0,
0,0,0,0,0,0,0,41,255,255,255,255,255,255,127,0,0,0,0,0,0,104,255,255,255,255,255,255,51,0,0,0,0,0,0,0,0,76,255,255,
255,255,255,255,82,0,0,0,0,0,0,44,254,255,255,255,255,255,130,0,0,0,0,0,0,0,0,156,255,255,255,255,255,248,26,0,0,0,0,0,
0,0,161,255,255,255,255,255,251,64,0,0,0,0,0,0,87,255,255,255,255,255,255,136,0,0,0,0,0,0,0,0,26,230,255,255,255,255,255,253,
151,59,18,21,64,167,255,255,255,255,255,255,218,13,0,0,0,0,0,0,0,0,0,29,210,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,195,19,0,0,0,0,0,0,0,0,0,0,0,13,112,211,255,255,255,255,255,255,255,255,255,255,255,255,205,104,7,0,0,0,0,0,0,0,0,
0,0,0,0,0,14,95,230,255,255,255,255,255,255,255,255,255,255,224,89,10,0,0,0,0,0,0,0,0,0,0,0,0,13,160,247,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,243,144,6,0,0,0,0,0,0,0,0,0,37,213,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,199,26,0,0,0,0,0,0,0,22,236,255,255,255,255,255,255,245,133,69,27,31,74,147,251,255,255,255,255,255,255,223,11,0,0,0,0,0,
0,161,255,255,255,255,255,255,202,28,0,0,0,0,0,0,41,221,255,255,255,255,255,255,140,0,0,0,0,0,58,255,255,255,255,255,255,238,31,0,
0,0,0,0,0,0,0,52,249,255,255,255,255,255,251,42,0,0,0,0,149,255,255,255,255,255,255,140,0,0,0,0,0,0,0,0,0,0,171,255,
255,255,255,255,255,132,0,0,0,0,202,255,255,255,255,255,255,86,0,0,0,0,0,0,0,0,0,0,116,255,255,255,255,255,255,187,0,0,0,0,
221,255,255,255,255,255,255,78,0,0,0,0,0,0,0,0,0,0,107,255,255,255,255,255,255,208,0,0,0,0,199,255,255,255,255,255,255,131,0,0,
0,0,0,0,0,0,0,0,160,255,255,255,255,255,255,186,0,0,0,0,161,255,255,255,255,255,255,228,21,0,0,0,0,0,0,0,0,35,240,255,
255,255,255,255,255,147,0,0,0,0,72,255,255,255,255,255,255,255,189,21,0,0,0,0,0,0,29,206,255,255,255,255,255,255,255,55,0,0,0,0,
2,208,255,255,255,255,255,255,255,241,129,72,33,36,74,139,247,255,255,255,255,255,255,255,193,0,0,0,0,0,0,42,246,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,240,31,0,0,0,0,0,0,0,66,241,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,234,54,0,0,0,0,0,0,0,0,0,45,203,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,192,36,0,0,0,0,0,0,0,
0,0,0,0,2,98,206,255,255,255,255,255,255,255,255,255,255,255,255,196,86,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,46,114,169,
213,231,248,247,230,210,163,109,38,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,33,104,172,212,233,250,234,211,170,101,29,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,46,169,253,255,255,255,255,255,255,255,255,255,252,160,37,0,0,0,0,0,0,0,0,0,0,0,0,0,1,117,251,255,255,255,255,
255,255,255,255,255,255,255,255,255,249,98,0,0,0,0,0,0,0,0,0,0,0,1,163,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,141,0,0,0,0,0,0,0,0,0,0,134,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,102,0,0,0,0,0,0,0,
0,57,255,255,255,255,255,255,255,255,229,177,137,172,223,255,255,255,255,255,255,255,248,48,0,0,0,0,0,0,0,198,255,255,255,255,255,255,246,114,
2,0,0,0,0,93,239,255,255,255,255,255,255,182,0,0,0,0,0,0,46,255,255,255,255,255,255,252,58,0,0,0,0,0,0,0,35,243,255,255,
255,255,255,255,47,0,0,0,0,0,125,255,255,255,255,255,255,139,0,0,0,0,0,0,0,0,0,105,255,255,255,255,255,255,159,0,0,0,0,0,
180,255,255,255,255,255,255,30,0,0,0,0,0,0,0,0,0,3,246,255,255,255,255,255,229,0,0,0,0,0,205,255,255,255,255,255,237,0,0,0,
0,0,0,0,0,0,0,0,200,255,255,255,255,255,255,35,0,0,0,0,222,255,255,255,255,255,214,0,0,0,0,0,0,0,0,0,0,0,181,255,
255,255,255,255,255,97,0,0,0,0,203,255,255,255,255,255,251,8,0,0,0,0,0,0,0,0,0,0,230,255,255,255,255,255,255,134,0,0,0,0,
177,255,255,255,255,255,255,65,0,0,0,0,0,0,0,0,0,42,255,255,255,255,255,255,255,152,0,0,0,0,114,255,255,255,255,255,255,210,5,0,
0,0,0,0,0,0,2,198,255,255,255,255,255,255,255,171,0,0,0,0,38,255,255,255,255,255,255,255,161,12,0,0,0,0,0,11,156,255,255,255,
255,255,255,255,255,189,0,0,0,0,0,175,255,255,255,255,255,255,255,223,101,49,12,48,101,220,255,255,255,255,255,255,255,255,255,192,0,0,0,0,
0,42,247,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,188,248,255,255,255,255,255,175,0,0,0,0,0,0,94,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,230,22,253,255,255,255,255,255,159,0,0,0,0,0,0,0,101,251,255,255,255,255,255,255,255,255,255,255,255,228,53,11,255,
255,255,255,255,255,144,0,0,0,0,0,0,0,0,58,202,255,255,255,255,255,255,255,255,255,177,20,0,44,255,255,255,255,255,255,111,0,0,0,0,
0,0,0,0,0,1,71,151,215,241,254,230,199,138,37,0,0,0,85,255,255,255,255,255,255,60,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
2,0,0,0,0,0,0,0,148,255,255,255,255,255,253,12,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,225,255,
255,255,255,255,210,0,0,0,0,0,99,207,207,207,207,207,207,78,0,0,0,0,0,0,0,0,0,85,255,255,255,255,255,255,120,0,0,0,0,0,
72,255,255,255,255,255,255,191,0,0,0,0,0,0,0,0,14,219,255,255,255,255,255,253,28,0,0,0,0,0,13,244,255,255,255,255,255,255,121,0,
0,0,0,0,0,12,183,255,255,255,255,255,255,182,0,0,0,0,0,0,0,152,255,255,255,255,255,255,255,172,53,7,0,14,93,216,255,255,255,255,
255,255,251,44,0,0,0,0,0,0,0,31,243,255,255,255,255,255,255,255,255,253,241,255,255,255,255,255,255,255,255,255,140,0,0,0,0,0,0,0,
0,0,104,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,203,11,0,0,0,0,0,0,0,0,0,0,134,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,211,18,0,0,0,0,0,0,0,0,0,0,0,0,111,248,255,255,255,255,255,255,255,255,255,255,255,255,255,180,
22,0,0,0,0,0,0,0,0,0,0,0,0,0,0,43,174,253,255,255,255,255,255,255,255,255,255,226,90,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,36,114,180,219,240,248,223,196,150,63,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,20,124,169,141,56,0,0,
0,0,0,0,0,0,0,47,234,255,255,255,253,114,0,0,0,0,0,0,0,0,206,255,255,255,255,255,251,41,0,0,0,0,0,0,23,255,255,255,
255,255,255,255,110,0,0,0,0,0,0,37,255,255,255,255,255,255,255,124,0,0,0,0,0,0,1,235,255,255,255,255,255,255,68,0,0,0,0,0,
0,0,92,255,255,255,255,255,178,0,0,0,0,0,0,0,0,0,74,198,242,215,129,6,0,0,0,0,0,0,0,85,99,99,99,99,99,99,99,99,
99,99,99,99,99,99,35,0,0,0,0,0,0,219,255,255,255,255,255,255,255,255,255,255,255,255,255,255,91,0,0,0,0,0,0,219,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,91,0,0,0,0,0,0,219,255,255,255,255,255,255,255,255,255,255,255,255,255,255,91,0,0,0,0,0,0,219,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,91,0,0,0,0,0,0,219,255,255,255,255,255,255,255,255,255,255,255,255,255,255,91,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0
};
const AAGlyph F46_g[] PROGMEM = {
{48,31,34,31,0,34,0},
{49,20,34,20,0,34,1054},
{50,29,34,29,0,34,1734},
{51,30,34,30,0,34,2720},
{52,31,34,31,0,34,3740},
{53,29,34,29,0,34,4794},
{54,30,34,30,0,34,5780},
{55,27,34,27,0,34,6800},
{56,30,34,30,0,34,7718},
{57,30,35,30,0,35,8738},
{46,15,8,15,0,8,9788},
{45,22,17,22,0,17,9908}
};
const AAFont F46 = { 12, F46_g, F46_bmp };
// F30: Inter-Bold.otf 30px, 16 glyphs, 6129 bytes
const uint8_t F30_bmp[] PROGMEM = {
0,0,0,0,0,0,62,151,214,239,244,219,168,83,2,0,0,0,0,0,0,0,0,0,20,172,255,255,255,255,255,255,255,255,202,42,0,0,0,0,
0,0,0,18,208,255,255,255,255,255,255,255,255,255,255,235,45,0,0,0,0,0,0,173,255,255,255,255,255,221,209,254,255,255,255,255,218,8,0,0,
0,0,70,255,255,255,255,229,46,0,0,24,199,255,255,255,255,124,0,0,0,0,176,255,255,255,249,44,0,0,0,0,17,222,255,255,255,228,2,0,
0,15,251,255,255,255,176,0,0,0,0,0,0,122,255,255,255,255,65,0,0,66,255,255,255,255,87,0,0,0,0,0,0,32,255,255,255,255,120,0,
0,115,255,255,255,255,35,0,0,0,0,0,0,0,237,255,255,255,169,0,0,136,255,255,255,255,13,0,0,0,0,0,0,0,214,255,255,255,190,0,
0,152,255,255,255,247,0,0,0,0,0,0,0,0,192,255,255,255,206,0,0,153,255,255,255,246,0,0,0,0,0,0,0,0,191,255,255,255,208,0,
0,138,255,255,255,255,12,0,0,0,0,0,0,0,214,255,255,255,191,0,0,117,255,255,255,255,35,0,0,0,0,0,0,0,237,255,255,255,171,0,
0,69,255,255,255,255,84,0,0,0,0,0,0,31,255,255,255,255,123,0,0,17,252,255,255,255,173,0,0,0,0,0,0,121,255,255,255,255,69,0,
0,0,182,255,255,255,248,42,0,0,0,0,16,221,255,255,255,232,4,0,0,0,77,255,255,255,255,227,43,0,0,22,197,255,255,255,255,131,0,0,
0,0,0,180,255,255,255,255,255,220,208,254,255,255,255,255,224,11,0,0,0,0,0,22,213,255,255,255,255,255,255,255,255,255,255,239,52,0,0,0,
0,0,0,0,22,179,255,255,255,255,255,255,255,255,208,48,0,0,0,0,0,0,0,0,0,0,66,155,216,241,245,221,171,88,3,0,0,0,0,0,
0,0,0,0,5,133,253,255,255,255,247,0,0,0,0,0,40,204,255,255,255,255,255,247,0,0,0,0,104,245,255,255,255,255,255,255,247,0,0,0,
136,255,255,255,255,253,255,255,255,247,0,0,0,163,255,255,254,142,135,255,255,255,247,0,0,0,163,255,231,70,0,127,255,255,255,247,0,0,0,163,
178,20,0,0,127,255,255,255,247,0,0,0,47,0,0,0,0,127,255,255,255,247,0,0,0,0,0,0,0,0,127,255,255,255,247,0,0,0,0,0,
0,0,0,127,255,255,255,247,0,0,0,0,0,0,0,0,127,255,255,255,247,0,0,0,0,0,0,0,0,127,255,255,255,247,0,0,0,0,0,0,
0,0,127,255,255,255,247,0,0,0,0,0,0,0,0,127,255,255,255,247,0,0,0,0,0,0,0,0,127,255,255,255,247,0,0,0,0,0,0,0,
0,127,255,255,255,247,0,0,0,0,0,0,0,0,127,255,255,255,247,0,0,0,0,0,0,0,0,127,255,255,255,247,0,0,0,0,0,0,0,0,
127,255,255,255,247,0,0,0,0,0,0,0,0,127,255,255,255,247,0,0,0,0,0,0,0,0,127,255,255,255,247,0,0,0,0,0,0,0,0,127,
255,255,255,247,0,0,0,0,0,0,0,37,141,183,224,243,207,168,112,9,0,0,0,0,0,0,0,0,12,137,249,255,255,255,255,255,255,255,227,93,
0,0,0,0,0,0,0,175,255,255,255,255,255,255,255,255,255,255,255,93,0,0,0,0,0,103,255,255,255,255,241,180,161,209,255,255,255,255,245,43,
0,0,0,4,238,255,255,255,203,25,0,0,0,96,255,255,255,255,157,0,0,0,59,255,255,255,255,49,0,0,0,0,0,168,255,255,255,218,0,0,
0,85,247,247,247,236,1,0,0,0,0,0,110,255,255,255,235,0,0,0,0,0,0,0,0,0,0,0,0,0,0,130,255,255,255,204,0,0,0,0,
0,0,0,0,0,0,0,0,0,4,219,255,255,255,141,0,0,0,0,0,0,0,0,0,0,0,0,0,132,255,255,255,249,27,0,0,0,0,0,0,
0,0,0,0,0,0,98,255,255,255,255,118,0,0,0,0,0,0,0,0,0,0,0,0,96,253,255,255,255,170,0,0,0,0,0,0,0,0,0,0,
0,0,110,254,255,255,255,198,11,0,0,0,0,0,0,0,0,0,0,0,128,255,255,255,255,200,14,0,0,0,0,0,0,0,0,0,0,1,146,255,
255,255,255,190,12,0,0,0,0,0,0,0,0,0,0,4,163,255,255,255,255,184,8,0,0,0,0,0,0,0,0,0,0,8,178,255,255,255,255,179,
7,0,0,0,0,0,0,0,0,0,0,14,192,255,255,255,255,174,5,0,0,0,0,0,0,0,0,0,0,14,205,255,255,255,255,255,177,171,171,171,
171,171,171,171,171,42,0,0,47,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,63,0,0,47,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,63,0,0,47,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,63,0,0,0,0,0,0,10,113,169,208,244,223,184,142,37,0,0,
0,0,0,0,0,0,0,107,232,255,255,255,255,255,255,255,250,147,14,0,0,0,0,0,0,123,255,255,255,255,255,255,255,255,255,255,255,180,1,0,
0,0,0,79,254,255,255,255,254,194,158,179,245,255,255,255,255,124,0,0,0,0,205,255,255,255,230,46,0,0,0,25,213,255,255,255,235,3,0,0,
10,255,255,255,255,93,0,0,0,0,0,78,255,255,255,255,33,0,0,5,47,47,47,47,9,0,0,0,0,0,54,255,255,255,255,23,0,0,0,0,
0,0,0,0,0,0,0,0,0,142,255,255,255,221,0,0,0,0,0,0,0,0,0,0,0,9,46,151,255,255,255,255,78,0,0,0,0,0,0,0,
0,0,195,255,255,255,255,255,255,236,81,0,0,0,0,0,0,0,0,0,0,195,255,255,255,255,255,153,15,0,0,0,0,0,0,0,0,0,0,0,
195,255,255,255,255,255,255,246,149,10,0,0,0,0,0,0,0,0,0,64,83,93,126,208,255,255,255,255,207,6,0,0,0,0,0,0,0,0,0,0,
0,0,1,129,255,255,255,255,122,0,0,0,0,0,0,0,0,0,0,0,0,0,0,223,255,255,255,201,0,0,51,95,95,95,94,0,0,0,0,0,
0,0,188,255,255,255,240,0,0,115,255,255,255,255,54,0,0,0,0,0,14,238,255,255,255,225,0,0,49,255,255,255,255,223,56,0,0,0,32,188,
255,255,255,255,159,0,0,0,170,255,255,255,255,255,220,188,210,250,255,255,255,255,249,44,0,0,0,9,203,255,255,255,255,255,255,255,255,255,255,255,
253,98,0,0,0,0,0,21,150,249,255,255,255,255,255,255,255,255,198,65,0,0,0,0,0,0,0,0,31,135,178,214,246,225,189,153,70,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,184,255,255,255,255,255,51,0,0,0,0,0,0,0,0,0,0,0,0,90,255,255,255,255,255,255,51,0,
0,0,0,0,0,0,0,0,0,0,17,233,255,255,255,255,255,255,51,0,0,0,0,0,0,0,0,0,0,0,156,255,255,255,255,255,255,255,51,0,
0,0,0,0,0,0,0,0,0,63,254,255,255,252,255,255,255,255,51,0,0,0,0,0,0,0,0,0,7,216,255,255,255,118,255,255,255,255,51,0,
0,0,0,0,0,0,0,0,128,255,255,255,209,4,255,255,255,255,51,0,0,0,0,0,0,0,0,40,249,255,255,253,56,0,255,255,255,255,51,0,
0,0,0,0,0,0,1,193,255,255,255,148,0,0,255,255,255,255,51,0,0,0,0,0,0,0,100,255,255,255,229,14,0,0,255,255,255,255,51,0,
0,0,0,0,0,23,238,255,255,255,83,0,0,0,255,255,255,255,51,0,0,0,0,0,0,167,255,255,255,178,0,0,0,0,255,255,255,255,51,0,
0,0,0,0,72,255,255,255,244,30,0,0,0,0,255,255,255,255,51,0,0,0,0,10,222,255,255,255,113,0,0,0,0,0,255,255,255,255,51,0,
0,0,0,117,255,255,255,255,155,151,151,151,151,151,255,255,255,255,173,151,142,0,0,135,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
239,0,0,135,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,239,0,0,135,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
239,0,0,0,0,0,0,0,0,0,0,0,0,19,255,255,255,255,51,0,0,0,0,0,0,0,0,0,0,0,0,0,0,19,255,255,255,255,51,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,19,255,255,255,255,51,0,0,0,0,0,0,0,0,0,0,0,0,0,0,19,255,255,255,255,51,0,
0,0,0,0,0,167,255,255,255,255,255,255,255,255,255,255,255,255,151,0,0,0,0,0,190,255,255,255,255,255,255,255,255,255,255,255,255,151,0,0,
0,0,0,213,255,255,255,255,255,255,255,255,255,255,255,255,151,0,0,0,0,0,236,255,255,255,185,179,179,179,179,179,179,179,179,106,0,0,0,0,
4,254,255,255,254,5,0,0,0,0,0,0,0,0,0,0,0,0,0,26,255,255,255,234,0,0,0,0,0,0,0,0,0,0,0,0,0,0,49,255,
255,255,209,0,0,0,0,0,0,0,0,0,0,0,0,0,0,72,255,255,255,185,0,0,33,68,65,31,0,0,0,0,0,0,0,0,95,255,255,255,
160,72,210,255,255,255,255,209,60,0,0,0,0,0,0,117,255,255,255,215,253,255,255,255,255,255,255,253,139,0,0,0,0,0,140,255,255,255,255,255,
255,255,255,255,255,255,255,255,97,0,0,0,0,122,230,255,255,254,147,58,13,40,126,248,255,255,255,238,24,0,0,0,0,0,17,60,55,0,0,0,
0,0,82,255,255,255,255,116,0,0,0,0,0,0,0,0,0,0,0,0,0,0,204,255,255,255,174,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,155,255,255,255,197,0,0,7,67,67,67,67,18,0,0,0,0,0,0,175,255,255,255,181,0,0,7,254,255,255,255,117,0,0,0,0,0,18,236,
255,255,255,138,0,0,0,197,255,255,255,241,57,0,0,0,18,180,255,255,255,255,53,0,0,0,69,252,255,255,255,254,189,147,170,235,255,255,255,255,
175,0,0,0,0,0,113,255,255,255,255,255,255,255,255,255,255,255,214,18,0,0,0,0,0,0,101,229,255,255,255,255,255,255,255,250,139,12,0,0,
0,0,0,0,0,0,9,112,170,208,245,240,211,148,42,0,0,0,0,0,0,0,0,0,0,0,41,136,208,237,240,198,156,74,0,0,0,0,0,0,
0,0,0,4,135,251,255,255,255,255,255,255,255,185,39,0,0,0,0,0,0,2,167,255,255,255,255,255,255,255,255,255,255,231,26,0,0,0,0,0,
123,255,255,255,255,255,251,233,255,255,255,255,255,200,5,0,0,0,31,246,255,255,255,235,78,12,0,31,179,255,255,255,255,75,0,0,0,136,255,255,
255,241,46,0,0,0,0,6,218,255,255,255,156,0,0,2,229,255,255,255,148,0,0,0,0,0,0,49,83,83,83,69,0,0,40,255,255,255,255,50,
0,0,7,39,60,38,1,0,0,0,0,0,0,91,255,255,255,251,1,21,152,242,255,255,255,231,112,1,0,0,0,0,126,255,255,255,227,45,230,255,
255,255,255,255,255,255,193,24,0,0,0,142,255,255,255,218,215,255,255,255,255,255,255,255,255,255,180,0,0,0,155,255,255,255,255,255,232,94,26,24,
87,226,255,255,255,255,89,0,0,144,255,255,255,255,239,29,0,0,0,0,23,237,255,255,255,197,0,0,126,255,255,255,255,124,0,0,0,0,0,0,
124,255,255,255,252,3,0,97,255,255,255,255,71,0,0,0,0,0,0,75,255,255,255,255,21,0,37,255,255,255,255,93,0,0,0,0,0,0,99,255,
255,255,254,6,0,0,223,255,255,255,183,0,0,0,0,0,0,186,255,255,255,215,0,0,0,112,255,255,255,255,132,3,0,0,4,135,255,255,255,255,
130,0,0,0,11,218,255,255,255,255,219,159,160,220,255,255,255,255,234,16,0,0,0,0,43,239,255,255,255,255,255,255,255,255,255,255,246,60,0,0,
0,0,0,0,37,207,255,255,255,255,255,255,255,255,183,36,0,0,0,0,0,0,0,0,1,80,165,222,245,246,223,168,79,0,0,0,0,0,0,231,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,87,0,231,255,255,255,255,255,255,255,255,255,255,255,255,255,255,87,0,231,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,87,0,163,179,179,179,179,179,179,179,179,179,206,255,255,255,255,84,0,0,0,0,0,0,0,0,0,0,0,168,255,255,
255,236,13,0,0,0,0,0,0,0,0,0,0,42,253,255,255,255,121,0,0,0,0,0,0,0,0,0,0,0,168,255,255,255,236,13,0,0,0,0,
0,0,0,0,0,0,42,253,255,255,255,121,0,0,0,0,0,0,0,0,0,0,0,169,255,255,255,237,13,0,0,0,0,0,0,0,0,0,0,43,
254,255,255,255,121,0,0,0,0,0,0,0,0,0,0,0,169,255,255,255,237,13,0,0,0,0,0,0,0,0,0,0,43,254,255,255,255,121,0,0,
0,0,0,0,0,0,0,0,0,170,255,255,255,237,13,0,0,0,0,0,0,0,0,0,0,44,254,255,255,255,122,0,0,0,0,0,0,0,0,0,
0,0,170,255,255,255,237,13,0,0,0,0,0,0,0,0,0,0,44,254,255,255,255,122,0,0,0,0,0,0,0,0,0,0,0,170,255,255,255,237,
13,0,0,0,0,0,0,0,0,0,0,44,254,255,255,255,122,0,0,0,0,0,0,0,0,0,0,0,171,255,255,255,237,13,0,0,0,0,0,0,
0,0,0,0,45,254,255,255,255,122,0,0,0,0,0,0,0,0,0,0,0,171,255,255,255,237,13,0,0,0,0,0,0,0,0,0,0,45,254,255,
255,255,123,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,111,171,208,244,226,189,149,48,0,0,0,0,0,0,0,0,0,0,104,231,255,255,
255,255,255,255,255,254,174,30,0,0,0,0,0,0,0,108,255,255,255,255,255,255,255,255,255,255,255,215,17,0,0,0,0,0,61,253,255,255,255,251,
167,120,137,218,255,255,255,255,184,0,0,0,0,0,166,255,255,255,249,58,0,0,0,5,177,255,255,255,255,37,0,0,0,0,206,255,255,255,172,0,
0,0,0,0,46,255,255,255,255,77,0,0,0,0,182,255,255,255,154,0,0,0,0,0,27,255,255,255,255,51,0,0,0,0,115,255,255,255,224,12,
0,0,0,0,111,255,255,255,232,8,0,0,0,0,7,216,255,255,255,197,60,12,29,119,248,255,255,255,93,0,0,0,0,0,0,22,191,255,255,255,
255,255,255,255,255,255,242,96,0,0,0,0,0,0,0,0,3,115,252,255,255,255,255,255,255,210,39,0,0,0,0,0,0,0,0,67,229,255,255,255,
255,255,255,255,255,255,253,160,9,0,0,0,0,0,83,252,255,255,255,231,127,85,103,176,255,255,255,255,201,5,0,0,0,15,232,255,255,255,207,13,
0,0,0,0,96,255,255,255,255,122,0,0,0,93,255,255,255,255,64,0,0,0,0,0,0,194,255,255,255,225,0,0,0,141,255,255,255,255,17,0,
0,0,0,0,0,146,255,255,255,255,20,0,0,141,255,255,255,255,57,0,0,0,0,0,0,185,255,255,255,255,19,0,0,85,255,255,255,255,198,9,
0,0,0,0,81,254,255,255,255,218,0,0,0,6,216,255,255,255,255,227,126,87,104,170,254,255,255,255,255,99,0,0,0,0,46,234,255,255,255,255,
255,255,255,255,255,255,255,255,155,0,0,0,0,0,0,33,163,252,255,255,255,255,255,255,255,255,221,100,0,0,0,0,0,0,0,0,0,35,137,178,
213,245,230,195,160,91,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,22,131,199,234,
250,232,192,122,20,0,0,0,0,0,0,0,0,2,104,239,255,255,255,255,255,255,255,244,109,0,0,0,0,0,0,3,177,255,255,255,255,255,255,255,
255,255,255,255,141,0,0,0,0,0,125,255,255,255,255,252,197,160,198,252,255,255,255,255,87,0,0,0,16,244,255,255,255,225,53,0,0,0,55,225,
255,255,255,224,4,0,0,91,255,255,255,255,61,0,0,0,0,0,63,255,255,255,255,86,0,0,137,255,255,255,222,0,0,0,0,0,0,0,224,255,
255,255,157,0,0,154,255,255,255,197,0,0,0,0,0,0,0,199,255,255,255,218,0,0,134,255,255,255,240,4,0,0,0,0,0,7,244,255,255,255,
249,0,0,78,255,255,255,255,123,0,0,0,0,0,138,255,255,255,255,255,11,0,7,218,255,255,255,254,149,45,12,51,163,255,255,255,255,255,255,23,
0,0,64,254,255,255,255,255,255,255,255,255,255,180,255,255,255,255,8,0,0,0,111,248,255,255,255,255,255,255,255,148,101,255,255,255,249,0,0,0,
0,0,44,193,254,255,255,255,206,87,0,126,255,255,255,213,0,0,0,0,0,0,0,22,57,59,27,0,0,0,179,255,255,255,162,0,0,29,83,83,
83,83,6,0,0,0,0,0,26,252,255,255,255,98,0,0,37,255,255,255,255,94,0,0,0,0,1,158,255,255,255,245,13,0,0,0,213,255,255,255,
243,85,1,0,35,157,255,255,255,255,144,0,0,0,0,88,255,255,255,255,255,244,232,255,255,255,255,255,230,15,0,0,0,0,0,138,255,255,255,255,
255,255,255,255,255,255,241,52,0,0,0,0,0,0,3,116,241,255,255,255,255,255,255,255,210,49,0,0,0,0,0,0,0,0,0,22,131,179,221,247,
224,179,91,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,125,238,237,125,0,0,0,0,0,72,255,255,255,255,71,0,0,0,
0,123,255,255,255,255,123,0,0,0,0,69,255,255,255,255,68,0,0,0,0,0,118,231,231,118,0,0,0,0,0,125,131,131,131,131,131,131,131,131,
127,0,0,0,0,243,255,255,255,255,255,255,255,255,247,0,0,0,0,243,255,255,255,255,255,255,255,255,247,0,0,0,0,243,255,255,255,255,255,255,
255,255,247,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,54,135,202,230,249,233,207,142,64,0,
0,0,0,0,0,0,0,0,0,0,0,38,182,255,255,255,255,255,255,255,255,255,195,50,0,0,0,0,0,0,0,0,0,69,240,255,255,255,255,255,
255,255,255,255,255,255,247,88,0,0,0,0,0,0,0,50,247,255,255,255,255,255,255,255,255,255,255,255,255,255,253,70,0,0,0,0,0,8,222,255,
255,255,255,232,105,49,9,44,96,221,255,255,255,255,236,20,0,0,0,0,109,255,255,255,255,197,25,0,0,0,0,0,16,179,255,255,255,255,134,0,
0,0,0,222,255,255,255,249,30,0,0,0,0,0,0,0,15,238,255,255,255,241,7,0,0,38,255,255,255,255,146,0,0,0,0,0,0,0,0,0,
121,255,255,255,255,64,0,0,100,255,255,255,255,61,0,0,0,0,0,0,0,0,0,38,255,255,255,255,127,0,0,128,255,255,255,255,27,0,0,0,
0,0,0,0,0,0,6,253,255,255,255,155,0,0,149,255,255,255,247,1,0,0,0,0,0,0,0,0,0,0,225,255,255,255,176,0,0,150,255,255,
255,247,1,0,0,0,0,0,0,0,0,0,0,224,255,255,255,177,0,0,129,255,255,255,255,26,0,0,0,0,0,0,0,0,0,6,253,255,255,255,
156,0,0,102,255,255,255,255,60,0,0,0,0,0,0,0,0,0,37,255,255,255,255,128,0,0,39,255,255,255,255,142,0,0,0,0,0,0,0,0,
0,118,255,255,255,255,65,0,0,0,223,255,255,255,247,28,0,0,0,0,0,0,0,14,237,255,255,255,243,7,0,0,0,111,255,255,255,255,192,21,
0,0,0,0,0,13,175,255,255,255,255,137,0,0,0,0,9,224,255,255,255,255,229,98,43,5,39,91,218,255,255,255,255,237,22,0,0,0,0,0,
51,248,255,255,255,255,255,255,254,255,255,255,255,255,255,253,73,0,0,0,0,0,0,0,71,241,255,255,255,255,255,255,255,255,255,255,255,248,91,0,
0,0,0,0,0,0,0,0,40,184,255,255,255,255,255,255,255,255,255,198,52,0,0,0,0,0,0,0,0,0,0,0,0,55,137,204,232,250,234,208,
145,66,0,0,0,0,0,0,0,32,255,255,255,255,212,0,0,0,0,0,0,0,0,0,0,109,255,255,255,255,127,0,198,255,255,255,255,41,0,0,
0,0,0,0,0,0,0,190,255,255,255,255,41,0,109,255,255,255,255,124,0,0,0,0,0,0,0,0,19,252,255,255,255,210,0,0,23,252,255,255,
255,208,0,0,0,0,0,0,0,0,96,255,255,255,255,123,0,0,0,187,255,255,255,255,37,0,0,0,0,0,0,0,178,255,255,255,255,37,0,0,
0,98,255,255,255,255,120,0,0,0,0,0,0,11,247,255,255,255,205,0,0,0,0,16,249,255,255,255,204,0,0,0,0,0,0,84,255,255,255,255,
118,0,0,0,0,0,176,255,255,255,255,33,0,0,0,0,0,166,255,255,255,255,32,0,0,0,0,0,87,255,255,255,255,116,0,0,0,0,6,241,
255,255,255,200,0,0,0,0,0,0,10,243,255,255,255,200,0,0,0,0,72,255,255,255,255,113,0,0,0,0,0,0,0,165,255,255,255,255,29,0,
0,0,153,255,255,255,254,28,0,0,0,0,0,0,0,75,255,255,255,255,104,0,0,0,226,255,255,255,195,0,0,0,0,0,0,0,0,5,237,255,
255,255,179,0,0,42,255,255,255,255,109,0,0,0,0,0,0,0,0,0,153,255,255,255,245,8,0,114,255,255,255,253,25,0,0,0,0,0,0,0,
0,0,64,255,255,255,255,70,0,184,255,255,255,191,0,0,0,0,0,0,0,0,0,0,2,229,255,255,255,137,5,244,255,255,255,104,0,0,0,0,
0,0,0,0,0,0,0,142,255,255,255,205,58,255,255,255,252,21,0,0,0,0,0,0,0,0,0,0,0,53,255,255,255,253,142,255,255,255,186,0,
0,0,0,0,0,0,0,0,0,0,0,0,219,255,255,255,246,255,255,255,99,0,0,0,0,0,0,0,0,0,0,0,0,0,131,255,255,255,255,255,
255,250,18,0,0,0,0,0,0,0,0,0,0,0,0,0,42,255,255,255,255,255,255,181,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,208,
255,255,255,255,255,94,0,0,0,0,0,0,0,0,7,255,255,255,255,255,255,255,255,255,255,255,255,255,255,147,0,0,7,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,147,0,0,7,255,255,255,255,255,255,255,255,255,255,255,255,255,255,147,0,0,7,255,255,255,255,215,183,183,183,183,183,183,
183,183,183,106,0,0,7,255,255,255,255,111,0,0,0,0,0,0,0,0,0,0,0,0,7,255,255,255,255,111,0,0,0,0,0,0,0,0,0,0,
0,0,7,255,255,255,255,111,0,0,0,0,0,0,0,0,0,0,0,0,7,255,255,255,255,111,0,0,0,0,0,0,0,0,0,0,0,0,7,255,
255,255,255,111,0,0,0,0,0,0,0,0,0,0,0,0,7,255,255,255,255,255,255,255,255,255,255,255,255,255,207,0,0,0,7,255,255,255,255,255,
255,255,255,255,255,255,255,255,207,0,0,0,7,255,255,255,255,255,255,255,255,255,255,255,255,255,207,0,0,0,7,255,255,255,255,206,167,167,167,167,
167,167,167,167,136,0,0,0,7,255,255,255,255,111,0,0,0,0,0,0,0,0,0,0,0,0,7,255,255,255,255,111,0,0,0,0,0,0,0,0,
0,0,0,0,7,255,255,255,255,111,0,0,0,0,0,0,0,0,0,0,0,0,7,255,255,255,255,111,0,0,0,0,0,0,0,0,0,0,0,0,
7,255,255,255,255,111,0,0,0,0,0,0,0,0,0,0,0,0,7,255,255,255,255,213,179,179,179,179,179,179,179,179,179,109,0,0,7,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,155,0,0,7,255,255,255,255,255,255,255,255,255,255,255,255,255,255,155,0,0,7,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,155,0,0,7,255,255,255,255,255,255,255,255,253,235,204,142,30,0,0,0,0,0,0,7,255,255,255,255,255,255,255,255,255,
255,255,255,246,125,8,0,0,0,0,7,255,255,255,255,255,255,255,255,255,255,255,255,255,255,159,0,0,0,0,7,255,255,255,255,213,179,179,182,207,
243,255,255,255,255,255,76,0,0,0,7,255,255,255,255,111,0,0,0,0,12,134,255,255,255,255,215,0,0,0,7,255,255,255,255,111,0,0,0,0,
0,0,181,255,255,255,255,28,0,0,7,255,255,255,255,111,0,0,0,0,0,0,110,255,255,255,255,57,0,0,7,255,255,255,255,111,0,0,0,0,
0,0,102,255,255,255,255,58,0,0,7,255,255,255,255,111,0,0,0,0,0,0,161,255,255,255,255,27,0,0,7,255,255,255,255,111,0,0,0,0,
2,100,254,255,255,255,223,1,0,0,7,255,255,255,255,199,155,155,157,180,224,255,255,255,255,255,115,0,0,0,7,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,189,5,0,0,0,7,255,255,255,255,255,255,255,255,255,255,255,255,255,163,15,0,0,0,0,7,255,255,255,255,255,255,255,255,255,
255,255,255,159,0,0,0,0,0,0,7,255,255,255,255,111,0,0,0,201,255,255,255,245,25,0,0,0,0,0,7,255,255,255,255,111,0,0,0,69,
255,255,255,255,151,0,0,0,0,0,7,255,255,255,255,111,0,0,0,0,192,255,255,255,251,37,0,0,0,0,7,255,255,255,255,111,0,0,0,0,
59,255,255,255,255,168,0,0,0,0,7,255,255,255,255,111,0,0,0,0,0,182,255,255,255,254,51,0,0,0,7,255,255,255,255,111,0,0,0,0,
0,50,254,255,255,255,185,0,0,0,7,255,255,255,255,111,0,0,0,0,0,0,172,255,255,255,255,66,0,0,7,255,255,255,255,111,0,0,0,0,
0,0,42,253,255,255,255,202,0
};
const AAGlyph F30_g[] PROGMEM = {
{48,20,22,20,0,22,0},
{49,13,22,13,0,22,440},
{50,19,22,19,0,22,726},
{51,19,22,19,0,22,1144},
{52,20,22,20,0,22,1562},
{53,19,22,19,0,22,2002},
{54,19,22,19,0,22,2420},
{55,17,22,17,0,22,2838},
{56,20,22,20,0,22,3212},
{57,19,23,19,0,23,3652},
{46,10,6,10,0,6,4089},
{45,14,11,14,0,11,4149},
{79,23,22,23,0,22,4303},
{86,22,22,22,0,22,4809},
{69,18,22,18,0,22,5293},
{82,20,22,20,0,22,5689}
};
const AAFont F30 = { 16, F30_g, F30_bmp };
// F17: Inter-SemiBold.otf 17px, 95 glyphs, 11591 bytes
const uint8_t F17_bmp[] PROGMEM = {
0,114,255,224,0,0,109,255,220,0,0,105,255,215,0,0,101,255,211,0,0,97,255,206,0,0,93,255,202,0,0,89,255,197,0,0,85,255,193,0,
0,64,199,148,0,0,0,0,0,0,0,20,126,69,0,0,130,255,242,3,0,63,226,153,0,0,74,255,205,0,238,255,41,0,0,62,255,194,0,227,
255,29,0,0,50,255,183,0,216,255,17,0,0,38,255,172,0,205,255,5,0,0,26,255,161,0,194,250,0,0,0,0,3,2,0,2,3,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,156,255,15,0,68,255,103,0,0,0,0,197,230,0,0,
109,255,63,0,0,0,0,238,189,0,0,150,255,24,0,0,144,175,255,225,171,171,230,255,172,111,2,247,255,255,255,255,255,255,255,255,131,0,0,103,
255,68,0,18,255,156,0,0,0,0,140,255,31,0,54,255,120,0,0,0,0,177,248,2,0,90,255,85,0,0,153,255,255,255,255,255,255,255,255,234,
0,130,181,255,225,175,175,234,252,175,136,0,0,44,255,130,0,0,212,218,0,0,0,0,84,255,89,0,3,249,177,0,0,0,0,124,255,48,0,36,
255,136,0,0,0,0,0,0,0,0,129,8,0,0,0,0,0,0,0,0,0,235,15,0,0,0,0,0,0,22,150,221,255,230,169,42,0,0,0,26,
233,255,255,255,255,255,250,61,0,0,156,255,216,49,235,52,185,255,209,0,0,209,255,115,0,235,15,36,163,162,4,0,186,255,195,24,235,15,0,0,
0,0,0,68,253,255,247,251,106,20,0,0,0,0,0,64,200,255,255,255,251,148,11,0,0,0,0,0,33,241,200,255,255,172,0,0,0,0,0,0,
235,15,74,255,255,24,28,231,231,21,0,235,15,8,253,255,52,0,227,255,181,44,235,51,165,255,247,9,0,73,253,255,255,255,255,255,255,106,0,0,
0,49,173,228,255,233,182,66,0,0,0,0,0,0,0,235,15,0,0,0,0,0,0,0,0,0,129,8,0,0,0,0,0,0,78,216,240,170,17,0,
0,0,0,104,253,58,0,0,0,0,24,246,206,135,250,149,0,0,0,29,242,144,0,0,0,0,0,78,255,74,0,197,210,0,0,0,182,222,11,0,
0,0,0,0,78,255,74,0,198,210,0,0,94,255,66,0,0,0,0,0,0,25,247,204,134,250,149,0,23,237,154,0,0,0,0,0,0,0,0,82,
218,241,171,17,0,172,228,15,0,0,0,0,0,0,0,0,0,0,0,0,0,84,255,76,0,0,0,0,0,0,0,0,0,0,0,0,0,18,232,163,
0,10,158,238,220,89,0,0,0,0,0,0,0,0,163,233,19,0,129,254,146,202,250,32,0,0,0,0,0,0,74,255,85,0,0,194,215,0,63,255,
90,0,0,0,0,0,14,227,174,0,0,0,204,201,0,49,255,100,0,0,0,0,0,153,238,24,0,0,0,167,245,47,135,255,61,0,0,0,0,65,
254,95,0,0,0,0,48,239,255,255,180,0,0,0,0,0,0,0,0,0,0,0,0,0,23,83,65,2,0,0,0,0,20,159,231,234,164,23,0,0,
0,0,0,0,191,255,242,235,255,191,0,0,0,0,0,28,255,236,12,5,220,255,20,0,0,0,0,31,255,211,0,3,224,245,7,0,0,0,0,0,
225,255,87,174,255,161,0,0,0,0,0,0,86,255,255,255,191,8,0,0,0,0,0,0,98,254,255,235,12,0,7,15,6,0,0,117,255,240,203,255,
157,0,140,255,79,0,21,250,255,59,18,219,255,110,193,255,43,0,56,255,234,0,0,39,238,252,254,231,6,0,18,254,254,75,0,0,133,255,255,123,
0,0,0,140,255,255,224,227,255,254,255,213,14,0,0,2,101,204,242,228,175,49,152,255,179,2,0,74,255,205,0,0,0,62,255,194,0,0,0,50,
255,183,0,0,0,38,255,172,0,0,0,26,255,161,0,0,0,0,3,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,148,255,130,0,0,25,248,255,41,0,0,113,255,
207,0,0,0,197,255,117,0,0,8,251,255,45,0,0,55,255,248,4,0,0,90,255,216,0,0,0,109,255,197,0,0,0,112,255,202,0,0,0,93,
255,236,0,0,0,54,255,255,16,0,0,6,247,255,58,0,0,0,173,255,136,0,0,0,78,255,223,2,0,0,1,209,255,73,0,0,0,51,115,68,
51,255,224,5,0,0,0,216,255,99,0,0,0,126,255,195,0,0,0,36,255,254,26,0,0,0,218,255,85,0,0,0,170,255,138,0,0,0,133,255,
172,0,0,0,114,255,191,0,0,0,118,255,195,0,0,0,153,255,176,0,0,0,189,255,137,0,0,1,231,255,80,0,0,56,255,244,11,0,0,145,
255,160,0,0,10,239,252,40,0,0,32,115,89,0,0,0,0,0,0,33,255,71,0,0,0,0,72,132,25,255,58,104,106,0,0,109,245,217,255,217,
252,146,0,0,0,46,236,255,248,74,0,0,0,111,246,212,255,212,252,148,0,0,70,128,24,255,58,100,103,0,0,0,0,31,247,70,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,11,7,0,0,0,0,0,0,0,0,55,255,167,0,0,0,0,0,0,0,0,55,
255,167,0,0,0,0,0,0,0,0,55,255,167,0,0,0,0,0,88,195,195,209,255,235,195,195,174,0,0,115,255,255,255,255,255,255,255,227,0,0,
0,0,0,55,255,167,0,0,0,0,0,0,0,0,55,255,167,0,0,0,0,0,0,0,0,55,255,167,0,0,0,0,0,0,0,0,16,75,49,0,
0,0,0,0,87,255,209,0,0,124,255,135,0,0,161,255,61,0,0,199,240,4,0,0,236,169,0,0,0,3,2,0,0,0,215,255,255,255,255,183,
0,0,172,203,203,203,203,146,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,22,133,67,0,0,130,255,233,0,0,61,226,143,0,0,0,0,0,222,249,11,0,0,0,35,255,192,0,0,0,0,104,255,123,0,0,0,0,
173,255,54,0,0,0,3,239,238,3,0,0,0,55,255,172,0,0,0,0,124,255,103,0,0,0,0,192,255,34,0,0,0,11,249,222,0,0,0,0,
74,255,153,0,0,0,0,143,255,84,0,0,0,0,212,253,18,0,0,0,25,255,202,0,0,0,0,94,255,133,0,0,0,0,127,207,58,0,0,0,
0,0,0,0,109,193,241,204,136,7,0,0,0,0,159,255,255,239,255,255,197,13,0,0,67,255,250,76,0,42,232,255,117,0,0,181,255,152,0,0,
0,100,255,228,3,0,236,255,72,0,0,0,21,255,255,30,10,255,255,40,0,0,0,0,246,255,60,32,255,255,24,0,0,0,0,229,255,84,11,255,
255,40,0,0,0,0,245,255,62,0,237,255,70,0,0,0,20,255,255,32,0,185,255,149,0,0,0,98,255,232,4,0,72,255,249,73,0,40,231,255,
123,0,0,0,166,255,255,238,255,255,202,16,0,0,0,0,114,196,242,207,141,9,0,0,0,0,75,234,255,239,0,7,146,255,255,255,239,0,59,255,
211,103,255,239,0,59,160,10,63,255,239,0,5,0,0,63,255,239,0,0,0,0,63,255,239,0,0,0,0,63,255,239,0,0,0,0,63,255,239,0,
0,0,0,63,255,239,0,0,0,0,63,255,239,0,0,0,0,63,255,239,0,0,0,0,63,255,239,0,0,0,0,63,255,239,0,0,0,34,167,224,
241,202,98,1,0,0,0,35,243,255,240,223,255,255,144,0,0,0,170,255,175,7,0,76,253,255,32,0,0,221,251,58,0,0,0,209,255,81,0,0,
0,0,0,0,0,6,242,255,65,0,0,0,0,0,0,0,104,255,225,4,0,0,0,0,0,0,59,249,254,71,0,0,0,0,0,0,46,237,255,121,
0,0,0,0,0,0,41,234,255,142,0,0,0,0,0,0,36,230,255,159,1,0,0,0,0,0,32,226,255,174,3,0,0,0,0,0,0,207,255,255,
223,215,215,215,215,121,0,0,227,255,255,255,255,255,255,255,143,0,0,0,28,152,218,245,210,128,10,0,0,0,32,234,255,254,229,255,255,193,3,0,
0,158,255,181,23,0,50,238,255,76,0,0,96,119,39,0,0,0,181,255,115,0,0,0,0,0,0,0,38,236,255,57,0,0,0,0,0,165,206,255,
222,84,0,0,0,0,0,0,235,255,255,208,103,0,0,0,0,0,0,0,13,82,239,255,114,0,0,0,0,0,0,0,0,120,255,214,0,8,171,171,
34,0,0,0,105,255,225,0,0,214,255,175,34,0,35,220,255,168,0,0,64,251,255,255,233,254,255,234,30,0,0,0,46,170,223,246,214,146,24,0,
0,0,0,0,0,0,42,251,255,227,0,0,0,0,0,0,0,187,255,255,227,0,0,0,0,0,0,80,255,226,255,227,0,0,0,0,0,7,221,252,
92,255,227,0,0,0,0,0,121,255,153,47,255,227,0,0,0,0,26,243,243,23,47,255,227,0,0,0,0,163,255,123,0,47,255,227,0,0,0,56,
254,227,9,0,47,255,227,0,0,1,202,255,94,0,0,47,255,227,0,0,31,255,255,255,255,255,255,255,255,255,123,26,211,211,211,211,211,222,255,251,
211,102,0,0,0,0,0,0,59,255,227,0,0,0,0,0,0,0,0,59,255,227,0,0,0,9,255,255,255,255,255,255,255,47,0,0,34,255,250,219,
219,219,219,219,41,0,0,59,255,201,0,0,0,0,0,0,0,0,84,255,174,0,0,0,0,0,0,0,0,110,255,148,92,159,150,82,0,0,0,0,
135,255,243,255,255,255,255,164,0,0,0,83,175,178,49,15,99,250,255,84,0,0,0,0,0,0,0,0,150,255,164,0,0,0,0,0,0,0,0,95,
255,200,0,0,131,167,63,0,0,0,126,255,168,0,0,148,255,200,17,0,42,235,255,100,0,0,24,230,255,244,216,252,255,190,1,0,0,0,25,153,
220,244,206,121,5,0,0,0,0,0,91,186,240,220,151,23,0,0,0,0,133,255,255,250,255,255,225,22,0,0,49,255,245,80,4,45,216,255,149,0,
0,167,255,127,0,0,0,45,107,88,0,0,229,255,42,59,132,157,101,4,0,0,6,254,255,113,254,255,255,255,202,10,0,33,255,255,246,93,15,69,
232,255,134,0,27,255,255,133,0,0,0,100,255,213,0,7,251,255,75,0,0,0,47,255,248,0,0,202,255,111,0,0,0,84,255,216,0,0,106,255,
230,40,0,27,214,255,146,0,0,4,189,255,252,218,249,255,223,15,0,0,0,3,119,209,245,213,144,15,0,0,79,255,255,255,255,255,255,255,255,23,
68,219,219,219,219,219,227,255,255,23,0,0,0,0,0,0,118,255,208,0,0,0,0,0,0,8,232,255,87,0,0,0,0,0,0,106,255,219,3,0,
0,0,0,0,4,224,255,99,0,0,0,0,0,0,94,255,228,6,0,0,0,0,0,1,214,255,112,0,0,0,0,0,0,82,255,236,11,0,0,0,
0,0,0,204,255,125,0,0,0,0,0,0,70,255,242,17,0,0,0,0,0,0,192,255,137,0,0,0,0,0,0,58,255,248,24,0,0,0,0,0,
0,0,26,151,219,245,212,132,14,0,0,0,24,228,255,251,214,255,255,203,9,0,0,132,255,206,20,0,34,231,255,92,0,0,167,255,131,0,0,0,
169,255,130,0,0,104,255,201,11,0,23,228,255,66,0,0,3,147,249,244,195,250,243,119,0,0,0,3,128,235,255,255,255,224,103,0,0,0,143,255,
217,57,12,75,236,255,103,0,6,249,255,73,0,0,0,111,255,218,0,26,255,255,47,0,0,0,84,255,244,0,0,229,255,157,3,0,10,188,255,192,
0,0,77,253,255,225,190,234,255,244,48,0,0,0,50,171,222,247,217,156,34,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,26,160,218,242,
198,92,0,0,0,0,33,239,255,247,224,255,255,151,0,0,0,181,255,193,16,0,63,246,255,66,0,3,248,255,48,0,0,0,145,255,163,0,29,255,
255,10,0,0,0,108,255,219,0,3,248,255,62,0,0,0,167,255,244,0,0,174,255,215,52,16,114,254,255,251,0,0,30,224,255,255,255,248,126,255,
224,0,0,0,14,118,165,128,49,78,255,193,0,0,103,107,31,0,0,0,161,255,132,0,0,186,255,191,30,5,98,251,247,23,0,0,44,242,255,255,
248,255,253,105,0,0,0,0,37,168,227,236,181,73,0,0,0,0,59,221,140,0,0,131,255,234,0,0,25,141,73,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,22,133,67,0,0,130,255,233,0,0,61,226,143,0,0,30,215,182,1,0,0,86,255,254,24,0,0,8,127,96,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,83,247,204,0,0,0,123,255,137,0,0,0,161,255,63,
0,0,0,198,240,4,0,0,0,236,170,0,0,0,0,7,4,0,0,0,0,0,0,0,0,0,0,0,1,34,0,0,0,0,0,0,0,4,93,211,
163,0,0,0,0,0,11,111,225,255,255,149,0,0,0,21,129,237,255,254,180,70,0,0,0,104,246,255,242,145,36,0,0,0,0,0,155,255,248,97,
3,0,0,0,0,0,0,51,179,254,255,226,119,19,0,0,0,0,0,0,43,160,250,255,246,156,42,0,0,0,0,0,0,28,140,242,255,163,0,0,
0,0,0,0,0,0,16,121,139,0,0,62,211,211,211,211,211,211,211,152,0,0,75,255,255,255,255,255,255,255,183,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,65,219,219,219,219,219,219,219,158,0,0,75,255,255,255,255,255,255,255,183,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,17,18,0,0,0,0,0,0,0,0,0,55,244,143,29,0,0,0,0,0,0,
0,46,251,255,250,161,44,0,0,0,0,0,0,28,133,235,255,254,180,61,0,0,0,0,0,0,8,97,207,255,255,198,4,0,0,0,0,0,0,38,
200,255,255,7,0,0,0,1,72,182,254,255,224,111,1,0,9,109,218,255,255,209,91,4,0,0,0,55,255,255,190,72,0,0,0,0,0,0,53,171,
52,0,0,0,0,0,0,0,0,9,123,216,245,217,138,15,0,0,166,255,255,224,255,255,193,3,32,255,255,68,0,60,255,255,57,38,131,116,0,0,
0,228,255,86,0,0,0,0,0,63,255,255,27,0,0,0,0,76,245,255,144,0,0,0,0,69,251,255,127,0,0,0,0,0,167,255,132,0,0,0,
0,0,0,187,247,58,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,77,123,16,0,0,0,0,0,12,249,255,117,0,0,0,0,0,0,171,
223,52,0,0,0,0,0,0,0,16,128,196,233,250,238,208,144,30,0,0,0,0,0,0,0,91,232,255,249,216,183,200,236,255,245,123,6,0,0,0,
0,108,255,236,106,4,0,0,0,0,52,196,255,144,0,0,0,26,241,239,26,0,47,133,152,84,65,123,6,188,254,65,0,0,158,255,100,0,82,249,
255,255,255,233,255,11,32,253,193,0,0,231,253,8,9,230,232,62,11,80,252,255,11,0,219,251,4,17,255,222,0,54,255,117,0,0,0,183,255,11,
0,179,255,25,34,255,193,0,82,255,84,0,0,0,159,255,11,0,177,255,15,25,255,221,0,54,255,123,0,0,0,194,255,11,0,194,250,1,5,250,
252,6,8,230,233,60,12,97,255,255,22,8,240,187,0,0,196,255,77,0,84,249,255,255,254,157,255,177,186,255,84,0,0,89,255,217,5,0,46,122,
132,61,3,147,229,214,110,0,0,0,0,188,255,166,15,0,0,0,0,0,0,0,0,0,0,0,0,0,38,234,255,227,108,60,17,20,57,130,53,0,
0,0,0,0,0,0,30,177,255,255,255,255,255,255,255,144,0,0,0,0,0,0,0,0,0,38,109,151,169,155,126,73,7,0,0,0,0,0,0,0,
0,114,255,255,178,0,0,0,0,0,0,0,0,199,255,251,249,16,0,0,0,0,0,0,29,254,235,179,255,97,0,0,0,0,0,0,113,255,174,113,
255,184,0,0,0,0,0,0,198,255,111,46,255,251,20,0,0,0,0,28,254,255,43,0,229,255,102,0,0,0,0,112,255,228,0,0,155,255,189,0,
0,0,0,197,255,150,0,0,75,255,252,23,0,0,27,254,255,255,255,255,255,255,255,107,0,0,111,255,246,203,203,203,203,229,255,194,0,0,196,255,
158,0,0,0,0,77,255,254,27,26,254,255,77,0,0,0,0,7,242,255,112,109,255,244,8,0,0,0,0,0,165,255,199,0,195,255,255,255,255,235,
195,82,0,0,0,195,255,234,215,219,254,255,255,97,0,0,195,255,115,0,0,26,174,255,209,0,0,195,255,115,0,0,0,77,255,234,0,0,195,255,
115,0,0,0,146,255,170,0,0,195,255,199,151,156,220,252,149,17,0,0,195,255,255,255,255,255,248,178,36,0,0,195,255,115,0,0,27,164,255,223,
8,0,195,255,115,0,0,0,7,241,255,73,0,195,255,115,0,0,0,0,222,255,100,0,195,255,115,0,0,0,92,255,255,61,0,195,255,234,215,216,
237,255,255,190,1,0,195,255,255,255,255,245,210,129,12,0,0,0,0,25,151,204,244,216,160,36,0,0,0,0,0,72,236,255,255,255,255,255,250,75,
0,0,0,18,236,255,198,55,8,42,178,255,242,27,0,0,145,255,220,6,0,0,0,5,215,255,112,0,0,215,255,103,0,0,0,0,0,53,91,60,
0,4,250,255,53,0,0,0,0,0,0,0,0,0,30,255,255,28,0,0,0,0,0,0,0,0,0,5,251,255,52,0,0,0,0,0,0,0,0,0,
0,217,255,101,0,0,0,0,0,53,91,62,0,0,149,255,217,4,0,0,0,5,215,255,116,0,0,20,239,255,192,48,5,38,176,255,239,27,0,0,
0,76,238,255,255,254,255,255,246,73,0,0,0,0,0,28,154,206,244,214,158,31,0,0,0,0,195,255,255,255,250,210,165,56,0,0,0,0,195,255,
245,235,244,255,255,252,132,0,0,0,195,255,115,0,0,30,137,253,255,74,0,0,195,255,115,0,0,0,0,137,255,216,1,0,195,255,115,0,0,0,
0,20,254,255,28,0,195,255,115,0,0,0,0,0,231,255,65,0,195,255,115,0,0,0,0,0,208,255,94,0,195,255,115,0,0,0,0,0,232,255,
65,0,195,255,115,0,0,0,0,20,254,255,27,0,195,255,115,0,0,0,0,137,255,215,1,0,195,255,115,0,0,26,136,253,255,72,0,0,195,255,
242,231,241,255,255,252,129,0,0,0,195,255,255,255,248,208,164,53,0,0,0,0,195,255,255,255,255,255,255,255,67,0,195,255,238,223,223,223,223,223,
59,0,195,255,115,0,0,0,0,0,0,0,195,255,115,0,0,0,0,0,0,0,195,255,115,0,0,0,0,0,0,0,195,255,231,211,211,211,211,178,
0,0,195,255,255,255,255,255,255,215,0,0,195,255,115,0,0,0,0,0,0,0,195,255,115,0,0,0,0,0,0,0,195,255,115,0,0,0,0,0,
0,0,195,255,115,0,0,0,0,0,0,0,195,255,236,219,219,219,219,219,68,0,195,255,255,255,255,255,255,255,79,0,195,255,255,255,255,255,255,255,
39,0,195,255,238,223,223,223,223,223,34,0,195,255,115,0,0,0,0,0,0,0,195,255,115,0,0,0,0,0,0,0,195,255,115,0,0,0,0,0,
0,0,195,255,115,0,0,0,0,0,0,0,195,255,255,255,255,255,255,151,0,0,195,255,234,215,215,215,215,128,0,0,195,255,115,0,0,0,0,0,
0,0,195,255,115,0,0,0,0,0,0,0,195,255,115,0,0,0,0,0,0,0,195,255,115,0,0,0,0,0,0,0,195,255,115,0,0,0,0,0,
0,0,0,0,25,150,204,244,215,157,33,0,0,0,0,0,70,235,255,255,255,255,255,249,73,0,0,0,16,236,255,200,55,8,43,188,255,242,29,0,
0,142,255,222,7,0,0,0,21,236,255,119,0,0,215,255,106,0,0,0,0,0,34,55,38,0,4,250,255,54,0,0,0,0,0,0,0,0,0,30,
255,255,28,0,0,35,187,187,187,187,155,0,6,252,255,51,0,0,47,255,255,255,255,210,0,0,220,255,100,0,0,0,0,0,109,255,176,0,0,150,
255,217,5,0,0,0,0,190,255,131,0,0,22,240,255,195,51,6,33,157,255,243,32,0,0,0,77,238,255,255,254,255,255,249,95,0,0,0,0,0,
28,152,204,244,216,167,41,0,0,0,0,195,255,115,0,0,0,0,0,203,255,107,0,0,195,255,115,0,0,0,0,0,203,255,107,0,0,195,255,115,
0,0,0,0,0,203,255,107,0,0,195,255,115,0,0,0,0,0,203,255,107,0,0,195,255,115,0,0,0,0,0,203,255,107,0,0,195,255,236,219,
219,219,219,219,248,255,107,0,0,195,255,255,255,255,255,255,255,255,255,107,0,0,195,255,115,0,0,0,0,0,203,255,107,0,0,195,255,115,0,0,
0,0,0,203,255,107,0,0,195,255,115,0,0,0,0,0,203,255,107,0,0,195,255,115,0,0,0,0,0,203,255,107,0,0,195,255,115,0,0,0,
0,0,203,255,107,0,0,195,255,115,0,0,0,0,0,203,255,107,0,0,195,255,115,0,0,195,255,115,0,0,195,255,115,0,0,195,255,115,0,0,
195,255,115,0,0,195,255,115,0,0,195,255,115,0,0,195,255,115,0,0,195,255,115,0,0,195,255,115,0,0,195,255,115,0,0,195,255,115,0,0,
195,255,115,0,0,0,0,0,0,0,151,255,155,0,0,0,0,0,0,0,151,255,155,0,0,0,0,0,0,0,151,255,155,0,0,0,0,0,0,0,
151,255,155,0,0,0,0,0,0,0,151,255,155,0,0,0,0,0,0,0,151,255,155,0,0,0,0,0,0,0,151,255,155,0,0,0,0,0,0,0,
151,255,155,0,58,163,140,0,0,0,151,255,155,0,74,255,243,6,0,0,182,255,138,0,30,255,255,89,0,28,248,255,92,0,0,156,255,253,219,246,
255,213,6,0,0,6,119,212,245,222,154,22,0,0,0,195,255,115,0,0,0,4,187,255,228,26,0,195,255,115,0,0,0,148,255,245,50,0,0,195,
255,115,0,0,106,255,254,81,0,0,0,195,255,115,0,68,251,255,118,0,0,0,0,195,255,113,26,235,255,159,0,0,0,0,0,195,255,112,189,255,
219,6,0,0,0,0,0,195,255,218,255,255,254,63,0,0,0,0,0,195,255,255,206,172,255,219,9,0,0,0,0,195,255,226,25,21,236,255,141,0,
0,0,0,195,255,117,0,0,92,255,253,55,0,0,0,195,255,115,0,0,0,183,255,213,6,0,0,195,255,115,0,0,0,31,243,255,131,0,0,195,
255,115,0,0,0,0,110,255,251,48,0,195,255,115,0,0,0,0,0,0,0,195,255,115,0,0,0,0,0,0,0,195,255,115,0,0,0,0,0,0,
0,195,255,115,0,0,0,0,0,0,0,195,255,115,0,0,0,0,0,0,0,195,255,115,0,0,0,0,0,0,0,195,255,115,0,0,0,0,0,0,
0,195,255,115,0,0,0,0,0,0,0,195,255,115,0,0,0,0,0,0,0,195,255,115,0,0,0,0,0,0,0,195,255,115,0,0,0,0,0,0,
0,195,255,236,219,219,219,219,202,0,0,195,255,255,255,255,255,255,235,0,0,195,255,255,192,0,0,0,0,0,26,253,255,255,107,0,0,195,255,255,
254,28,0,0,0,0,113,255,255,255,107,0,0,195,255,228,255,116,0,0,0,0,202,255,224,255,107,0,0,195,255,162,255,206,0,0,0,35,255,228,
188,255,107,0,0,195,255,105,246,255,41,0,0,124,255,159,193,255,107,0,0,195,255,99,180,255,130,0,0,213,255,84,197,255,107,0,0,195,255,103,
97,255,220,0,45,255,247,13,199,255,107,0,0,195,255,103,16,248,255,37,118,255,173,0,199,255,107,0,0,195,255,103,0,175,255,105,184,255,84,0,
199,255,107,0,0,195,255,103,0,85,255,167,241,243,9,0,199,255,107,0,0,195,255,103,0,9,242,248,255,164,0,0,199,255,107,0,0,195,255,103,
0,0,162,255,255,75,0,0,199,255,107,0,0,195,255,103,0,0,72,255,238,5,0,0,199,255,107,0,0,195,255,242,25,0,0,0,0,151,255,167,
0,0,195,255,255,163,0,0,0,0,151,255,167,0,0,195,255,254,255,60,0,0,0,151,255,167,0,0,195,255,180,255,208,3,0,0,151,255,167,0,
0,195,255,109,212,255,108,0,0,151,255,167,0,0,195,255,118,79,255,239,21,0,151,255,167,0,0,195,255,126,0,191,255,156,0,151,255,167,0,0,
195,255,127,0,44,251,254,51,148,255,167,0,0,195,255,127,0,0,142,255,189,141,255,167,0,0,195,255,127,0,0,14,231,255,182,255,167,0,0,195,
255,127,0,0,0,91,255,253,255,167,0,0,195,255,127,0,0,0,0,193,255,255,167,0,0,195,255,127,0,0,0,0,45,251,255,167,0,0,0,0,
25,150,204,243,206,154,29,0,0,0,0,0,71,235,255,255,255,255,255,239,79,0,0,0,18,236,255,196,54,10,49,187,255,241,23,0,0,145,255,218,
5,0,0,0,3,209,255,155,0,0,215,255,102,0,0,0,0,0,91,255,226,0,4,250,255,53,0,0,0,0,0,42,255,254,12,30,255,255,28,0,
0,0,0,0,16,255,255,41,5,251,255,52,0,0,0,0,0,41,255,255,12,0,216,255,100,0,0,0,0,0,88,255,227,0,0,147,255,217,5,0,
0,0,2,206,255,158,0,0,19,237,255,193,49,6,44,183,255,242,25,0,0,0,73,237,255,255,254,255,255,241,82,0,0,0,0,0,26,152,206,244,
209,156,31,0,0,0,0,195,255,255,255,255,227,180,51,0,0,0,195,255,236,219,225,253,255,251,69,0,0,195,255,115,0,0,19,190,255,214,0,0,
195,255,115,0,0,0,52,255,255,19,0,195,255,115,0,0,0,21,255,255,45,0,195,255,115,0,0,0,65,255,255,13,0,195,255,115,0,4,53,211,
255,196,0,0,195,255,255,255,255,255,255,237,46,0,0,195,255,231,211,211,184,137,22,0,0,0,195,255,115,0,0,0,0,0,0,0,0,195,255,115,
0,0,0,0,0,0,0,0,195,255,115,0,0,0,0,0,0,0,0,195,255,115,0,0,0,0,0,0,0,0,0,0,25,150,204,243,206,154,29,0,
0,0,0,0,71,235,255,255,255,255,255,239,79,0,0,0,18,236,255,196,54,10,49,187,255,241,23,0,0,145,255,218,5,0,0,0,3,209,255,155,
0,0,215,255,102,0,0,0,0,0,91,255,226,0,4,250,255,53,0,0,0,0,0,42,255,254,12,30,255,255,28,0,0,0,0,0,16,255,255,43,
5,251,255,52,0,0,0,0,0,44,255,255,29,0,216,255,100,0,4,90,86,1,90,255,241,4,0,147,255,217,5,0,135,255,122,212,255,164,0,0,
19,237,255,193,49,10,210,255,255,236,24,0,0,0,73,237,255,255,254,255,255,255,100,0,0,0,0,0,26,152,206,244,211,175,255,200,6,0,0,0,
0,0,0,0,0,0,0,133,231,128,0,0,195,255,255,255,255,229,186,64,0,0,0,195,255,236,219,224,250,255,255,90,0,0,195,255,115,0,0,12,
167,255,234,1,0,195,255,115,0,0,0,34,255,255,31,0,195,255,115,0,0,0,30,255,255,42,0,195,255,115,0,0,7,154,255,239,7,0,195,255,
231,211,215,244,255,255,112,0,0,195,255,255,255,255,255,255,96,0,0,0,195,255,115,0,14,239,255,108,0,0,0,195,255,115,0,0,130,255,229,7,
0,0,195,255,115,0,0,19,243,255,108,0,0,195,255,115,0,0,0,138,255,228,7,0,195,255,115,0,0,0,23,246,255,107,0,0,21,141,212,245,
222,169,44,0,0,0,23,227,255,255,245,255,255,250,61,0,0,152,255,206,53,1,35,165,255,209,0,0,208,255,107,0,0,0,24,155,154,4,0,186,
255,197,22,0,0,0,0,0,0,0,68,253,255,250,177,103,23,0,0,0,0,0,64,202,255,255,255,252,151,12,0,0,0,0,0,36,107,199,255,255,
172,0,0,0,0,0,0,0,0,79,255,255,23,26,223,223,20,0,0,0,9,253,255,51,0,227,255,173,45,0,14,152,255,248,8,0,70,253,255,255,
241,253,255,255,103,0,0,0,48,168,220,247,224,180,64,0,0,79,255,255,255,255,255,255,255,255,255,135,69,223,223,223,239,255,246,223,223,223,118,0,
0,0,0,127,255,183,0,0,0,0,0,0,0,0,127,255,183,0,0,0,0,0,0,0,0,127,255,183,0,0,0,0,0,0,0,0,127,255,183,0,
0,0,0,0,0,0,0,127,255,183,0,0,0,0,0,0,0,0,127,255,183,0,0,0,0,0,0,0,0,127,255,183,0,0,0,0,0,0,0,0,
127,255,183,0,0,0,0,0,0,0,0,127,255,183,0,0,0,0,0,0,0,0,127,255,183,0,0,0,0,0,0,0,0,127,255,183,0,0,0,0,
0,195,255,115,0,0,0,0,0,243,255,67,0,0,195,255,115,0,0,0,0,0,243,255,67,0,0,195,255,115,0,0,0,0,0,243,255,67,0,0,
195,255,115,0,0,0,0,0,243,255,67,0,0,195,255,115,0,0,0,0,0,243,255,67,0,0,195,255,115,0,0,0,0,0,243,255,67,0,0,195,
255,115,0,0,0,0,0,243,255,67,0,0,195,255,115,0,0,0,0,0,243,255,67,0,0,188,255,129,0,0,0,0,5,251,255,59,0,0,144,255,
195,0,0,0,0,68,255,255,15,0,0,65,254,255,149,26,7,70,225,255,189,0,0,0,0,138,255,255,255,254,255,255,232,30,0,0,0,0,0,70,
183,225,243,205,138,15,0,0,0,109,255,245,10,0,0,0,0,0,160,255,200,24,253,255,82,0,0,0,0,3,236,255,115,0,191,255,164,0,0,0,
0,65,255,255,31,0,104,255,240,6,0,0,0,145,255,201,0,0,21,252,255,73,0,0,0,225,255,115,0,0,0,187,255,155,0,0,50,255,255,31,
0,0,0,100,255,234,3,0,130,255,201,0,0,0,0,18,250,255,56,0,203,255,116,0,0,0,0,0,182,255,129,18,253,255,32,0,0,0,0,0,
96,255,198,83,255,201,0,0,0,0,0,0,16,249,251,162,255,116,0,0,0,0,0,0,0,178,255,252,255,32,0,0,0,0,0,0,0,91,255,255,
202,0,0,0,0,119,255,232,0,0,0,0,163,255,242,3,0,0,0,150,255,202,53,255,255,36,0,0,0,224,255,255,51,0,0,0,209,255,136,3,
240,255,96,0,0,30,255,253,255,112,0,0,15,253,255,70,0,177,255,155,0,0,92,255,184,255,173,0,0,72,255,250,11,0,111,255,215,0,0,154,
255,92,252,234,0,0,132,255,194,0,0,45,255,255,20,0,216,255,35,212,255,40,0,191,255,128,0,0,1,235,255,78,22,255,231,0,152,255,101,5,
246,255,63,0,0,0,170,255,129,76,255,167,0,88,255,155,47,255,246,7,0,0,0,104,255,174,126,255,103,0,25,255,205,94,255,187,0,0,0,0,
38,255,219,175,255,39,0,0,216,249,147,255,121,0,0,0,0,0,228,253,232,231,0,0,0,152,255,232,255,55,0,0,0,0,0,162,255,255,167,0,
0,0,88,255,255,242,4,0,0,0,0,0,96,255,255,103,0,0,0,25,255,255,180,0,0,0,8,217,255,171,0,0,0,0,94,255,249,43,0,63,
254,255,78,0,0,20,236,255,126,0,0,0,153,255,226,10,0,161,255,212,6,0,0,0,15,230,255,126,56,254,253,55,0,0,0,0,0,81,255,240,
196,255,142,0,0,0,0,0,0,0,173,255,255,222,10,0,0,0,0,0,0,0,77,255,255,153,0,0,0,0,0,0,0,9,217,255,255,252,53,0,
0,0,0,0,0,142,255,210,159,255,211,6,0,0,0,0,59,253,255,65,26,242,255,127,0,0,0,9,218,255,159,0,0,109,255,250,44,0,0,143,
255,230,16,0,0,2,198,255,202,3,60,253,255,79,0,0,0,0,41,249,255,116,79,255,255,68,0,0,0,0,39,252,255,106,0,188,255,204,1,0,
0,0,169,255,213,4,0,45,252,255,86,0,0,49,254,255,72,0,0,0,151,255,218,4,0,182,255,184,0,0,0,0,20,240,255,97,53,255,252,42,
0,0,0,0,0,114,255,214,172,255,149,0,0,0,0,0,0,5,217,255,255,240,20,0,0,0,0,0,0,0,76,255,255,115,0,0,0,0,0,0,
0,0,7,255,255,47,0,0,0,0,0,0,0,0,7,255,255,47,0,0,0,0,0,0,0,0,7,255,255,47,0,0,0,0,0,0,0,0,7,255,
255,47,0,0,0,0,0,0,0,0,7,255,255,47,0,0,0,0,11,255,255,255,255,255,255,255,255,255,23,10,223,223,223,223,222,217,237,255,238,11,
0,0,0,0,0,0,22,231,255,96,0,0,0,0,0,0,0,183,255,185,0,0,0,0,0,0,0,99,255,243,31,0,0,0,0,0,0,25,239,255,
107,0,0,0,0,0,0,0,175,255,195,1,0,0,0,0,0,0,86,255,247,38,0,0,0,0,0,0,19,233,255,118,0,0,0,0,0,0,0,162,
255,198,3,0,0,0,0,0,0,73,255,241,33,0,0,0,0,0,0,1,226,255,239,211,216,219,219,219,219,27,3,255,255,255,255,255,255,255,255,255,
31,0,95,255,255,255,107,0,95,255,232,175,74,0,95,255,179,0,0,0,95,255,179,0,0,0,95,255,179,0,0,0,95,255,179,0,0,0,95,255,
179,0,0,0,95,255,179,0,0,0,95,255,179,0,0,0,95,255,179,0,0,0,95,255,179,0,0,0,95,255,179,0,0,0,95,255,179,0,0,0,
95,255,231,171,72,0,95,255,255,255,107,150,255,77,0,0,0,0,81,255,146,0,0,0,0,15,252,215,0,0,0,0,0,199,255,28,0,0,0,0,
130,255,97,0,0,0,0,61,255,166,0,0,0,0,5,243,234,1,0,0,0,0,179,255,47,0,0,0,0,111,255,116,0,0,0,0,42,255,185,0,
0,0,0,0,228,246,7,0,0,0,0,160,255,67,0,0,0,0,91,255,136,0,0,0,0,23,254,205,0,0,0,0,0,175,206,11,27,255,255,255,
179,0,19,175,205,255,179,0,0,0,95,255,179,0,0,0,95,255,179,0,0,0,95,255,179,0,0,0,95,255,179,0,0,0,95,255,179,0,0,0,
95,255,179,0,0,0,95,255,179,0,0,0,95,255,179,0,0,0,95,255,179,0,0,0,95,255,179,0,0,0,95,255,179,0,18,171,203,255,179,0,
27,255,255,255,179,0,0,0,33,251,255,72,0,0,0,0,151,248,230,195,0,0,0,25,248,157,114,255,61,0,0,140,255,45,14,242,183,0,18,243,
185,0,0,142,255,50,25,75,33,0,0,20,75,38,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,255,255,255,255,255,255,247,191,191,191,191,191,191,191,185,0,96,
255,157,0,0,0,0,183,249,24,0,0,0,29,139,59,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,104,207,
245,234,175,47,0,0,0,137,255,253,199,227,255,236,31,0,3,157,216,59,0,1,206,255,109,0,0,0,0,0,0,13,182,255,152,0,0,15,118,188,
229,254,255,255,155,0,7,203,255,246,177,127,169,255,155,0,61,255,242,21,0,0,146,255,155,0,77,255,226,4,0,18,225,255,155,0,21,239,255,211,
166,232,218,255,155,0,0,59,197,243,207,115,116,255,155,0,0,219,255,75,0,0,0,0,0,0,0,0,219,255,75,0,0,0,0,0,0,0,0,219,
255,75,0,0,0,0,0,0,0,0,219,255,77,141,233,231,162,18,0,0,0,219,255,197,249,216,252,255,215,5,0,0,219,255,229,32,0,46,241,255,
102,0,0,219,255,122,0,0,0,148,255,170,0,0,219,255,76,0,0,0,101,255,205,0,0,219,255,77,0,0,0,102,255,205,0,0,219,255,124,0,
0,0,149,255,171,0,0,219,255,230,31,0,45,241,255,102,0,0,219,255,199,248,211,251,255,215,5,0,0,219,255,66,145,235,230,162,18,0,0,0,
0,30,167,223,242,200,86,0,0,0,41,239,255,238,226,255,255,109,0,0,182,255,179,5,1,110,255,239,9,11,255,255,45,0,0,2,90,38,0,48,
255,251,2,0,0,0,0,0,0,49,255,251,1,0,0,0,0,0,0,12,255,255,43,0,0,2,104,54,3,0,184,255,176,4,0,106,255,239,8,0,
43,240,255,234,223,255,255,109,0,0,0,31,168,224,242,201,87,0,0,0,0,0,0,0,0,0,175,255,119,0,0,0,0,0,0,0,0,175,255,119,
0,0,0,0,0,0,0,0,175,255,119,0,0,0,68,199,243,209,67,175,255,119,0,0,69,253,255,239,223,241,211,255,119,0,0,204,255,178,5,0,
110,255,255,119,0,16,255,255,43,0,0,2,225,255,119,0,50,255,251,2,0,0,0,181,255,119,0,51,255,251,2,0,0,0,182,255,119,0,17,255,
255,45,0,0,2,226,255,119,0,0,206,255,179,4,0,110,255,255,119,0,0,71,253,255,235,219,242,208,255,119,0,0,0,70,199,244,212,69,167,255,
119,0,0,0,32,168,225,237,189,57,0,0,0,43,240,255,214,200,254,253,73,0,0,185,255,130,0,0,99,255,223,2,13,255,249,7,0,0,7,237,
255,34,51,255,255,255,255,255,255,255,255,74,47,255,244,123,123,123,123,123,123,42,11,255,254,16,0,0,0,0,0,0,0,183,255,148,1,0,51,237,
186,12,0,43,240,255,217,197,251,255,156,0,0,0,31,168,222,245,211,114,2,0,0,0,13,132,191,184,50,0,0,178,255,255,255,32,0,12,254,255,
88,18,0,0,35,255,255,5,0,0,215,255,255,255,255,255,3,151,190,255,255,181,179,2,0,35,255,255,3,0,0,0,35,255,255,3,0,0,0,35,
255,255,3,0,0,0,35,255,255,3,0,0,0,35,255,255,3,0,0,0,35,255,255,3,0,0,0,35,255,255,3,0,0,0,35,255,255,3,0,0,
0,0,67,198,243,211,70,163,255,123,0,0,69,252,255,239,223,243,205,255,123,0,0,205,255,179,5,0,106,255,255,123,0,16,255,255,43,0,0,1,
221,255,123,0,51,255,250,1,0,0,0,177,255,123,0,48,255,252,3,0,0,0,180,255,123,0,13,255,255,55,0,0,4,229,255,123,0,0,187,255,
205,41,20,147,255,255,123,0,0,45,236,255,255,255,217,189,255,123,0,0,0,26,129,169,139,28,168,255,123,0,0,0,19,1,0,0,0,194,255,92,
0,0,141,250,150,23,12,102,254,250,35,0,0,61,239,255,255,255,255,251,106,0,0,0,0,20,109,151,155,122,35,0,0,0,0,219,255,67,0,0,
0,0,0,0,0,219,255,67,0,0,0,0,0,0,0,219,255,67,0,0,0,0,0,0,0,219,255,68,127,214,235,163,19,0,0,219,255,182,249,231,
255,255,178,0,0,219,255,211,21,0,116,255,255,20,0,219,255,101,0,0,19,252,255,55,0,219,255,76,0,0,0,231,255,63,0,219,255,75,0,0,
0,231,255,63,0,219,255,75,0,0,0,231,255,63,0,219,255,75,0,0,0,231,255,63,0,219,255,75,0,0,0,231,255,63,0,219,255,75,0,0,
0,231,255,63,0,23,58,0,0,223,255,82,0,158,223,43,0,0,0,0,0,219,255,75,0,219,255,75,0,219,255,75,0,219,255,75,0,219,255,75,
0,219,255,75,0,219,255,75,0,219,255,75,0,219,255,75,0,219,255,75,0,0,23,58,0,0,0,223,255,82,0,0,158,223,43,0,0,0,0,0,
0,0,223,255,75,0,0,223,255,75,0,0,223,255,75,0,0,223,255,75,0,0,223,255,75,0,0,223,255,75,0,0,223,255,75,0,0,223,255,75,
0,0,223,255,75,0,0,223,255,75,0,0,226,255,74,0,58,255,255,44,91,255,255,205,3,71,186,136,20,0,0,219,255,75,0,0,0,0,0,0,
0,219,255,75,0,0,0,0,0,0,0,219,255,75,0,0,0,0,0,0,0,219,255,75,0,0,158,255,217,16,0,219,255,75,0,113,255,238,38,0,
0,219,255,75,72,252,251,68,0,0,0,219,255,116,240,255,107,0,0,0,0,219,255,253,255,231,6,0,0,0,0,219,255,252,243,255,132,0,0,0,
0,219,255,121,98,255,252,52,0,0,0,219,255,75,0,183,255,213,7,0,0,219,255,75,0,28,240,255,136,0,0,219,255,75,0,0,97,255,252,56,
0,219,255,75,0,219,255,75,0,219,255,75,0,219,255,75,0,219,255,75,0,219,255,75,0,219,255,75,0,219,255,75,0,219,255,75,0,219,255,75,
0,219,255,75,0,219,255,75,0,219,255,75,0,219,255,51,142,226,223,69,0,115,228,240,170,23,0,0,219,255,192,241,247,255,227,98,251,236,255,255,
180,0,0,219,255,191,9,19,206,255,254,48,2,125,255,254,9,0,219,255,91,0,0,117,255,202,0,0,22,255,255,38,0,219,255,75,0,0,103,255,
179,0,0,3,255,255,39,0,219,255,75,0,0,103,255,179,0,0,3,255,255,39,0,219,255,75,0,0,103,255,179,0,0,3,255,255,39,0,219,255,
75,0,0,103,255,179,0,0,3,255,255,39,0,219,255,75,0,0,103,255,179,0,0,3,255,255,39,0,219,255,75,0,0,103,255,179,0,0,3,255,
255,39,0,219,255,48,124,212,235,163,19,0,0,219,255,160,249,231,255,255,178,0,0,219,255,211,21,0,116,255,255,20,0,219,255,101,0,0,19,252,
255,55,0,219,255,76,0,0,0,231,255,63,0,219,255,75,0,0,0,231,255,63,0,219,255,75,0,0,0,231,255,63,0,219,255,75,0,0,0,231,
255,63,0,219,255,75,0,0,0,231,255,63,0,219,255,75,0,0,0,231,255,63,0,0,30,167,223,237,189,74,0,0,0,41,239,255,237,220,255,255,
111,0,0,182,255,179,5,0,100,255,243,24,11,255,255,44,0,0,0,215,255,96,48,255,251,2,0,0,0,166,255,135,49,255,250,1,0,0,0,166,
255,136,12,255,255,43,0,0,0,213,255,97,0,184,255,176,3,0,96,255,244,25,0,43,240,255,233,215,255,255,112,0,0,0,31,168,224,238,191,75,
0,0,0,219,255,65,141,233,231,162,18,0,0,0,219,255,190,249,216,252,255,215,5,0,0,219,255,229,32,0,46,241,255,102,0,0,219,255,122,0,
0,0,148,255,170,0,0,219,255,76,0,0,0,101,255,205,0,0,219,255,77,0,0,0,102,255,205,0,0,219,255,124,0,0,0,149,255,171,0,0,
219,255,230,31,0,45,241,255,102,0,0,219,255,201,248,211,251,255,215,5,0,0,219,255,78,144,235,230,162,18,0,0,0,219,255,75,0,0,0,0,
0,0,0,0,219,255,75,0,0,0,0,0,0,0,0,219,255,75,0,0,0,0,0,0,0,0,68,79,23,0,0,0,0,0,0,0,0,0,68,199,
243,209,67,167,255,119,0,0,70,253,255,239,223,241,206,255,119,0,0,205,255,178,5,0,110,255,255,119,0,16,255,255,43,0,0,2,225,255,119,0,
50,255,251,2,0,0,0,181,255,119,0,51,255,251,2,0,0,0,182,255,119,0,17,255,255,45,0,0,2,226,255,119,0,0,205,255,179,4,0,110,
255,255,119,0,0,71,253,255,235,219,242,213,255,119,0,0,0,70,199,244,212,69,175,255,119,0,0,0,0,0,0,0,0,175,255,119,0,0,0,0,
0,0,0,0,175,255,119,0,0,0,0,0,0,0,0,175,255,119,0,0,0,0,0,0,0,0,54,79,37,0,0,219,255,66,160,240,103,0,219,255,
197,255,249,103,0,219,255,227,48,0,4,0,219,255,109,0,0,0,0,219,255,76,0,0,0,0,219,255,75,0,0,0,0,219,255,75,0,0,0,0,
219,255,75,0,0,0,0,219,255,75,0,0,0,0,219,255,75,0,0,0,0,4,116,212,245,230,167,34,0,0,141,255,244,182,219,255,228,14,0,236,
255,80,0,1,167,165,42,0,235,255,157,21,0,0,0,0,0,115,255,255,254,212,142,35,0,0,0,66,161,227,255,255,244,44,0,0,6,0,0,50,
224,255,144,28,215,244,34,0,0,197,255,130,0,188,255,242,183,219,255,239,37,0,12,135,220,247,227,168,42,0,0,26,51,33,0,0,0,131,255,163,
0,0,0,131,255,163,0,0,215,255,255,255,255,107,151,219,255,228,179,75,0,131,255,163,0,0,0,131,255,163,0,0,0,131,255,163,0,0,0,131,
255,163,0,0,0,131,255,163,0,0,0,124,255,187,0,0,0,71,255,255,210,94,0,0,124,228,245,133,0,219,255,75,0,0,0,231,255,63,0,219,
255,75,0,0,0,231,255,63,0,219,255,75,0,0,0,231,255,63,0,219,255,75,0,0,0,231,255,63,0,219,255,75,0,0,0,231,255,63,0,219,
255,75,0,0,0,232,255,63,0,212,255,112,0,0,6,251,255,63,0,178,255,202,9,0,128,255,255,63,0,81,255,255,240,231,225,216,255,63,0,0,
106,219,236,170,41,205,255,63,132,255,198,0,0,0,0,204,255,122,43,255,254,24,0,0,28,255,255,35,0,211,255,101,0,0,106,255,201,0,0,122,
255,180,0,0,185,255,112,0,0,35,255,247,12,14,249,253,26,0,0,0,202,255,81,85,255,190,0,0,0,0,113,255,148,151,255,102,0,0,0,0,
27,253,213,215,250,19,0,0,0,0,0,192,255,255,180,0,0,0,0,0,0,104,255,255,91,0,0,0,129,255,188,0,0,8,249,255,71,0,0,130,
255,191,58,255,244,3,0,62,255,255,129,0,0,190,255,121,3,239,255,50,0,122,255,250,187,0,5,245,255,50,0,172,255,109,0,182,239,179,243,3,
54,255,234,1,0,101,255,166,1,239,182,119,255,46,110,255,164,0,0,31,255,221,42,255,125,61,255,103,162,255,93,0,0,0,216,255,111,255,60,7,
246,156,211,255,24,0,0,0,145,255,209,243,5,0,185,214,251,208,0,0,0,0,74,255,255,180,0,0,117,255,255,137,0,0,0,0,10,248,255,113,
0,0,49,255,255,66,0,0,36,248,255,62,0,0,130,255,204,2,0,132,255,201,0,29,246,254,56,0,0,10,226,255,75,150,255,157,0,0,0,0,
84,255,210,247,239,21,0,0,0,0,0,191,255,255,108,0,0,0,0,0,7,220,255,255,133,0,0,0,0,0,125,255,169,240,249,39,0,0,0,32,
246,252,35,128,255,187,0,0,0,177,255,152,0,14,233,255,86,0,75,255,240,21,0,0,101,255,228,13,132,255,198,0,0,0,0,197,255,126,43,255,
253,23,0,0,24,253,255,37,0,210,255,100,0,0,104,255,202,0,0,122,255,179,0,0,186,255,113,0,0,34,255,247,10,16,250,253,26,0,0,0,
200,255,78,91,255,189,0,0,0,0,112,255,143,160,255,100,0,0,0,0,26,253,206,228,249,17,0,0,0,0,0,190,255,255,176,0,0,0,0,0,
0,102,255,255,87,0,0,0,0,0,0,69,255,243,10,0,0,0,0,14,29,171,255,157,0,0,0,0,0,165,255,255,237,33,0,0,0,0,0,147,
188,153,38,0,0,0,0,0,0,215,255,255,255,255,255,255,111,0,0,175,207,207,207,218,255,255,89,0,0,0,0,0,0,163,255,189,1,0,0,0,
0,0,89,255,241,30,0,0,0,0,0,31,240,255,94,0,0,0,0,0,3,194,255,174,0,0,0,0,0,0,124,255,233,21,0,0,0,0,0,55,
251,255,78,0,0,0,0,0,0,220,255,250,203,203,203,203,121,0,0,255,255,255,255,255,255,255,151,0,0,0,0,0,70,152,141,0,0,0,0,104,
255,255,205,0,0,0,0,212,255,116,3,0,0,0,0,248,255,20,0,0,0,0,0,255,255,0,0,0,0,0,21,255,254,0,0,0,0,8,125,255,
203,0,0,0,0,215,255,199,56,0,0,0,0,215,255,198,57,0,0,0,0,8,116,255,209,0,0,0,0,0,12,255,255,0,0,0,0,0,0,255,
255,2,0,0,0,0,0,231,255,58,0,0,0,0,0,164,255,235,145,0,0,0,0,14,150,226,201,0,0,0,81,83,3,0,0,0,247,255,11,0,
0,0,247,255,11,0,0,0,247,255,11,0,0,0,247,255,11,0,0,0,247,255,11,0,0,0,247,255,11,0,0,0,247,255,11,0,0,0,247,255,
11,0,0,0,247,255,11,0,0,0,247,255,11,0,0,0,247,255,11,0,0,0,247,255,11,0,0,0,247,255,11,0,0,0,247,255,11,0,0,0,
247,255,11,0,0,0,247,255,11,0,0,0,247,255,11,0,0,0,247,255,11,0,0,0,247,255,11,0,0,0,247,255,11,0,27,241,207,96,0,0,
0,0,19,199,255,255,69,0,0,0,0,0,131,255,143,0,0,0,0,0,76,255,176,0,0,0,0,0,71,255,179,0,0,0,0,0,69,255,190,0,
0,0,0,0,21,254,246,60,4,0,0,0,0,102,226,255,139,0,0,0,0,105,226,255,139,0,0,0,29,255,251,55,4,0,0,0,71,255,193,0,
0,0,0,0,74,255,179,0,0,0,0,0,131,255,154,0,0,0,18,186,251,255,86,0,0,0,27,243,211,109,0,0,0,0,0,0,94,192,173,47,
0,0,75,107,8,0,64,255,255,255,245,97,29,227,251,3,0,145,255,106,39,204,255,255,255,172,0,0,72,111,14,0,8,137,201,155,18,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
const AAGlyph F17_g[] PROGMEM = {
{32,0,0,4,0,0,0},
{33,5,13,5,0,13,0},
{34,9,13,9,0,13,65},
{35,11,13,11,0,13,182},
{36,11,17,11,0,15,325},
{37,17,14,17,0,13,512},
{38,12,13,11,0,13,750},
{39,6,13,6,0,13,906},
{40,6,16,6,0,13,984},
{41,6,16,6,0,13,1080},
{42,9,13,9,0,13,1176},
{43,11,10,11,0,10,1293},
{44,5,6,5,0,2,1403},
{45,8,6,8,0,6,1433},
{46,5,3,5,0,3,1481},
{47,7,15,6,0,13,1496},
{48,11,13,11,0,13,1601},
{49,7,13,7,0,13,1744},
{50,11,13,11,0,13,1835},
{51,11,13,11,0,13,1978},
{52,11,13,11,0,13,2121},
{53,11,13,11,0,13,2264},
{54,11,13,11,0,13,2407},
{55,10,13,10,0,13,2550},
{56,11,13,11,0,13,2680},
{57,11,14,11,0,14,2823},
{58,5,9,5,0,9,2977},
{59,6,13,6,0,9,3022},
{60,11,10,11,0,10,3100},
{61,11,8,11,0,8,3210},
{62,11,10,11,0,10,3298},
{63,9,13,9,0,13,3408},
{64,17,16,17,0,12,3525},
{65,12,13,12,0,13,3797},
{66,11,13,11,0,13,3953},
{67,13,13,13,0,13,4096},
{68,12,13,12,0,13,4265},
{69,10,13,10,0,13,4421},
{70,10,13,10,0,13,4551},
{71,13,13,13,0,13,4681},
{72,13,13,13,0,13,4850},
{73,5,13,5,0,13,5019},
{74,10,13,10,0,13,5084},
{75,12,13,12,0,13,5214},
{76,10,13,10,0,13,5370},
{77,16,13,16,0,13,5500},
{78,13,13,13,0,13,5708},
{79,13,13,13,0,13,5877},
{80,11,13,11,0,13,6046},
{81,13,14,13,0,13,6189},
{82,11,13,11,0,13,6371},
{83,11,13,11,0,13,6514},
{84,11,13,11,0,13,6657},
{85,13,13,13,0,13,6800},
{86,12,13,12,0,13,6969},
{87,17,13,17,0,13,7125},
{88,12,13,12,0,13,7346},
{89,12,13,12,0,13,7502},
{90,11,13,11,0,13,7658},
{91,6,15,6,0,13,7801},
{92,7,15,6,0,13,7891},
{93,6,15,6,0,13,7996},
{94,8,12,8,0,12,8086},
{95,8,2,8,0,0,8182},
{96,6,13,6,0,13,8198},
{97,10,10,10,0,10,8276},
{98,11,13,11,0,13,8376},
{99,10,10,10,0,10,8519},
{100,11,13,11,0,13,8619},
{101,10,10,10,0,10,8762},
{102,7,14,7,0,14,8862},
{103,11,14,11,0,10,8960},
{104,10,13,10,0,13,9114},
{105,4,14,4,0,14,9244},
{106,5,18,4,-1,14,9300},
{107,10,13,10,0,13,9390},
{108,4,13,4,0,13,9520},
{109,15,10,15,0,10,9572},
{110,10,10,10,0,10,9722},
{111,10,10,10,0,10,9822},
{112,11,14,11,0,10,9922},
{113,11,14,11,0,10,10076},
{114,7,10,7,0,10,10230},
{115,9,10,9,0,10,10300},
{116,6,13,6,0,13,10390},
{117,10,10,10,0,10,10468},
{118,10,10,10,0,10,10568},
{119,14,10,14,0,10,10668},
{120,10,10,10,0,10,10808},
{121,10,14,10,0,10,10908},
{122,10,10,10,0,10,11048},
{123,8,15,8,0,13,11148},
{124,6,21,6,0,17,11268},
{125,8,15,8,0,13,11394},
{126,11,7,11,0,7,11514}
};
const AAFont F17 = { 95, F17_g, F17_bmp };
// F13: Inter-SemiBold.otf 13px, 95 glyphs, 6973 bytes
const uint8_t F13_bmp[] PROGMEM = {
0,210,244,0,0,205,240,0,0,200,236,0,0,195,232,0,0,191,228,0,0,186,224,0,0,136,166,0,0,0,0,0,0,166,202,4,0,169,203,2,
0,174,229,30,255,117,0,0,163,218,19,255,106,0,0,151,206,7,255,94,0,0,116,162,0,209,69,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,110,216,0,26,255,44,0,0,
0,147,180,0,63,254,10,0,7,71,200,179,71,140,239,71,11,54,255,255,255,255,255,255,255,16,0,14,255,57,0,185,141,0,0,0,55,255,16,0,
227,100,0,0,182,255,255,255,255,255,255,142,0,62,175,206,75,119,255,85,34,0,0,174,152,0,93,237,0,0,0,0,212,114,0,129,198,0,0,0,
0,0,0,5,30,0,0,0,0,0,0,31,163,0,0,0,0,15,149,232,252,208,88,0,0,182,253,188,215,217,255,75,17,255,177,31,163,24,198,125,
9,250,229,75,163,0,0,0,0,110,253,255,231,119,14,0,0,0,40,141,247,255,240,55,0,0,0,31,163,64,253,180,76,255,127,31,163,16,245,196,
12,224,249,180,216,217,255,111,0,36,173,237,252,212,111,1,0,0,0,31,163,0,0,0,0,0,0,5,30,0,0,0,0,34,205,240,131,0,0,0,
54,236,25,0,0,0,155,184,69,255,32,0,5,211,97,0,0,0,0,186,122,0,246,62,0,128,187,0,0,0,0,0,157,182,64,255,32,44,238,32,
0,0,0,0,0,37,208,241,131,3,201,110,0,0,0,0,0,0,0,0,0,0,115,198,2,0,2,0,0,0,0,0,0,0,35,238,41,0,141,248,
225,55,0,0,0,0,1,190,123,0,38,255,46,152,182,0,0,0,0,102,208,4,0,40,255,43,150,184,0,0,0,27,237,51,0,0,0,148,251,230,
59,0,0,0,0,0,0,0,0,0,0,5,0,0,0,0,6,156,239,224,92,0,0,0,0,111,255,133,178,250,22,0,0,0,149,235,0,71,255,37,
0,0,0,92,255,116,220,202,0,0,0,0,3,222,255,205,22,0,0,0,0,139,255,241,206,10,96,153,0,42,255,132,52,246,169,192,198,0,96,255,
77,0,84,254,255,128,0,35,247,215,110,137,248,255,88,0,0,69,202,245,223,129,173,240,39,0,174,229,0,0,163,218,0,0,151,206,0,0,116,162,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,112,255,45,0,2,227,211,0,0,60,255,120,0,
0,115,255,51,0,0,151,255,16,0,0,187,239,0,0,0,199,228,0,0,0,163,254,8,0,0,126,255,44,0,0,53,255,123,0,0,0,196,219,0,
0,0,76,207,42,87,255,69,0,0,10,243,186,0,0,0,163,252,20,0,0,94,255,71,0,0,60,255,107,0,0,27,255,142,0,0,16,255,154,0,
0,50,255,118,0,0,88,255,81,0,0,168,247,20,0,15,247,154,0,0,75,207,43,0,0,0,6,9,255,7,6,0,2,211,123,246,123,212,2,0,
36,210,255,210,36,0,2,191,204,247,203,191,2,0,59,8,255,7,59,0,0,0,4,83,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,75,219,17,0,0,0,0,0,0,87,255,19,0,0,0,0,72,87,145,255,101,87,
49,0,0,211,255,255,255,255,255,143,0,0,0,0,87,255,19,0,0,0,0,0,0,87,255,19,0,0,0,0,0,0,46,135,10,0,0,0,0,191,
226,0,0,228,151,0,11,255,77,0,36,209,11,0,10,91,91,91,91,12,27,255,255,255,255,35,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,173,202,2,0,169,198,1,0,0,0,226,144,0,0,39,255,76,0,0,108,250,12,0,0,177,194,0,0,4,241,125,
0,0,59,255,56,0,0,127,240,3,0,0,196,175,0,0,13,251,106,0,0,78,255,37,0,0,147,224,0,0,0,63,58,0,0,0,0,0,111,211,
238,184,38,0,0,0,115,255,186,130,234,237,20,0,7,232,212,4,0,73,255,125,0,48,255,131,0,0,3,241,189,0,77,255,102,0,0,0,215,219,
0,78,255,101,0,0,0,214,221,0,49,255,129,0,0,2,240,191,0,8,235,210,3,0,71,255,130,0,0,120,255,184,129,233,239,22,0,0,0,114,
213,239,186,40,0,0,0,46,214,255,139,0,67,248,209,255,139,0,102,132,38,255,139,0,7,0,35,255,139,0,0,0,35,255,139,0,0,0,35,255,
139,0,0,0,35,255,139,0,0,0,35,255,139,0,0,0,35,255,139,0,0,0,35,255,139,0,0,23,164,234,233,165,25,0,0,194,246,147,149,248,
198,0,25,255,152,0,0,160,255,24,3,15,6,0,0,162,253,19,0,0,0,0,49,246,168,0,0,0,0,30,227,221,19,0,0,0,25,219,231,35,
0,0,0,21,214,240,47,0,0,0,11,208,255,163,103,103,103,32,39,255,255,255,255,255,255,79,0,19,153,228,240,182,42,0,0,190,250,148,132,237,
227,9,7,159,107,0,0,131,255,49,0,0,0,0,41,206,231,4,0,0,0,227,255,225,41,0,0,0,0,71,127,226,233,34,0,0,0,0,0,66,
255,132,50,219,130,0,0,91,255,131,8,219,248,157,142,235,248,47,0,32,169,233,241,193,63,0,0,0,0,0,157,255,203,0,0,0,0,0,57,254,
254,203,0,0,0,0,3,206,201,205,203,0,0,0,0,108,253,53,203,203,0,0,0,23,240,154,0,203,203,0,0,0,161,239,20,0,203,203,0,0,
51,255,184,99,99,224,224,99,1,83,255,255,255,255,255,255,255,3,0,0,0,0,0,211,203,0,0,0,0,0,0,0,211,203,0,0,0,128,255,255,
255,255,255,3,0,151,245,107,107,107,107,1,0,175,217,0,0,0,0,0,0,199,192,23,67,28,0,0,0,223,229,247,255,253,127,0,0,100,127,31,
23,182,255,54,0,0,0,0,0,54,255,115,8,171,119,0,0,100,255,96,0,187,252,147,135,235,231,19,0,19,156,231,239,180,41,0,0,0,86,202,
243,196,56,0,0,84,254,200,145,234,240,28,0,209,213,7,0,74,227,98,34,255,122,6,56,34,0,0,64,255,137,236,255,255,158,1,83,255,238,61,
16,158,255,89,54,255,143,0,0,30,255,152,15,249,191,0,0,43,255,132,0,136,255,153,111,219,246,38,0,5,126,223,242,193,57,0,123,255,255,255,
255,255,243,52,107,107,107,108,242,226,0,0,0,0,73,255,118,0,0,0,0,194,240,14,0,0,0,59,255,133,0,0,0,0,180,247,22,0,0,0,
45,255,148,0,0,0,0,166,251,33,0,0,0,34,252,163,0,0,0,0,151,254,45,0,0,0,0,17,153,229,241,185,48,0,0,177,254,144,119,231,
236,16,0,241,194,0,0,119,255,60,0,167,242,57,32,194,234,6,0,12,196,255,255,238,47,0,1,189,246,140,113,223,233,31,62,255,154,0,0,79,
255,136,72,255,145,0,0,71,255,147,13,226,243,134,109,217,252,62,0,37,171,232,242,198,74,0,0,0,0,0,0,0,0,0,0,21,160,230,235,165,
23,0,3,201,248,137,127,240,206,2,55,255,123,0,0,94,255,82,77,255,107,0,0,62,255,128,25,248,214,33,28,192,255,159,0,98,248,255,254,123,
255,140,0,0,22,66,21,42,255,111,31,223,140,0,0,139,253,34,0,194,251,160,158,254,162,0,0,22,165,236,223,140,4,0,0,136,162,0,0,200,
228,3,0,5,10,0,0,0,0,0,0,0,0,0,0,173,202,2,0,169,198,1,0,136,218,13,0,144,226,15,0,0,0,0,0,0,0,0,0,0,
0,0,0,114,149,0,0,214,179,0,2,249,105,0,33,255,32,0,11,49,0,0,0,0,0,0,0,0,0,6,0,0,0,0,0,0,72,191,95,0,
0,0,3,89,208,255,218,54,0,0,104,223,254,184,74,1,0,0,0,239,254,113,1,0,0,0,0,0,71,190,255,218,109,13,0,0,0,0,0,53,
171,253,242,67,0,0,0,0,0,0,37,152,87,0,0,175,255,255,255,255,255,111,0,0,74,107,107,107,107,107,47,0,0,0,0,0,0,0,0,0,
0,0,175,255,255,255,255,255,111,0,0,74,107,107,107,107,107,47,0,0,0,0,0,0,0,0,0,0,0,6,0,0,0,0,0,0,0,0,158,159,
42,0,0,0,0,0,0,102,238,254,177,58,0,0,0,0,0,10,102,211,255,195,64,0,0,0,0,0,10,167,255,171,0,0,0,31,137,238,250,159,
40,0,0,124,252,242,140,27,0,0,0,0,139,121,16,0,0,0,0,0,0,87,214,246,210,80,0,47,252,207,116,217,250,38,76,179,49,0,93,255,
93,0,0,0,1,167,255,46,0,0,5,166,255,132,0,0,0,95,255,118,0,0,0,0,120,230,11,0,0,0,0,0,0,0,0,0,0,0,132,220,
22,0,0,0,0,136,223,20,0,0,0,0,0,0,0,17,55,26,0,0,0,0,0,0,0,2,111,228,255,255,255,236,132,6,0,0,0,4,190,250,
131,54,13,37,96,232,209,11,0,0,113,253,59,0,29,54,7,31,28,232,143,0,3,240,153,0,129,254,255,213,255,51,103,251,7,36,255,72,45,254,
97,21,134,255,51,40,255,50,76,255,35,101,233,1,0,23,255,51,12,255,75,67,255,42,92,246,7,0,59,255,51,30,255,51,27,255,82,27,248,146,
81,195,254,111,134,231,3,0,227,186,0,81,218,236,138,108,235,212,62,0,0,84,255,120,2,0,0,0,0,0,0,0,0,0,0,139,253,201,125,85,
94,141,86,0,0,0,0,0,0,53,165,207,244,234,198,88,0,0,0,0,0,0,142,255,234,3,0,0,0,0,0,0,225,226,255,68,0,0,0,0,
0,55,255,108,250,155,0,0,0,0,0,139,255,30,194,237,4,0,0,0,0,223,216,0,120,255,71,0,0,0,52,255,139,0,39,255,158,0,0,0,
136,255,255,255,255,255,239,6,0,0,220,230,95,95,95,165,255,75,0,49,255,153,0,0,0,47,255,161,0,133,255,77,0,0,0,0,223,241,7,15,
255,255,255,252,220,120,0,0,15,255,202,107,122,211,255,91,0,15,255,163,0,0,51,255,137,0,15,255,163,0,17,145,252,57,0,15,255,255,255,255,
255,117,0,0,15,255,185,59,65,149,255,121,0,15,255,163,0,0,4,233,224,0,15,255,163,0,0,2,229,236,0,15,255,201,103,108,171,255,166,0,
15,255,255,255,255,230,157,18,0,0,0,42,178,232,233,178,48,0,0,0,57,246,242,164,177,248,244,47,0,1,204,246,42,0,0,63,255,175,0,32,
255,152,0,0,0,0,78,83,0,71,255,104,0,0,0,0,0,0,0,73,255,103,0,0,0,0,0,0,0,34,255,148,0,0,0,0,74,80,0,1,
208,244,36,0,0,61,255,178,0,0,60,248,239,157,172,245,244,44,0,0,0,45,180,233,233,177,44,0,0,15,255,255,255,241,197,100,0,0,15,255,
207,119,137,205,255,152,0,15,255,163,0,0,3,169,254,52,15,255,163,0,0,0,44,255,128,15,255,163,0,0,0,5,253,165,15,255,163,0,0,0,
6,253,164,15,255,163,0,0,0,45,255,126,15,255,163,0,0,3,170,254,50,15,255,205,115,135,204,255,149,0,15,255,255,255,240,197,98,0,0,15,
255,255,255,255,255,255,23,15,255,204,111,111,111,111,10,15,255,163,0,0,0,0,0,15,255,163,0,0,0,0,0,15,255,255,255,255,255,195,0,15,
255,202,107,107,107,82,0,15,255,163,0,0,0,0,0,15,255,163,0,0,0,0,0,15,255,202,107,107,107,107,11,15,255,255,255,255,255,255,27,15,
255,255,255,255,255,255,0,15,255,204,111,111,111,111,0,15,255,163,0,0,0,0,0,15,255,163,0,0,0,0,0,15,255,201,103,103,103,60,0,15,
255,255,255,255,255,147,0,15,255,163,0,0,0,0,0,15,255,163,0,0,0,0,0,15,255,163,0,0,0,0,0,15,255,163,0,0,0,0,0,0,
0,43,178,232,233,177,47,0,0,0,60,247,241,163,175,245,245,51,0,1,208,244,38,0,0,60,253,182,0,35,255,147,0,0,0,0,36,40,0,75,
255,101,0,0,66,83,83,83,2,69,255,107,0,0,203,255,255,255,7,32,255,156,0,0,0,0,179,233,0,1,202,248,45,0,0,34,245,181,0,0,
57,245,242,161,153,237,250,46,0,0,0,41,177,230,235,184,48,0,0,15,255,163,0,0,0,0,247,187,0,15,255,163,0,0,0,0,247,187,0,15,
255,163,0,0,0,0,247,187,0,15,255,163,0,0,0,0,247,187,0,15,255,255,255,255,255,255,255,187,0,15,255,204,111,111,111,111,251,187,0,15,
255,163,0,0,0,0,247,187,0,15,255,163,0,0,0,0,247,187,0,15,255,163,0,0,0,0,247,187,0,15,255,163,0,0,0,0,247,187,0,15,
255,163,0,15,255,163,0,15,255,163,0,15,255,163,0,15,255,163,0,15,255,163,0,15,255,163,0,15,255,163,0,15,255,163,0,15,255,163,0,0,
0,0,0,23,255,147,0,0,0,0,0,23,255,147,0,0,0,0,0,23,255,147,0,0,0,0,0,23,255,147,0,0,0,0,0,23,255,147,0,0,
0,0,0,23,255,147,0,74,143,26,0,23,255,147,0,111,255,84,0,62,255,125,0,42,251,219,125,210,254,55,0,0,82,211,246,216,94,0,0,15,
255,163,0,0,6,195,254,82,15,255,163,0,0,158,255,121,0,15,255,163,0,114,255,163,0,0,15,255,161,60,252,197,7,0,0,15,255,175,225,255,
76,0,0,0,15,255,254,221,245,215,7,0,0,15,255,236,37,114,255,132,0,0,15,255,163,0,3,203,251,48,0,15,255,163,0,0,47,251,205,4,
15,255,163,0,0,0,134,255,120,15,255,163,0,0,0,0,15,255,163,0,0,0,0,15,255,163,0,0,0,0,15,255,163,0,0,0,0,15,255,163,
0,0,0,0,15,255,163,0,0,0,0,15,255,163,0,0,0,0,15,255,163,0,0,0,0,15,255,202,107,107,107,87,15,255,255,255,255,255,207,15,
255,255,171,0,0,0,0,180,255,255,7,15,255,250,247,14,0,0,18,250,249,255,7,15,255,196,255,95,0,0,100,255,195,255,7,15,255,151,234,185,
0,0,188,223,163,255,7,15,255,153,159,252,22,23,252,146,166,255,7,15,255,155,75,255,101,101,255,62,167,255,7,15,255,155,5,237,168,168,229,2,
167,255,7,15,255,155,0,154,227,228,142,0,167,255,7,15,255,155,0,65,255,255,54,0,167,255,7,15,255,155,0,2,229,223,0,0,167,255,7,15,
255,251,43,0,0,0,207,231,0,15,255,255,190,0,0,0,207,231,0,15,255,218,255,86,0,0,207,231,0,15,255,157,203,226,11,0,207,231,0,15,
255,166,68,255,133,0,207,231,0,15,255,171,0,179,248,35,206,231,0,15,255,171,0,34,247,168,198,231,0,15,255,171,0,0,129,253,220,231,0,15,
255,171,0,0,9,223,255,231,0,15,255,171,0,0,0,80,255,231,0,0,0,40,176,231,230,175,38,0,0,0,56,246,242,163,163,243,244,51,0,1,
203,245,41,0,0,44,248,196,0,32,255,150,0,0,0,0,159,255,22,71,255,104,0,0,0,0,113,255,62,72,255,103,0,0,0,0,112,255,64,33,
255,148,0,0,0,0,157,255,23,1,206,244,37,0,0,41,247,200,0,0,58,247,240,157,158,241,246,53,0,0,0,42,177,231,231,177,40,0,0,15,
255,255,255,250,212,100,0,15,255,202,107,120,210,255,85,15,255,163,0,0,43,255,177,15,255,163,0,0,14,251,193,15,255,163,0,11,131,255,137,15,
255,255,255,255,255,201,15,15,255,201,103,97,60,2,0,15,255,163,0,0,0,0,0,15,255,163,0,0,0,0,0,15,255,163,0,0,0,0,0,0,
0,40,176,231,230,175,38,0,0,0,56,246,242,163,163,243,244,51,0,1,203,245,41,0,0,44,248,196,0,32,255,150,0,0,0,0,159,255,22,71,
255,104,0,0,0,0,113,255,62,72,255,103,0,0,0,0,113,255,62,33,255,148,0,35,87,9,158,255,17,1,206,244,37,14,222,176,248,202,0,0,
58,247,240,157,189,255,248,44,0,0,0,42,177,231,232,202,248,51,0,0,0,0,0,0,0,0,141,147,1,15,255,255,255,250,214,105,0,0,15,255,
202,107,120,209,255,89,0,15,255,163,0,0,44,255,176,0,15,255,163,0,0,11,251,189,0,15,255,163,0,9,123,255,131,0,15,255,255,255,255,255,
198,22,0,15,255,201,103,171,255,81,0,0,15,255,163,0,20,242,211,2,0,15,255,163,0,0,130,255,92,0,15,255,163,0,0,15,238,222,5,0,
15,144,224,244,205,86,0,0,180,254,174,137,216,255,75,16,254,181,0,0,29,198,124,9,250,220,44,0,0,0,0,0,110,253,255,210,118,14,0,0,
0,40,131,209,255,239,53,0,0,0,0,0,71,254,179,73,247,106,0,0,7,239,196,13,224,246,164,134,211,255,111,0,37,170,231,246,212,109,0,123,
255,255,255,255,255,255,255,11,54,111,111,192,255,132,111,111,5,0,0,0,143,255,35,0,0,0,0,0,0,143,255,35,0,0,0,0,0,0,143,255,
35,0,0,0,0,0,0,143,255,35,0,0,0,0,0,0,143,255,35,0,0,0,0,0,0,143,255,35,0,0,0,0,0,0,143,255,35,0,0,0,
0,0,0,143,255,35,0,0,0,15,255,163,0,0,0,19,255,155,0,15,255,163,0,0,0,19,255,155,0,15,255,163,0,0,0,19,255,155,0,15,
255,163,0,0,0,19,255,155,0,15,255,163,0,0,0,19,255,155,0,15,255,163,0,0,0,19,255,155,0,10,255,173,0,0,0,29,255,149,0,0,
223,234,18,0,0,110,255,105,0,0,99,255,225,146,170,253,224,14,0,0,0,86,202,243,227,162,28,0,0,133,255,83,0,0,0,0,223,241,7,46,
255,165,0,0,0,47,255,164,0,0,216,241,6,0,0,127,255,79,0,0,129,255,72,0,0,207,242,8,0,0,43,255,154,0,32,255,165,0,0,0,
0,212,232,1,108,255,80,0,0,0,0,126,255,50,177,243,9,0,0,0,0,40,255,121,239,167,0,0,0,0,0,0,209,228,255,82,0,0,0,0,
0,0,123,255,244,9,0,0,0,143,255,60,0,0,103,255,161,0,0,8,249,206,78,255,120,0,0,164,255,223,0,0,61,255,140,16,253,179,0,0,
226,219,255,29,0,120,255,75,0,204,237,1,31,255,124,255,90,0,179,252,14,0,139,255,42,93,255,45,236,152,1,237,199,0,0,73,255,99,151,236,
1,175,211,39,255,134,0,0,12,251,145,200,173,0,111,252,94,255,68,0,0,0,199,194,245,108,0,47,255,185,249,10,0,0,0,134,252,255,44,0,
1,237,253,193,0,0,0,0,68,255,235,1,0,0,175,255,128,0,0,31,244,216,7,0,0,121,255,112,0,0,112,255,130,0,35,246,201,2,0,0,
2,202,247,26,173,250,45,0,0,0,0,47,251,194,255,130,0,0,0,0,0,0,137,255,219,7,0,0,0,0,0,0,175,255,245,33,0,0,0,0,
0,89,255,159,252,188,0,0,0,0,22,236,220,8,147,255,99,0,0,0,173,255,66,0,14,228,240,26,0,87,255,154,0,0,0,78,255,176,0,103,
255,143,0,0,0,71,255,171,3,211,249,31,0,1,204,249,34,0,69,255,161,0,84,255,139,0,0,0,179,253,38,210,236,16,0,0,0,39,251,208,
255,107,0,0,0,0,0,145,255,215,4,0,0,0,0,0,53,255,129,0,0,0,0,0,0,51,255,127,0,0,0,0,0,0,51,255,127,0,0,0,
0,0,0,51,255,127,0,0,0,67,255,255,255,255,255,255,183,29,111,111,111,109,178,255,113,0,0,0,0,30,239,201,2,0,0,0,0,185,250,44,
0,0,0,0,95,255,127,0,0,0,0,23,237,212,6,0,0,0,0,170,253,55,0,0,0,0,79,255,135,0,0,0,0,15,229,243,108,107,107,107,
80,63,255,255,255,255,255,255,191,0,195,255,255,39,0,195,225,75,11,0,195,211,0,0,0,195,211,0,0,0,195,211,0,0,0,195,211,0,0,0,
195,211,0,0,0,195,211,0,0,0,195,211,0,0,0,195,211,0,0,0,195,224,71,11,0,195,255,255,39,170,201,0,0,0,101,253,17,0,0,32,
255,83,0,0,0,219,152,0,0,0,150,221,0,0,0,81,255,34,0,0,16,252,102,0,0,0,200,171,0,0,0,131,238,2,0,0,62,255,53,0,
0,5,243,122,0,0,0,66,55,79,255,255,151,0,23,75,255,151,0,0,0,255,151,0,0,0,255,151,0,0,0,255,151,0,0,0,255,151,0,0,
0,255,151,0,0,0,255,151,0,0,0,255,151,0,0,0,255,151,0,22,71,255,151,0,79,255,255,151,0,0,0,31,43,0,0,0,8,232,255,48,
0,0,108,217,160,168,0,5,225,104,44,252,38,74,205,10,0,162,125,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,255,255,255,255,255,255,23,83,83,83,83,83,83,7,0,169,193,0,0,0,30,242,64,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,67,178,239,227,119,0,
29,249,206,95,171,255,78,0,33,22,0,48,255,137,0,38,123,189,237,255,147,46,248,227,142,93,255,147,119,255,54,0,55,255,147,96,255,150,86,200,
255,147,5,160,242,215,89,255,147,31,255,131,0,0,0,0,0,31,255,131,0,0,0,0,0,31,255,138,157,236,203,51,0,31,255,243,156,118,242,232,
9,31,255,194,0,0,127,255,86,31,255,136,0,0,68,255,122,31,255,135,0,0,68,255,122,31,255,194,0,0,126,255,88,31,255,241,153,114,241,234,
10,31,255,130,159,236,204,52,0,0,18,163,233,234,156,12,0,0,194,248,123,142,254,163,0,53,255,148,0,0,127,117,0,94,255,88,0,0,0,0,
0,94,255,88,0,0,0,0,0,54,255,147,0,0,133,130,0,0,195,247,119,138,253,166,0,0,19,164,234,236,158,13,0,0,0,0,0,0,107,255,
59,0,0,0,0,0,107,255,59,0,38,193,238,170,123,255,59,2,215,249,127,142,243,255,59,61,255,150,0,0,170,255,59,96,255,92,0,0,113,255,
59,95,255,91,0,0,113,255,59,61,255,150,0,0,171,255,59,2,215,248,122,139,242,255,59,0,39,195,239,172,115,255,59,0,22,168,235,228,140,9,
0,1,203,233,119,138,250,145,0,61,255,104,0,0,169,243,1,100,255,255,255,255,255,255,28,86,255,90,31,31,31,31,4,45,255,124,0,0,76,61,
0,0,187,244,109,113,247,193,0,0,16,160,231,239,172,24,0,0,10,177,246,183,0,111,255,158,73,0,146,255,20,0,223,255,255,255,151,66,180,255,
87,45,0,147,255,15,0,0,147,255,15,0,0,147,255,15,0,0,147,255,15,0,0,147,255,15,0,0,147,255,15,0,0,35,191,237,168,109,255,63,
1,211,250,128,143,233,255,63,57,255,154,0,0,170,255,63,93,255,95,0,0,111,255,63,98,255,74,0,0,107,255,63,66,255,127,0,0,158,255,63,
5,223,244,109,122,243,255,63,0,47,200,240,179,119,255,63,0,3,12,0,0,133,255,45,7,219,207,93,108,234,223,4,0,63,196,244,237,179,40,0,
31,255,127,0,0,0,0,0,31,255,127,0,0,0,0,0,31,255,132,159,238,206,49,0,31,255,236,165,158,253,206,0,31,255,192,0,0,195,254,8,
31,255,134,0,0,148,255,19,31,255,131,0,0,147,255,19,31,255,131,0,0,147,255,19,31,255,131,0,0,147,255,19,31,255,131,0,0,147,255,19,
14,175,74,26,227,106,0,0,0,31,255,131,31,255,131,31,255,131,31,255,131,31,255,131,31,255,131,31,255,131,31,255,131,0,14,175,74,0,26,227,
106,0,0,0,0,0,35,255,131,0,35,255,131,0,35,255,131,0,35,255,131,0,35,255,131,0,35,255,131,0,35,255,131,0,35,255,131,0,88,255,
113,71,255,237,31,25,77,17,0,31,255,131,0,0,0,0,0,31,255,131,0,0,0,0,0,31,255,131,0,56,249,187,3,31,255,131,23,228,224,19,
0,31,255,136,194,246,49,0,0,31,255,242,255,145,0,0,0,31,255,248,230,241,27,0,0,31,255,142,73,255,177,0,0,31,255,131,0,165,255,84,
0,31,255,131,0,21,236,232,17,31,255,131,31,255,131,31,255,131,31,255,131,31,255,131,31,255,131,31,255,131,31,255,131,31,255,131,31,255,131,31,
255,117,167,234,138,11,171,239,193,28,0,31,255,228,149,189,255,199,158,172,255,157,0,31,255,177,0,20,253,202,0,4,238,205,0,31,255,132,0,0,
247,167,0,0,211,211,0,31,255,131,0,0,247,167,0,0,211,211,0,31,255,131,0,0,247,167,0,0,211,211,0,31,255,131,0,0,247,167,0,0,
211,211,0,31,255,131,0,0,247,167,0,0,211,211,0,31,255,116,158,237,206,48,0,31,255,227,165,158,253,203,0,31,255,192,0,0,195,252,5,31,
255,134,0,0,148,255,15,31,255,131,0,0,147,255,15,31,255,131,0,0,147,255,15,31,255,131,0,0,147,255,15,31,255,131,0,0,147,255,15,0,
18,163,233,227,149,10,0,0,194,248,123,135,253,168,0,53,255,148,0,0,172,254,28,94,255,88,0,0,113,255,68,94,255,88,0,0,112,255,69,54,
255,147,0,0,171,255,29,0,195,248,119,130,253,169,0,0,19,164,234,229,150,11,0,31,255,130,157,236,203,51,0,31,255,241,156,118,242,232,9,31,
255,194,0,0,127,255,86,31,255,136,0,0,68,255,122,31,255,135,0,0,68,255,122,31,255,194,0,0,125,255,88,31,255,244,153,113,240,234,10,31,
255,139,160,237,204,52,0,31,255,131,0,0,0,0,0,31,255,131,0,0,0,0,0,16,135,70,0,0,0,0,0,0,37,193,238,168,112,255,59,2,
214,249,127,142,235,255,59,60,255,150,0,0,170,255,59,95,255,92,0,0,113,255,59,96,255,91,0,0,113,255,59,61,255,150,0,0,171,255,59,2,
216,248,122,139,244,255,59,0,39,195,239,172,123,255,59,0,0,0,0,0,107,255,59,0,0,0,0,0,107,255,59,0,0,0,0,0,57,135,31,31,
255,131,185,228,31,255,242,193,122,31,255,203,0,0,31,255,136,0,0,31,255,131,0,0,31,255,131,0,0,31,255,131,0,0,31,255,131,0,0,0,
75,207,246,222,99,0,19,247,205,81,164,250,42,40,255,147,0,7,34,0,0,203,255,221,152,54,0,0,6,108,192,252,253,50,17,78,24,0,68,255,
131,55,255,188,83,163,255,87,0,96,216,248,221,115,0,0,153,137,0,0,0,223,199,0,0,223,255,255,255,39,66,233,216,75,11,0,223,199,0,0,
0,223,199,0,0,0,223,199,0,0,0,223,201,0,0,0,200,243,104,14,0,67,222,245,59,31,255,131,0,0,147,255,19,31,255,131,0,0,147,255,
19,31,255,131,0,0,147,255,19,31,255,131,0,0,147,255,19,31,255,132,0,0,151,255,19,19,255,178,0,0,211,255,19,0,221,250,148,168,229,255,
19,0,59,213,236,151,127,255,19,154,255,37,0,0,134,255,52,69,255,113,0,0,209,224,0,4,236,189,0,28,255,139,0,0,156,250,15,103,255,55,
0,0,72,255,83,175,226,1,0,0,5,238,149,237,142,0,0,0,0,159,240,255,57,0,0,0,0,74,255,228,1,0,0,154,255,30,0,112,255,90,
0,61,255,124,86,255,84,0,169,255,144,0,118,255,56,20,254,139,0,226,215,199,0,175,241,4,0,206,193,25,255,107,248,5,228,176,0,0,138,242,
78,239,17,253,70,255,108,0,0,70,255,163,178,0,207,166,255,40,0,0,9,248,246,113,0,142,246,229,0,0,0,0,190,255,48,0,76,255,161,0,
0,68,255,134,0,30,247,173,0,0,179,245,22,159,249,35,0,0,40,251,160,250,137,0,0,0,0,148,255,234,14,0,0,0,0,175,254,242,24,0,
0,0,65,255,117,245,157,0,0,2,209,226,6,141,254,52,0,102,255,93,0,18,239,196,0,154,255,37,0,0,131,255,56,69,255,112,0,0,208,226,
1,4,236,187,0,29,255,142,0,0,156,250,13,106,255,57,0,0,71,255,79,181,227,1,0,0,5,238,143,242,142,0,0,0,0,158,239,255,57,0,
0,0,0,74,255,227,1,0,0,0,0,46,255,143,0,0,0,0,96,190,247,37,0,0,0,18,240,209,91,0,0,0,0,31,255,255,255,255,255,115,
12,99,99,99,215,254,58,0,0,0,70,255,149,0,0,0,14,226,227,13,0,0,0,154,255,76,0,0,0,69,255,166,0,0,0,13,225,255,117,95,
95,55,59,255,255,255,255,255,147,0,0,0,0,15,5,0,0,27,207,255,51,0,0,142,255,82,8,0,0,176,218,0,0,0,0,184,207,0,0,0,
40,237,178,0,0,27,255,200,41,0,0,15,185,253,108,0,0,0,0,214,197,0,0,0,0,180,207,0,0,0,0,172,228,0,0,0,0,131,255,114,
14,0,0,15,176,240,51,0,63,139,13,0,0,115,255,23,0,0,115,255,23,0,0,115,255,23,0,0,115,255,23,0,0,115,255,23,0,0,115,255,
23,0,0,115,255,23,0,0,115,255,23,0,0,115,255,23,0,0,115,255,23,0,0,115,255,23,0,0,115,255,23,0,0,115,255,23,0,0,115,255,
23,0,0,115,255,23,0,78,236,161,7,0,0,25,135,255,106,0,0,0,4,246,146,0,0,0,0,235,155,0,0,0,0,207,219,34,0,0,0,53,
214,255,0,0,0,134,251,178,0,0,0,225,188,0,0,0,0,235,152,0,0,0,7,248,143,0,0,22,134,255,102,0,0,79,238,160,5,0,0,0,
0,0,0,0,0,12,14,0,0,73,230,214,135,24,202,165,0,0,215,204,150,252,255,255,82,0,0,126,53,0,16,81,55,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0
};
const AAGlyph F13_g[] PROGMEM = {
{32,0,0,3,0,0,0},
{33,4,10,4,0,10,0},
{34,7,10,7,0,10,40},
{35,9,10,8,0,10,110},
{36,8,14,8,0,12,200},
{37,13,11,13,0,10,312},
{38,9,10,9,0,10,455},
{39,4,10,4,0,10,545},
{40,5,12,5,0,10,585},
{41,5,12,5,0,10,645},
{42,7,10,7,0,10,705},
{43,9,7,9,0,7,775},
{44,4,4,4,0,1,838},
{45,6,5,6,0,5,854},
{46,4,3,4,0,3,884},
{47,5,12,5,0,10,896},
{48,9,10,9,0,10,956},
{49,6,10,6,0,10,1046},
{50,8,10,8,0,10,1106},
{51,8,10,8,0,10,1186},
{52,9,10,9,0,10,1266},
{53,8,10,8,0,10,1356},
{54,8,10,8,0,10,1436},
{55,7,10,7,0,10,1516},
{56,8,10,8,0,10,1586},
{57,8,11,8,0,11,1666},
{58,4,7,4,0,7,1754},
{59,4,10,4,0,7,1782},
{60,9,8,9,0,8,1822},
{61,9,6,9,0,6,1894},
{62,9,8,9,0,8,1948},
{63,7,10,7,0,10,2020},
{64,13,13,13,0,10,2090},
{65,10,10,9,0,10,2259},
{66,9,10,9,0,10,2359},
{67,10,10,10,0,10,2449},
{68,9,10,9,0,10,2549},
{69,8,10,8,0,10,2639},
{70,8,10,8,0,10,2719},
{71,10,10,10,0,10,2799},
{72,10,10,10,0,10,2899},
{73,4,10,4,0,10,2999},
{74,8,10,8,0,10,3039},
{75,9,10,9,0,10,3119},
{76,7,10,7,0,10,3209},
{77,12,10,12,0,10,3279},
{78,10,10,10,0,10,3399},
{79,10,10,10,0,10,3499},
{80,8,10,8,0,10,3599},
{81,10,11,10,0,10,3679},
{82,9,10,8,0,10,3789},
{83,8,10,8,0,10,3879},
{84,9,10,9,0,10,3959},
{85,10,10,10,0,10,4049},
{86,10,10,9,0,10,4149},
{87,13,10,13,0,10,4249},
{88,10,10,9,0,10,4379},
{89,9,10,9,0,10,4479},
{90,8,10,8,0,10,4569},
{91,5,12,5,0,10,4649},
{92,5,12,5,0,10,4709},
{93,5,12,5,0,10,4769},
{94,6,10,6,0,10,4829},
{95,7,2,6,0,0,4889},
{96,5,10,5,0,10,4903},
{97,7,8,7,0,8,4953},
{98,8,10,8,0,10,5009},
{99,8,8,8,0,8,5089},
{100,8,10,8,0,10,5153},
{101,8,8,8,0,8,5233},
{102,5,11,5,0,11,5297},
{103,8,11,8,0,8,5352},
{104,8,10,8,0,10,5440},
{105,3,11,3,0,11,5520},
{106,4,14,3,-1,11,5553},
{107,8,10,7,0,10,5609},
{108,3,10,3,0,10,5689},
{109,12,8,12,0,8,5719},
{110,8,8,8,0,8,5815},
{111,8,8,8,0,8,5879},
{112,8,11,8,0,8,5943},
{113,8,11,8,0,8,6031},
{114,5,8,5,0,8,6119},
{115,7,8,7,0,8,6159},
{116,5,10,5,0,10,6215},
{117,8,8,8,0,8,6265},
{118,8,8,8,0,8,6329},
{119,11,8,11,0,8,6393},
{120,8,8,7,0,8,6481},
{121,8,11,8,0,8,6545},
{122,7,8,7,0,8,6633},
{123,6,13,6,0,11,6689},
{124,5,16,5,0,13,6767},
{125,6,12,6,0,10,6847},
{126,9,6,9,0,6,6919}
};
const AAFont F13 = { 95, F13_g, F13_bmp };
// F9: Inter-SemiBold.otf 9px, 95 glyphs, 3336 bytes
const uint8_t F9_bmp[] PROGMEM = {
46,255,12,41,255,7,37,255,2,33,253,0,22,183,0,5,71,0,40,230,14,22,253,80,196,0,9,243,68,184,0,0,153,39,115,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,62,164,0,227,0,0,98,129,19,208,0,101,246,238,239,246,137,0,181,46,103,123,0,
183,250,226,240,236,48,11,216,0,189,36,0,46,181,0,219,5,0,0,0,64,42,0,0,1,123,230,223,114,0,74,237,117,98,233,54,55,246,142,52,
0,0,0,104,225,235,122,1,14,14,79,104,239,77,116,216,112,96,238,79,10,144,233,226,129,2,0,0,66,43,0,0,0,156,191,61,0,21,192,5,
0,20,188,60,148,0,164,55,0,0,0,159,189,61,75,143,0,0,0,0,0,0,13,194,11,0,0,0,0,0,0,148,71,57,191,163,0,0,0,59,
160,0,144,64,184,28,0,6,193,19,0,59,189,164,0,0,112,227,170,2,0,3,242,26,196,57,0,0,212,160,220,14,0,2,173,255,70,35,32,101,
200,107,211,182,100,135,179,7,184,253,26,25,193,242,202,197,123,22,253,0,9,243,0,0,153,0,0,0,0,0,0,0,0,0,0,0,0,0,0,68,
214,0,0,171,123,0,0,240,52,0,20,255,17,0,37,254,4,0,6,252,34,0,0,201,97,0,0,92,192,0,0,4,46,0,127,154,0,36,243,14,
0,221,72,0,186,107,0,170,124,0,203,90,17,246,35,105,179,0,30,21,0,10,34,184,44,0,29,180,235,164,12,39,183,210,188,13,0,2,130,3,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,88,96,0,0,0,0,119,131,0,0,44,239,247,248,239,56,0,0,119,131,0,0,
0,0,82,90,0,0,39,239,3,76,168,0,70,70,0,95,243,243,137,0,0,0,0,0,0,0,0,6,72,0,40,228,9,0,0,183,28,0,28,227,
0,0,97,158,0,0,166,89,0,1,232,22,0,48,207,0,0,117,138,0,0,185,69,0,0,0,107,229,225,91,0,42,246,49,62,248,26,115,186,0,
0,208,93,137,164,0,0,185,116,116,185,0,0,207,94,44,246,47,60,248,28,0,109,230,227,94,0,27,190,255,35,143,103,255,35,9,3,255,35,0,
3,255,35,0,3,255,35,0,3,255,35,0,3,255,35,2,156,241,218,68,0,82,226,24,116,220,0,15,25,0,80,209,0,0,0,18,220,96,0,0,
13,201,141,0,0,9,193,161,1,0,0,105,255,249,247,247,15,1,145,239,226,85,0,69,221,26,92,240,0,0,0,2,108,209,0,0,0,198,250,76,
0,0,0,0,58,251,22,96,201,24,67,254,39,8,165,242,230,112,0,0,0,21,241,179,0,0,0,148,218,179,0,0,38,225,113,179,0,0,175,105,
103,179,0,61,222,5,103,179,0,131,247,247,251,253,135,0,0,0,107,179,0,2,251,252,251,216,0,24,249,1,0,0,0,51,238,189,212,81,0,22,
110,44,110,243,11,3,5,0,7,252,41,68,237,28,90,243,10,0,143,239,224,82,0,0,101,226,234,103,0,42,241,45,49,171,11,115,198,170,211,97,
0,135,251,61,83,251,26,109,204,0,0,236,65,44,247,47,74,251,26,0,113,234,230,99,0,161,251,251,253,207,0,0,0,158,151,0,0,27,246,35,
0,0,141,168,0,0,17,243,50,0,0,124,186,0,0,9,234,67,0,0,1,143,237,227,91,0,59,244,27,83,246,3,28,243,42,100,214,0,2,146,
249,253,84,0,88,222,15,49,250,22,119,217,12,42,252,50,12,169,234,230,124,0,0,0,0,0,0,0,2,150,239,216,60,0,89,232,37,109,223,4,
134,171,0,14,255,40,93,232,42,118,255,67,4,151,217,141,245,46,52,160,20,91,224,4,1,160,243,210,55,0,26,164,6,20,140,3,0,0,0,6,
72,0,40,228,9,20,229,27,1,78,3,0,0,0,7,71,3,49,222,0,87,147,0,42,36,0,0,0,0,64,182,27,2,82,201,211,102,4,66,255,
137,1,0,0,1,75,194,216,106,5,0,0,0,57,176,26,23,251,251,251,251,35,0,0,0,0,0,0,23,251,251,251,251,35,0,0,0,0,0,0,
15,188,69,0,0,0,2,97,206,206,87,3,0,0,0,126,255,78,2,101,211,199,81,1,15,181,62,0,0,0,32,179,232,161,11,127,156,14,220,97,
0,0,33,238,65,0,20,228,113,0,0,58,174,0,0,0,18,58,0,0,0,86,199,0,0,0,0,90,176,212,181,102,1,0,0,146,190,52,11,39,
159,163,0,45,225,15,154,201,39,0,198,58,106,147,94,174,28,189,95,124,115,134,120,131,99,0,143,95,109,109,106,146,94,176,33,206,123,167,63,48,
225,17,157,202,103,184,141,2,0,152,191,54,11,26,37,0,0,0,0,96,182,218,204,102,0,0,0,0,170,255,38,0,0,0,9,243,177,123,0,0,
0,80,210,82,209,0,0,0,164,135,14,248,39,0,6,240,245,239,253,124,0,75,229,0,0,97,210,0,158,161,0,0,27,255,41,87,255,251,242,154,
1,87,211,0,46,252,51,87,211,0,58,242,15,87,249,219,244,144,4,87,211,0,12,223,79,87,211,0,23,219,116,87,254,247,246,186,18,0,61,204,
244,198,49,0,23,242,112,34,147,228,3,105,203,0,0,7,87,13,134,167,0,0,0,0,0,106,200,0,0,6,83,12,25,243,105,30,144,227,4,0,
64,206,244,197,48,0,87,255,255,228,143,8,0,87,212,4,39,201,135,0,87,211,0,0,70,226,0,87,211,0,0,39,252,1,87,211,0,0,70,226,
0,87,211,0,36,200,135,0,87,255,255,227,141,8,0,87,255,255,255,231,87,211,0,0,0,87,211,0,0,0,87,255,251,251,169,87,211,0,0,0,
87,211,0,0,0,87,255,251,251,232,87,255,255,255,215,87,211,0,0,0,87,211,0,0,0,87,254,247,247,135,87,211,0,0,0,87,211,0,0,0,
87,211,0,0,0,0,66,205,244,197,52,0,32,244,101,32,135,235,8,112,191,0,0,1,27,5,132,169,0,92,235,235,58,99,214,0,0,17,251,39,
16,238,122,25,135,221,4,0,55,202,244,200,49,0,87,211,0,0,31,255,11,87,211,0,0,31,255,11,87,211,0,0,31,255,11,87,255,255,255,255,
255,11,87,211,0,0,31,255,11,87,211,0,0,31,255,11,87,211,0,0,31,255,11,87,211,87,211,87,211,87,211,87,211,87,211,87,211,0,0,0,
155,139,0,0,0,155,139,0,0,0,155,139,0,0,0,155,139,83,64,0,156,139,145,184,16,206,113,31,201,245,188,15,87,211,0,9,204,161,0,87,
211,0,166,198,7,0,87,210,109,225,23,0,0,87,234,244,208,4,0,0,87,247,63,218,122,0,0,87,211,0,65,247,39,0,87,211,0,0,157,195,
1,87,211,0,0,0,87,211,0,0,0,87,211,0,0,0,87,211,0,0,0,87,211,0,0,0,87,211,0,0,0,87,255,251,251,181,87,255,151,0,
0,80,255,163,87,230,235,4,0,167,229,163,87,200,219,71,10,243,164,163,87,203,140,159,83,208,135,163,87,203,54,230,154,122,135,163,87,203,0,222,
236,37,135,163,87,203,0,135,206,0,135,163,87,255,70,0,7,255,43,87,243,214,4,7,255,43,87,207,194,112,7,255,43,87,214,56,239,29,255,43,
87,215,0,162,143,255,43,87,215,0,25,232,252,43,87,215,0,0,118,255,43,0,60,203,243,194,46,0,23,241,111,30,131,227,10,105,202,0,0,1,
226,75,134,167,0,0,0,192,105,106,200,0,0,1,224,76,25,242,106,25,126,229,11,0,62,205,244,196,48,0,87,255,252,237,136,0,87,211,0,64,
254,57,87,211,0,0,222,94,87,211,0,68,254,54,87,254,247,230,128,0,87,211,0,0,0,0,87,211,0,0,0,0,0,60,203,243,194,46,0,23,
241,111,30,131,227,10,105,202,0,0,1,226,75,134,167,0,0,0,192,105,106,200,0,69,24,225,71,25,242,106,103,227,226,13,0,62,205,244,225,155,
0,0,0,0,0,7,118,14,87,255,252,239,150,1,87,211,0,41,244,75,87,211,0,34,241,70,87,254,248,255,161,4,87,211,0,189,137,0,87,211,
0,80,238,11,87,211,0,3,223,107,1,140,237,235,130,0,74,239,37,45,230,53,53,248,79,5,0,0,0,103,224,236,121,1,11,11,0,58,242,76,
115,217,31,43,239,80,9,166,241,237,148,2,163,255,255,255,255,143,0,0,159,139,0,0,0,0,159,139,0,0,0,0,159,139,0,0,0,0,159,139,
0,0,0,0,159,139,0,0,0,0,159,139,0,0,87,211,0,0,55,243,0,87,211,0,0,55,243,0,87,211,0,0,55,243,0,87,211,0,0,55,
243,0,83,225,0,0,70,239,0,35,252,108,40,182,186,0,0,93,221,244,188,34,0,157,167,0,0,28,255,41,72,241,6,0,107,213,0,5,238,71,
0,186,128,0,0,158,152,14,250,44,0,0,73,226,79,217,0,0,0,5,238,181,132,0,0,0,0,158,255,49,0,0,168,148,0,42,255,81,0,109,
210,103,207,0,101,243,143,0,166,146,38,253,13,161,160,204,0,224,81,0,229,69,221,59,247,38,254,19,0,165,135,238,2,198,135,208,0,0,101,227,
178,0,135,228,144,0,0,36,255,115,0,71,255,79,0,68,243,30,0,148,190,0,0,162,172,47,245,37,0,0,20,235,211,123,0,0,0,0,150,253,
24,0,0,0,41,242,191,160,0,0,2,200,135,33,246,69,0,115,220,9,0,117,222,10,128,212,2,0,110,224,7,11,230,94,10,231,89,0,0,97,
221,115,202,1,0,0,2,208,248,61,0,0,0,0,102,210,0,0,0,0,0,95,203,0,0,0,0,0,95,203,0,0,0,127,255,255,251,255,82,0,
0,0,103,214,6,0,0,30,242,59,0,0,0,181,149,0,0,0,89,228,14,0,0,18,232,72,0,0,0,117,254,246,251,251,90,35,254,202,35,243,
0,35,243,0,35,243,0,35,243,0,35,243,0,35,243,0,35,254,199,158,53,0,0,128,127,0,0,59,196,0,0,4,237,14,0,0,177,78,0,0,
108,147,0,0,39,216,0,0,0,226,29,123,246,123,0,155,123,0,155,123,0,155,123,0,155,123,0,155,123,0,155,123,121,245,123,0,149,221,5,24,
204,134,96,102,89,26,166,0,0,0,0,0,0,0,0,0,0,0,0,235,235,235,235,51,19,222,16,0,51,29,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,14,182,228,188,29,38,88,1,190,125,49,180,196,220,139,151,145,3,190,139,56,215,205,191,139,103,187,0,0,0,0,103,187,0,0,
0,0,103,203,190,236,103,0,103,245,38,89,248,15,103,200,0,7,253,46,103,245,37,86,248,15,103,205,195,237,104,0,5,158,237,196,47,101,216,24,
93,110,145,156,0,0,0,102,214,20,90,114,5,159,236,197,48,0,0,0,35,255,0,0,0,0,35,255,0,10,180,235,154,255,0,108,217,27,127,255,
0,146,156,0,45,255,0,110,216,24,125,255,0,11,182,236,150,255,0,5,160,235,208,46,102,196,10,110,192,146,231,199,199,183,104,203,11,53,77,6,
161,232,205,62,0,116,227,61,1,243,57,2,214,255,234,43,3,255,31,0,3,255,31,0,3,255,31,0,3,255,31,0,10,179,235,153,255,3,108,217,
27,128,255,3,146,157,0,45,255,3,108,219,35,132,255,3,9,163,215,134,255,3,22,87,1,79,234,0,12,175,221,197,80,0,103,183,0,0,0,0,
103,183,0,0,0,0,103,197,192,222,77,0,103,237,29,126,196,0,103,188,0,64,227,0,103,187,0,63,227,0,103,187,0,63,227,0,89,174,11,31,
103,187,103,187,103,187,103,187,103,187,0,89,174,0,11,31,0,103,187,0,103,187,0,103,187,0,103,187,0,103,187,0,128,168,44,205,59,103,187,0,
0,0,0,103,187,0,0,0,0,103,187,11,204,126,0,103,191,178,158,0,0,103,255,251,73,0,0,103,200,100,230,19,0,103,187,0,174,175,0,103,
187,103,187,103,187,103,187,103,187,103,187,103,187,103,188,198,217,79,223,197,19,103,230,23,172,198,20,205,101,103,187,0,131,155,0,167,123,103,187,0,
131,155,0,167,123,103,187,0,131,155,0,167,123,103,188,191,222,77,0,103,237,29,126,196,0,103,188,0,64,227,0,103,187,0,63,227,0,103,187,0,
63,227,0,5,158,239,212,57,0,101,217,26,121,219,0,145,156,0,37,255,9,102,215,22,117,221,0,5,159,238,213,58,0,103,199,190,236,103,0,103,
245,38,89,248,15,103,200,0,7,253,46,103,245,37,86,248,15,103,204,192,237,104,0,103,187,0,0,0,0,77,140,0,0,0,0,11,181,236,153,255,
0,109,217,27,127,255,0,146,156,0,45,255,0,109,216,24,125,255,0,11,181,236,157,255,0,0,0,0,35,255,0,0,0,0,26,191,0,103,198,211,
99,103,240,43,6,103,188,0,0,103,187,0,0,103,187,0,0,28,182,219,182,13,104,207,8,80,29,20,166,235,178,31,56,87,4,196,123,26,193,218,
179,32,8,36,0,55,235,0,219,254,202,55,235,0,55,235,0,50,242,1,2,188,217,103,187,0,63,227,0,103,187,0,63,227,0,103,188,0,64,227,
0,73,230,21,133,227,0,5,177,233,144,227,0,169,140,0,70,232,4,75,224,0,153,141,0,4,233,54,233,47,0,0,144,177,209,0,0,0,50,255,
114,0,0,170,130,0,228,113,5,245,53,95,193,37,229,174,58,233,1,22,245,104,127,219,118,159,0,0,201,208,57,177,208,84,0,0,125,239,3,104,
250,14,0,96,213,4,183,126,0,190,165,216,6,0,81,255,108,0,4,208,136,224,12,118,191,0,170,145,169,140,0,67,232,4,74,223,0,152,141,0,
4,232,53,234,46,0,0,142,179,207,0,0,0,48,255,112,0,0,3,49,247,22,0,0,73,219,97,0,0,0,96,247,247,255,115,0,0,83,223,16,
0,38,235,53,0,10,211,110,0,0,112,255,244,243,133,0,1,88,84,0,76,226,58,0,107,166,0,9,163,146,0,99,229,30,0,2,145,152,0,0,
100,183,0,0,28,193,137,0,180,27,0,235,35,0,235,35,0,235,35,0,235,35,0,235,35,0,235,35,0,235,35,0,235,35,0,235,35,0,235,35,
122,190,27,0,2,180,109,0,0,143,124,0,0,124,182,14,0,21,220,119,0,132,162,6,0,163,116,0,119,199,37,0,3,184,208,49,129,43,37,147,
54,212,199,7,0,0,0,0,0,0,0,0,0,0,0,0
};
const AAGlyph F9_g[] PROGMEM = {
{32,0,0,2,0,0,0},
{33,3,7,3,0,7,0},
{34,5,7,5,0,7,21},
{35,6,7,6,0,7,56},
{36,6,9,6,0,8,98},
{37,9,7,9,0,7,152},
{38,6,7,6,0,7,215},
{39,3,7,3,0,7,257},
{40,4,9,3,0,7,278},
{41,3,9,3,0,7,314},
{42,5,7,5,0,7,341},
{43,6,5,6,0,5,376},
{44,3,3,3,0,1,406},
{45,4,3,4,0,3,415},
{46,3,2,3,0,2,427},
{47,4,8,3,0,7,433},
{48,6,7,6,0,7,465},
{49,4,7,4,0,7,507},
{50,6,7,6,0,7,535},
{51,6,7,6,0,7,577},
{52,6,7,6,0,7,619},
{53,6,7,6,0,7,661},
{54,6,7,6,0,7,703},
{55,5,7,5,0,7,745},
{56,6,7,6,0,7,780},
{57,6,8,6,0,8,822},
{58,3,5,3,0,5,870},
{59,3,7,3,0,5,885},
{60,6,5,6,0,5,906},
{61,6,4,6,0,4,936},
{62,6,5,6,0,5,960},
{63,5,7,5,0,7,990},
{64,9,9,9,0,7,1025},
{65,7,7,7,0,7,1106},
{66,6,7,6,0,7,1155},
{67,7,7,7,0,7,1197},
{68,7,7,6,0,7,1246},
{69,5,7,5,0,7,1295},
{70,5,7,5,0,7,1330},
{71,7,7,7,0,7,1365},
{72,7,7,7,0,7,1414},
{73,2,7,2,0,7,1463},
{74,5,7,5,0,7,1477},
{75,7,7,6,0,7,1512},
{76,5,7,5,0,7,1561},
{77,8,7,8,0,7,1596},
{78,7,7,7,0,7,1652},
{79,7,7,7,0,7,1701},
{80,6,7,6,0,7,1750},
{81,7,8,7,0,7,1792},
{82,6,7,6,0,7,1848},
{83,6,7,6,0,7,1890},
{84,6,7,6,0,7,1932},
{85,7,7,7,0,7,1974},
{86,7,7,7,0,7,2023},
{87,9,7,9,0,7,2072},
{88,7,7,6,0,7,2135},
{89,7,7,6,0,7,2184},
{90,6,7,6,0,7,2233},
{91,3,8,3,0,7,2275},
{92,4,8,3,0,7,2299},
{93,3,8,3,0,7,2331},
{94,4,6,4,0,6,2355},
{95,5,1,4,0,0,2379},
{96,3,7,3,0,7,2384},
{97,5,5,5,0,5,2405},
{98,6,7,6,0,7,2430},
{99,5,5,5,0,5,2472},
{100,6,7,6,0,7,2497},
{101,5,5,5,0,5,2539},
{102,4,7,4,0,7,2564},
{103,6,7,6,0,5,2592},
{104,6,7,6,0,7,2634},
{105,2,7,2,0,7,2676},
{106,3,9,2,-1,7,2690},
{107,6,7,5,0,7,2717},
{108,2,7,2,0,7,2759},
{109,8,5,8,0,5,2773},
{110,6,5,6,0,5,2813},
{111,6,5,5,0,5,2843},
{112,6,7,6,0,5,2873},
{113,6,7,6,0,5,2915},
{114,4,5,4,0,5,2957},
{115,5,5,5,0,5,2977},
{116,3,7,3,0,7,3002},
{117,6,5,6,0,5,3023},
{118,6,5,5,0,5,3053},
{119,8,5,8,0,5,3083},
{120,5,5,5,0,5,3123},
{121,6,7,5,0,5,3148},
{122,5,5,5,0,5,3190},
{123,4,8,4,0,7,3215},
{124,3,11,3,0,9,3247},
{125,4,8,4,0,7,3280},
{126,6,4,6,0,4,3312}
};
const AAFont F9 = { 95, F9_g, F9_bmp };

// ======================================================================
// ---------------- NEW LOOK (the "Clean" theme) ----------------
// Black screen, colour only for level (green / amber / red, blended
// across the amber and red settings), anti-aliased Inter text, smooth
// rounded shapes, and a reading that glides instead of jumping.
// The measurement itself is never smoothed: MAX, Leq, the limit alarm,
// alerts and the log all use the true value. Only the screen eases.
// The Studio and Bold looks are still there (Settings > General > Look).
// ======================================================================
const AAFont* const AAF[] = { &F54, &F46, &F30, &F17, &F13, &F9 };
enum { FT54 = 0, FT46, FT30, FT17, FT13, FT9 };
const int8_t AAF_TRACK[] = { -2, -2, -1, 0, 0, 0 };   // big digits sit a little tighter

#define CL_SURFACE  0x18E3
#define CL_SURFACE2 0x2965
#define CL_TEXT     0xFFFF
#define CL_TEXT2    0x8C72
#define CL_TEXT3    0x52AB
#define CL_SAFE     0x368B
#define CL_CAUTION  0xFCE1
#define CL_OVER     0xFA27
#define CL_SELECT   0x0C3F
const int CL_FOOT = 114;                  // footer baseline on every screen

static inline uint16_t blend565(uint16_t b, uint16_t f, uint8_t a) {
  uint16_t i = 255 - a;
  uint16_t r = ((((f >> 11) & 0x1F) * a) + (((b >> 11) & 0x1F) * i) + 127) / 255;
  uint16_t g = ((((f >>  5) & 0x3F) * a) + (((b >>  5) & 0x3F) * i) + 127) / 255;
  uint16_t l = ((( f        & 0x1F) * a) + (( b        & 0x1F) * i) + 127) / 255;
  return (r << 11) | (g << 5) | l;
}
static inline float clamp01(float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }
uint16_t mixc(uint16_t a, uint16_t b, float t) {          // a -> b, t = 0..1
  if (t <= 0) return a;
  if (t >= 1) return b;
  return blend565(a, b, (uint8_t)(t * 255 + 0.5f));
}
uint16_t fadeIn(uint16_t c, float a) { return mixc(C_BG, c, a); }   // colour faded up from black

static inline void pxAA(int x, int y, uint16_t c, uint8_t a) {
  if (!a || x < 0 || y < 0 || x > 127 || y > 127) return;
  uint16_t* p = canvas.getBuffer() + y * 128 + x;
  *p = a == 255 ? c : blend565(*p, c, a);
}

// rounded rectangle with soft (anti-aliased) edges
void fillRRAA(int x, int y, int w, int h, int r, uint16_t c) {
  if (w <= 0 || h <= 0) return;
  if (r > w / 2) r = w / 2;
  if (r > h / 2) r = h / 2;
  for (int py = y; py < y + h; py++) {
    float fy = py + 0.5f;
    float cy = fy < y + r ? y + r : (fy > y + h - r ? y + h - r : fy);
    for (int px = x; px < x + w; px++) {
      float fx = px + 0.5f;
      float cx = fx < x + r ? x + r : (fx > x + w - r ? x + w - r : fx);
      float dx = fx - cx, dy = fy - cy;
      if (dx == 0 && dy == 0) { pxAA(px, py, c, 255); continue; }
      float cov = clamp01(r + 0.5f - sqrtf(dx * dx + dy * dy));
      pxAA(px, py, c, (uint8_t)(cov * 255 + 0.5f));
    }
  }
}

int aaGlyph(int f, char ch) {             // index of the glyph in font f, -1 if it has none
  const AAFont* F = AAF[f];
  if (F->count == 95) { int i = (uint8_t)ch - 32; return (i >= 0 && i < 95) ? i : -1; }
  for (int i = 0; i < F->count; i++) if (F->g[i].cp == (uint8_t)ch) return i;
  return -1;
}
int textW(int f, const char* s) {
  int w = 0, n = 0;
  for (; *s; s++) { int i = aaGlyph(f, *s); if (i >= 0) { w += AAF[f]->g[i].adv; n++; } }
  if (n > 1) w += AAF_TRACK[f] * (n - 1);
  return w;
}
void textAA(int f, const char* s, int x, int baseline, uint16_t col) {
  const AAFont* F = AAF[f];
  for (; *s; s++) {
    int i = aaGlyph(f, *s);
    if (i < 0) continue;
    const AAGlyph& g = F->g[i];
    const uint8_t* p = F->bmp + g.off;
    for (int yy = 0; yy < g.h; yy++)
      for (int xx = 0; xx < g.w; xx++) pxAA(x + g.dx + xx, baseline - g.dy + yy, col, *p++);
    x += g.adv + AAF_TRACK[f];
  }
}
void textAAc(int f, const char* s, int cx, int bl, uint16_t c) { textAA(f, s, cx - textW(f, s) / 2, bl, c); }
void textAAr(int f, const char* s, int rx, int bl, uint16_t c) { textAA(f, s, rx - textW(f, s), bl, c); }
void upperCopy(char* out, const char* in, size_t n) {
  size_t i = 0;
  for (; in[i] && i < n - 1; i++) out[i] = (in[i] >= 'a' && in[i] <= 'z') ? in[i] - 32 : in[i];
  out[i] = 0;
}

// level colour, blended over 4 dB around the amber and red settings
uint16_t levelColourSmooth(float v) {
  const float F = 2.0f;
  if (v < amberFrom - F) return CL_SAFE;
  if (v < amberFrom + F) return mixc(CL_SAFE, CL_CAUTION, (v - amberFrom + F) / (2 * F));
  if (v < redFrom - F)   return CL_CAUTION;
  if (v < redFrom + F)   return mixc(CL_CAUTION, CL_OVER, (v - redFrom + F) / (2 * F));
  return CL_OVER;
}

// ---- the gliding reading (screen only) ----
float uiDb = 0, uiFast = 0; uint32_t uiAt = 0; int uiMode = -1;
void easeReading(float db, float fastDb, uint32_t now) {
  bool snapNow = warmup > 0 || uiAt == 0 || uiMode != rMode * 3 + wMode || now - uiAt > 1000;
  float k = snapNow ? 1.0f : 1.0f - expf(-(float)(now - uiAt) / 120.0f);   // ~0.12 s glide
  uiDb += (db - uiDb) * k;
  uiFast += (fastDb - uiFast) * k;
  uiAt = now; uiMode = rMode * 3 + wMode;
}

// ---- pieces shared by the screens ----
void clHeader(uint32_t now) {               // weighting pill, WiFi, battery
  bool edit = editUntil && (int32_t)(now - editUntil) < 0;
  fillRRAA(7, 7, 33, 14, 7, edit ? CL_SELECT : CL_SURFACE);
  textAAc(FT9, W_NAME[wMode], 23, 18, edit ? CL_TEXT : CL_TEXT2);
  if (wifiOn) drawWifiIcon(64, 7, phoneLinked() ? CL_SELECT : CL_TEXT3);
  if (wiredModel) return;
  if (gauge == G_NONE) { textAAr(FT9, "--", 120, 18, CL_TEXT3); return; }
  bool low = battLow(), blinkOn = (now / 500) % 2 == 0;
  if (low && !blinkOn) return;
  uint16_t fill = charging ? (battPct < BATT_CHARGE_RED_PCT ? CL_OVER : CL_SAFE)
                : battPct < BATT_LOW_PCT ? CL_OVER : battPct < 30 ? CL_CAUTION : CL_SAFE;
  canvas.drawRoundRect(100, 9, 19, 10, 3, CL_TEXT3);
  canvas.fillRect(119, 12, 2, 4, CL_TEXT3);
  int bw = constrain(battPct, 8, 100) * 15 / 100;
  fillRRAA(102, 11, bw, 6, 2, fill);
  if (charging) drawBolt(92, 9, fill);
}

void clFooter(const char* label, const char* value, uint16_t vcol, const char* right, uint16_t rcol) {
  textAA(FT9, label, 12, CL_FOOT, CL_TEXT2);
  textAA(FT9, value, 12 + textW(FT9, label) + 5, CL_FOOT, vcol);
  textAAr(FT9, right, 116, CL_FOOT, rcol);
}

void clLimitFrame() {                       // amber near the limit, red over it
  if (!(limOn && limState > 0)) return;
  uint16_t c = limState == 2 ? CL_OVER : CL_CAUTION;
  canvas.drawRoundRect(0, 0, 128, 128, 8, c);
  canvas.drawRoundRect(1, 1, 126, 126, 7, c);
}

// a status word for the footer's right side: mic problems win
const char* clRightWord(char* buf, size_t n, uint16_t& col) {
  if (micFaulty()) { col = CL_OVER; return "MIC!"; }
  if (micClipping()) { col = CL_OVER; return "CLIP"; }
  col = CL_TEXT2; upperCopy(buf, R_NAME[rMode], n); return buf;
}

// ---- pop-up card (toasts) ----
void clToast(uint32_t now) {
  bool live = toastUntil && (int32_t)(now - toastUntil) < 0;
  float a;
  if (live) a = clamp01((now - toastAt) / 150.0f);
  else {
    if (toastUntil) { toastFadeAt = toastUntil; toastUntil = 0; }
    if (!toastFadeAt || now - toastFadeAt >= 200) { toastFadeAt = 0; return; }
    a = 1.0f - (now - toastFadeAt) / 200.0f;
  }
  a = a * a * (3 - 2 * a);                  // ease in and out
  fillRRAA(7, 26, 114, 76, 10, fadeIn(CL_SURFACE, a));
  bool edit = editUntil && (int32_t)(now - editUntil) < 0;
  if (edit) {                               // weighting picker
    static const char* OPT[3] = {"A", "C", "Z"};
    textAAc(FT9, "WEIGHTING", 64, 43, fadeIn(CL_TEXT2, a));
    for (int i = 0; i < 3; i++) {
      int x = 13 + i * 34;
      fillRRAA(x + 1, 52, 32, 22, 11, fadeIn(i == wMode ? CL_SELECT : CL_SURFACE2, a));
      textAAc(FT13, OPT[i], x + 17, 68, fadeIn(i == wMode ? CL_TEXT : CL_TEXT2, a));
    }
    textAAc(FT9, "tap to change", 64, 90, fadeIn(CL_TEXT3, a));
    return;
  }
  int f = textW(FT17, toastBig) <= 104 ? FT17 : FT13;
  bool two = toastSmall[0] != 0;
  textAAc(f, toastBig, 64, two ? 63 : 70, fadeIn(CL_TEXT, a));
  if (two) textAAc(FT9, toastSmall, 64, 81, fadeIn(CL_TEXT2, a));
}

// ---- main meter: FAST / SLOW / LEQ ----
void clMeter(uint32_t now, float db, float fastDb) {
  bool warm = warmup > 0, fault = micFaulty(), overload = fastDb > OVERLOAD_DB;
  char buf[24], rb[8];
  easeReading(db, fastDb, now);
  canvas.fillScreen(C_BG);
  if (!warm && fastDb >= redFrom) canvas.fillRect(0, 0, 128, 2, CL_OVER);
  clHeader(now);

  if (fault) {
    textAAc(FT17, "Mic fault", 64, 62, CL_OVER);
    textAAc(FT9, "Check the mic wires", 64, 78, CL_TEXT2);
  } else {
    int f = FT54, bl = 72;
    if (warm) strcpy(buf, "---");
    else if (overload) { strcpy(buf, "OVER"); f = FT30; bl = 68; }
    else snprintf(buf, sizeof(buf), "%.1f", uiDb);
    if (f == FT54 && strlen(buf) > 4) { f = FT46; bl = 70; }
    uint16_t nc = warm ? CL_TEXT3 : (overload || db >= redFrom) ? CL_OVER : CL_TEXT;
    textAAc(f, buf, 64, bl, nc);
  }

  fillRRAA(12, 90, 104, 9, 4, CL_SURFACE);
  if (!warm && !fault) {
    int w = (int)(104 * clamp01((uiFast - BAR_MIN_DB) / (BAR_MAX_DB - BAR_MIN_DB)) + 0.5f);
    if (w > 0) fillRRAA(12, 90, w < 4 ? 4 : w, 9, 4, levelColourSmooth(uiFast));
    if (peakDb > BAR_MIN_DB) {               // peak hold (2 s)
      int tx = 12 + (int)(104 * clamp01((peakDb - BAR_MIN_DB) / (BAR_MAX_DB - BAR_MIN_DB)) + 0.5f);
      if (tx > 114) tx = 114;
      if (tx < 13) tx = 13;
      canvas.fillRect(tx - 1, 87, 2, 15, CL_TEXT);
    }
  }

  uint16_t rc; const char* right = clRightWord(rb, sizeof(rb), rc);
  if (rMode == R_LEQ) {
    unsigned long secs = snapLeqSecs();
    if (secs >= 3600) snprintf(buf, sizeof(buf), "%luh %02lum", secs / 3600, (secs / 60) % 60);
    else snprintf(buf, sizeof(buf), "%02lu:%02lu", secs / 60, secs % 60);
    clFooter("TIME", buf, CL_TEXT, right, rc);
  } else {
    if (warm || snap.maxF[wMode] <= 0) strcpy(buf, "--"); else snprintf(buf, sizeof(buf), "%.1f", toDb(snap.maxF[wMode]));
    clFooter("MAX", buf, CL_TEXT, right, rc);
  }
  clLimitFrame();
  clToast(now);
}

// ---- RTA: 31 bands, 20 Hz - 20 kHz, 30 - 110 dB ----
void clRta(uint32_t now, float fastDb) {
  const int top = 30, bottom = 86, x0 = 2;
  const float lo = 30, hi = 110;
  bool warm = warmup > 0;
  char buf[24], rb[8];
  canvas.fillScreen(C_BG);
  clHeader(now);
  for (float g = 50; g < hi; g += 20) {
    int y = bottom - (int)((g - lo) / (hi - lo) * (bottom - top));
    for (int x = x0; x < x0 + 124; x += 2) canvas.drawPixel(x, y, CL_SURFACE2);   // dotted guides
  }
  for (int b = 0; b < RTA_BANDS; b++) {
    int x = x0 + b * 4;
    canvas.fillRect(x, bottom, 3, 1, CL_SURFACE2);
    if (warm || !rtaReady) continue;
    float d = rtaBandDb(b);
    int h = (int)((constrain(d, lo, hi) - lo) / (hi - lo) * (bottom - top) + 0.5f);
    if (h > 0) fillRRAA(x, bottom - h, 3, h + 1, 1, levelColourSmooth(d));
  }
  const int lab[] = {5, 11, 17, 23, 29};                        // 63, 250, 1k, 4k, 16k
  for (int i = 0; i < 5; i++) {
    int cx = x0 + lab[i] * 4 + 1, w = textW(FT9, RTA_LABEL[lab[i]]);
    if (cx + w / 2 > 125) textAAr(FT9, RTA_LABEL[lab[i]], 125, 98, CL_TEXT3);
    else textAAc(FT9, RTA_LABEL[lab[i]], cx, 98, CL_TEXT3);
  }
  if (warm) strcpy(buf, "--"); else if (fastDb > OVERLOAD_DB) strcpy(buf, "OVER"); else snprintf(buf, sizeof(buf), "%.1f", fastDb);
  uint16_t rc; const char* right = clRightWord(rb, sizeof(rb), rc);
  clFooter("LEVEL", buf, warm ? CL_TEXT3 : levelColourSmooth(fastDb), right, rc);
  clLimitFrame();
  clToast(now);
}

// ---- GIG: the limit (or 15 min) Leq, the bar against the limit, dose ----
void clGig(uint32_t now) {
  char buf[32], rb[8];
  bool warm = warmup > 0;
  int k = limOn ? limWin : 2, w = limOn ? limW : wMode;
  float v = 0; bool have = !warm && rollDb(k, w, v);
  uint16_t col = !have ? CL_TEXT3 : limOn ? (limState == 2 ? CL_OVER : limState == 1 ? CL_CAUTION : CL_SAFE) : levelColourSmooth(v);
  canvas.fillScreen(C_BG);
  clHeader(now);
  snprintf(buf, sizeof(buf), "L%ceq  %s", w == W_A ? 'A' : w == W_C ? 'C' : 'Z', LIM_WIN_NAME[k]);
  textAA(FT9, buf, 12, 34, CL_TEXT2);
  if (have && !rollFull(k)) textAAr(FT9, "so far", 116, 34, CL_TEXT3);

  if (have) snprintf(buf, sizeof(buf), "%.1f", v); else strcpy(buf, "---");
  textAAc(FT46, buf, 64, 73, have ? (limOn && limState == 2 ? CL_OVER : CL_TEXT) : CL_TEXT3);

  if (limOn) {                               // bar from limit-20 to limit+5, white tick at the limit
    float lo = limDb - 20, hi = limDb + 5;
    fillRRAA(12, 82, 104, 7, 3, CL_SURFACE);
    if (have) { int fw = (int)((constrain(v, lo, hi) - lo) / (hi - lo) * 104 + 0.5f); if (fw > 0) fillRRAA(12, 82, fw < 4 ? 4 : fw, 7, 3, col); }
    int lx = 12 + (int)((limDb - lo) / (hi - lo) * 104);
    canvas.fillRect(lx - 1, 79, 2, 13, CL_TEXT);
    snprintf(buf, sizeof(buf), "LIMIT %d", limDb);
    textAAr(FT9, buf, 116, 101, CL_TEXT2);
  } else {
    textAAr(FT9, "NO LIMIT", 116, 101, CL_TEXT3);
  }
  float v1;
  textAA(FT9, "1 MIN", 12, 101, CL_TEXT2);
  if (!warm && rollDb(0, w, v1)) snprintf(buf, sizeof(buf), "%.1f", v1); else strcpy(buf, "--");
  textAA(FT9, buf, 12 + textW(FT9, "1 MIN") + 5, 101, CL_TEXT);

  float d = warm ? 0 : dosePct();
  snprintf(buf, sizeof(buf), "%.0f%%", d);
  uint16_t rc; const char* right = clRightWord(rb, sizeof(rb), rc);
  clFooter("DOSE", buf, d >= 100 ? CL_OVER : d >= 50 ? CL_CAUTION : CL_TEXT, right, rc);
  clLimitFrame();
  clToast(now);
}

// ---- other screens ----
void clStandby(uint32_t now) {
  char b[32], nb[40];
  canvas.fillScreen(C_BG);
  textAAc(FT17, "Standby", 64, 54, CL_TEXT);
  int wm = weekMinuteNow(), nx = wm >= 0 ? nextChange(wm) : -1;
  textAAc(FT9, "COMES ON", 64, 76, CL_TEXT3);
  if (nx >= 0) { fmtWeekMinute(nx, b, sizeof(b)); textAAc(FT13, b, 64, 93, CL_TEXT); }
  if (wm >= 0) { fmtWeekMinute(wm, b, sizeof(b)); snprintf(nb, sizeof(nb), "Now %s", b); textAAc(FT9, nb, 64, 116, CL_TEXT3); }
}

void clCalibration(uint32_t now) {
  canvas.fillScreen(C_BG);
  textAAc(FT9, "CALIBRATE", 64, 18, CL_TEXT2);
  textAAc(FT54, "94.0", 64, 72, CL_TEXT);
  textAAc(FT9, calState == CAL_SETTLE ? "Fit the calibrator" : "Measuring", 64, 88, CL_TEXT2);
  uint32_t el = now - calT0 + (calState == CAL_MEASURE ? 2000 : 0);
  int w = constrain((int)(el * 104 / 7000), 0, 104);
  fillRRAA(12, 100, 104, 6, 3, CL_SURFACE);
  if (w > 0) fillRRAA(12, 100, w < 6 ? 6 : w, 6, 3, CL_SELECT);
}

void clUpdateScreen(const char* big, const char* small, int pct, uint16_t colour, const char* toVer) {
  canvas.fillScreen(C_BG);
  textAAc(FT9, "UPDATE", 64, 18, CL_TEXT2);
  String v = "v" FW_VERSION;
  if (toVer[0]) { v += "  to  v"; v += toVer; }
  textAAc(FT9, v.c_str(), 64, 32, CL_TEXT3);
  textAAc(textW(FT17, big) <= 116 ? FT17 : FT13, big, 64, 60, colour);
  if (pct >= 0) {
    int p = constrain(pct, 0, 100);
    fillRRAA(12, 70, 104, 6, 3, CL_SURFACE);
    if (p > 0) fillRRAA(12, 70, max(6, 104 * p / 100), 6, 3, colour);
    char b[8]; snprintf(b, sizeof(b), "%d%%", p);
    textAAc(FT13, b, 64, 93, CL_TEXT);
  }
  textAAc(FT9, small, 64, 110, CL_TEXT2);
}

void clPowerOff(const char* l1, const char* l2) {
  canvas.fillScreen(C_BG);
  textAAc(FT17, l1, 64, 56, CL_OVER);
  textAAc(FT17, l2, 64, 76, CL_OVER);
  textAAc(FT9, "Charge with USB-C", 64, 100, CL_TEXT2);
}

// ---- start-up: logo, a progress bar that tracks the real start-up, and details underneath ----
char bootLab[3][14], bootVal[3][24]; uint16_t bootCol[3]; int bootRows = 0;
void bootRow(int i, const char* label, const char* value, uint16_t col) {
  upperCopy(bootLab[i], label, sizeof(bootLab[i]));
  strncpy(bootVal[i], value, sizeof(bootVal[i]) - 1); bootVal[i][sizeof(bootVal[i]) - 1] = 0;
  bootCol[i] = col;
  if (i + 1 > bootRows) bootRows = i + 1;
}
void clBootFrame(float prog, float fade) {
  canvas.fillScreen(C_BG);
  int wa = textW(FT17, "Pocket"), wb = textW(FT17, "SPL");
  int x = 64 - (wa + wb) / 2;
  textAA(FT17, "Pocket", x, 50, fadeIn(CL_TEXT2, fade));
  textAA(FT17, "SPL", x + wa, 50, fadeIn(CL_TEXT, fade));
  fillRRAA(34, 62, 60, 4, 2, fadeIn(CL_SURFACE, fade));
  int w = (int)(60 * clamp01(prog) + 0.5f);
  if (w > 0) fillRRAA(34, 62, w < 4 ? 4 : w, 4, 2, fadeIn(CL_SAFE, fade));
  if (!bootRows) { textAAc(FT9, "SOUND LEVEL METER", 64, 86, fadeIn(CL_TEXT3, fade)); return; }
  for (int i = 0; i < bootRows; i++) {
    int y = 86 + i * 14;
    textAA(FT9, bootLab[i], 10, y, fadeIn(CL_TEXT3, fade));
    textAAr(FT9, bootVal[i], 118, y, fadeIn(bootCol[i], fade));
  }
}
float bootProg = 0;
void bootAnim(float to, uint32_t ms, float fadeFrom, float fadeTo) {   // glide the bar (and fade) over ms
  float from = bootProg;
  uint32_t t0 = millis();
  while (true) {
    float t = ms ? clamp01((millis() - t0) / (float)ms) : 1.0f;
    float e = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
    clBootFrame(from + (to - from) * e, fadeFrom + (fadeTo - fadeFrom) * t);
    pushCanvas();
    if (t >= 1.0f) break;
    delay(5);
  }
  bootProg = to;
}

void powerOff(const char* l1, const char* l2) {
  if (theme == TH_CLEAN) clPowerOff(l1, l2);
  else {
    canvas.fillScreen(C_BG);
    canvas.setFont(L.big); canvas.setTextColor(C_RED);
    textCentered(l1, 64, 56);
    textCentered(l2, 64, 80);
    canvas.setFont(L.small); canvas.setTextColor(C_DIM);
    textCentered("Charge with USB-C", 64, 100);
  }
  pushCanvas(); delay(3000);
  oled.enableDisplay(false);
  if (wifiOn) wifiStop();
  esp_deep_sleep_start();   // stays off until the power switch is turned off and on
}


// ---------------- Screen layout (128 x 128) ----------------
// Header 0-20: weighting, WiFi, response. Body 22-100. Footer 104-127: one reading + battery.
void drawHeader(uint32_t now) {        // weighting left, WiFi middle, response / RTA right
  canvas.setFont(L.label);
  canvas.setTextColor(C_ACCENT);
  if (editUntil && (int32_t)(now - editUntil) < 0) {   // weighting can be changed: shown lit, like the phone
    int16_t x1, y1; uint16_t w, h;
    canvas.getTextBounds(W_NAME[wMode], 4, 15, &x1, &y1, &w, &h);
    canvas.fillRoundRect(x1 - 3, 1, w + 6, 18, 3, C_ACCENT);
    canvas.setTextColor(C_BG);
  }
  canvas.setCursor(4, 15); canvas.print(W_NAME[wMode]);
  bool fault = micFaulty(), clip = micClipping();
  canvas.setTextColor(fault || clip ? C_RED : C_DIM);
  textRight(fault ? "Mic!" : clip ? "Clip" : R_NAME[rMode], 124, 15);
  if (wifiOn) drawWifiIcon(70, 4, phoneLinked() ? C_TEXT : C_FAINT);
  canvas.drawFastHLine(4, 20, 120, C_FAINT);
}

void drawBatteryCorner(uint32_t now) {   // bottom right: % and battery, a bolt while charging; flashes red when low
  if (wiredModel) return;                  // wired model: no battery to show
  bool low = battLow();
  bool blinkOn = (now / 500) % 2 == 0;
  char buf[12];
  canvas.setFont(L.small); canvas.setTextSize(1);
  uint16_t bcol = onColourBg ? C_TEXT : batteryColour();
  if (gauge == G_NONE) {
    canvas.setTextColor(C_DIM); textRight("--", 124, 109);
  } else if (!low || blinkOn) {
    drawBatteryIcon(102, 110, battPct, bcol);
    int textRightX = 97;
    if (charging) { drawBolt(93, 110, bcol); textRightX = 90; }
    canvas.setTextColor(bcol);
    snprintf(buf, sizeof(buf), "%d%%", battPct);
    textRight(buf, textRightX, 109);
  }
  if (low && blinkOn) { canvas.drawRect(0, 0, 128, 128, C_RED); canvas.drawRect(1, 1, 126, 126, C_RED); }
}

void drawToast(uint32_t now) {
  if (limOn && limState > 0 && !(battLow() && (now / 500) % 2 == 0)) {
    uint16_t c = limState == 2 ? C_RED : C_AMBER;
    canvas.drawRect(0, 0, 128, 128, c); canvas.drawRect(1, 1, 126, 126, c);
  }
  if (!toastUntil || (int32_t)(now - toastUntil) >= 0) { toastUntil = 0; return; }
  canvas.fillRoundRect(6, 27, 116, 50, 4, C_PANEL);
  canvas.drawRoundRect(6, 27, 116, 50, 4, C_FAINT);
  canvas.setFont(L.big); canvas.setTextColor(C_TEXT);
  int16_t x1, y1; uint16_t w, h;               // long words use the smaller font so they fit
  canvas.getTextBounds(toastBig, 0, 60, &x1, &y1, &w, &h);
  if (w > 108) canvas.setFont(L.label);
  textCentered(toastBig, 64, toastSmall[0] ? 53 : 60);
  if (toastSmall[0]) {
    canvas.setFont(L.small); canvas.setTextColor(C_DIM);
    textCentered(toastSmall, 64, 59);
  }
}

// LED ladder, 40 segments from 60 to 110 dB (the phone page draws the same one)
void drawLadder(int y, int h, float level, float peak, bool show) {
  const int segs = 40, x0 = 4;
  int pkI = (show && peak > BAR_MIN_DB) ? constrain((int)((peak - BAR_MIN_DB) / (BAR_MAX_DB - BAR_MIN_DB) * segs), 0, segs - 1) : -1;
  for (int i = 0; i < segs; i++) {
    float lo = BAR_MIN_DB + i * (BAR_MAX_DB - BAR_MIN_DB) / segs;
    uint16_t c = (show && level >= lo) ? levelColour(lo) : (i == pkI ? C_TEXT : C_FAINT);
    canvas.fillRect(x0 + i * 3, y, 2, h, c);
  }
}

void drawFooterReading(const char* label, const char* value, uint16_t col) {   // e.g. "Max 92.1" bottom left
  canvas.setFont(L.small); canvas.setTextSize(1);
  canvas.setTextColor(C_DIM); canvas.setCursor(4, 109); canvas.print(label); canvas.print(" ");
  canvas.setTextColor(col); canvas.print(value);
}

// Clean theme: a 270-degree ring from 60 to 110 dB, coloured along its length
void drawRing(int cx, int cy, int r, float level, float peak, bool show) {
  const float a0 = 135, span = 270;
  float litTo = show ? constrain((level - BAR_MIN_DB) / (BAR_MAX_DB - BAR_MIN_DB), 0.0f, 1.0f) : 0;
  for (float a = 0; a <= span; a += 3) {
    float rad = (a0 + a) * (float)M_PI / 180.0f;
    float v = BAR_MIN_DB + a / span * (BAR_MAX_DB - BAR_MIN_DB);
    uint16_t c = (a / span <= litTo && litTo > 0) ? levelColour(v) : C_FAINT;
    canvas.fillCircle(cx + (int)lroundf(cosf(rad) * r), cy + (int)lroundf(sinf(rad) * r), 3, c);
  }
  if (show && peak > BAR_MIN_DB) {                               // peak: a white dot on the ring
    float a = constrain((peak - BAR_MIN_DB) / (BAR_MAX_DB - BAR_MIN_DB), 0.0f, 1.0f) * span;
    float rad = (a0 + a) * (float)M_PI / 180.0f;
    canvas.fillCircle(cx + (int)lroundf(cosf(rad) * r), cy + (int)lroundf(sinf(rad) * r), 2, C_TEXT);
  }
}

// The reading, drawn the theme's way. right = right edge (Studio, Bold) or centre x (Clean)
void drawReadout(const char* s, uint16_t col, int x, int baseline, bool centred) {
  canvas.setFont(L.readout);
  if (theme == TH_STUDIO) {                                      // unlit 7-segment digits behind, like an LCD
    canvas.setTextColor(darker(darker(C_FAINT)) | 0x0120);
    textRight("888.8", x, baseline);
  }
  canvas.setTextColor(col);
  if (centred) textCentered(s, x, baseline); else textRight(s, x, baseline);
}

void drawMeter(uint32_t now, float db, float fastDb) {
  bool warm = warmup > 0, fault = micFaulty();
  char buf[24];
  bool colourBg = theme == TH_BOLD && !warm && !fault;
  uint16_t bg = colourBg ? levelColour(fastDb > OVERLOAD_DB ? 200 : db) : C_BG;
  canvas.fillScreen(bg);
  SavedInk ink; if (colourBg) ink = inkOnColour(bg);
  drawHeader(now);

  if (warm) strcpy(buf, "---"); else if (fastDb > OVERLOAD_DB) strcpy(buf, "OVER"); else snprintf(buf, sizeof(buf), "%.1f", db);
  uint16_t numCol = warm ? C_DIM : colourBg ? C_TEXT : (fastDb > OVERLOAD_DB ? C_RED : levelColour(db));
  if (fault) {
    canvas.setFont(L.big); canvas.setTextColor(C_RED); textCentered("Mic fault", 64, 54);
    canvas.setFont(L.small); canvas.setTextColor(C_DIM); textCentered("Check the mic wires", 64, 60);
  } else if (theme == TH_CLEAN) {
    drawRing(64, 64, 38, fastDb, peakDb, !warm);
    drawReadout(buf, warm ? C_DIM : C_TEXT, 64, 70, true);
    canvas.setFont(L.small); canvas.setTextColor(C_DIM); textCentered("dB", 64, 76);
  } else {
    canvas.setFont(L.small); canvas.setTextColor(C_DIM); canvas.setCursor(4, theme == TH_BOLD ? 58 : 52); canvas.print("dB");
    drawReadout(buf, numCol, 124, theme == TH_BOLD ? 70 : 64, false);
  }

  if (theme != TH_CLEAN) {
    if (theme == TH_BOLD) {                                      // white bar on a darker track
      int w = (!warm && !fault) ? (int)(constrain((fastDb - BAR_MIN_DB) / (BAR_MAX_DB - BAR_MIN_DB), 0.0f, 1.0f) * 120) : 0;
      canvas.fillRoundRect(4, 78, 120, 6, 3, C_FAINT);
      if (w > 0) canvas.fillRoundRect(4, 78, max(w, 6), 6, 3, C_TEXT);
    } else drawLadder(72, 9, fastDb, peakDb, !warm && !fault);
    canvas.setFont(L.small); canvas.setTextColor(C_DIM);
    snprintf(buf, sizeof(buf), "%d", (int)BAR_MIN_DB); canvas.setCursor(4, 87); canvas.print(buf);
    snprintf(buf, sizeof(buf), "%d", (int)BAR_MAX_DB); textRight(buf, 124, 87);
    if (amberFrom > BAR_MIN_DB + 8 && amberFrom < BAR_MAX_DB - 8) {
      int ax = 4 + (int)((amberFrom - BAR_MIN_DB) / (BAR_MAX_DB - BAR_MIN_DB) * 120);
      snprintf(buf, sizeof(buf), "%d", amberFrom); canvas.setTextColor(colourBg ? C_TEXT : C_AMBER); textCentered(buf, ax, 87);
    }
  }
  canvas.drawFastHLine(4, 103, 120, C_FAINT);

  if (rMode == R_LEQ) {
    unsigned long secs = snapLeqSecs();
    if (secs >= 3600) snprintf(buf, sizeof(buf), "%luh %02lum", secs / 3600, (secs / 60) % 60);
    else snprintf(buf, sizeof(buf), "%02lu:%02lu", secs / 60, secs % 60);
    drawFooterReading("Time", buf, C_TEXT);
  } else {
    if (warm || snap.maxF[wMode] <= 0) strcpy(buf, "--"); else snprintf(buf, sizeof(buf), "%.1f", toDb(snap.maxF[wMode]));
    drawFooterReading("Max", buf, C_TEXT);
  }
  drawBatteryCorner(now);
  if (colourBg) restoreInk(ink);
  drawToast(now);
}

// RTA screen: 31 bars (20 Hz - 20 kHz), 30-110 dB, colours follow the amber / red settings
void drawRta(uint32_t now, float fastDb) {
  const int top = 26, bottom = 84, x0 = 2;
  const float lo = 30, hi = 110;
  bool warm = warmup > 0;
  char buf[24];
  canvas.fillScreen(C_BG);
  drawHeader(now);
  for (float g = 50; g < hi; g += 20) {                         // faint guide lines at 50 / 70 / 90 dB
    int y = bottom - (int)((g - lo) / (hi - lo) * (bottom - top));
    canvas.drawFastHLine(x0, y, 124, C_FAINT);
  }
  for (int b = 0; b < RTA_BANDS; b++) {
    int x = x0 + b * 4;
    canvas.drawFastHLine(x, bottom, 3, C_FAINT);
    if (warm || !rtaReady) continue;
    float d = rtaBandDb(b);
    int h = (int)((constrain(d, lo, hi) - lo) / (hi - lo) * (bottom - top));
    if (h > 0) canvas.fillRect(x, bottom - h, 3, h, levelColour(d));
  }
  canvas.setFont(L.small); canvas.setTextSize(1); canvas.setTextColor(C_DIM);
  const int lab[] = {5, 11, 17, 23, 29};                        // 63, 250, 1k, 4k, 16k
  for (int i = 0; i < 5; i++) {
    int cx = x0 + lab[i] * 4 + 1;
    if (i == 0) { canvas.setCursor(x0, 87); canvas.print(RTA_LABEL[lab[i]]); }
    else if (i == 4) textRight(RTA_LABEL[lab[i]], 126, 87);
    else textCentered(RTA_LABEL[lab[i]], cx, 87);
  }
  canvas.drawFastHLine(4, 103, 120, C_FAINT);
  if (warm) strcpy(buf, "--"); else if (fastDb > OVERLOAD_DB) strcpy(buf, "OVER"); else snprintf(buf, sizeof(buf), "%.1f", fastDb);
  drawFooterReading("Level", buf, warm ? C_DIM : levelColour(fastDb));
  drawBatteryCorner(now);
  drawToast(now);
}

void drawCalibration(uint32_t now) {
  canvas.fillScreen(C_BG);
  canvas.setFont(L.label); canvas.setTextColor(C_TEXT);
  canvas.setCursor(4, 15); canvas.print("Calibrate");
  canvas.drawFastHLine(4, 20, 120, C_FAINT);
  canvas.setFont(L.readout); canvas.setTextColor(C_TEXT);
  textRight("94.0", 124, 66);
  canvas.setFont(L.small); canvas.setTextColor(C_DIM);
  canvas.setCursor(4, 57); canvas.print("dB");
  textCentered(calState == CAL_SETTLE ? "Fit the calibrator" : "Measuring", 64, 80);
  uint32_t el = now - calT0 + (calState == CAL_MEASURE ? 2000 : 0);
  int w = constrain((int)(el * 120 / 7000), 0, 120);
  canvas.fillRect(4, 98, 120, 4, C_FAINT);
  canvas.fillRect(4, 98, w, 4, C_TEXT);
}

// ---------------- Production check (type "test" in the Serial Monitor) ----------------
// Steps through the parts one by one, on the screen and in the Serial Monitor, then shows PASS or FAIL.
void checkScreen(const char* l1, const char* l2, uint16_t col) {
  canvas.fillScreen(C_BG);
  canvas.setFont(L.label); canvas.setTextColor(C_TEXT); canvas.setCursor(4, 15); canvas.print("Production check");
  canvas.drawFastHLine(4, 20, 120, C_FAINT);
  canvas.setFont(L.big); canvas.setTextColor(col); textCentered(l1, 64, 62);
  canvas.setFont(L.small); canvas.setTextSize(1); canvas.setTextColor(C_DIM); textCentered(l2, 64, 76);
  pushCanvas();
}
void productionCheck() {
  struct Res { const char* name; bool pass; String note; } res[8];
  int n = 0;
  Serial.println("\n=== PocketSPL production check ===");
  Serial.printf("Model %s, software %s, serial %s\n", modelName(), FW_VERSION, serialNo.c_str());
  setBrightness(15);
  // 1. Screen: look for clean red, green, blue and white with no dead lines
  const uint16_t cols[4] = {C_RED, C_GREEN, rgb(0, 0, 255), rgb(255, 255, 255)};
  Serial.println("Screen: watch for red, green, blue, white, with no lines or dead spots.");
  for (int i = 0; i < 4; i++) { canvas.fillScreen(cols[i]); pushCanvas(); delay(800); }
  res[n++] = {"Screen", true, "check by eye"};
  // 2. Button
  Serial.println("Button: press it within 10 s.");
  bool pressed = false;
  for (uint32_t t0 = millis(); millis() - t0 < 10000 && !pressed; ) {
    char b[24]; snprintf(b, sizeof(b), "within %lu s", (unsigned long)(10 - (millis() - t0) / 1000));
    checkScreen("Press button", b, C_TEXT);
    if (digitalRead(PIN_BTN) == HIGH) pressed = true;
    delay(50);
  }
  while (digitalRead(PIN_BTN) == HIGH) delay(20);
  res[n++] = {"Button", pressed, pressed ? "" : "not pressed in 10 s"};
  // 3. Mic: a clap should read 75 dB or more
  Serial.println("Mic: clap near the mic within 6 s.");
  float best = -99;
  for (uint32_t t0 = millis(); millis() - t0 < 6000 && best < 75; ) {
    takeSnapshot(); float v = toDb(snap.msqF[W_Z]); if (v > best) best = v;
    char b[24]; snprintf(b, sizeof(b), "%.1f dB", v);
    checkScreen("Clap", b, C_TEXT);
    delay(30);
  }
  bool micOk = !micFaulty() && best >= 75;
  char mb[40]; snprintf(mb, sizeof(mb), micFaulty() ? "no signal (check wires)" : "loudest %.1f dB", best);
  res[n++] = {"Mic", micOk, mb};
  // 4. Battery gauge
  readBattery();
  bool battOk = gauge != G_NONE && battV > 3.0f && battV < 4.4f;
  char bb[40]; snprintf(bb, sizeof(bb), gauge == G_NONE ? "gauge not found" : "%.2f V, %d%%", battV, battPct);
  if (wiredModel) res[n++] = {"Power", true, "USB, wired model"};
  else res[n++] = {"Battery", battOk, bb};
  // 5. WiFi
  bool wifiOk = wifiOn && (apOn || netMode == NET_STA || netMode == NET_JOINING);
  res[n++] = {"WiFi", wifiOk, wifiOk ? "" : "WiFi is off (hold the button 3 s)"};
  // 6. Storage for saved logs, 7. room for updates
  res[n++] = {"Storage", fsOk, fsOk ? "" : "check Partition Scheme"};
  res[n++] = {"Update space", otaReady(), otaReady() ? "" : "check Partition Scheme"};
  // 8. Factory calibration saved
  char cb[40]; snprintf(cb, sizeof(cb), factoryCalSet ? "%+.2f dB" : "not saved yet: cal94, then factory cal", factoryCal);
  res[n++] = {"Factory cal", factoryCalSet, cb};

  bool all = true;
  canvas.fillScreen(C_BG);
  canvas.setFont(L.small); canvas.setTextSize(1);
  for (int i = 0; i < n; i++) {
    all = all && res[i].pass;
    Serial.printf("  %-13s %s  %s\n", res[i].name, res[i].pass ? "PASS" : "FAIL", res[i].note.c_str());
    canvas.setTextColor(C_DIM); canvas.setCursor(4, 3 + i * 13); canvas.print(res[i].name);
    canvas.setTextColor(res[i].pass ? C_GREEN : C_RED); textRight(res[i].pass ? "Pass" : "Fail", 124, 3 + i * 13);
  }
  canvas.drawFastHLine(4, 107, 120, C_FAINT);
  canvas.setFont(L.label); canvas.setTextColor(all ? C_GREEN : C_RED);
  textCentered(all ? "All pass" : "Check the fails", 64, 124);
  canvas.setFont(L.small);
  pushCanvas();
  Serial.println(all ? "=== ALL PASS ===" : "=== SOMETHING FAILED (see above) ===");
  delay(8000);
  setBrightness(brightness);
  lastActivity = millis();
}

void handleSerial() {
  if (!Serial.available()) return;
  String s = Serial.readStringUntil('\n'); s.trim();
  if (s == "help") {
    Serial.printf("PocketSPL  model %s  software v%s  serial %s\n", modelName(), FW_VERSION, serialNo.c_str());
    Serial.println("test | cal94 | cal 2.5 | cal? | factory cal | sn 0007 | sn auto | model wired | model battery | info");
    return;
  }
  if (s == "test") { productionCheck(); return; }
  if (s == "info") { Serial.printf("Model %s\nSoftware v%s\nSerial %s\nFactory cal %s\n", modelName(), FW_VERSION, serialNo.c_str(), factoryCalSet ? String(factoryCal, 2).c_str() : "not saved"); return; }
  if (s.startsWith("cal?")) { Serial.printf("Offset: %+.2f dB  (factory: %s)\n", calOffset, factoryCalSet ? String(factoryCal, 2).c_str() : "not saved"); return; }
  if (s.startsWith("cal94")) { startCal94(); return; }
  if (s == "factory cal") {
    factoryCal = calOffset; factoryCalSet = true;
    prefs.putFloat("fcal", factoryCal); prefs.putBool("fcalok", true);
    Serial.printf("Factory calibration saved: %+.2f dB\n", factoryCal);
    toast("Factory cal", "Saved", 1500);
    return;
  }
  if (s == "model wired" || s == "model battery") {
    wiredModel = s == "model wired"; prefs.putBool("wired", wiredModel);
    if (!wiredModel && screenOff) { screenOff = false; oled.sendCommand(0xAF); }
    Serial.printf("Model set to %s (%s)\n", modelName(), wiredModel ? "wired, no battery" : "battery");
    toast("Model", modelName(), 1500);
    return;
  }
  if (s.startsWith("sn ")) {
    String v = s.substring(3); v.trim();
    if (v == "auto") prefs.putString("sn", "");
    else if (v.length() >= 1 && v.length() <= 8) prefs.putString("sn", "PSPL-" + v);
    else { Serial.println("Use 1 to 8 characters, e.g. sn 0007"); return; }
    serialNo = makeSerial();
    Serial.println("Serial number: " + serialNo);
    return;
  }
  if (s.startsWith("cal ")) {
    calOffset = s.substring(4).toFloat(); savePrefs();
    Serial.printf("Offset set to %+.2f dB\n", calOffset);
    return;
  }
  if (s.length()) Serial.println("Unknown command. Type help");
}

void drawGig(uint32_t now);
void drawStandby(uint32_t now) {           // wired model, outside the schedule (shown for a few seconds after a press)
  char b[32];
  canvas.fillScreen(C_BG);
  canvas.setFont(L.big); canvas.setTextColor(C_TEXT);
  textCentered("Standby", 64, 54);
  canvas.setFont(L.small);
  int wm = weekMinuteNow(), nx = wm >= 0 ? nextChange(wm) : -1;
  canvas.setTextColor(C_DIM); textCentered("Comes on", 64, 78);
  canvas.setTextColor(C_ACCENT);
  if (nx >= 0) { fmtWeekMinute(nx, b, sizeof(b)); textCentered(b, 64, 94); }
  if (wm >= 0) { char nb[40]; fmtWeekMinute(wm, b, sizeof(b)); snprintf(nb, sizeof(nb), "Now %s", b); canvas.setTextColor(C_DIM); textCentered(nb, 64, 118); }
}
// Sends only the parts of the screen that changed (in 8-row bands) and answers the phone
// between them, so the screen never keeps the phone page waiting. Main loop frame only.
bool pushServe = false, inServer = false;
static uint16_t lastFrame[128 * 128];
static bool lastFrameOk = false;
void pushCanvasServed() {
  uint16_t* b = canvas.getBuffer();
  int sent = 0;
  for (int y = 0; y < 128; y += 8) {
    uint16_t* row = b + y * 128;
    if (lastFrameOk && memcmp(row, lastFrame + y * 128, 128 * 8 * 2) == 0) continue;   // band unchanged
    oled.drawRGBBitmap(0, y, row, 128, 8);
    memcpy(lastFrame + y * 128, row, 128 * 8 * 2);
    if (++sent % 4 == 0 && wifiOn && !inServer) { inServer = true; server.handleClient(); inServer = false; }
  }
  lastFrameOk = true;
}
void pushCanvas() { oled.drawRGBBitmap(0, 0, canvas.getBuffer(), 128, 128); lastFrameOk = false; }   // full frame (other screens)

void drawFrame(uint32_t now) {             // draw whichever screen is showing and send it to the OLED
  takeSnapshot();
  evalLimit(now);
  bool clean = theme == TH_CLEAN;
  if (standby) { if (clean) clStandby(now); else drawStandby(now); }
  else if (calState != CAL_IDLE) { if (clean) clCalibration(now); else drawCalibration(now); }
  else if (rMode == R_RTA) { if (clean) clRta(now, snapFastDb()); else drawRta(now, snapFastDb()); }
  else if (rMode == R_GIG) { if (clean) clGig(now); else drawGig(now); }
  else { if (clean) clMeter(now, snapDb(), snapFastDb()); else drawMeter(now, snapDb(), snapFastDb()); }
  if (pushServe) pushCanvasServed(); else pushCanvas();
}

// GIG screen: the limit (or 15-min) Leq big, the 1 and 5 minute values, a bar against the limit, and the dose
void drawGig(uint32_t now) {
  char buf[32];
  bool warm = warmup > 0;
  int k = limOn ? limWin : 2, w = limOn ? limW : wMode;
  float v = 0; bool have = !warm && rollDb(k, w, v);
  uint16_t col = !have ? C_DIM : limOn ? (limState == 2 ? C_RED : limState == 1 ? C_AMBER : C_GREEN) : levelColour(v);
  bool colourBg = theme == TH_BOLD && have;
  canvas.fillScreen(colourBg ? col : C_BG);
  SavedInk ink; if (colourBg) ink = inkOnColour(col);
  drawHeader(now);
  canvas.setFont(L.small); canvas.setTextSize(1); canvas.setTextColor(C_DIM);
  snprintf(buf, sizeof(buf), "L%ceq %s", w == W_A ? 'A' : w == W_C ? 'C' : 'Z', LIM_WIN_NAME[k]);
  canvas.setCursor(4, 25); canvas.print(buf);
  if (have && !rollFull(k)) textRight("so far", 124, 25);
  if (have) snprintf(buf, sizeof(buf), "%.1f", v); else strcpy(buf, "---");
  if (theme == TH_CLEAN) drawReadout(buf, have ? col : C_DIM, 64, 68, true);
  else drawReadout(buf, colourBg ? C_TEXT : col, 124, theme == TH_BOLD ? 74 : 70, false);
  if (colourBg) col = C_TEXT;                    // the limit bar draws white on the colour
  canvas.setFont(L.small); canvas.setTextColor(C_DIM);
  if (limOn) {                                   // bar from limit-20 to limit+5, tick at the limit
    const int bx = 4, by = 80, bw = 120, bh = 5;
    float lo = limDb - 20, hi = limDb + 5;
    canvas.fillRect(bx, by, bw, bh, C_FAINT);
    if (have) { int fw = (int)((constrain(v, lo, hi) - lo) / (hi - lo) * bw); canvas.fillRect(bx, by, fw, bh, col); }
    int lx = bx + (int)((limDb - lo) / (hi - lo) * bw);
    canvas.fillRect(lx - 1, by - 3, 2, bh + 6, C_TEXT);
    snprintf(buf, sizeof(buf), "Limit %d", limDb);
    textRight(buf, 124, 88);
  } else {
    textRight("No limit", 124, 88);
  }
  float v1;
  char a1[16] = "1 min --";
  if (!warm && rollDb(0, w, v1)) snprintf(a1, sizeof(a1), "1 min %.1f", v1);
  canvas.setCursor(4, 88); canvas.print(a1);
  canvas.drawFastHLine(4, 103, 120, C_FAINT);
  float d = warm ? 0 : dosePct();
  snprintf(buf, sizeof(buf), "%.0f%%", d);
  drawFooterReading("Dose", buf, colourBg ? C_TEXT : d >= 100 ? C_RED : d >= 50 ? C_AMBER : C_TEXT);
  drawBatteryCorner(now);
  if (colourBg) restoreInk(ink);
  drawToast(now);
}

// ---------------- Start-up logo ----------------
// About 3 seconds: the 7 meter bars rise one by one, bounce like a meter on a live mix with a
// peak-hold cap, the name wipes in behind a blue edge, a glint runs along the floor and a tagline fades in.
// Bars green -> amber -> red, "Pocket" in white, "SPL" in blue.
const int LOGO_FRAMES = 96;                // one frame every 30 ms
const int LOGO_FRAME_MS = 30;

static uint16_t mix565(uint16_t a, uint16_t b, float t) {   // blend two colours, t = 0 (a) .. 1 (b)
  t = constrain(t, 0.0f, 1.0f);
  int ar = a >> 11, ag = (a >> 5) & 63, ab = a & 31, br = b >> 11, bg = (b >> 5) & 63, bb = b & 31;
  return ((int)(ar + (br - ar) * t + 0.5f) << 11) | ((int)(ag + (bg - ag) * t + 0.5f) << 5) | (int)(ab + (bb - ab) * t + 0.5f);
}

void drawLogo(int frame) {                 // frame 0..LOGO_FRAMES
  const int n = 7, bw = 9, gap = 3, base = 46;
  const int h[n] = {7, 11, 15, 20, 25, 31, 37};
  const uint16_t col[n] = {C_GREEN, C_GREEN, C_GREEN, C_GREEN, C_AMBER, C_AMBER, C_RED};
  // How many bars are lit while the meter "dances" (frames 24..57); it finishes on all 7.
  static const uint8_t dance[34] = {7,6,5,4,3,4,5,6,5,3,2,3,5,6,7,7,6,4,3,2,2,3,4,6,7,6,5,4,4,5,6,7,7,7};
  static int peakBar = -1, peakHold = 0;
  if (frame == 0) { peakBar = -1; peakHold = 0; }
  int x0 = (128 - (n * bw + (n - 1) * gap)) / 2;

  int lit = n;                                                  // bars fully lit (dance phase)
  if (frame >= 24 && frame < 58) lit = dance[frame - 24];
  if (frame >= 24 && frame < 58) {                              // peak-hold cap: jumps up, holds, then falls
    if (lit - 1 >= peakBar) { peakBar = lit - 1; peakHold = 8; }
    else if (peakHold > 0) peakHold--;
    else if (frame % 2 == 0) peakBar--;
  }

  for (int i = 0; i < n; i++) {
    int x = x0 + i * (bw + gap);
    canvas.fillRect(x, base - h[i], bw, h[i], C_FAINT);         // empty slot
    int hh;
    if (frame < 24) {                                           // rise: each bar starts 2 frames after the last
      float t = constrain((frame - i * 2) / 10.0f, 0.0f, 1.0f);
      hh = (int)(h[i] * (1 - (1 - t) * (1 - t) * (1 - t)) + 0.5f);
    } else hh = i < lit ? h[i] : 0;
    if (hh > 0) canvas.fillRoundRect(x, base - hh, bw, hh, 2, col[i]);
    if (frame >= 24 && frame < 58 && i == peakBar && i >= lit)  // cap sits on top of an unlit slot
      canvas.fillRect(x, base - h[i], bw, 2, C_TEXT);
  }

  // Floor line, with a white glint running along it (frames 58..74)
  int fx = x0 - 4, fw = n * bw + (n - 1) * gap + 8;
  canvas.drawFastHLine(fx, base + 3, fw, C_DIM);
  if (frame >= 58 && frame <= 74) {
    int gx = fx + (frame - 58) * fw / 16;
    for (int k = -6; k <= 6; k++) {
      int px = gx + k;
      if (px >= fx && px < fx + fw) canvas.drawPixel(px, base + 3, mix565(C_DIM, C_TEXT, 1.0f - abs(k) / 7.0f));
    }
  }

  // Name: wipes in left to right behind a blue edge (frames 14..38)
  if (frame >= 14) {
    canvas.setFont(&BarlowLogo);
    int16_t x1, y1; uint16_t wP, wS, hh2;
    canvas.getTextBounds("Pocket", 0, 0, &x1, &y1, &wP, &hh2);
    canvas.getTextBounds("SPL", 0, 0, &x1, &y1, &wS, &hh2);
    int tx = (128 - (int)(wP + wS + 1)) / 2, tw = wP + wS + 1;
    canvas.setTextColor(C_TEXT);   canvas.setCursor(tx, 70);          canvas.print("Pocket");
    canvas.setTextColor(C_ACCENT); canvas.setCursor(tx + wP + 1, 70); canvas.print("SPL");
    canvas.setFont(L.small);
    if (frame < 38) {
      float t = (frame - 14) / 24.0f;
      int edge = tx - 2 + (int)((tw + 4) * (1 - (1 - t) * (1 - t)));   // eases to a stop
      canvas.fillRect(edge, 51, 128 - edge, 24, C_BG);                 // hide what's not revealed yet
      canvas.fillRect(edge, 52, 2, 22, C_ACCENT);                      // the moving blue edge
    }
  }

  // Tagline fades in (frames 62..80)
  if (frame >= 62) {
    canvas.setFont(L.small); canvas.setTextSize(1);
    canvas.setTextColor(mix565(C_BG, C_DIM, (frame - 62) / 18.0f));
    textCentered("Sound level meter", 64, 80);
  }
}

// ---------------- Serial number ----------------
String makeSerial() {                       // set with "sn 0007" in the Serial Monitor, otherwise from the chip's own ID
  String s = prefs.getString("sn", "");
  if (s.length()) return s;
  uint64_t mac = ESP.getEfuseMac();
  char b[16]; snprintf(b, sizeof(b), "PSPL-%06lX", (unsigned long)((mac >> 24) & 0xFFFFFF));
  return String(b);
}

// ---------------- WiFi reset at switch-on ----------------
// Button held while switching on: keep holding 5 s to put the meter's own WiFi back to PocketSPL / pocketspl
// and remove the settings lock.
void bootHoldWifiReset() {
  const uint32_t HOLD_MS = 5000;
  uint32_t t0 = millis();
  bool done = false;
  while (digitalRead(PIN_BTN) == HIGH) {
    uint32_t held = millis() - t0;
    canvas.fillScreen(C_BG);
    canvas.setFont(L.small); canvas.setTextSize(1);
    if (!done && held >= HOLD_MS) {
      apName = DEFAULT_WIFI_NAME; apPass = DEFAULT_WIFI_PASS; saveApWifi();
      wifiOn = true; prefs.putBool("wifi", true);
      lockPin = ""; prefs.putString("pin", "");      // the settings lock comes off too
      done = true;
    }
    canvas.setFont(L.label); canvas.setTextColor(C_TEXT);
    canvas.setCursor(4, 15); canvas.print("WiFi reset");
    canvas.drawFastHLine(4, 20, 120, C_FAINT);
    canvas.setFont(L.small);
    if (done) {
      canvas.setFont(L.big); canvas.setTextColor(C_GREEN); textCentered("Done", 64, 52);
      canvas.setFont(L.small);
      canvas.setTextColor(C_DIM);  canvas.setCursor(4, 64); canvas.print("Name");     canvas.setTextColor(C_TEXT); textRight("PocketSPL", 124, 64);
      canvas.setTextColor(C_DIM);  canvas.setCursor(4, 78); canvas.print("Password"); canvas.setTextColor(C_TEXT); textRight("pocketspl", 124, 78);
      canvas.setTextColor(C_DIM);  textCentered("Lock removed. Let go.", 64, 106);
    } else {
      canvas.setTextColor(C_TEXT); textCentered("Keep holding to put the", 64, 30); textCentered("WiFi back to PocketSPL", 64, 44);
      canvas.fillRect(4, 66, 120, 4, C_FAINT); canvas.fillRect(4, 66, (int)(120L * held / HOLD_MS), 4, C_TEXT);
      char b[8]; snprintf(b, sizeof(b), "%lu", (unsigned long)((HOLD_MS - held) / 1000 + 1));
      canvas.setFont(L.label); textCentered(b, 64, 94);
      canvas.setFont(L.small); canvas.setTextColor(C_DIM); textCentered("Let go to cancel", 64, 106);
    }
    pushCanvas();
    delay(50);
  }
  delay(100);                                // ignore contact bounce on release
}

// ---------------- Software update (from the phone) ----------------
// The phone sends a .bin file (exported from Arduino). It's written to the spare half of the flash,
// checked, and only then made the one to start from, so a failed or wrong file changes nothing.
bool updBusy = false, updOk = false;
size_t updDone = 0, updTotal = 0, updDrawnAt = 0;
String updErr;
// An update file must contain "POCKETSPL-FW PS-1 " (PocketSPL software for this model).
// "POCKETSPL-FW" on its own means PocketSPL software for a different model.
struct TagScan { const char* tag; size_t len; uint8_t tail[32]; size_t tailLen; bool seen; };
TagScan tagModel = {"POCKETSPL-FW " MODEL_NO " ", sizeof("POCKETSPL-FW " MODEL_NO " ") - 1, {0}, 0, false};
TagScan tagAny   = {"POCKETSPL-FW", sizeof("POCKETSPL-FW") - 1, {0}, 0, false};
TagScan tagFlash = {"POCKETSPL-FLASH " BOARD_FLASH ";", sizeof("POCKETSPL-FLASH " BOARD_FLASH ";") - 1, {0}, 0, false};
void scanTag(TagScan& s, const uint8_t* d, size_t n);   // (declared here so Arduino keeps it after the type)

bool otaReady() { return esp_ota_get_next_update_partition(NULL) != NULL; }

void scanTag(TagScan& s, const uint8_t* d, size_t n) {   // finds the tag, also across chunk edges
  if (s.seen || n == 0) return;
  const size_t L = s.len;
  uint8_t joint[64];
  size_t k = min(n, L - 1);
  memcpy(joint, s.tail, s.tailLen); memcpy(joint + s.tailLen, d, k);
  if (memmem(joint, s.tailLen + k, s.tag, L) || memmem(d, n, s.tag, L)) { s.seen = true; return; }
  if (n >= L - 1) { memcpy(s.tail, d + n - (L - 1), L - 1); s.tailLen = L - 1; }
  else {
    size_t keep = min(s.tailLen + n, L - 1);
    uint8_t tmp[64]; memcpy(tmp, s.tail, s.tailLen); memcpy(tmp + s.tailLen, d, n);
    memcpy(s.tail, tmp + (s.tailLen + n - keep), keep); s.tailLen = keep;
  }
}
void scanUpdTag(const uint8_t* d, size_t n) { scanTag(tagModel, d, n); scanTag(tagAny, d, n); scanTag(tagFlash, d, n); }

String updToVer;                            // version being installed (sent by the phone), for the screen
void drawUpdateScreen(const char* big, const char* small, int pct, uint16_t colour) {
  if (theme == TH_CLEAN) {
    clUpdateScreen(big, small, pct, colour, updToVer.c_str());
    pushCanvas();
    setBrightness(brightness);
    return;
  }
  canvas.fillScreen(C_BG);
  canvas.setFont(L.label); canvas.setTextColor(C_TEXT);
  canvas.setCursor(4, 15); canvas.print("Update");
  canvas.drawFastHLine(4, 20, 120, C_FAINT);
  canvas.setFont(L.small); canvas.setTextColor(C_DIM);
  String v = "v" FW_VERSION;
  if (updToVer.length()) v += "  to  v" + updToVer;
  textCentered(v.c_str(), 64, 25);
  canvas.setFont(L.big); canvas.setTextColor(colour);
  textCentered(big, 64, 58);
  if (pct >= 0) {
    canvas.fillRect(4, 68, 120, 4, C_FAINT);
    canvas.fillRect(4, 68, 120 * constrain(pct, 0, 100) / 100, 4, colour);
    char b[8]; snprintf(b, sizeof(b), "%d%%", constrain(pct, 0, 100));
    canvas.setFont(L.label); canvas.setTextColor(C_TEXT); textCentered(b, 64, 92);
  }
  canvas.setFont(L.small); canvas.setTextColor(C_DIM); textCentered(small, 64, 106);
  pushCanvas();
  setBrightness(brightness);
}

void handleUpdateUpload() {
  HTTPUpload& up = server.upload();
  if (up.status == UPLOAD_FILE_START) {
    updErr = ""; updOk = false; updDone = 0; updDrawnAt = 0;
    tagModel.seen = tagAny.seen = tagFlash.seen = false; tagModel.tailLen = tagAny.tailLen = tagFlash.tailLen = 0;
    updTotal = server.hasArg("size") ? (size_t)server.arg("size").toInt() : 0;
    updToVer = validVersion(server.arg("ver")) ? server.arg("ver") : String("");
    if (!pinOk()) updErr = "Settings are locked. Unlock them in Settings first.";
    else if (!otaReady()) updErr = "This meter has no room for updates. Load it once by USB with the Partition Scheme from the build guide.";
    else if (!Update.begin(updTotal > 0 ? updTotal : UPDATE_SIZE_UNKNOWN)) updErr = String("Couldn't start: ") + Update.errorString();
    else { updBusy = true; drawUpdateScreen("Updating", "Don't switch off", 0, C_ACCENT); }
  } else if (up.status == UPLOAD_FILE_WRITE) {
    if (!updBusy) return;
    scanUpdTag(up.buf, up.currentSize);
    if (Update.write(up.buf, up.currentSize) != up.currentSize) {
      updErr = String("Writing failed: ") + Update.errorString(); Update.abort(); updBusy = false;
      drawUpdateScreen("Failed", "Nothing was changed", -1, C_RED);
      return;
    }
    updDone += up.currentSize;
    if (updTotal && updDone - updDrawnAt >= 16384) {
      updDrawnAt = updDone;
      drawUpdateScreen("Updating", "Don't switch off", (int)min((size_t)99, updDone * 100 / updTotal), C_ACCENT);
    }
  } else if (up.status == UPLOAD_FILE_END) {
    if (!updBusy) return;
    updBusy = false;
    if (tagModel.seen && tagFlash.seen) drawUpdateScreen("Checking", "Don't switch off", 100, C_ACCENT);
    if (tagModel.seen && !tagFlash.seen) {
      Update.abort();
      updErr = "That software is for the other board size (this meter is the " BOARD_FLASH " board). Nothing was changed.";
    }
    else if (!tagModel.seen) {
      Update.abort();
      updErr = tagAny.seen ? "That software is for a different PocketSPL model (this one is " MODEL_NO "). Nothing was changed."
                           : "That file isn't PocketSPL software. Nothing was changed.";
    }
    else if (!Update.end(true)) updErr = String("The file didn't check out: ") + Update.errorString() + ". Nothing was changed.";
    else updOk = true;
    if (!updOk) drawUpdateScreen("Failed", "Nothing was changed", -1, C_RED);
  } else if (up.status == UPLOAD_FILE_ABORTED) {
    if (updBusy) Update.abort();
    updBusy = false; updErr = "The upload stopped part way. Nothing was changed.";
    drawUpdateScreen("Failed", "Nothing was changed", -1, C_RED);
  }
}

void handleUpdateDone() {
  String s = updOk ? String("{\"ok\":true}")
                   : "{\"ok\":false,\"err\":\"" + jsonEsc(updErr.length() ? updErr : String("No file arrived.")) + "\"}";
  server.sendHeader("Connection", "close");
  server.send(200, "application/json", s);
  if (updOk) {
    drawUpdateScreen("Updated", "Restarting", 100, C_GREEN);
    restartAt = millis() + 1500;
  } else toast("Not updated", "Nothing changed", 3000);
}

// ---------------- Setup / loop ----------------
void setup() {
  Serial.begin(115200);
  pinMode(PIN_NEO_POWER, OUTPUT); digitalWrite(PIN_NEO_POWER, LOW);   // on-board NeoPixel off
  pinMode(PIN_MIC_GND, OUTPUT);   digitalWrite(PIN_MIC_GND, LOW);     // mic ground
  pinMode(PIN_BTN_HIGH, OUTPUT);  digitalWrite(PIN_BTN_HIGH, HIGH);  // button supply
  pinMode(PIN_BTN, INPUT_PULLDOWN);

  prefs.begin("pocketspl", false);
  wMode = constrain(prefs.getUChar("w", W_A), 0, 2);
  rMode = constrain(prefs.getUChar("r", R_FAST), 0, NUM_R - 1);
  calOffset = prefs.getFloat("cal", DEFAULT_CAL_OFFSET);
  amberFrom = prefs.getUChar("amb", DEFAULT_AMBER);
  redFrom = prefs.getUChar("red", DEFAULT_RED);
  if (redFrom <= amberFrom) { amberFrom = DEFAULT_AMBER; redFrom = DEFAULT_RED; }
  brightness = constrain(prefs.getUChar("bri", DEFAULT_BRIGHTNESS), 1, 15);
  wifiOn = prefs.getBool("wifi", true);
  limOn = prefs.getBool("limOn", false);
  limDb = constrain(prefs.getUChar("limDb", 100), 60, 125);
  limWin = constrain(prefs.getUChar("limWin", 2), 0, 2);
  limW = prefs.getUChar("limW", W_A) == W_C ? W_C : W_A;
  dimSecs = prefs.getUShort("dim", 60);
  theme = prefs.getUChar("theme", DEFAULT_THEME); applyTheme();
  if (!prefs.getBool("ui2", false)) {      // first start on this version: switch to the new look once
    theme = TH_CLEAN; prefs.putUChar("theme", theme); prefs.putBool("ui2", true); applyTheme();
  }
  loadNets();
  loadApWifi();
  latestFw = prefs.getString("latest", "");
  if (!validVersion(latestFw)) latestFw = "";
  lockPin = prefs.getString("pin", "");
  if (lockPin.length() && !validPin(lockPin)) lockPin = "";
  factoryCalSet = prefs.getBool("fcalok", false);
  factoryCal = prefs.getFloat("fcal", 0.0f);
  serialNo = makeSerial();
  wiredModel = prefs.getBool("wired", false);
  tzPosix = prefs.getString("tzp", "AEST-10"); if (!validTz(tzPosix)) tzPosix = "AEST-10";
  applyTz();
  if (!parseSchedule(prefs.getString("sch", ""))) schN = 0;
  schEnabled = prefs.getBool("schon", false);
  loadAlerts();
  Serial.printf("%s %s  serial %s\n", FW_TAG, FLASH_TAG, serialNo.c_str());   // e.g. POCKETSPL-FW PS-1 1.0.0  serial PSPL-0007
  fsOk = LittleFS.begin(true);             // flash storage for saved gig logs (formats itself the first time)
  if (!fsOk) Serial.println("Storage: not available (check Tools > Partition Scheme)");
  archiveCurrentLog();                     // the log from before switch-off becomes a saved log

  SPI.begin(PIN_OLED_SCK, -1, PIN_OLED_MOSI, PIN_OLED_CS);
  oled.begin(OLED_SPI_HZ);
  pinMode(PIN_BTN_HIGH, OUTPUT); digitalWrite(PIN_BTN_HIGH, HIGH);   // button pins again, after the screen starts
  pinMode(PIN_BTN, INPUT_PULLDOWN);
  setBrightness(brightness);
  canvas.setTextWrap(false);
  canvas.setFont(L.small);

  gaugeInit();
  readBattery();
  if (!wiredModel && gauge != G_NONE && battPct < BATT_CRIT_PCT && battV < 3.5f && !charging) powerOff("Battery", "flat");
  if (digitalRead(PIN_BTN) == HIGH) bootHoldWifiReset();   // button held at switch-on

  // Splash: the logo, a bar that follows the real start-up, then calibration, battery and WiFi details
  char buf[32];
  if (theme == TH_CLEAN) {
    bootRows = 0; bootProg = 0;
    bootAnim(0.0f, 350, 0.0f, 1.0f);                 // fade up from black
    bootAnim(0.45f, 1300, 1.0f, 1.0f);               // bar fills
    snprintf(buf, sizeof(buf), "%s  v%s", modelName(), FW_VERSION);
    bootRow(0, "Model", buf, CL_TEXT);
    bootRow(1, "Serial", serialNo.c_str(), CL_TEXT);
    snprintf(buf, sizeof(buf), "%+.1f dB", calOffset);
    bootRow(2, "Calibration", buf, CL_TEXT);
    bootAnim(0.6f, 1400, 1.0f, 1.0f);
    bootRows = 0;
    if (wiredModel) bootRow(0, "Power", "USB", CL_TEXT);
    else if (gauge != G_NONE) {
      snprintf(buf, sizeof(buf), "%d%%%s", battPct, charging ? " charging" : "");
      bootRow(0, "Battery", buf, charging ? CL_SAFE : battPct < BATT_LOW_PCT ? CL_OVER : CL_TEXT);
    } else bootRow(0, "Battery", "--", CL_TEXT3);
    bootRow(1, "WiFi", wifiOn ? "starting" : "off", CL_TEXT);
    if (!wifiOn) bootRow(2, "To turn on", "hold 3 s", CL_TEXT3);
    bootAnim(0.7f, 300, 1.0f, 1.0f);
    if (wifiOn) {
      wifiStart();                                   // may take ~3 s to look for saved networks
      if (netMode == NET_JOINING) {
        bootRow(1, "Joining", joinSsid.substring(0, 16).c_str(), CL_TEXT);
        bootRow(2, "Address", "shows soon", CL_TEXT3);
      } else {
        bootRow(1, "WiFi", apName.substring(0, 16).c_str(), CL_TEXT);
        bootRow(2, "Address", "192.168.4.1", CL_TEXT);
      }
    }
    bootAnim(1.0f, 1500, 1.0f, 1.0f);
    delay(250);
    bootAnim(1.0f, 250, 1.0f, 0.0f);                 // fade out into the meter
  } else {
    for (int f = 0; f <= LOGO_FRAMES; f++) {           // ~3 s animation
      uint32_t t0 = millis();
      canvas.fillScreen(C_BG);
      drawLogo(f);
      pushCanvas();
      uint32_t used = millis() - t0;
      if (used < LOGO_FRAME_MS) delay(LOGO_FRAME_MS - used);
    }
    delay(400);
    // Start-up details, two short pages under the logo: this unit, then battery and WiFi
    auto detailRow = [&](int y, const char* label, const char* value, uint16_t col) {
      canvas.setTextColor(C_DIM); canvas.setCursor(8, y); canvas.print(label);
      canvas.setTextColor(col); textRight(value, 120, y);
    };
    canvas.fillRect(0, 76, 128, 52, C_BG);             // tagline makes way for the start-up details
    canvas.setFont(L.small); canvas.setTextSize(1);
    snprintf(buf, sizeof(buf), "%s  v%s", modelName(), FW_VERSION);
    detailRow(80, "Model", buf, C_TEXT);
    detailRow(94, "Serial", serialNo.c_str(), C_TEXT);
    snprintf(buf, sizeof(buf), "%+.1f dB", calOffset);
    detailRow(108, "Calibration", buf, C_TEXT);
    pushCanvas();
    delay(1400);

    canvas.fillRect(0, 76, 128, 52, C_BG);
    if (wiredModel) {
      detailRow(80, "Power", "USB", C_TEXT);
    } else if (gauge != G_NONE) {
      snprintf(buf, sizeof(buf), "%d%%%s", battPct, charging ? " charging" : "");
      detailRow(80, "Battery", buf, (charging || battPct < BATT_LOW_PCT) ? batteryColour() : C_TEXT);
    }
    detailRow(94, "WiFi", wifiOn ? "starting" : "off", C_TEXT);
    pushCanvas();
    if (wifiOn) {
      wifiStart();                                     // may take ~3 s to look for saved networks
      canvas.fillRect(0, 92, 128, 36, C_BG);
      if (netMode == NET_JOINING) {
        detailRow(94, "Joining", joinSsid.substring(0, 16).c_str(), C_TEXT);
        detailRow(108, "Address", "shows soon", C_DIM);
      } else {
        detailRow(94, "WiFi", apName.substring(0, 16).c_str(), C_TEXT);
        detailRow(108, "Address", "192.168.4.1", C_TEXT);
      }
      pushCanvas();
    } else { detailRow(108, "To turn on", "hold 3 s", C_DIM); pushCanvas(); }
    delay(1800);
  }

  designFilters();
  aFast = 1.0f - expf(-1.0f / (TAU_FAST * FS));
  aSlow = 1.0f - expf(-1.0f / (TAU_SLOW * FS));
  i2sInit();
  if (rtaInit()) xTaskCreatePinnedToCore(rtaTask, "rta", 4096, NULL, 2, NULL, 0);
  else Serial.println("RTA: not enough memory");
  xTaskCreatePinnedToCore(audioTask, "audio", 6144, NULL, 5, NULL, 1);
  lastActivity = millis();
  String prevFw = prefs.getString("fwv", "");
  if (prevFw != FW_VERSION) { prefs.putString("fwv", FW_VERSION); if (prevFw.length()) toast("Updated", "Now v" FW_VERSION, 4000); }
  else if (updateWaiting()) toast("Update ready", ("v" + latestFw + ": see the phone").c_str(), 4000);
}

void loop() {
  uint32_t now = millis();
  handleSerial();
  if (wifiOn) { inServer = true; server.handleClient(); inServer = false; netService(now); }
  if (wifiOffAt && (int32_t)(now - wifiOffAt) >= 0) { wifiOffAt = 0; wifiStop(); savePrefs(); toast("WiFi off", "Hold 3 s to turn on", 1500); }
  if (apRestartAt && (int32_t)(now - apRestartAt) >= 0) {           // new name / password for the meter's own WiFi
    apRestartAt = 0;
    if (wifiOn && apOn) startAP(netMode == NET_STA || netMode == NET_JOINING);
    if (wifiOn && netMode == NET_STA) startMdns();
  }
  if (restartAt && (int32_t)(now - restartAt) >= 0) { server.stop(); delay(100); ESP.restart(); }
  if (now - lastLogSave >= (wiredModel ? WIRED_LOG_SAVE_MS : 5000UL)) { lastLogSave = now; saveCurrentLog(false); }   // writes only when something changed
  if (wiredModel && !scheduleActive()) checkNightOver();   // with a schedule, each session is its own log
  scheduleTick(now);
  alertTick(now);

  // ---- Button ----
  int m = pollButton(btn);
  if (standby) { if (btn.down) wake(now); m = 0; }   // standby: a press just shows when it comes on next
  if (m == 1) {
    if (editUntil && (int32_t)(now - editUntil) < 0) {             // unlocked: tap changes weighting
      setWeighting((wMode + 1) % 3);
      editUntil = now + EDIT_TIMEOUT_MS;
      toast(W_NAME[wMode], "Tap to change", EDIT_TIMEOUT_MS);
    } else if (tapPending && now - lastTapAt <= DOUBLE_CLICK_MS) {  // double-click
      tapPending = false;
      editUntil = now + EDIT_TIMEOUT_MS;
      toast(W_NAME[wMode], "Tap to change", EDIT_TIMEOUT_MS);
    } else {
      tapPending = true; lastTapAt = now;
    }
  } else if (m == 2) {                                             // held 1-3 s and let go
    tapPending = false; editUntil = 0;
    setResponse((rMode + 1) % NUM_R);
    toast(R_NAME[rMode], rMode == R_LEQ ? "Leq and Max reset" : "", 900);
  } else if (m == 3) {                                             // held 3 s
    tapPending = false; editUntil = 0;
    if (wifiOn) { wifiStop(); toast("WiFi off", "Hold 3 s to turn on", 1500); }
    else {
      toast("WiFi on", "Searching...", 4000); drawFrame(now);
      wifiStart();
      if (netMode == NET_AP) toast("WiFi on", apName.substring(0, 21).c_str(), 3000);
      else toast("WiFi on", ("Joining " + joinSsid).substring(0, 23).c_str(), 4000);
    }
    savePrefs();
  }
  if (btn.down && !btn.longDone) {                                 // show what letting go will do
    uint32_t held = now - btn.downAt;
    if (held >= HOLD_RESPONSE_MS) toast(R_NAME[(rMode + 1) % NUM_R], wifiOn ? "Hold 3 s: WiFi off" : "Hold 3 s: WiFi on", 150);
  }
  if (tapPending && now - lastTapAt > DOUBLE_CLICK_MS) {
    tapPending = false;
    toast(W_NAME[wMode], "Double-click first", 1200);
  }
  if (editUntil && (int32_t)(now - editUntil) >= 0) { editUntil = 0; toastUntil = 0; }

  // ---- Calibration: 2 s settle, then 5 s measure at 94 dB (flat, before weighting) ----
  if (calState == CAL_SETTLE && now - calT0 > 2000) {
    portENTER_CRITICAL(&audioLock); calSum = 0; calN = 0; calState = CAL_MEASURE; portEXIT_CRITICAL(&audioLock);
    calT0 = now;
  } else if (calState == CAL_MEASURE && now - calT0 > 5000) {
    portENTER_CRITICAL(&audioLock); double sum = calSum; uint64_t cnt = calN; calState = CAL_IDLE; portEXIT_CRITICAL(&audioLock);
    float measured = 10.0f * log10f((float)(sum / (double)(cnt ? cnt : 1))) + MIC_REF_DB;
    float off = 94.0f - measured;
    char small[24];
    if (fabsf(off) <= 20.0f) {
      calOffset = off; savePrefs();
      snprintf(small, sizeof(small), "Offset %+.1f dB", off);
      toast("Done", small, 2500);
    } else {
      toast("Error", "No 94 dB tone found", 2500);
    }
    Serial.printf("Calibration offset: %+.2f dB\n", off);
  }

  // ---- Screen dimming ----
  if (dimSecs > 0 && !dimmed && calState == CAL_IDLE && now - lastActivity > (uint32_t)dimSecs * 1000UL) {
    dimmed = true; setBrightness(1);
  }
  {                                        // wired model: screen off after 10 min idle; on by itself while over the limit
    bool alarm = limOn && limState == 2;
    bool wantOff = wiredModel && dimSecs > 0 && dimmed && !alarm && calState == CAL_IDLE && now - lastActivity > WIRED_SCREEN_OFF_MS;
    if (standby) wantOff = !(standbyShowUntil && (int32_t)(now - standbyShowUntil) < 0);
    if (wantOff && !screenOff) { screenOff = true; oled.sendCommand(0xAE); }        // display off (sleep)
    else if (!wantOff && screenOff) { screenOff = false; oled.sendCommand(0xAF); }  // display on
  }

  // ---- Battery every 2 s ----
  if (now - lastBatt >= 2000) {
    lastBatt = now; readBattery();
    if (!wiredModel && gauge != G_NONE && battPct < BATT_CRIT_PCT && battV < 3.5f && !charging) {
      if (!critSince) critSince = now;
      if (now - critSince > 10000) powerOff("Battery", "flat");
    } else critSince = 0;
  }

  // ---- Screen at ~10 Hz ----
  static uint32_t lastUi = 0, lastLog = 0;
  if (now - lastUi < (theme == TH_CLEAN ? 50u : 100u)) { delay(2); return; }   // new look: 20 frames a second
  lastUi = now;

  takeSnapshot();
  float db = snapDb();
  float fastDb = snapFastDb();
  if (warmup <= 0 && (fastDb >= peakDb || now - peakAt > 2000)) { peakDb = fastDb; peakAt = now; }

  if (!restartAt) { pushServe = true; drawFrame(now); pushServe = false; }   // keep the UPDATED / RESET screen up until the restart

  if (now - lastLog >= 1000) {
    lastLog = now;
    Serial.printf("%s %s %.1f dB  max %.1f  batt %.2fV %d%%%s  wifi %s %s\n",
                  W_NAME[wMode], R_NAME[rMode], db, toDb(snap.maxF[wMode]), battV, battPct,
                  charging ? " (charging)" : "", wifiOn ? "on" : "off", netIp().c_str());
  }
}
