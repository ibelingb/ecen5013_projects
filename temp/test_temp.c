/* unit tests */

#include <stdlib.h>
#include <stdio.h>

#include "lu_iic.h"
#include "temp.h"

int main(void)
{
	printf("test cases for TMP02 temp sensor library\n");

	if(initIic("/dev/i2c-2") < 0)
		printf("i2c init failed\n");
	
	return EXIT_SUCCESS;
}
