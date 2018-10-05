
#ifndef keystore_h__
#define keystore_h__

#include <stdint.h>
#include <string>
#include <vector>

#ifdef __unix__
#include "gnome.h"
#endif

class Keystore {
public:
  Keystore();
  ~Keystore();
  bool StoreSecret(const std::vector<uint8_t> &secret, const std::string label);
  std::string RetrieveSecret(const std::string &label);

private:
#ifdef __unix__
  LinuxSecretStore ss;
#endif
};

#endif // keystore_h__
