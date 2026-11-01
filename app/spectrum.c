/*
 * PY2ALC Fox Hunt for VUURWERK v1.5.1
 * Compact receive-only field-strength hunter.
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

void PatrolSaveSettings(void) { patrolActive = false; }
void PatrolLoadSettings(void) { patrolActive = false; }

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

static void TuneFox(const uint32_t freq)
{
    BK4819_PickRXFilterPathBasedOnFrequency(freq);
    BK4819_SetFrequency(freq);

    /* Re-trigger RX/VCO calibration, matching the old spectrum tuner. */
    const uint16_t reg30 = BK4819_ReadRegister(BK4819_REG_30);
    BK4819_WriteRegister(BK4819_REG_30, 0);
    BK4819_WriteRegister(BK4819_REG_30, reg30);
    SYSTEM_DelayMs(40);
}

static uint8_t FoxLevel(const int16_t dbm)
{
    int level = ((int)dbm + 140) / 5;
    if (level < 0) level = 0;
    if (level > 20) level = 20;
    return (uint8_t)level;
}

static void DrawFox(const uint32_t freq,
                    const ModulationMode_t modulation,
                    const int16_t dbm,
                    const int16_t peakDbm,
                    const int16_t minDbm,
                    const int16_t trendDb,
                    const uint8_t attStep)
{
    char line[32];
    char bar[21];
    const uint8_t level = FoxLevel(dbm);

    memset(gFrameBuffer, 0, sizeof(gFrameBuffer));
    UI_PrintStringSmallBold("FOX HUNT", 0, 127, 0);

    sprintf(line, "%u.%05u %s",
            (unsigned)(freq / 100000u),
            (unsigned)(freq % 100000u),
            gModulationStr[modulation]);
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

    UI_PrintStringSmallNormal("*=FREQ UP/DN ATT 0=RST", 0, 127, 6);
    ST7565_BlitFullScreen();
}

static void DrawFrequencyInput(const char digits[8], const uint8_t count, const bool invalid)
{
    char shown[10] = "___._____";

    for (uint8_t i = 0; i < count && i < 8; i++)
        shown[i < 3 ? i : i + 1] = digits[i];

    memset(gFrameBuffer, 0, sizeof(gFrameBuffer));
    UI_PrintStringSmallBold("FOX FREQUENCY", 0, 127, 0);
    UI_PrintStringSmallNormal("TYPE 8 DIGITS", 0, 127, 1);
    UI_PrintStringSmallBold(shown, 0, 127, 3);
    UI_PrintStringSmallNormal(invalid ? "INVALID FREQUENCY" : "MENU=OK  *=BACK", 0, 127, 5);
    UI_PrintStringSmallNormal("EXIT=CANCEL", 0, 127, 6);
    ST7565_BlitFullScreen();
}

static bool EnterFrequency(uint32_t *freq)
{
    char digits[8] = {0};
    uint8_t count = 0;
    bool invalid = false;

    DrainKeys();

    for (;;) {
        DrawFrequencyInput(digits, count, invalid);
        const KEY_Code_t key = KEYBOARD_Poll();

        if (key == KEY_INVALID) {
            SYSTEM_DelayMs(20);
            continue;
        }

        if (key >= KEY_0 && key <= KEY_9) {
            if (count < 8)
                digits[count++] = (char)('0' + (key - KEY_0));
            invalid = false;
            DrainKeys();
            continue;
        }

        if (key == KEY_STAR) {
            if (count > 0)
                count--;
            invalid = false;
            DrainKeys();
            continue;
        }

        if (key == KEY_EXIT) {
            DrainKeys();
            return false;
        }

        if (key == KEY_MENU) {
            if (count == 8) {
                uint32_t value = 0;
                for (uint8_t i = 0; i < 8; i++)
                    value = value * 10u + (uint32_t)(digits[i] - '0');

                if (RX_freq_check(value) == 0) {
                    *freq = value;
                    DrainKeys();
                    return true;
                }
            }
            invalid = true;
            DrainKeys();
            continue;
        }

        DrainKeys();
    }
}

void APP_RunSpectrum(void)
{
    const uint16_t savedReg13 = BK4819_ReadRegister(BK4819_REG_13);
    const ModulationMode_t huntModulation = gTxVfo->Modulation;
    uint32_t huntFreq = gTxVfo->pRX->Frequency;
    uint8_t attStep = 0;
    int16_t peakDbm = -160;
    int16_t minDbm = 0;
    int16_t trendRef = -160;
    int16_t trendDb = 0;
    uint8_t trendTicks = 0;
    bool first = true;

    /* Fox Hunt always follows the operator-selected VFO, not whichever VFO
     * Dual RX happened to be sampling when the app was opened. */
    TuneFox(huntFreq);
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

        if (dbm > peakDbm) peakDbm = dbm;
        if (dbm < minDbm) minDbm = dbm;

        if (++trendTicks >= 20) {
            trendDb = dbm - trendRef;
            trendRef = dbm;
            trendTicks = 0;
        }

        DrawFox(huntFreq, huntModulation, dbm, peakDbm, minDbm, trendDb, attStep);

        const KEY_Code_t key = KEYBOARD_Poll();
        if (key == KEY_EXIT) {
            DrainKeys();
            break;
        }

        if (key == KEY_STAR) {
            if (EnterFrequency(&huntFreq)) {
                TuneFox(huntFreq);
                BK4819_WriteRegister(BK4819_REG_13, sAttReg13[attStep]);
                first = true;
                trendDb = 0;
                trendTicks = 0;
            }
            continue;
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
    RADIO_SetupRegisters(true);
    gRequestDisplayScreen = DISPLAY_MAIN;
}
