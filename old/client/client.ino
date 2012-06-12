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

/* Return codes */
#define NO_BUTTON 0
#define SUCCESS 0
#define TIMEOUT 1
#define NO_DATA 1

/* RX/TX packet structure */
#define CLIENT 0xC1
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
  /* Go trough all the button and check if one was pressed */
  for ( i = 2; i <= 6; i++ )
    if ( digitalRead( i ) ) *button = i; else continue;
  /* When any of the buttons was pressed */
  if ( *button != 0 ) {
    Serial.print( "button was pressed, nr " );
    Serial.println( *button );
  }
  
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
uint8_t transmitPacket( byte data ) {  
  tx_message[DESTINATION] = 0xB1;
  tx_message[SOURCE] = CLIENT;
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
  Serial.println( "Finished sending" );
  delay( 10 );
  
  return SUCCESS;
}

/* 
 * function to receive the datapacket with timeout delay
 * returns - 0 when successful, 1 when timeout
 */
uint8_t timeoutReceive() {
  /* Timeout for receiving */
  long time = millis();
  while ( /*!rx_finished*/ 1 ) {
    /* When we received byte successfuly, refresh timeout */
    if( receivePacket() == SUCCESS ) time = millis();
    
    /* Timeout when data packet was not received within 1 second */
    if ( ( millis() - time ) > 1000 ) {
      Serial.println( "Timeout on response from server!" );
      return TIMEOUT;
    }
  }
  /* When transmission was successful */
  return SUCCESS;
}

void loop() {
  //uint8_t button;
  //readButtons( &button );
  /* Wait for button to be pressed, TODO: implement interrupt */
  /*if ( button > NO_BUTTON ) {
    digitalWrite( 9, HIGH );
    delay(1000);
  }*/
  
  /* Transmit the button and receive acknowledgement */
  transmitPacket( (byte) 0x01 );
  while( receivePacket() == NO_DATA ) delay(10);
  
  /* When the destination is us and we received ACK */
  if ( rx_message[DESTINATION] == CLIENT && rx_message[DATA] == ACK )
    Serial.println( "Ack received" );
  
  /* Don't freeze the MCU */
  delay( 1000 );
}

