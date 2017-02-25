#include "Platform/PlatformInfo.h"
#include "Font/FontLoading.h"

#if(INTRA_LIBRARY_FONT_LOADING==INTRA_LIBRARY_FONT_LOADING_STB)

INTRA_PUSH_DISABLE_ALL_WARNINGS

#include "Platform/FundamentalTypes.h"
#include "Math/Vector.h"
#include "Memory/Allocator/Global.h"
#include "Container/Sequential/Array.h"
#include "Algo/Sort/Insertion.h"

using Intra::null;
using Intra::sbyte;
using Intra::byte;
using Intra::ushort;
using Intra::uint;
using Intra::Math::SVec2;
using Intra::Math::Vec2;

using Intra::Math::Sqrt;
using Intra::Math::Ceil;
using Intra::Memory::CopyBits;
using Intra::Memory::GlobalHeap;
using Intra::Container::Array;
using Intra::Algo::ShellSort;

extern "C" {

struct stbtt_fontinfo
{
   void* userdata;
   byte* data;            // pointer to .ttf file
   int fontstart;         // offset of start of font

   int numGlyphs;                     // number of glyphs, needed for range checking

   int loca, head, glyf, hhea, hmtx, kern; // table locations as offset from start of .ttf
   int index_map;                     // a cmap mapping for our chosen character encoding
   int indexToLocFormat;              // format needed to map from glyph index to glyph
};


//////////////////////////////////////////////////////////////////////////////
//
// GLYPH SHAPES (you probably don't need these, but they have to go before
// the bitmaps for C declaration-order reasons)
//

#ifndef STBTT_vmove // you can predefine these to use different values (but why?)
   enum {
      STBTT_vmove=1,
      STBTT_vline,
      STBTT_vcurve
   };
#endif

#ifndef stbtt_vertex // you can predefine this to use different values
                   // (we share this with other code at RAD)
#define stbtt_vertex_type short // can't use short because that's not visible in the header file
	struct stbtt_vertex
	{
		SVec2 pos, cpos;
		byte type;
		byte _padding;
	};
#endif


// @TODO: don't expose this structure
struct stbtt__bitmap
{
	SVec2 size;
	int stride;
	byte* pixels;
};


const ushort STBTT_MACSTYLE_DONTCARE=0, STBTT_MACSTYLE_BOLD=1, STBTT_MACSTYLE_ITALIC=2, STBTT_MACSTYLE_UNDERSCORE=4, STBTT_MACSTYLE_NONE=8;

enum {STBTT_PLATFORM_ID_UNICODE, STBTT_PLATFORM_ID_MAC, STBTT_PLATFORM_ID_ISO, STBTT_PLATFORM_ID_MICROSOFT};

enum {STBTT_UNICODE_EID_UNICODE_1_0, STBTT_UNICODE_EID_UNICODE_1_1,
	STBTT_UNICODE_EID_ISO_10646, STBTT_UNICODE_EID_UNICODE_2_0_BMP, STBTT_UNICODE_EID_UNICODE_2_0_FULL};

enum {STBTT_MS_EID_SYMBOL=0, STBTT_MS_EID_UNICODE_BMP=1, STBTT_MS_EID_SHIFTJIS=2, STBTT_MS_EID_UNICODE_FULL=10};

enum {STBTT_MAC_EID_ROMAN, STBTT_MAC_EID_JAPANESE, STBTT_MAC_EID_CHINESE_TRAD,
	STBTT_MAC_EID_KOREAN, STBTT_MAC_EID_ARABIC, STBTT_MAC_EID_HEBREW, STBTT_MAC_EID_GREEK, STBTT_MAC_EID_RUSSIAN};

enum { // languageID for STBTT_PLATFORM_ID_MICROSOFT; same as LCID...
       // problematic because there are e.g. 16 english LCIDs and 16 arabic LCIDs
   STBTT_MS_LANG_ENGLISH=0x0409,   STBTT_MS_LANG_ITALIAN=0x0410,
   STBTT_MS_LANG_CHINESE=0x0804,   STBTT_MS_LANG_JAPANESE=0x0411,
   STBTT_MS_LANG_DUTCH=0x0413,   STBTT_MS_LANG_KOREAN=0x0412,
   STBTT_MS_LANG_FRENCH=0x040c,   STBTT_MS_LANG_RUSSIAN=0x0419,
   STBTT_MS_LANG_GERMAN=0x0407,   STBTT_MS_LANG_SPANISH=0x0409,
   STBTT_MS_LANG_HEBREW=0x040d,   STBTT_MS_LANG_SWEDISH=0x041D
};

enum { // languageID for STBTT_PLATFORM_ID_MAC
   STBTT_MAC_LANG_ENGLISH=0,   STBTT_MAC_LANG_JAPANESE=11,
   STBTT_MAC_LANG_ARABIC=12,   STBTT_MAC_LANG_KOREAN=23,
   STBTT_MAC_LANG_DUTCH=4,   STBTT_MAC_LANG_RUSSIAN=32,
   STBTT_MAC_LANG_FRENCH=1,   STBTT_MAC_LANG_SPANISH=6 ,
   STBTT_MAC_LANG_GERMAN=2,   STBTT_MAC_LANG_SWEDISH=5 ,
   STBTT_MAC_LANG_HEBREW=10,   STBTT_MAC_LANG_CHINESE_SIMPLIFIED=33,
   STBTT_MAC_LANG_ITALIAN=3 ,   STBTT_MAC_LANG_CHINESE_TRAD=19
};

}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
////
////   IMPLEMENTATION
////
////

//////////////////////////////////////////////////////////////////////////
//
// accessors to parse data from file
//

// on platforms that don't allow misaligned reads, if we want to allow
// truetype fonts that aren't padded to alignment, define ALLOW_UNALIGNED_TRUETYPE

#define ttBYTE(p)     (*(byte*)(p))
#define ttCHAR(p)     (*(sbyte*)(p))
#define ttFixed(p)    ttLONG(p)

#if defined(STB_TRUETYPE_BIGENDIAN) && !defined(ALLOW_UNALIGNED_TRUETYPE)

   #define ttUSHORT(p)   (*(ushort*)(p))
   #define ttSHORT(p)    (*(short*)(p))
   #define ttULONG(p)    (*(uint*)(p))
   #define ttLONG(p)     (*(int*)(p))

#else

   ushort ttUSHORT(const byte *p) {return ushort((p[0] << 8)+p[1]);}
   short ttSHORT(const byte *p) {return short((p[0] << 8)+p[1]);}
   uint ttULONG(const byte *p) {return uint((p[0] << 24)+(p[1] << 16)+(p[2] << 8)+p[3]);}
   int ttLONG(const byte *p) {return int((p[0] << 24)+(p[1] << 16)+(p[2] << 8)+p[3]);}

#endif

#define stbtt_tag4(p,c0,c1,c2,c3) ((p)[0]==(c0) && (p)[1]==(c1) && (p)[2]==(c2) && (p)[3]==(c3))
#define stbtt_tag(p,str)           stbtt_tag4(p,str[0],str[1],str[2],str[3])

static int stbtt__isfont(const byte *font)
{
   // check the version number
   if(stbtt_tag4(font, '1', 0, 0, 0))  return 1; // TrueType 1
   if(stbtt_tag(font, "typ1"))   return 1; // TrueType with type 1 font -- we don't support this!
   if(stbtt_tag(font, "OTTO"))   return 1; // OpenType with CFF
   if(stbtt_tag4(font, 0,1,0,0)) return 1; // OpenType 1.0
   return 0;
}

// @OPTIMIZE: binary search
static uint stbtt__find_table(byte *data, uint fontstart, const char *tag)
{
   int num_tables=ttUSHORT(data+fontstart+4);
   uint tabledir=fontstart+12;
   for(int i=0; i<num_tables; i++)
   {
      uint loc=tabledir+16*i;
      if(stbtt_tag(data+loc+0, tag)) return ttULONG(data+loc+8);
   }
   return 0;
}

int stbtt_GetFontOffsetForIndex(const byte* font_collection, int index)
{
   // if it's just a font, there's only one valid index
   if(stbtt__isfont(font_collection))
      return index==0? 0: -1;

   // check if it's a TTC
   if(stbtt_tag(font_collection, "ttcf"))
   {
      // version 1?
      if(ttULONG(font_collection+4)==0x00010000 || ttULONG(font_collection+4)==0x00020000)
      {
         int n=ttLONG(font_collection+8);
         if(index>=n) return -1;
         return ttLONG(font_collection+12+index*14);
      }
   }
   return -1;
}

int stbtt_InitFont(stbtt_fontinfo* info, const byte* data2, int fontstart)
{
   byte* data=(byte*)data2;
   uint cmap, t;
   int i, numTables;

   info->data=data;
   info->fontstart=fontstart;

   cmap = stbtt__find_table(data, uint(fontstart), "cmap");       // required
   info->loca = int(stbtt__find_table(data, uint(fontstart), "loca")); // required
   info->head = int(stbtt__find_table(data, uint(fontstart), "head")); // required
   info->glyf = int(stbtt__find_table(data, uint(fontstart), "glyf")); // required
   info->hhea = int(stbtt__find_table(data, uint(fontstart), "hhea")); // required
   info->hmtx = int(stbtt__find_table(data, uint(fontstart), "hmtx")); // required
   info->kern = int(stbtt__find_table(data, uint(fontstart), "kern")); // not required
   if(!cmap || !info->loca || !info->head || !info->glyf || !info->hhea || !info->hmtx) return 0;

   t = stbtt__find_table(data, uint(fontstart), "maxp");
   if(t!=0) info->numGlyphs=ttUSHORT(data+t+4);
   else info->numGlyphs=0xffff;

   // find a cmap encoding table we understand *now* to avoid searching
   // later. (todo: could make this installable)
   // the same regardless of glyph.
   numTables = ttUSHORT(data+cmap+2);
   info->index_map=0;
   for(i=0; i<numTables; i++)
   {
      uint encoding_record=cmap+4 + 8*i;
      // find an encoding we understand:
      switch(ttUSHORT(data+encoding_record))
      {
         case STBTT_PLATFORM_ID_MICROSOFT:
            switch(ttUSHORT(data+encoding_record+2))
            {
               case STBTT_MS_EID_UNICODE_BMP:
               case STBTT_MS_EID_UNICODE_FULL:
                  // MS/Unicode
                  info->index_map = int(cmap) + ttLONG(data+encoding_record+4);
                  break;
            }
            break;
      }
   }
   if(info->index_map==0) return 0;

   info->indexToLocFormat=ttUSHORT(data+info->head+50);
   return 1;
}

int stbtt_FindGlyphIndex(const stbtt_fontinfo* info, int unicode_codepoint)
{
   byte* data = info->data;
   int index_map = info->index_map;

	ushort format=ttUSHORT(data+index_map);
	if(format==0)
	{
	  // apple byte encoding
      int bytes=ttUSHORT(data+index_map+2);
      if(unicode_codepoint<bytes-6)
         return ttBYTE(data+index_map+6+unicode_codepoint);
      return 0;
   }
	else if(format==6)
	{
		uint first=ttUSHORT(data+index_map+6);
		uint count=ttUSHORT(data+index_map+8);
		if((uint)unicode_codepoint>=first && (uint)unicode_codepoint<first+count)
			return ttUSHORT(data+index_map+10+(unicode_codepoint-first)*2);
		return 0;
	}
	else if(format==2)
	{
		INTRA_ASSERT(!"@TODO: high-byte mapping for japanese/chinese/korean");
		return 0;
	}
	else if(format==4)
	{
		// standard mapping for windows fonts: binary search collection of ranges
		ushort segcount=ttUSHORT(data+index_map+6)/2;
	    ushort searchRange=ttUSHORT(data+index_map+8)/2;
	    ushort entrySelector=ttUSHORT(data+index_map+10);
	    ushort rangeShift=ttUSHORT(data+index_map+12)/2;

		// do a binary search of the segments
		uint endCount = uint(index_map+14);
		uint search = endCount;

		if(unicode_codepoint>0xffff) return 0;

		// they lie from endCount .. endCount + segCount
		// but searchRange is the nearest power of two, so...
		if(unicode_codepoint>=ttUSHORT(data+search+rangeShift*2))
			search+=rangeShift*2;

		// now decrement to bias correctly to find smallest
		ushort start, end;
		search-=2;
		while(entrySelector)
		{
			searchRange/=2;
		//Зачем нужны повторные присваивания?
		//	start=ttUSHORT(data+search+2+segcount*2+2);
		//	end=ttUSHORT(data+search+2);
			start=ttUSHORT(data+search+searchRange*2+segcount*2+2);
			end=ttUSHORT(data+search+searchRange*2);
			if(unicode_codepoint>end) search+=searchRange*2;
			--entrySelector;
		}
		search+=2;

		ushort item = ushort((search-endCount)/2);

		INTRA_ASSERT(unicode_codepoint<=ttUSHORT(data+endCount+2*item));
		start=ttUSHORT(data+index_map+14+segcount*2+2+2*item);
		end=ttUSHORT(data+index_map+14+2+2*item);
		if(unicode_codepoint<start) return 0;

		ushort offset=ttUSHORT(data+index_map+14+segcount*6+2+2*item);
		if(offset==0) return (ushort)(unicode_codepoint+ttSHORT(data+index_map+14+segcount*4+2+2*item));

		return ttUSHORT(data+offset+(unicode_codepoint-start)*2+index_map+14+segcount*6+2+2*item);
	}
	else if(format==12 || format==13)
	{
      uint ngroups = ttULONG(data+index_map+12);
      int low=0, high = int(ngroups);

      // Binary search the right group.
      while(low<high)
	  {
         int mid=low+(high-low)/2; // rounds down, so low <= mid < high
         uint start_char = ttULONG(data+index_map+16+mid*12);
         uint end_char = ttULONG(data+index_map+16+mid*12+4);
         if(uint(unicode_codepoint)<start_char) high=mid;
         else if(uint(unicode_codepoint)>end_char) low=mid+1;
         else
		 {
            uint start_glyph = ttULONG(data+index_map+16+mid*12+8);
            if(format==12) return start_glyph+unicode_codepoint-start_char;
            else return start_glyph; // format == 13
         }
      }
      return 0; // not found
   }
   // @TODO
   INTRA_ASSERT(0);
   return 0;
}


static int stbtt__close_shape(stbtt_vertex* vertices, int num_vertices, int was_off, int start_off, SVec2 spos, SVec2 scpos, SVec2 cpos)
{
	if(start_off)
	{
		if(was_off) vertices[num_vertices++]={(cpos+scpos)/2, cpos, STBTT_vcurve};
		vertices[num_vertices++]={spos, scpos, STBTT_vcurve};
		return num_vertices;
	}
	if(was_off) vertices[num_vertices++]={spos, cpos, STBTT_vcurve};
	else vertices[num_vertices++]={spos, {0,0}, STBTT_vline};
	return num_vertices;
}

static int stbtt__GetGlyfOffset(const stbtt_fontinfo* info, int glyph_index)
{
   int g1, g2;

   if(glyph_index >= info->numGlyphs) return -1; // glyph index out of range
   if(info->indexToLocFormat >= 2)    return -1; // unknown index->glyph map format

   if(info->indexToLocFormat == 0)
   {
      g1 = info->glyf+ttUSHORT(info->data+info->loca+glyph_index*2)*2;
      g2 = info->glyf+ttUSHORT(info->data+info->loca+glyph_index*2+2)*2;
   }
   else
   {
      g1=info->glyf+ttULONG(info->data+info->loca+glyph_index*4);
      g2=info->glyf+ttULONG(info->data+info->loca+glyph_index*4+4);
   }

   return g1==g2 ? -1 : g1; // if length is 0, return -1
}

int stbtt_GetGlyphShape(const stbtt_fontinfo* info, int glyph_index, stbtt_vertex** pvertices)
{
	short numberOfContours;
	byte* endPtsOfContours;
	byte* data=info->data;
	stbtt_vertex* vertices=null;
	int num_vertices=0;
	int g=stbtt__GetGlyfOffset(info, glyph_index);

	*pvertices=null;

	if(g<0) return 0;

	numberOfContours=ttSHORT(data+g);

	if(numberOfContours>0)
	{
		byte flags=0, flagcount;
		int ins, j=0, m, n, next_move, was_off=0, off, start_off=0;
		SVec2 pos, cpos, spos, scpos;
		byte* points;
		endPtsOfContours=data+g+10;
		ins=ttUSHORT(data+g+10+numberOfContours*2);
		points=data+g+10+numberOfContours*2+2+ins;

		n=1+ttUSHORT(endPtsOfContours+numberOfContours*2-2);

		m=n+2*numberOfContours;  // a loose bound on how many vertices we might need
		vertices=new stbtt_vertex[m];

		next_move=0;
		flagcount=0;

		// in first pass, we load uninterpreted data into the allocated Array
		// above, shifted to the end of the Array so we won't overwrite it when
		// we create our final data starting from the front

		off=m-n; // starting offset for uninterpreted data, regardless of how m ends up being calculated

		// first load flags

		for(int i=0; i<n; i++)
		{
			if(flagcount==0)
			{
				flags=*points++;
				if(flags&8) flagcount=*points++;
			}
			else flagcount--;
			vertices[off+i].type=flags;
		}

		// now load x coordinates
		pos.x=0;
		for(int i=0; i<n; i++)
		{
			flags=vertices[off+i].type;
			if(flags&2)
			{
				short dx=*points++;
				pos.x+=(flags&16)? dx: -dx; // ???
			}
			else if(!(flags&16))
			{
				pos.x+=(short)(points[0]*256+points[1]);
				points+=2;
			}
			vertices[off+i].pos.x=pos.x;
		}

		// now load y coordinates
		pos.y=0;
		for(int i=0; i<n; i++)
		{
			flags=vertices[off+i].type;
			if(flags&4)
			{
				short dy=*points++;
				pos.y+=(flags&32)? dy: -dy; // ???
			}
			else if(!(flags&32))
			{
				pos.y+=(short)(points[0]*256+points[1]);
				points+=2;
			}
			vertices[off+i].pos.y=pos.y;
		}

		// now convert them to our format
		num_vertices=0;
		spos=cpos=scpos={0,0};
		for(int i=0; i<n; i++)
		{
			flags=vertices[off+i].type;
			pos=vertices[off+i].pos;

			if(next_move==i)
			{
				if(i!=0) num_vertices=stbtt__close_shape(vertices, num_vertices, was_off, start_off, spos, scpos, cpos);

				// now start the new one
				start_off=((flags&1)==0);
				if(start_off)
				{
					// if we start off with an off-curve point, then when we need to find a point on the curve
					// where we can start, and we need to save some state for when we wraparound.
					scpos=pos;
					if(!(vertices[off+i+1].type&1))
					{
						// next point is also a curve point, so interpolate an on-point curve
						spos=(pos+vertices[off+i+1].pos)/2;
					}
					else
					{
						// otherwise just use the next point as our start point
						spos=vertices[off+i+1].pos;
						i++; // we're using point i+1 as the starting point, so skip it
					}
				}
				else spos=pos;
				vertices[num_vertices++]={spos, {0,0}, STBTT_vmove};
				was_off=0;
				next_move=1+ttUSHORT(endPtsOfContours+j*2);
				j++;
			}
			else
			{
				if(!(flags&1))
				{ // if it's a curve
					if(was_off) // two off-curve control points in a row means interpolate an on-curve midpoint
						vertices[num_vertices++]={(cpos+pos)/2, cpos, STBTT_vcurve};
					cpos=pos;
					was_off=1;
				}
				else
				{
					if(was_off) vertices[num_vertices++]={pos, cpos, STBTT_vcurve};
					else vertices[num_vertices++]={pos, {0,0}, STBTT_vline};
					was_off=0;
				}
			}
		}
		num_vertices=stbtt__close_shape(vertices, num_vertices, was_off, start_off, spos, scpos, cpos);
	}
	else if(numberOfContours==-1)
	{
		// Compound shapes.
		int more=1;
		byte* comp=data+g+10;
		num_vertices=0;
		vertices=0;
		while(more)
		{
			ushort flags, gidx;
			int comp_num_verts=0;
			stbtt_vertex* comp_verts=null;
			stbtt_vertex* tmp=null;
			float mtx[6]={1,0,0,1,0,0}; float m, n;
         
			flags=ttSHORT(comp); comp+=2;
			gidx=ttSHORT(comp); comp+=2;

			if(flags&2)
			{ // XY values
				if(flags&1)
				{ // shorts
					mtx[4]=ttSHORT(comp); comp+=2;
					mtx[5]=ttSHORT(comp); comp+=2;
				}
				else
				{
					mtx[4]=ttCHAR(comp); comp+=1;
					mtx[5]=ttCHAR(comp); comp+=1;
				}
			}
			else
			{
				// @TODO handle matching point
				INTRA_ASSERT(0);
			}
			if(flags & (1<<3))
			{ // WE_HAVE_A_SCALE
				mtx[0] = mtx[3] = ttSHORT(comp)/16384.0f; comp+=2;
				mtx[1] = mtx[2] = 0;
			}
			else if(flags & (1<<6))
			{ // WE_HAVE_AN_X_AND_YSCALE
				mtx[0] = ttSHORT(comp)/16384.0f; comp+=2;
				mtx[1] = mtx[2] = 0;
				mtx[3] = ttSHORT(comp)/16384.0f; comp+=2;
			}
			else if(flags & (1<<7)) // WE_HAVE_A_TWO_BY_TWO
				for(ushort j=0; j<4; j++) mtx[j]=ttSHORT(comp)/16384.0f, comp+=2;
         
			// Find transformation scales.
			m = Sqrt(mtx[0]*mtx[0]+mtx[1]*mtx[1]);
			n = Sqrt(mtx[2]*mtx[2]+mtx[3]*mtx[3]);

			// Get indexed glyph.
			comp_num_verts = stbtt_GetGlyphShape(info, gidx, &comp_verts);
			if(comp_num_verts>0)
			{
				// Transform vertices.
				for(int i=0; i<comp_num_verts; i++)
				{
					stbtt_vertex* v=&comp_verts[i];
					stbtt_vertex_type x, y;
					x=v->pos.x; y=v->pos.y;
					v->pos.x=(stbtt_vertex_type)(m*(mtx[0]*x+mtx[2]*y+mtx[4]));
					v->pos.y=(stbtt_vertex_type)(n*(mtx[1]*x+mtx[3]*y+mtx[5]));
					x=v->cpos.x; y=v->cpos.y;
					v->cpos.x=(stbtt_vertex_type)(m*(mtx[0]*x+mtx[2]*y+mtx[4]));
					v->cpos.y=(stbtt_vertex_type)(n*(mtx[1]*x+mtx[3]*y+mtx[5]));
				}
				// Append vertices.
				tmp = new stbtt_vertex[num_vertices+comp_num_verts];
				CopyBits<stbtt_vertex>({tmp, size_t(num_vertices)}, {vertices, size_t(num_vertices)});
				CopyBits<stbtt_vertex>({tmp+num_vertices, size_t(comp_num_verts)}, {comp_verts, size_t(comp_num_verts)});
				delete[] vertices;
				vertices=tmp;
				delete[] comp_verts;
				num_vertices += comp_num_verts;
			}
			// More components ?
			more=flags&(1<<5);
		}
	}
	else if(numberOfContours<0)
	{
		// @TODO other compound variations?
		INTRA_ASSERT(0);
	}
	else
	{
		// numberOfCounters == 0, do nothing
	}

	*pvertices=vertices;
	return num_vertices;
}

int stbtt_GetCodepointShape(const stbtt_fontinfo* info, int unicode_codepoint, stbtt_vertex** vertices)
{
   return stbtt_GetGlyphShape(info, stbtt_FindGlyphIndex(info, unicode_codepoint), vertices);
}



int stbtt_GetGlyphBox(const stbtt_fontinfo *info, int glyph_index, SVec2* pos0, SVec2* pos1)
{
   int g=stbtt__GetGlyfOffset(info, glyph_index);
   if(g<0) return 0;

   if(pos0!=null) *pos0=SVec2(ttSHORT(info->data+g+2), ttSHORT(info->data+g+4));
   if(pos1!=null) *pos1=SVec2(ttSHORT(info->data+g+6), ttSHORT(info->data+g+8));
   return 1;
}

int stbtt_GetCodepointBox(const stbtt_fontinfo* info, int codepoint, SVec2* pos0, SVec2* pos1)
{
   return stbtt_GetGlyphBox(info, stbtt_FindGlyphIndex(info,codepoint), pos0, pos1);
}

bool stbtt_IsGlyphEmpty(const stbtt_fontinfo* info, int glyph_index)
{
   short numberOfContours;
   int g=stbtt__GetGlyfOffset(info, glyph_index);
   if(g<0) return true;
   numberOfContours=ttSHORT(info->data+g);
   return numberOfContours==0;
}



void stbtt_GetGlyphHMetrics(const stbtt_fontinfo* info, int glyph_index, short* advanceWidth, short* leftSideBearing)
{
   ushort numOfLongHorMetrics=ttUSHORT(info->data+info->hhea+34);
   int awindex, lsbindex;
   if(glyph_index<numOfLongHorMetrics) awindex=4*glyph_index, lsbindex=4*glyph_index+2;
   else awindex=4*(numOfLongHorMetrics-1), lsbindex=4*numOfLongHorMetrics+2*(glyph_index-numOfLongHorMetrics);
   if(advanceWidth!=null) *advanceWidth=ttSHORT(info->data+info->hmtx+awindex);
   if(leftSideBearing!=null) *leftSideBearing=ttSHORT(info->data+info->hmtx+lsbindex);
}

int stbtt_GetGlyphKernAdvance(const stbtt_fontinfo* info, int glyph1, int glyph2)
{
   byte* data=info->data+info->kern;

   // we only look at the first table. it must be 'horizontal' and format 0.
   if(info->kern==0) return 0;
   if(ttUSHORT(data+2)<1) return 0; // number of tables, need at least 1
   if(ttUSHORT(data+8)!=1) return 0; // horizontal flag must be set in format

   int l=0, r=ttUSHORT(data+10)-1;
   uint needle=(glyph1<<16)|glyph2;
   while(l<=r)
   {
      int m=(l+r)/2;
      const uint straw=ttULONG(data+18+(m*6)); // note: unaligned read
      if(needle<straw) r=m-1;
      else if(needle>straw) l=m+1;
      else return ttSHORT(data+22+(m*6));
   }
   return 0;
}

int stbtt_GetCodepointKernAdvance(const stbtt_fontinfo* info, int ch1, int ch2)
{
   if(info->kern==0) return 0; // if no kerning table, don't waste time looking up both codepoint->glyphs
   return stbtt_GetGlyphKernAdvance(info, stbtt_FindGlyphIndex(info, ch1), stbtt_FindGlyphIndex(info, ch2));
}

void stbtt_GetCodepointHMetrics(const stbtt_fontinfo* info, int codepoint, short* advanceWidth, short* leftSideBearing)
{
   stbtt_GetGlyphHMetrics(info, stbtt_FindGlyphIndex(info, codepoint), advanceWidth, leftSideBearing);
}

void stbtt_GetFontVMetrics(const stbtt_fontinfo* info, short* ascent, short* descent, short* lineGap)
{
   if(ascent!=null) *ascent=ttSHORT(info->data+info->hhea+4);
   if(descent!=null) *descent=ttSHORT(info->data+info->hhea+6);
   if(lineGap!=null) *lineGap=ttSHORT(info->data+info->hhea+8);
}

void stbtt_GetFontBoundingBox(const stbtt_fontinfo* info, SVec2* pos0, SVec2* pos1)
{
   *pos0=SVec2(ttSHORT(info->data+info->head+36), ttSHORT(info->data+info->head+38));
   *pos1=SVec2(ttSHORT(info->data+info->head+40), ttSHORT(info->data+info->head+42));
}

float stbtt_ScaleForPixelHeight(const stbtt_fontinfo* info, float height)
{
   int fheight=ttSHORT(info->data+info->hhea+4)-ttSHORT(info->data+info->hhea+6);
   return (float)height/fheight;
}

float stbtt_ScaleForMappingEmToPixels(const stbtt_fontinfo* info, float pixels)
{
   int unitsPerEm=ttUSHORT(info->data+info->head+18);
   return pixels/unitsPerEm;
}

void stbtt_FreeShape(const stbtt_fontinfo* info, stbtt_vertex* v)
{
	info;
	GlobalHeap.Free(v);
}

//////////////////////////////////////////////////////////////////////////////
//
// antialiasing software rasterizer
//

void stbtt_GetGlyphBitmapBoxSubpixel(const stbtt_fontinfo* font, int glyph, Vec2 scale, Vec2 shift, SVec2* ipos0, SVec2* ipos1)
{
   SVec2 pos0, pos1;
   if(!stbtt_GetGlyphBox(font, glyph, &pos0, &pos1)) pos0=pos1={0,0}; // e.g. space character
   // now move to integral bboxes (treating pixels as little squares, what pixels get touched)?
   if(ipos0!=null) *ipos0=SVec2(short(pos0.x*scale.x+shift.x), -(short)Ceil(pos1.y*scale.y+shift.y));
   if(ipos1!=null) *ipos1=SVec2(short(Ceil(pos1.x*scale.x+shift.x)), -short(pos0.y*scale.y+shift.y));
}
void stbtt_GetGlyphBitmapBox(const stbtt_fontinfo* font, int glyph, Vec2 scale, SVec2* ipos0, SVec2* ipos1)
{
	stbtt_GetGlyphBitmapBoxSubpixel(font, glyph, scale, {0,0}, ipos0, ipos1);
}

void stbtt_GetCodepointBitmapBoxSubpixel(const stbtt_fontinfo* font, int codepoint, Vec2 scale, Vec2 shift, SVec2* ipos0, SVec2* ipos1)
{
   stbtt_GetGlyphBitmapBoxSubpixel(font, stbtt_FindGlyphIndex(font, codepoint), scale, shift, ipos0, ipos1);
}

void stbtt_GetCodepointBitmapBox(const stbtt_fontinfo* font, int codepoint, Vec2 scale, SVec2* ipos0, SVec2* ipos1)
{
	stbtt_GetCodepointBitmapBoxSubpixel(font, codepoint, scale, {0,0}, ipos0, ipos1);
}

struct stbtt__edge
{
	Vec2 pos0, pos1;
	int invert;
};

struct stbtt__active_edge
{
   int x, dx;
   float ey;
   stbtt__active_edge* next;
   int valid;
};

#define FIXSHIFT   10
#define FIX        (1<<FIXSHIFT)
#define FIXMASK    (FIX-1)

static stbtt__active_edge* new_active(stbtt__edge* e, int off_x, float start_point, void* userdata)
{
	userdata;

	auto z=new stbtt__active_edge; // TODO: make a pool of these!!!
	float dxdy=(e->pos1.x-e->pos0.x)/(e->pos1.y-e->pos0.y);
	INTRA_ASSERT(e->pos0.y<=start_point);
	z->dx=(int)(FIX * dxdy);
	z->x=(int)(FIX*(e->pos0.x+dxdy*(start_point-e->pos0.y)));
	z->x-=off_x*FIX;
	z->ey=e->pos1.y;
	z->next=0;
	z->valid=e->invert? 1: -1;
	return z;
}

// note: this routine clips fills that extend off the edges... ideally this
// wouldn't happen, but it could happen if the truetype glyph bounding boxes
// are wrong, or if the user supplies a too-small bitmap
static void stbtt__fill_active_edges(byte* scanline, int len, stbtt__active_edge* e, int max_weight)
{
	// non-zero winding fill
	int x0=0, w=0;

	for(; e; e=e->next)
	{
		if(w==0) // if we're currently at zero, we need to record the edge start point
		{
			x0=e->x; w+=e->valid;
			continue;
		}

		int x1=e->x; w+=e->valid;
		if(w!=0) continue; // if we went to zero, we need to draw

		int i=x0>>FIXSHIFT, j=x1>>FIXSHIFT;
		if(i>=len || j<0) continue;

		if(i==j) //x0, x1 are the same pixel, so compute combined coverage
		{
			scanline[i]=scanline[i]+(byte)((x1-x0)*max_weight >> FIXSHIFT);
			continue;
		}

		if(i>=0) // add antialiasing for x0
			scanline[i]=scanline[i] + (byte)(((FIX-(x0&FIXMASK))*max_weight) >> FIXSHIFT);
		else i=-1; // clip

		if(j<len) // add antialiasing for x1
			scanline[j]=scanline[j] + (byte)(((x1&FIXMASK)*max_weight) >> FIXSHIFT);
		else j=len; // clip

		for(++i; i<j; ++i) // fill pixels between x0 and x1
			scanline[i]=scanline[i]+(byte)max_weight;

	}
}

static void stbtt__rasterize_sorted_edges(stbtt__bitmap* result, stbtt__edge* e, int n, int vsubsample, SVec2 off, void* userdata)
{
   stbtt__active_edge* active=null;
   int y, j=0;
   int max_weight=255/vsubsample;  // weight per vertical scanline
   byte scanline_data[512];
   byte* scanline;

   if(result->size.x>512)
   {
	   size_t size = result->size.x;
	   scanline = GlobalHeap.Allocate(size, INTRA_SOURCE_INFO);
   }
   else scanline=scanline_data;

   y=off.y*vsubsample;
   e[n].pos0.y=(off.y+result->size.y)*(float)vsubsample+1;

   while(j<result->size.y)
   {
      Intra::C::memset(scanline, 0, result->size.x);
      for (int s=0; s<vsubsample; s++)
      {
         // find center of pixel for this scanline
         float scan_y=y+0.5f;
         stbtt__active_edge** step=&active;

         // update all active edges;
         // remove all active edges that terminate before the center of this scanline
         while(*step)
         {
            stbtt__active_edge* z=*step;
            if(z->ey<=scan_y)
            {
               *step=z->next; // delete from list
               INTRA_ASSERT(z->valid);
               z->valid=0;
               GlobalHeap.Free(z);
            }
            else
            {
               z->x+=z->dx; // advance to position for current scanline
               step=&((*step)->next); // advance through list
            }
         }

         // resort the list if needed
         for(;;)
         {
            bool changed=false;
            step=&active;
            while(*step && (*step)->next)
            {
               if((*step)->x>(*step)->next->x)
               {
                  stbtt__active_edge* t=*step;
                  stbtt__active_edge* q=t->next;

                  t->next=q->next;
                  q->next=t;
                  *step=q;
                  changed=true;
               }
               step=&(*step)->next;
            }
            if(!changed) break;
         }

         // insert all edges that start before the center of this scanline -- omit ones that also end on this scanline
         while(e->pos0.y<=scan_y)
         {
            if(e->pos1.y>scan_y)
            {
               stbtt__active_edge* z=new_active(e, off.x, scan_y, userdata);
               // find insertion point
               if(active==null) active=z;
               else if(z->x < active->x)
               {
                  // insert at front
                  z->next=active;
                  active=z;
               }
               else
               {
                  // find thing to insert AFTER
                  stbtt__active_edge* p=active;
                  while(p->next && p->next->x<z->x) p=p->next;
                  // at this point, p->next->x is NOT < z->x
                  z->next=p->next;
                  p->next=z;
               }
            }
            ++e;
         }

         // now process all active edges in XOR fashion
         if(active) stbtt__fill_active_edges(scanline, result->size.x, active, max_weight);

         y++;
      }
      Intra::C::memcpy(result->pixels+j*result->stride, scanline, result->size.x);
      j++;
   }

   while(active)
   {
      stbtt__active_edge* z=active;
      active=active->next;
      delete z;
   }

   if(scanline!=scanline_data) GlobalHeap.Free(scanline);
}


static void stbtt__rasterize(stbtt__bitmap* result, Vec2* pts, int* wcount, int windings, Vec2 scale, Vec2 shift, SVec2 off, int invert, void* userdata)
{
   float y_scale_inv=invert? -scale.y: scale.y;
   int vsubsample=result->size.y<8? 15: 5;
   // vsubsample should divide 255 evenly; otherwise we won't reach full opacity

   // now we have to blow out the windings into explicit edge lists
   int count=0;
   for(int i=0; i<windings; i++) count+=wcount[i];

   Array<stbtt__edge> e(count+1); // add an extra one as a sentinel

   int m=0;
   for(int i=0; i<windings; i++)
   {
      Vec2* p=pts+m;
      m+=wcount[i];
      int j=wcount[i]-1;
      for(int k=0; k<wcount[i]; j=k++)
      {
         int a=k, b=j;
         // skip the edge if horizontal
         if(p[j].y==p[k].y) continue;
         // add edge from j to k to the list
		 stbtt__edge edge;
         edge.invert=false;
         if(invert? p[j].y>p[k].y: p[j].y<p[k].y)
         {
            edge.invert=true;
            a=j, b=k;
         }
         edge.pos0=Vec2(p[a].x*scale.x+shift.x, p[a].y*y_scale_inv*vsubsample+shift.y);
         edge.pos1=Vec2(p[b].x*scale.x+shift.x, p[b].y*y_scale_inv*vsubsample+shift.y);
         e.AddLast(edge);
      }
   }

   // now sort the edges by their highest point (should snap to integer, and then by x)
   Intra::Algo::ShellSort(e, [](const stbtt__edge& a, const stbtt__edge& b) {return a.pos0.y<b.pos0.y;});

   // now, traverse the scanlines and find the intersections on each scanline, use xor winding rule
   stbtt__rasterize_sorted_edges(result, &e[0], (uint)e.Count(), vsubsample, off, userdata);
}

static void stbtt__add_point(Vec2* points, int n, Vec2 pos)
{
   if(points==null) return; // during first pass, it's unallocated
   points[n]=pos;
}

// tesselate until threshhold p is happy... @TODO warped to compensate for non-linear stretching
static int stbtt__tesselate_curve(Vec2* points, int* num_points, Vec2 pos0, Vec2 pos1, Vec2 pos2, float objspace_flatness_squared, int n)
{
   // midpoint
   Vec2 m=(pos0 + pos1*2.0f + pos2)*0.25f;
   // versus directly drawn line
   Vec2 d=(pos0+pos2)*0.5f-m;
   if(n>16) // 65536 segments on one curve better be enough!
      return 1;
   if(d.x*d.x+d.y*d.y>objspace_flatness_squared)
   { // Half-pixel error allowed... need to be smaller if AA
      stbtt__tesselate_curve(points, num_points, pos0, (pos0+pos1)*0.5f, m, objspace_flatness_squared, n+1);
      stbtt__tesselate_curve(points, num_points, m, (pos1+pos2)*0.5f, pos2, objspace_flatness_squared, n+1);
   }
   else
   {
      stbtt__add_point(points, *num_points, pos2);
      *num_points = *num_points+1;
   }
   return 1;
}

// returns number of contours
static Vec2* stbtt_FlattenCurves(stbtt_vertex* vertices, int num_verts, float objspace_flatness, int** contour_lengths, int* num_contours, void* userdata)
{
	userdata;

	Vec2* points=null;
	int num_points=0;

	float objspace_flatness_squared=objspace_flatness*objspace_flatness;
	int n=0, start=0, pass;

	// count how many "moves" there are to get the contour count
	for(int i=0; i<num_verts; i++) if(vertices[i].type==STBTT_vmove) n++;

	*num_contours=n;
	if(n==0) return null;

	*contour_lengths=new int[n];

	// make two passes through the points so we don't need to realloc
	for(pass=0; pass<2; pass++)
	{
		SVec2 pos={0,0};
		if(pass==1) points=new Vec2[num_points];
		num_points=0;
		n=-1;
		for(int i=0; i<num_verts; i++)
		{
			switch(vertices[i].type)
			{
			case STBTT_vmove:
				// start the next contour
				if(n>=0) (*contour_lengths)[n]=num_points-start;
				++n;
				start=num_points;

				pos=vertices[i].pos;
				stbtt__add_point(points, num_points++, Vec2(pos.x, pos.y));
				break;

			case STBTT_vline:
				pos=vertices[i].pos;
				stbtt__add_point(points, num_points++, Vec2(pos.x, pos.y));
				break;

			case STBTT_vcurve:
				stbtt__tesselate_curve(points, &num_points, Vec2(pos.x, pos.y), Vec2(vertices[i].cpos.x, vertices[i].cpos.y),
					Vec2(vertices[i].pos.x, vertices[i].pos.y), objspace_flatness_squared, 0);
				pos=vertices[i].pos;
				break;
			}
		}
		(*contour_lengths)[n]=num_points-start;
	}

	return points;
}

static void stbtt_Rasterize(stbtt__bitmap* result, float flatness_in_pixels, stbtt_vertex* vertices, int num_verts, Vec2 scale, Vec2 shift, SVec2 off, int invert, void* userdata)
{
   float scale_min = Intra::Math::Min(scale.x, scale.y);
   int winding_count; int* winding_lengths;
   Vec2* windings=stbtt_FlattenCurves(vertices, num_verts, flatness_in_pixels/scale_min, &winding_lengths, &winding_count, userdata);
   if(windings)
   {
      stbtt__rasterize(result, windings, winding_lengths, winding_count, scale, shift, off, invert, userdata);
      delete[] winding_lengths;
      delete[] windings;
   }
}

static void stbtt_FreeBitmap(byte* bitmap, void* userdata)
{
	(void)userdata;
	GlobalHeap.Free(bitmap);
}

static byte* stbtt_GetGlyphBitmapSubpixel(const stbtt_fontinfo* info, Vec2 scale, Vec2 shift, int glyph, SVec2* size, SVec2* off)
{
   SVec2 i0, i1;
   stbtt__bitmap gbm;
   stbtt_vertex* vertices;
   int num_verts=stbtt_GetGlyphShape(info, glyph, &vertices);

   if(scale.x==0) scale.x=scale.y;
   if(scale.y==0)
   {
      if(scale.x==0) return null;
      scale.y=scale.x;
   }

   stbtt_GetGlyphBitmapBoxSubpixel(info, glyph, scale, shift, &i0, &i1);

   // now we get the size
   gbm.size=i1-i0;
   gbm.pixels=null; // in case we error

   if(size!=null) *size=gbm.size;
   if(off!=null) *off=i0;

   if(gbm.size.x && gbm.size.y)
   {
	  size_t bytesToAllocate = gbm.size.x*gbm.size.y;
      gbm.pixels = GlobalHeap.Allocate(bytesToAllocate, INTRA_SOURCE_INFO);
      if(gbm.pixels!=null)
      {
         gbm.stride=gbm.size.x;
         stbtt_Rasterize(&gbm, 0.35f, vertices, num_verts, scale, shift, i0, 1, info->userdata);
      }
   }
   GlobalHeap.Free(vertices);
   return gbm.pixels;
}   

byte* stbtt_GetGlyphBitmap(const stbtt_fontinfo* info, Vec2 scale, int glyph, SVec2* size, SVec2* off)
{
   return stbtt_GetGlyphBitmapSubpixel(info, scale, Vec2(0), glyph, size, off);
}

static void stbtt_MakeGlyphBitmapSubpixel(const stbtt_fontinfo* info, byte* output, SVec2 out_size, int out_stride, Vec2 scale, Vec2 shift, int glyph)
{

   stbtt_vertex* vertices;
   int num_verts=stbtt_GetGlyphShape(info, glyph, &vertices);

   SVec2 i0;
   stbtt_GetGlyphBitmapBoxSubpixel(info, glyph, scale, shift, &i0, null);

   stbtt__bitmap gbm={out_size, out_stride, output};
   if(gbm.size.x!=0 && gbm.size.y!=0)
      stbtt_Rasterize(&gbm, 0.35f, vertices, num_verts, scale, shift, i0, true, info->userdata);

   GlobalHeap.Free(vertices);
}

void stbtt_MakeGlyphBitmap(const stbtt_fontinfo* info, byte* output, SVec2 out_size, int out_stride, Vec2 scale, int glyph)
{
   stbtt_MakeGlyphBitmapSubpixel(info, output, out_size, out_stride, scale, Vec2(0), glyph);
}

static byte *stbtt_GetCodepointBitmapSubpixel(const stbtt_fontinfo* info, Vec2 scale, Vec2 shift, int codepoint, SVec2* size, SVec2* off)
{
   return stbtt_GetGlyphBitmapSubpixel(info, scale, shift, stbtt_FindGlyphIndex(info, codepoint), size, off);
}   

static void stbtt_MakeCodepointBitmapSubpixel(const stbtt_fontinfo* info, byte* output, SVec2 out_size, int out_stride, Vec2 scale, Vec2 shift, int codepoint)
{
   stbtt_MakeGlyphBitmapSubpixel(info, output, out_size, out_stride, scale, shift, stbtt_FindGlyphIndex(info, codepoint));
}

static byte* stbtt_GetCodepointBitmap(const stbtt_fontinfo* info, Vec2 scale, int codepoint, SVec2* size, SVec2* off)
{
   return stbtt_GetCodepointBitmapSubpixel(info, scale, Vec2(0), codepoint, size, off);
}   

void stbtt_MakeCodepointBitmap(const stbtt_fontinfo* info, byte* output, SVec2 out_size, int out_stride, Vec2 scale, int codepoint)
{
   stbtt_MakeCodepointBitmapSubpixel(info, output, out_size, out_stride, scale, Vec2(0), codepoint);
}



//////////////////////////////////////////////////////////////////////////////
//
// font name matching -- recommended not to use this
//

// check if a utf8 String contains a prefix which is the utf16 String; if so return length of matching utf8 String
static int stbtt__CompareUTF8toUTF16_bigendian_prefix(const byte *s1, int len1, const byte* s2, int len2)
{
   int i=0;

   // convert utf16 to utf8 and compare the results while converting
   while(len2)
   {
      ushort ch=s2[0]*256+s2[1];
      if(ch<0x80)
      {
         if(i>=len1) return -1;
         if(s1[i++]!=ch) return -1;
      }
      else if(ch<0x800)
      {
         if(i+1>=len1) return -1;
         if(s1[i++]!=0xc0+(ch>>6)) return -1;
         if(s1[i++]!=0x80+(ch&0x3f)) return -1;
      }
      else if(ch>=0xd800 && ch<0xdc00)
      {
         uint c;
         ushort ch2=s2[2]*256+s2[3];
         if(i+3>=len1) return -1;
         c=((ch-0xd800)<<10)+(ch2-0xdc00)+0x10000;
         if(s1[i++]!=0xf0+(c>>18)) return -1;
         if(s1[i++]!=0x80+((c>>12)&0x3f)) return -1;
         if(s1[i++]!=0x80+((c>>6)&0x3f)) return -1;
         if(s1[i++]!=0x80+(c&0x3f)) return -1;
         s2+=2; // plus another 2 below
         len2-=2;
      }
      else if(ch>=0xdc00 && ch<0xe000) return -1;
      else
      {
         if(i+2>=len1) return -1;
         if(s1[i++]!=0xe0+(ch>>12)) return -1;
         if(s1[i++]!=0x80+((ch>>6)&0x3f)) return -1;
         if(s1[i++]!=0x80+(ch&0x3f)) return -1;
      }
      s2+=2;
      len2-=2;
   }
   return i;
}

static int stbtt_CompareUTF8toUTF16_bigendian(const char* s1, int len1, const char* s2, int len2)
{
   return len1==stbtt__CompareUTF8toUTF16_bigendian_prefix((const byte*)s1, len1, (const byte*)s2, len2);
}

// returns results in whatever encoding you request... but note that 2-byte encodings
// will be BIG-ENDIAN... use stbtt_CompareUTF8toUTF16_bigendian() to compare
const char* stbtt_GetFontNameString(const stbtt_fontinfo* font, int* length, int platformID, int encodingID, int languageID, int nameID)
{
   int count,stringOffset;
   byte* fc=font->data;
   uint offset=font->fontstart;
   uint nm=stbtt__find_table(fc, offset, "name");
   if(nm==0) return null;

   count=ttUSHORT(fc+nm+2);
   stringOffset=nm+ttUSHORT(fc+nm+4);
   for(int i=0; i<count; i++)
   {
      uint loc=nm+6+12*i;
      if(platformID==ttUSHORT(fc+loc+0) && encodingID == ttUSHORT(fc+loc+2)
          && languageID==ttUSHORT(fc+loc+4) && nameID==ttUSHORT(fc+loc+6))
      {
         *length=ttUSHORT(fc+loc+8);
         return (const char*)(fc+stringOffset+ttUSHORT(fc+loc+10));
      }
   }
   return null;
}

static bool stbtt__matchpair(byte* fc, uint nm, byte* name, int nlen, int target_id, int next_id)
{
   int count=ttUSHORT(fc+nm+2);
   int stringOffset=nm+ttUSHORT(fc+nm+4);

   for(int i=0; i<count; i++)
   {
      uint loc=nm+6+12*i;
      int id=ttUSHORT(fc+loc+6);
      if(id==target_id)
      {
         // find the encoding
         int platform=ttUSHORT(fc+loc+0), encoding=ttUSHORT(fc+loc+2), language=ttUSHORT(fc+loc+4);

         // is this a Unicode encoding?
         if(platform==0 || (platform==3 && encoding==1) || (platform==3 && encoding==10))
         {
            int slen=ttUSHORT(fc+loc+8), off=ttUSHORT(fc+loc+10);

            // check if there's a prefix match
            int matchlen=stbtt__CompareUTF8toUTF16_bigendian_prefix(name, nlen, fc+stringOffset+off,slen);
            if(matchlen >= 0)
            {
               // check for target_id+1 immediately following, with same encoding & language
               if(i+1<count && ttUSHORT(fc+loc+12+6)==next_id && ttUSHORT(fc+loc+12)==platform && ttUSHORT(fc+loc+12+2)==encoding && ttUSHORT(fc+loc+12+4)==language)
               {
                  int slen2=ttUSHORT(fc+loc+12+8), off2=ttUSHORT(fc+loc+12+10);
                  if(slen2==0) {if(matchlen==nlen) return true;}
                  else if(matchlen<nlen && name[matchlen]==' ')
                  {
                     ++matchlen;
                     if(stbtt_CompareUTF8toUTF16_bigendian((char*)(name+matchlen), nlen-matchlen, (char*)(fc+stringOffset+off2), slen2)) return true;
                  }
               }
               else if(matchlen==nlen) return true; // if nothing immediately following
            }
         }

         // @TODO handle other encodings
      }
   }
   return false;
}

static bool stbtt__matches(byte* fc, uint offset, byte* name, int flags)
{
   int nlen = (int)Intra::C::strlen((char*)name);
   if(!stbtt__isfont(fc+offset)) return 0;

   // check italics/bold/underline flags in macStyle...
   if(flags)
   {
      const uint hd=stbtt__find_table(fc, offset, "head");
      if((ttUSHORT(fc+hd+44)&7)!=(flags&7)) return false;
   }

   const uint nm=stbtt__find_table(fc, offset, "name");
   if(nm==0) return false;

   static const int ids[]={16, 1, 3,   17, 2, -1}; // if we checked the macStyle flags, then just check the family and ignore the subfamily
   for(int i=0; i<3; i++) if(stbtt__matchpair(fc, nm, name, nlen, ids[i], flags? -1: ids[3+i])) return true;
   return false;
}

int stbtt_FindMatchingFont(const byte* font_collection, const char* name_utf8, int flags)
{
   for(int i=0; ; i++)
   {
      int off=stbtt_GetFontOffsetForIndex(font_collection, i);
      if(off<0 || stbtt__matches((byte*)font_collection, off, (byte*)name_utf8, flags)) return off;
   }
}



#include "IO/File.h"

namespace Intra {
using namespace Math;
using namespace IO;
	
namespace FontLoadingAPI {

struct Font
{
	stbtt_fontinfo font;
	float height_scale;
	byte* last_bitmap;
};


static FontHandle create_font(byte* data, uint height, uint* yadvance)
{
	FontHandle desc = new Font;
	desc->last_bitmap=null;
	stbtt_InitFont(&desc->font, data, stbtt_GetFontOffsetForIndex(data, 0));
	desc->height_scale=stbtt_ScaleForPixelHeight(&desc->font, height);
	short ascent, descent, lineGap;
	stbtt_GetFontVMetrics(&desc->font, &ascent, &descent, &lineGap);
	if(yadvance!=null) *yadvance = ushort((ascent-descent+lineGap)*desc->height_scale);
	return desc;
}

FontHandle FontCreate(StringView name, uint height, uint* yadvance)
{
	DiskFile::Reader file = DiskFile::Reader(name);
	if(file==null) return null;
	size_t size = uint(file.GetSize());
	byte* data = GlobalHeap.Allocate(size, INTRA_SOURCE_INFO);
	file.ReadData(data, size);
	return create_font(data, height, yadvance);
}

FontHandle FontCreateFromMemory(const void* src, size_t length, uint height, uint* yadvance)
{
	byte* data = GlobalHeap.Allocate(length, INTRA_SOURCE_INFO);
	Intra::C::memcpy(data, src, length);
	return create_font(data, height, yadvance);
}

void FontDelete(FontHandle font)
{
	if(font!=null) GlobalHeap.Free(font->font.data);
	delete font;
}

const byte* FontGetCharBitmap(FontHandle font, int code, SVec2* offset, USVec2* size)
{
	if(font==null) return null;
	stbtt_FreeBitmap(font->last_bitmap, null);
	font->last_bitmap = stbtt_GetCodepointBitmap(&font->font, Vec2(0, font->height_scale), code, (SVec2*)size, offset);
	offset->y = -offset->y;
	return font->last_bitmap;
}

void FontGetCharMetrics(FontHandle font, int code, short* xadvance, short* leftSideBearing)
{
	if(font==null) return;

	short xAdvance, lsb;
	stbtt_GetCodepointHMetrics(&font->font, code, &xAdvance, &lsb);
	if(leftSideBearing!=null) *leftSideBearing = short(lsb*font->height_scale);
	if(xadvance!=null) *xadvance = short(xAdvance*font->height_scale);
}

short FontGetKerning(FontHandle font, int left, int right)
{
	if(font==null) return 0;
	return short(font->height_scale*stbtt_GetCodepointKernAdvance(&font->font, left, right));
}

}}

INTRA_WARNING_POP

#endif
