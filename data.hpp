#ifndef DATA_HPP
#define DATA_HPP


#define DEFAULT_WEIGHT 1.0
#define NOT_CALCULATED -1

#define NO_PACKETS_WAITING -1

#define QUEUE_EMPTY -1

enum info_tuple_index {
	PACKET,
	CONNECTION_DESIGNATOR,
	WEIGHT,
	UPDATE_WEIGHT
};

enum arrival_state_index
{
	ROUND,
	WEIGHTS_SUM
};


#endif