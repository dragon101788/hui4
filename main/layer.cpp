#include "layer.h"
#include "xmlproc.h"
#include "ttf_font.h"
#include "include/autoconf.h"

void element::Flush()
{
	if (mgr != NULL)
	{
		lock();
		//resetRenderOffset();//父控件主动绘制时恢复整个控件输出
		mgr->que.addele(this);
		unlock();
	}
	else
	{
		errexitf("$$$$HU$$$$ Flush element %s manager is NULL\r\n",
				name.c_str());
	}
}

void element::revocation()
{
	if (mgr != NULL)
	{
		lock();
		mgr->que.delele(this);
		unlock();
	}
	else
	{
		errexitf("$$$$HU$$$$ revocation element %s manager is NULL\r\n",
				name.c_str());
	}
}

int crossAlgorithm(element * r1, element * r2)
{
	int nMaxLeft = 0;
	int nMaxTop = 0;
	int nMinRight = 0;
	int nMinBottom = 0;

	//计算两矩形可能的相交矩形的边界
	nMaxLeft = r1->x >= r2->x ? r1->x : r2->x;
	nMaxTop = r1->y >= r2->y ? r1->y : r2->y;
	nMinRight =
			(r1->x + r1->width) <= (r2->x + r2->width) ?
					(r1->x + r1->width) : (r2->x + r2->width);
	nMinBottom =
			(r1->y + r1->height) <= (r2->y + r2->height) ?
					(r1->y + r1->height) : (r2->y + r2->height);
	// 判断是否相交
	//log_i("crossAlgorithm %d %d %d %d\r\n",nMaxLeft , nMinRight , nMaxTop , nMinBottom);
	if (nMaxLeft >= nMinRight || nMaxTop >= nMinBottom)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}


void element::initstack()
{

	element_manager::iterator it;
//	element_manager *ele_mgr;
//	if(hasParent()){
//		ele_mgr=parent;
//
//	}
//	else
//		ele_mgr=xml_mgr;//有一个问题：非子元素会将子元素考虑进来
	for (it = xml_mgr->begin(); it != xml_mgr->end(); ++it)
	{
		element * ele = it->second;
		if(ele->parent==parent){  //只有当在同一父亲下，或父亲都为0，才相互作用
			if (crossAlgorithm(ele, this))
			{
				if(ele->lay != lay){
					addLayers(ele);   //区域内有重叠元素，并且层小于自己则加入底队列
					ele->addLayers(this); //将自己加入顶队列
				}
			}
		}
	}
	addLayers(this);  //自己也要加在自己的队列中

}

void element::RenderOut()
{
	if(isDraw!=1){
		render_offset_x=0;
		render_offset_y=0;
		render_width=width;
		render_height=height;
	}
	isDraw=0;


	if (hide == 0)
	{
		doRender();
	}
	else
	{
		log_i("Render %s hide\r\n", name.c_str());
	}
	renderLayers();  //如果自己隐藏的话，此函数是不会绘制自己的。

	if(parent!=NULL){
		//log_i("%s draw to parent!!!!!!!\n",name.c_str());
		if(!parent->isParent()){
			parent->tobeParent(name,this);
		}
		parent->Draw(this, render_offset_x, render_offset_y, render_width, render_height, x+render_offset_x, y+render_offset_y);//控件输出到父控件
		parent->RenderOut();

	}else{
		//log_i("%s draw to xmlout!!!!!!!\n",name.c_str());
		if(xml_mgr->directDraw){ //一级父容器直接输出到fb
			xml_mgr->drawDirect(this, render_offset_x, render_offset_y, render_width, render_height, x+render_offset_x, y+render_offset_y);
		}else
		xml_mgr->Draw(this, render_offset_x, render_offset_y, render_width, render_height, x+render_offset_x, y+render_offset_y);//控件局部输出到容器
	}

}



void element::cleanLastPos()
{
	lock();

	renderLayers();  //如果自己隐藏的话，此函数是不会绘制自己的。
	if(parent!=NULL){
		log_i("%s draw to parent!!!!!!!\n",name.c_str());
		if(!parent->isParent()){
			parent->tobeParent(name,this);
		}
		parent->Draw(this,  0, 0, width, height, x, y);//控件输出到父控件
		parent->RenderOut();

	}else{
		if(xml_mgr->directDraw){ //一级父容器直接输出到fb
			xml_mgr->drawDirect(this,  0, 0, width, height, x, y);
		}else
		xml_mgr->Draw(this,  0, 0, width, height, x, y);//控件局部输出到容器
	}
	unlock();

}
//void element::Back()
//{
//	lock();
//	RenderEB();
//	if(parent!=NULL){
//		if(!parent->isParent()){
//			parent->tobeParent(name,this);
//		}
//		parent->Draw(this, render_offset_x, render_offset_y, render_width, render_height, x+render_offset_x, y+render_offset_y);//控件输出到父控件
//		parent->Flush_for_Child();
//
//	}else
//	xml_mgr->Draw(this, render_offset_x, render_offset_y, render_width, render_height, x+render_offset_x, y+render_offset_y);//控件局部输出到容器
//
//	RenderET();
//	unlock();
//
//}
void element::FlushConfig()
{
	lock();
	doFlushConfig();
	xml_mgr->AddElement(name, this);
	unlock();
}

void element::FlushConfigReduced()
{
	lock();
	doFlushConfigReduced();
	xml_mgr->AddElement(name, this);
	unlock();
}
void element::Delete()
{
	doDelete();
	if(isParent()){
		delChildren();
	}
	revocation();
	hide = 1;
	RenderOut();
	//ResetLayers();
	//debug("Delete after ResetLayers !!\n");
	if(hasParent()){
		parent->elem.erase(name);
	}
	xml_mgr->elem.erase(name);
}


void element::DeleteByParent()
{
	doDelete();
	if(isParent()){
		delChildren();
	}
	revocation();
	xml_mgr->elem.erase(name);
}
/*
 * 在加载图片及文字，显示在同一层
 */
void element::ParseModifRes()
{
	for (int i = 0; i < m_mp.count("res"); ++i)
	{
		HUMap  xmlmp = m_mp["res"][i];
		hustr meth = xmlmp["meth"]->getvalue();
		int id = xmlmp["id"]->getvalue_int();
		int src_x = xmlmp["src_x"]->getvalue_int();
		int src_y = xmlmp["src_y"]->getvalue_int();
		int cp_width = xmlmp["cp_width"]->getvalue_int();
		int cp_height = xmlmp["cp_height"]->getvalue_int();
		int dst_x = xmlmp["dst_x"]->getvalue_int();
		int dst_y = xmlmp["dst_y"]->getvalue_int();

		res[id].LoadResource();
		if (cp_width + cp_height == 0)
		{
			cp_width = res[id].GetWidth();
			cp_height = res[id].GetHeight();
		}
		if (meth == "render")
		{
			image file;
			file.SetResource(xmlmp["file"]->getvalue());
			file.LoadResource();

			log_i(
					"$$$HU$$$ XML Draw Render %s to %s res=%d %d %d %d %d %d %d\r\n",
					file.path.c_str(), name.c_str(), id, src_x, src_y, cp_width,
					cp_height, dst_x, dst_y);

			if (!res[id].isNULL())
			{

				//ele->image::Render(&file, src_x, src_y, cp_width, cp_height, dst_x, dst_y);
				lock();

				res[id].Render(&file, src_x, src_y, cp_width, cp_height, dst_x,
						dst_y);
				//ele->Render();
				unlock();

			}
			else
			{
				errexitf("%s res %d no init\r\n", name.c_str(), id);
			}
		}
		else if (meth == "text")
		{
#ifdef CONFIG_USING_FONT
			int red = xmlmp["red"]->getvalue_int();
			int green = xmlmp["green"]->getvalue_int();
			int blue = xmlmp["blue"]->getvalue_int();
			int color = (red & 0xff) << 16 | (green & 0xff) << 8 | blue & 0xff;
			//bkcolor = m_mp["bkcolor"]->getvalue_int();
			hustr font = xmlmp["font"]->getvalue();
			hustr txt = xmlmp["txt"]->getvalue();
			int style = (unsigned char) m_mp["style"]->getvalue_int();
			int size = xmlmp["size"]->getvalue_int();


			text tmpttf;
			tmpttf.m_font = &font_mp[font];
			log_i("get font_mp %s %x %x\r\n",font.nstr() ,tmpttf.m_font->face,
					tmpttf.m_font->ft_Lib);
			tmpttf.fontHeight = size;
			tmpttf.color = color;
			tmpttf.style = style;
			tmpttf.SetBuffer(cp_width, cp_height);
			tmpttf.DrawText("UTF-8", (char *) txt.c_str(), txt.length());

			log_i("ParseModifRes text=%s\r\n",txt.c_str());
			if (!res[id].isNULL())
			{

				//ele->image::Render(&file, src_x, src_y, cp_width, cp_height, dst_x, dst_y);
				lock();

				res[id].Render(&tmpttf, src_x, src_y, cp_width, cp_height,
						dst_x, dst_y);
				//ele->Render();
				unlock();

			}
#endif
		}

	}
	m_mp.erase("res");
}


 void element_manager::DelElement(const char * name)
{
	element * ele = GetElementByName(name);
	if (ele == NULL)
	{
		log_w("can't del layer element %s\r\n", name);
	}
	else
	{
		ele->Delete();
	//	elem.erase(name);
	}
}
 void element_manager_for_ele::DelElement(const char * name)
{
	element * ele = GetElementByName(name);
	if (ele == NULL)
	{
		log_w("can't del layer element %s\r\n", name);
	}
	else
	{
		ele->Delete();
	//	elem.erase(name);
	}
}



 void ele_nest_extend::configChildAbsPos(){  //当父控件的scroll_x改变时，子控件的绝对位置就会改变，父控件需要调用此函数
	 iterator it;
	 element * ele;
		for (it = elem.begin(); it != elem.end(); ++it)
		{
			ele=it->second;
			ele->abs_x=ele->x+abs_x-scroll_x;
			ele->abs_y=ele->y+abs_y-scroll_y;
			ele->onAbsPosChanged();
		}

 }
 void ele_nest_extend::delChildren(){
	iterator it;

	for (it = elem.begin(); it != elem.end(); ++it)
	{
		element *ele=it->second;
		if(ele!=NULL)
		ele->DeleteByParent();
	}
 }
	void ele_nest_extend::resetChildren()
	{
		iterator it;
		element * ele;
		for(it=elem.begin();it!=elem.end();++it){
			ele=it->second;
			if(ele!=NULL){
				ele->parent=NULL;
			}
		}
	}