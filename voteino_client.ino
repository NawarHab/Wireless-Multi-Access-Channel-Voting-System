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
  Mirf.setRADDR( (byte *) "voting_client" );
  /* Set server address */
  Mirf.setTADDR( (byte *) "voting_server" );
  /* Configure and power up tranceiver */
  Mirf.config();
  
  /* Initialize button */
  pinMode( 2, INPUT );
  digitalWrite( 2, HIGH );
  Serial.println( "Beginning ... " );
}

void loop() {
  /* Timeout for receiving */
  long time = millis();
  /* Transmission data packet */
  byte data[Mirf.payload] = ['s', 'i', 'l', 'b', 'o'];
  
  /* Wait for button to be pressed, TODO: implement interrupt */
  while ( digitalRead( 2 ) == HIGH );
  
  /* Send the data packet */
  Mirf.send(data);
  
  /* While data is still transmitted */
  while ( Mirf.isSending() );
  Serial.println( "Finished sending" );
  delay( 10 );
  
  /* While we are receiving the data packet */
  while ( !Mirf.dataReady() ) {
    /* Get the byte from the tranceiver */
    Mirf.getData( data );
    /* TODO: do something useful */
    Serial.println( (char) data );
    
    /* Timeout when data packet was not received within 1 second */
    if ( ( millis() - time ) > 1000 ) {
        Serial.println("Timeout on response from server!");
        return;
    }
  }
  
  /* When transmission was successful */
  delay(1000);
} 
  
  
  
