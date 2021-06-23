#include	<delay.h>
//fuse bits:	(high)0xd1		(low)0xe3
///////////////////////////////////////////////////////////
#define 	SLEEP	       do  {	MCUCR	=0x90;	#asm	("sleep");\
									MCUCR	=0x00;}    while(0);	//sleep for adc noise reduction
#define		EEP_SIZE		(10)
#define		KEY_DBNC		(5)		//*8ms
#define		KEY_LNG			(64-1)	//*8ms
#define		KEY_SHRT		(16-1)	//*8ms
#define		KEY_SHTTOLNG	(4)
#define		TO_SETPNTVW		(16)	//*64ms
#define		TO_SEGNULL		(2-1)	//*64ms
#define		TMPRTR_MAX		(30)
#define		TMPRTR_MIN		(20)
#define		TMPRTR_BUFSZ	(10)
			
///////////////////

#define		SET_ALLLED		PORTB	|=	 0x3c;
#define		RST_ALLLED		PORTB	&=	~0x3c;
#define		MOD_ECO			(0x01)
#define		MOD_AUTO		(0x02)
#define		MOD_MANL		(0x03)

///////////////////

#define		LED_PWR		PORTB.1	
#define		LED_DCR		PORTB.2	
#define		LED_INC		PORTB.3	
#define		LED_MODL	PORTB.4	
#define		LED_MODR	PORTB.5	
///////////////////
//#define		RLY_ACT		PORTA.0
//#define		RLY_CLDHOT	PORTA.1
//#define		RLY_LOWMDM	PORTA.2
//#define		RLY_FST		PORTA.3
///////////////////
#define		RLY_PWR			PORTA.0	
#define		RLY_LOW_MDHI	PORTA.1	
#define		RLY_MD_HI		PORTA.2	
#define		RLY_CLDHOT		PORTA.3	
///////////////////
#define		SHRG_S_DAT	PORTA.4	
#define		SHRG_S_LAT	PORTA.5	
#define		SHRG_S_CLK	PORTA.6	
///////////////////
#define		SHRG_L_DAT	PORTD.5	
#define		SHRG_L_LAT	PORTD.6	
#define		SHRG_L_CLK	PORTD.7	
///////////////////
//PC0-3	=>touch
///////////////////////////////////////////////////////////
eeprom	unsigned char Eep[EEP_SIZE];

unsigned char Tick,	TickCnt;
struct
{
	unsigned char 	Pwr;
	unsigned char 	Sum1Win0;
	unsigned char	NewKey;
	unsigned char	NewKeyFst1Slw0;
	
	unsigned char 	Seg_Tmprtr_Vw;
	unsigned char 	Seg_SetPnt_Vw;
	unsigned char 	Seg_Null;
	unsigned char 	aaaa;
}	Flg;
struct	
{
	unsigned int 	LstAdc;
	unsigned int 	Tmprtr10x,	TmprtrSP10x;
	unsigned char 	Key;
	unsigned char 	ShrgL[4],	ShrgS;
	unsigned char 	Mode,	Spd;
	unsigned char 	Rgb;
	unsigned char 	aaaa;
	unsigned char 	aaaa;
}	Sts;
///////////////////////////////////////////////////////////
void	F_ScanTouch_RdDI	(void);
void	F_KeyAnlz			(void);

void	F_Calc				(void);////////////
void	F_Disp				(void);

void	F_Dlyus				(unsigned char x);

void	F_Anlg_RdAI			(void);
void	F_ShRg_SpiSw		(void);
void	F_Led_WrDo			(void);
void	F_Rly_WrDo			(void);
//void	F_ClkDS1307_RdI2C	(void);	////////////
//void	F_Esp12_Uart		(void);	////////////
///////////////////////////////////////////////////////////	
INT_TIM0
{
	Tick	=1;
	TickCnt++;
}
///////////////////////////////////////////////////////////
main:
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

TCCR0	=0x23;	//pre=64,	CTC mode
OCR0	=62;
//TCCR2	=0x02;	//pre=8,	each cnt 2us
ADMUX	=0xc7;
/////////////////////////////
SHRG_L_LAT	=1;
SHRG_S_LAT	=1;
LED_PWR		=1;
Flg.Seg_Tmprtr_Vw	=1;
Sts.Mode	=MOD_ECO;
/////////////////////////////
//eeprom load
for	( i=0;	i<EEP_SIZE;	i++)
	LEep[i]	=Eep[i];
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
	Flg.Hot1Cld0	=1;	
///////////////////////////////////////////////////////////
if	( Tick)
{
	Tick	=0;
	if	( ( TickCnt	&0x7)	==0)
	{	//each 	8ms
		F_ScanTouch_RdDI	();
		if	( Flg.NewKey)
		{
			F_KeyAnlz	();
			Flg.NewKey	=0;
		}
	}
	else
	{
		switch	( TickCnt	&0x3e)
		{	//each	64ms
			case	0x04:
				F_Anlg_RdAI		();
				F_ShRg_SpiSw	();
				F_Led_WrDo		();
				F_Rly_WrDo		();
				F_TmOut			();
			break;
			case	0x14:
				F_Calc	();
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
///////////////////////////////////////////////////////////
void	F_ScanTouch_RdDI	(void)
{
	static	unsigned char	LKeyn,	LKeyo,	LCntShrt,	i;
	static	unsigned int 	LCnt;
		
	LKeyn=	PINB	&0xf;	

	
	if	( LKeyn	!=	LKeyo)
	{
		LKeyo	=LKeyn;
		LCntShrt	=0;
		Sts.NewKeyFst1Slw0	=0;
		
		if	( LCnt	>KEY_SHRT)
			LCnt	-=KEY_SHRT;
		else	if	( LCnt	>KEY_DBNC)
			LCnt	-=KEY_DBNC;
		else
			LCnt	=0;
	}
	else
	{
		if	( LKeyn	==0xf)
		{
			Sts.Key	=0xf;
			LCnt	=0;
		}
		else
		{			
			if		(LCnt	<KEY_DBNC)		//5*8ms
				LCnt++;
			else
			{
				Sts.Key	=LKeyn;
				if	(LCnt	==KEY_DBNC)
				{
					LCntShrt++;
					Flg.NewKey	=1;
				}
				
				LCnt++;
				if	(LCntShrt	<KEY_SHTTOLNG)
				{
					//Sts.NewKeyFst1Slw0	=0;
					if	(LCnt	==	KEY_LNG)
					{
						LCnt	=0;
						LCntShrt++;
					}
				}
				else
				{
					Sts.NewKeyFst1Slw0	=1;
					if	(LCnt	==	KEY_LNG)
					//if	(LCnt	==	KEY_SHRT)
					{
						LCnt	=0;
						LCntShrt++;
					}
				}
			}
		}
	}
}
///////////////////////////////////////////////////////////
void	F_KeyAnlz			(void)
{
	//		4_DCR		2_INC
	//		8_MODL		10_MODR
	//			12_PWR
			
	//unsigned char 	Seg_SetPnt_Vw;
	//unsigned char 	Seg_Null;
	
	if	( Sts.Key	==0xc)
		Flg.Pwr	=1-	Flg.Pwr;
	
	if	( Flg.Pwr)
	{
		switch	( Sts.Key)
		{
			case	0x02:
				if	( Flg.Seg_Tmprtr_Vw)
					Flg.Seg_Null	=1;
				else	if	( Flg.SetPnt_Vw)
				{					
					if	( Sts.NewKeyFst1Slw0	==0)
					{
						if	( Sts.TempSP10x	<( ( TMPRTR_MAX	*10)-1))
							Sts.TempSP10x++;
					}
					else
					{
						if	( Sts.TempSP10x	<( ( TMPRTR_MAX	*10)-11))
							Sts.TempSP10x	+=10;
						else
							Sts.TempSP10x	= (TMPRTR_MAX	*10)-1;
					}
				}
			break;
			case	0x04:
				if	( Flg.Seg_Tmprtr_Vw)
					Flg.Seg_Null	=1;
				else	if	( Flg.SetPnt_Vw)
				{	
					if	( Sts.NewKeyFst1Slw0	==0)
					{
						if	( Sts.TempSP10x	>(TMPRTR_MIN 	*10))
							Sts.TempSP10x--;
					}
					else
					{
						if	( Sts.TempSP10x	>( ( TMPRTR_MIN	*10)+10))
							Sts.TempSP10x	-=10;
						else
							Sts.TempSP10x	= TMPRTR_MIN	*10;
					}	
				}				
			break;
			case	0x08:
				if	( Sts.NewKeyFst1Slw0	==0)
				{
					if	( Sts.Mode	==MOD_MANL)
					{					
						Sts.Spd	=(Sts.Spd+1)	&0x3;
						if	( Sts.Spd	==0)
							Sts.Spd	=1;
					}
				}
				else
				{
					if	( Sts.Mode	==MOD_ECO)
						Sts.Mode	=MOD_AUTO;
					else	if	( Sts.Mode	==MOD_AUTO)
						Sts.Mode	=MOD_MANL;
					else
						Sts.Mode	=MOD_ECO;
				}				
			break;
			case	0x0a:
				if	( Sts.NewKeyFst1Slw0	==0)
					Sts.Rgb	=(Sts.Rgb+1)	&0x7;
				else
					Flg.Sum1Win0	=1-	Flg.Sum1Win0;
			break;
		}
	}
	else
	{
		Flg.Seg_Tmprtr_Vw	=1;
		Flg.Seg_SetPnt_Vw	=0;
		Flg.Seg_Null		=0;
	}
}
///////////////////////////////////////////////////////////
void	F_Anlg_RdAI			(void)
{
	ADCSRA	=0xcd;
	ADCSRA	|=	0x10;
	ADCSRA	|=	0x40;
	SLEEP;
	while	( ( ADCSRA	&0x10)	==0));	//about 100us
	Sts.LstAdc	=ADCW;		
}
///////////////////////////////////////////////////////////
void	F_TmOut		(void)
{
	static	unsigned	int		LCntSeg,	LCntStPntVw;

	if	( Flg.Seg_Null	==0)
		LCntSeg	=0;
	else
	{
		LCntSeg++;
		if	( LCntSeg	>TO_SEGNULL)
		{
			Flg.Seg_Null	=0;
			if	( Flg.Seg_Tmprtr_Vw)
			{
				Flg.Seg_Tmprtr_Vw	=0;
				Flg.SetPntVw		=1;
			}
			else	if	( Flg.SetPntVw)
			{
				Flg.Seg_Tmprtr_Vw	=1;
				Flg.SetPntVw		=0;
			}
	}
	
	if	( Flg.SetPntVw	==0)
		LCntStPntVw	=0;
	else
	{
		if	( LCntStPntVw	<TO_SETPNTVW)
			LCntStPntVw++;
		else	if	( LCntStPntVw	==TO_SETPNTVW)
		{
			Flg.Seg_Null	=1;
			LCntStPntVw++;
			//save to eep
		}			
	}
}
///////////////////////////////////////////////////////////
void	F_Calc		(void)
{
	static	unsigned int 	LTmprtr[TMPRTR_BUFSZ],	LAdcMean;
	static	signed	int		LDiff;
	static	unsigned char 	i,j;
	////////////////////	
	LTmprtr[i++]	=Sts.LstAdc;
	if	( i	>=TMPRTR_BUFSZ)
		i=0;
	LAdcMean	=0;
	j	=0;
	for	( i=0;	i<TMPRTR_BUFSZ;	i++)
		if	( LTmprtr[i])
		{
			LAdcMean	+=LAdcMean[i];
			j++;
		}
	////////////////////	
	LAdcMean	=(10*	LAdcMean)	/j;
	Sts.Tmprtr10x	=LAdcMean	/4;
	
	LDiff=Sts.Tmprtr10x	-Sts.TmprtrSP10x;
	if	(	LDiff	<0)
		LDiff	=-LDiff;
	////////////////////
	if	( Sts.Mode	==MOD_ECO)
	{
		if	( LDiff	>5)
			Sts.Spd	=1;
		else
			Sts.Spd	=0;
	}
	else	if	( Sts.Mode	==MOD_AUTO)
	{
		if	( LDiff	>70)
			Sts.Spd	=3;
		else	if	( LDiff>40)
			Sts.Spd	=2;
		else
			Sts.Spd	=1;
	}
	////////////////////
}
///////////////////////////////////////////////////////////
void	F_Disp		(void)
{
//ShrgS:		x	x	B	R		G	B	R	G
//ShrgL:		Led	SgL	SgM	SgR
//ShrgL_Led:	ECO	CLD	HOT	AUTO	LOW	MDM	HI	CILC
	static	unsigned char Lx;
	static	unsigned char LSeg[]=	{0x3f,	0x06,	0x5b,	0x4f,	0x66,\
									0x6d,	0x7c,	0x07,	0x7f,	0x67};
	////////////////////
	Sts.ShrgS		=0;
	Sts.ShrgL[0]	=0;
	Sts.ShrgL[1]	=0;
	Sts.ShrgL[2]	=0;
	Sts.ShrgL[3]	=0;
	////////////////////
	if	( Flg.Pwr)
	{
		if	( Flg.Sum1Win0)
			Sts.ShrgL[0]	|=	0x20;
		else
			Sts.ShrgL[0]	|=	0x40;
	////////////////////
		if	( Sts.Mode	==MOD_ECO)
			Sts.ShrgL[0]	|=	0x80;
		else	if	( Sts.Mode	==MOD_AUTO)
			Sts.ShrgL[0]	|=	0x10;
	////////////////////
		if			( Sts.Spd	==1)
			Sts.ShrgL[0]	|=	0x08;
		else	if	( Sts.Spd	==2)
			Sts.ShrgL[0]	|=	0x0c;
		else	if	( Sts.Spd	==3)
			Sts.ShrgL[0]	|=	0x0e;
	////////////////////
		if	( Flg.Seg_Null	==0)
		{
			if	( Flg.Seg_SetPnt_Vw)
			{
				Lx	=TmprtrSP10x	/100;
				Sts.ShrgL[1]	=Lseg[Lx];
				Lx	=(TmprtrSP10x	/10)	-(10*Lx);
				Sts.ShrgL[2]	=Lseg[Lx];
				Lx	=TmprtrSP10x	%10;
				Sts.ShrgL[3]	=Lseg[Lx];
			}		
			Sts.ShrgL[0]	|=0x01;
			Sts.ShrgL[2]	|=0x80;
		}
	////////////////////
		Sts.ShrgS	=(Sts.Rgb	<<3)	|	(Sts.Rgb);
	}
	else
	{
		Lx	=Tmprtr10x	/100;
		Sts.ShrgL[1]	=Lseg[Lx];
		Lx	=(Tmprtr10x	/10)	-(10*Lx);
		Sts.ShrgL[2]	=Lseg[Lx];
		Lx	=Tmprtr10x	%10;
		Sts.ShrgL[3]	=Lseg[Lx];
		
		Sts.ShrgL[0]	|=0x01;
		Sts.ShrgL[2]	|=0x80;
		
		Sts.ShrgS		=0x02;
	}
}
///////////////////////////////////////////////////////////
/*
void	F_Dlyus		(unsigned char	x)
{
	//each cnt=>2us
	TCNT2	=0;
	while	(TCNT2	<x);
}
*/
///////////////////////////////////////////////////////////
void 	F_ShRg_SpiSw		(void)
{
	static	unsigned char	i,	j,	Lbit,	Lx;
///////////////////
	Lx	=0x10;	//F_Dlyus		(1);

	for	( i=0;	i<4;	i++)
	{
		Lbit	=0x80;		
		for	( j=8;	j;	j--)
		{
			if	(	(	Sts.ShrgS		&	Lbit)	==0)
				SHRG_S_DAT	=0;
			else
				SHRG_S_DAT	=1;
			
			if	(	(	Sts.ShrgL[i]	&	Lbit)	==0)
				SHRG_L_DAT	=0;
			else
				SHRG_L_DAT	=1;			
			
			Lx=	(Lx	>>1)	+0x10;	//F_Dlyus		(1);
			SHRG_S_CLK	=1;
			SHRG_L_CLK	=1;
			Lx=	(Lx	>>1)	+0x10;	//F_Dlyus		(1);
			SHRG_S_CLK	=0;
			SHRG_L_CLK	=0;
			Lbit		>>=1;
		}
	}
	
	SHRG_S_LAT	=0;
	SHRG_L_LAT	=0;
	Lx=	(Lx	>>1)	+0x10;	//F_Dlyus	(1);
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
		if	( Sts.SPd	==1)	//low speed
		{
			RLY_LOW_MDHI	=1;
			RLY_MD_HI		=0;	
		}
		if	( Sts.Rly	==2)	//mdm speed
		{
			RLY_LOW_MDHI	=0;
			RLY_MD_HI		=0;
		}
		if	( Sts.Rly	==3)	//hgh speed
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