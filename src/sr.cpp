#include "../include/simulator.h"

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <cstring>
#include <vector>
#include <queue>
#include <deque>
#include <map>
#include <unistd.h>

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

float TIMEOUT = 15.0;

int seq_number_A; 
int ack_number_A; 
int window_size_A;
int send_base;
int next_seq_num;

int expected_seq_at_B; 
int ack_number_B; 
int window_size_B;
int recv_base;

struct pkt packet_in_action;

int generate_checksum(pkt packet);
bool is_pkt_corrupt(pkt packet);
void send_data(int seq_num, bool is_selective_repeat);
void insertionSort (vector<int> data, int n);

struct msg_metadata{
	float start_time;
	bool is_acknowleged;
	char msg_data[20];
};

vector <msg_metadata> msg_meta_buffer;

deque<int> timer_sequence;

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
	float current_time = get_sim_time();
	//cout<<"*******A_output******* time of msg : "<<message.data<<" , base : "<<send_base<<", next seq : "<<next_seq_num<<", curr time : "<<current_time<< endl;
	
	msg_metadata msg_meta;
	msg_meta.is_acknowleged = false;
	msg_meta.start_time = -1;
	strncpy(msg_meta.msg_data,message.data,sizeof(message.data));
	msg_meta_buffer.push_back(msg_meta);
	
	send_data(-1, false);// First time send, repeat and sequence number not required

}

void send_data(int seq_num, bool is_selective_repeat){
	
	//cout<<"Send data : base : "<<send_base<<", next seq : "<<next_seq_num<<", seq_num : "<<seq_num<< endl;
	
	if(is_selective_repeat && seq_num >= send_base && seq_num < send_base + window_size_A){
		//cout<<"Send data : 1 , seq_number : "<<seq_num<<endl;
		
		pkt packet;
		
		strncpy(packet.payload, msg_meta_buffer[seq_num].msg_data ,sizeof(packet.payload)); // Copying message from above to current packet payload
		packet.seqnum = seq_num;
		packet.acknum = ack_number_A;
		packet.checksum = 0;	
		
		packet.checksum = generate_checksum(packet);
		
		tolayer3(0, packet);
		
		msg_meta_buffer[seq_num].start_time = get_sim_time();
		
		timer_sequence.push_back(seq_num);
		
		if(timer_sequence.size()==1){
			//cout<<">>>>>>>>>  IMP : STARTING MAIN TIMER with TIMEOUT "<<TIMEOUT<<endl;
			starttimer(0, TIMEOUT);
		}
		
		//cout<<"ADDING TIMER 1 for seq num : "<<seq_num<<endl;
	}
	else if(next_seq_num >= send_base && next_seq_num < send_base + window_size_A /**&& next_seq_num<msg_buffer.size()**/){
		//cout<<"Send data : 2 , SENDING : "<<msg_meta_buffer[next_seq_num].msg_data <<endl;
		pkt packet; // Resetting the current packet for new data
			
		strncpy(packet.payload, msg_meta_buffer[next_seq_num].msg_data ,sizeof(packet.payload)); // Copying message from above to current packet payload
		packet.seqnum = next_seq_num;
		packet.acknum = ack_number_A;
		packet.checksum = 0;	
		
		packet.checksum = generate_checksum(packet);
		
		tolayer3(0, packet);
		
		msg_meta_buffer[next_seq_num].start_time = get_sim_time();
		timer_sequence.push_back(next_seq_num);
		
		//cout<<"ADDING TIMER 2 for next_seq_num : "<<next_seq_num<<endl;
	
		if(timer_sequence.size()==1){
			//cout<<">>>>>>>>>  IMP : STARTING MAIN TIMER with TIMEOUT "<<TIMEOUT<<endl;
			starttimer(0, TIMEOUT);
		}		
		
		next_seq_num++;
	}
	else{
	//	cout<<">>>>>>>>> OUT of Window >>>>>>>>>> window : [ "<<send_base<<" , "<<send_base+window_size_A<<endl;
	}
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
	//if good acknowledgement move next seq to last unack seq
	
	if(!is_pkt_corrupt(packet)){
		//cout<<"******** A_input :  Receiving good ack of seq : "<<packet.acknum<<", base rite now : "<<send_base<<" : next_seq_num : "<<next_seq_num<<", size of buffer : "<<msg_meta_buffer.size()<<endl;

		
		msg_meta_buffer[packet.acknum].is_acknowleged = true;
		if(send_base == packet.acknum){
			//cout<<"******** A_input : send_base is equal to acknowledged" <<endl;
			//Move base to the next unacknowledged
			while(msg_meta_buffer.size()>send_base && msg_meta_buffer[send_base].is_acknowleged)
				send_base++;
			//cout<<"	A_input : window size changed : [ "<<send_base<<" , "<<send_base+window_size_A<<endl;
			while(next_seq_num < send_base + window_size_A && next_seq_num < msg_meta_buffer.size()){
				send_data(-1, false);
			}	
		}
		
		if(timer_sequence.front() == packet.acknum){
			//cout<<" A_input : stopping timer "<<endl;
			stoptimer(0);
			timer_sequence.pop_front();
			
			while(timer_sequence.size() > 0 && timer_sequence.size() <= window_size_A && msg_meta_buffer[timer_sequence.front()].is_acknowleged){
			//	cout<< "A_input ********** timer seq item was acknowledged ********* "<<timer_sequence.front()<<", tseq size : "<<timer_sequence.size()<<endl;
				timer_sequence.pop_front();
			}
			//cout<< "A_input timer seq queue size remaining : "<<timer_sequence.size() <<endl;
			if(timer_sequence.size() > 0 && timer_sequence.size() <= window_size_A){				
				float next_timeout = TIMEOUT - (get_sim_time() - msg_meta_buffer[timer_sequence.front()].start_time);
				//cout<< "A_input ***** next_timeout : "<<next_timeout<<endl;
				starttimer(0, next_timeout);
			}
		}
	}
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
	//cout<< "**********A_timerinterrupt********* "<<endl;
	
	int interrupted_seq = timer_sequence.front();
	timer_sequence.pop_front();
	
	//cout<< "**********A_timerinterrupt********* interrupted_seq : "<<interrupted_seq<<", timer seq que size ; "<<timer_sequence.size()<<endl;
	
	while(timer_sequence.size() > 0 && timer_sequence.size() <= window_size_A && msg_meta_buffer[timer_sequence.front()].is_acknowleged){
		//cout<< "********** timer seq item was acknowledged ********* "<<timer_sequence.front()<<endl;
		timer_sequence.pop_front();
	}
	
	if(timer_sequence.size() > 0 && timer_sequence.size() <= window_size_A){
		cout<< "current time : "<<get_sim_time()<<", front timer : "<<msg_meta_buffer[timer_sequence.front()].start_time<<endl;
		////cout<<"time diff : "<<(get_sim_time() - msg_meta_buffer[timer_sequence.front()].start_time);
		float next_timeout = TIMEOUT - (get_sim_time() - msg_meta_buffer[timer_sequence.front()].start_time);
		//cout<< "next_timeout : "<<next_timeout<<endl;
		starttimer(0, next_timeout);
	}
	
	send_data(interrupted_seq, true);
	
	//cout<< "**********A_timerinterrupt END ********* timer seq que size : "<<timer_sequence.size()<<endl;
	
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
	
	seq_number_A = 0; 
	ack_number_A = 0; 
	window_size_A = getwinsize()/2;
	send_base = 0;
	next_seq_num = 0;
	
	//cout<< "**********A_INIT********* Window size at A : "<<window_size_A<<endl;
	
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */


map<int, pkt> recv_buffer;

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
	//cout<< "**********B_input********* : "<<packet.payload<<", recv_base : "<<recv_base<<", pkt seq number : "<<packet.seqnum<<endl;
	
	if(!is_pkt_corrupt(packet) && recv_base == packet.seqnum){// recv base receive case
		//cout<< "**********Sending data to layer 5 of B********* "<<endl;
		tolayer5(1, packet.payload);
		
		pkt recev_ack;
		
		recev_ack.acknum = packet.seqnum;
		recev_ack.checksum = 0;
		
		recev_ack.checksum = generate_checksum(recev_ack);
		
		tolayer3(1, recev_ack);	
		
		recv_base++;
		
		//expected_seq_at_B++;	
	}
	else if(!is_pkt_corrupt(packet) &&  packet.seqnum >= recv_base && packet.seqnum < recv_base+window_size_B){
		//cout<< "**********Buffering data at B********* "<<endl;
		
		recv_buffer[packet.seqnum] = packet;
		
		pkt recev_ack;
		
		recev_ack.acknum = packet.seqnum;
		recev_ack.checksum = 0;
		
		recev_ack.checksum = generate_checksum(recev_ack);
		
		tolayer3(1, recev_ack);	
	}
	else if(!is_pkt_corrupt(packet)){
		pkt recev_ack;
		recev_ack.acknum = packet.seqnum;
		recev_ack.checksum = 0;
		
		recev_ack.checksum = generate_checksum(recev_ack);
		
		tolayer3(1, recev_ack);	
	}
	
	map<int, pkt>::iterator it;

	it = recv_buffer.find(recv_base);
	
	while (it != recv_buffer.end()){
		pkt buffered_pkt = it->second;
		char buffered_data[20];
		strncpy(buffered_data, buffered_pkt.payload ,sizeof(buffered_data));
		//cout<< "**********Sending buffered data to layer 5 at B*********  : "<<buffered_data<<", at seq : "<<recv_base<<endl;
		tolayer5(1, buffered_data);
		recv_buffer.erase(it);
		recv_base++;
		it = recv_buffer.find(recv_base);
	}
		
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
	expected_seq_at_B = 0;
	window_size_B = getwinsize()/2;
	recv_base = 0;
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
