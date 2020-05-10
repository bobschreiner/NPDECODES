/**
 * @file nonlinschroedingerequation_main.cc
 * @brief NPDE homework NonLinSchroedingerEquation code
 * @author Oliver Rietmann
 * @date 22.04.2020
 * @copyright Developed at ETH Zurich
 */

#include <cmath>
#include <complex>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <utility>

#include <Eigen/Core>

#include <lf/assemble/assemble.h>
#include <lf/io/io.h>
#include <lf/mesh/hybrid2d/hybrid2d.h>
#include <lf/uscalfe/uscalfe.h>

#include "nonlinschroedingerequation.h"
#include "propagator.h"

int main() {
  /* SAM_LISTING_BEGIN_9 */
  // Load mesh and initalize FE space and DOF handler
  auto mesh_factory = std::make_unique<lf::mesh::hybrid2d::MeshFactory>(2);
  const lf::io::GmshReader reader(std::move(mesh_factory), CURRENT_SOURCE_DIR
                                  "/../meshes/square_64.msh");
  auto mesh_p = reader.mesh();
  auto fe_space =
      std::make_shared<lf::uscalfe::FeSpaceLagrangeO1<std::complex<double>>>(
          mesh_p);
  const lf::assemble::DofHandler &dofh{fe_space->LocGlobMap()};
  const lf::uscalfe::size_type N_dofs(dofh.NumDofs());

  // Mass matrix
  lf::assemble::COOMatrix<double> D_COO(N_dofs, N_dofs);
  NonLinSchroedingerEquation::MassElementMatrixProvider mass_emp;
  lf::assemble::AssembleMatrixLocally(0, dofh, dofh, mass_emp, D_COO);
  Eigen::SparseMatrix<double> D = D_COO.makeSparse();
  Eigen::SparseMatrix<std::complex<double>> M = std::complex<double>(0, 1) * D;

  // Stiffness matrix
  lf::assemble::COOMatrix<double> A_COO(N_dofs, N_dofs);
  lf::uscalfe::LinearFELaplaceElementMatrix stiffness_emp;
  lf::assemble::AssembleMatrixLocally(0, dofh, dofh, stiffness_emp, A_COO);
  Eigen::SparseMatrix<double> A = A_COO.makeSparse();

  // Prepare timestepping
  int timesteps = 100;
  double T = 1.0;
  double tau = T / timesteps;

// Prepare inital data
#if SOLUTION
  const double PI = 3.14159265358979323846;
  auto u0 = [PI](Eigen::Vector2d x) -> std::complex<double> {
    return 4.0 * std::cos(PI * x(0)) * std::cos(PI * x(1));
  };
  lf::mesh::utils::MeshFunctionGlobal mf_u0{u0};
  Eigen::VectorXcd mu = lf::uscalfe::NodalProjection(*fe_space, mf_u0);
#else
  //====================
  // Your code here
  // Initialize the vector mu with the basis expansion coefficients
  // of an interpolant of the function $u_0$.
  //====================
  Eigen::VectorXcd mu(N_dofs);
#endif

  // Prepare (pointer to) kinetic propagator for semi step ($\Psi^{0,\frac{\tau}{2}}$)
  auto kineticPropagator = std::make_unique<NonLinSchroedingerEquation::KineticPropagator>(A, M, 0.5 * tau);
  // Prepare (pointer to) interaction propagator for full step
  auto interactionPropagator = std::make_unique<NonLinSchroedingerEquation::InteractionPropagator>(tau);
  // Prepare split-step propagator for full step $\tau$
  NonLinSchroedingerEquation::SplitStepPropagator splitStepPropagator(std::move(kineticPropagator), std::move(interactionPropagator));

  // Arrays for storing "energies" contributing to the Hamiltonian
  Eigen::VectorXd norm(timesteps + 1);
  Eigen::VectorXd E_kin(timesteps + 1);
  Eigen::VectorXd E_int(timesteps + 1);
#if SOLUTION
  // Timestepping
  for (int j = 0; j < timesteps; ++j) {
    // Compute norm and energy along the solution
    norm(j) = NonLinSchroedingerEquation::Norm(mu, D);
    E_kin(j) = NonLinSchroedingerEquation::KineticEnergy(mu, A);
    E_int(j) = NonLinSchroedingerEquation::InteractionEnergy(mu, D);
    // Timestep tau according to Strang splitting
    mu = splitStepPropagator(mu);
  }
  norm(timesteps) = NonLinSchroedingerEquation::Norm(mu, D);
  E_kin(timesteps) = NonLinSchroedingerEquation::KineticEnergy(mu, A);
  E_int(timesteps) = NonLinSchroedingerEquation::InteractionEnergy(mu, D);
#else
  //====================
  // Your code goes here
  // Implement timestepping based on Strang splitting
  // Record contributions to the Hamiltonian
  //====================
#endif

  // Timegrid
  Eigen::VectorXd t = Eigen::VectorXd::LinSpaced(timesteps + 1, 0.0, T);

  // Nice output format
  const static Eigen::IOFormat CSVFormat(Eigen::FullPrecision,
                                         Eigen::DontAlignCols, ", ", "\n");

  // Write norm to file
  std::ofstream norm_csv;
  norm_csv.open("norm.csv");
  norm_csv << t.transpose().format(CSVFormat) << std::endl;
  norm_csv << norm.transpose().format(CSVFormat) << std::endl;
  norm_csv.close();
  /* SAM_LISTING_END_9 */

  // Call python script to plot norm
  std::cout << "Generated " CURRENT_BINARY_DIR "/norm.csv" << std::endl;
  std::system("python3 " CURRENT_SOURCE_DIR "/plot_norm.py " CURRENT_BINARY_DIR
              "/norm.csv " CURRENT_BINARY_DIR "/norm.eps");

  // Write energies to file
  std::ofstream energies_csv;
  energies_csv.open("energies.csv");
  energies_csv << t.transpose().format(CSVFormat) << std::endl;
  energies_csv << E_kin.transpose().format(CSVFormat) << std::endl;
  energies_csv << E_int.transpose().format(CSVFormat) << std::endl;
  energies_csv.close();

  // Call python script to plot energies
  std::cout << "Generated " CURRENT_BINARY_DIR "/energies.csv" << std::endl;
  std::system("python3 " CURRENT_SOURCE_DIR
              "/plot_energies.py " CURRENT_BINARY_DIR
              "/energies.csv " CURRENT_BINARY_DIR "/energies.eps");

  // Write entry-wise squared modulus of $\mu$ to .vtk file
  std::cout << "Generated " CURRENT_BINARY_DIR "/solution.vtk" << std::endl;
  lf::io::VtkWriter vtk_writer(mesh_p, "solution.vtk");
  auto nodal_data = lf::mesh::utils::make_CodimMeshDataSet<double>(mesh_p, 2);
  for (int global_idx = 0; global_idx < N_dofs; global_idx++) {
    if (dofh.Entity(global_idx).RefEl() == lf::base::RefElType::kPoint) {
      nodal_data->operator()(dofh.Entity(global_idx)) = std::norm(mu(global_idx));
    }
  };
  vtk_writer.WritePointData("mu_abs2", *nodal_data);

  return 0;
}
