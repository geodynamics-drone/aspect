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


#include <aspect/postprocess/stokes_residual.h>
#include <aspect/simulator.h>
#include <aspect/global.h>

#include <deal.II/grid/grid_generator.h>
#include <deal.II/grid/tria.h>
#include <deal.II/fe/fe_dgq.h>
#include <deal.II/dofs/dof_handler.h>
#include <deal.II/numerics/data_out_stack.h>


#include <math.h>
#include <vector>

namespace aspect
{
  namespace Postprocess
  {
    template <int dim>
    template <class Archive>
    void StokesResidual<dim>::DataPoint::serialize (Archive &ar,
                                                    const unsigned int)
    {
      ar &time &solve_index &values;
    }


    template <int dim>
    StokesResidual<dim>::StokesResidual ()
    {}




    template <int dim>
    std::pair<std::string,std::string>
    StokesResidual<dim>::execute (TableHandler &)
    {
      // On the root process, write out the file.
      if (Utilities::MPI::this_mpi_process(this->get_mpi_communicator()) == 0)
        {
          std::ofstream f((this->get_output_directory() +
                           "stokes_residuals.txt").c_str());
          f << "# time solveidx residual\n";
          for (unsigned int i=0; i<entries.size(); ++i)
            {
              for (unsigned int j=0; j<entries[i].values.size(); ++j)
                f << entries[i].time << ' '
                  << entries[i].solve_index << ' '
                  << entries[i].values[j] << '\n';

              f << '\n';
            }
          f.close();
        }

      return std::make_pair (std::string ("Writing stokes residuals"),
                             this->get_output_directory() +
                             "stokes_residuals.txt");
    }

    template <int dim>
    void StokesResidual<dim>::stokes_solver_callback (const SolverControl &solver_control_cheap,
                                                      const SolverControl &solver_control_expensive)
    {
      unsigned int current_solve_index = 0;
      if (entries.size()>0 && entries.back().time == this->get_time())
        current_solve_index = entries.back().solve_index+1;

      DataPoint data_point;
      data_point.time = this->get_time();
      data_point.solve_index = current_solve_index;

      data_point.values = solver_control_cheap.get_history_data();

#if !DEAL_II_VERSION_GTE(9,0,0)
      // Pre deal.II 9.0 history_data contained 0 for all iterations
      // up to max steps (e.g. because the solver converged earlier).
      // Remove those entries.
      std::vector<double>::iterator zero_value = std::find(data_point.values.begin(),
                                                           data_point.values.end(),
                                                           0);
      data_point.values.erase(zero_value,data_point.values.end());
#endif

      // If there were expensive iterations add them after a signalling -1.
      if ((solver_control_cheap.last_check() == dealii::SolverControl::failure)
          && (solver_control_cheap.last_step() == solver_control_cheap.max_steps()))
        {
          data_point.values.push_back(-1.0);
          data_point.values.insert(data_point.values.end(),
                                   solver_control_expensive.get_history_data().begin(),
                                   solver_control_expensive.get_history_data().end());
        }

#if !DEAL_II_VERSION_GTE(9,0,0)
      // Pre deal.II 9.0 history_data contained 0 for all iterations
      // up to max steps (e.g. because the solver converged earlier).
      // Remove those entries.
      zero_value = std::find(data_point.values.begin(),
                             data_point.values.end(),
                             0);
      data_point.values.erase(zero_value,data_point.values.end());
#endif

      entries.push_back(data_point);
    }

    template <int dim>
    void
    StokesResidual<dim>::initialize ()
    {
      this->get_signals().post_stokes_solver.connect(
        std_cxx11::bind(&StokesResidual<dim>::stokes_solver_callback,
                        this,
                        /* do not need the first arguments
                         * std_cxx11::_1,
                         * std_cxx11::_2,
                         * std_cxx11::_3,
                         */
                        std_cxx11::_4,
                        std_cxx11::_5)
      );
    }

    template <int dim>
    template <class Archive>
    void StokesResidual<dim>::serialize (Archive &ar, const unsigned int)
    {
      ar &entries;
    }


    template <int dim>
    void
    StokesResidual<dim>::save (std::map<std::string, std::string> &status_strings) const
    {
      std::ostringstream os;
      aspect::oarchive oa (os);
      oa << (*this);

      status_strings["StokesResidual"] = os.str();
    }


    template <int dim>
    void
    StokesResidual<dim>::load (const std::map<std::string, std::string> &status_strings)
    {
      // see if something was saved
      if (status_strings.find("StokesResidual") != status_strings.end())
        {
          std::istringstream is (status_strings.find("StokesResidual")->second);
          aspect::iarchive ia (is);
          ia >> (*this);
        }
    }


  }
}


// explicit instantiations
namespace aspect
{
  namespace Postprocess
  {
    ASPECT_REGISTER_POSTPROCESSOR(StokesResidual,
                                  "Stokes residual",
                                  "A postprocessor that outputs the Stokes residuals during the iterative solver algorithm into a file stokes_residuals.txt in the output directory.")
  }
}
