#include <s3c44b0x.h>
#include <s3cev40.h>
#include <pbs.h>
#include <timers.h>

extern void isr_PB_dummy( void );

void pbs_init( void )
{
    timers_init();
}

uint8 pb_scan( void )
{
    if( pb_status(PB_LEFT) == PB_DOWN )
        return PB_LEFT;
    else if( pb_status(PB_RIGHT) == PB_DOWN )
        return PB_RIGHT;
    else
        return PB_FAILURE;
}

uint8 pb_status( uint8 scancode )
{
    if( (scancode & PDATG) == 0 )
        return PB_DOWN;
    else
        return PB_UP;
    
}

void pb_wait_keydown( uint8 scancode )
{
    while( (PDATG & scancode) != 0 );
    sw_delay_ms( PB_KEYDOWN_DELAY ); 
}

void pb_wait_keyup( uint8 scancode )
{
	 pb_wait_keydown(scancode);
     while( (PDATG & scancode) == 0 );
     sw_delay_ms( PB_KEYUP_DELAY );
}

void pb_wait_any_keydown( void )
{
    while( ((PDATG & 0x40) == 0x40) && ((PDATG & 0x80) == 0x80) );
    sw_delay_ms( PB_KEYDOWN_DELAY );
}

void pb_wait_any_keyup( void )
{
    while( ((PDATG & 0x40) == 0x40) && ((PDATG & 0x80) == 0x80) );
    sw_delay_ms( PB_KEYUP_DELAY );
    while( ((PDATG & 0x40) == 0x0) || ((PDATG & 0x80) == 0x0) );
}

uint8 pb_getchar( void )
{
    uint8 scancode;

    while( ((PDATG & 0x40) == 0x40) && ((PDATG & 0x80) == 0x80));
    sw_delay_ms( PB_KEYDOWN_DELAY );
    scancode = pb_scan();

    while( (scancode & PDATG) == 0 );
    sw_delay_ms( PB_KEYUP_DELAY );

    return scancode;
}

uint8 pb_getchartime( uint16 *ms )
{
    uint8 scancode;
    
    while( ((PDATG & 0x40) == 0x40) && ((PDATG & 0x80) == 0x80) );
    timer3_start();
    sw_delay_ms( PB_KEYDOWN_DELAY );
    
    scancode = pb_scan();
    
    while( (scancode & PDATG) == 0 );
    *ms = timer3_stop() / 10;
    sw_delay_ms( PB_KEYUP_DELAY );

    return scancode;
}

uint8 pb_timeout_getchar( uint16 ms )
{
    uint8 scancode;
    timer3_start_timeout(ms);
    while( timer3_timeout() && ((PDATG & 0x40) == 0x40) && ((PDATG & 0x80) == 0x80) );
    if( !timer3_timeout())
        return PB_TIMEOUT;
    else
    {
        sw_delay_ms( PB_KEYDOWN_DELAY );
        scancode = pb_scan();
        while( (scancode & PDATG) == 0 );
        sw_delay_ms( PB_KEYUP_DELAY );
        return scancode;
    }
    
}

void pbs_open( void (*isr)(void) )
{
    pISR_PB   = (uint32) isr;
    EXTINTPND = 0xf;
    I_ISPC    = BIT_EINT4567;
    INTMSK   &= ~(BIT_GLOBAL | BIT_EINT4567);
}

void pbs_close( void )
{
    INTMSK  |= BIT_EINT4567;
    pISR_PB  = (uint32) isr_PB_dummy;
}
