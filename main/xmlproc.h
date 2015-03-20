#ifndef __XML_PORG_H__
#define __XML_PORG_H__

#include "manager_touch.h"
#include "manager_timer.h"
#include "manager_cs.h"
#include "Framebuffer.h"
#include "page_control.h"
class xmlproc;
typedef SmartPtr<xmlproc> pXmlproc;
extern pXmlproc g_cur_xml;
extern map<hustr, pXmlproc> g_xml_proc;
extern int go;
int ParseXMLElement2(hustr parentName,hustr name, HUMap & xmlmp, xmlproc * xml);
void ParaseTinyXmlFile(const char * file, xmlproc * xml);
void hui_exit(const char * cmd);
void JumpToFile(const char * jump, const char * snap);

extern DebugTimer fps;

class HuExec
{
public:
	hustr run;
	hustr cs;
	int have;
	HuExec()
	{
		have = 0;
	}
	HuExec(HUMap & mp)
	{
		have = 0;
		parse(mp);
	}
	int parse(HUMap & mp)
	{
		if (mp.exist("run"))
		{
			run = mp["run"]->getvalue();
			have = 1;
			//HUTimerAdd(run, g_exec.GetUpTimer() + ptimer, timerfun_run, run);
		}
		if (mp.exist("cs"))
		{
			cs = mp["cs"]->getvalue();
			have = 1;
			//HUTimerAdd(cs, g_exec.GetUpTimer() + ptimer, timerfun_cs, cs);
		}
	}
	virtual int doStart();
	static int _exec(int tm, HuExec is)
	{
		is.doStart();
	}
};

typedef HUTimer<HuExec> ProcTimer;
extern ProcTimer g_exec;
class xmlproc: public element_manager,
		public CS_manager,
		public timer_manager,
		public touch_manager,
		public schedule_draw,
		public thread,
		public ProcTimer::HUTimerContainer,
		virtual public Mutex
{
private:
	class xmlout: public image
	{
	public:
		void Draw(image * src_img, int src_x, int src_y, int cp_width, int cp_height, int dst_x, int dst_y)
		{
			lock();
			AreaCopy(src_img, src_x, src_y, cp_width, cp_height, dst_x, dst_y);
			unlock();
		}
		void RenderToFramebuffer(framebuffer * fb)
		{
			//lock();
		//	log_i("before fb->RenderImageToFrameBuffer(this);!!!!!!!!\r\n");
			fb->NotifyRenderFrameBuffer(this);
		//	unlock();
		}
	};
	xmlout out;
//	class tmexe
//	{
//	public:
//		tmexe(HUMap &mp, xmlproc * xml)
//		{
//			exec.parase(mp, xml);
//			tm = mp["tm"].getvalue_int();
//		}
//		executable exec;
//		int tm;
//	};
	class save_snap: public schedule_ele
	{
	public:
		xmlproc * m_xml;
		hustr name;
		save_snap(xmlproc * xml, const char * path)
		{
			m_xml = xml;
			name = path;
		}
		~save_snap()
		{
			log_i("~snap\r\n");
		}
		int SaveSnap(const char * file)
		{
			if (!access_Image(file))
			{
				log_i("%s no exist , save Snap !!!\r\n", file);
				m_xml->out.SaveResource(file);
				return 1;

			}
			else
			{
				log_i("%s exist , no save Snap !!!\r\n", file);
				return 0;
			}
		}
		void onSchedule()
		{
			SaveSnap(name);
			delete this;
		}
	};
	int fore; //前台
	int done; //完成解析
	int m_exit; //线程退出
	int isDraw; //有改变图像

public:
	int directDraw;
	hustr filename;
	PageControl *windCtl;
	void Draw(image * src_img, int src_x, int src_y, int cp_width, int cp_height, int dst_x, int dst_y)
	{
		//lock();
		out.Draw(src_img, src_x, src_y, cp_width, cp_height, dst_x, dst_y);
		isDraw++;
		//unlock();
	}



	void ForeProc(); //将页面切换前台
	void UnForeProc(); //将页面切换后台
	void DoneProc(); //强制完成
	void UnDoneProc(); //强制未完成

	static int timerfun_run(int tm, hustr cmd)
	{
		system(cmd);
	}
	static int timerfun_cs(int tm, hustr cs)
	{
		log_i("exec xml=%s cs=%s\r\n", g_cur_xml->filename.c_str(), cs.c_str());
		g_cur_xml->PostCS(cs);
	}
	void AddExec(int ptimer, HuExec c)
	{
		log_i("$$$HU$$$ exec %s %s\r\n", c.run.nstr(), c.cs.nstr());
		HUTimerAdd(filename, g_exec.GetUpTimer() + ptimer, HuExec::_exec, c);
	}

	int ScheduleSaveSnap(const char * file)
	{
		que.addele(new save_snap(this, file));
	}
	int run()
	{
		while (go && m_exit)
		{
			if(!fore){ //如果非前台，直接睡眠等待
				resetSem();
				waitSem();
			}
			int ret = ScheduleProc();
			if(directDraw)
				printFps();
			else
			ProcDraw();
			FPSWaitFPS(30);
		}
	}
	void ProcDraw()
	{
//		if (fore == 0 || isDraw == 0)
//			return;

		//lock();
		if (isDraw != 0 && fore == 1 && done == 1)
		{
			log_i("%s RenderFromBuffer\r\n",filename.c_str());
			out.RenderToFramebuffer(&fb);
			fps.debug_timer("<fps>");
			isDraw = 0;
		}
	//	unlock();
	}
	void printFps()
	{
		if (isDraw != 0 && fore == 1 && done == 1)
		{
			fps.debug_timer("<fps>");
			isDraw = 0;
		}
	}

	void ProcTouch(touch_sample * samp)
	{
		if (fore == 0)
			return;

		lock();
		if (fore == 1 && done == 1)
		{

			touch_proc_event(samp);

		}
		else
		{
			log_i("ProcTouch [%s] Not ready to touch fore=%d done=%d\r\n", filename.c_str(), fore, done);
		}
		unlock();
	}
	void ProcTimer()
	{
		if (fore == 0)
			return;

		lock();
		if (fore == 1 && done == 1)
		{
			UpdateTimer();
		}
		else
		{
			//log_i("ProcTimer %s Not ready to timer fore=%d done=%d\r\n", filename.c_str(), fore, done);
		}
		unlock();
	}
	virtual void doLoader()
	{
		ParseXMLFile();
	}
	void ParseXMLFile()
	{
		if (filename.empty())
		{
			huErrExit("can't init filename\r\n");
		}
		log_i("+++++++++++++%s++++++++++++++\r\n", filename.c_str());
		UnDoneProc();
		//DebugTimer dbg;
		ParaseTinyXmlFile(filename, this);
		//dbg.debug_timer("ParaseTinyXmlFile3");
		if(windCtl!=NULL)
			windCtl->loadDone();
		DoneProc();
		log_i("+++++++++++++%s++++++++++++++OK\r\n", filename.c_str());
	}
	void ParseXMLElementFile(const char * file)
	{

		ParaseTinyXmlFile(file, this);

	}

	virtual ~xmlproc()
	{
		log_i("~xmlproc %s\r\n", filename.c_str());
		Destroy();
	}

	int init();

	xmlproc()
	{
		init();
		directDraw=0;
		windCtl=NULL;

	}
	xmlproc(const char * file)
	{
		init();
		out.path.format("xml-%s", file);
		filename = file;

	}
	void drawDirect(image* src_img,int src_x,int src_y,int src_w,int src_h,int dst_x, int dst_y)
	{
		//lock();
		fb.RenderImageToFrameBuffer_part(src_img,src_x,src_y,src_w,src_h,dst_x,dst_y);
		isDraw++;
		//unlock();
	}


	void Destroy()
	{

		log_i("---------------%s----------------\r\n", filename.c_str());
		//system("ps|grep hui|sed -n \'1 p\'");

		UnForeProc();
		UnDoneProc();
		if(windCtl!=NULL){
			delete windCtl;
			windCtl=NULL;
		}
		m_exit = 0;
		lock();
		log_i("Destroy wait thread \r\n");
		cancel();
		//wait();
		log_i("Destroy wait timer_manager::ClearElement \r\n");
		timer_manager::ClearElement();

		log_i("Destroy wait touch_manager::ClearElement \r\n");
		touch_manager::ClearElement();

		log_i("Destroy wait element_manager::ClearElement \r\n");
		element_manager::ClearElement();
		//system("ps|grep hui|sed -n \'1 p\'");
		out.destroy();
		unlock();
		log_i("---------------%s----------------OK\r\n", filename.c_str());

	}
//	int LoadSnap(const char * file)
//	{
//		if (access(file, F_OK) == 0)
//		{
//			log_i("%s exist Load Snap !!!\r\n", file);
//			out.SetResource(file);
//			out.LoadResource();
//			return 1;
//		}
//		else
//		{
//			return 0;
//		}
//
//	}
//	int SaveSnap(const char * file)
//	{
//		if (access(file, F_OK) != 0)
//		{
//			log_i("%s no exist , save Snap !!!\r\n", file);
//			out.SaveResource(file);
//			return 1;
//		}
//		else
//		{
//			log_i("%s exist , no save Snap !!!\r\n", file);
//			return 0;
//		}
//	}
};

#endif //__XML_PORG_H__