#include "xmlproc.h"
#include "thread_touch.h"
#include "thread_timer.h"
#include "thread_msg.h"

DebugTimer fps;
pXmlproc g_cur_xml;
map<hustr, pXmlproc> g_xml_proc;

ProcTimer g_exec;

int HuExec::doStart()
{
	if (have)
	{
		if (!run.empty())
		{
			system(run);
		}
		if (!cs.empty())
		{
			printf("exec xml=%s cs=%s\r\n", g_cur_xml->filename.c_str(), cs.c_str());
			g_cur_xml->PostCS(cs);
		}
	}

}

int xmlproc::init()
{
	elemgr = this;
	isDraw = 0; //默认无绘制图像
	fore = 0; //默认为后台进程
	done = 0; //默认非完成状态
	if (out.isNULL())
	{
		if (out.SetBuffer(fb.u32Width, fb.u32Height))
		{
			huErrExit("$$$HU$$$ blt_set_dest_buff error\r\n");
		}
	}
	m_exit = 1;
	create();
}

void xmlproc::ForeProc()
{
	fore = 1;
	g_th_touch.SwitchProc(this);
	g_th_timer.SwitchProc(this);
	g_exec.ChangeContainer(this);
}
void xmlproc::UnForeProc()
{
	fore = 0;
	g_th_touch.SwitchProc(NULL);
	g_th_timer.SwitchProc(NULL);
	g_exec.ChangeContainer(NULL);
}

void xmlproc::DoneProc()
{
	done = 1;
}
void xmlproc::UnDoneProc()
{
	done = 0;
}

