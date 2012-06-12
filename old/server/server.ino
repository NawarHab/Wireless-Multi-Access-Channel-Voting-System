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
#define NO_DATA 1

/* RX/TX packet structure */
#define ACK 0xAC
#define DESTINATION 0
#define SOURCE 1
#define DATA 2
#define CHECKSUM 3
#define PACKET_LEN 4
byte tx_message[PACKET_LEN];
byte rx_message[PACKET_LEN];

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
  Mirf.payload = sizeof( byte ) * PACKET_LEN;
  
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
  /* When nothing was received */
  if ( !Mirf.dataReady() ) return NO_DATA;
  Mirf.getData( rx_message );
  Serial.println( "Packet received" );
  return SUCCESS;
}

/* 
 * function to transmits the datapacket to the server
 * return - 0 when transmission successfuly finished
 */
uint8_t transmitPacket( byte destination, byte data ) {
  tx_message[DESTINATION] = destination;
  tx_message[SOURCE] = 0xB1;
  tx_message[DATA] = data;
  /* Calculate the checksum, using XOR */
  tx_message[CHECKSUM] = 0x00;
  int i;
  /* Calculate the checksum for the data packet */
  for ( i = 0; i < PACKET_LEN - 1; i++ )
    tx_message[CHECKSUM] ^= tx_message[i];
  /* Send the data packet */
  Mirf.send( tx_message );
  
  /* Wait for data to be transmitted */
  while ( Mirf.isSending() );
  delay( 10 );
  
  return SUCCESS;
}

void loop() {
  /* A buffer to store the data packet */
  byte data[Mirf.payload];
  
  /* Wait for the packet */
  while ( receivePacket() == NO_DATA ) delay(10);
  /* Don't freeze the MCU */
  //delay( 100 );
  
  Serial.print( "Got packet: " );
  Serial.print( "client= " );
  Serial.print( rx_message[SOURCE], HEX );
  Serial.print( " data= " );
  Serial.println( rx_message[DATA], HEX );
  
  /* Send verification */
  if ( transmitPacket( rx_message[SOURCE], ACK ) == SUCCESS )
    Serial.println( "Ack sent" );
}

