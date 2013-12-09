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
 * Header file for \c TriggerOutPort, the output port for triggered components
 *
 * @author Daniel Pustka <daniel.pustka@in.tum.de>
 */
#ifndef __UBITRACK_DATAFLOW_TRIGGEROUTPORT_H_INCLUDED__
#define __UBITRACK_DATAFLOW_TRIGGEROUTPORT_H_INCLUDED__

#include <assert.h>
#include "TriggerComponent.h"
#include "PushSupplier.h"
#include "PullSupplier.h"
#include <utMeasurement/Measurement.h>
#include <log4cpp/Category.hh>

namespace Ubitrack { namespace Dataflow {

/**
 * @ingroup dataflow_framework
 *
 * Class that provides an output port for use in a \c TriggerComponent.
 *
 * @param EventType type of events to flow into this port
 */
template< class EventType >
class TriggerOutPort
	: public Port
	, public PushSupplierCore< EventType >
	, public PullSupplierCore< EventType >
{
public:
	/**
	 * Constructor.
	 *
	 * @param sName name of the port
	 * @param rParent reference to the \c TriggerComponent this port belongs to
	 */
	TriggerOutPort( const std::string& sName, TriggerComponent& rParent );

	//@{
	/** implements the Port interface */
	void connect( Port& rOther );
	void disconnect( Port& rOther );
	//@}

	/**
	 * Send result to receivers.
	 *
	 * @param rEvent The data to send.
	 */
	void send( const EventType& rEvent )
	{
		LOG4CPP_DEBUG( m_logger, fullName() << " sending event" );

		if ( m_bPush )
			PushSupplierCore< EventType >::send( rEvent );
		else
			m_measurement = rEvent;
	}

protected:
	/** is this a push port? */
	bool m_bPush;

	/** the stored measurement */
	EventType m_measurement;

	/** receives pull requests from a \c PullConsumer */
	EventType pullRequest( Measurement::Timestamp );
	
	/** the pull time */
	Measurement::Timestamp m_timestamp;

	/** reference to logger */
	log4cpp::Category& m_logger;
};


template< class EventType >
TriggerOutPort< EventType >::TriggerOutPort( const std::string& sName, TriggerComponent& rParent )
	: Port( sName, rParent )
	, PullSupplierCore< EventType >( boost::bind( &TriggerOutPort< EventType >::pullRequest, this, _1 ) )
	, m_bPush( rParent.isPortPush( sName ) )
	, m_logger( log4cpp::Category::getInstance( "Ubitrack.Events.Dataflow.TriggerOutPort" ) )
{
	rParent.addTriggerOutput( m_bPush );
}


template< class EventType >
EventType TriggerOutPort< EventType >::pullRequest( Measurement::Timestamp t )
{
	LOG4CPP_DEBUG( m_logger, fullName() << " got pullRequest for " << t );

	// reset infos about this query
	m_timestamp = t;

	// trigger computation - should either throw an exception or put something in m_pMeasurement
	static_cast< TriggerComponent& >( m_rComponent ).triggerOut( t );

	// return computed result
	return m_measurement;
}


template< class EventType >
void TriggerOutPort< EventType >::connect( Port& rOther )
{
	if ( m_bPush )
		PushSupplierCore< EventType >::addPushConsumer( rOther );
}


template< class EventType >
void TriggerOutPort< EventType >::disconnect( Port& rOther )
{
	if ( m_bPush )
		PushSupplierCore< EventType >::removePushConsumer( rOther );
}

} } // namespace Ubitrack::Dataflow

#endif
