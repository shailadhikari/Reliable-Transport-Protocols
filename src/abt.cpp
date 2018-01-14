#include "../include/simulator.h"

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <cstring>
#include <queue>

using namespace std;


/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional data transfer 
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

/* called from layer 5, passed the data to be sent to other side */


float TIMEOUT = 20.0;

bool is_A_transmit_ready;

int seq_number_A; // Seq number to be sent
int ack_number_A; // Always zero

int seq_number_B; // Seq number expected from below
int ack_number_B; // next expected seq no from A

struct pkt packet_in_action;

int generate_checksum(pkt packet);
bool isPktCorrupt(pkt packet);

queue<msg> msg_queue;

void A_output(struct msg message)
{
	//cout << "*****  MSG FROM ABOVE ****** :" << message.data<< endl;
	
	if(is_A_transmit_ready){ // A is ready to take the new msg as ack received for previous
	
		is_A_transmit_ready = false; // making check to hold till acknowledgement

		packet_in_action = {}; // Resetting the current packet for new data
		
		strncpy(packet_in_action.payload,message.data,sizeof(packet_in_action.payload)); // Copying message from above to current packet payload
		packet_in_action.seqnum = seq_number_A;
		packet_in_action.acknum = ack_number_A;
		packet_in_action.checksum = 0;
		
		packet_in_action.checksum = generate_checksum(packet_in_action);
		
		//cout << "*****A******* MSG FROM ABOVE ****** : " << packet_in_action.payload <<" ; seq : "<<packet_in_action.seqnum<<" ; ack : "<<packet_in_action.acknum<< endl;
	
		tolayer3(0, packet_in_action);
		
		starttimer(0, TIMEOUT);
	}
	else{
		msg_queue.push(message);
		//cout << "*****  pushing MESSAGE to QUEUE ****** "<< endl;
	}	
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
	
	//cout << "In acknowledgement : packet.acknum "<<packet.acknum <<", next_seq_number_A : "<<seq_number_A<<", is packet corrupt : "<<isPktCorrupt(packet)<<endl;
	if(!isPktCorrupt(packet) && packet.acknum == seq_number_A ){ // packet is not corrupt and B is ready to get next seq number
		//cout << ">>>>>>>>>>>>>>>>>>>>good acknowledgement : msg_queue size : " <<msg_queue.size()<<endl;
		stoptimer(0);
		seq_number_A = !seq_number_A;
		
		if(!msg_queue.empty()){
			msg next_msg = msg_queue.front();
			msg_queue.pop();
			is_A_transmit_ready = true;
			A_output(next_msg);	
		}
		else
			is_A_transmit_ready = true;	
	}	
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
	//cout << "*****A_timerinterrupt********* : seq_number_A : "<<seq_number_A<<" ; pkt seq : "<<packet_in_action.seqnum <<endl;
	is_A_transmit_ready = false;
	tolayer3(0, packet_in_action);
	starttimer(0, TIMEOUT);
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{	
	is_A_transmit_ready = true;
	seq_number_A = 0;
	ack_number_A = 0;	
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
	
	//cout << "****B******* MSG FROM BELOW ****** : " << packet.payload <<" , se expctd : "<<seq_number_B<<" ; received : "<<packet.seqnum<<endl;
		
	pkt ack_packet = {};
	
	 if(!isPktCorrupt(packet) && packet.seqnum == seq_number_B){
		tolayer5(1, packet.payload);
		ack_packet.acknum = seq_number_B;
		ack_packet.checksum = 0;
		ack_packet.checksum = generate_checksum(ack_packet);	
		//cout << "**** B_input 2 ****** Sending to layer 5, packet was NOT corrupt, new seq can be send " << ack_packet.acknum << endl;
		tolayer3(1, ack_packet);
		seq_number_B = !seq_number_B;
	}
	else if(!isPktCorrupt(packet) && packet.seqnum != seq_number_B){
		ack_packet.acknum = !seq_number_B;
		ack_packet.checksum = 0;
		ack_packet.checksum = generate_checksum(ack_packet);	
		//cout << "**** B_input 3 ****** Already received packet , ackn of current seq number " << ack_packet.acknum << endl;
		tolayer3(1, ack_packet);
	}
		
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
	seq_number_B = 0;
}

int generate_checksum(pkt packet){
	int checksum = 0;
	
	int seq_number = packet.seqnum;
	int ack_number = packet.acknum;
	
	checksum += seq_number;
	checksum += ack_number;
	
	for(int p=0; p<sizeof(packet.payload); p++)
		checksum += packet.payload[p];
	
	return ~checksum;
}

bool isPktCorrupt(pkt packet){
	
	if(packet.checksum == generate_checksum(packet))
		return false;
	else
		return true;
		
}
