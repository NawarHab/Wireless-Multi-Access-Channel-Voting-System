/**
 * Arduino voting system server
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

#define SUCCESS 0

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
  Mirf.setRADDR( (byte *) "server" );
  /* Set client address */
  Mirf.setTADDR( (byte *) "client" );
  /* Configure and power up tranceiver */
  Mirf.config();
  
  Serial.println( "Listening ... " );
}

/*
 * function to receive datapacket, must be called several times
 * to receive all the bytes
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
  tx_message.destination = 0xA2;
  tx_message.source = 0xA1;
  tx_message.data = 0x1F;
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
  delay( 10 );
  
  return SUCCESS;
}

void loop() {
  /* A buffer to store the data packet */
  byte data[Mirf.payload];
  
  /* while we are receiving packet */
  while ( !rx_finished ) {
    if ( receivePacket() == SUCCESS );
    /* don't freeze the MCU */
    delay( 10 );
  }
  Serial.println( "Got packet" );
  /* reset receiving status */
  rx_finished = false;
  
  /* send verification */
  if ( transmitPacket() == SUCCESS )
    Serial.println( "Reply sent" );
}

