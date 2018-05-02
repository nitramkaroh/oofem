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

#ifndef quadmembrane_h
#define quadmembrane_h

#include "planstrss.h"
#include "Loads/pressurefollowerloadinterface.h"

#define _IFT_QuadMembrane_Name "quadmembrane"

namespace oofem {
class FEI2dQuadLin;

/**
 * This class implements an isoparametric four-node quadrilateral plane-
 * stress membrane finite element. Each node has 3 degrees of freedom.
 */
 class QuadMembrane : public PlaneStress2d, public PressureFollowerLoadElementInterface
{
protected:
    static FEI2dQuadLin interpolation;

public:
    QuadMembrane(int n, Domain * d);
    virtual ~QuadMembrane();


      virtual FEInterpolation *giveInterpolation() const;
      void computeNmatrixAt(const FloatArray &iLocCoord, FloatMatrix &answer);
    // definition & identification
    virtual const char *giveInputRecordName() const { return _IFT_QuadMembrane_Name; }
    virtual const char *giveClassName() const { return "QuadMembrane"; }
    void giveDofManDofIDMask(int inode, IntArray &answer) const;
    virtual IRResultType initializeFrom(InputRecord *ir);

    void computeDeformationGradientVector(FloatArray &answer, GaussPoint *gp, TimeStep *tStep, ValueModeType modeType);
    virtual MaterialMode giveMaterialMode() { return _Membrane2d; }


    // support for pressure follower load interface
    virtual Interface *giveInterface(InterfaceType it);

    virtual double surfaceEvalVolumeAround(GaussPoint *gp, int iSurf){return this->computeSurfaceVolumeAround(gp, iSurf);}
    virtual void surfaceEvalNmatrixAt(FloatMatrix &answer, int iSurf, GaussPoint *gp){this->computeNmatrixAt(gp->giveNaturalCoordinates(),answer);}

    void computeSurfaceNMatrix (FloatMatrix &answer, int boundaryID, const FloatArray& lcoords)
    {this->computeNmatrixAt(lcoords,answer);}
    
    virtual void surfaceEvaldNdxi(FloatMatrix &answer, int iSurf, GaussPoint *gp);
    virtual void surfaceEvalDeformedNormalAt(FloatArray &answer, FloatArray &dxdksi,FloatArray &dxdeta, int iSurf, GaussPoint *gp, TimeStep *tStep);
    virtual IntegrationRule* surfaceGiveIntegrationRule(int order, int iSurf) { return this->giveInterpolation()->giveBoundarySurfaceIntegrationRule(order, iSurf);}

    
    
protected:

    virtual void computeBmatrixAt(GaussPoint *gp, FloatMatrix &answer, TimeStep *tStep = NULL, int = 0, int = ALL_STRAINS);
    virtual void computeBHmatrixAt(GaussPoint *gp, FloatMatrix &answer, TimeStep *tStep, double alpha = 0);



    

};
} // end namespace oofem
#endif // quadmembrane_h
