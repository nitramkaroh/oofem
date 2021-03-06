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
 *               Copyright (C) 1993 - 2015   Borek Patzak
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

#include "../sm/Materials/Micromorphic/GradientPolyconvex/gradientpolyconvexmaterialold.h"
#include "gausspoint.h"
#include "floatmatrix.h"
#include "floatarray.h"
#include "mathfem.h"
#include "error.h"
#include "classfactory.h"


namespace oofem {
  REGISTER_Material(GradientPolyconvexMaterialOld);

GradientPolyconvexMaterialOld :: GradientPolyconvexMaterialOld(int n, Domain *d) :IsotropicLinearElasticMaterial(n, d), MicromorphicMaterialExtensionInterface(d)
{
  Hk = Ak = 0.;
  alpha = 1.e3;
  gamma = 1.e-5;
}

GradientPolyconvexMaterialOld :: ~GradientPolyconvexMaterialOld()
{ }



void
GradientPolyconvexMaterialOld :: giveStiffnessMatrix(FloatMatrix &answer, MatResponseMode rMode, GaussPoint *gp, TimeStep *tStep)
//
// Returns characteristic material stiffness matrix of the receiver
//
{
    OOFEM_ERROR("Shouldn't be called.");
}



void
GradientPolyconvexMaterialOld :: giveMicromorphicMatrix_dSigdUgrad(FloatMatrix &answer, MatResponseMode mode, GaussPoint *gp, TimeStep *tStep)
{
    GradientPolyconvexMaterialOldStatus *status = static_cast< GradientPolyconvexMaterialOldStatus * >( this->giveStatus(gp) );

    answer.resize(9,9);
    answer.zero();
    //deformation gradient, its inverse, cofactor, and determinant
    double normC_tC1, normC_tC2;
    FloatArray vF, vCofF, vInvF, vB, delta, vC_tC1, vC_tC2;
    FloatMatrix F, invF, cofF, B, C, C_tC1, C_tC2, dCdF;
    delta = {1, 1, 1, 0, 0, 0, 0, 0, 0};
    F.beMatrixForm(status->giveTempFVector());
    B.beProductTOf(F,F);
    vB.beVectorForm(B);
    invF.beInverseOf(F);
    double J = F.giveDeterminant();
    cofF.beTranspositionOf(invF);
    cofF.times(J);
    vF.beVectorForm(F);
    vCofF.beVectorForm(cofF);
    vInvF.beVectorForm(invF);

    ///////////
    C.beTProductOf(F,F);

    FloatMatrix tC1, tC2;
    tC1 = this->givetC1(tStep);
    tC2 = this->givetC2(tStep);
    
    C_tC1 = C;
    C_tC1.subtract(tC1);

    C_tC2 = C;
    C_tC2.subtract(tC2);
    

    normC_tC1 = C_tC1.computeFrobeniusNorm();
    normC_tC2 = C_tC2.computeFrobeniusNorm();

    vC_tC1.beVectorForm(C_tC1);
    vC_tC2.beVectorForm(C_tC2);
    this->compute_dC_dF(dCdF,vF);
    

    
    // relative stress for cofactor
    FloatArray s, micromorphicVar, mvCof;
    mvCof = status->giveTempMicromorphicVar();
    s = vCofF;
    s.subtract(mvCof);
    s.times(-Hk);
   

    FloatArray vC_tC1_dCdF, vC_tC2_dCdF;
    vC_tC1_dCdF.beTProductOf(dCdF, vC_tC1);
    vC_tC2_dCdF.beTProductOf(dCdF, vC_tC2);

    
    for (int i = 1; i <= 3; i++) {
      for (int j = 1; j <= 3; j++) {
	for (int p = 1; p <= 3; p++) {
	  for (int q = 1; q <= 3; q++) {
	    // double well material stiffness
	    answer.at(giveVI(i,j),giveVI(p,q)) += alpha * (4. * ( vB.at(giveVI(i,p)) * delta.at(giveVI(j,q)) + vF.at(giveVI(i,q)) *  vF.at(giveVI(p,j))) * (normC_tC1*normC_tC1 + normC_tC2 * normC_tC2) + 4. * vC_tC1.at(giveVI(j,q)) * delta.at(giveVI(i,p)) * (normC_tC2 * normC_tC2) + 4. * vC_tC2.at(giveVI(j,q)) * delta.at(giveVI(i,p)) * (normC_tC1 * normC_tC1) + 4.*vC_tC1_dCdF.at(giveVI(i,j))*vC_tC2_dCdF.at(giveVI(p,q)) + 4.*vC_tC2_dCdF.at(giveVI(i,j))*vC_tC1_dCdF.at(giveVI(p,q)) );
	    /// part with J^-1
	    //answer.at(giveVI(i,j),giveVI(p,q)) += gamma/J * ( vInvF.at(giveVI(j,i)) *  vInvF.at(giveVI(q,p)) + vInvF.at(giveVI(j,p)) *  vInvF.at(giveVI(q,i)) ) ;


	    //new
	    //answer.at(giveVI(i,j),giveVI(p,q)) += gamma * ( vInvF.at(giveVI(j,i)) *  vInvF.at(giveVI(q,p)) - log(J) *  vInvF.at(giveVI(j,q)) *  vInvF.at(giveVI(p,i)) ) ;


	    /// penalty on negative volume
	    /*	    if(J < 0) {
	      double penalty = 1.e15;
	      answer.at(giveVI(i,j),giveVI(p,q)) += penalty * (J*J * (1 + J) * vInvF.at(giveVI(j,i)) *  vInvF.at(giveVI(q,p))  + J*J * vInvF.at(giveVI(j,p)) *  vInvF.at(giveVI(q,i)));
	      }*/


	    
	  }
	}
      }
    }




    FloatMatrix answer1(9,9), answer2(9,9), answer3(9,9), answer4(9,9), answer5(9,9), answer6(9,9), CFCF(9,9);


    for (int i = 1; i <= 3; i++) {
      for (int j = 1; j <= 3; j++) {
	for (int p = 1; p <= 3; p++) {
	  for (int q = 1; q <= 3; q++) {
	    // double well material stiffness
	    CFCF.at(giveVI(i,j),giveVI(p,q)) += 2. * (vB.at(giveVI(i,p))*delta.at(giveVI(j,q)) + vF.at(giveVI(i,q)) *  vF.at(giveVI(p,j)));
	    answer1.at(giveVI(i,j),giveVI(p,q)) += 4. * (vB.at(giveVI(i,p))*delta.at(giveVI(j,q)) + vF.at(giveVI(i,q)) *  vF.at(giveVI(p,j))) * (normC_tC1*normC_tC1);
	    answer2.at(giveVI(i,j),giveVI(p,q)) += 4. * (vB.at(giveVI(i,p))*delta.at(giveVI(j,q)) + vF.at(giveVI(i,q)) *  vF.at(giveVI(p,j))) * (normC_tC2 * normC_tC2);
	    answer3.at(giveVI(i,j),giveVI(p,q)) += 4. * vC_tC1.at(giveVI(j,q)) * delta.at(giveVI(i,p)) * (normC_tC2 * normC_tC2);
	    answer4.at(giveVI(i,j),giveVI(p,q)) += 4. * vC_tC2.at(giveVI(j,q)) * delta.at(giveVI(i,p)) * (normC_tC1 * normC_tC1);
	    answer5.at(giveVI(i,j),giveVI(p,q)) += 4. * vC_tC1_dCdF.at(giveVI(i,j))*vC_tC2_dCdF.at(giveVI(p,q));
	    answer6.at(giveVI(i,j),giveVI(p,q)) += 4. * vC_tC2_dCdF.at(giveVI(i,j))*vC_tC1_dCdF.at(giveVI(p,q));
	  }
	}
      }
    }

 
    
    







    
    answer1.zero(),answer2.zero(),answer3.zero();
    // micromorphic terms
    for (int i = 1; i <= 3; i++) {
      for (int j = 1; j <= 3; j++) {
	for (int o = 1; o <= 3; o++) {
	  for (int p = 1; p <= 3; p++) {
	    for (int m = 1; m <= 3; m++) {
	      for (int n = 1; n <= 3; n++) {		  
		answer.at(giveVI(i,j),giveVI(o,p)) += - J*s.at(giveVI(m,n)) * (vInvF.at(giveVI(j,i))*vInvF.at(giveVI(n,m))-vInvF.at(giveVI(n,i))*vInvF.at(giveVI(j,m)))*vInvF.at(giveVI(p,o));
		answer1.at(giveVI(i,j),giveVI(o,p)) += - J*s.at(giveVI(m,n)) * (vInvF.at(giveVI(j,i))*vInvF.at(giveVI(n,m))-vInvF.at(giveVI(n,i))*vInvF.at(giveVI(j,m)))*vInvF.at(giveVI(p,o));
		answer.at(giveVI(i,j),giveVI(o,p)) += Hk * J * J *( vInvF.at(giveVI(p,o)) * vInvF.at(giveVI(n,m)) - vInvF.at(giveVI(n,o)) * vInvF.at(giveVI(p,m)) ) * (vInvF.at(giveVI(j,i))*vInvF.at(giveVI(n,m))-vInvF.at(giveVI(n,i))*vInvF.at(giveVI(j,m)));
		answer2.at(giveVI(i,j),giveVI(o,p)) += Hk * J * J *( vInvF.at(giveVI(p,o)) * vInvF.at(giveVI(n,m)) - vInvF.at(giveVI(n,o)) * vInvF.at(giveVI(p,m)) ) * (vInvF.at(giveVI(j,i))*vInvF.at(giveVI(n,m))-vInvF.at(giveVI(n,i))*vInvF.at(giveVI(j,m)));
		answer.at(giveVI(i,j),giveVI(o,p)) += - J * s.at(giveVI(m,n))* (-vInvF.at(giveVI(j,o))*vInvF.at(giveVI(p,i))*vInvF.at(giveVI(n,m))-vInvF.at(giveVI(j,i))*vInvF.at(giveVI(n,o))*vInvF.at(giveVI(p,m))+vInvF.at(giveVI(n,o))*vInvF.at(giveVI(p,i))*vInvF.at(giveVI(j,m))+vInvF.at(giveVI(n,i))*vInvF.at(giveVI(j,o))*vInvF.at(giveVI(p,m)));
		answer3.at(giveVI(i,j),giveVI(o,p)) += - J * s.at(giveVI(m,n))* (-vInvF.at(giveVI(j,o))*vInvF.at(giveVI(p,i))*vInvF.at(giveVI(n,m))-vInvF.at(giveVI(j,i))*vInvF.at(giveVI(n,o))*vInvF.at(giveVI(p,m))+vInvF.at(giveVI(n,o))*vInvF.at(giveVI(p,i))*vInvF.at(giveVI(j,m))+vInvF.at(giveVI(n,i))*vInvF.at(giveVI(j,o))*vInvF.at(giveVI(p,m)));
	      }
	    }
	  }
	}
      }
    }

    if(gp->giveMaterialMode() == _PlaneStrain) {
      FloatMatrix m3d = answer;
      answer.resize(5,5);
      answer.zero();
      answer.at(1, 1) = m3d.at(1, 1);
      answer.at(1, 2) = m3d.at(1, 2);
      answer.at(1, 3) = m3d.at(1, 3);
      answer.at(1, 4) = m3d.at(1, 6);
      answer.at(1, 5) = m3d.at(1, 9);
      
      answer.at(2, 1) = m3d.at(2, 1);
      answer.at(2, 2) = m3d.at(2, 2);
      answer.at(2, 3) = m3d.at(2, 3);
      answer.at(2, 4) = m3d.at(2, 6);
      answer.at(2, 5) = m3d.at(2, 9);
      
      answer.at(3, 1) = m3d.at(3, 1);
      answer.at(3, 2) = m3d.at(3, 2);
      answer.at(3, 3) = m3d.at(3, 3);
      answer.at(3, 4) = m3d.at(3, 6);
      answer.at(3, 5) = m3d.at(3, 9);
      
      answer.at(4, 1) = m3d.at(6, 1);
      answer.at(4, 2) = m3d.at(6, 2);
      answer.at(4, 3) = m3d.at(6, 3);
      answer.at(4, 4) = m3d.at(6, 6);
      answer.at(4, 5) = m3d.at(6, 9);
      
      answer.at(5, 1) = m3d.at(9, 1);
      answer.at(5, 2) = m3d.at(9, 2);
      answer.at(5, 3) = m3d.at(9, 3);
      answer.at(5, 4) = m3d.at(9, 6);
      answer.at(5, 5) = m3d.at(9, 9);
    }

    /*  FloatMatrix testA(9,9);
    FloatArray vP, pvP, M, pertF, ps, reducedvF, reducedMicromorphicVar(9), micromorphicVarGrad(1);
    FloatArray col;
    double e = 1.e-6;
    reducedvF.beVectorForm(F);
    reducedMicromorphicVar = status->giveTempMicromorphicVar();
    this->giveFiniteStrainGeneralizedStressVectors_3d(vP, s, M, gp, reducedvF, reducedMicromorphicVar, micromorphicVarGrad, tStep);
    pertF = reducedvF;
    pertF.at(1) += e;
    this->giveFiniteStrainGeneralizedStressVectors_3d(pvP, s, M, gp, pertF, reducedMicromorphicVar, micromorphicVarGrad, tStep);
    col = pvP;
    col.subtract(vP);
    col.times(1./e);
    testA.setColumn(col, 1);
    pertF = reducedvF;
    pertF.at(2) += e;
    this->giveFiniteStrainGeneralizedStressVectors_3d(pvP, s, M, gp, pertF, reducedMicromorphicVar, micromorphicVarGrad, tStep);
    col = pvP;
    col.subtract(vP);
    col.times(1./e);
    testA.setColumn(col, 2);
    pertF = reducedvF;
    pertF.at(3) += e;
    this->giveFiniteStrainGeneralizedStressVectors_3d(pvP, s, M, gp, pertF, reducedMicromorphicVar, micromorphicVarGrad, tStep);
    col = pvP;
    col.subtract(vP);
    col.times(1./e);
    testA.setColumn(col, 3);
    pertF = reducedvF;
    pertF.at(4) += e;
    this->giveFiniteStrainGeneralizedStressVectors_3d(pvP, s, M, gp, pertF, reducedMicromorphicVar, micromorphicVarGrad, tStep);
    col = pvP;
    col.subtract(vP);
    col.times(1./e);
    testA.setColumn(col, 4);
    pertF = reducedvF;
    pertF.at(5) += e;
    this->giveFiniteStrainGeneralizedStressVectors_3d(pvP, s, M, gp, pertF, reducedMicromorphicVar, micromorphicVarGrad, tStep);
    col = pvP;
    col.subtract(vP);
    col.times(1./e);
    testA.setColumn(col, 5);
    pertF = reducedvF;
    pertF.at(6) += e;
    this->giveFiniteStrainGeneralizedStressVectors_3d(vP, s, M, gp, pertF, reducedMicromorphicVar, micromorphicVarGrad, tStep);
    col = pvP;
    col.subtract(vP);
    col.times(1./e);
    testA.setColumn(col, 6);
    pertF = reducedvF;
    pertF.at(7) += e;
    this->giveFiniteStrainGeneralizedStressVectors_3d(pvP, s, M, gp, pertF, reducedMicromorphicVar, micromorphicVarGrad, tStep);
    col = pvP;
    col.subtract(vP);
    col.times(1./e);
    testA.setColumn(col, 7);
    pertF = reducedvF;
    pertF.at(8) += e;
    this->giveFiniteStrainGeneralizedStressVectors_3d(pvP, s, M, gp, pertF, reducedMicromorphicVar, micromorphicVarGrad, tStep);
    col = pvP;
    col.subtract(vP);
    col.times(1./e);
    testA.setColumn(col, 8);
    pertF = reducedvF;
    pertF.at(9) += e;
    this->giveFiniteStrainGeneralizedStressVectors_3d(pvP, s, M, gp, pertF, reducedMicromorphicVar, micromorphicVarGrad, tStep);
    col = pvP;
    col.subtract(vP);
    col.times(1./e);
    testA.setColumn(col, 9);
    pertF = reducedvF;    
    this->giveFiniteStrainGeneralizedStressVectors_3d(pvP, s, M, gp, reducedvF, reducedMicromorphicVar, micromorphicVarGrad, tStep);
    

    FloatMatrix m3d = testA;
    testA.resize(5,5);
    
    testA.zero();
    testA.at(1, 1) = m3d.at(1, 1);
    testA.at(1, 2) = m3d.at(1, 2);
    testA.at(1, 3) = m3d.at(1, 3);
    testA.at(1, 4) = m3d.at(1, 6);
    testA.at(1, 5) = m3d.at(1, 9);
    
    testA.at(2, 1) = m3d.at(2, 1);
    testA.at(2, 2) = m3d.at(2, 2);
    testA.at(2, 3) = m3d.at(2, 3);
    testA.at(2, 4) = m3d.at(2, 6);
    testA.at(2, 5) = m3d.at(2, 9);
    
    testA.at(3, 1) = m3d.at(3, 1);
    testA.at(3, 2) = m3d.at(3, 2);
    testA.at(3, 3) = m3d.at(3, 3);
    testA.at(3, 4) = m3d.at(3, 6);
    testA.at(3, 5) = m3d.at(3, 9);
    
    testA.at(4, 1) = m3d.at(6, 1);
    testA.at(4, 2) = m3d.at(6, 2);
    testA.at(4, 3) = m3d.at(6, 3);
    testA.at(4, 4) = m3d.at(6, 6);
    testA.at(4, 5) = m3d.at(6, 9);
    
    testA.at(5, 1) = m3d.at(9, 1);
    testA.at(5, 2) = m3d.at(9, 2);
    testA.at(5, 3) = m3d.at(9, 3);
    testA.at(5, 4) = m3d.at(9, 6);
    testA.at(5, 5) = m3d.at(9, 9);
    */
    
  
}

void
GradientPolyconvexMaterialOld :: giveMicromorphicMatrix_dSigdPhi(FloatMatrix &answer, MatResponseMode mode, GaussPoint *gp, TimeStep *tStep)
{


    GradientPolyconvexMaterialOldStatus *status = static_cast< GradientPolyconvexMaterialOldStatus* >( this->giveStatus(gp) );

    FloatArray vInvF;
    FloatMatrix F, invF;
    F.beMatrixForm(status->giveTempFVector());
    invF.beInverseOf(F);
    double J = F.giveDeterminant();
    vInvF.beVectorForm(invF);
    
    answer.resize(9,9);
    //answer.setColumn(vInvF, 1);
    for (int i = 1; i <= 3; i++) {
      for (int j = 1; j <= 3; j++) {
	for (int o = 1; o <= 3; o++) {
	  for (int p = 1; p <= 3; p++) {
	    answer.at(giveVI(i,j),giveVI(o,p)) = (vInvF.at(giveVI(j,i))*vInvF.at(giveVI(p,o))-vInvF.at(giveVI(p,i))*vInvF.at(giveVI(j,o)));
	  }
	}
      }
    }
   answer.times(-J*Hk);   
   if(gp->giveMaterialMode() == _PlaneStrain) {
     FloatMatrix m3d = answer;
     answer.resize(5,5);
     answer.zero();
     /*

     answer.at(1, 1) = m3d.at(1, 1);
     answer.at(1, 2) = m3d.at(1, 2);
     answer.at(1, 4) = m3d.at(1, 6);
     answer.at(1, 5) = m3d.at(1, 9);
      
     answer.at(2, 1) = m3d.at(2, 1);
     answer.at(2, 2) = m3d.at(2, 2);
     answer.at(2, 4) = m3d.at(2, 6);
     answer.at(2, 5) = m3d.at(2, 9);
      
     answer.at(4, 1) = m3d.at(6, 1);
     answer.at(4, 2) = m3d.at(6, 2);
     answer.at(4, 4) = m3d.at(6, 6);
     answer.at(4, 5) = m3d.at(6, 9);
      
     answer.at(5, 1) = m3d.at(9, 1);
     answer.at(5, 2) = m3d.at(9, 2);
     answer.at(5, 4) = m3d.at(9, 6);
     answer.at(5, 5) = m3d.at(9, 9);
*/
       
     answer.at(1, 1) = m3d.at(1, 1);
     answer.at(1, 2) = m3d.at(1, 2);
     answer.at(1, 3) = m3d.at(1, 3);
     answer.at(1, 4) = m3d.at(1, 6);
     answer.at(1, 5) = m3d.at(1, 9);
     
     answer.at(2, 1) = m3d.at(2, 1);
     answer.at(2, 2) = m3d.at(2, 2);
     answer.at(2, 3) = m3d.at(2, 3);
     answer.at(2, 4) = m3d.at(2, 6);
     answer.at(2, 5) = m3d.at(2, 9);
     
     answer.at(3, 1) = m3d.at(3, 1);
     answer.at(3, 2) = m3d.at(3, 2);
     answer.at(3, 3) = m3d.at(3, 3);
     answer.at(3, 4) = m3d.at(3, 6);
     answer.at(3, 5) = m3d.at(3, 9);
     
     answer.at(4, 1) = m3d.at(6, 1);
     answer.at(4, 2) = m3d.at(6, 2);
     answer.at(4, 3) = m3d.at(6, 3);
     answer.at(4, 4) = m3d.at(6, 6);
     answer.at(4, 5) = m3d.at(6, 9);
     
     answer.at(5, 1) = m3d.at(9, 1);
     answer.at(5, 2) = m3d.at(9, 2);
     answer.at(5, 3) = m3d.at(9, 3);
     answer.at(5, 4) = m3d.at(9, 6);
     answer.at(5, 5) = m3d.at(9, 9);
     
   }
   
   
   /*
   
   FloatMatrix testA(9,9);
   FloatArray vP, pvP, M, pertF, ps, s, reducedvF, reducedMicromorphicVar(9), micromorphicVarGrad(1);
   FloatArray col;
   double e = 1.e-6;
   reducedvF.beVectorForm(F);
   reducedMicromorphicVar = status->giveTempMicromorphicVar();
   this->giveFiniteStrainGeneralizedStressVectors_3d(vP, s, M, gp, reducedvF, reducedMicromorphicVar, micromorphicVarGrad, tStep);
   pertF = reducedvF;
   pertF.at(1) += e;
   this->giveFiniteStrainGeneralizedStressVectors_3d(pvP, ps, M, gp, pertF, reducedMicromorphicVar, micromorphicVarGrad, tStep);
   col = ps;
   col.subtract(s);
   col.times(1./e);
   testA.setColumn(col, 1);
   pertF = reducedvF;
   pertF.at(2) += e;
   this->giveFiniteStrainGeneralizedStressVectors_3d(pvP,ps, M, gp, pertF, reducedMicromorphicVar, micromorphicVarGrad, tStep);
   col = ps;
   col.subtract(s);
   col.times(1./e);
   testA.setColumn(col, 2);
   pertF = reducedvF;
   pertF.at(3) += e;
   this->giveFiniteStrainGeneralizedStressVectors_3d(pvP, ps, M, gp, pertF, reducedMicromorphicVar, micromorphicVarGrad, tStep);
   col = ps;
   col.subtract(s);
   col.times(1./e);
   testA.setColumn(col, 3);
   pertF = reducedvF;
   pertF.at(4) += e;
   this->giveFiniteStrainGeneralizedStressVectors_3d(pvP, ps, M, gp, pertF, reducedMicromorphicVar, micromorphicVarGrad, tStep);
   col = ps;
   col.subtract(s);
   col.times(1./e);
   testA.setColumn(col, 4);
   pertF = reducedvF;
   pertF.at(5) += e;
   this->giveFiniteStrainGeneralizedStressVectors_3d(pvP, ps, M, gp, pertF, reducedMicromorphicVar, micromorphicVarGrad, tStep);
   col = ps;
   col.subtract(s);
   col.times(1./e);
   testA.setColumn(col, 5);
   pertF = reducedvF;
   pertF.at(6) += e;
   this->giveFiniteStrainGeneralizedStressVectors_3d(pvP, ps, M, gp, pertF, reducedMicromorphicVar, micromorphicVarGrad, tStep);
   col = ps;
   col.subtract(s);
   col.times(1./e);
   testA.setColumn(col, 6);
   pertF = reducedvF;
   pertF.at(7) += e;
   this->giveFiniteStrainGeneralizedStressVectors_3d(pvP, ps, M, gp, pertF, reducedMicromorphicVar, micromorphicVarGrad, tStep);
   col = ps;
   col.subtract(s);
   col.times(1./e);
   testA.setColumn(col, 7);
   pertF = reducedvF;
   pertF.at(8) += e;
   this->giveFiniteStrainGeneralizedStressVectors_3d(pvP, ps, M, gp, pertF, reducedMicromorphicVar, micromorphicVarGrad, tStep);
   col = ps;
   col.subtract(s);
   col.times(1./e);
   testA.setColumn(col, 8);
   pertF = reducedvF;
   pertF.at(9) += e;
   this->giveFiniteStrainGeneralizedStressVectors_3d(pvP, ps, M, gp, pertF, reducedMicromorphicVar, micromorphicVarGrad, tStep);
   col = ps;
   col.subtract(s);
   col.times(1./e);
   testA.setColumn(col, 9);
   pertF = reducedvF;    
   this->giveFiniteStrainGeneralizedStressVectors_3d(pvP, ps, M, gp, reducedvF, reducedMicromorphicVar, micromorphicVarGrad, tStep);


   FloatMatrix m3d = testA;
   testA.resize(5,5);
    
   testA.zero();
   testA.at(1, 1) = m3d.at(1, 1);
   testA.at(1, 2) = m3d.at(1, 2);
   testA.at(1, 4) = m3d.at(1, 6);
   testA.at(1, 5) = m3d.at(1, 9);
    
   testA.at(2, 1) = m3d.at(2, 1);
   testA.at(2, 2) = m3d.at(2, 2);
   testA.at(2, 4) = m3d.at(2, 6);
   testA.at(2, 5) = m3d.at(2, 9);
    
   testA.at(3, 1) = m3d.at(3, 1);
   testA.at(3, 2) = m3d.at(3, 2);
   testA.at(3, 4) = m3d.at(3, 6);
   testA.at(3, 5) = m3d.at(3, 9);
    
   testA.at(4, 1) = m3d.at(6, 1);
   testA.at(4, 2) = m3d.at(6, 2);
   testA.at(4, 4) = m3d.at(6, 6);
   testA.at(4, 5) = m3d.at(6, 9);
    
   testA.at(5, 1) = m3d.at(9, 1);
   testA.at(5, 2) = m3d.at(9, 2);
   testA.at(5, 4) = m3d.at(9, 6);
   testA.at(5, 5) = m3d.at(9, 9);
   */

   //   answer.zero();
   
  
}


void
GradientPolyconvexMaterialOld :: giveMicromorphicMatrix_dSdUgrad(FloatMatrix &answer, MatResponseMode mode, GaussPoint *gp, TimeStep *tStep)
{
    GradientPolyconvexMaterialOldStatus *status = static_cast< GradientPolyconvexMaterialOldStatus* >( this->giveStatus(gp) );
  
    FloatArray vInvF;
    FloatMatrix F, invF;
    F.beMatrixForm(status->giveTempFVector());
    invF.beInverseOf(F);
    double J = F.giveDeterminant();
    vInvF.beVectorForm(invF);
    
    answer.resize(9,9);
    answer.setColumn(vInvF, 1);
    for (int i = 1; i <= 3; i++) {
      for (int j = 1; j <= 3; j++) {
	for (int o = 1; o <= 3; o++) {
	  for (int p = 1; p <= 3; p++) {
	    answer.at(giveVI(i,j),giveVI(o,p)) = (vInvF.at(giveVI(j,i))*vInvF.at(giveVI(p,o))-vInvF.at(giveVI(p,i))*vInvF.at(giveVI(j,o)));
	  }
	}
      }
    }
    answer.times(-J*Hk);   
     if(gp->giveMaterialMode() == _PlaneStrain) {
       
      FloatMatrix m3d = answer;
      answer.resize(5,5);
      answer.zero();

      
      answer.at(1, 1) = m3d.at(1, 1);
      answer.at(1, 2) = m3d.at(1, 2);
      answer.at(1, 3) = m3d.at(1, 3);
      answer.at(1, 4) = m3d.at(1, 6);
      answer.at(1, 5) = m3d.at(1, 9);
     
      answer.at(2, 1) = m3d.at(2, 1);
      answer.at(2, 2) = m3d.at(2, 2);
      answer.at(2, 3) = m3d.at(2, 3);
      answer.at(2, 4) = m3d.at(2, 6);
      answer.at(2, 5) = m3d.at(2, 9);
     
      answer.at(3, 1) = m3d.at(3, 1);
      answer.at(3, 2) = m3d.at(3, 2);
      answer.at(3, 3) = m3d.at(3, 3);
      answer.at(3, 4) = m3d.at(3, 6);
      answer.at(3, 5) = m3d.at(3, 9);
     
      answer.at(4, 1) = m3d.at(6, 1);
      answer.at(4, 2) = m3d.at(6, 2);
      answer.at(4, 3) = m3d.at(6, 3);
      answer.at(4, 4) = m3d.at(6, 6);
      answer.at(4, 5) = m3d.at(6, 9);
     
      answer.at(5, 1) = m3d.at(9, 1);
      answer.at(5, 2) = m3d.at(9, 2);
      answer.at(5, 3) = m3d.at(9, 3);
      answer.at(5, 4) = m3d.at(9, 6);
      answer.at(5, 5) = m3d.at(9, 9);

      /*
      
      answer.at(1, 1) = m3d.at(1, 1);
      answer.at(1, 2) = m3d.at(1, 2);
      answer.at(1, 4) = m3d.at(1, 6);
      answer.at(1, 5) = m3d.at(1, 9);
      
      answer.at(2, 1) = m3d.at(2, 1);
      answer.at(2, 2) = m3d.at(2, 2);
      answer.at(2, 4) = m3d.at(2, 6);
      answer.at(2, 5) = m3d.at(2, 9);
      
      answer.at(4, 1) = m3d.at(6, 1);
      answer.at(4, 2) = m3d.at(6, 2);
      answer.at(4, 4) = m3d.at(6, 6);
      answer.at(4, 5) = m3d.at(6, 9);
      
      answer.at(5, 1) = m3d.at(9, 1);
      answer.at(5, 2) = m3d.at(9, 2);
      answer.at(5, 4) = m3d.at(9, 6);
      answer.at(5, 5) = m3d.at(9, 9);
      */
    }
     //     answer.zero();
    
}



void
GradientPolyconvexMaterialOld :: giveMicromorphicMatrix_dSdPhi(FloatMatrix &answer, MatResponseMode mode, GaussPoint *gp, TimeStep *tStep)
{

  MaterialMode matMode = gp->giveMaterialMode();
  if (matMode == _PlaneStrain) {
    answer.resize(5,5);
  } else {
    answer.resize(9,9);
  } 
  answer.beUnitMatrix();
  answer.times(Hk);

}


void
GradientPolyconvexMaterialOld :: giveMicromorphicMatrix_dMdPhiGrad(FloatMatrix &answer, MatResponseMode mode, GaussPoint *gp, TimeStep *tStep)
{
  MaterialMode matMode = gp->giveMaterialMode();
  if (matMode == _PlaneStrain) {
    answer.resize(10,10);
  } else {
    answer.resize(27,27);
  } 
  answer.beUnitMatrix();
  answer.times(Ak);

}



void
GradientPolyconvexMaterialOld :: giveFiniteStrainGeneralizedStressVectors(FloatArray &vP, FloatArray &s, FloatArray &M, GaussPoint *gp, const FloatArray &vF, const FloatArray &micromorphicVar, const FloatArray micromorphicVarGrad, TimeStep *tStep)
{
    ///@todo Move this to StructuralCrossSection ?
    MaterialMode mode = gp->giveMaterialMode();
    if ( mode == _3dMat ) {
      this->giveFiniteStrainGeneralizedStressVectors_3d(vP, s, M, gp, vF, micromorphicVar, micromorphicVarGrad, tStep);
    } else if ( mode == _PlaneStrain ) {
      this->giveFiniteStrainGeneralizedStressVectors_PlaneStrain(vP, s, M, gp, vF, micromorphicVar, micromorphicVarGrad, tStep);
    } else {
      OOFEM_ERROR("Unknown material mode for the gradient polyconvex formulation");
    }
}
  


void
GradientPolyconvexMaterialOld :: giveFiniteStrainGeneralizedStressVectors_3d(FloatArray &vP, FloatArray &s, FloatArray &M, GaussPoint *gp, const FloatArray &vF, const FloatArray &micromorphicVar, const FloatArray micromorphicVarGrad, TimeStep *tStep)
{
  
    GradientPolyconvexMaterialOldStatus *status = static_cast< GradientPolyconvexMaterialOldStatus* >( this->giveStatus(gp) );
    //deformation gradient, its inverse, cofactor, and determinant
    FloatArray vCofF, vInvF, vInvFt;
    FloatMatrix F, invF, invFt, cofF;
    
    F.beMatrixForm(vF);
    invF.beInverseOf(F);
    double J = F.giveDeterminant();
    cofF.beTranspositionOf(invF);
    cofF.times(J);
    vCofF.beVectorForm(cofF);
    vInvF.beVectorForm(invF);

    invFt.beTranspositionOf(invF);
    vInvFt.beVectorForm(invFt);
    
    // relative stress for cofactor
    s = vCofF;
    s.subtract(micromorphicVar);
    s.times(-Hk);
    // higher order stress
    M = micromorphicVarGrad;
    M.times(Ak);   

    // first PK stress
    FloatArray vPm(9);
    // double well material model    
    double normC_tC1, normC_tC2;
    FloatArray arb1, arb2, vC_tC1, vC_tC2;
    FloatMatrix C, dCdF, C_tC1, C_tC2;
    C.beTProductOf(F,F);
    this->compute_dC_dF(dCdF,vF);

    FloatMatrix tC1, tC2;
    tC1 = this->givetC1(tStep);
    tC2 = this->givetC2(tStep);
    
    C_tC1 = C;
    C_tC1.subtract(tC1);

    C_tC2 = C;
    C_tC2.subtract(tC2);
    

    normC_tC1 = C_tC1.computeFrobeniusNorm();
    normC_tC2 = C_tC2.computeFrobeniusNorm();

    vC_tC1.beVectorForm(C_tC1);
    vC_tC2.beVectorForm(C_tC2);   


    arb1.beTProductOf(dCdF, vC_tC1);
    arb1.times(2. * normC_tC2 * normC_tC2);
      
    arb2.beTProductOf(dCdF, vC_tC2);
    arb2.times(2. * normC_tC1 * normC_tC1);

    
    vP = arb1;
    vP.add(arb2);
    vP.times(alpha);
    
    // micromorphic contribution
    for (int i = 1; i <= 3; i++) {
      for (int j = 1; j <= 3; j++) {
	for (int m = 1; m <=3; m++) {
	  for (int n = 1; n<=3; n++) {
	    vPm.at(giveVI(i,j)) -= (vInvF.at(giveVI(j,i))*vInvF.at(giveVI(n,m))-vInvF.at(giveVI(n,i))*vInvF.at(giveVI(j,m))) * J * s.at(giveVI(m,n));
	  }
	}
      }
    }
    
    vP.add(vPm);

    /// part with J^-1
    /*    FloatArray vPd(vInvFt);
    vPd.times(gamma/J);    
    vP.subtract(vPd);
    */
    //new
    /*    FloatArray vPd(vInvFt);
    vPd.times(log(J) * gamma);
    vP.add(vPd);
    */

    /// penalty on negative volume
    /*    if(J < 0) {
      FloatArray vPp(vInvFt);
      double penalty = 1.e15;
      vPp.times(J*J*penalty);
      }*/

    
    
    status->letTempMicromorphicVarBe(micromorphicVar);
    status->letTempMicromorphicVarGradBe(micromorphicVarGrad);

    status->letTempPVectorBe(vP);
    status->letTempFVectorBe(vF);
    status->letTempMicromorphicStressBe(s);
    status->letTempMicromorphicStressGradBe(M); 
      
}

  


void
GradientPolyconvexMaterialOld :: giveFiniteStrainGeneralizedStressVectors_PlaneStrain(FloatArray &vP, FloatArray &s, FloatArray &M, GaussPoint *gp, const FloatArray &reducedvF, const FloatArray &reducedMicromorphicVar, const FloatArray micromorphicVarGrad, TimeStep *tStep)
{

  FloatArray vF, vMV, fullvP, fullS;
  StructuralMaterial :: giveFullVectorFormF(vF, reducedvF, _PlaneStrain);
  StructuralMaterial :: giveFullVectorFormF(vMV, reducedMicromorphicVar, _PlaneStrain);
  this->giveFiniteStrainGeneralizedStressVectors_3d(fullvP, fullS, M, gp, vF, vMV, micromorphicVarGrad, tStep) ;
  StructuralMaterial :: giveReducedVectorForm(vP, fullvP, _PlaneStrain);
  StructuralMaterial :: giveReducedVectorForm(s, fullS, _PlaneStrain);

      
}
  

void
GradientPolyconvexMaterialOld :: compute_dC_dF(FloatMatrix &dCdF,const FloatArray &vF)
{

  dCdF.resize(9,9);
  FloatArray delta = {1, 1, 1, 0, 0, 0, 0, 0, 0};
  for (int i = 1; i <= 3; i++) {
    for (int j = 1; j <= 3; j++) {
      for (int m = 1; m <=3; m++) {
	for (int n = 1; n<=3; n++) {
	  dCdF.at(giveVI(i,j),giveVI(m,n)) = vF.at(giveVI(m,j)) * delta.at(giveVI(i,n)) + vF.at(giveVI(m,i)) * delta.at(giveVI(j,n));
	}
      }
    }
  }
  
}



FloatMatrix&
GradientPolyconvexMaterialOld :: givetC1(TimeStep *tStep)
 {
   double t = tStep->giveIntrinsicTime();
   double tEps = (t+1) * eps;
     
   tC1_0 = {{ 1.0, tEps, 0},{tEps, 1.+tEps*tEps, 0}, {0, 0, 1}};
   // tC1_0 = {{ 1.0, tEps, 0},{tEps, 1., 0}, {0, 0, 1.}};
   //   tC1_0 = {{(1. + tEps) * (1. + tEps), 0, 0},{0, 1. / (1. + tEps) / (1. + tEps), 0}, {0, 0, 1}};
   //tC1_0 = {{(1. + tEps) * (1. + tEps), tEps, 0},{tEps, (1. + tEps) * (1. + tEps), 0}, {0, 0, 1}};
   
      
   return tC1_0;
 }
  

FloatMatrix &
GradientPolyconvexMaterialOld :: givetC2(TimeStep *tStep)
 {
   double t = tStep->giveIntrinsicTime();
   double tEps = (t+1) * eps;
     
   tC2_0 = {{ 1.0, -tEps, 0},{-tEps, 1.+tEps*tEps, 0}, {0, 0, 1}};
   //tC2_0 = {{1.0, -tEps, 0},{-tEps, 1., 0}, {0, 0, 1.}};
   //tC2_0 = {{1. / (1. + tEps) / (1. + tEps), 0, 0},{0, (1. + tEps)*(1. + tEps), 0}, {0, 0, 1}};
   //tC1_0 = {{(1. + tEps) * (1. + tEps), -tEps, 0},{-tEps, (1. + tEps) * (1. + tEps), 0}, {0, 0, 1}};

   
   return tC2_0;
 }  
  

IRResultType
GradientPolyconvexMaterialOld :: initializeFrom(InputRecord *ir)
{
    IRResultType result;                             // Required by IR_GIVE_FIELD macro
    //IsotropicLinearElasticMaterial :: initializeFrom(ir);
    
    IR_GIVE_FIELD(ir, Hk, _IFT_MicromorphicMaterialExtensionInterface_Hk);
    IR_GIVE_FIELD(ir, Ak, _IFT_MicromorphicMaterialExtensionInterface_Ak);

    IR_GIVE_FIELD(ir, eps, _IFT_GradientPolyconvexMaterialOld_eps);
    IR_GIVE_OPTIONAL_FIELD(ir, alpha, _IFT_GradientPolyconvexMaterialOld_alpha );
    gamma = 1;
    IR_GIVE_OPTIONAL_FIELD(ir, gamma, _IFT_GradientPolyconvexMaterialOld_gamma );
    
    /*    tC1 = {{(1. + eps) * (1. + eps), 0, 0},{0, 1. / (1. + eps) / (1. + eps), 0}, {0, 0, 1}};
    tC2 = {{1. / (1. + eps) / (1. + eps), 0, 0},{0, (1. + eps)*(1. + eps), 0}, {0, 0, 1}};
    */

    tC1_0 = {{ 1.0, eps, 0},{eps, 1.+eps*eps, 0}, {0, 0, 1}};
    tC2_0 = {{1.0, -eps, 0},{-eps, 1.+eps*eps, 0}, {0, 0, 1}};
    

    //tC1 = {{ 1.0, eps, 0},{eps, 1., 0}, {0, 0, 1.}};
    //tC2 = {{1.0, -eps, 0},{-eps, 1., 0}, {0, 0, 1.}};
    
    
    
    return IRRT_OK;
}

int
GradientPolyconvexMaterialOld :: giveIPValue(FloatArray &answer, GaussPoint *gp, InternalStateType type, TimeStep *tStep)
{

    GradientPolyconvexMaterialOldStatus *status = static_cast< GradientPolyconvexMaterialOldStatus * >( this->giveStatus(gp) );
    

    if( type == IST_MicromorphicStress) {
      StructuralMaterial :: giveFullVectorForm( answer, status->giveStressVector(), gp->giveMaterialMode() );

    } else if( type == IST_MicromorphicStrain ) {
      StructuralMaterial :: giveFullVectorForm( answer, status->giveStrainVector(), gp->giveMaterialMode() );
    } else if( type == IST_MicromorphicRelativeStress ) {
      FloatArray s = status->giveMicromorphicStress();
      // @todo this is correct only for 2d problems, something like giveFullVectorForm for MicromorphicMaterial class would be necessary
      answer.resize(9);
      answer.zero();
      answer.at(6) = s.at(1);
      answer.at(9) = -s.at(1);
    } else if ( type == IST_MicromorphicRelativeStrain ) {
      answer.resize(9);
      answer.zero();
    } else if ( type == IST_MicromorphicHigherOrderStress ) {
      FloatArray M = status->giveMicromorphicStressGrad(); 
      answer.resize(9);
      answer.zero();
      answer.at(6) = M.at(1);
      answer.at(9) = M.at(2);
    } else if ( type == IST_MicromorphicHigherOrderStrain ) {
      FloatArray kappa = status->giveMicromorphicVarGrad();
      answer.resize(9);
      answer.zero();
      answer.at(6) = kappa.at(1);
      answer.at(9) = kappa.at(2);
    } else if(type == IST_MaxEquivalentStrainLevel) {
      FloatMatrix F, C, C_tC1, C_tC2;
      F.beMatrixForm(status->giveTempFVector());
      C.beTProductOf(F,F);
      C_tC1 = C;
      C_tC2 = C;
      FloatMatrix tC1, tC2;
      tC1 = this->givetC1(tStep);
      tC2 = this->givetC2(tStep);
      C_tC1.subtract(tC1);
      C_tC2.subtract(tC2);
      double normC_tC1, normC_tC2;
      normC_tC1 = C_tC1.computeFrobeniusNorm();
      normC_tC2 = C_tC2.computeFrobeniusNorm();
      answer.resize(1);
      if(normC_tC1 == 0 && normC_tC2 == 0) {
	answer = 0;
      } else {
	answer.at(1) = normC_tC1 * normC_tC1/(normC_tC1 * normC_tC1 + normC_tC2 * normC_tC2);
      }
    } else if( type ==  IST_DeformationGradientTensor ) {
      GradientPolyconvexMaterialOldStatus *status = static_cast< GradientPolyconvexMaterialOldStatus* >( this->giveStatus(gp) );
      //deformation gradient, its inverse, cofactor, and determinant
      FloatMatrix F, invF, cofF;
      F.beMatrixForm(status->giveTempFVector());
      invF.beInverseOf(F);
      double J = F.giveDeterminant();
      cofF.beTranspositionOf(invF);
      cofF.times(J);
      answer.beVectorForm(cofF);      
    } else {
      return StructuralMaterial :: giveIPValue(answer, gp, type, tStep);
    }
    return 1;
}
    

  GradientPolyconvexMaterialOldStatus :: GradientPolyconvexMaterialOldStatus(int n, Domain *d, GaussPoint *g, bool sym) : MicromorphicMaterialStatus(n, d, g, sym)
{
    micromorphicVar.resize(9);
    micromorphicVar.at(1) = micromorphicVar.at(2) = micromorphicVar.at(3) = 1.;
    micromorphicStress.resize(9);
    micromorphicVarGrad.resize(18);
    micromorphicStressGrad.resize(18);


    tempMicromorphicVar = micromorphicVar;
    tempMicromorphicVarGrad = micromorphicVarGrad;
    tempMicromorphicStress = micromorphicStress;
    tempMicromorphicStressGrad = micromorphicStressGrad;
}


} // end namespace oofem
