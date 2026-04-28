#include "script.h"
#include "Settings.h"
#include "Util/UIutils.hpp"
#include "Util/INIutils.hpp"
#include "Memory/VehicleExtensions.hpp"
#include "FuelStation.hpp"
#include "simpleFuelEnums.h"

#include <vector>
#include <string>
#include <menu.h>

#include <ctime> 
#include "Util/Logger.hpp"
#include "Util/Versions.h"
#include "Memory/VehicleFlags.h"

NativeMenu::Menu menu;
VehicleExtensions ext;

DWORD nextAudioSignal = 0xFFFFFFFF;
DWORD nextDistCheck = 0;

Player player;				//Reference to current player
Ped playerPed;				//Reference to current ped
Vehicle playerVeh;			//Reference to current vehicle

float ratioComp = 1.0f;

struct Color{
	int R, G, B;
};
Color barNorm, barCrit;

////GAMEPLAY
int lastMainCharacter = Michael;

FuelStation station[100];

bool didRefuelInput = false;	//Only part of try to refuel
bool belowCrit = false;		    //Below critical fuel level, now 20%
bool showJerryHelp = false;		//Show help text for refueling? (Jerry Can) indicates possibility to refuel with Jerry Can
bool showInputHelp = false;		//Show help text for refueling? (Gas Station)
int nearestRefuel = -1;
float cashToPay = 0.0f;

float fuelBarLevel = -1.0f;
float tankCapacity = -1.0f;

//NOTIFICATIONS
std::string noteFull, noteRefuel, noteRefuelInput, noteCrit, noteEmpty, noteBlip, noteJerryCan;
int handleNoteRefuel = 0; //handle of current tank state note
int handleNoteEmpty = 0;
int handleNoteCreate = 0; //handle of current station modifying state

float getConsumption()
{
	//No consumption in cutscenes
	if (!ENTITY::DOES_ENTITY_EXIST(playerPed) || !PLAYER::IS_PLAYER_CONTROL_ON(player) || playerVeh == 0) return 0.0f;

	float factor = ext.GetCurrentRPM(playerVeh)*ext.GetThrottle(playerVeh);
	if (factor < 0.0f) factor = 0.0f;
	float consumption = 0.05f + 0.95f * sqrtf(factor);
	if (belowCrit) consumption *= 0.666f;
	return consumption;
}

void drawFuelRect(float posX, float posY, float width, float height, int r, int g, int b, int a)
{
	GRAPHICS::DRAW_RECT(posX + 0.5f*width*ratioComp, posY, width*ratioComp, height, r, g, b, a, 0);
}
void drawFuelRect(float posX, float posY, float width, float height, Color color, int a)
{
	drawFuelRect(posX, posY, width, height, color.R, color.G, color.B, a);
}
void drawFuelLevel(const float &fuel)
{
	drawFuelRect(Settings::barX,	Settings::barY, Settings::barW,		Settings::barH,	      0,   0,  0, 120);
	if (belowCrit) {
		drawFuelRect(Settings::barX, Settings::barY, Settings::barW*fuel,	Settings::barH*0.6f, barCrit, 255);
	}else {
		drawFuelRect(Settings::barX, Settings::barY, Settings::barW*fuel,	Settings::barH*0.6f, barNorm, 255);
	}
}

void toggleBlips()
{
	bool visible;
	switch (Settings::blipsVisibility)
	{
	case BlipsOff:
		visible = false;
		break;
	case BlipsOn:
		visible = true;
		break;
	case BlipsAuto:
		visible = tankCapacity > 0.0f;
		break;
	}

	//Set Blips
	for (int n = 0; n < Settings::numOfStations; n++)
	{
		if (n != nearestRefuel)
		{
			station[n].setBlipVisible(visible);
		}
	}
}

int getCash(int character) 
{
	char statNameFull[32];
	sprintf_s(statNameFull, "SP%d_TOTAL_CASH", lastMainCharacter);
	Hash hash = GAMEPLAY::GET_HASH_KEY(statNameFull);
	int val;
	STATS::STAT_GET_INT(hash, &val, -1);
	return val;
}

void addCash(int amount)
{
	char statNameFull[32];
	sprintf_s(statNameFull, "SP%d_TOTAL_CASH", lastMainCharacter);
	Hash hash = GAMEPLAY::GET_HASH_KEY(statNameFull);
	int val;
	STATS::STAT_GET_INT(hash, &val, -1);
	val += amount;
	STATS::STAT_SET_INT(hash, val, 1);
}
void refuel()
{
	float liter = 0.0f;

	VEHICLE::SET_VEHICLE_UNDRIVEABLE(playerVeh, false);

	//Last percent
	if (!Settings::isRefuelRealistic || fuelBarLevel > 0.99f)
	{
		showNotification(handleNoteEmpty, &noteFull[0u]);
		liter = (1.0f - fuelBarLevel) * tankCapacity;
	}
	//Realistic Refueling
	else if (Settings::isRefuelRealistic)
	{
		replaceNotification(handleNoteRefuel, &noteRefuel[0u]);
		liter = GAMEPLAY::GET_FRAME_TIME() * Settings::literPerSecond;
	}
	
	
	//Set norm fuel
	fuelBarLevel += liter / tankCapacity;
	if (fuelBarLevel > 1.0f) fuelBarLevel = 1.0f;

	//Money
	if (Settings::hasGasPrice)
	{
		cashToPay += liter * Settings::gasPrice;
		int bucks = (int)cashToPay;
		addCash(-bucks);
		cashToPay -= bucks;
	}

	//Write data
	ext.SetFuelLevel(playerVeh, fuelBarLevel*tankCapacity);
}
void tryToRefuel()
{
	//Check if vehicle cannot be refueled (realistic refueling and vehicle too fast)
	if (Settings::isRefuelRealistic && 0.1f < ENTITY::GET_ENTITY_SPEED(playerVeh))
	{
		showInputHelp = false;
		didRefuelInput = false;
		removeNotification(handleNoteRefuel);
		return;
	}

	//Check if refueling is actually wanted (depends on refuelInputMode)
	switch (Settings::refuelInputMode)
	{
	case RefuelInputHold:
		//Show Input help if player doesn't press refuel input even though tank is not full
		showInputHelp = fuelBarLevel < 0.99f && !CONTROLS::IS_CONTROL_PRESSED(2, Settings::refuelControlStation);
		//Refuel or remove refueling notification
		CONTROLS::IS_CONTROL_PRESSED(2, Settings::refuelControlStation) ? refuel() : removeNotification(handleNoteRefuel);
		break;

	case RefuelInputOnce:
		//Check if input is pressed
		if (CONTROLS::IS_CONTROL_PRESSED(2, Settings::refuelControlStation)) didRefuelInput = true;
		//Show Input help if player didn't press refuel input yet even though tank is not full
		showInputHelp = !didRefuelInput && fuelBarLevel < 0.99f;
		//Refuel if player wants to
		if (didRefuelInput) refuel();
		break;

	case RefuelInputAuto:
		showInputHelp = false;
		refuel();
		break;
	}
}

int getNearStationAir(int max)
{
	int x = -1;
	int distX = max*max;
	for (int n = 0; n < Settings::numOfStations; n++)
	{
		if (station[n].exists && station[n].isEnabled)
		{
			int distance = station[n].getAirDistanceSquared(playerPed);
			if (distance < distX)
			{
				distX = distance;
				x = n;
			}
		}
	}
	return x;
}
int getNearStationPath(int max)
{
	int x = -1;
	int distX = max;
	for (int n = 0; n < Settings::numOfStations; n++)
	{
		if (station[n].exists && station[n].isEnabled)
		{
			int distance = station[n].getTravelDistance(playerPed);
			if (distance < distX)
			{
				distX = distance;
				x = n;
			}
		}
	}
	return x;
}

void tryToCreateStation()
{
	//Triggered when users tries to spawn new fuel station
	int n = 0;
	while (station[n].exists)
	{
		n++;
	}
	if (n >= Settings::numOfStations)
	{
		replaceNotification(handleNoteCreate, "Limit reached!");
	}
	else
	{
		if (getNearStationAir(Settings::removeDistance) == -1) //Remove area also used for making sure there's only one fuel station at one place
		{
			replaceNotification(handleNoteCreate, "Gas station created.");
			station[n] = FuelStation();
			station[n].setup(playerPed);
			station[n].setEnabled(true);
			station[n].setBlipVisible(Settings::blipsVisibility != BlipsOff);
			station[n].saveStationLine(n);
		}
		else
		{
			replaceNotification(handleNoteCreate, "Here already exists a gas station!");
		}
	}
}
void tryToRemoveStation()
{
	int n = getNearStationAir(Settings::removeDistance);
	if (n != -1)
	{
		replaceNotification(handleNoteCreate, "Nearest gas station removed.");
		station[n].setEnabled(false);
		station[n].exists = false;
		station[n].removeStationLine(n);
	}
	else
	{
		replaceNotification(handleNoteCreate, "No gas station found.");
	}
}

bool canRefuelClosestVehWithJerryCan()
{
	//Sitting in Vehicle
	if (playerVeh != 0) return false;

	//Jerry Can Equipped
	Hash currentWeapon;
	WEAPON::GET_CURRENT_PED_WEAPON(playerPed, &currentWeapon, TRUE);
	if (currentWeapon != Settings::weaponHashJerry) return false;

	//Determine closest vehicle
	Vector3 vectP = ENTITY::GET_ENTITY_COORDS(playerPed, false);
	float pX = vectP.x, pY = vectP.y, pZ = vectP.z;
	Vehicle closeVeh = VEHICLE::GET_CLOSEST_VEHICLE(pX, pY, pZ, Settings::maxJerryDist, 0, 70);

	//No Vehcile found
	if (closeVeh == 0) return false;

	//Valid vehicle with combustion engine
	if (ext.GetPetrolTankVolume(closeVeh) <= 0.0f) return false;

	//Already full
	if (0.99f < ext.GetFuelLevel(closeVeh)/ext.GetPetrolTankVolume(closeVeh)) return false;

	//No fuel left in Jerry Can
	if (WEAPON::GET_AMMO_IN_PED_WEAPON(playerPed, Settings::weaponHashJerry) < 1) return false;

	//Vektor zum Fahrzeug
	Vector3 vectV = ENTITY::GET_ENTITY_COORDS(closeVeh, false);
	float vX = vectV.x, vY = vectV.y, vZ = vectV.z;
	//Vektor in Spielerrichtung
	Vector3 vectF = ENTITY::GET_ENTITY_FORWARD_VECTOR(playerPed);
	float fX = vectF.x, fY = vectF.y, fZ = vectF.z;
	//Vektorkomponenten vom Spieler zum Fahrzeug
	float cX = vX - pX;
	float cY = vY - pY;

	float cosPhi = (cX*fX + cY*fY) / (SYSTEM::VDIST(cX, cY, 0, 0, 0, 0) * SYSTEM::VDIST(fX, fY, 0, 0, 0, 0));
	
	//Facing vehicle
	if (cosPhi < 0.6f) return false;
	
	//Nothing wrong
	return true;
}
void refuelWithJerryCan()
{
	//Get closest vehicle and its fuel value
	Vector3 vectP = ENTITY::GET_ENTITY_COORDS(playerPed, false);
	float pX = vectP.x, pY = vectP.y, pZ = vectP.z;
	Vehicle closeVeh = VEHICLE::GET_CLOSEST_VEHICLE(pX, pY, pZ, Settings::maxJerryDist*1.5f, 0, 70);
	if (closeVeh == 0) return;
	float closeLiter = ext.GetFuelLevel(closeVeh);
	float closeCap = ext.GetPetrolTankVolume(closeVeh);

	//Request animation dicationary
	STREAMING::REQUEST_ANIM_DICT("weapon@w_sp_jerrycan");
	DWORD maxWait = GetTickCount() + 2000;
	while (!STREAMING::HAS_ANIM_DICT_LOADED("weapon@w_sp_jerrycan"))
	{
		WAIT(0);
		if (GetTickCount() > maxWait)
		{
			showSubtitle("Didn't load weapon@w_sp_jerrycan. Time out.", 1000);
			return;
		}
	}
	DWORD next;

	//Intro
	AI::TASK_PLAY_ANIM(playerPed, "weapon@w_sp_jerrycan", "fire_intro", 8.0, 0.0, -1, 16, 0.0f, false, false, false);
	next = GetTickCount() + 833;
	while (GetTickCount() < next) WAIT(0);

	int ammo = WEAPON::GET_AMMO_IN_PED_WEAPON(playerPed, eWeapon::WeaponPetrolCan);

	//Refueling
	replaceNotification(handleNoteRefuel, &noteRefuel[0u]);

	while (CONTROLS::IS_CONTROL_PRESSED(2, Settings::refuelControlJerry) && closeLiter < closeCap && ammo > 0)
	{
		AI::TASK_PLAY_ANIM(playerPed, "weapon@w_sp_jerrycan", "fire", 8.0, 0.0, -1, 16, 0.0f, false, false, false);

		for (int x = 0; x < 8; x++) //Simulate continous fuel flow during animation
		{
			if (ammo > 0 && CONTROLS::IS_CONTROL_PRESSED(2, Settings::refuelControlJerry))
			{
				float addLiter = 0.125f * Settings::literPerSecond * 0.3f;       // 1/8 second, 0.5f: slower than refueling at gas station
				ammo -= (int)(4500.0f * addLiter / Settings::capOfJerryCan);     //max 4500 Units gasoline in jerry can
				if (ammo < 0) ammo = 0;
				WEAPON::SET_PED_AMMO(playerPed, Settings::weaponHashJerry, ammo, 0); //Method parameter defined as hash, can't use eWeapon.WeaponPetrolCan ?
				closeLiter += addLiter;
			}
			else
			{
				x = 8;
			}
			next = GetTickCount() + 120;
			while (GetTickCount() < next) { UI::SHOW_HUD_COMPONENT_THIS_FRAME(2); WAIT(0); }
		}
	}

	removeNotification(handleNoteRefuel);
	if (closeLiter > closeCap)
	{
		closeLiter = closeCap;
		showNotification(handleNoteRefuel, &noteFull[0u]);
	}

	//Outro
	AI::TASK_PLAY_ANIM(playerPed, "weapon@w_sp_jerrycan", "fire_outro", 8.0, 0.0, -1, 16, 0.0f, false, false, false);
	next = GetTickCount() + 1033;
	while (GetTickCount() < next) WAIT(0);
	ext.SetFuelLevel(closeVeh, closeLiter);
	showJerryHelp = false;
}

void enableMod(bool enable)
{
	if (!enable)
	{
		nearestRefuel = -1; //Delete 
	}
	for (int n = 0; n < Settings::numOfStations; n++)
	{
		station[n].setEnabled(enable);
		if (enable) station[n].setBlipVisible(Settings::blipsVisibility == BlipsOn); //Show all Blips if they should be always visible
	}
}

//Using the ped model, this will check which main character is currently played (for payment system)
void setLastMainCharacter()
{
	if (PED::IS_PED_MODEL(playerPed, ModelMichael))  lastMainCharacter = Michael;
	if (PED::IS_PED_MODEL(playerPed, ModelFranklin)) lastMainCharacter = Franklin;
	if (PED::IS_PED_MODEL(playerPed, ModelTrevor))   lastMainCharacter = Trevor;
}

void onMenuEnter() {
	Settings::Load();
}
void onMenuExit() {
	Settings::Save();
}

void initialize()
{
	logger.SetFile(GetCurrentModulePath() + "LeFixSimpleFuel\\LeFixSimpleFuel.log");
	logger.Clear();
	logger.Write("LeFixSimpleFuel " + std::string(DISPLAY_VERSION));
	logger.Write("Game version " + eGameVersionToString(getGameVersion()));
	ext.GetOffsets();
	srand((unsigned)time(0));

	//Paths
	std::string path_settings_mod = GetCurrentModulePath() + "LeFixSimpleFuel\\settings_mod.ini";
	std::string path_settings_menu = GetCurrentModulePath() + "LeFixSimpleFuel\\settings_menu.ini";
	std::string path_settings_stations = GetCurrentModulePath() + "LeFixSimpleFuel\\settings_stations.ini";
	std::string path_settings_language = GetCurrentModulePath() + "LeFixSimpleFuel\\settings_language.ini";

	//Mod Settings
	Settings::SetFile(path_settings_mod);
	Settings::Load();

	//Menu
	menu.SetFiles(path_settings_menu);
	menu.RegisterOnMain(std::bind(onMenuEnter));
	menu.RegisterOnExit(std::bind(onMenuExit));
	menu.ReadSettings();
    menu.Initialize();

	//Notes
	noteFull = readString(path_settings_language, "NOTIFICATIONS", "noteFull", "Refueled tank.");
	noteRefuel = readString(path_settings_language, "NOTIFICATIONS", "noteRefuel", "Refueling...");
	noteCrit = readString(path_settings_language, "NOTIFICATIONS", "noteCrit", "Your fuel level is low!");
	std::string inp1 = readString(path_settings_language, "NOTIFICATIONS", "noteRefuelInput1", "Press");
	std::string inp2 = readString(path_settings_language, "NOTIFICATIONS", "noteRefuelInput2", "to refuel.");
	noteRefuelInput = inp1 + " ~INPUT_VEH_HANDBRAKE~ " + inp2;
	noteEmpty = readString(path_settings_language, "NOTIFICATIONS", "noteEmpty", "You ran out of fuel.");
	noteBlip = readString(path_settings_language, "NOTIFICATIONS", "noteBlip", "Gas Station");
	std::string can1 = readString(path_settings_language, "NOTIFICATIONS", "noteJerryCan1", "Hold");
	std::string can2 = readString(path_settings_language, "NOTIFICATIONS", "noteJerryCan2", "to refuel with jerry can.");
	noteJerryCan = can1 + " ~INPUT_CONTEXT~ " + can2;

	barNorm.R = 200;
	barNorm.G = 100;
	barNorm.B = 10;

	barCrit.R = 230;
	barCrit.G = 20;
	barCrit.B = 20;

	//Fuel Stations INI path
	FuelStation::path = path_settings_stations;
	//Set FuelStation Blipname (is written in ini)
	FuelStation::blipName = &noteBlip[0u];

	//Load FuelStation locations
	for (int n = 0; n < Settings::numOfStations; n++)
	{
		std::string key = "coords";
		if (n < 10) key += "0";
		key += std::to_string(n);
		std::string coords = readString(path_settings_stations, "STATIONS", key, "");

		station[n] = FuelStation();
		station[n].setup(coords);
	}
}

void update()
{
	//Refresh references
	player = PLAYER::PLAYER_ID();
	playerPed = PLAYER::PLAYER_PED_ID();

	//Refresh vehicle reference and check if vehicle has changed
	if (playerVeh != PED::GET_VEHICLE_PED_IS_USING(playerPed))
	{
		playerVeh = PED::GET_VEHICLE_PED_IS_USING(playerPed);

		// If has valid vehicle reference get tank Capacatiy
		if (playerVeh == 0)
		{
			tankCapacity = -1.0f;
			fuelBarLevel = -1.0f;
		}
		else
		{
			bool isRoadVehicle;
			//bool isElectric;
			int vClass = VEHICLE::GET_VEHICLE_CLASS(playerVeh);
			switch (vClass)
			{
			case eVehicleClass::VehicleClassCycles:
			case eVehicleClass::VehicleClassBoats:
			case eVehicleClass::VehicleClassHelicopters:
			case eVehicleClass::VehicleClassPlanes:
			case eVehicleClass::VehicleClassTrains:
				isRoadVehicle = false; break;
			default: isRoadVehicle = true; break;
			}

			//auto flags = ext.GetVehicleFlags(playerVeh);
			//isElectric = flags[1] & eVehicleFlag2::FLAG_IS_ELECTRIC;

			tankCapacity = ext.GetPetrolTankVolume(playerVeh);
			//Reference is valid but has vehicle a fuel tank?
			if (!isRoadVehicle || tankCapacity <= 0.0f/* || isElectric*/)
			{
				tankCapacity = -1.0f;
				fuelBarLevel = -1.0f;
			}
			else
			{
				fuelBarLevel = ext.GetFuelLevel(playerVeh) / tankCapacity;
				//Random fuel level
				if (fuelBarLevel == 1.0f) fuelBarLevel = 0.333f + (rand() % 100)*0.666f*0.01f;
			}
		}

		// Reset Display and Inputs
		didRefuelInput = false;
		showInputHelp = false;
		removeNotification(handleNoteRefuel);
		removeNotification(handleNoteEmpty);
	}

	//Valid fuel vehicle
	if (tankCapacity > 0.0f)
	{
		float fuelLiter = ext.GetFuelLevel(playerVeh);

		//Prevent repair and refuel
		if (!Settings::isRefuelingOnRepair && fuelLiter / tankCapacity > fuelBarLevel)
		{
			fuelLiter = tankCapacity * fuelBarLevel;
			ext.SetFuelLevel(playerVeh, fuelLiter);
		}

		//Fuel left and engine on -> consume fuel
		if (fuelBarLevel > 0.0f && VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(playerVeh))
		{
			//Calculate New Fuel
			fuelBarLevel -= GAMEPLAY::GET_FRAME_TIME() * getConsumption() / (60.0f * Settings::fuelTime);

			//Fuel gone
			if (fuelBarLevel < 0.01f)
			{
				fuelBarLevel = 0.0f;
				//Display "Out of Fuel" notification
				replaceNotification(handleNoteEmpty, &noteEmpty[0u]);
			}

			//Set new fuel
			fuelLiter = fuelBarLevel * tankCapacity;
			ext.SetFuelLevel(playerVeh, fuelLiter);
		}

		//Check closest gas station
		int n = getNearStationAir(Settings::maxFillDist);
		if (n != -1 && station[n].isEnabled)
		{
			//In refuel range
			tryToRefuel();
		}
		else
		{
			//No gas station nearby
			showInputHelp = false;
			removeNotification(handleNoteRefuel);
		}

		//Draw Fuel Bar
		if (Settings::isBarDisplayed && PLAYER::IS_PLAYER_CONTROL_ON(player)) drawFuelLevel(fuelBarLevel);
	}
	else
	{
		//Check for refueling manually
		if (showJerryHelp && CONTROLS::IS_CONTROL_PRESSED(2, Settings::refuelControlJerry))
		{
			refuelWithJerryCan();
		}
	}

	if (showJerryHelp) showTextboxTop(noteJerryCan, true);
	if (showInputHelp) showTextboxTop(noteRefuelInput, false);
}
void updateRare()
{
	//No fuel vehicle, no need to go anywhere
	if (tankCapacity <= 0.0f)
	{
		belowCrit = false;
		//Nächstgelegene Station deaktivieren
		if (nearestRefuel != -1)
		{
			station[nearestRefuel].setNearest(false, false);
			station[nearestRefuel].setBlipVisible(Settings::blipsVisibility == BlipsOn);
			nearestRefuel = -1;
		}
	}
	else
	{
		//Grenzwert unterschritten
		if (belowCrit)
		{
			//Ueberprufen
			if (fuelBarLevel > Settings::grenzwert)
			{
				//Grenzwert nicht laenger unterschritten
				belowCrit = false;
				//Nächstgelegene Station deaktivieren
				if (nearestRefuel != -1)
				{
					station[nearestRefuel].setNearest(false, false);
					station[nearestRefuel].setBlipVisible(Settings::blipsVisibility == BlipsOn);
					nearestRefuel = -1;
				}
			}
			else
			{
				//Grenzwert immernoch unterschritten

				//Audio
				if (VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(playerVeh) && Settings::isLowFuelAudioEnabled)
				{
					if (GetTickCount() > nextAudioSignal)
					{
						AUDIO::PLAY_SOUND_FRONTEND(-1, "Virus_Eradicated", "LESTER1A_SOUNDS", 0);
						if (Settings::lowFuelAudioInterval > 0)
						{
							nextAudioSignal = GetTickCount() + Settings::lowFuelAudioInterval*1000;
						}
						else
						{
							nextAudioSignal = 0xFFFFFFFF;
						}
					}
				}

				//Fuel Station
				if (GetTickCount() > nextDistCheck) {
					nextDistCheck = GetTickCount() + Settings::distCalcTime;
					int currentNearest = getNearStationPath(5000);
					if (nearestRefuel != currentNearest)
					{
						//naechstgelegene Station hat sich geändert
						if (nearestRefuel != -1)
						{
							station[nearestRefuel].setNearest(false, false); //alt
							station[nearestRefuel].setBlipVisible(Settings::blipsVisibility == BlipsOn);
						}
						nearestRefuel = currentNearest;
						if (currentNearest != -1)
						{
							station[nearestRefuel].setNearest(true, Settings::isRouteEnabled); //neu
						}
					}
				}
			}
		}
		//Grenzwert gerade unterschritten
		else if (fuelBarLevel <= Settings::grenzwert)
		{
			belowCrit = true;
			//Low Fuel Audio
			if (VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(playerVeh) && Settings::isLowFuelAudioEnabled)
			{
				AUDIO::PLAY_SOUND_FRONTEND(-1, "Virus_Eradicated", "LESTER1A_SOUNDS", 0);
				nextAudioSignal = GetTickCount() + 200;
			}
			//Low Fuel Notification
			if (VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(playerVeh) && Settings::isLowFuelNoteEnabled) replaceNotification(handleNoteEmpty, &noteCrit[0u]);
			//Fuel Station
			if (GetTickCount() > nextDistCheck) {
				nextDistCheck = GetTickCount() + Settings::distCalcTime;
				nearestRefuel = getNearStationPath(5000);
				if (nearestRefuel != -1) station[nearestRefuel].setNearest(true, Settings::isRouteEnabled);
			}
		}
		//Grenzwert immer noch nicht unterschritten
		else
		{
			//Nächstgelegene Station deaktivieren
			if (nearestRefuel != -1)
			{
				station[nearestRefuel].setNearest(false, false);
				station[nearestRefuel].setBlipVisible(Settings::blipsVisibility == BlipsOn);
				nearestRefuel = -1;
			}
		}

		//Blink TODO?
		//if (fuelBarLevel = 0.0f)
		//{
		//	VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(playerVeh, 1, TRUE);
		//	VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(playerVeh, 0, TRUE);
		//}
		//else
		//{

		//}
	}

	//Check screen ratio
	ratioComp = 16.0f/9.0f/GRAPHICS::_GET_ASPECT_RATIO(FALSE);

	//Stop electric cars
	//if (isVehicleValid(playerVeh, false, true) && fuelBarLevel == 0.0f) VEHICLE::SET_VEHICLE_UNDRIVEABLE(playerVeh, true);
	
	//Check if refueling with jerry can is possible
	showJerryHelp = canRefuelClosestVehWithJerryCan();

	//Check which main character is active
	setLastMainCharacter();

	//Toggle blips dynamically if [DRIVE] selected
	if (Settings::blipsVisibility == BlipsAuto) toggleBlips();
}

void updateMenu()
{
	menu.CheckKeys();

	if (menu.CurrentMenu("mainmenu"))
	{
		menu.Title("Simple Fuel");
		menu.Subtitle(DISPLAY_VERSION);
		if (menu.BoolOption("Mod Enabled", Settings::isActive, { "Enable/Disable the entire mod." })) enableMod(Settings::isActive);
		menu.FloatOption("Fuel Time", Settings::fuelTime, 0.1f, 20.0f, 0.1f, { "Time in minutes until tank capacity is consumed when consumption is at 100%. Visible at the bottom while menu is open." });
		menu.MenuOption("Refueling", "refuelmenu");
		menu.MenuOption("Navigation", "navigationmenu");
		menu.MenuOption("Money", "moneymenu");
		menu.MenuOption("Low Fuel", "lowmenu");
		menu.MenuOption("Fuel Bar", "barmenu");
		menu.MenuOption("Stations", "stationsmenu");
		if(tankCapacity > 0) showSubtitle("Consumption: " + std::to_string((int)(100 * getConsumption())) + "%", 100);
	}

	if (menu.CurrentMenu("refuelmenu"))
	{
		menu.Title("Simple Fuel");
		menu.Subtitle("Refueling");
		menu.BoolOption("Realistic", Settings::isRefuelRealistic, { "Vehicle must stop and it'll take some time to refuel. If disabled it will refuel instantly when vehicle is near of a gasstation." });
		menu.BoolOption("Refuel on Repair", Settings::isRefuelingOnRepair, { "GTA V natively refuels the tank when vehicle get repaired, set to false the mod will prevent this." });
		menu.StringArray("Input", { "Auto", "Once", "Hold" }, Settings::refuelInputMode, { "Auto: Automatically refuel if possible.", "Once: Refuels after pressing input button once.","Hold: Refueling only while holding input button."});
	}

	if (menu.CurrentMenu("navigationmenu"))
	{
		menu.Title("Simple Fuel");
		menu.Subtitle("Navigation");
		menu.IntOption("Shortest Route update frequency", Settings::distCalcTime, 1000, 60000, 200, { "In milliseconds. Set it higher if you experience performance problems." });
		if (menu.BoolOption("Shortest Route", Settings::isRouteEnabled, { "This will show the route to the nearest gas station when fuel level is critical. It doesn't interfere with existing waypoints." }))
		{
			if (nearestRefuel > -1) station[nearestRefuel].setNearest(true, Settings::isRouteEnabled);
		}
		if (menu.StringArray("Blips Visible", { "Off", "On", "Auto" }, Settings::blipsVisibility, { "Off: Only closest gas station blip when fuel level is critical.", "On: Gas stations blips are always visible.","Auto: Blips are only visible while driving a vehicle." }))
		{
			toggleBlips();
		}
	}

	if (menu.CurrentMenu("moneymenu"))
	{
		menu.Title("Simple Fuel");
		menu.Subtitle("Money");
		menu.BoolOption("Pay for Fuel", Settings::hasGasPrice, { "Toggle if current character's money is used to buy fuel." });
		menu.FloatOption("Gas Price", Settings::gasPrice, 0.1f, 10.0f, 0.1f, { "Set gas price per liter." });
	}

	if (menu.CurrentMenu("lowmenu"))
	{
		menu.Title("Simple Fuel");
		menu.Subtitle("Low Fuel");
		menu.BoolOption("Notification", Settings::isLowFuelNoteEnabled, { "Will show a notification when fuel level becomes critical." });
		menu.BoolOption("Audio Reminder", Settings::isLowFuelAudioEnabled, { "Will play a sound as a reminder when fuel level becomes critical." });
		menu.IntOption("Audio Interval", Settings::lowFuelAudioInterval, 0, 60, 5, { "Will repeat sound with specified time interval in seconds. Zero won't repeat the sound." });
	}

	if (menu.CurrentMenu("barmenu"))
	{
		menu.Title("Simple Fuel");
		menu.Subtitle("Fuel Bar");
		menu.BoolOption("Visible", Settings::isBarDisplayed, { "Prints the fuel level on the screen. Provided that player is using a fuel vehicle and player stats Health/Stamina are visible." });
		menu.FloatOption("X Position", Settings::barX, 0.0f, 1.0f, 0.001f);
		menu.FloatOption("Y Position", Settings::barY, 0.0f, 1.0f, 0.001f);
		menu.FloatOption("Width", Settings::barW, 0.0f, 1.0f, 0.001f);
		menu.FloatOption("Height", Settings::barH, 0.0f, 1.0f, 0.001f);
	}

	if (menu.CurrentMenu("stationsmenu"))
	{
		menu.Title("Simple Fuel");
		menu.Subtitle("Stations");
		if (menu.Option("Create Station Here", { "Creates a new gas station at current location. Must be 30m away from another gas station." }))
		{
			tryToCreateStation();
		}
		if (menu.Option("Remove Closest Station", { "Remove nearby gas station. Must be within 30m." }))
		{
			tryToRemoveStation();
		}
	}

	menu.EndMenu();
}

////MAIN////
void ScriptMain()
{
	static int runCount = 0;
	initialize();
	enableMod(Settings::isActive);

	while (true) {

		if (Settings::isActive)
		{
			update();
			runCount++;
			if (runCount >= 20)
			{
				updateRare();
				runCount = 0;
			}
		}
		updateMenu();
		WAIT(0);
	}
}
