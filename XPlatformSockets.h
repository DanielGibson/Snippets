// Crossplatform-Sockets-API ("XSA"), abstracting the differences between
// UNIX Sockets (from Linux, *BSD, OSX, ...) and Winsock (WSA)

/*
 * Do:
 *   #define XSA_IMPLEMENTATION
 * before you include this file in *one* of your .c/.cpp files.
 *
 * For further usage please read the comments over the xsa_* function definitions below.
 * Also check out the XSA_* errorcode defitions that unify Winsock WSA errorcodes like
 * WSAEINTR and UNIX errorcodes like EINTR.
 *
 * If you're porting Winsock code, you can `#define XSA_USE_WSAE_EVERYWHERE` to use
 * WSA errorcodes on all platforms (as far as they're portable).
 *
 * A *temporary* porting aid (in any direction) can be found towards the end of
 * the file, search for "ERRORS FOR PORTING" and read further description there.
 *
 * (C) 2017-2026 Daniel Gibson
 *
 * License:
 *  This software is dual-licensed to the public domain and under the following
 *  license: you are granted a perpetual, irrevocable license to copy, modify,
 *  publish, and distribute this file as you see fit.
 *  No warranty implied; use at your own risk.
 *
 * So you can do whatever you want with this code, including copying it
 * (or parts of it) into your own source.
 * No need to mention me or this "license" in your code or docs, even though
 * it would be appreciated, of course.
 */


#ifndef XPLATFORMSOCKETS__H
#define XPLATFORMSOCKETS__H

#ifdef _WIN32
	#include <winsock2.h>
	#include <ws2tcpip.h> // getaddrinfo()
#else // Unix-like
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <errno.h>
	#include <netdb.h>
#endif

#ifndef XSA_DEF
	#define XSA_DEF
#endif

#ifdef __cplusplus
extern "C" {
#endif

// call before doing any networking (probably when starting your application)
XSA_DEF int xsa_init( void );

// call after you're done with all networking (probably when quitting your application)
XSA_DEF void xsa_shutdown( void );

// returns 1 if xsa_init() has been successfully called, else 0
XSA_DEF int xsa_is_initialized( void );

// to get a string for an errorcode from xsa_errno, use this function.
XSA_DEF const char* xsa_strerror( int errorCode );

// custom version of gai_strerror() - on Windows we use a define to use this instead of the original
// because according to Microsoft their gai_strerror() isn't thread-safe
// we don't have XSA_EAI_* constants, though, just use the regular EAI_* everywhere!
XSA_DEF const char* xsa_gai_strerror( int gaiErrorCode );

#ifndef _WIN32 // additions for POSIX

	// Winsock uses the SOCKET type instead of int for socket handles
	// and so should you, from now on. This typedef allows you to do that on Unix
	typedef int SOCKET; // TODO: or call it XSA_SOCKET or xsa_sock_t or something?

	// Winsock requires you to call closesocket() instead of close()
	// on socket handles. Do this everywhere else, too!
	XSA_DEF int closesocket( SOCKET s );

	// With Winsock, socket handles are unsigned and thus the check
	// if(socket(...) < 0) handle_error(); DOES NOT work,
	// they have the INVALID_SOCKET constant for that. Use it whenever a socket
	// handle is returned, like: if(socket(...) == INVALID_SOCKET) handle_error();
	#define INVALID_SOCKET -1

	// Socket functions often return an int that is 0 on success and -1 on error
	// This is the same for Winsock and Unix, but Winsock has a SOCKET_ERROR #define
	// for that. I provide that on other platforms too, for ease of porting
	#define SOCKET_ERROR -1

	// Get the error code after something failed - Winsock doesn't use errno there,
	// so use this abstraction
	#define xsa_errno errno

	// abstraction for poll() vs WSApoll() (which, apart from the name and the error constants, are pretty much identical)
	#define xsa_poll(FDS, NFDS, TIMEOUT_MS) poll(FDS, NFDS, TIMEOUT_MS)
#endif // not _WIN32

#ifdef _WIN32

	typedef unsigned long in_addr_t;

	// on Unix, there is no WSAGetLastError(), errno is used instead.
	// xsa_errno does the right thing on all platforms
	#define xsa_errno WSAGetLastError()

	// https://learn.microsoft.com/en-us/windows/win32/api/ws2tcpip/nf-ws2tcpip-gai_strerrora
	// says that their gai_strerror() isn't threadsafe, so I implement our own..
	#ifdef gai_strerror
		#undef gai_strerror
	#endif
	#define gai_strerror( ecode ) xsa_gai_strerror( ecode )

	// TODO: #undef WSA_NODATA + #define EAI_NODATA WSA_NODATA ws2tcpip.h defines it as EAI_NONAME which doesn't make much sense..

	// TODO: EAI_OVERFLOW and EAI_SYSTEM are defined by POSIX, but not by Windows - define them to some invalid value so they're available and compile?
	// TODO: what happens when calling getnameinfo() with too small node/service buffers?

	// abstraction for poll() vs WSApoll() (which, apart from the name and the error constants, are pretty much identical)
	// make sure to use "struct pollfd" instead of its alias WSAPOLLFD
	#define xsa_poll(FDS, NFDS, TIMEOUT_MS) WSAPoll(FDS, NFDS, TIMEOUT_MS)
#endif

#ifdef __cplusplus
} // extern "C"
#endif

// winsock uses WSAE* for socket error codes, while posix just uses E*
// always use XSA_E* instead for sockets.
enum
{
	// this is used as a dummy error code for error codes not supported by the current platform
	// (I hope no platform uses INT32_MIN as valid error code?!)
	_XSA_UNSUPPORTED_ERRORCODE = ( -2147483647 - 1 ),

#ifdef _WIN32
#define XSA__ORIGCONSTANT(name) WSA##name
#define _XSA_WSA_CONSTANT(name)  WSA##name
#else
#define XSA__ORIGCONSTANT(name) name
	// it's winsock-only, assign dummy value on other platforms
#define _XSA_WSA_CONSTANT(name) _XSA_UNSUPPORTED_ERRORCODE
#endif

	// some WinSock-only error codes
	XSA_INVALID_HANDLE  = _XSA_WSA_CONSTANT( _INVALID_HANDLE ),
	XSA_NOT_ENOUGH_MEMORY = _XSA_WSA_CONSTANT( _NOT_ENOUGH_MEMORY ),
	XSA_INVALID_PARAMETER = _XSA_WSA_CONSTANT( _INVALID_PARAMETER ),
	XSA_OPERATION_ABORTED = _XSA_WSA_CONSTANT( _OPERATION_ABORTED ),
	XSA_IO_INCOMPLETE   = _XSA_WSA_CONSTANT( _IO_INCOMPLETE ),
	XSA_IO_PENDING      = _XSA_WSA_CONSTANT( _IO_PENDING ),

	// this huge block of error codes is supported by BSD sockets as well (I hope? at least on Linux?)
	XSA_EINTR           = XSA__ORIGCONSTANT( EINTR ),
	XSA_EBADF           = XSA__ORIGCONSTANT( EBADF ),
	XSA_EACCES          = XSA__ORIGCONSTANT( EACCES ),
	XSA_EFAULT          = XSA__ORIGCONSTANT( EFAULT ),
	XSA_EINVAL          = XSA__ORIGCONSTANT( EINVAL ),
	XSA_EMFILE          = XSA__ORIGCONSTANT( EMFILE ),
	XSA_EWOULDBLOCK     = XSA__ORIGCONSTANT( EWOULDBLOCK ),
	XSA_EAGAIN          = XSA_EWOULDBLOCK, // common alias on Unices
	XSA_EINPROGRESS     = XSA__ORIGCONSTANT( EINPROGRESS ),
	XSA_EALREADY        = XSA__ORIGCONSTANT( EALREADY ),
	XSA_ENOTSOCK        = XSA__ORIGCONSTANT( ENOTSOCK ),
	XSA_EDESTADDRREQ    = XSA__ORIGCONSTANT( EDESTADDRREQ ),
	XSA_EMSGSIZE        = XSA__ORIGCONSTANT( EMSGSIZE ),
	XSA_EPROTOTYPE      = XSA__ORIGCONSTANT( EPROTOTYPE ),
	XSA_ENOPROTOOPT     = XSA__ORIGCONSTANT( ENOPROTOOPT ),
	XSA_EPROTONOSUPPORT = XSA__ORIGCONSTANT( EPROTONOSUPPORT ),
	XSA_ESOCKTNOSUPPORT = XSA__ORIGCONSTANT( ESOCKTNOSUPPORT ),
	XSA_EOPNOTSUPP      = XSA__ORIGCONSTANT( EOPNOTSUPP ),
	XSA_EPFNOSUPPORT    = XSA__ORIGCONSTANT( EPFNOSUPPORT ),
	XSA_EAFNOSUPPORT    = XSA__ORIGCONSTANT( EAFNOSUPPORT ),
	XSA_EADDRINUSE      = XSA__ORIGCONSTANT( EADDRINUSE ),
	XSA_EADDRNOTAVAIL   = XSA__ORIGCONSTANT( EADDRNOTAVAIL ),
	XSA_ENETDOWN        = XSA__ORIGCONSTANT( ENETDOWN ),
	XSA_ENETUNREACH     = XSA__ORIGCONSTANT( ENETUNREACH ),
	XSA_ENETRESET       = XSA__ORIGCONSTANT( ENETRESET ),
	XSA_ECONNABORTED    = XSA__ORIGCONSTANT( ECONNABORTED ),
	XSA_ECONNRESET      = XSA__ORIGCONSTANT( ECONNRESET ),
	XSA_ENOBUFS         = XSA__ORIGCONSTANT( ENOBUFS ),
	XSA_EISCONN         = XSA__ORIGCONSTANT( EISCONN ),
	XSA_ENOTCONN        = XSA__ORIGCONSTANT( ENOTCONN ),
	XSA_ESHUTDOWN       = XSA__ORIGCONSTANT( ESHUTDOWN ),
	XSA_ETOOMANYREFS    = XSA__ORIGCONSTANT( ETOOMANYREFS ),
	XSA_ETIMEDOUT       = XSA__ORIGCONSTANT( ETIMEDOUT ),
	XSA_ECONNREFUSED    = XSA__ORIGCONSTANT( ECONNREFUSED ),
	XSA_ELOOP           = XSA__ORIGCONSTANT( ELOOP ),
	XSA_ENAMETOOLONG    = XSA__ORIGCONSTANT( ENAMETOOLONG ),
	XSA_EHOSTDOWN       = XSA__ORIGCONSTANT( EHOSTDOWN ),
	XSA_EHOSTUNREACH    = XSA__ORIGCONSTANT( EHOSTUNREACH ),
	XSA_ENOTEMPTY       = XSA__ORIGCONSTANT( ENOTEMPTY ),

	XSA_EUSERS          = XSA__ORIGCONSTANT( EUSERS ),
	XSA_EDQUOT          = XSA__ORIGCONSTANT( EDQUOT ),
	XSA_ESTALE          = XSA__ORIGCONSTANT( ESTALE ),
	XSA_EREMOTE         = XSA__ORIGCONSTANT( EREMOTE ),

	// the following seem to be winsock-only
	XSA_EPROCLIM        = _XSA_WSA_CONSTANT( EPROCLIM ), // only used by WSAStartUp()
	XSA_SYSNOTREADY     = _XSA_WSA_CONSTANT( SYSNOTREADY ),
	XSA_VERNOTSUPPORTED = _XSA_WSA_CONSTANT( VERNOTSUPPORTED ),
	XSA_NOTINITIALISED  = _XSA_WSA_CONSTANT( NOTINITIALISED ),
	XSA_EDISCON         = _XSA_WSA_CONSTANT( EDISCON ),
	XSA_ENOMORE         = _XSA_WSA_CONSTANT( ENOMORE ),         // will be removed in the future, same as WSA_E_NO_MORE, check both !!
	XSA_E_NO_MORE       = _XSA_WSA_CONSTANT( _E_NO_MORE ),
	XSA_ECANCELLED      = _XSA_WSA_CONSTANT( ECANCELLED ),      // will be removed in the future, same as WSA_E_CANCELLED, check both !!
	XSA_E_CANCELLED     = _XSA_WSA_CONSTANT( _E_CANCELLED ),    // TODO: same as ECANCELED (one L) on UNIX?
	XSA_EINVALIDPROCTABLE = _XSA_WSA_CONSTANT( EINVALIDPROCTABLE ),
	XSA_EINVALIDPROVIDER  = _XSA_WSA_CONSTANT( EINVALIDPROVIDER ),
	XSA_EPROVIDERFAILEDINIT = _XSA_WSA_CONSTANT( EPROVIDERFAILEDINIT ),
	XSA_SYSCALLFAILURE    = _XSA_WSA_CONSTANT( SYSCALLFAILURE ),
	XSA_SERVICE_NOT_FOUND = _XSA_WSA_CONSTANT( SERVICE_NOT_FOUND ),
	XSA_TYPE_NOT_FOUND  = _XSA_WSA_CONSTANT( TYPE_NOT_FOUND ),

	// TODO: "database" stuff (netdb.h equivalents)
	XSA_EREFUSED        = _XSA_WSA_CONSTANT( EREFUSED ), // not sure if part of that

	// NOTE: intentionally not defining XSA_TRY_AGAIN, XSA_HOST_NOT_FOUND, XSA_NO_RECOVERY (from netdb.h)
	//       that would be used with gethostbyaddr() and gethostbyname(),
	//       because those functions are deprecated (both in POSIX and WinSock)!
	//       Use getaddrinfo() and getnameinfo() instead!

	// WinSock uses WSANO_DATA as return value of getaddrinfo(), so define it here(?)
	XSA_NO_DATA         = _XSA_WSA_CONSTANT( NO_DATA ),


	// TODO: WSA_QOS_* ?

#undef XSA__ORIGCONSTANT
#undef _XSA_WSA_CONSTANT
};

#if !defined(_WIN32) && defined(XSA_USE_WSAE_EVERYWHERE)
	// if you're porting code from winsocks and just wanna keep using
	// WSAE* instead of XSA_E*, this allows you to do that

	// unfortunately, we can't do the same for porting from unix to windows,
	// because windows does use EINTR etc itself, just not for sockets, so
	// #define EINTR XSA_EINTR etc would screw up things there.

	#define WSAEINTR            XSA_EINTR
	#define WSAEBADF            XSA_EBADF
	#define WSAEACCES           XSA_EACCES
	#define WSAEFAULT           XSA_EFAULT
	#define WSAEINVAL           XSA_EINVAL
	#define WSAEMFILE           XSA_EMFILE
	#define WSAEWOULDBLOCK      XSA_EWOULDBLOCK
	#define WSAEAGAIN           XSA_EAGAIN
	#define WSAEINPROGRESS      XSA_EINPROGRESS
	#define WSAEALREADY         XSA_EALREADY
	#define WSAENOTSOCK         XSA_ENOTSOCK
	#define WSAEDESTADDRREQ     XSA_EDESTADDRREQ
	#define WSAEMSGSIZE         XSA_EMSGSIZE
	#define WSAEPROTOTYPE       XSA_EPROTOTYPE
	#define WSAENOPROTOOPT      XSA_ENOPROTOOPT
	#define WSAEPROTONOSUPPORT  XSA_EPROTONOSUPPORT
	#define WSAESOCKTNOSUPPORT  XSA_ESOCKTNOSUPPORT
	#define WSAEOPNOTSUPP       XSA_EOPNOTSUPP
	#define WSAEPFNOSUPPORT     XSA_EPFNOSUPPORT
	#define WSAEAFNOSUPPORT     XSA_EAFNOSUPPORT
	#define WSAEADDRINUSE       XSA_EADDRINUSE
	#define WSAEADDRNOTAVAIL    XSA_EADDRNOTAVAIL
	#define WSAENETDOWN         XSA_ENETDOWN
	#define WSAENETUNREACH      XSA_ENETUNREACH
	#define WSAENETRESET        XSA_ENETRESET
	#define WSAECONNABORTED     XSA_ECONNABORTED
	#define WSAECONNRESET       XSA_ECONNRESET
	#define WSAENOBUFS          XSA_ENOBUFS
	#define WSAEISCONN          XSA_EISCONN
	#define WSAENOTCONN         XSA_ENOTCONN
	#define WSAESHUTDOWN        XSA_ESHUTDOWN
	#define WSAETOOMANYREFS     XSA_ETOOMANYREFS
	#define WSAETIMEDOUT        XSA_ETIMEDOUT
	#define WSAECONNREFUSED     XSA_ECONNREFUSED
	#define WSAELOOP            XSA_ELOOP
	#define WSAENAMETOOLONG     XSA_ENAMETOOLONG
	#define WSAEHOSTDOWN        XSA_EHOSTDOWN
	#define WSAEHOSTUNREACH     XSA_EHOSTUNREACH
	#define WSAENOTEMPTY        XSA_ENOTEMPTY

	#define WSAEUSERS           XSA_EUSERS
	#define WSAEDQUOT           XSA_EDQUOT
	#define WSAESTALE           XSA_ESTALE
	#define WSAEREMOTE          XSA_EREMOTE

#endif // !defined(_WIN32) && defined(XSA_USE_WSAE_EVERYWHERE)

#endif // XPLATFORMSOCKETS__H


// ######################## BELOW: IMPLEMENTATION #######################

#if defined(XSA_IMPLEMENTATION) && !defined(_XPLATFORMSOCKETS_H_IMPL)
#define _XPLATFORMSOCKETS_H_IMPL

#include <assert.h>
#include <stdio.h>

#ifndef _WIN32
	#include <unistd.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

static int xsa__is_initialized = 0;

XSA_DEF int xsa_is_initialized( void )
{
	return xsa__is_initialized;
}

XSA_DEF int xsa_init( void )
{
	int ret = 0;
#ifdef _WIN32
	WSADATA wsaData;
#endif
	if( xsa__is_initialized )
	{
		assert( 0 && "Don't call XSA_Init() twice!" );
		return 0;
	}
#ifdef _WIN32
	ret = WSAStartup( MAKEWORD( 2, 2 ), &wsaData );
#endif
	xsa__is_initialized = ( ret == 0 );
	return ret;
}

XSA_DEF void xsa_shutdown( void )
{
	if( !xsa__is_initialized )
	{
		assert( 0 && "Don't call XSA_Shutdown() twice or without successful initialization!" );
		return;
	}
#ifdef _WIN32
	WSACleanup();
#endif
	xsa__is_initialized = 0;
}

XSA_DEF const char* xsa_strerror( int errorCode )
{
	// Descriptions from http://tangentsoft.net/wskfaq/examples/basics/ws-util.cpp
	// which is placed in the Public Domain

#define XSA__ERR_ENTRY(name, descr) case XSA_##name : return #name " : " descr ;

	switch( errorCode )
	{
		case XSA_EWOULDBLOCK:
			return "EWOULDBLOCK/EAGAIN : Operation would block";
		case 0:
			return "NO ERROR";
		case _XSA_UNSUPPORTED_ERRORCODE :
			return "An errorcode that's not supported/used on the current platform (or so I thought..)";

			XSA__ERR_ENTRY( EINTR, "Interrupted system call" )
			XSA__ERR_ENTRY( EBADF, "Bad file number" )
			XSA__ERR_ENTRY( EACCES, "Permission denied" )
			XSA__ERR_ENTRY( EFAULT, "Bad address" )
			XSA__ERR_ENTRY( EINVAL, "Invalid argument" )
			XSA__ERR_ENTRY( EMFILE, "Too many open sockets" )
			// XSA__ERR_ENTRY( EWOULDBLOCK    , "Operation would block" )
			XSA__ERR_ENTRY( EINPROGRESS, "Operation now in progress" )
			XSA__ERR_ENTRY( EALREADY, "Operation already in progress" )
			XSA__ERR_ENTRY( ENOTSOCK, "Socket operation on non-socket" )
			XSA__ERR_ENTRY( EDESTADDRREQ, "Destination address required" )
			XSA__ERR_ENTRY( EMSGSIZE, "Message too long" )
			XSA__ERR_ENTRY( EPROTOTYPE, "Protocol wrong type for socket" )
			XSA__ERR_ENTRY( ENOPROTOOPT, "Bad protocol option" )
			XSA__ERR_ENTRY( EPROTONOSUPPORT, "Protocol not supported" )
			XSA__ERR_ENTRY( ESOCKTNOSUPPORT, "Socket type not supported" )
			XSA__ERR_ENTRY( EOPNOTSUPP, "Operation not supported on socket" )
			XSA__ERR_ENTRY( EPFNOSUPPORT, "Protocol family not supported" )
			XSA__ERR_ENTRY( EAFNOSUPPORT, "Address family not supported" )
			XSA__ERR_ENTRY( EADDRINUSE, "Address already in use" )
			XSA__ERR_ENTRY( EADDRNOTAVAIL, "Can't assign requested address" )
			XSA__ERR_ENTRY( ENETDOWN, "Network is down" )
			XSA__ERR_ENTRY( ENETUNREACH, "Network is unreachable" )
			XSA__ERR_ENTRY( ENETRESET, "Net connection reset" )
			XSA__ERR_ENTRY( ECONNABORTED, "Software caused connection abort" )
			XSA__ERR_ENTRY( ECONNRESET, "Connection reset by peer" )
			XSA__ERR_ENTRY( ENOBUFS, "No buffer space available" )
			XSA__ERR_ENTRY( EISCONN, "Socket is already connected" )
			XSA__ERR_ENTRY( ENOTCONN, "Socket is not connected" )
			XSA__ERR_ENTRY( ESHUTDOWN, "Can't send after socket shutdown" )
			XSA__ERR_ENTRY( ETOOMANYREFS, "Too many references, can't splice" )
			XSA__ERR_ENTRY( ETIMEDOUT, "Connection timed out" )
			XSA__ERR_ENTRY( ECONNREFUSED, "Connection refused" )
			XSA__ERR_ENTRY( ELOOP, "Too many levels of symbolic links" )
			XSA__ERR_ENTRY( ENAMETOOLONG, "File name too long" )
			XSA__ERR_ENTRY( EHOSTDOWN, "Host is down" )
			XSA__ERR_ENTRY( EHOSTUNREACH, "No route to host" )
			XSA__ERR_ENTRY( ENOTEMPTY, "Directory not empty" )

			XSA__ERR_ENTRY( EUSERS, "Too many users" )
			XSA__ERR_ENTRY( EDQUOT, "Disc quota exceeded" )
			XSA__ERR_ENTRY( ESTALE, "Stale NFS file handle" )
			XSA__ERR_ENTRY( EREMOTE, "Too many levels of remote in path" )

#ifdef _WIN32 // WinSock-only
			XSA__ERR_ENTRY( INVALID_HANDLE, "Event Object Handle is invalid" )
			XSA__ERR_ENTRY( NOT_ENOUGH_MEMORY, "Windows doesn't have enough memory" )
			XSA__ERR_ENTRY( INVALID_PARAMETER, "Invalid parameter(s)" )
			XSA__ERR_ENTRY( OPERATION_ABORTED, "Overlapped operation aborted" )
			XSA__ERR_ENTRY( IO_INCOMPLETE, "Overlapped I/O operation not completed yet" )
			XSA__ERR_ENTRY( IO_PENDING, "Overlapped operation pending, will indicate completion later" )        // ?? how is this different from IO_INCOMPLETE ?!

			XSA__ERR_ENTRY( EPROCLIM, "Too many processes using WinSock" )

			XSA__ERR_ENTRY( SYSNOTREADY, "Network subsystem unavailable (missing WinSock DLL?)" )
			XSA__ERR_ENTRY( VERNOTSUPPORTED, "Requested WinSock version not supported" )
			XSA__ERR_ENTRY( NOTINITIALISED, "WSAStartup() must be called successfully before using this function" )
			XSA__ERR_ENTRY( EDISCON, "Remote host has shut the connection down" )
			XSA__ERR_ENTRY( ENOMORE, "No more results" )
			XSA__ERR_ENTRY( E_NO_MORE, "No more results" )
			XSA__ERR_ENTRY( ECANCELLED, "Call has been canceled" )
			XSA__ERR_ENTRY( E_CANCELLED, "Call has been canceled" )
			XSA__ERR_ENTRY( EINVALIDPROCTABLE, "Service provider procedure table is invalid" )
			XSA__ERR_ENTRY( EINVALIDPROVIDER, "Service provider is invalid" )
			XSA__ERR_ENTRY( EPROVIDERFAILEDINIT, "Service provider failed to initialize" )
			XSA__ERR_ENTRY( SYSCALLFAILURE, "System call has failed, even though it shouldn't" )
			XSA__ERR_ENTRY( SERVICE_NOT_FOUND, "Service not found" )
			XSA__ERR_ENTRY( TYPE_NOT_FOUND, "Class type not found" )

			XSA__ERR_ENTRY( EREFUSED, "Database query was refused" ) // whatever this means

			XSA__ERR_ENTRY( NO_DATA, "The requested name is valid, but no data of the requested type was found" )
#endif // _WIN32

		default:

			break;
	}

#undef XSA__ERR_ENTRY

	static char buf[32]; // FIXME: not thread-safe! thread_local?
	snprintf( buf, sizeof( buf ), "Unknown errorcode %d", errorCode );

	return buf;

	//return "Unknown Error Code";
}

XSA_DEF const char* xsa_gai_strerror( int gaiErrorCode )
{
#define XSA__ERR_ENTRY(name, descr) case name : return #name " : " descr ;
	switch( gaiErrorCode )
	{
			XSA__ERR_ENTRY( EAI_BADFLAGS, "The flags had an invalid value" )
			XSA__ERR_ENTRY( EAI_NONAME,   "Name/Node or Service is unknown or invalid" )
			XSA__ERR_ENTRY( EAI_AGAIN,    "Temporary failure in name resolution" )
			XSA__ERR_ENTRY( EAI_FAIL,     "Non-recoverable failure in name resolution" )
			XSA__ERR_ENTRY( EAI_FAMILY,   "addrinfo::ai_family has unsupported value" )
			XSA__ERR_ENTRY( EAI_SOCKTYPE, "addrinfo::ai_socktype invalid or inconsistent with ai_protocol" )
			XSA__ERR_ENTRY( EAI_SERVICE,  "addrinfo::ai_socktype doesn't support given SERVICE" )
			XSA__ERR_ENTRY( EAI_MEMORY,   "Memory allocation failure" )

			// the following two are defined by POSIX, but not by WinSock
#ifdef EAI_OVERFLOW
			XSA__ERR_ENTRY( EAI_OVERFLOW, "An argument buffer was too small" )
#endif
#ifdef EAI_SYSTEM
			XSA__ERR_ENTRY( EAI_SYSTEM,   "Another system error, see errno" )
#endif

#ifdef _WIN32 // winsock-specific errors of getaddrinfo() and getnameinfo() that use WSA constants directly
			XSA__ERR_ENTRY( WSANO_DATA,        "The requested name is valid, but no data of the requested type was found" )
			XSA__ERR_ENTRY( WSANOTINITIALISED, "WSAStartup() must be called successfully before using this function" )
			XSA__ERR_ENTRY( WSAEFAULT,         "SockaddrLength argument was too small for struct sockaddr_in" )
#ifdef EAI_NOSECURENAME
		case EAI_NOSECURENAME :
			return "EAI_NOSECURENAME"; // TODO: what is this? it's not documented.. defined as WSA_SECURE_HOST_NOT_FOUND
#endif
#ifdef EAI_IPSECPOLICY
		case EAI_IPSECPOLICY  :
			return "EAI_IPSECPOLICY"; // ditto (WSA_IPSEC_NAME_POLICY_ERROR)
#endif
#endif

			// error code defined by Linux but not by POSIX (or WinSock)
#if defined(EAI_NODATA) && (EAI_NODATA != EAI_NONAME && EAI_NODATA != WSANO_DATA)
			// WinSock defines this one, but as EAI_NONAME (in VS) or WSANO_DATA (mingw)
			XSA__ERR_ENTRY( EAI_NODATA,     "The requested name is valid, but no data of the requested type was found" )
#endif
#ifdef EAI_ADDRFAMILY
			XSA__ERR_ENTRY( EAI_ADDRFAMILY, "No network address with requested address family for given Host" )
#endif
#ifdef EAI_INPROGRESS // these are for getaddrinfo_a(), gai_suspend(), gai_cancel() and gai_error() - async versions of getaddrinfo()
			XSA__ERR_ENTRY( EAI_INPROGRESS, "Request is in progress" )
			XSA__ERR_ENTRY( EAI_CANCELED,  "Request has been sucessfully cancelled" )
			XSA__ERR_ENTRY( EAI_NOTCANCELED, "Request has not been cancelled" )
			XSA__ERR_ENTRY( EAI_ALLDONE,   "Requests is already done" )
			XSA__ERR_ENTRY( EAI_INTR,      "Function was interrupted by a signal" )
#endif
#ifdef EAI_IDN_ENCODE
			XSA__ERR_ENTRY( EAI_IDN_ENCODE, "Parameter string was not encoded correctly" )
#endif
	}
#undef XSA__ERR_ENTRY

	static char buf[70]; // FIXME: not thread-safe!
	snprintf( buf, sizeof( buf ), "Unknown netdb.h/getaddrinfo() (and related) errorcode %d", gaiErrorCode );

	return buf;
}

#ifndef _WIN32 // TODO: The HAIKU OS uses closesocket() instead of close(), too
XSA_DEF int closesocket( SOCKET s )
{
	return close( s );
}
#endif // not _WIN32

#ifdef __cplusplus
} // extern "C"
#endif

#endif // XSA_IMPLEMENTATION


// ######################## BELOW: ERRORS FOR PORTING #######################

#if defined(XSA_ERRORS_FOR_PORTING) && !defined(XSA__ERRORS_FOR_PORTING_INCL)
#define XSA__ERRORS_FOR_PORTING_INCL

// you can #define XSA_ERRORS_FOR_PORTING and you'll get compiler-errors
// when using a platform-specific function/constant instead of one defined
// by XSA, that should make porting easier.
// !! NOTE HOWEVER, that you might get false positives !!
// for example, it will give an compiler-error if you use close(), but you
// should only change close() to closesocket() if it's actually used on a socket
// and not a normal file or something
// Similarly, it may give you errors for using error-codes like EWOULDBLOCK,
// but again, only replace those with XSA_EWOULDBLOCK if the error code is
// from a socket operation.
// So it is a porting aid, but should only be defined temporarily and for each
// compiler error you'll get, you must make sure it really is an error.

#ifndef _WIN32
// the following makes sure that you don't try to assign to xsa_errno
// "xsa_errno = 0;" would (probably) work on Unix, but not Windows
#undef xsa_errno
static int xsa__get_errno( void )
{
	return errno;
}
#define xsa_errno xsa__get_errno()

#define close(x) MAYBE_USE_closesocket_INSTEAD // but only if it's actually called on a socket and not a normal file!
#undef errno
#define errno MAYBE_USE_xsa_errno_INSTEAD // but only if it's for errors from socket functions!
#define strerror(x) MAYBE_USE_xsa_strerror_INSTEAD // but only if it's an error code from sockets!

#define poll MAYBE_USE_xsa_poll_INSTEAD // but only if it's used with sockets!
#endif // not _WIN32

#ifdef _WIN32
	#define WSAGetLastError USE_xsa_errno_INSTEAD

	#define WSAPoll USE_xsa_poll_INSTEAD
	#define WSAPOLLFD USE_struct_pollfd_INSTEAD

	#define WSAStartup MAYBE_USE_xsa_init_INSTEAD
	#define WSACleanup MAYBE_USE_xsa_shutdown_INSTEAD
#endif

// TODO: error constants, other things?

#ifndef _WIN32
	#undef EINTR
	#undef EBADF
	#undef EACCES
	#undef EFAULT
	#undef EINVAL
	#undef EMFILE
	#undef EWOULDBLOCK
	#undef EAGAIN
	#undef EINPROGRESS
	#undef EALREADY
	#undef ENOTSOCK
	#undef EDESTADDRREQ
	#undef EMSGSIZE
	#undef EPROTOTYPE
	#undef ENOPROTOOPT
	#undef EPROTONOSUPPORT
	#undef ESOCKTNOSUPPORT
	#undef EOPNOTSUPP
	#undef EPFNOSUPPORT
	#undef EAFNOSUPPORT
	#undef EADDRINUSE
	#undef EADDRNOTAVAIL
	#undef ENETDOWN
	#undef ENETUNREACH
	#undef ENETRESET
	#undef ECONNABORTED
	#undef ECONNRESET
	#undef ENOBUFS
	#undef EISCONN
	#undef ENOTCONN
	#undef ESHUTDOWN
	#undef ETOOMANYREFS
	#undef ETIMEDOUT
	#undef ECONNREFUSED
	#undef ELOOP
	#undef ENAMETOOLONG
	#undef EHOSTDOWN
	#undef EHOSTUNREACH
	#undef ENOTEMPTY
	#undef EUSERS
	#undef EDQUOT
	#undef ESTALE
	#undef EREMOTE


	#define EINTR            MAYBE_USE_XSA_E*_INSTEAD
	#define EBADF            MAYBE_USE_XSA_E*_INSTEAD
	#define EACCES           MAYBE_USE_XSA_E*_INSTEAD
	#define EFAULT           MAYBE_USE_XSA_E*_INSTEAD
	#define EINVAL           MAYBE_USE_XSA_E*_INSTEAD
	#define EMFILE           MAYBE_USE_XSA_E*_INSTEAD
	#define EWOULDBLOCK      MAYBE_USE_XSA_E*_INSTEAD
	#define EAGAIN           MAYBE_USE_XSA_E*_INSTEAD
	#define EINPROGRESS      MAYBE_USE_XSA_E*_INSTEAD
	#define EALREADY         MAYBE_USE_XSA_E*_INSTEAD
	#define ENOTSOCK         MAYBE_USE_XSA_E*_INSTEAD
	#define EDESTADDRREQ     MAYBE_USE_XSA_E*_INSTEAD
	#define EMSGSIZE         MAYBE_USE_XSA_E*_INSTEAD
	#define EPROTOTYPE       MAYBE_USE_XSA_E*_INSTEAD
	#define ENOPROTOOPT      MAYBE_USE_XSA_E*_INSTEAD
	#define EPROTONOSUPPORT  MAYBE_USE_XSA_E*_INSTEAD
	#define ESOCKTNOSUPPORT  MAYBE_USE_XSA_E*_INSTEAD
	#define EOPNOTSUPP       MAYBE_USE_XSA_E*_INSTEAD
	#define EPFNOSUPPORT     MAYBE_USE_XSA_E*_INSTEAD
	#define EAFNOSUPPORT     MAYBE_USE_XSA_E*_INSTEAD
	#define EADDRINUSE       MAYBE_USE_XSA_E*_INSTEAD
	#define EADDRNOTAVAIL    MAYBE_USE_XSA_E*_INSTEAD
	#define ENETDOWN         MAYBE_USE_XSA_E*_INSTEAD
	#define ENETUNREACH      MAYBE_USE_XSA_E*_INSTEAD
	#define ENETRESET        MAYBE_USE_XSA_E*_INSTEAD
	#define ECONNABORTED     MAYBE_USE_XSA_E*_INSTEAD
	#define ECONNRESET       MAYBE_USE_XSA_E*_INSTEAD
	#define ENOBUFS          MAYBE_USE_XSA_E*_INSTEAD
	#define EISCONN          MAYBE_USE_XSA_E*_INSTEAD
	#define ENOTCONN         MAYBE_USE_XSA_E*_INSTEAD
	#define ESHUTDOWN        MAYBE_USE_XSA_E*_INSTEAD
	#define ETOOMANYREFS     MAYBE_USE_XSA_E*_INSTEAD
	#define ETIMEDOUT        MAYBE_USE_XSA_E*_INSTEAD
	#define ECONNREFUSED     MAYBE_USE_XSA_E*_INSTEAD
	#define ELOOP            MAYBE_USE_XSA_E*_INSTEAD
	#define ENAMETOOLONG     MAYBE_USE_XSA_E*_INSTEAD
	#define EHOSTDOWN        MAYBE_USE_XSA_E*_INSTEAD
	#define EHOSTUNREACH     MAYBE_USE_XSA_E*_INSTEAD
	#define ENOTEMPTY        MAYBE_USE_XSA_E*_INSTEAD

	#define EUSERS           MAYBE_USE_XSA_E*_INSTEAD
	#define EDQUOT           MAYBE_USE_XSA_E*_INSTEAD
	#define ESTALE           MAYBE_USE_XSA_E*_INSTEAD
	#define EREMOTE          MAYBE_USE_XSA_E*_INSTEAD

#endif // not _WIN32

#if defined(_WIN32) && !defined(XSA_USE_WSAE_EVERYWHERE)

	#undef WSAEINTR
	#undef WSAEBADF
	#undef WSAEACCES
	#undef WSAEFAULT
	#undef WSAEINVAL
	#undef WSAEMFILE
	#undef WSAEWOULDBLOCK
	#undef WSAEAGAIN
	#undef WSAEINPROGRESS
	#undef WSAEALREADY
	#undef WSAENOTSOCK
	#undef WSAEDESTADDRREQ
	#undef WSAEMSGSIZE
	#undef WSAEPROTOTYPE
	#undef WSAENOPROTOOPT
	#undef WSAEPROTONOSUPPORT
	#undef WSAESOCKTNOSUPPORT
	#undef WSAEOPNOTSUPP
	#undef WSAEPFNOSUPPORT
	#undef WSAEAFNOSUPPORT
	#undef WSAEADDRINUSE
	#undef WSAEADDRNOTAVAIL
	#undef WSAENETDOWN
	#undef WSAENETUNREACH
	#undef WSAENETRESET
	#undef WSAECONNABORTED
	#undef WSAECONNRESET
	#undef WSAENOBUFS
	#undef WSAEISCONN
	#undef WSAENOTCONN
	#undef WSAESHUTDOWN
	#undef WSAETOOMANYREFS
	#undef WSAETIMEDOUT
	#undef WSAECONNREFUSED
	#undef WSAELOOP
	#undef WSAENAMETOOLONG
	#undef WSAEHOSTDOWN
	#undef WSAEHOSTUNREACH
	#undef WSAENOTEMPTY
	#undef WSAEUSERS
	#undef WSAEDQUOT
	#undef WSAESTALE
	#undef WSAEREMOTE


	#define WSAEINTR            USE_XSA_E*_INSTEAD
	#define WSAEBADF            USE_XSA_E*_INSTEAD
	#define WSAEACCES           USE_XSA_E*_INSTEAD
	#define WSAEFAULT           USE_XSA_E*_INSTEAD
	#define WSAEINVAL           USE_XSA_E*_INSTEAD
	#define WSAEMFILE           USE_XSA_E*_INSTEAD
	#define WSAEWOULDBLOCK      USE_XSA_E*_INSTEAD
	#define WSAEAGAIN           USE_XSA_E*_INSTEAD
	#define WSAEINPROGRESS      USE_XSA_E*_INSTEAD
	#define WSAEALREADY         USE_XSA_E*_INSTEAD
	#define WSAENOTSOCK         USE_XSA_E*_INSTEAD
	#define WSAEDESTADDRREQ     USE_XSA_E*_INSTEAD
	#define WSAEMSGSIZE         USE_XSA_E*_INSTEAD
	#define WSAEPROTOTYPE       USE_XSA_E*_INSTEAD
	#define WSAENOPROTOOPT      USE_XSA_E*_INSTEAD
	#define WSAEPROTONOSUPPORT  USE_XSA_E*_INSTEAD
	#define WSAESOCKTNOSUPPORT  USE_XSA_E*_INSTEAD
	#define WSAEOPNOTSUPP       USE_XSA_E*_INSTEAD
	#define WSAEPFNOSUPPORT     USE_XSA_E*_INSTEAD
	#define WSAEAFNOSUPPORT     USE_XSA_E*_INSTEAD
	#define WSAEADDRINUSE       USE_XSA_E*_INSTEAD
	#define WSAEADDRNOTAVAIL    USE_XSA_E*_INSTEAD
	#define WSAENETDOWN         USE_XSA_E*_INSTEAD
	#define WSAENETUNREACH      USE_XSA_E*_INSTEAD
	#define WSAENETRESET        USE_XSA_E*_INSTEAD
	#define WSAECONNABORTED     USE_XSA_E*_INSTEAD
	#define WSAECONNRESET       USE_XSA_E*_INSTEAD
	#define WSAENOBUFS          USE_XSA_E*_INSTEAD
	#define WSAEISCONN          USE_XSA_E*_INSTEAD
	#define WSAENOTCONN         USE_XSA_E*_INSTEAD
	#define WSAESHUTDOWN        USE_XSA_E*_INSTEAD
	#define WSAETOOMANYREFS     USE_XSA_E*_INSTEAD
	#define WSAETIMEDOUT        USE_XSA_E*_INSTEAD
	#define WSAECONNREFUSED     USE_XSA_E*_INSTEAD
	#define WSAELOOP            USE_XSA_E*_INSTEAD
	#define WSAENAMETOOLONG     USE_XSA_E*_INSTEAD
	#define WSAEHOSTDOWN        USE_XSA_E*_INSTEAD
	#define WSAEHOSTUNREACH     USE_XSA_E*_INSTEAD
	#define WSAENOTEMPTY        USE_XSA_E*_INSTEAD

	#define WSAEUSERS           USE_XSA_E*_INSTEAD
	#define WSAEDQUOT           USE_XSA_E*_INSTEAD
	#define WSAESTALE           USE_XSA_E*_INSTEAD
	#define WSAEREMOTE          USE_XSA_E*_INSTEAD

#endif // _WIN32

#endif // XSA_ERRORS_FOR_PORTING && XSA__ERRORS_FOR_PORTING_INCL
