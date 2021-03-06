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

#include "rankinematgrad.h"
#include "stressvector.h"
#include "gausspoint.h"
#include "floatmatrix.h"
#include "floatarray.h"
#include "error.h"
#include "classfactory.h"
#include "mathfem.h"

namespace oofem {
/////////////////////////////////////////////////
// gradient regularization of Rankine plasticity
// coupled with isotropic damage
/////////////////////////////////////////////////

REGISTER_Material(RankineMatGrad);

// constructor
RankineMatGrad :: RankineMatGrad(int n, Domain *d) : RankineMat(n, d), GradientDamageMaterialExtensionInterface(d)
{
    negligible_damage = 0.;
}

/////////////////////////////////////////////////////////////////////////////
IRResultType
RankineMatGrad :: initializeFrom(InputRecord *ir)
{
    IRResultType result;                             // Required by IR_GIVE_FIELD macro


    mParam = 2.;
    IR_GIVE_OPTIONAL_FIELD(ir, mParam, _IFT_RankineMatGrad_m);

    negligible_damage = 0.;
    IR_GIVE_OPTIONAL_FIELD(ir, negligible_damage, _IFT_RankineMatGrad_negligibleDamage);


    int formulationType = 0;
    IR_GIVE_OPTIONAL_FIELD(ir, formulationType, _IFT_RankineMatGrad_formulationType);
    if ( formulationType == 0 ) {
        this->gradientDamageFormulationType = GDFT_Standard;
    } else if ( formulationType == 2 ) {
        this->gradientDamageFormulationType =   GDFT_Eikonal;
    } else {
        OOFEM_ERROR("Unknown gradient damage formulation %d", formulationType);
        return IRRT_BAD_FORMAT;
    }


    result = GradientDamageMaterialExtensionInterface :: initializeFrom(ir);
    if ( result != IRRT_OK ) {
      return result;
    }   
    
    return RankineMat :: initializeFrom(ir);
}
/////////////////////////////////////////////////////////////////////////////

int
RankineMatGrad :: hasMaterialModeCapability(MaterialMode mode)
{
    return mode == _PlaneStress;
}

void
RankineMatGrad :: giveStiffnessMatrix(FloatMatrix &answer, MatResponseMode rMode, GaussPoint *gp, TimeStep *tStep)
//
// Returns characteristic material matrix of the receiver
//
{
    OOFEM_ERROR("Shouldn't be called.");
}

void
RankineMatGrad :: giveGradientDamageStiffnessMatrix_uu(FloatMatrix &answer, MatResponseMode mode, GaussPoint *gp, TimeStep *tStep)
{
    MaterialMode mMode = gp->giveMaterialMode();
    switch ( mMode ) {
    case _PlaneStress:
        if ( mode == ElasticStiffness ) {
            this->giveLinearElasticMaterial()->giveStiffnessMatrix(answer, mode, gp, tStep);
        } else {
            RankineMatGradStatus *status = static_cast< RankineMatGradStatus * >( this->giveStatus(gp) );
            double tempDamage = status->giveTempDamage();
            double damage = status->giveDamage();
            double gprime;
            // Note:
            // The approximate solution of Helmholtz equation can lead
            // to very small but nonzero nonlocal kappa at some points that
            // are actually elastic. If such small values are positive,
            // they lead to a very small but nonzero damage. If this is
            // interpreted as "loading", the tangent terms are activated,
            // but damage will not actually grow at such points and the
            // convergence rate is slowed down. It is better to consider
            // such points as elastic.
            if ( tempDamage - damage <= negligible_damage ) {
                gprime = 0.;
            } else {
                double nonlocalCumulatedStrain = status->giveTempNonlocalCumulativePlasticStrain();
                double tempLocalCumulatedStrain = status->giveTempCumulativePlasticStrain();
                double overNonlocalCumulatedStrain = mParam * nonlocalCumulatedStrain + ( 1. - mParam ) * tempLocalCumulatedStrain;
                gprime = computeDamageParamPrime(overNonlocalCumulatedStrain);
                gprime *= ( 1. - mParam );
            }

            evaluatePlaneStressStiffMtrx(answer, mode, gp, tStep, gprime);
        }
        break;
    case _1dMat:
        if ( mode == ElasticStiffness ) {
            this->giveLinearElasticMaterial()->giveStiffnessMatrix(answer, mode, gp, tStep);
        } else {
	  RankineMatGradStatus *status = static_cast< RankineMatGradStatus * >( this->giveStatus(gp) );
	  const FloatArray &stressVector = status->giveTempEffectiveStress();
	  double tempDamage = status->giveTempDamage();
	  double stress;
	  if(stressVector.giveSize()) {
	    stress = stressVector.at(1);
	  } else  {
	    stress = 0;
	  }
	  answer.resize(1, 1);
	  double nonlocalCumulativePlasticStrain = status->giveTempNonlocalCumulativePlasticStrain();
	  double localCumulativePlasticStrain = status->giveTempCumulativePlasticStrain();
	  double overNonlocalCumulativePlasticStrain = mParam * nonlocalCumulativePlasticStrain + ( 1. - mParam ) * localCumulativePlasticStrain;  
	  answer.at(1, 1) = ( 1 - tempDamage ) * E * H0 / ( E + H0 );
	  if(tempDamage > status->giveDamage() &&  mode == TangentStiffness) {   
	    answer.at(1,1) -= (1. - mParam) * computeDamageParamPrime(overNonlocalCumulativePlasticStrain) * E / ( E + H0 ) * stress * signum(stress);
	  }
	  double pert = 1.e-8;
	  FloatArray strain;
	  strain= status->giveTempStrainVector();
	  strain.resizeWithValues(1);
	  FloatArray Ep(strain);
	  FloatArray S,Sp;
	  Ep.at(1) += pert;
	  /// numerical stiffness
	  this->giveRealStressVectorGradientDamage(Sp, localCumulativePlasticStrain, gp, Ep, nonlocalCumulativePlasticStrain, tStep);
	  this->giveRealStressVectorGradientDamage(S, localCumulativePlasticStrain, gp, strain, nonlocalCumulativePlasticStrain, tStep);
	  double A = (Sp.at(1) - S.at(1)) / pert;

	  if(answer.at(1,1) - A > 1.e-5)
	    int ahoj = 1;
	}

	
	break;
      
    default:
        OOFEM_ERROR("mMode = %d not supported\n", mMode);
    }
}

void
RankineMatGrad :: giveGradientDamageStiffnessMatrix_ud(FloatMatrix &answer, MatResponseMode mode, GaussPoint *gp, TimeStep *tStep)
{
    MaterialMode mMode = gp->giveMaterialMode();
    switch ( mMode ) {
    case _PlaneStress:
      {
        answer.resize(3, 1);
        answer.zero();
        if ( mode != TangentStiffness ) {
	  return;
        }
	
        RankineMatGradStatus *status = static_cast< RankineMatGradStatus * >( this->giveStatus(gp) );
        double damage = status->giveDamage();
        double tempDamage = status->giveTempDamage();
        if ( tempDamage - damage <= negligible_damage ) {
	  return;
        }
        
        double nonlocalCumulativePlasticStrain = status->giveTempNonlocalCumulativePlasticStrain();
        double localCumulativePlasticStrain = status->giveTempCumulativePlasticStrain();
        double overNonlocalCumulativePlasticStrain = mParam * nonlocalCumulativePlasticStrain + ( 1. - mParam ) * localCumulativePlasticStrain;
        const FloatArray &tempEffStress = status->giveTempEffectiveStress();
        answer.at(1, 1) = tempEffStress.at(1);
        answer.at(2, 1) = tempEffStress.at(2);
        answer.at(3, 1) = tempEffStress.at(3);
        double gPrime = computeDamageParamPrime(overNonlocalCumulativePlasticStrain);
        answer.times(- gPrime * mParam);



	/*	double lddv;
		FloatArray strainP, stressP, oldStrain, oldStress;
		oldStress = status->giveTempStressVector();
		oldStrain = status->giveTempStrainVector();
		double nonlocalCumulatedStrain = status->giveNonlocalCumulatedStrain();
		double localCumulatedStrain = status->giveTempCumulativePlasticStrain();
		double overNonlocalCumulatedStrain = mParam * nonlocalCumulatedStrain + ( 1. - mParam ) * localCumulatedStrain;
	
		double mddv = nonlocalCumulatedStrain;
		double pert = 1.e-6 * mddv;
		strainP = oldStrain;
		mddv += pert;
		this->giveRealStressVectorGradientDamage(stressP, lddv, gp, strainP, mddv, tStep);
		FloatMatrix stiff;
		stiff.resize(3,1);
		stiff.at(1,1) = ( stressP.at(1) - oldStress.at(1) )/pert;
		this->giveRealStressVectorGradientDamage(stressP, lddv, gp, oldStrain, mddv, tStep);
	*/
      }
      break;
    
    case _1dMat:
      {
	answer.resize(1, 1);
	answer.at(1, 1) = 0;

	if ( mode == TangentStiffness ) {
	  RankineMatGradStatus *status = static_cast< RankineMatGradStatus * >( this->giveStatus(gp) );
	  if ( status->giveTempDamage() > status->giveDamage() ) {
	    double nonlocalCumulativePlasticStrain = status->giveTempNonlocalCumulativePlasticStrain();
	    double localCumulativePlasticStrain = status->giveTempCumulativePlasticStrain();
	    double overNonlocalCumulativePlasticStrain = mParam * nonlocalCumulativePlasticStrain + ( 1. - mParam ) * localCumulativePlasticStrain;
	    const FloatArray &tempEffStress = status->giveTempEffectiveStress();
	    answer.at(1,1) = tempEffStress.at(1);
	    double gPrime = computeDamageParamPrime(overNonlocalCumulativePlasticStrain);
	    answer.times(- gPrime * mParam);
	    ///
	    double pert = 1.e-8;
	    FloatArray strain;
	    strain= status->giveTempStrainVector();
	    strain.resizeWithValues(1);
	    FloatArray Ep(strain);
	    FloatArray S,Sp;
	    double nlp = nonlocalCumulativePlasticStrain + pert;
	    /// numerical stiffness
	    this->giveRealStressVectorGradientDamage(Sp, localCumulativePlasticStrain, gp, Ep, nlp, tStep);
	    this->giveRealStressVectorGradientDamage(S, localCumulativePlasticStrain, gp, strain, nonlocalCumulativePlasticStrain, tStep);
	    double A = (Sp.at(1) - S.at(1)) / pert;
	    if(answer.at(1,1) - A > 1.e-5)
	      int ahoj = 1;

	  }
	}
      }
      break;
      
    default:
      OOFEM_ERROR("mMode = %d not supported\n", mMode);
    }
    
}

void
RankineMatGrad :: giveGradientDamageStiffnessMatrix_du(FloatMatrix &answer, MatResponseMode mode, GaussPoint *gp, TimeStep *tStep)
{
    MaterialMode mMode = gp->giveMaterialMode();
    switch ( mMode ) {
    case _PlaneStress:
    {

        RankineMatGradStatus *status = static_cast< RankineMatGradStatus * >( this->giveStatus(gp) );
        answer.resize(3, 1);
        answer.zero();
        if ( mode != TangentStiffness ) {
            return;
        }

        double tempKappa = status->giveTempCumulativePlasticStrain();
        double dKappa = tempKappa - status->giveCumulativePlasticStrain();
        if ( dKappa <= 0. ) {
            return;
        }

        FloatArray eta(3);
        double dkap1 = status->giveDKappa(1);
        double H = evalPlasticModulus(tempKappa);

        // evaluate in principal coordinates

        if ( dkap1 == 0. ) {
            // regular case
            double Estar = E / ( 1. - nu * nu );
            double aux = Estar / ( H + Estar );
            eta.at(1) = aux;
            eta.at(2) = nu * aux;
            eta.at(3) = 0.;
        } else {
            // vertex case
            double dkap2 = status->giveDKappa(2);
            double denom = E * dkap1 + H * ( 1. - nu ) * ( dkap1 + dkap2 );
            eta.at(1) = E * dkap1 / denom;
            eta.at(2) = E * dkap2 / denom;
            eta.at(3) = 0.;
        }

        // transform to global coordinates

        FloatArray sigPrinc(2);
        FloatMatrix nPrinc(2, 2);
        StressVector effStress(status->giveTempEffectiveStress(), _PlaneStress);
        effStress.computePrincipalValDir(sigPrinc, nPrinc);

        FloatMatrix T(3, 3);
        givePlaneStressVectorTranformationMtrx(T, nPrinc, true);
        FloatArray etaglob(3);
        etaglob.beProductOf(T, eta);

        answer.at(1, 1) = -etaglob.at(1);
        answer.at(2, 1) = -etaglob.at(2);
        answer.at(3, 1) = -etaglob.at(3);


	/*	double lddv;
	FloatArray strainP, stressP, oldStrain, oldStress;
	oldStress = status->giveTempStressVector();
	oldStrain = status->giveTempStrainVector();
	double nonlocalCumulatedStrain = status->giveNonlocalCumulatedStrain();
	double localCumulatedStrain = status->giveTempCumulativePlasticStrain();
	double overNonlocalCumulatedStrain = mParam * nonlocalCumulatedStrain + ( 1. - mParam ) * localCumulatedStrain;
	double mddv = nonlocalCumulatedStrain;
	double pert = 1.e-6 * oldStrain.at(1);
	strainP = oldStrain;
	strainP.at(1) += pert;
	this->giveRealStressVectorGradientDamage(stressP, lddv, gp, strainP, mddv, tStep);
	FloatMatrix stiff;
	stiff.resize(3,1);
	stiff.at(1,1) = ( lddv - localCumulatedStrain )/pert;
	this->giveRealStressVectorGradientDamage(stressP, lddv, gp, oldStrain, mddv, tStep);
	*/
		
	if ( gradientDamageFormulationType  == GDFT_Eikonal ) {
	  double iA = this->computeEikonalInternalLength_a(gp);
	  if ( iA != 0 ) {
	    double nonlocalCumulativePlasticStrain = status->giveTempNonlocalCumulativePlasticStrain();
	    double localCumulativePlasticStrain = status->giveTempCumulativePlasticStrain();
	    double overNonlocalCumulativePlasticStrain = mParam * nonlocalCumulativePlasticStrain + ( 1. - mParam ) * localCumulativePlasticStrain;
	    double iAPrime = this->computeEikonalInternalLength_aPrime(gp);
	    double gPrime = this->computeDamageParamPrime(overNonlocalCumulativePlasticStrain);
	    double factor = (1. - mParam ) * iAPrime * gPrime * (localCumulativePlasticStrain - nonlocalCumulativePlasticStrain ) / ( iA * iA );           
	    answer.times( 1. / iA - factor);
	  }
	}
	
    }
    break;

    case _1dMat:
      {
	answer.resize(1, 1);
	answer.at(1, 1) = 0;
	
	if ( mode == TangentStiffness ) {
	  RankineMatGradStatus *status = static_cast< RankineMatGradStatus * >( this->giveStatus(gp) );
	  double nonlocalCumulativePlasticStrain = status->giveTempNonlocalCumulativePlasticStrain();
	  double localCumulativePlasticStrain = status->giveTempCumulativePlasticStrain();
	  double overNonlocalCumulativePlasticStrain = mParam * nonlocalCumulativePlasticStrain + ( 1. - mParam ) * localCumulativePlasticStrain;
	  const FloatArray &effStress = status->giveTempEffectiveStress();
	  double stress;
	  if(effStress.giveSize()) {
	    stress = effStress.at(1);
	  } else {
	    stress = 0;
	  }
	  double damage = status->giveDamage();
	  double tempDamage = status->giveTempDamage();
	  double dKappa = localCumulativePlasticStrain - status->giveCumulativePlasticStrain();
	  if ( dKappa <= 0. ) {
	    answer.clear();
	    return;
	  } 

	  //if ( tempDamage > damage ) {
	  double trialS = signum(stress);
	  double factor = trialS * E / ( E + H0 );
	  answer.at(1, 1) = factor;
	    //}
	  if ( gradientDamageFormulationType  == GDFT_Eikonal ) {
	    double iA = this->computeEikonalInternalLength_a(gp);
	    if ( iA != 0 ) {
	      double iAPrime = this->computeEikonalInternalLength_aPrime(gp);
	      double factor =  - iAPrime * (nonlocalCumulativePlasticStrain - localCumulativePlasticStrain) / ( iA * iA );           
	      answer.times( factor);
	    }
	  }
	  double pert = 1.e-8;
	  FloatArray strain;
	  strain = status->giveTempStrainVector();
	  strain.resizeWithValues(1);
	  FloatArray Ep(strain);
	  FloatArray S;
	  double lp;
	  Ep.at(1) += pert;
	  /// numerical stiffness
	  this->giveRealStressVectorGradientDamage(S, lp, gp, Ep, nonlocalCumulativePlasticStrain, tStep);
	  this->giveRealStressVectorGradientDamage(S, localCumulativePlasticStrain, gp, strain, nonlocalCumulativePlasticStrain, tStep);
	  double A = (lp - localCumulativePlasticStrain) / pert;
	  if(answer.at(1,1) - A > 1.e-5)
	    int ahoj = 1;

	}
	
      }
      break;      

    default:
        OOFEM_ERROR("mMode = %d not supported\n", mMode);
    }
}


void
RankineMatGrad :: giveGradientDamageStiffnessMatrix_du_BB(FloatMatrix &answer, MatResponseMode mode, GaussPoint *gp, TimeStep *tStep)
{
    if ( gradientDamageFormulationType == GDFT_Standard ) {
        answer.clear();
    } else if ( gradientDamageFormulationType == GDFT_Eikonal )  {
        MaterialMode mMode = gp->giveMaterialMode();
        switch ( mMode ) {
        case _PlaneStress:
	  {
            RankineMatGradStatus *status = static_cast< RankineMatGradStatus * >( this->giveStatus(gp) );
            if ( mode != TangentStiffness ) {
	      answer.clear();
	      return;
            }

            double tempKappa = status->giveTempCumulativePlasticStrain();
            double dKappa = tempKappa - status->giveCumulativePlasticStrain();
            if ( dKappa <= 0. ) {
	      answer.clear();
	      return;
            }

            FloatArray eta(3);
            double dkap1 = status->giveDKappa(1);
            double H = evalPlasticModulus(tempKappa);

            // evaluate in principal coordinates

            if ( dkap1 == 0. ) {
	      // regular case
	      double Estar = E / ( 1. - nu * nu );
	      double aux = Estar / ( H + Estar );
	      eta.at(1) = aux;
	      eta.at(2) = nu * aux;
	      eta.at(3) = 0.;
            } else {
	      // vertex case
	      double dkap2 = status->giveDKappa(2);
	      double denom = E * dkap1 + H * ( 1. - nu ) * ( dkap1 + dkap2 );
	      eta.at(1) = E * dkap1 / denom;
	      eta.at(2) = E * dkap2 / denom;
	      eta.at(3) = 0.;
            }

            // transform to global coordinates

            FloatArray sigPrinc(2);
            FloatMatrix nPrinc(2, 2);
            StressVector effStress(status->giveTempEffectiveStress(), _PlaneStress);
            effStress.computePrincipalValDir(sigPrinc, nPrinc);

            FloatMatrix T(3, 3);
            givePlaneStressVectorTranformationMtrx(T, nPrinc, true);
            FloatArray etaglob(3);
            etaglob.beProductOf(T, eta);

            FloatArray GradP = status->giveTempNonlocalCumulativePlasticStrainGrad();
            answer.beDyadicProductOf(GradP, etaglob);
            double iBPrime = this->computeEikonalInternalLength_bPrime(gp);
            double gPrime = this->computeDamageParamPrime(tempKappa);
            answer.times( iBPrime * gPrime * ( 1. - mParam ) );
	  }
	  break;
	
	case _1dMat:
	  {
	    RankineMatGradStatus *status = static_cast< RankineMatGradStatus * >( this->giveStatus(gp) );
	    answer.initFromVector(status->giveTempNonlocalCumulativePlasticStrainGrad(), false);
	    if ( mode == TangentStiffness ) {
	      double iBPrime = this->computeEikonalInternalLength_bPrime(gp);
	      double nonlocalCumulativePlasticStrain = status->giveTempNonlocalCumulativePlasticStrain();
	      double localCumulativePlasticStrain = status->giveTempCumulativePlasticStrain();
	      double overNonlocalCumulativePlasticStrain = mParam * nonlocalCumulativePlasticStrain + ( 1. - mParam ) * localCumulativePlasticStrain;
	      double gPrime = this->computeDamageParamPrime(overNonlocalCumulativePlasticStrain);
	      answer.times( iBPrime * gPrime * ( 1. - mParam ) * E / ( E + H0 ) );
	    } else {
	      answer.clear();
	    }
	  }
	  break;      
	  
        default:
            OOFEM_ERROR("mMode = %d not supported\n", mMode);
        }
    } else {
        OOFEM_WARNING("Unknown internalLengthDependenceType");
    }
}

void
RankineMatGrad :: giveGradientDamageStiffnessMatrix_dd_NN(FloatMatrix &answer, MatResponseMode mode, GaussPoint *gp, TimeStep *tStep)
{
  if ( gradientDamageFormulationType == GDFT_Standard ) {
    answer.clear();
  } else if ( gradientDamageFormulationType == GDFT_Eikonal )  {
    RankineMatGradStatus *status = static_cast< RankineMatGradStatus * >( this->giveStatus(gp) );
    answer.resize(1, 1);
    answer.zero();
    double iA = this->computeEikonalInternalLength_a(gp);
    
    if ( iA != 0 ) {
      answer.at(1, 1) += 1. / iA;
    }
    if ( mode == TangentStiffness ) {
      double localCumulativePlasticStrain = status->giveTempCumulativePlasticStrain();
      if ( localCumulativePlasticStrain > status->giveCumulativePlasticStrain() && iA != 0 ) {
	double iAPrime = this->computeEikonalInternalLength_aPrime(gp);
	double nonlocalCumulativePlasticStrain = status->giveTempNonlocalCumulativePlasticStrain();
	double overNonlocalCumulativePlasticStrain = mParam * nonlocalCumulativePlasticStrain + ( 1. - mParam ) * localCumulativePlasticStrain;
	
	double gPrime = this->computeDamageParamPrime(overNonlocalCumulativePlasticStrain);
	answer.at(1, 1) -= iAPrime / iA / iA * gPrime * mParam * ( nonlocalCumulativePlasticStrain - localCumulativePlasticStrain );
      }
    }
  } else {
    OOFEM_WARNING("Unknown internalLengthDependenceType");
  }
 
}

void
RankineMatGrad :: giveGradientDamageStiffnessMatrix_dd_BB(FloatMatrix &answer, MatResponseMode mode, GaussPoint *gp, TimeStep *tStep)
{
  int n = this->giveDimension(gp);
  answer.resize(n, n);
  answer.beUnitMatrix();
  if ( gradientDamageFormulationType == GDFT_Standard ) {
    answer.times(internalLength * internalLength);
  } else if ( gradientDamageFormulationType == GDFT_Eikonal ) {
    double iB = this->computeEikonalInternalLength_b(gp);
    answer.times(iB);
  } else {
    OOFEM_WARNING("Unknown internalLengthDependenceType");
  }
}

void
RankineMatGrad :: giveGradientDamageStiffnessMatrix_dd_BN(FloatMatrix &answer, MatResponseMode mode, GaussPoint *gp, TimeStep *tStep)
{
    if ( gradientDamageFormulationType == GDFT_Standard ) {
        answer.clear();
    } else if ( gradientDamageFormulationType == GDFT_Eikonal )  {
      RankineMatGradStatus *status = static_cast< RankineMatGradStatus * >( this->giveStatus(gp) );
      answer.initFromVector(status->giveTempNonlocalCumulativePlasticStrainGrad(), false);
      double iBPrime = this->computeEikonalInternalLength_bPrime(gp);
      double nonlocalCumulativePlasticStrain = status->giveTempNonlocalCumulativePlasticStrain();
      double localCumulativePlasticStrain = status->giveTempCumulativePlasticStrain();
      double overNonlocalCumulativePlasticStrain = mParam * nonlocalCumulativePlasticStrain + ( 1. - mParam ) * localCumulativePlasticStrain;
      double gPrime = this->computeDamageParamPrime(overNonlocalCumulativePlasticStrain);
      answer.times(iBPrime * gPrime * mParam);
    } else {
      OOFEM_WARNING("Unknown internalLengthDependenceType");
    }
}

void
RankineMatGrad :: givePlaneStressStiffMtrx(FloatMatrix &answer, MatResponseMode mode, GaussPoint *gp, TimeStep *tStep)
{
    RankineMatGradStatus *status = static_cast< RankineMatGradStatus * >( this->giveStatus(gp) );
    double tempDamage = status->giveTempDamage();
    double damage = status->giveDamage();
    double gprime;
    // Note:
    // The approximate solution of Helmholtz equation can lead
    // to very small but nonzero nonlocal kappa at some points that
    // are actually elastic. If such small values are positive,
    // they lead to a very small but nonzero damage. If this is
    // interpreted as "loading", the tangent terms are activated,
    // but damage will not actually grow at such points and the
    // convergence rate is slowed down. It is better to consider
    // such points as elastic.
    if ( tempDamage - damage <= negligible_damage ) {
        gprime = 0.;
    } else {
        double nonlocalCumulatedStrain = status->giveTempNonlocalCumulativePlasticStrain();
        double tempLocalCumulatedStrain = status->giveTempCumulativePlasticStrain();
        double overNonlocalCumulatedStrain = mParam * nonlocalCumulatedStrain + ( 1. - mParam ) * tempLocalCumulatedStrain;
        gprime = computeDamageParamPrime(overNonlocalCumulatedStrain);
        gprime *= ( 1. - mParam );
    }

    evaluatePlaneStressStiffMtrx(answer, mode, gp, tStep, gprime);
}


void
RankineMatGrad :: giveNonlocalInternalForces_N_factor(double &answer, double nlDamageDrivingVariable, GaussPoint *gp, TimeStep *tStep)
{
    // I modified this one to put pnl-p instead of just pnl
    RankineMatGradStatus *status = static_cast< RankineMatGradStatus * >( this->giveStatus(gp) );
    double localCumulativePlasticStrain = status->giveTempCumulativePlasticStrain();
    double nonlocalCumulativePlasticStrain = status->giveTempNonlocalCumulativePlasticStrain();
    answer = nonlocalCumulativePlasticStrain - localCumulativePlasticStrain;
    if ( gradientDamageFormulationType == GDFT_Eikonal ) {
        double iA = this->computeEikonalInternalLength_a(gp);
        if ( iA != 0 ) {
            answer = answer / iA;
        }
    }
}

void
RankineMatGrad :: giveNonlocalInternalForces_B_factor(FloatArray &answer, const FloatArray &nlDamageDrivingVariable_grad, GaussPoint *gp, TimeStep *tStep)
{
    answer = nlDamageDrivingVariable_grad;
    if ( gradientDamageFormulationType == GDFT_Eikonal ) {
        double iB = this->computeEikonalInternalLength_b(gp);
        answer.times(iB);
    } else {
        answer.times(internalLength * internalLength);
    }
}

void
RankineMatGrad :: computeLocalDamageDrivingVariable(double &answer, GaussPoint *gp, TimeStep *tStep)
{
    RankineMatGradStatus *status = static_cast< RankineMatGradStatus * >( this->giveStatus(gp) );
    answer =  status->giveTempCumulativePlasticStrain();
}

void
RankineMatGrad :: giveRealStressVectorGradientDamage(FloatArray &stress, double &localCumulativePlasticStrain, GaussPoint *gp, const FloatArray &totalStrain, double nonlocalCumulativePlasticStrain, TimeStep *tStep)
{
    RankineMatGradStatus *status = static_cast< RankineMatGradStatus * >( this->giveStatus(gp) );

    this->initTempStatus(gp);

    double tempDamage;
    RankineMat :: performPlasticityReturn(gp, totalStrain);
    status->setTempNonlocalCumulativePlasticStrain(nonlocalCumulativePlasticStrain);
    localCumulativePlasticStrain = status->giveTempCumulativePlasticStrain();
    tempDamage = computeDamage(gp, tStep);
    const FloatArray &tempEffStress = status->giveTempEffectiveStress();
    stress.beScaled(1.0 - tempDamage, tempEffStress);


    status->letTempStrainVectorBe(totalStrain);
    status->setTempDamage(tempDamage);
    status->letTempEffectiveStressBe(tempEffStress);
    status->letTempStressVectorBe(stress);
#ifdef keep_track_of_dissipated_energy
    double gf = sig0 * sig0 / E; // only estimated, but OK for this purpose
    MaterialMode mMode = gp->giveMaterialMode();
    switch ( mMode ) {
    case _PlaneStress:
      status->computeWork_PlaneStress(gp, gf);
      break;
    default:
      int i  = 15;
    }
#endif
    double khat = mParam * nonlocalCumulativePlasticStrain + ( 1. - mParam ) * localCumulativePlasticStrain;
    status->setKappa_hat(khat);
}

void
RankineMatGrad :: computeCumPlastStrain(double &kappa, GaussPoint *gp, TimeStep *tStep)
{
    RankineMatGradStatus *status = static_cast< RankineMatGradStatus * >( this->giveStatus(gp) );
    double localCumPlastStrain = status->giveTempCumulativePlasticStrain();
    double nlCumPlastStrain = status->giveTempNonlocalCumulativePlasticStrain();
    kappa = mParam * nlCumPlastStrain + ( 1. - mParam ) * localCumPlastStrain;
}


  
int
RankineMatGrad :: giveIPValue(FloatArray &answer, GaussPoint *gp, InternalStateType type, TimeStep *tStep)
{
    if ( type == IST_CumPlasticStrain_2 ) {
        answer.resize(1);
	 RankineMatGradStatus *status = static_cast< RankineMatGradStatus * >( this->giveStatus(gp) );
        answer.at(1) = status->giveTempNonlocalCumulativePlasticStrain();
        return 1;
    } else if ( type == IST_MaxEquivalentStrainLevel ) {
        answer.resize(1);
        computeCumPlastStrain(answer.at(1), gp, tStep);
        return 1;
    } else {
        return RankineMat :: giveIPValue(answer, gp, type, tStep);
    }
}


double
RankineMatGrad :: computeEikonalInternalLength_a(GaussPoint *gp)
{
    RankineMatGradStatus *status = static_cast< RankineMatGradStatus * >( this->giveStatus(gp) );

    double damage = status->giveTempDamage();
    return sqrt(1. - damage) * internalLength;
}

double
RankineMatGrad :: computeEikonalInternalLength_b(GaussPoint *gp)
{
    RankineMatGradStatus *status = static_cast< RankineMatGradStatus * >( this->giveStatus(gp) );

    double damage = status->giveTempDamage();
    return sqrt(1. - damage) * internalLength;
}


double
RankineMatGrad :: computeEikonalInternalLength_aPrime(GaussPoint *gp)
{
    RankineMatGradStatus *status = static_cast< RankineMatGradStatus * >( this->giveStatus(gp) );

    double damage = status->giveTempDamage();
    return -0.5 / sqrt(1. - damage) * internalLength;
}

double
RankineMatGrad :: computeEikonalInternalLength_bPrime(GaussPoint *gp)
{
    RankineMatGradStatus *status = static_cast< RankineMatGradStatus * >( this->giveStatus(gp) );

    double damage = status->giveTempDamage();
    return -0.5 / sqrt(1. - damage) * internalLength;
}

int
RankineMatGrad :: giveDimension(GaussPoint *gp)
{
    if ( gp->giveMaterialMode() == _1dMat ) {
        return 1;
    } else if ( gp->giveMaterialMode() == _PlaneStress ) {
        return 2;
    } else if ( gp->giveMaterialMode() == _PlaneStrain ) {
        return 3;
    } else if ( gp->giveMaterialMode() == _3dMat ) {
        return 3;
    } else {
        return 0;
    }
}



//=============================================================================
// GRADIENT RANKINE MATERIAL STATUS
//=============================================================================


RankineMatGradStatus :: RankineMatGradStatus(int n, Domain *d, GaussPoint *g) :
  RankineMatStatus(n, d, g)
{
  nonlocalCumulativePlasticStrain = 0;
  tempNonlocalCumulativePlasticStrain = 0;
}

void
RankineMatGradStatus :: printOutputAt(FILE *file, TimeStep *tStep)
{
    StructuralMaterialStatus :: printOutputAt(file, tStep);

    fprintf(file, "status {");
    fprintf(file, "damage %g, kappa %g, kappa_nl %g, kappa_hat %g", damage, kappa, nonlocalCumulativePlasticStrain, kappa_hat);
#ifdef keep_track_of_dissipated_energy
    fprintf(file, ", dissW %g, freeE %g, stressW %g", this->dissWork, ( this->stressWork ) - ( this->dissWork ), this->stressWork);
#endif
    fprintf(file, " }\n");
}


void
RankineMatGradStatus :: initTempStatus()
{
    RankineMatStatus :: initTempStatus();
    tempNonlocalCumulativePlasticStrain =  nonlocalCumulativePlasticStrain;
    kappa_hat = 0.;  
}


void
RankineMatGradStatus :: updateYourself(TimeStep *tStep)
{
    RankineMatStatus :: updateYourself(tStep);
    nonlocalCumulativePlasticStrain = tempNonlocalCumulativePlasticStrain;
}
  
} // end namespace oofem
