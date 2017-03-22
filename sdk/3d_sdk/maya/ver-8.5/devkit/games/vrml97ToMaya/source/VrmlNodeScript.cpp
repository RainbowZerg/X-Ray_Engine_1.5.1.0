//
//  Vrml 97 library
//  Copyright (C) 1998 Chris Morley
//
//  %W% %G%
//  VrmlNodeScript.cpp

#include <stdlib.h>
#include "VrmlNodeScript.h"
#include "VrmlNodeType.h"
#include "VrmlSFTime.h"
#include "VrmlSFNode.h"
#include "VrmlMFNode.h"

#include "ScriptObject.h"

#include "VrmlScene.h"

#if defined(WIN32)
// Disable warning messages about forcing value to bool 'true' or 'false'
#pragma warning (disable:4800)
#endif

// Script factory. Add each Script to the scene for fast access.

static VrmlNode *creator( VrmlScene *scene ) 
{
  return new VrmlNodeScript(scene);
}


// Define the built in VrmlNodeType:: "Script" fields

VrmlNodeType *VrmlNodeScript::defineType(VrmlNodeType *t)
{
  static VrmlNodeType *st = 0;

  if (! t)
    {
      if (st) return st;		// Only define the type once.
      t = st = new VrmlNodeType("Script", creator);
    }

  VrmlNodeChild::defineType(t);	// Parent class
  t->addExposedField("url", VrmlField::MFSTRING);
  t->addField("directOutput", VrmlField::SFBOOL);
  t->addField("mustEvaluate", VrmlField::SFBOOL);

  return t;
}

// Should subclass NodeType and have each Script maintain its own type...

VrmlNodeType *VrmlNodeScript::nodeType() const { return defineType(0); }


VrmlNodeScript::VrmlNodeScript( VrmlScene *scene ) :
  VrmlNodeChild(scene),
  d_directOutput(false),
  d_mustEvaluate(false),
  d_script(0),
  d_eventsReceived(0)
{
  if (d_scene) d_scene->addScript(this);
}

VrmlNodeScript::VrmlNodeScript( const VrmlNodeScript &n ) :
  VrmlNodeChild(0),
  d_directOutput(n.d_directOutput),
  d_mustEvaluate(n.d_mustEvaluate),
  d_url(n.d_url),
  d_script(0),		// Force creation of a distinct script object
  d_eventIns(0),
  d_eventOuts(0),
  d_fields(0),  
  d_eventsReceived(0)
{
  // add eventIn/eventOut/fields from source Script
  FieldList::const_iterator i;

  for (i = n.d_eventIns.begin(); i != n.d_eventIns.end(); ++i)
    addEventIn( (*i)->name, (*i)->type );
  for (i = n.d_eventOuts.begin(); i != n.d_eventOuts.end(); ++i)
    addEventOut( (*i)->name, (*i)->type );
  for (i = n.d_fields.begin(); i != n.d_fields.end(); ++i)
    addField(  (*i)->name, (*i)->type, (*i)->value );
}


VrmlNodeScript::~VrmlNodeScript()
{
  shutdown( theSystem->time() );

  // removeScript ought to call shutdown...
  if (d_scene) d_scene->removeScript(this);

  delete d_script;

  // delete eventIn/eventOut/field ScriptField list contents
  FieldList::iterator i;
  for (i = d_eventIns.begin(); i != d_eventIns.end(); ++i)
    {
      ScriptField *r = *i;
      free(r->name);
      delete r->value;
      delete r;
    }
  for (i = d_eventOuts.begin(); i != d_eventOuts.end(); ++i)
    {
      ScriptField *r = *i;
      free(r->name);
      delete r->value;
      delete r;
    }
  for (i = d_fields.begin(); i != d_fields.end(); ++i)
    {
      ScriptField *r = *i;
      free(r->name);
      delete r->value;
      delete r;
    }
}


VrmlNode *VrmlNodeScript::cloneMe() const
{
  return new VrmlNodeScript(*this);
}

//
// Any SFNode or MFNode fields need to be cloned as well.
//

void VrmlNodeScript::cloneChildren( VrmlNamespace* ns ) 
{
  FieldList::iterator i;
  for (i = d_fields.begin(); i != d_fields.end(); ++i)
    if ((*i)->value)
      {
	if ( (*i)->type == VrmlField::SFNODE )
	  {
	    const VrmlSFNode *sfn = (*i)->value->toSFNode();
	    if (sfn && sfn->get())
	      {
		(*i)->value = new VrmlSFNode( sfn->get()->clone(ns) );
		delete sfn;
	      }
	  }

	else if ( (*i)->type == VrmlField::MFNODE &&
		  (*i)->value->toMFNode() )
	  {
	    int nk = (*i)->value->toMFNode()->size();
	    VrmlNode **kids = (*i)->value->toMFNode()->get();
	    for (int k=0; k<nk; ++k)
	      if (kids[k])
		{
		  VrmlNode *tmp = kids[k];
		  kids[k] = tmp->clone(ns);
		  tmp->dereference();
		}
	  }
      }
}



VrmlNodeScript* VrmlNodeScript::toScript() const
{ return (VrmlNodeScript*) this; }

void VrmlNodeScript::addToScene(VrmlScene *s, const char *relUrl)
{
  theSystem->debug("VrmlNodeScript::%s 0x%p addToScene 0x%p\n",
		   name(), this, s);

  d_relativeUrl.set(relUrl);
  if (d_scene == s) return;
  initialize( theSystem->time() );
  if ((d_scene = s) != 0) d_scene->addScript(this);
}


ostream& VrmlNodeScript::printFields(ostream& os, int indent)
{
  if (d_url.size() > 0) PRINT_FIELD(url);
  if (d_directOutput.get()) PRINT_FIELD(directOutput);
  if (d_mustEvaluate.get()) PRINT_FIELD(mustEvaluate);

  return os;
}


void VrmlNodeScript::initialize( double ts )
{
  theSystem->debug("Script.%s::initialize\n", name());

  if (d_script) return;	       // How did I get here? Letting the days go by...
  d_eventsReceived = 0;
  if (d_url.size() > 0)
    {
      d_script = ScriptObject::create( this, d_url );
      if (d_script)
	d_script->activate( ts, "initialize", 0, 0 );
    }
}

void VrmlNodeScript::shutdown( double ts )
{
  if (d_script)
    d_script->activate( ts, "shutdown", 0, 0 );
}

void VrmlNodeScript::update( VrmlSFTime &timeNow )
{
  if (d_eventsReceived > 0)
    {
      //theSystem->debug("Script.%s::update\n", name());
      if (d_script)
	d_script->activate( timeNow.get(), "eventsProcessed", 0, 0 );
      d_eventsReceived = 0;
    }
}


// 

void VrmlNodeScript::eventIn(double timeStamp,
			     const char *eventName,
			     const VrmlField *fieldValue)
{
  if (! d_script ) initialize( timeStamp );
  if (! d_script ) return;

  const char *origEventName = eventName;
  bool valid = hasEventIn( eventName );
  if (! valid && strncmp(eventName, "set_", 4) == 0 )
    {
      eventName += 4;
      valid = hasEventIn( eventName );
    }
#if 0
  cerr << "eventIn Script::" << name() << "." << origEventName
       << " " << (*fieldValue) << ", valid " << valid
       << ", d_script " << (unsigned long)d_script
       << endl;
#endif
  if ( valid )
    {
      setEventIn( eventName, fieldValue );

      VrmlSFTime ts( timeStamp );
      const VrmlField *args[] = { fieldValue, &ts };

      FieldList::const_iterator i;
      for (i = d_eventOuts.begin(); i != d_eventOuts.end(); ++i)
	(*i)->modified = false;

      d_script->activate( timeStamp, eventName, 2, args );

      // For each modified eventOut, send an event
      for (i = d_eventOuts.begin(); i != d_eventOuts.end(); ++i)
	if ((*i)->modified)
	  eventOut( timeStamp, (*i)->name, *((*i)->value) );

      ++d_eventsReceived;	// call eventsProcessed later
    }

  // Let the generic code handle the rest.
  else
    VrmlNode::eventIn( timeStamp, origEventName, fieldValue );

  // Scripts shouldn't generate redraws.
  clearModified();
}



// add events/fields

void VrmlNodeScript::addEventIn(const char *ename, VrmlField::VrmlFieldType t)
{
  add(d_eventIns, ename, t);
}

void VrmlNodeScript::addEventOut(const char *ename, VrmlField::VrmlFieldType t)
{
  add(d_eventOuts, ename, t);
}

void VrmlNodeScript::addField(const char *ename, VrmlField::VrmlFieldType t,
			      VrmlField *val)
{
  add(d_fields, ename, t);
  if (val) set(d_fields, ename, val);
}

void VrmlNodeScript::add(FieldList &recs,
			 const char *ename,
			 VrmlField::VrmlFieldType type)
{
  ScriptField *r = new ScriptField;
  r->name = strdup(ename);
  r->type = type;
  r->value = 0;
  recs.push_front(r);
}

// get event/field values
#if 0
VrmlField*
VrmlNodeScript::getEventIn(const char *fname) const
{
  return get(d_eventIns, fname);
}

VrmlField*
VrmlNodeScript::getEventOut(const char *fname) const
{
  return get(d_eventOuts, fname);
}

VrmlField*
VrmlNodeScript::getField(const char *fname) const
{
  return get(d_fields, fname);
}
#endif

VrmlField*
VrmlNodeScript::get(const FieldList &recs, const char *fname) const
{
  FieldList::const_iterator i;
  for (i = recs.begin(); i != recs.end(); ++i) {
    if (strcmp((*i)->name, fname) == 0)
      return (*i)->value;
  }
  return 0;
}

// has

VrmlField::VrmlFieldType
VrmlNodeScript::hasEventIn(const char *ename) const
{
  return has(d_eventIns, ename);
}

VrmlField::VrmlFieldType
VrmlNodeScript::hasEventOut(const char *ename) const
{
  return has(d_eventOuts, ename);
}

VrmlField::VrmlFieldType
VrmlNodeScript::hasField(const char *ename) const
{
  return has(d_fields, ename);
}


VrmlField::VrmlFieldType
VrmlNodeScript::has(const FieldList &recs, const char *ename) const
{
  FieldList::const_iterator i;
  for (i = recs.begin(); i != recs.end(); ++i) {
    if (strcmp((*i)->name, ename) == 0)
      return (*i)->type;
  }
  return VrmlField::NO_FIELD;
}


// Set the value of one of the node fields/events.
// setField is public so the parser can access it.

void VrmlNodeScript::setField(const char *fieldName,
			      const VrmlField &fieldValue)
{
  VrmlField::VrmlFieldType ft;

  if TRY_FIELD(url, MFString)	// need to re-initialize() if url changes...
  else if TRY_FIELD(directOutput, SFBool)
  else if TRY_FIELD(mustEvaluate, SFBool)
  else if ( (ft = hasField(fieldName)) != 0 )
    {
      if (ft == VrmlField::fieldType( fieldValue.fieldTypeName() ))
	set( d_fields, fieldName, &fieldValue );
      else
	theSystem->error("Invalid type (%s) for %s field of Script node.\n",
		      fieldValue.fieldTypeName(), fieldName );
    }
  else
    VrmlNodeChild::setField(fieldName, fieldValue);
}

void
VrmlNodeScript::setEventIn(const char *fname, const VrmlField *value)
{
  set(d_eventIns, fname, value);
}

void
VrmlNodeScript::setEventOut(const char *fname, const VrmlField *value)
{
#if 0
  cerr << "Script::" << name() << " setEventOut(" << fname << ", "
       << (*value) << endl;
#endif
  set(d_eventOuts, fname, value);
}

void
VrmlNodeScript::set(const FieldList &recs,
		    const char *fname,
		    const VrmlField *value)
{
  FieldList::const_iterator i;
  for (i = recs.begin(); i != recs.end(); ++i) {
    if (strcmp((*i)->name, fname) == 0)
      {
	delete ((*i)->value);
	(*i)->value = value->clone();
	(*i)->modified = true;
	return;
      }
  }
}

