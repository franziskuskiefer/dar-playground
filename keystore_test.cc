
#include "keystore.h"

int main() {
    Keystore ks;
    auto s = ks.RetrieveSecret("Firefox Secret");
    printf("secret: \"%s\"\n", s.c_str());
    return 0;
}
