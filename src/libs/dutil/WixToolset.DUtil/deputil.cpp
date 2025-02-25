// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.

#include "precomp.h"


// Exit macros
#define DepExitOnLastError(x, s, ...) ExitOnLastErrorSource(DUTIL_SOURCE_DEPUTIL, x, s, __VA_ARGS__)
#define DepExitOnLastErrorDebugTrace(x, s, ...) ExitOnLastErrorDebugTraceSource(DUTIL_SOURCE_DEPUTIL, x, s, __VA_ARGS__)
#define DepExitWithLastError(x, s, ...) ExitWithLastErrorSource(DUTIL_SOURCE_DEPUTIL, x, s, __VA_ARGS__)
#define DepExitOnFailure(x, s, ...) ExitOnFailureSource(DUTIL_SOURCE_DEPUTIL, x, s, __VA_ARGS__)
#define DepExitOnRootFailure(x, s, ...) ExitOnRootFailureSource(DUTIL_SOURCE_DEPUTIL, x, s, __VA_ARGS__)
#define DepExitOnFailureDebugTrace(x, s, ...) ExitOnFailureDebugTraceSource(DUTIL_SOURCE_DEPUTIL, x, s, __VA_ARGS__)
#define DepExitOnNull(p, x, e, s, ...) ExitOnNullSource(DUTIL_SOURCE_DEPUTIL, p, x, e, s, __VA_ARGS__)
#define DepExitOnNullWithLastError(p, x, s, ...) ExitOnNullWithLastErrorSource(DUTIL_SOURCE_DEPUTIL, p, x, s, __VA_ARGS__)
#define DepExitOnNullDebugTrace(p, x, e, s, ...)  ExitOnNullDebugTraceSource(DUTIL_SOURCE_DEPUTIL, p, x, e, s, __VA_ARGS__)
#define DepExitOnInvalidHandleWithLastError(p, x, s, ...) ExitOnInvalidHandleWithLastErrorSource(DUTIL_SOURCE_DEPUTIL, p, x, s, __VA_ARGS__)
#define DepExitOnWin32Error(e, x, s, ...) ExitOnWin32ErrorSource(DUTIL_SOURCE_DEPUTIL, e, x, s, __VA_ARGS__)
#define DepExitOnGdipFailure(g, x, s, ...) ExitOnGdipFailureSource(DUTIL_SOURCE_DEPUTIL, g, x, s, __VA_ARGS__)

#define ARRAY_GROWTH_SIZE 5

static LPCWSTR vcszVersionValue = L"Version";
static LPCWSTR vcszDisplayNameValue = L"DisplayName";
static LPCWSTR vcszMinVersionValue = L"MinVersion";
static LPCWSTR vcszMaxVersionValue = L"MaxVersion";
static LPCWSTR vcszAttributesValue = L"Attributes";

enum eRequiresAttributes { RequiresAttributesMinVersionInclusive = 256, RequiresAttributesMaxVersionInclusive = 512 };

// We write to Software\Classes explicitly based on the current security context instead of HKCR.
// See http://msdn.microsoft.com/en-us/library/ms724475(VS.85).aspx for more information.
static LPCWSTR vsczRegistryRoot = L"Software\\Classes\\Installer\\Dependencies\\";
static LPCWSTR vsczRegistryDependents = L"Dependents";

static HRESULT AllocDependencyKeyName(
    __in_z LPCWSTR wzName,
    __deref_out_z LPWSTR* psczKeyName
    );

static HRESULT GetDependencyNameFromKey(
    __in HKEY hkHive,
    __in LPCWSTR wzKey,
    __deref_out_z LPWSTR* psczName
    );

DAPI_(HRESULT) DepGetProviderInformation(
    __in HKEY hkHive,
    __in_z LPCWSTR wzProviderKey,
    __deref_out_z_opt LPWSTR* psczId,
    __deref_out_z_opt LPWSTR* psczName,
    __deref_out_z_opt LPWSTR* psczVersion
    )
{
    HRESULT hr = S_OK;
    LPWSTR sczKey = NULL;
    HKEY hkKey = NULL;

    // Format the provider dependency registry key.
    hr = AllocDependencyKeyName(wzProviderKey, &sczKey);
    DepExitOnFailure(hr, "Failed to allocate the registry key for dependency \"%ls\".", wzProviderKey);

    // Try to open the dependency key.
    hr = RegOpen(hkHive, sczKey, KEY_READ, &hkKey);
    if (E_FILENOTFOUND == hr)
    {
        ExitFunction1(hr = E_NOTFOUND);
    }
    DepExitOnFailure(hr, "Failed to open the registry key for the dependency \"%ls\".", wzProviderKey);

    // Get the Id if requested and available.
    if (psczId)
    {
        hr = RegReadString(hkKey, NULL, psczId);
        if (E_FILENOTFOUND == hr)
        {
            hr = S_OK;
        }
        DepExitOnFailure(hr, "Failed to get the id for the dependency \"%ls\".", wzProviderKey);
    }

    // Get the DisplayName if requested and available.
    if (psczName)
    {
        hr = RegReadString(hkKey, vcszDisplayNameValue, psczName);
        if (E_FILENOTFOUND == hr)
        {
            hr = S_OK;
        }
        DepExitOnFailure(hr, "Failed to get the name for the dependency \"%ls\".", wzProviderKey);
    }

    // Get the Version if requested and available.
    if (psczVersion)
    {
        hr = RegReadString(hkKey, vcszVersionValue, psczVersion);
        if (E_FILENOTFOUND == hr)
        {
            hr = S_OK;
        }
        DepExitOnFailure(hr, "Failed to get the version for the dependency \"%ls\".", wzProviderKey);
    }

LExit:
    ReleaseRegKey(hkKey);
    ReleaseStr(sczKey);

    return hr;
}

DAPI_(HRESULT) DepCheckDependency(
    __in HKEY hkHive,
    __in_z LPCWSTR wzProviderKey,
    __in_z_opt LPCWSTR wzMinVersion,
    __in_z_opt LPCWSTR wzMaxVersion,
    __in int iAttributes,
    __in STRINGDICT_HANDLE sdDependencies,
    __deref_inout_ecount_opt(*pcDependencies) DEPENDENCY** prgDependencies,
    __inout LPUINT pcDependencies
    )
{
    HRESULT hr = S_OK;
    LPWSTR sczKey = NULL;
    HKEY hkKey = NULL;
    VERUTIL_VERSION* pVersion = NULL;
    VERUTIL_VERSION* pMinVersion = NULL;
    VERUTIL_VERSION* pMaxVersion = NULL;
    int nResult = 0;
    BOOL fAllowEqual = FALSE;
    LPWSTR sczName = NULL;

    // Format the provider dependency registry key.
    hr = AllocDependencyKeyName(wzProviderKey, &sczKey);
    DepExitOnFailure(hr, "Failed to allocate the registry key for dependency \"%ls\".", wzProviderKey);

    // Try to open the key. If that fails, add the missing dependency key to the dependency array if it doesn't already exist.
    hr = RegOpen(hkHive, sczKey, KEY_READ, &hkKey);
    if (E_FILENOTFOUND != hr)
    {
        DepExitOnFailure(hr, "Failed to open the registry key for dependency \"%ls\".", wzProviderKey);

        // If there are no registry values, consider the key orphaned and treat it as missing.
        hr = RegReadWixVersion(hkKey, vcszVersionValue, &pVersion);
        if (E_FILENOTFOUND != hr)
        {
            DepExitOnFailure(hr, "Failed to read the %ls registry value for dependency \"%ls\".", vcszVersionValue, wzProviderKey);
        }
    }

    // If the key was not found or the Version value was not found, add the missing dependency key to the dependency array.
    if (E_FILENOTFOUND == hr)
    {
        hr = DictKeyExists(sdDependencies, wzProviderKey);
        if (E_NOTFOUND != hr)
        {
            DepExitOnFailure(hr, "Failed to check the dictionary for missing dependency \"%ls\".", wzProviderKey);
        }
        else
        {
            hr = DepDependencyArrayAlloc(prgDependencies, pcDependencies, wzProviderKey, NULL);
            DepExitOnFailure(hr, "Failed to add the missing dependency \"%ls\" to the array.", wzProviderKey);

            hr = DictAddKey(sdDependencies, wzProviderKey);
            DepExitOnFailure(hr, "Failed to add the missing dependency \"%ls\" to the dictionary.", wzProviderKey);
        }

        // Exit since the check already failed.
        ExitFunction1(hr = E_NOTFOUND);
    }

    // Check MinVersion if provided.
    if (wzMinVersion)
    {
        if (*wzMinVersion)
        {
            hr = VerParseVersion(wzMinVersion, 0, FALSE, &pMinVersion);
            DepExitOnFailure(hr, "Failed to get the min version number from \"%ls\".", wzMinVersion);

            hr = VerCompareParsedVersions(pVersion, pMinVersion, &nResult);
            DepExitOnFailure(hr, "Failed to compare dependency with min version \"%ls\".", wzMinVersion);

            fAllowEqual = iAttributes & RequiresAttributesMinVersionInclusive;
            //  !(fAllowEqual && pMinVersion <= pVersion || pMinVersion < pVersion))
            if (!(fAllowEqual && 0 <= nResult || 0 < nResult))
            {
                hr = DictKeyExists(sdDependencies, wzProviderKey);
                if (E_NOTFOUND != hr)
                {
                    DepExitOnFailure(hr, "Failed to check the dictionary for the older dependency \"%ls\".", wzProviderKey);
                }
                else
                {
                    hr = RegReadString(hkKey, vcszDisplayNameValue, &sczName);
                    DepExitOnFailure(hr, "Failed to get the display name of the older dependency \"%ls\".", wzProviderKey);

                    hr = DepDependencyArrayAlloc(prgDependencies, pcDependencies, wzProviderKey, sczName);
                    DepExitOnFailure(hr, "Failed to add the older dependency \"%ls\" to the dependencies array.", wzProviderKey);

                    hr = DictAddKey(sdDependencies, wzProviderKey);
                    DepExitOnFailure(hr, "Failed to add the older dependency \"%ls\" to the unique dependency string list.", wzProviderKey);
                }

                // Exit since the check already failed.
                ExitFunction1(hr = E_NOTFOUND);
            }
        }
    }

    // Check MaxVersion if provided.
    if (wzMaxVersion)
    {
        if (*wzMaxVersion)
        {
            hr = VerParseVersion(wzMaxVersion, 0, FALSE, &pMaxVersion);
            DepExitOnFailure(hr, "Failed to get the max version number from \"%ls\".", wzMaxVersion);

            hr = VerCompareParsedVersions(pMaxVersion, pVersion, &nResult);
            DepExitOnFailure(hr, "Failed to compare dependency with max version \"%ls\".", wzMaxVersion);

            fAllowEqual = iAttributes & RequiresAttributesMaxVersionInclusive;
            //  !(fAllowEqual && pVersion <= pMaxVersion || pVersion < pMaxVersion)
            if (!(fAllowEqual && 0 <= nResult || 0 < nResult))
            {
                hr = DictKeyExists(sdDependencies, wzProviderKey);
                if (E_NOTFOUND != hr)
                {
                    DepExitOnFailure(hr, "Failed to check the dictionary for the newer dependency \"%ls\".", wzProviderKey);
                }
                else
                {
                    hr = RegReadString(hkKey, vcszDisplayNameValue, &sczName);
                    DepExitOnFailure(hr, "Failed to get the display name of the newer dependency \"%ls\".", wzProviderKey);

                    hr = DepDependencyArrayAlloc(prgDependencies, pcDependencies, wzProviderKey, sczName);
                    DepExitOnFailure(hr, "Failed to add the newer dependency \"%ls\" to the dependencies array.", wzProviderKey);

                    hr = DictAddKey(sdDependencies, wzProviderKey);
                    DepExitOnFailure(hr, "Failed to add the newer dependency \"%ls\" to the unique dependency string list.", wzProviderKey);
                }

                // Exit since the check already failed.
                ExitFunction1(hr = E_NOTFOUND);
            }
        }
    }

LExit:
    ReleaseVerutilVersion(pMaxVersion);
    ReleaseVerutilVersion(pMinVersion);
    ReleaseVerutilVersion(pVersion);
    ReleaseStr(sczName);
    ReleaseRegKey(hkKey);
    ReleaseStr(sczKey);

    return hr;
}

DAPI_(HRESULT) DepCheckDependents(
    __in HKEY hkHive,
    __in_z LPCWSTR wzProviderKey,
    __reserved int /*iAttributes*/,
    __in_opt C_STRINGDICT_HANDLE sdIgnoredDependents,
    __deref_inout_ecount_opt(*pcDependents) DEPENDENCY** prgDependents,
    __inout LPUINT pcDependents
    )
{
    HRESULT hr = S_OK;
    LPWSTR sczKey = NULL;
    HKEY hkProviderKey = NULL;
    HKEY hkDependentsKey = NULL;
    LPWSTR sczDependentKey = NULL;
    LPWSTR sczDependentName = NULL;
    BOOL fIgnore = FALSE;

    // Format the provider dependency registry key.
    hr = AllocDependencyKeyName(wzProviderKey, &sczKey);
    DepExitOnFailure(hr, "Failed to allocate the registry key for dependency \"%ls\".", wzProviderKey);

    // Try to open the key. If that fails, the dependency information is corrupt.
    hr = RegOpen(hkHive, sczKey, KEY_READ, &hkProviderKey);
    DepExitOnFailure(hr, "Failed to open the registry key \"%ls\". The dependency store is corrupt.", sczKey);

    // Try to open the dependencies key. If that does not exist, there are no dependents.
    hr = RegOpen(hkProviderKey, vsczRegistryDependents, KEY_READ, &hkDependentsKey);
    if (E_FILENOTFOUND != hr)
    {
        DepExitOnFailure(hr, "Failed to open the registry key for dependents of \"%ls\".", wzProviderKey);
    }
    else
    {
        ExitFunction1(hr = S_OK);
    }

    // Now enumerate the dependent keys. If they are not defined in the ignored list, add them to the array.
    for (DWORD dwIndex = 0; ; ++dwIndex)
    {
        fIgnore = FALSE;

        hr = RegKeyEnum(hkDependentsKey, dwIndex, &sczDependentKey);
        if (E_NOMOREITEMS != hr)
        {
            DepExitOnFailure(hr, "Failed to enumerate the dependents key of \"%ls\".", wzProviderKey);
        }
        else
        {
            hr = S_OK;
            break;
        }

        // If the key isn't ignored, add it to the dependent array.
        if (sdIgnoredDependents)
        {
            hr = DictKeyExists(sdIgnoredDependents, sczDependentKey);
            if (E_NOTFOUND != hr)
            {
                DepExitOnFailure(hr, "Failed to check the dictionary of ignored dependents.");

                fIgnore = TRUE;
            }
        }

        if (!fIgnore)
        {
            // Get the name of the dependent from the key.
            hr = GetDependencyNameFromKey(hkHive, sczDependentKey, &sczDependentName);
            DepExitOnFailure(hr, "Failed to get the name of the dependent from the key \"%ls\".", sczDependentKey);

            hr = DepDependencyArrayAlloc(prgDependents, pcDependents, sczDependentKey, sczDependentName);
            DepExitOnFailure(hr, "Failed to add the dependent key \"%ls\" to the string array.", sczDependentKey);
        }
    }

LExit:
    ReleaseStr(sczDependentName);
    ReleaseStr(sczDependentKey);
    ReleaseRegKey(hkDependentsKey);
    ReleaseRegKey(hkProviderKey);
    ReleaseStr(sczKey);

    return hr;
}

DAPI_(HRESULT) DepRegisterDependency(
    __in HKEY hkHive,
    __in_z LPCWSTR wzProviderKey,
    __in_z LPCWSTR wzVersion,
    __in_z LPCWSTR wzDisplayName,
    __in_z_opt LPCWSTR wzId,
    __in int iAttributes
    )
{
    HRESULT hr = S_OK;
    LPWSTR sczKey = NULL;
    HKEY hkKey = NULL;
    BOOL fCreated = FALSE;

    // Format the provider dependency registry key.
    hr = AllocDependencyKeyName(wzProviderKey, &sczKey);
    DepExitOnFailure(hr, "Failed to allocate the registry key for dependency \"%ls\".", wzProviderKey);

    // Create the dependency key (or open it if it already exists).
    hr = RegCreateEx(hkHive, sczKey, KEY_WRITE, REG_KEY_DEFAULT, FALSE, NULL, &hkKey, &fCreated);
    DepExitOnFailure(hr, "Failed to create the dependency registry key \"%ls\".", sczKey);

    // Set the id if it was provided.
    if (wzId)
    {
        hr = RegWriteString(hkKey, NULL, wzId);
        DepExitOnFailure(hr, "Failed to set the %ls registry value to \"%ls\".", L"default", wzId);
    }

    // Set the version.
    hr = RegWriteString(hkKey, vcszVersionValue, wzVersion);
    DepExitOnFailure(hr, "Failed to set the %ls registry value to \"%ls\".", vcszVersionValue, wzVersion);

    // Set the display name.
    hr = RegWriteString(hkKey, vcszDisplayNameValue, wzDisplayName);
    DepExitOnFailure(hr, "Failed to set the %ls registry value to \"%ls\".", vcszDisplayNameValue, wzDisplayName);

    // Set the attributes if non-zero.
    if (0 != iAttributes)
    {
        hr = RegWriteNumber(hkKey, vcszAttributesValue, static_cast<DWORD>(iAttributes));
        DepExitOnFailure(hr, "Failed to set the %ls registry value to %d.", vcszAttributesValue, iAttributes);
    }

LExit:
    ReleaseRegKey(hkKey);
    ReleaseStr(sczKey);

    return hr;
}

DAPI_(HRESULT) DepDependentExists(
    __in HKEY hkHive,
    __in_z LPCWSTR wzDependencyProviderKey,
    __in_z LPCWSTR wzProviderKey
    )
{
    HRESULT hr = S_OK;
    LPWSTR sczDependentKey = NULL;
    HKEY hkDependentKey = NULL;

    // Format the provider dependents registry key.
    hr = StrAllocFormatted(&sczDependentKey, L"%ls%ls\\%ls\\%ls", vsczRegistryRoot, wzDependencyProviderKey, vsczRegistryDependents, wzProviderKey);
    DepExitOnFailure(hr, "Failed to format registry key to dependent.");

    hr = RegOpen(hkHive, sczDependentKey, KEY_READ, &hkDependentKey);
    if (E_FILENOTFOUND != hr)
    {
        DepExitOnFailure(hr, "Failed to open the dependent registry key at: \"%ls\".", sczDependentKey);
    }

LExit:
    ReleaseRegKey(hkDependentKey);
    ReleaseStr(sczDependentKey);

    return hr;
}

DAPI_(HRESULT) DepRegisterDependent(
    __in HKEY hkHive,
    __in_z LPCWSTR wzDependencyProviderKey,
    __in_z LPCWSTR wzProviderKey,
    __in_z_opt LPCWSTR wzMinVersion,
    __in_z_opt LPCWSTR wzMaxVersion,
    __in int iAttributes
    )
{
    HRESULT hr = S_OK;
    LPWSTR sczDependencyKey = NULL;
    HKEY hkDependencyKey = NULL;
    LPWSTR sczKey = NULL;
    HKEY hkKey = NULL;
    BOOL fCreated = FALSE;

    // Format the provider dependency registry key.
    hr = AllocDependencyKeyName(wzDependencyProviderKey, &sczDependencyKey);
    DepExitOnFailure(hr, "Failed to allocate the registry key for dependency \"%ls\".", wzDependencyProviderKey);

    // Create the dependency key (or open it if it already exists).
    hr = RegCreateEx(hkHive, sczDependencyKey, KEY_WRITE, REG_KEY_DEFAULT, FALSE, NULL, &hkDependencyKey, &fCreated);
    DepExitOnFailure(hr, "Failed to create the dependency registry key \"%ls\".", sczDependencyKey);

    // Create the subkey to register the dependent.
    hr = StrAllocFormatted(&sczKey, L"%ls\\%ls", vsczRegistryDependents, wzProviderKey);
    DepExitOnFailure(hr, "Failed to allocate dependent subkey \"%ls\" under dependency \"%ls\".", wzProviderKey, wzDependencyProviderKey);

    hr = RegCreateEx(hkDependencyKey, sczKey, KEY_WRITE, REG_KEY_DEFAULT, FALSE, NULL, &hkKey, &fCreated);
    DepExitOnFailure(hr, "Failed to create the dependency subkey \"%ls\".", sczKey);

    // Set the minimum version if not NULL.
    hr = RegWriteString(hkKey, vcszMinVersionValue, wzMinVersion);
    DepExitOnFailure(hr, "Failed to set the %ls registry value to \"%ls\".", vcszMinVersionValue, wzMinVersion);

    // Set the maximum version if not NULL.
    hr = RegWriteString(hkKey, vcszMaxVersionValue, wzMaxVersion);
    DepExitOnFailure(hr, "Failed to set the %ls registry value to \"%ls\".", vcszMaxVersionValue, wzMaxVersion);

    // Set the attributes if non-zero.
    if (0 != iAttributes)
    {
        hr = RegWriteNumber(hkKey, vcszAttributesValue, static_cast<DWORD>(iAttributes));
        DepExitOnFailure(hr, "Failed to set the %ls registry value to %d.", vcszAttributesValue, iAttributes);
    }

LExit:
    ReleaseRegKey(hkKey);
    ReleaseStr(sczKey);
    ReleaseRegKey(hkDependencyKey);
    ReleaseStr(sczDependencyKey);

    return hr;
}

DAPI_(HRESULT) DepUnregisterDependency(
    __in HKEY hkHive,
    __in_z LPCWSTR wzProviderKey
    )
{
    HRESULT hr = S_OK;
    LPWSTR sczKey = NULL;
    HKEY hkKey = NULL;

    // Format the provider dependency registry key.
    hr = AllocDependencyKeyName(wzProviderKey, &sczKey);
    DepExitOnFailure(hr, "Failed to allocate the registry key for dependency \"%ls\".", wzProviderKey);

    // Delete the entire key including all sub-keys.
    hr = RegDelete(hkHive, sczKey, REG_KEY_DEFAULT, TRUE);
    if (E_FILENOTFOUND != hr)
    {
        DepExitOnFailure(hr, "Failed to delete the key \"%ls\".", sczKey);
    }

LExit:
    ReleaseRegKey(hkKey);
    ReleaseStr(sczKey);

    return hr;
}

DAPI_(HRESULT) DepUnregisterDependent(
    __in HKEY hkHive,
    __in_z LPCWSTR wzDependencyProviderKey,
    __in_z LPCWSTR wzProviderKey
    )
{
    HRESULT hr = S_OK;
    HKEY hkRegistryRoot = NULL;
    HKEY hkDependencyProviderKey = NULL;
    HKEY hkRegistryDependents = NULL;
    DWORD cSubKeys = 0;
    DWORD cValues = 0;

    // Open the root key. We may delete the wzDependencyProviderKey during clean up.
    hr = RegOpen(hkHive, vsczRegistryRoot, KEY_READ, &hkRegistryRoot);
    if (E_FILENOTFOUND != hr)
    {
        DepExitOnFailure(hr, "Failed to open root registry key \"%ls\".", vsczRegistryRoot);
    }
    else
    {
        ExitFunction();
    }

    // Try to open the dependency key. If that does not exist, simply return.
    hr = RegOpen(hkRegistryRoot, wzDependencyProviderKey, KEY_READ, &hkDependencyProviderKey);
    if (E_FILENOTFOUND != hr)
    {
        DepExitOnFailure(hr, "Failed to open the registry key for the dependency \"%ls\".", wzDependencyProviderKey);
    }
    else
    {
        ExitFunction();
    }

    // Try to open the dependents subkey to enumerate.
    hr = RegOpen(hkDependencyProviderKey, vsczRegistryDependents, KEY_READ, &hkRegistryDependents);
    if (E_FILENOTFOUND != hr)
    {
        DepExitOnFailure(hr, "Failed to open the dependents subkey under the dependency \"%ls\".", wzDependencyProviderKey);
    }
    else
    {
        ExitFunction();
    }

    // Delete the wzProviderKey dependent sub-key.
    hr = RegDelete(hkRegistryDependents, wzProviderKey, REG_KEY_DEFAULT, TRUE);
    DepExitOnFailure(hr, "Failed to delete the dependent \"%ls\" under the dependency \"%ls\".", wzProviderKey, wzDependencyProviderKey);

    // If there are no remaining dependents, delete the Dependents subkey.
    hr = RegQueryKey(hkRegistryDependents, &cSubKeys, NULL);
    DepExitOnFailure(hr, "Failed to get the number of dependent subkeys under the dependency \"%ls\".", wzDependencyProviderKey);
    
    if (0 < cSubKeys)
    {
        ExitFunction();
    }

    // Release the handle to make sure it's deleted immediately.
    ReleaseRegKey(hkRegistryDependents);

    // Fail if there are any subkeys since we just checked.
    hr = RegDelete(hkDependencyProviderKey, vsczRegistryDependents, REG_KEY_DEFAULT, FALSE);
    DepExitOnFailure(hr, "Failed to delete the dependents subkey under the dependency \"%ls\".", wzDependencyProviderKey);

    // If there are no values, delete the provider dependency key.
    hr = RegQueryKey(hkDependencyProviderKey, NULL, &cValues);
    DepExitOnFailure(hr, "Failed to get the number of values under the dependency \"%ls\".", wzDependencyProviderKey);

    if (0 == cValues)
    {
        // Release the handle to make sure it's deleted immediately.
        ReleaseRegKey(hkDependencyProviderKey);

        // Fail if there are any subkeys since we just checked.
        hr = RegDelete(hkRegistryRoot, wzDependencyProviderKey, REG_KEY_DEFAULT, FALSE);
        DepExitOnFailure(hr, "Failed to delete the dependency \"%ls\".", wzDependencyProviderKey);
    }

LExit:
    ReleaseRegKey(hkRegistryDependents);
    ReleaseRegKey(hkDependencyProviderKey);
    ReleaseRegKey(hkRegistryRoot);

    return hr;
}

DAPI_(HRESULT) DepDependencyArrayAlloc(
    __deref_inout_ecount_opt(*pcDependencies) DEPENDENCY** prgDependencies,
    __inout LPUINT pcDependencies,
    __in_z LPCWSTR wzKey,
    __in_z_opt LPCWSTR wzName
    )
{
    HRESULT hr = S_OK;
    UINT cRequired = 0;
    DEPENDENCY* pDependency = NULL;

    hr = ::UIntAdd(*pcDependencies, 1, &cRequired);
    DepExitOnFailure(hr, "Failed to increment the number of elements required in the dependency array.");

    hr = MemEnsureArraySize(reinterpret_cast<LPVOID*>(prgDependencies), cRequired, sizeof(DEPENDENCY), ARRAY_GROWTH_SIZE);
    DepExitOnFailure(hr, "Failed to allocate memory for the dependency array.");

    pDependency = static_cast<DEPENDENCY*>(&(*prgDependencies)[*pcDependencies]);
    DepExitOnNull(pDependency, hr, E_POINTER, "The dependency element in the array is invalid.");

    hr = StrAllocString(&(pDependency->sczKey), wzKey, 0);
    DepExitOnFailure(hr, "Failed to allocate the string key in the dependency array.");

    if (wzName)
    {
        hr = StrAllocString(&(pDependency->sczName), wzName, 0);
        DepExitOnFailure(hr, "Failed to allocate the string name in the dependency array.");
    }

    // Update the number of current elements in the dependency array.
    *pcDependencies = cRequired;

LExit:
    return hr;
}

DAPI_(void) DepDependencyArrayFree(
    __in_ecount(cDependencies) DEPENDENCY* rgDependencies,
    __in UINT cDependencies
    )
{
    for (UINT i = 0; i < cDependencies; ++i)
    {
        ReleaseStr(rgDependencies[i].sczKey);
        ReleaseStr(rgDependencies[i].sczName);
    }

    ReleaseMem(rgDependencies);
}

/***************************************************************************
 AllocDependencyKeyName - Allocates and formats the root registry key name.

***************************************************************************/
static HRESULT AllocDependencyKeyName(
    __in_z LPCWSTR wzName,
    __deref_out_z LPWSTR* psczKeyName
    )
{
    HRESULT hr = S_OK;
    size_t cchName = 0;
    size_t cchKeyName = 0;

    // Get the length of the static registry root once.
    static size_t cchRegistryRoot = ::lstrlenW(vsczRegistryRoot);

    // Get the length of the dependency, and add to the length of the root.
    hr = ::StringCchLengthW(wzName, STRSAFE_MAX_CCH, &cchName);
    DepExitOnFailure(hr, "Failed to get string length of dependency name.");

    // Add the sizes together to allocate memory once (callee will add space for nul).
    hr = ::SizeTAdd(cchRegistryRoot, cchName, &cchKeyName);
    DepExitOnFailure(hr, "Failed to add the string lengths together.");

    // Allocate and concat the strings together.
    hr = StrAllocString(psczKeyName, vsczRegistryRoot, cchKeyName);
    DepExitOnFailure(hr, "Failed to allocate string for dependency registry root.");

    hr = StrAllocConcat(psczKeyName, wzName, cchName);
    DepExitOnFailure(hr, "Failed to concatenate the dependency key name.");

LExit:
    return hr;
}

/***************************************************************************
 GetDependencyNameFromKey - Attempts to name of the dependency from the key.

***************************************************************************/
static HRESULT GetDependencyNameFromKey(
    __in HKEY hkHive,
    __in LPCWSTR wzProviderKey,
    __deref_out_z LPWSTR* psczName
    )
{
    HRESULT hr = S_OK;
    LPWSTR sczKey = NULL;
    HKEY hkKey = NULL;

    // Format the provider dependency registry key.
    hr = AllocDependencyKeyName(wzProviderKey, &sczKey);
    DepExitOnFailure(hr, "Failed to allocate the registry key for dependency \"%ls\".", wzProviderKey);

    // Try to open the dependency key.
    hr = RegOpen(hkHive, sczKey, KEY_READ, &hkKey);
    if (E_FILENOTFOUND != hr)
    {
        DepExitOnFailure(hr, "Failed to open the registry key for the dependency \"%ls\".", wzProviderKey);
    }
    else
    {
        ExitFunction1(hr = S_OK);
    }

    // Get the DisplayName if available.
    hr = RegReadString(hkKey, vcszDisplayNameValue, psczName);
    if (E_FILENOTFOUND != hr)
    {
        DepExitOnFailure(hr, "Failed to get the dependency name for the dependency \"%ls\".", wzProviderKey);
    }
    else
    {
        ExitFunction1(hr = S_OK);
    }

LExit:
    ReleaseRegKey(hkKey);
    ReleaseStr(sczKey);

    return hr;
}
