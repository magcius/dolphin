// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#pragma once

#include "Core/HW/DSPHLE/UCodes/UCodes.h"

class INITUCode : public UCodeInterface
{
public:
	INITUCode(DSPHLE *dsphle, u32 crc);
	virtual ~INITUCode();
	u32 GetUpdateMs() override;

	void HandleMail(u32 mail) override;
	void Update(int cycles) override;
	void Init();
};
