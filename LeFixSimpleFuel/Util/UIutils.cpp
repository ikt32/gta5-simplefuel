#include "UIutils.hpp"

//Notification

void showNotification(std::string message) {
	int x;
	showNotification(x, message);
}

void showNotification(int &handle, std::string message) {
	HUD::BEGIN_TEXT_COMMAND_THEFEED_POST("STRING");
	HUD::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(&message[0u]);
	handle = HUD::END_TEXT_COMMAND_THEFEED_POST_TICKER(false, false);
}

void removeNotification(int &handle)
{
	if (handle != 0) HUD::THEFEED_REMOVE_ITEM(handle);
}

void replaceNotification(int &handle, std::string message)
{
	removeNotification(handle);
	showNotification(handle, message);
}

//Subtitle
void showSubtitle(std::string text, int msec)
{
	HUD::BEGIN_TEXT_COMMAND_PRINT("STRING");
	HUD::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(&text[0u]);
	HUD::END_TEXT_COMMAND_PRINT(msec, 1);
}

//Textbox
void showTextboxTop(std::string text, bool beep) {
	HUD::BEGIN_TEXT_COMMAND_DISPLAY_HELP("STRING");
	HUD::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(&text[0u]);
	HUD::END_TEXT_COMMAND_DISPLAY_HELP(0, false, beep, -1);
}

//Dbg
void showText(float x, float y, float scale, const std::string &text, int font, bool outline) {
    HUD::SET_TEXT_FONT(font);
    HUD::SET_TEXT_SCALE(scale, scale);
    HUD::SET_TEXT_COLOUR(255, 255, 255, 255);
    HUD::SET_TEXT_WRAP(0.0, 1.0);
    HUD::SET_TEXT_CENTRE(0);
    if (outline) HUD::SET_TEXT_OUTLINE();
    HUD::BEGIN_TEXT_COMMAND_DISPLAY_TEXT("STRING");
    HUD::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(text.c_str());
    HUD::END_TEXT_COMMAND_DISPLAY_TEXT(Vector2{ x, y }, 0);
}
