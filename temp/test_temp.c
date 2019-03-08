/* unit tests */

#include <stdlib.h>
#include <stdio.h>

#include "lu_iic.h"
#include "temp.h"

int main(void)
{
	int fd;
	uint8_t reg[2];

	printf("test cases for TMP02 temp sensor library\n");

	fd = initIic("/dev/i2c-2");
	if(fd < 0)
		printf("i2c init failed\n");

	if(tmp102_getTemp(fd, &reg[0]) < 0)
		printf("getTemp failed\n");
	else
		printf("got temp value: %x\n", reg[0]);

	if(tmp102_getConfiguration(fd, &reg[0]) < 0)
		printf("getConfig failed\n");
	else
		printf("got config value: %x\n", reg[0]);
	
	return EXIT_SUCCESS;
}
