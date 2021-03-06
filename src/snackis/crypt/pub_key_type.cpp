#include "snackis/crypt/pub_key_type.hpp"

namespace snackis {
namespace crypt {
  PubKeyType pub_key_type;

  PubKeyType::PubKeyType(): Type<PubKey>("crypt::PubKey")
  { }

  bool PubKeyType::is_null(const PubKey &val) const { return val == null_pub_key; }

  PubKey PubKeyType::from_val(const Val &in) const { return get<PubKey>(in); }

  Val PubKeyType::to_val(const PubKey &in) const { return in; }

  PubKey PubKeyType::read(std::istream &in) const {
    return PubKey(in);
  }
  
  void PubKeyType::write(const PubKey &val, std::ostream &out) const {
    out.write((const char *)val.data, sizeof val.data);
  }
}}
