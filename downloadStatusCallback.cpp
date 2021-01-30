#include <windows.h>
#include <iostream>
#include <string>
using namespace std;

class DownloadStatusCallback : public IBindStatusCallback {
private:
    bool isFirstOutput = true;
public:
    HRESULT STDMETHODCALLTYPE OnStartBinding(DWORD dwReserved, IBinding *pib) override {
        //cout << "Receiving data -> 0 / 0";
        return 0;
    }

    HRESULT STDMETHODCALLTYPE GetPriority(LONG *pnPriority) override {
        return 0;
    }

    HRESULT STDMETHODCALLTYPE OnLowResource(DWORD reserved) override {
        return 0;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(const IID &riid, void **ppvObject) override {
        return 0;
    }

    ULONG STDMETHODCALLTYPE AddRef(void) override {
        return 0;
    }

    ULONG STDMETHODCALLTYPE Release(void) override {
        return 0;
    }

    HRESULT STDMETHODCALLTYPE OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText) override {
        printf("\rReceiving data -> %lu / %lu", ulProgress, ulProgressMax);
        this->isFirstOutput = false;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnStopBinding(HRESULT hresult, LPCWSTR szError) override {
        return 0;
    }

    HRESULT STDMETHODCALLTYPE GetBindInfo(DWORD *grfBINDF, BINDINFO *pbindinfo) override {
        return 0;
    }

    HRESULT STDMETHODCALLTYPE OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC *pformatetc, STGMEDIUM *pstgmed) override {
        return 0;
    }

    HRESULT STDMETHODCALLTYPE OnObjectAvailable(const IID &riid, IUnknown *punk) override {
        return 0;
    }

};