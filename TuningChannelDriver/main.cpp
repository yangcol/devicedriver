#include <stdlib.h>
#include <stdio.h>
#include "TuningChannelDriver.h"
#include "TCD_FTDICHIP.h"
int main()
{
	TCDHandle tcdH = NULL;
	TuningChannelDriver* p = new TCD_FTDICHIP(tcdH);
	p->InitialChannel();
	printf("I do nothing here!\n");
	return 0;
}