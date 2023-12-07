#pragma once
#include <atlcomcli.h>
#include <atlsafe.h>
#include <WTypesbase.h>
//
namespace ibom {

	// wstring to BSTR wrapper
	inline CComBSTR ToBstr(const std::wstring& s)
	{
		if (s.empty())
		{
			return CComBSTR();
		}
		return CComBSTR(static_cast<int>(s.size()), s.data());
	}

#define MAX_GUIDS 255

	class IBom : public IDataObject {

	public:

		IBom() {
			//..
		}

		~IBom() {
			//..
		}

		static void GetDecoClassName(std::wstring& out) {

			CComBSTR cbstr = ibom::IBom::get_deco_class_name();
			out = cbstr.Detach();
		}

		static size_t GetVer() {
			return 1;
		}

		static bool isBookmarkDeco(IDataObject* obj) {

			bool bres = false;

			//L"vbookmark data object"
			std::wstring mydeco;
			ibom::IBom::GetDecoClassName(mydeco);

			pfc::com_ptr_t <IEnumFORMATETC> pEnumFORMATETC;
			obj->EnumFormatEtc(DATADIR_GET, pEnumFORMATETC.receive_ptr());

			FORMATETC fe;
			memset(&fe, 0, sizeof(fe));
			while (S_OK == pEnumFORMATETC->Next(1, &fe, NULL))
			{

				if (fe.cfFormat == CF_TEXT) {
					UINT cb = static_cast<UINT>(sizeof(mydeco[0]) * (mydeco.length() + 1)); //44
					STGMEDIUM stgm;
					memset(&stgm, 0, sizeof(stgm));
					obj->GetData(&fe, &stgm);

					bool bcheck = false;
					char* data = (char*)GlobalLock(stgm.hGlobal);

					if (data) {
						pfc::stringcvt::string_utf8_from_wide to_str(mydeco.c_str());
						bres = !_stricmp(data, to_str.get_ptr());
					}

					GlobalUnlock(stgm.hGlobal);
					ReleaseStgMedium(&stgm);

					if (bres) {
						//found
						break;
					}
				}
			}

			pEnumFORMATETC.release();

			return bres;
		}

		void SetPlaylistGuids(std::vector<std::wstring> v) {
			SetArray(v);
		}

		LPSAFEARRAY GetPlaylistGuids() {
			return GetArray();
		}

		// Inherited via IDataObject
		virtual HRESULT __stdcall QueryInterface(REFIID riid, void** ppvObject) override;

		virtual ULONG __stdcall AddRef(void) override;

		virtual ULONG __stdcall Release(void) override;

		virtual HRESULT __stdcall GetData(FORMATETC* pformatetcIn, STGMEDIUM* pmedium) override;

		virtual HRESULT __stdcall GetDataHere(FORMATETC* pformatetc, STGMEDIUM* pmedium) override;

		virtual HRESULT __stdcall QueryGetData(FORMATETC* pformatetc) override;

		virtual HRESULT __stdcall GetCanonicalFormatEtc(FORMATETC* pformatectIn, FORMATETC* pformatetcOut) override;

		virtual HRESULT __stdcall SetData(FORMATETC* pformatetc, STGMEDIUM* pmedium, BOOL fRelease) override;

		virtual HRESULT __stdcall EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC** ppenumFormatEtc) override;

		virtual HRESULT __stdcall DAdvise(FORMATETC* pformatetc, DWORD advf, IAdviseSink* pAdvSink, DWORD* pdwConnection) override;

		virtual HRESULT __stdcall DUnadvise(DWORD dwConnection) override;

		virtual HRESULT __stdcall EnumDAdvise(IEnumSTATDATA** ppenumAdvise) override;

	private:

		static const BSTR get_deco_class_name() {
			return L"vbookmark data object";
		}

		void SetArray(std::vector<std::wstring> v) {

			m_data.m_psa = nullptr;
			
			int cv = static_cast<int>(v.size());

			SAFEARRAYBOUND bounds[1];

			bounds[0].cElements = 0;
			bounds[0].lLbound = 0;

			for (LONG i = 0; i < MAX_GUIDS && i < v.size(); i++)
			{
				CComBSTR bstr = ToBstr(v[i]);
				HRESULT hr;

				hr = m_data.Add(bstr);

				if (FAILED(hr))
				{
					AtlThrow(hr);
				}
			}
		}

		LPSAFEARRAY GetArray() {
			return m_data.Detach();
		}

	private:
		CComSafeArray<BSTR> m_data;
		pfc::com_ptr_t<IDataObject> m_cp;
	};

}
