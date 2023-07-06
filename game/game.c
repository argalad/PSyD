#include <s3c44b0x.h>
#include <s3cev40.h>
#include <common_types.h>
#include <system.h>
#include <timers.h>
#include <lcd.h>
#include <pbs.h>
#include <iis.h>
#include <keypad.h>
#include <stdlib.h>
#include <rtc.h>

#define TICKS_PER_SEC (100)

/* Declaración de graficos */

#define LANDSCAPE  ((uint8 *)0x0c250000)
#define FIREMEN    ((uint8 *)0x0c260000)
#define CRASH      ((uint8 *)0x0c260800)
#define DUMMY_0    ((uint8 *)0x0c270000)
#define DUMMY_90   ((uint8 *)0x0c270400)
#define DUMMY_180  ((uint8 *)0x0c270800)
#define DUMMY_270  ((uint8 *)0x0c270C00)
#define LIFE       ((uint8 *)0x0c271000)
#define LOGO	   ((uint8 *)0x0c280000)
#define INTRO	   ((uint8 *)0x0c290000)
#define GAME_OVER  ((uint8 *)0x0c2A0000)

/* Declaración de audios */
#define MOVIMIENTO      ((int16 *)0x0c400000)
#define REBOTE      	((int16 *)0x0c420000)
#define CHOQUE      	((int16 *)0x0c440000)
#define GAMEOVER      	((int16 *)0x0c600000)

/* Tamaños en bytes de los sonidos cargados */

#define GAMEOVER_SIZE       (120000)
#define MOVIMIENTO_SIZE  	(16500)
#define REBOTE_SIZE 		(11900)
#define CHOQUE_SIZE    		(76400)

typedef struct plots {
    uint16 x;               // Posición x en donde se pinta el gráfico
    uint16 y;               // Posición y en donde se pinta el gráfico
    uint8 *plot;            // Puntero al BMP que contiene el gráfico
} plots_t;

typedef struct sprite {
    uint16 width;           // Anchura del gráfico en pixeles
    uint16 height;          // Altura del gráfico en pixeles
    uint16 num_plots;       // Número de posiciones diferentes en donde pintar el gráfico
    plots_t plots[];        // Array de posiciones en donde pintar el gráfico
} sprite_t;

const sprite_t firemen = 
{
    64, 32, 3,                      // Los bomberos de tamaño 64x32 se pintan en 3 posiciones distintas
    {
        {  32, 176, FIREMEN },
        { 128, 176, FIREMEN },
        { 224, 176, FIREMEN }
    }
};

const sprite_t dummy = 
{
    32, 32, 19,                    // Los dummies de tamaño 32x32 se pintan en 19 posiciones distintas con 4 formas diferentes que se alternan
    {
        {   0,  64, DUMMY_0   },
        {  16,  96, DUMMY_90  },
        {  32, 128, DUMMY_180 },
        {  48, 160, DUMMY_270 },
        {  64, 128, DUMMY_0   },
        {  80,  96, DUMMY_90  },
        {  96,  64, DUMMY_180 },
        { 112,  96, DUMMY_270 },
        { 128, 128, DUMMY_0   },
        { 144, 160, DUMMY_90  },
        { 160, 128, DUMMY_180 },
        { 176,  96, DUMMY_270 },
        { 192,  64, DUMMY_0   },
        { 208,  96, DUMMY_90  },
        { 224, 128, DUMMY_180 },
        { 240, 160, DUMMY_270 },
        { 256, 128, DUMMY_0   },
        { 272, 96,  DUMMY_90  },
        { 288, 64,  DUMMY_180 }
    }
};

const sprite_t crash = 
{
    64, 32, 3,                     // Los dummies estrellados de tamaño 64x32 se pintan en 3 posiciones distintas
    {
        {   32, 208, CRASH },
        {  128, 208, CRASH },
        {  224, 208, CRASH }
    }
};

const sprite_t life =
{
    16, 16, 3,                    // Los corazones estrellados de tamaño 16x16 se pintan en 3 posiciones distintas
    {
        {   8, 8, LIFE },
        {  24, 8, LIFE },
        {  40, 8, LIFE }
    }
};

/* Declaración de fifo de punteros a funciones */

#define BUFFER_LEN   (512)

typedef void (*pf_t)(void);

typedef struct dummy {
	uint8 pos;
	boolean visible;
} dummy_t;

typedef struct fifo {
    uint16 head;
    uint16 tail;
    uint16 size;
    pf_t buffer[BUFFER_LEN];
} fifo_t;

void fifo_init( void );
void fifo_enqueue( pf_t pf );
pf_t fifo_dequeue( void );
boolean fifo_is_empty( void );
boolean fifo_is_full( void );

/* Declaración de recursos */

volatile fifo_t fifo;       // Cola de tareas
boolean gameOver;           // Flag de señalización del fin de la partida
boolean gameEnded;			// Flag de señalización del fin del juego
boolean flagCrash;			// Flag del choque de un dummy
boolean opt;

/* Declaración de variables */

/* GAME A: incrementa contador tras salvar un dummy.
 * GAME B: incrementa contador tras el rebote de un dummy.
 */
static enum { game_a, game_b, none } game_mode;
static enum { wait_keydown, scan, wait_keyup } keypad_state;
dummy_t dummies[2];     // Array de dummies
uint8 dummiesCount = 2;
uint16 score;       // Número de dummies salvados o de rebotes conseguidos
uint8 firemanPos;   // Posición del fireman
uint8 lifes;		// Número de vidas
uint8 delay;
uint16 contDelay;

/* Declaración de funciones */

void dummy_init( void );                                    // Inicializa la posición del dummy y lo dibuja
void score_init( void );                                    // Inicializa el contador de dummies salvados y lo dibuja
void lifes_init( void );									// Inicializa el contador de vidas y lo dibuja
void fireman_init( void );									// Inicializa la posición del fireman y lo dibuja
void sprite_plot( sprite_t const *sprite, uint16 pos );     // Dibuja el gráfico en la posición indicada
void sprite_clear( sprite_t const *sprite, uint16 pos );    // Borra el gráfico pintado en la posición indicada

/* Declaración de tareas */

void dummy_move( void );    		// Mueve el dummy
void next_dummy( void );
void scan_keypad ( void );			// Escanea el keypad para mover el fireman
void fireman_move_left ( void );	// Mueve el fireman a la derecha
void fireman_move_right ( void );	// Mueve el fireman a la derecha
void score_inc( void );     		// Incrementa el contador de dummies salvados
void lifes_dec( void );				// Decrementa el contador de vidas
void check_dummy_crash ( void );	// Comprueba si un dummy choca contra el suelo
void dummy_crash ( uint8 pos, uint8 dummy );

/* Declaración de RTI */

void isr_tick( void ) __attribute__ ((interrupt ("IRQ")));

/*******************************************************************/

void main( void )
{
    uint8 i;
    pf_t pf;
    uint8 scancode;
    
    // Inicializamos
    sys_init();
    timers_init();
    lcd_init();
    pbs_init();
    keypad_init();
    uda1341ts_init();
    
    iis_init( IIS_POLLING );

	game_mode = none;
	keypad_state = wait_keydown;

	lcd_on();
	lcd_clear();
	lcd_putWallpaper( LOGO );
	sw_delay_ms( 3000 );

	gameEnded = FALSE;
	gameOver = FALSE;

	// Empieza el juego
    while ( !gameEnded )
    {
    	lcd_clear();
    	lcd_putWallpaper( INTRO );

    	keypad_state = wait_keydown;

    	// Elegimos entre GAME A y GAME B
		while (game_mode == none)
		{
			switch( keypad_state )
			{
			case wait_keydown:
				if( keypad_pressed() )
					keypad_state = scan;
				break;
			case scan:
				scancode = keypad_scan();
				if ( scancode != KEYPAD_FAILURE ) // Comprobar modo juego
				{
					if (scancode == KEYPAD_KEY0)
						game_mode = game_a;
					else if (scancode == KEYPAD_KEY1)
						game_mode = game_b;
				}
				keypad_state = wait_keyup;
				break;
			case wait_keyup:
				if ( !keypad_pressed() )
					keypad_state = wait_keydown;
				break;
			}
		}
    	keypad_state = wait_keydown;

		lcd_clear();
		lcd_putWallpaper( LANDSCAPE );             		 // Dibuja el fondo de la pantalla

		// Inicializa las tareas
		dummy_init();
		score_init();
		lifes_init();
		fireman_init();

		flagCrash = FALSE; 								// Flag de choque a FALSE
		delay = 1;
		contDelay = 1680;								// Delay inicial del segundo dummy

		fifo_init();                                 	 // Inicializa cola de funciones
		timer0_open_tick( isr_tick, TICKS_PER_SEC ); 	 // Instala isr_tick como RTI del timer0

        while( !gameOver )
        {
            while( !fifo_is_empty() )
            {
                pf = fifo_dequeue();
                (*pf)();                    // Las tareas encoladas se ejecutan en esta hebra (background) en orden de encolado
            }
        }

        // Fin del juego
        lcd_putWallpaper( GAME_OVER );
        lcd_puts( 125, 136, 255, "Score: ");			// Muestra la puntuación
        lcd_putint( 180, 136, 255, score);
		iis_play( GAMEOVER, GAMEOVER_SIZE, FALSE );		// Reproduce sonido GAME_OVER
        opt = FALSE;

        // Elegimos entre seguir jugando o salir
        while ( !opt )
        {
        	switch( keypad_state )
			{
			case wait_keydown:
				if( keypad_pressed() )
					keypad_state = scan;
				break;
			case scan:
				scancode = keypad_scan();
				if ( scancode != KEYPAD_FAILURE )
				{
					if (scancode == KEYPAD_KEY0)
					{
						gameOver = FALSE;
						opt = TRUE;
					}
					else if (scancode == KEYPAD_KEY1)
					{
						gameEnded = TRUE;
						opt = TRUE;
						lcd_clear();
					}
				}
				keypad_state = wait_keyup;
				break;
			case wait_keyup:
				if ( !keypad_pressed() )
					keypad_state = wait_keydown;
				break;
			}
        }
    }

    
    timer0_close();
    while(1);
}

/*******************************************************************/

void dummy_init( void )
{
	dummies[0].pos = 0;                           // Inicializa las posiciones de los dummies y su visibilidad...
	dummies[0].visible = TRUE;
	dummies[1].pos = 0;
	dummies[1].visible = FALSE;
    sprite_plot( &dummy, 0 );              		 // ... y dibuja el primero.
}

void dummy_move( void )
{
	uint8 i;
	for(i=0; i<dummiesCount; i++)
	{
		// Si el dummy es visible, lo mueve
		if (dummies[i].visible)
		{
			sprite_clear( &dummy, dummies[i].pos );       // Borra el dummy de su posición actual

			if( dummies[i].pos == dummy.num_plots-1 )     // Si el dummy ha alcanzado la última posición...
			{
				dummies[i].pos = 0;                       // ... lo coloca en la posición de salida
				if (i > 0)								  // Si no es el primer dummy, añadimos delay de salida.
				{
					dummies[i].visible = FALSE;
					fifo_enqueue( next_dummy );
				}
				if (game_mode == game_a)
					fifo_enqueue( score_inc );          // ... incremeta el contador de dummies rescatados
			} else
			{
				if (i == 0 || (dummies[i].pos != 0) || dummies[i-1].pos != 0) // Si ambos coinciden en la salida, el segundo dummy no avanza
					dummies[i].pos++;
			}
			if (dummies[i].visible) 					// Si el dummy es visible, lo dibuja en la nueva posición
				sprite_plot( &dummy, dummies[i].pos );
		}
	}
}

void check_dummy_crash ( void )
{
	uint8 i;

	for(i=0; i<dummiesCount; i++)
	{
		if (dummies[i].visible)						// Si el dummy es visible, comprobamos si hay un choque
		{
			if (dummies[i].pos == 3)
				{
					if (firemanPos != 0)
					{
						sprite_clear ( &dummy, dummies[i].pos );
						dummy_crash(0, i);
					}
					else							// Si no hay choque, comprobamos si incrementa puntuación y reproducimos el rebote.
					{
						if (game_mode == game_b)
							 fifo_enqueue (score_inc );
						iis_play( REBOTE, REBOTE_SIZE, FALSE );
					}

				}
				else if (dummies[i].pos == 9)
				{
					if (firemanPos != 1)
					{
						sprite_clear ( &dummy, dummies[i].pos );
						dummy_crash(1, i);
					}
					else
					{
						if (game_mode == game_b)
							 fifo_enqueue (score_inc );
						iis_play( REBOTE, REBOTE_SIZE, FALSE );
					}
				}
				else if (dummies[i].pos == 15)
				{
					if (firemanPos != 2)
					{
						sprite_clear ( &dummy, dummies[i].pos );
						dummy_crash(2, i);
					}
					else
					{
						if (game_mode == game_b)
							 fifo_enqueue (score_inc );
						iis_play( REBOTE, REBOTE_SIZE, FALSE );
					}
				}
		}
	}

}

void dummy_crash ( uint8 pos, uint8 dummy )
{
	uint8 i;

	flagCrash = TRUE;							// Flag de choque
	sprite_plot (&crash, pos);
	iis_play( CHOQUE, CHOQUE_SIZE, FALSE );
	lifes_dec ();
	sprite_clear (&crash, pos);
	lcd_putWallpaper( LANDSCAPE );				// Redibujamos el LANDSCAPE que se altera tras el choque y por ende el resto de elementos de la imagen.
	lcd_putint_x2( 287, 0, BLACK, score );
	for( i=0; i<lifes; i++ )
		        sprite_plot( &life, i );
	sprite_plot( &firemen, firemanPos );

	flagCrash = FALSE;
	dummies[dummy].pos = 0;						// Ponemos el dummy en la posición inicial

	if (dummy == 1)								// Si no es el primer dummy, le añadimos un delay de salida.
	{
		dummies[dummy].visible = FALSE;
		fifo_enqueue( next_dummy );
	}
}

/*******************************************************************/

void fireman_init( void )
{
    firemanPos = 0;                           // Inicializa la posición del fireman...
    sprite_plot( &firemen, 0 );               // ... y lo dibuja
}

void scan_keypad( void )
{
	uint8 scancode;

	switch( keypad_state )
	{
	case wait_keydown:
		if ( keypad_pressed() )
			keypad_state = scan;
		break;
	case scan:
		scancode = keypad_scan();
		if( scancode != KEYPAD_FAILURE )
		{
			if (scancode == KEYPAD_KEY0)
				fifo_enqueue (fireman_move_left);
			else if (scancode == KEYPAD_KEY1)
				fifo_enqueue (fireman_move_right);
		}
		keypad_state = wait_keyup;
		break;
	case wait_keyup:
		if ( !keypad_pressed() )
			keypad_state = wait_keydown;
		break;
	}
}

void fireman_move_left ( void )
{
	sprite_clear( &firemen, firemanPos );       // Borra el fireman de su posición actual
	if (firemanPos != 0)						// Si no está en la primera posición, lo movemos a la izquierda
		firemanPos--;
	sprite_plot( &firemen, firemanPos );        // Dibuja el fireman en la nueva posición
	iis_play( MOVIMIENTO, MOVIMIENTO_SIZE, FALSE );
}

void fireman_move_right ( void )
{
	sprite_clear( &firemen, firemanPos );       // Borra el fireman de su posición actual
	if (firemanPos != firemen.num_plots-1)		// Si no está en la última posición, lo movemos a la derecha
		firemanPos++;
	sprite_plot( &firemen, firemanPos );        // Dibuja el fireman en la nueva posición
	iis_play( MOVIMIENTO, MOVIMIENTO_SIZE, FALSE );
}

/*******************************************************************/

void score_init( void )
{
    score = 0;                              // Inicializa la puntuación
    lcd_putint_x2( 287, 0, BLACK, score );  // ... y la dibuja
}

void score_inc( void )
{
    score++;                                // Incrementa la puntuación
    lcd_putint_x2( 287, 0, BLACK, score );
}

void lifes_init( void )
{
	uint8 i;

	lifes = life.num_plots;					// Inicializa el contador de vidas
	for( i=0; i<life.num_plots; i++ )       // ... y dibuja los corazones en todas sus posiciones iniciales
	        sprite_plot( &life, i );
}

void lifes_dec( void )
{
	sprite_clear( &life, lifes-1);
	lifes--;
	if ( lifes == 0)						// Si llegamos a cero vidas, acaba la partida.
	{
		game_mode = none;
		gameOver = TRUE;

		while (!fifo_is_empty())
			fifo_dequeue();
	}
}

/*
 * Delay para el segundo (o posteriores) dummies que hubiera.
 * Como hay 19 posiciones posibles, cogemos un número aleatorio entre 1 y 18
 *  y lo multiplicamos por los ticks por movimiento.
 */
void next_dummy( void )
{
	uint8 lower = 1, upper = 18;

	delay = rand() % (upper - lower + 1) + lower;
	contDelay = delay*70;
}

/*******************************************************************/

void isr_tick( void )
{   
    static uint16 cont70ticks = 70;
    static uint16 cont2ticks = 2;

    if( !(--cont2ticks))				// Escanea el keypad para el movimiento del fireman cada 2 ticks.
    {
    	cont2ticks = 2;
    	fifo_enqueue ( scan_keypad );
    }

    if( !(--cont70ticks) )				// Mueve los dummies cada 70 ticks
    {
        cont70ticks = 70;

        if (!flagCrash)
        {
    		fifo_enqueue ( check_dummy_crash );			// Comprobamos si hay un choque.
    		if (!flagCrash)								// Movemos si no hay choque.
    			fifo_enqueue( dummy_move );
        }
    }

    if( !(--contDelay))									// Delay que activa la salida del segundo dummy.
    {
    	dummies[1].pos = 0;
    	dummies[1].visible = TRUE;
    }

    I_ISPC = BIT_TIMER0;
};

/*******************************************************************/

extern uint8 lcd_buffer[];

void lcd_putBmp( uint8 *bmp, uint16 x, uint16 y, uint16 xsize, uint16 ysize );
void lcd_clearWindow( uint16 x, uint16 y, uint16 xsize, uint16 ysize );

void sprite_plot( sprite_t const *sprite, uint16 num )
{
    lcd_putBmp( sprite->plots[num].plot, sprite->plots[num].x, sprite->plots[num].y, sprite->width, sprite->height );
}

void sprite_clear( sprite_t const *sprite, uint16 num )
{
    lcd_clearWindow( sprite->plots[num].x, sprite->plots[num].y, sprite->width, sprite->height );
}

/*
** Muestra un BMP de tamaño (xsize, ysize) píxeles en la posición (x,y)
** Esta función es una generalización de lcd_putWallpaper
*/
void lcd_putBmp( uint8 *bmp, uint16 x, uint16 y, uint16 xsize, uint16 ysize )
{
	uint32 headerSize;

	uint16 xSrc, ySrc, yDst;
	uint16 offsetSrc, offsetDst;

	headerSize = bmp[10] + (bmp[11] << 8) + (bmp[12] << 16) + (bmp[13] << 24);

	bmp = bmp + headerSize; 

	for( ySrc=0, yDst=ysize-1; ySrc<ysize; ySrc++, yDst-- )
	{
		offsetDst = (yDst+y)*LCD_WIDTH/2+x/2;
		offsetSrc = ySrc*xsize/2;
		for( xSrc=0; xSrc<xsize/2; xSrc++ )
			lcd_buffer[offsetDst+xSrc] = ~bmp[offsetSrc+xSrc];
	}
}

/*
** Borra una porción de la pantalla de tamaño (xsize, ysize) píxeles desde la posición (x,y)
** Esta función es una generalización de lcd_clear
*/
void lcd_clearWindow( uint16 x, uint16 y, uint16 xsize, uint16 ysize )
{
	uint16 xi, yi;

	for( yi=y; yi<y+ysize; yi++ )
		for( xi=x; xi<x+xsize; xi++ )
			lcd_putpixel( xi, yi, WHITE );
}

/*******************************************************************/

void fifo_init( void )
{
    fifo.head = 0;
    fifo.tail = 0;
    fifo.size = 0;
}

void fifo_enqueue( pf_t pf )
{
    fifo.buffer[fifo.tail++] = pf;
    if( fifo.tail == BUFFER_LEN )
        fifo.tail = 0;
    INT_DISABLE;
    fifo.size++;
    INT_ENABLE;
}

pf_t fifo_dequeue( void )
{
    pf_t pf;
    
    pf = fifo.buffer[fifo.head++];
    if( fifo.head == BUFFER_LEN )
        fifo.head = 0;
    INT_DISABLE;
    fifo.size--;
    INT_ENABLE;
    return pf;
}

boolean fifo_is_empty( void )
{
    return (fifo.size == 0);
}

boolean fifo_is_full( void )
{
    return (fifo.size == BUFFER_LEN-1);
}

/*******************************************************************/
