// Intel Power Gadget/EnergyLib "\\.\EnergyDriver" handle leak
// Enable "Leak" under "Basics" in Application Verifier and run this in a debugger to see the leaked handle.
// Application Verifier comes with the Debugging Tools for Windows
// Find the Debugging Tools for Windows here: https://msdn.microsoft.com/en-us/library/windows/hardware/ff551063.aspx

#include <string>
#include <Windows.h>
#include <cstdio>

#if _M_X64
PCWSTR const dllName = L"\\EnergyLib64.dll";
#else
PCWSTR const dllName = L"\\EnergyLib32.dll";
#endif

typedef _Return_type_success_(return != 0) int POWER_GADGET_SUCCESS;

typedef POWER_GADGET_SUCCESS(*IntelEnergyLibInitialize_t)();
void printLastError(const DWORD lastErr = ::GetLastError( ))
{
	const DWORD errMsgSize = 1024u;
	wchar_t errBuff[errMsgSize] = {0};
	const DWORD ret = ::FormatMessageW(
		(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS),
		NULL, lastErr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		errBuff, errMsgSize, NULL);

	if (ret == 0)
		std::terminate();//FormatMessageW failed.
	wprintf_s(L"Encountered an error: %s\r\n", errBuff);
}

std::wstring GetEnvironmentVariableString(_In_z_ PCWSTR const variable)
{
	const rsize_t bufferSize = 512u;
	wchar_t buffer[bufferSize] = {0};
	rsize_t sizeRequired = 0u;
	const errno_t result = _wgetenv_s(&sizeRequired, buffer, variable);
	if (result == 0)
		return buffer;
	return L"";
}

HMODULE loadLib( ) {
	const std::wstring IPG_DIR( GetEnvironmentVariableString( L"IPG_Dir" ) );
	const std::wstring EnergyLibDll( IPG_DIR + dllName );
	//wprintf_s( L"Attempting to load DLL `%s`\r\n", EnergyLibDll.c_str( ) );
	HMODULE EnergyLib = ::LoadLibraryW( EnergyLibDll.c_str( ) );
	if ( EnergyLib == NULL ) {
		wprintf_s( L"Failed to load DLL!\r\n" );
		printLastError( );
		abort();
		}
	return EnergyLib;
	}

FARPROC getFuncAddr( HMODULE EnergyLib, _In_z_ PCSTR const funcName ) {
	FARPROC FuncAddr = ::GetProcAddress( EnergyLib, funcName );
	if ( FuncAddr == NULL ) {
		wprintf_s( L"Failed to load find address of %S!\r\n", funcName );
		printLastError( );
		abort();
		}
	return FuncAddr;
	}

IntelEnergyLibInitialize_t initialize( HMODULE EnergyLib ) {
	const IntelEnergyLibInitialize_t IntelEnergyLibInitialize =
		reinterpret_cast<IntelEnergyLibInitialize_t>( getFuncAddr( EnergyLib, "IntelEnergyLibInitialize" ) );

	const POWER_GADGET_SUCCESS Initialized = ( *IntelEnergyLibInitialize )( );
	if ( !Initialized ) {
		wprintf_s( L"IntelEnergyLibInitialize failed!\r\n" );
		abort();
		}
	//wprintf_s( L"IntelEnergyLibInitialize succeeded!\r\n" );
	return IntelEnergyLibInitialize;
	}


void freeLib( _Pre_valid_ _Post_ptr_invalid_ HMODULE lib ) {

	//"\\.\EnergyDriver" is leaked here.
	const BOOL freed = ::FreeLibrary( lib );
	if ( freed == 0 ) {
		printLastError( );
		abort( );
		}
	}


void doEnergyLib( ) {
	const HMODULE EnergyLib = loadLib( );
	initialize( EnergyLib );
	freeLib( EnergyLib );
	}

int main( ) {
	const size_t maxEnergyLib = 10000u;
	for ( size_t i = 0u; i < maxEnergyLib; ++i ) {
		doEnergyLib( );
		}
	}
