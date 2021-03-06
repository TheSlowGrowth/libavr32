#include "types.h"

#include "spi.h"

#include "dac.h"

#include "conf_board.h"


struct {
    u16 value;
    u16 now;
    u16 off;
    u16 target;
    u16 slew;
    u16 slew_ms;
    u16 step;
    s32 delta;
    u32 a;
} aout[4];

static bool is_slewing[4];



void init_dacs(void) {
	// setup daisy chain for two dacs
	spi_selectChip(SPI,DAC_SPI);
	spi_write(SPI,0x80);
	spi_write(SPI,0xff);
	spi_write(SPI,0xff);
	spi_unselectChip(SPI,DAC_SPI);

	reset_dacs();
}

void reset_dacs(void) {
	for(uint8_t i=0;i<4;i++) {
		aout[i].now = 0;
		aout[i].off = 0;
		aout[i].target = 0;
		aout[i].slew = 1;
		aout[i].step = 1;
		aout[i].delta = 0;
		aout[i].a = 0;

		is_slewing[i] = false;
	}

	dac_timer_update();
}




void update_dacs(uint16_t *d) {
	spi_selectChip(SPI,DAC_SPI);
	spi_write(SPI,0x31);
	spi_write(SPI,d[2]>>4); // 2
	spi_write(SPI,d[2]<<4);
	spi_write(SPI,0x31);
	spi_write(SPI,d[0]>>4); // 0
	spi_write(SPI,d[0]<<4);
	spi_unselectChip(SPI,DAC_SPI);
	
	spi_selectChip(SPI,DAC_SPI);
	spi_write(SPI,0x38);
	spi_write(SPI,d[3]>>4); // 3
	spi_write(SPI,d[3]<<4);
	spi_write(SPI,0x38);
	spi_write(SPI,d[1]>>4); // 1
	spi_write(SPI,d[1]<<4);
	spi_unselectChip(SPI,DAC_SPI);
}


void dac_set_value_noslew(uint8_t n, uint16_t v) {
    aout[n].value = v;
	int16_t t = v + aout[n].off;
    if (t < 0)
        t = 0;
    else if (t > 16383)
        t = 16383;
    aout[n].target = t;

    aout[n].step = 1;
    aout[n].now = aout[n].target;

    aout[n].a = aout[n].now << 16;
}

void dac_set_value(uint8_t n, uint16_t v) {
    aout[n].value = v;
	int16_t t = v + aout[n].off;
    if (t < 0)
        t = 0;
    else if (t > 16383)
        t = 16383;
    aout[n].target = t;

    aout[n].step = aout[n].slew;
    aout[n].delta = ((aout[n].target - aout[n].now) << 16) / aout[n].step;

    aout[n].a = aout[n].now << 16;
}

void dac_set_slew(uint8_t n, uint16_t s) {
    aout[n].slew_ms = s;
	aout[n].slew = s / DAC_RATE_CV;
    if (aout[n].slew == 0)
    	aout[n].slew = 1;
}

void dac_set_off(uint8_t n, int16_t o) {
    aout[n].off = o;
}

uint16_t dac_get_value(uint8_t n) {
    return aout[n].value;
}

uint16_t dac_get_slew(uint8_t n) {
    return aout[n].slew_ms;
}

uint16_t dac_get_off(uint8_t n) {
    return aout[n].off;
}


void dac_timer_update(void) {
    u8 i, r = 0;
    u16 a;

    for (i = 0; i < 4; i++)
        if (aout[i].step) {
            aout[i].step--;

            is_slewing[i] = false;

            if (aout[i].step == 0) { aout[i].now = aout[i].target; }
            else {
                aout[i].a += aout[i].delta;
                aout[i].now = aout[i].a >> 16;
                is_slewing[i] = true;
            }

            r++;
        }

    if (r) {
        spi_selectChip(SPI, DAC_SPI);
        spi_write(SPI, 0x31);
        a = aout[2].now >> 2;
        spi_write(SPI, a >> 4);
        spi_write(SPI, a << 4);
        spi_write(SPI, 0x31);
        a = aout[0].now >> 2;
        spi_write(SPI, a >> 4);
        spi_write(SPI, a << 4);
        spi_unselectChip(SPI, DAC_SPI);

        spi_selectChip(SPI, DAC_SPI);
        spi_write(SPI, 0x38);
        a = aout[3].now >> 2;
        spi_write(SPI, a >> 4);
        spi_write(SPI, a << 4);
        spi_write(SPI, 0x38);
        a = aout[1].now >> 2;
        spi_write(SPI, a >> 4);
        spi_write(SPI, a << 4);
        spi_unselectChip(SPI, DAC_SPI);
    }
}


bool dac_is_slewing(uint8_t n) {
	return is_slewing[n];
}

