// dllmain.cpp : Defines the entry point for the DLL application.

#include "arduino_passthru.h"
#include "pch.h"
#include "XentryComm.h"
#include "Logger.h"

bool startup() {
    LOGGER.logInfo("dllmain::startup", "Setting up!");
    return XentryComm::CreateCommThread();
}

void close() {
    LOGGER.logInfo("dllmain::close", "Exiting dll");
    XentryComm::CloseCommThread();
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        if (!startup()) {
            return FALSE;
        }
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        LOGGER.logInfo("dllmain", "Process detached!");
        close();
        break;
    }
    return TRUE;
}