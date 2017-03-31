/**
 * \copyright
 * Copyright (c) 2012-2017, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/project/license
 *
 *  \file   EvolutionaryPIDcontroller.h
 *  Created on March 31, 2017, 4:13 PM
 */

#pragma once

#include "ITimeStepAlgorithm.h"

namespace BaseLib
{
class ConfigTree;
}

namespace NumLib
{
/**
 *  This class gives an adaptive algorithm whose time step control is
 *  evolutionary PID controller. With an definition of relative solution change
 *  \f$e_n=\frac{\|u^{n+1}-u^n\|}{\|u^{n+1}\|}\f$, the algorithm gives a time
 *   step size estimation as
 *   \f[
 *     h_{n+1} =  \left(\frac{e_{n-1}}{e_n}\right)^{k_P}
 *                \left(\frac{TOL}{e_n}\right)^{k_I}
 *                \left(\frac{e^2_{n-1}}{e_n e_{n-2}}\right)^{k_D}
 *   \f]
 *   where \f$k_P=0.075\f$, \f$k_I=0.175\f$, \f$k_D=0.01\f$ are empirical PID
 *   parameters.
 *
 *   In the computation, \f$ e_n\f$ is calculated firstly. If \f$e_n>TOL\f$, the
 *   current time step is rejected and repeated with a new time step size of
 *   \f$h=\frac{TOL}{e_n} h_n\f$.
 *
 *   Limits of the time step size are given as
 *   \f[
 *        h_{\mbox{min}} \leq h_{n+1} \leq h_{\mbox{max}},
 *        l \leq \frac{h_{n+1}}{h_n} \leq L
 *   \f]
 *
 *   Similar algorithm can be found in \cite ahmed2015adaptive .
 */
class EvolutionaryPIDcontroller
{
public:
    EvolutionaryPIDcontroller();
    virtual ~EvolutionaryPIDcontroller();

private:
};

}  // end of namespace NumLib