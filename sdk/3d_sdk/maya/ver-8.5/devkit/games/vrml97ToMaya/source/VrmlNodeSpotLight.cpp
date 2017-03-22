//
//  Vrml 97 library
//  Copyright (C) 1998 Chris Morley
//
//  %W% %G%
//  VrmlNodeSpotLight.cpp

#include "VrmlNodeSpotLight.h"
#include "MathUtils.h"
#include "VrmlNodeType.h"
#include "VrmlScene.h"
#include "Viewer.h"

// Return a new VrmlNodeSpotLight
static VrmlNode *creator( VrmlScene *s ) { return new VrmlNodeSpotLight(s); }


// Define the built in VrmlNodeType:: "SpotLight" fields

VrmlNodeType *VrmlNodeSpotLight::defineType(VrmlNodeType *t)
{
  static VrmlNodeType *st = 0;

  if (! t)
    {
      if (st) return st;
      t = st = new VrmlNodeType("SpotLight", creator);
    }

  VrmlNodeLight::defineType(t);	// Parent class
  t->addExposedField("attenuation", VrmlField::SFVEC3F);
  t->addExposedField("beamWidth", VrmlField::SFFLOAT);
  t->addExposedField("cutOffAngle", VrmlField::SFFLOAT);
  t->addExposedField("direction", VrmlField::SFVEC3F);
  t->addExposedField("location", VrmlField::SFVEC3F);
  t->addExposedField("radius", VrmlField::SFFLOAT);

  return t;
}


VrmlNodeType *VrmlNodeSpotLight::nodeType() const { return defineType(0); }


VrmlNodeSpotLight::VrmlNodeSpotLight(VrmlScene *scene) :
  VrmlNodeLight(scene),
  d_attenuation(1.0, 0.0, 0.0),
  d_beamWidth(1.570796),
  d_cutOffAngle(0.785398),
  d_direction(0.0, 0.0, -1.0),
  d_location(0.0, 0.0, 0.0),
  d_radius(100)
{
  if (d_scene) d_scene->addScopedLight(this);
}

VrmlNodeSpotLight::~VrmlNodeSpotLight()
{
  if (d_scene) d_scene->removeScopedLight(this);
}


VrmlNode *VrmlNodeSpotLight::cloneMe() const
{
  return new VrmlNodeSpotLight(*this);
}


VrmlNodeSpotLight* VrmlNodeSpotLight::toSpotLight() const
{ return (VrmlNodeSpotLight*) this; }


void VrmlNodeSpotLight::addToScene(VrmlScene *s, const char *)
{ if (d_scene != s && (d_scene = s) != 0) d_scene->addScopedLight(this); }


ostream& VrmlNodeSpotLight::printFields(ostream& os, int indent)
{
  VrmlNodeLight::printFields(os, indent);
  if (! FPEQUAL(d_attenuation.x(), 1.0) ||
      ! FPZERO(d_attenuation.y()) ||
      ! FPZERO(d_attenuation.z()) )
    PRINT_FIELD(attenuation);
  if (! FPEQUAL(d_beamWidth.get(), 1.570796))
    PRINT_FIELD(beamWidth);

  if (! FPEQUAL(d_cutOffAngle.get(), 1.570796))
    PRINT_FIELD(cutOffAngle);
  if (! FPZERO(d_direction.x()) ||
      ! FPZERO(d_direction.y()) ||
      ! FPEQUAL(d_direction.z(), -1.0) )
    PRINT_FIELD(direction);

  if (! FPZERO(d_location.x()) ||
      ! FPZERO(d_location.y()) ||
      ! FPZERO(d_location.z()) )
    PRINT_FIELD(location);

  if (! FPEQUAL(d_radius.get(), 100.0))
    PRINT_FIELD(radius);

  return os;
}


// This should be called before rendering any geometry in the scene.
// Since this is called from Scene::render() before traversing the
// scene graph, the proper transformation matrix hasn't been set up.
// Somehow it needs to figure out the accumulated xforms of its
// parents and apply them before rendering. This is not easy with
// DEF/USEd nodes...
void VrmlNodeSpotLight::renderScoped(Viewer *viewer)
{
  if ( d_on.get() && d_radius.get() > 0.0 )
    viewer->insertSpotLight( d_ambientIntensity.get(),
			     d_attenuation.get(),
			     d_beamWidth.get(),
			     d_color.get(),
			     d_cutOffAngle.get(),
			     d_direction.get(),
			     d_intensity.get(),
			     d_location.get(),
			     d_radius.get() );
  clearModified();
}

// Set the value of one of the node fields.

void VrmlNodeSpotLight::setField(const char *fieldName,
				 const VrmlField &fieldValue)
{
  if TRY_FIELD(attenuation, SFVec3f)
  else if TRY_FIELD(beamWidth, SFFloat)
  else if TRY_FIELD(cutOffAngle, SFFloat)
  else if TRY_FIELD(direction, SFVec3f)
  else if TRY_FIELD(location, SFVec3f)
  else if TRY_FIELD(radius, SFFloat)
  else 
    VrmlNodeLight::setField(fieldName, fieldValue);
}

const VrmlSFVec3f& VrmlNodeSpotLight::getAttenuation() const   // LarryD Mar 04/99
{  return d_attenuation; }

const VrmlSFVec3f& VrmlNodeSpotLight::getDirection() const   // LarryD Mar 04/99
{  return d_direction; }

const VrmlSFVec3f& VrmlNodeSpotLight::getLocation() const   // LarryD Mar 04/99
{  return d_location; }
