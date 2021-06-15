#ifndef CLASSES_HPP
#define CLASSES_HPP


#include "data.hpp"
#include <string>
#include <iostream>
#include <tuple>
#include <queue>
#include <assert.h>
#include <algorithm>
#include <unordered_map>
#include <stdexcept>
#include <functional>
#include <iterator>
#include <fstream>
#include <iomanip>


using namespace std;


class packet
{
public:
	int length;
	int arrival_time;
	float arrival_round;
	int initial_connection_arrival_time;
	float weight;
	bool weight_given;
	string connection;
	float last;
	bool queued;
	float sum_weights_at_arrival;
	int time_of_transmission_start;

	//default constructor
	packet()
	{
		length = 0;
		arrival_time = 0;
		initial_connection_arrival_time = 0;
		weight = DEFAULT_WEIGHT;
		connection = "DEFAULT";
		last = NOT_CALCULATED;
		weight_given = false;
		queued = false;
		sum_weights_at_arrival = DEFAULT_WEIGHT;
		arrival_round = 0;
		time_of_transmission_start = 0;
		
	}

	//parameterized constructor
	packet(int length, int arrival_time, float weight, string connection, bool weight_given)
	{
		this->length = length;
		this->arrival_time = arrival_time;
		this->initial_connection_arrival_time = arrival_time;
		this->weight = weight;
		this->connection = connection;
		this->weight_given = weight_given;
		this->last = 0;
		queued = false;
		sum_weights_at_arrival = DEFAULT_WEIGHT;
		arrival_round = 0;
		time_of_transmission_start = arrival_time;
	}

	//lt operator for packets from different conections
	bool operator< (packet const& pckt)
	{
		if (last < pckt.last)
		{
			return true;
		}
		else if (last == pckt.last)
		{
			return initial_connection_arrival_time < pckt.initial_connection_arrival_time;
		}
		else
			return false;
	}


	//gt operator for packets from different conections
	bool operator>(packet const& pckt)
	{
		if (last > pckt.last)
		{
			return true;
		}
		else if (last == pckt.last)
		{
			return initial_connection_arrival_time > pckt.initial_connection_arrival_time;
		}
		else
			return false;
	}

	bool operator==(packet const& pckt)
	{
		return (arrival_time == pckt.arrival_time && connection == pckt.connection);
	}

	void print()
	{
		cout << "\n=============" << endl;
		cout << "QUEUED: ";
		if (queued) cout << "true" << endl;
		else cout << "false" << endl;
		cout << "Length: " << length << endl;
		cout << "Arrival Time: " << arrival_time << endl;
		cout << "Arrival Round: " << arrival_round << endl;
		cout << "Weight: " << weight << endl;
		cout << "Connection: " << connection << endl;
		cout << "Last: " << last << endl;
		cout << "initial arrival of connection: "<< initial_connection_arrival_time << endl;
		cout << "=============\n" << endl;
	}

	//method to write to output file using redirection
	void write_to_file(int send_time)
	{
		cout << send_time << ": " << arrival_time << " " << connection << " " << length;
		cout << std::setprecision(2) << std::fixed;
		if (weight_given)
		{
			cout << " " << weight;
		}
		cout << " last: " << last << endl;
		//cout << endl;
	}

};


class umap_val
{
public:
	int initial_arrival_time;
	float weight;
	queue<packet> connection_q;

	//default constructor 
	umap_val()
	{
		initial_arrival_time = 0;
		weight = DEFAULT_WEIGHT;
	}

	//parameterized contructor
	umap_val(float weight, packet p)
	{
		this->initial_arrival_time = p.initial_connection_arrival_time;
		this->weight = weight;
		connection_q.push(p);
	}
};


#endif