#include <cstdlib>
#include <iostream>

#include "simplelinearfiniteelements.h"

/* SAM_LISTING_BEGIN_5 */
#define MESH "Square4"

int main()
{
  std::string meshfile = CURRENT_SOURCE_DIR "/../meshes/" MESH ".txt";

  SimpleLinearFiniteElements::TriaMesh2D square_mesh(meshfile);
  std::cout << "Mesh loaded " << std::endl;
  std::cout << "Mesh info: " << square_mesh.Vertices.size() << " vertices, "
            << square_mesh.Elements.size() << " elements" << std::endl;

  // print both H1 and L2 errors and plot Mesh
  std::tuple<Eigen::VectorXd, double, double> solution = solve(square_mesh);

  std::cout << "L2-error:  " << std::get<1>(solution) << std::endl;
  std::cout << "H1s-error: " << std::get<2>(solution) << std::endl;

  // plot MESH
  std::string meshplot = CURRENT_BINARY_DIR "/" MESH ".png";
  std::system(("python3 -B " CURRENT_SOURCE_DIR "/../scripts/plot_mesh.py " + meshfile + " " + meshplot).c_str());
  std::cout << "Generated " + meshplot << std::endl;

  // path and name of output files
  std::string meshfile_solution = CURRENT_BINARY_DIR "/" MESH "_solution.txt";
  std::string meshplot_solution = CURRENT_BINARY_DIR "/" MESH "_solution.png";

  // generate 3d mesh file from solution
  SimpleLinearFiniteElements::TriaMesh2D::addZComponent(meshfile, meshfile_solution, std::get<0>(solution));
  std::cout << "Generated " + meshfile_solution << std::endl;

  // plot the 3d mesh file
  std::system(("python3 -B " CURRENT_SOURCE_DIR "/../scripts/plot_surf.py " + meshfile_solution + " " + meshplot_solution).c_str());
  std::cout << "Generated " + meshplot_solution << std::endl;

  return 0;
}
/* SAM_LISTING_END_5 */
