#include "nodes.hpp"

void register_nodes(geoflow::NodeRegister& node_register) {
  node_register.register_node<ValidatorNode>("Validator");
}