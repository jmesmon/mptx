#include "serial.hh"
#include <stdint.h>
#include <avr/interrupt.h>

#ifdef BASH_SERIAL
// For bit-bashed serial output
static volatile struct t_serial_bash
{
    uint8_t head ;
    uint8_t tail ;
    uint8_t data[64] ;
    uint8_t busy ;
    uint8_t speed ;
} SerialControl ;
#endif

#ifndef BASH_SERIAL
# define TXBUFFER_SIZE 32
static volatile uint8_t tx_buff[TXBUFFER_SIZE];
static volatile uint8_t tx_head=0;
static volatile uint8_t tx_tail=0;
#endif // BASH_SERIAL

/**************************/
/**************************/
/**  Serial TX routines  **/
/**************************/
/**************************/
void serial_reset_buf(void)
{
#ifndef BASH_SERIAL
    tx_tail=0;
    tx_head=0;
#endif
}


bool tx_buff_has_space(void)
{
#ifdef BASH_SERIAL
	uint8_t h ;
	uint8_t t ;
	h = SerialControl.head ;
	t = SerialControl.tail ;
	if ( h >= t )
	{
		t += 64 - h ;
	}
	else
	{
		t -= h ;
	}
	if ( t < 32 )
	{
		return false;
	}

#else
	uint8_t h ;
	uint8_t t ;
	h = tx_head ;
	t = tx_tail ;
	if ( h >= t )
	{
		t += TXBUFFER_SIZE - h ;
	}
	else
	{
		t -= h ;
	}
	if ( t < 16 )
	{
		return false;
	}
#endif

	return true;
}

#ifndef BASH_SERIAL
// Routines for normal serial output
void Serial_write(uint8_t data)
{
    uint8_t nextHead ;
    nextHead = tx_head + 1 ;
    if ( nextHead >= TXBUFFER_SIZE )
        nextHead = 0 ;
    tx_buff[nextHead]=data;
    tx_head = nextHead ;
    tx_resume();
}

void initTXSerial( uint8_t speed)
{
    #ifdef ENABLE_PPM
        if(speed==SPEED_9600)
        { // 9600
            #ifdef ORANGE_TX
                USARTC0.BAUDCTRLA = 207 ;
                USARTC0.BAUDCTRLB = 0 ;
                USARTC0.CTRLB = 0x18 ;
                USARTC0.CTRLA = (USARTC0.CTRLA & 0xCF) | 0x10 ;
                USARTC0.CTRLC = 0x03 ;
            #else
                #ifdef STM32_BOARD
                    usart3_begin(9600,SERIAL_8N1);		//USART3 
                    USART3_BASE->CR1 &= ~ USART_CR1_RE;	//disable RX leave TX enabled
                #else
                    UBRR0H = 0x00;
                    UBRR0L = 0x67;
                    UCSR0A = 0 ;						// Clear X2 bit
                    //Set frame format to 8 data bits, none, 1 stop bit
                    UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);
                #endif
            #endif
        }
        else if(speed==SPEED_57600)
        { // 57600
            #ifdef ORANGE_TX
                /*USARTC0.BAUDCTRLA = 207 ;
                USARTC0.BAUDCTRLB = 0 ;
                USARTC0.CTRLB = 0x18 ;
                USARTC0.CTRLA = (USARTC0.CTRLA & 0xCF) | 0x10 ;
                USARTC0.CTRLC = 0x03 ;*/
            #else
                #ifdef STM32_BOARD
                    usart3_begin(57600,SERIAL_8N1);		//USART3 
                    USART3_BASE->CR1 &= ~ USART_CR1_RE;	//disable RX leave TX enabled
                #else
                    UBRR0H = 0x00;
                    UBRR0L = 0x22;
                    UCSR0A = 0x02 ;	// Set X2 bit
                    //Set frame format to 8 data bits, none, 1 stop bit
                    UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);
                #endif
            #endif
        }
        else if(speed==SPEED_125K)
        { // 125000
            #ifdef ORANGE_TX
                /*USARTC0.BAUDCTRLA = 207 ;
                USARTC0.BAUDCTRLB = 0 ;
                USARTC0.CTRLB = 0x18 ;
                USARTC0.CTRLA = (USARTC0.CTRLA & 0xCF) | 0x10 ;
                USARTC0.CTRLC = 0x03 ;*/
            #else
                #ifdef STM32_BOARD
                    usart3_begin(125000,SERIAL_8N1);	//USART3 
                    USART3_BASE->CR1 &= ~ USART_CR1_RE;	//disable RX leave TX enabled
                #else
                    UBRR0H = 0x00;
                    UBRR0L = 0x07;
                    UCSR0A = 0x00 ;	// Clear X2 bit
                    //Set frame format to 8 data bits, none, 1 stop bit
                    UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);
                #endif
            #endif
        }
    #endif
    #ifndef ORANGE_TX
        #ifndef STM32_BOARD
            UCSR0B |= (1<<TXEN0);//tx enable
        #endif
    #endif
}

//Serial TX
#ifdef ORANGE_TX
    ISR(USARTC0_DRE_vect)
#else
    #ifdef STM32_BOARD
        void __irq_usart3()			
    #else
        ISR(USART_UDRE_vect)
    #endif
#endif
{	// Transmit interrupt
    #ifdef STM32_BOARD
        if(USART3_BASE->SR & USART_SR_TXE)
        {
    #endif
    if(tx_head!=tx_tail)
    {
        if(++tx_tail>=TXBUFFER_SIZE)//head 
            tx_tail=0;
        #ifdef STM32_BOARD	
            USART3_BASE->DR=tx_buff[tx_tail];//clears TXE bit				
        #else
            UDR0=tx_buff[tx_tail];
        #endif
    }
    if (tx_tail == tx_head)
        tx_pause(); // Check if all data is transmitted . if yes disable transmitter UDRE interrupt
    #ifdef STM32_BOARD	
        }
    #endif		
}
#ifdef STM32_BOARD
    void usart2_begin(uint32_t baud,uint32_t config )
    {
        usart_init(USART2); 
        usart_config_gpios_async(USART2,GPIOA,PIN_MAP[PA3].gpio_bit,GPIOA,PIN_MAP[PA2].gpio_bit,config);
        usart_set_baud_rate(USART2, STM32_PCLK1, baud);//
        usart_enable(USART2);
    }
    void usart3_begin(uint32_t baud,uint32_t config )
    {
        usart_init(USART3);
        usart_config_gpios_async(USART3,GPIOB,PIN_MAP[PB11].gpio_bit,GPIOB,PIN_MAP[PB10].gpio_bit,config);
        usart_set_baud_rate(USART3, STM32_PCLK1, baud);
        usart_enable(USART3);
    }
#endif
#else	//BASH_SERIAL
// Routines for bit-bashed serial output

// Speed is 0 for 100K and 1 for 9600
void initTXSerial( uint8_t speed)
{
	TIMSK0 = 0 ;	// Stop all timer 0 interrupts
	#ifdef INVERT_SERIAL
		SERIAL_TX_off;
	#else
		SERIAL_TX_on;
	#endif
	UCSR0B &= ~(1<<TXEN0) ;

	SerialControl.speed = speed ;
	if ( speed == SPEED_9600 )
	{
		OCR0A = 207 ;	// 104uS period
		TCCR0A = 3 ;
		TCCR0B = 0x0A ; // Fast PMM, 2MHz
	}
	else	// 100K
	{
		TCCR0A = 0 ;
		TCCR0B = 2 ;	// Clock/8 (0.5uS)
	}
}

void Serial_write( uint8_t byte )
{
	uint8_t temp ;
	uint8_t temp1 ;
	uint8_t byteLo ;

	#ifdef INVERT_SERIAL
		byte = ~byte ;
	#endif

	byteLo = byte ;
	byteLo >>= 7 ;		// Top bit
	if ( SerialControl.speed == SPEED_100K )
	{
		#ifdef INVERT_SERIAL
				byteLo |= 0x02 ;	// Parity bit
		#else
				byteLo |= 0xFC ;	// Stop bits
		#endif
		// calc parity
		temp = byte ;
		temp >>= 4 ;
		temp = byte ^ temp ;
		temp1 = temp ;
		temp1 >>= 2 ;
		temp = temp ^ temp1 ;
		temp1 = temp ;
		temp1 <<= 1 ;
		temp ^= temp1 ;
		temp &= 0x02 ;
		#ifdef INVERT_SERIAL
				byteLo ^= temp ;
		#else	
				byteLo |= temp ;
		#endif
	}
	else
	{
		byteLo |= 0xFE ;	// Stop bit
	}
	byte <<= 1 ;
	#ifdef INVERT_SERIAL
		byte |= 1 ;		// Start bit
	#endif
	uint8_t next = (SerialControl.head + 2) & 0x3f ;
	if ( next != SerialControl.tail )
	{
		SerialControl.data[SerialControl.head] = byte ;
		SerialControl.data[SerialControl.head+1] = byteLo ;
		SerialControl.head = next ;
	}
	if(!IS_TX_PAUSE_on)
		tx_resume();
}

void resumeBashSerial()
{
	cli() ;
	if ( SerialControl.busy == 0 )
	{
		sei() ;
		// Start the transmission here
		#ifdef INVERT_SERIAL
			GPIOR2 = 0 ;
		#else
			GPIOR2 = 0x01 ;
		#endif
		if ( SerialControl.speed == SPEED_100K )
		{
			GPIOR1 = 1 ;
			OCR0B = TCNT0 + 40 ;
			OCR0A = OCR0B + 210 ;
			TIFR0 = (1<<OCF0A) | (1<<OCF0B) ;
			TIMSK0 |= (1<<OCIE0B) ;
			SerialControl.busy = 1 ;
		}
		else
		{
			GPIOR1 = 1 ;
			TIFR0 = (1<<TOV0) ;
			TIMSK0 |= (1<<TOIE0) ;
			SerialControl.busy = 1 ;
		}
	}
	else
	{
		sei() ;
	}
}

// Assume timer0 at 0.5uS clock

ISR(TIMER0_COMPA_vect)
{
	uint8_t byte ;
	byte = GPIOR0 ;
	if ( byte & 0x01 )
		SERIAL_TX_on;
	else
		SERIAL_TX_off;
	byte /= 2 ;		// Generates shorter code than byte >>= 1
	GPIOR0 = byte ;
	if ( --GPIOR1 == 0 )
	{
		TIMSK0 &= ~(1<<OCIE0A) ;
		GPIOR1 = 3 ;
	}
	else
	{
		OCR0A += 20 ;
	}
}

ISR(TIMER0_COMPB_vect)
{
	uint8_t byte ;
	byte = GPIOR2 ;
	if ( byte & 0x01 )
		SERIAL_TX_on;
	else
		SERIAL_TX_off;
	byte /= 2 ;		// Generates shorter code than byte >>= 1
	GPIOR2 = byte ;
	if ( --GPIOR1 == 0 )
	{
		if ( IS_TX_PAUSE_on )
		{
			SerialControl.busy = 0 ;
			TIMSK0 &= ~(1<<OCIE0B) ;
		}
		else
		{
			// prepare next byte and allow for 2 stop bits
			volatile struct t_serial_bash *ptr = &SerialControl ;
			if ( ptr->head != ptr->tail )
			{
				GPIOR0 = ptr->data[ptr->tail] ;
				GPIOR2 = ptr->data[ptr->tail+1] ;
				ptr->tail = ( ptr->tail + 2 ) & 0x3F ;
				GPIOR1 = 8 ;
				OCR0A = OCR0B + 40 ;
				OCR0B = OCR0A + 8 * 20 ;
				TIMSK0 |= (1<<OCIE0A) ;
			}
			else
			{
				SerialControl.busy = 0 ;
				TIMSK0 &= ~(1<<OCIE0B) ;
			}
		}
	}
	else
	{
		OCR0B += 20 ;
	}
}

ISR(TIMER0_OVF_vect)
{
	uint8_t byte ;
	if ( GPIOR1 > 2 )
	{
		byte = GPIOR0 ;
	}
	else
	{
		byte = GPIOR2 ;
	}
	if ( byte & 0x01 )
		SERIAL_TX_on;
	else
		SERIAL_TX_off;
	byte /= 2 ;		// Generates shorter code than byte >>= 1
	if ( GPIOR1 > 2 )
	{
		GPIOR0 = byte ;
	}
	else
	{
		GPIOR2 = byte ;
	}
	if ( --GPIOR1 == 0 )
	{
		// prepare next byte
		volatile struct t_serial_bash *ptr = &SerialControl ;
		if ( ptr->head != ptr->tail )
		{
			GPIOR0 = ptr->data[ptr->tail] ;
			GPIOR2 = ptr->data[ptr->tail+1] ;
			ptr->tail = ( ptr->tail + 2 ) & 0x3F ;
			GPIOR1 = 10 ;
		}
		else
		{
			SerialControl.busy = 0 ;
			TIMSK0 &= ~(1<<TOIE0) ;
		}
	}
}


#endif // BASH_SERIAL
