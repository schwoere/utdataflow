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
 * @ingroup clientserver
 * @file
 * Defines an abstract interface for bi-directional connections between clients and servers
 *
 * @author Daniel Pustka <daniel.pustka@in.tum.de>
 */ 

#ifndef __UBITRACK_CLIENTSERVER_CLIENTSERVERCONNECTION_H_INCLUDED__
#define __UBITRACK_CLIENTSERVER_CLIENTSERVERCONNECTION_H_INCLUDED__
 
#include <vector>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <utDataflow.h>

namespace Ubitrack { namespace ClientServer {

/**
 * Interface for connections between clients and servers.
 */
class UTDATAFLOW_EXPORT ClientServerConnection
{
public:
	/** add a virtual destructor */
	virtual ~ClientServerConnection()
	{}
	
	/** type of buffers passed between client and server */
	typedef std::vector< char > BufferType;

	/** signature of callback functions when data is coming in */
	typedef boost::function< void( boost::shared_ptr< BufferType > buf ) > ReceiveHandler;
	
	/**
	 * Sends a packet of data to the other side (synchronously)
	 * @param buf data to be sent. 
	 */
	virtual void send( const BufferType& buf ) = 0;

	/**
	 * Sends a packet of data to the other side (synchronously)
	 * @param buf data to be sent. 
	 */
	virtual void send( const std::string& buf ) = 0;
	
	/**
	 * Set the callback function for (asynchronously) receiving data
	 * @param receiver the callback function
	 */
	virtual void setReceiver( ReceiveHandler receiver ) = 0;
	
	/** returns true if the connection is down */
	virtual bool badConnection() = 0;
	
	/** returns the name of the connection for logging etc. */
	virtual const std::string& getName() = 0;
};


} } // namespace Ubitrack::ClientServer

#endif
