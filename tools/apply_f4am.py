from pathlib import Path

p = Path("radio.c")
s = p.read_text()

inc = '#include "am_fix.h"\n'
if '#include "f4am.h"\n' not in s:
    assert inc in s
    s = s.replace(inc, inc + '#include "f4am.h"\n', 1)

old_head = '''void RADIO_SetModulation(ModulationMode_t modulation)
{
\tBK4819_AF_Type_t mod;
'''
new_head = '''void RADIO_SetModulation(ModulationMode_t modulation)
{
\tconst uint32_t rx_frequency = gRxVfo->pRX->Frequency;
\tconst bool f4am = F4AM_ShouldApply((uint8_t)modulation, rx_frequency, (uint8_t)MODULATION_AM);
\tBK4819_AF_Type_t mod;
'''
assert old_head in s
s = s.replace(old_head, new_head, 1)

old_am = '''\t\tcase MODULATION_AM:
\t\t\tmod = BK4819_AF_AM;
\t\t\tbreak;
'''
new_am = '''\t\tcase MODULATION_AM:
\t\t\t// F4HWN airband path: keep AF in FM and enable the BK4819 AM demodulator explicitly.
\t\t\tmod = f4am ? BK4819_AF_FM : BK4819_AF_AM;
\t\t\tbreak;
'''
assert old_am in s
s = s.replace(old_am, new_am, 1)

old_after_af = '''\tBK4819_SetAF(mod);

\tBK4819_SetRegValue(afDacGainRegSpec, 0xF);
'''
new_after_af = '''\tBK4819_SetAF(mod);

\t// PY2ALC F4AM: port the F4HWN AM demodulator path only for 118-137 MHz.
\tuint16_t reg31 = BK4819_ReadRegister(0x31);
\tif (f4am) {
\t\tBK4819_WriteRegister(0x31, reg31 | 1u);      // AM demodulation enable
\t\tBK4819_WriteRegister(0x42, F4AM_REG_42_AM);
\t\tBK4819_WriteRegister(0x2A, F4AM_REG_2A_AM);
\t\tBK4819_WriteRegister(0x2B, F4AM_REG_2B_AM);  // F4HWN STOCK AM audio profile
\t\tBK4819_WriteRegister(0x2F, F4AM_REG_2F_AM);
\t\tBK4819_WriteRegister(0x54, F4AM_REG_54_AM);
\t\tBK4819_WriteRegister(0x55, F4AM_REG_55_AM);
\t} else {
\t\tBK4819_WriteRegister(0x31, reg31 & 0xFFFEu); // AM demodulation disable
\t\tBK4819_WriteRegister(0x42, F4AM_REG_42_NORMAL);
\t\tBK4819_WriteRegister(0x2A, F4AM_REG_2A_NORMAL);
\t\tBK4819_WriteRegister(0x2B, F4AM_REG_2B_NORMAL);
\t\tBK4819_WriteRegister(0x2F, F4AM_REG_2F_NORMAL);
\t}

\t// PY2ALC F4AM VOL: +2 dB only after AM demodulation; preserve all other REG48 fields.
\tuint16_t reg48 = BK4819_ReadRegister(BK4819_REG_48);
\tconst uint16_t gain2 = f4am ? F4AM_RX_GAIN2_AM : F4AM_RX_GAIN2_NORMAL;
\tBK4819_WriteRegister(BK4819_REG_48,
\t\t(reg48 & (uint16_t)~0x03F0u) | ((gain2 & 0x3Fu) << 4));

\tBK4819_SetRegValue(afDacGainRegSpec, 0xF);
'''
assert old_after_af in s
s = s.replace(old_after_af, new_after_af, 1)

p.write_text(s)
