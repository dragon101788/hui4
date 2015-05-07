#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <signal.h>
#include "ParseXML.h"
#include "manager_touch.h"
#include "manager_timer.h"
#include "manager_cs.h"
#include "thread_msg.h"
#include "xmlproc.h"
#include "thread_timer.h"
#include "thread_touch.h"
#include "thread_keypad.h"
#include "XMLInstal.h"
#include "framebuffer.h"
#include "loaderDL.h"
#include "dlproc.h"
#include <sys/sysinfo.h>




using namespace std;

#define DEFAULT_XMLFILE "start.xml"
#define MIN_MEN_PERCENT 20  //当剩余内存小于20%时开始清楚多余的页面



void RefreshPage(xmlproc * xml);


int getFreeMemPercent()
{
        static struct sysinfo si;
        sysinfo(&si);
        int all=si.totalram>>20;//MB
        int free= si.freeram>>20;
        int percent=(free*100)/all;
        log_s("Totalram:       %d MB\n", all);
        log_s("Available:      %d MB\n", free);
//        log_i("Available:      %d \n",percent );
        return percent;
}
void huErrExit(const char * str)
{
	log_e("**************error exit*****************\r\n");
	puts(str);
	log_e("*****************************************\r\n");

	dumpstack();
	go = 0;
	g_th_timer.cancel();
	log_i("wait g_th_timer OK\r\n");
	g_th_touch.cancel();
	log_i("wait g_th_touch OK\r\n");
	g_th_msg.cancel();
	log_i("wait g_th_msg OK\r\n");
	map<hustr, pXmlproc>::iterator it;
	for (it = g_xml_proc.begin(); it != g_xml_proc.end(); ++it)
	{
		it->second->cancel();
	}
	g_cur_xml->cancel();
	exit(-1);
}

void IncludeXml(const char * include, const char * snap)
{
	g_cur_xml->ParseXMLElementFile(include);
}


void JumpToFile(const char * jump, const char * snap)
{

	//pXmlproc tmp = g_cur_xml;
	hustr lastFileName=NULL;
	if (!g_cur_xml.isNULL())
	{
		lastFileName=g_cur_xml->filename.c_str();
		g_cur_xml->UnForeProc();//这个函数应该具有使当前页线程暂停的功能

	}

	hustr math = strrchr(jump,'.');
	if(math==".so")
	{
		g_cur_xml = new dlproc(jump);
		g_cur_xml->doLoader();
	}
	else if(math==".xml")
	{
		int mem=getFreeMemPercent();
			if (g_xml_proc.find(jump) != g_xml_proc.end())  //已经缓存好了
			{
				log_i("$$$HU$$$ JumpToFile %s find cus\r\n", jump);
//				if (g_xml_proc[jump]->done == 1)
//				{
//					//fb.DumpToXml(g_xml_proc[jump]->out);
//
//				}
				g_cur_xml = g_xml_proc[jump];
				g_cur_xml->ForeProc(lastFileName.c_str());
			//	RefreshPage(g_cur_xml);

			}
			else  //重新打开，这里最好添加一个内存判断，如果内存不够，释放掉缓存中的某个页面
			{
				if(mem<MIN_MEN_PERCENT){

					map<hustr, pXmlproc>::iterator it;
					it = g_xml_proc.begin();
						if(it!=g_xml_proc.end()){
							log_i("kill %s page to free mem!!!\n",it->first.c_str());
							//it->second->cancel();
							g_xml_proc.erase(it);
						}

				}
//				hustr snapfile;
//				if (snap == NULL)
//				{
//					snapfile.format("%s.png", jump);
//				}
//				else
//				{
//					snapfile = snap;
//				}
//				log_i("snapfile = %s snap=%s\r\n", snap, snapfile.c_str());
//				if (access_Image(snapfile))
//				{
//					fb.DumpToSnap(snapfile);
//				}
				g_xml_proc[jump]=new xmlproc(jump);
				//g_cur_xml = new xmlproc(jump);
				g_cur_xml=g_xml_proc[jump];
				g_cur_xml->doLoader();
				g_cur_xml->ForeProc(lastFileName.c_str());
			}
	}




}

//BLT * g_blt = NULL;

int go = 1;

static void my_sighdlr(int sig)
{
	log_i("SIG = %d\r\n", sig);
	if (sig == SIGPIPE)
	{ // Ignore SIGPIPE.
		log_i("SIG = %d SIGPIPE\r\n", sig);
		return;
	}
	if (sig == SIGSEGV)
	{ // Ignore SIGPIPE.
		huErrExit("Segmentation fault\r\n");
		return;
	}

	go = 0;
}

static int _Demo_RegisterSigint()
{
	//signal(SIGINT, my_sighdlr);
	signal(SIGHUP, my_sighdlr);
	//signal(SIGQUIT, my_sighdlr);
	signal(SIGILL, my_sighdlr);
	//signal(SIGABRT, my_sighdlr);
	signal(SIGFPE, my_sighdlr);
	//signal(SIGKILL, my_sighdlr);
	signal(SIGPIPE, my_sighdlr);
	//signal(SIGTERM, my_sighdlr);
	signal(SIGSEGV, my_sighdlr);

	return 0;
}

int InitSystem()
{
	if (_Demo_RegisterSigint())
	{
		huErrExit("### Register Sigint Failed\n");
	}

	return 0;
}

void ParseCUS(hustr parentName,HUMap & xmlmp, xmlproc * xml)
{
	hustr cus = xmlmp["xmlfile"]->getvalue();
	//g_cur_xml = &g_xml_proc[cus];
	log_i("$$$HU$$$ CUS xmlfile %s\r\n", cus.c_str());
	g_xml_proc[cus] = new xmlproc(cus);
	g_xml_proc[cus]->doLoader();
}

void ParseJump(hustr parentName,HUMap & xmlmp, xmlproc * xml)
{
	hustr jump = xmlmp["xmlfile"]->getvalue();
	hustr snap = xmlmp["snap"]->getvalue();
	if (g_cur_xml->filename != jump)
	{
		JumpToFile(jump.nstr(), snap.nstr());
	}
}


void ParseWidget(hustr parentName,HUMap & xmlmp, xmlproc * xml)
{
	hustr dlfile = xmlmp["dlfile"]->getvalue();
	g_dl_map[dlfile].open(dlfile);

	HUMap arg;
	g_dl_map[dlfile].Func("init",arg,NULL);
}


void ParseSAVECUS(hustr parentName,HUMap & xmlmp, xmlproc * xml)
{
	log_i("ParseSAVECUS %s\r\n", xml->filename.c_str());
	g_xml_proc[xml->filename] = g_cur_xml;
}
void ParseEnv(hustr parentName,HUMap & xmlmp, xmlproc * xml)
{
	HUMap::iterator it;
	for (it = xmlmp.begin(); it != xmlmp.end(); ++it)
	{
		setenv(it.key().c_str(), it.value().getvalue(), 1);
		log_i("%s=%s\r\n", it.key().c_str(), getenv(it.value().c_str()));
		//debug[it->second.getkey()] = it->second->getvalue_int();
	}
}

void ParseControl(hustr parentName,HUMap & xmlmp, xmlproc * xml)
{
	hustr event = xmlmp["event"]->getvalue();

	if (event == "del")
	{
		hustr name = xmlmp["name"]->getvalue();
//		xml->timer_manager::DelElement(name);
//		xml->touch_manager::DelElement(name);

		xml->element_manager::DelElement(name);
	}
	else if (event == "proc")
	{
		int ptimer=0;
		if (xmlmp.exist("ptimer"))
		{
			//xml->AddExec();
			ptimer = xmlmp["ptimer"]->getvalue_int();
		}
		xml->AddExec(ptimer,xmlmp);
	}
	else if (event == "sleep")
	{
		int msec = xmlmp["msec"]->getvalue_int();
		int sec = xmlmp["sec"]->getvalue_int();
		int time = (sec * 1000 + msec) * 1000;
		log_i("$$HU$$ usleep %d\r\n", time);
		usleep(time);
	}
	else if (event == "xmlproc")
	{
		if (xmlmp.exist("done"))
		{
			int done = xmlmp["done"]->getvalue_int();
			if (done)
			{
				xml->DoneProc();
			}
			else
			{
				xml->UnDoneProc();
			}
		}
		if (xmlmp.exist("fore"))  //可以缓存后切前后台意义不大，直接用跳转
		{
			int fore = xmlmp["fore"]->getvalue_int();
			if (fore)
			{
				xml->ForeProc(NULL);
			}
			else
			{
				xml->UnForeProc();
			}
		}
//		log_i("%s done=%d fore=%d\r\n", xml->filename.c_str(), xml->done,
//				xml->fore);
	}
//	else if (event == "snap")
//	{
//		log_i("event = snap\r\n");
//		hustr file = xmlmp["file"]->getvalue();
//		int force = xmlmp["force"]->getvalue_int();
//		if (file.empty())
//		{
//			file.format("%s.png", xml->filename.c_str());
//			log_i("save default snap\r\n");
//		}
//		else
//		{
//			log_i("save snap to file %s\r\n", file.c_str());
//		}
//		if (force)
//		{
//			remove(file);
//		}
//		xml->ScheduleSaveSnap(file);
//	}
	else if (event == "cs")
	{
		hustr cont = xmlmp["cont"]->getvalue();
		if (cont == "stop")
		{
			log_i("stop\r\n");
			g_cur_xml->CS_manager::Stop();
		}
		else if (cont == "start")
		{
			log_i("start\r\n");
			g_cur_xml->CS_manager::Start();
		}
		else if (cont == "clear")
		{
			log_i("clear\r\n");
			//g_th_timer.create();
			g_cur_xml->m_cs.clear();
		}
		if (xmlmp.exist("post"))
		{
			hustr post = xmlmp["post"]->getvalue();
			if (xmlmp.exist("cus"))
			{
				g_xml_proc[xmlmp["cus"]->getvalue()]->PostCS(post);
			}
			else
			{
				g_cur_xml->PostCS(post);
			}

		}

	}
	else if (event == "timer")
	{
		hustr cont = xmlmp["cont"]->getvalue();
		if (cont == "stopall")
		{
			xml->StopAll();
		}
		else if (cont == "startall")
		{
			xml->StartAll();
		}
	}
	else if (event == "touch")
	{
		hustr cont = xmlmp["cont"]->getvalue();
		if (cont == "lockall")
		{
			xml->LockAll();
		}
		else if (cont == "unlockall")
		{
			xml->UnLockAll();
		}
	}
}

//XMLMap xmlmp;

void ParseInclude(hustr parentName,HUMap & xmlmp, xmlproc * xml)
{

	const char * filename = xmlmp["xmlfile"]->getvalue();
	if (xmlmp.exist("cus"))
	{
		hustr cus = xmlmp["cus"]->getvalue();
		log_i("include xmlfile=[%s] to %s\r\n",  xmlmp["xmlfile"]->getvalue(), cus.c_str());
		g_xml_proc[cus]->ParseXMLElementFile(filename);

	}
	else
	{
		log_i("include xmlfile=[%s] to %s\r\n", xmlmp["xmlfile"]->getvalue(), xml->filename.c_str());
		xml->ParseXMLElementFile(filename);
	}

}

void ParseCS(hustr parentName,HUMap & xmlmp, xmlproc * xml)
{
	xml->CreateCS(xmlmp["name"]->getvalue(), xmlmp);
}

void Parse_gcfg(hustr parentName,HUMap & xmlmp, xmlproc * xml)
{
	const char * name = xmlmp["name"]->getvalue();
	info info;
	element * ele = xml->GetElementByName(name);
	if (ele != NULL)
	{
		ele->GetInfo(info);
	}
	//log_i("snd msg:\r\n %s\r\n", info.nstr());
	g_th_msg.msg.send_message(101, info);
}


inline void RefreshPage( xmlproc * xml)
{

	element_manager::iterator it;
	element * ele ;
	xml->UnDoneProc();
	for (it = xml->begin(); it != xml->end(); ++it)
	{
		 ele = it->second;
		 //ele->Render();
		 ele->Flush();
		// ele->doFlushConfig();
		// xml->que.addele( ele);
	}
	xml->DoneProc();


}
//重新绘制页面所有元素
void ParseRefreshPage(hustr parentName,HUMap & xmlmp, xmlproc * xml)
{
	RefreshPage( xml);
}


//重新绘制页面所有元素
void ParseRefreshElement(hustr parentName,HUMap & xmlmp, xmlproc * xml)
{

	xml->UnDoneProc();

	for (int i = 0; i < xmlmp.count("element"); i++)
	{
		HUMap & mp = xmlmp["element"][i];
		const char * name = mp["name"]->getvalue();
		element * ele = xml->GetElementByName(name);
		 if(ele!=NULL)
		 ele->Flush();
	}
	xml->DoneProc();


}

void post_scfg(hustr parentName,HUMap & xmlmp, xmlproc * xml)
{
	if (xmlmp.exist("cus"))
	{
		hustr cus = xmlmp["cus"]->getvalue();
		g_xml_proc[cus]->PostScfg(xmlmp);
	}
	else
	{
		xml->PostScfg(xmlmp);
	}
}
////用于改变动态的参数
//void sendDynamicConfig(hustr parentName,HUMap & xmlmp, xmlproc * xml)
//{
//	if (xmlmp.exist("cus"))
//	{
//		hustr cus = xmlmp["cus"]->getvalue();
//		g_xml_proc[cus]->PostPartialConfig(xmlmp);
//	}
//	else
//	{
//		xml->PostPartialConfig(xmlmp);
//	}
//}


//批量发送scfg

void post_scfg_set(hustr parentName,HUMap & xmlmp, xmlproc * xml)
{
	xml->UnDoneProc();
	for (int i = 0; i < xmlmp.count("scfg"); i++)
	{
		HUMap & mp = xmlmp["scfg"][i];
		post_scfg(parentName,mp, xml);
	}
	xml->DoneProc();
}
//void post_sdcfg_set(hustr parentName,HUMap & xmlmp, xmlproc * xml)
//{
//	xml->UnDoneProc();
//	for (int i = 0; i < xmlmp.count("sdcfg"); i++)
//	{
//		HUMap & mp = xmlmp["sdcfg"][i];
//		sendDynamicConfig(parentName,mp, xml);
//	}
//	xml->DoneProc();
//}

//各种控制集合，如果单纯的scfg集合，建议使用scfgSet

void ParseSet(hustr parentName,HUMap & xmlmp, xmlproc * xml)
{
	xml->UnDoneProc();
	for (int i = 0; i < xmlmp.count("scfg"); i++)
	{
		HUMap & mp = xmlmp["scfg"][i];
		post_scfg(parentName,mp, xml);
	}
	for (int i = 0; i < xmlmp.count("include"); i++)
	{
		HUMap & mp = xmlmp["include"][i];
		ParseInclude(parentName,mp, xml);
	}
	for (int i = 0; i < xmlmp.count("control"); i++)
	{
		HUMap & mp = xmlmp["control"][i];
		ParseControl(parentName,mp, xml);
	}
	for (int i = 0; i < xmlmp.count("jump"); i++)
	{
		HUMap & mp = xmlmp["jump"][i];
		ParseJump(parentName,mp, xml);
	}
	for (int i = 0; i < xmlmp.count("widget"); i++)
	{
		HUMap & mp = xmlmp["widget"][i];
		ParseWidget(parentName,mp, xml);
	}
	for (int i = 0; i < xmlmp.count("cus"); i++)
	{
		HUMap & mp = xmlmp["cus"][i];
		ParseCUS(parentName,mp, xml);
	}
	for (int i = 0; i < xmlmp.count("savecus"); i++)
	{
		HUMap & mp = xmlmp["savecus"][i];
		ParseSAVECUS(parentName,mp, xml);
	}
	for (int i = 0; i < xmlmp.count("cs"); i++)
	{
		HUMap & mp = xmlmp["cs"][i];
		ParseCS(parentName,mp, xml);
	}
	for (int i = 0; i < xmlmp.count("env"); i++)
	{
		HUMap & mp = xmlmp["env"][i];
		ParseEnv(parentName,mp, xml);
	}
	for (int i = 0; i < xmlmp.count("gcfg"); i++)
	{
		HUMap & mp = xmlmp["gcfg"][i];
		Parse_gcfg(parentName,mp, xml);
	}
//	for (int i = 0; i < xmlmp.count("sdcfg"); i++)
//	{
//		HUMap & mp = xmlmp["sdcfg"][i];
//		sendDynamicConfig(parentName,mp, xml);
//	}
	for (int i = 0; i < xmlmp.count("reDrawElement"); i++)
	{
		HUMap & mp = xmlmp["reDrawElement"][i];
		ParseRefreshElement(parentName,mp, xml);
	}
	xml->DoneProc();
}


//void parseDirectDraw(hustr parentName,HUMap & xmlmp, xmlproc * xml)
//{
//	xml->directDraw = xmlmp["direct"]->getvalue_int();
//}
//HUTMap<XMLinstan_tf> XMLinstan;
void init_xml_instan()
{
	XMLinstan["include"] = ParseInclude;
	XMLinstan["control"] = ParseControl;
	XMLinstan["jump"] = ParseJump;
	XMLinstan["widget"] = ParseWidget;
	XMLinstan["cus"] = ParseCUS;
	XMLinstan["savecus"] = ParseSAVECUS;
	XMLinstan["scfg"] = post_scfg;
//	XMLinstan["sdcfg"] = sendDynamicConfig;
	XMLinstan["gcfg"] = Parse_gcfg;
	XMLinstan["cs"] = ParseCS;
	XMLinstan["env"] = ParseEnv;
	XMLinstan["scfgSet"] = post_scfg_set;
//	XMLinstan["sdcfgSet"] = post_sdcfg_set;
	XMLinstan["set"] = ParseSet;
	XMLinstan["reDrawPage"] = ParseRefreshPage;
	XMLinstan["reDrawElement"] = ParseRefreshElement;
//	XMLinstan["directDraw"] = parseDirectDraw;

}
//int ParseXMLElement2(hustr name, HUMap & xmlmp, xmlproc * xml)

void ParaseTinyXmlFile(const char * file, xmlproc * xml);

void hui_exit(const char * cmd)
{
	go = 0;

	g_th_timer.wait();
	log_i("wait g_th_timer OK\r\n");
	g_th_touch.wait();
	log_i("wait g_th_touch OK\r\n");
	g_th_msg.cancel();
	log_i("wait g_th_msg OK\r\n");

	log_i("cmd=%s\r\n", cmd);
	system(cmd);
}

//#include <sys/stat.h>
//#include <unistd.h>
//#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
//#include <string.h>
int image_write_to_snap(image * img, const char * rawpath);
void Dir(hustr dir, int l)
{
	DIR *dp;
	struct dirent *entry;
	struct stat statbuf;
	log_i("open %s\r\n", dir.c_str());
	if ((dp = opendir(dir)) == NULL)
	{
		fprintf(stderr, "cannot open directory: %s\n", dir.c_str());
		return;
	}
	while ((entry = readdir(dp)) != NULL)
	{

		if (strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0)
			continue;

		//printf("%*s%s/\n", depth, "", entry->d_name);

		hustr subfile = dir;
		subfile += "/";
		subfile += entry->d_name;
		lstat(subfile, &statbuf);
		if (S_ISDIR(statbuf.st_mode) && l > 0)
		{
			Dir(subfile, l - 1);
		}
		else
		{
			char * p = strrchr(entry->d_name, '.');
			if (p != NULL)
			{
				hustr type = p + 1;
				if (type == "png")
				{
					image tmp;
					hustr file("%s/%s", dir.c_str(), entry->d_name);
					tmp.SetResource(file);
					tmp.LoadResource();
					log_i("image_write_to_snap %s\r\n", entry->d_name);
					image_write_to_snap(&tmp, file);
				}
			}

		}

		//printf("%*s%s\n", depth, "", entry->d_name);

	}
	closedir(dp);
}

int main(int argc, char *argv[])
{
	log_i("%s\r\n", __TIME__);

	char * huipid = getenv("CURHUI");
	if (huipid != NULL)
	{
		int pid = strtoul(huipid, NULL, 10);
		log_i("kill %d\r\n", pid);
		kill(pid, 9);
	}
	int pid = getpid();
	setenv("CURHUI", hustr("%d", pid), 1);

	init_xml_instan();
	if (InitSystem())
	{
		errexitf("$$$HU$$$ InitSystem error %s", strerror(errno));
	}

	//debug["fps"] = 1;
	//fb.AcceptFrameBuffer();

	g_var.init_arg(argc, argv);

	int snaplevel = g_var.getvar_int("snapcache");
	if (snaplevel)
	{
		Dir(".", snaplevel - 1);
		return 0;
	}
	const char * xmlfile = g_var.getvar("xmlfile");
	if (xmlfile == NULL)
	{
		xmlfile = "start.xml";
	}
//	hustr snapfile("%s.png", xmlfile);
//	if (access(snapfile, F_OK) == 0)
//	{
//		fbf.BlackToSnap(snapfile);
//	}
#ifdef CONFIG_TOUCH_NONE

#else
	g_th_touch.init();
#endif
#ifdef CONFIG_KEYPAD_NONE

#else
	g_th_keypad.init();
#endif
	g_th_msg.create();
	g_th_timer.create();
	//fb.create();
//	g_cur_xml = new xmlproc(xmlfile);
//	//g_xml_proc[xmlfile] = g_cur_xml;
//	g_cur_xml->ParseXMLFile();
//	g_cur_xml->fore = 1;
	JumpToFile(xmlfile, hustr("%s.png", xmlfile));

	log_i("Press Ctrl-C to exit ...\n");
	g_th_timer.wait();
	log_i("wait g_th_timer OK\r\n");
#ifdef CONFIG_TOUCH_NONE

#else
	g_th_touch.wait();
#endif
#ifdef CONFIG_KEYPAD_NONE

#else
	g_th_keypad.wait();
#endif
	log_i("wait g_th_touch OK\r\n");
	g_th_msg.cancel();
	//fb.wait();
	log_i("wait g_th_msg OK\r\n");

	log_i("demo exit %d\r\n", go);

	return 0;
}
