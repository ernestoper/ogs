/**
 * \copyright
 * Copyright (c) 2012-2016, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/project/license
 *
 * \file   LiquidFlowMaterialProperties.cpp
 *
 * Created on August 18, 2016, 11:49 AM
 */

#include "LiquidFlowMaterialProperties.h"

#include <logog/include/logog.hpp>

#include "MeshLib/Mesh.h"
#include "MeshLib/PropertyVector.h"

#include "ProcessLib/Parameter/Parameter.h"
#include "ProcessLib/Parameter/SpatialPosition.h"

#include "MaterialLib/Fluid/FluidProperty.h"
#include "MaterialLib/PorousMedium/Porosity/Porosity.h"
#include "MaterialLib/PorousMedium/Storage/Storage.h"

namespace ProcessLib
{
namespace LiquidFlow
{
LiquidFlowMaterialProperties::LiquidFlowMaterialProperties(
            BaseLib::ConfigTree const& config,
            MeshLib::PropertyVector<int> const& material_ids,
            Parameter<double> const& intrinsic_permeability_data,
            Parameter<double> const& porosity_data,
            Parameter<double> const& storage_data)
    : _material_ids(material_ids),
      _intrinsic_permeability_data(intrinsic_permeability_data),
      _porosity_data(porosity_data),
      _storage_data(storage_data)
{
    DBUG("Reading material properties of liquid flow process.");

    //! \ogs_file_param{prj__material_property__fluid}
    auto const& fluid_config = config.getConfigSubtree("fluid");

    // Get fluid properties
    //! \ogs_file_param{prj__material_property__fluid__density}
    auto const& rho_conf = fluid_config.getConfigSubtree("density");
    _liquid_density = MaterialLib::Fluid::createFluidDensityModel(rho_conf);
    //! \ogs_file_param{prj__material_property__fluid__viscosity}
    auto const& mu_conf = fluid_config.getConfigSubtree("viscosity");
    _viscosity = MaterialLib::Fluid::createViscosityModel(mu_conf);

    // Get porous properties
    //! \ogs_file_param{prj__material_property__porous_medium}
    auto const& poro_config = config.getConfigSubtree("porous_medium");
    //! \ogs_file_param{prj__material_property__porous_medium__porous_medium}
    for (auto const& conf : poro_config.getConfigSubtreeList("porous_medium"))
    {
        //! \ogs_file_param{prj__material_property__porous_medium__porous_medium__permeability}
        auto const& perm_conf = conf.getConfigSubtree("permeability");
        _intrinsic_permeability_models.emplace_back(
            MaterialLib::PorousMedium::createPermeabilityModel(perm_conf));

        //! \ogs_file_param{prj__material_property__porous_medium__porous_medium__porosity}
        auto const& poro_conf = conf.getConfigSubtree("porosity");
        auto n = MaterialLib::PorousMedium::createPorosityModel(poro_conf);
        _porosity_models.emplace_back(std::move(n));

        //! \ogs_file_param{prj__material_property__porous_medium__porous_medium__storage}
        auto const& stora_conf = conf.getConfigSubtree("storage");
        auto beta = MaterialLib::PorousMedium::createStorageModel(stora_conf);
        _storage_models.emplace_back(std::move(beta));
    }
}

double LiquidFlowMaterialProperties::getLiquidDensity(const double p,
                                                      const double T) const
{
    ArrayType vars;
    vars[static_cast<int>(MaterialLib::Fluid::PropertyVariableType::T)] = T;
    vars[static_cast<int>(MaterialLib::Fluid::PropertyVariableType::pl)] = p;
    return _liquid_density->getValue(vars);
}

double LiquidFlowMaterialProperties::getViscosity(const double p,
                                                  const double T) const
{
    ArrayType vars;
    vars[static_cast<int>(MaterialLib::Fluid::PropertyVariableType::T)] = T;
    vars[static_cast<int>(MaterialLib::Fluid::PropertyVariableType::pl)] = p;
    return _viscosity->getValue(vars);
}

double LiquidFlowMaterialProperties::getMassCoefficient(
                                     const double t, const SpatialPosition& pos,
                                     const double p, const double T,
                                     const double porosity_variable,
                                     const double storage_variable) const
{
    ArrayType vars;
    vars[static_cast<int>(MaterialLib::Fluid::PropertyVariableType::T)] = T;
    vars[static_cast<int>(MaterialLib::Fluid::PropertyVariableType::pl)] = p;
    const double drho_dp =
         _liquid_density->getdValue(vars,
                                   MaterialLib::Fluid::PropertyVariableType::pl);
    const double rho = _liquid_density->getValue(vars);

    if (_storage_models.size() > 0)
    {
        const int mat_id = _material_ids[pos.getElementID()];
        const double porosity = _porosity_models[mat_id]->getValue(porosity_variable, T);
        const double storage = _storage_models[mat_id]->getValue(storage_variable);
        return porosity * drho_dp / rho + storage;
    }
    else
    {
        const double storage = _storage_data(t, pos)[0];
        const double porosity = _porosity_data(t, pos)[0];
        return porosity * drho_dp / rho + storage;
    }
}

Eigen::MatrixXd const& LiquidFlowMaterialProperties
                ::getPermeability(const double t,
                                  const SpatialPosition& pos,
                                  const int dim) const
{
    if (_intrinsic_permeability_models.size() > 0)
    {
        const int mat_id = _material_ids[pos.getElementID()];
        return _intrinsic_permeability_models[mat_id];
    }
    else
    {
        auto const permeability = _intrinsic_permeability_data(t, pos)[0];
        return MathLib::toMatrix(permeability, dim, dim);
    }
}

}  // end of namespace
}  // end of namespace
