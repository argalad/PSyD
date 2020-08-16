#include <s3c44b0x.h>
#include <lcd.h>

extern uint8 font[];
static uint8 lcd_buffer[LCD_BUFFER_SIZE];

static uint8 state;

void lcd_init( void )
{      
    DITHMODE = 0x12210;
    DP1_2    = 0xA5A5;      
    DP4_7    = 0xBA5DA65;
    DP3_5    = 0xA5A5F;
    DP2_3    = 0xD6B;
    DP5_7    = 0xEB7B5ED;
    DP3_4    =  0x7DBE;
    DP4_5    = 0x7EBDF;
    DP6_7    = 0x7FDFBFE;
    
    REDLUT   = 0x0;
    GREENLUT = 0x0; 
    BLUELUT  = 0x0;

    LCDCON1  = 0x1C020;
    LCDCON2  = 0x13CEF;
    LCDCON3  = 0x0;    

    LCDSADDR1 = (2 << 27) | ((uint32)lcd_buffer >> 1);
    LCDSADDR2 = (1 << 29) | (((uint32)lcd_buffer + LCD_BUFFER_SIZE) & 0x3FFFFF) >> 1;
    LCDSADDR3 = 0x50;
    
    lcd_off();
}

void lcd_on( void )
{
	LCDCON1 |= (1 << 0);
    state = ON;
}

void lcd_off( void )
{
	LCDCON1 &= ~(1 << 0);
    state = OFF;
}

uint8 lcd_status( void )
{
    return state;
}

void lcd_clear( void )
{
    uint16 x, y;

    for(x = 0; x < LCD_WIDTH; x++)
    	for(y = 0; y < LCD_HEIGHT; y++)
    		lcd_putpixel(x, y, WHITE);

}

void lcd_putpixel( uint16 x, uint16 y, uint8 c)
{
    uint8 byte, bit;
    uint16 i;

    i = x/2 + y*(LCD_WIDTH/2);
    bit = (1-x%2)*4;
    
    byte = lcd_buffer[i];
    byte &= ~(0xF << bit);
    byte |= c << bit;
    lcd_buffer[i] = byte;
}

uint8 lcd_getpixel( uint16 x, uint16 y )
{
	uint8 color;
	uint32 word;

	word = lcd_buffer[x/8 + y*(320/8)];
	word &= (0xf0000000 >> (x%8)*4);
	color = (uint32) word >> (7 - x%8)*4;

	return color;
}

void lcd_draw_hline( uint16 xleft, uint16 xright, uint16 y, uint8 color, uint16 width )
{
	uint16 length;

    while(width--)
    {
    	length = xright - xleft + 5;
    	while(length--)
    	{
    		lcd_putpixel(xleft + length, y, color);
    	}
    	y++;
    }
}

void lcd_draw_vline( uint16 yup, uint16 ydown, uint16 x, uint8 color, uint16 width )
{
    uint16 length;

    while(width--)
    {
    	length = ydown - yup + 5;
    	while(length--)
    	{
    		lcd_putpixel(x, yup + length, color);
    	}
    	x++;
    }
}

void lcd_draw_box( uint16 xleft, uint16 yup, uint16 xright, uint16 ydown, uint8 color, uint16 width )
{
    lcd_draw_hline(xleft, xright, yup, color, width);
    lcd_draw_hline(xleft, xright, ydown, color, width);
    lcd_draw_vline(yup, ydown, xleft, color, width);
    lcd_draw_vline(yup, ydown, xright, color, width);
}

void lcd_putchar( uint16 x, uint16 y, uint8 color, char ch )
{
    uint8 line, row;
    uint8 *bitmap;

    bitmap = font + ch*16;
    for( line=0; line<16; line++ )
        for( row=0; row<8; row++ )                    
            if( bitmap[line] & (0x80 >> row) )
                lcd_putpixel( x+row, y+line, color );
            else
                lcd_putpixel( x+row, y+line, WHITE );
}

void lcd_puts( uint16 x, uint16 y, uint8 color, char *s )
{
	uint16 aux_x = x, aux_y = y;

    while (*s){
    	lcd_putchar(aux_x, aux_y, color, *s++);
    	aux_x += 8;
    }
}

void lcd_putint( uint16 x, uint16 y, uint8 color, int32 i )
{
    char buf[8 + 1];
	char *p = buf + 10;
	int8 c;
	boolean negativo = FALSE;

	*p = '\0';
	if(i < 0) {
		i *= -1;
		negativo = TRUE;
	}

	do {
		c = i % 10;
		*--p = '0' + c;
		i /= 10;
	} while (i);

	if(negativo)
		*--p = '-';

	lcd_puts(x, y, color, p);
}

void lcd_puthex( uint16 x, uint16 y, uint8 color, uint32 i )
{
    char buf[8 + 1];
    char *p = buf + 8;
    uint8 c;

    *p = '\0';

    do {
        c = i & 0xf;
        if( c < 10 )
            *--p = '0' + c;
        else
            *--p = 'a' + c - 10;
        i = i >> 4;
    } while( i );

    lcd_puts(x, y, color, p);
}

void lcd_putchar_x2( uint16 x, uint16 y, uint8 color, char ch )
{
	uint8 row, line;
	uint8 *bitmap;

	bitmap = font + ch*16;
	for(line = 0; line < 16; line++){
		for(row = 0; row < 8; row++){
			if(bitmap[line] & (0x80>>row)) {
				lcd_putpixel(2*(x+row)-18, 2*(y+line) - y, color);
				lcd_putpixel(2*(x+row)-1-18, 2*(y+line) - y, color);
				lcd_putpixel(2*(x+row)-18, 2*(y+line)+1 - y, color);
				lcd_putpixel(2*(x+row)-1-18, 2*(y+line)+1 - y, color);

			} else {
				lcd_putpixel(2*(x+row)-18, 2*(y+line) - y, WHITE);
				lcd_putpixel(2*(x+row)-1-18, 2*(y+line) - y, WHITE);
				lcd_putpixel(2*(x+row)-18, 2*(y+line)+1 - y, WHITE);
				lcd_putpixel(2*(x+row)-1-18, 2*(y+line)+1 - y, WHITE);
			}
		}
	}
}

void lcd_puts_x2( uint16 x, uint16 y, uint8 color, char *s )
{
	while(*s) {
		lcd_putchar_x2(x, y, color, *s);
		s++;
		x = x + 8;
	}
}

void lcd_putint_x2( uint16 x, uint16 y, uint8 color, int32 i )
{
	char buf[8 + 1];
	char *p = buf + 10;
	int8 c;
	boolean negativo = FALSE;

	*p = '\0';
	if(i < 0) {
		i *= -1;
		negativo = TRUE;
	}

	do {
		c = i % 10;
		*--p = '0' + c;
		i /= 10;
	} while (i);

	if(negativo)
		*--p = '-';

	lcd_puts_x2(x, y, color, p);
}

void lcd_puthex_x2( uint16 x, uint16 y, uint8 color, uint32 i )
{
	char buf[8 + 1];
	char *p = buf + 8;
	uint8 c;

	*p = '\0';

	do {
		c = i & 0xf;
		if( c < 10 )
			*--p = '0' + c;
		else
			*--p = 'a' + c - 10;
		i = i >> 4;
	} while( i );

	lcd_puts_x2(x, y, color, p);
}

void lcd_putWallpaper( uint8 *bmp )
{
    uint32 headerSize;

    uint16 x, ySrc, yDst;
    uint16 offsetSrc, offsetDst;

    headerSize = bmp[10] + (bmp[11] << 8) + (bmp[12] << 16) + (bmp[13] << 24);

    bmp = bmp + headerSize;
    
    for( ySrc=0, yDst=LCD_HEIGHT-1; ySrc<LCD_HEIGHT; ySrc++, yDst-- )                                                                       
    {
        offsetDst = yDst*LCD_WIDTH/2;
        offsetSrc = ySrc*LCD_WIDTH/2;
        for( x=0; x<LCD_WIDTH/2; x++ )
            lcd_buffer[offsetDst+x] = ~bmp[offsetSrc+x];
    }
}
