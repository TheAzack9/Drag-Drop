#pragma once
#include "ParentMeasure.h"
#include <Windows.h>
#include <string>

enum class Action {
	Move,
	Copy,
	Delete,
	Shortcut,
	Path
};

struct ChildMeasure
{
	void* skin;
	void* rm;
	ParentMeasure* parent;
	bool enabled;
	bool dropActive;
	bool spamAction;
	bool replaceIfExists;
	bool silent;
	RECT bounds;
	//Only used if the Bounds option is set to a meter, needed to fetch the position of the meter (in a hacky way)
	std::wstring meter;
	std::wstring dropAction;
	std::wstring enterAction;
	std::wstring overAction;
	std::wstring leaveAction;
	std::wstring measureName;
	std::wstring filePath;
	std::wstring previousFile;
	Action dragAction;

	ChildMeasure() : dropAction(), enterAction(), leaveAction(), bounds(), meter(), enabled(true), dropActive(false), spamAction(false), silent(false) {}

	bool ContainsPointL(POINTL pt)
	{
		return bounds.left <= pt.x && bounds.top <= pt.y && bounds.left + bounds.right >= pt.x && bounds.top + bounds.bottom >= pt.y;
	}
};