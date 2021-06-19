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


enum event_type {
	ARRIVAL,
	MIN_LAST,
	DEPARTURE
};

enum current_event_params {
	TYPE,
	TIME,
	ROUND,
	SUM_WEIGHTS
};


#endif