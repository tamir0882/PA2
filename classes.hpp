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
	double arrival_round;
	int initial_connection_arrival_time;
	double weight;
	bool weight_given;
	string connection;
	double last;
	bool queued;
	int time_of_transmission_start;
	double round_of_calculation_of_last;

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
		arrival_round = 0;
		time_of_transmission_start = 0;
		round_of_calculation_of_last = 0;
		
	}

	//parameterized constructor
	packet(int length, int arrival_time, double weight, string connection, bool weight_given)
	{
		this->length = length;
		this->arrival_time = arrival_time;
		this->initial_connection_arrival_time = arrival_time;
		this->weight = weight;
		this->connection = connection;
		this->weight_given = weight_given;
		this->last = 0;
		this->queued = false;
		this->arrival_round = 0;
		this->round_of_calculation_of_last = 0;
		this->time_of_transmission_start = arrival_time;
	}

	//lt operator for packets from different conections
	bool operator< (packet const& pckt)
	{
		if (this->last < pckt.last)
		{
			return true;
		}
		else if (this->last == pckt.last)
		{
			return this->initial_connection_arrival_time < pckt.initial_connection_arrival_time;
		}
		else
			return false;
	}


	//gt operator for packets from different conections
	bool operator>(packet const& pckt)
	{
		if (this->last > pckt.last)
		{
			return true;
		}
		else if (this->last == pckt.last)
		{
			return this->initial_connection_arrival_time > pckt.initial_connection_arrival_time;
		}
		else
			return false;
	}

	//eq operator
	bool operator==(packet const& pckt)
	{
		return (this->arrival_time == pckt.arrival_time && this->connection == pckt.connection);
	}

	//ne operatopr
	bool operator!=(packet const& pckt)
	{
		return (!(*this == pckt));
	}

	//print method - for debugging
	void print()
	{
		cout << "\n=============" << endl;
		cout << "QUEUED: ";
		if (this->queued) cout << "true" << endl;
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
		//cout << " last: " << last << " arrival round: " << arrival_round << " last_round_calculation: " << round_of_calculation_of_last << " weight: " << weight << endl;
		cout << endl;
	}

};


class umap_val
{
public:
	int initial_arrival_time;
	double weight;
	queue<packet> connection_q;

	//default constructor 
	umap_val()
	{
		this->initial_arrival_time = 0;
		this->weight = DEFAULT_WEIGHT;
	}

	//parameterized contructor
	umap_val(double weight, packet p)
	{
		this->initial_arrival_time = p.initial_connection_arrival_time;
		this->weight = weight;
		this->connection_q.push(p);
	}
};


#endif