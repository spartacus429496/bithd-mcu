#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/gpio.h>
#include "util.h"
#include <libopencm3/stm32/f2/rng.h>
#include "../../libopencm3/include/libopencm3/cm3/nvic.h"
#include <libopencm3/cm3/systick.h>


#include "timer.h"
#include "timerbitpie.h"
//////////////////////定时器相关///////////////////////////////////
#define HWtimerFreq 100             //底层定时器频率（Hz）。                               *移植需要修改*
unsigned long  volatile timer0_sec_count= 0;    //定时器计数累加,根据底层定时器进行配合                *移植需要添加*
/////////////////////////////////////////////////////////////////////////
TimDispl Timerdisplaybuf;//init timer display buf
BalanceDispl Balancedisplaybuf;//init balance display bufer


void timer_init(void) {
	system_millis = 0;

	/*
	 * MCU clock (120 MHz) as source
	 *
	 *     (120 MHz / 8) = 15 clock pulses
	 *
	 */
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB_DIV8);
	STK_CVR = 0;

	/*
	 * 1 tick = 1 ms @ 120 MHz
	 *
	 *     (15 clock pulses * 1000 ms) = 15000 clock pulses
	 *
	 * Send an interrupt every (N - 1) clock pulses
	 */
	systick_set_reload(14999);

	/* SysTick as interrupt */
	systick_interrupt_enable();

	systick_counter_enable();
}

void sys_tick_handler(void) {
        gpio_toggle(GPIOB,GPIO5);
    timer0_sec_count++;
	system_millis++;
}











// //10ms interrupt
// void tim3_isr(void)
// {
//     gpio_toggle(GPIOB,GPIO5);
  
//   system_millis++;
//   timer_clear_flag(TIM3,0xffffffff);
// }



// /*
//  * Function Name:Timer init funtcion,every 10ms go interrupt TIM3
//  */
// void timer_init(void)
// {

//       // clock initialization
//   	rcc_periph_clock_enable(RCC_APB1ENR_TIM3EN);
//   	rcc_periph_clock_enable(RCC_TIM3);
//        timer_reset(TIM3); /* Time Base configuration */
//        // CK_INT = f_periph(TIM3=APB1) = 30[MHz]??          //RM0090.pdf
//            //____________________________________________________
//            // MODE SETTING
//            // - use internal clock as a trigger
//            timer_set_mode(TIM3, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
//            // TIM_CR1_CKD_CK_INT = Clock Division Ratio
//            //  - set CK_PSC = CK_INT = f_periph = 30[MHz]
//            // TIM_CR1_CMS_EDGE = Center-aligned Mode Selection
//            //  - edge..??
//            // TIM_CR1_DIR_UP = counting direction up

//            //0xfff 20000

//                // PSC = 0 --> CK_CNT = CK_PSC = 30[MHz]
//                // 168Mhz / 168k = 10kHz
//                //timer_set_prescaler(t, 0x0);
//                timer_set_prescaler(TIM3, 120);

//                // Input Filter clock prescaler -
//                //timer_set_clock_division(t, 0x0); // 30[MHz] down to 30[Mhz] .= 33.3 [ns]
//                timer_set_clock_division(TIM3,0); // 30[MHz] down to 30[Mhz] .= 33.3 [ns] -> no effect ??


//                // TIMx_ARR - Period in counter clock ticks.
//                timer_set_period(TIM3, 10000-1);//14100);//7057); // ctj?? 30MHz/3000Hz -1 ??? = theoreticly generates 100[ms] intervals
//                //timer_set_period(t, 20000-1); // ctj?? 30MHz/3000Hz -1 ??? = theoreticly generates 100[ms] intervals

//                /* Generate TRGO on every update. */
//                timer_set_master_mode(TIM3, TIM_CR2_MMS_UPDATE);

//                // start
//                timer_enable_counter(TIM3);
//                uint16_t irqs = 0 ;

//                irqs = TIM_DIER_CC1IE | TIM_DIER_BIE ;
//                //irqs = TIM_DIER_BIE;
//                //irqs = TIM_DIER_TIE;
//                //irqs = TIM_DIER_UIE; // update is the best :)
//                irqs = 0xFFFF;
//                timer_enable_irq(TIM3,irqs);

//            //____________________________________________________
//                // NVIC isr setting

//            	//exti_select_source(exti, port);
//            	//exti_set_trigger(exti, trig);
//            	//exti_enable_request(exti);

//                // enable interrupt in NestedVectorInterrupt
//                // -> if some of the timer interrupts is enabled -> it will call the tim isr function
//            	nvic_enable_irq(29);
//                    system_millis=0;
// }

//*********************虚拟定时器 时间间隔1秒**********************************/////////////
////////////////////////////////////////////////////////////////////////////////
/***************************************
函数名称:开启虚拟定时器
入参:T虚拟定时器相关参数存储地址，Delaytime
***************************************/
void TP_START(TimProg* T,unsigned long  Delaytime)
{
	T->T_0=timer0_sec_count;
	T->T_1=timer0_sec_count+(Delaytime*HWtimerFreq);  //底层定时器为1秒进入一次中断，记录1次为1S
}


/***************************************
函数名称:关闭虚拟定时器
入参:T虚拟定时器相关参数存储地址
***************************************/
void TP_CLOSE(TimProg* T)
{
	T->T_0=0;
	T->T_1=0;
}


/***************************************
函数名称:检查时钟是否到时
入参:T虚拟定时器相关参数存储地址
出参:0 到时
***************************************/
unsigned char CheckTP(TimProg* T)
{
    if((timer0_sec_count)<(T->T_1))
    	{
          return 1;
    	}
	return 0;
}



