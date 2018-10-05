// Interface to abstract OS specifics for storing and retrieving secrets.

#include "keystore.h"

Keystore::Keystore() {}
Keystore::~Keystore() {}

bool Keystore::StoreSecret(const std::vector<uint8_t> &secret,
                           const std::string label) {
  (void)secret;
  (void)label;
  return true;
}

std::string Keystore::RetrieveSecret(const std::string &label) {
#ifdef __unix__
  return ss.get_secret(label);
#endif
  return "not implemented yet ...";
}
