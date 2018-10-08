
#include <string>
#include <stdio.h>
#include <windows.h>
#include <wincred.h>
#include <ntsecapi.h>

char* GetTokenInfo(HANDLE user_token)
{
    DWORD token_info_length = 0;
    GetTokenInformation(user_token, TokenUser, nullptr, 0, &token_info_length);
    char *token_info = new char[token_info_length];
    GetTokenInformation(user_token, TokenUser, token_info, token_info_length,
                        &token_info_length);
    CloseHandle(user_token);
    if (!token_info_length) {
        printf("Unable to obtain current token info %lx\n", GetLastError);
        return nullptr;
    }
    return token_info;
}

int main()
{
    // Get authentication handle.
    HANDLE lsa;
    if (LsaConnectUntrusted(&lsa) != ERROR_SUCCESS) {
        // couldn't get lsa handle, INVALID_HANDLE_VALUE
        printf("Error getting lsa\n");
        return 1;
    }

    // Get current user uid to make sure the same user got logged in.
    HANDLE user_token;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &user_token)) {
        printf("Unable to obtain process token %lx\n", GetLastError);
        return 1;
    }

    DWORD token_info_length = 0;
    GetTokenInformation(user_token, TokenUser, nullptr, 0, &token_info_length);
    char *cur_token_info = new char[token_info_length];
    GetTokenInformation(user_token, TokenUser, cur_token_info, token_info_length, &token_info_length);
    CloseHandle(user_token);
    if (!token_info_length) {
        printf("Unable to obtain current token info %lx\n", GetLastError);
        return 1;
    }
    PSID cur_sid = reinterpret_cast<TOKEN_USER*>(cur_token_info)->User.Sid;

    // CredUI prompt.
    CREDUI_INFOW credui = {};
    credui.cbSize = sizeof(credui);  
    // TODO: set parent (Firefox) here.
    credui.hwndParent = nullptr;
    // credui.pszMessageText = strMsg.c_str();
    credui.pszCaptionText = L"Please verify your Windows user credentials to proceed.";
    credui.hbmBanner = nullptr;

    ULONG authPackage = 0;  
    LPVOID outCredBuffer = nullptr;  
    ULONG outCredSize = 0;  
    BOOL save = false;
    // Used in next iteration if the previous login failed.
    DWORD err = 0;

    // Get user's Windows credentials.
    // Maybe add CREDUIWIN_AUTHPACKAGE_ONLY?
    err = CredUIPromptForWindowsCredentialsW(&credui, err, &authPackage,
                            nullptr, 0, &outCredBuffer, &outCredSize, &save,
                            CREDUIWIN_ENUMERATE_CURRENT_USER);
    if (err != ERROR_SUCCESS) {
        printf("Error getting authPackage %lx - %x\n", outCredSize, err);
        return 1;
    }
    printf("result: %lx\n", err);

    // WCHAR pszName[CREDUI_MAX_USERNAME_LENGTH*sizeof(WCHAR)];
    // WCHAR pszPwd[CREDUI_MAX_PASSWORD_LENGTH*sizeof(WCHAR)];
    // WCHAR domain[CREDUI_MAX_DOMAIN_TARGET_LENGTH*sizeof(WCHAR)];
    // DWORD maxLenName =  CREDUI_MAX_USERNAME_LENGTH+1;
    // DWORD maxLenPassword =  CREDUI_MAX_PASSWORD_LENGTH+1;
    // DWORD maxLenDomain = CREDUI_MAX_DOMAIN_TARGET_LENGTH+1;
    // CredUnPackAuthenticationBufferW(0, outCredBuffer,
    //                                 outCredSize,
    //                                 pszName,
    //                                 &maxLenName,
    //                                 domain,
    //                                 &maxLenDomain,
    //                                 pszPwd,
    //                                 &maxLenPassword);
    // wprintf(L"Credentials:\n  user: \"%s\"\n  pwd: \"%s\"\n  domain: \"%s\"\n",
    //         pszName, pszPwd, domain);

    // Verify the credentials
    NTSTATUS substs;
    MSV1_0_INTERACTIVE_PROFILE* profile_buffer = nullptr;
    ULONG profile_buffer_length = 0;
    QUOTA_LIMITS limits = {0};
    LUID luid;
    HANDLE token;
    LSA_STRING name;
    PCHAR context_name = const_cast<PCHAR>("Firefox");
    name.Buffer = context_name;
    name.Length = strlen(name.Buffer);
    name.MaximumLength = name.Length;

    TOKEN_SOURCE source;
    strcpy_s(source.SourceName, sizeof(context_name)/sizeof(*context_name),
             "Firefox");
    if (!AllocateLocallyUniqueId(&source.SourceIdentifier)) {
        printf("Last error: %lx\n", GetLastError());
        return 1;
    }

    NTSTATUS sts = LsaLogonUser(lsa, &name, (SECURITY_LOGON_TYPE)Interactive,
                        authPackage, outCredBuffer,
                        outCredSize, nullptr, &source, (PVOID*)&profile_buffer,
                        &profile_buffer_length, &luid, &token, &limits, &substs);
    if (sts == ERROR_SUCCESS) {
        printf("Logged in successfully.\n");
    } else {
        printf("Login failed with: %lx (%lx)\n", sts, LsaNtStatusToWinError(sts));
        printf("%lx\n", luid);
        return 1;
    }
    LsaFreeReturnBuffer(profile_buffer);

    // Make sure that the logged in user is the current user.
    char *logon_token_info = GetTokenInfo(token);
    if (!logon_token_info) {
        printf("Error getting logon token info\n");
        return 1;
    }
    PSID logon_sid = reinterpret_cast<TOKEN_USER*>(logon_token_info)->User.Sid;
    if (EqualSid(cur_sid, logon_sid)) {
        printf("Login successfully (correct user)\n");
    } else {
        printf("Login failed (wrong user)\n");
        return 1;
    }
}
