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
 * Implementation of the event queue used for push communication
 *
 * @author Daniel Pustka <daniel.pustka@in.tum.de>
 */

#include <boost/bind.hpp>
#include <boost/scoped_ptr.hpp>
#include <log4cpp/Category.hh>
#include <utUtil/Exception.h>
#include <utMeasurement/Timestamp.h>
#include "Port.h"
#include "EventQueue.h"

// get loggers
static log4cpp::Category& logger( log4cpp::Category::getInstance( "Ubitrack.Dataflow.EventQueue" ) );
static log4cpp::Category& eventLogger( log4cpp::Category::getInstance( "Ubitrack.Events.Dataflow.EventQueue" ) );

// minimum time between "events dropped" messages
static const unsigned long long g_eventsDroppedMessageInterval( 1000000000ll );

namespace Ubitrack { namespace Dataflow {

// the singleton event queue object
static boost::scoped_ptr< EventQueue > g_pEventQueue;
static int g_RefEventQueue = 0;

EventQueue& EventQueue::singleton()
{
    // race condition between network receiving thread and main thread
    static boost::mutex singletonMutex;
    boost::mutex::scoped_lock l( singletonMutex );

	// create a new singleton if necessary
	if ( !g_pEventQueue )
		g_pEventQueue.reset( new EventQueue );

	return *g_pEventQueue;
}

void EventQueue::destroyEventQueue()
{	
	g_pEventQueue.reset( 0 );
}


EventQueue::EventQueue()
	: m_State( state_stopped )
{
	// create a new thread
	m_pThread = boost::shared_ptr< boost::thread >( new boost::thread( boost::bind( &EventQueue::threadFunction, this ) ) );
}


EventQueue::~EventQueue()
{
	LOG4CPP_INFO( eventLogger, "Destroying EventQueue" );
	// tell thread to quit
	{
		boost::mutex::scoped_lock l( m_Mutex );
		m_State = state_end;
		m_NewEventCondition.notify_all();
	}

	// wait until thread has actually quit
	m_pThread->join();
	LOG4CPP_INFO( eventLogger, "Destroyed EventQueue" );
}


void EventQueue::start()
{
	LOG4CPP_NOTICE( logger, "Event queue started" );

	// tell thread to start
	boost::mutex::scoped_lock l( m_Mutex );
	m_State = state_running;
	m_NewEventCondition.notify_all();
}


void EventQueue::stop()
{
	LOG4CPP_DEBUG( logger, "Stopping event queue" );

	boost::mutex::scoped_lock l( m_Mutex );
	if ( m_State != state_stopped )
	{
		// tell thread to stop
		m_State = state_stopping;
		m_NewEventCondition.notify_all();

		// wait until thread has actually stopped
		while ( m_State != state_stopped )
			m_NewEventCondition.wait( l );
	}
	
	LOG4CPP_NOTICE( logger, "Event queue stopped" );
}


void EventQueue::queue( std::vector< QueueData >& events )
{
	boost::mutex::scoped_lock l( m_Mutex );

	std::vector<QueueData>::iterator pos = events.begin();
	std::vector<QueueData>::iterator end = events.end();

	LOG4CPP_DEBUG( eventLogger, "Received " << events.size() << " events: enqueueing." );

	for ( ; pos != end; pos++ ) {

		LOG4CPP_DEBUG( eventLogger, "Queueing event to port "
			<< ( pos->pReceiverInfo ? pos->pReceiverInfo->pPort->fullName() : "(unknown)" ) 
			<< ", priority=" << pos->priority );

		// sort event into queue
		if ( m_Queue.empty() )
			m_Queue.push_back( *pos );
		else
			// adding an event to the back will be a common case
			if ( m_Queue.back().priority <= pos->priority )
				m_Queue.push_back( *pos );
			else
			{
				// ... as will be adding near the front
				QueueType::iterator it = m_Queue.begin();
				while ( it->priority < pos->priority )
					it++;
				m_Queue.insert( it, *pos );
			}

		if ( pos->pReceiverInfo )
			pos->pReceiverInfo->nQueuedEvents++;
	}

	// Patrick Maier: Prevents the queue to be filled up when the receiving thread is paused (.NET, Java, etc.)
	// remove events from the queue if it is too long
	// just to be sure that there is something
	if ( !m_Queue.empty() ) {
			while ( m_Queue.front().pReceiverInfo && m_Queue.front().pReceiverInfo->nMaxQueueLength > 0 &&
				m_Queue.front().pReceiverInfo->nQueuedEvents > m_Queue.front().pReceiverInfo->nMaxQueueLength )
			{
				// limit number of "events dropped" messages in WARN level
				static unsigned nDropMessages = 0;
				static Measurement::Timestamp lastDropMessageTime = 0;
				
				if ( Measurement::now() > lastDropMessageTime + g_eventsDroppedMessageInterval )
				{
					LOG4CPP_WARN( eventLogger, "Queue too long, dropping event for "
						<< ( m_Queue.front().pReceiverInfo ? m_Queue.front().pReceiverInfo->pPort->fullName() : "(unknown)" )
						<< " (skipped " << nDropMessages << " messages)" );
					nDropMessages = 0;
					lastDropMessageTime = Measurement::now();
				}
				else
				{
					nDropMessages++;
					LOG4CPP_DEBUG( eventLogger, "Queue too long, dropping event for "
						<< ( m_Queue.front().pReceiverInfo ? m_Queue.front().pReceiverInfo->pPort->fullName() : "(unknown)" ) );
				}

				if ( m_Queue.front().pReceiverInfo )
					m_Queue.front().pReceiverInfo->nQueuedEvents--;
				m_Queue.pop_front();
			}
	}

	// notify waiting thread
	if ( m_State == state_running )
		m_NewEventCondition.notify_all();
}


void EventQueue::removeComponent( const Component* pComponent )
{
	LOG4CPP_DEBUG( logger, "Removing events for component " << pComponent->getName() );

	boost::mutex::scoped_lock l( m_Mutex );

	// remove all events that belong to the component
	for ( QueueType::iterator it = m_Queue.begin(); it != m_Queue.end(); )
		if ( it->pReceiverInfo && &it->pReceiverInfo->pPort->getComponent() == pComponent )
		{
			it->pReceiverInfo->nQueuedEvents--;
			it = m_Queue.erase( it );
		}
		else
			it++;
}


void EventQueue::clear()
{
	LOG4CPP_DEBUG( logger, "Removing all events from queue" );

	boost::mutex::scoped_lock l( m_Mutex );

	// decrement all event counters
	for ( QueueType::iterator it = m_Queue.begin(); it != m_Queue.end(); it++ )
		if ( it->pReceiverInfo )
			it->pReceiverInfo->nQueuedEvents--;

	// remove all events
	m_Queue.clear();

	LOG4CPP_DEBUG( logger, "All events removed" );
}



void EventQueue::dispatchNow()
{
	while ( true )
	{
		// need this type of logic, as boost is very strict with locking...
		EventType dispatchEvent;
		ReceiverInfo* pReceiverInfo( 0 );

		{
			// lock the mutex
			boost::mutex::scoped_lock l( m_Mutex );

			if ( m_Queue.empty() )
				return;

			// fetch and remove first event from the queue
			LOG4CPP_TRACE( eventLogger, "dispatchNow(): dispatching event for " 
				<< ( m_Queue.front().pReceiverInfo ? m_Queue.front().pReceiverInfo->pPort->fullName() : "(unknown)" ) );

			if ( m_Queue.front().pReceiverInfo && m_Queue.front().pReceiverInfo->nMaxQueueLength > 0 &&
				m_Queue.front().pReceiverInfo->nQueuedEvents > m_Queue.front().pReceiverInfo->nMaxQueueLength )
			{
				// limit number of "events dropped" messages in WARN level
				static unsigned nDropMessages = 0;
				static Measurement::Timestamp lastDropMessageTime = 0;
				
				if ( Measurement::now() > lastDropMessageTime + g_eventsDroppedMessageInterval )
				{
					LOG4CPP_WARN( eventLogger, "Queue too long, dropping event for "
						<< ( m_Queue.front().pReceiverInfo ? m_Queue.front().pReceiverInfo->pPort->fullName() : "(unknown)" )
						<< " (skipped " << nDropMessages << " messages)" );
					nDropMessages = 0;
					lastDropMessageTime = Measurement::now();
				}
				else
				{
					nDropMessages++;
					LOG4CPP_DEBUG( eventLogger, "Queue too long, dropping event for "
						<< ( m_Queue.front().pReceiverInfo ? m_Queue.front().pReceiverInfo->pPort->fullName() : "(unknown)" ) );
				}
			}
			else
			{
				pReceiverInfo = m_Queue.front().pReceiverInfo;
				dispatchEvent = m_Queue.front().event;
			}
			
			if ( m_Queue.front().pReceiverInfo )
				m_Queue.front().pReceiverInfo->nQueuedEvents--;
			m_Queue.pop_front();
		}

		// dispatch the event
		if ( dispatchEvent )
		{
			try
			{
				if ( pReceiverInfo && pReceiverInfo->pMutex )
				{
					// lock the mutex
					ReceiverInfo::MutexType::scoped_lock l( *pReceiverInfo->pMutex );
					dispatchEvent();
				}
				else
					// no mutex
					dispatchEvent();
			}
			catch ( const Ubitrack::Util::Exception& e )
			{
				LOG4CPP_WARN( eventLogger, e );
			}
			catch ( const std::exception& e )
			{
				LOG4CPP_WARN( eventLogger, "Caught std::exception: " << e.what() );
			}
			catch ( ... )
			{
				LOG4CPP_WARN( eventLogger, "Caught unknown exception" );
			}
		}
	}
}


void EventQueue::threadFunction()
{
	// Note: In case we want to extend this to multiple threads later, we need to make sure that only
	// events of SAME PRIORITY are processed simultaneously, unless they are on unrelated paths!
	
	while ( true )
	{
		// need this type of logic, as boost is very strict with locking...
		EventType dispatchEvent;
		ReceiverInfo* pReceiverInfo( 0 );		
		{
			// lock the mutex
			boost::mutex::scoped_lock l( m_Mutex );

			if ( m_State == state_running && !m_Queue.empty() )
			{
				// dispatch an event
				LOG4CPP_DEBUG( eventLogger, "dispatching event for "
					<< ( m_Queue.front().pReceiverInfo ? m_Queue.front().pReceiverInfo->pPort->fullName() : "(unknown)" ) );
					
				if ( m_Queue.front().pReceiverInfo && m_Queue.front().pReceiverInfo->nMaxQueueLength > 0 &&
					m_Queue.front().pReceiverInfo->nQueuedEvents > m_Queue.front().pReceiverInfo->nMaxQueueLength )
				{
					// limit number of "events dropped" messages in WARN level
					static unsigned nDropMessages = 0;
					static Measurement::Timestamp lastDropMessageTime = 0;
					
					if ( Measurement::now() > lastDropMessageTime + g_eventsDroppedMessageInterval )
					{
						LOG4CPP_WARN( eventLogger, "Queue too long, dropping event for "
							<< ( m_Queue.front().pReceiverInfo ? m_Queue.front().pReceiverInfo->pPort->fullName() : "(unknown)" )
							<< " (skipped " << nDropMessages << " messages)" );
						nDropMessages = 0;
						lastDropMessageTime = Measurement::now();
					}
					else
					{
						nDropMessages++;
						LOG4CPP_DEBUG( eventLogger, "Queue too long, dropping event for "
							<< ( m_Queue.front().pReceiverInfo ? m_Queue.front().pReceiverInfo->pPort->fullName() : "(unknown)" ) );
					}
				}
				else
				{
					pReceiverInfo = m_Queue.front().pReceiverInfo;
					dispatchEvent = m_Queue.front().event;
				}
					
				if ( m_Queue.front().pReceiverInfo )
					m_Queue.front().pReceiverInfo->nQueuedEvents--;
				m_Queue.pop_front();
			}
			else if ( m_State == state_end )
			{
				LOG4CPP_DEBUG( logger, "Ending event queue thread");
				// end this thread
				return;
			}
			else if ( m_State == state_stopping )
			{
				LOG4CPP_DEBUG( logger, "Ending other queue threads");
				// stop and wait for something to happen
				m_State = state_stopped;

				// tell other threads we have stopped
				m_NewEventCondition.notify_all();
			}
			else
			{				
				// wait for something to happen
				m_NewEventCondition.wait( l );
			}
		}

		// dispatch the event if one was taken from the queue
		if ( dispatchEvent )
		{
			try
			{
				if ( pReceiverInfo && pReceiverInfo->pMutex )
				{
					// lock the mutex
					ReceiverInfo::MutexType::scoped_lock l( *pReceiverInfo->pMutex );
					dispatchEvent();
				}
				else
					// no mutex
					dispatchEvent();
			}
			catch ( const Ubitrack::Util::Exception& e )
			{
				LOG4CPP_WARN( eventLogger, e );
			}
			catch ( const std::exception& e )
			{
				LOG4CPP_WARN( eventLogger, "Caught std::exception: " << e.what() << " occurred when pushing on port " << pReceiverInfo->pPort->fullName() );
			}
			catch ( ... )
			{
				LOG4CPP_WARN( eventLogger, "Caught unknown exception" << " when pushing on port " << pReceiverInfo->pPort->fullName() );
			}
		}
	}
}


} } // namespace Ubitrack::Dataflow
