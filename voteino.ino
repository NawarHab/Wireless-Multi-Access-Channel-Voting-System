/*
 Copyright (C) 2011 J. Coliz <maniacbug@ymail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

/**
 * Example for Getting Started with nRF24L01+ radios. 
 *
 * This is an example of how to use the RF24 class.  Write this sketch to two 
 * different nodes.  Put one of the nodes into 'transmit' mode by connecting 
 * with the serial monitor and sending a 'T'.  The ping node sends the current 
 * time to the pong node, which responds by sending the value back.  The ping 
 * node can then see how long the whole cycle took.
 */

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

#define SUCCESS 1
#define NO_BUTTON 0
/* RX/TX packet structure */
#define ACK 0xAC
#define DESTINATION 0
#define SOURCE 1
#define DATA 2
//#define CHECKSUM 3
#define PACKET_LEN 3
#define PAYLOAD ( sizeof( byte ) * PACKET_LEN )
byte tx_message[PACKET_LEN];
byte rx_message[PACKET_LEN];

/* May need to be changed for different stations */
#define DEBUG 0
#define SERVER_ID 0xB1
#define CLIENT_ID 0xC1
#define BUTTONS_COUNT 1
uint8_t buttons[BUTTONS_COUNT] = { 3 };
/* The role of the current running sketch */
uint8_t role_id = CLIENT_ID;

//
// Hardware configuration
//

// Set up nRF24L01 radio on SPI bus plus pins 9 & 10 
// CE 9, CSN 10
RF24 radio(8, 7 );

//
// Topology
//

// Radio pipe addresses for the 2 nodes to communicate.
// why LL ?
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

//
// Role management
//
// Set up role.  This sketch uses the same software for all the nodes
// in this system.  Doing so greatly simplifies testing.  
//

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
    if ( DEBUG ) printf( "...button %d was pressed...", *button );
  
  return SUCCESS;
}

/* 
 * function to transmits the datapacket to the server
 * return - 0 when transmission successfuly finished
 */
uint8_t transmitPacket( byte destination, byte data ) {
  tx_message[DESTINATION] = destination;
  tx_message[SOURCE] = role_id;
  tx_message[DATA] = data;
  
  bool ok = radio.write( tx_message, radio.getPayloadSize() );
    
  //if ( ok )
    //printf( "ok..." );
  //else
    //printf( "failed.\n\r" );
  
  return SUCCESS;
}

void setup()
{
  //
  // Print init
  //
  Serial.begin( 57600 );
  printf_begin();
  if ( DEBUG ) printf( "\n\rRF24/examples/GettingStarted/\n\r" );

  //
  // Setup and configure rf radio
  //

  radio.begin();

  // optionally, increase the delay between retries & # of retries
  // delay, retries
  radio.setRetries( 15, 15 );

  // optionally, reduce the payload size.  seems to
  // improve reliability
  radio.setPayloadSize( PAYLOAD );
  radio.setAutoAck( false );
  radio.disableCRC();
  //
  // Open pipes to other nodes for communication
  //

  // This simple sketch opens two pipes for these two nodes to communicate
  // back and forth.
  // Open 'our' pipe for writing
  // Open the 'other' pipe for reading, in position #1 (we can have up to 5 pipes open for reading)

  pinMode( 9, OUTPUT );
  if ( role_id == CLIENT_ID )
  {
    radio.openWritingPipe(pipes[0]);
    radio.openReadingPipe(1,pipes[1]);
    if ( DEBUG ) printf( "ROLE = client" );
    /* Initialize button (set as input) */
    int i;
    for ( i = 2; i <= 6; i++ )
      pinMode( i, INPUT );
  }
  else
  {
    radio.openWritingPipe(pipes[1]);
    radio.openReadingPipe(1,pipes[0]);
    if ( DEBUG ) printf( "ROLE = server" );
  }

  //
  // Start listening
  //

  radio.startListening();

  //
  // Dump the configuration of the rf unit for debugging
  //

  if ( DEBUG ) radio.printDetails();
}

void loop()
{
  //
  // Ping out role.  Repeatedly send the current time
  //

  if ( role_id == CLIENT_ID )
  {
    // First, stop listening so we can talk.
    radio.stopListening();
    
    uint8_t button = NO_BUTTON;
    while ( button == NO_BUTTON )
    {
      getPressedButton( &button );
      delay( 100 );
    }
    
    // Take the time, and send it.  This will block until complete
    unsigned long time = millis();
    if ( DEBUG ) printf( "Now sending %X...", (byte) button );

    /* send packet to server */
    transmitPacket( SERVER_ID, (byte) button );

    // Now, continue listening
    radio.startListening();

    // Wait here until we get a response, or timeout (250ms)
    unsigned long started_waiting_at = millis();
    bool timeout = false;
    while ( ! radio.available() && ! timeout )
      if ( millis() - started_waiting_at > 200 )
        timeout = true;

    // Describe the results
    if ( timeout )
    {
      if ( DEBUG ) printf("Failed, response timed out.\n\r");
    }
    else
    {
      // Grab the response, compare, and send to debugging spew
      radio.read( rx_message, radio.getPayloadSize() );
      printf( "...time: %d...", millis() - time );
      // Spew it
      if ( DEBUG ) printf( "Got response %X \n", rx_message[DATA] );
      digitalWrite( 9, HIGH );
    }

    // Try again 1s later
    //delay(1000);
    digitalWrite( 9, LOW );
  }

  //
  // Pong back role.  Receive each packet, dump it out, and send it back
  //

  if ( role_id == SERVER_ID )
  {
    // if there is data ready
    if ( radio.available() )
    {
      // Dump the payloads until we've gotten everything
      bool done = false;
      while (!done)
      {
        // Fetch the payload, and see if this was the last one.
        done = radio.read( rx_message, radio.getPayloadSize() );

	// Delay just a little bit to let the other unit
	// make the transition to receiver
	delay(20);
      }

      // Spew it
      //printf( "Button was pressed %X by client %X...", rx_message[DATA], rx_message[SOURCE] );
      //digitalWrite( 9, HIGH );
      // First, stop listening so we can talk
      radio.stopListening();

      // Send the final one back.
      transmitPacket( CLIENT_ID, ACK );
      if ( DEBUG ) printf("Sent ACK.\n\r");

      // Now, resume listening so we catch the next packets.
      radio.startListening();
      //digitalWrite( 9, LOW );
    }
  }
}
// vim:cin:ai:sts=2 sw=2 ft=cpp
