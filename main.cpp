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

tuple<float, float> calc_arrival_round(int event_time);

float get_round(int event_time, bool exclude_first);

void add_packet_to_list(packet *p, float prev_pckt_last, float round_of_arrival);

void handle_arrival(list<tuple<packet, string, float, bool>>::iterator info_tup_iter);

void handle_wfq(int arrival_time);

void handle_all_arrivals(int arrival_time);

float get_time_delta_of_departure(packet p);

void enqueue_packet(list<packet>::iterator it);

void remove_from_GPS();

void remove_from_GPS_after_write_to_file(int finish_time);


float round_of_last_event = 0;

tuple<float, float> arrival_state;

float time_of_last_event = 0;
int event_time = 0;

int last_time_of_transmission_finish = 0;

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

	int arrival_time = 0, departure_time = 0, next_arrival_time = 0;
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
			arrival_time = get<PACKET>(info_tup).arrival_time;

			same_time_buffer.emplace_front(info_tup);

			last_same_time_packet = get<PACKET>(info_tup);

			if (cin) continue;			
			
		}
		
		else
		{
			arrival_time = get<PACKET>(same_time_buffer.front()).arrival_time;

			if (get<PACKET>(info_tup).arrival_time == arrival_time)
			{
				same_time_buffer.emplace_back(info_tup);

				//cout << "get<PACKET>(info_tup).arrival_time == arrival_time" << endl;
				last_same_time_packet = get<PACKET>(info_tup);

				if (cin) continue;				
			}
		}

		/*
		cout << "\n========================================\nsame_time_buffer contains these packets:\n" << endl;
 		for (it_same_time = same_time_buffer.begin(); it_same_time != same_time_buffer.end(); it_same_time++)
		{
			get<PACKET>(*it_same_time).print();
		}
		cout << "\n========================================\n" << endl;
		*/

		handle_wfq(arrival_time);


		//now insert the info_tup that wasn't inserted yet to the buffer.
		if (!(last_same_time_packet == get<PACKET>(info_tup)))
		{
			same_time_buffer.emplace_front(info_tup);
			handle_wfq(get<PACKET>(info_tup).arrival_time);
		}
	}


	/*===========================================================================
	   here - we finished reading the arrivals from the file and now we need
	   to simply finish processing all the remaining packets in our database.
	===========================================================================*/


	int finish_time = 0;
	
	if (!same_time_buffer.empty())
	{
		handle_wfq(get<PACKET>(same_time_buffer.front()).arrival_time);
	}
	
	while (!sorted_packets.empty())
	{
		if (sorted_packets.front().queued)
		{
			remove_from_GPS();
		}
		else
		{
			/*
			cout << "\n\n\n\nmain at while (!sorted_packets.empty())" << endl;
			cout << "===========================================================================================================pushing packet" << endl;
			sorted_packets.front().print();
			*/

			enqueue_packet(sorted_packets.begin());
			remove_from_GPS();
		}
	}

	while (!wfq.empty())
	{
		wfq.front().write_to_file(wfq.front().time_of_transmission_start);
		/*
		cout << "\n\n\n\nmain at while (!wfq.empty())" << endl;
		cout << "\n=====================================================================================popping packet from wfq: " << endl;
		wfq.front().print();
		cout << "\n=====================================================================================\n" << endl;
		*/
		
		finish_time = wfq.front().time_of_transmission_start + wfq.front().length;
		last_time_of_transmission_finish = finish_time;

		wfq.pop();
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



//save in memory list of sorted packets according to their last
//every arrival: 
//calculate the round and last of the new packet
//insert the packet to the list and sort it











//the list is assumed to include all of the packets currently 
//sending messages via GPS except for the new packet arrived
//that we calculate the round for
tuple<float,float> calc_arrival_round(int arrival_time)
{
	float sum_weights = 0;
	list<packet>::iterator it;

	if (sorted_packets.empty())
	{
		sum_weights = 1;
	}
	else
	{
		for (it = sorted_packets.begin(); it != sorted_packets.end(); it++)
		{
			if(*it == umap[(*it).connection].connection_q.front())
				sum_weights += it->weight;
		}
	}
	
	return make_tuple(  round_of_last_event + ( (arrival_time - time_of_last_event) / sum_weights ), sum_weights  );
}


float get_round(int event_time, bool exclude_first)
{
	float sum_weights = 0;
	list<packet>::iterator it;

	if (sorted_packets.empty())
	{
		sum_weights = 1;
	}
	else
	{
		for (it = sorted_packets.begin(); it != sorted_packets.end(); it++)
		{
			sum_weights += it->weight;
		}
		if (exclude_first)
		{
			sum_weights -= sorted_packets.front().weight;
			if (sum_weights <= 0)
				sum_weights = 1;
		}
	}
	return round_of_last_event + ((event_time - time_of_last_event) / sum_weights);
}


void handle_wfq(int arrival_time)
{
	list<tuple<packet, string, float, bool>>::iterator it_info_tup;
	list<packet>::iterator it_sorted;

	int finish_time = 0;
	

	while (!wfq.empty())
	{
		finish_time = wfq.front().time_of_transmission_start + wfq.front().length;

		if (finish_time > arrival_time) break;

		wfq.front().write_to_file(wfq.front().time_of_transmission_start);

		remove_from_GPS_after_write_to_file(finish_time);

		time_of_last_event = finish_time;

		/*
		cout << "\n=====================================================================================popping packet from wfq: " << endl;
		wfq.front().print();
		cout << "\n=====================================================================================\n" << endl;
		*/

		last_time_of_transmission_finish = finish_time;

		//pop packet out of wfq
		wfq.pop();
		/*
		cout << "inside check of !wfq.empty():\n"
			<< "	time of last event:		" << time_of_last_event << endl
			<< "	round of last event:	" << round_of_last_event << endl;
			*/
	}


	//calculate round of the arrival
	//cout << "calc_arrival_round: time of last event is	" << time_of_last_event << endl;
	//cout << "arrival time:	" << arrival_time << endl;

	arrival_state = calc_arrival_round(arrival_time);

	//cout << "round_arrival: " << get<ROUND>(arrival_state) << endl;
	//cout << "sum weights: " << get<WEIGHTS_SUM>(arrival_state) << endl;

	if (wfq.empty())
	{
		if (sorted_packets.empty())
		{
			handle_all_arrivals(arrival_time);

			//cout << "\n\n\n\ninside handle_wfq at if (wfq.empty())->if (sorted_packets.empty())" << endl;
			for (it_sorted = sorted_packets.begin(); it_sorted != sorted_packets.end(); it_sorted++)
			{
				if (*it_sorted == umap[(*it_sorted).connection].connection_q.front())
				{
					/*
					cout << "inside wfq.empty() at sorted_packets.empty()" << endl;
					cout << "===========================================================================================================pushing packet" << endl;
					it_sorted->print();
					*/
					enqueue_packet(it_sorted);
					//cout << "\ncalculated transmission start:	sorted_packets.empty()\n" << wfq.front().time_of_transmission_start << endl;
				}

			}

		}

		else if (sorted_packets.begin()->last > get<ROUND>(arrival_state))
		{
			handle_all_arrivals(arrival_time);

			//cout << "\n\n\n\ninside handle_wfq at if (wfq.empty())->else if (sorted_packets.begin()->last > get<ROUND>(arrival_state))" << endl;
			for (it_sorted = sorted_packets.begin(); it_sorted != sorted_packets.end(); it_sorted++)
			{
				if (*it_sorted == umap[(*it_sorted).connection].connection_q.front())
				{
					/*
					cout << "inside wfq.empty() at sorted_packets.begin()->last > get<ROUND>(arrival_state)" << endl;
					cout << "===========================================================================================================pushing packet" << endl;
					it_sorted->print();
					*/

					enqueue_packet(it_sorted);
					
					//cout << "\ncalculated transmission start:	sorted_packets.empty()\n" << wfq.front().time_of_transmission_start << endl;
				}

			}
		}

		else
		{
			//cout << "\n\n\n\ninside handle_wfq at if (wfq.empty())->else" << endl;
			for (it_sorted = sorted_packets.begin(); it_sorted != sorted_packets.end(); it_sorted++)
			{
				if (*it_sorted == umap[(*it_sorted).connection].connection_q.front())
				{
					/*
					cout << "inside wfq.empty() at else" << endl;
					cout << "===========================================================================================================pushing packet" << endl;
					it_sorted->print();
					*/

					enqueue_packet(it_sorted);
					//cout << "\ncalculated transmission start:	sorted_packets.empty()\n" << wfq.front().time_of_transmission_start << endl;
				}
			}

			handle_all_arrivals(arrival_time);
		}

	}

	else
	{
		if (sorted_packets.empty())
		{
			//here - there are no packets being processed on GPS.
			handle_all_arrivals(arrival_time);
		}

		else if (sorted_packets.front().last > get<ROUND>(arrival_state))
		{
			//here - the arriving packet is a small packet such that the wfq is not empty
			//but the time of transmission finish is bigger than the arrival time
			handle_all_arrivals(arrival_time);
		}

		else
		{
			//here - the arrival is after or with the first departure from the GPS.
			while (sorted_packets.front().last <= get<ROUND>(arrival_state))
			{
				time_of_last_event += get_time_delta_of_departure(sorted_packets.front());
				round_of_last_event = sorted_packets.front().last;
				if (sorted_packets.front().queued)
				{
					remove_from_GPS();
				}

				else
				{
					/*
					cout << "\n\n\n\ninside handle_wfq at else->while (sorted_packets.front().last <= get<ROUND>(arrival_state))->else" << endl;
					cout << "===========================================================================================================pushing packet" << endl;
					sorted_packets.front().print();
					*/
					for (it_sorted = sorted_packets.begin(); it_sorted != sorted_packets.end();)
					{
						if (*it_sorted == umap[(*it_sorted).connection].connection_q.front())
						{

							enqueue_packet(it_sorted);
							remove_from_GPS();
							//cout << "\ncalculated transmission start:	sorted_packets.empty()\n" << wfq.front().time_of_transmission_start << endl;
							it_sorted = sorted_packets.begin();
						}
						else it_sorted = sorted_packets.begin();
					}						
				}

				if (sorted_packets.empty()) break;
			}

			handle_all_arrivals(arrival_time);
		}
	}
}



void add_packet_to_list(packet *p, float prev_pckt_last, float round_of_arrival)
{
	
	(*p).last = max(round_of_arrival, prev_pckt_last) + (*p).length / (*p).weight;

	//cout << "\n\n\n\n========================================================================inserting packet to the list" << endl;
	//(*p).print();

	sorted_packets.emplace_front(*p);
	sorted_packets.sort();

}


void handle_arrival(list<tuple<packet, string, float, bool>>::iterator info_tup_iter)
{
	float prev_pckt_last = QUEUE_EMPTY;
	//the value QUEUE_EMPTY indicates that there were no packets 
	//in the connection_q at the time the new packet arrived
	

	get<PACKET>(*info_tup_iter).sum_weights_at_arrival = get<WEIGHTS_SUM>(arrival_state);
	get<PACKET>(*info_tup_iter).arrival_round = get<ROUND>(arrival_state);

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
		/*
		cout << "connection " << get<CONNECTION_DESIGNATOR>(*info_tup_iter) << " is in the umap! initial arrival time: " 
			<< umap[get<CONNECTION_DESIGNATOR>(*info_tup_iter)].initial_arrival_time << endl;
			*/

		get<PACKET>(*info_tup_iter).initial_connection_arrival_time = umap[get<CONNECTION_DESIGNATOR>(*info_tup_iter)].initial_arrival_time;

		//if we reached here the connection was already mapped
		if (!umap[get<CONNECTION_DESIGNATOR>(*info_tup_iter)].connection_q.empty())
		{
			//if there is a packet in the connection_q, then we need to 
			//save its last for calculation of the last of the new packet
			prev_pckt_last = umap[get<CONNECTION_DESIGNATOR>(*info_tup_iter)].connection_q.back().last;
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

	add_packet_to_list(&get<PACKET>(*info_tup_iter), prev_pckt_last, get<ROUND>(arrival_state));
}


void handle_all_arrivals(int arrival_time)
{
	list<tuple<packet, string, float, bool>>::iterator it;

	//handle all of the same_time arrivals
	for (it = same_time_buffer.begin(); it != same_time_buffer.end(); it = same_time_buffer.begin())
	{
		handle_arrival(it);
		
		/*
		cout << endl << "handled packet:" << endl;
		get<PACKET>(*it).print();
		cout << endl << endl;
		*/

		same_time_buffer.pop_front();
	}
	
	round_of_last_event = get<ROUND>(arrival_state);
	time_of_last_event = arrival_time;
}


float get_time_delta_of_departure(packet p)
{
	//cout << "p.arrival_round:	" << p.arrival_round << endl;
	//cout << "p.last:	" << p.last << endl;
	//cout << "p.sum_weights_at_arrival:	" << p.sum_weights_at_arrival << endl;

	return p.sum_weights_at_arrival * (p.last - p.arrival_round);
}

void enqueue_packet(list<packet>::iterator it)
{
	if (it->queued == false)
	{
		if (wfq.empty())
		{
			(*it).time_of_transmission_start = max((*it).arrival_time, last_time_of_transmission_finish);
		}
		else
		{
			(*it).time_of_transmission_start = max((*it).arrival_time, wfq.back().length+wfq.back().time_of_transmission_start);
		}

		wfq.push(*it);
		(*it).queued = true;
		
	}
	/*
	cout << "\n=====================================================================================\pushing packet to wfq: " << endl;
	sorted_packets.front().print();
	cout << "\n=====================================================================================\n" << endl;
	*/

}


void remove_from_GPS_after_write_to_file(int finish_time)
{
	round_of_last_event = get_round(finish_time, true);

	if (sorted_packets.front() == wfq.front())
	{
		//cout << "===================sorted_packets.front() == wfq.front()=================" << endl;
		if (round_of_last_event >= sorted_packets.front().last)
		{
			//cout << "===================round_of_last_event >= sorted_packets.front().last================== " << endl;
			remove_from_GPS();
		}
		else round_of_last_event = get_round(finish_time, false);

	}
	//else cout << "!!!!!!!!!!!!!!		PACKETS NOT EQUAL	!!!!!!!!!!!!!!!!" << endl;
}

void remove_from_GPS()
{
	string connection = sorted_packets.front().connection;

	if(!umap[connection].connection_q.empty()) umap[connection].connection_q.pop();
	/*
	cout << "\n=====================================================================================popping packet from list: " << endl;
	sorted_packets.front().print();
	cout << "\n=====================================================================================\n" << endl;
	*/
	sorted_packets.pop_front();
}