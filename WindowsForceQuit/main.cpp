#include <Windows.h>
#include <iostream>
#include <conio.h>
#include <cassert>

#include <SimpleIni.h>
#include <atomic>
#include <thread>

const int HOTKEY_ID = 123;
//const int HOTKEY_MOD = MOD_ALT | MOD_CONTROL | MOD_SHIFT | MOD_NOREPEAT;
//const int HOTKEY_KEY = VK_F4;
const bool config_utf8 = true;
const bool config_multikeys = false;
const bool config_multiline = true;
const char* config_file = "configuration.ini";

void DisableWindow(HWND);
void EnableWindow(HWND);
void KillProcess(DWORD);

std::string GetDefaultConfig();
bool CheckConfigValidity(SI_Error validity_error, std::string error_msg)
{
	if (validity_error < 0) {
		std::cout << "Error: " << error_msg << '\n';
		assert(false);
		return false;
	}
	return true;
}

bool showing = true;
HWND console = NULL;
HWND selected_window = NULL;

int main(int argc, char *argv[]) 
{
	(void)argc, argv;
	//Configuration config("config.json");

	std::cout << "Welcome to WindowsForceQuit! May the force be quit you!\n";
	std::cout << "Press 'Escape' to exit the application.\n";

	CSimpleIniA config{ config_utf8, config_multikeys, config_multiline };
	auto err = config.LoadData(GetDefaultConfig());
	CheckConfigValidity(err, "loading base data in config object");
	config.LoadFile(config_file);
	bool with_ctrl = config.GetBoolValue("Hotkey", "With ctrl", false);
	bool with_shift = config.GetBoolValue("Hotkey", "With shift", false);
	bool with_alt = config.GetBoolValue("Hotkey", "With alt", false);
	bool with_win_key = config.GetBoolValue("Hotkey", "With windows key", false);
	long vk_hotkey = config.GetLongValue("Hotkey", "Key", VK_F4);

	console = GetConsoleWindow();
	
	int hotkey_mod = (MOD_NOREPEAT) |
		(with_ctrl ? MOD_CONTROL : 0) | 
		(with_shift ? MOD_SHIFT : 0) |
		(with_alt ? MOD_ALT : 0) |
		(with_win_key ? MOD_WIN : 0);
	BOOL registered = RegisterHotKey(NULL, HOTKEY_ID, hotkey_mod, vk_hotkey);
	if (!registered) {
		std::cout << "Could not register hotkey, error: '" << GetLastError() << "'" << std::endl;
		assert(false);
	}

	std::string hotkey_string;
	char c = MapVirtualKeyA(vk_hotkey, MAPVK_VK_TO_CHAR);
	if (c != 0) {
		hotkey_string = { c };
	} else {
		UINT scancode = MapVirtualKeyA(vk_hotkey, MAPVK_VK_TO_VSC);
		char string_buffer[20];
		int ret = GetKeyNameTextA(scancode, string_buffer, 20);
		if(ret == 0) {
			hotkey_string = "??";
		}else {
			hotkey_string = string_buffer;
		}
	}
	std::cout << "Your hotkey for quitting the application which is in focus at that time is: " << hotkey_string
		<< "(0x" << std::hex << vk_hotkey << std::dec << ')';
	if (with_ctrl)
		std::cout << " + ctrl";
	if (with_shift)
		std::cout << " + shift";
	if (with_alt)
		std::cout << " + alt";
	if (with_win_key)
		std::cout << " + windows key";
	std::cout << " ";

	//add the icon to system tray
	/*NOTIFYICONDATA icon = {};
	icon.cbSize = sizeof(icon);
	icon.hWnd = console;
	icon.uFlags = NIF_ICON | */

	std::atomic<bool> run = true;
	std::thread cin_thread{ [&run]() {
		while(run) {
			/*std::string line;
			std::getline(std::cin, line);
			if (line == "exit")
				run = false;*/
			int key = _getch();
			if (key == VK_ESCAPE)
				run = false;
		}
	} };
	MSG msg;
	while (run /*&& GetMessage(&msg, 0, 0, 0)*/)
	{
		if (PeekMessage(&msg, 0, 0, 0, 0x0001) != 0) {
			switch (msg.message) {
			case WM_HOTKEY:

				if (msg.wParam == HOTKEY_ID) {
					HWND top = GetForegroundWindow();
					if (selected_window != top) {//select window
						//stop blinking of the old window
						FLASHWINFO old_flash_info{};
						old_flash_info.cbSize = sizeof(FLASHWINFO);
						old_flash_info.hwnd = selected_window;
						old_flash_info.dwFlags = FLASHW_STOP;
						old_flash_info.uCount = 0;
						old_flash_info.dwTimeout = 0;
						FlashWindowEx(&old_flash_info);

						selected_window = top;
						//blink the window
						FLASHWINFO flash_info{};
						flash_info.cbSize = sizeof(FLASHWINFO);
						flash_info.hwnd = selected_window;
						flash_info.dwFlags = FLASHW_ALL | FLASHW_TIMER;
						flash_info.uCount = 0;
						flash_info.dwTimeout = 0;
						FlashWindowEx(&flash_info);

						if (!IsHungAppWindow(selected_window)) {
							//play the windows notification sound
							PlaySound((LPCSTR)SND_ALIAS_SYSTEMQUESTION, NULL, SND_ASYNC | SND_ALIAS_ID);
						}
					} else {//confirm window
						DWORD thread_id = GetWindowThreadProcessId(selected_window, NULL);
						HANDLE thread_handle = OpenThread(THREAD_QUERY_LIMITED_INFORMATION, false, thread_id);
						DWORD proc_id = GetProcessIdOfThread(thread_handle);
						KillProcess(proc_id);
						PlaySound((LPCSTR)SND_ALIAS_SYSTEMEXIT, NULL, SND_ASYNC | SND_ALIAS_ID);
					}
				}

				/*if (showing) {
					DisableWindow(console);
				}
				else {
					EnableWindow(console);
				}
				showing = !showing;*/

			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds{ 150 });
	}
	run = false;
	if (cin_thread.joinable())
		cin_thread.join();

	err = config.SaveFile(config_file);
	CheckConfigValidity(err, "saving configuration file");

	UnregisterHotKey(NULL, HOTKEY_ID);

	FLASHWINFO old_flash_info{};
	old_flash_info.cbSize = sizeof(FLASHWINFO);
	old_flash_info.hwnd = selected_window;
	old_flash_info.dwFlags = FLASHW_STOP;
	old_flash_info.uCount = 0;
	old_flash_info.dwTimeout = 0;
	FlashWindowEx(&old_flash_info);

	return 0;
}

void KillProcess(DWORD id) {
	HANDLE proc = OpenProcess(PROCESS_TERMINATE, false, id);
	if (!proc) {
		std::cout << "Error gaining access to process with id=" << id << " ,Error code while opening process=" << GetLastError() << std::endl;
	}
	BOOL terminated = TerminateProcess(proc, 1); //is there a standard 'terminated' exit code?
	if (!terminated) {
		std::cout << "Error terminating process with id=" << id << " ,Error code while terminating=" << GetLastError() << std::endl;
	}
}

void DisableWindow(HWND wd) {
	ShowWindow(wd, SW_HIDE);
}

void EnableWindow(HWND wd) {
	ShowWindow(wd, SW_RESTORE);
}

std::string GetDefaultConfig()
{
	CSimpleIniA ini{ config_utf8, config_multikeys, config_multiline };//TODO: Add the keycode list
	ini.SetLongValue("Hotkey", "Key", VK_F4, 
		R"(;The key used for the hotkey of force quiting the top application. 
;NOTE: this should be the virtual keycode with the a value from this list: https://docs.microsoft.com/nl-nl/windows/desktop/inputdev/virtual-key-codes
;
;Default: 0x73 (F4))",
true);
	ini.SetBoolValue("Hotkey", "With ctrl", true, ";Should the control key be pressed for the hotkey to activate?");
	ini.SetBoolValue("Hotkey", "With shift", true, ";Should the shift key be pressed for the hotkey to activate?");
	ini.SetBoolValue("Hotkey", "With alt", true, ";Should the alt key be pressed for the hotkey to activate?");
	ini.SetBoolValue("Hotkey", "With windows key", false, ";Should the windows key be pressed for the hotkey to activate?");

	std::string serialized_ini;
	auto err = ini.Save(serialized_ini);
	CheckConfigValidity(err, "could not get string from default config");

	return serialized_ini;
}
