/** @file VersionInfoEx.h
 * OleCallback interface (struct) to handle Richedit OLE functions.
 *
 * Original idea extracted from simple example here:
 * https://stackoverflow.com/questions/69767789/richedit-doesnt-show-pictures
 * Modified to use newer StgCreateStorageEx instead of old StgCreateDocfile.
 **/

#pragma once

static int g_iNumStorages = 0;

interface OleCallback : public IRichEditOleCallback
{
public:
    IStorage* pstorage;
    DWORD m_ref;
    int grfmode;

    OleCallback() : pstorage(nullptr), m_ref(0), grfmode(STGM_TRANSACTED | STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE)
    {
        CreateStorage();
    }

    HRESULT STDMETHODCALLTYPE GetNewStorage(LPSTORAGE* lplpstg)
    {
        if (!pstorage)
            CreateStorage();

        wchar_t name[256] = { 0 };
        g_iNumStorages++;
        swprintf(name, 256, L"NWScript-REOLEStorage%d", g_iNumStorages);

        HRESULT pResult = pstorage->CreateStorage(name, grfmode, 0, 0, lplpstg);
        OutputDebugString((L"AboutBox: pstorage->CreateStorage result is: " + std::to_wstring(pResult)).c_str());
        return pResult;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** lplpObj) 
    {
        *lplpObj = NULL;
        if (iid == IID_IUnknown || iid == IID_IRichEditOleCallback)
        {
            *lplpObj = this;
            AddRef();
            return NOERROR;
        }
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return ++m_ref;
    }

    ULONG STDMETHODCALLTYPE Release()
    {
        return --m_ref;
    }

    STDMETHOD(GetInPlaceContext) (LPOLEINPLACEFRAME FAR*, LPOLEINPLACEUIWINDOW FAR*, LPOLEINPLACEFRAMEINFO) { return S_OK; }
    STDMETHOD(ShowContainerUI) (BOOL) { return S_OK; }
    STDMETHOD(QueryInsertObject) (LPCLSID, LPSTORAGE, LONG) { return S_OK; }
    STDMETHOD(DeleteObject) (LPOLEOBJECT) { return S_OK; }
    STDMETHOD(QueryAcceptData) (LPDATAOBJECT, CLIPFORMAT FAR*, DWORD, BOOL, HGLOBAL) { return S_OK; }
    STDMETHOD(ContextSensitiveHelp) (BOOL) { return S_OK; }
    STDMETHOD(GetClipboardData) (CHARRANGE FAR*, DWORD, LPDATAOBJECT FAR*) { return S_OK; }
    STDMETHOD(GetDragDropEffect) (BOOL, DWORD, LPDWORD) { return S_OK; }
    STDMETHOD(GetContextMenu) (WORD, LPOLEOBJECT, CHARRANGE FAR*, HMENU FAR*) { return S_OK; }

private:
    void CreateStorage() {
        pstorage = nullptr;
        m_ref = 0;

        HRESULT hSuccess = StgCreateStorageEx(NULL, grfmode | STGM_DELETEONRELEASE, STGFMT_STORAGE, 0, 0, NULL,
            IID_IStorage, reinterpret_cast<void**>(&pstorage));
        OutputDebugString((L"AboutBox: StgCreateStorageEx result is: " + std::to_wstring(hSuccess)).c_str());
    }
};


