//
//  Vrml 97 library
//  Copyright (C) 1998 Chris Morley
//
//  %W% %G%
//  VrmlNodeGeometry.cpp

#include "VrmlNodeGeometry.h"
#include "VrmlNodeType.h"

// Define the fields of all built in geometry nodes
VrmlNodeType *VrmlNodeGeometry::defineType(VrmlNodeType *t)
{
  return VrmlNode::defineType(t);
}

VrmlNodeGeometry::VrmlNodeGeometry(VrmlScene *s) :
  VrmlNode(s),
  d_viewerObject(0) 
{
}

VrmlNodeGeometry::~VrmlNodeGeometry()
{
  /* Need access to viewer to delete viewerObject...*/
}


VrmlNodeGeometry* VrmlNodeGeometry::toGeometry() const
{ return (VrmlNodeGeometry*) this; }


VrmlNodeColor *VrmlNodeGeometry::color() { return 0; }



// Geometry nodes need only define insertGeometry(), not render().

void VrmlNodeGeometry::render(Viewer *v) 
{
  if ( d_viewerObject && isModified() )
    {
      v->removeObject(d_viewerObject);
      d_viewerObject = 0;
    }

  if (d_viewerObject)
    v->insertReference(d_viewerObject);
  else
    {
      d_viewerObject = insertGeometry(v);
      clearModified();
    }
}


