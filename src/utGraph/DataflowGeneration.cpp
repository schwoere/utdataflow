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
 * @ingroup srg_algorithms
 * @file
 * Implements the one-function pattern matching
 *
 * @author Daniel Pustka <daniel.pustka@in.tum.de>
 */

#include <log4cpp/Category.hh>
#include "DataflowGeneration.h"
#include <utGraph/UTQLReader.h>
#include <utGraph/UTQLWriter.h>
#include <utGraph/UTQLServer.h>

static log4cpp::Category& logger( log4cpp::Category::getInstance( "Ubitrack.Graph.DataflowGeneration" ) );
static log4cpp::Category& utqlLogger( log4cpp::Category::getInstance( "Ubitrack.Graph.DataflowGeneration.UTQL" ) );

namespace Ubitrack { namespace Graph {

boost::shared_ptr< UTQLDocument > generateDataflow( UTQLDocument& patterns )
{
	// add all patterns to the server
	UTQLServer server;
	for ( UTQLDocument::SubgraphList::const_iterator it = patterns.m_Subgraphs.begin(); it != patterns.m_Subgraphs.end(); it++ )
	{
		LOG4CPP_DEBUG( logger, "Announcing " << (*it)->m_Name << ":" << (*it)->m_ID );
		server.processAnnouncement( *it, "noclient" );
	}

	// run server
	boost::shared_ptr< std::map< std::string, boost::shared_ptr< UTQLDocument > > > pDfMap = server.generateDocuments();


	// create a new dataflow description
	boost::shared_ptr< UTQLDocument > pDf( (*pDfMap)[ "noclient" ] );

	if ( !pDf )
		pDf.reset( new UTQLDocument()  );

	LOG4CPP_DEBUG( utqlLogger, std::endl << UTQLWriter::processDocument( pDf ) << std::endl );

	return pDf;
}


std::string generateDataflow( const std::string& query )
{
	// read UTQLDocument from string
	std::istringstream iss( query );
	boost::shared_ptr< Ubitrack::Graph::UTQLDocument > doc = Ubitrack::Graph::UTQLReader::processInput(iss);

	// run pattern matching
	boost::shared_ptr< UTQLDocument > pResultDoc = generateDataflow( *doc );

	// generate string response
	return UTQLWriter::processDocument( pResultDoc );
}


std::string dataflowToGraphviz( const std::string& dataflow )
{
	// read UTQLDocument from string
	std::istringstream iss( dataflow );
	boost::shared_ptr< Ubitrack::Graph::UTQLDocument > doc = Ubitrack::Graph::UTQLReader::processInput( iss );

	// convert to dot
	std::ostringstream oss;
	doc->exportDot( oss );

	// return string
	return oss.str();
}

} } // namespace Ubitrack::Graph
