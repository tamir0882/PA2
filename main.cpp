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







tuple<packet, string, double, bool> get_line_info(string parameters_line);

double calc_round(int event_time);

double calc_sum_active_weights();

void update_last_and_add_packet_to_list(packet &p, double prev_pckt_last, double round_of_arrival);

void handle_arrival(list<tuple<packet, string, double, bool>>::iterator info_tup_iter);

void handle_wfq();

void handle_all_arrivals();

double get_time_of_last(double last);

bool enqueue_packet_and_write_to_file(packet &p);

void dequeue_packet();

void pop_from_GPS();

void enqueue_and_pop_from_GPS();

void set_current_event();


double round_current = 0;
double arrival_round = 0;


tuple<double, double> arrival_state;


int departure_time = 0;
int last_departure_time = 0;
int arrival_time = 0;
double time_of_min_last = 0;
tuple<int, double, double, double>current_event = make_tuple(ARRIVAL, 0, 0, 1);
tuple<int, double, double, double>last_event = make_tuple(ARRIVAL, 0, 0, 1);


tuple<int, int>event_tup;
queue<packet> wfq;
unordered_map <string, umap_val> umap;
list<packet> sorted_packets;
list<tuple<packet, string, double, bool>> same_time_buffer;







int main()
{
	
	string connection_designator, parameters_line; 
	packet pckt;
	umap_val u_val;

	double weight = DEFAULT_WEIGHT;
	double prev_pckt_last = 0;

	packet last_same_time_packet;

	tuple<packet, string, double, bool> info_tup;

	list<tuple<packet, string, double, bool>>::iterator it_same_time;

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
		if (last_same_time_packet != get<PACKET>(info_tup))
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











tuple<packet, string, double, bool> get_line_info(string parameters_line)
{
	stringstream stream(parameters_line);
	string str_current, ip_s, port_s, ip_d, port_d, connection_designator;
	int arrival_time = 0;
	int length = 0;
	packet pckt;
	double weight = DEFAULT_WEIGHT;
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

	/*
	if (update_weight)
	{
		cout << "received weight: " << pckt.weight << endl;
	}
	*/

	return make_tuple(pckt, connection_designator, weight, update_weight);
}


//the list is assumed to include all of the packets currently 
//sending messages via GPS except for the new packet arrived
//that we calculate the round for
double calc_round(int event_time)
{
	double sum_weights = 0;
	
	sum_weights = calc_sum_active_weights();

	//cout << "round of last event: " << get<ROUND>(last_event) << endl;
	//cout << "get<TIME>(last_event): " << get<TIME>(last_event) << endl;

	/*
	cout << "\nget<SUM_WEIGHTS>(last_event) = " << get<SUM_WEIGHTS>(last_event) << endl;
	cout << "get<SUM_WEIGHTS>(current_event) =  " << get<SUM_WEIGHTS>(current_event) << endl;
	cout << "calc_sum_active_weights() = " << calc_sum_active_weights() << endl;
	cout << "type of last event: " << get<TYPE>(current_event) << " get<TIME>(last_event) = " << get<TIME>(last_event) << " event time = " << event_time << endl;
	cout << "x = " << (event_time - get<TIME>(last_event)) << " last round = " << get<ROUND>(last_event) << endl;
	*/



	return (  get<ROUND>(last_event) + ( (event_time - get<TIME>(last_event)) / calc_sum_active_weights()));
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

	arrival_time = get<PACKET>(same_time_buffer.front()).arrival_time;
	
	while (!same_time_buffer.empty())
	{
		if (wfq.empty() && !sorted_packets.empty())
		{
			for (it_sorted = sorted_packets.begin(); it_sorted != sorted_packets.end(); it_sorted++)
			{
				//enqueue_packet will push the next packet to the queue if it wasn't pushed already, 
				//and returns a bool variable that states wheather the packet was pushed by the function now or not
				if (enqueue_packet_and_write_to_file(sorted_packets.front()))
				{
					//cout << "wfq empty, pushed packet to wfq: " << endl;
					//wfq.back().print();
					break; //will push only the next packet in line to the wfq
				}
			}
		}

		/*
		cout << "\nchecking if need to update last event parameters:" << endl;
			
		cout << "last event was: ";
		if (get<TYPE>(last_event) == ARRIVAL)
		{
			cout << "ARRIVAL ";
		}
		else if(get<TYPE>(last_event) == MIN_LAST)
		{
			cout << "MIN_LAST ";
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
		cout << "at time = " << get<TIME>(current_event) << ", round = " << get<ROUND>(current_event) << ", SUM_WEIGHTS = " << get<SUM_WEIGHTS>(current_event) << endl;
			

		if (get<TIME>(last_event) <= get<TIME>(current_event))
		{
			cout << "updating!" << endl;
			get<TYPE>(last_event) = get<TYPE>(current_event);
			get<TIME>(last_event) = get<TIME>(current_event);
			get<ROUND>(last_event) = get<ROUND>(current_event);
			get<SUM_WEIGHTS>(last_event) = get<SUM_WEIGHTS>(current_event);
		}
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



void update_last_and_add_packet_to_list(packet &p, double prev_pckt_last, double round_of_arrival)
{
	p.round_of_calculation_of_last = max(round_of_arrival, prev_pckt_last);
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
	

	get<PACKET>(*info_tup_iter).sum_weights_at_arrival = get<SUM_WEIGHTS>(current_event);
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
	update_last_and_add_packet_to_list(get<PACKET>(*info_tup_iter), prev_pckt_last, get<PACKET>(*info_tup_iter).arrival_round);

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
}

void set_current_event()
{
	double departure_round = 0;
	double min_last = 0;


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
		departure_round = min_last + 1;

	}
	else
	{
		departure_time = wfq.front().time_of_transmission_start + wfq.front().length;
		departure_round = calc_round(departure_time);
		//cout << "departure_round = calc_round = " << departure_round << endl;
	}
/*
	if (!wfq.empty())
	{
		departure_time = wfq.front().time_of_transmission_start + wfq.front().length;
	}
	else
	{
		departure_time = arrival_time + 1; //so departure_time > arrival_time for calculation of minimum
	}
	if (!sorted_packets.empty())
	{
		time_of_min_last = get_time_of_last(sorted_packets.front());
		//cout << "time_of_min_last: " << time_of_min_last << endl;
	}
	else
	{
		time_of_min_last = arrival_time + 1; //so time_of_min_last > arrival_time for calculation of minimum
	}

	if (arrival_time < departure_time)
	{
		if (arrival_time < time_of_min_last)
		{
			//arrival time is minimum
			get<TYPE>(current_event) = ARRIVAL;
			get<SUM_WEIGHTS>(current_event) = calc_sum_active_weights();
			get<TIME>(current_event) = arrival_time;
			get<ROUND>(current_event) = calc_round(arrival_time);
		}
		else if (time_of_min_last < departure_time)
		{
			//min last is minimum
			get<TYPE>(current_event) = MIN_LAST;
			get<SUM_WEIGHTS>(current_event) = calc_sum_active_weights();
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
			get<SUM_WEIGHTS>(current_event) = calc_sum_active_weights();
			get<TIME>(current_event) = time_of_min_last;
			get<ROUND>(current_event) = sorted_packets.front().last;
		}
		else
		{
			//departure time is minimum
			get<TYPE>(current_event) = DEPARTURE;
		}
	}
*/


	if (arrival_time < departure_time && arrival_round < min_last)
	{
		//arrival time is minimum
		get<TYPE>(current_event) = ARRIVAL;
		//current event sum weights will be updated at the function handle_all_arrivals.
		get<TIME>(current_event) = arrival_time;
		get<ROUND>(current_event) = arrival_round;
	}
	else
	{
		if (min_last < departure_round)
		{
			//min last is minimum
			get<TYPE>(current_event) = MIN_LAST;
			get<SUM_WEIGHTS>(current_event) = calc_sum_active_weights();
			get<TIME>(current_event) = get_time_of_last(min_last);
			get<ROUND>(current_event) = min_last;
		}
		else
		{
			//departure is minimum
			get<TYPE>(current_event) = DEPARTURE;
		}
	}


}

void dequeue_packet()
{
	last_departure_time = departure_time;
	//cout << "pop packet from wfq:" << endl;
	//wfq.front().print();
	wfq.pop();
}
