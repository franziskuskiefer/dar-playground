
#ifndef gnome_keystore_h__
#define gnome_keystore_h__

#include <libsecret/secret.h>
#include <memory>

struct ScopedDelete {
  void operator()(SecretService *ss) { g_object_unref(ss); }
  void operator()(GError *error) { g_error_free(error); }
  void operator()(GList *list) { g_list_free(list); }
  void operator()(SecretValue *val) { secret_value_unref(val); }
};

template <class T> struct ScopedMaybeDelete {
  void operator()(T *ptr) {
    if (ptr) {
      ScopedDelete del;
      del(ptr);
    }
  }
};

#define SCOPED(x) typedef std::unique_ptr<x, ScopedMaybeDelete<x>> Scoped##x

SCOPED(SecretService);
SCOPED(GError);
SCOPED(GList);
SCOPED(SecretValue);

#undef SCOPED

class LinuxSecretStore {
public:
  LinuxSecretStore();
  ~LinuxSecretStore(){};
  std::string get_secret(const std::string &alias);

private:
  void init_secret_service();
  void get_secret_collection(const std::string &collection_alias);

  ScopedSecretService ss;
  SecretCollection *sc;
};

#endif // gnome_keystore_h__
