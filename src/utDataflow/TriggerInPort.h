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
 * Header file for \c TriggerInPort, the input port for triggered components
 *
 * @author Daniel Pustka <daniel.pustka@in.tum.de>
 */

#ifndef __UBITRACK_DATAFLOW_TRIGGERINPORT_H_INCLUDED__
#define __UBITRACK_DATAFLOW_TRIGGERINPORT_H_INCLUDED__

#include <assert.h>
#include "TriggerComponent.h"
#include "PushConsumer.h"
#include "PullConsumer.h"
#include <utMeasurement/Measurement.h>
#include <log4cpp/Category.hh>
#include <boost/bind.hpp>

namespace Ubitrack { namespace Dataflow {

/**
 * @ingroup dataflow_framework
 *
 * Port class for input ports of a \c TriggerComponent
 *
 * @param EventType type of events to be passed over this port
 */
template< class EventType >
class TriggerInPort
	: public TriggerInPortBase
	, public PullConsumerCore< EventType >
	, public PushConsumerCore< EventType >
{
public:
	/**
	 * Constructor.
	 *
	 * @param sName name of the port
	 * @param rParent reference to the \c TriggerComponent this port belongs to
	 */
	TriggerInPort( const std::string& sName, TriggerComponent& rParent, int triggerGroup = 0 );

	//@{
	/** implements the Port interface */
	void connect( Port& rOther );
	void disconnect( Port& rOther );
	//@}

	/** retrieves the stored measurement from this port */
	const EventType& get() const
	{ return m_measurement; }

	/** pull an event from a connected pushSupplier */
	void pull( Ubitrack::Measurement::Timestamp );

	/** are there any events queued for this port? */
	bool eventsWaiting();
	
protected:

	/** one measurement is stored in this port */
	EventType m_measurement;
	
	/** called when an event is pushed in */
	void receivePush( const EventType& );

	/** prevent people from calling this method of PullConsumerCore directly */
	EventType get( Ubitrack::Measurement::Timestamp )
	{ assert( false ); return EventType(); }

	/** for logging */
	log4cpp::Category& m_logger;
};


template< class EventType >
TriggerInPort< EventType >::TriggerInPort( const std::string& sName, TriggerComponent& rParent, int triggerGroup )
	: TriggerInPortBase( sName, rParent, triggerGroup )
	, PushConsumerCore< EventType >( *this, boost::bind( &TriggerInPort< EventType >::receivePush, this, _1 ), &rParent.getMutex() )
	, m_logger( log4cpp::Category::getInstance( "Ubitrack.Events.Dataflow.TriggerInPort" ) )
{
}


template< class EventType >
void TriggerInPort< EventType >::pull( Measurement::Timestamp t )
{
	assert( !isPush() );
	m_measurement = PullConsumerCore< EventType >::get( t );
	m_timestamp = m_measurement.time();

	LOG4CPP_DEBUG( m_logger, fullName() << " pulled measurement at " << m_timestamp );
}


template< class EventType >
bool TriggerInPort< EventType >::eventsWaiting()
{
	return PushConsumerCore< EventType >::getQueuedEvents() != 0;
}


template< class EventType >
void TriggerInPort< EventType >::receivePush( const EventType& e )
{
	LOG4CPP_DEBUG( m_logger, fullName() << " received measurement at " << e.time() );

	m_measurement = e;
	m_timestamp = e.time();

	static_cast< TriggerComponent& >( m_rComponent ).triggerIn( this );
}


template< class EventType >
void TriggerInPort< EventType >::connect( Port& rOther )
{
	if ( !isPush() )
		PullConsumerCore< EventType >::setPullSupplier( rOther, &rOther.getComponent().getMutex() );
}


template< class EventType >
void TriggerInPort< EventType >::disconnect( Port& )
{
	if ( !isPush() )
		PullConsumerCore< EventType >::removePullSupplier();
}

} } // namespace Ubitrack::Dataflow

#endif
