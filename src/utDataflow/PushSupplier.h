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
 * The PushSupplier port
 *
 * @author Daniel Pustka <daniel.pustka@in.tum.de>
 */

#ifndef __Ubitrack_Dataflow_PushSupplier_INCLUDED__
#define __Ubitrack_Dataflow_PushSupplier_INCLUDED__

#include <list>
#include <algorithm>
#include <typeinfo>
#include <boost/bind.hpp>
#include <utUtil/Exception.h>


#include "Port.h"
#include "Component.h"
#include "PushConsumer.h"
#include "EventQueue.h"
#include "EventTypeTraits.h"

namespace Ubitrack { namespace Dataflow {


/**
 * \internal
 * Implements the core functionality of a push supplier.
 * Used by the PushSupplier port and other ports.
 *
 * @param EventType type of measurements to be pushed
 */
template< class EventType >
class PushSupplierCore
{
public:
	/**
	 * Send events to the connected PushConsumers.
	 * Events are not sent directly, but stored in a queue to prevent deep recursions.
	 *
	 * @param rEvent Measurement to be sent
	 */
	void send( const EventType& rEvent );

	/**
	 * returns true if at least one consumer is connected
	 */
	bool isConnected() const
	{ return !m_pushConsumers.empty(); }
	
protected:

	/**
	 * Add a consumer to the list.
	 *
	 * @param rConsumer a reference to an object that must be dynamic_castable to a \c PushConsumerCore< EventType >&
	 */
	template< class OtherSide >
	void addPushConsumer( OtherSide& rConsumer );

	/** remove a consumer to the list */
	template< class OtherSide >
	void removePushConsumer( OtherSide& rConsumer );

	/** type shortcut for list of consumers */
	typedef std::list< PushConsumerCore< EventType >* > ConsumerList;

	/** the list of consumers */
	ConsumerList m_pushConsumers;

};


template< class EventType > template< class OtherSide >
void PushSupplierCore< EventType >::addPushConsumer( OtherSide& rConsumer )
{
	try
	{				
		PushConsumerCore< EventType >& rTyped( dynamic_cast< PushConsumerCore< EventType >& >( rConsumer ) );
		m_pushConsumers.push_back( &rTyped );
	}
	catch ( const std::bad_cast& )
	{
		UBITRACK_THROW( "PushSupplier can only connect to a PushConsumer of equal EventType: " + std::string(typeid(this).name()) + " to " + std::string(typeid(rConsumer).name()));
	}
}


template< class EventType > template< class OtherSide >
void PushSupplierCore< EventType >::removePushConsumer( OtherSide& rConsumer )
{
	PushConsumerCore< EventType >& rTyped( dynamic_cast< PushConsumerCore< EventType >& >( rConsumer ) );
	for ( typename ConsumerList::iterator it = m_pushConsumers.begin(); it != m_pushConsumers.end(); )
		if ( *it == &rTyped )
			it = m_pushConsumers.erase( it );
		else
			it++;
}


template< class EventType >
void PushSupplierCore< EventType >::send( const EventType& rEvent )
{
	// here I am lazy and just assume that timestamps are so fine-grained that I can add the 
	// priority to the timestamp without disturbing the time order...

	// create list of events for each consumer
	std::vector< EventQueue::QueueData > events;
	for ( typename ConsumerList::iterator it = m_pushConsumers.begin(); it != m_pushConsumers.end(); it++ ) {
		
		events.push_back( EventQueue::QueueData( &(*it)->getReceiverInfo(), boost::bind( (*it)->getSlot(), EventType( rEvent ) ),
			EventTypeTraits< EventType >().getPriority( rEvent ) + (*it)->getPort().getComponent().getEventPriority() ) );		
			
	}
	
	// enqueue it all in one go
	EventQueue::singleton().queue( events );
}


/**
 * \ingroup dataflow_framework
 * Implements a port that pushes events to multiple PushConsumers
 *
 * @param EventType type of measurements to be pushed
 */
template< class EventType >
class PushSupplier
	: public Port
    , public PushSupplierCore< EventType >
{
public:
	/**
	 * Constructor.
	 *
	 * @param sName the name of this port
	 * @param rParent reference to the component this port belongs to
	 */
	PushSupplier( const std::string& sName, Component& rParent )
		: Port( sName, rParent )
	{}

	//@{
	/** implements the Port interface */
	void connect( Port& rOther );

	void disconnect( Port& rOther );
	//@}
};


template< class EventType >
void PushSupplier< EventType >::connect( Port& rOther )
{
	// add slot of other side to consumer list
	PushSupplierCore< EventType >::addPushConsumer( rOther );
}


template< class EventType >
void PushSupplier< EventType >::disconnect( Port& rOther )
{
	// remove slot from consumer list
	PushSupplierCore< EventType >::removePushConsumer( rOther );
}


} } // namespace Ubitrack::Dataflow

#endif
