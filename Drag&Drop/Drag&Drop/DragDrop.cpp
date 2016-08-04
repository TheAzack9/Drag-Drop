#include<Windows.h>
#include <string>
#include <map>
#include <vector>
#include "../API/RainmeterAPI.h"
#include "RainmeterMathParser.h"
#include "HelperFunctions.h"
#include "RainmeterDropTarget.h"
#include "ParentMeasure.h"
#include "ChildMeasure.h"

std::vector<ParentMeasure*> g_ParentMeasures;
//Probably not needed as i don't think that the static plugin instance is the same accross different skins, but why not be cautious right :)
std::map<HWND, RainmeterDropTarget*> dropTargets;

PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
	ChildMeasure* child = new ChildMeasure;
	*data = child;
	void* skin = RmGetSkin(rm);
	child->skin = skin;
	child->rm = rm;

	LPCWSTR parentName = RmReadString(rm, L"Parent", L"");
	if (!*parentName)
	{
		HWND wnd = RmGetSkinWindow(rm);
		child->parent = new ParentMeasure;
		child->parent->name = RmGetMeasureName(rm);
		child->parent->skin = skin;
		child->parent->ownerChild = child;
		child->parent->wnd = wnd;
		g_ParentMeasures.push_back(child->parent);

		auto it = dropTargets.find(wnd);
		if (it == dropTargets.end())
		{
			dropTargets.insert(std::pair<HWND, RainmeterDropTarget*>( wnd, new RainmeterDropTarget(wnd)));
		}
		child->parent->children.push_back(child);
		dropTargets[wnd]->AddParent(child->parent);
	}
	else
	{
		std::vector<ParentMeasure*>::const_iterator iter = g_ParentMeasures.begin();
		for (; iter != g_ParentMeasures.end(); ++iter)
		{
			if (_wcsicmp((*iter)->name, parentName) == 0 &&
				(*iter)->skin == skin)
			{
				child->parent = (*iter);
				child->parent->children.push_back(child);
				return;
			}
		}

		RmLog(rm, LOG_ERROR, L"Invalid Parent!");
	}
}

PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
{
	ChildMeasure* child = (ChildMeasure*)data;
	ParentMeasure* parent = child->parent;

	if (!parent)
	{
		return;
	}

	child->measureName = RmGetMeasureName(rm);
	child->dropAction = RmReadString(rm, L"OnDropAction", L"");
	child->enterAction = RmReadString(rm, L"OnEnterAction", L"");
	child->leaveAction = RmReadString(rm, L"OnLeaveAction", L"");
	child->overAction = RmReadString(rm, L"OnOverAction", L"");
	child->filePath = RmReadPath(rm, L"FilePath", L"");

	LPCWSTR action = RmReadString(rm, L"Action", L"");
	if (_wcsicmp(action, L"Move") == 0)				child->dragAction = Action::Move;
	else if (_wcsicmp(action, L"Copy") == 0)		child->dragAction = Action::Copy;
	else if (_wcsicmp(action, L"Delete") == 0)		child->dragAction = Action::Delete;
	else if (_wcsicmp(action, L"Shortcut") == 0)	child->dragAction = Action::Shortcut;
	else if (_wcsicmp(action, L"Path") == 0)		child->dragAction = Action::Path;

	child->replaceIfExists = RmReadInt(rm, L"OverrideExisting", 0) != 0;
	child->silent = RmReadInt(rm, L"Silent", 0) != 0;
	child->spamAction = RmReadInt(rm, L"ProcessAllFiles", 0) != 0;

	if (child->filePath.empty() && (child->dragAction == Action::Move || child->dragAction == Action::Copy))
	{
		RmLog(rm, LOG_WARNING, (std::wstring(L"No path specified for ") + action + L" Please specify the location the Drag&Drop plugin should place the dropped file! Use the FilePath option.").c_str());
		child->enabled = false;
		return;
	}

	if (child->parent && child->parent->ownerChild == child)
	{
		child->bounds.left = INT_MIN / 4;
		child->bounds.top = INT_MIN / 4;
		child->bounds.bottom = INT_MAX;
		child->bounds.right = INT_MAX;
		if (dropTargets.find(child->parent->wnd) != dropTargets.end())
		{
			dropTargets[child->parent->wnd]->SetFancyRenderer(RmReadInt(rm, L"FancyRenderer", 1) != 0);
		}
	}
	else {

		std::wstring bounds = RmReadString(rm, L"Bounds", L"");
		std::vector<std::wstring> tokens = HelperFunctions::CustomTokenize(bounds, L",");
		if (tokens.size() == 1)
		{
			child->meter = bounds;
		}
		else if (tokens.size() == 4 && !(child->parent && child->parent->ownerChild == child))
		{
			double rect[4] = { 0 };
			int tokenId = -1;
			for (auto token : tokens)
			{
				RainmeterMathParser::Parse(token.c_str(), &rect[++tokenId]);
			}
			child->bounds.left = rect[0];
			child->bounds.top = rect[1];
			child->bounds.right = rect[2];
			child->bounds.bottom = rect[3];
			if (rect[2] == 0 || rect[3] == 0) {
				RmLogF(rm, LOG_WARNING, L"%s: Width or Height is 0", RmGetMeasureName(rm));
				child->enabled = false;
				return;
			}
		}
		else
		{
			RmLogF(rm, LOG_ERROR, L"%s: Bounds does not contain either a meter or X,Y,W,H. The amount of tokens is not correct! The current number is %s", RmGetMeasureName(rm), std::to_wstring(tokens.size()).c_str());
		}
	}
	bool disabled = RmReadInt(rm, L"Disabled", 0) != 0;
	child->enabled = !disabled;
}

PLUGIN_EXPORT double Update(void* data)
{
	ChildMeasure* child = (ChildMeasure*)data;
	if (!child->meter.empty() && child->enabled)
	{
		std::wstring bang = L"[!CommandMeasure " + child->measureName + L" \"SetBounds ";
		bang += L"[" + child->meter + L":X] ";
		bang += L"[" + child->meter + L":Y] ";
		bang += L"[" + child->meter + L":W] ";
		bang += L"[" + child->meter + L":H]]\"";
		RmExecute(child->skin, bang.c_str());
	}
	return 0.0;
}

PLUGIN_EXPORT LPCWSTR GetString(void* data)
{
	ChildMeasure* child = (ChildMeasure*)data;
	return child->previousFile.c_str();
}

PLUGIN_EXPORT void ExecuteBang(void* data, LPCWSTR args)
{
	ChildMeasure* child = (ChildMeasure*)data;
	std::vector<std::wstring> tokens = HelperFunctions::CustomTokenize(args, L" ");
	if(tokens.size() > 0 && tokens[0] == L"SetBounds")
	{
		double result;
		RainmeterMathParser::Parse(tokens[1].c_str(), &result);
		child->bounds.left = result;
		RainmeterMathParser::Parse(tokens[2].c_str(), &result);
		child->bounds.top = result;
		RainmeterMathParser::Parse(tokens[3].c_str(), &result);
		child->bounds.right = result;
		RainmeterMathParser::Parse(tokens[4].c_str(), &result);
		child->bounds.bottom = result;
		if (child->bounds.right == 0 || child->bounds.bottom == 0) {
			RmLogF(child->rm, LOG_WARNING, L"%s: Width or Height is 0", RmGetMeasureName(child->rm));
			child->enabled = false;
			return;
		}
	}
	
}

PLUGIN_EXPORT void Finalize(void* data)
{
	ChildMeasure* child = (ChildMeasure*)data;
	ParentMeasure* parent = child->parent;

	if (parent)
	{
		auto it = std::find(parent->children.begin(), parent->children.end(), child);

		if(it != parent->children.end())
			parent->children.erase(it);
		
		if (parent->ownerChild == child) {
			auto it = dropTargets.find(parent->wnd);
			if (it != dropTargets.end())
			{
				dropTargets[parent->wnd]->RemoveParent(parent);
				if (dropTargets[parent->wnd]->ParentCount() == 0) {
					dropTargets[parent->wnd]->Unregister();
					delete it->second;
					dropTargets.erase(parent->wnd);
				}
			}
			delete parent;

			std::vector<ParentMeasure*>::iterator iter = std::find(g_ParentMeasures.begin(), g_ParentMeasures.end(), parent);
			g_ParentMeasures.erase(iter);
		}
	}

	delete child;
}
