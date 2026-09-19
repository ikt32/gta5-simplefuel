#pragma once

#include <string>
#include <inc/types.h>

class FuelStation
{
public:

	static const char* blipName;
	static std::string path;

	bool exists;
	bool isEnabled;

	FuelStation();

	void setup();
	void setup(  int x,   int y,   int z);
	void setup(float x, float y, float z);
	void setup(Entity e);
	void setup(std::string coords);

	int getAirDistanceSquared(Entity e); //Distance to entity, squared
	int getTravelDistance(Entity e); //Pathfind method
	void saveStationLine(int n); //Save coordinates of station to .ini file
	void removeStationLine(int n); //Remove coordinates of station in .ini file
	void setEnabled(bool enable); //Enable/Disable station without removing data
	void setBlipVisible(bool visible);
	void setNearest(bool active, bool route);

private:

	Blip blip;
	int X, Y, Z;
	bool visible;
	
	void setupBlip();
	void removeBlip();
};

/*
coords00=(  -64|-2530|    5)
coords01=(  -67|-1760|   29)
coords02=(  173|-1560|   28)
coords03=( -319|-1472|   30)
coords04=( 1207|-1400|   35)
coords05=(  267|-1261|   29)
coords06=( -525|-1210|   18)
coords07=(  821|-1029|   26)
coords08=( -720| -934|   18)
coords09=( 1178| -328|   69)
coords10=(-2094| -318|   12)
coords11=(-1436| -275|   45)
coords12=(  623|  268|  102)
coords13=( 2578|  361|  108)
coords14=(-1802|  802|  138)
coords15=(-2554| 2331|   32)
coords16=( 2537| 2593|   37)
coords17=(  263| 2604|   44)
coords18=( 1206| 2658|   37)
coords19=( 1039| 2669|   39)
coords20=(   52| 2783|   57)
coords21=( 2677| 3265|   55)
coords22=( 1780| 3327|   40)
coords24=( 2003| 3776|   32)
coords23=( 1691| 4926|   41)
coords25=(  177| 6602|   31)
coords26=(  -90| 6415|   31)
coords27=( 1702| 6418|   32)
*/
