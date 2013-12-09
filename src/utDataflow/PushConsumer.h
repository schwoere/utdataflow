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
 * A push consumer port.
 *
 * @author Daniel Pustka <daniel.pustka@in.tum.de>
 */

#ifndef __Ubitrack_Dataflow_PushConsumer_INCLUDED__
#define __Ubitrack_Dataflow_PushConsumer_INCLUDED__

#include <boost/function.hpp>

#include "Port.h"
#include "EventQueue.h"
#include "EventTypeTraits.h"

namespace Ubitrack { namespace Dataflow {

/**
 * \internal
 * Implements the core functionality of a push consumer.
 * Used by the PushConsumer port and other ports.
 *
 * @param EventType type of measurements to be pushed
 */
template< class EventType >
class PushConsumerCore
{
public:
	/** type of pointers to functions receiving events */
	typedef boost::function< void ( const EventType& ) > SlotType;

	/**
	 * constructor.
	 * @param slot the function to call when events are received
	 */
	PushConsumerCore( Port& rPort, const SlotType& slot, EventQueue::ReceiverInfo::MutexType* pMutex = 0 )
		: m_slot( slot )
		, m_rPort( rPort )
		, m_receiverInfo( &rPort, pMutex, EventTypeTraits< EventType >().getMaxQueueLength() )
	{}

	/** returns the function to be called by the supplier */
	SlotType& getSlot()
	{ return m_slot; }

	/** returns the port this thing belongs to */
	Port& getPort()
	{ return m_rPort; }

	/** for queue length management, etc.. */
	EventQueue::ReceiverInfo& getReceiverInfo()
	{ return m_receiverInfo; }

	/** returns the number of queued events */
	unsigned getQueuedEvents() const
	{ return m_receiverInfo.nQueuedEvents; }

private:
	/** function to be called by the supplier */
	SlotType m_slot;

	/** reference to parent component */
	Port& m_rPort;

	/** for queue length management, etc.. */
	EventQueue::ReceiverInfo m_receiverInfo;
};


/**
 * @ingroup dataflow_framework
 * Implements a port that receives events from one or more PushSuppliers
 *
 * @param EventType type of measurements to be pushed
 */
template< class EventType > class PushConsumer
	: public Port
	, public PushConsumerCore< EventType >
{
public:
	/**
	 * Constructor.
	 *
	 * @param sName the name of this port
	 * @param rParent Reference to the component this port belongs to
	 * @param slot Pointer to function receiving events. Must be of type <tt>void( const EventType& )</tt>.
	 *    Use \c boost::bind to supply member functions of objects
	 */
	PushConsumer( const std::string& sName, Component& rParent, const typename PushConsumerCore< EventType >::SlotType& slot )
		: Port( sName, rParent )
		, PushConsumerCore< EventType >( *this, slot, &rParent.getMutex() )
	{}

};

} } // namespace Ubitrack::Dataflow

#endif

