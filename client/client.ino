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
  uint8_t id;
  //uint8_t destination;
  //uint8_t source;
  uint8_t data;
  uint8_t checksum[2];
};

Message message;

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

int sendPacket() {
  /* Timeout for receiving */
  long time = millis();
  
  /* Send the data packet */
  Mirf.send( (byte *) message.id );
  Mirf.send( (byte *) message.data );
  Mirf.send( (byte *) message.checksum[0] );
  Mirf.send( (byte *) message.checksum[1] );
  
  /* While data is still transmitted */
  while ( Mirf.isSending() );
  Serial.println( "Finished sending" );
  delay( 10 );
  
  /* While we are receiving the data packet */
  while ( !Mirf.dataReady() );
  
  Serial.print( "received: " );
  /* Get the byte from the tranceiver */
  Mirf.getData( (byte *) message.data );
  /* TODO: do something useful */
  int i;
  for ( i = 0; i < Mirf.payload; ) {
    Serial.println( (char) message.data );
  }
  
  /* Timeout when data packet was not received within 1 second */
  if ( ( millis() - time ) > 1000 ) {
    Serial.println("Timeout on response from server!");
    return 1;
  }
  
  /* When transmission was successful */
  return 0;
}

void loop() {
  /* Wait for button to be pressed, TODO: implement interrupt */
  if ( getButton() > 0 ) {
    digitalWrite( 9, HIGH );
    Serial.println( "Finished sending!" );
  }
}

