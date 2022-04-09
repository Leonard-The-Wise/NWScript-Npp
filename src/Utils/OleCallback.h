/** @file VersionInfoEx.h
 * OleCallback interface (struct) to handle Richedit OLE functions.
 *
 * Original idea extracted from simple example here:
 * https://stackoverflow.com/questions/69767789/richedit-doesnt-show-pictures
 * Modified to use newer StgCreateStorageEx instead of old StgCreateDocfile.
 **/

#pragma once

interface OleCallback : public IRichEditOleCallback
{
public:
    IStorage* pstorage;
    DWORD m_ref;
    int grfmode;
    OleCallback() : grfmode(STGM_TRANSACTED | STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE)
    {
        pstorage = nullptr;
        m_ref = 0;        
        std::ignore = StgCreateStorageEx(NULL, grfmode | STGM_DELETEONRELEASE, STGFMT_STORAGE, 0, 0, NULL, 
            IID_IStorage, reinterpret_cast<void**>(&pstorage));
    }

    HRESULT STDMETHODCALLTYPE GetNewStorage(LPSTORAGE* lplpstg)
    {
        wchar_t name[256] = { 0 };
        HRESULT pResult = pstorage->CreateStorage(name, grfmode, 0, 0, lplpstg);
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
};


