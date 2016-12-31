#include <Windows.h>
#include <iostream>

const int HOTKEY_ID = 123;
const int HOTKEY_MOD = MOD_ALT | MOD_CONTROL | MOD_SHIFT | MOD_NOREPEAT;
const int HOTKEY_KEY = VK_F4;

void DisableWindow(HWND);
void EnableWindow(HWND);
void KillProcess(DWORD);

bool showing = true;
HWND console = NULL;
HWND selected_window = NULL;

int main() {
	console = GetConsoleWindow();
	BOOL registered = RegisterHotKey(NULL, HOTKEY_ID, HOTKEY_MOD, HOTKEY_KEY);
	if (!registered)
		std::cout << "Could not register hotkey, error: '" << GetLastError() << "'" << std::endl;

	//add the icon to system tray
	/*NOTIFYICONDATA icon = {};
	icon.cbSize = sizeof(icon);
	icon.hWnd = console;
	icon.uFlags = NIF_ICON | */

	MSG msg;
	while (GetMessage(&msg, 0, 0, 0))
	{
		PeekMessage(&msg, 0, 0, 0, 0x0001);
		switch (msg.message)
		{
		case WM_HOTKEY:

			if (msg.wParam == HOTKEY_ID)
			{
				HWND top = GetForegroundWindow();
				if (selected_window != top) {//select window
					selected_window = top;
					//blink the window
					if (!IsHungAppWindow(selected_window)) {
						//play the windows notification sound
						PlaySound((LPCSTR)SND_ALIAS_SYSTEMQUESTION, NULL, SND_ASYNC | SND_ALIAS_ID);
					}
				}else{//confirm window
					DWORD thread_id = GetWindowThreadProcessId(selected_window, NULL);
					HANDLE thread_handle = OpenThread(THREAD_QUERY_LIMITED_INFORMATION, false, thread_id);
					DWORD proc_id = GetProcessIdOfThread(thread_handle);
					KillProcess(proc_id);
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

	UnregisterHotKey(NULL, HOTKEY_ID);

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