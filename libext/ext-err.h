/*######   Copyright (c) 2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once


namespace Ext {

ENUM_CLASS(ExtErr) {
	EndOfStream								= 1
	, IndexOutOfRange						= 2
	, VartypeNotSupported
	, InvalidDimCount
	, BlobNotInitialized
	, Compress
	, Registry
	, InvalidFlags
	, IncorrectVariant
	, ItemNotFound
	, DCOMnotInstalled
	, VariantMustBeArray
	, InvalidTypeForThisOperation
	, QueueOverflow
	, NoMoreServiceHandlers
	, AlreadyOpened
	, InvalidRegistrationCode
	, UnknownVersionRegcode
	, DuplicateName
	, InvalidValue
	, ThreadInterrupted
	, InvalidFileHeader
	, IncompatibleFileVersion
	, InvalidSignature
	, InterfaceAlreadyAssigned
	, InFindHandler
	, FindHandler
	, InvalidExceptionMagicNumber
	, InvalidResultCallHandler
	, ControlNotFound
	, ClientSizeNULL
	, DialogTemplate
	, DispatchIsNull
	, BlobIsNotString
	, ControlHWNDIsNULL
	, UnknownHostAddressType
	, TooltipNotFound
	, LoadXML
	, XMLError
	, NoInputStream
	, NoOutputStream
	, InvalidExtendOfNumber
	, UnknownStructuredStorageElementType
	, UnknownTypeOfVariant
	, DifferentTypeOfVariant
	, FontNotFound
	, NoAdviseHolder
	, OleControlError
	, UnknownWin32Error
	, VarTypeInNotStringCompatible
	, UnknownSocketsError
	, InvalidUTF8String
	, UnsupportedVariantType
	, NoControlWithThisID
	, NameOfApplicationKeyIsEmpty
	, AssertFailedLine
	, Trace
	, AssertValidObject
	, InvalidSplitType
	, BlackRedTree
	, QueueUserAPC
	, SignalBreak
	, NormalExit		//!!!?
	, BigNumConversion
	, DivideByZero
	, InvalidInteger
	, InvalidCast
	, MaxTries
	, NonEmptyPointer
	, ObjectNotInitialized
	, ObjectDisposed
	, Dynamic_Library
	, CodeNotReachable
	, Imaging
	, Arithmetic
	, Overflow
	, EncodingNotSupported
	, Protocol_Violation
	, New_Protocol_Version
	, FileFormat
	, ReadTapeSync
	, CrashRpt
	, RecursionTooDeep
	, Map
	
	, DB_NoRecord = 2000
	, DB_DupKey
	, DB_Corrupt
	, DB_Version
	, DB_InternalError

	, JSON_Parse = 3000
	, JSON_RPC_ParseError
	, JSON_RPC_Internal
	, JSON_RPC_InvalidParams
	, JSON_RPC_MethodNotFound
	, JSON_RPC_IsNotRequest

	, OBJECTCODE_DATACHANGED		= 4000
	, OBJECTCODE_VIEWCHANGED

	, SAT_EmptyStructure = 5000

	, Crypto						= 7000	
	, UnsupportedEncryptionAlgorithm

	, PROXY_LongDomainName			= 8000
	, PROXY_InvalidAddressType
	, PROXY_VeryLongLine
	, PROXY_BadReturnCode
	, PROXY_ConnectTimeOut
	, PROXY_NoLocalHosts
	, PROXY_MethodNotSupported
	, PROXY_InvalidHttpRequest

	, SOCKS_InvalidVersion			= 8100
	, SOCKS_IncorrectProtocol
	, SOCKS_RejectedBecauseIDENTD
	, SOCKS_RejectedOrFailed
	, SOCKS_DifferentUserIDs
	, SOCKS_AuthNotSupported
	, SOCKS_BadUserOrPassword

	, SOCKS_Base					= 8200
	, SOCKS_GeneralFailure
	, SOCKS_NotAllowedByRuleset
	, SOCKS_NetworkUnreachable
	, SOCKS_HostUnreachable
	, SOCKS_Refused
	, SOCKS_TTLExpired
	, SOCKS_CommandNotSupported
	, SOCKS_AddressTypeNotSupported

} END_ENUM_CLASS(ExtErr);




} // Ext::

