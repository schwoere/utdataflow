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
 * Header file for \c ExpansionInPort, the input port for time/space expansions of triggered components
 *
 * @author Daniel Pustka <daniel.pustka@in.tum.de>
 */
#ifndef __UBITRACK_DATAFLOW_EXPANSIONINPORT_H_INCLUDED__
#define __UBITRACK_DATAFLOW_EXPANSIONINPORT_H_INCLUDED__

#include <assert.h>
#include "TriggerComponent.h"
#include "PushConsumer.h"
#include "PullConsumer.h"
#include <utMeasurement/Measurement.h>
#include <log4cpp/Category.hh>
#include <boost/bind.hpp>


namespace Ubitrack { namespace Dataflow {

/**
 * Class that provides input ports of a \c TriggerComponent that can be push or pull and time or space-expanded.
 *
 * The port can be connected to push/pull suppliers of either Measurement< EventType > or Measurement< std::vector< EventType > >.
 *
 * @param EventType Type of events received via this port, without the \c Measurement wrapper, which is added automatically.
 */
template< class EventType >
class ExpansionInPort
	: public TriggerInPortBase
	, public PushConsumerCore< Measurement::Measurement< EventType > >
	, public PushConsumerCore< Measurement::Measurement< std::vector< EventType > > >
	, public PullConsumerCore< Measurement::Measurement< EventType > >
	, public PullConsumerCore< Measurement::Measurement< std::vector< EventType > > >
{
public:
	/** type of time-expanded events */
	typedef Measurement::Measurement< EventType > SingleEvent;

	/** type of space-expanded events */
	typedef Measurement::Measurement< std::vector< EventType > > VectorEvent;

	/**
	 * Constructor.
	 *
	 * @param sName name of the port
	 * @param rParent reference to the \c TriggerComponent this port belongs to
	 */
	ExpansionInPort( const std::string& sName, TriggerComponent& rParent, int triggerGroup = -1 );

	virtual boost::shared_ptr< TriggerInPortBase > newSlave( const std::string& name, int triggerGroup );
	
	//@{
	/** implements the Port interface */
	void connect( Port& rOther );
	void disconnect( Port& rOther );
	//@}

	/** retrieves the stored measurement vector from this port */
	const VectorEvent& get() const
	{ return m_vectorMeasurement; }

	/** pull an event from a connected pushSupplier */
	void pull( Measurement::Timestamp );

	/** If the port is time expanded, add the stored measurement to the list. */
	void storeMeasurement();

	/** if the port is push, return true if there are events waiting for this port */
	bool eventsWaiting();
	
	/** make this port a slave of another port */
	void setMaster( ExpansionInPort< EventType >* pMaster )
	{ m_pMaster = pMaster; }
	
protected:
	/** stores the last received time-expanded measurement */
	SingleEvent m_singleMeasurement;

	/** stores the current measurement vector */
	VectorEvent m_vectorMeasurement;

	/** receives pushs of time-expanded single events */
	void receivePushSingle( const SingleEvent& e );

	/** receives pushs of space-expanded vectors of events */
	void receivePushVector( const VectorEvent& e );

	/** called when a slave receives a measurement */
	void slaveTrigger( Measurement::Timestamp t );
	
	/** reference to logger */
	log4cpp::Category& m_logger;
	
	/** pointer to master component (if this is a slave) */
	ExpansionInPort< EventType >* m_pMaster;
	
	typedef std::vector< ExpansionInPort< EventType >* > SlaveList;
	
	/** pointer to slave component (if this is a master) */
	SlaveList m_slaves;
};


template< class EventType >
ExpansionInPort< EventType >::ExpansionInPort( const std::string& sName, TriggerComponent& rParent, int triggerGroup )
/*
 * Time-expanded input ports by default reside in a seperate trigger
 * group (1) to decouple them from other triggered input ports. The
 * default trigger group for non-space expanded input ports and normal
 * trigger input ports is (0). When a triggered output port is pulled,
 * this special group (1) of triggered input ports is NOT pulled! Only
 * push events may trigger them. Other input ports in group (0),
 * however, ARE pulled. See also receivePushSingle() below.
 */
: TriggerInPortBase( sName, rParent, triggerGroup >= 0 ? triggerGroup : ( rParent.isTimeExpansion() ? 1 : 0 ) )
	, PushConsumerCore< SingleEvent >( *this, boost::bind( &ExpansionInPort< EventType >::receivePushSingle, this, _1 ), &rParent.getMutex() )
	, PushConsumerCore< VectorEvent >( *this, boost::bind( &ExpansionInPort< EventType >::receivePushVector, this, _1 ), &rParent.getMutex() )
	, m_vectorMeasurement( boost::shared_ptr< std::vector< EventType > >( new std::vector< EventType > ) )
	, m_logger( log4cpp::Category::getInstance( "Ubitrack.Events.Dataflow.ExpansionInPort" ) )
	, m_pMaster( 0 )
{
	LOG4CPP_TRACE( m_logger, fullName() << " expansion input port created with triggerGroup " << triggerGroup << " ; is time expansion: " << rParent.isTimeExpansion() );
}


template< class EventType >
boost::shared_ptr< TriggerInPortBase > ExpansionInPort< EventType >::newSlave( const std::string& name, int triggerGroup )
{
	ExpansionInPort< EventType >* pNew = new ExpansionInPort< EventType >( name, 
		static_cast< TriggerComponent& >( getComponent() ), triggerGroup );
	pNew->setMaster( this );
	m_slaves.push_back( pNew );
	return boost::shared_ptr< TriggerInPortBase >( pNew );
}


template< class EventType >
void ExpansionInPort< EventType >::receivePushSingle( const typename ExpansionInPort< EventType >::SingleEvent& e )
{
	LOG4CPP_DEBUG( m_logger, fullName() << " received single measurement, trigger the expansion group" );

	m_timestamp = e.time();
	m_singleMeasurement = e;

	// trigger the group
	if ( m_pTriggerGroup->trigger( m_timestamp ) ) 
	{
		// store measurements in master list
		m_pTriggerGroup->storeMeasurements();
		
		if ( m_pMaster ) 
		{
			LOG4CPP_TRACE( m_logger, fullName() << " master available" );
			m_pMaster->slaveTrigger( m_timestamp );
		}
		// PK: This seems to be necessary for space expansion to omit
		// a call to triggerIn if this is the master port and it has slaves
		else if ( ! m_slaves.empty() ) 
		{
			// do nothing
		}
		// Default behaviour for time expansion. There are no slaves, so we trigger the master port (this port!)
		else 
		{
			// Trigger the whole component. This will implicitly also trigger trigger group 0, if the output is push!
			LOG4CPP_TRACE( m_logger, fullName() << " no master available, directly trigger the component" );
			static_cast< TriggerComponent& >( m_rComponent ).triggerIn( this );
		}
	}
}


template< class EventType >
void ExpansionInPort< EventType >::receivePushVector( const typename ExpansionInPort< EventType >::VectorEvent& e )
{
	LOG4CPP_DEBUG( m_logger, fullName() << " received vector measurement at " << e.time() );
	assert( m_bPush );

	m_timestamp = e.time();
	m_vectorMeasurement = e;

	// trigger the group
	if ( m_pTriggerGroup->trigger( m_timestamp ) ) 
	{
		// store measurements in master list
		m_pTriggerGroup->storeMeasurements();
		
		if ( m_pMaster )
			m_pMaster->slaveTrigger( m_timestamp );
		else
			static_cast< TriggerComponent& >( m_rComponent ).triggerIn( this );
	}
}


template< class EventType >
void ExpansionInPort< EventType >::slaveTrigger( Measurement::Timestamp t )
{
	// check if any push slave has measurements waiting and omit computation in this case
	for ( typename SlaveList::iterator it = m_slaves.begin(); it != m_slaves.end(); it++ )
		if ( (*it)->eventsWaiting() )
			return;
	
	static_cast< TriggerComponent& >( m_rComponent ).triggerIn( this );
}


template< class EventType >
void ExpansionInPort< EventType >::pull( Measurement::Timestamp t )
{
	LOG4CPP_DEBUG( m_logger, fullName() << " pull");
	assert( !m_bPush );

	if ( m_slaves.empty() )
	{
		if ( PullConsumerCore< SingleEvent >::m_pullSupplier )
		{
			LOG4CPP_DEBUG( m_logger, fullName() << " no slaves, single event pull supplier" );
			m_singleMeasurement = PullConsumerCore< SingleEvent >::get( t );
		}
		else
		{
			LOG4CPP_DEBUG( m_logger, fullName() << " no slaves, vector event pull supplier" );
			m_vectorMeasurement = PullConsumerCore< VectorEvent >::get( t );
		}
	}
	else
	{
		LOG4CPP_DEBUG( m_logger, fullName() << " slaves" );
		
		// port duplication space expansion: only check timestamp
		if ( m_timestamp != t )
		{
			//###PK This is thrown! The master port inputA is pulled first, m_timestamp is however 0.
			LOG4CPP_DEBUG( m_logger, fullName() << " wrong timestamp for port duplication space expansion, t=" << t << ", m_timestamp=" << m_timestamp );
			UBITRACK_THROW( "wrong timestamp for port duplication space expansion" );
		}

		//###PK Here seems to be missing something! Who pulls all the measurements and puts them into m_vectorMeasurement?
	}

	m_timestamp = t;
}


template< class EventType >
void ExpansionInPort< EventType >::storeMeasurement()
{
	LOG4CPP_DEBUG( m_logger, fullName() << " storing measurement" );
	if ( m_pMaster )
	{
		LOG4CPP_TRACE( m_logger, fullName() << " slave port" );

		// clear the master's list if the timestamp is different
		if ( m_timestamp != m_pMaster->m_timestamp )
		{
			LOG4CPP_TRACE( m_logger, fullName() << " reset" );

			// trigger computation if we forgot to do this (without this, computation may be skipped if the event queue is well filled)
			//if ( m_pMaster->m_timestamp != m_pMaster->m_lastTriggerTimestamp )
			//  static_cast< TriggerComponent& >( m_rComponent ).triggerIn( m_pMaster );
				
			m_pMaster->m_timestamp = m_timestamp;
			m_pMaster->m_vectorMeasurement.reset( new std::vector< EventType > );
		}
		
		// append single measurement or whole vector to master list
		if ( m_singleMeasurement ) {
			LOG4CPP_TRACE( m_logger, fullName() << " single measurement" );

			m_pMaster->m_vectorMeasurement->push_back( *m_singleMeasurement );
		}
		else {
			LOG4CPP_TRACE( m_logger, fullName() << " vector measurement" );

			m_pMaster->m_vectorMeasurement->insert( m_pMaster->m_vectorMeasurement->end(), 
				m_vectorMeasurement->begin(), m_vectorMeasurement->end() );
		}
	}
	else {
		LOG4CPP_TRACE( m_logger, fullName() << " master port" );

		// no master -> append single measurement to own list
		if ( m_singleMeasurement ) {
			LOG4CPP_TRACE( m_logger, fullName() << " single measurement" );
			m_vectorMeasurement->push_back( *m_singleMeasurement );
		}	
	}
}


template< class EventType >
bool ExpansionInPort< EventType >::eventsWaiting()
{
	return PushConsumerCore< SingleEvent >::getQueuedEvents() != 0 
		|| PushConsumerCore< VectorEvent >::getQueuedEvents() != 0;
}


template< class EventType >
void ExpansionInPort< EventType >::connect( Port& rOther )
{
	if ( !isPush() )
	{
		try
		{			
			PullConsumerCore< SingleEvent >::setPullSupplier( rOther, &rOther.getComponent().getMutex() );
		}
		catch ( ... )
		{			
			PullConsumerCore< VectorEvent >::setPullSupplier( rOther, &rOther.getComponent().getMutex() );
		}
	}
}


template< class EventType >
void ExpansionInPort< EventType >::disconnect( Port& )
{
	if ( !isPush() )
	{
		try
		{			
			PullConsumerCore< SingleEvent >::removePullSupplier();
		}
		catch ( ... )
		{
			PullConsumerCore< VectorEvent >::removePullSupplier();
		}
	}
}

} } // namespace Ubitrack::Dataflow

#endif
