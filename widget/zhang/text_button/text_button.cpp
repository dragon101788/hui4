#include "text_button.h"


//XMLinstan["button"] = Install_Element<button>;
static InstallXMLinstan install("text_button",Install_Element<text_button>);


hucall int init(HUMap & mp,void * data)
{
	log_i("module init\r\n");
}
