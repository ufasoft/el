/*######   Copyright (c) 2011-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>


namespace Ext {
using namespace std;

static const CodeMessage<ExtErr> s_extMessageTable[] {
	{ ExtErr::EndOfStream										, "End Of Stream"								}
	, { ExtErr::VartypeNotSupported								, "VarType not supported"						}
	, { ExtErr::InvalidDimCount									, "Invalid dim count"							}
	, { ExtErr::BlobNotInitialized								, "Blob not initialized"						}
	, { ExtErr::Compress										, "Compress error"								}
	, { ExtErr::Registry										, "Registry error"								}
	, { ExtErr::IndexOutOfRange									, "Index out of range"							}
	, { ExtErr::InvalidFlags									, "Invalid flags"								}
	, { ExtErr::IncorrectVariant								, "Incorrect variant"							}
	, { ExtErr::ItemNotFound									, "Item not found"								}
	, { ExtErr::DCOMnotInstalled								, "DCOM not installed"							}
	, { ExtErr::VariantMustBeArray								, "Variant must be array"						}
	, { ExtErr::InvalidTypeForThisOperation						, "Invalid type for this operation"				}
	, { ExtErr::QueueOverflow									, "Overflow of queue"							}
	, { ExtErr::NoMoreServiceHandlers							, "No more Service handlers"					}
	, { ExtErr::AlreadyOpened									, "Object already Opened"						}
	, { ExtErr::InvalidRegistrationCode							, "Invalid Registration Code"					}
	, { ExtErr::UnknownVersionRegcode							, "Unknown version of Regcode"					}
	, { ExtErr::DuplicateName									, "Duplicate Name"								}
	, { ExtErr::InvalidValue									, "Invalid Value"								}
	, { ExtErr::ThreadInterrupted								, "Thread Interupted"							}
	, { ExtErr::InvalidFileHeader								, "Invalid File Header"							}
	, { ExtErr::IncompatibleFileVersion							, "Incompatible File Version"					}
	, { ExtErr::InvalidSignature								, "Invalid Signature"							}
	, { ExtErr::InterfaceAlreadyAssigned						, "Interface already assigned"					}
	, { ExtErr::InFindHandler									, "Exception In FindHandlerForForeignException"	}
	, { ExtErr::FindHandler										, "Exception In FindHandler"					}
	, { ExtErr::InvalidExceptionMagicNumber						, "Invalid Exception Magic number"				}
	, { ExtErr::InvalidResultCallHandler						, "Invalid result of EXC_CallHandler"			}
	, { ExtErr::ControlNotFound									, "Control not found"							}
	, { ExtErr::ClientSizeNULL									, "ClientSize is NULL"							}
	, { ExtErr::DialogTemplate									, "Error in CDialogTemplate constructor"		}
	, { ExtErr::DispatchIsNull									, "Dispatch Is NULL"							}
	, { ExtErr::BlobIsNotString									, "Blob is not string"							}
	, { ExtErr::ControlHWNDIsNULL								, "Control HWND is NULL"						}
	, { ExtErr::UnknownHostAddressType							, "Unknown host address type"					}
	, { ExtErr::TooltipNotFound									, "Tooltip not found"							}
	, { ExtErr::LoadXML											, "Cannot load XML"								}
	, { ExtErr::XMLError										, "XML Error"									}
	, { ExtErr::NoInputStream									, "No Input Stream"								}
	, { ExtErr::NoOutputStream									, "No Output stream"							}
	, { ExtErr::InvalidExtendOfNumber							, "Invalid extend of number"					}
	, { ExtErr::UnknownStructuredStorageElementType				, "Unknown Structured Storage element type"		}
	, { ExtErr::UnknownTypeOfVariant							, "Unknown type of variant"						}
	, { ExtErr::DifferentTypeOfVariant							, "Different type of Variant"					}
	, { ExtErr::FontNotFound									, "Font not found"								}
	, { ExtErr::NoAdviseHolder									, "No Advise Holder"							}
	, { ExtErr::OleControlError									, "OLE Control Error"							}
	, { ExtErr::UnknownWin32Error								, "Unknown Win32 Error"							}
	, { ExtErr::VarTypeInNotStringCompatible					, "Variant type is not string-compatible"		}
	, { ExtErr::UnknownSocketsError								, "Unknown Sockets error"						}
	, { ExtErr::InvalidUTF8String								, "Invalid UTF-8 string"						}
	, { ExtErr::UnsupportedVariantType							, "Unsupported variant type"					}
	, { ExtErr::NoControlWithThisID								, "No control with this ID"						}
	, { ExtErr::NameOfApplicationKeyIsEmpty						, "Name of application key is empty"			}
	, { ExtErr::AssertFailedLine								, "Error AssertFailedLine"						}
	, { ExtErr::Trace											, "Error Trace"									}
	, { ExtErr::AssertValidObject								, "Error AssertValidObject"						}
	, { ExtErr::InvalidSplitType								, "Invalid Split type"							}
	, { ExtErr::BlackRedTree									, "Error in black-red tree"						}
	, { ExtErr::QueueUserAPC									, "QueueUserAPC Error"							}
	, { ExtErr::SignalBreak										, "Signal BREAK"								}
	, { ExtErr::NormalExit										, "Normal Exit"									}
	, { ExtErr::BigNumConversion								, "BigNum conversion error"						}
	, { ExtErr::DivideByZero									, "Divide by zero"								}
	, { ExtErr::InvalidInteger									, "Invalid format of Integer"					}
	, { ExtErr::InvalidCast										, "Invalid Cast"								}
	, { ExtErr::MaxTries										, "Maximum Tries Failed"						}
	, { ExtErr::NonEmptyPointer									, "Non-empty pointer"							}
	, { ExtErr::ObjectNotInitialized							, "Object not initialized"						}
	, { ExtErr::ObjectDisposed									, "Object disposed"								}
	, { ExtErr::Dynamic_Library									, "Dynamic Library error"						}
	, { ExtErr::CodeNotReachable								, "Code Not Reachable"							}
	, { ExtErr::Imaging											, "Imaging Error"								}
	, { ExtErr::Arithmetic										, "Aritmetic error"								}
	, { ExtErr::Overflow										, "Overlow error"								}
	, { ExtErr::EncodingNotSupported							, "Encoding not supported"						}
	, { ExtErr::Protocol_Violation								, "Protocol Violation"							}
	, { ExtErr::New_Protocol_Version							, "New Unsupported Protocol Verion used"		}
	, { ExtErr::FileFormat										, "Input File Format violated"					}
	, { ExtErr::ReadTapeSync									, "Synchronization error during reading of Tape"	}
	, { ExtErr::CrashRpt										, "CrashRpt error"								}
	, { ExtErr::RecursionTooDeep								, "Recusion Too Deep"							}
	, { ExtErr::Map												, "Error in MapFile"		 }
	, { ExtErr::DB_NoRecord										, "No record found for scalar DB query"		 }
	, { ExtErr::DB_DupKey										, "Cannot insert Duplicate Key into Database"		 }
	, { ExtErr::DB_Corrupt										, "Database corruption"		 }
	, { ExtErr::DB_Version										, "Database created with newer version of software"		 }
	, { ExtErr::DB_InternalError								, "Internal Error of DB engine"		 }
	, { ExtErr::JSON_Parse										, "JSON Parse error"		 }
	, { ExtErr::JSON_RPC_ParseError								, "JSON-RPC Parse Error"		 }
	, { ExtErr::JSON_RPC_Internal								, "JSON-RPC Internal Error"		 }
	, { ExtErr::JSON_RPC_InvalidParams							, "JSON-RPC Invalid method parameter(s)"		 }
	, { ExtErr::JSON_RPC_MethodNotFound							, "JSON-RPC Method not found"		 }
	, { ExtErr::JSON_RPC_IsNotRequest							, "The JSON sent is not a valid Request object"		 }
	, { ExtErr::OBJECTCODE_DATACHANGED							, "OBJECTCODE_DATACHANGED Advice"		 }
	, { ExtErr::OBJECTCODE_VIEWCHANGED							, "OBJECTCODE_VIEWCHANGED Advice"		 }
	, { ExtErr::SAT_EmptyStructure								, "SAT Empty Structure"		 }
	, { ExtErr::Crypto											, "Crypto Error"		 }
	, { ExtErr::UnsupportedEncryptionAlgorithm					, "Unsupported Encryption Algorithm"		 }
	, { ExtErr::PROXY_LongDomainName							, "Domain name very long"		 }
	, { ExtErr::PROXY_InvalidAddressType						, "Invalid address type"		 }
	, { ExtErr::PROXY_VeryLongLine								, "Very long line"		 }
	, { ExtErr::PROXY_BadReturnCode								, "Bad return code"		 }
	, { ExtErr::PROXY_ConnectTimeOut							, "Connect time out"		 }
	, { ExtErr::PROXY_NoLocalHosts								, "No local hosts found"		 }
	, { ExtErr::PROXY_MethodNotSupported						, "SOCKS Method not supported"		 }
	, { ExtErr::PROXY_InvalidHttpRequest						, "Invalid HTTP request"		 }
	, { ExtErr::SOCKS_InvalidVersion							, "Invalid Version Number (only V4 & V5 are supported)"		 }
	, { ExtErr::SOCKS_IncorrectProtocol							, "Incorrect bytes in protocol"		 }
	, { ExtErr::SOCKS_RejectedBecauseIDENTD						, "Request rejected because SOCKS server cannot connect to identd on the client"		 }
	, { ExtErr::SOCKS_RejectedOrFailed							, "Request rejected or failed"		 }
	, { ExtErr::SOCKS_DifferentUserIDs							, "Request rejected because the client program and identd report different user-ids"		 }
	, { ExtErr::SOCKS_AuthNotSupported							, "Server's authentication method does not supported by client"		 }
	, { ExtErr::SOCKS_BadUserOrPassword							, "Bad SOCKS Username or Password"		 }
	, { ExtErr::SOCKS_Base										, "Base SOCKS error"		 }
	, { ExtErr::SOCKS_GeneralFailure							, "General SOCKS server failure"		 }
	, { ExtErr::SOCKS_NotAllowedByRuleset						, "Connection not allowed by ruleset"		 }
	, { ExtErr::SOCKS_NetworkUnreachable						, "Network unreachable"		 }
	, { ExtErr::SOCKS_HostUnreachable							, "Host unreachable"		 }
	, { ExtErr::SOCKS_Refused									, "Connection refused"		 }
	, { ExtErr::SOCKS_TTLExpired								, "TTL expired"		 }
	, { ExtErr::SOCKS_CommandNotSupported						, "SOCKS command not supported"		 }
	, { ExtErr::SOCKS_AddressTypeNotSupported					, "AddressTypeNotSupported"		 }
	, { ExtErr::HTTP_Base										, "HTTP error code"		 }
};
	


static class ExtCategory : public ErrorCategoryBase {
	typedef ErrorCategoryBase base;
public:
	ExtCategory()
		:	base("Ext", FACILITY_EXT)
	{}

	string message(int errval) const override {
		if (const char *s = FindInCodeMessageTable(s_extMessageTable, errval))
			return s;
		return "Unknown Ext error";
	}
} s_ext_category;

const error_category& AFXAPI ext_category() {
	return s_ext_category;
}

template<> ErrorCategoryBase * StaticList<ErrorCategoryBase>::Root = 0;

ErrorCategoryBase* ErrorCategoryBase::GetRoot() {
	return Root;
}

const error_category *ErrorCategoryBase::Find(int fac) {
	for (const ErrorCategoryBase *p=Root; p; p=p->Next) {
		if (p->Facility == fac)
			return p;
	}
	return nullptr;
}


} // Ext::

