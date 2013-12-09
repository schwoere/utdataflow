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
 * Header file of a TCP-based interface for bi-directional connections between clients and servers
 *
 * @author Daniel Pustka <daniel.pustka@in.tum.de>
 */ 

#ifndef __UBITRACK_CLIENTSERVER_TCPCONNECTION_H_INCLUDED__
#define __UBITRACK_CLIENTSERVER_TCPCONNECTION_H_INCLUDED__
 
#include <boost/asio.hpp>
#include "ClientServerConnection.h"

using namespace boost;

namespace Ubitrack { namespace ClientServer {

/**
 * Implements the \c ClientServer interface using a simple TCP-based protocol.
 *
 * The protocol consists of an eight-byte header giving the payload size as a
 * hexadecial number, followed by the actual payload. Each connection can handle 
 * an arbitrary number of messages in both directions.
 *
 * @par Example message
 * \verbatim 0000000BHello world \endverbatim
 *
 * @par Remarks
 * \li For security reasons, the maximum message size is 1 Mb
 * \li Packets with a size of 00000000 are considered keep-alive-packets and not 
 *     passed to the application.
 */
class UTDATAFLOW_EXPORT TcpConnection
	: public ClientServerConnection
{
public:

	/** 
	 * Creates a connection object.
	 * @param pSocket an initialized asio socket
	 */
	TcpConnection( boost::shared_ptr< boost::asio::ip::tcp::socket > pSocket );

	/**
	 * Sends a packet of data to the other side (synchronously).
	 * Implements the \c ClientServerConnection interface
	 * @param buf data to be sent. 
	 */
	void send( const BufferType& buf );

	/**
	 * Sends a packet of data to the other side (synchronously).
	 * Implements the \c ClientServerConnection interface
	 * @param buf data to be sent. 
	 */
	void send( const std::string& buf );
	
	/**
	 * Set the callback function for (asynchronously) receiving data.
	 * Implements the \c ClientServerConnection interface
	 * @param receiver the callback function
	 */
	void setReceiver( ReceiveHandler receiver );
	
	/** 
	 * returns true if the connection is down.
	 * Implements the \c ClientServerConnection interface
	 */
	bool badConnection();

	/** returns the name of the connection for logging etc. */
	const std::string& getName();

protected:
	/** starts a read operation for a message size */
	void readSize();
	
	/** starts a read operation for payload data */
	void readData( std::size_t size );
	
	/** creates a size field */
	std::string makeSize( std::size_t nSize );
	
	/** checks if the size is valid */
	bool checkSize( std::size_t nSize );

	/** asio callback when a size block is received */
	void handleSizeRead( const boost::system::error_code error, std::size_t bytes_transferred );
	
	/** asio callback when a data block is received */
	void handleDataRead( const boost::system::error_code error, std::size_t bytes_transferred );

	/** state of the socket */
	bool m_bBadSocket;
	
	/** name of the socket endpoint */
	std::string m_sName;

	/** Client/Server callback */
	ReceiveHandler m_readHandler;
	
	/** pointer to the socket */
	boost::shared_ptr< boost::asio::ip::tcp::socket > m_pSocket;
	
	/** receive buffer */
	boost::shared_ptr< BufferType > m_pReadBuffer;
	
	/** next packet size receive buffer */
	char m_receivePacketSize[ 9 ];
};


} } // namespace Ubitrack::ClientServer

#endif
