#include "ui.h"

#include <TFT_eSPI.h>
#include <math.h>

// FreeFonts (FreeSans9pt7b, FreeSansBold9pt7b, FreeSansBold18pt7b) are pulled
// in automatically by TFT_eSPI when LOAD_GFXFF is defined — do not re-include
// the headers here or the symbols are redefined at link time.

namespace {

// --- Display geometry -------------------------------------------------------
constexpr int      W       = 240;
constexpr int      H       = 240;
constexpr int      CX      = W / 2;
constexpr int      CY      = H / 2;
constexpr float    R_OUT   = 118.0f;
constexpr float    R_TICK_MAJOR_IN = 105.0f;
constexpr float    R_TICK_MINOR_IN = 112.0f;
constexpr float    R_HOUR  = 52.0f;
constexpr float    R_MIN   = 82.0f;
constexpr float    R_SEC   = 92.0f;
constexpr float    R_SEC_TAIL = 18.0f;

// --- Palette ----------------------------------------------------------------
// 16-bit RGB565 helpers
constexpr uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint16_t)(r & 0xF8) << 8) | ((uint16_t)(g & 0xFC) << 3) | (b >> 3);
}

constexpr uint16_t COL_BLACK    = 0x0000;
constexpr uint16_t COL_WHITE    = 0xFFFF;
constexpr uint16_t COL_INK      = rgb(235, 240, 248);   // primary text/hands
constexpr uint16_t COL_DIM      = rgb(165, 175, 190);   // tick / date
constexpr uint16_t COL_ACCENT   = rgb(255, 138, 76);    // second hand / sun
constexpr uint16_t COL_FACE_RIM = rgb(40, 46, 56);

// --- TFT objects ------------------------------------------------------------
TFT_eSPI    tft;
TFT_eSprite frame(&tft);   // full-screen back-buffer (240x240 16bpp ~113KB)
bool        useSprite = false;

// --- Config copy ------------------------------------------------------------
bool gTwentyFourHour = false;
bool gShowSeconds    = true;

// Drawing target: sprite if available, else direct to TFT.
inline void drawPixel(int x, int y, uint16_t c) {
    useSprite ? frame.drawPixel(x, y, c) : tft.drawPixel(x, y, c);
}

// --- Linear gradient fill (top-bottom) for the round face -------------------
void fillRoundGradient(uint16_t topColor, uint16_t bottomColor) {
    // We restrict pixels to the inscribed circle so off-screen corners stay black.
    const int r2 = (int)(R_OUT * R_OUT);
    for (int y = 0; y < H; y++) {
        float t = (float)y / (H - 1);
        uint8_t r = (uint8_t)(((topColor >> 11) & 0x1F) * (1 - t) + ((bottomColor >> 11) & 0x1F) * t) << 3;
        uint8_t g = (uint8_t)(((topColor >> 5)  & 0x3F) * (1 - t) + ((bottomColor >> 5)  & 0x3F) * t) << 2;
        uint8_t b = (uint8_t)(( topColor        & 0x1F) * (1 - t) + ( bottomColor        & 0x1F) * t) << 3;
        uint16_t row = rgb(r, g, b);
        for (int x = 0; x < W; x++) {
            int dx = x - CX, dy = y - CY;
            if (dx * dx + dy * dy <= r2) drawPixel(x, y, row);
        }
    }
}

// Draw a soft filled circle (anti-alias-ish via two passes) used for sun/moon.
void softDisc(int cx, int cy, int radius, uint16_t inner, uint16_t halo) {
    if (useSprite) {
        frame.fillCircle(cx, cy, radius + 6, halo);
        frame.fillCircle(cx, cy, radius,     inner);
    } else {
        tft.fillCircle(cx, cy, radius + 6, halo);
        tft.fillCircle(cx, cy, radius,     inner);
    }
}

void fillCircle(int x, int y, int r, uint16_t c) {
    useSprite ? frame.fillCircle(x, y, r, c) : tft.fillCircle(x, y, r, c);
}
void drawLine(int x0, int y0, int x1, int y1, uint16_t c) {
    useSprite ? frame.drawLine(x0, y0, x1, y1, c) : tft.drawLine(x0, y0, x1, y1, c);
}
void drawWideLine(int x0, int y0, int x1, int y1, float w, uint16_t c) {
    useSprite ? frame.drawWideLine(x0, y0, x1, y1, w, c)
              : tft.drawWideLine(x0, y0, x1, y1, w, c);
}
void fillTri(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t c) {
    useSprite ? frame.fillTriangle(x0, y0, x1, y1, x2, y2, c)
              : tft.fillTriangle(x0, y0, x1, y1, x2, y2, c);
}
void drawCircle(int x, int y, int r, uint16_t c) {
    useSprite ? frame.drawCircle(x, y, r, c) : tft.drawCircle(x, y, r, c);
}

// --- Weather background scenes ---------------------------------------------
void drawWeatherBackground(const WeatherData& w) {
    const bool day = w.isDay;

    switch (w.kind) {
        case WeatherKind::Clear: {
            if (day) {
                fillRoundGradient(rgb(255, 175, 90), rgb(90, 150, 220));
                softDisc(CX + 56, CY - 60, 16, rgb(255, 230, 140), rgb(255, 190, 110));
            } else {
                fillRoundGradient(rgb(8, 12, 32), rgb(28, 38, 70));
                // Stars
                const int stars[][2] = {{42,38},{60,80},{180,50},{200,90},{86,200},{170,180},{120,30}};
                for (auto& s : stars) fillCircle(s[0], s[1], 1, rgb(220,225,255));
                softDisc(CX + 56, CY - 60, 14, rgb(230, 235, 255), rgb(140, 150, 200));
            }
            break;
        }
        case WeatherKind::PartlyCloudy: {
            if (day) fillRoundGradient(rgb(180, 200, 230), rgb(120, 150, 190));
            else     fillRoundGradient(rgb(20, 28, 50),    rgb(40, 50, 80));
            // Sun/moon peeking
            if (day) softDisc(CX + 50, CY - 55, 14, rgb(255, 230, 140), rgb(255, 190, 110));
            else     softDisc(CX + 50, CY - 55, 12, rgb(220, 225, 255), rgb(120, 130, 180));
            // Cloud cluster
            uint16_t cl = day ? rgb(245,247,252) : rgb(180,188,210);
            fillCircle(CX - 30, CY - 40, 20, cl);
            fillCircle(CX -  5, CY - 50, 24, cl);
            fillCircle(CX + 18, CY - 38, 18, cl);
            fillCircle(CX -  8, CY - 28, 22, cl);
            break;
        }
        case WeatherKind::Cloudy: {
            fillRoundGradient(rgb(140, 148, 160), rgb(90, 98, 110));
            uint16_t cl = rgb(210, 215, 225);
            fillCircle(CX - 40, CY - 30, 22, cl);
            fillCircle(CX - 10, CY - 45, 26, cl);
            fillCircle(CX + 25, CY - 30, 22, cl);
            fillCircle(CX -  5, CY - 18, 26, cl);
            break;
        }
        case WeatherKind::Fog: {
            fillRoundGradient(rgb(170, 170, 165), rgb(120, 122, 125));
            uint16_t l = rgb(200, 200, 198);
            for (int y = 60; y <= 200; y += 16) {
                drawWideLine(40, y, 200, y, 6.0f, l);
            }
            break;
        }
        case WeatherKind::Rain: {
            fillRoundGradient(rgb(60, 70, 88), rgb(30, 38, 52));
            uint16_t cl = rgb(120, 130, 150);
            fillCircle(CX - 30, CY - 50, 22, cl);
            fillCircle(CX -  5, CY - 60, 26, cl);
            fillCircle(CX + 22, CY - 48, 22, cl);
            uint16_t drop = rgb(120, 175, 230);
            for (int i = 0; i < 14; i++) {
                int x = 40 + (i * 13) % 160;
                int y = 90 + (i * 23) % 110;
                drawWideLine(x, y, x - 4, y + 12, 1.6f, drop);
            }
            break;
        }
        case WeatherKind::Snow: {
            fillRoundGradient(rgb(220, 230, 240), rgb(150, 175, 200));
            uint16_t fl = rgb(250, 252, 255);
            const int snow[][2] = {{60,60},{160,70},{90,110},{150,130},{50,150},{180,170},{120,90},{200,130},{80,180},{130,170}};
            for (auto& s : snow) fillCircle(s[0], s[1], 2, fl);
            break;
        }
        case WeatherKind::Storm: {
            fillRoundGradient(rgb(50, 50, 70), rgb(20, 20, 35));
            uint16_t cl = rgb(80, 84, 100);
            fillCircle(CX - 30, CY - 50, 22, cl);
            fillCircle(CX -  5, CY - 60, 26, cl);
            fillCircle(CX + 22, CY - 48, 22, cl);
            // Lightning bolt
            uint16_t bolt = rgb(255, 230, 100);
            const int b[][2] = {{120,80},{108,120},{122,118},{108,160},{132,112},{118,114}};
            fillTri(b[0][0],b[0][1], b[1][0],b[1][1], b[2][0],b[2][1], bolt);
            fillTri(b[2][0],b[2][1], b[3][0],b[3][1], b[4][0],b[4][1], bolt);
            break;
        }
        case WeatherKind::Unknown:
        default: {
            fillRoundGradient(rgb(38, 44, 56), rgb(14, 18, 26));
            break;
        }
    }
}

// --- Clock face decoration --------------------------------------------------
void drawTicks() {
    for (int i = 0; i < 60; i++) {
        const float a = (float)i * (PI / 30.0f) - PI / 2.0f;
        const float ca = cosf(a), sa = sinf(a);
        const bool major = (i % 5) == 0;
        const float rIn = major ? R_TICK_MAJOR_IN : R_TICK_MINOR_IN;
        const int x0 = CX + (int)(ca * R_OUT);
        const int y0 = CY + (int)(sa * R_OUT);
        const int x1 = CX + (int)(ca * rIn);
        const int y1 = CY + (int)(sa * rIn);
        if (major) {
            drawWideLine(x0, y0, x1, y1, 2.4f, COL_INK);
        } else {
            drawLine(x0, y0, x1, y1, COL_DIM);
        }
    }
    // subtle inner rim ring
    drawCircle(CX, CY, (int)R_OUT,     COL_FACE_RIM);
    drawCircle(CX, CY, (int)R_OUT - 1, COL_FACE_RIM);
}

// --- Hands ------------------------------------------------------------------
// Tapered hand: triangle from base-left to tip to base-right + circular pivot.
void drawHand(float angleRad, float length, float baseHalfWidth, uint16_t color) {
    const float ca = cosf(angleRad), sa = sinf(angleRad);
    // Perpendicular (for base width)
    const float px = -sa, py = ca;

    const int tipX  = CX + (int)(ca * length);
    const int tipY  = CY + (int)(sa * length);
    const int blX   = CX + (int)(px *  baseHalfWidth);
    const int blY   = CY + (int)(py *  baseHalfWidth);
    const int brX   = CX + (int)(px * -baseHalfWidth);
    const int brY   = CY + (int)(py * -baseHalfWidth);
    fillTri(tipX, tipY, blX, blY, brX, brY, color);
}

void drawSecondHand(int seconds) {
    const float a = (float)seconds * (PI / 30.0f) - PI / 2.0f;
    const float ca = cosf(a), sa = sinf(a);
    const int tipX  = CX + (int)(ca *  R_SEC);
    const int tipY  = CY + (int)(sa *  R_SEC);
    const int tailX = CX - (int)(ca *  R_SEC_TAIL);
    const int tailY = CY - (int)(sa *  R_SEC_TAIL);
    drawWideLine(tailX, tailY, tipX, tipY, 1.6f, COL_ACCENT);
    fillCircle(tipX, tipY, 2, COL_ACCENT);
}

// --- Text helpers -----------------------------------------------------------
void setSpriteFont(const GFXfont* f) {
    if (useSprite) frame.setFreeFont(f);
    else           tft.setFreeFont(f);
}
void setTextColors(uint16_t fg) {
    if (useSprite) { frame.setTextColor(fg); }
    else           { tft.setTextColor(fg);   }
}
void setTextDatumC() {
    if (useSprite) frame.setTextDatum(MC_DATUM);
    else           tft.setTextDatum(MC_DATUM);
}
void drawStringC(const char* s, int x, int y) {
    if (useSprite) frame.drawString(s, x, y);
    else           tft.drawString(s, x, y);
}

// --- Center digital time, day, date ----------------------------------------
const char* kDayShort[]   = {"SUN","MON","TUE","WED","THU","FRI","SAT"};
const char* kMonthShort[] = {"JAN","FEB","MAR","APR","MAY","JUN",
                             "JUL","AUG","SEP","OCT","NOV","DEC"};

void drawCenterText(const struct tm& t, const WeatherData& w) {
    char timeBuf[8];
    int  hour = t.tm_hour;
    if (!gTwentyFourHour) {
        hour = hour % 12;
        if (hour == 0) hour = 12;
    }
    snprintf(timeBuf, sizeof(timeBuf), "%d:%02d", hour, t.tm_min);

    setTextDatumC();
    setSpriteFont(&FreeSansBold18pt7b);
    setTextColors(COL_INK);
    drawStringC(timeBuf, CX, CY - 6);

    char dateBuf[24];
    snprintf(dateBuf, sizeof(dateBuf), "%s  %s %d",
             kDayShort[t.tm_wday % 7],
             kMonthShort[t.tm_mon  % 12],
             t.tm_mday);
    setSpriteFont(&FreeSans9pt7b);
    setTextColors(COL_DIM);
    drawStringC(dateBuf, CX, CY + 24);

    // Tiny temperature at the bottom (above 6 o'clock) when valid
    if (w.valid) {
        char tBuf[12];
        snprintf(tBuf, sizeof(tBuf), "%d°%s",
                 (int)lroundf(w.temperature), w.unitsTemp);
        setSpriteFont(&FreeSansBold9pt7b);
        setTextColors(COL_INK);
        drawStringC(tBuf, CX, CY + 56);
    }

    // AM/PM marker (when 12-hour) above the digital time
    if (!gTwentyFourHour) {
        const char* ampm = (t.tm_hour < 12) ? "AM" : "PM";
        setSpriteFont(&FreeSans9pt7b);
        setTextColors(COL_DIM);
        drawStringC(ampm, CX, CY - 42);
    }
}

}  // namespace

namespace UI {

void begin(bool twentyFourHour, bool showSeconds) {
    gTwentyFourHour = twentyFourHour;
    gShowSeconds    = showSeconds;

    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);

    // Try to allocate a full-screen back-buffer for flicker-free rendering.
    frame.setColorDepth(16);
    if (frame.createSprite(W, H)) {
        useSprite = true;
        log_i("UI: using 240x240 16bpp sprite (back-buffer)");
    } else {
        useSprite = false;
        log_w("UI: sprite alloc failed, falling back to direct draw");
    }
}

void showStatus(const char* line1, const char* line2) {
    // Drawn directly; never via sprite (used during boot before allocation).
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSansBold9pt7b);
    tft.setTextColor(COL_INK);
    tft.drawString(line1, CX, CY - (line2 ? 10 : 0));
    if (line2) {
        tft.setFreeFont(&FreeSans9pt7b);
        tft.setTextColor(COL_DIM);
        tft.drawString(line2, CX, CY + 14);
    }
}

void render(const struct tm& t, const WeatherData& w) {
    if (useSprite) frame.fillSprite(COL_BLACK);

    drawWeatherBackground(w);
    drawTicks();

    // Hour hand
    const float hourAngle =
        ((t.tm_hour % 12) + t.tm_min / 60.0f) * (PI / 6.0f) - PI / 2.0f;
    drawHand(hourAngle, R_HOUR, 4.0f, COL_INK);

    // Minute hand
    const float minAngle =
        (t.tm_min + t.tm_sec / 60.0f) * (PI / 30.0f) - PI / 2.0f;
    drawHand(minAngle, R_MIN, 3.0f, COL_INK);

    drawCenterText(t, w);

    // Second hand drawn last so it sits on top
    if (gShowSeconds) drawSecondHand(t.tm_sec);

    // Center pivot
    fillCircle(CX, CY, 4, COL_INK);
    fillCircle(CX, CY, 2, COL_ACCENT);

    if (useSprite) frame.pushSprite(0, 0);
}

}  // namespace UI
