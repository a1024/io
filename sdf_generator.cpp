#define _CRT_SECURE_NO_WARNINGS
#include<stdio.h>
#include<stdarg.h>

#include<ft2build.h>
#include FT_FREETYPE_H
#pragma comment(lib, "freetype.lib")

#include"lodepng.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include"stb_rect_pack.h"

int panic(int line, const char *format, ...)
{
	va_list args;
	if(format)
	{
		va_start(args, format);
		vprintf(format, args);
		va_end(args);
	}
	printf("\nEnter 0 to exit... ");
	int k=0;
	scanf("%d", &k);
	exit(1);
	return 0;
}
void pause()
{
	//if(format)
	//{
	//	va_start(args, format);
	//	vprintf(format, args);
	//	va_end(args);
	//}
	printf("Enter 0 to continue... ");
	int k=0;
	scanf("%d", &k);
}
#define		PANIC(FORMAT, ...)			panic(__LINE__, FORMAT, ##__VA_ARGS__)
#define		CHECK_FT(ERROR)				(!(ERROR)||panic(__LINE__, "FreeType error %d", ERROR))

#define		G_BUF_SIZE	1024
char		g_buf[G_BUF_SIZE]={};

struct		Glyph
{
	int sym, w, h;
	unsigned char *buffer;

	//in points (1/64th of a pixel)
	int m_w, m_h,//glyph dimensions
		m_hbx, m_hby,//horizontal bearing
		m_hadvance,//horizontal advance
		m_vadvance;//vertical advance
};
#define		GLYPH_OFFSET	33
#define		GLYPH_COUNT		94
Glyph		glyphs[GLYPH_COUNT]={};
struct		GlyphCoordInfo//8 bytes
{
	unsigned short x0, y0;
	unsigned char w, h;
	char ofx, ofy;
};
void		blit(unsigned char *dst, int dw, int dh, int x, int y, unsigned char *src, int sw, int sh)
{
	for(int ky=0;ky<sh;++ky)
	{
		if(y+ky>=0&&y+ky<dh)
		{
			for(int kx=0;kx<sw;++kx)
			{
				if(x+kx>=0&&x+kx<dw)
				{
					auto &s0=src[sw*ky+kx], &d0=dst[dw*(y+ky)+x+kx];
					if(d0<s0)
						d0=s0;
					//d0=~s0;
				}
			}
		}
	}
}
void		update_min(int &m, int x)
{
	if(m>x)
		m=x;
}
void		update_max(int &m, int x)
{
	if(m<x)
		m=x;
}
void		scan_bitmap(unsigned char *bmp, int bw, int bh, int bearx, int beary, int &xmin, int &xmax, int &ymin, int &ymax)
{
	int x1=0, x2=0, y1=0, y2=0;
	bool found=false;
	for(int ky=0;ky<bh&&!found;++ky)
	{
		for(int kx=0;kx<bw;++kx)
		{
			auto &p=bmp[bw*ky+kx];
			if(p>=128)
			{
				y1=ky;
				found=true;
				break;
			}
		}
	}
	found=false;
	for(int ky=bh-1;ky>=0&&!found;--ky)
	{
		for(int kx=0;kx<bw;++kx)
		{
			auto &p=bmp[bw*ky+kx];
			if(p>=128)
			{
				y2=ky+1;
				found=true;
				break;
			}
		}
	}
	found=false;
	for(int kx=0;kx<bw&&!found;++kx)
	{
		for(int ky=0;ky<bh;++ky)
		{
			auto &p=bmp[bw*ky+kx];
			if(p>=128)
			{
				x1=kx;
				found=true;
				break;
			}
		}
	}
	found=false;
	for(int kx=bw-1;kx>=0&&!found;--kx)
	{
		for(int ky=0;ky<bh;++ky)
		{
			auto &p=bmp[bw*ky+kx];
			if(p>=128)
			{
				x2=kx+1;
				found=true;
				break;
			}
		}
	}
	update_min(xmin, bearx+x1);
	update_max(xmax, bearx+x2);
	update_min(ymin, beary+y1);
	update_max(ymax, beary+y2);
}
double		estimate_slope(unsigned char *bmp, int argmax, int coeff)
{
	double slope=0;
	int count=0;
	for(int k=3;bmp[argmax+coeff*k];++k, ++count)
	{
		slope+=bmp[argmax+coeff*(k-1)]-bmp[argmax+coeff*k];
		printf("slope %lf (average of %d samples)\n", slope/(count+1), count+1);
	}
	printf("\n");
	slope/=count;
	return slope;
}
void		dec2frac(double x, double error, int *i, int *num, int *den)//https://stackoverflow.com/questions/5124743/algorithm-for-simplifying-decimal-to-fractions
{
	int lower_n, upper_n, middle_n,
		lower_d, upper_d, middle_d;
	int n=(int)floor(x);
	x-=n;
	if(x<error)
	{
		*i=n, *num=0, *den=1;
		return;
	}
	if(1-error<x)
	{
		*i=n+1, *num=0, *den=1;
		return;
	}
	lower_n=0, upper_n=1;
	lower_d=1, upper_d=1;
	for(;;)
	{
		middle_n=lower_n+upper_n;//The middle fraction is (lower_n + upper_n) / (lower_d + upper_d)
		middle_d=lower_d+upper_d;
		if(middle_d*(x+error)<middle_n)//If x + error < middle
			upper_n=middle_n, upper_d=middle_d;
		else if(middle_n<(x-error)*middle_d)//Else If middle < x - error
			lower_n=middle_n, lower_d=middle_d;
		else
			break;
	}
	*i=n, *num=middle_n, *den=middle_d;
}
struct		SDFTextureHeader
{
	double slope;
	char
		grid_start_x, grid_start_y,
		cell_size_x, cell_size_y,
		csize_x, csize_y,
		reserved[2];
	//unsigned short
	//	slope_i;
	//unsigned
	//	slope_num, slope_den;
};
int			main(int argc, char **argv)
{
	int font_size=64;//hadvance=35

	printf("Initializing FreeType...\n");
	FT_Library library=nullptr;
	FT_Face face=nullptr;
	int error=FT_Init_FreeType(&library);	CHECK_FT(error);
	error=FT_New_Face(library, "C:/Windows/Fonts/consola.ttf", 0, &face);	CHECK_FT(error);
	//error=FT_Set_Char_Size(face, 0, font_size*64, 300, 300);	CHECK_FT(error);
	error=FT_Set_Pixel_Sizes(face, 0, font_size);				CHECK_FT(error);
	
	printf("Rendering...\n");

	int iw=8*font_size, ih=16*font_size, imsize=iw*ih;
	auto bmp=new unsigned char[imsize];
	memset(bmp, 0, imsize);

	int advance=0, font_height=0;

	int max_ybearing=0, min_ybearing=0;//
	char c_max_ybearing=0, c_min_ybearing=0;
	int min_bx=1000, max_bx=-1000, min_by=1000, max_by=-1000;
	int min_cx=1000, max_cx=-1000, min_cy=1000, max_cy=-1000;

	int xoffset=0, yoffset=64, cell_height=81, cell_width=64;

	for(int k=33;k<127;++k)
	{
		unsigned glyph_index=FT_Get_Char_Index(face, k);
	//	unsigned glyph_index=FT_Get_Char_Index(face, "(|)"[k%3]);
		error=FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
		auto slot=face->glyph;
		error=FT_Render_Glyph(slot, FT_RENDER_MODE_SDF);

#if 1
		//update_min(min_ybearing, (slot->metrics.horiBearingY-slot->metrics.height)>>6);
		//update_max(max_ybearing, (slot->metrics.horiBearingY-slot->metrics.height)>>6);
		if(max_ybearing<(slot->metrics.horiBearingY>>6))
			max_ybearing=slot->metrics.horiBearingY>>6, c_max_ybearing=k;
		if(min_ybearing>((slot->metrics.horiBearingY-slot->metrics.height)>>6))
			min_ybearing=(slot->metrics.horiBearingY-slot->metrics.height)>>6, c_min_ybearing=k;
		advance=slot->metrics.horiAdvance>>6;
		font_height=slot->metrics.vertAdvance>>6;

		//int x=(iw>>1)+(slot->metrics.horiBearingX>>6),//
		//	y=(ih>>1)-(slot->metrics.horiBearingY>>6);//
		int x=(k&7)*font_size+xoffset+(slot->metrics.horiBearingX>>6),
			y=((k>>3)-4)*cell_height+yoffset-(slot->metrics.horiBearingY>>6);	//(1024-12*80)/2 = 32
		//int x=(k&7)*font_size+(slot->metrics.horiBearingX>>6),
		//	y=ih-1-(ih/font_size-1-(k>>3))*80-(slot->metrics.horiBearingY>>6)-20;
		
		update_min(min_bx, slot->metrics.horiBearingX>>6);
		update_max(max_bx, (slot->metrics.horiBearingX>>6)+slot->bitmap.width);
		update_min(min_by, -(slot->metrics.horiBearingY>>6));
		update_max(max_by, -(slot->metrics.horiBearingY>>6)+slot->bitmap.rows);
		scan_bitmap(slot->bitmap.buffer, slot->bitmap.width, slot->bitmap.rows, slot->metrics.horiBearingX>>6, -(slot->metrics.horiBearingY>>6), min_cx, max_cx, min_cy, max_cy);

		blit(bmp, iw, ih, x, y, slot->bitmap.buffer, slot->bitmap.width, slot->bitmap.rows);
#endif
#if 0
		auto &g=glyphs[k-33];
		g.sym=k;
		g.w=slot->bitmap.width;
		g.h=slot->bitmap.rows;
		int size=g.w*g.h;
		g.buffer=new unsigned char[size];
		memcpy(g.buffer, slot->bitmap.buffer, size);
		g.m_w=slot->metrics.width;
		g.m_h=slot->metrics.height;
		g.m_hbx=slot->metrics.horiBearingX;
		g.m_hby=slot->metrics.horiBearingY;
		g.m_hadvance=slot->metrics.horiAdvance;
		g.m_vadvance=slot->metrics.vertAdvance;
#endif

		//sprintf_s(g_buf, G_BUF_SIZE, "result_%03d.PNG", k);
		//lodepng::encode(g_buf, slot->bitmap.buffer, slot->bitmap.width, slot->bitmap.rows, LCT_GREY, 8);

		//int px=(k&7)*font_size, py=(k>>3)*font_size;
		//for(int ky=0, ks=0;ky<font_size;++ky)
		//	for(int kx=0;kx<font_size;++kx, ++ks)
		//		bmp[iw*(py+ky)+px+kx]=slot->bitmap.buffer[ks];
	}
	int argmax=0;
	int k=1;
	for(;k<imsize;++k)
		if(bmp[argmax]<bmp[k])
			argmax=k;
	int amx=argmax%iw, amy=argmax/iw;
	double slope=0;
	slope+=estimate_slope(bmp, argmax, 1);		//!!! this assumes that argmax is at a period which is a circular hill
	slope+=estimate_slope(bmp, argmax, -1);		//    therefore going away from center in the 4 directions calculates the SDF slope
	slope+=estimate_slope(bmp, argmax, iw);
	slope+=estimate_slope(bmp, argmax, -iw);
	slope*=0.25/255;
/*	float slope=0;
	int count=0;
	for(int k=3;bmp[argmax+k];++k, ++count)
	{
		slope+=bmp[argmax+k-1]-bmp[argmax+k];
		if(count)
			printf("slope %lf (average of %d samples)\n", slope/(count+1), count+1);
	}
	slope/=count;
	printf("slope %lf (average of %d samples)\n", slope, count);

	slope=0;
	count=0;
	for(int k=3;bmp[argmax+iw*k];++k, ++count)
	{
		slope+=bmp[argmax+iw*(k-1)]-bmp[argmax+iw*k];
		if(count)
			printf("slope %lf (average of %d samples)\n", slope/(count+1), count+1);
	}
	slope/=count;
	printf("slope %lf (average of %d samples)\n", slope, count);//*/
	//for(int k=0;k<imsize;++k)//
	//	bmp[k]=255*(bmp[k]>=128);//
	printf("bitmaps  X %d~%d\tY %d~%d\t%dx%d\n", min_bx, max_bx, min_by, max_by, max_bx-min_bx, max_by-min_by);
	printf("content  X %d~%d\tY %d~%d\t%dx%d\n", min_cx, max_cx, min_cy, max_cy, max_cx-min_cx, max_cy-min_cy);
	printf("Estimated SDF slope: %lf\n", slope);
	//int slope_i, slope_num, slope_den;
	//dec2frac(slope, 1e-10, &slope_i, &slope_num, &slope_den);//float gives 7 digits, double gives 15
	SDFTextureHeader header=
	{
		slope,							//SDF slope
		xoffset+min_cx, yoffset+min_cy,	//grid start
		cell_width, cell_height,		//cell size
		max_cx-min_cx, max_cy-min_cy,	//character size
		{0, 0},
	};
	memcpy(bmp, &header, sizeof(SDFTextureHeader));
	//bmp[0]=xoffset+min_cx;//grid start
	//bmp[1]=yoffset+min_cy;
	//bmp[2]=cell_width;//cell size
	//bmp[3]=cell_height;
	//bmp[4]=max_cx-min_cx;//character size
	//bmp[5]=max_cy-min_cy;
	
#if 0
	printf("Packing...\n");
	int iw=512, ih=512;
	stbrp_context packctx={};

#define NODE_COUNT	1024
	auto packtemp=new stbrp_node[NODE_COUNT];
	stbrp_init_target(&packctx, iw, ih, packtemp, NODE_COUNT);

	auto packrects=new stbrp_rect[GLYPH_COUNT];
	memset(packrects, 0, GLYPH_COUNT*sizeof(stbrp_rect));
	for(int k=0;k<GLYPH_COUNT;++k)//init
	{
		packrects[k].id=GLYPH_OFFSET+k;
		packrects[k].w=glyphs[k].w;
		packrects[k].h=glyphs[k].h;
		packrects[k].x=0;
		packrects[k].y=0;
		packrects[k].was_packed=0;
	}
	stbrp_pack_rects(&packctx, packrects, GLYPH_COUNT);
	delete[] packtemp;
	
	printf("Printing...\n");
	int imsize=iw*ih;
	auto bmp=new unsigned char[imsize];
	memset(bmp, 0, imsize);
	int yoffset=4;//max number of info rows in texture
	int p=0;
	GlyphCoordInfo info;
	info.x0=iw-2;
	info.y0=ih-2;
	info.w=glyphs->m_hadvance>>6;
	info.h=glyphs->m_vadvance>>6;
	info.ofx=0;
	info.ofy=0;
	memcpy(bmp+p, &info, sizeof(info));
	p+=sizeof(info);
	for(int k=0;k<GLYPH_COUNT;++k, p+=sizeof(info))
	{
		auto &r=packrects[k];
		auto &g=glyphs[k];
		for(int ky=0;ky<r.h;++ky)
			memcpy(bmp+iw*(r.y+yoffset+ky)+r.x, g.buffer+r.w*ky, r.w);

		info.x0=r.x;
		info.y0=r.y+yoffset;
		info.w=r.w;
		info.h=r.h;
		info.ofx=g.m_hbx>>6;
		info.ofy=g.m_hby>>6;
		printf("%3d %c pos %3d %3d  dim %3d %3d  off %3g %3g\n", k+GLYPH_OFFSET, (char)(k+GLYPH_OFFSET), info.x0, info.y0, info.w, info.h, g.m_hbx/64., g.m_hby/64.);
		if(g.m_hbx&63||g.m_hby&63)
			PANIC("Need more precision at glyph %d = %c\n", k+GLYPH_OFFSET, (char)(k+GLYPH_OFFSET));
		memcpy(bmp+p, &info, sizeof(info));
	}
	delete[] packrects;
#endif
	
	printf("Saving...\n");
	lodepng::encode("font_sdf.PNG", bmp, iw, ih, LCT_GREY, 8);
	delete[] bmp;

	printf("Done.\n");
	pause();
	return 0;
}