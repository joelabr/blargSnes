/*
    Copyright 2014 StapleButter

    This file is part of blargSnes.

    blargSnes is free software: you can redistribute it and/or modify it under
    the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    blargSnes is distributed in the hope that it will be useful, but WITHOUT ANY 
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS 
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along 
    with blargSnes. If not, see http://www.gnu.org/licenses/.
*/

#include <ctr/types.h>

#include "snes.h"


extern u8* TopFB;


u32 Mem_WRAMAddr = 0;
u8 SPC_IOPorts[8];


u16 PPU_VCount = 0;

u16 PPU_ColorTable[0x10000];

u16 PPU_MainBuffer[256];
u16 PPU_SubBuffer[256];

u8 PPU_MainPrio[256];
u8 PPU_SubPrio[256];

u16 PPU_CGRAMAddr = 0;
u8 PPU_CGRAMVal = 0;
u16 PPU_CGRAM[256];

u16 PPU_VRAMAddr = 0;
u16 PPU_VRAMPref = 0;
u8 PPU_VRAMInc = 0;
u8 PPU_VRAMStep = 0;
u8 PPU_VRAM[0x10000];

u16 PPU_OAMAddr = 0;
u8 PPU_OAMVal = 0;
u8 PPU_OAMPrio = 0;
u8 PPU_FirstOBJ = 0;
u16 PPU_OAMReload = 0;
u8 PPU_OAM[0x220];

u8 PPU_OBJWidths[16] = 
{
	8, 16,
	8, 32,
	8, 64,
	16, 32,
	16, 64,
	32, 64,
	16, 32,
	16, 32
};
u8 PPU_OBJHeights[16] = 
{
	8, 16,
	8, 32,
	8, 64,
	16, 32,
	16, 64,
	32, 64,
	32, 64,
	32, 32
};

u8* PPU_OBJWidth;
u8* PPU_OBJHeight;


u8 PPU_Mode = 0;


u16 PPU_SubBackdrop = 0;

u8 PPU_BGOld = 0;


typedef struct
{
	u16* Tileset;
	u16* Tilemap;
	u8 Size;
	
	u16 XScroll, YScroll;

} PPU_Background;
PPU_Background PPU_BG[4];

u16* PPU_OBJTileset;
u32 PPU_OBJGap;



void PPU_Reset()
{
	int i;
	
	//PPU_MainBuffer = (u16*)MemAlloc(256 * 2);
	
	memset(PPU_VRAM, 0, 0x10000);
	memset(PPU_CGRAM, 0, 0x200);
	memset(PPU_OAM, 0, 0x220);
	
	memset(&PPU_BG[0], 0, sizeof(PPU_Background)*4);
	
	for (i = 0; i < 4; i++)
	{
		PPU_BG[i].Tileset = (u16*)PPU_VRAM;
		PPU_BG[i].Tilemap = (u16*)PPU_VRAM;
	}
	
	PPU_OBJTileset = (u16*)PPU_VRAM;
	
	PPU_OBJWidth = &PPU_OBJWidths[0];
	PPU_OBJHeight = &PPU_OBJHeights[0];
}


inline void PPU_SetXScroll(int nbg, u8 val)
{
	/*u32 m7stuff = 0;
	
	if (nbg == 0)
	{
		m7stuff = (val << 8) | PPU_M7Old;
		PPU_M7Old = val;
		
		if (PPU_ModeNow == 7)
		{
			PPU_ScheduleLineChange(PPU_SetM7ScrollX, m7stuff);
			return;
		}
	}*/
	
	PPU_Background* bg = &PPU_BG[nbg];
	
	bg->XScroll = (val << 8) | (PPU_BGOld & 0xFFF8) | ((bg->XScroll >> 8) & 0x7);
	PPU_BGOld = val;
}

inline void PPU_SetYScroll(int nbg, u8 val)
{
	/*u32 m7stuff = 0;
	
	if (nbg == 0)
	{
		m7stuff = (val << 8) | PPU_M7Old;
		PPU_M7Old = val;
		
		if (PPU_ModeNow == 7)
		{
			PPU_ScheduleLineChange(PPU_SetM7ScrollY, m7stuff);
			return;
		}
	}*/
	
	PPU_Background* bg = &PPU_BG[nbg];
	
	bg->YScroll = (val << 8) | PPU_BGOld;
	PPU_BGOld = val;
}

inline void PPU_SetBGSCR(int nbg, u8 val)
{
	PPU_Background* bg = &PPU_BG[nbg];
	
	bg->Size = (val & 0x03);
	bg->Tilemap = (u16*)&PPU_VRAM[(val & 0xFC) << 9];
}

inline void PPU_SetBGCHR(int nbg, u8 val)
{
	PPU_Background* bg = &PPU_BG[nbg];
	
	bg->Tileset = (u16*)&PPU_VRAM[val << 13];
}


u8 PPU_Read8(u32 addr)
{
	u8 ret = 0;
	switch (addr)
	{
		/*case 0x34: ret = PPU_MulResult & 0xFF; break;
		case 0x35: ret = (PPU_MulResult >> 8) & 0xFF; break;
		case 0x36: ret = (PPU_MulResult >> 16) & 0xFF; break;
		
		case 0x37:
			PPU_LatchHVCounters();
			break;*/
			
		case 0x38:
			ret = PPU_OAM[PPU_OAMAddr];
			PPU_OAMAddr++;
			break;
		
		case 0x39:
			{
				ret = PPU_VRAMPref & 0xFF;
				if (!(PPU_VRAMInc & 0x80))
				{
					PPU_VRAMPref = *(u16*)&PPU_VRAM[PPU_VRAMAddr];
					PPU_VRAMAddr += PPU_VRAMStep;
				}
			}
			break;
		case 0x3A:
			{
				ret = PPU_VRAMPref >> 8;
				if (PPU_VRAMInc & 0x80)
				{
					PPU_VRAMPref = *(u16*)&PPU_VRAM[PPU_VRAMAddr];
					PPU_VRAMAddr += PPU_VRAMStep;
				}
			}
			break;
			
		/*case 0x3C:
			if (PPU_OPHFlag)
			{
				PPU_OPHFlag = 0;
				ret = PPU_OPHCT >> 8;
			}
			else
			{
				PPU_OPHFlag = 1;
				ret = PPU_OPHCT & 0xFF;
			}
			break;
		case 0x3D:
			if (PPU_OPVFlag)
			{
				PPU_OPVFlag = 0;
				ret = PPU_OPVCT >> 8;
			}
			else
			{
				PPU_OPVFlag = 1;
				ret = PPU_OPVCT & 0xFF;
			}
			break;
			
		case 0x3E: ret = 0x01; break;
		case 0x3F: 
			ret = 0x01 | (ROM_Region ? 0x10 : 0x00) | PPU_OPLatch;
			PPU_OPLatch = 0;
			PPU_OPHFlag = 0;
			PPU_OPVFlag = 0;
			break;*/
		
		case 0x40: ret = SPC_IOPorts[0]; /*ret = SPC_IOPorts[4];*/ break;
		case 0x41: ret = SPC_IOPorts[5]; break;
		case 0x42: ret = SPC_IOPorts[6]; break;
		case 0x43: ret = SPC_IOPorts[7]; break;
		
		case 0x80: ret = SNES_SysRAM[Mem_WRAMAddr++]; break;
	}

	return ret;
}

u16 PPU_Read16(u32 addr)
{
	u16 ret = 0;
	switch (addr)
	{
		// not in the right place, but well
		// our I/O functions are mapped to the whole $21xx range
		
		case 0x40: ret = 0xBBAA; /*ret = *(u16*)&SPC_IOPorts[4];*/ break;
		case 0x42: ret = *(u16*)&SPC_IOPorts[6]; break;
		
		default:
			ret = PPU_Read8(addr);
			ret |= (PPU_Read8(addr+1) << 8);
			break;
	}

	return ret;
}

void PPU_Write8(u32 addr, u8 val)
{
	switch (addr)
	{
		/*case 0x00: // force blank/master brightness
			{
				u16 mb;
				
				if (val & 0x80) val = 0;
				else val &= 0x0F;
				if (val == 0x0F)
					mb = 0x0000;
				else if (val == 0x00)
					mb = 0x8010;
				else
					mb = 0x8000 | (15 - (val & 0x0F));
					
				PPU_ScheduleLineChange(PPU_SetMasterBright, mb);
			}
			break;*/
			
		case 0x01:
			{
				PPU_OBJWidth = &PPU_OBJWidths[(val & 0xE0) >> 4];
				PPU_OBJHeight = &PPU_OBJHeights[(val & 0xE0) >> 4];
				
				PPU_OBJTileset = (u16*)&PPU_VRAM[(val & 0x03) << 14];
				PPU_OBJGap = (val & 0x1C) << 9;
			}
			break;
			
		case 0x02:
			PPU_OAMAddr = (PPU_OAMAddr & 0x200) | (val << 1);
			PPU_OAMReload = PPU_OAMAddr;
			PPU_FirstOBJ = PPU_OAMPrio ? ((PPU_OAMAddr >> 1) & 0x7F) : 0;
			break;
		case 0x03:
			PPU_OAMAddr = (PPU_OAMAddr & 0x1FE) | ((val & 0x01) << 9);
			PPU_OAMPrio = val & 0x80;
			PPU_OAMReload = PPU_OAMAddr;
			PPU_FirstOBJ = PPU_OAMPrio ? ((PPU_OAMAddr >> 1) & 0x7F) : 0;
			break;
			
		case 0x04:
			if (PPU_OAMAddr >= 0x200)
			{
				PPU_OAM[PPU_OAMAddr & 0x21F] = val;
			}
			else if (PPU_OAMAddr & 0x1)
			{
				*(u16*)&PPU_OAM[PPU_OAMAddr - 1] = PPU_OAMVal | (val << 8);
			}
			else
			{
				PPU_OAMVal = val;
			}
			PPU_OAMAddr++;
			PPU_OAMAddr &= ~0x400;
			break;
			
		case 0x05:
			PPU_Mode = val;
			break;
			
		/*case 0x06: // mosaic
			{
				if (PPU_ModeNow == 7)
				{
					*(u16*)0x0400000C = 0xC082 | (2 << 2) | (24 << 8) | (val & 0x01) << 6;
				}
				else
				{
					int i;
					for (i = 0; i < 4; i++)
					{
						PPU_Background* bg = &PPU_BG[i];
						if (val & (1 << i)) bg->BGCnt |= 0x0040;
						else bg->BGCnt &= 0xFFFFFFBF;
						
						*bg->BGCNT = bg->BGCnt;
					}
				}
			
				*(vu16*)0x0400004C = (val & 0xF0) | (val >> 4);
			}
			break;*/
			
		case 0x07: PPU_SetBGSCR(0, val); break;
		case 0x08: PPU_SetBGSCR(1, val); break;
		case 0x09: PPU_SetBGSCR(2, val); break;
		case 0x0A: PPU_SetBGSCR(3, val); break;
			
			
		case 0x0B:
			PPU_SetBGCHR(0, val & 0x0F);
			PPU_SetBGCHR(1, val >> 4);
			break;
		case 0x0C:
			PPU_SetBGCHR(2, val & 0x0F);
			PPU_SetBGCHR(3, val >> 4);
			break;
			
		
		case 0x0D: PPU_SetXScroll(0, val); break;
		case 0x0E: PPU_SetYScroll(0, val); break;
		case 0x0F: PPU_SetXScroll(1, val); break;
		case 0x10: PPU_SetYScroll(1, val); break;
		case 0x11: PPU_SetXScroll(2, val); break;
		case 0x12: PPU_SetYScroll(2, val); break;
		case 0x13: PPU_SetXScroll(3, val); break;
		case 0x14: PPU_SetYScroll(3, val); break;
			
		
		case 0x15:
			if ((val & 0x7C) != 0x00) bprintf("UNSUPPORTED VRAM MODE %02X\n", val);
			PPU_VRAMInc = val;
			switch (val & 0x03)
			{
				case 0x00: PPU_VRAMStep = 2; break;
				case 0x01: PPU_VRAMStep = 64; break;
				case 0x02:
				case 0x03: PPU_VRAMStep = 256; break;
			}
			break;
			
		case 0x16:
			PPU_VRAMAddr &= 0xFE00;
			PPU_VRAMAddr |= (val << 1);
			PPU_VRAMPref = *(u16*)&PPU_VRAM[PPU_VRAMAddr];
			break;
		case 0x17:
			PPU_VRAMAddr &= 0x01FE;
			PPU_VRAMAddr |= ((val & 0x7F) << 9);
			PPU_VRAMPref = *(u16*)&PPU_VRAM[PPU_VRAMAddr];
			break;
		
		case 0x18: // VRAM shit
			{
				PPU_VRAM[PPU_VRAMAddr] = val;
				if (!(PPU_VRAMInc & 0x80))
					PPU_VRAMAddr += PPU_VRAMStep;
			}
			break;
		case 0x19:
			{
				PPU_VRAM[PPU_VRAMAddr+1] = val;
				if (PPU_VRAMInc & 0x80)
					PPU_VRAMAddr += PPU_VRAMStep;
			}
			break;
			
		/*case 0x1B: // multiply/mode7 shiz
			{
				u16 fval = (u16)(PPU_M7Old | (val << 8));
				PPU_MulA = (s16)fval;
				PPU_MulResult = (s32)PPU_MulA * (s32)PPU_MulB;
				PPU_ScheduleLineChange(PPU_SetM7A, fval);
				PPU_M7Old = val;
			}
			break;
		case 0x1C:
			PPU_ScheduleLineChange(PPU_SetM7B, (val << 8) | PPU_M7Old);
			PPU_M7Old = val;
			PPU_MulB = (s8)val;
			PPU_MulResult = (s32)PPU_MulA * (s32)PPU_MulB;
			break;
		case 0x1D:
			PPU_ScheduleLineChange(PPU_SetM7C, (val << 8) | PPU_M7Old);
			PPU_M7Old = val;
			break;
		case 0x1E:
			PPU_ScheduleLineChange(PPU_SetM7D, (val << 8) | PPU_M7Old);
			PPU_M7Old = val;
			break;
			
		case 0x1F: // mode7 center
			PPU_ScheduleLineChange(PPU_SetM7RefX, (val << 8) | PPU_M7Old);
			PPU_M7Old = val;
			break;
		case 0x20:
			PPU_ScheduleLineChange(PPU_SetM7RefY, (val << 8) | PPU_M7Old);
			PPU_M7Old = val;
			break;*/
			
		case 0x21:
			PPU_CGRAMAddr = val << 1;
			break;
		
		case 0x22:
			if (!(PPU_CGRAMAddr & 0x1))
				PPU_CGRAMVal = val;
			else
				PPU_CGRAM[PPU_CGRAMAddr >> 1] = (val << 8) | PPU_CGRAMVal;

			PPU_CGRAMAddr++;
			PPU_CGRAMAddr &= ~0x200; // prevent overflow
			break;
		
		/*case 0x2C:
			PPU_ScheduleLineChange(PPU_SetMainScreen, val & 0x1F);
			break;
		case 0x2D:
			PPU_ScheduleLineChange(PPU_SetSubScreen, val & 0x1F);
			break;
			
		case 0x31:
			PPU_ScheduleLineChange(PPU_SetColorMath, val);
			break;*/
			
		case 0x32:
			{
				u8 intensity = val & 0x1F;
				if (val & 0x20) PPU_SubBackdrop = (PPU_SubBackdrop & ~0x001F) | intensity;
				if (val & 0x40) PPU_SubBackdrop = (PPU_SubBackdrop & ~0x03E0) | (intensity << 5);
				if (val & 0x80) PPU_SubBackdrop = (PPU_SubBackdrop & ~0x7C00) | (intensity << 10);
			}
			break;
			
		/*case 0x33: // SETINI
			if (val & 0x80) iprintf("!! PPU EXT SYNC\n");
			if (val & 0x40) iprintf("!! MODE7 EXTBG\n");
			if (val & 0x08) iprintf("!! PSEUDO HIRES\n");
			if (val & 0x02) iprintf("!! SMALL SPRITES\n");
			break;*/
			
		case 0x40: SPC_IOPorts[0] = val; break;
		case 0x41: SPC_IOPorts[1] = val; break;
		case 0x42: SPC_IOPorts[2] = val; break;
		case 0x43: SPC_IOPorts[3] = val; break;
		
		case 0x80: SNES_SysRAM[Mem_WRAMAddr++] = val; break;
		case 0x81: Mem_WRAMAddr = (Mem_WRAMAddr & 0x0001FF00) | val; break;
		case 0x82: Mem_WRAMAddr = (Mem_WRAMAddr & 0x000100FF) | (val << 8); break;
		case 0x83: Mem_WRAMAddr = (Mem_WRAMAddr & 0x0000FFFF) | ((val & 0x01) << 16); break;
				
		default:
			//iprintf("PPU_Write8(%08X, %08X)\n", addr, val);
			break;
	}
}

void PPU_Write16(u32 addr, u16 val)
{
	switch (addr)
	{
		// optimized route
		
		case 0x16:
			PPU_VRAMAddr = (val << 1) & 0xFFFEFFFF;
			PPU_VRAMPref = *(u16*)&PPU_VRAM[PPU_VRAMAddr];
			break;
			
		case 0x18:
			*(u16*)&PPU_VRAM[PPU_VRAMAddr] = val;
			PPU_VRAMAddr += PPU_VRAMStep;
			break;
			
		case 0x40: *(u16*)&SPC_IOPorts[0] = val; break;
		case 0x41: *(u16*)&SPC_IOPorts[1] = val; break;
		case 0x42: *(u16*)&SPC_IOPorts[2] = val; bprintf("SPC: %02X\n", val);break;
		
		case 0x43: bprintf("!! write $21%02X %04X\n", addr, val); break;
		
		case 0x81: Mem_WRAMAddr = (Mem_WRAMAddr & 0x00010000) | val; break;
		
		// otherwise, just do two 8bit writes
		default:
			PPU_Write8(addr, val & 0x00FF);
			PPU_Write8(addr+1, val >> 8);
			break;
	}
}



#define PRIO_HIGH 0x2000
#define PRIO_LOW 0

void PPU_RenderBG_2bpp_8x8(PPU_Background* bg, u16* buffer, u32 line, u16* pal, u32 prio)
{
	u16* tileset = bg->Tileset;
	u16* tilemap = bg->Tilemap;
	u32 xoff;
	u32 tiley;
	u16 curtile;
	u32 tilepixels;
	u32 colorval;
	u32 i;
	u32 idx;
	
	line += bg->YScroll;
	tiley = (line & 0x07);
	tilemap += (line & 0xF8) << 2;
	if (line & 0x100)
	{
		if (bg->Size & 0x2)
			tilemap += (bg->Size & 0x1) ? 2048 : 1024;
	}
	
	xoff = bg->XScroll;
	idx = (xoff & 0xF8) >> 3;
	if (xoff & 0x100)
	{
		if (bg->Size & 0x1)
			idx += 1024;
	}
	curtile = tilemap[idx];
	
	idx = (curtile & 0x3FF) << 3;
	if (curtile & 0x8000) 	idx += (7 - tiley);
	else					idx += tiley;
	
	tilepixels = tileset[idx];
	if (curtile & 0x4000)	tilepixels >>= (xoff & 0x7);
	else					tilepixels <<= (xoff & 0x7);
	
	for (i = 0; i < 256;)
	{
		// simple way to skip tiles that don't have the wanted prio
		if ((curtile ^ prio) & 0x2000)
		{
			u32 npix = 8 - (xoff & 0x7);
			i += npix;
			buffer += npix;
			xoff += npix;
			goto _8x8_2_newtile;
		}
		
		colorval = 0;
		if (curtile & 0x4000) // hflip
		{
			if (tilepixels & 0x0001) colorval |= 0x01;
			if (tilepixels & 0x0100) colorval |= 0x02;
			tilepixels >>= 1;
		}
		else
		{
			if (tilepixels & 0x0080) colorval |= 0x01;
			if (tilepixels & 0x8000) colorval |= 0x02;
			tilepixels <<= 1;
		}
		
		if (colorval)
		{
			colorval |= (curtile & 0x1C00) >> 8;
			*buffer = pal[colorval];
		}
		buffer++;
		i++;
		
		xoff++;
		if (!(xoff & 0x7)) // reload tile if needed
		{
_8x8_2_newtile:
			idx = (xoff & 0xF8) >> 3;
			if (xoff & 0x100)
			{
				if (bg->Size & 0x1)
					idx += 1024;
			}
			curtile = tilemap[idx];
			
			idx = (curtile & 0x3FF) << 3;
			if (curtile & 0x8000) 	idx += (7 - tiley);
			else					idx += tiley;
			
			tilepixels = tileset[idx];
		}
	}
}

void PPU_RenderBG_4bpp_8x8(PPU_Background* bg, u16* buffer, u32 line, u16* pal, u32 prio)
{
	u16* tileset = bg->Tileset;
	u16* tilemap = bg->Tilemap;
	u32 xoff;
	u32 tiley;
	u16 curtile;
	u32 tilepixels;
	u32 colorval;
	u32 i;
	u32 idx;
	
	line += bg->YScroll;
	tiley = (line & 0x07);
	tilemap += (line & 0xF8) << 2;
	if (line & 0x100)
	{
		if (bg->Size & 0x2)
			tilemap += (bg->Size & 0x1) ? 2048 : 1024;
	}
	
	xoff = bg->XScroll;
	idx = (xoff & 0xF8) >> 3;
	if (xoff & 0x100)
	{
		if (bg->Size & 0x1)
			idx += 1024;
	}
	curtile = tilemap[idx];
	
	idx = (curtile & 0x3FF) << 4;
	if (curtile & 0x8000) 	idx += (7 - tiley);
	else					idx += tiley;
	
	tilepixels = tileset[idx] | (tileset[idx+8] << 16);
	if (curtile & 0x4000)	tilepixels >>= (xoff & 0x7);
	else					tilepixels <<= (xoff & 0x7);
	
	for (i = 0; i < 256;)
	{
		// simple way to skip tiles that don't have the wanted prio
		if ((curtile ^ prio) & 0x2000)
		{
			u32 npix = 8 - (xoff & 0x7);
			i += npix;
			buffer += npix;
			xoff += npix;
			goto _8x8_4_newtile;
		}
		
		colorval = 0;
		if (curtile & 0x4000) // hflip
		{
			if (tilepixels & 0x00000001) colorval |= 0x01;
			if (tilepixels & 0x00000100) colorval |= 0x02;
			if (tilepixels & 0x00010000) colorval |= 0x04;
			if (tilepixels & 0x01000000) colorval |= 0x08;
			tilepixels >>= 1;
		}
		else
		{
			if (tilepixels & 0x00000080) colorval |= 0x01;
			if (tilepixels & 0x00008000) colorval |= 0x02;
			if (tilepixels & 0x00800000) colorval |= 0x04;
			if (tilepixels & 0x80000000) colorval |= 0x08;
			tilepixels <<= 1;
		}
		
		if (colorval)
		{
			colorval |= (curtile & 0x1C00) >> 6;
			*buffer = pal[colorval];
		}
		buffer++;
		i++;
		
		xoff++;
		if (!(xoff & 0x7)) // reload tile if needed
		{
_8x8_4_newtile:
			idx = (xoff & 0xF8) >> 3;
			if (xoff & 0x100)
			{
				if (bg->Size & 0x1)
					idx += 1024;
			}
			curtile = tilemap[idx];
			
			idx = (curtile & 0x3FF) << 4;
			if (curtile & 0x8000) 	idx += (7 - tiley);
			else					idx += tiley;
			
			tilepixels = tileset[idx] | (tileset[idx+8] << 16);
		}
	}
}


void PPU_RenderOBJ(u8* oam, u32 oamextra, u32 ymask, u16* buffer, u32 line, u16* pal)
{
	u16* tileset = PPU_OBJTileset;
	s32 xoff;
	u16 attrib;
	u32 tilepixels;
	u32 colorval;
	u32 idx;
	u32 i;
	u8 width = PPU_OBJWidth[(oamextra & 0x2) >> 1];
	
	attrib = *(u16*)&oam[2];
	
	idx = (attrib & 0x01FF) << 4;
	
	if (attrib & 0x8000) line = ymask - line;
	idx += (line & 0x07) | ((line & 0x38) << 5);
	
	if (attrib & 0x4000)
		idx += ((width-1) & 0x38) << 1;
	
	xoff = oam[0];
	if (oamextra & 0x1) // xpos bit8, sign bit
	{
		xoff = 0x100 - xoff;
		if (xoff >= width) return;
		i = 0;
		
		if (attrib & 0x4000) 	idx -= ((xoff & 0x38) << 1);
		else 					idx += ((xoff & 0x38) << 1);
	}
	else
	{
		i = xoff;
		buffer += xoff;
		xoff = 0;
	}
	
	tilepixels = tileset[idx] | (tileset[idx+8] << 16);
	if (attrib & 0x4000)	tilepixels >>= (xoff & 0x7);
	else					tilepixels <<= (xoff & 0x7);
	
	pal += (oam[3] & 0x0E) << 3;
	
	while (xoff < width)
	{
		colorval = 0;
		if (attrib & 0x4000) // hflip
		{
			if (tilepixels & 0x00000001) colorval |= 0x01;
			if (tilepixels & 0x00000100) colorval |= 0x02;
			if (tilepixels & 0x00010000) colorval |= 0x04;
			if (tilepixels & 0x01000000) colorval |= 0x08;
			tilepixels >>= 1;
		}
		else
		{
			if (tilepixels & 0x00000080) colorval |= 0x01;
			if (tilepixels & 0x00008000) colorval |= 0x02;
			if (tilepixels & 0x00800000) colorval |= 0x04;
			if (tilepixels & 0x80000000) colorval |= 0x08;
			tilepixels <<= 1;
		}
		
		if (colorval)
		{
			*buffer = pal[colorval];
		}
		buffer++;
		
		i++;
		if (i >= 256) return;
		
		xoff++;
		if (!(xoff & 0x7)) // reload tile if needed
		{
			if (attrib & 0x4000) 	idx -= 16;
			else 					idx += 16;
			
			tilepixels = tileset[idx] | (tileset[idx+8] << 16);
		}
	}
}

inline void PPU_RenderOBJs(u16* buf, u32 line, u32 prio)
{
	int i = PPU_FirstOBJ;
	i--;
	if (i < 0) i = 127;
	int last = i;
	
	do
	{
		u8* oam = &PPU_OAM[i << 2];
		if ((oam[3] & 0x30) == prio)
		{
			u8 oamextra = PPU_OAM[0x200 + (i >> 2)] >> ((i & 0x03) << 1);
			u8 oy = oam[1] + 1;
			u8 oh = PPU_OBJHeight[(oamextra & 0x2) >> 1];
			
			if (line >= oy && line < (oy+oh))
				PPU_RenderOBJ(oam, oamextra, oh-1, buf, line-oy, &PPU_CGRAM[128]);
		}
		
		i--;
		if (i < 0) i = 127;
	}
	while (i != last);
}


int blarg=0;
void PPU_RenderScanline(u32 line)
{
	// test frameskip code
	if (line==0) blarg=!blarg;
	if (blarg)
	{
		return;
	}
	
	int i;
	u16* buf = &PPU_MainBuffer[0];
	
	u32 backdrop = PPU_CGRAM[0];
	backdrop |= (backdrop << 16);
	for (i = 0; i < 256; i += 2)
		*(u32*)&buf[i] = backdrop;
		
	switch (PPU_Mode & 0x07)
	{
		case 1:
			PPU_RenderBG_2bpp_8x8(&PPU_BG[2], buf, line, &PPU_CGRAM[0], PRIO_LOW);
			PPU_RenderOBJs(buf, line, 0x00);
			
			if (!(PPU_Mode & 0x08))
				PPU_RenderBG_2bpp_8x8(&PPU_BG[2], buf, line, &PPU_CGRAM[0], PRIO_HIGH);
			PPU_RenderOBJs(buf, line, 0x10);
			
			PPU_RenderBG_4bpp_8x8(&PPU_BG[1], buf, line, &PPU_CGRAM[0], PRIO_LOW);
			PPU_RenderBG_4bpp_8x8(&PPU_BG[0], buf, line, &PPU_CGRAM[0], PRIO_LOW);
			PPU_RenderOBJs(buf, line, 0x20);
			
			PPU_RenderBG_4bpp_8x8(&PPU_BG[1], buf, line, &PPU_CGRAM[0], PRIO_HIGH);
			PPU_RenderBG_4bpp_8x8(&PPU_BG[0], buf, line, &PPU_CGRAM[0], PRIO_HIGH);
			PPU_RenderOBJs(buf, line, 0x30);
			
			if (PPU_Mode & 0x08)
				PPU_RenderBG_2bpp_8x8(&PPU_BG[2], buf, line, &PPU_CGRAM[0], PRIO_HIGH);
			
			break;
	}
	
	// copy to final framebuffer
	u16* finalbuf = &((u16*)TopFB)[17512 - line];
	for (i = 0; i < 256; i++)
	{
		*finalbuf = PPU_ColorTable[*buf++];
		finalbuf += 240;
	}
}

void PPU_VBlank()
{
	PPU_OAMAddr = PPU_OAMReload;
}