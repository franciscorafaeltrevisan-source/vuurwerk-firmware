/*
 * PY2ALC Fox Hunt for VUURWERK v1.5.1
 * Replaces the heavy Spectrum/PATROL application with a compact
 * receive-only field-strength hunter on the currently tuned frequency.
 *
 * GPL-3.0-or-later as part of VUURWERK.
 */

#include "app/spectrum.h"
#include "ui/ui.h"
#include "ui/helper.h"
#include "driver/st7565.h"
#include "driver/system.h"
#include "external/printf/printf.h"
#include <string.h>

/* Keep the v1.5.1 PATROL menu ABI satisfied.  PATROL itself is intentionally
 * disabled in this edition; the former Spectrum entry now launches Fox Hunt.
 */
PatrolSettings patrolSettings = {
    .sensitivity = 1,
    .speed       = 1,
    .dwell       = 1,
    .resume      = 1,
    .step        = PATROL_STEP_VIEW,
    .modulation  = 0x0F,
    .rangeStart  = 0,
    .rangeStop   = 0,
};
bool patrolActive = false;

void PatrolSaveSettings(void)
{
    patrolActive = false;
}

void PatrolLoadSettings(void)
{
    patrolActive = false;
}

/* BK4819 REG_13 gain ladder.  The first four steps reduce PGA gain, while
 * the last two also reduce front-end LNA gain so a very close fox does not
 * pin the receiver at full scale.
 */
static const uint16_t sAttReg13[] = {
    0x03DF, /*   0 dB */
    0x03DD, /*  -6 dB */
    0x03DB, /* -15 dB */
    0x03D9, /* -27 dB */
    0x0379, /* strong-signal front-end cut */
    0x0139, /* deepest front-end cut */
};

static const uint8_t sAttDb[] = {0, 6, 15, 27, 36, 70};

static void DrainKeys(void)
{
    while (KEYBOARD_Poll() != KEY_INVALID)
        SYSTEM_DelayMs(10);
}

static uint8_t FoxLevel(const int16_t dbm)
{
    /* 20-cell bar from roughly noise-floor (-140 dBm) to very strong
     * close-range signal (-40 dBm).  Values outside the window clamp.
     */
    int level = ((int)dbm + 140) / 5;
    if (level < 0)
        level = 0;
    if (level > 20)
        level = 20;
    return (uint8_t)level;
}

static void DrawFox(const int16_t dbm,
                    const int16_t peakDbm,
                    const int16_t minDbm,
                    const int16_t trendDb,
                    const uint8_t attStep)
{
    char line[32];
    char bar[21];
    const uint32_t f = gRxVfo->pRX->Frequency;
    const uint8_t level = FoxLevel(dbm);

    memset(gFrameBuffer, 0, sizeof(gFrameBuffer));

    UI_PrintStringSmallBold("FOX HUNT", 0, 127, 0);

    sprintf(line, "%u.%05u %s",
            (unsigned)(f / 100000u),
            (unsigned)(f % 100000u),
            gModulationStr[gRxVfo->Modulation]);
    UI_PrintStringSmallNormal(line, 0, 127, 1);

    sprintf(line, "RSSI %d dBm", (int)dbm);
    UI_PrintStringSmallNormal(line, 0, 127, 2);

    sprintf(line, "PK %d  MN %d", (int)peakDbm, (int)minDbm);
    UI_PrintStringSmallNormal(line, 0, 127, 3);

    for (uint8_t i = 0; i < 20; i++)
        bar[i] = (i < level) ? '#' : '.';
    bar[20] = '\0';
    UI_PrintStringSmallNormal(bar, 0, 127, 4);

    if (trendDb > 0)
        sprintf(line, "UP +%ddB ATT %udB", (int)trendDb, sAttDb[attStep]);
    else if (trendDb < 0)
        sprintf(line, "DN %ddB ATT %udB", (int)trendDb, sAttDb[attStep]);
    else
        sprintf(line, "HOLD ATT %udB", sAttDb[attStep]);
    UI_PrintStringSmallNormal(line, 0, 127, 5);

    UI_PrintStringSmallNormal("UP/DN ATT  0=RST  X", 0, 127, 6);
    ST7565_BlitFullScreen();
}

void APP_RunSpectrum(void)
{
    const uint16_t savedReg13 = BK4819_ReadRegister(BK4819_REG_13);
    uint8_t attStep = 0;
    int16_t peakDbm = -160;
    int16_t minDbm = 0;
    int16_t trendRef = -160;
    int16_t trendDb = 0;
    uint8_t trendTicks = 0;
    bool first = true;

    /* Always start with the normal receive gain used by the radio. */
    BK4819_WriteRegister(BK4819_REG_13, sAttReg13[0]);
    SYSTEM_DelayMs(40);
    DrainKeys();

    for (;;) {
        const int16_t dbm = BK4819_GetRSSI_dBm();

        if (first) {
            peakDbm = dbm;
            minDbm = dbm;
            trendRef = dbm;
            first = false;
        }

        if (dbm > peakDbm)
            peakDbm = dbm;
        if (dbm < minDbm)
            minDbm = dbm;

        if (++trendTicks >= 20) { /* about once per second */
            trendDb = dbm - trendRef;
            trendRef = dbm;
            trendTicks = 0;
        }

        DrawFox(dbm, peakDbm, minDbm, trendDb, attStep);

        const KEY_Code_t key = KEYBOARD_Poll();
        if (key == KEY_EXIT) {
            DrainKeys();
            break;
        }

        if (key == KEY_UP && attStep + 1u < (uint8_t)(sizeof(sAttReg13) / sizeof(sAttReg13[0]))) {
            attStep++;
            BK4819_WriteRegister(BK4819_REG_13, sAttReg13[attStep]);
            SYSTEM_DelayMs(40);
            first = true;
            trendDb = 0;
            DrainKeys();
        } else if (key == KEY_DOWN && attStep > 0) {
            attStep--;
            BK4819_WriteRegister(BK4819_REG_13, sAttReg13[attStep]);
            SYSTEM_DelayMs(40);
            first = true;
            trendDb = 0;
            DrainKeys();
        } else if (key == KEY_0) {
            first = true;
            trendDb = 0;
            DrainKeys();
        }

        SYSTEM_DelayMs(50);
    }

    BK4819_WriteRegister(BK4819_REG_13, savedReg13);
    gRequestDisplayScreen = DISPLAY_MAIN;
}
