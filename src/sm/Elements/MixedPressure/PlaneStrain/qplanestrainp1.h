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

#ifndef qplanestrainp1_h
#define qplanestrainp1_h

#include "../sm/Elements/PlaneStrain/qplanestrain.h"
#include "../sm/Elements/MixedPressure/basemixedpressureelement.h"

#define _IFT_QPlaneStrainP1_Name "qplanestrainp1"


namespace oofem {
class FEI2dQuadLin;


class QPlaneStrainP1 : public QPlaneStrain, public BaseMixedPressureElement
{
protected:
    static FEI2dQuadLin interpolation_lin;

public:
    QPlaneStrainP1(int n, Domain *d);
    virtual ~QPlaneStrainP1() { }

protected:
    virtual void computePressureNMatrixAt(GaussPoint *, FloatArray &);
    virtual void computeVolumetricBmatrixAt(GaussPoint *gp, FloatArray &Bvol, NLStructuralElement *element);
    virtual NLStructuralElement *giveElement() { return this; }
public:
    virtual const char *giveClassName() const { return "QTrPlaneStrainP1"; }
    virtual const char *giveInputRecordName() const { return _IFT_QPlaneStrainP1_Name; }


    virtual void giveDofManDofIDMask(int inode, IntArray &answer) const;
    virtual void giveDofManDofIDMask_u(IntArray &answer);
    virtual void giveDofManDofIDMask_p(IntArray &answer);


    virtual void computeStiffnessMatrix(FloatMatrix &answer, MatResponseMode mode, TimeStep *tStep) { BaseMixedPressureElement :: computeStiffnessMatrix(answer, mode, tStep); }
    virtual void giveInternalForcesVector(FloatArray &answer, TimeStep *tStep, int useUpdatedGpRecord) { BaseMixedPressureElement :: giveInternalForcesVector(answer, tStep, useUpdatedGpRecord); }


    virtual int giveNumberOfPressureDofs() { return 4; }
    virtual int giveNumberOfDisplacementDofs() { return 16; }
    virtual int giveNumberOfDofs() { return 20; }
    virtual void postInitialize();
    virtual void updateInternalState(TimeStep *tStep)
    { BaseMixedPressureElement :: updateInternalState(tStep);}
    
};
} // end namespace oofem
#endif // quad2planestrainp1_h
