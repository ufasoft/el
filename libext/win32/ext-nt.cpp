#ifndef _ELDRV
#	define WIN32_NO_STATUS
#endif

#include <el/ext.h>

#undef WIN32_NO_STATUS
#include <ntstatus.h>

namespace Ext {
using namespace std;

static class Ntos_error_category : public error_category {
	typedef error_category base;

	const char *name() const noexcept override { return "ntos"; }

	string message(int errval) const override {
		return "Unknow error"; //!!!TODO
	}

	error_condition default_error_condition(int errval) const noexcept override {
		errc e;
		switch (errval) {
		case STATUS_NO_MEMORY:				e = errc::not_enough_memory;			break;
		case STATUS_TIMEOUT:				e = errc::timed_out;					break;
		case STATUS_BUFFER_OVERFLOW:		e = errc::no_buffer_space;				break;
		case STATUS_NOT_FOUND:				e = errc::no_such_file_or_directory;	break;
		case STATUS_INVALID_HANDLE:			e = errc::bad_file_descriptor;			break;
		case STATUS_NAME_TOO_LONG:			e = errc::filename_too_long;			break;
		case STATUS_NOT_A_DIRECTORY:		e = errc::not_a_directory;					break;
		case STATUS_DIRECTORY_NOT_EMPTY:	e = errc::directory_not_empty;			break;			
		case STATUS_CONNECTION_DISCONNECTED: e = errc::connection_aborted;			break;
		case STATUS_ADDRESS_ALREADY_EXISTS:	e = errc::address_in_use;				break;
		case STATUS_CONNECTION_REFUSED:		e = errc::connection_refused;			break;
		case STATUS_CONNECTION_RESET:		e = errc::connection_reset;				break;
		case STATUS_NETWORK_UNREACHABLE:	e = errc::network_unreachable;			break;
		case STATUS_HOST_UNREACHABLE:		e = errc::host_unreachable;				break;
		case STATUS_PROTOCOL_NOT_SUPPORTED:	e = errc::protocol_not_supported;		break;
		case STATUS_TOO_MANY_OPENED_FILES:	e = errc::too_many_files_open;			break;
		case STATUS_INVALID_PARAMETER_1:
		case STATUS_INVALID_PARAMETER_2:
		case STATUS_INVALID_PARAMETER_3:
		case STATUS_INVALID_PARAMETER_4:
		case STATUS_INVALID_PARAMETER_5:
		case STATUS_INVALID_PARAMETER_6:
		case STATUS_INVALID_PARAMETER_7:
		case STATUS_INVALID_PARAMETER_8:
		case STATUS_INVALID_PARAMETER_9:
		case STATUS_INVALID_PARAMETER_10:
		case STATUS_INVALID_PARAMETER_11:
		case STATUS_INVALID_PARAMETER_12:
			e = errc::invalid_argument;
			break;
		case STATUS_ACCESS_DENIED:
		case STATUS_ACCESS_VIOLATION:
			e = errc::permission_denied;
			break;
		default:
			return base::default_error_condition(errval);
		}
		return e;
	}

} s_ntosErrorCategory;

const error_category& AFXAPI ntos_category() { return s_ntosErrorCategory; }

NTSTATUS AFXAPI NtCheck(NTSTATUS status, NTSTATUS allowableError) {
	if (status < 0 && status != allowableError)
		Throw(error_code(status, ntos_category()));
	return status;
}

} // Ext::



