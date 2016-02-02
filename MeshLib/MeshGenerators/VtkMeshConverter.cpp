/**
 * \file
 * \author Karsten Rink
 * \date   2011-08-23
 * \brief  Implementation of the VtkMeshConverter class.
 *
 * \copyright
 * Copyright (c) 2012-2016, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/project/license
 *
 */

#include "VtkMeshConverter.h"

#include "MeshLib/Elements/Elements.h"
#include "MeshLib/Mesh.h"
#include "MeshLib/Node.h"

// Conversion from Image to QuadMesh
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkSmartPointer.h>
#include <vtkBitArray.h>
#include <vtkCharArray.h>
#include <vtkUnsignedCharArray.h>
#include <vtkDoubleArray.h>
#include <vtkIntArray.h>

// Conversion from vtkUnstructuredGrid
#include <vtkCell.h>
#include <vtkCellData.h>
#include <vtkUnstructuredGrid.h>
#include <vtkUnsignedIntArray.h>

namespace MeshLib
{

namespace detail
{
template <class T_ELEMENT>
MeshLib::Element* createElementWithSameNodeOrder(const std::vector<MeshLib::Node*> &nodes,
		vtkIdList* const node_ids)
{
	MeshLib::Node** ele_nodes = new MeshLib::Node*[T_ELEMENT::n_all_nodes];
	for (unsigned k(0); k<T_ELEMENT::n_all_nodes; k++)
		ele_nodes[k] = nodes[node_ids->GetId(k)];
	return new T_ELEMENT(ele_nodes);
}
}

MeshLib::Mesh* VtkMeshConverter::convertRasterToMesh(GeoLib::Raster const& raster,
                                                     MeshElemType elem_type,
                                                     UseIntensityAs intensity_type)
{
	return convertImgToMesh(raster.begin(), raster.getHeader(), elem_type, intensity_type);
}

MeshLib::Mesh* VtkMeshConverter::convertImgToMesh(vtkImageData* img,
                                                  const double origin[3],
                                                  const double scalingFactor,
                                                  MeshElemType elem_type,
                                                  UseIntensityAs intensity_type)
{
	if ((elem_type != MeshElemType::TRIANGLE) && (elem_type != MeshElemType::QUAD))
	{
		ERR("VtkMeshConverter::convertImgToMesh(): Invalid Mesh Element Type.");
		return nullptr;
	}

	vtkSmartPointer<vtkDataArray> pixelData = vtkSmartPointer<vtkDataArray>(img->GetPointData()->GetScalars());
	int* dims = img->GetDimensions();
	int nTuple = pixelData->GetNumberOfComponents();
	if (nTuple < 1 || nTuple > 4)
	{
		ERR("VtkMeshConverter::convertImgToMesh(): Unsupported pixel composition!");
		return nullptr;
	}

	MathLib::Point3d const orig (std::array<double,3>{{origin[0], origin[1], origin[2]}});
	GeoLib::RasterHeader const header = 
		{static_cast<std::size_t>(dims[0]), static_cast<std::size_t>(dims[1]), orig, scalingFactor, -9999};
	const std::size_t incHeight = header.n_rows+1;
	const std::size_t incWidth  = header.n_cols+1;
	std::vector<double> pix_val (incHeight * incWidth, std::numeric_limits<double>::max());
	std::vector<bool> pix_vis (header.n_rows * header.n_cols, false);

	for (std::size_t i = 0; i < header.n_rows; i++)
		for (std::size_t j = 0; j < header.n_cols; j++)
		{
			std::size_t const img_idx = i*header.n_cols + j;
			std::size_t const fld_idx = i*incWidth + j;

			// colour of current pixel
			double* colour = pixelData->GetTuple(img_idx);
			// is current pixel visible?
			bool const visible = (nTuple == 2 || nTuple == 4) ? (colour[nTuple-1] != 0) : true;
			if (!visible)
				continue;

			double const value = (nTuple < 3) ?
				colour[0] : // grey (+ alpha)
				(0.3 * colour[0] + 0.6 * colour[1] + 0.1 * colour[2]); // rgb(a)
			pix_vis[img_idx] = true;
			pix_val[fld_idx] = value;
			pix_val[fld_idx+1] = value;
			pix_val[fld_idx+incWidth] = value;
			pix_val[fld_idx+incWidth+1] = value;
		}

	return constructMesh(pix_val, pix_vis, header, elem_type, intensity_type);
}

MeshLib::Mesh* VtkMeshConverter::convertImgToMesh(
	const double* img,
	GeoLib::RasterHeader const& header,
	MeshElemType elem_type,
	UseIntensityAs intensity_type)
{
	if ((elem_type != MeshElemType::TRIANGLE) && (elem_type != MeshElemType::QUAD))
	{
		ERR("Invalid Mesh Element Type.");
		return nullptr;
	}

	std::size_t const incHeight (header.n_rows+1);
	std::size_t const incWidth (header.n_cols+1);
	std::vector<double> pix_val (incHeight * incWidth, std::numeric_limits<double>::max());
	std::vector<bool> pix_vis (incHeight * incWidth, false);

	for (std::size_t i = 0; i < header.n_rows; i++)
		for (std::size_t j = 0; j < header.n_cols; j++)
		{
			std::size_t const img_idx = i*header.n_cols + j;
			std::size_t const fld_idx = i*incWidth + j;
			if (img[img_idx] == -9999)
				continue;

			pix_vis[img_idx] = true;
			pix_val[fld_idx] = img[img_idx];
			pix_val[fld_idx+1] = img[img_idx];
			pix_val[fld_idx+incWidth] = img[img_idx];
			pix_val[fld_idx+incWidth+1] = img[img_idx];
		}

	return constructMesh(pix_val, pix_vis, header, elem_type, intensity_type);
}

MeshLib::Mesh* VtkMeshConverter::constructMesh(
	std::vector<double> const& pix_val,
	std::vector<bool> const& pix_vis,
	GeoLib::RasterHeader const& header,
	MeshLib::MeshElemType elem_type,
	MeshLib::UseIntensityAs intensity_type)
{
	std::vector<int> node_idx_map ((header.n_rows+1) * (header.n_cols+1), -1);
	bool const use_elevation (intensity_type == MeshLib::UseIntensityAs::ELEVATION);
	std::vector<MeshLib::Node*> nodes (createNodeVector(pix_val, node_idx_map, header, use_elevation));
	if (nodes.empty())
		return nullptr;

	std::vector<MeshLib::Element*> elements (createElementVector(pix_val, pix_vis, nodes, node_idx_map, header.n_rows, header.n_cols, elem_type));
	if (elements.empty())
		return nullptr;

	MeshLib::Properties properties;
	if (intensity_type == MeshLib::UseIntensityAs::MATERIALS)
	{
		boost::optional< MeshLib::PropertyVector<int>& > prop_vec =
			properties.createNewPropertyVector<int>("MaterialIDs", MeshLib::MeshItemType::Cell, 1);
		fillPropertyVector<int>(*prop_vec, pix_val, pix_vis, header.n_rows, header.n_cols, elem_type);
	}
	else if (intensity_type == MeshLib::UseIntensityAs::DATAVECTOR)
	{
		boost::optional< MeshLib::PropertyVector<double>& > prop_vec =
			properties.createNewPropertyVector<double>("Colour", MeshLib::MeshItemType::Cell, 1);
		fillPropertyVector<double>(*prop_vec, pix_val, pix_vis, header.n_rows, header.n_cols, elem_type);
	}

	return new MeshLib::Mesh("RasterDataMesh", nodes, elements, properties);
}

std::vector<MeshLib::Node*> VtkMeshConverter::createNodeVector(
	std::vector<double> const&  elevation,
	std::vector<int> & node_idx_map,
	GeoLib::RasterHeader const& header,
	bool use_elevation)
{
	std::size_t node_idx_count(0);
	double const x_offset(header.origin[0] - header.cell_size/2.0);
	double const y_offset(header.origin[1] - header.cell_size/2.0);
	std::vector<MeshLib::Node*> nodes;
	for (std::size_t i = 0; i < (header.n_rows+1); i++)
		for (std::size_t j = 0; j < (header.n_cols+1); j++)
		{
			std::size_t const index = i * (header.n_cols+1) + j;
			if (elevation[index] == std::numeric_limits<double>::max())
				continue;

			double const zValue = (use_elevation) ? elevation[index] : 0;
			MeshLib::Node* node (new MeshLib::Node(x_offset + (header.cell_size * j), y_offset + (header.cell_size * i), zValue));
			nodes.push_back(node);
			node_idx_map[index] = node_idx_count;
			node_idx_count++;
		}
	return nodes;
}

std::vector<MeshLib::Element*> VtkMeshConverter::createElementVector(
	std::vector<double> const&  pix_val,
	std::vector<bool> const& pix_vis,
	std::vector<MeshLib::Node*> const& nodes,
	std::vector<int> const&node_idx_map,
	std::size_t const imgHeight,
	std::size_t const imgWidth,
	MeshElemType elem_type)
{
	std::vector<MeshLib::Element*> elements;
	std::size_t const incWidth (imgWidth+1);
	for (std::size_t i = 0; i < imgHeight; i++)
		for (std::size_t j = 0; j < imgWidth; j++)
		{
			if (!pix_vis[i*imgWidth+j])
				continue;

			int const idx = i * incWidth + j;
			if (elem_type == MeshElemType::TRIANGLE)
			{
				MeshLib::Node** tri1_nodes = new MeshLib::Node*[3];
				tri1_nodes[0] = nodes[node_idx_map[idx]];
				tri1_nodes[1] = nodes[node_idx_map[idx+1]];
				tri1_nodes[2] = nodes[node_idx_map[idx+incWidth]];

				MeshLib::Node** tri2_nodes = new MeshLib::Node*[3];
				tri2_nodes[0] = nodes[node_idx_map[idx+1]];
				tri2_nodes[1] = nodes[node_idx_map[idx+incWidth+1]];
				tri2_nodes[2] = nodes[node_idx_map[idx+incWidth]];

				elements.push_back(new MeshLib::Tri(tri1_nodes)); // upper left triangle
				elements.push_back(new MeshLib::Tri(tri2_nodes)); // lower right triangle
			}
			else if (elem_type == MeshElemType::QUAD)
			{
				MeshLib::Node** quad_nodes = new MeshLib::Node*[4];
				quad_nodes[0] = nodes[node_idx_map[idx]];
				quad_nodes[1] = nodes[node_idx_map[idx + 1]];
				quad_nodes[2] = nodes[node_idx_map[idx + incWidth + 1]];
				quad_nodes[3] = nodes[node_idx_map[idx + incWidth]];
				elements.push_back(new MeshLib::Quad(quad_nodes));
			}
		}
	return elements;
}

MeshLib::Mesh* VtkMeshConverter::convertUnstructuredGrid(vtkUnstructuredGrid* grid, std::string const& mesh_name)
{
	if (!grid)
		return nullptr;

	// set mesh nodes
	const std::size_t nNodes = grid->GetPoints()->GetNumberOfPoints();
	std::vector<MeshLib::Node*> nodes(nNodes);
	double* coords = nullptr;
	for (std::size_t i = 0; i < nNodes; i++)
	{
		coords = grid->GetPoints()->GetPoint(i);
		nodes[i] = new MeshLib::Node(coords[0], coords[1], coords[2]);
	}

	// set mesh elements
	const std::size_t nElems = grid->GetNumberOfCells();
	std::vector<MeshLib::Element*> elements(nElems);
	auto node_ids = vtkSmartPointer<vtkIdList>::New();
	for (std::size_t i = 0; i < nElems; i++)
	{
		MeshLib::Element* elem;
		grid->GetCellPoints(i, node_ids);

		int cell_type = grid->GetCellType(i);
		switch (cell_type)
		{
		case VTK_LINE: {
			elem = detail::createElementWithSameNodeOrder<MeshLib::Line>(nodes, node_ids);
			break;
		}
		case VTK_TRIANGLE: {
			elem = detail::createElementWithSameNodeOrder<MeshLib::Tri>(nodes, node_ids);
			break;
		}
		case VTK_QUAD: {
			elem = detail::createElementWithSameNodeOrder<MeshLib::Quad>(nodes, node_ids);
			break;
		}
		case VTK_PIXEL: {
			MeshLib::Node** quad_nodes = new MeshLib::Node*[4];
			quad_nodes[0] = nodes[node_ids->GetId(0)];
			quad_nodes[1] = nodes[node_ids->GetId(1)];
			quad_nodes[2] = nodes[node_ids->GetId(3)];
			quad_nodes[3] = nodes[node_ids->GetId(2)];
			elem = new MeshLib::Quad(quad_nodes);
			break;
		}
		case VTK_TETRA: {
			elem = detail::createElementWithSameNodeOrder<MeshLib::Tet>(nodes, node_ids);
			break;
		}
		case VTK_HEXAHEDRON: {
			elem = detail::createElementWithSameNodeOrder<MeshLib::Hex>(nodes, node_ids);
			break;
		}
		case VTK_VOXEL: {
			MeshLib::Node** voxel_nodes = new MeshLib::Node*[8];
			voxel_nodes[0] = nodes[node_ids->GetId(0)];
			voxel_nodes[1] = nodes[node_ids->GetId(1)];
			voxel_nodes[2] = nodes[node_ids->GetId(3)];
			voxel_nodes[3] = nodes[node_ids->GetId(2)];
			voxel_nodes[4] = nodes[node_ids->GetId(4)];
			voxel_nodes[5] = nodes[node_ids->GetId(5)];
			voxel_nodes[6] = nodes[node_ids->GetId(7)];
			voxel_nodes[7] = nodes[node_ids->GetId(6)];
			elem = new MeshLib::Hex(voxel_nodes);
			break;
		}
		case VTK_PYRAMID: {
			elem = detail::createElementWithSameNodeOrder<MeshLib::Pyramid>(nodes, node_ids);
			break;
		}
		case VTK_WEDGE: {
			MeshLib::Node** prism_nodes = new MeshLib::Node*[6];
			for (unsigned i=0; i<3; ++i)
			{
				prism_nodes[i] = nodes[node_ids->GetId(i+3)];
				prism_nodes[i+3] = nodes[node_ids->GetId(i)];
			}
			elem = new MeshLib::Prism(prism_nodes);
			break;
		}
		case VTK_QUADRATIC_EDGE: {
			elem = detail::createElementWithSameNodeOrder<MeshLib::Line3>(nodes, node_ids);
			break;
		}
		case VTK_QUADRATIC_TRIANGLE: {
			elem = detail::createElementWithSameNodeOrder<MeshLib::Tri6>(nodes, node_ids);
			break;
		}
		case VTK_QUADRATIC_QUAD: {
			elem = detail::createElementWithSameNodeOrder<MeshLib::Quad8>(nodes, node_ids);
			break;
		}
		case VTK_BIQUADRATIC_QUAD: {
			elem = detail::createElementWithSameNodeOrder<MeshLib::Quad9>(nodes, node_ids);
			break;
		}
		case VTK_QUADRATIC_TETRA: {
			elem = detail::createElementWithSameNodeOrder<MeshLib::Tet10>(nodes, node_ids);
			break;
		}
		case VTK_QUADRATIC_HEXAHEDRON: {
			elem = detail::createElementWithSameNodeOrder<MeshLib::Hex20>(nodes, node_ids);
			break;
		}
		case VTK_QUADRATIC_PYRAMID: {
			elem = detail::createElementWithSameNodeOrder<MeshLib::Pyramid13>(nodes, node_ids);
			break;
		}
		case VTK_QUADRATIC_WEDGE: {
			MeshLib::Node** prism_nodes = new MeshLib::Node*[15];
			for (unsigned i=0; i<3; ++i)
			{
				prism_nodes[i] = nodes[node_ids->GetId(i+3)];
				prism_nodes[i+3] = nodes[node_ids->GetId(i)];
			}
			for (unsigned i=0; i<3; ++i)
				prism_nodes[6+i] = nodes[node_ids->GetId(8-i)];
			prism_nodes[9] = nodes[node_ids->GetId(12)];
			prism_nodes[10] = nodes[node_ids->GetId(14)];
			prism_nodes[11] = nodes[node_ids->GetId(13)];
			for (unsigned i=0; i<3; ++i)
				prism_nodes[12+i] = nodes[node_ids->GetId(11-i)];
			elem = new MeshLib::Prism15(prism_nodes);
			break;
		}
		default:
			ERR("VtkMeshConverter::convertUnstructuredGrid(): Unknown mesh element type \"%d\".", cell_type);
			return nullptr;
		}

		elements[i] = elem;
	}

	MeshLib::Mesh* mesh = new MeshLib::Mesh(mesh_name, nodes, elements);
	convertScalarArrays(*grid, *mesh);

	return mesh;
}

void VtkMeshConverter::convertScalarArrays(vtkUnstructuredGrid &grid, MeshLib::Mesh &mesh)
{
	vtkPointData* point_data = grid.GetPointData();
	unsigned const n_point_arrays = static_cast<unsigned>(point_data->GetNumberOfArrays());
	for (unsigned i=0; i<n_point_arrays; ++i)
		convertArray(*point_data->GetArray(i), mesh.getProperties(), MeshLib::MeshItemType::Node);

	vtkCellData* cell_data = grid.GetCellData();
	unsigned const n_cell_arrays = static_cast<unsigned>(cell_data->GetNumberOfArrays());
	for (unsigned i=0; i<n_cell_arrays; ++i)
		convertArray(*cell_data->GetArray(i), mesh.getProperties(), MeshLib::MeshItemType::Cell);
}

void VtkMeshConverter::convertArray(vtkDataArray &array, MeshLib::Properties &properties, MeshLib::MeshItemType type)
{
	if (vtkDoubleArray::SafeDownCast(&array))
	{
		VtkMeshConverter::convertTypedArray<double>(array, properties, type);
		return;
	}

	if (vtkIntArray::SafeDownCast(&array))
	{
		VtkMeshConverter::convertTypedArray<int>(array, properties, type);
		return;
	}

	if (vtkBitArray::SafeDownCast(&array))
	{
		VtkMeshConverter::convertTypedArray<bool>(array, properties, type);
		return;
	}

	if (vtkCharArray::SafeDownCast(&array))
	{
		VtkMeshConverter::convertTypedArray<char>(array, properties, type);
		return;
	}

	if (vtkUnsignedIntArray::SafeDownCast(&array))
	{
		// MaterialIDs are assumed to be integers
		if(std::strncmp(array.GetName(), "MaterialIDs", 11) == 0)
			VtkMeshConverter::convertTypedArray<int>(array, properties, type);
		else
			VtkMeshConverter::convertTypedArray<unsigned>(array, properties, type);

		return;
	}

	ERR ("Array \"%s\" in VTU file uses unsupported data type.", array.GetName());
	return;
}

double VtkMeshConverter::getExistingValue(const double* img, std::size_t length)
{
	for (std::size_t i=0; i<length; i++)
	{
		if (img[i] != -9999)
			return img[i];
	}
	return -9999;
}

} // end namespace MeshLib
