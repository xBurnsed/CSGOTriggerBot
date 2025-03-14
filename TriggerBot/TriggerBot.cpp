#include <iostream>
#include "MemoryAddies.h"
#include "ExternalLib.h"
#include <cmath>

using namespace std;

struct vec2 {
	float x, y;
};

struct vec {
	float x, y, z;
	vec operator+(vec vecParam) {
		return { x + vecParam.x, y + vecParam.y, z + vecParam.z };
	}
};

static DWORD moduleB;
static DWORD engineM;
static int currentTeam;
static boolean onOff = false;
static INPUT input;

static int playerVel;
static DWORD locPlayer;
static int delay;
static int weapID;
static bool inGame = false;
static DWORD clState;

//aim
static vec2 angles;

bool checkScoped(ExternalLib exL) {
	return exL.readFromMem<bool>(locPlayer + m_bIsScoped);
}

void getWeapon(ExternalLib exL) {
	int weapon = exL.readFromMem<int>(locPlayer + m_hActiveWeapon);
	int weaponEnt = exL.readFromMem<int>(moduleB + dwEntityList + ((weapon & 0xFFF) - 1) * 0x10);
	if (weaponEnt != NULL) 
		weapID = exL.readFromMem<int>(weaponEnt + m_iItemDefinitionIndex);
}

void useInput(ExternalLib exL) {
	input.type = INPUT_MOUSE;
	input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
	WriteProcessMemory(exL.getHandle(), (LPVOID)(clState + dwClientState_ViewAngles), &angles, sizeof(angles), 0);
	SendInput(1, &input, sizeof(INPUT));
	ZeroMemory(&input, sizeof(INPUT));
	input.type = INPUT_MOUSE;
	input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
	SendInput(1, &input, sizeof(INPUT));
	Sleep(delay);
}

void calcDistanceDelay(ExternalLib exL, DWORD ent) {
	vec myLoc = exL.readFromMem<vec>(locPlayer + m_vecOrigin);
	vec enemyLoc = exL.readFromMem<vec>(ent + m_vecOrigin);
	delay = sqrt(pow(myLoc.x - enemyLoc.x, 2) + pow(myLoc.y - enemyLoc.y, 2) + pow(myLoc.z - enemyLoc.z, 2)) * 0.0254 * 4.3;
}

vec2 getTargetAngle(ExternalLib exL, vec target) {
	vec origin = exL.readFromMem<vec>(locPlayer + m_vecOrigin);
	vec viewOff = exL.readFromMem<vec>(locPlayer + m_vecViewOffset);
	
	vec myPos = { origin.x,origin.y,origin.z + 64 };

	vec deltaVec = { myPos.x - target.x, myPos.y - target.y, myPos.z  - target.z};
	double hyp = sqrt(deltaVec.x * deltaVec.x + deltaVec.y * deltaVec.y);

	float x = atan(deltaVec.z / hyp) * (180.0f / 3.14159265358);
	float y = atan(deltaVec.y / deltaVec.x) * (180.0f / 3.14159265358);

	if (deltaVec.x >= 0.0) y += 180.0f;

	while (x > 89.0f)
		x -= 180.f;

	while (x < -89.0f)
		x += 180.f;

	while (y > 180.f)
		y -= 360.f;

	while (y < -180.f)
		y += 360.f;

	return { x , y };
}

bool shouldFireAtTarget(ExternalLib exL) {
	bool canShoot = false;
	int cHair = exL.readFromMem<int>(locPlayer + m_iCrosshairId);
	vec enemyBone = { 0,0,0 };

	if (cHair < 64 && cHair != 0) {
		DWORD ent = exL.readFromMem<DWORD>(moduleB + dwEntityList + ((cHair -1) *0x10));

		DWORD dwBoneMatrix = exL.readFromMem<DWORD>(ent + m_dwBoneMatrix);
		enemyBone = { exL.readFromMem <float>(dwBoneMatrix + 0x30 * 8 + 0x0C), exL.readFromMem <float>(dwBoneMatrix + 0x30 * 8 + 0x1C), exL.readFromMem <float>(dwBoneMatrix + 0x30 * 8 + 0x2C) };

		int enemyTeam = exL.readFromMem<int>(ent + m_iTeamNum);
		int enemyHealth = exL.readFromMem<int>(ent + m_iHealth);
		if (enemyHealth > 0 && enemyTeam != currentTeam) {
			calcDistanceDelay(exL, ent);
			getWeapon(exL);
			canShoot = (weapID != 40 && weapID != 9) || checkScoped(exL);
		}
	} if (canShoot) {
		angles = getTargetAngle(exL, enemyBone);
	}
	return canShoot;
}

bool isInGame(ExternalLib exLib) {
	return exLib.readFromMem<DWORD>(clState + dwClientState_State) == 6;
}

int main() {
	HWND ownWHandle = GetConsoleWindow();
	CHAR ConsoleTitle[30] = "#general - Discord";
	SetWindowTextA(ownWHandle, (LPCSTR)ConsoleTitle);

	ExternalLib exLib;
	exLib.setHandle();

	cout << "Process PID: " << exLib.getProcessPID() << endl;
	cout << "Process Name: " << exLib.getProcessName() << endl;

	moduleB = exLib.GetModuleAddr(exLib.getProcessPID(), "client_panorama.dll");
	engineM = exLib.GetModuleAddr(exLib.getProcessPID(), "engine.dll");

	if (moduleB == NULL && engineM == NULL) {
		cout << "Couldn't find client_panorama.dll or engine.dll baseAddr" << endl;
		return EXIT_FAILURE;
	}	
	clState = exLib.readFromMem<DWORD>(engineM + dwClientState);

	while (1) {
		if (!inGame && isInGame(exLib)) {
			inGame = true;
			//GET LOCALPLAYER
			locPlayer = exLib.readFromMem<DWORD>(moduleB + dwLocalPlayer);
			if (locPlayer == NULL) {
				cout << "Couldn't read localPlayer.. update offsets" << endl;
				return EXIT_FAILURE;
			}
			cout << "locPlayer offset found!!" << endl;
		}
		else if(!isInGame(exLib) && inGame) {
			inGame = false;
		}

		if (inGame == true) {
			if (GetAsyncKeyState(VK_XBUTTON2) & 1) {
				onOff = !onOff;
				if (onOff) 
					cout << endl << "Currently enabled." << endl;
				else 
					cout << endl << "Currently disabled." << endl;
			}
			if (onOff) {
				//GET CURRENT TEAM
				currentTeam = exLib.readFromMem<int>(locPlayer + m_iTeamNum);
				locPlayer = exLib.readFromMem<DWORD>(moduleB + dwLocalPlayer);
				if (shouldFireAtTarget(exLib)) {
					useInput(exLib);
				}
			}
		}
		Sleep(1);
	}
	cout << "Press any key to end the program... " << endl;
	system("PAUSE");
	return 0;
}
