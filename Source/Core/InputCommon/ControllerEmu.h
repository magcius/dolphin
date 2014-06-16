// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#pragma once

// windows crap
#define NOMINMAX

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include "Common/IniFile.h"
#include "InputCommon/GCPadStatus.h"
#include "InputCommon/ControllerInterface/ControllerInterface.h"

#define sign(x) ((x)?(x)<0?-1:1:0)

enum
{
	GROUP_TYPE_OTHER,
	GROUP_TYPE_STICK,
	GROUP_TYPE_MIXED_TRIGGERS,
	GROUP_TYPE_BUTTONS,
	GROUP_TYPE_FORCE,
	GROUP_TYPE_EXTENSION,
	GROUP_TYPE_TILT,
	GROUP_TYPE_CURSOR,
	GROUP_TYPE_TRIGGERS,
	GROUP_TYPE_UDPWII,
	GROUP_TYPE_SLIDER,
};

enum
{
	SETTING_RADIUS,
	SETTING_DEADZONE,
	SETTING_SQUARE,
};

const char* const named_directions[] =
{
	"Up",
	"Down",
	"Left",
	"Right"
};

class ControllerEmu
{
public:

	class ControlGroup
	{
	public:

		class Control
		{
		protected:
			Control(ControllerInterface::ControlReference* const _ref, const std::string& _name)
				: control_ref(_ref), name(_name) {}

		public:
			virtual ~Control() {}
			std::unique_ptr<ControllerInterface::ControlReference> const control_ref;
			const std::string name;

		};

		class Input : public Control
		{
		public:

			Input(const std::string& _name)
				: Control(new ControllerInterface::InputReference, _name) {}
		};

		class Output : public Control
		{
		public:

			Output(const std::string& _name)
				: Control(new ControllerInterface::OutputReference, _name) {}
		};

		class Setting
		{
		public:

			Setting(const std::string& _name, const ControlState def_value
				, const unsigned int _low = 0, const unsigned int _high = 100)
				: name(_name)
				, value(def_value)
				, default_value(def_value)
				, low(_low)
				, high(_high){}

			const std::string   name;
			ControlState        value;
			const ControlState  default_value;
			const unsigned int  low, high;
		};

		ControlGroup(const std::string& _name, const unsigned int _type = GROUP_TYPE_OTHER) : name(_name), type(_type) {}
		virtual ~ControlGroup() {}

		virtual void LoadConfig(IniFile::Section *sec, const std::string& defdev = "", const std::string& base = "" );
		virtual void SaveConfig(IniFile::Section *sec, const std::string& defdev = "", const std::string& base = "" );

		const std::string     name;
		const unsigned int    type;

		std::vector<std::unique_ptr<Control>> controls;
		std::vector<std::unique_ptr<Setting>> settings;

	};

	class AnalogStick : public ControlGroup
	{
	public:
		AnalogStick(const char* const _name);

		template <typename C>
		void GetState(C* const x, C* const y, const unsigned int base, const unsigned int range);
	};

	class Buttons : public ControlGroup
	{
	public:
		Buttons(const std::string& _name);

		template <typename C>
		void GetState(C* const buttons, const C* bitmasks);
	};

	class MixedTriggers : public ControlGroup
	{
	public:
		MixedTriggers(const std::string& _name);

		template <typename C, typename S>
		void GetState(C* const digital, const C* bitmasks, S* analog, const unsigned int range);
	};

	class Triggers : public ControlGroup
	{
	public:
		Triggers(const std::string& _name);

		template <typename S>
		void GetState(S* analog, const unsigned int range);
	};

	class Slider : public ControlGroup
	{
	public:
		Slider(const std::string& _name);

		template <typename S>
		void GetState(S* const slider, const unsigned int range, const unsigned int base = 0);
	};

	class Force : public ControlGroup
	{
	public:
		Force(const std::string& _name);

		template <typename C, typename R>
		void GetState(C* axis, const u8 base, const R range);

	private:
		float m_swing[3];
	};

	class Tilt : public ControlGroup
	{
	public:
		Tilt(const std::string& _name);

		template <typename C, typename R>
		void GetState(C* const x, C* const y, const unsigned int base, const R range, const bool step = true);

	private:
		float m_tilt[2];
	};

	class Cursor : public ControlGroup
	{
	public:
		Cursor(const std::string& _name);

		template <typename C>
		void GetState(C* const x, C* const y, C* const z, const bool adjusted = false);

		float m_z;
	};

	class Extension : public ControlGroup
	{
	public:
		Extension(const std::string& _name)
			: ControlGroup(_name, GROUP_TYPE_EXTENSION)
			, switch_extension(0)
			, active_extension(0) {}

		~Extension() {}

		void GetState(u8* const data, const bool focus = true);

		std::vector<std::unique_ptr<ControllerEmu>> attachments;

		int switch_extension;
		int active_extension;
	};

	virtual ~ControllerEmu() {}

	virtual std::string GetName() const = 0;

	virtual void LoadDefaults(const ControllerInterface& ciface);

	virtual void LoadConfig(IniFile::Section *sec, const std::string& base = "");
	virtual void SaveConfig(IniFile::Section *sec, const std::string& base = "");
	void UpdateDefaultDevice();

	void UpdateReferences(ControllerInterface& devi);

	std::vector<std::unique_ptr<ControlGroup>> groups;

	DeviceQualifier default_device;
};
