#include "../include/simulator.h"

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <cstring>
#include <vector>

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
float TIMEOUT = 25.0;

int seq_number_A; 
int ack_number_A; 
int window_size_A;
int send_base;
int next_seq_num;

int expected_seq_at_B; 

vector <msg> msg_buffer;

void send_data();
int generate_checksum(pkt packet);
bool is_pkt_corrupt(pkt packet);

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
	//cout<<"*****A output ****** msg to buffer : "<<message.data<<" , adding at : "<<(msg_buffer.size()-1)<<endl ;
	
	//Adding the message to a queue
	msg_buffer.push_back(message);
	
	// Starting a sending routine
	send_data();
}

void send_data(){
	//cout<< "******* udt_send ********* next_seq_num : "<< next_seq_num << " , send_base : "<< send_base<< endl;
	while(next_seq_num < send_base + window_size_A && next_seq_num<msg_buffer.size()){
		
		msg message = msg_buffer[next_seq_num];
		
		//cout<<"Sending message : "<<message.data<<", at next_seq_num : "<<next_seq_num<<endl;
		// Create packet 
		pkt packet;
		
		strncpy(packet.payload,message.data,sizeof(packet.payload)); // Copying message from above to current packet payload
		packet.seqnum = next_seq_num;
		packet.acknum = ack_number_A;
		packet.checksum = 0;
		
		packet.checksum = generate_checksum(packet);
		
		tolayer3(0, packet);
		
		if(send_base == next_seq_num){
			//cout<<"starting timeout in send data for : "<<next_seq_num<<endl;
			starttimer(0, TIMEOUT);
		}
		
		next_seq_num++;
		
	}
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
	//cout<< "**********A_input********* base num : "<<send_base<<endl;
	
	if(!is_pkt_corrupt(packet)){
		send_base = packet.acknum +1;
		
		if(send_base == next_seq_num)
			stoptimer(0);
		else{
			stoptimer(0);
			starttimer(0, TIMEOUT);
		}
		//cout<< "ack pkt not corrupt at A"<<endl;
	}
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
	//cout<< "**********A_timerinterrupt********* base num : "<<send_base<<endl;
	next_seq_num = send_base;
	send_data();
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
	seq_number_A = 0; 
	ack_number_A = 0; 
	window_size_A = getwinsize();
	send_base = 0;
	next_seq_num = 0;
	
	//cout<< "**********A_INIT********* Window size at A : "<<window_size_A<<endl;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
	//cout<< "**********B_input********* : "<<packet.payload<<", expected seq num : "<<expected_seq_at_B<<", pkt seq number : "<<packet.seqnum<<endl;
	if(!is_pkt_corrupt(packet) && expected_seq_at_B == packet.seqnum){// add packet corrupttion check
		//cout<< "**********Sending data to layer 5 of B********* "<<endl;
		tolayer5(1, packet.payload);
		
		pkt recev_ack;
		
		recev_ack.acknum = expected_seq_at_B;
		recev_ack.checksum = 0;
		
		recev_ack.checksum = generate_checksum(recev_ack);
		
		tolayer3(1, recev_ack);	
		
		expected_seq_at_B++;	
	}
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
	expected_seq_at_B = 0;
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

bool is_pkt_corrupt(pkt packet){
	
	if(packet.checksum == generate_checksum(packet))
		return false;
	else
		return true;
		
}