#include <stdio.h>
#include "stm8s.h"

void Delay(uint16_t nCount);

void CLK_DisableDiv8(void);

typedef enum {
	BLACK, RED, GREEN, YELLOW, BLUE, PURPLE, CYAN, WHITE,
	BLACK2, RED2, GREEN2, YELLOW2, BLUE2, PURPLE2, CYAN2, WHITE2
} Color;

typedef enum {
	CLEAR_TO_CURSOR = 0,
	CLEAR_FROM_CURSOR = 1,
	CLEAR_ALL = 2,
} ClearMode;

inline void sendcharsafe(char c)
{
	while((UART1->SR & UART1_SR_TXE) == 0) {}
	UART1->DR = (uint8_t)c;
}

void UART1_SendStr(const char *str)
{
	char c;
	while((c = *str++)) {
		while((UART1->SR & UART1_SR_TXE) == 0) {}
		UART1->DR = (uint8_t)c;
	}
}

void A_Fg(Color c)
{
	int ci = c;
	char buf[10];
	if(ci > 7) {
		ci += (90-8);
	} else {
		ci += 30;
	}
	sprintf(buf, "\033[%dm", ci);
	UART1_SendStr(buf);
}

void A_Bg(Color c)
{
	int ci = c;
	char buf[10];
	if(ci > 7) {
		ci += (100-8);
	} else {
		ci += 40;
	}
	sprintf(buf, "\033[%dm", ci);
	UART1_SendStr(buf);
}

void A_GoTo(u8 y, u8 x)
{
	char buf[10];
	sprintf(buf, "\033[%d;%dH", y, x);
	UART1_SendStr(buf);
}

inline void A_Str(const char *str)
{
	UART1_SendStr(str);
}

inline void A_Char(const char c)
{
	sendcharsafe(c);
}

inline void A_ClearScreen(ClearMode mode)
{
	char buf[10];
	sprintf(buf, "\033[%dJ", mode);
	UART1_SendStr(buf);
}

inline void A_ClearLine(ClearMode mode)
{
	char buf[10];
	sprintf(buf, "\033[%dK", mode);
	UART1_SendStr(buf);
}

inline void A_Reset(void)
{
	UART1_SendStr("\033c");
}

inline void A_FgBg(Color fg, Color bg)
{
	A_Fg(fg);
	A_Bg(bg);
}

inline void A_ShowCursor(int yes)
{
	if (yes)
		UART1_SendStr("\033[?25h");
	else
		UART1_SendStr("\033[?25l");
}

void LED_DISPL(void) {
	A_GoTo(10,0);
	A_FgBg(WHITE,BLACK);
	A_ClearLine(CLEAR_ALL);

	if (GPIOB->ODR & GPIO_PIN_4) {
		A_Fg(BLACK2);
	} else {
		A_Fg(GREEN2);
	}
	A_Str("[LED]");
}

int timecounter = 0;

volatile int esp_is_ready = 0;

void TIME_DISP(void) {
	char buf[100];
	// time counter
	A_GoTo(4,8);
	A_Bg(BLUE);
	A_ClearLine(CLEAR_FROM_CURSOR);
	A_Fg(YELLOW2);

	sprintf(buf, "t = %d", timecounter);
	A_Str(buf);
	A_Fg(BLACK);
}

void main(void)
{
	//char buf[100];
	int16_t j,k,l;

	CLK_DisableDiv8();

	// LED
	GPIO_Init(GPIOB, GPIO_PIN_5, GPIO_MODE_OUT_PP_LOW_FAST);
	GPIO_Init(GPIOB, GPIO_PIN_4, GPIO_MODE_OUT_PP_HIGH_FAST);

	UART1_Init
		(
			115200,
			UART1_WORDLENGTH_8D,
			UART1_STOPBITS_1,
			UART1_PARITY_NO,
			UART1_SYNCMODE_CLOCK_DISABLE,
			UART1_MODE_TXRX_ENABLE
		);

	UART1->CR2 |= UART1_CR2_RIEN;
	//UART1_ITConfig(UART1_IT_RXNE_OR, ENABLE);
	enableInterrupts();

	esp_is_ready = 0;
	while(!esp_is_ready) {
		GPIOB->ODR ^= GPIO_PIN_4;
		UART1_SendStr("\033[5n");
		Delay(200);
	}
	GPIOB->ODR |=GPIO_PIN_4;

	A_Reset();
	A_ShowCursor(0);

	A_FgBg(WHITE2, BLUE);
	A_ClearScreen(CLEAR_ALL);

	A_GoTo(2, 8);
	A_Fg(CYAN2);
	A_Str("STM8");
	A_Fg(WHITE2);
	A_Str(" Hello!");

	A_Bg(BLACK);
	for(j=7;j<10;j++) {
		A_GoTo(j, 0);
		A_ClearLine(CLEAR_ALL);
	}

	LED_DISPL();

	Delay(10);

	timecounter = 0;
	while (1) {
		GPIOB->ODR ^= GPIO_PIN_5;

		TIME_DISP();

		// Animated line
		j=(timecounter%52)-26;
		k=j+26;
		if(j<0)j=0;
		if(k>=26)k=25;
		A_GoTo(8, 1+j);
		A_Bg(BLACK);
		A_ClearLine(CLEAR_ALL);
		A_Bg(WHITE2);
		for(l=0;l<=k-j;l++) {
			A_Char(' ');
		}

		timecounter++;
		Delay(2000);
	}
}

/**
 * Disable HSI div/8
 */
void CLK_DisableDiv8()
{
	// --- Disable the x8 divider ---
	/* Clear High speed internal clock prescaler */
	CLK->CKDIVR &= (uint8_t) (~CLK_CKDIVR_HSIDIV);
	/* Set High speed internal clock prescaler */
	CLK->CKDIVR |= (uint8_t) CLK_PRESCALER_HSIDIV1;
}

#define MOUSE_IN(y,x,y1,y2,x1,x2) ((y)>=(y1)&&(y)<=(y2)&&(x)>=(x1)&&(x)<=(x2))

void handleRxChar(char c) {
	static int i1, i2;
	static int state;

	if (state==0) {
		if (c == 1) {
			// hard button
			GPIOB->ODR ^= GPIO_PIN_4;
			LED_DISPL();
		}
		else if (c == '\033') {
			state = 1;
			i1 = 0;
			i2 = 0;
		}
	}
	else if (state==1) {
		if (c == '[') {
			state = 2;
		}
		else {
			state = 0;
		}
	}
	else if (state==2) {
		if (c >= '0' && c <= '9') {
			i1 = i1*10+c-'0';
		}
		else if (c == ';') {
			state = 3;
		}
		else if (c == 'n') {
			if (i1==0) {
				esp_is_ready = 1;
				state = 0;
			}
		}
		else {
			state = 0;
		}
	}
	else if (state==3) {
		if (c >= '0' && c <= '9') {
			i2 = i2*10+c-'0';
		}
		else if (c == 'M') {
			// MOUSE

			if (MOUSE_IN(i1, i2, 10, 10, 1, 5)) {
				GPIOB->ODR ^= GPIO_PIN_4;
				LED_DISPL();
			}

			if (MOUSE_IN(i1, i2, 4, 4, 8, 20)) {
				timecounter=0;
				TIME_DISP();

			}
			state = 0;
		}
		else {
			// other, fail
			state = 0;
		}
	}
}

/**
 * A delay
 */
void Delay(uint16_t nCount)
{
	uint8_t i;
	for (; nCount != 0; nCount--) {
		for (i = 255; i != 0; i--) {}
	}
}

/**
  * @brief UART1 RX Interrupt routine.
  * @param  None
  * @retval None
  */
INTERRUPT_HANDLER(UART1_RX_IRQHandler, 18)
{
  handleRxChar(UART1->DR);
  return;
}
