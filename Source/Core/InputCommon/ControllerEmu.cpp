// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include "InputCommon/ControllerEmu.h"

void ControllerEmu::UpdateReferences(ControllerInterface& devi)
{
	for (auto& ctrlGroup : groups)
	{
		for (auto& control : ctrlGroup->controls)
			devi.UpdateReference(control->control_ref.get(), default_device);

		// extension
		if (ctrlGroup->type == GROUP_TYPE_EXTENSION)
		{
			for (auto& attachment : ((Extension*)ctrlGroup.get())->attachments)
				attachment->UpdateReferences(devi);
		}
	}
}

void ControllerEmu::UpdateDefaultDevice()
{
	for (auto& ctrlGroup : groups)
	{
		// extension
		if (ctrlGroup->type == GROUP_TYPE_EXTENSION)
		{
			for (auto& ai : ((Extension*)ctrlGroup.get())->attachments)
			{
				ai->default_device = default_device;
				ai->UpdateDefaultDevice();
			}
		}
	}
}

void ControllerEmu::ControlGroup::LoadConfig(IniFile::Section *sec, const std::string& defdev, const std::string& base)
{
	std::string group(base + name); group += "/";

	// settings
	for (auto& s : settings)
	{
		sec->Get(group + s->name, &s->value, s->default_value * 100);
		s->value /= 100;
	}

	for (auto& c : controls)
	{
		// control expression
		sec->Get(group + c->name, &c->control_ref->expression, "");

		// range
		sec->Get(group + c->name + "/Range", &c->control_ref->range, 100.0f);
		c->control_ref->range /= 100;

	}

	// extensions
	if (type == GROUP_TYPE_EXTENSION)
	{
		Extension* const ext = (Extension*)this;

		ext->switch_extension = 0;
		unsigned int n = 0;
		std::string extname;
		sec->Get(base + name, &extname, "");

		for (auto& ai : ext->attachments)
		{
			ai->default_device.FromString(defdev);
			ai->LoadConfig(sec, base + ai->GetName() + "/");

			if (ai->GetName() == extname)
				ext->switch_extension = n;

			n++;
		}
	}
}

void ControllerEmu::LoadConfig(IniFile::Section *sec, const std::string& base)
{
	std::string defdev = default_device.ToString();
	if (base.empty())
	{
		sec->Get(base + "Device", &defdev, "");
		default_device.FromString(defdev);
	}

	for (auto& cg : groups)
		cg->LoadConfig(sec, defdev, base);
}

void ControllerEmu::ControlGroup::SaveConfig(IniFile::Section *sec, const std::string& defdev, const std::string& base)
{
	std::string group(base + name); group += "/";

	for (auto& s : settings)
		sec->Set(group + s->name, s->value*100.0f, s->default_value*100.0f);

	for (auto& c : controls)
	{
		// control expression
		sec->Set(group + c->name, c->control_ref->expression, "");

		// range
		sec->Set(group + c->name + "/Range", c->control_ref->range*100.0f, 100.0f);
	}

	// extensions
	if (type == GROUP_TYPE_EXTENSION)
	{
		Extension* const ext = (Extension*)this;
		sec->Set(base + name, ext->attachments[ext->switch_extension]->GetName(), "None");

		for (auto& ai : ext->attachments)
			ai->SaveConfig(sec, base + ai->GetName() + "/");
	}
}

void ControllerEmu::SaveConfig(IniFile::Section *sec, const std::string& base)
{
	const std::string defdev = default_device.ToString();
	if (base.empty())
		sec->Set(/*std::string(" ") +*/ base + "Device", defdev, "");

	for (auto& ctrlGroup : groups)
		ctrlGroup->SaveConfig(sec, defdev, base);
}

ControllerEmu::AnalogStick::AnalogStick(const char* const _name) : ControlGroup(_name, GROUP_TYPE_STICK)
{
	for (auto& named_direction : named_directions)
		controls.emplace_back(new Input(named_direction));

	controls.emplace_back(new Input(_trans("Modifier")));

	settings.emplace_back(new Setting(_trans("Radius"), 0.7f, 0, 100));
	settings.emplace_back(new Setting(_trans("Dead Zone"), 0, 0, 50));
	settings.emplace_back(new Setting(_trans("Square Stick"), 0));

}

template <typename C>
void ControllerEmu::AnalogStick::GetState(C* const x, C* const y, const unsigned int base, const unsigned int range)
{
	// this is all a mess

	ControlState yy = controls[0]->control_ref->State() - controls[1]->control_ref->State();
	ControlState xx = controls[3]->control_ref->State() - controls[2]->control_ref->State();

	ControlState radius = settings[SETTING_RADIUS]->value;
	ControlState deadzone = settings[SETTING_DEADZONE]->value;
	ControlState square = settings[SETTING_SQUARE]->value;
	ControlState m = controls[4]->control_ref->State();

	// modifier code
	if (m)
	{
		yy = (fabsf(yy)>deadzone) * sign(yy) * (m + deadzone/2);
		xx = (fabsf(xx)>deadzone) * sign(xx) * (m + deadzone/2);
	}

	// deadzone / square stick code
	if (radius != 1 || deadzone || square)
	{
		// this section might be all wrong, but its working good enough, I think

		ControlState ang = atan2(yy, xx);
		ControlState ang_sin = sin(ang);
		ControlState ang_cos = cos(ang);

		// the amt a full square stick would have at current angle
		ControlState square_full = std::min(ang_sin ? 1/fabsf(ang_sin) : 2, ang_cos ? 1/fabsf(ang_cos) : 2);

		// the amt a full stick would have that was ( user setting squareness) at current angle
		// I think this is more like a pointed circle rather than a rounded square like it should be
		ControlState stick_full = (1 + (square_full - 1) * square);

		ControlState dist = sqrt(xx*xx + yy*yy);

		// dead zone code
		dist = std::max(0.0f, dist - deadzone * stick_full);
		dist /= (1 - deadzone);

		// square stick code
		ControlState amt = dist / stick_full;
		dist -= ((square_full - 1) * amt * square);

		// radius
		dist *= radius;

		yy = std::max(-1.0f, std::min(1.0f, ang_sin * dist));
		xx = std::max(-1.0f, std::min(1.0f, ang_cos * dist));
	}

	*y = C(yy * range + base);
	*x = C(xx * range + base);
}

ControllerEmu::Buttons::Buttons(const std::string& _name) : ControlGroup(_name, GROUP_TYPE_BUTTONS)
{
	settings.emplace_back(new Setting(_trans("Threshold"), 0.5f));
}

template <typename C>
void ControllerEmu::Buttons::GetState(C* const buttons, const C* bitmasks)
{
	for (auto& control : controls)
	{
		if (control->control_ref->State() > settings[0]->value) // threshold
			*buttons |= *bitmasks;

		bitmasks++;
	}
}

ControllerEmu::MixedTriggers::MixedTriggers(const std::string& _name) : ControlGroup(_name, GROUP_TYPE_MIXED_TRIGGERS)
{
	settings.emplace_back(new Setting(_trans("Threshold"), 0.9f));
}

template <typename C, typename S>
void ControllerEmu::MixedTriggers::GetState(C* const digital, const C* bitmasks, S* analog, const unsigned int range)
{
	const unsigned int trig_count = ((unsigned int) (controls.size() / 2));
	for (unsigned int i=0; i<trig_count; ++i,++bitmasks,++analog)
	{
		if (controls[i]->control_ref->State() > settings[0]->value) //threshold
		{
			*analog = range;
			*digital |= *bitmasks;
		}
		else
		{
			*analog = S(controls[i+trig_count]->control_ref->State() * range);
		}
	}
}

ControllerEmu::Triggers::Triggers(const std::string& _name) : ControlGroup(_name, GROUP_TYPE_TRIGGERS)
{
	settings.emplace_back(new Setting(_trans("Dead Zone"), 0, 0, 50));
}

template <typename S>
void ControllerEmu::Triggers::GetState(S* analog, const unsigned int range)
{
	const unsigned int trig_count = ((unsigned int) (controls.size()));
	const ControlState deadzone = settings[0]->value;
	for (unsigned int i=0; i<trig_count; ++i,++analog)
		*analog = S(std::max(controls[i]->control_ref->State() - deadzone, 0.0f) / (1 - deadzone) * range);
}

ControllerEmu::Slider::Slider(const std::string& _name) : ControlGroup(_name, GROUP_TYPE_SLIDER)
{
	controls.emplace_back(new Input("Left"));
	controls.emplace_back(new Input("Right"));

	settings.emplace_back(new Setting(_trans("Dead Zone"), 0, 0, 50));
}

template <typename S>
void ControllerEmu::Slider::GetState(S* const slider, const unsigned int range, const unsigned int base /* = 0 */)
{
	const float deadzone = settings[0]->value;
	const float state = controls[1]->control_ref->State() - controls[0]->control_ref->State();

	if (fabsf(state) > deadzone)
		*slider = (S)((state - (deadzone * sign(state))) / (1 - deadzone) * range + base);
	else
		*slider = 0;
}

ControllerEmu::Force::Force(const std::string& _name) : ControlGroup(_name, GROUP_TYPE_FORCE)
{
	memset(m_swing, 0, sizeof(m_swing));

	controls.emplace_back(new Input(_trans("Up")));
	controls.emplace_back(new Input(_trans("Down")));
	controls.emplace_back(new Input(_trans("Left")));
	controls.emplace_back(new Input(_trans("Right")));
	controls.emplace_back(new Input(_trans("Forward")));
	controls.emplace_back(new Input(_trans("Backward")));

	settings.emplace_back(new Setting(_trans("Dead Zone"), 0, 0, 50));
}

template <typename C, typename R>
void ControllerEmu::Force::GetState(C* axis, const u8 base, const R range)
{
	const float deadzone = settings[0]->value;
	for (unsigned int i=0; i<6; i+=2)
	{
		float tmpf = 0;
		const float state = controls[i+1]->control_ref->State() - controls[i]->control_ref->State();
		if (fabsf(state) > deadzone)
			tmpf = ((state - (deadzone * sign(state))) / (1 - deadzone));

		float &ax = m_swing[i >> 1];
		*axis++ = (C)((tmpf - ax) * range + base);
		ax = tmpf;
	}
}

ControllerEmu::Tilt::Tilt(const std::string& _name)
	: ControlGroup(_name, GROUP_TYPE_TILT)
{
	memset(m_tilt, 0, sizeof(m_tilt));

	controls.emplace_back(new Input("Forward"));
	controls.emplace_back(new Input("Backward"));
	controls.emplace_back(new Input("Left"));
	controls.emplace_back(new Input("Right"));

	controls.emplace_back(new Input(_trans("Modifier")));

	settings.emplace_back(new Setting(_trans("Dead Zone"), 0, 0, 50));
	settings.emplace_back(new Setting(_trans("Circle Stick"), 0));
	settings.emplace_back(new Setting(_trans("Angle"), 0.9f, 0, 180));
}

template <typename C, typename R>
void ControllerEmu::Tilt::GetState(C* const x, C* const y, const unsigned int base, const R range, const bool step /* = true */)
{
	// this is all a mess

	ControlState yy = controls[0]->control_ref->State() - controls[1]->control_ref->State();
	ControlState xx = controls[3]->control_ref->State() - controls[2]->control_ref->State();

	ControlState deadzone = settings[0]->value;
	ControlState circle = settings[1]->value;
	auto const angle = settings[2]->value / 1.8f;
	ControlState m = controls[4]->control_ref->State();

	// modifier code
	if (m)
	{
		yy = (fabsf(yy)>deadzone) * sign(yy) * (m + deadzone/2);
		xx = (fabsf(xx)>deadzone) * sign(xx) * (m + deadzone/2);
	}

	// deadzone / circle stick code
	if (deadzone || circle)
	{
		// this section might be all wrong, but its working good enough, I think

		ControlState ang = atan2(yy, xx);
		ControlState ang_sin = sin(ang);
		ControlState ang_cos = cos(ang);

		// the amt a full square stick would have at current angle
		ControlState square_full = std::min(ang_sin ? 1/fabsf(ang_sin) : 2, ang_cos ? 1/fabsf(ang_cos) : 2);

		// the amt a full stick would have that was (user setting circular) at current angle
		// I think this is more like a pointed circle rather than a rounded square like it should be
		ControlState stick_full = (square_full * (1 - circle)) + (circle);

		ControlState dist = sqrt(xx*xx + yy*yy);

		// dead zone code
		dist = std::max(0.0f, dist - deadzone * stick_full);
		dist /= (1 - deadzone);

		// circle stick code
		ControlState amt = dist / stick_full;
		dist += (square_full - 1) * amt * circle;

		yy = std::max(-1.0f, std::min(1.0f, ang_sin * dist));
		xx = std::max(-1.0f, std::min(1.0f, ang_cos * dist));
	}

	// this is kinda silly here
	// gui being open will make this happen 2x as fast, o well

	// silly
	if (step)
	{
		if (xx > m_tilt[0])
			m_tilt[0] = std::min(m_tilt[0] + 0.1f, xx);
		else if (xx < m_tilt[0])
			m_tilt[0] = std::max(m_tilt[0] - 0.1f, xx);

		if (yy > m_tilt[1])
			m_tilt[1] = std::min(m_tilt[1] + 0.1f, yy);
		else if (yy < m_tilt[1])
			m_tilt[1] = std::max(m_tilt[1] - 0.1f, yy);
	}

	*y = C(m_tilt[1] * angle * range + base);
	*x = C(m_tilt[0] * angle * range + base);
}

ControllerEmu::Cursor::Cursor(const std::string& _name)
	: ControlGroup(_name, GROUP_TYPE_CURSOR)
	, m_z(0)
{
	for (auto& named_direction : named_directions)
		controls.emplace_back(new Input(named_direction));
	controls.emplace_back(new Input("Forward"));
	controls.emplace_back(new Input("Backward"));
	controls.emplace_back(new Input(_trans("Hide")));

	settings.emplace_back(new Setting(_trans("Center"), 0.5f));
	settings.emplace_back(new Setting(_trans("Width"), 0.5f));
	settings.emplace_back(new Setting(_trans("Height"), 0.5f));

}

template <typename C>
void ControllerEmu::Cursor::GetState(C* const x, C* const y, C* const z, const bool adjusted /* = false */)
{
	const float zz = controls[4]->control_ref->State() - controls[5]->control_ref->State();

	// silly being here
	if (zz > m_z)
		m_z = std::min(m_z + 0.1f, zz);
	else if (zz < m_z)
		m_z = std::max(m_z - 0.1f, zz);

	*z = m_z;

	// hide
	if (controls[6]->control_ref->State() > 0.5f)
	{
		*x = 10000; *y = 0;
	}
	else
	{
		float yy = controls[0]->control_ref->State() - controls[1]->control_ref->State();
		float xx = controls[3]->control_ref->State() - controls[2]->control_ref->State();

		// adjust cursor according to settings
		if (adjusted)
		{
			xx *= (settings[1]->value * 2);
			yy *= (settings[2]->value * 2);
			yy += (settings[0]->value - 0.5f);
		}

		*x = xx;
		*y = yy;
	}
}

void ControllerEmu::LoadDefaults(const ControllerInterface &ciface)
{
	// load an empty inifile section, clears everything
	IniFile::Section sec;
	LoadConfig(&sec);

	if (ciface.Devices().size())
	{
		default_device.FromDevice(ciface.Devices()[0]);
		UpdateDefaultDevice();
	}
}
