#ifndef USER_INC_ITINERARY_H_
#define USER_INC_ITINERARY_H_
#include "protocol.h"



int itinerary_store(MSG_ITINERARY_REQ* itinerary_msg);

int itinerary_get(MSG_ITINERARY_REQ* itinerary_msg);



#define ITINERARYFILE_NAME L"C:\\itinerary"




#endif /* USER_INC_ITINERARY_H_ */

