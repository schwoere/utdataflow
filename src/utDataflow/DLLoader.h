/*
 * Ubitrack - Library for Ubiquitous Tracking
 * Copyright 2006, Technische Universitaet Muenchen, and individual
 * contributors as indicated by the @authors tag. See the 
 * copyright.txt in the distribution for a full listing of individual
 * contributors.
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this software; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA, or see the FSF site: http://www.fsf.org.
 */


/**
 * @ingroup dataflow_framework
 * @file
 * inline wrapper functions for transparent loading of dynamic libraries
 * syntax compatible to libtool wrapper library libltdl
 * @author Florian Echtler <echtler@in.tum.de>
 */


#ifndef __DLLOADER_H__
#define __DLLOADER_H__


#ifdef USE_LIBLTDL

	#include <ltdl.h>

#elif _WIN32
	
	#include <utUtil/CleanWindows.h>

	extern "C" {

		typedef FARPROC lt_ptr;
		typedef HMODULE lt_dlhandle;
		
		inline int lt_dlinit() { return 0; }
		inline int lt_dlexit() { return 0; }

		inline lt_dlhandle lt_dlopen   ( const char* filename ) { return LoadLibrary( (LPCSTR)filename ); }
		inline lt_dlhandle lt_dlopenext( const char* filename ) { return LoadLibrary( (LPCSTR)filename ); }

		inline int lt_dlclose( lt_dlhandle handle ) { return FreeLibrary( handle ) == TRUE ? 0 : 1; }

		inline lt_ptr lt_dlsym( lt_dlhandle handle, const char* name ) { return GetProcAddress( handle, (LPCSTR)name ); }

		inline char* lt_dlerror() {
			
			DWORD error = GetLastError();
			LPVOID buffer;
			
			FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				NULL,
				error,
				MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
				(LPTSTR)&buffer,
				0,
				NULL
			);
			
			return (char*)buffer;
		}

	}

#else // !(USE_LIBLTDL) && !(_WIN32)

	#include <dlfcn.h>

	extern "C" {

		typedef void* lt_ptr;
		typedef void* lt_dlhandle;

		inline int lt_dlinit() { return 0; }
		inline int lt_dlexit() { return 0; }

		// Lazy (only at execution) and global (for all subsequent libraries) symbol evaluation
		// is needed to resolve circular dependencies between component libraries.
		inline lt_dlhandle lt_dlopen   ( const char* filename ) { return dlopen( filename, RTLD_LAZY | RTLD_GLOBAL ); }
		inline lt_dlhandle lt_dlopenext( const char* filename ) { return dlopen( filename, RTLD_LAZY | RTLD_GLOBAL ); }

		inline int lt_dlclose( lt_dlhandle handle ) { return dlclose( handle ); }

		inline lt_ptr lt_dlsym( lt_dlhandle handle, const char* name ) { return dlsym( handle, name ); }
#ifdef ANDROID
		inline const char* lt_dlerror() { return dlerror(); }
#else
		inline char* lt_dlerror() { return dlerror(); }
#endif

	}

#endif // __WINDOWS__

#endif // __DLLOADER_H__

