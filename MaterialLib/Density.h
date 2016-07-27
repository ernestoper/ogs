/*!
   \file  Density.h
   \brief Declaration of generic class for material density.

   \author Wenqing Wang
   \date Jan 2015

   \copyright
    Copyright (c) 2012-2015, OpenGeoSys Community (http://www.opengeosys.org)
               Distributed under a Modified BSD License.
               See accompanying file LICENSE.txt or
               http://www.opengeosys.org/project/license
*/
#ifndef DENSITY_H_
#define DENSITY_H_

#include<memory>
#include<string>

#include<DensityBase.h>

namespace MaterialLib
{
/// Density class
template<typename T_DENSITY_MODEL> class Density : public DensityBase
{
    public:

        template<typename... Args> Density(Args... args)
        {
            _density_model.reset(new T_DENSITY_MODEL(args...));
        }

        /// Get desity model name.
        std::string getName() const
        {
            return _density_model->getName();
        }

        DensityType getType() const
        {
            return DensityType::CONSTANT;
        }

        /// Get density value
        template<typename... Args> double getDensity(Args... args) const
        {
            return _density_model->getDensity(args...);
        }
    private:
        std::unique_ptr<T_DENSITY_MODEL> _density_model;
};

} // end namespace
#endif

