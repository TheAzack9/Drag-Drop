#pragma once
#include "ISimpleDropTarget.h"
#include <map>
#include <vector>
#include "ChildMeasure.h"
#include "HelperFunctions.h"
#include "../API/RainmeterAPI.h"

struct ParentMeasure;

class RainmeterDropTarget :
	public ISimpleDropTarget
{
public:
	RainmeterDropTarget();
	RainmeterDropTarget(HWND wnd);
	~RainmeterDropTarget();

	void AddParent(ParentMeasure* parent);
	void RemoveParent(ParentMeasure* parent);
	int ParentCount();

private:
	// Inherited via ISimpleDropTarget
	virtual DropEffect OnDragEnter(std::vector<std::wstring> files, DWORD grfKeyState, const POINTL & mousePos) override;
	virtual DropEffect OnDragOver(DWORD grfKeyState, const POINTL & mousePos) override;
	virtual DropEffect OnDragDrop(std::vector<std::wstring> files, DWORD grfKeyState, const POINTL & mousePos) override;
	virtual void OnDragLeave(void) override;
	virtual bool AllowDrop(const POINTL & pt) override;
	void ExecuteDragBang(std::wstring bang, ChildMeasure *child, std::wstring file, const POINTL & mousePos, int number = 0, bool isDrop = false);
	DropEffect GetDropEffect();
	//The number of registered HWNDs and how many measures that are "connected" to it
	std::vector<ParentMeasure*> registeredParents;
	std::vector<std::wstring> prevFiles;
};

