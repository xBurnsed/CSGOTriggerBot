#include "ExternalLib.h"
#include <iostream>


HANDLE ExternalLib::getHandle() {
	return this->pHandle;
}

DWORD ExternalLib::getProcessPID() {
	return this->processPID;
}

std::string ExternalLib::getProcessName() {
	return this->processName;
}

void ExternalLib::setHandle() {
	// Don't get any Handle until Insert key is pressed ingame
		do {
			while (!this->isKeyDown(VK_INSERT)) {}

			HWND tempWindow = GetForegroundWindow();

			//Had to alloc memory in some way.
			LPDWORD tempPDWORD = new DWORD();
			GetWindowThreadProcessId(tempWindow, tempPDWORD);
			this->processPID = *tempPDWORD;
			delete tempPDWORD;

			this->pHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_VM_WRITE, 0, this->processPID);

			CHAR Buffer[40];
			GetModuleBaseNameA(this->pHandle, NULL, Buffer, 40);
			this->processName = (std::string)Buffer;
		} while (this->processName != "csgo.exe");
		std::cout << "Game detected succesfully!!!" << std::endl;
}

DWORD ExternalLib::GetModuleAddr(DWORD pID, const char* modName) {
	HANDLE modSnapHandle = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE32 | TH32CS_SNAPMODULE, this->processPID);
	if (modSnapHandle == INVALID_HANDLE_VALUE) {
		std::cout << "Error reading the module list of that ProcessPID. GetLastError = " << GetLastError() << std::endl;
		//CloseHandle(modSnapHandle);
		return NULL;
	}
	MODULEENTRY32 mEntry32;
	mEntry32.dwSize = sizeof(MODULEENTRY32);

	if (Module32First(modSnapHandle, &mEntry32) == FALSE) {
		std::cout << "Error getting the first module from the Snapshot. GetLastError = " << GetLastError() << std::endl;
		//CloseHandle(modSnapHandle);
		return NULL;
	}

	//Loop through all modules and compare its names.
	while (_stricmp(mEntry32.szModule, modName) != 0) {
		Module32Next(modSnapHandle, &mEntry32);
	}

	CloseHandle(modSnapHandle);

	return !_stricmp(mEntry32.szModule, modName) ? (DWORD)mEntry32.hModule : NULL;
}

DWORD ExternalLib::FindAddrByVec(HANDLE hProcess, DWORD bAddr, std::vector<int> offsets) {
	DWORD rAddr = bAddr;
	for (unsigned int i = 0; i < offsets.size(); i++) {
		ReadProcessMemory(hProcess, (void*)rAddr, &rAddr, sizeof(rAddr), 0);
		rAddr += offsets[i];
	}
	return rAddr;
}