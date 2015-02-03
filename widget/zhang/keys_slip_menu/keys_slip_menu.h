#ifndef __KEYS_SLIP_MENU_H__
#define __KEYS_SLIP_MENU_H__

#include "XMLInstal.h"
#include "layer.h"
#include "thread_msg.h"
class keys_slip_menu: public element,  public timer_element
{
public:

int hu_abs(int number) 
{               
        return (number >= 0 ? number : -number);                     
}   

	 class dm_image : public image
       	 {
       	 public:
                dm_image()
                {
                        dx = 0;
                        dy = 0;
                }
                HuExec  exec;
                int dx;
                int dy;
        };

	class node: public element
	{
	public:
        int cx;
		node()
		{ 
			cx=0;
		}
		void doFlushConfig()
		{
                name = m_mp["name"]->getvalue();
                x = m_mp["x"]->getvalue_int();
                y = m_mp["y"]->getvalue_int();

                x=x+this->parent->x;
                y=y+this->parent->y;
                width = m_mp["width"]->getvalue_int();
                height = m_mp["height"]->getvalue_int();

                if (m_mp.exist("lay"))
                {
                        lay = m_mp["lay"]->getvalue_int();
                }
                else    
                {               
                        lay = 5;
                }       

                if (isNULL())
                {
                        //printf("%s SetBuffer width=%d height=%d\r\n", name.c_str(), width, height);
                        SetBuffer(width, height);
                        path.format("ele-%s %dx%d", name.c_str(), width, height);
                }
		int temp_x=x;
		x=x%this->parent->page_w; //取第一页位置
                initstack();
		x=temp_x;//还原x
			//exec.parse(m_mp);
			//exec.name = parent->name;

			Flush();
		}

		void doRender()
		{
              //  printf("this->x=%d,this->cx=%d\n",x,cx);
               //
		      image::Render(&this->parent->img[1], x+this->parent->cx-


		    		  this->parent->x, y-this->parent->y,width,height,0,0);

		      //  image::Render(&img[0],               cx ,                                0,    (int)page_w, (int)height, 0,0);
		}

		keys_slip_menu * parent;
	};

	int doTimer(int tm)
	{
	//	printf("slip_menu::OnTimer %dms cx=%d dx=%d mx=%d \r\n", tm,cx,dx,mx);
		for (int i = 0; i < rcn && cx > dx; i++)
			if (cx > dx){ //当前x大于目的x，则减小当前x(cx),包括翻页和回弹(自动向左运动)page-

				// nodemp[select_id]->cx++;
				cx--;
			}
		for (int i = 0; i < rcn && cx < dx; i++)//(自动向右运动) page+
			if (cx < dx){

				cx++;
			}
		Flush();
		if (dx == cx)
		{
			TimerStop();
			if(isFlip)
			nodemp[select_id]->Flush();
		}
		else
		{
			TimerSet(tm + rsp);
		}

	}


	void changePage(){
		printf("page_node_num=%d\r\n", page_node_num);
		printf("page=%d\r\n", page);
		if (select_id>=page_node_num*(page+1))//右移，页面+1
		{
			isFlip=true;
						if (page  < const_page)
						{
							page++;
									dx=page_w*page;

									for (int i = 0; i < node_num; i++)
									{
											 nodemp[i]->cx-=page_w;
								//nodemp[i]->touch_init_area(nodemp[i]->x,nodemp[i]->y, nodemp[i]->width, nodemp[i]->height);
									 }

						}
						printf("++OK\r\n", page);
						TimerSet(0);
						xml_mgr->PostCS(hustr("page%d", page + 1));
					//	Flush();
					}

		else if(page>0){
		 if(select_id<page_node_num*(page))//左移，页面-1
			{
			 isFlip=true;
				printf("--\r\n");

				page--;
				dx=page_w*page;

						 for (int i = 0; i < node_num; i++)
						 {
								 nodemp[i]->cx+=page_w;
					//nodemp[i]->touch_init_area(nodemp[i]->x,nodemp[i]->y, nodemp[i]->width, nodemp[i]->height);
						 }

					 TimerSet(0);
					xml_mgr->PostCS(hustr("page%d", page + 1));
					//Flush();

			}
		 else{
			 isFlip=false;
		 }
		}
		else{
			isFlip=false;
		}


	}



	void doGetInfo(info & info)
		{
			GetElementInfo(info);

			info.AddInfo("select", select_id);
			printf("in doGetInfo() select_id=%d\r\n", select_id);
		//	changePage();

		}

	void doRender()
	{
		image::Render(&img[0], cx , 0, (int)page_w, (int)height, 0, 0);
		if(!isFlip)
		nodemp[select_id]->Flush();//都使用第一页的节点显示
	}
	void doFlushConfig()
	{
		int i,j; 
		PraseElement();

		if (m_mp.exist("page"))
		const_page = m_mp["page"]->getvalue_int()-1;//0代表1页，2代表3页,在xml中1代表1页
                rsp = m_mp["rsp"]->getvalue_int();
                rcn = m_mp["rcn"]->getvalue_int();
		remax = m_mp["remax"]->getvalue_int();
		remin = m_mp["remin"]->getvalue_int();

		select_id=m_mp["select"]->getvalue_int();

		sum_w=width;
		width/=const_page+1;//触摸及可视宽度等于图片宽度除以页数
		page_w=width;

		//node_name="pic_node";
		name = m_mp["name"]->getvalue();
		if (m_mp.exist("small_pic"))
		{
			use_small=m_mp["small_pic"]->getvalue_int();
		}
		for(j=0;j<2;j++)
       		 {
		if(j==0)
		{
		pic_sc="up";
		}
		else
		{
		pic_sc="dn";
		}
		if (img[j].isNULL())
	   	  {

		   if(use_small==0)//使用大图
		   {
			img[j].SetBuffer(sum_w, height);
                            
			img[j].SetResource(m_mp[pic_sc.c_str()]->getvalue());//设置全局图片
			img[j].LoadResource();//加载按下前的图片，此图片可以替代背景图片
		   }
	       	  else //使用小图片
	      	   {
			printf("use small pic!!!!!!!!!!!!!!!!!!!\n");
			hustr filename("%s_%d.png", name.c_str(),j);
			if (access_Image(filename))
			{
				img[j].SetResource(filename);
				img[j].LoadResource();
			}
			else
			{

				map<int, dm_image> tmp;
				for (int i = 0; i < m_mp.count("node"); i++)
				{
					HUMap & mp = m_mp["node"][i];
					tmp[i].dx=mp["x"]->getvalue_int();	
					tmp[i].dy=mp["y"]->getvalue_int();
					tmp[i].SetResource(mp[pic_sc.c_str()]->getvalue());
					tmp[i].LoadResource();
				}
				img[j].path.format("slip_menu-%s output", name.c_str());
				img[j].SetBuffer(sum_w, height);

				for (int i = 0; i < tmp.size(); i++)
				{
					img[j].Render(&tmp[i], tmp[i].dx,tmp[i].dy);
				}
				img[j].SaveResource(filename);
			}
		   }
		  }
	       	 }
		int touch_lock = m_mp["lock"]->getvalue_int();
                node_num=m_mp.count("node");
		for (int i = 0; i < node_num; i++)
		{
			if (nodemp[i] == NULL)
			{
				nodemp[i] = new node;
				nodemp[i]->m_mp.fetch(m_mp["node"][i]);
				nodemp[i]->parent = this;
				nodemp[i]->xml_mgr = xml_mgr;
				nodemp[i]->mgr = mgr;
			}

			if (m_mp.exist("page_node_num"))
			        page_node_num=m_mp["page_node_num"]->getvalue_int();
			else
				page_node_num=node_num/(const_page+1);
			nodemp[i]->FlushConfig();


		//	nodemp[i]->exec.name = name;
		}
	//	touch_init_area(x, y, page_w, height);

	//	xml_mgr->AddEleArea( this);

		xml_mgr->AddTimerElement( this);
		changePage();
		nodemp[select_id]->x+=nodemp[select_id]->cx;
		Flush();

	}
	keys_slip_menu()
	{
		pic_sc=NULL;
		isFlip=false;
		node_name=NULL;
		sum_w = 0;
		sum_h = 0;
		page_w = 0;
		index = 0;
		cx = 0; //当前x
		//mx = 0; //拖动轴mx
		dx = 0; //时间轴x
		page = 0;
		 rsp=0;//返回定时器频率
		rcn=0;///返回速度
        const_page=0;
		page_max = 0;
		remax = 0;
		node_num=0;
		remin = 0;
		use_small=0;
	    select_id=0;
	    page_node_num=0;
	//	timer_element::mutex.SetMutex(this);
	}
	~keys_slip_menu()
	{
		//printf("~drag_menu\r\n");
	//	imgs.clear();
	}
//	map<int, dm_image> imgs;

	int sum_w;//menu的宽度
	int sum_h; 
	int page_w;//一页的宽度
	int index;
	//int op;
	int select_id;  //哪张图片选中
	bool isFlip;
	int use_small;
	hustr node_name;
	hustr pic_sc;
	int cx; //图像当前x，绝对值
	//int mx; //拖动轴mx
	int dx; //时间轴x,目标x，跟page宽度对齐
	int page;
	int const_page;
	int page_max;
	int remax;
	int remin;
	int rsp;//返回定时器频率
	int rcn;///返回速度
	int page_node_num;
	map<int, node *> nodemp;
	image img[2];
        int node_num;
	image output;

};







#endif //__STATIC_IMAGE_H__
