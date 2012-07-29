/*
Voteino - Multiuser Voting system for Arduino

Copyright (C) 2012 Silver Kuusik <silver.kuusik@gmail.com>
Copyright (C) 2012 Balachandar Vittal <561989@gmail.com>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

/* MAY NEED TO BE CHANGED FOR DIFFERENT STATIONS */
#define BAUD_RATE 115200
/* Switch on = 1 or off = 0 for debug messages */
#define DEBUG 1
/* Start up delay in milliseconds */
#define WAIT_BEFORE_START 0
/* Voting buttons for clients */
#define BUTTONS_COUNT 4
uint8_t buttons[BUTTONS_COUNT] = { A1, A2, A3, A4 };
/* Role IDs */
uint8_t server_id = 0xB0;
uint8_t client_id = 0xC0;
uint8_t client_all_id = 0xAA;
/* The role of the current running sketch ( server_id or client_id )*/
uint8_t role_id = server_id;

/* VOTING RESULTS */
int votes[] = {0, 0, 0, 0, 0};

/* RETURN CODES */
#define SUCCESS 0
#define NO_BUTTON 0
#define NO_DATA 1
#define TRANSMIT_FAILED 2
#define WRONG_DESTINATION 3
/* RX/TX PACKET STRUCTURE */
#define DESTINATION 0
#define SOURCE 1
#define COMMAND 2
#define DATA 3
/* Each packet is 4 bytes */
#define PAYLOAD 4

/* COMMAND CODES */
/* Sent from server when vote was received */
#define VOTE_ACK 0xAC
/* Sent from client when user has voted */
#define VOTE_ID 0xAB
/* Sent from server when voting got active/inactive */
#define VOTE_STATE 0xEA
/* Sent from server when client requested new ID */
#define REQUEST_ACK 0xEC
/* Sent from client when it got active and needs new ID */
#define REQUEST_ID 0x1D
/* For debugging */
#define debug_printf( args ... ) if ( DEBUG ) printf( args )
/* VOTING STATES */
#define WAITING_VOTING 0xE1
#define ACTIVE_VOTING 0xE2
#define FINISHED_VOTING 0xE3
#define WAITING_TRANSMISSION 0xE4
/* Saves the time, so we can wait for a short period */
unsigned long started_wait_time = 0;
uint8_t client_old_state;
uint8_t client_state = REQUEST_ID;

/* RADIO CONFIGURATION */

/* Set up nRF24L01 radio on SPI bus plus pins CE 8 & CSN 7 */
RF24 radio( 8, 7 );

/* Radio pipe addresses for the 2 nodes to communicate */
const uint64_t pipes[2] = { 0xF0F0F0F0AA, 0xF0F0F0F0BB };

/* Number of clients who are voting */
uint8_t client_count = 0;

/*
 * function to get a client ID from the ID pool
 * @return - gives the new generated id
 */
uint8_t getNewClientID() {
  /* Increase number of clients */
  client_count++;
  /* Generate a new ID for the client */
  uint8_t id = client_id + client_count;
  debug_printf( "id %X was generated...", id );
  return id;
}

/*
 * function to switch status LEDS
 * @param green - the green LED status (HIGH or LOW)
 * @param yellow - the yellow LED status (HIGH or LOW)
 * @param flash - to make the chosen LED blink every second
 */
void setStatusLEDs( boolean green, boolean yellow, boolean flash ) {
  boolean green_led_status = green;
  boolean yellow_led_status = yellow;
  /* When blink is enabled */
  if ( flash ) {
    /* Change status every second */
    green_led_status = green && !((millis() / 1000) % 2);
    yellow_led_status = yellow && !((millis() /1000) % 2);
  }
  /* Set the status of the LEDs */
  digitalWrite( 10, green_led_status );
  digitalWrite( 9, yellow_led_status );
}

/*
 * function to check which button was pressed
 * @return - the pressed button or NO_BUTTON
 */
uint8_t getPressedButton() {
  uint8_t button = NO_BUTTON;
  int i;
  /* Go trough all the button and check if one was pressed */
  for ( i = 0; i < BUTTONS_COUNT; i++ )
    if ( analogRead( buttons[i] ) > 800 ) button = i + 1;
  /* When any of the buttons was pressed */
  if ( button != NO_BUTTON ) debug_printf( "button %d was pressed...", button );
  
  return button;
}

/*
 * function to receive datapacket
 * return - 0 when reception was successful
 */
uint8_t receivePacket( uint8_t *rx_packet ) {
  /* When we don't use timeout */
  if ( !radio.available() ) return NO_DATA;
  /* Read the data from the tranceiver */
  radio.read( rx_packet, radio.getPayloadSize() );
  /* Only receive packets which are for us or broadcast packets for all clients */
  if ( rx_packet[DESTINATION] != role_id && rx_packet[DESTINATION] != client_all_id ) {
    debug_printf( "different destination %X source %X...", rx_packet[DESTINATION], rx_packet[SOURCE] );
    return WRONG_DESTINATION;
  }
  debug_printf( "source %X...", rx_packet[SOURCE] );
  return SUCCESS;
}

/* 
 * function to transmits the datapacket
 * return - 0 when transmission successfuly finished
 */
uint8_t transmitPacket( byte destination, byte command, byte data ) {
  /* Assemble the transmission packet */
  uint8_t tx_packet[PAYLOAD];
  tx_packet[DESTINATION] = destination;
  tx_packet[SOURCE] = role_id;
  tx_packet[COMMAND] = command;
  tx_packet[DATA] = data;
  
  /* First, stop listening so we can talk */
  radio.stopListening();
  /* Send the packet to the chosen destination */
  bool ok = radio.write( tx_packet, radio.getPayloadSize() );
  /* Now, resume listening so we catch the next packets. */
  radio.startListening();
  
  /* When the transmission was not successful */
  if ( !ok ) {
    debug_printf( "transmit %X to %X failed.\n\r", command, destination );
    return TRANSMIT_FAILED;
  }
  
  debug_printf( "transmit %X to %X ok\n", command, destination );
  return SUCCESS;
}

void setup() {
  /* Wait specified time before startup */
  delay( WAIT_BEFORE_START );
  /* Initialize serial connection */
  Serial.begin( BAUD_RATE );
  /* Direct printf to Arduino serial */
  printf_begin();

  /* Tranceiver configuration */
  radio.begin();

  /* optionally, increase the delay(1) between retries(2) & # of retries */
  /* We can't use this because the transmission is on one channel */
  //radio.setRetries( 15, 15 );

  /* Optionally, reduce the payload size, seems to improve reliability */
  radio.setPayloadSize( PAYLOAD );
  /* Use manual ACK when needed, we can't use this because the transmission is on one channel */
  radio.setAutoAck( false );
  /* Set maximum datarate for decreasing delay xD */
  radio.setDataRate( RF24_2MBPS );

  /* Open 'our' pipe for writing */
  /* Open the 'other' pipe for reading, in position #1 (we can have up to 5 pipes open for reading) */
  if ( role_id == client_id ) {
    radio.openWritingPipe( pipes[1] );
    radio.openReadingPipe( 1, pipes[0] );
    debug_printf( "ROLE = client\n" );
  } else {
    radio.openWritingPipe( pipes[0] );
    radio.openReadingPipe( 1, pipes[1] );
    debug_printf( "ROLE = server\n" );
  }

  /* Start listening */
  radio.startListening();

  /* Dump the configuration of the rf unit for debugging */
  if ( DEBUG ) radio.printDetails();
    
  /* Initialize the LEDs, D10 is also SPIs->SS */
  pinMode( 9, OUTPUT );
  pinMode( 10, OUTPUT   );
  digitalWrite( 9, LOW );
  digitalWrite( 10, LOW );
}

void loop() {
  /* Client role, send vote and receive ack */
  if ( role_id == client_id ) {
    uint8_t button = NO_BUTTON;
    switch ( client_state ) {
      /* Request for a new ID */
      case REQUEST_ID: 
        transmitPacket( server_id, REQUEST_ID, role_id );
        client_old_state = REQUEST_ID;
        client_state = WAITING_TRANSMISSION;
        break;
      /* Waiting voting to be started */
      case WAITING_VOTING: 
        setStatusLEDs( LOW, HIGH, false ); 
        break;
      /* Wait for user to vote */
      case ACTIVE_VOTING:
        setStatusLEDs( LOW, HIGH, true );
        button = getPressedButton();
        /* When the user has voted, send the vote */
        if ( button != NO_BUTTON ) {
          transmitPacket( server_id, VOTE_ID, button );
          client_old_state = ACTIVE_VOTING;
          client_state = WAITING_TRANSMISSION;
        }
        break;
      /* Done voting */
      case FINISHED_VOTING: 
        setStatusLEDs( HIGH, LOW, false ); 
        break;
      /* Wait packet to be transmitted */
      case WAITING_TRANSMISSION: 
        if ( started_wait_time == 0 ) started_wait_time = millis();
        if ( millis() - started_wait_time > 100 ) client_state = client_old_state;
        break;
    }
    /* When we escaped transmission waiting state */
    if ( client_state != WAITING_TRANSMISSION ) started_wait_time = 0;
    
    /* Check for received packet */
    uint8_t rx_packet[PAYLOAD];
    uint8_t response = receivePacket( rx_packet );
    /* When responded with no data or wrong destination, don't process packet */
    if ( response == NO_DATA || response == WRONG_DESTINATION ) return;
    /* Process the received command in the packet */
    switch ( rx_packet[COMMAND] ) {
      /* When vote state was received, change the voting state */
      case VOTE_STATE: 
        debug_printf( "got vote status %X", rx_packet[DATA] ); 
        client_state = rx_packet[DATA]; 
        break;
      /* When new ID from the server was received, assign it */
      case REQUEST_ACK: 
        debug_printf( "got new id %X\n", rx_packet[DATA] ); 
        role_id = client_id = rx_packet[DATA]; 
        client_state = WAITING_VOTING; 
        break;
      /* When vote ack was received from server */
      case VOTE_ACK: 
        debug_printf( "got vote ack from server\n" );  
        client_state = FINISHED_VOTING; 
        break;
      /* When unknown command was received */
      default: 
        debug_printf( "unknown command received %X\n", rx_packet[COMMAND] ); 
        break;
    }
  }

  /* Server role, receive vote and send ack */
  if ( role_id == server_id ) {
    /* When there are clients connected to server and we receive command from serial */
    if ( client_count >= 0 ) {
      uint8_t button = getPressedButton();
      char command;
      /* When there is data available in serial, read the data */
      if ( Serial.available() ) command = Serial.read();
      uint8_t state = 0;
      /* Specify the command from serial */
      if ( command == 'w' || button == 1 ) state = WAITING_VOTING;
      else if ( command == 'a' || button == 2 ) state = ACTIVE_VOTING;
      else if ( command == 'f' || button == 3 ) state = FINISHED_VOTING;
      else if ( command == 'v' || button == 4 ) {
        /* Print the voting result, last value represents not voted count */
        printf( "%d,%d,%d,%d,%d\n", votes[0], votes[1], votes[2], votes[3], client_count - votes[0] - votes[1] - votes[2] - votes[3] );
        delay(100); 
      }
      /* When state was selected, send it to all the clients */
      if ( state != 0 ) { transmitPacket( client_all_id, VOTE_STATE, state ); delay( 100 ); }
    }
    
    /* Check for received packet */
    uint8_t rx_packet[PAYLOAD];
    uint8_t response = receivePacket( rx_packet );
    /* When responded with no data or wrong destination, don't process packet */
    if ( response == NO_DATA || response == WRONG_DESTINATION ) return;
    
    switch ( rx_packet[COMMAND] ) {
      /* When ID request was received, generate and send a new ID for the new client */
      case REQUEST_ID: 
        debug_printf( "id request\n" ); 
        transmitPacket( rx_packet[SOURCE], REQUEST_ACK, getNewClientID() ); 
        break;
      /* When vote was received, send ack back to the client */
      case VOTE_ID: 
        debug_printf( "button %X was pressed\n", rx_packet[DATA] );
        votes[rx_packet[DATA]-1] += 1;
        transmitPacket( rx_packet[SOURCE], VOTE_ACK, rx_packet[DATA] );
        break;
      /* When we received unknown command */
      default: 
        debug_printf( "unknown command received %X\n", rx_packet[COMMAND] ); 
        break;
    }
  }

}



