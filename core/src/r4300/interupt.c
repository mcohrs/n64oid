/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - interupt.c                                              *
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

#include <stdlib.h>
#include <time.h>

#include <SDL.h>

#include "interupt.h"
#include "r4300.h"
#include "macros.h"
#include "exception.h"

#include "api/m64p_types.h"
#include "api/callbacks.h"
#include "memory/memory.h"
#include "main/rom.h"
#include "main/main.h"
#include "main/savestates.h"
#include "main/cheat.h"

#ifdef WITH_OSD
#include "osd/osd.h"
#endif

#include "plugin/plugin.h"

#ifdef WITH_LIRC
#include "main/lirc.h"
#endif

unsigned int next_vi;
int vi_field=0;
int vi_counter=0;
typedef struct _interupt_queue
{
   int type;
   unsigned int count;
   struct _interupt_queue *next;
} interupt_queue;

static interupt_queue *q = NULL;

void clear_queue()
{
    while(q != NULL)
    {
        interupt_queue *aux = q->next;
        free(q);
        q = aux;
    }
}

void print_queue()
{
    interupt_queue *aux;
    //if (Count < 0x7000000) return;
    DebugMessage(M64MSG_INFO, "------------------ 0x%x", (unsigned int)Count);
    aux = q;
    while (aux != NULL)
    {
        DebugMessage(M64MSG_INFO, "Count:%x, %x", (unsigned int)aux->count, aux->type);
        aux = aux->next;
    }
}

static int SPECIAL_done = 0;

int before_event(unsigned int evt1, unsigned int evt2, int type2)
{
    if(evt1 - Count < 0x80000000)
    {
        if(evt2 - Count < 0x80000000)
        {
            if((evt1 - Count) < (evt2 - Count)) return 1;
            else return 0;
        }
        else
        {
            if((Count - evt2) < 0x10000000)
            {
                switch(type2)
                {
                    case SPECIAL_INT:
                        if(SPECIAL_done) return 1;
                        else return 0;
                        break;
                    default:
                        return 0;
                }
            }
            else return 1;
        }
    }
    else return 0;
}

void add_interupt_event(int type, unsigned int delay)
{
    unsigned int count = Count + delay/**2*/;
    int special = 0;
   
    if(type == SPECIAL_INT /*|| type == COMPARE_INT*/) special = 1;
    if(Count > 0x80000000) SPECIAL_done = 0;
   
    if (get_event(type)) {
        DebugMessage(M64MSG_WARNING, "two events of type 0x%x in interrupt queue", type);
    }
    interupt_queue *aux = q;
   
    if (q == NULL)
    {
        q = malloc(sizeof(interupt_queue));
        q->next = NULL;
        q->count = count;
        q->type = type;
        next_interupt = q->count;
        //print_queue();
        return;
    }
   
    if(before_event(count, q->count, q->type) && !special)
    {
        q = malloc(sizeof(interupt_queue));
        q->next = aux;
        q->count = count;
        q->type = type;
        next_interupt = q->count;
        //print_queue();
        return;
    }
   
    while (aux->next != NULL && (!before_event(count, aux->next->count, aux->next->type) || special))
        aux = aux->next;
   
    if (aux->next == NULL)
    {
        aux->next = malloc(sizeof(interupt_queue));
        aux = aux->next;
        aux->next = NULL;
        aux->count = count;
        aux->type = type;
    }
    else
    {
        interupt_queue *aux2;
        if (type != SPECIAL_INT)
            while(aux->next != NULL && aux->next->count == count)
                aux = aux->next;
        aux2 = aux->next;
        aux->next = malloc(sizeof(interupt_queue));
        aux = aux->next;
        aux->next = aux2;
        aux->count = count;
        aux->type = type;
    }
}

void add_interupt_event_count(int type, unsigned int count)
{
    add_interupt_event(type, (count - Count)/*/2*/);
}

void remove_interupt_event()
{
    interupt_queue *aux = q->next;
    if(q->type == SPECIAL_INT) SPECIAL_done = 1;
    free(q);
    q = aux;
    if (q != NULL && (q->count > Count || (Count - q->count) < 0x80000000))
        next_interupt = q->count;
    else
        next_interupt = 0;
}

unsigned int get_event(int type)
{
    interupt_queue *aux = q;
    if (q == NULL) return 0;
    if (q->type == type)
        return q->count;
    while (aux->next != NULL && aux->next->type != type)
        aux = aux->next;
    if (aux->next != NULL)
        return aux->next->count;
    return 0;
}

int get_next_event_type()
{
    if (q == NULL) return 0;
    return q->type;
}

void remove_event(int type)
{
    interupt_queue *aux = q;
    if (q == NULL) return;
    if (q->type == type)
    {
        aux = aux->next;
        free(q);
        q = aux;
        return;
    }
    while (aux->next != NULL && aux->next->type != type)
        aux = aux->next;
    if (aux->next != NULL) // it's a type int
    {
        interupt_queue *aux2 = aux->next->next;
        free(aux->next);
        aux->next = aux2;
    }
}

void translate_event_queue(unsigned int base)
{
    interupt_queue *aux;
    remove_event(COMPARE_INT);
    remove_event(SPECIAL_INT);
    aux=q;
    while (aux != NULL)
    {
        aux->count = (aux->count - Count)+base;
        aux = aux->next;
    }
    add_interupt_event_count(COMPARE_INT, Compare);
    add_interupt_event_count(SPECIAL_INT, 0);
}

int save_eventqueue_infos(char *buf)
{
    int len = 0;
    interupt_queue *aux = q;
    if (q == NULL)
    {
        *((unsigned int*)&buf[0]) = 0xFFFFFFFF;
        return 4;
    }
    while (aux != NULL)
    {
        memcpy(buf+len  , &aux->type , 4);
        memcpy(buf+len+4, &aux->count, 4);
        len += 8;
        aux = aux->next;
    }
    *((unsigned int*)&buf[len]) = 0xFFFFFFFF;
    return len+4;
}

void load_eventqueue_infos(char *buf)
{
    int len = 0;
    clear_queue();
    while (*((unsigned int*)&buf[len]) != 0xFFFFFFFF)
    {
        int type = *((unsigned int*)&buf[len]);
        unsigned int count = *((unsigned int*)&buf[len+4]);
        add_interupt_event_count(type, count);
        len += 8;
    }
}

void init_interupt()
{
    SPECIAL_done = 1;
    next_vi = next_interupt = 5000;
    vi_register.vi_delay = next_vi;
    vi_field = 0;
    clear_queue();
    add_interupt_event_count(VI_INT, next_vi);
    add_interupt_event_count(SPECIAL_INT, 0);
}

void check_interupt()
{
    if (MI_register.mi_intr_reg & MI_register.mi_intr_mask_reg)
        Cause = (Cause | 0x400) & 0xFFFFFF83;
    else
        Cause &= ~0x400;
    if ((Status & 7) != 1) return;
    if (Status & Cause & 0xFF00)
    {
        if(q == NULL)
        {
            q = malloc(sizeof(interupt_queue));
            q->next = NULL;
            q->count = Count;
            q->type = CHECK_INT;
        }
        else
        {
            interupt_queue* aux = malloc(sizeof(interupt_queue));
            aux->next = q;
            aux->count = Count;
            aux->type = CHECK_INT;
            q = aux;
        }
        next_interupt = Count;
    }
}

void gen_interupt()
{

    if (stop == 1)
    {
        vi_counter = 0; // debug
        dyna_stop();
    }
    if (savestates_job & LOADSTATE) 
    {
        savestates_load();
        savestates_job &= ~LOADSTATE;
        return;
    }
   
    if (skip_jump)
    {
        if (q->count > Count || (Count - q->count) < 0x80000000)
            next_interupt = q->count;
        else
            next_interupt = 0;
        if (r4300emu == CORE_PURE_INTERPRETER)
        {
             interp_addr = skip_jump;
             last_addr = interp_addr;
        }
        else
        {
            unsigned int dest = skip_jump;
            skip_jump=0;
            jump_to(dest);
            last_addr = PC->addr;
        }
        skip_jump=0;
        return;
    } 

    switch(q->type)
    {
        case SPECIAL_INT:
            if (Count > 0x10000000) return;
            remove_interupt_event();
            add_interupt_event_count(SPECIAL_INT, 0);
            return;
            break;
        case VI_INT:
            if(vi_counter < 60)
            {
                if (vi_counter == 0)
                    cheat_apply_cheats(ENTRY_BOOT);
                vi_counter++;
            }
            else
            {
                cheat_apply_cheats(ENTRY_VI);
            }
            updateScreen();
#ifdef WITH_LIRC
            lircCheckInput();
#endif
            SDL_PumpEvents();
            refresh_stat();

            // if paused, poll for input events
            if(rompause)
            {
#ifdef WITH_OSD
                osd_render();  // draw Paused message in case updateScreen didn't do it
#endif
                SDL_GL_SwapBuffers();
                while(rompause)
                {
#ifdef __WIN32__
                    Sleep(10);
#else
                    struct timespec ts;
                    ts.tv_sec = 0;
                    ts.tv_nsec = 10000000;
                    nanosleep(&ts, NULL); // sleep for 10 milliseconds
#endif
                    SDL_PumpEvents();
#ifdef WITH_LIRC
                    lircCheckInput();
#endif //WITH_LIRC
                }
            }

            new_vi();
            if (vi_register.vi_v_sync == 0) vi_register.vi_delay = 500000;
            else vi_register.vi_delay = ((vi_register.vi_v_sync + 1)*1500);
            next_vi += vi_register.vi_delay;
            if (vi_register.vi_status&0x40) vi_field=1-vi_field;
            else vi_field=0;

            remove_interupt_event();
            add_interupt_event_count(VI_INT, next_vi);
    
            MI_register.mi_intr_reg |= 0x08;
            if (MI_register.mi_intr_reg & MI_register.mi_intr_mask_reg)
                Cause = (Cause | 0x400) & 0xFFFFFF83;
            else
                return;
            if ((Status & 7) != 1) return;
            if (!(Status & Cause & 0xFF00)) return;
            break;
    
        case COMPARE_INT:
            remove_interupt_event();
            Count+=2;
            add_interupt_event_count(COMPARE_INT, Compare);
            Count-=2;
    
            Cause = (Cause | 0x8000) & 0xFFFFFF83;
            if ((Status & 7) != 1) return;
            if (!(Status & Cause & 0xFF00)) return;
            break;
    
        case CHECK_INT:
            remove_interupt_event();
            break;
    
        case SI_INT:
#ifdef WITH_LIRC
            lircCheckInput();
#endif //WITH_LIRC
            SDL_PumpEvents();
            PIF_RAMb[0x3F] = 0x0;
            remove_interupt_event();
            MI_register.mi_intr_reg |= 0x02;
            si_register.si_stat |= 0x1000;
            if (MI_register.mi_intr_reg & MI_register.mi_intr_mask_reg)
                Cause = (Cause | 0x400) & 0xFFFFFF83;
            else
                return;
            if ((Status & 7) != 1) return;
            if (!(Status & Cause & 0xFF00)) return;
            break;
    
        case PI_INT:
            remove_interupt_event();
            MI_register.mi_intr_reg |= 0x10;
            pi_register.read_pi_status_reg &= ~3;
            if (MI_register.mi_intr_reg & MI_register.mi_intr_mask_reg)
                Cause = (Cause | 0x400) & 0xFFFFFF83;
            else
                return;
            if ((Status & 7) != 1) return;
            if (!(Status & Cause & 0xFF00)) return;
            break;
    
        case AI_INT:
            if (ai_register.ai_status & 0x80000000) // full
            {
                unsigned int ai_event = get_event(AI_INT);
                remove_interupt_event();
                ai_register.ai_status &= ~0x80000000;
                ai_register.current_delay = ai_register.next_delay;
                ai_register.current_len = ai_register.next_len;
                add_interupt_event_count(AI_INT, ai_event+ai_register.next_delay);
         
                MI_register.mi_intr_reg |= 0x04;
                if (MI_register.mi_intr_reg & MI_register.mi_intr_mask_reg)
                    Cause = (Cause | 0x400) & 0xFFFFFF83;
                else
                    return;
                if ((Status & 7) != 1) return;
                if (!(Status & Cause & 0xFF00)) return;
            }
            else
            {
                remove_interupt_event();
                ai_register.ai_status &= ~0x40000000;

                //-------
                MI_register.mi_intr_reg |= 0x04;
                if (MI_register.mi_intr_reg & MI_register.mi_intr_mask_reg)
                    Cause = (Cause | 0x400) & 0xFFFFFF83;
                else
                    return;
                if ((Status & 7) != 1) return;
                if (!(Status & Cause & 0xFF00)) return;
            }
            break;

        case SP_INT:
            remove_interupt_event();
            sp_register.sp_status_reg |= 0x303;
            //sp_register.signal1 = 1;
            sp_register.signal2 = 1;
            sp_register.broke = 1;
            sp_register.halt = 1;
    
            if (!sp_register.intr_break) return;
            MI_register.mi_intr_reg |= 0x01;
            if (MI_register.mi_intr_reg & MI_register.mi_intr_mask_reg)
                Cause = (Cause | 0x400) & 0xFFFFFF83;
            else
                return;
            if ((Status & 7) != 1) return;
            if (!(Status & Cause & 0xFF00)) return;
            break;
    
        case DP_INT:
            remove_interupt_event();
            dpc_register.dpc_status &= ~2;
            dpc_register.dpc_status |= 0x81;
            MI_register.mi_intr_reg |= 0x20;
            if (MI_register.mi_intr_reg & MI_register.mi_intr_mask_reg)
                Cause = (Cause | 0x400) & 0xFFFFFF83;
            else
                return;
            if ((Status & 7) != 1) return;
            if (!(Status & Cause & 0xFF00)) return;
            break;

        case HW2_INT:
            // Hardware Interrupt 2 -- remove interrupt event from queue
            remove_interupt_event();
            // setup r4300 Status flags: reset TS, and SR, set IM2
            Status = (Status & ~0x00380000) | 0x1000;
            Cause = (Cause | 0x1000) & 0xFFFFFF83;
            /* the exception_general() call below will jump to the interrupt vector (0x80000180) and setup the
             * interpreter or dynarec
             */
            break;

        case NMI_INT:
            // Non Maskable Interrupt -- remove interrupt event from queue
            remove_interupt_event();
            // setup r4300 Status flags: reset TS and SR, set BEV, ERL, and SR
            Status = (Status & ~0x00380000) | 0x00500004;
            Cause  = 0x00000000;
            // simulate the soft reset code which would run from the PIF ROM
            r4300_reset_soft();
            // clear all interrupts, reset interrupt counters back to 0
            Count = 0;
            vi_counter = 0;
            init_interupt();
            // clear the audio status register so that subsequent write_ai() calls will work properly
            ai_register.ai_status = 0;
            // reset the r4300 internal state
            if (r4300emu == CORE_PURE_INTERPRETER) /* pure interpreter only */
            {
                // set ErrorEPC with last instruction address and set next instruction address to reset vector
                ErrorEPC = interp_addr;
                interp_addr = 0xa4000040;
                last_addr = interp_addr;
            }
            else  /* decode-cached interpreter or dynamic recompiler */
            {
                int i;
                // clear all the compiled instruction blocks
                for (i=0; i<0x100000; i++)
                {
                    if (blocks[i])
                    {
                        if (blocks[i]->block) { free(blocks[i]->block); blocks[i]->block = NULL; }
                        if (blocks[i]->code) { free(blocks[i]->code); blocks[i]->code = NULL; }
                        if (blocks[i]->jumps_table) { free(blocks[i]->jumps_table); blocks[i]->jumps_table = NULL; }
                        if (blocks[i]->riprel_table) { free(blocks[i]->riprel_table); blocks[i]->riprel_table = NULL; }
                        free(blocks[i]);
                        blocks[i] = NULL;
                    }
                }
                // re-initialize
                init_blocks();
                // jump to the start
                ErrorEPC = PC->addr;
                jump_to(0xa4000040);
                last_addr = PC->addr;
            }
            // adjust ErrorEPC if we were in a delay slot, and clear the delay_slot and dyna_interp flags
            if(delay_slot==1 || delay_slot==3)
            {
                ErrorEPC-=4;
            }
            delay_slot = 0;
            dyna_interp = 0;
            return;

        default:
            remove_interupt_event();
            break;
    }

    exception_general();

    if(savestates_job & SAVESTATE)
    {
        if(savestates_job & SAVEPJ64STATE)
        {
            if(savestates_save_pj64() != -1)
            {
            savestates_job &= ~(SAVESTATE+SAVEPJ64STATE);
            }
        }
        else
        {
            savestates_save();
            savestates_job &= ~SAVESTATE;
        }
    }
}

