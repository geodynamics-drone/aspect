/*
  Copyright (C) 2011 - 2016 by the authors of the ASPECT code.

  This file is part of ASPECT.

  ASPECT is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2, or (at your option)
  any later version.

  ASPECT is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with ASPECT; see the file LICENSE.  If not see
  <http://www.gnu.org/licenses/>.
*/


#ifndef _aspect_adiabatic_conditions_initial_profile_h
#define _aspect_adiabatic_conditions_initial_profile_h


#include <aspect/adiabatic_conditions/interface.h>
#include <deal.II/base/point.h>

#include <deal.II/base/parsed_function.h>

namespace aspect
{
  namespace AdiabaticConditions
  {
    using namespace dealii;

    /**
     * A model in which the adiabatic profile is
     * calculated once at the start of the model run.
     * The gravity is assumed to be in depth direction
     * and the composition is either given by the initial
     * composition at reference points or computed
     * as a reference depth-function.
     * All material parameters are computed by the
     * material model plugin.
     */
    template <int dim>
    class InitialProfile : public Interface<dim>
    {
      public:
        /**
         * Constructor. Initialize variables.
         */
        InitialProfile ();

        /**
         * Initialization function. Because this function is called after
         * initializing the SimulatorAccess, all of the necessary information
         * is available to calculate the adiabatic profile. It computes the
         * adiabatic conditions along a vertical transect of the geometry
         * based on the given material model and other quantities.
         */
        virtual void initialize ();

        /**
         * Some plugins need to know whether the adiabatic conditions are
         * already calculated. This is for example the case for the simple
         * compressible material model, which uses the adiabatic temperature
         * as reference temperature to calculate the density. For the
         * calculation of the adiabatic conditions this functionality is
         * simply switched off, because we are always on the reference
         * profile. This way the plugin behaves differently at initialization
         * time of the adiabatic conditions and during the main model run.
         */
        virtual bool is_initialized() const;

        /**
         * Return the adiabatic temperature at a given point of the domain.
         */
        virtual double temperature (const Point<dim> &p) const;

        /**
         * Return the adiabatic pressure at a given point of the domain.
         */
        virtual double pressure (const Point<dim> &p) const;

        /**
         * Return the reference density at a given point of the domain.
         */
        virtual double density (const Point<dim> &p) const;

        /**
         * Return the derivative of the density with respect to depth
         * at the given point @p p.
         */
        virtual
        double density_derivative (const Point<dim> &p) const;

        static
        void
        declare_parameters (ParameterHandler &prm);

        virtual
        void
        parse_parameters (ParameterHandler &prm);

      private:

        /**
         * Whether the adiabatic conditions are already calculated. This is
         * important for plugins that are used by the adiabatic conditions but
         * also depend on the adiabatic conditions. This way they can behave
         * differently in initialization and model run.
         */
        bool initialized;

        /**
         * Number of points at which we compute the adiabatic values.
         */
        const unsigned int n_points;

        /**
         * Vectors of values of temperatures and pressures on a transect into
         * depth at which we have computed them. The public member functions
         * of this class interpolate linearly between these points.
         */
        std::vector<double> temperatures;
        std::vector<double> pressures;
        std::vector<double> densities;

        /**
         * Interval spacing between each two data points in the tables above
         * with regard to the depth coordinate.
         */
        double delta_z;

        /**
         * An enum describing the different options to compute the reference
         * profile for composition.
         */
        enum CompositionProfile
        {
          initial_composition,
          reference_function
        };

        /**
         * Selected option to compute the reference profile for composition.
         */
        CompositionProfile reference_composition;

        /**
         * Function object that computes the reference composition profile
         * if the reference_composition variable is set to function.
         */
        std_cxx11::unique_ptr<Functions::ParsedFunction<1> > composition_function;

        /**
         * Internal helper function. Returns the reference property at a
         * given point of the domain.
         */
        double get_property (const Point<dim> &p,
                             const std::vector<double> &property) const;
    };
  }
}


#endif
