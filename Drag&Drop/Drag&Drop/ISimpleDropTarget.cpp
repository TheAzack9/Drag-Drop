#include "ISimpleDropTarget.h"

ISimpleDropTarget::ISimpleDropTarget(HWND hwnd)
{
	/* This call might fail, in which case OLE sets m_pdth = NULL. If this happens wont the fancy OLE renderer be used! */
	CoCreateInstance(CLSID_DragDropHelper, NULL, CLSCTX_INPROC_SERVER,
		IID_IDropTargetHelper, (LPVOID*)&m_pdth);
	Register(hwnd);
}

ISimpleDropTarget::~ISimpleDropTarget()
{
	Unregister();
}

bool ISimpleDropTarget::Register(HWND hwnd)
{
	if (IsWindow(m_hwnd))
	{
		Unregister();
	}
	if (!IsWindow(hwnd)) {
		throw L"Tried to register non window for Drag&Drop!";
		return false;
	}
	HRESULT result = OleInitialize(NULL);
	if (SUCCEEDED(result)) {
		result = RegisterDragDrop(hwnd, this);
		if (SUCCEEDED(result)) {
			m_hwnd = hwnd;
			return true;
		}
		else
		{
			RevokeDragDrop(hwnd);
			OleUninitialize();
			throw L"Drag&Drop already attached to window!";
		}
	}
	else
	{
		throw L"Drag&Drop already attached to window!";
		OleUninitialize();
	}
	return false;
}

bool ISimpleDropTarget::Unregister()
{
	RevokeDragDrop(m_hwnd);
	OleUninitialize();
	m_hwnd = NULL;
	return true;
}

void ISimpleDropTarget::SetFancyRenderer(bool use)
{
	if (!dragInAction)
		FancyRender = use;
	PrevFancyRenderer = FancyRender;
}

inline HRESULT __stdcall ISimpleDropTarget::DragEnter(IDataObject * pDataObject, DWORD grfKeyState, POINTL ptl, DWORD * pdwEffect)
{
	POINT pt;
	pt.x = ptl.x;
	pt.y = ptl.y;
	if (m_pdth && PrevFancyRenderer) {              
		m_pdth->DragEnter(m_hwnd, pDataObject, &pt, *pdwEffect);
	}
	m_DataObject = pDataObject;     
	dragInAction = true;
	m_RightData = QueryDataObject(pDataObject);

	bool canDrop = m_RightData && AllowDrop(ptl);
	if (canDrop)
	{
		SetFocus(m_hwnd);
		auto files = GetFiles(pDataObject);
		*pdwEffect = OnDragEnter(files, grfKeyState, ptl);
	}
	else
	{
		*pdwEffect = DROPEFFECT_NONE;
	}
	return S_OK;

}
inline HRESULT __stdcall ISimpleDropTarget::DragOver(DWORD grfKeyState, POINTL ptl, DWORD * pdwEffect)
{
	POINT pt;
	pt.x = ptl.x;
	pt.y = ptl.y;
	if (m_pdth && PrevFancyRenderer) {              
		m_pdth->DragOver(&pt, *pdwEffect);
	}

	bool canDrop = m_RightData && AllowDrop(ptl);
	if (canDrop) {
		*pdwEffect = OnDragOver(grfKeyState, ptl);
	}

	if (m_DataObject && m_pdth) {
		m_pdth->Show(canDrop);
	}

	if (!canDrop)
	{
		*pdwEffect = DROPEFFECT_NONE;
	}

	return S_OK;

}
inline HRESULT __stdcall ISimpleDropTarget::DragLeave(void)
{
	if (m_pdth) {              
		m_pdth->DragLeave();
	}
	m_DataObject = NULL; 
	dragInAction = false;
	OnDragLeave();
	PrevFancyRenderer = FancyRender;
	return S_OK;
}
inline HRESULT __stdcall ISimpleDropTarget::Drop(IDataObject * pDataObject, DWORD grfKeyState, POINTL ptl, DWORD * pdwEffect)
{
	POINT pt;
	pt.x = ptl.x;
	pt.y = ptl.y;
	if (m_pdth) {
		m_pdth->Drop(pDataObject, &pt, *pdwEffect);
	}

	m_DataObject = NULL; 
	dragInAction = false;
	bool canDrop = m_RightData && AllowDrop(ptl);
	if (canDrop)
	{
		auto files = GetFiles(pDataObject);
		OnDragDrop(files, grfKeyState, ptl);
	}
	else
	{
		*pdwEffect = DROPEFFECT_NONE;
	}
	return S_OK;

}

std::vector<std::wstring> ISimpleDropTarget::GetFiles(IDataObject * pDataObject)
{
	FORMATETC fmtetc = { CF_HDROP, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	STGMEDIUM med;
	pDataObject->GetData(&fmtetc, &med);
	HDROP hdrop = reinterpret_cast<HDROP>(med.hGlobal);
	UINT cFiles = DragQueryFile(hdrop, 0xFFFFFFFF, NULL, 0);
	std::vector<std::wstring> files;
	for (UINT i = 0; i < cFiles; i++)
	{
		WCHAR szFile[MAX_PATH];
		UINT cch = DragQueryFile(hdrop, i, szFile, MAX_PATH);
		if (cch > 0 && cch < MAX_PATH) {
			files.push_back(szFile);
		}
	}
	ReleaseStgMedium(&med);
	return files;
}