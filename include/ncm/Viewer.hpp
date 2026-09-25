#pragma once
#include <string>

#include "ncm/Matrix.hpp"

namespace ncm {

// Writes a snapshot of the matrix as a BMP image (opens natively on Windows).
// Columns: the four fields (Input, Memory, Reasoning, Output).
// Rows, for the middle z-slice of each field:
//   1. 3D voxel activity
//   2. 2D sheet activity (each voxel shown as its own sheet)
//   3. 1D line activity (each sheet cell shown as the mean of its line)
void writeSnapshot(const NeuralCellularMatrix& m, const std::string& path, int scale = 2);

} // namespace ncm
