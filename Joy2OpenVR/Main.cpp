﻿#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <string>
#include <map>
#include <Windows.h>
#include "SimpleIni.h"
#include "functions.h"
#include "InputEmulatorSequences.h"
#include <iostream>  
#include <math.h>  

namespace
{

	struct JoystickObject
	{
		sf::Text label;
		sf::Text value;
	};

	typedef std::map<std::string, JoystickObject> Texts;
	Texts texts;
	std::ostringstream sstr;
	float threshold = 1.0f;
	float deadzonerange = 0.2f;
	float axisvalue = 0;

	// ini main
	CString steamvr_controller1_id;
	CString steamvr_controller2_id;
	CString temp_steamvr_controller_id;

	// ini buttons
	unsigned int trigger1;
	unsigned int menu1;
	unsigned int system1;
	unsigned int grip1;
	unsigned int trigger2;
	unsigned int menu2;
	unsigned int system2;
	unsigned int grip2;

	// ini buttons (keyboard)
	CString ktrigger1;
	CString kmenu1;
	CString ksystem1;
	CString kgrip1;
	CString ktrigger2;
	CString kmenu2;
	CString ksystem2;
	CString kgrip2;
	CString ktrackpad1left;
	CString ktrackpad1right;
	CString ktrackpad1up;
	CString ktrackpad1down;
	CString ktrackpad2left;
	CString ktrackpad2right;
	CString ktrackpad2up;
	CString ktrackpad2down;
	bool ktrackpad1_click;
	bool ktrackpad2_click;

	// ini trackpad
	char * xy_axes;
	char * zr_axes;
	char * uv_axes;
	unsigned int trackpad1_click;
	unsigned int trackpad2_click;
	unsigned int temp_trackpad_click;

	// press / release
	bool p_trigger1 = false;
	bool p_menu1 = false;
	bool p_system1 = false;
	bool p_grip1 = false;
	bool p_trigger2 = false;
	bool p_menu2 = false;
	bool p_system2 = false;
	bool p_grip2 = false;
	bool touchpadpressed = false;

	// trackpad modifier, click if pressed, touch otherwise
	bool p_trackpad1_click = false;
	bool p_trackpad2_click = false;
	bool trackpad1_click_reverse;
	bool trackpad2_click_reverse;

	DWORD exitCode = 0;
	CString lastcmd;
	
	// SteamVR controller ID associated with joystick axes
	CString xyctrlid;
	CString zrctrlid;
	CString uvctrlid;

	bool windowontop = false;
	int unresponsiveness;
	HWND activewindow;


	// Axes labels in as C strings
	const char* axislabels[] = { "X", "Y", "Z", "R", "U", "V", "PovX", "PovY" };

	// Helper to set text entries to a specified value
	template<typename T>
	void set(const char* label, const T& value)
	{
		sstr.str("");
		sstr << value;
		texts[label].value.setString(sstr.str());
	}

	//************************************************
	// Update joystick identification
	void updateIdentification(unsigned int index)
	{
		sstr.str("");
		sstr << "Joystick " << index << ":";
		texts["ID"].label.setString(sstr.str());
		texts["ID"].value.setString(sf::Joystick::getIdentification(index).name);
	}

	//************************************************
	// associate axes to controller id
	void Axes2Controller(CString steamvr_ctrl1_id, CString steamvr_ctrl2_id)
	{
		if (xy_axes == (CString)"trackpad1" || xy_axes == (CString)"trigger1")
		{
			xyctrlid = steamvr_ctrl1_id;
		}
		else {
			xyctrlid = steamvr_ctrl2_id;
		}

		if (zr_axes == (CString)"trackpad1" || zr_axes == (CString)"trigger1")
		{
			zrctrlid = steamvr_ctrl1_id;
		}
		else {
			zrctrlid = steamvr_ctrl2_id;
		}

		if (uv_axes == (CString)"trackpad1" || uv_axes == (CString)"trigger1")
		{
			uvctrlid = steamvr_ctrl1_id;
		}
		else {
			uvctrlid = steamvr_ctrl2_id;
		}
	}


	//************************************************
	// Update joystick axes
	void updateAxes(unsigned int index)
	{
		for (unsigned int j = 0; j < sf::Joystick::AxisCount; ++j)
		{
			if (sf::Joystick::hasAxis(index, static_cast<sf::Joystick::Axis>(j)))
				set(axislabels[j], sf::Joystick::getAxisPosition(index, static_cast<sf::Joystick::Axis>(j)));
		}
	}

	//************************************************
	// Update joystick buttons
	void updateButtons(unsigned int index)
	{
		for (unsigned int j = 0; j < sf::Joystick::getButtonCount(index); ++j)
		{
			sstr.str("");
			sstr << "Button " << j;

			set(sstr.str().c_str(), sf::Joystick::isButtonPressed(index, j));
		}
	}


	//************************************************
	void trigger_up(CString id)
	{
		executeCommandLine("client_commandline.exe axisevent " + id + "  1 0 0", exitCode);
		lastcmd = "Trigger Up >> id" + id;
		texts["LastCommand"].value.setString((sf::String)lastcmd);
	}

	//************************************************
	void trigger_down(CString id)
	{
		executeCommandLine("client_commandline.exe axisevent "+id+"  1 1 0", exitCode);
		lastcmd = "Trigger Down >> id " + id;
		texts["LastCommand"].value.setString((sf::String)lastcmd);
	}


	//************************************************
	void system_up(CString id)
	{
		executeCommandLine("client_commandline.exe buttonevent unpress " + id + "  0", exitCode);
		lastcmd = "System Up >> id " + id;
		texts["LastCommand"].value.setString((sf::String)lastcmd);
	}

	//************************************************
	void system_down(CString id)
	{
		executeCommandLine("client_commandline.exe buttonevent pressandhold " + id + "  0", exitCode);
		lastcmd = "System Down >> id " + id;
		texts["LastCommand"].value.setString((sf::String)lastcmd);
	}


	//************************************************
	void menu_up(CString id)
	{
		executeCommandLine("client_commandline.exe buttonevent unpress " + id + "  1", exitCode);
		lastcmd = "Menu Up >> id " + id;
		texts["LastCommand"].value.setString((sf::String)lastcmd);
	}

	//************************************************
	void menu_down(CString id)
	{
		executeCommandLine("client_commandline.exe buttonevent pressandhold " + id + "  1", exitCode);
		lastcmd = "Menu Down >> id " + id;
		texts["LastCommand"].value.setString((sf::String)lastcmd);
	}


	//************************************************
	void grip_up(CString id)
	{
		executeCommandLine("client_commandline.exe buttonevent unpress " + id + "  2", exitCode);
		lastcmd = "Grip Up >> id " + id;
		texts["LastCommand"].value.setString((sf::String)lastcmd);
	}

	//************************************************
	void grip_down(CString id)
	{
		executeCommandLine("client_commandline.exe buttonevent pressandhold " + id + "  2", exitCode);
		lastcmd = "Grip Down >> id " + id;
		texts["LastCommand"].value.setString((sf::String)lastcmd);
	}


	//************************************************
	// Buttons press
	void ButtonPress(unsigned int index)
	{

		// controller1 buttons

		// trigger1
		if(sf::Joystick::isButtonPressed(index, trigger1))
		{
			trigger_down(steamvr_controller1_id);
			p_trigger1 = true;

		}

		// menu1
		if (sf::Joystick::isButtonPressed(index, menu1))
		{
			menu_down(steamvr_controller1_id);
			p_menu1 = true;
		}



		// system1
		if (sf::Joystick::isButtonPressed(index, system1))
		{
			system_down(steamvr_controller1_id);
			p_system1 = true;
		}


		// grip1
		if (sf::Joystick::isButtonPressed(index, grip1))
		{
			grip_down(steamvr_controller1_id);
			p_grip1 = true;
		}



		// controller2 buttons

		// trigger2
		if (sf::Joystick::isButtonPressed(index, trigger2))
		{
			trigger_down(steamvr_controller2_id);
			p_trigger2 = true;
		}

		// menu2
		if (sf::Joystick::isButtonPressed(index, menu2))
		{
			menu_down(steamvr_controller2_id);
			p_menu2 = true;
		}


		// system2
		if (sf::Joystick::isButtonPressed(index, system2))
		{
			system_down(steamvr_controller2_id); 
			p_system2 = true;
		}


		// grip2
		if (sf::Joystick::isButtonPressed(index, grip2))
		{
			grip_down(steamvr_controller2_id);
			p_grip2 = true;
		}


		// trackpad1_click
		if (sf::Joystick::isButtonPressed(index, trackpad1_click))
		{
			p_trackpad1_click = true;			
		} else { 
			p_trackpad1_click = false; 
		}

		// trackpad2_click
		if (sf::Joystick::isButtonPressed(index, trackpad2_click))
		{
			p_trackpad2_click = true;
		} else { 
			p_trackpad2_click = false; 
		}



	}

	//************************************************
	// Buttons release
	void ButtonRelease()
	{

		// controller1 buttons

		// trigger1
		if (p_trigger1)
		{
			trigger_up(steamvr_controller1_id);
			p_trigger1 = false;
		}

		// menu1
		if (p_menu1)
		{
			menu_up(steamvr_controller1_id);
			p_menu1 = false;
		}



		// system1
		if (p_system1)
		{
			system_up(steamvr_controller1_id);
			p_system1 = false;
		}


		// grip1
		if (p_grip1)
		{
			grip_up(steamvr_controller1_id);
			p_grip1 = false;
		}



		// controller2 buttons

		// trigger2
		if (p_trigger2)
		{
			trigger_up(steamvr_controller2_id);
			p_trigger2 = false;
		}

		// menu2
		if (p_menu2)
		{
			menu_up(steamvr_controller2_id);
			p_menu2 = false;
		}



		// system2
		if (p_system2)
		{
			system_up(steamvr_controller2_id);
			p_system2 = false;
		}


		// grip2
		if (p_grip2)
		{
			grip_up(steamvr_controller2_id);
			p_grip2 = false;
		}


	}

	//************************************************
	void Axis_Set(CString id, CString xvalue, CString yvalue)
	{
		// touch event at x,y
		executeCommandLine("client_commandline.exe buttonevent touchandhold " + id + "  32", exitCode);
		executeCommandLine("client_commandline.exe axisevent " + id + "  0 "+ xvalue+" "+yvalue, exitCode);
		
		// emulate trackpad click
		if (id == steamvr_controller1_id && (!p_trackpad1_click == trackpad1_click_reverse))
		{
			executeCommandLine("client_commandline.exe buttonevent pressandhold " + id + "  32", exitCode); // touchpad pressed
			touchpadpressed = true;
			return;
		}


		if (id == steamvr_controller2_id && (!p_trackpad2_click == trackpad2_click_reverse))
		{
			executeCommandLine("client_commandline.exe buttonevent pressandhold " + id + "  32", exitCode); // touchpad pressed
			touchpadpressed = true;
			return; 
		}

		// if pressed, but no click request, unpress
		if (touchpadpressed)
		{
			executeCommandLine("client_commandline.exe buttonevent unpress " + id + "  32", exitCode); // touchpad unpressed
			touchpadpressed = false;
		}

	}


	// Axes actions
	bool Xdz = false; // dead zone
	bool Ydz = false; // dead zone
	bool Rdz = false; // dead zone
	bool Zdz = false; // dead zone
	bool Udz = false; // dead zone
	bool Vdz = false; // dead zone
	bool xychanged = true;
	bool zrchanged = true;
	bool uvchanged = true;
	CString xstraxisvalue; // last x value
	CString ystraxisvalue; // last y value
	CString rstraxisvalue; // last r value
	CString zstraxisvalue; // last z value
	CString ustraxisvalue; // last u value
	CString vstraxisvalue; // last v value
	
	void AxesToAction(unsigned int index)
	{

		// XY AXES
		// X
		if (sf::Joystick::hasAxis(index, static_cast<sf::Joystick::Axis>(0)))
		{
			axisvalue = sf::Joystick::getAxisPosition(index, static_cast<sf::Joystick::Axis>(0)) / 100;
			if (axisvalue > -deadzonerange && axisvalue < deadzonerange) 
			{ 
				axisvalue = 0;
			}
			else {
				Xdz = false;
			}

			if (!Xdz)
			{
				xstraxisvalue.Format(L"%.*f", 1, axisvalue);
				if (axisvalue == 0) { Xdz = true; }
				xychanged = true;
			}
		}

		// Y
		if (sf::Joystick::hasAxis(index, static_cast<sf::Joystick::Axis>(1)))
		{
			axisvalue = sf::Joystick::getAxisPosition(index, static_cast<sf::Joystick::Axis>(1)) / 100;
			// y is inverted on Vive trackpad
			axisvalue = -axisvalue;
			if (axisvalue > -deadzonerange && axisvalue < deadzonerange)
			{
				axisvalue = 0;
			}
			else {
				Ydz = false;
			}
			if (!Ydz)
			{
				ystraxisvalue.Format(L"%.*f", 1, axisvalue);
				if (axisvalue == 0) { Ydz = true; }
				xychanged = true;
			}
		}

		if (xychanged)
		{
			if (xy_axes == (CString)"trackpad1" || xy_axes == (CString)"trackpad2")
			{
				Axis_Set(xyctrlid, xstraxisvalue, ystraxisvalue);
				if (touchpadpressed)
				{
					lastcmd = "Axis X,Y >> id " + xyctrlid + ": " + xstraxisvalue + "," + ystraxisvalue + " (pressed)";
				}
				else {
					lastcmd = "Axis X,Y >> id " + xyctrlid + ": " + xstraxisvalue + "," + ystraxisvalue;
				}
				texts["LastCommand"].value.setString((sf::String)lastcmd);
			}
			else {
				// trigger event: xbox 360 triggers act as single axis in -100 +100 range
				if (axisvalue != 0)
				{
					trigger_down(xyctrlid);
				}
				else {
					trigger_up(xyctrlid);
				}
			}
			xychanged = false;
		}




		// ZR AXES
		// Z
		if (sf::Joystick::hasAxis(index, static_cast<sf::Joystick::Axis>(2)))
		{
			axisvalue = sf::Joystick::getAxisPosition(index, static_cast<sf::Joystick::Axis>(2)) / 100;
			// y is inverted on SteamVR
			axisvalue = -axisvalue;
			if (axisvalue > -deadzonerange && axisvalue < deadzonerange)
			{
				axisvalue = 0;
			}
			else {
				Zdz = false;
			}
			if (!Zdz)
			{
				zstraxisvalue.Format(L"%.*f", 1, axisvalue);
				if (axisvalue == 0) { Zdz = true; }
				zrchanged = true;
			}
		}

		// R
		if (sf::Joystick::hasAxis(index, static_cast<sf::Joystick::Axis>(3)))
		{
			axisvalue = sf::Joystick::getAxisPosition(index, static_cast<sf::Joystick::Axis>(3)) / 100;
			if (axisvalue > -deadzonerange && axisvalue < deadzonerange)
			{
				axisvalue = 0;
			}
			else {
				Rdz = false;
			}

			if (!Rdz)
			{
				rstraxisvalue.Format(L"%.*f", 1, axisvalue);
				if (axisvalue == 0) { Rdz = true; }
				zrchanged = true;
			}
		}

		if (zrchanged)
		{
			//std::cout << "zrchanged" << std::endl;
			if (zr_axes == (CString)"trackpad1" || zr_axes == (CString)"trackpad2")
			{
				Axis_Set(zrctrlid, rstraxisvalue, zstraxisvalue);
				if (touchpadpressed)
				{
					lastcmd = "Axis Z,R >> id " + zrctrlid + ": " + rstraxisvalue + "," + zstraxisvalue + " (pressed)";
				}
				else {
					lastcmd = "Axis Z,R >> id " + zrctrlid + ": " + rstraxisvalue + "," + zstraxisvalue;
				}
				texts["LastCommand"].value.setString((sf::String)lastcmd);
			}
			else {
				// trigger event: xbox 360 triggers act as single axis in -100 +100 range
				if (axisvalue != 0)
				{
					trigger_down(zrctrlid);
				}
				else {
					trigger_up(zrctrlid);
				}
			}
			zrchanged = false;
		}



		// UV AXES
		// U
		if (sf::Joystick::hasAxis(index, static_cast<sf::Joystick::Axis>(4)))
		{
			axisvalue = sf::Joystick::getAxisPosition(index, static_cast<sf::Joystick::Axis>(4)) / 100;
			// y is inverted on SteamVR
			axisvalue = -axisvalue;
			if (axisvalue > -deadzonerange && axisvalue < deadzonerange)
			{
				axisvalue = 0;
			}
			else {
				Udz = false;
			}
			if (!Udz)
			{
				zstraxisvalue.Format(L"%.*f", 1, axisvalue);
				if (axisvalue == 0) { Udz = true; }
				uvchanged = true;
			}
		}

		// V
		if (sf::Joystick::hasAxis(index, static_cast<sf::Joystick::Axis>(5)))
		{
			axisvalue = sf::Joystick::getAxisPosition(index, static_cast<sf::Joystick::Axis>(5)) / 100;
			if (axisvalue > -deadzonerange && axisvalue < deadzonerange)
			{
				axisvalue = 0;
			}
			else {
				Vdz = false;
			}

			if (!Vdz)
			{
				rstraxisvalue.Format(L"%.*f", 1, axisvalue);
				if (axisvalue == 0) { Vdz = true; }
				uvchanged = true;
			}
		}

		if (uvchanged)
		{
			if (uv_axes == (CString)"trackpad1" || uv_axes == (CString)"trackpad2")
			{
				Axis_Set(uvctrlid, rstraxisvalue, zstraxisvalue);
				if (touchpadpressed)
				{
					lastcmd = "Axis U,V >> id " + uvctrlid + ": " + rstraxisvalue + "," + zstraxisvalue + " (pressed)";
				}
				else {
					lastcmd = "Axis U,V >> id " + uvctrlid + ": " + rstraxisvalue + "," + zstraxisvalue;
				}
				texts["LastCommand"].value.setString((sf::String)lastcmd);
			}
			else {
				// trigger event: xbox 360 triggers act as single axis in -100 +100 range
				if (axisvalue != 0)
				{
					trigger_down(uvctrlid);
				}
				else {
					trigger_up(uvctrlid);
				}
			}
			uvchanged = false;
		}



	}


	// terminare - gestirere up, down, click e lastcommand

	//************************************************
	void Ktrackpad(CString id, int action,CString cnum)
	{

		// 0 = off - 1 = left - 2 = right - 3 = up - 4 = down

		switch (action)
		{
		case 0:
			lastcmd = "trackpad "+cnum+" released";
			executeCommandLine("client_commandline.exe axisevent " + id + "  0 0 0", exitCode);
			executeCommandLine("client_commandline.exe buttonevent unpress " + id + "  32", exitCode); // touchpad unpressed
			break;
		case 1:
			lastcmd = "trackpad " + cnum + " LEFT";
			executeCommandLine("client_commandline.exe buttonevent touchandhold " + id + "  32", exitCode);
			executeCommandLine("client_commandline.exe axisevent " + id + "  0 -1 0", exitCode);
			break;
		case 2:
			lastcmd = "trackpad " + cnum + " RIGHT";
			executeCommandLine("client_commandline.exe buttonevent touchandhold " + id + "  32", exitCode);
			executeCommandLine("client_commandline.exe axisevent " + id + "  0 1 0", exitCode);
			break;
		case 3:
			lastcmd = "trackpad " + cnum + " UP";
			executeCommandLine("client_commandline.exe buttonevent touchandhold " + id + "  32", exitCode);
			executeCommandLine("client_commandline.exe axisevent " + id + "  0 0 1", exitCode);
			break;
		case 4:
			lastcmd = "trackpad " + cnum + " DOWN";
			executeCommandLine("client_commandline.exe buttonevent touchandhold " + id + "  32", exitCode);
			executeCommandLine("client_commandline.exe axisevent " + id + "  0 0 -1", exitCode);
			break;
		}

		if (((id == steamvr_controller1_id && ktrackpad1_click) || (id == steamvr_controller2_id && ktrackpad2_click)) && action != 0)
		{
			executeCommandLine("client_commandline.exe buttonevent pressandhold " + id + "  32", exitCode);
			lastcmd = lastcmd + " clicked";
		}



		texts["LastCommand"].value.setString((sf::String)lastcmd);



	}



	//************************************************
	// KeyToAction
	void KeyToAction(CString keypressed,bool pressed)
	{


		// controller1 buttons

		// trigger1
		if (keypressed == ktrigger1)
		{
			if (pressed) { trigger_down(steamvr_controller1_id); } else { trigger_up(steamvr_controller1_id); }
		}


		// menu1
		if (keypressed == kmenu1)
		{
			if (pressed) { menu_down(steamvr_controller1_id); }	else { menu_up(steamvr_controller1_id); }
		}
		

		// system1
		if (keypressed == ksystem1)
		{
			if (pressed) { system_down(steamvr_controller1_id); } else { system_up(steamvr_controller1_id); }
		}


		// grip1
		if (keypressed == kgrip1)
		{
			if (pressed) { grip_down(steamvr_controller1_id); } else { grip_up(steamvr_controller1_id); }
		}

		// trackpad1 left
		if (keypressed == ktrackpad1left)
		{
			if (pressed) { Ktrackpad(steamvr_controller1_id, 1,"1"); } else { Ktrackpad(steamvr_controller1_id, 0, "1"); }
		}

		// trackpad1 right
		if (keypressed == ktrackpad1right)
		{
			if (pressed) { Ktrackpad(steamvr_controller1_id, 2, "1"); } else { Ktrackpad(steamvr_controller1_id, 0, "1"); }
		}

		// trackpad1 up
		if (keypressed == ktrackpad1up)
		{
			if (pressed) { Ktrackpad(steamvr_controller1_id, 3, "1"); } else { Ktrackpad(steamvr_controller1_id, 0, "1"); }
		}

		// trackpad1 down
		if (keypressed == ktrackpad1down)
		{
			if (pressed) { Ktrackpad(steamvr_controller1_id, 4, "1"); } else { Ktrackpad(steamvr_controller1_id, 0, "1"); }
		}


		// controller2 buttons

		// trigger2
		if (keypressed == ktrigger2)
		{
			if (pressed) { trigger_down(steamvr_controller2_id); } else { trigger_up(steamvr_controller2_id); }
		}


		// menu2
		if (keypressed == kmenu2)
		{
			if (pressed) { menu_down(steamvr_controller2_id); } else { menu_up(steamvr_controller2_id); }
		}


		// system2
		if (keypressed == ksystem2)
		{
			if (pressed) { system_down(steamvr_controller2_id); } else { system_up(steamvr_controller2_id); }
		}


		// grip2
		if (keypressed == kgrip2)
		{
			if (pressed) { grip_down(steamvr_controller2_id); } else { grip_up(steamvr_controller2_id); }
		}


		// trackpad2 left
		if (keypressed == ktrackpad2left)
		{
			if (pressed) { Ktrackpad(steamvr_controller2_id, 1, "2"); } else { Ktrackpad(steamvr_controller2_id, 0, "2"); }
		}

		// trackpad2 right
		if (keypressed == ktrackpad2right)
		{
			if (pressed) { Ktrackpad(steamvr_controller2_id, 2, "2"); } else { Ktrackpad(steamvr_controller2_id, 0, "2"); }
		}

		// trackpad2 up
		if (keypressed == ktrackpad2up)
		{
			if (pressed) { Ktrackpad(steamvr_controller2_id, 3, "2"); } else { Ktrackpad(steamvr_controller2_id, 0, "2"); }
		}

		// trackpad2 down
		if (keypressed == ktrackpad2down)
		{
			if (pressed) { Ktrackpad(steamvr_controller2_id, 4, "2"); } else { Ktrackpad(steamvr_controller2_id, 0, "2"); }
		}



	}


	// Helper to update displayed joystick values
	void updateValues(unsigned int index)
	{
		if (sf::Joystick::isConnected(index)) {
			// Update the label-value sf::Text objects based on the current joystick state
			updateIdentification(index);
			updateAxes(index);
			updateButtons(index);
		}
	}


	// integer to string conversion
	template<class T>
	std::string toString(const T &value) {
		std::ostringstream os;
		os << value;
		return os.str();
	}


	// string to bool
	bool ToBool(const std::string & s) {
		return s == "true";
	}


	void makeWindowOnTop(sf::RenderWindow& window)
	{
		HWND hwnd = window.getSystemHandle();
		SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	}

}



////////////////////////////////////////////////////////////
/// Entry point of application
////////////////////////////////////////////////////////////
int main(int argc,char *argv[])
{

	// std::cout << "argc " << argc << argv[1] << std::endl;


	// initialize sound
	sf::SoundBuffer clickbuffer;
	sf::Sound click;
	if (!clickbuffer.loadFromFile("resources/click.wav"))
		return -1;
	click.setBuffer(clickbuffer);


	// read configuration
	CSimpleIniA ini;
	if (argc > 1)
	{
		ini.LoadFile(argv[1]);
	}
	else {
		ini.LoadFile("config.ini");
	}

	steamvr_controller1_id = (CString)ini.GetValue("main", "steamvr_controller1_id", "0");
	steamvr_controller2_id = (CString)ini.GetValue("main", "steamvr_controller2_id", "0");
	threshold = atof(ini.GetValue("main", "threshold", "1.0"));
	deadzonerange = atof(ini.GetValue("main", "deadzonerange", "0.2"));
	windowontop = ToBool(ini.GetValue("main", "WindowOnTop", "false"));
	unresponsiveness = std::stoi(ini.GetValue("main", "unresponsiveness", "10"));

	// xinput prefs
	trigger1 = std::stoi(ini.GetValue("buttons", "trigger1", "999"));
	menu1 = std::stoi(ini.GetValue("buttons", "menu1", "999"));
	system1 = std::stoi(ini.GetValue("buttons", "system1", "999"));
	grip1 = std::stoi(ini.GetValue("buttons", "grip1", "999"));

	trigger2 = std::stoi(ini.GetValue("buttons", "trigger2", "999"));
	menu2 = std::stoi(ini.GetValue("buttons", "menu2", "999"));
	system2 = std::stoi(ini.GetValue("buttons", "system2", "999"));
	grip2 = std::stoi(ini.GetValue("buttons", "grip2", "999"));

	// keyboard prefs
	ktrigger1 = (CString)ini.GetValue("keys", "trigger1", "999");
	kmenu1 = (CString)ini.GetValue("keys", "menu1", "999");
	ksystem1 = (CString)ini.GetValue("keys", "system1", "999");
	kgrip1 = (CString)ini.GetValue("keys", "grip1", "999");

	ktrigger2 = (CString)ini.GetValue("keys", "trigger2", "999");
	kmenu2 = (CString)ini.GetValue("keys", "menu2", "999");
	ksystem2 = (CString)ini.GetValue("keys", "system2", "999");
	kgrip2 = (CString)ini.GetValue("keys", "grip2", "999");

	ktrackpad1left = (CString)ini.GetValue("keys", "trackpad1left", "999");
	ktrackpad1right = (CString)ini.GetValue("keys", "trackpad1right", "999");
	ktrackpad1up = (CString)ini.GetValue("keys", "trackpad1up", "999");
	ktrackpad1down = (CString)ini.GetValue("keys", "trackpad1down", "999");

	ktrackpad2left = (CString)ini.GetValue("keys", "trackpad2left", "999");
	ktrackpad2right = (CString)ini.GetValue("keys", "trackpad2right", "999");
	ktrackpad2up = (CString)ini.GetValue("keys", "trackpad2up", "999");
	ktrackpad2down = (CString)ini.GetValue("keys", "trackpad2down", "999");
	ktrackpad1_click = ToBool(ini.GetValue("keys", "trackpad1_click", "false"));
	ktrackpad2_click = ToBool(ini.GetValue("keys", "trackpad2_click", "false"));

	// axes
	xy_axes = (char *)ini.GetValue("axes", "xy_axes", "");
	zr_axes = (char *)ini.GetValue("axes", "zr_axes", "");
	uv_axes = (char *)ini.GetValue("axes", "uv_axes", "");

	Axes2Controller(steamvr_controller1_id, steamvr_controller2_id); // associate trackpad num to controller id


	trackpad1_click = std::stoi(ini.GetValue("axes", "trackpad1_click", 0));
	trackpad2_click = std::stoi(ini.GetValue("axes", "trackpad2_click", 0));

	sf::String profile = (sf::String)ini.GetValue("main", "profile", "0");


	trackpad1_click_reverse = ToBool(ini.GetValue("axes", "trackpad1_click_reverse", "false"));
	trackpad2_click_reverse = ToBool(ini.GetValue("axes", "trackpad2_click_reverse", "false"));


	// Create the window of the application
	sf::String appname="Joy2OpenVR ";
	appname += "0.11b";
	sf::RenderWindow window(sf::VideoMode(600, 780), appname, sf::Style::Close);
	window.setVerticalSyncEnabled(true);

	sf::Image AppIcon;
	if (!AppIcon.loadFromFile("resources/icon.png"))
		return EXIT_FAILURE;
	window.setIcon(256, 256, AppIcon.getPixelsPtr());



	// Threshold (0-100)
	window.setJoystickThreshold(threshold);

	// Load the text font
	sf::Font font;
	if (!font.loadFromFile("resources/Roboto-Regular.ttf"))
		return EXIT_FAILURE;


	// buttons

	// Invert controller IDs
	sf::Texture invertButton;
	sf::Sprite invertButtonImage;
	if (!invertButton.loadFromFile("resources/invertButton.png"))
		return EXIT_FAILURE;
	invertButtonImage.setPosition(380.0f, 707.0f);

	float invertButtonWidth = invertButtonImage.getLocalBounds().width;
	float invertButtonHeight = invertButtonImage.getLocalBounds().height;

	invertButtonImage.setTexture(invertButton);


	// Invert Axes IDs
	sf::Texture invertAxesButton;
	sf::Sprite invertAxesButtonImage;
	if (!invertAxesButton.loadFromFile("resources/invertAxesButton.png"))
		return EXIT_FAILURE;
	invertAxesButtonImage.setPosition(380.0f, 637.0f);

	float invertAxesButtonWidth = invertAxesButtonImage.getLocalBounds().width;
	float invertAxesButtonHeight = invertAxesButtonImage.getLocalBounds().height;

	invertAxesButtonImage.setTexture(invertAxesButton);



	// Set up our string conversion parameters
	sstr.precision(2);
	sstr.setf(std::ios::fixed | std::ios::boolalpha);

	// loaded profile
	texts["profile"].label.setPosition(5.f, 5.f);
	texts["profile"].value.setPosition(60.f, 5.f);

	texts["profile"].label.setString("Profile: ");
	texts["profile"].value.setString(profile);


	// Set up our joystick identification sf::Text objects
	texts["ID"].label.setPosition(5.f, 5.f + 1 * font.getLineSpacing(14));
	texts["ID"].value.setPosition(80.f, 5.f + 1 * font.getLineSpacing(14));

	texts["ID"].label.setString("Joypad Not Connected..."); 
	texts["ID"].value.setString("");

	// Target SteamVR Controller ID
	texts["SteamVr1ID"].label.setPosition(5.f, 5.f + 2 * font.getLineSpacing(14));
	texts["SteamVr1ID"].value.setPosition(225.f, 5.f + 2 * font.getLineSpacing(14));

	texts["SteamVr1ID"].label.setString("Target SteamVR Controller 1 ID:");
	texts["SteamVr1ID"].value.setString((sf::String)steamvr_controller1_id + " - Button " + toString(trackpad1_click) + " as trackpad click");

	texts["SteamVr2ID"].label.setPosition(5.f, 5.f + 3 * font.getLineSpacing(14));
	texts["SteamVr2ID"].value.setPosition(225.f, 5.f + 3 * font.getLineSpacing(14));

	texts["SteamVr2ID"].label.setString("Target SteamVR Controller 2 ID:");
	texts["SteamVr2ID"].value.setString((sf::String)steamvr_controller2_id + " - Button " + toString(trackpad2_click) + " as trackpad click");

	texts["LastCommand"].label.setPosition(5.f, 5.f + 5 * font.getLineSpacing(14));
	texts["LastCommand"].value.setPosition(80.f, 5.f + 5 * font.getLineSpacing(14));

	texts["LastCommand"].label.setString("last cmd:");
	texts["LastCommand"].value.setString("");


	// Set up our label-value sf::Text objects
	for (unsigned int i = 0; i < sf::Joystick::AxisCount; ++i)
	{
		JoystickObject& object = texts[axislabels[i]];

		object.label.setPosition(5.f, 5.f + ((i + 7) * font.getLineSpacing(14)));
		object.label.setString(std::string(axislabels[i]) + ":");

		object.value.setPosition(80.f, 5.f + ((i + 7) * font.getLineSpacing(14)));
		object.value.setString("N/A");
	}

	for (unsigned int i = 0; i < sf::Joystick::ButtonCount; ++i)
	{
		sstr.str("");
		sstr << "Button " << i;
		JoystickObject& object = texts[sstr.str()];

		object.label.setPosition(5.f, 5.f + ((sf::Joystick::AxisCount + i + 7) * font.getLineSpacing(14)));
		object.label.setString(sstr.str() + ":");

		object.value.setPosition(80.f, 5.f + ((sf::Joystick::AxisCount + i + 7) * font.getLineSpacing(14)));
		object.value.setString("N/A");
	}

	for (Texts::iterator it = texts.begin(); it != texts.end(); ++it)
	{
		it->second.label.setFont(font);
		it->second.label.setCharacterSize(15);
		it->second.label.setFillColor(sf::Color::White);

		it->second.value.setFont(font);
		it->second.value.setCharacterSize(15);
		it->second.value.setFillColor(sf::Color::White);
	}

	// Update initially displayed joystick values if a joystick is already connected on startup
	for (unsigned int i = 0; i < sf::Joystick::Count; ++i)
	{
		if (sf::Joystick::isConnected(i))
		{
			updateValues(i);
			break;
		}
	}

	while (window.isOpen())
	{
		Sleep(unresponsiveness);
		// Handle events
		sf::Event event;
		while (window.pollEvent(event))
		{
			// Window closed or escape key pressed: exit
			if ((event.type == sf::Event::Closed) ||
				((event.type == sf::Event::KeyPressed) && (event.key.code == sf::Keyboard::Escape)))
			{
				window.close();
				break;
			}
			else if (event.type == sf::Event::KeyPressed)
			{
				//std::cout << event.key.code << std::endl;
				CString keypressed;
				keypressed.Format(_T("%d"), event.size.width);
				texts["LastCommand"].value.setString((sf::String)("key pressed: "+keypressed));
				KeyToAction(keypressed,true);
			}
			else if (event.type == sf::Event::KeyReleased)
			{
				//std::cout << event.key.code << std::endl;
				CString keypressed;
				keypressed.Format(_T("%d"), event.size.width);
				texts["LastCommand"].value.setString((sf::String)("key released: " + keypressed));
				KeyToAction(keypressed,false);
			}
			else if (event.type == sf::Event::JoystickConnected)
			{
				// Update displayed joystick values
				updateValues(event.joystickConnect.joystickId);
			}
			else if(event.type == sf::Event::JoystickButtonPressed)
			{
				updateValues(event.joystickConnect.joystickId);
				ButtonPress(event.joystickConnect.joystickId);
			}
			else if (event.type == sf::Event::JoystickButtonReleased)
			{
				updateValues(event.joystickConnect.joystickId);
				ButtonRelease();
			}
			else if (event.type == sf::Event::JoystickMoved)
			{
				updateValues(event.joystickConnect.joystickId);
				AxesToAction(event.joystickConnect.joystickId);
			}
			else if (event.type == sf::Event::JoystickDisconnected)
			{
				// Reset displayed joystick values to empty
				for (Texts::iterator it = texts.begin(); it != texts.end(); ++it)
					it->second.value.setString("N/A");

				texts["ID"].label.setString("<Not Connected>");
				texts["ID"].value.setString("");

			}
			else if (event.type == sf::Event::MouseMoved)
			{
				sf::Vector2i mousePos = sf::Mouse::getPosition(window);
				sf::Vector2f mousePosF(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));

				if (invertButtonImage.getGlobalBounds().contains(mousePosF))
				{
					invertButtonImage.setColor(sf::Color(250, 20, 20));
				}
				else
				{
					invertButtonImage.setColor(sf::Color(255, 255, 255));
				}

				if (invertAxesButtonImage.getGlobalBounds().contains(mousePosF))
				{
					invertAxesButtonImage.setColor(sf::Color(250, 20, 20));
				}
				else
				{
					invertAxesButtonImage.setColor(sf::Color(255, 255, 255));
				}

			}
			else if (event.type == sf::Event::MouseButtonPressed)
			{
				sf::Vector2i mousePos = sf::Mouse::getPosition(window);
				sf::Vector2f mousePosF(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));
				
				if (invertButtonImage.getGlobalBounds().contains(mousePosF))
				{
					//std::cout << "click" << std::endl;
					temp_steamvr_controller_id = steamvr_controller1_id;
					steamvr_controller1_id = steamvr_controller2_id;
					steamvr_controller2_id = temp_steamvr_controller_id;
					Axes2Controller(steamvr_controller1_id, steamvr_controller2_id); // re-associate trackpad num to controller id
					texts["SteamVr1ID"].value.setString((sf::String)steamvr_controller1_id+" - Button "+ toString(trackpad1_click)+" as trackpad click");
					texts["SteamVr2ID"].value.setString((sf::String)steamvr_controller2_id + " - Button " + toString(trackpad2_click) + " as trackpad click");
					click.play();
				}
				if (invertAxesButtonImage.getGlobalBounds().contains(mousePosF))
				{
					Axes2Controller(steamvr_controller2_id, steamvr_controller1_id); // re-associate trackpad num to controller id
					temp_trackpad_click=trackpad1_click;
					trackpad1_click = trackpad2_click;
					trackpad2_click = temp_trackpad_click;
					texts["SteamVr1ID"].value.setString((sf::String)steamvr_controller1_id + " - Button " + toString(trackpad1_click) + " as trackpad click");
					texts["SteamVr2ID"].value.setString((sf::String)steamvr_controller2_id + " - Button " + toString(trackpad2_click) + " as trackpad click");
					click.play();
				}
			}
			else if (event.type == sf::Event::LostFocus)
			{
				if (windowontop)
				{
					SetForegroundWindow(activewindow);
				}
			}


		}

		
		// Clear the window
		window.clear(sf::Color(28, 41, 57));

		// Draw the label-value sf::Text objects
		for (Texts::const_iterator it = texts.begin(); it != texts.end(); ++it)
		{
			window.draw(it->second.label);
			window.draw(it->second.value);
		}

		window.draw(invertButtonImage);
		window.draw(invertAxesButtonImage);

		// Display things on screen
		window.display();

		// always on top? (needed for keyboard input)
		if (windowontop)
		{ 
			activewindow = GetActiveWindow();
			makeWindowOnTop(window); 
		}

	}
}
