//
//  Vrml 97 library
//  Copyright (C) 1998 Chris Morley
//
#ifndef  _VRMLSFTIME_
#define  _VRMLSFTIME_

#include "VrmlField.h"


class VrmlSFTime : public VrmlSField {
public:

  VrmlSFTime(double value = 0.0);
  VrmlSFTime(const VrmlSFTime& );

  virtual ostream& print(ostream& os) const;

  virtual VrmlField *clone() const;

  virtual VrmlFieldType fieldType() const;
  virtual const VrmlSFTime* toSFTime() const;
  virtual VrmlSFTime* toSFTime();

  // Assignment.
  VrmlSFTime& operator=(const VrmlSFTime& rhs);
  VrmlSFTime& operator=(double rhs);

  double get(void) const	{ return d_value; }
  void set(double value)	{ d_value = value; }

private:
  double d_value;

};

#endif
