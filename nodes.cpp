#include "nodes.hpp"

// #include <iomanip>
// #include <fstream>

// val3dity
#include "Surface.h"
#include "Solid.h"
#include "GenericObject.h"


void ValidatorNode::process()
{
  auto& faces = vector_input("faces");
  // create a vertex list and vertex IDs, taking care of duplicates
  std::map<arr3f, size_t> vertex_map;
  std::vector<arr3f> vertex_vec;
  {
    size_t v_cntr = 0;
    std::set<arr3f> vertex_set;
    for (size_t j = 0; j < faces.size(); ++j)
    {
      auto& face = faces.get<LinearRing>(j);

      for (auto &vertex : face)
      {
        auto [it, did_insert] = vertex_set.insert(vertex);
        if (did_insert)
        {
          vertex_map[vertex] = v_cntr++;
          vertex_vec.push_back(vertex);
        }
      }
      for (auto& iring : face.interior_rings()) 
      {
        for (auto &vertex : iring)
        {
          auto [it, did_insert] = vertex_set.insert(vertex);
          if (did_insert)
          {
            vertex_map[vertex] = v_cntr++;
            vertex_vec.push_back(vertex);
          }
        }
      }
    }
  }
  
  // build the val3dity data structures
  auto o = std::make_unique<val3dity::GenericObject>("none");

  val3dity::Primitive::set_translation_min_values((*manager.data_offset)[0], (*manager.data_offset)[1]);
  val3dity::Surface::set_translation_min_values((*manager.data_offset)[0], (*manager.data_offset)[1]);

  auto sh = std::make_unique<val3dity::Surface>(0);
  for (auto &v : vertex_vec)
  {
    val3dity::Point3 p(v[0], v[1], v[2]);
    sh->add_point(p);
  }
  //-- add the facets
  for (size_t j = 0; j < faces.size(); ++j)
  {
    auto& face = faces.get<LinearRing>(j);
    std::vector< std::vector<int> > pgnids;
    std::vector<int> ids;
    // ofs << "f";
    for (auto &vertex : face)
    {
      ids.push_back(vertex_map[vertex]);
      // ofs << " " << vertex_map[vertex];
    }
    // ofs << std::endl;
    pgnids.push_back(ids);
    for (auto& iring : face.interior_rings())
    {
      std::vector<int> ids;
      for (auto &vertex : iring)
      {
        ids.push_back(vertex_map[vertex]);
      }
      pgnids.push_back(ids);
    }
    sh->add_face(pgnids);
  }
  // ofs.close();

  auto s = std::make_unique<val3dity::Solid>();
  s->set_oshell(sh.get());
  o->add_primitive(s.get());

  // validate
  o->validate(tol_planarity_d2p_, tol_planarity_normals_);

  json j = o->get_unique_error_codes();
  vector_output("errors").push_back(j.dump());

  PointCollection pc;
  for (const auto& p : sh->get_error_points()) {
    pc.push_back(arr3f{float(p.x()), float(p.y()), float(p.z())});
  }
  auto& error_faces = vector_output("error_faces");
  for (auto sfid : sh->get_error_face_ids()) {
    int fid = stoi(sfid)-1;
    std::cout << "number of faces: " << faces.size() << "; error: " <<fid << "\n";
    error_faces.push_back(faces.get<LinearRing>(fid));
  }
  output("error_locations").set(pc);

}