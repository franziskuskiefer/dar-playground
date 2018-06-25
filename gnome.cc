
#include <stdio.h>
#include <string>

#include "gnome.h"

LinuxSecretStore::LinuxSecretStore() : ss(nullptr), sc(nullptr) {
  init_secret_service();
  get_secret_collection("default");
}

void LinuxSecretStore::init_secret_service() {
  ScopedGError error = nullptr;
  GError *raw_error = error.get();
  // Get secret service
  this->ss = ScopedSecretService(secret_service_get_sync(
      static_cast<SecretServiceFlags>(0) /* SecretServiceFlags */,
      nullptr /* GCancellable */, &raw_error));
  if (raw_error || !ss) {
    printf("Couldn't get a secret service.\n");
    return;
  }

  // Make sure the service is loaded.
  if (!secret_service_load_collections_sync(ss.get(), nullptr, &raw_error)) {
    printf("Couldn't load the secret service.\n");
  }
}

void LinuxSecretStore::get_secret_collection(
    const std::string &collection_alias) {
  ScopedGError error = nullptr;
  GError *raw_error = error.get();
  this->sc =
      secret_collection_for_alias_sync(this->ss.get(), collection_alias.c_str(),
                                       static_cast<SecretCollectionFlags>(0),
                                       nullptr /* GCancellable */, &raw_error);
  if (!this->sc || error) {
    printf("Couldn't get \"%s\" collection.\n", collection_alias.c_str());
  }
}

std::string LinuxSecretStore::get_secret(const std::string &alias) {
  ScopedGError error = nullptr;
  GError *raw_error = error.get();
  // Get list of items in the collection and unlock them (unlock might prompt
  // for the password).
  ScopedGList items(secret_collection_get_items(this->sc));
  GList *locked_items = nullptr;
  ScopedGList scoped_locked_items(locked_items);
  // int locked_items_len =
  (void)secret_service_unlock_sync(this->ss.get(), items.get(),
                                   nullptr /* GCancellable */, &locked_items,
                                   &raw_error);
  if (raw_error) {
    printf("Couldn't lock items.\n");
    return "e1";
  }
  for (GList *elem = locked_items; elem; elem = elem->next) {
    SecretItem *item = static_cast<SecretItem *>(elem->data);
    if (!item) {
      printf("Couldn't get item.\n");
      return "e2";
    }
    if (!secret_item_load_secret_sync(item, nullptr /* GCancellable */,
                                      &raw_error)) {
      printf("Couldn't load secret item. It's locked.\n");
      continue;
    }
    if (raw_error) {
      printf("Couldn't load item.\n");
      return "e3";
    }
    ScopedSecretValue secret(secret_item_get_secret(item));
    if (!secret) {
      printf("Secret is locked or has not been loaded yet.\n");
      continue;
    }
    std::string label(secret_item_get_label(item));
    if (label.compare(alias) == 0) {
        // TODO: what's the difference between text and secret_value_get?
        std::string s2(secret_value_get_text(secret.get()));
        return s2;
    }
  }
  return "didn't find it";
}

// int main() {
//   GError *error = nullptr;
//   // Remote error from secret service:
//   org.freedesktop.DBus.Error.NotSupported:
//   // Only the 'default' alias is supported
//   const std::string collection_alias = "default";

//   // Get secret service
//   SecretService *ss = secret_service_get_sync(
//       static_cast<SecretServiceFlags>(0) /* SecretServiceFlags */,
//       nullptr /* GCancellable */, &error);
//   if (error) {
//     printf("Couldn't get a secret service.\n");
//     g_error_free(error);
//     return 1;
//   }

//   // Make sure the service is loaded.
//   if (!secret_service_load_collections_sync(ss, NULL, &error)) {
//     printf("Couldn't load the secret service.\n");
//     return 1;
//   }
//   error = nullptr;

//   // List all collections.
//   GList *collections = secret_service_get_collections(ss);
//   if (!collections) {
//     printf("There aren't any collections here.\n");
//     return 1;
//   }
//   bool found_collection = false;
//   for (GList *elem = collections; elem; elem = elem->next) {
//     SecretCollection *collection = static_cast<SecretCollection
//     *>(elem->data); std::string
//     label(secret_collection_get_label(collection)); printf("Found collection
//     \"%s\"\n", label.c_str()); if (label.compare(collection_alias) == 0) {
//       printf("Found my collection.\n");
//       found_collection = true;
//       break;
//     }
//   }
//   g_list_free(collections);

//   // The default collection seems to be named Login for me...
//   if (!found_collection) {
//     //   printf("Couldn't find my collection \"%s\".\n",
//     //   collection_alias.c_str()); printf("Creating collection \"%s\".\n",
//     //   collection_alias.c_str()); SecretCollection *new_col =
//     //   secret_collection_create_sync(
//     //       ss, "DAR testing collection", collection_alias.c_str(),
//     //       static_cast<SecretCollectionCreateFlags>(
//     //           0) /* SecretCollectionCreateFlags */,
//     //       nullptr /* GCancellable */, &error);
//     //   if (new_col) {
//     //     g_object_unref(new_col);
//     //   }
//     //   if (error) {
//     //     printf("Couldn't create new collection.\n");
//     //     g_error_free(error);
//     //     return 1;
//     //   }
//     //   error = nullptr;
//   }

//   // Get collection.
//   SecretCollection *sc = secret_collection_for_alias_sync(
//       ss, collection_alias.c_str(), static_cast<SecretCollectionFlags>(0),
//       nullptr /* GCancellable */, &error);
//   if (!sc) {
//     printf("Couldn't get \"%s\" collection.\n", collection_alias.c_str());
//     return 1;
//   }
//   error = nullptr;

//   // Store something in the collection.
//   // SecretSchema *my_schema =
//   //     secret_schema_new("org.mozilla.secret", SECRET_SCHEMA_NONE,
//   //                       {"string", SECRET_SCHEMA_ATTRIBUTE_STRING});
//   SecretValue *my_value =
//       secret_value_new("very secret string", 18, "text/text");
//   SecretItem *my_secret = secret_item_create_sync(
//       sc, nullptr /* SecretSchema */, g_hash_table_new(nullptr, nullptr),
//       "Firefox Secret", my_value, static_cast<SecretItemCreateFlags>(0),
//       nullptr /* GCancellable * cancellable */, &error);
//   if (!my_secret) {
//     printf("Couldn't create new secret.\n");
//     return 1;
//   }
//   g_object_unref(my_secret);

//   // Get list of items in the collection and unlock them (unlock might prompt
//   // for the password).
//   GList *items = secret_collection_get_items(sc);
//   GList *locked_items;
//   int locked_items_len = secret_service_unlock_sync(
//       ss, items, nullptr /* GCancellable */, &locked_items, &error);
//   printf("locked_items_len %d\n", locked_items_len);
//   for (GList *elem = locked_items; elem; elem = elem->next) {
//     SecretItem *item = static_cast<SecretItem *>(elem->data);
//     if (!item) {
//       printf("Couldn't get item.\n");
//       return 1;
//     }
//     if (!secret_item_load_secret_sync(item, nullptr /* GCancellable */,
//                                       &error)) {
//       printf("Couldn't load secret item. It's locked.\n");
//       continue;
//     }
//     error = nullptr;
//     SecretValue *secret = secret_item_get_secret(item);
//     if (!secret) {
//       printf("Secret is locked or has not been loaded yet.\n");
//       continue;
//     }
//     const char *label = secret_item_get_label(item);
//     printf("secret label: %s\n", label);
//     size_t s_len = 0;
//     const char *s = secret_value_get(secret, &s_len);
//     printf("secret: %.*s (%lu)\n", static_cast<int>(s_len), s, s_len);
//     s = secret_value_get_text(secret);
//     printf("secret text: %s\n", s);
//     secret_value_unref(secret);
//     // GHashTable *attributes = secret_item_get_attributes(item);
//   }
//   g_list_free(items);
//   g_list_free(locked_items);

//   return 0;
// }
