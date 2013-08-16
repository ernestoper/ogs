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


#include <iostream>
#include <cassert>

#include "logog/include/logog.hpp"

namespace FemLib
{
template <class T_SHAPE_FUNC>
NaturalCoordinatesMapping<T_SHAPE_FUNC>::NaturalCoordinatesMapping(const MeshLib::Element &ele)
: _ele(&ele)
{
    this->reset(ele);
};

template <class T_SHAPE_FUNC>
void NaturalCoordinatesMapping<T_SHAPE_FUNC>::reset(const MeshLib::Element &ele)
{
    _ele = &ele;

    const std::size_t dim = _ele->getDimension();
    const std::size_t nnodes = _ele->getNNodes();
    _nodes_coords.resize(dim, nnodes);
    for (std::size_t i=0; i<nnodes; i++)
        for (std::size_t j=0; j<dim; j++)
            _nodes_coords(j,i) = ele.getNode(i)->getCoords()[j];
};

/// computeMappingProperty mapping properties at the given location in natural coordinates
template <class T_SHAPE_FUNC>
void NaturalCoordinatesMapping<T_SHAPE_FUNC>::computeMappingMatrices(const double* natural_pt, ShapeData &prop) const
{
    //prepare
    const std::size_t dim = _ele->getDimension();
    const std::size_t nnodes = _ele->getNNodes();
    prop.setZero();

    //shape, dshape/dr
    T_SHAPE_FUNC::computeShapeFunction(natural_pt, prop.N);
    double* dNdr =  prop.dNdr.data();
    T_SHAPE_FUNC::computeGradShapeFunction(natural_pt, dNdr);

    //jacobian: J=[dx/dr dy/dr // dx/ds dy/ds]
    for (std::size_t i_r=0; i_r<dim; i_r++) {
        for (std::size_t j_x=0; j_x<dim; j_x++) {
            for (std::size_t k=0; k<nnodes; k++) {
                prop.J(i_r,j_x) += prop.dNdr(i_r, k) * _nodes_coords(j_x, k);
            }
        }
    }

    //|J|, J^-1, dshape/dx
    prop.detJ = prop.J.determinant();
    if (prop.detJ>.0) {
        prop.invJ = prop.J.inverse();
        prop.dNdx = prop.invJ * prop.dNdr;
    } else {
        ERR("***error: det_j=%e is not positive.\n", prop.detJ);
    }
};

template <class T_SHAPE_FUNC>
void NaturalCoordinatesMapping<T_SHAPE_FUNC>::mapToPhysicalCoordinates(const ShapeData &prop, double* physical_pt) const
{
    const std::size_t dim = _ele->getDimension();
    const std::size_t nnodes = _ele->getNNodes();

    for (std::size_t i=0; i<dim; i++) {
        physical_pt[i] = .0;
        for (std::size_t j=0; j<nnodes; j++)
            physical_pt[i] += prop.N(j) * _nodes_coords(i,j);
    }
}

template <class T_SHAPE_FUNC>
void NaturalCoordinatesMapping<T_SHAPE_FUNC>::mapToNaturalCoordinates(const ShapeData &prop, const double* physical_pt, double* natural_pt) const
{
    const std::size_t dim = _ele->getDimension();
    const std::size_t nnodes = _ele->getNNodes();

    // calculate dx which is relative coordinates from the element center
    std::vector<double> dx(dim, .0);
    // x_avg = sum_i {x_i} / n
    for (std::size_t i=0; i<dim; i++)
        for (std::size_t j=0; j<nnodes; j++)
            dx[i] += _nodes_coords(i,j);
    for (std::size_t i=0; i<dim; i++)
        dx[i] /= (double)nnodes;
    // dx = pt - x_avg
    for (std::size_t i=0; i<dim; i++)
        dx[i] = physical_pt[i] - dx[i];

    // r = invJ^T * dx
    for (std::size_t i=0; i<dim; i++) {
        natural_pt[i] = 0.0;
        for (std::size_t j=0; j<dim; j++)
            natural_pt[i] += prop.invJ(j * dim, i) * dx[j];
    }

}

} //end
