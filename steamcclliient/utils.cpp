#include <iostream>
#include <windows.h>
#include <vector>
#include "clientcontext.h"
#include "utils.h"


void ShowDownloadProgress(uint64_t bytesDownloaded, uint64_t bytesToDownload)
{
	float donloadProgress = (float)bytesDownloaded / (float)bytesToDownload;
	int progressWidth = 50;
	int progress = progressWidth * donloadProgress;

	std::cout << "[";
	for (int i = 0; i < progressWidth; ++i)
	{
		if (i < progress)
		{
			std::cout << "=";
		}
		else if (i == progress)
		{
			std::cout << ">";
		}
		else
		{
			std::cout << " ";
		}
	}
	std::cout << "] " << int(donloadProgress * 100.0) << "%\r";
}

bool IsSteamRunning()
{
	int steamPID = 0;
	DWORD pidSize = sizeof(steamPID);
	LSTATUS err_no = RegGetValueA(HKEY_CURRENT_USER, "SOFTWARE\\Valve\\Steam\\ActiveProcess\\", "pid", RRF_RT_DWORD, NULL, &steamPID, &pidSize);
	if (!err_no && steamPID != 0)
	{
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, steamPID);
		if (hProcess == NULL)
		{
			return false;
		}

		DWORD exitCode = 0;
		if (GetExitCodeProcess(hProcess, &exitCode))
		{
			if (exitCode == STILL_ACTIVE)
			{
				return true;
			}
		}
	}

	return false;
}


void GetAppMissingDeps(AppId_t appID, std::vector<AppId_t>* deps)
{
	uint32 depCount = GClientContext()->ClientAppManager()->GetAppDependencies(appID, NULL, 0);
	if (depCount)
	{
		AppId_t* appDeps = new AppId_t[depCount];
		GClientContext()->ClientAppManager()->GetAppDependencies(appID, appDeps, depCount);
		for (uint32 i = 0; i < depCount; ++i)
		{
			if (GClientContext()->ClientAppManager()->GetAppInstallState(appDeps[i]) != k_EAppStateFullyInstalled)
			{
				GetAppMissingDeps(appDeps[i], deps);
				if (std::find(deps->cbegin(), deps->cend(), appDeps[i]) == deps->cend())
				{
					deps->push_back(appDeps[i]);
				}
			}
		}
	}
}

bool RunInstallScript(AppId_t appID, bool bUninstall)
{
	std::cout << "Running app install script..." << std::endl;

	bool res = false;
	char* appLang = new char[128];
	GClientContext()->ClientAppManager()->GetAppConfigValue(appID, "language", appLang, 128);
	res = GClientContext()->ClientUser()->RunInstallScript(appID, appLang, bUninstall);
	delete[] appLang;

	while (GClientContext()->ClientUser()->IsInstallScriptRunning())
	{
		Sleep(300);
	}

	return res;
}


std::string GetSteamAutoLoginUser()
{
	char nameBuf[256];
	DWORD nameSize = sizeof(nameBuf);
	LSTATUS err_no = RegGetValueA(HKEY_CURRENT_USER, "SOFTWARE\\Valve\\Steam\\", "AutoLoginUser", RRF_RT_REG_SZ, NULL, nameBuf, &nameSize);
	if (!err_no)
	{
		return std::string(nameBuf);
	}
	return std::string();
}

bool SetSteamProtocolHandler()
{
	std::string selfPath = GetSelfPath();
	if (selfPath.empty())
	{
		return false;
	}

	HKEY hKey;
	LSTATUS err_no = RegOpenKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\Classes\\steam\\Shell\\Open\\Command", 0, KEY_SET_VALUE, &hKey);
	if (!err_no)
	{
		char openComm[512] = { '\0' };
		sprintf(openComm, "\"\%s\" \"%%1\"", selfPath.c_str());
		err_no = RegSetValueExA(hKey, NULL, NULL, RRF_RT_REG_SZ, LPBYTE(openComm), DWORD(sizeof(openComm)));
		if (!err_no)
		{
			return true;
		}
	}

	return false;
}



bool SetSteamAutoLoginUser(std::string user)
{
	HKEY hKey;
	LSTATUS err_no = RegOpenKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\Valve\\Steam\\", 0, KEY_SET_VALUE, &hKey);
	if (!err_no)
	{
		err_no = RegSetValueExA(hKey, "AutoLoginUser", NULL, RRF_RT_REG_SZ, LPBYTE(user.c_str()), DWORD(user.size()+1));
		if (!err_no)
		{
			return true;
		}
	}

	return false;
}

std::string GetSelfPath()
{
	char buf[260] = { '\0' };
	GetModuleFileName(NULL, buf, sizeof(buf));
	return std::string(buf);
}