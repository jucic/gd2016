/*
 * init.c
 *
 *  Created on: Dec 26, 2015
 *      Author: lenovo
 */

#include "includes.h"

void initALL(void)
{
	disableWatchdog();
	initModesAndClock();
	initEMIOS_0MotorAndSteer();
//	initEMIOS_0Image();
//	initLINFlex_0_UART();
//	initOLED();
//	initKeys_Switchs_Infrared();
	initTestIO();
	enableIrq();
}

/*********************************************************************************************/
/*********************************  �رտ��Ź� **********************************************/
/*********************************************************************************************/
void disableWatchdog(void)
{
	SWT.SR.R = 0x0000c520; /* Write keys to clear soft lock bit */
	SWT.SR.R = 0x0000d928;
	SWT.CR.R = 0x8000010A; /* Clear watchdog enable (WEN) */
}

//*****************************************************************************************************************
//*			 *************************ʱ�ӳ�ʼ�� *******************************************************    	  *
//*****************************************************************************************************************
void initModesAndClock(void) 
{
	ME.MER.R = 0x0000001D;          /* Enable DRUN, RUN0, SAFE, RESET modes */
	//Initialize PLL before turning it on                    
	//����sysclk
	//CGM.FMPLL_CR.R = 0x02400100;    /* 8 MHz xtal: Set PLL0 to 64 MHz */  
	CGM.FMPLL_CR.R = 0x01280000;      /* 8 MHz xtal: Set PLL0 to 80 MHz */
	//CGM.FMPLL_CR.R = 0x013C0000;    /* 8 MHz xtal: Set PLL0 to 120 MHz */   
	ME.RUN[0].R = 0x001F0064;       /* RUN0 cfg: 16MHzIRCON,OSC0ON,PLL0ON,syclk=PLL     sysclkѡ�����໷ʱ��*/
	ME.RUNPC[0].R = 0x00000010;    /* Peri. Cfg. 1 settings: only run in RUN0 mode      ѡ��RUN0ģʽ*/
	/*************************PCTL[?] ѡ����Ҫʱ��ģ��****************************/
	ME.PCTL[32].R = 0x00; 			/* MPC56xxB/P/S ADC 0: select ME.RUNPC[0] */
	ME.PCTL[48].R = 0x00;           /* MPC56xxB/P/S LINFlex 0: select ME.RUNPC[0] */
	ME.PCTL[68].R = 0x00;           /* MPC56xxB/S SIUL:  select ME.RUNPC[0] */ 
	ME.PCTL[72].R = 0x00;           /* MPC56xxB/S EMIOS 0:  select ME.RUNPC[0] */ 
	//Mode Transition to enter RUN0 mode      
	ME.MCTL.R = 0x40005AF0;         /* Enter RUN0 Mode & Key */
	ME.MCTL.R = 0x4000A50F;         /* Enter RUN0 Mode & Inverted Key */ 
	    
	while (ME.GS.B.S_MTRANS) {}     // Wait for mode transition to complete �ȴ�ģʽת�����    
	/********************************** Note: could wait here using timer and/or I_TC IRQ*/                          
	while(ME.GS.B.S_CURRENTMODE != 4) {} // Verify RUN0 is the current mode �ȴ�ѡ��RUN0ģʽ
	//��peri0��1��2
	CGM.SC_DC[0].R = 0x80;//LIN
	CGM.SC_DC[1].R = 0x80;//FLEXCAN,DSPI
	CGM.SC_DC[2].R = 0x80;//eMIOS,CTU,ADC
}

//*****************************************************************************************************************
//*************************	PWM��ʼ��:�������תE3��E4��E5��E6��������A4  **********************************************
//*****************************************************************************************************************
void initEMIOS_0MotorAndSteer(void)
{
  //eMIOS0��ʼ��80MHz��Ϊ1MHz
	EMIOS_0.MCR.B.GPRE= 31;   //GPRE+1=��Ƶϵ����/* Divide 80 MHz sysclk by 31+1 = 32 for 2.5MHz(0.4us) eMIOS clk*/
	EMIOS_0.MCR.B.GPREN = 1;	/* Enable eMIOS clock */
	EMIOS_0.MCR.B.GTBE = 1;   /* Enable global time base */
	EMIOS_0.MCR.B.FRZ = 1;    /* Enable stopping channels when in debug mode */	
  /**********���PWM 5kHZ E3��E4��E5��E6*********************************************************************************/ 
	//eMIOS0Dͨ��16����/* EMIOS 0 CH 16: Modulus Up Counter */
    EMIOS_0.CH[16].CCR.B.UCPRE=0;	    /* Set channel prescaler to divide by 1 */
	EMIOS_0.CH[16].CCR.B.UCPEN = 1;   /* Enable prescaler; uses default divide by 1 */
	EMIOS_0.CH[16].CCR.B.FREN = 1; 	/* Freeze channel counting when in debug mode */
	EMIOS_0.CH[16].CADR.R = 500;	/********��������200us  5kHZ********/
	EMIOS_0.CH[16].CCR.B.MODE = 0x50; /* Modulus Counter Buffered (MCB) */
	EMIOS_0.CH[16].CCR.B.BSL = 0x3;	/* Use internal counter */
	/*E3(EMIOS 0 CH 19)��ǰ�����: Output Pulse Width Modulation */
	EMIOS_0.CH[19].CCR.B.BSL = 0x1;//use counter bus D
	EMIOS_0.CH[19].CCR.B.MODE = 0x60;//Mode is OPWM Buffered
	EMIOS_0.CH[19].CCR.B.EDPOL = 1;//Polarity-leading edge sets output/trailing clears
	EMIOS_0.CH[19].CADR.R = 0;//Leading edge when channel counter bus=250
	EMIOS_0.CH[19].CBDR.R = 0;//Trailing edge when channel counter bus=500
	SIU.PCR[67].R = 0x0600;//Assign EMIOS_0 CH19 to Pad
	/*E4(EMIOS 0 CH 20)��������: Output Pulse Width Modulation */
	EMIOS_0.CH[20].CCR.B.BSL = 0x1;//use counter bus D
	EMIOS_0.CH[20].CCR.B.MODE = 0x60;//Mode is OPWM Buffered
	EMIOS_0.CH[20].CCR.B.EDPOL = 1;//Polarity-leading edge sets output/trailing clears
	EMIOS_0.CH[20].CADR.R = 0;//Leading edge when channel counter bus=250
	EMIOS_0.CH[20].CBDR.R = 0;//Trailing edge when channel counter bus=500
	SIU.PCR[68].R = 0x0600;//Assign EMIOS_0 CH20 to Pad
	 /* E5(EMIOS 0 CH 21)��ǰ�����: Output Pulse Width Modulation */
	EMIOS_0.CH[21].CCR.B.BSL = 0x1;	/* Use counter bus D (default) */
	EMIOS_0.CH[21].CCR.B.MODE = 0x60; /* Mode is OPWM Buffered */
    EMIOS_0.CH[21].CCR.B.EDPOL = 1;	/* Polarity-leading edge sets output/trailing clears*/
	EMIOS_0.CH[21].CADR.R = 0;     /* Leading edge when channel counter bus=250*/
	EMIOS_0.CH[21].CBDR.R = 0;      /* Trailing edge when channel counter bus=500*/
	SIU.PCR[69].R = 0x0600;    //[11:10]ѡ��AFx �˴�AF1 /* MPC56xxS: Assign EMIOS_0 ch 21 to pad */
	/* E6��EMIOS 0 CH 22���Һ������: Output Pulse Width Modulation */
	EMIOS_0.CH[22].CCR.B.BSL = 0x1;	/* Use counter bus D (default) */
	EMIOS_0.CH[22].CCR.B.MODE = 0x60; /* Mode is OPWM Buffered */
    EMIOS_0.CH[22].CCR.B.EDPOL = 1;	/* Polarity-leading edge sets output/trailing clears*/
	EMIOS_0.CH[22].CADR.R = 0;     /* Leading edge when channel counter bus=250*/
	EMIOS_0.CH[22].CBDR.R = 0;     /* Trailing edge when channel counter bus=500*/
	SIU.PCR[70].R = 0x0600;   //[11:10]ѡ��AFx �˴�AF1 /* MPC56xxS: Assign EMIOS_0 ch 21 to pad */
	
	/**********���PWM 50HZ A11�����50000*7.5%=3750��λ**********/
	//eMIOS0 Aͨ��23����/* EMIOS 0 CH 0: Modulus Counter */
	EMIOS_0.CH[23].CCR.B.UCPRE=0;	    /* Set channel prescaler to divide by 1 */
	EMIOS_0.CH[23].CCR.B.UCPEN = 1;   /* Enable prescaler; uses default divide by 1 */
	EMIOS_0.CH[23].CCR.B.FREN = 1; 	/* Freeze channel counting when in debug mode */
	EMIOS_0.CH[23].CADR.R = 50000;/********��������20ms  50HZ*******/
	EMIOS_0.CH[23].CCR.B.MODE = 0x50; /* Modulus Counter Buffered (MCB) */
	EMIOS_0.CH[23].CCR.B.BSL = 0x3;	/* Use internal counter */
		
	/* EMIOS 0 CH 11: Output Pulse Width Modulation */
	EMIOS_0.CH[11].CCR.B.BSL = 0;	/* Use counter bus A (default) */
	EMIOS_0.CH[11].CCR.B.MODE = 0x60; /* Mode is OPWM Buffered */  
	EMIOS_0.CH[11].CCR.B.EDPOL = 1;	/* Polarity-leading edge sets output/trailing clears*/
	EMIOS_0.CH[11].CADR.R = 0;//��ռ�ձ�/* Leading edge when channel counter bus=250*/
	EMIOS_0.CH[11].CBDR.R = 0;            /* Trailing edge when channel counter bus=500*/
	SIU.PCR[11].R = 0x0600;    //[11:10]ѡ��AFx �˴�AF1   A4�ڶ�����
}

//*****************************************************************************************************************
//****************************************�жϳ�ʼ��******************************************************    	  *
//*****************************************************************************************************************
void enableIrq(void) 
{
  INTC.CPR.B.PRI = 0;          /* Single Core: Lower INTC's current priority */
  asm(" wrteei 1");	    	   /* Enable external interrupts */
}



/*********************����IO��ʼ��***********************/
void initTestIO(void)
{
	SIU.PCR[24].R = 0x0200;//CCDL AO  B8
	SIU.PCR[27].R = 0x0200;//CCDL CLK B11
	SIU.PCR[61].R = 0x0200;//CCDL SI  D13
	SIU.PCR[26].R = 0x0200;//CCDR AO  B10
	SIU.PCR[63].R = 0x0200;//CCDR CLK D15
	SIU.PCR[62].R = 0x0200;//CCDR SI  D14
	SIU.PCR[25].R = 0x0200;//CCDM AO  B9
	SIU.PCR[46].R = 0x0200;//CCDM CLK C14
	SIU.PCR[2].R = 0x0200; //CCDM SI  A2
	SIU.PCR[59].R = 0x0200;//COUNTER1 D11
	SIU.PCR[60].R = 0x0200;//COUNTER1 D12
	SIU.PCR[6].R = 0x0200; //COUNTER2 A6
	SIU.PCR[8].R = 0x0200; //COUNTER1 A8
	SIU.PCR[72].R = 0x0200;//OLED     E8
	SIU.PCR[74].R = 0x0200;//OLED     E10
	SIU.PCR[75].R = 0x0200;//OLED     E11
	SIU.PCR[42].R = 0x0200;//OLED     C10
	SIU.PCR[17].R = 0x0200;//OLED     B1
	SIU.PCR[0].R = 0x0200; //BEE      A0
	SIU.PCR[9].R = 0x0200; //SUPER1   A9
	SIU.PCR[5].R = 0x0200; //SUPER1   A5
	SIU.PCR[66].R = 0x0200;//SUPER1   E2
	SIU.PCR[18].R = 0x0200;//UART     B2
	SIU.PCR[19].R = 0x0200;//UART     B3
	SIU.PCR[50].R = 0x0103;//SWITCH   D2
	SIU.PCR[52].R = 0x0103;//SWITCH   D4
	SIU.PCR[54].R = 0x0103;//SWITCH   D6
	SIU.PCR[56].R = 0x0103;//SWITCH   D8
	SIU.PCR[28].R = 0x0103;//KEY S6   B12
	SIU.PCR[29].R = 0x0103;//KEY S5   B13
	SIU.PCR[30].R = 0x0103;//KEY S4   B14
	SIU.PCR[31].R = 0x0103;//KEY S3   B15
	
	SIU.GPDO[24].R = 0;//CCDL AO  B8
	SIU.GPDO[27].R = 0;//CCDL CLK B11
	SIU.GPDO[61].R = 0;//CCDL SI  D13
	SIU.GPDO[26].R = 0;//CCDR AO  B10
	SIU.GPDO[63].R = 0;//CCDR CLK D15
	SIU.GPDO[62].R = 0;//CCDR SI  D14
	SIU.GPDO[25].R = 0;//CCDM AO  B9
	SIU.GPDO[46].R = 0;//CCDM CLK C14
	SIU.GPDO[2].R = 0; //CCDM SI  A2
	SIU.GPDO[59].R = 0;//COUNTER1 D11
	SIU.GPDO[60].R = 0;//COUNTER1 D12
	SIU.GPDO[6].R = 0; //COUNTER2 A6
	SIU.GPDO[8].R = 0; //COUNTER1 A8
	SIU.GPDO[72].R = 0;//OLED     E8
	SIU.GPDO[74].R = 0;//OLED     E10
	SIU.GPDO[75].R = 0;//OLED     E11
	SIU.GPDO[42].R = 0;//OLED     C10
	SIU.GPDO[17].R = 0;//OLED     B1
	SIU.GPDO[0].R = 0; //BEE      A0
	SIU.GPDO[9].R = 0; //SUPER1   A9
	SIU.GPDO[5].R = 0; //SUPER1   A5
	SIU.GPDO[66].R = 0;//SUPER1   E2
	SIU.GPDO[18].R = 0;//UART     B2
	SIU.GPDO[19].R = 0;//UART     B3
}