#include <dos.h>
#include <string.h>
#include <stdio.h>
#include <conio.h>

#include "sound.h"

// PPI stands for Programmable Peripheral Interface (which is the Intel 8255A chip)
// The PPI ports are only for IBM PC and XT, however port A is mapped to the same
// I/O address as the Keyboard Controller's (Intel 8042 chip) output buffer for compatibility.
#define PPI_PORT_A 0x60
#define PPI_PORT_B 0x61
#define PPI_PORT_C 0x62
#define PPI_COMMAND_REGISTER 0x63

// 8253 Programmable Interval Timer ports and registers
// The clock input is a 1.19318 MHz signal
#define CLOCK_FREQ              1193180L
#define PIT_COMMAND_REGISTER    0x43
#define PIT_COUNT_REGISTER      0x42

#define TIMER_INTERRUPT_VECTOR  0x1C

#define START_SOUND(cnt)        {outp(PIT_COMMAND_REGISTER, 0xB6);  \
                                 outp(PIT_COUNT_REGISTER, (cnt));   \
                                 outp(PIT_COUNT_REGISTER, ((cnt)>>8));\
                                 outp(PPI_PORT_B, inp(PPI_PORT_B)|0x03);}
#define STOP_SOUND               outp(PPI_PORT_B, inp(PPI_PORT_B)&0xFC)

void interrupt (far *chainTimerISR)(void) = 0L;
t_timer timerTick           = 0;
struct s_tone *loopPlayHead = NULL;
struct s_tone *loopPlaying  = NULL;
struct s_tone *seqPlaying   = NULL;
unsigned int loopLenHead    = 0;
unsigned int loopLen        = 0;
unsigned int loopDuration   = 0;
unsigned int seqLen         = 0;
unsigned int seqDuration    = 0;

void SoundStart(unsigned int freq)
{
    unsigned int count;

    if (freq)
    {
        count = CLOCK_FREQ / freq;
        START_SOUND(count);
    }
    else
    {
        STOP_SOUND;
    }
}
void SoundStop(void)
{
    STOP_SOUND;
}
void SoundPrepSeq(unsigned int len, struct s_tone *seq)
{
    unsigned int i, freq, count;

    for (i = 0; i < len; i++)
    {
        freq = seq->Frequency;
        if (freq)
            count = CLOCK_FREQ / (long)freq;
        else
            count = 0;
        seq->Count = count;
        seq++;
    }
}
void SoundPlaySeq(unsigned int len, struct s_tone *seq)
{
    disable();
    seqPlaying = NULL;
    if (len && seq)
    {
        seqLen      = len;
        seqDuration = seq->Duration;
        if (seq->Count)
        {
            START_SOUND(seq->Count);
        }
        else
        {
            STOP_SOUND;
        }
        seqPlaying = seq;
    }
    else
    {
        STOP_SOUND;
    }
    enable();
}
void SoundBackgroundLoop(unsigned int len, struct s_tone *seq)
{
    disable();
    loopPlayHead = NULL;
    loopPlaying  = NULL;
    if (len && seq)
    {
        loopLenHead  = len;
        loopLen      = len;
        loopDuration = seq->Duration;
        if (seq->Count)
        {
            START_SOUND(seq->Count);
        }
        else
        {
            STOP_SOUND;
        }
        loopPlayHead = seq;
        loopPlaying  = seq;
    }
    else
    {
        STOP_SOUND;
    }
    enable();
}
void TimerReset(void)
{
    disable();
    timerTick = 0;
    enable();
}
t_timer TimerCount(void)
{
    t_timer tick;

    disable();
    tick = timerTick;
    enable();
    return tick;
}
void TimerDelay(t_timer ticks)
{
    if (ticks && chainTimerISR != 0L)
    {
        disable();
        timerTick = 0;
        enable();
        while (timerTick < ticks);
    }
}
void interrupt far TimerISR(void)
{
    ++timerTick;
    //
    // Check for playing note sequence
    //
    if (seqPlaying)
    {
        if (--seqDuration == 0)
        {
            //
            // Move to next note
            //
            if (--seqLen)
            {
                seqPlaying++;
                seqDuration = seqPlaying->Duration;
                if (seqPlaying->Count)
                {
                    START_SOUND(seqPlaying->Count);
                }
                else
                {
                    STOP_SOUND;
                }
            }
            else
            {
                seqPlaying = NULL;
                if (loopPlaying)
                {
                    //
                    // Jump back into background loop
                    //
                    if (loopPlaying->Count)
                    {
                        START_SOUND(loopPlaying->Count);
                    }
                    else
                    {
                        STOP_SOUND;
                    }
                }
                else
                {
                    STOP_SOUND;
                }
            }
        }
    }
    else if (loopPlaying)
    {
        if (--loopDuration == 0)
        {
            //
            // Move to next note
            //
            if (--loopLen)
            {
                loopPlaying++;
            }
            else
            {
                //
                // Restart loop
                //
                loopPlaying = loopPlayHead;
                loopLen     = loopLenHead;
            }
            loopDuration = loopPlaying->Duration;
            if (loopPlaying->Count)
            {
                START_SOUND(loopPlaying->Count);
            }
            else
            {
                STOP_SOUND;
            }
        }
    }
    //
    // Chain previous timer ISR
    //
    chainTimerISR();
}
void TimerInstall(void)
{
    if (chainTimerISR == 0L)
    {
        chainTimerISR = getvect(TIMER_INTERRUPT_VECTOR);
        setvect(TIMER_INTERRUPT_VECTOR, TimerISR);
    }
}

void TimerUninstall(void)
{
    if (chainTimerISR != 0L)
    {
        setvect(TIMER_INTERRUPT_VECTOR, chainTimerISR);
        chainTimerISR = 0L;
    }
    seqPlaying = NULL;
    STOP_SOUND;
}

