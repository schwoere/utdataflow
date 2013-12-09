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
 * Defines Ubitrack::Dataflow::Component
 *
 * @author Daniel Pustka <daniel.pustka@in.tum.de>
 */

#ifndef __Ubitrack_Dataflow_Component_INCLUDED__
#define __Ubitrack_Dataflow_Component_INCLUDED__

#include <string>
#include <map>

#include <boost/utility.hpp>
#include <boost/thread.hpp>

#include <utDataflow.h>


namespace Ubitrack { namespace Dataflow {

// external class declarations
class Port;


/**
 * @ingroup dataflow_framework
 *
 * The common base class of all components.
 * It maintains a map of all ports that belong to it.
 */
class UTDATAFLOW_EXPORT Component
	: private boost::noncopyable
{
public:
	/**
	 * Constructor.
	 *
	 * @param sName Unique Id of the component.
	 * @param config Configuration data from the XML
	 */
	Component( const std::string& name );

	/**
	 * Destructor.
	 */
	virtual ~Component();

	/** returns the ID of this component */
	const std::string& getName() const
	{	return m_name; }

	/**
	 * Adds a port to the component.
	 * Throws a Ubitrack::Util:Exception if the port already exists.
	 *
	 * @param sName Name of the port. Must be unique!
	 * @param pPort Pointer to the actual port
	 */
	void addPort( const std::string& sName, Port* pPort );

	/**
	 * Removes a port from the component.
	 *
	 * @param sName Name of the port to be removed
	 */
	void removePort( const std::string& sName );

    /**
     * Returns a port reference by name.
     * Throws a Ubitrack::Util:Exception if the port is not registered.
     *
     * @param sName Name of the port to be returned
     * @return pointer to the Port
     */
    Port& getPortByName( const std::string& sName );

	/**
	 * Start the component.
	 *
	 * Starts the component. Default implementation
	 * only sets a flag. Components with more complex needs
	 * have to overload this method.
	 */
	virtual void start()
	{
		m_running = true;
	}

	/**
	 * Stop the component.
	 *
	 * Stops the component. Default implementation
	 * only sets a flag. Components with more complex needs
	 * have to overload this method.
	 */
	virtual void stop()
	{
		m_running = false;
	}

	/** 
	 * Sets the event scheduling priority. 
	 * This method is used by the data flow network and the event queue for improved event scheduling.
	 */
	void setEventPriority( int priority )
	{ m_eventPriority = priority; }

	/** Returns the event scheduling priority. */
	int getEventPriority() const
	{ return m_eventPriority; }

	/** type of mutex for later reference */
	typedef boost::recursive_mutex MutexType;
	
	/** returns a reference to the mutex */
	MutexType& getMutex()
	{ return m_componentMutex; }
	
protected:
	/** the name of the component */
	std::string m_name;

	/** a map of all ports */
	std::map< std::string, Port* > m_PortMap;


	/** mutex to lock the component */
	MutexType m_componentMutex;

	bool m_running;

	/** 
	 * The priority of this component for event scheduling.
	 * Note: this is the priority of events received (not sent) by the component!
	 */
	int m_eventPriority;
};


} } // namespace Ubitrack::Dataflow

#endif // __Ubitrack_Dataflow_Component_INCLUDED__
