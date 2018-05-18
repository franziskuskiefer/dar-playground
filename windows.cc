#include <windows.h>
#include <wincred.h>
#include <tchar.h>
#include <stdio.h>

int main() {
    // Store value.
    char* password = "super secret string";
    DWORD cbCreds = 1 + strlen(password);

    CREDENTIALW cred = {0};
    cred.Type = CRED_TYPE_GENERIC;
    cred.TargetName = L"Mozilla/Firefox";
    cred.CredentialBlobSize = cbCreds;
    cred.CredentialBlob = (LPBYTE) password;
    cred.Persist = CRED_PERSIST_LOCAL_MACHINE;
    cred.UserName = L"";

    BOOL ok = ::CredWriteW (&cred, 0);
    wprintf (L"CredWrite() - errno %d\n", ok ? 0 : ::GetLastError());
    if (!ok) {
        exit(1);
    }

    // Read value.
    PCREDENTIALW pcred;
    ok = ::CredReadW(L"Mozilla/Firefox",
                          CRED_TYPE_GENERIC, 0, &pcred);
    wprintf(L"CredRead() - errno %d\n", ok ? 0 : ::GetLastError());
    if (!ok) {
        exit(1);
    }
    wprintf (L"Read username = '%s', password='%S' (%d bytes)\n", 
             pcred->UserName, (char*)pcred->CredentialBlob, pcred->CredentialBlobSize);
    ::CredFree(pcred);
    return 0;
}