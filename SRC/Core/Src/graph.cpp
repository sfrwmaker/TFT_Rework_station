/*
 * graph.cpp
 *
 */

#include <stdlib.h>
#include "graph.h"
#include "tools.h"

bool GRAPH::allocate(uint16_t size) {
	data_index	= 0;
	full_buff	= false;
	if (this->size > 0 && this->size < size) {
		free(h_temp);
		free(h_disp);
		this->size = 0;
	}
	if (this->size == 0) {
		h_temp = (int16_t *)malloc(size * sizeof(int16_t));
		if (h_temp) {
			h_disp = (uint16_t *)malloc(size * sizeof(uint16_t));
			if (!h_disp) {
				free(h_temp);
				h_temp = 0;
				h_disp = 0;
				return false;
			}
			this->size = size;
		}
	}
	return true;
}

void GRAPH::freeData(void) {
	if (size > 0) {
		free(h_temp);
		free(h_disp);
	}
}

void GRAPH::put(int16_t t, uint16_t d) {
	if (size == 0) return;
	uint8_t	i 	= data_index;
	t 	= constrain(t, -500, 500);										// Limit graph value
	d	= constrain(d,    0, 999);

	h_temp[i]	= t;
	h_disp[i]	= d;
	if (++i >= size) {
		i = 0;
		full_buff = true;
	}
	data_index	= i;
}

int16_t	GRAPH::temp(uint16_t index) {
	if (size == 0) return 0;
	uint16_t i = indx(index);
	return h_temp[i];
}

uint16_t GRAPH::disp(uint16_t index) {
	if (size == 0) return 0;
	uint16_t i = indx(index);
	return h_disp[i];
}

uint16_t GRAPH::indx(uint16_t i) {
	uint16_t zero = (full_buff)?data_index+1:0;
	i += zero;
	if (i >= size) i -= size;
	return i;
}
