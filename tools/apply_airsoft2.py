from pathlib import Path

p = Path("radio.c")
s = p.read_text()

inc = '#include "am_fix.h"\n'
if '#include "air_soft.h"\n' not in s:
    assert inc in s
    s = s.replace(inc, inc + '#include "air_soft.h"\n', 1)

old = '\tBK4819_SetAF(mod);\n\n\tBK4819_SetRegValue(afDacGainRegSpec, 0xF);'
new = '''\tBK4819_SetAF(mod);

\t// PY2ALC AIR SOFT 2: aviation AM gets a tighter IF/audio path.
\t// All other modes restore their normal channel bandwidth/profile.
\tconst uint32_t rx_frequency = gRxVfo->pRX->Frequency;
\tconst bool air_soft = AIRSOFT_ShouldApply((uint8_t)modulation, rx_frequency, (uint8_t)MODULATION_AM);
\tif (air_soft) {
\t\tBK4819_WriteRegister(BK4819_REG_2B, AIRSOFT_REG_2B);
\t\tBK4819_WriteRegister(0x2F, AIRSOFT_REG_2F);
\t\tBK4819_WriteRegister(0x54, AIRSOFT_REG_54);
\t\tBK4819_WriteRegister(0x55, AIRSOFT_REG_55);
\t\tBK4819_WriteRegister(BK4819_REG_43, AIRSOFT_REG_43);
\t} else {
\t\tBK4819_WriteRegister(BK4819_REG_2B, 0x0000);
\t\tBK4819_WriteRegister(0x2F, 0x9890);
\t\tBK4819_WriteRegister(0x54, 0x9009);
\t\tBK4819_WriteRegister(0x55, 0x31A9);
\t\tBK4819_SetFilterBandwidth(gRxVfo->CHANNEL_BANDWIDTH, true);
\t}

\tBK4819_SetRegValue(afDacGainRegSpec, 0xF);'''

assert old in s
p.write_text(s.replace(old, new, 1))
