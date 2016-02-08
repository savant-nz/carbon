#ifndef _MPxIkSolver
#define _MPxIkSolver
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
// CLASS:    MPxIkSolver
//
// ****************************************************************************

#if defined __cplusplus

// ****************************************************************************
// INCLUDED HEADER FILES


#include <maya/MStatus.h>
#include <maya/MTypes.h>
#include <maya/MObject.h>

// ****************************************************************************
// DECLARATIONS

class MString;
class MArgList;
class MIkHandleGroup;
class MMatrix;
class MDoubleArray;

// ****************************************************************************
// CLASS DECLARATION (MPxIkSolver)

//! \ingroup OpenMayaAnim MPx
//! \brief OBSOLETE CLASS: Base class for user defined IK solvers  
/*!
\deprecated
This class is obsolete and will be removed in a future version of Maya.
It has been replaced with the MPxIkSolverNode class.

This is the obsolete base class for writing user-defined IK solvers.
Users must at least override the following methods in order to write a solver:

	\li <b>doSolve</b>
	\li <b>solverTypeName</b>
	\li <b>isSingleChainOnly</b>
	\li <b>isPositionOnly</b>
	\li <b>hasJointLimitSupport</b>
	\li <b>hasUniqueSolution</b>
	\li <b>groupHandlesByTopology</b>


To register a solver, write a creator method to return an instance of the
user solver:

    \code
    userSolver::creator() {
        return new userSolver;
    }
    \endcode

The solver can then be registered using <b>MFnPlugin::registerSolver</b>.

Once the solver is registered it can be assigned to IK handles and its
solve methods will be called in the same manner as the solvers within
Maya.
*/
class OPENMAYAANIM_EXPORT MPxIkSolver
{
public:
	virtual ~MPxIkSolver();

	static void			registerSolver( const MString & solverName,
								MCreatorFunction creatorFunction );


	virtual MStatus		preSolve();
	virtual MStatus		doSolve();
	virtual MStatus		postSolve( MStatus );


	// These methods MUST be overridden by the user.
	//
	virtual MString		solverTypeName() const;
	virtual bool		isSingleChainOnly() const;
	virtual bool		isPositionOnly() const;
	virtual bool		hasJointLimitSupport() const;
	virtual bool		hasUniqueSolution() const;
	virtual bool		groupHandlesByTopology() const;


	virtual MStatus		setFuncValueTolerance( double tolerance );
	virtual MStatus		setMaxIterations( int value );


	MIkHandleGroup * 	handleGroup() const;
	virtual void 		setHandleGroup( MIkHandleGroup* );
	const MMatrix *		toWorldSpace() const;
	const MMatrix *		toSolverSpace() const;
	double 				funcValueTolerance() const;
	int 				maxIterations() const;
	virtual void 		snapHandle( MObject& handle );

	void				create();

protected:

	MStatus				getJointAngles( MDoubleArray& ) const;
	MStatus				setJointAngles( const MDoubleArray& );
	void				setToRestAngles();

	MPxIkSolver();

	virtual const char*	className() const;

private:
	void*	instance;

};

#endif /* __cplusplus */
#endif /* _MPxIkSolver */