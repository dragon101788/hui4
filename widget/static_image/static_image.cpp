#include "static_image.h"


static InstallXMLinstan install("static_image",Install_Element<static_image>);

hucall int init(HUMap & mp,void * data)
{
	printf("module init\r\n");
}
