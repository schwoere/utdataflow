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
 * The base class of all ports
 *
 * @author Daniel Pustka <daniel.pustka@in.tum.de>
 */ 

#ifndef __Ubitrack_Dataflow_Port_INCLUDED__
#define __Ubitrack_Dataflow_Port_INCLUDED__

#include <string>
#include <boost/utility.hpp>
#include <utDataflow.h>
#include "Component.h"

namespace Ubitrack { namespace Dataflow {


/**
 * @ingroup dataflow_framework
 * The common base class of all ports.
 */
class UTDATAFLOW_EXPORT Port
	: private boost::noncopyable
{
public:	
	/**
	 * Registers the class at the component that owns it.
	 *
	 * @param sName Name of the port. Used for network instantiation.
	 * @param rComponent Reference to owning component where this port will be registered.
	 */
	Port( const std::string& sName, Component& rComponent );
	
	/**
	 * Unregisters the port at the owning component
	 */
	virtual ~Port();

	/** returns the name of this port */
	const std::string& getName() const
	{	return m_sName; }
	
	/** returns the full name of the port (including component name) */
	std::string fullName() const
	{ return m_rComponent.getName() + ":" + m_sName; }
	
	/** returns a reference to the component that this port belongs to */
	Component& getComponent() const
	{ return m_rComponent; }
	
	/**
	 * Connect this port to another.
	 * Usually is overrided by derived classes, but default implementation is provided for those that do not need it.
	 * 
	 * @param rOther The port to connect to.
	 */
	virtual void connect( Port& rOther );
	
	/**
	 * Disconnect this port from another.
	 * Usually is overrided by derived classes, but default implementation is provided for those that do not need it.
	 * 
	 * @param rOther The port to disconnect from.
	 */
	virtual void disconnect( Port& rOther );
	
protected:
	/** the name of the port */
	std::string m_sName;
	
	/** reference to the component that owns this port */
	Component& m_rComponent;
}; 


} } // namespace Ubitrack::Dataflow

#endif
