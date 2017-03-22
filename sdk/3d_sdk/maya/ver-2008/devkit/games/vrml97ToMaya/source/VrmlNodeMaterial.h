//
//  Vrml 97 library
//  Copyright (C) 1998 Chris Morley
//
//  %W% %G%
//  VrmlNodeMaterial.h

#ifndef  _VRMLNODEMATERIAL_
#define  _VRMLNODEMATERIAL_

#include "VrmlNode.h"
#include "VrmlSFColor.h"
#include "VrmlSFFloat.h"

class VrmlNodeMaterial : public VrmlNode {

public:

  // Define the fields of Material nodes
  static VrmlNodeType *defineType(VrmlNodeType *t = 0);
  virtual VrmlNodeType *nodeType() const;

  VrmlNodeMaterial(VrmlScene *);
  virtual ~VrmlNodeMaterial();

  virtual VrmlNode *cloneMe() const;

  virtual VrmlNodeMaterial* toMaterial()	const;

  virtual ostream& printFields(ostream& os, int indent);

  virtual void render(Viewer *);

  virtual void setField(const char *fieldName, const VrmlField &fieldValue);

  float  ambientIntensity() { return d_ambientIntensity.get(); }
  float* diffuseColor() { return d_diffuseColor.get(); }
  float* emissiveColor() { return d_emissiveColor.get(); }
  float  shininess() { return d_shininess.get(); }
  float* specularColor() { return d_specularColor.get(); }
  float  transparency() { return d_transparency.get(); }

private:

  VrmlSFFloat d_ambientIntensity;
  VrmlSFColor d_diffuseColor;
  VrmlSFColor d_emissiveColor;
  VrmlSFFloat d_shininess;
  VrmlSFColor d_specularColor;
  VrmlSFFloat d_transparency;

};

#endif // _VRMLNODEMATERIAL_

