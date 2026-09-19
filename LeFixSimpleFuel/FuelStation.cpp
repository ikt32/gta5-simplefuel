#include "FuelStation.hpp"
#include "Util/INIutils.hpp"
#include <inc/natives.h>
#include <stdexcept>

std::string FuelStation::path = "";

FuelStation::FuelStation()
{
	X = 0;
	Y = 0;
	Z = 0;
	exists = false;
	isEnabled = false;
	visible = false;
}

std::string getStringBetween(std::string s, size_t pos1, size_t pos2)
{
	return s.substr(pos1 + 1, pos2 - pos1 - 1);
}

void FuelStation::setup()
{
	X = 0;
	Y = 0;
	Z = 0;
	exists = false;
}
void FuelStation::setup(int x, int y, int z)
{
	X = x;
	Y = y;
	Z = z;
	exists = true;
}
void FuelStation::setup(float x, float y, float z)
{
	setup((int)x, (int)y, (int)z);
}
void FuelStation::setup(Entity e)
{
	Vector3 pos = ENTITY::GET_ENTITY_COORDS(e, false);
	setup(pos.x, pos.y, pos.z);
}
void FuelStation::setup(std::string coords)
{
	if (coords.length() < 5)
	{
		setup();
	}
	else
	{
		size_t pos0 = coords.find_first_of('(', 0);
		size_t pos1 = coords.find_first_of('|', 0);
		size_t pos2 = coords.find_last_of('|', coords.length() - 1);
		size_t pos3 = coords.find_first_of(')', 0);

		std::string xs = getStringBetween(coords, pos0, pos1);
		std::string ys = getStringBetween(coords, pos1, pos2);
		std::string zs = getStringBetween(coords, pos2, pos3);

		try{
			int x = std::stoi(xs);
			int y = std::stoi(ys);
			int z = std::stoi(zs);
			setup(x, y, z);
		}
		catch (const std::invalid_argument&)
		{
			setup();
		}
	}
}

int FuelStation::getAirDistanceSquared(Entity e)
{
	Vector3 pos = ENTITY::GET_ENTITY_COORDS(e, false);
	return (
		(X - (int)pos.x)*(X - (int)pos.x) +
		(Y - (int)pos.y)*(Y - (int)pos.y) +
		(Z - (int)pos.z)*(Z - (int)pos.z)
	);
}

int FuelStation::getTravelDistance(Entity e)
{
	Vector3 pos = ENTITY::GET_ENTITY_COORDS(e, false);
	return int(PATHFIND::CALCULATE_TRAVEL_DISTANCE_BETWEEN_POINTS(
		pos, Vector3{ static_cast<float>(X), static_cast<float>(Y), static_cast<float>(Z) }));
}

void FuelStation::saveStationLine(int n)
{
	std::string section = "STATIONS";

	std::string key = "coords";
	if (n < 10) key = key + "0";
	key = key + std::to_string(n);

	std::string xs = std::to_string(X);
	while (xs.length() < 5) xs = " " + xs;

	std::string ys = std::to_string(Y);
	while (ys.length() < 5) ys = " " + ys;

	std::string zs = std::to_string(Z);
	while (zs.length() < 5) zs = " " + zs;

	std::string value = "(" + xs + "|" + ys + "|" + zs + ")";

	saveString(path, section, key, value);
}

void FuelStation::removeStationLine(int n)
{
	std::string section = "STATIONS";

	std::string key = "coords";
	if (n < 10) key = key + "0";
	key = key + std::to_string(n);

	std::string value = "";

	saveString(path, section, key, value);
}

void FuelStation::setEnabled(bool enable)
{
	if (exists)
	{
		//Disable
		if (isEnabled && !enable)
		{
			isEnabled = false;
			removeBlip();
		}
		//Enable
		else if (!isEnabled && enable)
		{
			isEnabled = true;
		}
	}
}

void FuelStation::setBlipVisible(bool visible)
{
	if (exists)
	{
		if (visible)
		{
			setupBlip();
		}
		else
		{
			removeBlip();
		}
	}
}

void FuelStation::setNearest(bool active, bool route)
{
	if (active) setBlipVisible(true);
	HUD::SET_BLIP_FLASHES(blip, active);
	HUD::SET_BLIP_ROUTE(blip, route);
	HUD::SET_BLIP_ROUTE_COLOUR(blip, 4); //White
}

void FuelStation::setupBlip()
{
	if (!visible)
	{
		blip = HUD::ADD_BLIP_FOR_COORD(
			Vector3{ static_cast<float>(X), static_cast<float>(Y), static_cast<float>(Z) });
		HUD::SET_BLIP_SPRITE(blip, 361); //Jerry Can
		HUD::SET_BLIP_AS_SHORT_RANGE(blip, true);

		HUD::BEGIN_TEXT_COMMAND_SET_BLIP_NAME("STRING");
		HUD::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(blipName);
		HUD::END_TEXT_COMMAND_SET_BLIP_NAME(blip);

		visible = true;
	}
}

void FuelStation::removeBlip()
{
	if (visible)
	{
		HUD::REMOVE_BLIP(&blip);
		blip = 0;
		visible = false;
	}
}

const char* FuelStation::blipName = "Gas Station";
