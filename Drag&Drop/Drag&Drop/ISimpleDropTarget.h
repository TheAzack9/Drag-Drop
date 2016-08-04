#ifndef ISIMPLEDROPTARGET
#define ISIMPLEDROPTARGET

#include <Shlobj.h>
#include <functional>
#include <atlbase.h>
#include <vector>
#include <string>

//the different DropEffects that exists
enum DropEffect {
	None = 0,
	Copy = 1,
	Move = 2,
	Link = 4,
	Scroll = 0x80000000
};

class ISimpleDropTarget : public IDropTarget, public IDropTargetHelper
{
public:
	ISimpleDropTarget(HWND hwnd);
	~ISimpleDropTarget();

	//Used to register the window as a drop target
	bool Register(HWND hwnd);
	//Used to unregister the window as a drop target
	bool Unregister();

	void SetFancyRenderer(bool use);
	
protected:
	//Called when drop item enters the window
	virtual DropEffect OnDragEnter(std::vector<std::wstring> files, DWORD grfKeyState, const POINTL& mousePos) = 0;
	//Called when drop item is over the window
	virtual DropEffect OnDragOver(DWORD grfKeyState, const POINTL& mousePos) = 0;
	//Called when drop item is dropped inside the window, this will only return the file name that drops. If you want additional support you'll need to do it with the DataObject handle.
	virtual DropEffect OnDragDrop(std::vector<std::wstring> files, DWORD grfKeyState, const POINTL& mousePos) = 0;
	//Called when drop item leaves the window
	virtual void OnDragLeave(void) = 0;
	//Called to check if the file is allowed to drop
	virtual bool AllowDrop(const POINTL& pt) = 0;

	//Override this to create custom conditions for the FORMATETC
	virtual bool  QueryDataObject(IDataObject *pDataObject)
	{
		FORMATETC fmtetc = { CF_HDROP, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
		// does the data object support CF_HDROP using a HGLOBAL?
		return pDataObject->QueryGetData(&fmtetc) == S_OK ? true : false;
	}
	//Getting the pure IDataObjec, will be NULL if no Drag Drop is currently happening on the window. Will only not be NULL if the file is above the designated HWND!
	CComPtr<IDataObject> DataObject() const { return m_DataObject; }
	// Is true if the data in the current dragged file is of the right type defined in QueryDataObject
	bool CorrectDataType() const { return m_RightData; }
private:

	// The window handle to the window currently set to accept Drag & Drop
	HWND m_hwnd;
	// Number of references to this class, needed due to the IUnknown interface
	long m_lRefCount;
	// Is true if the data in the current dragged file is of the right type defined in QueryDataObject
	bool m_RightData;
	//Getting the pure IDataObjec, will be NULL if no Drag Drop is currently happening on the window. Will only not be NULL if the file is above the designated HWND!
	CComPtr<IDataObject> m_DataObject;

	//Determines if it should use the OLE renderer instead of windows' basic renderer. This will render a preview of the item besides the mouse!
	bool FancyRender = true;
	//Determines if it should use the OLE renderer instead of windows' basic renderer. This will render a preview of the item besides the mouse!
	bool PrevFancyRenderer = true;
	//True if there is currently a drag happening
	bool dragInAction = false;

	//Gets the file name from a dataobject
	std::vector<std::wstring> GetFiles(IDataObject* pDataObject);

	// Private functions
	HRESULT __stdcall DragEnter(IDataObject * pDataObject, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect) override;
	HRESULT __stdcall DragOver(DWORD grfKeyState, POINTL pt, DWORD * pdwEffect) override;
	HRESULT __stdcall DragLeave(void) override;
	HRESULT __stdcall Drop(IDataObject * pDataObject, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect) override;


	CComPtr<IDropTargetHelper> m_pdth;		/* Used for hack explained bellow */

	// Inherited via IDropTargetHelper
	// Used to trick windows into displaying a preview icon of the file. Nasty hack, but the best i could find :)
	HRESULT STDMETHODCALLTYPE DragEnter(HWND hwndTarget, IDataObject * pDataObject, POINT * ppt, DWORD dwEffect) override
	{
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE DragOver(POINT * ppt, DWORD dwEffect) override
	{
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE Drop(IDataObject * pDataObject, POINT * ppt, DWORD dwEffect) override
	{
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE Show(BOOL fShow) override
	{
		return E_NOTIMPL;
	}

	// IUnknown implementation
	HRESULT __stdcall QueryInterface(REFIID iid, void ** ppvObject)
	{
		return S_OK;
	}
	ULONG   __stdcall AddRef(void)
	{
		m_lRefCount++;
		return m_lRefCount;
	}
	ULONG   __stdcall Release(void)
	{
		m_lRefCount--;
		return m_lRefCount;
	}
};

#endif