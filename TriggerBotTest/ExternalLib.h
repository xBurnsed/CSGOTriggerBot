#pragma once

#include <Windows.h>
#include <TlHelp32.h>
#include <string>
#include <Psapi.h>
#include <vector>
#include <iostream>

class ExternalLib {
	private:
		HANDLE pHandle = NULL;
		DWORD processPID = 0;
		std::string processName;

		BOOL isKeyDown(int key){
			Sleep(100);
			short res = GetAsyncKeyState(key);
			if (((0x80000000 & res) != 0) || ((0x00000001 & res) != 0)) return TRUE;
			return FALSE;
		}

	public:	
		HANDLE getHandle();
		DWORD getProcessPID();
		std::string getProcessName();
		void setHandle();
		DWORD GetModuleAddr(DWORD pID, const char* modName);
		DWORD FindAddrByVec(HANDLE hProcess, DWORD bAddr, std::vector<int> offsets);

		template<typename T>
		T readFromMem(DWORD readAddress) {
			T retValue;
			if (ReadProcessMemory(this->pHandle, (void*)readAddress, &retValue, sizeof(T), NULL) == 0) {
				std::cout << "Error reading the address of the handle. GetLastError() = " << GetLastError() << std::endl;
				throw;
			}
			return retValue;
		}
};