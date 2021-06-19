#include <sstream>
#include <istream>
#include <iterator>
#include <fstream>

#include <list>
#include <math.h>
#include <iostream>
#include <unordered_map>
#include <queue>
#include <string>

using namespace std;

#include "classes.hpp"
#include "data.hpp"






/*input: global containers
* a line read from the input file
*output:
* a tupple containing all information in the correct type
*/
tuple<packet, string, double, bool> get_line_info(string parameters_line);


/*input: global containers
* time of a certain event
*output:
* the corresponding round of the event
*/
double calc_round(int event_time);


/*input: global containers
*
*output:
* the sum of active weights at the current event
*/
double calc_sum_active_weights();


/*inputs:
* an arriving packet,
* the last of the previous packet in the same connection,
*
* the function calculates the last of the packet and inserts the arriving packet to the GPS container - sorted_packets - while
* also updating the packet in the connection_q accordingly
*/
void update_last_and_add_packet_to_list(packet &p, double prev_pckt_last);


/*input: global containers
* a tuple as created by get_line_info containing all valuable information about the arrival
*
* the function handles the umap container and inserts connections to it if they are not already in it.
* if the connection exists already in the umap, we simply push the new packet to the connection queue,
* while updating the time of initial arrival of the packet, and also updating the weight as needed
*  - update weight of connection if a new weight was assigned, or change the weight of the packet
* to be the same as the connection weight.
*
* finaly - call function update_last_and_add_packet_to_list.
*
*/
void handle_arrival(list<tuple<packet, string, double, bool>>::iterator info_tup_iter);


/*input: global containers
*
* this function is a wrapper to all the other functions for handling the wfq algorithm
*/
void handle_wfq();


/*input: global containers
* this function is a wrapper to handle_arrival for handling all of the arrivals from different connections at the same time.
*/
void handle_all_arrivals();


/*input: global containers
* last of a packet
* output:
* the real time of the departure of the packet according to GPS
*/
double get_time_of_last(double last);


/*input: global containers
* a packet
* output:
* indicator if before the call to the function the packet wasn't already queued. true if this is the case.
*
* the function inserts a packet to the wfq and writes it to the file only in case it wasn't already queued.
*/
bool enqueue_packet_and_write_to_file(packet &p);


/*input: global containers
*
* this function simply updates the time of last departure, and pops the departuring packet from the wfq.
*/
void dequeue_packet();


/*input: global containers
*
* the function pops a packet that recetly finished processing according to GPS.
*/
void pop_from_GPS();


/*input: global containers
*
* the function is a wrapper to enqueue_packet_and_write_to_file and pop_from_GPS.
*/
void enqueue_and_pop_from_GPS();


/*input: global containers
*
* this function checks all possible events and sets the current_event tuple as the next one (with minimal time\round)
*
*/
void set_current_event();



int departure_time = 0;
int last_departure_time = 0;
int arrival_time = 0;


tuple<int, double, double, double>current_event = make_tuple(ARRIVAL, 0, 0, 1);
tuple<int, double, double, double>last_event = make_tuple(ARRIVAL, 0, 0, 1);


queue<packet> wfq;
unordered_map <string, umap_val> umap;
list<packet> sorted_packets;
list<tuple<packet, string, double, bool>> same_time_buffer;




int main()
{
	
	string parameters_line; 
	packet last_same_time_packet;
	tuple<packet, string, double, bool> info_tup;
	list<tuple<packet, string, double, bool>>::iterator it;

	ofstream out("out.txt");
	auto coutbuf = cout.rdbuf(out.rdbuf()); //save and redirect

	while (getline(cin, parameters_line))
	{
		//cout << "read line :\n" << parameters_line << "\n" << endl;

		info_tup = get_line_info(parameters_line);

		if (same_time_buffer.empty())
		{
			same_time_buffer.emplace_front(info_tup);

			last_same_time_packet = get<PACKET>(info_tup);

			if (cin) continue;			
			
		}
		
		else if (get<PACKET>(info_tup).arrival_time == get<PACKET>(same_time_buffer.front()).arrival_time)
		{
			same_time_buffer.emplace_back(info_tup);

			last_same_time_packet = get<PACKET>(info_tup);

			if (cin) continue;				
		}

		/*
		cout << "\n========================================\nsame_time_buffer contains these packets:\n" << endl;
 		for (it_same_time = same_time_buffer.begin(); it_same_time != same_time_buffer.end(); it_same_time++)
		{
			get<PACKET>(*it_same_time).print();
		}
		cout << "\n========================================\n" << endl;
		*/
		/*
		cout << "before handle_wfq, queue is: ";
		if (wfq.empty())
		{
			cout << "EMPTY" << endl;
		}
		else
		{
			cout << "FULL" << endl;
			cout << "front of wfq:" << endl;
			wfq.front().print();
			if (wfq.back() != wfq.front())
			{
				cout << "back of wfq:" << endl;
				wfq.back().print();
			}
		}
		cout << "same_time_buffer contains:" << endl;
		for (it = same_time_buffer.begin(); it != same_time_buffer.end(); it++)
		{
			get<PACKET>(*it).print();
		}
		*/

		handle_wfq();


		//now insert the info_tup that wasn't inserted yet to the buffer.
		if (last_same_time_packet != get<PACKET>(info_tup))
		{
			same_time_buffer.emplace_front(info_tup);
			last_same_time_packet = get<PACKET>(info_tup);
		}
	}


	/*===========================================================================
	   here - we finished reading the arrivals from the file and now we need
	   to simply finish processing all the remaining packets in our containers.
	===========================================================================*/
	
	if (!same_time_buffer.empty())
	{
		handle_wfq();
	}
	
	while (!sorted_packets.empty())
	{
		enqueue_and_pop_from_GPS();
	}

	while (!wfq.empty())
	{
		dequeue_packet();
	}
	
	return 0; 
}










tuple<packet, string, double, bool> get_line_info(string parameters_line)
{
	stringstream stream(parameters_line);
	string str_current, ip_s, port_s, ip_d, port_d, connection_designator;
	int arrival_time = 0;
	int length = 0;
	packet pckt;
	double weight = 0;
	bool update_weight = false;

	weight = DEFAULT_WEIGHT;

	//store arrival time to str_current, then convert it to integer
	stream >> str_current;
	arrival_time = stoi(str_current);

	//fetch connection data and store it in the designator
	stream >> ip_s >> port_s >> ip_d >> port_d;
	connection_designator = ip_s + " " + port_s + " " + ip_d + " " + port_d;

	//store length in str_current, then convert it to integer
	stream >> str_current;
	length = stoi(str_current);

	//read stream into str_current last time for weight
	stream >> str_current;

	//if true - it means there was a weight to read, otherwise it means 
	//we already reached the end of line, so the waight is the default
	if (stream)
	{
		update_weight = true;
		weight = stof(str_current);
	}

	//create packet
	pckt = packet(length, arrival_time, weight, connection_designator, update_weight);

	/*
	if (update_weight)
	{
		cout << "received weight: " << pckt.weight << endl;
	}
	*/

	return make_tuple(pckt, connection_designator, weight, update_weight);
}


double calc_round(int event_time)
{
	//cout << "round of last event: " << get<ROUND>(last_event) << endl;
	//cout << "get<TIME>(last_event): " << get<TIME>(last_event) << endl;

	/*
	cout << "\nget<SUM_WEIGHTS>(last_event) = " << get<SUM_WEIGHTS>(last_event) << endl;
	cout << "get<SUM_WEIGHTS>(current_event) =  " << get<SUM_WEIGHTS>(current_event) << endl;
	cout << "calc_sum_active_weights() = " << calc_sum_active_weights() << endl;
	cout << "type of last event: " << get<TYPE>(current_event) << " get<TIME>(last_event) = " << get<TIME>(last_event) << " event time = " << event_time << endl;
	cout << "x = " << (event_time - get<TIME>(last_event)) << " last round = " << get<ROUND>(last_event) << endl;
	*/

	//return (  get<ROUND>(last_event) + ( (event_time - get<TIME>(last_event)) / calc_sum_active_weights()));

	return (get<ROUND>(last_event) + ((event_time - get<TIME>(last_event)) / get<SUM_WEIGHTS>(last_event)));
}

double calc_sum_active_weights()  
{
	double sum_weights = 0;
	list<packet>::iterator it;

	if (sorted_packets.empty())
	{
		sum_weights = 1; //round advances as fast as time
	}
	else
	{
		for (it = sorted_packets.begin(); it != sorted_packets.end(); it++)
		{
			//cout << "at calc weighst outsid the if, packet: " << endl;
			//it->print();
			if ((*it) == umap[(*it).connection].connection_q.front())
			{
				//cout << "entered if at calc weighst with packet: "  << endl;
				//it->print();
				sum_weights += it->weight;
			}
				
		}
	}
	//cout << "calc_sum_active_weights returns: " << sum_weights << endl;
	return sum_weights;
}


void handle_wfq()
{
	list<tuple<packet, string, double, bool>>::iterator it_info_tup;
	list<packet>::iterator it_sorted;
	
	while (!same_time_buffer.empty())
	{
		arrival_time = get<PACKET>(same_time_buffer.front()).arrival_time;

		if (wfq.empty() && !sorted_packets.empty())
		{
			for (it_sorted = sorted_packets.begin(); it_sorted != sorted_packets.end(); it_sorted++)
			{
				//cout << "wfq is empty, outside of the if, arrival time of packet: " << it_sorted->arrival_time << endl;
				//enqueue_packet will push the next packet to the queue if it wasn't pushed already, 
				//and returns a bool variable that states wheather the packet was pushed by the function now or not
				if (enqueue_packet_and_write_to_file(*it_sorted))
				{
					break; //will push only the next packet in line to the wfq
				}
			}
		}

		/*
		cout << "\nlast event was: ";
		if (get<TYPE>(last_event) == ARRIVAL)
		{
			cout << "ARRIVAL ";
		}
		else if(get<TYPE>(last_event) == MIN_LAST)
		{
			cout << "MIN_LAST ";
		}
		else
		{
			cout << "DEPARTURE ";
		}
		cout << "at time = " << get<TIME>(last_event) << ", round = " << get<ROUND>(last_event) << ", SUM_WEIGHTS = " << get<SUM_WEIGHTS>(last_event) << endl;

		cout << "current_event is: ";
		if (get<TYPE>(current_event) == ARRIVAL)
		{
			cout << "ARRIVAL ";
		}
		else if (get<TYPE>(current_event) == MIN_LAST)
		{
			cout << "MIN_LAST ";
		}
		else
		{
			cout << "DEPARTURE ";
		}
		cout << "at time = " << get<TIME>(current_event) << ", round = " << get<ROUND>(current_event) << ", SUM_WEIGHTS = " << get<SUM_WEIGHTS>(current_event) << endl;
		*/

		get<TYPE>(last_event) = get<TYPE>(current_event);
		get<TIME>(last_event) = get<TIME>(current_event);
		get<ROUND>(last_event) = get<ROUND>(current_event);
		get<SUM_WEIGHTS>(last_event) = get<SUM_WEIGHTS>(current_event);

		set_current_event();

		if (DEPARTURE == get<TYPE>(current_event))
		{
			//cout << "departure :" << departure_time << endl;
			dequeue_packet();
		}
		else if (MIN_LAST == get<TYPE>(current_event))
		{
			//cout << "last real time: " << get<TIME>(current_event) << endl;
			enqueue_and_pop_from_GPS();
		}
		else // ARRIVAL
		{
			//cout << "arrival time: " << get<TIME>(current_event) << " arrival round: " << get<ROUND>(current_event) <<endl;
			handle_all_arrivals();

		}
	}
}


void update_last_and_add_packet_to_list(packet &p, double prev_pckt_last)
{
	p.round_of_calculation_of_last = max(p.arrival_round, prev_pckt_last);
	p.last = p.round_of_calculation_of_last + p.length / p.weight;

	umap[p.connection].connection_q.back().last = p.last;
	umap[p.connection].connection_q.back().round_of_calculation_of_last = p.round_of_calculation_of_last;

	sorted_packets.emplace_front(p);
	//cout << "sorted_packets.emplace_front(packet) with weight: " << sorted_packets.front().weight << endl;
	sorted_packets.sort();
}


void handle_arrival(list<tuple<packet, string, double, bool>>::iterator info_tup_iter)
{
	double prev_pckt_last = QUEUE_EMPTY;
	//the value QUEUE_EMPTY indicates that there were no packets 
	//in the connection_q at the time the new packet arrived
	
	get<PACKET>(*info_tup_iter).arrival_round = get<ROUND>(current_event);
	
	/*=================================
			take care of umap
	  =================================*/

	if (umap.find(get<CONNECTION_DESIGNATOR>(*info_tup_iter)) == umap.end())
	{
		//if we reached here the connection was not mapped yet, so map it
		
		//cout << "connection " << get<CONNECTION_DESIGNATOR>(*info_tup_iter) << " NOT found in umap";

		umap[get<CONNECTION_DESIGNATOR>(*info_tup_iter)] =
			umap_val(get<WEIGHT>(*info_tup_iter), get<PACKET>(*info_tup_iter));
	}
	else
	{
		get<PACKET>(*info_tup_iter).initial_connection_arrival_time = umap[get<CONNECTION_DESIGNATOR>(*info_tup_iter)].initial_arrival_time;

		//if we reached here the connection was already mapped
		if (!umap[get<CONNECTION_DESIGNATOR>(*info_tup_iter)].connection_q.empty())
		{
			//if there is a packet in the connection_q, then we need to 
			//save its last for calculation of the last of the new packet
			prev_pckt_last = umap[get<CONNECTION_DESIGNATOR>(*info_tup_iter)].connection_q.back().last;

			//cout << "prev pckt last: " << prev_pckt_last << " connection: " << get<CONNECTION_DESIGNATOR>(*info_tup_iter) 
				//<< " arrival time: " << get<PACKET>(*info_tup_iter).arrival_time << " current time: " << get<TIME>(current_event) << endl;
		}

		//if a new weight was given, update the weight of the connection
		if (get<UPDATE_WEIGHT>(*info_tup_iter))
		{
			umap[get<CONNECTION_DESIGNATOR>(*info_tup_iter)].weight = get<WEIGHT>(*info_tup_iter);
		}

		//otherwise, update the weight of the packet to be the same as the last weight given
		else
		{
			get<PACKET>(*info_tup_iter).weight = umap[get<CONNECTION_DESIGNATOR>(*info_tup_iter)].weight;
		}

		//push the packet to the connection queue


		umap[get<CONNECTION_DESIGNATOR>(*info_tup_iter)].connection_q.push(get<PACKET>(*info_tup_iter));
	}


	/*============================================
			take care of sorted packets list
	  ============================================*/
	update_last_and_add_packet_to_list(get<PACKET>(*info_tup_iter), prev_pckt_last);

	//cout << "packet at front of connection q:" << endl;
	//umap[get<CONNECTION_DESIGNATOR>(*info_tup_iter)].connection_q.front().print();
}


void handle_all_arrivals()
{
	list<tuple<packet, string, double, bool>>::iterator it;

	//sum_active_weights_at_arrival = calc_sum_active_weights();

	//handle all of the same_time arrivals
	for (it = same_time_buffer.begin(); it != same_time_buffer.end(); it = same_time_buffer.begin())
	{
		handle_arrival(it);
		same_time_buffer.pop_front();
	}

	get<SUM_WEIGHTS>(current_event) = calc_sum_active_weights();
}


double get_time_of_last(double last)
{
	//return p.arrival_time + p.sum_weights_at_arrival * (p.last - p.round_of_calculation_of_last);

	//return get<TIME>(current_event) + calc_sum_active_weights() * (p.last - get<ROUND>(current_event));

	return get<TIME>(last_event) + get<SUM_WEIGHTS>(last_event) * (last - get<ROUND>(last_event));
}


bool enqueue_packet_and_write_to_file(packet &p)
{
	if (p.queued == false)
	{
		if (wfq.empty())
		{
			p.time_of_transmission_start = max(p.arrival_time, last_departure_time);
		}
		else
		{
			p.time_of_transmission_start = max(p.arrival_time, wfq.back().length + wfq.back().time_of_transmission_start);
		}

		p.queued = true;

		wfq.push(p);
		//cout << "\ncurrent time: " << get<TIME>(current_event) << endl;
		p.write_to_file(p.time_of_transmission_start);

		return true;
	}
	/*
	cout << "\n=====================================================================================\pushing packet to wfq: " << endl;
	sorted_packets.front().print();
	cout << "\n=====================================================================================\n" << endl;
	*/

	return false;

}


void enqueue_and_pop_from_GPS()
{
	if (enqueue_packet_and_write_to_file(sorted_packets.front()))
	{
		//cout << "pushed packet to wfq at time: " << get<TIME>(current_event) << " and round: " << get<ROUND>(current_event) << endl;
		//wfq.back().print();
	}

	pop_from_GPS();
}



void pop_from_GPS()
{
	//cout << "pop packet from GPS at time: " << get<TIME>(current_event) << " and round: " << get<ROUND>(current_event) << endl;
	//sorted_packets.front().print();

	umap[sorted_packets.front().connection].connection_q.pop();

	sorted_packets.pop_front();

	get<SUM_WEIGHTS>(current_event) = calc_sum_active_weights();
}



void set_current_event()
{
	double min_last = 0;
	double arrival_round = 0;


	arrival_time = get<PACKET>(same_time_buffer.front()).arrival_time;
	arrival_round = calc_round(arrival_time);

	//cout << "arrival round = " << arrival_round << endl;

	if (sorted_packets.empty())
	{
		min_last = arrival_round + 1; //so min_last > arrival_round
	}
	else
	{
		min_last = sorted_packets.front().last;
	}

	if (wfq.empty())
	{
		departure_time = arrival_time + 1; //so departure_time > arrival_time
	}
	else
	{
		departure_time = wfq.front().time_of_transmission_start + wfq.front().length;
		//cout << "departure_round = calc_round = " << departure_round << endl;
	}


	if (departure_time < arrival_time)
	{
		//curren event is a deprture from WFQ
		get<TYPE>(current_event) = DEPARTURE;
	}

	else if (arrival_round < min_last)
	{
		//current event is an arrival
		get<TYPE>(current_event) = ARRIVAL;
		//current event sum weights will be updated at the function handle_all_arrivals.
		get<TIME>(current_event) = arrival_time;
		get<ROUND>(current_event) = arrival_round;
	}
	else
	{
		//curren event is a deprture from GPS
		get<TYPE>(current_event) = MIN_LAST;
		//current event sum weights will be updated at the function pop_from_GPS.
		get<TIME>(current_event) = get_time_of_last(min_last);
		get<ROUND>(current_event) = min_last;
	}

}


void dequeue_packet()
{
	list<packet>::iterator it_sorted;
	/*
	cout << "\nentered dequeue_packets" << endl;
	cout << "pop packet:" << endl;
	wfq.front().print();
	*/
	last_departure_time = departure_time;

	wfq.pop();
	/*
	if (wfq.empty() && !sorted_packets.empty())
	{
		cout << "wfq is empty. front of GPS:" << endl;
		sorted_packets.front().print();
	}*/
}
