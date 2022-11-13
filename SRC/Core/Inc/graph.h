/*
 * graph.h
 *
 */

#ifndef GRAPH_H_
#define GRAPH_H_

#include <stdint.h>

class GRAPH {
	public:
		GRAPH(void)											{ }
		bool		isFull(void)							{ return full_buff; 					}
		uint16_t	dataSize(void)							{ return (full_buff)?size:data_index;	}
		void		reset(void)								{ data_index = 0; full_buff = false;	}
		bool		allocate(uint16_t size);
		void		freeData(void);
		void		put(int16_t t, uint16_t d);
		int16_t		temp(uint16_t index);
		uint16_t	disp(uint16_t index);
	private:
		uint16_t	indx(uint16_t i);
		uint16_t	size				= 0;				// The graph size
		int16_t		*h_temp				= 0;				// The temperature history data, allocated later
		uint16_t	*h_disp				= 0;				// The dispersion  history data, allocated later
		uint16_t	data_index			= 0;				// The index in the array to put new data
		bool		full_buff			= false;			// Whether the history data buffer is full
};

#endif
