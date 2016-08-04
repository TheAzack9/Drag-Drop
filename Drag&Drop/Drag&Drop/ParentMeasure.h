#pragma once

struct ChildMeasure;

struct ParentMeasure
{
	void* skin;
	LPCWSTR name;
	ChildMeasure* ownerChild;
	std::vector<ChildMeasure*> children;
	HWND wnd;

	ParentMeasure() : skin(), name(), ownerChild(), children() {}
};