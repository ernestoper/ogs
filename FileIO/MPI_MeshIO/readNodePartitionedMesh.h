/*!
  \file readNodePartitionedMesh.h
  \author Wenqing Wang
  \date   2014.08
  \brief  Declare a class to read node-wise partitioned mesh with MPI functions.

  \copyright
  Copyright (c) 2014, OpenGeoSys Community (http://www.opengeosys.org)
             Distributed under a Modified BSD License.
               See accompanying file LICENSE.txt or
               http://www.opengeosys.org/project/license
*/

#ifndef READ_NODE_PARTITIONED_MESH_H
#define READ_NODE_PARTITIONED_MESH_H

#include <mpi.h>
#include <string>

#inlcude "NodePartitionedMesh"

namespace MeshLib
{
class Node;
class Element;
};

namespace FileIO
{
/// Node data only for parallel reading.
struct NodeData
{
    MyInt index;
    double x;
    double y;
    double z;
};

/// Define MPI data type, MPI_Node_ptr, for struct MeshNode for palllel reading of nodes
void buildNodeStrucTypeMPI(NodeData *anode, MPI_Datatype *MPI_Node_ptr);

/// Class to handle reading data of partitioned mesh.
class readNodePartitionedMesh
{
    public:

        readNodePartitionedMesh() : _num_controls(14)
        {
        }

        /*!
             \brief Create a NodePartitionedMesh object, read data to it,
                    and return a pointer to it.
             \param comm  MPI  communicator.
             \param file_name  Name of file to be read.
             \return           Pointer to Mesh object.
        */
        MeshLib::NodePartitionedMesh* read(MPI_Comm comm, const std::string &file_name);

    private:
        /*!
             \brief Create a NodePartitionedMesh object, read binary mesh data to it,
                    and return a pointer to it.
             \param comm  MPI  Communicator.
             \param file_name  Name of file to be read.
             \return           Pointer to Mesh object.
        */
        MeshLib::NodePartitionedMesh* readBinary(MPI_Comm comm, const std::string &file_name);

        /*!
             \brief Create a NodePartitionedMesh object, read ASCII mesh data to it,
                    and return a pointer to it.
             \param comm  MPI  Communicator.
             \param file_name  Name of file to be read.
             \return           Pointer to Mesh object.
        */
        MeshLib::NodePartitionedMesh* readASCII(MPI_Comm comm, const std::string &file_name);

        /*!
             \brief Set mesh nodes from a tempory array containing node data read from file.
             \param node_data  Array containing node data read from file.
             \param mesh_node  Vector of mesh nodes to be set.
        */
        void setNodes(const NodeData *node_data, std::vector<MeshLib::Node*> &mesh_node);

        /*!
             \brief Set mesh elements from a tempory  array containing node data read from file.
             \param node_data  Array containing element data read from file.
             \param mesh_elem  Vector of mesh elements to be set.
             \param ghost      Flag for ghost element
        */
        void setElements(const MyInt *elem_data, std::vector<MeshLib::Element*> &mesh_elem,
                         const bool ghost = false);

        /// Numbers define the partition.
        MyInt _mesh_controls[14];

        /// How many numbers that define the partition, fixed to 14
        unsigned _num_controls;

        /// Number of MPI processes
        int _size;

        /// _size converted to string
        string _size_str;

        /// Rank of compute core
        int _rank;
}

} // End of namespace

#endif // end of #ifndef READ_NODE_PARTITIONED_MESH_H