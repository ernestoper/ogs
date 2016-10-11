/**
 * \copyright
 * Copyright (c) 2012-2016, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/project/license
 *
 * \file   LiquidFlowProcess.h
 *
 * Created on August 19, 2016, 1:38 PM
 */

#ifndef OGS_LIQUIDFLOWPROCESS_H
#define OGS_LIQUIDFLOWPROCESS_H

#include "NumLib/DOF/LocalToGlobalIndexMap.h"
#include "ProcessLib/Process.h"

#include "LiquidFlowMaterialProperties.h"
#include "LiquidFlowLocalAssembler.h"

namespace MeshLib
{
class Element;
class Mesh;
template <typename PROP_VAL_TYPE>
class PropertyVector;
}

namespace ProcessLib
{
namespace LiquidFlow
{
/**
 * \brief A class to simulate the liquid flow process in porous media described by
 *
 * \f[
 *     \frac{\partial n \rho_l}{\partial T} \frac{\partial T}{\partial t}/\rho_l
 *       + (\frac{\partial n \rho_l}{\partial p}/\rho_l + \beta_s)
 *          \frac{\partial p}{\partial t}
 *       -\nabla (\frac{K}{\mu}(\nabla p + \rho_l g \nabla z) ) = Q
 * \f]
 * where
 *    \f{eqnarray*}{
 *       &p:&        \mbox{pore pressure,}\\
 *       &T: &       \mbox{Temperature,}\\
 *       &\rho_l:&   \mbox{liquid density,}\\
 *       &\beta_s:&  \mbox{specific storage,}\\
 *       &K:&        \mbox{permeability,}\\
 *       &\mu:&      \mbox{viscosity,}\\
 *    \f}
 */
class LiquidFlowProcess final : public Process
{
public:
    LiquidFlowProcess(
        MeshLib::Mesh& mesh,
        std::unique_ptr<AbstractJacobianAssembler>&& jacobian_assembler,
        std::vector<std::unique_ptr<ParameterBase>> const& parameters,
        std::vector<std::reference_wrapper<ProcessVariable>>&&
            process_variables,
        SecondaryVariableCollection&& secondary_variables,
        NumLib::NamedFunctionCaller&& named_function_caller,
        const bool compute_gravitational_term,
        BaseLib::ConfigTree const& config);

    void preTimestepConcreteProcess(GlobalVector const& x, const double t,
                                    const double delta_t) override;

    void preIterationConcreteProcess(const unsigned iter,
                                     GlobalVector const& x) override;

    bool isLinear() const override { return true; }
private:
    void initializeConcreteProcess(
        NumLib::LocalToGlobalIndexMap const& dof_table,
        MeshLib::Mesh const& mesh, unsigned const integration_order) override;

    void assembleConcreteProcess(const double t, GlobalVector const& p,
                                 GlobalMatrix& M, GlobalMatrix& K,
                                 GlobalVector& b) override;

    void assembleWithJacobianConcreteProcess(
        const double t, GlobalVector const& x, GlobalVector const& xdot,
        const double dxdot_dx, const double dx_dx, GlobalMatrix& M,
        GlobalMatrix& K, GlobalVector& b, GlobalMatrix& Jac) override;

    const bool _compute_gravitational_term;
    LiquidFlowMaterialProperties _material_properties;

    std::vector<std::unique_ptr<LiquidFlowLocalAssemblerInterface>>
        _local_assemblers;

    std::unique_ptr<GlobalVector> _p_previous_timestep;
};

}  // end of namespace
}  // end of namespace

#endif /* LIQUIDFLOWPROCESS_H */
