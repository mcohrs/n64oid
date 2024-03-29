/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - exception.c                                             *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2002 Hacktarux                                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "exception.h"
#include "r4300.h"
#include "macros.h"
#include "recomph.h"

#include "api/m64p_types.h"
#include "api/callbacks.h"
#include "memory/memory.h"

extern unsigned int interp_addr;

void address_error_exception(void)
{
   DebugMessage(M64MSG_ERROR, "address_error_exception");
   stop=1;
}

void TLB_invalid_exception(void)
{
   if (delay_slot)
   {
     skip_jump = 1;
     DebugMessage(M64MSG_ERROR, "delay slot - TLB refill exception");
     stop=1;
   }
   DebugMessage(M64MSG_ERROR, "TLB invalid exception");
   stop=1;
}

void XTLB_refill_exception(unsigned long long int addresse)
{
   DebugMessage(M64MSG_ERROR, "XTLB refill exception");
   stop=1;
}

void TLB_refill_exception(unsigned int address, int w)
{
   int usual_handler = 0, i;

   if (r4300emu != CORE_DYNAREC && w != 2) update_count();
   if (w == 1) Cause = (3 << 2);
   else Cause = (2 << 2);
   BadVAddr = address;
   Context = (Context & 0xFF80000F) | ((address >> 9) & 0x007FFFF0);
   EntryHi = address & 0xFFFFE000;
   if (Status & 0x2) // Test de EXL
     {
    if (r4300emu == CORE_PURE_INTERPRETER) interp_addr = 0x80000180;
    else jump_to(0x80000180);
    if(delay_slot==1 || delay_slot==3) Cause |= 0x80000000;
    else Cause &= 0x7FFFFFFF;
     }
   else
     {
    if (r4300emu != CORE_PURE_INTERPRETER) 
      {
         if (w!=2)
           EPC = PC->addr;
         else
           EPC = address;
      }
    else EPC = interp_addr;
         
    Cause &= ~0x80000000;
    Status |= 0x2; //EXL=1
    
    if (address >= 0x80000000 && address < 0xc0000000)
      usual_handler = 1;
    for (i=0; i<32; i++)
      {
         if (/*tlb_e[i].v_even &&*/ address >= tlb_e[i].start_even &&
         address <= tlb_e[i].end_even)
           usual_handler = 1;
         if (/*tlb_e[i].v_odd &&*/ address >= tlb_e[i].start_odd &&
         address <= tlb_e[i].end_odd)
           usual_handler = 1;
      }
    if (usual_handler)
      {
         if (r4300emu == CORE_PURE_INTERPRETER) interp_addr = 0x80000180;
         else jump_to(0x80000180);
      }
    else
      {
         if (r4300emu == CORE_PURE_INTERPRETER) interp_addr = 0x80000000;
         else jump_to(0x80000000);
      }
     }
   if(delay_slot==1 || delay_slot==3)
     {
    Cause |= 0x80000000;
    EPC-=4;
     }
   else
     {
    Cause &= 0x7FFFFFFF;
     }
   if(w != 2) EPC-=4;
   
   if (r4300emu == CORE_PURE_INTERPRETER) last_addr = interp_addr;
   else last_addr = PC->addr;
   
   if (r4300emu == CORE_DYNAREC) 
     {
    dyna_jump();
    if (!dyna_interp) delay_slot = 0;
     }
   
   if (r4300emu != CORE_DYNAREC || dyna_interp)
     {
    dyna_interp = 0;
    if (delay_slot)
      {
         if (interp_addr) skip_jump = interp_addr;
         else skip_jump = PC->addr;
         next_interupt = 0;
      }
     }
}

void TLB_mod_exception(void)
{
   DebugMessage(M64MSG_ERROR, "TLB mod exception");
   stop=1;
}

void integer_overflow_exception(void)
{
   DebugMessage(M64MSG_ERROR, "integer overflow exception");
   stop=1;
}

void coprocessor_unusable_exception(void)
{
   DebugMessage(M64MSG_ERROR, "coprocessor_unusable_exception");
   stop=1;
}

void exception_general(void)
{
   update_count();
   Status |= 2;
   
   if (r4300emu != CORE_PURE_INTERPRETER) EPC = PC->addr;
   else EPC = interp_addr;
   
   if(delay_slot==1 || delay_slot==3)
     {
    Cause |= 0x80000000;
    EPC-=4;
     }
   else
     {
    Cause &= 0x7FFFFFFF;
     }
   if (r4300emu == CORE_PURE_INTERPRETER)
     {
    interp_addr = 0x80000180;
    last_addr = interp_addr;
     }
   else
     {
    jump_to(0x80000180);
    last_addr = PC->addr;
     }
   if (r4300emu == CORE_DYNAREC)
     {
    dyna_jump();
    if (!dyna_interp) delay_slot = 0;
     }
   if (r4300emu != CORE_DYNAREC || dyna_interp)
     {
    dyna_interp = 0;
    if (delay_slot)
      {
         if (r4300emu == CORE_PURE_INTERPRETER) skip_jump = interp_addr;
         else skip_jump = PC->addr;
         next_interupt = 0;
      }
     }
}

