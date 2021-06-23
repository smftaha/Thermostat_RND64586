/*******************************************************
This program was created by the
CodeWizardAVR V3.12 Advanced
Automatic Program Generator
© Copyright 1998-2014 Pavel Haiduc, HP InfoTech s.r.l.
http://www.hpinfotech.com

Project : V1.1
Version : 
Date    : 6/23/2021
Author  : 
Company : 
Comments: 


Chip type               : ATmega32A
Program type            : Application
AVR Core Clock frequency: 4.000000 MHz
Memory model            : Small
External RAM size       : 0
Data Stack size         : 512
*******************************************************/

#include <mega32a.h>

#include <delay.h>

// 1 Wire Bus interface functions
#include <1wire.h>

// DS1820 Temperature Sensor functions
#include <ds1820.h>

// Declare your global variables here      
///////////////////////////////////////////////////////////
//fuse bits:    (high)0xd1        (low)0xe3 

//sleep for adc noise reduction
#define     SLEEP           do  {   MCUCR    =0x90;    #asm    ("sleep");\
                                    MCUCR    =0x00;}    while(0);
#define     EEP_SIZE        (10)
#define     KEY_DBNC        (5)     //*8ms
#define     KEY_LNG         (128-1)  //*8ms
#define     KEY_SHRT        (32-1)  //*8ms
#define     KEY_SWTTOLNG    (6)
#define     TO_SETPNTVW     (16)    //*64ms
#define     TO_SEGNULL      (3-1)   //*64ms
#define     TMPRTR_MAX      (35)
#define     TMPRTR_MIN      (15)
#define     TMPRTR_BUFSZ    (10)            
///////////////////
#define     MOD_ECO         (0x01)
#define     MOD_AUTO        (0x02)
#define     MOD_MANL        (0x03)
///////////////////
#define     LED_PWR        PORTB.1    
#define     SET_ALLLED     PORTB    |=     0x3c;
#define     RST_ALLLED     PORTB    &=    ~0x3c;
/*
#define        LED_DCR     PORTB.2    
#define        LED_INC     PORTB.3    
#define        LED_MODL    PORTB.4    
#define        LED_MODR    PORTB.5    
*/
///////////////////
//#define        RLY_ACT       PORTA.0
//#define        RLY_CLDHOT    PORTA.1
//#define        RLY_LOWMDM    PORTA.2
//#define        RLY_FST       PORTA.3
///////////////////
#define     RLY_PWR         PORTA.0    
#define     RLY_LOW_MDHI    PORTA.1    
#define     RLY_MD_HI       PORTA.2    
#define     RLY_CLDHOT      PORTA.3    
///////////////////
#define     SHRG_S_DAT    PORTA.4    
#define     SHRG_S_LAT    PORTA.5    
#define     SHRG_S_CLK    PORTA.6    
///////////////////
#define     SHRG_L_DAT    PORTD.5    
#define     SHRG_L_LAT    PORTD.6    
#define     SHRG_L_CLK    PORTD.7    
///////////////////
//PC0-3    =>touch
/////////////////////////////////////////////////////////// 
eeprom    unsigned char Eep[EEP_SIZE];

unsigned char Tick,    TickCnt;
struct
{
    unsigned char    Pwr;
    unsigned char    Sum1Win0;
    unsigned char    NewKey;
    unsigned char    NewKeyFst1Slw0;
    
    unsigned char    Seg_Tmprtr_Vw;
    unsigned char    Seg_SetPnt_Vw;
    unsigned char    Seg_SetPnt_TmRst;
    unsigned char    Seg_Null;
}    Flg;
struct    
{
    unsigned int     LstAdc;
    unsigned int     Tmprtr10x,    TmprtrSP10x;
    unsigned char    Key;
    unsigned char    ShrgL[4],     ShrgS;
    unsigned char    Mode,    Spd;
    unsigned char    Rgb;
}    Sts;
/////////////////////////////////////////////////////////// 
void    F_ScanTouch_RdDI    (void);
void    F_KeyAnlz           (void);

void    F_Calc              (void);////////////
void    F_Disp              (void);

//void    F_Dlyus           (unsigned char x);

void    F_Anlg_RdAI         (void);
void    F_ShRg_SpiSw        (void);
void    F_Led_WrDo          (void);
void    F_Rly_WrDo          (void);
//void    F_ClkDS1307_RdI2C (void);    ////////////
//void    F_Esp12_Uart      (void);    ////////////
///////////////////////////////////////////////////////////
void    F_ScanTouch_RdDI    (void)
{
    static    unsigned char    LKeyn,    LKeyo,    LCntShrt;
    static    unsigned int     LCnt;
        
    LKeyn=    PINC    &0xf;
    
    if    ( LKeyn    !=    LKeyo)
    {
        LKeyo    =LKeyn;
        LCntShrt    =0;
        Flg.NewKeyFst1Slw0    =0;
        
        if    ( LCnt    >KEY_SHRT)
            LCnt    -=KEY_SHRT;
        else    if    ( LCnt    >KEY_DBNC)
            LCnt    -=KEY_DBNC;
        else
            LCnt    =0;
    }
    else
    {
        if    ( LKeyn    ==0xf)
        {
            Sts.Key    =0xf;
            LCnt    =0;
        }
        else
        {            
            if        (LCnt    <KEY_DBNC)        //5*8ms
                LCnt++;
            else
            {
                Sts.Key    =LKeyn;
                if    (LCnt    ==KEY_DBNC)
                {
                    LCntShrt++;
                    Flg.NewKey    =1;
                }
                
                LCnt++;
                
                if    (LCntShrt    <KEY_SWTTOLNG)
                {
                    //Sts.NewKeyFst1Slw0    =0;
                    if    (LCnt    ==    KEY_LNG)
                    {
                        LCnt    =0;
                        LCntShrt++;  
                        if  (   ( LKeyn ==0x0a) ||  ( LKeyn ==0x08))
                            Flg.NewKeyFst1Slw0    =1;
                    }   
                }
                else
                {
                    Flg.NewKeyFst1Slw0    =1;
                    if    (LCnt    ==    KEY_LNG)
                    //if    (LCnt    ==    KEY_SHRT)
                    {
                        LCnt    =0;
                        LCntShrt++;
                    }
                }
            }
        }
    }
}
///////////////////////////////////////////////////////////
void    F_KeyAnlz            (void)
{
    //        4_DCR        2_INC
    //        8_MODL        10_MODR
    //            12_PWR
            
    //unsigned char     Seg_SetPnt_Vw;
    //unsigned char     Seg_Null;
    
    if    ( Sts.Key    ==0xc) 
    {
        if  ( Flg.Pwr   ==0)
            Flg.Pwr =1;
        else
            Flg.Pwr =0;
    }
        
    if    ( Flg.Pwr)
    {
        switch    ( Sts.Key)
        {
            case    0x02:
                if    ( Flg.Seg_Tmprtr_Vw)
                    Flg.Seg_Null    =1;
                else    if    ( Flg.Seg_SetPnt_Vw)
                { 
                    Flg.Seg_SetPnt_TmRst    =1;                   
                    if    ( Flg.NewKeyFst1Slw0    ==0)
                    {
                        if    ( Sts.TmprtrSP10x    <( TMPRTR_MAX    *10))
                            Sts.TmprtrSP10x++;
                    }
                    else
                    {
                        if    ( Sts.TmprtrSP10x    <( ( TMPRTR_MAX    *10)-10))
                            Sts.TmprtrSP10x    +=10;
                        else
                            Sts.TmprtrSP10x    = TMPRTR_MAX    *10;
                    }
                }
            break;
            case    0x04:
                if    ( Flg.Seg_Tmprtr_Vw)
                    Flg.Seg_Null    =1;
                else    if    ( Flg.Seg_SetPnt_Vw)
                {        
                    Flg.Seg_SetPnt_TmRst    =1;   
                    if    ( Flg.NewKeyFst1Slw0    ==0)
                    {
                        if    ( Sts.TmprtrSP10x    >(TMPRTR_MIN     *10))
                            Sts.TmprtrSP10x--;
                    }
                    else
                    {
                        if    ( Sts.TmprtrSP10x    >( ( TMPRTR_MIN    *10)+10))
                            Sts.TmprtrSP10x    -=10;
                        else
                            Sts.TmprtrSP10x    = TMPRTR_MIN    *10;
                    }    
                }                
            break;
            case    0x08:
                if    ( Flg.NewKeyFst1Slw0    ==0)
                {
                    if    ( Sts.Mode    ==MOD_MANL)
                    {                    
                        Sts.Spd    =(Sts.Spd+1)    &0x3;
                    }
                }
                else   
                    Flg.Sum1Win0    =1-    Flg.Sum1Win0;             
            break;
            case    0x0a:
                if    ( Flg.NewKeyFst1Slw0    ==0)
                {            
                    if    ( Sts.Mode    ==MOD_ECO)
                        Sts.Mode    =MOD_AUTO;
                    else    if    ( Sts.Mode    ==MOD_AUTO)
                        Sts.Mode    =MOD_MANL;
                    else
                        Sts.Mode    =MOD_ECO;
                }
                else 
                {     
                    Sts.Rgb    =(Sts.Rgb+1)    &0x7;
                    if  ( Sts.Rgb   ==0)
                        Sts.Rgb =1;
                }   
            break;
        }
    }
    else
    {
        Flg.Seg_Tmprtr_Vw    =1;
        Flg.Seg_SetPnt_Vw    =0;
        Flg.Seg_Null         =0;
    }
}
///////////////////////////////////////////////////////////
void    F_Anlg_RdAI            (void)
{
    ADCSRA    =0xcd;
    ADCSRA    |=    0x10;
    ADCSRA    |=    0x40;
    SLEEP;
    while    ( ( ADCSRA    &0x10)    ==0);    //about 100us
    Sts.LstAdc    =ADCW;        
}
///////////////////////////////////////////////////////////
void    F_TmOut        (void)
{
    static    unsigned    int        LCntSeg,    LCntStPntVw;

    if    ( Flg.Seg_Null    ==0)
        LCntSeg    =0;
    else
    {
        LCntSeg++;
        if    ( LCntSeg    >TO_SEGNULL)
        {
            Flg.Seg_Null    =0;
            if    ( Flg.Seg_Tmprtr_Vw)
            {
                Flg.Seg_Tmprtr_Vw    =0;
                Flg.Seg_SetPnt_Vw    =1;
            }
            else    if    ( Flg.Seg_SetPnt_Vw)
            {
                Flg.Seg_Tmprtr_Vw    =1;
                Flg.Seg_SetPnt_Vw    =0;
            }
        }
    }
    
    if    ( Flg.Seg_SetPnt_Vw   ==0)
        LCntStPntVw    =0;
    else
    {   
        if  (Flg.Seg_SetPnt_TmRst)
        { 
            LCntStPntVw    =0;
            Flg.Seg_SetPnt_TmRst    =0; 
        }              
        if    ( LCntStPntVw    <TO_SETPNTVW)
            LCntStPntVw++;
        else    if    ( LCntStPntVw    ==TO_SETPNTVW)
        {
            Flg.Seg_Null    =1;
            LCntStPntVw++;
            //save to eep
        }            
    }
}
///////////////////////////////////////////////////////////
void    F_Calc        (void)
{
    static    unsigned int     LTmprtr[TMPRTR_BUFSZ],    LAdcMean;
    static    signed    int    LDiff;
    static    unsigned char     i,j;
    ////////////////////    
    LTmprtr[i++]    =Sts.LstAdc;
    if    ( i    >=TMPRTR_BUFSZ)
        i=0;
    LAdcMean    =0;
    j    =0;
    for    ( i=0;    i<TMPRTR_BUFSZ;    i++)
        if    ( LTmprtr[i])
        {
            LAdcMean    +=LTmprtr[i];
            j++;
        }
    ////////////////////    
    LAdcMean    =(10*    LAdcMean)    /j;
    Sts.Tmprtr10x    =LAdcMean    /4;
    
    LDiff=Sts.Tmprtr10x    -Sts.TmprtrSP10x;
    if    (    LDiff    <0)
        LDiff    =-LDiff;
    ////////////////////
    if    ( Sts.Mode    ==MOD_ECO)
    {
        if    ( LDiff    >5)
            Sts.Spd    =1;
        else
            Sts.Spd    =0;
    }
    else    if    ( Sts.Mode    ==MOD_AUTO)
    {
        if    ( LDiff    >70)
            Sts.Spd    =3;
        else    if    ( LDiff   >40)
            Sts.Spd    =2;
        else    if    ( LDiff    >5)
            Sts.Spd    =1;
        else
            Sts.Spd    =0;
    }
    ////////////////////
}
///////////////////////////////////////////////////////////
void    F_Disp        (void)
{
//ShrgS:        x    x    B    R        G    B    R    G
//ShrgL:        Led    SgL    SgM    SgR
//ShrgL_Led:    CLD    HOT    ECO    AUTO    LOW    MDM    HI    CILC
    static    unsigned int  LTmp;
    static    unsigned char Lx;
    static    unsigned char LSeg[]=    {0x3f,    0x06,    0x5b,    0x4f,    0x66,\
                                        0x6d,    0x7d,    0x07,    0x7f,    0x6f};
    ////////////////////
    Sts.ShrgS       =0;
    Sts.ShrgL[0]    =0;
    Sts.ShrgL[1]    =0;
    Sts.ShrgL[2]    =0;
    Sts.ShrgL[3]    =0;
    ////////////////////
    if    ( Flg.Pwr)
    {
        if    ( Flg.Sum1Win0)
            Sts.ShrgL[0]    |=    0x40;
        else
            Sts.ShrgL[0]    |=    0x80;
    ////////////////////
        if    ( Sts.Mode    ==MOD_ECO)
            Sts.ShrgL[0]    |=    0x10;
        else    if    ( Sts.Mode    ==MOD_AUTO)
            Sts.ShrgL[0]    |=    0x20;
    ////////////////////
        if            ( Sts.Spd    ==1)
            Sts.ShrgL[0]    |=    0x08;
        else    if    ( Sts.Spd    ==2)
            Sts.ShrgL[0]    |=    0x0c;
        else    if    ( Sts.Spd    ==3)
            Sts.ShrgL[0]    |=    0x0e;
    ////////////////////
        Lx=0;
        if    ( Flg.Seg_Null    ==0)
        {
            if    ( Flg.Seg_SetPnt_Vw)
            {
                LTmp    =Sts.TmprtrSP10x;
                Lx=1;
            }
            else    if    ( Flg.Seg_Tmprtr_Vw)
            {  
                LTmp    =Sts.Tmprtr10x;      
                Lx=1;
            }
        }
    ////////////////////
        Sts.ShrgS    =(Sts.Rgb    <<3)    |    (Sts.Rgb);
    }
    else
    {     
        LTmp  =Sts.Tmprtr10x;      
        Lx=1;
        
        Sts.ShrgS        =0x10;
    }  
    if  ( Lx)
    { 
        Lx    =LTmp    /100;
        Sts.ShrgL[3]    =LSeg[Lx];
        Lx    =(LTmp    /10)    -(10*Lx);
        Sts.ShrgL[2]    =LSeg[Lx];
        Lx    =LTmp    %10;
        Sts.ShrgL[1]    =LSeg[Lx];
        
        Sts.ShrgL[0]    |=0x01;
        Sts.ShrgL[2]    |=0x80;
    }
}
///////////////////////////////////////////////////////////
/*
void    F_Dlyus        (unsigned char    x)
{
    //each cnt=>2us
    TCNT2    =0;
    while    (TCNT2    <x);
}
*/
///////////////////////////////////////////////////////////
void     F_ShRg_SpiSw        (void)
{
    static    unsigned char    i,    j,    Lbit,    Lx;
///////////////////
    Lx    =0x10;    //F_Dlyus        (1);

    for    ( i=0;    i<4;    i++)
    {
        Lbit    =0x80;        
        for    ( j=8;    j;    j--)
        {
            if    (    (    Sts.ShrgS        &    Lbit)    ==0)
                SHRG_S_DAT    =0;
            else
                SHRG_S_DAT    =1;
            
            if    (    (    Sts.ShrgL[i]    &    Lbit)    ==0)
                SHRG_L_DAT    =0;
            else
                SHRG_L_DAT    =1;            
            
            Lx=    (Lx    >>1)    +0x10;    //F_Dlyus        (1);
            SHRG_S_CLK    =1;
            SHRG_L_CLK    =1;
            Lx=    (Lx    >>1)    +0x10;    //F_Dlyus        (1);
            SHRG_S_CLK    =0;
            SHRG_L_CLK    =0;
            Lbit        >>=1;
        }
    }
    
    SHRG_S_LAT    =0;
    SHRG_L_LAT    =0;
    Lx=    (Lx    >>1)    +0x10;    //F_Dlyus	(1);
	SHRG_S_LAT	=1;
	SHRG_L_LAT	=1;
}
///////////////////////////////////////////////////////////
void	F_Led_WrDo		(void)
{
	if	( Flg.Pwr)
	{
		SET_ALLLED;
	}
	else
	{
		RST_ALLLED;
	}
}
///////////////////////////////////////////////////////////
void	F_Rly_WrDo		(void)
{
	if	( Flg.Pwr	==0)
	{
		RLY_PWR			=0;
		RLY_LOW_MDHI	=0;
		RLY_MD_HI		=0;	
	}
	else
	{
		if	( Sts.Spd	==1)	//low speed
		{
			RLY_LOW_MDHI	=1;
			RLY_MD_HI		=0;	
		}
		if	( Sts.Spd	==2)	//mdm speed
		{
			RLY_LOW_MDHI	=0;
			RLY_MD_HI		=0;
		}
		if	( Sts.Spd	==3)	//hgh speed
		{
			RLY_LOW_MDHI	=0;
			RLY_MD_HI		=1;
		}
		if	( Flg.Sum1Win0)
			RLY_CLDHOT	=1;
		else
			RLY_CLDHOT	=0;
		
		RLY_PWR		=1;
	}
}
///////////////////////////////////////////////////////////
// Timer 0 output compare interrupt service routine
interrupt [TIM0_COMP] void timer0_comp_isr(void)
{
	Tick	=1;
	TickCnt++;
}

// Voltage Reference: AREF pin
#define ADC_VREF_TYPE ((0<<REFS1) | (0<<REFS0) | (0<<ADLAR))

// ADC interrupt service routine
interrupt [ADC_INT] void adc_isr(void)
{
//unsigned int adc_data;
// Read the AD conversion result
//adc_data=ADCW;
// Place your code here

}

void main(void)
{
// Declare your local variables here
    static	unsigned char 	i;
    static	union
    {
        unsigned char All[EEP_SIZE];
        struct
        {
            unsigned char StPnt10xl,	StPnt10xh;
            unsigned char Mode;
            unsigned char Spd;
            unsigned char Hot1Cld0;
            unsigned char Dmy[5];
        }	Sub;
    }	LEep;
// Input/Output Ports initialization
// Port A initialization
// Function: Bit7=In Bit6=Out Bit5=Out Bit4=Out Bit3=Out Bit2=Out Bit1=Out Bit0=Out 
DDRA=(0<<DDA7) | (1<<DDA6) | (1<<DDA5) | (1<<DDA4) | (1<<DDA3) | (1<<DDA2) | (1<<DDA1) | (1<<DDA0);
// State: Bit7=T Bit6=0 Bit5=0 Bit4=0 Bit3=0 Bit2=0 Bit1=0 Bit0=0 
PORTA=(0<<PORTA7) | (0<<PORTA6) | (0<<PORTA5) | (0<<PORTA4) | (0<<PORTA3) | (0<<PORTA2) | (0<<PORTA1) | (0<<PORTA0);

// Port B initialization
// Function: Bit7=In Bit6=In Bit5=Out Bit4=Out Bit3=Out Bit2=Out Bit1=Out Bit0=In 
DDRB=(0<<DDB7) | (0<<DDB6) | (1<<DDB5) | (1<<DDB4) | (1<<DDB3) | (1<<DDB2) | (1<<DDB1) | (0<<DDB0);
// State: Bit7=T Bit6=T Bit5=0 Bit4=0 Bit3=0 Bit2=0 Bit1=0 Bit0=T 
PORTB=(0<<PORTB7) | (0<<PORTB6) | (0<<PORTB5) | (0<<PORTB4) | (0<<PORTB3) | (0<<PORTB2) | (0<<PORTB1) | (0<<PORTB0);

// Port C initialization
// Function: Bit7=In Bit6=In Bit5=In Bit4=In Bit3=In Bit2=In Bit1=In Bit0=In 
DDRC=(0<<DDC7) | (0<<DDC6) | (0<<DDC5) | (0<<DDC4) | (0<<DDC3) | (0<<DDC2) | (0<<DDC1) | (0<<DDC0);
// State: Bit7=T Bit6=T Bit5=T Bit4=T Bit3=T Bit2=T Bit1=T Bit0=T 
PORTC=(0<<PORTC7) | (0<<PORTC6) | (0<<PORTC5) | (0<<PORTC4) | (0<<PORTC3) | (0<<PORTC2) | (0<<PORTC1) | (0<<PORTC0);

// Port D initialization
// Function: Bit7=Out Bit6=Out Bit5=Out Bit4=In Bit3=In Bit2=In Bit1=In Bit0=In 
DDRD=(1<<DDD7) | (1<<DDD6) | (1<<DDD5) | (0<<DDD4) | (0<<DDD3) | (0<<DDD2) | (0<<DDD1) | (0<<DDD0);
// State: Bit7=0 Bit6=0 Bit5=0 Bit4=T Bit3=T Bit2=T Bit1=T Bit0=T 
PORTD=(0<<PORTD7) | (0<<PORTD6) | (0<<PORTD5) | (0<<PORTD4) | (0<<PORTD3) | (0<<PORTD2) | (0<<PORTD1) | (0<<PORTD0);

// Timer/Counter 0 initialization
// Clock source: System Clock
// Clock value: 62.500 kHz
// Mode: CTC top=OCR0
// OC0 output: Disconnected
// Timer Period: 1.008 ms
TCCR0=(0<<WGM00) | (0<<COM01) | (0<<COM00) | (1<<WGM01) | (0<<CS02) | (1<<CS01) | (1<<CS00);
TCNT0=0x00;
OCR0=0x3e;//31;    //0x3E;

// Timer/Counter 1 initialization
// Clock source: System Clock
// Clock value: Timer1 Stopped
// Mode: Normal top=0xFFFF
// OC1A output: Disconnected
// OC1B output: Disconnected
// Noise Canceler: Off
// Input Capture on Falling Edge
// Timer1 Overflow Interrupt: Off
// Input Capture Interrupt: Off
// Compare A Match Interrupt: Off
// Compare B Match Interrupt: Off
TCCR1A=(0<<COM1A1) | (0<<COM1A0) | (0<<COM1B1) | (0<<COM1B0) | (0<<WGM11) | (0<<WGM10);
TCCR1B=(0<<ICNC1) | (0<<ICES1) | (0<<WGM13) | (0<<WGM12) | (0<<CS12) | (0<<CS11) | (0<<CS10);
TCNT1H=0x00;
TCNT1L=0x00;
ICR1H=0x00;
ICR1L=0x00;
OCR1AH=0x00;
OCR1AL=0x00;
OCR1BH=0x00;
OCR1BL=0x00;

// Timer/Counter 2 initialization
// Clock source: System Clock
// Clock value: Timer2 Stopped
// Mode: Normal top=0xFF
// OC2 output: Disconnected
ASSR=0<<AS2;
TCCR2=(0<<PWM2) | (0<<COM21) | (0<<COM20) | (0<<CTC2) | (0<<CS22) | (0<<CS21) | (0<<CS20);
TCNT2=0x00;
OCR2=0x00;

// Timer(s)/Counter(s) Interrupt(s) initialization
TIMSK=(0<<OCIE2) | (0<<TOIE2) | (0<<TICIE1) | (0<<OCIE1A) | (0<<OCIE1B) | (0<<TOIE1) | (1<<OCIE0) | (0<<TOIE0);

// External Interrupt(s) initialization
// INT0: Off
// INT1: Off
// INT2: Off
MCUCR=(0<<ISC11) | (0<<ISC10) | (0<<ISC01) | (0<<ISC00);
MCUCSR=(0<<ISC2);

// USART initialization
// USART disabled
UCSRB=(0<<RXCIE) | (0<<TXCIE) | (0<<UDRIE) | (0<<RXEN) | (0<<TXEN) | (0<<UCSZ2) | (0<<RXB8) | (0<<TXB8);

// Analog Comparator initialization
// Analog Comparator: Off
// The Analog Comparator's positive input is
// connected to the AIN0 pin
// The Analog Comparator's negative input is
// connected to the AIN1 pin
ACSR=(1<<ACD) | (0<<ACBG) | (0<<ACO) | (0<<ACI) | (0<<ACIE) | (0<<ACIC) | (0<<ACIS1) | (0<<ACIS0);

// ADC initialization
// ADC Clock frequency: 125.000 kHz
// ADC Voltage Reference: AREF pin
// ADC Auto Trigger Source: ADC Stopped
ADMUX=ADC_VREF_TYPE;
ADCSRA=(1<<ADEN) | (0<<ADSC) | (0<<ADATE) | (0<<ADIF) | (1<<ADIE) | (1<<ADPS2) | (0<<ADPS1) | (1<<ADPS0);
SFIOR=(0<<ADTS2) | (0<<ADTS1) | (0<<ADTS0);

// SPI initialization
// SPI disabled
SPCR=(0<<SPIE) | (0<<SPE) | (0<<DORD) | (0<<MSTR) | (0<<CPOL) | (0<<CPHA) | (0<<SPR1) | (0<<SPR0);

// TWI initialization
// TWI disabled
TWCR=(0<<TWEA) | (0<<TWSTA) | (0<<TWSTO) | (0<<TWEN) | (0<<TWIE);

// 1 Wire Bus initialization
// 1 Wire Data port: PORTB
// 1 Wire Data bit: 0
// Note: 1 Wire port settings are specified in the
// Project|Configure|C Compiler|Libraries|1 Wire menu.
w1_init();

// Global enable interrupts
#asm("sei")

/////////////////////////////
SHRG_L_LAT	=1;
SHRG_S_LAT	=1;
LED_PWR		=1;
Flg.Seg_Tmprtr_Vw	=1;
Sts.Mode	=MOD_ECO;
Sts.Rgb =1;
//ADMUX	=0xc7;
WDTCR   =0x8;
/////////////////////////////
//eeprom load
for	( i=0;	i<EEP_SIZE;	i++)
	LEep.All[i]	=Eep[i];
Sts.TmprtrSP10x	=LEep.Sub.StPnt10xl+	(LEep.Sub.StPnt10xh	<<8);
if	(Sts.TmprtrSP10x	>	((10*TMPRTR_MAX)-1))
	Sts.TmprtrSP10x	=	(10*TMPRTR_MAX)-1;
Sts.Mode	=LEep.Sub.Mode;
if	( (Sts.Mode	!=MOD_ECO)	&&	(Sts.Mode	!=MOD_AUTO)	&&	(Sts.Mode	!=MOD_MANL))
	Sts.Mode	=MOD_ECO;
Sts.Spd	=	LEep.Sub.Spd	&0x3;
if	( Sts.Spd	==0)
	Sts.Spd	=0;
if	( LEep.Sub.Hot1Cld0	)
	Flg.Sum1Win0	=1;	
/////////////////////////////
while (1)
      {
      if	( Tick)
        {
            Tick	=0;
            if	( ( TickCnt	&0x7)	==0)
            {	//each 	8ms   
                #asm("wdr")
                F_ScanTouch_RdDI	();
                if	( Flg.NewKey)
                {
                    F_KeyAnlz	();
                    Flg.NewKey	=0;    
                }
            }  
            else
            {
                switch	( TickCnt	&0x3f)
                {	//each	64ms
                    case	0x04:
                        F_Anlg_RdAI		();
                        F_ShRg_SpiSw	();
                        F_Led_WrDo		();
                        F_Rly_WrDo		();
                        F_TmOut			();
                    break;
                    case	0x14:
                        //F_Calc	();
                        F_Disp	();
                    break;
                    case	0x24:
                        //F_ClkDS1307_RdI2C	();
                    break;
                    case	0x34:
                        //F_Esp12_Uart	();
                    break;
                }
            }    
            SLEEP;
        }
      }
}