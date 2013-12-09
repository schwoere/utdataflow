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
 * Implements the base class of all ports.
 *
 * @author Daniel Pustka <daniel.pustka@in.tum.de>
 */ 

#include <stdexcept>

#include "Port.h"
#include "Component.h"

namespace Ubitrack { namespace Dataflow {


Port::Port( const std::string &sName, Component& rComponent )
	: m_sName( sName )
	, m_rComponent( rComponent )
{ 
	m_rComponent.addPort( m_sName, this );
}


Port::~Port()
{
	m_rComponent.removePort( m_sName );
}


void Port::connect( Port& )
{ 
	// default implementation that does nothing
}


void Port::disconnect( Port& )
{ 
	// default implementation that does nothing
}


} } // namespace Ubitrack::Dataflow
