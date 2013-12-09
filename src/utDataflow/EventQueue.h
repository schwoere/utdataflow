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
 * Header file for the event queue use for push communication.
 *
 * @author Daniel Pustka <daniel.pustka@in.tum.de>
 */

#ifndef __Ubitrack_Dataflow_EventQueue_INCLUDED__
#define __Ubitrack_Dataflow_EventQueue_INCLUDED__

#include <list>
#include <vector>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <boost/thread/condition.hpp>

#include <utDataflow.h>

namespace Ubitrack { namespace Dataflow {

// external class declarations
class Port;
class Component;


/**
 * @ingroup dataflow_framework
 * Event queue used for push communication in data flow networks.
 */
class UTDATAFLOW_EXPORT EventQueue
	: private boost::noncopyable
{
public:
	/** shortcut for type of events stored in the queue */
	typedef boost::function< void () > EventType;

	/** each event receiver must fill out one of these for queue length management, etc. */
	struct ReceiverInfo
	{
		/** type of mutex to use */
		typedef boost::recursive_mutex MutexType;

		/** constructor */
		ReceiverInfo( Port* _pPort, MutexType* _pMutex = 0, int _nMaxQueueLength = -1 )
			: pPort( _pPort )
			, pMutex( _pMutex )
			, nMaxQueueLength( _nMaxQueueLength )
			, nQueuedEvents( 0 )
		{}
		
		/** pointer to receiving port */
		Port* pPort;
		
		/** mutex to lock before dispatching events */
		MutexType* pMutex;
		
		/** maximum number of allowed events, negative if infinite queueing is allowed */
		int nMaxQueueLength;
		
		/** number of events queued */
		int nQueuedEvents;
	};
	
	/** Constructor */
	EventQueue();

	/** destructor, stops the event thread */
	~EventQueue();

	/** starts the event thread */
	void start();

	/** stops the event thread */
	void stop();

	/**
	 * information stored in the queue internally and passed to queue()
	 */
	struct QueueData
	{
		/**
		 * Pointer to ReceiverInfo struct.
		 * Used to remove events from the queue before removing a component.
		 */
		ReceiverInfo* pReceiverInfo;

		/** payload */
		EventType event;

		/** priority */
		unsigned long long priority;

		/** simple constructor */
		QueueData( ReceiverInfo* _pReceiverInfo, const EventType& rEvent, unsigned long long prio = 0L )
			: pReceiverInfo( _pReceiverInfo )
			, event( rEvent )
			, priority( prio )
			{}
	};

	/**
	 * Add events to the queue
	 *
	 * @param events vector containing QueueData objects
	 */
	void queue( std::vector< QueueData >& events );

	/**
	 * Removes all events that belong to a particular component
	 *
	 * @param pComponent pointer to component whose events to remove
	 */
	void removeComponent( const Component* pComponent );

	/** immediately dispatches all events in the queue */
	void dispatchNow();

	/** remove all queued events */
	void clear();

	/** get the main eventqueue object */
	static EventQueue& singleton();

	static void destroyEventQueue();	

protected:
	/** queue thread function */
	void threadFunction();

	/** mutex for thread synchronization */
	boost::mutex m_Mutex;

	/** condition variable for thread synchronization */
	boost::condition m_NewEventCondition;

	/** type of queue data */
	typedef std::list< QueueData > QueueType;

	/** the queue */
	QueueType m_Queue;

	/** the event dispatching thread */
	boost::shared_ptr< boost::thread > m_pThread;

	/** current state of the event thread */
	enum { state_running, state_stopping, state_stopped, state_end } m_State;
};


} } // namespace Ubitrack::Dataflow

#endif
