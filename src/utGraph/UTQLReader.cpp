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
 *
 * @author Manuel Huber <huberma@in.tum.de>
 */

#include "UTQLReader.h"

#include <log4cpp/Category.hh>
#include <utUtil/Exception.h>
#include <utUtil/Logging.h>
#include <utGraph/PredicateParser.h>

#include "tinyxml.h"

static log4cpp::Category& logger( log4cpp::Category::getInstance( "Ubitrack.Graph.UTQLReader" ) );

namespace Ubitrack { namespace Graph {

    boost::shared_ptr< UTQLDocument > UTQLReader::processInput( std::istream& input )
    {
        // create new XML dom tree and read document into it
        TiXmlDocument xmlDocument;
        input >> xmlDocument;

		if ( xmlDocument.Error() )
		{
			// if desired use ErrorRow()/ErrorCol() and so on..
			// std::cout << doc.ErrorDesc() << std::endl;
			LOG4CPP_ERROR( logger, "Error parsing UTQL document: " << xmlDocument.ErrorDesc()
						   << " at line " << xmlDocument.ErrorRow() );
			UBITRACK_THROW( "Error parsing UTQL document" );
		}

		TiXmlElement* xmlElementRoot = xmlDocument.RootElement();
		if ( !xmlElementRoot )
		{
			LOG4CPP_ERROR( logger, "Error parsing UTQL document: no root element found" );
			UBITRACK_THROW( "Error parsing UTQL document: no root element" );
		}

		DocumentPtr document = DocumentPtr( new UTQLDocument( xmlElementRoot->ValueStr() == "UTQLRequest" ) );

		TiXmlHandle xmlHandleRoot( xmlElementRoot );

		TiXmlHandle xmlHandleSubgraph = xmlHandleRoot.Child( "Pattern", 0 );
		for (int child=0; xmlHandleSubgraph.Element(); xmlHandleSubgraph = xmlHandleRoot.Child( "Pattern", ++child ) )
		{
			TiXmlElement* xmlElementSubgraph = xmlHandleSubgraph.ToElement();

			std::string name;
			if ( xmlElementSubgraph->Attribute( "name" ) )
				name = xmlElementSubgraph->Attribute( "name" );

			std::string id;
			if ( xmlElementSubgraph->Attribute( "id" ) )
				id = xmlElementSubgraph->Attribute( "id" );
			if ( id.empty() )
			{
				static long tempID = 1000;

				std::ostringstream oss;
				if ( name.empty() )
					oss << "tempSubgraph" << tempID;
				else
					oss << name << tempID;
				id = oss.str();
				tempID++;
			}

			SubgraphPtr subgraph = SubgraphPtr( new UTQLSubgraph() );
			subgraph->m_Name = name;
			subgraph->m_ID = id;

			handleSubgraph( xmlHandleSubgraph, subgraph );

			document->addSubgraph( subgraph );
		}
		return document;
    }

	void UTQLReader::handleSubgraph( TiXmlHandle& xmlHandleSubgraph, SubgraphPtr subgraph )
	{
		TiXmlHandle xmlHandleInput = xmlHandleSubgraph.FirstChild( "Input" );
		TiXmlHandle xmlHandleOutput = xmlHandleSubgraph.FirstChild( "Output" );
		TiXmlHandle xmlHandleConstraints = xmlHandleSubgraph.FirstChild( "Constraints" );
		TiXmlHandle xmlHandleConfig = xmlHandleSubgraph.FirstChild( "DataflowConfiguration" );

		if ( xmlHandleInput.Element() )
		{
			handleGraph( xmlHandleInput, SectionInput, subgraph );
		}

		if ( xmlHandleOutput.Element() )
		{
			handleGraph( xmlHandleOutput, SectionOutput, subgraph );
		}

		if ( xmlHandleConstraints.Element() )
		{
			if ( xmlHandleConstraints.Element()->FirstChild( "OnlyBestEdgeMatch" ) )
				subgraph->m_onlyBestEdgeMatch = true;

			TiXmlHandle xmlHandleBestMatchExpression = xmlHandleConstraints.Element()->FirstChild( "BestMatchExpression" );
			if ( xmlHandleBestMatchExpression.Element() )
				subgraph->m_bestMatchExpression = parseAttributeExpression( xmlHandleBestMatchExpression.Element()->GetText() );
		}
		
		handleAttributes( xmlHandleConfig, &subgraph->m_DataflowAttributes );

		if ( xmlHandleConfig.Element() )
		{
			boost::shared_ptr< TiXmlElement > config = boost::shared_ptr< TiXmlElement >( xmlHandleConfig.ToElement()->Clone()->ToElement() );
			subgraph->m_DataflowConfiguration = AttributeValue( config );
		}
	}

	void UTQLReader::handleAttributes( TiXmlHandle& xmlHandleBase, KeyValueAttributes* attribs )
	{
		// handle attributes
		TiXmlHandle xmlHandleAttribute = xmlHandleBase.Child( "Attribute", 0 );
		for (int child=0; xmlHandleAttribute.Element(); xmlHandleAttribute = xmlHandleBase.Child( "Attribute", ++child ) )
		{
			TiXmlElement* xmlElementAttribute = xmlHandleAttribute.ToElement();

			std::string name;
			if ( !xmlElementAttribute->Attribute( "name" ) )
			{
				LOG4CPP_ERROR( logger, "UTQL Node Attribute without name" );
				UBITRACK_THROW( "UTQL Node Attribute without name" );
			}
			name = xmlElementAttribute->Attribute( "name" );

			if ( xmlElementAttribute->Attribute( "value" ) )
			{
				std::string value = xmlElementAttribute->Attribute( "value" );
				attribs->setAttribute( name, AttributeValue( value ) );
			}
			else
			{
				attribs->setAttribute( name, AttributeValue( boost::shared_ptr< TiXmlElement >( xmlElementAttribute->Clone()->ToElement() ) ) );
			}
		}
	}

	void UTQLReader::handleAttributeExpressions( TiXmlHandle& xmlHandleBase, InOutAttribute& attribs )
	{
		// handle attribute expressionss
		TiXmlHandle xmlHandleAttribute = xmlHandleBase.Child( "AttributeExpression", 0 );
		for (int child=0; xmlHandleAttribute.Element(); xmlHandleAttribute = xmlHandleBase.Child( "AttributeExpression", ++child ) )
		{
			TiXmlElement* xmlElementAttribute = xmlHandleAttribute.ToElement();

			std::string name;
			if ( !xmlElementAttribute->Attribute( "name" ) )
			{
				LOG4CPP_ERROR( logger, "UTQL AttributeExpression without name" );
				UBITRACK_THROW( "UTQL AttributeExpression without name" );
			}
			name = xmlElementAttribute->Attribute( "name" );

			try
			{
				boost::shared_ptr< AttributeExpression > expression = parseAttributeExpression( xmlElementAttribute->GetText() );
				attribs.m_attributeExpressions.push_back( std::make_pair( name, expression ) );
			}
			catch ( const Ubitrack::Util::Exception& e )
			{
				LOG4CPP_ERROR( logger, "Error parsing attribute expression \"" << xmlElementAttribute->GetText() << "\":\n" << e );
				UBITRACK_THROW( "Error parsing attribute expression: " + std::string( e.what() ) );
			}
		}
	}

	void UTQLReader::handlePredicates( TiXmlHandle& xmlHandleBase, PredicateList* attribs )
	{
		// handle predicates
		TiXmlHandle xmlHandlePredicate = xmlHandleBase.Child( "Predicate", 0 );
		for (int child=0; xmlHandlePredicate.Element(); xmlHandlePredicate = xmlHandleBase.Child( "Predicate", ++child ) )
		{
			TiXmlElement* xmlElementPredicate = xmlHandlePredicate.ToElement();

			try
			{
				boost::shared_ptr< Predicate > predicate = parsePredicate( xmlElementPredicate->GetText() );
				attribs->addPredicate( predicate );
			}
			catch ( const Ubitrack::Util::Exception& e )
			{
				LOG4CPP_ERROR( logger, "Error parsing predicate \"" << xmlElementPredicate->GetText() << "\":\n" << e );
				UBITRACK_THROW( "Error parsing predicate: " + std::string( e.what() ) );
			}
		}
	}

	void UTQLReader::handleGraph( TiXmlHandle& xmlHandle, Section section, SubgraphPtr subgraph )
	{
		// first find all nodes
		TiXmlHandle xmlHandleNode = xmlHandle.Child( "Node", 0 );
		for (int child=0; xmlHandleNode.Element(); xmlHandleNode = xmlHandle.Child( "Node", ++child ) )
		{
			TiXmlElement* xmlElementNode = xmlHandleNode.ToElement();

			std::string name;
			if ( !xmlElementNode->Attribute( "name" ) )
			{
				LOG4CPP_ERROR( logger, "UTQL Node without name" );
				UBITRACK_THROW( "UTQL Node without name" );
			}
			name = xmlElementNode->Attribute( "name" );

			std::string id;
			if ( xmlElementNode->Attribute( "id" ) )
			{
				id = xmlElementNode->Attribute( "id" );
			}

			UTQLSubgraph::GraphNodeAttributes::Tag tag = ( section == SectionInput ) ? UTQLSubgraph::GraphNodeAttributes::Input : UTQLSubgraph::GraphNodeAttributes::Output;
			UTQLSubgraph::NodePtr node = subgraph->addNode( name, tag );

			// one of these three mechanisms should be enough
			node->m_QualifiedName = id;

			if ( id.length() != 0 )
			{
				node->setAttribute( "id", AttributeValue( id ) );
// 				subgraph->registerNodeID( id, node );
			}


			// handle attributes
			handleAttributes( xmlHandleNode, node.get() );
			handleAttributeExpressions( xmlHandleNode, *node.get() );
			handlePredicates( xmlHandleNode, node.get() );
		}

		// find all edges
		TiXmlHandle xmlHandleEdge = xmlHandle.Child( "Edge", 0 );
		for (int child=0; xmlHandleEdge.Element(); xmlHandleEdge = xmlHandle.Child( "Edge", ++child ) )
		{
			TiXmlElement* xmlElementEdge = xmlHandleEdge.ToElement();

			std::string name;
			if ( !xmlElementEdge->Attribute( "name" ) )
			{
				LOG4CPP_ERROR( logger, "UTQL Edge without name" );
				UBITRACK_THROW( "UTQL Edge without name" );
			}
			name = xmlElementEdge->Attribute( "name" );

			std::string source;
			if ( !xmlElementEdge->Attribute( "source" ) )
			{
				LOG4CPP_ERROR( logger, "UTQL Edge without source" );
				UBITRACK_THROW( "UTQL Edge without source" );
			}
			source = xmlElementEdge->Attribute( "source" );

			std::string destination;
			if ( !xmlElementEdge->Attribute( "destination" ) )
			{
				LOG4CPP_ERROR( logger, "UTQL Edge without destination" );
				UBITRACK_THROW( "UTQL Edge without destination" );
			}
			destination = xmlElementEdge->Attribute( "destination" );

			UTQLSubgraph::GraphEdgeAttributes::Tag tag = ( section == SectionInput ) ? UTQLSubgraph::GraphEdgeAttributes::Input : UTQLSubgraph::GraphEdgeAttributes::Output;

			if ( !( subgraph->hasNode( source ) && subgraph->hasNode( destination ) ) )
			{
				LOG4CPP_ERROR( logger, "UTQL Edge: source or destination not in graph: " << source << " -> " << destination );
				UBITRACK_THROW( "UTQL Edge: source or destination not in graph" );
			}
			UTQLSubgraph::EdgePtr edge = subgraph->addEdge( name, source, destination, tag );

			// handle attributes

			handleAttributes( xmlHandleEdge, edge.get() );
			handleAttributeExpressions( xmlHandleEdge, *edge.get() );
			handlePredicates( xmlHandleEdge, edge.get() );

// 			TiXmlHandle xmlHandleAttribute = xmlHandleEdge.Child( "Attribute", 0 );
// 			for (int child=0; xmlHandleAttribute.Element(); xmlHandleAttribute = xmlHandleEdge.Child( "Attribute", ++child ) )
// 			{
// 				TiXmlElement* xmlElementAttribute = xmlHandleAttribute.ToElement();

// 				std::string name;
// 				if ( !xmlElementAttribute->Attribute( "name" ) )
// 				{
// 					LOG4CPP_ERROR( logger, "UTQL Node Attribute without name" );
// 					UBITRACK_THROW( "UTQL Node Attribute without name" );
// 				}
// 				name = xmlElementAttribute->Attribute( "name" );

// 				if ( xmlElementAttribute->Attribute( "value" ) )
// 				{
// 					std::string value = xmlElementAttribute->Attribute( "value" );
// 					edge->setAttribute( name, AttributeValue( value ) );
// 				}
// 				else
// 				{
// 					edge->setAttribute( name, AttributeValue( boost::shared_ptr< TiXmlElement >( xmlElementAttribute->Clone()->ToElement() ) ) );
// 				}
// 			}

			const char* graphRef = xmlElementEdge->Attribute( "pattern-ref" );
			const char* edgeRef = xmlElementEdge->Attribute( "edge-ref" );
			if ( graphRef && edgeRef )
				edge->m_EdgeReference = EdgeReference( graphRef, edgeRef );

		}
	}
}}
