typedef unsigned int t_timer;
struct s_tone
{
    unsigned int Frequency;
    unsigned int Count;
    unsigned int Duration;
};
void SoundStart(unsigned int freq);
void SoundStop(void);
void SoundPrepSeq(unsigned int len, struct s_tone *seq);
void SoundBackgroundLoop(unsigned int len, struct s_tone *seq);
void SoundPlaySeq(unsigned int len, struct s_tone *seq);
void TimerReset(void);
t_timer TimerCount(void);
void TimerDelay(t_timer ticks);
void TimerInstall(void);
void TimerUninstall(void);

