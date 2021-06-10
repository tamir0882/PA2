#include <sstream>
#include <list>
#include <math.h>
#include <iostream>
#include <unordered_map>
#include <queue>

using namespace std;

#include "classes.hpp"
#include "data.hpp"


enum {
	PACKET,
	CONNECTION_DESIGNATOR,
	WEIGHT,
	UPDATE_WEIGHT
}info_tuple_index;


tuple<packet, string, float, bool> get_line_info(string parameters_line);


int main()
{
	
	string connection_designator, parameters_line; 
	packet pckt;
	umap_val u_val;
	unordered_map <string, umap_val> umap;
	int current_time = 0, last_arrival_time = 0, last_departure_time = 0, next_arrival_time = 0;

	tuple<packet, string, float, bool> info_tup;

	while (getline(cin, parameters_line))
	{
		info_tup = get_line_info(parameters_line);

		//check if the connection is mapped,
		//if it is mapped - push the packet to the connection queue,
		//it not - insert the new connection to the umap.
		if (umap.find(get<CONNECTION_DESIGNATOR>(info_tup)) == umap.end())
		{
			//if we reached here the connection was not mapped yet, so map it
			u_val = umap_val(get<WEIGHT>(info_tup), get<PACKET>(info_tup));
			umap[get<CONNECTION_DESIGNATOR>(info_tup)] = u_val;
		}
		else
		{
			//if we reached here the connection was already mapped
			
			pckt = get<PACKET>(info_tup);

			//if a new weight was given, update the weight of the connection
			if (get<UPDATE_WEIGHT>(info_tup))
			{
				umap[get<CONNECTION_DESIGNATOR>(info_tup)].weight = get<WEIGHT>(info_tup);
			}

			//otherwise, update the weight of the packet to be the same as the last weight given
			else
			{
				pckt.weight = umap[get<CONNECTION_DESIGNATOR>(info_tup)].weight;
			}

			//push the packet to the connection queue
			umap[get<CONNECTION_DESIGNATOR>(info_tup)].connection_q.push(get<PACKET>(info_tup));
		}


		//check if the next arrival time is prior to the next departure time 
		//check if the next arrival is now
		
			
		//call GPS alg for every arrival(ONLINE)\departure(GPS)
		



	}
	

	//tuple<packet, string, float, bool> t = get_line_info("0 70.246.64.70 14770 4.71.70.4 11970 70 1.1");

	//cout << get<0>(t).arrival_time << " " << get<0>(t).length << " " << get<1>(t) << " " << get<2>(t) << " " << get<3>(t);

}



tuple<packet, string, float, bool> get_line_info(string parameters_line)
{
	stringstream stream(parameters_line);
	string str_current, ip_s, port_s, ip_d, port_d, connection_designator;
	int arrival_time = 0;
	int length = 0;
	packet p;
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

	// if true - it means there was a weight to read, otherwise it means 
	if (stream)
	{
		update_weight = true;
		weight = stof(str_current);
	}

	//create packet
	p = packet(length, arrival_time, weight);

	return make_tuple(p, connection_designator, weight, update_weight);
}


