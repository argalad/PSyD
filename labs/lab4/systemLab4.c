#include <s3c44b0x.h>
#include "systemLab4.h"    

static void port_init( void )
{
	PDATA = ~0;
	PCONA = 0xFE;

	PDATB = ~0;
	PCONB = 0x14F;

	PDATC = ~0;
	PCONC = 0x5FF555FF;
	PUPC  = 0x94FB;

	PDATD = ~0;
	PCOND = 0xAAAA;
	PUPD  = 0xFF;

	PDATE = ~0;
	PCONE = 0x255A9;
	PUPE  = 0x1FB;

	PDATF = ~0;
	PCONF = 0x251A;
	PUPF  = 0x74;

	PDATG = ~0;
	PCONG = 0xF5FF;
	PUPG  = 0x30;

	SPUCR = 0x7;

	EXTINT = 0x22000220;
}

void sys_init( void )
{
    WTCON  = 0; // Watchdog deshabilitado
    INTMSK = ~0; // Enmascara todas las interrupciones

    LOCKTIME = 0xFFF; // Estabilización de PLL: 512 us
    PLLCON   = 0x38021; // Frecuencia del MCLK_SLOW: 500KHz
    CLKSLOW  = 0x8; // Frecuencia del MCLK: 64MHz
    CLKCON   = 0x7FF8; // Modo de funcionamiento normal y reloj distribuido a todos los controladores
    
    SBUSCON = 0x8000001B; // Prioridades de bus del sistema fijas: LCD > ZDMA > BDMA > IRQ (por defecto)
    
    SYSCFG = 0x0; // Cache deshabilitada

    port_init();
}


