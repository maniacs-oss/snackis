#include "snabel/snabel.hpp"

namespace snabel {
  const int VERSION[3] = {0, 7, 14};

  str version_str() {
    Stream out;
    
    for (int i = 0; i < 3; i++) {
      if (i) { out << "."; }
      out << VERSION[i];
    }
    
    return out.str();
  }
}
