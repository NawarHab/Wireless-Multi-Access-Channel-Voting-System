/*
 Copyright (C) 2011 J. Coliz <maniacbug@ymail.com>
 Edit by (C) 2012 S. Kuusik <silver.kuusik@gmail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

/**
 * Voteino client and server.
 *
 * This is a multiuser voting system for nRF24L01+. It uses the RF24 library.
 */

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

/* MAY NEED TO BE CHANGED FOR DIFFERENT STATIONS */
/* Switch on = 1 or off = 0 for debug messages */
#define DEBUG 1
/* Voting buttons for clients */
#define BUTTONS_COUNT 4
uint8_t buttons[BUTTONS_COUNT] = { 3, 4, 5, 6 };
/* Role IDs */
uint8_t server_id = 0xB0;
uint8_t client_id = 0xC0;
/* The role of the current running sketch */
uint8_t role_id = client_id;

/* CONSTANTS */
#define SUCCESS 0
#define NO_BUTTON 0
#define TIMEOUT 1
#define WRONG_DESTINATION 2
/* RX/TX PACKET STRUCTURE */
#define DESTINATION 0
#define SOURCE 1
#define COMMAND 2
#define DATA 3
/* Each packet is 3 bytes */
#define PAYLOAD 4
byte tx_message[PAYLOAD];
byte rx_message[PAYLOAD];
/* Command codes */
#define VOTE_ACK 0xAC
#define VOTE_ID 0xAB
#define VOTE_STATUS 0xEA
#define REQUEST_ACK 0xEC
#define REQUEST_ID 0x1D
/* For debugging */
#define debug_printf( args ... ) if ( DEBUG ) printf( args )
/* Voting state */
boolean voting_active = false;

/* RADIO CONFIGURATION */

/* Set up nRF24L01 radio on SPI bus plus pins CE 8 & CSN 7 */
RF24 radio( 8, 7 );

/* Radio pipe addresses for the 2 nodes to communicate (why LL) ? */
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

/* number of clients who are voting */
uint8_t client_count = 0;
uint8_t clients[10];

/*
 * function to get a client ID from the ID pool
 * return - 0 when successful
 * return - 1 when no more IDs left
 */
uint8_t getClientID( uint8_t* id ) {
  /* increase number of clients */
  client_count++;
  /* generate a new ID for the client */
  *id = client_id + client_count;
  /* add the generated ID to the list */
  clients[ client_count - 1 ] = *id;
  debug_printf( "id %X was generated...", *id );
  return SUCCESS;
}

/*
 * function to check which button was pressed
 * return - 0 when button was successfuly read
 */
uint8_t getPressedButton( uint8_t* button ) {
  *button = NO_BUTTON;
  int i;
  /* Go trough all the button and check if one was pressed */
  for ( i = 0; i < BUTTONS_COUNT; i++ )
    if ( digitalRead( buttons[i] ) ) *button = buttons[i]; else continue;
  /* When any of the buttons was pressed */
  if ( *button != 0 )
    debug_printf( "button %d was pressed...", *button );
  
  return SUCCESS;
}

/*
 * function to receive datapacket
 * return - 0 when reception was successful
 */
uint8_t receivePacket( boolean timeout ) {
  int i;
  for ( i = 0; i < PAYLOAD; i++ ) 
    rx_message[i] = 0x00;
  if ( !timeout ) {
    boolean done = false;
    while ( !radio.available() );
    while ( !done )
      done = radio.read( rx_message, radio.getPayloadSize() );
  } else {
    // Wait here until we get a response, or timeout (200ms)
    unsigned long started_waiting_at = millis();
    bool timeout = false;
    while ( !radio.available() && !timeout )
      if ( millis() - started_waiting_at > 200 )
        timeout = true;

    // Describe the results
    if ( timeout ) {
      debug_printf("Failed, response timed out.\n\r");
      return TIMEOUT;
    } else {
      // Grab the response, compare, and send to debugging spew
      radio.read( rx_message, radio.getPayloadSize() );
    }
  }
  /* only receive packets which are for us */
  if ( rx_message[DESTINATION] != role_id ) {
    debug_printf( "different destination %X...", rx_message[DESTINATION] );
    return WRONG_DESTINATION;
  }
  debug_printf( "source %X...", rx_message[SOURCE] );
  return SUCCESS;
}

/* 
 * function to transmits the datapacket
 * return - 0 when transmission successfuly finished
 */
uint8_t transmitPacket( byte destination, byte command, byte data ) {
  tx_message[DESTINATION] = destination;
  tx_message[SOURCE] = role_id;
  tx_message[COMMAND] = command;
  tx_message[DATA] = data;
  //tx_message[CHECKSUM] = destination ^ role_id ^ data;
  
  /* First, stop listening so we can talk */
  radio.stopListening();
  /* send the packet to the chosen destination */
  bool ok = radio.write( tx_message, radio.getPayloadSize() );
  /* Now, resume listening so we catch the next packets. */
  radio.startListening();
  
  /* When the transmission was successful */
  if ( ok )
    debug_printf( "transmit %X ok\n", command );
  else
    debug_printf( "failed.\n\r" );
  
  return SUCCESS;
}

void setup() {
  /* Initialize serial connection */
  Serial.begin( 57600 );
  /* Direct printf to Arduino serial */
  printf_begin();

  /* Tranceiver configuration */
  radio.begin();

  /* optionally, increase the delay(1) between retries(2) & # of retries */
  radio.setRetries( 15, 15 );

  // optionally, reduce the payload size.  seems to
  // improve reliability
  radio.setPayloadSize( PAYLOAD );
  radio.setAutoAck( true );
  radio.setDataRate( RF24_2MBPS );
  //radio.disableCRC();

  /* Open 'our' pipe for writing */
  /* Open the 'other' pipe for reading, in position #1 (we can have up to 5 pipes open for reading) */

  pinMode( 9, OUTPUT );
  if ( role_id == client_id ) {
    radio.openWritingPipe( pipes[0] );
    radio.openReadingPipe( 1, pipes[1] );
    debug_printf( "ROLE = client\n" );
    /* Initialize defined button (set as input) */
    int i;
    for ( i = 0; i < BUTTONS_COUNT; i++ )
      pinMode( buttons[i], INPUT );
  } else {
    radio.openWritingPipe( pipes[1] );
    radio.openReadingPipe( 1, pipes[0] );
    debug_printf( "ROLE = server\n" );
  }

  /* Start listening */
  radio.startListening();

  /* Dump the configuration of the rf unit for debugging */
  if ( DEBUG ) 
    radio.printDetails();
}

void loop() {
  /* Client role, send vote and receive ack */
  if ( role_id == client_id ) {
    /* Require new ID */
    if ( role_id == 0xC0 ) {
      transmitPacket( server_id, REQUEST_ID, role_id );
    /* Active voting sate */
    } else if ( voting_active ) {
      uint8_t button = NO_BUTTON;
      while ( button == NO_BUTTON ) {
        getPressedButton( &button );
      }
      /* send packet to server */
      transmitPacket( server_id, VOTE_ID, (byte) button );
      
      /* Take the time, and send it.  This will block until complete */
      //unsigned long time = micros();
      /* Transmit and receive time */
      //debug_printf( "%d\n", micros() - time );
    /* Done voting or voting dind't start yet */
    } else {
      if ( !radio.available() )
        return;
    }
    /* Receive ack with timeout */
    uint8_t response = receivePacket( true );
    if ( response == TIMEOUT || response == WRONG_DESTINATION ) {
      return;
    }
    switch ( rx_message[COMMAND] ) {
      /* When vote status was received, change the voting status */
      case VOTE_STATUS: debug_printf( "got vote status %X", rx_message[DATA] ); voting_active = rx_message[DATA]; break;
      /* When new ID from the server was received, assign it */
      case REQUEST_ACK: debug_printf( "got new id %X\n", rx_message[DATA] ); role_id = client_id = rx_message[DATA]; break;
      /* When vote ack was received from server */
      case VOTE_ACK: debug_printf( "got vote ack from server\n" ); digitalWrite( 9, HIGH ); break;
      default: debug_printf( "unknown command received %X\n", rx_message[COMMAND] ); break;
    }

    // Try again 1s later
    //delay(1000);
    digitalWrite( 9, LOW );
  }

  /* Server role, receive vote and send ack */
  if ( role_id == server_id ) {
    //digitalWrite( 9, HIGH );
    uint8_t response = receivePacket( false );
    if ( response == WRONG_DESTINATION ) {
      
    }
    //digitalWrite( 9, LOW );
    
    switch ( rx_message[COMMAND] ) {
      /* when ID request was received, generate and send a new ID for the new client */
      case REQUEST_ID: debug_printf( "id request\n" ); uint8_t id; getClientID( &id ); transmitPacket( rx_message[SOURCE], REQUEST_ACK, id ); break;
      /* when vote was received, send ack back to the client */
      case VOTE_ID: debug_printf( "button %X was pressed\n", rx_message[DATA] ); transmitPacket( rx_message[SOURCE], VOTE_ACK, rx_message[DATA] ); break;
      default: debug_printf( "unknown command received %X\n", rx_message[COMMAND] ); break;
    }
  }

}
