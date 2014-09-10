/*!
  \file MeshDDC.h
  \author Wenqing Wang
  \date   2014.06
  \brief  Definition of mesh class for partitioned mesh for parallel computing within the
          framework of domain decomposition (DDC).

  \copyright
  Copyright (c) 2014, OpenGeoSys Community (http://www.opengeosys.org)
             Distributed under a Modified BSD License.
               See accompanying file LICENSE.txt or
               http://www.opengeosys.org/project/license

 */

#ifndef MESH_DDC_H_
#define MESH_DDC_H_

#include <cstdlib>
#include <string>

#include "Mesh"

typedef long MyInt;
namespace MeshLib
{
class Node;
class Element;

/// A subdomain mesh.
class MeshDDC : public Mesh
{
    public:
        /*!
            \brief  Constructor
            \param name          Name assigned to the mesh.
            \param nodes         Vector for nodes.
            \param elements      Vector for elements.
            \param nnodes_global Number of nodes of the whole mesh.
                                 0: with linear elements
                                 1: with quadratic elemens.
            \param nnodes_active Number of active nodes of the partition.
                                 0: with linear elements
                                 1: with quadratic elemens.

            \param axisymmetry   Indicator for axisymmetry.
        */
        MeshDDC(const std::string &name,
                const std::vector<Node*> &nodes,
                const std::vector<Element*> &elements,
                const MyInt [] nnodes_global,
                const MyInt [] nnodes_local
                const bool axisymmetry)
            : Mesh(name, nodes, elements, false), _axisymmetry(axisymmetry),
              _nnodes_global {nnodes_global[0], nnodes_global[1] },
        _nnodes_active {nnodes_active[0], nnodes_active[1] }
        {
        }

        /*!
            \brief Get the number of nodes of the whole mesh.
            \param order The order of elements (0 or 1).
                         Its default value is 0 for linear elements.
        */
        MyInt getGlobalNNodes(const int order = 0) const
        {
            return _nnodes_global[order];
        }

        /*!
            \brief Get the number of nodes of the whole mesh.
            \param order The order of elements (0 or 1).
                         Its default value is 0 for linear elements.
        */
        MyInt getActiveNNodes(const int order = 0) const
        {
            return _nnodes_active[order];
        }

        /// Get the largest ID of active nodes for higher order elements.
        MyInt getLargestActiveNodeID() const
        {
            // Note: _nodes.size() should be changed once the high order element is condidered
            // in the root class.
            return static_cast<MyInt>( _nodes.size() ) + _nnodes_active[1] - _nnodes_active[0];
        }

    private:
        /// Number of nodes of the whole mesh. 0: for linear elements; 1: for quadratic elements.
        MyInt _nnodes_global[2];

        /// Number of the active nodes. 0: for linear elements; 1: for quadratic elements.
        MyInt _nnodes_active[2];

};

} // end of namespace

#endif // end of #ifndef MESH_DDC_H_

