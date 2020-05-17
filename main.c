#include "stm32f0xx.h"
#include "stm32f0_discovery.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

//void generic_lcd_startup(void);
uint16_t dispmem_top[34] = {
        0x080 + 0,
        0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220,
        0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220,
        0x080 + 64,
        0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220,
        0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220,
};
uint16_t dispmem_bot[34] = {
        0x080 + 0,
        0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220,
        0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220,
        0x080 + 64,
        0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220,
        0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220,
};

void setup_wave(void);
void setup_dac(void);
void setup_tim(void);
void setup_gpio(void);
void setup_tim15(void);
void update_color(int);
void note(int);
void trigger(void);

int offset[26] = {0};
int ind[26] = {0};
int countt = 0;
int timer = 0;
char ans[4];
int flag= 1 , flag1 = 0 , flag2, flag3= 0;

int sixteenthtime = 0;
int16_t wavetable[1000];
int step[26] = {440 * 1000 / 12500 * (1 << 16), 493.88 * 1000 / 12500 * (1 << 16), 523.25 * 1000 / 12500 * (1 << 16), 587.33 * 1000 / 12500 * (1 << 16),
        659.26 * 1000 / 12500 * (1 << 16), 698.46 * 1000 / 12500 * (1 << 16), 783.99 * 1000 / 12500 * (1 << 16), 880 * 1000 / 12500 * (1 << 16), 0,
        (1209+ 697) * 1000 / 12500 * (1 << 16), (1336+697) * 1000 / 12500 * (1 << 16), (1477+ 697) * 1000 / 12500 * (1 << 16), (1633+ 697) * 1000 / 12500 * (1 << 16),
        (1209+ 770) * 1000 / 12500 * (1 << 16), (1336+770) * 1000 / 12500 * (1 << 16), (1477+ 770) * 1000 / 12500 * (1 << 16), (1633+ 770) * 1000 / 12500 * (1 << 16),
        (1209+ 852) * 1000 / 12500 * (1 << 16), (1336+852) * 1000 / 12500 * (1 << 16), (1477+ 852) * 1000 / 12500 * (1 << 16), (1633+ 852) * 1000 / 12500 * (1 << 16),
        (1209+ 941) * 1000 / 12500 * (1 << 16), (1336+941) * 1000 / 12500 * (1 << 16), (1477+ 941) * 1000 / 12500 * (1 << 16), (1633+ 941) * 1000 / 12500 * (1 << 16),
        };
//===================================Sound=================================================================
void setup_wave(void)
{
    int x;
    for(x=0; x < 1000; x += 1)
    {
        wavetable[x] = 32767 * sin(x * 2 * M_PI / 1000);
    }
}

void setup_gpio(void)
{
    RCC -> AHBENR |= RCC_AHBENR_GPIOAEN;
    GPIOA -> MODER &= ~0x3f0003;
    GPIOA -> MODER |= 0x2a0300;
    GPIOA -> AFR[1] &= ~(0xf << (4 * (8 - 8)));
    GPIOA -> AFR[1] &= ~(0xf << (4 * (9 - 8)));
    GPIOA -> AFR[1] &= ~(0xf << (4 * (10 - 8)));
    GPIOA -> AFR[1] |= 0x2 << (4 * (8 - 8));
    GPIOA -> AFR[1] |= 0x2 << (4 * (9 - 8));
    GPIOA -> AFR[1] |= 0x2 << (4 * (10 - 8));
}

void setup_dac(void)
{
    RCC -> APB1ENR |= RCC_APB1ENR_DACEN;
    DAC -> CR &= ~DAC_CR_EN1;
    DAC -> CR &= ~DAC_CR_BOFF1;
    DAC -> CR |= DAC_CR_TSEL1;
    DAC -> CR |= DAC_CR_TEN1;
    DAC -> CR |= DAC_CR_EN1;
}

void TIM15_IRQHandler()
{
    TIM15 -> SR &= ~TIM_SR_UIF;
    trigger();
}

void TIM3_IRQHandler()
{
    TIM3 -> SR &= ~TIM_SR_UIF;
    if(sixteenthtime >= 256)
    {
        sixteenthtime = 0;
    }
    note(sixteenthtime);
    sixteenthtime += 1;
}

void TIM1_BRK_UP_TRG_COM_IRQHandler()
{
    TIM1 -> SR &= ~TIM_SR_UIF;

    if(flag3== 0)
    {
        countt++;
    }

    update_color(countt);
}

void update_color(int countt)
{
    if(countt >= 14400)
    {

        TIM1 -> CCR1 = 80;                 //Red
        TIM1 -> CCR2 = 100;                //Green
    }
    else
    {
    	if((countt % 60)> 30)
    	{
            TIM1 -> CCR1 = 100 - countt / 400;  //Red
            TIM1 -> CCR2 = 64 + countt / 400;   //Green
    	}
    	else
    	{
            TIM1 -> CCR1 = 100;                 //Red
            TIM1 -> CCR2 = 100;                //Green
    	}
    	if(countt >= 12600 && countt < 13800)
    	{
    		TIM3 -> ARR = 39;
    	}
    	else if(countt >= 13800)
    	{
    		TIM3 -> ARR = 32;
    	}
    }
}

void trigger(void)
{
    int x;
    int sample = 0;
    DAC -> SWTRIGR |= DAC_SWTRIGR_SWTRIG1;
    for(x = 0; x < 26; x++)
    {
        if(ind[x])
        {
            offset[x] += step[x] / 2;

            if((offset[x] >> 16) >= 1000)
            {
                offset[x] -= 1000 << 16;
            }

            sample = wavetable[offset[x] >> 16];

        }
    }

    sample = sample+ 20 / 16 + 2048;

    if(sample > 4095)
    {
        sample = 4095;
    }
    else if(sample < 0)
    {
        sample = 0;
    }

    DAC -> DHR12R1 = sample;
}

void note(int time)
{
    if((time >= 0 && time <= 7) || (time >= 48 && time <= 55) || (time >= 88 && time <= 95) || (time >= 160 && time <= 171) || (time >= 176 && time <= 183) || (time >= 216 && time <= 223))
    {
        ind[0] = 0;
        ind[1] = 0;
        ind[2] = 0;
        ind[3] = 0;
        ind[4] = 1;
        ind[5] = 0;
        ind[6] = 0;
        ind[7] = 0;
        ind[8] = 0;
    }
    else if((time >= 16 && time <= 23) || (time >= 56 && time <= 59) || (time >= 80 && time <= 87) || (time >= 132 && time <= 139) || (time >= 184 && time <= 187) || (time >= 208 && time <= 215))
    {
        ind[0] = 0;
        ind[1] = 0;
        ind[2] = 0;
        ind[3] = 1;
        ind[4] = 0;
        ind[5] = 0;
        ind[6] = 0;
        ind[7] = 0;
        ind[8] = 0;
    }
    else if((time >= 12 && time <= 15) || (time >= 24 && time <= 27) || (time >= 44 && time <= 47) || (time >= 60 && time <= 63) || (time >= 76 && time <= 79) || (time >= 96 && time <= 103) || (time >= 172 && time <= 175)|| (time >= 188 && time <= 191) || (time >= 204 && time <= 207) || (time >= 224 && time <= 231))
    {
        ind[0] = 0;
        ind[1] = 0;
        ind[2] = 1;
        ind[3] = 0;
        ind[4] = 0;
        ind[5] = 0;
        ind[6] = 0;
        ind[7] = 0;
        ind[8] = 0;
    }
    else if((time >= 8 && time <= 11) || (time >= 28 && time <= 31) || (time >= 64 && time <= 75) || (time >= 172 && time <= 178) || (time >= 192 && time <= 198) || (time >= 200 && time <= 203))
    {
        ind[0] = 0;
        ind[1] = 1;
        ind[2] = 0;
        ind[3] = 0;
        ind[4] = 0;
        ind[5] = 0;
        ind[6] = 0;
        ind[7] = 0;
        ind[8] = 0;
    }
    else if((time >= 32 && time <= 43) || (time >= 104 && time <= 110) || (time >= 112 && time <= 131) || (time >= 232 && time <= 238) || (time >= 240 && time <= 255))
    {
        ind[0] = 1;
        ind[1] = 0;
        ind[2] = 0;
        ind[3] = 0;
        ind[4] = 0;
        ind[5] = 0;
        ind[6] = 0;
        ind[7] = 0;
        ind[8] = 0;
    }
    else if((time >= 140 && time <= 143) || (time >= 156 && time <= 159))
    {
        ind[0] = 0;
        ind[1] = 0;
        ind[2] = 0;
        ind[3] = 0;
        ind[4] = 0;
        ind[5] = 1;
        ind[6] = 0;
        ind[7] = 0;
        ind[8] = 0;
    }
    else if(time >= 152 && time <= 155)
    {
        ind[0] = 0;
        ind[1] = 0;
        ind[2] = 0;
        ind[3] = 0;
        ind[4] = 0;
        ind[5] = 0;
        ind[6] = 1;
        ind[7] = 0;
        ind[8] = 0;
    }
    else if(time >= 144 && time <= 151)
    {
        ind[0] = 0;
        ind[1] = 0;
        ind[2] = 0;
        ind[3] = 0;
        ind[4] = 0;
        ind[5] = 0;
        ind[6] = 0;
        ind[7] = 1;
        ind[8] = 0;
    }
    else if(time == 111 || time == 199 || time == 239)
    {
        ind[0] = 0;
        ind[1] = 0;
        ind[2] = 0;
        ind[3] = 0;
        ind[4] = 0;
        ind[5] = 0;
        ind[6] = 0;
        ind[7] = 0;
        ind[8] = 1;

    }
    ind[9]= 0;//1
    ind[10]= 0;//2
    ind[11]= 0;//3
    ind[12]= 0;//A
    ind[13]= 0;//4
    ind[14]= 0;//5
    ind[15]= 0;//6
    ind[16]= 0;//B
    ind[17]= 0;//7
    ind[18]= 0;//8
    ind[19]= 0;//9
    ind[20]= 0;//C
    ind[21]= 0;//*
    ind[22]= 0;//0
    ind[23]= 0;//#
    ind[24]= 0;//D
}

void setup_tim15(void)
{
    RCC -> APB2ENR |= RCC_APB2ENR_TIM15EN;
    TIM15 -> ARR = 79;
    TIM15 -> PSC = 47;
    TIM15 -> CR1 |= TIM_CR1_ARPE;
    TIM15 -> DIER |= TIM_DIER_UIE;
    TIM15 -> CR1 |= TIM_CR1_CEN;
    NVIC -> ISER[0] = 1 << TIM15_IRQn;
}

void setup_tim(void)
{
    RCC -> APB1ENR |= RCC_APB1ENR_TIM3EN;
    TIM3 -> ARR = 49;
    TIM3 -> PSC = 59999;
    TIM3 -> CR1 |= TIM_CR1_ARPE;
    TIM3 -> DIER |= TIM_DIER_UIE;
    NVIC -> ISER[0] = 1 << TIM3_IRQn;

    RCC -> APB2ENR |= RCC_APB2ENR_TIM1EN;
    TIM1 -> CR1 &= ~TIM_CR1_DIR;
    TIM1 -> CCR1 = 100;                     //Red
    TIM1 -> CCR2 = 64;                      //Green
    TIM1 -> CCR3 = 100;                     //Blue
    TIM1 -> PSC = 7999;
    TIM1 -> ARR = 99;
    TIM1 -> CCMR1 &= ~TIM_CCMR1_OC1M_0;
    TIM1 -> CCMR1 |= TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | 0x8;
    TIM1 -> CCMR1 &= ~TIM_CCMR1_OC2M_0;
    TIM1 -> CCMR1 |= TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1 | 0x800;
    TIM1 -> CCMR2 &= ~TIM_CCMR2_OC3M_0;
    TIM1 -> CCMR2 |= TIM_CCMR2_OC3M_2 | TIM_CCMR2_OC3M_1 | 0x8;
    TIM1 -> CCER |= TIM_CCER_CC1E;
    TIM1 -> CCER |= TIM_CCER_CC2E;
    TIM1 -> CCER |= TIM_CCER_CC3E;
    TIM1 -> BDTR |= TIM_BDTR_MOE;
    TIM1 -> DIER |= TIM_DIER_UIE;
    TIM1 -> CR1 |= TIM_CR1_CEN;
    NVIC -> ISER[0] = 1 << TIM1_BRK_UP_TRG_COM_IRQn;
}

//===================================Key Pad=================================================================
int col = 0;
int8_t history[16] = {0};
int8_t lookup[16] = {1,4,7,0xe,2,5,8,0,3,6,9,0xf,0xa,0xb,0xc,0xd};
char char_lookup[16] = {'1','4','7','*','2','5','8','0','3','6','9','#','A','B','C','D'};

void init_keypad(){ // GPIOC for keypad and the timer (TIM6) for reading
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;
    // output pin and input pin for determine the keys on keypad
    GPIOC->MODER |= GPIO_MODER_MODER0_0 | GPIO_MODER_MODER1_0 | GPIO_MODER_MODER2_0 | GPIO_MODER_MODER3_0;
    GPIOC->PUPDR |= GPIO_PUPDR_PUPDR4_1 | GPIO_PUPDR_PUPDR5_1 | GPIO_PUPDR_PUPDR6_1 | GPIO_PUPDR_PUPDR7_1;
    // timer stuff
    TIM6->PSC = 47;
    TIM6->ARR = 999;
    TIM6->DIER |= TIM_DIER_UIE;// interrupt
    TIM6->CR1 |= TIM_CR1_CEN; // start running
    NVIC->ISER[0] |= 1 << TIM6_DAC_IRQn;
}

void TIM6_DAC_IRQHandler(){
    TIM6->SR &= ~TIM_SR_UIF; // acknowledge interrupt;
    if(timer > 0)//sound
    {
        timer -= 1;
    }
    int row;
    row = (GPIOC->IDR >> 4) & 0xf;
    int index = col << 2;
    // set history for checking if pressed and debouncing
    history[index]= history[index]<< 1;
    history[index+ 1]= history[index+ 1]<< 1;
    history[index+ 2]= history[index+ 2]<< 1;
    history[index+ 3]= history[index+ 3]<< 1;
    history[index] |= row & 0x1;
    history[index+ 1] |= (row>> 1) & 0x1;
    history[index+ 2] |= (row>> 2) & 0x1;
    history[index+ 3] |= (row>> 3) & 0x1;

    col++;
    if(col> 3) col= 0;
    GPIOC->ODR = (1 << col);
    if (flag1) countdown();
}

int get_key_press() {
    while(1) {
        for(int i = 0; i < 16; i++) {
            if(history[i] == 1) {
                return i;
            }
        }
    }
}

int get_key_release() {
    while(1) {
        for(int i = 0; i < 16; i++) {
            if(history[i] == -2) {
                return i;
            }
        }
    }
}

int get_key_pressed() {
    int key = get_key_press();
    while(key != get_key_release());
    return key;
}

char line1_top[16] = {"Count Down: 4:00"};
char line2_top[17] = {"     READY?     "};
char line1_bot[16] = {" Host :"};
char line2_bot[16] = {"Player:    "};
int guess= 0;

int get_user_val() {
    int freq = 0;
    int pos = 0;
    while(1) {
        int index = get_key_pressed();
        int key = lookup[index];
        if(key == 0x0d){
        	if (!flag1) continue;
//        	guess++;
//    		sprintf(line2_top , "  Guesses: %03d  " , guess);
//    		display2_top(line2_top);
        	break;
        }
        if(key == 0x0c){
        	if (!flag1) continue;
//            for (int i = 7 ; i < 16 ; i++) line2_top[i] = ' ';
//            display2_top(line2_top);
            for (int i = 7 ; i < 16 ; i++) line2_bot[i] = ' ';///////////////lcd2
            display2_bot(line2_bot);

            pos = 0;
        }
        if(key == 0x0f) //#
        {
        	if (!flag1 && !flag) continue;
            if((TIM1 -> CR1 & TIM_CR1_CEN) != 1 && flag == 1)
            {
                    flag = 0;
                    setup_tim();
                    display2_top("     START!     ");
            }

			TIM3 -> CR1 |= TIM_CR1_CEN;
			timer = 20;
			if (!flag1 && !flag) break;
        }
        if(key == 0x0e)//*
        {
        	if(flag) continue;
        		flag1 = !flag1;
                if((TIM3 -> CR1 & TIM_CR1_CEN) == 1)
                {

                        ind[0] = 0;
                        ind[1] = 0;
                        ind[2] = 0;
                        ind[3] = 0;
                        ind[4] = 0;
                        ind[5] = 0;
                        ind[6] = 0;
                        ind[7] = 0;
                        ind[8] = 1;
                        ind[9]= 0;//1
                        ind[10]= 0;//2
                        ind[11]= 0;//3
                        ind[12]= 0;//A
                        ind[13]= 0;//4
                        ind[14]= 0;//5
                        ind[15]= 0;//6
                        ind[16]= 0;//B
                        ind[17]= 0;//7
                        ind[18]= 0;//8
                        ind[19]= 0;//9
                        ind[20]= 0;//C
                        ind[21]= 0;//*
                        ind[22]= 0;//0
                        ind[23]= 0;//#
                        ind[24]= 0;//D
                        flag3= 1;

                        TIM3 -> CR1 &= ~TIM_CR1_CEN;
                        timer = 20;
                        display2_top("     PAUSED!   ");
                    }
                else
                {
                        TIM3 -> CR1 |= TIM_CR1_CEN;
                        flag3= 0;
                        timer = 20;
                        display2_top("     START!     ");
                }
        }
        if(key == 0x0a){
        	if (!flag1) continue;

//            if(pos < 5)  line2_top[6+pos] = 'A';
//            display2_top(line2_top);


            ind[9]= 0;//1
            ind[10]= 0;//2
            ind[11]= 0;//3
            ind[12]= 1;//A
            ind[13]= 0;//4
            ind[14]= 0;//5
            ind[15]= 0;//6
            ind[16]= 0;//B
            ind[17]= 0;//7
            ind[18]= 0;//8
            ind[19]= 0;//9
            ind[22]= 0;//0
        }
        if(key == 0x0b){
        	if (!flag1) continue;

//            if(pos < 5) line2_top[6+pos] = 'B';
//            display2_top(line2_top);



            ind[9]= 0;//1
            ind[10]= 0;//2
            ind[11]= 0;//3
            ind[12]= 0;//A
            ind[13]= 0;//4
            ind[14]= 0;//5
            ind[15]= 0;//6
            ind[16]= 1;//B
            ind[17]= 0;//7
            ind[18]= 0;//8
            ind[19]= 0;//9
            ind[22]= 0;//0
        }
        if(key >= 0 && key <= 9) {
        	if (!flag1) continue;
            pos++;
//            if(pos < 5)
//                line2_top[6+pos] = key + '0';
//            display2_top(line2_top);

            if(pos < 5)////////////////////////////////////////////////////lcd2
                line2_bot[6+pos] = key + '0';
            display2_bot(line2_bot);

            if(key== 1)
                {

                    ind[9]= 1;//1
                    ind[10]= 0;//2
                    ind[11]= 0;//3
                    ind[12]= 0;//A
                    ind[13]= 0;//4
                    ind[14]= 0;//5
                    ind[15]= 0;//6
                    ind[16]= 0;//B
                    ind[17]= 0;//7
                    ind[18]= 0;//8
                    ind[19]= 0;//9
                    ind[22]= 0;//0
                }
                else if(key== 2)
                {

                    ind[9]= 0;//1
                    ind[10]= 1;//2
                    ind[11]= 0;//3
                    ind[12]= 0;//A
                    ind[13]= 0;//4
                    ind[14]= 0;//5
                    ind[15]= 0;//6
                    ind[16]= 0;//B
                    ind[17]= 0;//7
                    ind[18]= 0;//8
                    ind[19]= 0;//9
                    ind[22]= 0;//0
                }
                else if(key== 3)
                {
                    ind[9]= 0;//1
                    ind[10]= 0;//2
                    ind[11]= 1;//3
                    ind[12]= 0;//A
                    ind[13]= 0;//4
                    ind[14]= 0;//5
                    ind[15]= 0;//6
                    ind[16]= 0;//B
                    ind[17]= 0;//7
                    ind[18]= 0;//8
                    ind[19]= 0;//9
                    ind[22]= 0;//0
                }
                else if(key== 4)
                {

                    ind[9]= 0;//1
                    ind[10]= 0;//2
                    ind[11]= 0;//3
                    ind[12]= 0;//A
                    ind[13]= 1;//4
                    ind[14]= 0;//5
                    ind[15]= 0;//6
                    ind[16]= 0;//B
                    ind[17]= 0;//7
                    ind[18]= 0;//8
                    ind[19]= 0;//9
                    ind[22]= 0;//0
                }
                else if(key== 5)
                {

                    ind[9]= 0;//1
                    ind[10]= 0;//2
                    ind[11]= 0;//3
                    ind[12]= 0;//A
                    ind[13]= 0;//4
                    ind[14]= 1;//5
                    ind[15]= 0;//6
                    ind[16]= 0;//B
                    ind[17]= 0;//7
                    ind[18]= 0;//8
                    ind[19]= 0;//9
                    ind[22]= 0;//0
                }
                else if(key== 6)
                {

                    ind[9]= 0;//1
                    ind[10]= 0;//2
                    ind[11]= 0;//3
                    ind[12]= 0;//A
                    ind[13]= 0;//4
                    ind[14]= 0;//5
                    ind[15]= 1;//6
                    ind[16]= 0;//B
                    ind[17]= 0;//7
                    ind[18]= 0;//8
                    ind[19]= 0;//9
                    ind[22]= 0;//0
                }
                else if(key== 7)
                {

                    ind[9]= 0;//1
                    ind[10]= 0;//2
                    ind[11]= 0;//3
                    ind[12]= 0;//A
                    ind[13]= 0;//4
                    ind[14]= 0;//5
                    ind[15]= 0;//6
                    ind[16]= 0;//B
                    ind[17]= 1;//7
                    ind[18]= 0;//8
                    ind[19]= 0;//9
                    ind[22]= 0;//0
                }
                else if(key== 8)
                {

                    ind[9]= 0;//1
                    ind[10]= 0;//2
                    ind[11]= 0;//3
                    ind[12]= 0;//A
                    ind[13]= 0;//4
                    ind[14]= 0;//5
                    ind[15]= 0;//6
                    ind[16]= 0;//B
                    ind[17]= 0;//7
                    ind[18]= 1;//8
                    ind[19]= 0;//9
                    ind[22]= 0;//0

                }
                else if(key== 9)
                {

                    ind[9]= 0;//1
                    ind[10]= 0;//2
                    ind[11]= 0;//3
                    ind[12]= 0;//A
                    ind[13]= 0;//4
                    ind[14]= 0;//5
                    ind[15]= 0;//6
                    ind[16]= 0;//B
                    ind[17]= 0;//7
                    ind[18]= 0;//8
                    ind[19]= 1;//9
                    ind[22]= 0;//0

                }
                else if(key== 0)
                {

                    ind[9]= 0;//1
                    ind[10]= 0;//2
                    ind[11]= 0;//3
                    ind[12]= 0;//A
                    ind[13]= 0;//4
                    ind[14]= 0;//5
                    ind[15]= 0;//6
                    ind[16]= 0;//B
                    ind[17]= 0;//7
                    ind[18]= 0;//8
                    ind[19]= 0;//9
                    ind[22]= 1;//0
                }
        }
    }
    return 0;
}

//========================================= LCD ========================================================

void cmd_top(char b){ // send command ?
    while (!(SPI1->SR & SPI_SR_TXE));
    SPI1->DR = b;
}

void data_top(char b){ // send data ?
    while(!(SPI1->SR & SPI_SR_TXE));
    SPI1->DR = 0x200 + b;
}

void spi_init_lcd_top(void){
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    GPIOA->MODER |= GPIO_MODER_MODER5_1 | GPIO_MODER_MODER7_1 | GPIO_MODER_MODER15_1;
    GPIOA->AFR[0] &= ~(GPIO_AFRL_AFRL5 | GPIO_AFRL_AFRL7);
    GPIOA->AFR[1] &= ~(GPIO_AFRH_AFRH7);

    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    SPI1->CR1 |= SPI_CR1_BIDIMODE | SPI_CR1_BIDIOE;
    SPI1->CR1 |= SPI_CR1_MSTR | SPI_CR1_BR_0 | SPI_CR1_BR_1;
    SPI1->CR1 &= ~SPI_CR1_CPOL;// 0 when idle
    SPI1->CR1 &= ~SPI_CR1_CPHA;// the first clock transition as the data capture edge
    SPI1->CR2 |= SPI_CR2_DS_0 | SPI_CR2_DS_3;//10 bit word size
    SPI1->CR2 &= ~SPI_CR2_DS_1 & ~SPI_CR2_DS_2;
    SPI1->CR2 |= SPI_CR2_NSSP;//NSS
    SPI1->CR2 |= SPI_CR2_SSOE;//slave select output enable
    SPI1->CR1 |= SPI_CR1_SPE;// setting the SPE bit

    generic_lcd_startup_top();
}

void dma_spi_init_lcd_top(void) {
    spi_init_lcd_top();

    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    DMA1_Channel3->CCR &= ~DMA_CCR_EN;
    DMA1_Channel3->CMAR = (uint32_t)(dispmem_top);//address
    DMA1_Channel3->CPAR = (uint32_t)(&(SPI1->DR));//get from SPI1DR
    DMA1_Channel3->CNDTR = 34;//move
    DMA1_Channel3->CCR |= DMA_CCR_DIR | DMA_CCR_TCIE;
    DMA1_Channel3->CCR &= ~DMA_CCR_MSIZE & ~DMA_CCR_PSIZE & ~DMA_CCR_HTIE;//size for bit
    DMA1_Channel3->CCR |= DMA_CCR_MSIZE_0;//set to 01
    DMA1_Channel3->CCR |= DMA_CCR_PSIZE_0;//set to 01
    DMA1_Channel3->CCR |= DMA_CCR_MINC | DMA_CCR_CIRC;
    DMA1_Channel3->CCR &= ~DMA_CCR_PL;
    DMA1_Channel3->CCR |= DMA_CCR_EN;
    SPI1->CR2 |= SPI_CR2_TXDMAEN;//SPI transmit buffer
}

void display1_top(const char *s) {
    cmd_top(0x80+0);
    int x;
    for(x= 0; x< 16; x++){
        if(s[x] != '\0'){
            dispmem_top[x+1] = s[x] | 0x200;
        }
        else break;
    }
}

void display2_top(const char *s) {
    cmd_top(0x80+64);
    int x;
    for(x= 0; x< 16; x++){
        if(s[x] != '\0'){
            dispmem_top[x+18] = s[x] | 0x200;
        }
        else break;
    }
}

//=========================================================================
// An inline assembly language version of nano_wait.
//=========================================================================
void nano_wait_top(unsigned int n) {
    asm(    "        mov r0,%0\n"
            "repeat_top: sub r0,#83\n"
            "        bgt repeat_top\n" : : "r"(n) : "r0", "cc");
}

//=========================================================================
// Generic subroutine to configure the LCD screen.
//=========================================================================
void generic_lcd_startup_top(void) {
    nano_wait_top(100000000); // Give it 100ms to initialize
    cmd_top(0x38);  // 0011 NF00 N=1, F=0: two lines
    cmd_top(0x0c);  // 0000 1DCB: display on, no cursor, no blink
    cmd_top(0x01);  // clear entire display
    nano_wait_top(6200000); // clear takes 6.2ms to complete
    cmd_top(0x02);  // put the cursor in the home position
    cmd_top(0x06);  // 0000 01IS: set display to increment
}


////////////////////////////////////////////////////////////////////////////lcd2
////////////////////////////////////////////////////////////////////////////lcd2


void cmd_bot(char b){ // send command ?
    while (!(SPI2->SR & SPI_SR_TXE));
    SPI2->DR = b;
}

void data_bot(char b){ // send data ?
    while(!(SPI2->SR & SPI_SR_TXE));
    SPI2->DR = 0x200 + b;
}

void spi_init_lcd_bot(void){
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    GPIOB->MODER |= GPIO_MODER_MODER12_1 | GPIO_MODER_MODER13_1 | GPIO_MODER_MODER15_1;
    GPIOB->AFR[1] &= ~(GPIO_AFRH_AFRH7 | GPIO_AFRH_AFRH5 | GPIO_AFRH_AFRH4);

    RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
    SPI2->CR1 |= SPI_CR1_BIDIMODE | SPI_CR1_BIDIOE;
    SPI2->CR1 |= SPI_CR1_MSTR | SPI_CR1_BR_0 | SPI_CR1_BR_1;
    SPI2->CR1 &= ~SPI_CR1_CPOL;// 0 when idle
    SPI2->CR1 &= ~SPI_CR1_CPHA;// the first clock transition as the data capture edge
    SPI2->CR2 |= SPI_CR2_DS_0 | SPI_CR2_DS_3;//10 bit word size
    SPI2->CR2 &= ~SPI_CR2_DS_1 & ~SPI_CR2_DS_2;
    SPI2->CR2 |= SPI_CR2_NSSP;//NSS
    SPI2->CR2 |= SPI_CR2_SSOE;//slave select output enable
    SPI2->CR1 |= SPI_CR1_SPE;// setting the SPE bit

    generic_lcd_startup_bot();
}

void dma_spi_init_lcd_bot(void) {
    spi_init_lcd_bot();

    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    DMA1_Channel5->CCR &= ~DMA_CCR_EN;
    DMA1_Channel5->CMAR = (uint32_t)(dispmem_bot);//address
    DMA1_Channel5->CPAR = (uint32_t)(&(SPI2->DR));//get from spi2DR
    DMA1_Channel5->CNDTR = 34;//move
    DMA1_Channel5->CCR |= DMA_CCR_DIR | DMA_CCR_TCIE;
    DMA1_Channel5->CCR &= ~DMA_CCR_MSIZE & ~DMA_CCR_PSIZE & ~DMA_CCR_HTIE;//size for bit
    DMA1_Channel5->CCR |= DMA_CCR_MSIZE_0;//set to 01
    DMA1_Channel5->CCR |= DMA_CCR_PSIZE_0;//set to 01
    DMA1_Channel5->CCR |= DMA_CCR_MINC | DMA_CCR_CIRC;
    DMA1_Channel5->CCR &= ~DMA_CCR_PL;
    DMA1_Channel5->CCR |= DMA_CCR_EN;
    SPI2->CR2 |= SPI_CR2_TXDMAEN;//SPI transmit buffer
}

void display1_bot(const char *s) {
    cmd_bot(0x80+0);
    int x;
    for(x= 0; x< 16; x++){
        if(s[x] != '\0'){
            dispmem_bot[x+1] = s[x] | 0x200;
        }
        else break;
    }
}

void display2_bot(const char *s) {
    cmd_bot(0x80+64);
    int x;
    for(x= 0; x< 16; x++){
        if(s[x] != '\0'){
            dispmem_bot[x+18] = s[x] | 0x200;
        }
        else break;
    }
}

//=========================================================================
// An inline assembly language version of nano_wait.
//=========================================================================
void nano_wait_bot(unsigned int n) {
    asm(    "        mov r0,%0\n"
            "repeat_bot: sub r0,#83\n"
            "        bgt repeat_bot\n" : : "r"(n) : "r0", "cc");
}

//=========================================================================
// Generic subroutine to configure the LCD screen.
//=========================================================================
void generic_lcd_startup_bot(void) {
    nano_wait_bot(100000000); // Give it 100ms to initialize
    cmd_bot(0x38);  // 0011 NF00 N=1, F=0: two lines
    cmd_bot(0x0c);  // 0000 1DCB: display on, no cursor, no blink
    cmd_bot(0x01);  // clear entire display
    nano_wait_bot(6200000); // clear takes 6.2ms to complete
    cmd_bot(0x02);  // put the cursor in the home position
    cmd_bot(0x06);  // 0000 01IS: set display to increment
}

//=========================================== countdown ====================================================
int calls = 0, count = 240;
void countdown()
{
	char line[16];
	calls++;
	if (calls == 1000)
	{
		calls = 0;
		if (count > 0)
		{
			count--;
			sprintf(line , "Count Down: %01d:%02d" , count/60, count%60);
			display1_top(line);
		}
		else{
			display1_top("  Time's up!!!  ");
			sprintf(line1_bot , " Host :%s" , ans);
			display1_bot(line1_bot);
			RCC->AHBENR &= ~(RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOCEN);
			RCC->APB1ENR &= ~(RCC_APB1ENR_TIM6EN | RCC_APB1ENR_TIM3EN);
			RCC->APB2ENR &= ~(RCC_APB2ENR_TIM15EN | RCC_APB2ENR_TIM1EN);
		}

	}
}
//======================================================================================================
int main(){
    init_keypad();
    dma_spi_init_lcd_top();
    dma_spi_init_lcd_bot();//////////////////////////////////////////////////lcd2

    display1_top(line1_top);
    display1_bot(line1_bot);///////////////////////////////////////////////////lcd2
    display2_top(line2_top);
    display2_bot(line2_bot);///////////////////////////////////////////////////lcd2

    setup_gpio();
    setup_wave();
    setup_dac();
    setup_tim15();
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
	TIM2->CR1 |= TIM_CR1_CEN;
	get_user_val();

    flag1 = 1;
    //char database[1153][5];
    char input[4];

    int i = 1 , j , a = 0, b = 0;

    memcpy(input , &line2_bot[7] , 4);
    srand(TIM2->CNT);
    ans[0] = rand()%9 + 1 + '0';
    while(i < 4) {
        ans[i] = rand()% 9 + 1 + '0';
        for (j = 0 ; j < i ; j++) if (ans[j] == ans[i]) {
            i--;
            break;
        }
        i++;
    }
    while(1) {
        for (i = 0 ; i < 4; i++) if (ans[i] == input[i]) a++;
        for (i = 0 ; i < j ; i++)  {
            for (j = 0 ; j < 4; j++) if ((ans [i] == input[j]) && (i != j)) b++;
        }
        if (a == 4) break;
        line1_bot[7] = a + '0';
        line1_bot[8] = 'A';
        line1_bot[9] = b + '0';
        line1_bot[10] = 'B';
        display1_bot(line1_bot);
        a = 0 , b = 0;
        do
        {
        	flag2 = 0;
        	get_user_val();
			memcpy(input , &line2_bot[7] , 4);
			for (i = 0 ; i < 3 ; i++)
			{
				for (j = i + 1; j < 4 ; j++)
				{
					if (input[j] == ' ')
					{
						display2_top("Not Enough Input");
						flag2 = 1;
						nano_wait_top(1000000000);
						sprintf(line2_top , "  Guesses: %03d  " , guess);
						display2_top(line2_top);
						break;
					}
					if (input[i] == input[j])
					{
						display2_top("Repeated Number!");
						flag2 = 1;
						nano_wait_top(1000000000);
						sprintf(line2_top , "  Guesses: %03d  " , guess);
						display2_top(line2_top);
						break;
					}
				}
				if (j != 4) break;
			}

        }while(flag2 == 1);
        guess++;
        sprintf(line2_top , "  Guesses: %03d  " , guess);
		display2_top(line2_top);
    }
    display1_bot(" Host : Correct!!!!");
    sprintf(line1_top , "Time Used: %01d:%02d " , (240 - count)/60, (240 - count)%60);
	display1_top(line1_top);
    RCC->AHBENR &= ~(RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOCEN);
    RCC->APB1ENR &= ~(RCC_APB1ENR_TIM6EN | RCC_APB1ENR_TIM3EN);
    RCC->APB2ENR &= ~(RCC_APB2ENR_TIM15EN | RCC_APB2ENR_TIM1EN);
    return 0;
}
//////////////////////////what happens when input two same numbers?????????
