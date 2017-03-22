
#ifndef _MFnPointArrayData
#define _MFnPointArrayData
//
//-
// ==========================================================================
// Copyright (C) 1995 - 2006 Autodesk, Inc., and/or its licensors.  All 
// rights reserved.
// 
// The coded instructions, statements, computer programs, and/or related 
// material (collectively the "Data") in these files contain unpublished 
// information proprietary to Autodesk, Inc. ("Autodesk") and/or its 
// licensors,  which is protected by U.S. and Canadian federal copyright law 
// and by international treaties.
// 
// The Data may not be disclosed or distributed to third parties or be 
// copied or duplicated, in whole or in part, without the prior written 
// consent of Autodesk.
// 
// The copyright notices in the Software and this entire statement, 
// including the above license grant, this restriction and the following 
// disclaimer, must be included in all copies of the Software, in whole 
// or in part, and all derivative works of the Software, unless such copies 
// or derivative works are solely in the form of machine-executable object 
// code generated by a source language processor.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND. 
// AUTODESK DOES NOT MAKE AND HEREBY DISCLAIMS ANY EXPRESS OR IMPLIED 
// WARRANTIES INCLUDING, BUT NOT LIMITED TO, THE WARRANTIES OF 
// NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, 
// OR ARISING FROM A COURSE OF DEALING, USAGE, OR TRADE PRACTICE. IN NO 
// EVENT WILL AUTODESK AND/OR ITS LICENSORS BE LIABLE FOR ANY LOST 
// REVENUES, DATA, OR PROFITS, OR SPECIAL, DIRECT, INDIRECT, OR 
// CONSEQUENTIAL DAMAGES, EVEN IF AUTODESK AND/OR ITS LICENSORS HAS 
// BEEN ADVISED OF THE POSSIBILITY OR PROBABILITY OF SUCH DAMAGES. 
// ==========================================================================
//+
//
// CLASS:    MFnPointArrayData
//
// *****************************************************************************
//
// CLASS DESCRIPTION (MFnPointArrayData)
//
// MFnPointArrayData allows the creation and manipulation of MPointArray data
// objects for use in the dependency graph.  
//
// If a user written dependency node either accepts or produces MPointArrays,
// then this class is used to extract or create the data that comes from or
// goes to other dependency graph nodes.  The MDataHandle::type method will
// return kPointArray when data of this type is present.  To access it, the
// MDataHandle::data method is used to get an MObject for the data and this 
// should then be used to initialize an instance of MFnPointArrayData.
//
// *****************************************************************************

#if defined __cplusplus

// *****************************************************************************

// INCLUDED HEADER FILES


#include <maya/MFnData.h>

// *****************************************************************************

// DECLARATIONS

class MPoint;
class MPointArray;

// *****************************************************************************

// CLASS DECLARATION (MFnPointArrayData)

/// Point array function set for dependency node data. (OpenMaya) (OpenMaya.py)
/**
  Create and manipulate MPointArray dependency node data.
*/
#ifdef _WIN32
#pragma warning(disable: 4522)
#endif // _WIN32

class OPENMAYA_EXPORT MFnPointArrayData : public MFnData 
{

	declareMFn(MFnPointArrayData, MFnData);

public:
	///
	unsigned int		length( MStatus* ReturnStatus = NULL ) const;
	///
	MStatus			set( MPoint& element, unsigned int index );
	///
	MStatus			copyTo( MPointArray& ) const;
	///
	MStatus			set( const MPointArray& newArray );
	///
	MPointArray		array( MStatus*ReturnStatus=NULL );
	///
	MObject			create( MStatus*ReturnStatus=NULL );
	///
	MObject			create( const MPointArray& in, MStatus*ReturnStatus=NULL );

BEGIN_NO_SCRIPT_SUPPORT:

 	declareMFnConstConstructor( MFnPointArrayData, MFnData );

	///	NO SCRIPT SUPPORT
	MPoint&			operator[]( unsigned int index );
	///	NO SCRIPT SUPPORT
	const MPoint&	operator[]( unsigned int index ) const;
	
END_NO_SCRIPT_SUPPORT:

protected:
// No protected members

private:
// No private members

};

#ifdef _WIN32
#pragma warning(default: 4522)
#endif // _WIN32

// *****************************************************************************
#endif /* __cplusplus */
#endif /* _MFnPointArrayData */
