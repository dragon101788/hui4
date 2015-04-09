#include "screen_handler.h"
#include "screen_timer_thread.h"
#include "xmlproc.h"

ScreenExecProc g_screenExec;
void ScreenHandler::setTimer(ScreenTimerThread *thread,unsigned int us){
		thread->SwitchProc(this,us);
}
void ScreenHandler::stopTimer(ScreenTimerThread *thread){
		thread->SwitchProc(NULL,0);
}
	BaseView * ScreenHandler::findViewByName(const char *name){
		BaseView * view=(BaseView *)viewManager->GetElementByName(name);
		if(view==NULL){
			log_e("************************ERROR***************************\n");
			log_e("%s view %s name is incorrect,please check in page!!\n",this->name.c_str(),name);
			log_e("******************************************************** \n");
			exit(-1);
		}
		return view;
	}

