#include "RainmeterDropTarget.h"
#include "ParentMeasure.h"

RainmeterDropTarget::RainmeterDropTarget() : ISimpleDropTarget(nullptr)
{
	RmLog(LOG_ERROR, L"Droptarget initialized without wnd!");
}

RainmeterDropTarget::RainmeterDropTarget(HWND wnd) : ISimpleDropTarget(wnd)
{
}

RainmeterDropTarget::~RainmeterDropTarget()
{
}

void RainmeterDropTarget::AddParent(ParentMeasure* parent)
{
	registeredParents.push_back(parent);
}

void RainmeterDropTarget::RemoveParent(ParentMeasure * parent)
{
		auto it = std::find(registeredParents.begin(), registeredParents.end(), parent);
		if(it != registeredParents.end())
		registeredParents.erase(it);
}

int RainmeterDropTarget::ParentCount()
{
	return registeredParents.size();
}

DropEffect RainmeterDropTarget::OnDragEnter(std::vector<std::wstring> files, DWORD grfKeyState, const POINTL & mousePos)
{
	for (auto parent : registeredParents)
	{
		for (auto child : parent->children)
		{
			if (!child->enabled)
				continue;
			if (child->ContainsPointL(mousePos) && !child->dropActive && !child->enterAction.empty())
			{
				child->dropActive = true;
				int number = 1;
				if (child->spamAction) 
					for (auto file : files) {
						ExecuteDragBang(child->enterAction, child, file, mousePos, number);
						++number;
				}
				else
					ExecuteDragBang(child->enterAction, child, files.front(), mousePos, number);
			}
		}
	}
	prevFiles = files;
	return GetDropEffect();
}

DropEffect RainmeterDropTarget::OnDragOver(DWORD grfKeyState, const POINTL & mousePos)
{
	for (auto parent : registeredParents)
	{
		for (auto child : parent->children)
		{
			if (!child->enabled)
				continue;
			if (child->ContainsPointL(mousePos)) {
				if (!child->dropActive && !child->enterAction.empty())
				{
					child->dropActive = true;
					int number = 1;
					if (child->spamAction)
						for (auto file : prevFiles) {
							ExecuteDragBang(child->enterAction, child, file, mousePos, number);
							++number;
						}
					else
						ExecuteDragBang(child->enterAction, child, prevFiles.front(), mousePos, number);

				}
				if (!child->overAction.empty())
				{
					child->dropActive = true;
					int number = 1;
					if (child->spamAction)
						for (auto file : prevFiles) {
							ExecuteDragBang(child->overAction, child, file, mousePos, number);
							++number;
						}
					else
						ExecuteDragBang(child->overAction, child, prevFiles.front(), mousePos, number);

				}
			}
			if (!child->ContainsPointL(mousePos) && child->dropActive)
			{
				child->dropActive = false;
				int number = 1;
				if (!child->leaveAction.empty()) {
					if (child->spamAction)
						for (auto file : prevFiles) {
							ExecuteDragBang(child->leaveAction, child, file, mousePos, number);
							++number;
						}
					else
						ExecuteDragBang(child->leaveAction, child, prevFiles.front(), mousePos, number);
				}

			}
		}
	}
	return GetDropEffect();
}

DropEffect RainmeterDropTarget::OnDragDrop(std::vector<std::wstring> files, DWORD grfKeyState, const POINTL & mousePos)
{
	for (auto parent : registeredParents)
	{
		for (auto child : parent->children)
		{
			if (!child->enabled || !child->ContainsPointL(mousePos))
				continue;
			child->dropActive = false;
			int number = 1;
			if (child->spamAction)
				for (auto file : files) {
					ExecuteDragBang(child->dropAction, child, file, mousePos, number, true);
					++number;
				}
			else
				ExecuteDragBang(child->dropAction, child, files.front(), mousePos, number, true);
			
		}
	}
	prevFiles = files;
	return GetDropEffect();
}

void RainmeterDropTarget::OnDragLeave(void)
{
	for (auto parent : registeredParents)
	{
		for (auto child : parent->children)
		{
			if (!child->enabled)
				continue;
			child->dropActive = false;
			int number = 1;
			if (!child->leaveAction.empty()) {
				POINTL mousePos = { mousePos.x = INT_MIN, mousePos.y = INT_MIN };
				if (child->spamAction)
					for (auto file : prevFiles) {
						ExecuteDragBang(child->leaveAction, child, file, mousePos, number);
						++number;
					}
				else
					ExecuteDragBang(child->leaveAction, child, prevFiles.front(), mousePos, number);
			}
		}
	}
}

bool RainmeterDropTarget::AllowDrop(const POINTL & pt)
{
	for (auto parent : registeredParents)
	{
		for (auto child : parent->children)
		{
			if (child->enabled && child->ContainsPointL(pt)) {
				return true;
			}
		}
	}
	return false;
}

void RainmeterDropTarget::ExecuteDragBang(std::wstring bang, ChildMeasure* child, std::wstring file, const POINTL & mousePos, int number, bool isDrop)
{
	std::wstring FileType = L"";
	std::wstring FileName = L"";
	std::wstring fileDirectory = L"";

	//Split it all up
	std::vector<std::wstring> path = HelperFunctions::wstringSplit(file, L'\\');
	std::vector<std::wstring> splitFile = HelperFunctions::wstringSplit(path[path.size() - 1], L'.');

	//Is this a folder?
	struct _stat64i32 s;
	DWORD ftyp = GetFileAttributesW(file.c_str());
	if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
	{
		FileName = path[path.size() - 1];
		FileType = L"folder";
	}
	else if (ftyp == INVALID_FILE_ATTRIBUTES)
	{
		//Hey, this file doesn't have any attibutes O.o
		FileName = path[path.size() - 1];
	}
	else
	{
		if (splitFile.size() > 1)
			FileType = splitFile[splitFile.size() - 1];
		for (int i = 0; i < splitFile.size() - 1; i++) {
			FileName += splitFile[i];
			if (i != splitFile.size() - 2)
				FileName += +L".";
		}
	}

	//stich all together :3
	for (int i = 0; i < path.size() - 2; i++)
		fileDirectory += path[i] + (i == path.size() - 3 ? L"" : L"\\");

	//Create the end location of the file
	std::wstring EndFile = child->filePath + L"\\" + FileName;
	if (!FileType.empty() && FileType != L"folder")
		EndFile += L"." + FileType;

	if(isDrop) {
		switch (child->dragAction)
		{
		case Action::Copy:
			if (SHCreateDirectoryEx(NULL, child->filePath.c_str(), NULL) == ERROR_SUCCESS || GetLastError() == ERROR_ALREADY_EXISTS) {
				if (FileType != L"folder")
					CopyFile(file.c_str(), EndFile.c_str(), !child->replaceIfExists);
				else
					HelperFunctions::CopyDirectory(file.c_str(), EndFile.c_str(), child->replaceIfExists);
			}
			else {
				RmLogF(child->rm, LOG_ERROR, L"Could not create directory: %s", child->filePath.c_str());
			}
			break;
		case Action::Move:
			if (SHCreateDirectoryEx(NULL, child->filePath.c_str(), NULL) == ERROR_SUCCESS || GetLastError() == ERROR_ALREADY_EXISTS) {
				MoveFile(file.c_str(), EndFile.c_str());
				
			}
			else {
				RmLogF(child->rm, LOG_ERROR, L"Could not create directory: %s", child->filePath.c_str());
			}
			break;
		case Action::Delete:
			SHFILEOPSTRUCT shFileStruct;
			memset(&shFileStruct, 0, sizeof(shFileStruct));
			shFileStruct.hwnd = NULL;
			shFileStruct.wFunc = FO_DELETE;
			shFileStruct.fFlags = FOF_ALLOWUNDO;
			if (child->silent)
				shFileStruct.fFlags |= FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI;
			shFileStruct.pFrom = file.c_str();

			SHFileOperation(&shFileStruct);
			break;
		case Action::Shortcut:
			IShellLink* pShellLink = NULL;
			HRESULT hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_ALL, IID_IShellLink, (void**)&pShellLink);
			if (SUCCEEDED(hres))
			{
				pShellLink->SetPath(file.c_str());  // Path to the object we are referring to
				pShellLink->SetDescription(L"This is a automatically generated shortcut by the Drag&Drop plugin for Rainmeter");

				IPersistFile *pPersistFile;
				hres = pShellLink->QueryInterface(IID_IPersistFile, (void**)&pPersistFile);

				if (SUCCEEDED(hres))
				{
					hres = pPersistFile->Save(EndFile.c_str(), TRUE);
					pPersistFile->Release();
				}
				else
					RmLogF(child->rm, LOG_ERROR, L"Could not create shortcut: %s", child->filePath.c_str());
				pShellLink->Release();
			}
			else
				RmLogF(child->rm, LOG_ERROR, L"Could not create shortcut: %s", child->filePath.c_str());
			break;
		}
	}

	// Substitute the different tokens
	std::wstring dragDropBang = HelperFunctions::wstringReplace(bang, L"$Name$", FileName);
	dragDropBang = HelperFunctions::wstringReplace(dragDropBang, L"$FileName$", FileName);
	dragDropBang = HelperFunctions::wstringReplace(dragDropBang, L"$Type$", FileType);
	dragDropBang = HelperFunctions::wstringReplace(dragDropBang, L"$FileType$", FileType);
	dragDropBang = HelperFunctions::wstringReplace(dragDropBang, L"$Directory$", fileDirectory);
	dragDropBang = HelperFunctions::wstringReplace(dragDropBang, L"$FileDirectory$", fileDirectory);
	dragDropBang = HelperFunctions::wstringReplace(dragDropBang, L"$File$", file);
	dragDropBang = HelperFunctions::wstringReplace(dragDropBang, L"$FilePath$", file);
	dragDropBang = HelperFunctions::wstringReplace(dragDropBang, L"$FileNr$", std::to_wstring(number));
	dragDropBang = HelperFunctions::wstringReplace(dragDropBang, L"$MouseX$", std::to_wstring(mousePos.x));
	dragDropBang = HelperFunctions::wstringReplace(dragDropBang, L"$MouseY$", std::to_wstring(mousePos.y));

	if (isDrop)
		child->previousFile = file;
	//Finally execute the bang... puh, that was some really hard work 
	RmExecute(child->skin, dragDropBang.c_str());
}

DropEffect RainmeterDropTarget::GetDropEffect()
{
	DropEffect effect = DropEffect::None;
	for (auto parent : registeredParents)
	{
		for (auto child : parent->children)
		{
			if (!child->enabled)
				continue;
			if (child->dragAction == Action::Copy || child->dragAction == Action::Shortcut)
				effect = DropEffect::Copy;
			if (child->dragAction == Action::Move || child->dragAction == Action::Delete)
				effect = DropEffect::Move;
			else if (child->dragAction == Action::Shortcut || child->dragAction == Action::Path)
				effect = DropEffect::Link;
		}
	}
	return effect;
}
