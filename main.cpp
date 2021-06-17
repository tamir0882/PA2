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







tuple<packet, string, float, bool> get_line_info(string parameters_line);

float calc_round(int event_time);

float calc_sum_active_weights();

void update_last_and_add_packet_to_list(packet &p, float prev_pckt_last, float round_of_arrival);

void handle_arrival(list<tuple<packet, string, float, bool>>::iterator info_tup_iter, float sum_active_weights_at_arrival);

void handle_wfq();

void handle_all_arrivals();

float get_time_of_last(packet p);

bool enqueue_packet_and_write_to_file(packet &p);

void dequeue_packet();

void pop_from_GPS();

void enqueue_and_pop_from_GPS();

void set_current_event();


float round_of_last_event = 0;
float round_current = 0;

tuple<float, float> arrival_state;

float time_of_last_event = 0;
int departure_time = 0;
int last_departure_time = 0;
int arrival_time = 0;
float time_of_min_last = 0;
tuple<int, float, float>current_event = make_tuple(0, 0, 0);



tuple<int, int>event_tup;
queue<packet> wfq;
unordered_map <string, umap_val> umap;
list<packet> sorted_packets;
list<tuple<packet, string, float, bool>> same_time_buffer;







int main()
{
	
	string connection_designator, parameters_line; 
	packet pckt;
	umap_val u_val;

	float weight = DEFAULT_WEIGHT;
	float prev_pckt_last = 0;

	packet last_same_time_packet;

	tuple<packet, string, float, bool> info_tup;

	list<tuple<packet, string, float, bool>>::iterator it_same_time;

	tuple<int, packet> dep_tup;

	//ifstream in("my_in.txt");
	//auto cinbuf = cin.rdbuf(in.rdbuf()); //save and redirect

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


		handle_wfq();


		//now insert the info_tup that wasn't inserted yet to the buffer.
		if (  !(last_same_time_packet == get<PACKET>(info_tup))  )
		{
			same_time_buffer.emplace_front(info_tup);
			last_same_time_packet = get<PACKET>(info_tup);
		}
	}


	/*===========================================================================
	   here - we finished reading the arrivals from the file and now we need
	   to simply finish processing all the remaining packets in our database.
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











tuple<packet, string, float, bool> get_line_info(string parameters_line)
{
	stringstream stream(parameters_line);
	string str_current, ip_s, port_s, ip_d, port_d, connection_designator;
	int arrival_time = 0;
	int length = 0;
	packet pckt;
	float weight = DEFAULT_WEIGHT;
	bool update_weight = false;

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

	return make_tuple(pckt, connection_designator, weight, update_weight);
}


//the list is assumed to include all of the packets currently 
//sending messages via GPS except for the new packet arrived
//that we calculate the round for
float calc_round(int event_time)
{
	float sum_weights = 0;
	
	sum_weights = calc_sum_active_weights();

	//cout << "round of last event: " << round_of_last_event << endl;
	//cout << "time_of_last_event: " << time_of_last_event << endl;
	
	return (  round_of_last_event + ( (event_time - time_of_last_event) / sum_weights ));
}


float calc_sum_active_weights()
{
	float sum_weights = 0;
	list<packet>::iterator it;

	if (sorted_packets.empty())
	{
		sum_weights = 1; //round advances as fast as time
	}
	else
	{
		for (it = sorted_packets.begin(); it != sorted_packets.end(); it++)
		{
			if (*it == umap[(*it).connection].connection_q.front())
				sum_weights += it->weight;
		}
	}

	return sum_weights;
}


void handle_wfq()
{
	list<tuple<packet, string, float, bool>>::iterator it_info_tup;
	list<packet>::iterator it_sorted;

	int finish_time = 0;
	arrival_time = get<PACKET>(same_time_buffer.front()).arrival_time;
	
	while (!same_time_buffer.empty())
	{
		if (wfq.empty() && !sorted_packets.empty())
		{
			for (it_sorted = sorted_packets.begin(); it_sorted != sorted_packets.end(); it_sorted++)
			{
				//enqueue_packet will push the next packet to the queue if it wasn't pushed already, 
				//and returns a bool variable that states wheather the packet was pushed by the function now or not
				if (enqueue_packet_and_write_to_file(sorted_packets.front())) break; //will push only the next packet in line to the wfq
			}
		}

		if (DEPARTURE != get<TYPE>(current_event))
		{
			time_of_last_event = get<TIME>(current_event);
			round_of_last_event = get<ROUND>(current_event);
		}

		set_current_event();

		if (DEPARTURE == get<TYPE>(current_event))
		{
			dequeue_packet();
		}
		else if (MIN_LAST == get<TYPE>(current_event))
		{
			enqueue_and_pop_from_GPS();
		}
		else // ARRIVAL
		{
			handle_all_arrivals();
		}
	}
}



void update_last_and_add_packet_to_list(packet &p, float prev_pckt_last, float round_of_arrival)
{
	p.round_of_calculation_of_last = max(round_of_arrival, prev_pckt_last);
	p.last = p.round_of_calculation_of_last + p.length / p.weight;

	umap[p.connection].connection_q.back().last = p.last;
	umap[p.connection].connection_q.back().round_of_calculation_of_last = p.round_of_calculation_of_last;

	sorted_packets.emplace_front(p);
	sorted_packets.sort();
}


void handle_arrival(list<tuple<packet, string, float, bool>>::iterator info_tup_iter, float sum_active_weights_at_arrival)
{
	float prev_pckt_last = QUEUE_EMPTY;
	//the value QUEUE_EMPTY indicates that there were no packets 
	//in the connection_q at the time the new packet arrived
	

	get<PACKET>(*info_tup_iter).sum_weights_at_arrival = sum_active_weights_at_arrival;
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

	update_last_and_add_packet_to_list(get<PACKET>(*info_tup_iter), prev_pckt_last, get<PACKET>(*info_tup_iter).arrival_round);



	/*============================================
			take care of sorted packets list
	  ============================================*/
}


void handle_all_arrivals()
{
	list<tuple<packet, string, float, bool>>::iterator it;

	float sum_active_weights_at_arrival = 0;

	sum_active_weights_at_arrival = calc_sum_active_weights();

	//handle all of the same_time arrivals
	for (it = same_time_buffer.begin(); it != same_time_buffer.end(); it = same_time_buffer.begin())
	{
		handle_arrival(it, sum_active_weights_at_arrival);
		same_time_buffer.pop_front();
	}
}


float get_time_of_last(packet p)
{
	return p.arrival_time + p.sum_weights_at_arrival * (p.last - p.round_of_calculation_of_last);
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
	enqueue_packet_and_write_to_file(sorted_packets.front());

	pop_from_GPS();
}

void pop_from_GPS()
{
	string connection = sorted_packets.front().connection;
	if (!umap[connection].connection_q.empty()) 
	{
			umap[connection].connection_q.pop();
	}
	sorted_packets.pop_front();
}

void set_current_event()
{
	if (!wfq.empty())
		departure_time = wfq.front().time_of_transmission_start + wfq.front().length;
	else
		departure_time = arrival_time + 1; //so departure_time > arrival_time for calculation of minimum
	if (!sorted_packets.empty())
	{
		time_of_min_last = get_time_of_last(sorted_packets.front());
		//cout << "time_of_min_last: " << time_of_min_last << endl;
	}	
	else
		time_of_min_last = arrival_time + 1; //so time_of_min_last > arrival_time for calculation of minimum


	


	if (arrival_time < departure_time)
	{
		if (arrival_time < time_of_min_last)
		{
			//arrival time is minimum
			get<TYPE>(current_event) = ARRIVAL;
			get<TIME>(current_event) = arrival_time; 
			get<ROUND>(current_event) = calc_round(arrival_time);
		}
		else if (time_of_min_last < departure_time)
		{
			//min last is minimum
			get<TYPE>(current_event) = MIN_LAST;
			get<TIME>(current_event) = time_of_min_last;
			get<ROUND>(current_event) = sorted_packets.front().last;
		}
	}
	else
	{
		if (time_of_min_last < departure_time)
		{
			//min last is minimum
			get<TYPE>(current_event) = MIN_LAST;
			get<TIME>(current_event) = time_of_min_last;
			get<ROUND>(current_event) = sorted_packets.front().last;
		}
		else
		{
			//departure time is minimum
			get<TYPE>(current_event) = DEPARTURE;
		}
	}
}

void dequeue_packet()
{
	last_departure_time = departure_time;

	wfq.pop();
}
