#ifndef __ROTATE_IMAGE_H__
#define __ROTATE_IMAGE_H__

#include "XMLInstal.h"
#include "layer.h"

class rotate_image: public element,public  timer_element
{
public:
	rotate_image()
	{
//		xpos = 0;
//		ypos = 0;
		id = 0;
		rsp=0;
		stepAngle=2;
	}
	~rotate_image()
	{

	}

	void doDelete()
	{

	}
	int doTimer(int tm)
	{

		if (0)
		{
			TimerStop();
		}
		else
		{
			TimerSet(tm + rsp);
		}
		Flush();
		angle=(angle+stepAngle)%360;
	}



	void doGetInfo(info & info)
	{
		GetElementInfo(info);
		info.AddInfo("id", id);
		info.AddInfo("scroll_x", scroll_x);
		info.AddInfo("scroll_y", scroll_y);
	}
	void doFlushConfig()
	{
		PraseElement();
		id = m_mp["id"]->getvalue_int();
		scroll_x = m_mp["scroll_x"]->getvalue_int();
		scroll_y = m_mp["scroll_y"]->getvalue_int();
		angle=m_mp["angle"]->getvalue_int();
		rsp = m_mp["timeSlot"]->getvalue_int();
		stepAngle = m_mp["stepAngle"]->getvalue_int();
		for (int i = 0; i < m_mp.count("node"); i++)
		{
			printf("doFlushConfig %d %s\r\n",i, m_mp["node"][i]->getvalue());
			SetRes(i, m_mp["node"][i]->getvalue());
		}
		ParseModifRes();
		xml_mgr->AddTimerElement( this);
		if(!m_mp["cached"]->getvalue_int()){//如果cached=1标志就不绘制，等待下一次动态参数调整后统一绘制
			Flush();
		}
		TimerSet(0);
	}
	void doFlushConfigReduced()
	{
		hide = m_mp["hide"]->getvalue_int();
		scroll_x = m_mp["scroll_x"]->getvalue_int();
		scroll_y = m_mp["scroll_y"]->getvalue_int();
		id = m_mp["id"]->getvalue_int();

		Flush();
	}


	void doRender()
	{
		//image::Render(&res[id], xpos, ypos, width, height, 0, 0);
		res[id].LoadResource();
		//rotate(&res[id],&rotated,angle);
		rotate( rotated, res[id],angle,1,1,0,0);
		cur_res=&rotated;
		//scroll_x=tan(angle*3.1415/180)*height;
		//scroll_y=tan(angle*3.1415/180)*width;
	}
//	int xpos;
//	int ypos;
	image rotated;
	int angle;
	int id;
	int rsp;
	int stepAngle;
};

#endif //__STATIC_IMAGE_H__
