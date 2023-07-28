#include "Dialogs.h"
#include "../SteamTypes.h"
#include "../Server/SteamServerSettings.h"
#include "Globals.h"
#include "resource.h"
#include "Log.h"
#include "DirectPlay/Utils.h"
#include "Utils/StringUtils.h"

static BOOL CALLBACK EnumWindowsCallback(HWND hWnd, LPARAM lParam)
{
	auto& args = *reinterpret_cast<std::pair<DWORD const, HWND>*>(lParam);

	DWORD windowPID;
	GetWindowThreadProcessId(hWnd, &windowPID);
	if (windowPID == args.first)
	{
		args.second = hWnd;
		return FALSE;
	}
	return TRUE;
}

HWND GetMainWindow()
{
	std::pair<DWORD const, HWND> args(GetCurrentProcessId(), NULL);
	if (args.first != NULL)
	{
		EnumWindows(&EnumWindowsCallback, (LPARAM)&args);
	}
	return args.second;
}


static constexpr std::pair<ELobbyType, char const*> s_lobbyTypes[] =
{
	{ k_ELobbyTypePublic,      "Public" },
	{ k_ELobbyTypeFriendsOnly, "Friends Only" },
	{ k_ELobbyTypePrivate,     "Private" }
};

INT_PTR CALLBACK OnServerSettingsDialog(HWND dlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
	static SSteamServerSettings* s_pSettings = nullptr;
	switch (msg)
	{
	case WM_INITDIALOG:
		s_pSettings = reinterpret_cast<SSteamServerSettings*>(lparam);
		if (s_pSettings)
		{
			SetDlgItemTextA(dlg, IDC_EDIT_SERVERNAME, s_pSettings->name);
			SetDlgItemTextA(dlg, IDC_EDIT_SERVERPW, s_pSettings->password);
	
			size_t lobbyTypeIndex = 0;
			for (size_t i = 0; i < ArrayCount(s_lobbyTypes); ++i)
			{
				auto const& [type, name] = s_lobbyTypes[i];
				SendDlgItemMessageA(dlg, IDC_COMBO_SERVERPERM, CB_ADDSTRING, NULL, reinterpret_cast<LPARAM>(name));
				if (type == s_pSettings->lobbyType)
				{
					lobbyTypeIndex = i;
				}
			}
			SendDlgItemMessage(dlg, IDC_COMBO_SERVERPERM, CB_SETCURSEL, (WPARAM)lobbyTypeIndex, NULL);

			char numBuf[24];
			_itoa(s_pSettings->maxPlayers, numBuf, 10);
			SetDlgItemTextA(dlg, IDC_EDIT_SERVERPLAYERS, numBuf);
		}
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		if (LOWORD(wparam) == IDOK || LOWORD(wparam) == IDCANCEL)
		{
			if (s_pSettings)
			{
				// GetDlgItemText always writes a terminating null-character, so we can use the actual arraySize
				GetDlgItemTextA(dlg, IDC_EDIT_SERVERNAME, s_pSettings->name.data(),     s_pSettings->name.array_size());
				GetDlgItemTextA(dlg, IDC_EDIT_SERVERPW,   s_pSettings->password.data(), s_pSettings->password.array_size());

				LRESULT const cbResult = SendDlgItemMessage(dlg, IDC_COMBO_SERVERPERM, CB_GETCURSEL, NULL, NULL);
				s_pSettings->lobbyType = (cbResult >= 0 && cbResult < ARRAYSIZE(s_lobbyTypes)) ? s_lobbyTypes[cbResult].first : ELobbyType::k_ELobbyTypePublic;

				char numBuf[24];
				GetDlgItemTextA(dlg, IDC_EDIT_SERVERPLAYERS, numBuf, ArrayCount(numBuf));
				s_pSettings->maxPlayers = strtoul(numBuf, nullptr, 10);
			}
			EndDialog(dlg, LOWORD(wparam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

bool ShowServerSettings(SSteamServerSettings& settings)
{
	return DialogBoxParam(g_module, MAKEINTRESOURCE(IDD_DIALOG_SERVERSETTINGS), GetMainWindow(), OnServerSettingsDialog, reinterpret_cast<LPARAM>(&settings)) == IDOK;
}

struct PasswordDialogData
{
	char*        ptr;
	size_t const size;
};

INT_PTR CALLBACK OnPasswordDialog(HWND dlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
	static PasswordDialogData const* s_pPassword = nullptr;
	switch (msg)
	{
	case WM_INITDIALOG:
		s_pPassword = reinterpret_cast<PasswordDialogData const*>(lparam);
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		if (LOWORD(wparam) == IDOK || LOWORD(wparam) == IDCANCEL)
		{
			if (s_pPassword)
			{
				GetDlgItemTextA(dlg, IDC_EDIT_CLIENTPW, s_pPassword->ptr, s_pPassword->size);
			}
			EndDialog(dlg, LOWORD(wparam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

bool ShowPasswordRequest(char* szPassword, size_t size)
{
	if (!szPassword || size == 0)
	{
		return false;
	}

	PasswordDialogData const data{ szPassword, size };
	return DialogBoxParam(g_module, MAKEINTRESOURCE(IDD_DIALOG_PASSWORD), GetMainWindow(), OnPasswordDialog, reinterpret_cast<LPARAM>(&data)) == IDOK;
}

