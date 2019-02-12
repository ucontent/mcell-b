/*
 * release_molecules.h
 *
 *  Created on: Jan 30, 2019
 *      Author: adam
 */

#ifndef SRC4_RELEASE_EVENT_H_
#define SRC4_RELEASE_EVENT_H_

#include "base_event.h"

namespace mcell {

class release_event_t: public base_event_t {

	virtual void step();
};

} // namespace mcell


#endif /* SRC4_RELEASE_EVENT_H_ */
