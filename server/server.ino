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

struct Message {
  uint8_t id;
  //uint8_t destination;
  //uint8_t source;
  uint8_t command;
  uint8_t checksum[2];
}

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
  Mirf.setRADDR( (byte *) "server" );
  /* Set client address */
  Mirf.setTADDR( (byte *) "client" );
  /* Configure and power up tranceiver */
  Mirf.config();
  
  Serial.println( "Listening ... " );
}

void loop() {
  /* A buffer to store the data packet */
  byte data[Mirf.payload];
  
  /* When sending is complete and we received something */
  if ( !Mirf.isSending() && Mirf.dataReady() ) {
    Serial.println( "Got packet" );
    
    /* Get the data packet from the tranceiver */
    Mirf.getData( data );
    
    /* TODO: implement address request packet and send address for clients */

    /* Send the response data packet to the client */
    Mirf.send( data );
    
    Serial.println( "Reply sent" );
  }
}

