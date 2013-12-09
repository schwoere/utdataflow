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
 * The PullConsumer port
 *
 * @author Daniel Pustka <daniel.pustka@in.tum.de>
 */

#ifndef __Ubitrack_Dataflow_PullConsumer_INCLUDED__
#define __Ubitrack_Dataflow_PullConsumer_INCLUDED__

#include <string>
#include <typeinfo>
#include <boost/shared_ptr.hpp>

#include "Port.h"
#include "PullSupplier.h"
#include <utMeasurement/Timestamp.h>
#include <utUtil/Exception.h>

namespace Ubitrack { namespace Dataflow {

// external class declarations
class Component;


/**
 * \internal
 * Implements the core functionality of a pull consumer.
 * Used by the PullConsumer port and other ports.
 *
 * @param EventType type of measurements to be queried
 */
template< class EventType > class PullConsumerCore
{
public:
	/**
	 * Queries a measurement from the connected supplier.
	 * Throws a \c Ubitrack::Util::Exception if unconnected
	 *
	 * @param t time the measurement is requested for
	 * @return pointer to the result, may be empty if no measurement is available
	 */
	EventType get( Ubitrack::Measurement::Timestamp t )
	{
		// check if connected
		if ( !m_pullSupplier )
			UBITRACK_THROW( "PullConsumer not connected" );

		// fetch and return a result from supplier
		if ( m_pMutex )
		{
			MutexType::scoped_lock l( *m_pMutex );
			return m_pullSupplier( t );
		}
		else
			return m_pullSupplier( t );
	}

protected:
	/** type of mutex to lock */
	typedef boost::recursive_mutex MutexType;

	/**
	 * Connect to \c PullSupplierCore
	 * @param rSupplier Reference that must be \c dynamic_castable to a \c PullSupplierCore< EventType >
	 */
	template< class OtherSide >
	void setPullSupplier( OtherSide& rSupplier, MutexType* pMutex = 0 );

	/** disconnect from supplier */
	void removePullSupplier();

	/** the supplier function */
	typename PullSupplierCore< EventType >::FunctionType m_pullSupplier;
	
	/** pointer to mutex to lock before calling */
	MutexType* m_pMutex;
};


template< class EventType > template< class OtherSide >
void PullConsumerCore< EventType >::setPullSupplier( OtherSide& rSupplier, typename PullConsumerCore< EventType >::MutexType* pMutex )
{
	// already connected to a supplier?
	if ( m_pullSupplier )
		UBITRACK_THROW( "PullConsumer can only be connected to ONE supplier" );

	// convert & check type of other side
	try
	{
		// store pointer to supplier function
		m_pullSupplier = dynamic_cast< PullSupplierCore< EventType >& >( rSupplier ).getFunction();
		m_pMutex = pMutex;
	}
	catch ( const std::bad_cast&)
	{
		UBITRACK_THROW( std::string("PullConsumer can only connect to a PullSupplier of equal EventType:").append(typeid(EventType).name()).append(" other:").append(typeid(rSupplier).name()) );
	}

	if ( !m_pullSupplier )
		UBITRACK_THROW( "Bad PullSupplier" );
}


template< class EventType >
void PullConsumerCore< EventType >::removePullSupplier()
{
	// clear function pointer
	m_pullSupplier.clear();
	m_pMutex = 0;
}


/**
 * @ingroup dataflow_framework
 * Implements a port that can be used to query measurements from a PullSupplier.
 *
 * @param EventType type of measurements to be queried
 */
template< class EventType > class PullConsumer
	: public Port
	, public PullConsumerCore< EventType >
{
public:
	/**
	 * Constructor.
	 *
	 * @param sName name of the port
	 * @param rParent reference to the Component this port belongs to
	 */
	PullConsumer( const std::string& sName, Component& rParent );

	//@{
	/** implements the Port interface */
	void connect( Port& rOther );
	void disconnect( Port& rOther );
	//@}

	/** has this port been connected in the dataflow? */
	bool isConnected();

};


template< class EventType >
PullConsumer< EventType >::PullConsumer( const std::string& sName, Component& rParent )
	: Port( sName, rParent )
{}


template< class EventType >
void PullConsumer< EventType >::connect( Port& rOther )
{
	PullConsumerCore< EventType >::setPullSupplier( rOther, &rOther.getComponent().getMutex() );
}


template< class EventType >
void PullConsumer< EventType >::disconnect( Port& rOther )
{
	PullConsumerCore< EventType >::removePullSupplier();
}


template< class EventType >
bool PullConsumer< EventType >::isConnected( )
{
	return this->m_pullSupplier;
}

} } // namespace Ubitrack::Dataflow

#endif
