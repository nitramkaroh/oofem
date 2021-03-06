/*
 *
 *                 #####    #####   ######  ######  ###   ###
 *               ##   ##  ##   ##  ##      ##      ## ### ##
 *              ##   ##  ##   ##  ####    ####    ##  #  ##
 *             ##   ##  ##   ##  ##      ##      ##     ##
 *            ##   ##  ##   ##  ##      ##      ##     ##
 *            #####    #####   ##      ######  ##     ##
 *
 *
 *             OOFEM : Object Oriented Finite Element Code
 *
 *               Copyright (C) 1993 - 2013   Borek Patzak
 *
 *
 *
 *       Czech Technical University, Faculty of Civil Engineering,
 *   Department of Structural Mechanics, 166 29 Prague, Czech Republic
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "dissipationarclengthactivebc.h"
#include "classfactory.h"
#include "masterdof.h"
#include "floatmatrix.h"
#include "sparsemtrx.h"
#include "unknownnumberingscheme.h"
#include "function.h"
#include "timestep.h"
#include "datastream.h"
#include "contextioerr.h"
#include "node.h"
#include "domain.h"
#include "element.h"
#include "engngm.h"


namespace oofem {
REGISTER_BoundaryCondition(DissipationArcLengthActiveBoundaryCondition);

DissipationArcLengthActiveBoundaryCondition ::  DissipationArcLengthActiveBoundaryCondition(int n, Domain * d) : ArcLengthActiveBoundaryCondition(n, d), lm( new Node(0, domain) )
{
    // this is internal lagrange multiplier used to enforce the receiver constrain
    // this allocates a new equation related to this constraint
    this->lm->appendDof( new MasterDof( this->lm.get(), ( DofIDItem ) ( d->giveNextFreeDofID() ) ) );
}



IRResultType
DissipationArcLengthActiveBoundaryCondition :: initializeFrom(InputRecord *ir)
{
    IRResultType result;
    IR_GIVE_FIELD(ir, dl, _IFT_DissipationArcLengthActiveBoundaryCondition_dissipationLength);
    elementSet = 0;
    IR_GIVE_OPTIONAL_FIELD(ir, elementSet, _IFT_DissipationArcLengthActiveBoundaryCondition_elementSet);
    return ActiveBoundaryCondition :: initializeFrom(ir);
}


void
DissipationArcLengthActiveBoundaryCondition :: postInitialize()
{
  if(elementSet) {
    this->elementList = domain->giveSet(elementSet)->giveElementList();
  }
}



void
DissipationArcLengthActiveBoundaryCondition :: assemble(SparseMtrx &answer, TimeStep *tStep,
                                    CharType type, const UnknownNumberingScheme &r_s,
                                    const UnknownNumberingScheme &c_s)
{
  
    int nelem = domain->giveNumberOfElements();
    int lambdaeq = r_s.giveDofEquationNumber( *lm->begin() );
    IntArray lambda_eq = {lambdaeq};
    FloatArray dofids;
    for ( int ielem = 1; ielem <= nelem; ielem++ ) {
        auto element = domain->giveElement(ielem);
	// skip remote elements (these are used as mirrors of remote elements on other domains
	// when nonlocal constitutive models are used. They introduction is necessary to
	// allow local averaging on domains without fine grain communication between domains).
	if ( element->giveParallelMode() == Element_remote || !element->isActivated(tStep) ) {
	  continue;
        }
	IntArray locationArray, dofids;
	element->giveLocationArray(locationArray, c_s, &dofids);
	/*DissipationDrivenArcLengthElementExtensionInterface *ddale = dynamic_cast< DissipationDrivenArcLengthElementExtensionInterface* >(elem);
	FloatArray dDiss_dEps;
	ddale->computeDissipationIncrementDerivative(dDiss_dEps);
	*/
	//@todo
	FloatArray dDiss;
	dDiss.resize(1);
	dDiss = 0;
	answer.assemble(locationArray, lambda_eq, dDiss);
	
    }

    this->assembleReferenceLoadVector(answer, lambda_eq);
    
}


void
DissipationArcLengthActiveBoundaryCondition :: assembleReferenceLoadVector(SparseMtrx &answer, const IntArray &lambdaeq)
{
  
  answer.assemble(lambdaeq, this->refLoadLocationArray, referenceLoadMatrix );
   
}
  

void
DissipationArcLengthActiveBoundaryCondition :: assembleVector(FloatArray &answer, TimeStep *tStep,
                                          CharType type, ValueModeType mode,
                                          const UnknownNumberingScheme &s, FloatArray *eNorms)
{


    if ( type == InternalForcesVector ) {
      int nelem = domain->giveNumberOfElements();
      int lambdaeq = s.giveDofEquationNumber( *lm->begin() );
      IntArray lambda_eq = {lambdaeq};
      FloatArray dofids;
      for ( int ielem = 1; ielem <= nelem; ielem++ ) {
        auto element = domain->giveElement(ielem);
	// skip remote elements (these are used as mirrors of remote elements on other domains
	// when nonlocal constitutive models are used. They introduction is necessary to
	// allow local averaging on domains without fine grain communication between domains).
	if ( element->giveParallelMode() == Element_remote || !element->isActivated(tStep) ) {
	  continue;
        }
	IntArray locationArray, dofids;
	element->giveLocationArray(locationArray, s, &dofids);
	/*
	DissipationDrivenArcLengthElementExtensionInterface *ddale = dynamic_cast< DissipationDrivenArcLengthElementExtensionInterface* >(elem);
	FloatArray dDiss_dEps;
	ddale->computeDissipationIncrementDerivative(dDiss_dEps);
	*/
	FloatArray dDiss(1);
	answer.assemble( dDiss, lambda_eq );
      }
      
    }
        
}

void
DissipationArcLengthActiveBoundaryCondition :: giveLocationArrays(std :: vector< IntArray > &rows, std :: vector< IntArray > &cols, CharType type, const UnknownNumberingScheme &r_s, const UnknownNumberingScheme &c_s)
{
  
  if(this->refLoadLocationArray.giveSize() == 0) {
    int neq = this->giveDomain()->giveEngngModel()->giveNumberOfDomainEquations(1, r_s) - 1;
    this->refLoadLocationArray.resize(neq);
    for (int i = 1; i <= neq;  i ++) {
      this->refLoadLocationArray.at(i) = i;
    }
    
  }

    rows.resize(3);
    cols.resize(3);
    IntArray loc, lambdaeq(1);
    lambdaeq.at(1) = r_s.giveDofEquationNumber( *lm->begin() );
    // column block
    rows [ 0 ] = this->refLoadLocationArray;
    cols [ 0 ] = lambdaeq;
    // row block
    cols [ 1 ] = refLoadLocationArray;
    rows [ 1 ] = lambdaeq;
    // diagonal enry (some sparse mtrx implementation requaire this)
    rows [ 2 ] = lambdaeq;
    cols [ 2 ] = lambdaeq;
}


} //end of oofem namespace
