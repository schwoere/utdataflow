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
 * Implementation of Ubitrack::Dataflow::Component
 *
 * @author Daniel Pustka <daniel.pustka@in.tum.de>
 */

#include "Component.h"
#include <iostream>
#include <utUtil/Exception.h>
#include <log4cpp/Category.hh>

// get a logger
static log4cpp::Category& logger( log4cpp::Category::getInstance( "Ubitrack.Dataflow.Component" ) );


namespace Ubitrack { namespace Dataflow {


Component::Component( const std::string& name )
	: m_name( name )
	, m_componentMutex()
	, m_running( false )
	, m_eventPriority( 0 )
{
	LOG4CPP_DEBUG( logger, "Component (" << name << ")" );
}

Component::~Component()
{
	LOG4CPP_DEBUG( logger, "~Component(): " << getName() );
}


void Component::addPort( const std::string& sName, Ubitrack::Dataflow::Port* pPort )
{
	LOG4CPP_TRACE( logger, "Adding port " << sName << " to component " << m_name );

	// check if port of this name already exists
	Port*& rpDest = m_PortMap[ sName ];
	if ( rpDest != 0 )
		UBITRACK_THROW( "Port " + sName + " already exists at component " + m_name );

	rpDest = pPort;
}


void Component::removePort( const std::string& sName )
{
	LOG4CPP_TRACE( logger, "Removing port " << sName << " from component " << m_name );
	m_PortMap.erase( sName );
}


Port& Component::getPortByName( const std::string& sName )
{
    std::map< std::string, Port* >::iterator it = m_PortMap.find( sName );
    if ( it == m_PortMap.end() )
		UBITRACK_THROW( "Port " + sName + " is not registered at component " + m_name );

    return *it->second;
}


} } // namespace Ubitrack::Dataflow
