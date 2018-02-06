#ifndef _TIMERBITPIE_H
#define _TIMERBITPIE_H
#define balance_size 59
typedef struct Stm32TimerDisplay		//for display time
{
	unsigned short  Year;
	unsigned char  Month;
	unsigned char Day;
	unsigned char hours;
	unsigned char minute;
	unsigned char second;
	unsigned char chargingstatus;
	unsigned char bluetoothconnetstatus;
	unsigned char bat_vol;
} TimDispl;

extern TimDispl Timerdisplaybuf;

typedef struct Stm32BalanceDisplay		//for display time
{
	unsigned char coin_name[32];
	unsigned char balance[10];
	unsigned char balance_after_di[10];
	unsigned char Year[2];
	unsigned char  Month;
	unsigned char Day;
	unsigned char hours;
	unsigned char minute;
	unsigned char second;
} BalanceDispl;

extern BalanceDispl Balancedisplaybuf;

typedef struct TimerProgram		//虚拟定时器
{
	unsigned long  T_0;   //记录时间轴当前时刻
	unsigned long  T_1;   //存储闹钟时间
} TimProg;

void TP_START(TimProg* T,unsigned long  Delaytime);
void TP_CLOSE(TimProg* T);
unsigned char CheckTP(TimProg* T);

void timer_init(void);
#endif
