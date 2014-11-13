// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#pragma once

#include "Core/HW/SI_DeviceGCController.h"

class CSIDevice_GCSteeringWheel : public CSIDevice_GCController
{
private:
	// Commands
	enum EBufferCommands
	{
		CMD_RESET       = 0x00,
		CMD_ORIGIN      = 0x41,
		CMD_RECALIBRATE = 0x42,
		CMD_MOTOR_OFF   = 0xff,
	};

	enum EDirectCommands
	{
		CMD_FORCE = 0x30,
		CMD_WRITE = 0x40
	};

public:
	CSIDevice_GCSteeringWheel(SIDevices device, int _iDeviceNumber);
	virtual void SendCommand(u32 _Cmd, u8 _Poll);
};
