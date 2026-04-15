#include "Settings.h"

#include <simpleini/SimpleIni.h>
#include "simpleFuelEnums.h"

std::string Settings::_filepath;

////CONSTANT////
const float Settings::literPerSecond = 6.0f;
const float Settings::capOfJerryCan = 20.0f;
const float Settings::maxFillVel = 0.1f;
const int Settings::maxFillDist = 13;
const float Settings::grenzwert = 0.2f;
const int Settings::numOfStations = 100;
const int Settings::removeDistance = 30;
const float Settings::maxJerryDist = 3.0f;
const float Settings::standardCap = 64.25f;

const int refuelControlStation = eControl::ControlVehicleHandbrake;
const int refuelControlJerry = eControl::ControlContext;

const DWORD weaponHashJerry = 0x34A67B97; //typedef Hash 

//// NOT CONSTANT ////

// Main
bool Settings::isActive = true;
float Settings::fuelTime = 6.0f;

// Refueling
bool Settings::isRefuelRealistic = true;
bool Settings::isRefuelingOnRepair = false;
int Settings::refuelInputMode = RefuelInputOnce;

// Navigation
int Settings::distCalcTime = 5000;
int Settings::blipsVisibility = BlipsAuto;
bool Settings::isRouteEnabled = true;

// Money
float Settings::gasPrice = 1.0f;
bool Settings::hasGasPrice = true;

// Low
bool Settings::isLowFuelAudioEnabled = true;
int Settings::lowFuelAudioInterval = 20;
bool Settings::isLowFuelNoteEnabled = false;

// Bar
bool Settings::isBarDisplayed = true;
float Settings::barX = 0.016f;
float Settings::barY = 0.990f;
float Settings::barW = 0.140f;
float Settings::barH = 0.014f;


void Settings::SetFile(const std::string &filepath) {
	_filepath = filepath;
}

void Settings::Save() {
	CSimpleIniA settings;
	settings.SetUnicode();
	settings.LoadFile(_filepath.c_str());

	// Main
	settings.SetBoolValue("MAIN", "isActive", isActive);
	settings.SetDoubleValue("MAIN", "fuelTime", fuelTime);

	// Refueling
	settings.SetBoolValue("REFUELING", "isRefuelRealistic", isRefuelRealistic);
	settings.SetBoolValue("REFUELING", "isRefuelingOnRepair", isRefuelingOnRepair);
	settings.SetLongValue("REFUELING", "refuelInputMode", refuelInputMode);

	// Navigation
	settings.SetLongValue("NAVIGATION", "distCalcTime", distCalcTime);
	settings.SetLongValue("NAVIGATION", "blipsVisibility", blipsVisibility);
	settings.SetBoolValue("NAVIGATION", "isRouteEnabled", isRouteEnabled);

	// Money
	settings.SetBoolValue("MONEY", "payForFuel", hasGasPrice);
	settings.SetDoubleValue("MOENY", "gasPrice", gasPrice);

	// Low Fuel
	settings.SetBoolValue("LOW", "lowFuelAudioReminder", isLowFuelAudioEnabled);
	settings.SetLongValue("LOW", "lowFuelAudioInterval", lowFuelAudioInterval);
	settings.SetBoolValue("LOW", "lowFuelNotification", isLowFuelNoteEnabled);

	// Bar
	settings.SetBoolValue("BAR", "isBarDisplayed", isBarDisplayed);
	settings.SetDoubleValue("BAR", "barX", barX);
	settings.SetDoubleValue("BAR", "barY", barY);
	settings.SetDoubleValue("BAR", "barW", barW);
	settings.SetDoubleValue("BAR", "barH", barH);

	//Write to file
	settings.SaveFile(_filepath.c_str());
}

void Settings::Load() {
#pragma warning(push)
#pragma warning(disable: 4244) // Make everything doubles later...
	CSimpleIniA settings;
	settings.SetUnicode();
	settings.LoadFile(_filepath.c_str());

	// Main
	isActive = settings.GetBoolValue("MAIN", "isActive", true);
	fuelTime = settings.GetDoubleValue("MAIN", "fuelTime", 6.0);

	// Refueling
	isRefuelRealistic = settings.GetBoolValue("REFUELING", "isRefuelRealistic", true);
	isRefuelingOnRepair = settings.GetBoolValue("REFUELING", "isRefuelingOnRepair", false);
	refuelInputMode = settings.GetLongValue("REFUELING", "refuelInputMode", RefuelInputOnce);

	// Navigation
	distCalcTime = settings.GetLongValue("NAVIGATION", "distCalcTime", 5000);
	blipsVisibility = settings.GetLongValue("NAVIGATION", "blipsVisibility", BlipsAuto);
	isRouteEnabled = settings.GetBoolValue("NAVIGATION", "isRouteEnabled", true);

	// Money
	hasGasPrice = settings.GetBoolValue("MONEY", "payForFuel", true);
	gasPrice = settings.GetDoubleValue("MOENY", "gasPrice", 1.0);

	// Low Fuel
	isLowFuelAudioEnabled = settings.GetBoolValue("LOW", "lowFuelAudioReminder", true);
	lowFuelAudioInterval = settings.GetLongValue("LOW", "lowFuelAudioInterval", 20);
	isLowFuelNoteEnabled = settings.GetBoolValue("LOW", "lowFuelNotification", true);

	// Bar
	isBarDisplayed = settings.GetBoolValue("BAR", "isBarDisplayed", true);
	barX = settings.GetDoubleValue("BAR", "barX", 0.015);
	barY = settings.GetDoubleValue("BAR", "barY", 0.990);
	barW = settings.GetDoubleValue("BAR", "barW", 0.140);
	barH = settings.GetDoubleValue("BAR", "barH", 0.014);
}