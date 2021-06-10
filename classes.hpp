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

using namespace std;


class packet
{
public:
	int length;
	int arrival_time;
	float weight;

	//default constructor
	packet()
	{
		length = 0;
		arrival_time = 0;
		weight = DEFAULT_WEIGHT;
	}

	//parameterized constructor
	packet(int length, int arrival_time, float weight)
	{
		this->length = length;
		this->arrival_time = arrival_time;
		this->weight = weight;
	}
};


class umap_val
{
public:
	float weight;
	queue<packet> connection_q;

	//default constructor 
	umap_val()
	{
		weight = DEFAULT_WEIGHT;
	}

	//parameterized contructor
	umap_val(float weight, packet p)
	{
		this->weight = weight;
		connection_q.push(p);
	}
};


#endif