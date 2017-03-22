//-
// ==========================================================================
// Copyright (C) 2005 ATI Technologies Inc. All rights reserved.
//
// Copyright (C) 1995 - 2006 Autodesk, Inc. and/or its licensors.  All 
// rights reserved.
//
// The coded instructions, statements, computer programs, and/or related 
// material (collectively the "Data") in these files contain unpublished 
// information proprietary to Autodesk, Inc. ("Autodesk") and/or its 
// licensors, which is protected by U.S. and Canadian federal copyright 
// law and by international treaties.
//
// The Data is provided for use exclusively by You. You have the right 
// to use, modify, and incorporate this Data into other products for 
// purposes authorized by the Autodesk software license agreement, 
// without fee.
//
// The copyright notices in the Software and this entire statement, 
// including the above license grant, this restriction and the 
// following disclaimer, must be included in all copies of the 
// Software, in whole or in part, and all derivative works of 
// the Software, unless such copies or derivative works are solely 
// in the form of machine-executable object code generated by a 
// source language processor.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND. 
// AUTODESK DOES NOT MAKE AND HEREBY DISCLAIMS ANY EXPRESS OR IMPLIED 
// WARRANTIES INCLUDING, BUT NOT LIMITED TO, THE WARRANTIES OF 
// NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR 
// PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE, OR 
// TRADE PRACTICE. IN NO EVENT WILL AUTODESK AND/OR ITS LICENSORS 
// BE LIABLE FOR ANY LOST REVENUES, DATA, OR PROFITS, OR SPECIAL, 
// DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES, EVEN IF AUTODESK 
// AND/OR ITS LICENSORS HAS BEEN ADVISED OF THE POSSIBILITY 
// OR PROBABILITY OF SUCH DAMAGES.
//
// ==========================================================================
//+
#ifndef DEFAULT_SHADER_H
#define DEFAULT_SHADER_H

#include "glExtensions.h"

#include "Shader.h"

class defaultShader : public shader {

  public:
    defaultShader();

    //no GL cleanup, because we may not have a context at shutdown
    virtual ~defaultShader() {};

    virtual bool valid() {return true;};
    virtual int passCount() {return 1;};
    virtual int techniqueCount() {return 1;};
    virtual const char* techniqueName( int n) { return NULL;};
    virtual bool build() { return true;};
    virtual void bind();
	virtual void setShapeDependentState();
    virtual void unbind();
    virtual void setTechnique( int t) {};
    virtual void setPass( int p) {};
    virtual const char* getVertexShader( int pass) {return NULL;};
    virtual const char* getPixelShader( int pass) {return NULL;};

    virtual int uniformCount() {return 0;};
    virtual int samplerCount() {return 0;};
    virtual int attributeCount() {return 0;};
    virtual const char* uniformName(int i) {return NULL;};
    virtual DataType uniformType(int i) { return dtUnknown;};
    virtual Semantic uniformSemantic(int i) { return smNone;};
    virtual float* uniformDefault(int i) { return NULL;};
    virtual const char* samplerName(int i) {return NULL;};
    virtual SamplerType samplerType(int i) {return stUnknown;};
    virtual const char* attributeName(int i) {return NULL;};
    virtual DataType attributeType(int i) {return dtUnknown;};
    virtual int attributeHandle(int i) {return 0;};

    //need set functions for current values
    virtual void updateUniformBool( int i, bool val) {};
    virtual void updateUniformInt( int i, int val) {};
    virtual void updateUniformFloat( int i, float val) {};
    virtual void updateUniformBVec( int i, const bool* val) {};
    virtual void updateUniformIVec( int i, const int* val) {};
    virtual void updateUniformVec( int i, const float* val) {};
    virtual void updateUniformMat( int i, const float* val) {};
    virtual void updateUniformMat( int i, const double* val) {};
    virtual void updateSampler( int i, unsigned int val) {};

    //predefined attributes
    virtual bool usesColor() {return false;};
    virtual bool usesNormal() {return true;};
    virtual bool usesTexCoord( int set) {return set==0;};
    virtual bool usesTangent() {return false;};
    virtual bool usesBinormal() {return false;};
    virtual int tangentSlot() {return 0;};
    virtual int binormalSlot() {return 0;};

    //error reporting
    virtual const char* errorString() { return "";};

  private:

    static const char vShader[];
    static const char fShader[];

    GLuint m_program;
    GLuint m_fShader;
    GLuint m_vShader;
};

#endif //DEFAULT_SHADER_H


