/**
 * \file
 * \author Norihiro Watanabe
 * \date   2013-08-13
 * \brief
 *
 * \copyright
 * Copyright (c) 2013, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/project/license
 *
 */


#ifndef C0ISOPARAMETRICELEMENTS_H_
#define C0ISOPARAMETRICELEMENTS_H_

#include "MeshLib/Elements/Quad.h"
#include "FemLib/ShapeFunction/ShapeQuad4.h"
#include "FemLib/Integration/IntegrationGaussRegular.h"
#include "FemLib/Extrapolation/ExtrapolationGaussQuad.h"

#include "TemplateIsoparametric.h"

namespace FemLib
{

typedef TemplateIsoparametric<MeshLib::Quad, ShapeQuad4, IntegrationGaussRegular<2>, ExtrapolationGaussQuad> QUAD4;

}

#endif //C0ISOPARAMETRICELEMENTS_H_
