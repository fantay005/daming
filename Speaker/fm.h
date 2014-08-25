#ifndef __FM_H__
#define __FM_H__

typedef enum FM_OPRATE {
	open  = 0x31,
	close = 0x32,
	search = 0x33,
	last = 0x34,
	next = 0x35,
	Fmplay = 0x36,
	FmSNR= 0x37,
  FmRSSI = 0x38
}FM_OPRA_TYPE;

void fmopen(int freq);
void handlefm(FM_OPRA_TYPE type, unsigned char *data);

#endif
