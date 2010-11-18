/*
 * Path.h
 *
 * A pair of values representing a path through a particular component of the
 * network. The first value is the input port, and the second is the output
 * port.
 *
 *  Created on: 16 Nov 2010
 *      Author: db434
 */

#ifndef PATH_H_
#define PATH_H_

#include "../../Typedefs.h"
#include "../../Datatype/AddressedWord.h"

class Path {

public:

  ChannelIndex source() const;
  ChannelIndex destination() const;
  AddressedWord data() const;

  Path& operator= (const Path& other);

public:

  Path(const Path& other);
  Path(ChannelIndex source, ChannelIndex destination, const AddressedWord& data);
  virtual ~Path();

private:

  ChannelIndex source_, destination_;
  AddressedWord data_;

};

#endif /* PATH_H_ */
