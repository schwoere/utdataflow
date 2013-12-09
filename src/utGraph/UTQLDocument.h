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

#ifndef __Ubitrack_Graph_UTQLDocument_INCLUDED__
#define __Ubitrack_Graph_UTQLDocument_INCLUDED__ __Ubitrack_Graph_UTQLDocument_INCLUDED__

#include <map>
#include <string>
#include <iostream>
#include <boost/shared_ptr.hpp>
#include <utDataflow.h>
#include <utGraph/Graph.h>
#include <utGraph/UTQLSubgraph.h>

#include <log4cpp/Category.hh>
#include <utUtil/Exception.h>
#include <utUtil/Logging.h>

/**
 * @ingroup srg_algorithms
 * @file
 *
 * @author Manuel Huber <huberma@in.tum.de>
 */

namespace Ubitrack { namespace Graph {

	class UTDATAFLOW_EXPORT UTQLDocument
	{
	public:
		typedef std::map< std::string, boost::shared_ptr< UTQLSubgraph > > SubgraphIDMap;
		typedef std::multimap< std::string, boost::shared_ptr< UTQLSubgraph > > SubgraphNameMap;
		typedef std::list< boost::shared_ptr< UTQLSubgraph > > SubgraphList;

		UTQLDocument( bool bRequest = false );
		void addSubgraph( boost::shared_ptr< UTQLSubgraph > subgraph );
		bool hasSubgraphById( std::string id ) const;
		boost::shared_ptr< UTQLSubgraph >& getSubgraphById( std::string id );
		void removeSubgraphById( std::string id );

		// prints a dataflow network in graphviz/DOT format for automatic layout & visualization
		void exportDot( std::ostream& out );

		/** is this document a query or a dataflow? */
		bool isRequest() const
		{ return m_bRequest; }

		SubgraphIDMap m_SubgraphById;
		SubgraphNameMap m_SubgraphByName;

		SubgraphList m_Subgraphs;

		static log4cpp::Category& m_Logger;

	protected:
		bool m_bRequest;
	};

}}

#endif // __Ubitrack_Graph_UTQLDocument_INCLUDED__
