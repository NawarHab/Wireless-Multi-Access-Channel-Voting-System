/**
 * Arduino voting system client
 *
 * Pins:
 * Hardware SPI:
 * MISO -> 12
 * MOSI -> 11
 * SCK -> 13
 *
 * Configurable:
 * CE -> 8
 * CSN -> 7
 *
 * Note: To see best case latency comment out all Serial.println
 */

#include <SPI.h>
#include <Mirf.h>
#include <nRF24L01.h>
#include <MirfHardwareSpiDriver.h>

#define NO_BUTTON 0
#define SUCCESS 0
#define TIMEOUT 1

struct Message {
  byte destination;
  byte source;
  byte data;
  byte checksum;
};

Message tx_message;
Message rx_message;
/* Initialy our recieving status is 0 and finished is false */
uint8_t rx_stat = 0;
volatile bool rx_finished = false;

void setup() {
  Serial.begin( 9600 );
  /* Get the instance of the Mirf object */
  Mirf.spi = &MirfHardwareSpi;
  
  /* Setup the SPI pins CE and CSN */
  Mirf.cePin = 8;
  Mirf.csnPin = 7;
  
  /* Set the payload length, 1 data packet size (5 bytes)
   * IMPORTANT!: payload on client and server must be the same
   */
  Mirf.payload = sizeof( byte ) * 5;
  
  /* Set transmission channel
   * IMPORTANT!: make sure channel is legal in your area
   */
  Mirf.channel = 1;
  
  /* Initialize SPI communication to tranceiver */
  Mirf.init();
  /* Set our device address */
  Mirf.setRADDR( (byte *) "client" );
  /* Set server address */
  Mirf.setTADDR( (byte *) "server" );
  /* Configure and power up tranceiver */
  Mirf.config();
  
  /* Initialize button (set as input) */
  int i;
  for ( i = 2; i <= 6; i++ )
    pinMode( i, INPUT );
  Serial.println( "Beginning ... " );
}

/*
 * function to check which button was pressed
 * return - 0 when button was successfuly read
 */
uint8_t readButtons( uint8_t* button ) {
  *button = NO_BUTTON;
  int i;
  /* go trough all the button and check which one was pressed */
  for ( i = 2; i <= 6; i++ )
    if ( digitalRead( i ) ) *button = i; else continue;
  Serial.print( "button was pressed, nr " );
  Serial.println( *button );
  return SUCCESS;
}

/*
 * function to receive datapacket, must be called several times
 * to receive all the bytes
 * TIPS: use timeoutReceive(), if unsure
 * returns - 0 when successfuly received
 *           1 when data is not ready to be read
 *           2 when receiving status was illegaly changed
 */
uint8_t receivePacket() {
  /* when nothing was received */
  if ( !Mirf.dataReady() ) return 1;
  byte b;
  Mirf.getData( &b );
  /* according to our status we save the received byte and go to next state */
  switch ( rx_stat ) {
    case 0: rx_message.destination = b; rx_stat = 1; break;
    case 1: rx_message.source = b; rx_stat = 2; break;
    case 2: rx_message.data = b; rx_stat = 3; break;
    /* TODO: implement checksum verification */
    case 3: rx_message.checksum = b; rx_finished = true; rx_stat = 0; break;
    default: Serial.println( "Status illegaly changed!" ); rx_stat = 0; return 2;
  }
  return SUCCESS;
}

/* 
 * function to transmits the datapacket to the server
 * return - 0 when transmission successfuly finished
 */
uint8_t transmitPacket() {
  tx_message.destination = 0xA1;
  tx_message.source = 0xA2;
  tx_message.data = 0x01;
  /* calculate the checksum, use XOR */
  tx_message.checksum = 0x00;
  tx_message.checksum ^= tx_message.destination;
  tx_message.checksum ^= tx_message.source;
  tx_message.checksum ^= tx_message.data;
  /* Send the data packet */
  Mirf.send( &tx_message.destination );
  Mirf.send( &tx_message.source );
  Mirf.send( &tx_message.data );
  Mirf.send( &tx_message.checksum );
  
  /* Wait for data to be transmitted */
  while ( Mirf.isSending() );
  Serial.println( "Finished sending" );
  delay( 10 );
  
  return SUCCESS;
}

/* 
 * function to receive the datapacket with timeout delay
 * returns - 0 when successful, 1 when timeout
 */
uint8_t timeoutReceive() {
  /* timeout for receiving */
  long time = millis();
  while ( !rx_finished ) {
    /* if we received byte successfuly, refresh timeout */
    if( receivePacket() == SUCCESS ) time = millis();
    
    /* Timeout when data packet was not received within 1 second */
    if ( ( millis() - time ) > 1000 ) {
      Serial.println( "Timeout on response from server!" );
      return TIMEOUT;
    }
  }
  /* reset receiving status */
  rx_finished = false;
  /* When transmission was successful */
  return SUCCESS;
}

void loop() {
  uint8_t button;
  readButtons( &button );
  /* Wait for button to be pressed, TODO: implement interrupt */
  if ( button > NO_BUTTON ) {
    digitalWrite( 9, HIGH );
    Serial.println( "Finished sending!" );
  }
  
  /* transmit and receive acknowledgement */
  //transmitPacket();
  //timeoutReceive();
  
  /* don't freeze the MCU */
  delay( 100 );
}

