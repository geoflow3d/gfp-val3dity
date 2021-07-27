#include "nodes.hpp"

// #include <iomanip>
// #include <fstream>

// val3dity
#include "Surface.h"
#include "Solid.h"
#include "MultiSolid.h"
#include "GenericObject.h"


void ValidatorNode::process()
{
  auto& input_geom = vector_input("input_geom");

  // init val3dity
  val3dity::Primitive::set_translation_min_values((*manager.data_offset)[0], (*manager.data_offset)[1]);
  val3dity::Surface::set_translation_min_values((*manager.data_offset)[0], (*manager.data_offset)[1]);

  std::vector<std::unique_ptr<val3dity::Surface>> surfaces;

  
  if(input_geom.is_connected_type(typeid(std::unordered_map<int, Mesh>))) {
    for(size_t i=0; i<input_geom.size(); ++i) {
      auto& meshmap = input_geom.get<std::unordered_map<int, Mesh>&>(i);
      for(auto& [sid, mesh] : meshmap) {
        // create a vertex list and vertex IDs, taking care of duplicates
        std::map<arr3f, size_t> vertex_map;
        std::vector<arr3f> vertex_vec;

        size_t v_cntr = 0;
        std::set<arr3f> vertex_set;
        
        auto& faces = mesh.get_polygons();
        for (auto& face : faces)
        {
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

        auto sh = std::make_unique<val3dity::Surface>(i);
        for (auto &v : vertex_vec)
        {
          val3dity::Point3 p(v[0], v[1], v[2]);
          sh->add_point(p);
        }
        //-- add the facets
        for (auto& face : faces)
        {
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
        surfaces.push_back(std::move(sh));
      }
    }

  } else if(input_geom.is_connected_type(typeid(TriangleCollection))) {
    // create a vertex list and vertex IDs, taking care of duplicates

    std::map<arr3f, size_t> vertex_map;
    std::vector<arr3f> vertex_vec;

    size_t v_cntr = 0;
    std::set<arr3f> vertex_set;
    auto& triangles = input_geom.get<TriangleCollection&>();
    for (auto& triangle : triangles)
    {
      for (auto &vertex : triangle)
      {
        auto [it, did_insert] = vertex_set.insert(vertex);
        if (did_insert)
        {
          vertex_map[vertex] = v_cntr++;
          vertex_vec.push_back(vertex);
        }
      }
    }

    auto sh = std::make_unique<val3dity::Surface>(0);
    for (auto &v : vertex_vec)
    {
      val3dity::Point3 p(v[0], v[1], v[2]);
      sh->add_point(p);
    }
    for (auto& triangle : triangles)
    {
      std::vector< std::vector<int> > pgnids;
      std::vector<int> ids;
      // ofs << "f";
      for (auto &vertex : triangle)
      {
        ids.push_back(vertex_map[vertex]);
        // ofs << " " << vertex_map[vertex];
      }
      // ofs << std::endl;
      pgnids.push_back(ids);
      sh->add_face(pgnids);
    }
    surfaces.push_back(std::move(sh));
  } 

  // validate
  std::vector<std::unique_ptr<val3dity::Solid>> solids;
  for (auto& sh : surfaces) {
    auto s = std::make_unique<val3dity::Solid>();
    s->set_oshell(sh.get());
    solids.push_back(std::move(s));
  }

  auto o = std::make_unique<val3dity::GenericObject>("none");
  auto ms = std::make_unique<val3dity::MultiSolid>();
  if (solids.size() == 1) {
    o->add_primitive(solids[0].get());
  } else if (solids.size() > 1) {
    for (auto& s : solids) {
      ms->add_solid(s.get());
    }
    o->add_primitive(ms.get());
  }
  
  o->validate(tol_planarity_d2p_, tol_planarity_normals_);
  json j = o->get_unique_error_codes();
  vector_output("errors").push_back(j.dump());

  if(log_invalids) std::cout << "\nVal3dity error codes: " << j.dump() << "\n";

  PointCollection pc;
  auto& error_faces = vector_output("error_faces");

  auto& offset = *manager.data_offset;
  
  for (auto& sh : surfaces) {
    auto errorpts = sh->get_error_points();
    if(log_invalids) {
      if(errorpts.size()) {
        std::cout << "Val3dity error points (" << errorpts.size() << " total):\n";
        std::cout << std::fixed << std::setprecision(3);
      }
    }
    for (const auto& p : errorpts) {
      pc.push_back(arr3f{float(p.x()), float(p.y()), float(p.z())});
      if(log_invalids) {
        if(errorpts.size()) std::cout << "\t" << (p.x()+offset[0]) << " " << (p.y()+offset[1]) << " " << (p.z()+offset[2]) << "\n";
      }
    }
    
    for (auto sfid : sh->get_error_face_ids()) {
      try {
          int fid = stoi(sfid) - 1;
          // std::cout << "number of faces: " << faces.size() << "; error: " << fid << "\n";
          if(input_geom.is_connected_type(typeid(Mesh))) {
            auto& mesh = input_geom.get<Mesh&>();
            auto& faces = mesh.get_polygons();
            error_faces.push_back(faces[fid]);
          } else if(input_geom.is_connected_type(typeid(TriangleCollection))) {
            auto& triangles = input_geom.get<TriangleCollection&>();
            auto tri = triangles[fid];
            LinearRing lr;
            lr.push_back(tri[0]);
            lr.push_back(tri[1]);
            lr.push_back(tri[2]);
            error_faces.push_back(lr);
          }
          
      } catch (std::exception& e) {
          continue;
      }
    }
  }
  if(log_invalids) {
    if(error_faces.size()) {
      std::cout << "Val3dity error faces (" << error_faces.size() << " total):\n";
      for (size_t i=0; i< error_faces.size(); ++i) {
        auto& face = error_faces.get<LinearRing&>(i);
        std::cout << "face " << i << "\n";
        for (auto& p : face) {
          std::cout << "\t" << (p[0]+offset[0]) << " " << (p[1]+offset[1]) << " " << (p[2]+offset[2]) << "\n";
        }
      }
    }
  }
  output("error_locations").set(pc);

}