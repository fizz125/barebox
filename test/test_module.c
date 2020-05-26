
#include <common.h>
#include <module.h>

int init_module()
{

	printf("Hello Module World\n");

	return 0;
}

void cleanup_module()
{
	printf("Goodbye Module World\n");
}
