// This file is part of gfp-val3dity
// Copyright (C) 2018-2022 Ravi Peters

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
#include <geoflow/geoflow.hpp>

using namespace geoflow;

class ValidatorNode : public Node
{
  // int precision=5;
  // std::string filepath = "faces.obj";
  // bool no_offset = false;
  bool log_invalids=false;
  float tol_planarity_d2p_ = 0.01;
  float tol_planarity_normals_ = 20;
  // float tol_overlap_ = -1;

public:
  using Node::Node;
  void init()
  {
    add_vector_input("input_geom", {typeid(std::unordered_map<int, Mesh>), typeid(TriangleCollection)});

    add_vector_output("errors", typeid(std::string));
    add_vector_output("error_faces", typeid(LinearRing));
    add_output("error_locations", typeid(PointCollection));

    // add_param(ParamPath(filepath, "filepath", "File path"));
    // add_param(ParamBool(no_offset, "no_offset", "Do not apply global offset"));
    // add_param(ParamInt(precision, "precision", "precision"));
    add_param(ParamBool(log_invalids, "log_invalids", "Print invalid geometries to cout"));
    add_param(ParamFloat(tol_planarity_d2p_, "tol_planarity_d2p_", "tolerance for planarity distance_to_plane"));
    add_param(ParamFloat(tol_planarity_normals_, "tol_planarity_normals_", "tolerance for planarity based on normals deviation in degrees"));
  }
  void process();
};
// } // namespace geoflow::nodes::val3dity