#pragma once
#include <metahost.h>
#include <unknwn.h>

#define FOO_GUID "A15DDC0D-53EF-4776-8DA2-E87399C6654D"

struct __declspec(uuid(FOO_GUID)) _FooInterface;

struct _FooInterface : IUnknown
{
	virtual
		HRESULT __stdcall HelloWorld(LPWSTR name, LPWSTR* result) = 0;
};

class SampleHostControl : IHostControl
{
public:
	SampleHostControl()
	{
		m_refCount = 0;
		m_defaultDomainManager = NULL;
	}

	virtual ~SampleHostControl()
	{
		if (m_defaultDomainManager != NULL)
		{
			m_defaultDomainManager->Release();
		}
	}

	HRESULT __stdcall SampleHostControl::GetHostManager(REFIID id, void **ppHostManager)
	{
		*ppHostManager = NULL;
		return E_NOINTERFACE;
	}

	HRESULT __stdcall SampleHostControl::SetAppDomainManager(DWORD dwAppDomainID, IUnknown *pUnkAppDomainManager)
	{
		HRESULT hr = S_OK;
		hr = pUnkAppDomainManager->QueryInterface(__uuidof(_FooInterface), (PVOID*)&m_defaultDomainManager);
		return hr;
	}

	_FooInterface* GetFooInterface()
	{
		if (m_defaultDomainManager)
		{
			m_defaultDomainManager->AddRef();
		}
		return m_defaultDomainManager;
	}

	HRESULT __stdcall QueryInterface(const IID &iid, void **ppv)
	{
		if (!ppv) return E_POINTER;
		*ppv = this;
		AddRef();
		return S_OK;
	}

	ULONG __stdcall AddRef()
	{
		return InterlockedIncrement(&m_refCount);
	}

	ULONG __stdcall Release()
	{
		if (InterlockedDecrement(&m_refCount) == 0)
		{
			delete this;
			return 0;
		}
		return m_refCount;
	}

private:
	long m_refCount;
	_FooInterface* m_defaultDomainManager;
};
