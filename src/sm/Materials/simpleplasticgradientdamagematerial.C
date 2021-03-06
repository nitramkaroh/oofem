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

#include "simpleplasticgradientdamagematerial.h"
#include "gausspoint.h"
#include "floatmatrix.h"
#include "floatarray.h"
#include "intarray.h"
#include "mathfem.h"
#include "datastream.h"
#include "contextioerr.h"
#include "inputrecord.h"

namespace oofem {

  SimplePlasticGradientDamageMaterial :: SimplePlasticGradientDamageMaterial(int n, Domain *d) : SimplePlasticMaterial(n, d), GradientDamageMaterialExtensionInterface(d)
{
}

SimplePlasticGradientDamageMaterial :: ~SimplePlasticGradientDamageMaterial()
{ }

IRResultType
SimplePlasticGradientDamageMaterial :: initializeFrom(InputRecord *ir)
{

 // Required by IR_GIVE_FIELD macro
    IRResultType result;
    // call the corresponding service of structural material
    result = SimplePlasticMaterial :: initializeFrom(ir);
    if ( result != IRRT_OK ) return result;
    kinematicHardeningFlag = false;


    result = GradientDamageMaterialExtensionInterface :: initializeFrom(ir);
    if ( result != IRRT_OK ) return result;

    // overnonlocal param m = 1 is standard gradient form
    mParam = 1.;  
    IR_GIVE_OPTIONAL_FIELD(ir, mParam, _IFT_SimplePlasticGradientDamageMaterial_m);
    
    return result;    
}


MaterialStatus *
SimplePlasticGradientDamageMaterial :: CreateStatus(GaussPoint *gp) const
{
    return new SimplePlasticGradientDamageMaterialStatus(1, this->giveDomain(), gp, this->giveSizeOfReducedHardeningVarsVector(gp));
}


    


double
SimplePlasticGradientDamageMaterial :: computeDamage(GaussPoint *gp, const FloatArray &strainSpaceHardeningVariables, TimeStep *tStep)
{


    double kappa, damage;
    SimplePlasticGradientDamageMaterialStatus *status = static_cast<     SimplePlasticGradientDamageMaterialStatus * >( this->giveStatus(gp) );

    FloatArray strainSpaceHardeningVarsVector = status->giveTempStrainSpaceHardeningVarsVector();

    //    status->giveStrainSpaceHardeningVars()
    int size = strainSpaceHardeningVarsVector.giveSize();
    damage = status->giveDamage();
    
     double nlCumPlastStrain = status->giveNonlocalCumulatedStrain();
     double localCumPlasticStrain = strainSpaceHardeningVariables.at(size);
     kappa = mParam * nlCumPlastStrain + ( 1 - mParam ) * localCumPlasticStrain;
     double tempDamage = giveDamageParam(kappa);
    if ( damage > tempDamage ) {
        tempDamage = damage;
    }

    return tempDamage;
}


double
SimplePlasticGradientDamageMaterial ::  compute_dDamage_dKappa(GaussPoint *gp, const FloatArray &strainSpaceHardeningVariables, TimeStep *tStep)
{


    double kappa;
    SimplePlasticGradientDamageMaterialStatus *status = static_cast<     SimplePlasticGradientDamageMaterialStatus * >( this->giveStatus(gp) );

    //    status->giveStrainSpaceHardeningVars()
    int size = strainSpaceHardeningVariables.giveSize();
   
     double nlCumPlastStrain = status->giveNonlocalCumulatedStrain();
     double localCumPlasticStrain = strainSpaceHardeningVariables.at(size);
     kappa = mParam * nlCumPlastStrain + ( 1 - mParam ) * localCumPlasticStrain;
     if(kappa > 0.) {
       double damagePrime = giveDamageParamPrime(kappa);
       return damagePrime;
     } else {
       return 0;
     }

}

  


void
SimplePlasticGradientDamageMaterial :: giveRealStressVectorGradientDamage(FloatArray &stress, double &localCumulatedStrain, GaussPoint *gp, const FloatArray &reducedStrain, double nonlocalCumulatedStrain, TimeStep *tStep)
{
    SimplePlasticGradientDamageMaterialStatus *status = static_cast< SimplePlasticGradientDamageMaterialStatus * >( this->giveStatus(gp) );

    FloatArray totalStrain;
    StructuralMaterial :: giveFullSymVectorForm(totalStrain, reducedStrain, gp->giveMaterialMode());
    MPlasticMaterial2 :: giveRealStressVector(stress, gp, reducedStrain, tStep);
    FloatArray strainSpaceHardeningVarsVector = status->giveTempStrainSpaceHardeningVarsVector();
    int size = strainSpaceHardeningVarsVector.giveSize();
    localCumulatedStrain = strainSpaceHardeningVarsVector.at(size);

    status->setNonlocalDamageDrivingVariable(nonlocalCumulatedStrain);
    status->setLocalDamageDrivingVariable(localCumulatedStrain);

}






  
void
SimplePlasticGradientDamageMaterial :: giveGradientDamageStiffnessMatrix_uu(FloatMatrix &answer, MatResponseMode mode, GaussPoint *gp, TimeStep *tStep)
{
     SimplePlasticGradientDamageMaterialStatus *status = static_cast< SimplePlasticGradientDamageMaterialStatus * >( this->giveStatus(gp) );
    double tempDamage = status->giveTempDamage();
    double damage = status->giveDamage();
	
  
    if ( rmType == mpm_ClosestPoint ) {
        this->giveConsistentStiffnessMatrix(answer, mode, gp, tStep);
    } else {
        this->giveElastoPlasticStiffnessMatrix(answer, mode, gp, tStep);
    }

    if((tempDamage-damage) > 0) {
      answer.times(1.0 - tempDamage);
      FloatMatrix stiffnessCorrection;
      this->giveStiffnessCorrection(stiffnessCorrection, mode, gp, tStep);
      stiffnessCorrection.times(1.-mParam);
      if(stiffnessCorrection.giveNumberOfRows()>0) {
	answer.subtract(stiffnessCorrection);
      }
    }
}
  
void                                    
SimplePlasticGradientDamageMaterial ::  giveGradientDamageStiffnessMatrix_du(FloatMatrix &answer, MatResponseMode mode, GaussPoint *gp, TimeStep *tStep)
{
    this->give_dLambda_dEps_Matrix(answer, mode, gp, tStep);
}
  
void
SimplePlasticGradientDamageMaterial :: giveGradientDamageStiffnessMatrix_ud(FloatMatrix &answer, MatResponseMode mode, GaussPoint *gp, TimeStep *tStep)
{
    SimplePlasticGradientDamageMaterialStatus *status = static_cast< SimplePlasticGradientDamageMaterialStatus* >( this->giveStatus(gp) );
    double damage, tempDamage;
    double gPrime;

    FloatArray strainSpaceHardeningVarsVector = status->giveTempStrainSpaceHardeningVarsVector();
    int size = strainSpaceHardeningVarsVector.giveSize();

    
    double nlCumPlastStrain = status->giveNonlocalCumulatedStrain();
    double localCumPlasticStrain = strainSpaceHardeningVarsVector.at(size);
    double kappa = mParam * nlCumPlastStrain + ( 1 - mParam ) * localCumPlasticStrain;
    
    damage = status->giveDamage();
    tempDamage = status->giveTempDamage();
    
    if ( ( tempDamage - damage ) > 0 ) {
      FloatArray tempEffStress = status->giveTempStressVector();
	tempEffStress.times(1./(1-tempDamage));
	int ncomp = tempEffStress.giveSize();
	answer.resize(ncomp, 1);
        for ( int i = 1; i <= ncomp; i++ ) {
            answer.at(i, 1) = tempEffStress.at(i);
        }
        gPrime = giveDamageParamPrime(kappa);
        answer.times(gPrime * mParam);
    } else {
      FloatArray tempEffStress = status->giveTempStressVector();
      answer = tempEffStress;
      answer.times(0.);
    }

  
}

  
void
SimplePlasticGradientDamageMaterial :: giveGradientDamageStiffnessMatrix_dd_BB(FloatMatrix &answer, MatResponseMode mode, GaussPoint *gp, TimeStep *tStep)
{
    this->giveInternalLengthMatrix(answer, mode, gp, tStep);

}


void
SimplePlasticGradientDamageMaterial :: giveInternalLengthMatrix(FloatMatrix &answer, MatResponseMode mode, GaussPoint *gp, TimeStep *tStep)
{
    answer.resize(1, 1);
    answer.at(1, 1) = l;
}



Interface *
SimplePlasticGradientDamageMaterial :: giveInterface(InterfaceType t)
{
    if ( t == GradientDamageMaterialExtensionInterfaceType ) {
      return static_cast< GradientDamageMaterialExtensionInterface * >(this);
    } else {
      return NULL;
    }
}


void
SimplePlasticGradientDamageMaterial :: giveNonlocalInternalForces_N_factor(double &answer, double nlDamageDrivingVariable, GaussPoint *gp, TimeStep *tStep)
{
 answer = nlDamageDrivingVariable;
}

void
SimplePlasticGradientDamageMaterial :: giveNonlocalInternalForces_B_factor(FloatArray &answer,const FloatArray &nlDamageDrivingVariable_grad, GaussPoint *gp, TimeStep *tStep)
{
  answer = nlDamageDrivingVariable_grad;
  answer.times(internalLength * internalLength);
}  


  
void
SimplePlasticGradientDamageMaterial :: computeLocalDamageDrivingVariable(double &answer, GaussPoint *gp, TimeStep *tStep)
{
  SimplePlasticGradientDamageMaterialStatus *status = static_cast< SimplePlasticGradientDamageMaterialStatus * >( this->giveStatus(gp) );
  int size = giveSizeOfReducedHardeningVarsVector(gp);
  if(size > 0) {
    FloatArray strainSpaceHardeningVarsVector = status->giveTempStrainSpaceHardeningVarsVector();
    answer = strainSpaceHardeningVarsVector.at(size);
  } else {
    answer = 0;
  }
}


  SimplePlasticGradientDamageMaterialStatus ::   SimplePlasticGradientDamageMaterialStatus(int n, Domain * d, GaussPoint * g,int statusSize) : MPlasticMaterial2Status(n, d, g, statusSize), GradientDamageMaterialStatusExtensionInterface()
{
    nonlocalDamageDrivingVariable = 0;
}

 

  
} // end namespace oofem
