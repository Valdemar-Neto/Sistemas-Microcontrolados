/*
 * Project_4.c
 *
 * Created: 12/05/2025 17:21:12
 * Author : chamo
 */ 
#include <avr/io.h>
#define F_CPU 16000000UL // define clock do sistema
#include <util/delay.h>
#include <avr/interrupt.h>

#define set_bit(Y,bit_X) (Y|=(1<<bit_X)) // set num bit
#define clr_bit(Y,bit_X) (Y&=~(1<<bit_X)) // clear num bit
#define cpl_bit(Y,bit_X) (Y^=(1<<bit_X)) // inverte um bit
#define tst_bit(Y,bit_X) (Y&(1<<bit_X)) // testa um bit(isola um bit)
#define pulso_enable() _delay_us(1); set_bit(LCD_CTRL,EN); _delay_us(100); clr_bit(LCD_CTRL,EN); _delay_us(45)

#define RS PD2
#define EN PD3
#define LCD_DATA PORTD // PD7 ate PD4
#define LCD_CTRL PORTD
#define RED OCR2A
#define GREEN OCR1B
#define BLUE OCR1A

void lcd_cmd(unsigned char c, char cd);
void lcd_write(char *c);
void lcd_init();
void mde(char s);
void numIntoString(char *str, int start_pos,int num);

volatile char estado = 0;
volatile char mudarRGB = 0;
volatile char mudarTexto = 1;

char bottom_line[] =  "  0    0    0\0";
char *ptr_bottomline = &bottom_line[0];

volatile short int numRed = 0;
volatile short int numGreen = 0;
volatile short int numBlue = 0;

int main(void)
{
	//set OC2x on CompMatch COM2x1:0 = 3
	//fast pwm 8-bit WGM13:0 = 5
	TCCR1A = 0xF1;
	TCCR1B = 0x09;// cs12:0 = 1 clk sem pre-escala
	
	// clear OC2A on CompMatch
	// fast pwm WGM22:0 = 3
	TCCR2A = 0xC3;
	TCCR2B = 0x01;// cs22:0 = 1 clk sem pre-escala

	BLUE = 0x0000;
	GREEN = 0x0000;
	RED = 0x00;
	
	DDRB = 0x0E;
	DDRC = 0x00;
	DDRD = 0xFC;
	
	PCICR = 0x02;
	PCMSK1 = 0x0E;
	sei();
	
	lcd_init();	
	lcd_cmd(0x01,0);
	lcd_cmd(0x02,0);
	while (1) 
	{
		if(RED == 0x00){
			//apaga led se OCRnx for zero
			clr_bit(TCCR2A,7);
			clr_bit(TCCR2A,6);
			set_bit(PORTB,3);
		}else{
			set_bit(TCCR2A,7);
			set_bit(TCCR2A,6);
		}

		if(GREEN == 0x0000){
			//apaga led se OCRnx for zero
			clr_bit(TCCR1A,5);
			clr_bit(TCCR1A,4);
			set_bit(PORTB,2);
		}else{
			set_bit(TCCR1A,5);
			set_bit(TCCR1A,4);
		}

		if(BLUE == 0x0000){
			//apaga led se OCRnx for zero
			clr_bit(TCCR1A,7);
			clr_bit(TCCR1A,6);
			set_bit(PORTB,1);
		}else{
			set_bit(TCCR1A,7);
			set_bit(TCCR1A,6);
		}
		
		mde(estado);
	}
	
	return 0;
}

void lcd_cmd(unsigned char c, char cd){
	//---------------Upper_nibble---------------
	LCD_DATA = (c & 0xF0) | (LCD_DATA & 0x0F);
	
	if(cd==0)
		clr_bit(LCD_CTRL,RS);
	else
		set_bit(LCD_CTRL,RS);
	pulso_enable();
	
	if((cd==0) && (c < 4)) _delay_ms(2);
	//---------------Lower_nibble---------------
	LCD_DATA = ((c & 0x0F) << 4) | (LCD_DATA & 0x0F);
	
	if(cd==0)
		clr_bit(LCD_CTRL,RS);
	else
		set_bit(LCD_CTRL,RS);
	pulso_enable();
	
	if((cd==0) && (c < 4)) _delay_ms(2);
	////------------------------------------------
	
	return;
}

void lcd_write(char *c)
{
	for (; *c!='\0';c++) lcd_cmd(*c,1);
}

void lcd_init(){
	clr_bit(LCD_CTRL,EN);
	clr_bit(LCD_CTRL,RS);
	_delay_ms(50);
	
	// Instruções: Canal de 4 bits, 2 linhas, caracteres de 5x10 bits
	LCD_DATA = (0x28 & 0xF0) | (LCD_DATA & 0x0F);
	pulso_enable();
	
	LCD_DATA = ((0x28 & 0x0F) << 4) | (LCD_DATA & 0x0F);
	pulso_enable();
	
	_delay_ms(5);
	pulso_enable();
	_delay_us(200);
	pulso_enable();
	pulso_enable();
	
	lcd_cmd(0x08,0); //Display off, Cursor off, Blink off
	lcd_cmd(0x01,0); //Clear display, DDRAM address in counter = 0
	lcd_cmd(0x0C,0); //Display on, Cursor off, Blink off
	lcd_cmd(0x80,0); //DDRAM address = 0 (primeira posição na esquerda)
	return;
}

void numIntoString(char *str, int start_pos,int num){
	int centena,dezena,unidade;
	
	if(num >= 100){
		centena = num/100;
		dezena = (num - centena*100)/10;
		unidade	= num - centena*100 - dezena*10;
		
		str[start_pos - 2] = centena + '0';
		str[start_pos - 1] = dezena + '0';
		str[start_pos] = unidade + '0';
	}else if(num < 100 && num >= 10){
		dezena = num/10;
		unidade	= num - dezena*10;
		
		str[start_pos - 2] = ' ';
		str[start_pos - 1] = dezena + '0';
		str[start_pos] = unidade + '0';
	}else{
		unidade	= num;
		
		str[start_pos - 2] = ' ';
		str[start_pos - 1] = ' ';
		str[start_pos] = unidade + '0';
	}
	
	return;
}

void mde(char s){
	switch(s){
	//--------------------ESTADO_INICIAL--------------------
	case 0:
		if(mudarTexto == 1){
			lcd_cmd(0x01,0);
			lcd_cmd(0x02,0);
			lcd_write("RED GREEN BLUE\0");
			_delay_ms(10);
			lcd_cmd(0xC0,0);
			
			bottom_line[3] = ' ';
			bottom_line[8] = ' ';
			bottom_line[13] = ' ';
			lcd_write(bottom_line);
			mudarTexto = 0;
		}
		break;
	//--------------------ESTADO_INICIAL--------------------
		
	//--------------------ESTADO_ALTERA_RED--------------------
	case 1:
		if(mudarRGB == 1){
			if(numRed == 255){
				numRed = 255;
			}else{
				numRed += 5;	
			}
			
			mudarRGB = 0;
		}else if(mudarRGB == 2){
			if(numRed == 0){
				numRed = 0;
			}else{
				numRed -= 5;
			}
			mudarRGB = 0;
		}else{}
			
		RED = numRed;
		
		if(mudarTexto == 1){
			lcd_cmd(0x01,0);
			lcd_cmd(0x02,0);
			lcd_write("RED GREEN BLUE\0");
			_delay_ms(10);
			lcd_cmd(0xC0,0);
			
			numIntoString(ptr_bottomline, 2, numRed);
			bottom_line[3] = '*';
			bottom_line[8] = ' ';
			bottom_line[13] = ' ';
			lcd_write(bottom_line);
			mudarTexto = 0;
		}
		break;
	//--------------------ESTADO_ALTERA_RED--------------------
	
	//--------------------ESTADO_ALTERA_BLUE--------------------
	case 2:
		if(mudarRGB == 1){
			if(numGreen == 255){
				numGreen = 255;
			}else{
				numGreen += 5;
			}
			mudarRGB = 0;
		}else if(mudarRGB == 2){
			if(numGreen == 0){
				numGreen = 0;
			}else{
				numGreen -= 5;
			}
			mudarRGB = 0;
		}else{}
			
		GREEN = numGreen;
		
		if(mudarTexto == 1){
			lcd_cmd(0x01,0);
			lcd_cmd(0x02,0);
			lcd_write("RED GREEN BLUE\0");
			_delay_ms(10);
			lcd_cmd(0xC0,0);
			
			numIntoString(ptr_bottomline, 7, numGreen);
			bottom_line[3] = ' ';
			bottom_line[8] = '*';
			bottom_line[13] = ' ';
			lcd_write(bottom_line);
			mudarTexto = 0;
		}
		break;
	//--------------------ESTADO_ALTERA_BLUE--------------------
	
	//--------------------ESTADO_ALTERA_GREEN--------------------
	case 3:
		if(mudarRGB == 1){
			if(numBlue == 255){
				numBlue = 255;
			}else{
				numBlue += 5;
			}
			mudarRGB = 0;
		}else if(mudarRGB == 2){
			if(numBlue == 0){
				numBlue = 0;
			}else{
				numBlue -= 5;
			}
			mudarRGB = 0;
		}else{}
			
		BLUE = numBlue;
		
		if(mudarTexto == 1){
			lcd_cmd(0x01,0);
			lcd_cmd(0x02,0);
			lcd_write("RED GREEN BLUE\0");
			_delay_ms(10);
			lcd_cmd(0xC0,0);
			
			numIntoString(ptr_bottomline, 12, numBlue);
			bottom_line[3] = ' ';
			bottom_line[8] = ' ';
			bottom_line[13] = '*';
			lcd_write(bottom_line);
			mudarTexto = 0;
		}
		break;
	//--------------------ESTADO_ALTERA_GREEN--------------------
	
	default:
		break;
	}
	return;
}

ISR(PCINT1_vect) {
	//PC1 -> aumenta
	//PC2 -> estado
	//PC3 -> diminui
	if((PINC & 0x0E) == 0x0A){
		switch(estado){
		case 0:
			estado = 1;
			break;
		case 1:
			estado = 2;
			break;
		case 2:
			estado = 3;
			break;
		case 3:
			estado = 0;
			break;
		default:
			break;
		}
		mudarTexto = 1;
		mudarRGB = 0;
	}else{}
	
	if((PINC & 0x0E) == 0x0C) mudarRGB = 1; mudarTexto = 1;
	if((PINC & 0x0E) == 0x06) mudarRGB = 2; mudarTexto = 1;
}
