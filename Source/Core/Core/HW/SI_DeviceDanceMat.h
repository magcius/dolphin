// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#pragma once

#include "Core/HW/SI_DeviceGCController.h"

class CSIDevice_DanceMat : public CSIDevice_GCController
{
public:
	CSIDevice_DanceMat(SIDevices device, int _iDeviceNumber);
	virtual u32 MapPadStatus(const GCPadStatus& padStatus) override;
	virtual void HandleButtonCombos(const GCPadStatus& padStatus) override;
};
