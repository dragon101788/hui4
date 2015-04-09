#ifndef __SCREEN_HANDLER_H__
#define __SCREEN_HANDLER_H__



#include "ParaseXML.h"
#include "codec.h"
#include "schedule.h"
#include "hulib.h"
#include <deque>
#include <set>

class ScreenTimerThread;

class ScreenExec
{
public:
	hustr xml;
	typedef void (*pfunc)(const char *, const char *);
	pfunc p;
	ScreenExec()
	{
		xml=NULL;
		p=NULL;
	}
	 ~ScreenExec()
	{
		xml=NULL;
		p=NULL;
	}

	 ScreenExec(const char *file,pfunc func)
	{
		xml=file;
		p=func;
	}
	int setExec(const char *file,pfunc func)
	{
		xml=file;
		p=func;
	}
	void doStart(){
		p(xml.c_str(),NULL);
	}
	static int _exec(ScreenExec is)
	{
		is.doStart();
	}

};
typedef DirectExecThread<ScreenExec> ScreenExecProc;
extern ScreenExecProc g_screenExec;

extern  void JumpToFile(const char * jump, const char * snap);
extern  void IncludeXml(const char * include, const char * snap);
class BaseView;
class xmlproc;
class OnTouchListener{
public:
	OnTouchListener(){

	}
	virtual ~OnTouchListener(){

	}

	virtual void onDrag(hustr ele_name){

	}
	virtual void onTouchDown(hustr ele_name){
		//log_s("eventListener  doTouchDown\n");
	}
	virtual void onTouchUp(hustr ele_name){

	}
	virtual void onTouchActive(hustr ele_name){

	}
	virtual void onTimer(hustr ele_name,int time){

	}
};

class OnTimerListener{
public:
	OnTimerListener(){

	}
	virtual ~OnTimerListener(){

	}
	virtual void onTimer(hustr ele_name,int time)=0;
};


class KeypadListener{
public:

	virtual ~KeypadListener(){}
	//按键回调函数
	virtual void onKeyPressed(int key){
		log_i("keypressed key=%d !!!\n",key);
	};
};

class ScreenHandler:public KeypadListener
{
	public:

	ScreenHandler(){
		isRunning=0;
	}
	virtual ~ScreenHandler(){
		//deleteThis();
		onDelete();
	}


//	/******************
//	 * 加载完成
//	 */
//	 void loadDone(){
//		 onLoadDone();
//	}
//	 void includeDone(){
//		 onIncludeDone();
//	}
//	 void deleteThis(){
//		 onDelete();
//	}
	/******************
	 * 切换到前台
	 */
	 void coming(const char *lastFile){
		 isRunning=0;
		onComing(lastFile);
		//onResume();
	}
	/******************
	 * 切换到后台
	 */
	 void leaving(){
		 isRunning=1;
		onLeaving();
	}



	virtual void onDelete(){

	}
	/***********************
	 *
	 * 加载完成，页面必须实现
	 */
	virtual void onLoadDone()=0;

	/***********************
	 *
	 * include完成
	 */
	virtual  void onIncludeDone(hustr file_name){}
	/******************
	 * 切换到前台，页面必须实现
	 */
	virtual void onComing(const char * lastFile)=0;
	/******************
	 * 切换到后台，页面必须实现
	 */
	virtual void onLeaving()=0;

//	/******************
//	 * 暂停
//	 */
//	 void pause(){
//		 isRunning=0;
//		 onPause();
//	}
//	/******************
//	 * 恢复
//	 */
//	void resume(){
//		isRunning=1;
//		onResume();
//
//	}
//	/******************
//	 * 暂停
//	 */
//	virtual void onPause(){
//
//	}
//	/******************
//	 * 恢复
//	 */
//	virtual void onResume(){
//	}

//	virtual void onTouchOn(){
//
//	}
//	virtual void onTouchUp(){
//
//	}
/****************************
 * 页面级别的定时器
 */
	virtual void onTimer(){
	}



	/***********************
	 * 设置定时器
	 */
	void setTimer(ScreenTimerThread *thread,unsigned int us);

	void stopTimer(ScreenTimerThread *thread);

	virtual void doFlushConfig(){
		//name = m_mp["name"]->getvalue();
	}
	void FlushConfig(){
		doFlushConfig();
	}



	void gotoScreen(const char * xml){
		ScreenExec myExec;
		myExec.setExec(xml,JumpToFile);
		screenAddExec(myExec);
	}
	void includeXml(const char * xml){
		ScreenExec myExec;
		myExec.setExec(xml,IncludeXml);
		screenAddExec(myExec);
		//JumpToFile(xml,NULL);//不能直接调用此函数
	}
	void screenAddExec( ScreenExec c)
		{
			log_i("%s DitectExe.xml=%s",name.c_str(),c.xml.c_str());
			g_screenExec.execAdd(name,ScreenExec::_exec,c);

		}



	bool isRun() const{
		return isRunning;
	}


	BaseView * findViewByName(const char *name);

	bool isRunning;
	xmlproc *viewManager;
	HUMap m_mp;
	hustr name;




};











#endif //__LAYER_H__