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

#ifndef __Ubitrack_Graph_PredicateList_INCLUDED__
#define __Ubitrack_Graph_PredicateList_INCLUDED__  __Ubitrack_Graph_PredicateList_INCLUDED__

#include <map>
#include <list>
#include <string>
#include <boost/shared_ptr.hpp>

#include <log4cpp/Category.hh>
#include <utDataflow.h>

/**
 * @ingroup srg_algorithms
 * @file
 *
 * @author Manuel Huber <huberma@in.tum.de>
 */

namespace Ubitrack { namespace Graph {

	// forward declaration
	class Predicate;

	class UTDATAFLOW_EXPORT PredicateList
	{
	public:
		void addPredicate( boost::shared_ptr< Predicate > p )
		{
			m_predicateList.push_back( p );
		}

		std::list< boost::shared_ptr< Predicate > > m_predicateList;
	};
}}

#endif // __Ubitrack_Graph_PredicateList_INCLUDED__

