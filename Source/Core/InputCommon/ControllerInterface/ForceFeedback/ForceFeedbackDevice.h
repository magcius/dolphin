// Copyright 2014 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#pragma once

#include <list>
#include <string>

#include "InputCommon/ControllerInterface/Device.h"

#ifdef _WIN32
#include <Windows.h>
#include "InputCommon/ControllerInterface/DInput/DInput8.h"
#elif __APPLE__
#include "InputCommon/ControllerInterface/ForceFeedback/OSX/DirectInputAdapter.h"
#endif

namespace ciface
{
namespace ForceFeedback
{


class ForceFeedbackDevice : public Core::Device
{
private:
	struct EffectState
	{
		EffectState(LPDIRECTINPUTEFFECT eff) : iface(eff), params(nullptr), size(0) {}

		LPDIRECTINPUTEFFECT iface;
		void*               params; // null when force hasn't changed
		u8                  size;   // zero when force should stop
	};

	template <typename P>
	class Force : public Output
	{
	public:
		std::string GetName() const;
		Force(const std::string& name, EffectState& state);
		void SetState(ControlState state);
	private:
		const std::string m_name;
		EffectState& m_state;
		P params;
	};
	typedef Force<DICONSTANTFORCE>  ForceConstant;
	typedef Force<DIRAMPFORCE>      ForceRamp;
	typedef Force<DIPERIODIC>       ForcePeriodic;

public:
	bool InitForceFeedback(const LPDIRECTINPUTDEVICE8, int cAxes);
	void UpdateOutput() override;

	virtual ~ForceFeedbackDevice();
private:
	std::list<EffectState>     m_state_out;
};


}
}
