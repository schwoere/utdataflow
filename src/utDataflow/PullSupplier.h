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
 * The Pull Supplier Port
 *
 * @author Daniel Pustka <daniel.pustka@in.tum.de>
 */ 

#ifndef __Ubitrack_Dataflow_PullSupplier_INCLUDED__
#define __Ubitrack_Dataflow_PullSupplier_INCLUDED__

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

#include "Port.h"
#include <utMeasurement/Timestamp.h>

namespace Ubitrack { namespace Dataflow {

/**
 * \internal
 * Implements the core functionality of a pull supplier.
 * Used by the PullSupplier port and other ports.
 *
 * @param EventType type of measurements to be queried
 */
template< class EventType > class PullSupplierCore
{
public:
	/** type of function pointers used to define callbacks */
	typedef boost::function< EventType( Ubitrack::Measurement::Timestamp ) > FunctionType;

	/** constructor */
	PullSupplierCore( const FunctionType& function )
		: m_function( function )
	{}

	/** returns the stored function pointer */
	const FunctionType& getFunction() const
	{ return m_function; }

protected:
	/** pointer to callback function */
	FunctionType m_function;
};

/**
 * @ingroup dataflow_framework
 * Implements a port where other components can query for measurements.
 *
 * @param EventType type of measurements to be queried
 */
template< class EventType > class PullSupplier
	: public Port
	, public PullSupplierCore< EventType >
{
public:

	/**
	 * Constructor.
	 *
	 * @param sName name of this port
	 * @param rParent reference to component owning this port
	 * @param function function to be called by clients to query measurements. Must be of type
	 *    <tt>boost::shared_ptr< EventType >( Timestamp )</tt>. Use \c boost::bind to supply
	 *    member functions of objects
	 */
	PullSupplier( const std::string& sName, Component& rParent, const typename PullSupplierCore< EventType >::FunctionType& function )
		: Port( sName, rParent )
		, PullSupplierCore< EventType >( function )
	{}
};

} } // namespace Ubitrack::Dataflow

#endif
