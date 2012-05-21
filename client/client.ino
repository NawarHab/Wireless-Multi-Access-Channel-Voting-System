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

struct Message {
  byte id;
  //uint8_t destination;
  //uint8_t source;
  byte data;
  byte checksum[2];
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
  
  /* Initialize button */
  int i;
  for ( i = 2; i <= 6; i++ ) {
    pinMode( i, INPUT );
    digitalWrite( i, LOW );
  }
  Serial.println( "Beginning ... " );
}

uint8_t getButton() {
  int i;
  for ( i = 2; i <= 6; i++ ) {
    if ( digitalRead( i ) ) {
      Serial.print( "button " );
      Serial.print( i );
      Serial.println( " was pressed" );
      return i;
    }
  }
  return 0;
}

int receivePacket() {
  /* when nothing was received */
  if ( !Mirf.dataReady() ) return 1;
  byte b;
  Mirf.getData( (byte *) b );
  /* according to our status we save the received byte and go to next state */
  switch ( rx_stat ) {
    case 0: rx_message.id = b; rx_stat = 1; break;
    case 1: rx_message.data = b; rx_stat = 2; break;
    case 2: rx_message.checksum[0] = b; rx_stat = 3; break;
    case 3: rx_message.checksum[1] = b; rx_finished = true; rx_stat = 0; break;
    default: Serial.println( "Receiving status was changed illegaly!" ); break;
  }
  return 0;
}

int transmitPacket() {
  /* Send the data packet */
  Mirf.send( (byte *) tx_message.id );
  Mirf.send( (byte *) tx_message.data );
  Mirf.send( (byte *) tx_message.checksum[0] );
  Mirf.send( (byte *) tx_message.checksum[1] );
  
  /* Wait for data to be transmitted */
  while ( Mirf.isSending() );
  Serial.println( "Finished sending" );
  delay( 10 );
}

void g() {
  /* transmit the datapacket */
  transmitPacket();
  
  /* timeout for receiving */
  long time = millis();
  while ( !rx_finished ) {
    /* if we received successfuly, refresh timeout */
    if( receivePacket() == 0 ) time = millis();
    
    /* Timeout when data packet was not received within 1 second */
    if ( ( millis() - time ) > 1000 ) {
      Serial.println( "Timeout on response from server!" );
      //return 1;
    }
  }
  
  /* When transmission was successful */
}

void loop() {
  /* Wait for button to be pressed, TODO: implement interrupt */
  if ( getButton() > 0 ) {
    digitalWrite( 9, HIGH );
    Serial.println( "Finished sending!" );
  }
  
  
  /* don't freeze the MCU */
  delay( 100 );
}

