#pragma once
// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.


#include "BalBaseBootstrapperApplicationProc.h"
#include "BAFunctions.h"
#include "IBAFunctions.h"

static HRESULT BalBaseBAFunctionsProcOnThemeLoaded(
    __in IBAFunctions* pBAFunctions,
    __in BA_FUNCTIONS_ONTHEMELOADED_ARGS* pArgs,
    __inout BA_FUNCTIONS_ONTHEMELOADED_RESULTS* /*pResults*/
    )
{
    return pBAFunctions->OnThemeLoaded(pArgs->hWnd);
}

static HRESULT BalBaseBAFunctionsProcWndProc(
    __in IBAFunctions* pBAFunctions,
    __in BA_FUNCTIONS_WNDPROC_ARGS* pArgs,
    __inout BA_FUNCTIONS_WNDPROC_RESULTS* pResults
    )
{
    return pBAFunctions->WndProc(pArgs->hWnd, pArgs->uMsg, pArgs->wParam, pArgs->lParam, &pResults->fProcessed, &pResults->lResult);
}

static HRESULT BalBaseBAFunctionsProcOnThemeControlLoading(
    __in IBAFunctions* pBAFunctions,
    __in BA_FUNCTIONS_ONTHEMECONTROLLOADING_ARGS* pArgs,
    __inout BA_FUNCTIONS_ONTHEMECONTROLLOADING_RESULTS* pResults
    )
{
    return pBAFunctions->OnThemeControlLoading(pArgs->wzName, &pResults->fProcessed, &pResults->wId, &pResults->fDisableAutomaticFunctionality);
}

static HRESULT BalBaseBAFunctionsProcOnThemeControlWmCommand(
    __in IBAFunctions* pBAFunctions,
    __in BA_FUNCTIONS_ONTHEMECONTROLWMCOMMAND_ARGS* pArgs,
    __inout BA_FUNCTIONS_ONTHEMECONTROLWMCOMMAND_RESULTS* pResults
    )
{
    return pBAFunctions->OnThemeControlWmCommand(pArgs->wParam, pArgs->wzName, pArgs->wId, pArgs->hWnd, &pResults->fProcessed, &pResults->lResult);
}

static HRESULT BalBaseBAFunctionsProcOnThemeControlWmNotify(
    __in IBAFunctions* pBAFunctions,
    __in BA_FUNCTIONS_ONTHEMECONTROLWMNOTIFY_ARGS* pArgs,
    __inout BA_FUNCTIONS_ONTHEMECONTROLWMNOTIFY_RESULTS* pResults
    )
{
    return pBAFunctions->OnThemeControlWmNotify(pArgs->lParam, pArgs->wzName, pArgs->wId, pArgs->hWnd, &pResults->fProcessed, &pResults->lResult);
}

static HRESULT BalBaseBAFunctionsProcOnThemeControlLoaded(
    __in IBAFunctions* pBAFunctions,
    __in BA_FUNCTIONS_ONTHEMECONTROLLOADED_ARGS* pArgs,
    __inout BA_FUNCTIONS_ONTHEMECONTROLLOADED_RESULTS* pResults
    )
{
    return pBAFunctions->OnThemeControlLoaded(pArgs->wzName, pArgs->wId, pArgs->hWnd, &pResults->fProcessed);
}

/*******************************************************************
BalBaseBAFunctionsProc - requires pvContext to be of type IBAFunctions.
Provides a default mapping between the message based BAFunctions interface and
the COM-based BAFunctions interface.

*******************************************************************/
static HRESULT WINAPI BalBaseBAFunctionsProc(
    __in BA_FUNCTIONS_MESSAGE message,
    __in const LPVOID pvArgs,
    __inout LPVOID pvResults,
    __in_opt LPVOID pvContext
    )
{
    IBAFunctions* pBAFunctions = reinterpret_cast<IBAFunctions*>(pvContext);
    HRESULT hr = pBAFunctions->BAFunctionsProc(message, pvArgs, pvResults, pvContext);

    if (E_NOTIMPL == hr)
    {
        switch (message)
        {
        case BA_FUNCTIONS_MESSAGE_ONDETECTBEGIN:
        case BA_FUNCTIONS_MESSAGE_ONDETECTCOMPLETE:
        case BA_FUNCTIONS_MESSAGE_ONPLANBEGIN:
        case BA_FUNCTIONS_MESSAGE_ONPLANCOMPLETE:
        case BA_FUNCTIONS_MESSAGE_ONSTARTUP:
        case BA_FUNCTIONS_MESSAGE_ONSHUTDOWN:
        case BA_FUNCTIONS_MESSAGE_ONDETECTFORWARDCOMPATIBLEBUNDLE:
        case BA_FUNCTIONS_MESSAGE_ONDETECTUPDATEBEGIN:
        case BA_FUNCTIONS_MESSAGE_ONDETECTUPDATE:
        case BA_FUNCTIONS_MESSAGE_ONDETECTUPDATECOMPLETE:
        case BA_FUNCTIONS_MESSAGE_ONDETECTRELATEDBUNDLE:
        case BA_FUNCTIONS_MESSAGE_ONDETECTPACKAGEBEGIN:
        case BA_FUNCTIONS_MESSAGE_ONDETECTRELATEDMSIPACKAGE:
        case BA_FUNCTIONS_MESSAGE_ONDETECTPATCHTARGET:
        case BA_FUNCTIONS_MESSAGE_ONDETECTMSIFEATURE:
        case BA_FUNCTIONS_MESSAGE_ONDETECTPACKAGECOMPLETE:
        case BA_FUNCTIONS_MESSAGE_ONPLANRELATEDBUNDLE:
        case BA_FUNCTIONS_MESSAGE_ONPLANPACKAGEBEGIN:
        case BA_FUNCTIONS_MESSAGE_ONPLANPATCHTARGET:
        case BA_FUNCTIONS_MESSAGE_ONPLANMSIFEATURE:
        case BA_FUNCTIONS_MESSAGE_ONPLANPACKAGECOMPLETE:
        case BA_FUNCTIONS_MESSAGE_ONAPPLYBEGIN:
        case BA_FUNCTIONS_MESSAGE_ONELEVATEBEGIN:
        case BA_FUNCTIONS_MESSAGE_ONELEVATECOMPLETE:
        case BA_FUNCTIONS_MESSAGE_ONPROGRESS:
        case BA_FUNCTIONS_MESSAGE_ONERROR:
        case BA_FUNCTIONS_MESSAGE_ONREGISTERBEGIN:
        case BA_FUNCTIONS_MESSAGE_ONREGISTERCOMPLETE:
        case BA_FUNCTIONS_MESSAGE_ONCACHEBEGIN:
        case BA_FUNCTIONS_MESSAGE_ONCACHEPACKAGEBEGIN:
        case BA_FUNCTIONS_MESSAGE_ONCACHEACQUIREBEGIN:
        case BA_FUNCTIONS_MESSAGE_ONCACHEACQUIREPROGRESS:
        case BA_FUNCTIONS_MESSAGE_ONCACHEACQUIRERESOLVING:
        case BA_FUNCTIONS_MESSAGE_ONCACHEACQUIRECOMPLETE:
        case BA_FUNCTIONS_MESSAGE_ONCACHEVERIFYBEGIN:
        case BA_FUNCTIONS_MESSAGE_ONCACHEVERIFYCOMPLETE:
        case BA_FUNCTIONS_MESSAGE_ONCACHEPACKAGECOMPLETE:
        case BA_FUNCTIONS_MESSAGE_ONCACHECOMPLETE:
        case BA_FUNCTIONS_MESSAGE_ONEXECUTEBEGIN:
        case BA_FUNCTIONS_MESSAGE_ONEXECUTEPACKAGEBEGIN:
        case BA_FUNCTIONS_MESSAGE_ONEXECUTEPATCHTARGET:
        case BA_FUNCTIONS_MESSAGE_ONEXECUTEPROGRESS:
        case BA_FUNCTIONS_MESSAGE_ONEXECUTEMSIMESSAGE:
        case BA_FUNCTIONS_MESSAGE_ONEXECUTEFILESINUSE:
        case BA_FUNCTIONS_MESSAGE_ONEXECUTEPACKAGECOMPLETE:
        case BA_FUNCTIONS_MESSAGE_ONEXECUTECOMPLETE:
        case BA_FUNCTIONS_MESSAGE_ONUNREGISTERBEGIN:
        case BA_FUNCTIONS_MESSAGE_ONUNREGISTERCOMPLETE:
        case BA_FUNCTIONS_MESSAGE_ONAPPLYCOMPLETE:
        case BA_FUNCTIONS_MESSAGE_ONLAUNCHAPPROVEDEXEBEGIN:
        case BA_FUNCTIONS_MESSAGE_ONLAUNCHAPPROVEDEXECOMPLETE:
        case BA_FUNCTIONS_MESSAGE_ONPLANMSIPACKAGE:
        case BA_FUNCTIONS_MESSAGE_ONBEGINMSITRANSACTIONBEGIN:
        case BA_FUNCTIONS_MESSAGE_ONBEGINMSITRANSACTIONCOMPLETE:
        case BA_FUNCTIONS_MESSAGE_ONCOMMITMSITRANSACTIONBEGIN:
        case BA_FUNCTIONS_MESSAGE_ONCOMMITMSITRANSACTIONCOMPLETE:
        case BA_FUNCTIONS_MESSAGE_ONROLLBACKMSITRANSACTIONBEGIN:
        case BA_FUNCTIONS_MESSAGE_ONROLLBACKMSITRANSACTIONCOMPLETE:
        case BA_FUNCTIONS_MESSAGE_ONPAUSEAUTOMATICUPDATESBEGIN:
        case BA_FUNCTIONS_MESSAGE_ONPAUSEAUTOMATICUPDATESCOMPLETE:
        case BA_FUNCTIONS_MESSAGE_ONSYSTEMRESTOREPOINTBEGIN:
        case BA_FUNCTIONS_MESSAGE_ONSYSTEMRESTOREPOINTCOMPLETE:
        case BA_FUNCTIONS_MESSAGE_ONPLANNEDPACKAGE:
        case BA_FUNCTIONS_MESSAGE_ONPLANFORWARDCOMPATIBLEBUNDLE:
        case BA_FUNCTIONS_MESSAGE_ONCACHEVERIFYPROGRESS:
        case BA_FUNCTIONS_MESSAGE_ONCACHECONTAINERORPAYLOADVERIFYBEGIN:
        case BA_FUNCTIONS_MESSAGE_ONCACHECONTAINERORPAYLOADVERIFYCOMPLETE:
        case BA_FUNCTIONS_MESSAGE_ONCACHECONTAINERORPAYLOADVERIFYPROGRESS:
        case BA_FUNCTIONS_MESSAGE_ONCACHEPAYLOADEXTRACTBEGIN:
        case BA_FUNCTIONS_MESSAGE_ONCACHEPAYLOADEXTRACTCOMPLETE:
        case BA_FUNCTIONS_MESSAGE_ONCACHEPAYLOADEXTRACTPROGRESS:
        case BA_FUNCTIONS_MESSAGE_ONPLANROLLBACKBOUNDARY:
        case BA_FUNCTIONS_MESSAGE_ONSETUPDATEBEGIN:
        case BA_FUNCTIONS_MESSAGE_ONSETUPDATECOMPLETE:
        case BA_FUNCTIONS_MESSAGE_ONDETECTCOMPATIBLEMSIPACKAGE:
        case BA_FUNCTIONS_MESSAGE_ONPLANCOMPATIBLEMSIPACKAGEBEGIN:
        case BA_FUNCTIONS_MESSAGE_ONPLANCOMPATIBLEMSIPACKAGECOMPLETE:
        case BA_FUNCTIONS_MESSAGE_ONPLANNEDCOMPATIBLEPACKAGE:
        case BA_FUNCTIONS_MESSAGE_ONPLANRESTORERELATEDBUNDLE:
        case BA_FUNCTIONS_MESSAGE_ONPLANRELATEDBUNDLETYPE:
        case BA_FUNCTIONS_MESSAGE_ONAPPLYDOWNGRADE:
        case BA_FUNCTIONS_MESSAGE_ONEXECUTEPROCESSCANCEL:
        case BA_FUNCTIONS_MESSAGE_ONDETECTRELATEDBUNDLEPACKAGE:
        case BA_FUNCTIONS_MESSAGE_ONCACHEPACKAGENONVITALVALIDATIONFAILURE:
            hr = BalBaseBootstrapperApplicationProc((BOOTSTRAPPER_APPLICATION_MESSAGE)message, pvArgs, pvResults, pvContext);
            break;
        case BA_FUNCTIONS_MESSAGE_ONTHEMELOADED:
            hr = BalBaseBAFunctionsProcOnThemeLoaded(pBAFunctions, reinterpret_cast<BA_FUNCTIONS_ONTHEMELOADED_ARGS*>(pvArgs), reinterpret_cast<BA_FUNCTIONS_ONTHEMELOADED_RESULTS*>(pvResults));
            break;
        case BA_FUNCTIONS_MESSAGE_WNDPROC:
            hr = BalBaseBAFunctionsProcWndProc(pBAFunctions, reinterpret_cast<BA_FUNCTIONS_WNDPROC_ARGS*>(pvArgs), reinterpret_cast<BA_FUNCTIONS_WNDPROC_RESULTS*>(pvResults));
            break;
        case BA_FUNCTIONS_MESSAGE_ONTHEMECONTROLLOADING:
            hr = BalBaseBAFunctionsProcOnThemeControlLoading(pBAFunctions, reinterpret_cast<BA_FUNCTIONS_ONTHEMECONTROLLOADING_ARGS*>(pvArgs), reinterpret_cast<BA_FUNCTIONS_ONTHEMECONTROLLOADING_RESULTS*>(pvResults));
            break;
        case BA_FUNCTIONS_MESSAGE_ONTHEMECONTROLWMCOMMAND:
            hr = BalBaseBAFunctionsProcOnThemeControlWmCommand(pBAFunctions, reinterpret_cast<BA_FUNCTIONS_ONTHEMECONTROLWMCOMMAND_ARGS*>(pvArgs), reinterpret_cast<BA_FUNCTIONS_ONTHEMECONTROLWMCOMMAND_RESULTS*>(pvResults));
            break;
        case BA_FUNCTIONS_MESSAGE_ONTHEMECONTROLWMNOTIFY:
            hr = BalBaseBAFunctionsProcOnThemeControlWmNotify(pBAFunctions, reinterpret_cast<BA_FUNCTIONS_ONTHEMECONTROLWMNOTIFY_ARGS*>(pvArgs), reinterpret_cast<BA_FUNCTIONS_ONTHEMECONTROLWMNOTIFY_RESULTS*>(pvResults));
            break;
        case BA_FUNCTIONS_MESSAGE_ONTHEMECONTROLLOADED:
            hr = BalBaseBAFunctionsProcOnThemeControlLoaded(pBAFunctions, reinterpret_cast<BA_FUNCTIONS_ONTHEMECONTROLLOADED_ARGS*>(pvArgs), reinterpret_cast<BA_FUNCTIONS_ONTHEMECONTROLLOADED_RESULTS*>(pvResults));
            break;
        }
    }

    return hr;
}
