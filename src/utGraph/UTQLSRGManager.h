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

#ifndef __Ubitrack_Graph_UTQLSRGManager_INCLUDED__
#define __Ubitrack_Graph_UTQLSRGManager_INCLUDED__ __Ubitrack_Graph_UTQLSRGManager_INCLUDED__

#include <map>
#include <list>
#include <string>
#include <sstream>
#include <math.h>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/functional/hash.hpp>
#include <utGraph/Graph.h>
#include <utGraph/UTQLSubgraph.h>
#include <utGraph/SRGraph.h>

#include <utGraph/Pattern.h>
#include <utGraph/EdgeMatching.h>
#include <utGraph/Predicate.h>
#include <utGraph/PredicateParser.h>
#include <utGraph/AttributeExpression.h>

#include <log4cpp/Category.hh>
#include <utUtil/Exception.h>
#include <utUtil/Logging.h>

#define DO_SRGMANAGER_TIMING
#ifdef DO_SRGMANAGER_TIMING
#include <utUtil/BlockTimer.h>

static Ubitrack::Util::BlockTimer g_timeApplyAllPatterns( "applyAllPatterns", "Ubitrack.Graph.UTQLSRGManager.Timing" );
static Ubitrack::Util::BlockTimer g_timeApplyDetectedPattern( "applyDetectedPattern", "Ubitrack.Graph.UTQLSRGManager.Timing" );
static Ubitrack::Util::BlockTimer g_timeApplyPattern( "applyPattern", "Ubitrack.Graph.UTQLSRGManager.Timing" );
static Ubitrack::Util::BlockTimer g_timeDeleteSRG( "deleteSRG", "Ubitrack.Graph.UTQLSRGManager.Timing" );
static Ubitrack::Util::BlockTimer g_timeExpandMatchingAttributes( "expandMatchingAttributes", "Ubitrack.Graph.UTQLSRGManager.Timing" );
static Ubitrack::Util::BlockTimer g_timeInstantiatePattern( "instantiatePattern", "Ubitrack.Graph.UTQLSRGManager.Timing" );
static Ubitrack::Util::BlockTimer g_timeMatchPattern( "matchPattern", "Ubitrack.Graph.UTQLSRGManager.Timing" );
static Ubitrack::Util::BlockTimer g_timeDecidePattern1( "decidePattern1", "Ubitrack.Graph.UTQLSRGManager.Timing" );
static Ubitrack::Util::BlockTimer g_timeDecidePattern2( "decidePattern2", "Ubitrack.Graph.UTQLSRGManager.Timing" );
static Ubitrack::Util::BlockTimer g_timeProcessQueries( "processQueries", "Ubitrack.Graph.UTQLSRGManager.Timing" );
static Ubitrack::Util::BlockTimer g_timeExpressionEvaluation( "expr->evaluateSubgraph", "Ubitrack.Graph.UTQLSRGManager.Timing" );
#endif

// adapt some strategies

// if ALLOW_WORSE_EDGES is set, the server will allow new edges with worse attributes, 
// when the information sources are different. Not setting this may prevent some fusion scenarios.
#define ALLOW_WORSE_EDGES 

// selection strategy if no bestmatch expression is given 
// Note that this can also be controlled at runtime using
// the sourceCount() function in a <BestMatchExpression>.
#define BMS_SELECT_LEAST_SOURCES 0
#define BMS_SELECT_MOST_SOURCES 1
#define DEFAULT_BEST_MATCH_SELECTION BMS_SELECT_LEAST_SOURCES

// different edges of a match must fulfil the following requirements (limits trivial fusion)
#define ERQ_NEW_INFORMATION_SOURCE 0 
#define ERQ_DISJOINT_INFORMATION_SOURCES 1 // more strict!
#define ERQ_NONE 2
#define MATCHING_EDGE_REQUIREMENTS ERQ_DISJOINT_INFORMATION_SOURCES

/**
 * @ingroup srg_algorithms
 * @file
 *
 * @author Manuel Huber <huberma@in.tum.de>
 */

namespace Ubitrack { namespace Graph {

	class /*UTDATAFLOW_EXPORT*/ UTQLSRGManager
	{
	public:

		UTQLSRGManager()
			: m_GlobalSrg()
			, m_logger( log4cpp::Category::getInstance( "Ubitrack.Graph.UTQLSRGManager" ) )
		{
			// initialize known attributes
			// TODO: make this run-time configureable
			m_knownAttributes[ "latency" ] = smallerIsBetter;
			m_knownAttributes[ "gaussT" ] = smallerIsBetter;
			m_knownAttributes[ "gaussR" ] = smallerIsBetter;
			m_knownAttributes[ "staticT" ] = smallerIsBetter;
			m_knownAttributes[ "staticR" ] = smallerIsBetter;
			m_knownAttributes[ "updateTime" ] = smallerIsBetter;
			m_knownAttributes[ "availability" ] = biggerIsBetter;
		}

		// register a new pattern
		// this method registeres a new pattern and stores it in the
		// pattern respository in the manager
		void registerPattern( boost::shared_ptr< UTQLSubgraph > utqlPattern, const std::string& clientID )
		{
			LOG4CPP_INFO( m_logger, "Registering pattern " << clientID << ":" << utqlPattern->m_Name );
			m_Patterns.push_back( boost::shared_ptr< Pattern >( new Pattern( utqlPattern, clientID ) ) );
		}

		// register new query
		// this method registers a new query which should be
		// computed
		// the query is converted to a pattern and stored in the
		// active queries repository
		void registerQuery( boost::shared_ptr< UTQLSubgraph > utqlQuery, const std::string& clientID )
		{
			LOG4CPP_INFO( m_logger, "Registering query " << clientID << ":" << utqlQuery->m_Name );
			m_ActiveQueries.push_back( boost::shared_ptr< Pattern >( new Pattern( utqlQuery, clientID ) ) );
		}

		// this structure represents the answer to a query
		// as a query may be answered by different objects, the
		// answer may contain multiple graphs which represent solutions
		struct QueryResponse
		{
			QueryResponse( const std::string& queryName, const std::string& clientID )
				: m_QueryName( queryName )
				, m_ClientID ( clientID )
			{}

			std::string m_QueryName;
			std::string m_ClientID;
			std::list< boost::shared_ptr< UTQLSubgraph > > m_Graphs;
		};

		/**
		 * Describes an instantiated pattern.
		 *
		 * Basically a UTQLSubgraph with some extra information.
		 */
		class InstantiatedPattern
			: public boost::shared_ptr< UTQLSubgraph >
		{
		public:
			InstantiatedPattern()
			{}

			InstantiatedPattern( boost::shared_ptr< UTQLSubgraph > utql, const std::string& clientId )
				: boost::shared_ptr< UTQLSubgraph >( utql )
				, m_clientId( clientId )
			{}

			std::string m_clientId;
		};

		// process all active queries and find solutions
		// this method tries for every active query to find
		// all possible soltions
		std::map< std::string, std::list< QueryResponse > > processQueries()
		{
			#ifdef DO_SRGMANAGER_TIMING
			UBITRACK_TIME( g_timeProcessQueries );
			#endif

			// store all found results
			std::map< std::string, std::list< QueryResponse > > results;

			// run over all active queries
			for ( PatternList::iterator itPattern = m_ActiveQueries.begin(); itPattern != m_ActiveQueries.end(); ++itPattern )
			{
				boost::shared_ptr< Pattern > pattern = *itPattern;

				// perform pattern matching to get all instances of the query
				// in the graph. every instance is also a solution to the query
				boost::shared_ptr< std::vector< EdgeMatching< UTQLSubgraph, SRGraph > > > matches;
				matches = matchPattern( pattern );

				// expand input attributes of matching
				for ( std::vector< Ubitrack::Graph::EdgeMatching< UTQLSubgraph, SRGraph > >::iterator it = matches->begin();
					 it != matches->end(); ++it )
					 expandMatchingAttributes( pattern, *it );

				if ( pattern->m_Graph->m_onlyBestEdgeMatch )
				{
					// run over all found instances and apply selection mechanism
					double bestCost = 0.0;
					std::vector< Ubitrack::Graph::EdgeMatching< UTQLSubgraph, SRGraph > >::iterator itBestMatch	= matches->end();

					for ( std::vector< Ubitrack::Graph::EdgeMatching< UTQLSubgraph, SRGraph > >::iterator it = matches->begin();
						 it != matches->end(); ++it )
					{
						// if no selection expression is specified, we prefer the solution with the lowest (!) 
						// number of involved sensors, causing the least processing overhead...
						#if DEFAULT_BEST_MATCH_SELECTION == BMS_SELECT_LEAST_SOURCES
							double cost = static_cast< double >( it->m_informationSources.size() );
						#elif DEFAULT_BEST_MATCH_SELECTION == BMS_SELECT_MOST_SOURCES
							double cost = -static_cast< double >( it->m_informationSources.size() );
						#endif
						
						try
						{
							if ( pattern->m_Graph->m_bestMatchExpression )
							{
								if ( m_logger.isTraceEnabled() )
								{
									std::ostringstream os;
									for ( EdgeMatching< UTQLSubgraph, SRGraph >::AttributeObjectRefs::const_iterator itX = it->m_allInputAttributes.begin();
										itX != it->m_allInputAttributes.end(); itX++ )
										os << "\n\t" << itX->first << " -> " << *itX->second;
									LOG4CPP_TRACE( m_logger, "Evaluating " << pattern->m_Name << "'s BestMatchExpression on " 
										<< os.str() );
								}

								cost = pattern->m_Graph->m_bestMatchExpression->evaluate( EvaluationContext( *it ) )
									.getNumber();
							}
						}
						catch ( Util::Exception& e )
						{
							LOG4CPP_NOTICE( m_logger, "Exception evaluating BestMatchExpression on " << 
								pattern->m_Name << ": " << e );
						}

						LOG4CPP_DEBUG( m_logger, "Evaluated " << pattern->m_Name << "'s BestMatchExpression: " << cost );
						if ( itBestMatch == matches->end() || cost < bestCost )
						{
							bestCost = cost;
							itBestMatch = it;
						}
					}

					// instantiate only best data flow
					if ( itBestMatch != matches->end() )
					{
						std::list< InstantiatedPattern > subgraphs = generateResponse( pattern, *itBestMatch );

						// distribute the response among the clients
						for ( std::list< InstantiatedPattern >::iterator itSg = subgraphs.begin(); itSg != subgraphs.end(); itSg++ )
						{
							std::list< QueryResponse >& clientResults( results[ itSg->m_clientId ] );

							// add a new QueryResponse to client if there is none for this query
							if ( clientResults.empty() || clientResults.back().m_QueryName != pattern->m_Name )
								clientResults.push_back( QueryResponse( pattern->m_Name, itSg->m_clientId ) );

							// add subgraph to QueryResponse
							clientResults.back().m_Graphs.push_back( *itSg );
						}
					}
				}
				else
				{
					// no selection, add all possible responses to the result
					for ( std::vector< Ubitrack::Graph::EdgeMatching< UTQLSubgraph, SRGraph > >::iterator it = matches->begin();
						 it != matches->end(); ++it )
					{
						// add all responses to the result list
						std::list< InstantiatedPattern > subgraphs = generateResponse( pattern, *it );

						// distribute the response among the clients
						for ( std::list< InstantiatedPattern >::iterator itSg = subgraphs.begin(); itSg != subgraphs.end(); itSg++ )
						{
							std::list< QueryResponse >& clientResults( results[ itSg->m_clientId ] );

							// add a new QueryResponse to client if there is none for this query
							if ( clientResults.empty() || clientResults.back().m_QueryName != pattern->m_Name )
								clientResults.push_back( QueryResponse( pattern->m_Name, itSg->m_clientId ) );

							// add subgraph to QueryResponse
							clientResults.back().m_Graphs.push_back( *itSg );
						}
					}
				}
			}
			return results;
		}

		/** 
		 * apply a pattern to the srg.
		 * this method performs pattern matching of a given pattern
		 * and applies all detected instances if they are
		 * useful (see simpledecidepattern)
		 * @return number of instantiated patterns
		 */
		unsigned applyPattern( const boost::shared_ptr< Pattern > pattern )
		{
			#ifdef DO_SRGMANAGER_TIMING
			UBITRACK_TIME( g_timeApplyPattern );
			#endif

			unsigned nInstances = 0;
			
			LOG4CPP_DEBUG( m_logger, "trying to applying pattern \"" << pattern->m_clientId << ":" << pattern->m_Name << "\"" );

			// find all instances of the pattern in the graph so far
			boost::shared_ptr< std::vector< EdgeMatching< UTQLSubgraph, SRGraph > > > matches;
			matches = matchPattern( pattern );

			// for each found instance, decide if it is useful and apply it if desired
			std::list< std::string > supersededSubgraphs;
			for (std::vector< Ubitrack::Graph::EdgeMatching< UTQLSubgraph, SRGraph > >::iterator it = matches->begin();
				 it != matches->end(); ++it)
			{
				// debug output
				std::ostringstream ossVertices;
				if ( m_logger.isDebugEnabled() )
				{
					// print node list
					ossVertices << "{ ";
					for ( EdgeMatching< UTQLSubgraph, SRGraph >::VertexForwardMap::const_iterator itVertex =
						it->m_vertexForwardMap.begin(); itVertex != it->m_vertexForwardMap.end(); itVertex++ )
						ossVertices << itVertex->second.m_correspondence->getAttributeString( "id" ) << ", ";
					ossVertices << "}";
				}

				// decide if the pattern should be applied based on the un-expanded attributes
				if ( !decidePattern1( pattern, *it ) )
				{
					LOG4CPP_TRACE( m_logger, " -> not applying (unexpanded) at " << ossVertices.str() );
					continue;
				}

				// extract the "information sources" from the found instance and put them into the matching
				expandMatchingAttributes( pattern, *it );

				// decide if the pattern should be applied
				std::list< std::string > supersedes;
				if ( !decidePattern2( pattern, *it, supersedes ) )
				{
					LOG4CPP_TRACE( m_logger, " -> not applying (expanded) at " << ossVertices.str() );
					continue;
				}

				// apply it
				LOG4CPP_DEBUG( m_logger, " -> applying at " << ossVertices.str() );
				applyDetectedPattern( pattern, *it );
				nInstances++;

				supersededSubgraphs.splice( supersededSubgraphs.end(), supersedes );
			}

			// remove all superseded subgraphs
			for ( std::list< std::string >::iterator it = supersededSubgraphs.begin(); it != supersededSubgraphs.end(); it++ )
			{
				// check if the pattern has only one output
				std::map< std::string, InstantiatedPattern >::iterator itG = m_GraphRepository.find( *it );
				if ( itG != m_GraphRepository.end() )
				{
					unsigned nOutputs = 0;
					for ( UTQLSubgraph::EdgeMap::iterator itE = itG->second->m_Edges.begin(); itE != itG->second->m_Edges.end(); itE++ )
						if ( itE->second->isOutput() )
							nOutputs++;

					if ( nOutputs == 1 )
						deleteSRG( *it );
				}
			}
			
			return nInstances;
		}

		/* --------------------------------------------------------------------------- */

		// perform the pattern matching
		boost::shared_ptr< std::vector< EdgeMatching< UTQLSubgraph, SRGraph > > > matchPattern( boost::shared_ptr< Pattern > pattern )
		{
			#ifdef DO_SRGMANAGER_TIMING
			UBITRACK_TIME( g_timeMatchPattern );
			#endif

			// pattern matching is capsulated by PatternFunc
			return PatternFunc::checkPattern( pattern, m_GlobalSrg );
		}

		/**
		 * Collect the "information sources" for a given pattern instance.
		 * The "information sources" should represent the set ofactual tracker information which is used for 
		 * calculating the result. The idea here is that edges which use data from more trackers probably should be 
		 * preferred.
		 */
		void expandMatchingAttributes( const boost::shared_ptr< Pattern > pattern, 
			EdgeMatching< UTQLSubgraph, SRGraph >& matching )
		{
			// TODO: make this a method of EdgeMatching

			#ifdef DO_SRGMANAGER_TIMING
			UBITRACK_TIME( g_timeExpandMatchingAttributes );
			#endif

			// clear attributes of the matching (just to be sure...)
			matching.m_informationSources.clear();
			matching.m_expandedNodeAttributes.clear();
			matching.m_expandedEdgeAttributes.clear();

			// run over all input edges in the pattern
			for ( std::map< std::string, UTQLSubgraph::EdgePtr >::const_iterator it = pattern->m_Graph->m_Edges.begin();
				 it != pattern->m_Graph->m_Edges.end(); ++it )
			{
				UTQLSubgraph::EdgePtr patternEdge = it->second;

				// if the edge is a pattern input, it should have been matched to an egde in the srg
				if ( patternEdge->isInput() )
				{
					// get this edge
					SRGraph::EdgePtr srgEdge = matching.getCorrespondingSrgEdge( patternEdge );

					// each edge keeps its own set of used information sources so the new set of information sources 
					// is just the set union of all sets in the input edges "inplace" set union
					matching.m_informationSources.insert( 
						srgEdge->m_InformationSources.begin(), srgEdge->m_InformationSources.end() );

					// TODO: do this only for patterns that use attribute expressions
					matching.m_allInputAttributes[ patternEdge->m_Name ] = srgEdge.get();
				}
			}

			// run over all input nodes in the pattern
			for ( std::map< std::string, UTQLSubgraph::NodePtr >::const_iterator it = pattern->m_Graph->m_Nodes.begin();
				 it != pattern->m_Graph->m_Nodes.end(); ++it )
			{
				UTQLSubgraph::NodePtr patternNode = it->second;

				// if the edge is a pattern input, it should have been matched to an egde in the srg
				if ( patternNode->isInput() )
				{
					// get this edge
					SRGraph::NodePtr srgNode = matching.getCorrespondingSrgVertex( patternNode );

					// TODO: do this only for patterns that use attribute expressions
					matching.m_allInputAttributes[ patternNode->m_Name ] = srgNode.get();
				}
			}

			// evaluate attributes and attribute expressions on all outputs

			// run over all output edges in the pattern
			for ( std::map< std::string, UTQLSubgraph::EdgePtr >::const_iterator it = pattern->m_Graph->m_Edges.begin();
				 it != pattern->m_Graph->m_Edges.end(); ++it )
			{
				UTQLSubgraph::EdgePtr patternEdge = it->second;

				// if the edge is a pattern input, it should have been matched to an egde in the srg
				if ( patternEdge->isOutput() )
				{
					// merge attributes with attribute expressions
					KeyValueAttributes& newAttributes( matching.m_expandedEdgeAttributes[ patternEdge->m_Name ] );
					newAttributes = *patternEdge;

					for ( UTQLNode::ExpressionList::iterator itExp = patternEdge->m_attributeExpressions.begin();
						itExp != patternEdge->m_attributeExpressions.end(); itExp++ )
					{
						try
						{
							#ifdef DO_SRGMANAGER_TIMING
							Util::BlockTimer::Time timeExpr( g_timeExpressionEvaluation );
							#endif

							AttributeValue v = itExp->second->evaluate( EvaluationContext( matching ) );
							newAttributes[ itExp->first ] = v;
						}
						catch ( const Util::Exception& e )
						{ LOG4CPP_INFO( m_logger, "exception evaluating attribute expression " << 
							it->first << "." << itExp->first << " on " << pattern->m_Name << ": " << e ); }
					}
				}
			}

			// run over all output nodes in the pattern
			for ( std::map< std::string, UTQLSubgraph::NodePtr >::const_iterator it = pattern->m_Graph->m_Nodes.begin();
				 it != pattern->m_Graph->m_Nodes.end(); ++it )
			{
				UTQLSubgraph::NodePtr patternNode = it->second;

				// if the edge is a pattern input, it should have been matched to an egde in the srg
				if ( patternNode->isOutput() )
				{
					// merge attributes with attribute expressions
					KeyValueAttributes& newAttributes( matching.m_expandedNodeAttributes[ patternNode->m_Name ] );
					newAttributes = *patternNode;

					for ( UTQLNode::ExpressionList::iterator itExp = patternNode->m_attributeExpressions.begin();
						itExp != patternNode->m_attributeExpressions.end(); itExp++ )
					{
						try
						{
							#ifdef DO_SRGMANAGER_TIMING
							Util::BlockTimer::Time timeExpr( g_timeExpressionEvaluation );
							#endif

							AttributeValue v = itExp->second->evaluate( EvaluationContext( matching ) );
							newAttributes[ itExp->first ] = v;
						}
						catch ( const Util::Exception& )
						{ LOG4CPP_INFO( m_logger, "exception evaluating attribute expression " << 
							it->first << "." << itExp->first << " on " << pattern->m_Name ); }
					}
				}
			}

		}


		/**
		 * instanciate a detected pattern instance
		 *
		 * this method duplicates a pattern graph such that the copy is fully qualified and contains all relevant
		 * node and edge references to the matched counterparts in the srg.
		 * Note: The expanded node and edge attributes from the matching object are MOVED (not copied) to the
		 * newly instantiated pattern.
		 */
		InstantiatedPattern instanciatePattern( const boost::shared_ptr< Pattern > pattern, 
			EdgeMatching< UTQLSubgraph, SRGraph >& matching )
		{
			#ifdef DO_SRGMANAGER_TIMING
			UBITRACK_TIME( g_timeInstantiatePattern );
			#endif

			// create a copy of the pattern as an fully qualified instance
			// this will also be stored as the pattern part in the repository
			InstantiatedPattern subgraph( boost::shared_ptr< UTQLSubgraph >( new UTQLSubgraph() ), pattern->m_clientId );
			subgraph->m_Name = pattern->m_Name;

			// copy the dataflow configuration and dataflow attributes
			subgraph->m_DataflowConfiguration = pattern->m_Graph->m_DataflowConfiguration;
			subgraph->m_DataflowAttributes = pattern->m_Graph->m_DataflowAttributes;

			// keep a map of the new nodes in the pattern copy
			std::map< std::string, UTQLSubgraph::NodePtr > newNodesMap;

			// run over all nodes in the pattern
			for (std::map< std::string, UTQLSubgraph::NodePtr >::const_iterator it = pattern->m_Graph->m_Nodes.begin();
				 it != pattern->m_Graph->m_Nodes.end(); ++it )
			{
				// XXXD: what if the pattern wants to insert new nodes?

				// retrieve the orininal pattern node and the corresponding matched srg node
				UTQLSubgraph::NodePtr patternNode = it->second;
				SRGraph::NodePtr srgNode = matching.getCorrespondingSrgVertex( patternNode );
				// get the id of the srg node
				std::string matchedID = srgNode->m_Name;

				// duplicate the nodes for the instanciated copy
				UTQLSubgraph::NodePtr utqlNode = subgraph->addNode( patternNode->m_Name, *patternNode );

				// merge the attributes contained in the corresponding srg node into the copy node
				utqlNode->mergeAttributes( *srgNode );

				// store the link to the node in the srg as the qualified name in the pattern copy
				utqlNode->m_QualifiedName = matchedID;

				// clear the predicates in the instanciated copy
				// the nodes are qualified and no longer have to be matched against attribute lists
				utqlNode->m_predicateList.clear();

				// store the new node in the node map under the name of the pattern node
				newNodesMap[ utqlNode->m_Name ] = utqlNode;

				// copy attributes from matching (might contain attribute expressions )
				// TODO: inhibit attribute copying above
				if ( patternNode->isOutput() )
					patternNode->swap( matching.m_expandedNodeAttributes[ patternNode->m_Name ] );
			}

			// now run over all edges in the pattern
			for (std::map< std::string, UTQLSubgraph::EdgePtr >::const_iterator it = pattern->m_Graph->m_Edges.begin();
				 it != pattern->m_Graph->m_Edges.end(); ++it )
			{
				UTQLSubgraph::EdgePtr patternEdge = it->second;

				// inf the source and target of the edge in the instanciated copy
				UTQLSubgraph::NodePtr source = newNodesMap[ patternEdge->m_Source.lock()->m_Name ];
				UTQLSubgraph::NodePtr target = newNodesMap[ patternEdge->m_Target.lock()->m_Name ];

				// for the input edges in the pattern also add the egde reference to the
				// corresponding matched edges in the srg
				if ( patternEdge->isInput() )
				{
					// Input edge only has to be replicated in the response srg
					// With added edge references

					// duplicate the edge
					UTQLSubgraph::EdgePtr utqlEdge = subgraph->addEdge( patternEdge->m_Name, source, target, 
						UTQLEdge( InOutAttribute::Input ) );

					// get the corresponding edge
					SRGraph::EdgePtr srgEdge = matching.getCorrespondingSrgEdge( patternEdge );

					// merge the attributes of the srg edge into the copy
					utqlEdge->mergeAttributes( *srgEdge );

					// construct an edge reference to the srg edge from
					// the srg subgraph id and the localname in the pattern
					std::string edgeRefEdgeName = srgEdge->m_LocalName;
					std::string edgeRefSubgraph = srgEdge->m_SubgraphID;

					EdgeReference edgeRef( edgeRefSubgraph, edgeRefEdgeName );

					// store edge reference
					utqlEdge->m_EdgeReference = edgeRef;
				}

				if ( patternEdge->isOutput() )
				{
					// duplicate the edge
					UTQLSubgraph::EdgePtr utqlEdge = subgraph->addEdge( patternEdge->m_Name, source, target, 
						UTQLEdge( InOutAttribute::Output ) );

					// move attributes from matching (might contain attribute expressions )
					utqlEdge->swap( matching.m_expandedEdgeAttributes[ patternEdge->m_Name ] );
					LOG4CPP_TRACE( m_logger, "New edge " << patternEdge->m_Name << " " << *patternEdge );

					// copy information sources from matching to edge
					utqlEdge->m_InformationSources = matching.m_informationSources;
				}
			}

			return subgraph;
		}

		/**
		 * decide if a given instance should be applied to the srg.
		 * First stage: The decision is not using the expanded pattern attributes.
		 *
		 * this method checks if a given instance of a pattern should be
		 * applied to the srg and its output edges inserted
		 */
		bool decidePattern1( boost::shared_ptr< Pattern > pattern, const EdgeMatching< UTQLSubgraph, SRGraph >& matching )
		{
			#ifdef DO_SRGMANAGER_TIMING
			UBITRACK_TIME( g_timeDecidePattern1 );
			#endif

			// HACK: run over all input edges and check if there is additional information coming in, compared to one
			// input edge alone. This prevents the server from instantiating things like A^-1 * ( A * B ), but may also prevent 
			// some useful instantiations (e.g. poseSplit->someRotationTransform->poseCombine).

			if ( matching.m_edgeForwardMap.size() > 1 )
			{
				#if MATCHING_EDGE_REQUIREMENTS == ERQ_NEW_INFORMATION_SOURCE
				
					unsigned nNoNewInfoEdges = 0;
					for ( EdgeMatching< UTQLSubgraph, SRGraph >::EdgeForwardMap::const_iterator it = matching.m_edgeForwardMap.begin();
						it != matching.m_edgeForwardMap.end(); it++ )
						for ( EdgeMatching< UTQLSubgraph, SRGraph >::EdgeForwardMap::const_iterator itOther = matching.m_edgeForwardMap.begin();
							itOther != matching.m_edgeForwardMap.end(); itOther++ )
							if ( it != itOther && 
								std::includes( it->second->m_InformationSources.begin(), it->second->m_InformationSources.end(),
									itOther->second->m_InformationSources.begin(), itOther->second->m_InformationSources.end() ) )
								nNoNewInfoEdges++;
					
					if ( nNoNewInfoEdges >= matching.m_edgeForwardMap.size() - 1 )
						return false;
						
				#elif MATCHING_EDGE_REQUIREMENTS == ERQ_DISJOINT_INFORMATION_SOURCES

					for ( EdgeMatching< UTQLSubgraph, SRGraph >::EdgeForwardMap::const_iterator it = matching.m_edgeForwardMap.begin();
						it != matching.m_edgeForwardMap.end(); it++ )
						for ( EdgeMatching< UTQLSubgraph, SRGraph >::EdgeForwardMap::const_iterator itOther = matching.m_edgeForwardMap.begin();
							itOther != matching.m_edgeForwardMap.end(); itOther++ )
							if ( it != itOther )
								for ( std::set< std::string >::const_iterator itSet = it->second->m_InformationSources.begin();
									itSet != it->second->m_InformationSources.end(); itSet++ )
									if ( itOther->second->m_InformationSources.find( *itSet ) != itOther->second->m_InformationSources.end() )
										return false;
					
				#endif
			}

			return true;
		}


		/**
		 * decide if a given instance should be applied to the srg.
		 * Second stage: The decision is using the expanded pattern attributes.
		 *
		 * this method checks if a given instance of a pattern should be
		 * applied to the srg and its output edges inserted
		 *
		 * the descision depends mostly if the newly derived edges
		 * ether have new attributes or contribute new information to the srg
		 */
		bool decidePattern2( boost::shared_ptr< Pattern > pattern, const EdgeMatching< UTQLSubgraph, SRGraph >& matching, 
			std::list< std::string >& supersedes )
		{
			#ifdef DO_SRGMANAGER_TIMING
			UBITRACK_TIME( g_timeDecidePattern2 );
			#endif

			// does this pattern instance create at least one edge with qualities not already conatined in the srg
  			bool createsNewEdge = false;

			// run over all edges in the pattern
			for (std::map< std::string, UTQLSubgraph::EdgePtr >::iterator it = pattern->m_Graph->m_Edges.begin();
				 it != pattern->m_Graph->m_Edges.end(); ++it )
			{
				const UTQLSubgraph::Edge& patternEdge = *it->second;

				// only output edges can contribute to the srg
				if ( patternEdge.isOutput() )
				{
					//get the corresponding source and target nodes in the srg
					SRGraph::NodePtr source = matching.getCorrespondingSrgVertex( patternEdge.m_Source.lock() );
					SRGraph::NodePtr target = matching.getCorrespondingSrgVertex( patternEdge.m_Target.lock() );

					if ( source == target )
						continue;

					// get list of expanded attributes to compare to
					const KeyValueAttributes& expandedAttributes( matching.m_expandedEdgeAttributes.find( patternEdge.m_Name )->second );

					// we have to check all edges orininating in the srg source
					// node and compare them to the currently inspected output edge
					SRGraph::Node::EdgeList outEdges = source->m_OutEdges;

					// did we find an already existing edge in the srg with
					// similar or better qualities than the currently inspected one?
					bool redundant = false;

					// run over all the out edges
					for ( SRGraph::Node::EdgeList::iterator j = outEdges.begin(); !redundant && j != outEdges.end(); ++j )
					{
						SRGraph::EdgePtr srgEdge( j->lock() );
						if ( srgEdge->m_Target.lock() == target )
						{
							// we found anouther edge with the same endpoints check if the edge attributes are 
							// different if the edge attributes are different, we accept this edge right away

							// an edge adds new information if
							//   - a fixed (non-expression) attribute is different
							//   - or at least one of the known expression attributes is "better"
							// rationale for not using unknown expressions: they can cause infinite recursions 
							// if it is not known what value is better.
							bool bFixedAttributesEqual = true;
							bool bBetterKnownAttributes = false;
							bool bAllKnownAttributesBetter = true;
							KeyValueAttributes::AttributeMapType::const_iterator itAttr;
							for ( itAttr = expandedAttributes.map().begin(); itAttr != expandedAttributes.map().end(); itAttr++ )
							{
								KeyValueAttributes::AttributeMapType::const_iterator itOtherAttr = 
									srgEdge->map().find( itAttr->first );

								// is it a fixed attribute?
								if ( patternEdge.hasAttribute( itAttr->first ) )
								{
									// compare its value
									if ( itOtherAttr == srgEdge->map().end() || !( itOtherAttr->second == itAttr->second ) )
									{
										bFixedAttributesEqual = false;
										break;
									}
								}

								// is it a known attribute?
								KnownAttributeMap::const_iterator itKA = m_knownAttributes.find( itAttr->first );
								if ( itKA != m_knownAttributes.end() )
								{
									if ( itOtherAttr == srgEdge->map().end() )
										// attribute not in SRG edge -> edge is better
										bBetterKnownAttributes = true;
									else
									{
										try 
										{
											// Speedup: An attribute is only better if it differs by at least 10%
											double myNum = itAttr->second.getNumber();
											double otherNum = itOtherAttr->second.getNumber();
											if ( itKA->second == biggerIsBetter )
												if ( myNum > otherNum + fabs( otherNum ) * 0.1 )
													bBetterKnownAttributes = true;
												else if ( myNum < otherNum - fabs( otherNum ) * 0.1 )
													bAllKnownAttributesBetter = false;
												else;
											else // smallerIsBetter
												if ( myNum < otherNum - fabs( otherNum ) * 0.1 )
													bBetterKnownAttributes = true;
												else if ( myNum > otherNum + fabs( otherNum ) * 0.1 )
													bAllKnownAttributesBetter = false;
												else;
										}
										catch ( const Util::Exception& )
										{ 
											LOG4CPP_NOTICE( m_logger, "Exception comparing known attribute " << itAttr->first 
												<< "(" << itAttr->second.getText() << " vs. " 
												<< itOtherAttr->second.getText() << ")" ); 
										}
									}
								}
							}

							LOG4CPP_TRACE( m_logger, "Comparing new attribute set\n   " << expandedAttributes << " to\n   " 
								<< *srgEdge << "\n   fixed attributes: " << ( bFixedAttributesEqual ? "equal" : "unequal" )
								<< ", known attributes: " << ( bBetterKnownAttributes ? "" : "not " ) << "better" );

							// not only the endpoints are equal but also the edge attributes. now check if the 
							// edge at least introduces new information by comapring the information sources
							redundant = redundant || 
							#ifndef ALLOW_WORSE_EDGES
								( bFixedAttributesEqual && !bBetterKnownAttributes );
							#else
								( bFixedAttributesEqual && !bBetterKnownAttributes && matching.m_informationSources == srgEdge->m_InformationSources );
							#endif

							// does this edge supersede another edge that can be deleted?
							if ( bFixedAttributesEqual && bBetterKnownAttributes && bAllKnownAttributesBetter )
							{
								// compare if the newly instantiated pattern depends on the pattern we want to delete
								std::map< std::string, UTQLSubgraph::EdgePtr >::iterator itDep;
								for ( itDep = pattern->m_Graph->m_Edges.begin(); itDep != pattern->m_Graph->m_Edges.end(); ++itDep )
									if ( itDep->second->isInput() )
										if ( subgraphDependsOnSubgraph( matching.getCorrespondingSrgEdge( itDep->second )->m_SubgraphID, srgEdge->m_SubgraphID ) )
											break;

								// TODO: check if other pattern has attributes we don't have
								if ( itDep == pattern->m_Graph->m_Edges.end() )
									supersedes.push_back( srgEdge->m_SubgraphID );
							}

							LOG4CPP_TRACE( m_logger, "Redundant: " << ( redundant ? "true" : "false" ) );
						}

					}

					// if at least one non-redundant edge is found, the pattern instance creates a new edge
					if ( !redundant )
						createsNewEdge = true;
				}
			}

			// if the instance creates a new edge, we accept it
			return createsNewEdge;
		}


		/**
		 * apply a pattern instance
		 *
		 * this method applies a detected pattern instance to the world srg
		 * by instanciating the pattern and inserting the new edges into the srg
		 */
		void applyDetectedPattern( boost::shared_ptr< Pattern > pattern, EdgeMatching< UTQLSubgraph, SRGraph >& matching )
		{
			#ifdef DO_SRGMANAGER_TIMING
			UBITRACK_TIME( g_timeApplyDetectedPattern );
			#endif

			// newly created subgraphs also need to have a unique id
			// here we just use the pattern name + some number
			static long patternApplicationID = 2000;
			std::ostringstream oss;
			oss << pattern->m_Name << patternApplicationID;
			patternApplicationID++;

			// instatiate the detected pattern matching using the new subgraph id
			boost::shared_ptr< UTQLSubgraph > subgraph = instanciatePattern( pattern, matching );
			subgraph->m_ID = oss.str();

			// run over all edges in the pattern and insert the output edges into the srg
			// also store references in the utilized input edges for deletion operations
			for ( std::map< std::string, UTQLSubgraph::EdgePtr >::iterator it = pattern->m_Graph->m_Edges.begin();
				it != pattern->m_Graph->m_Edges.end(); ++it )
			{
				UTQLSubgraph::EdgePtr edge = it->second;

				if ( edge->isInput() )
				{
					// Input edges have to back referenced from their dependencies
					// this is neccessary to be able to delete srgs or patterns
					// and detect which other parts of the srg are dependent on this
					SRGraph::EdgePtr srgEdge = matching.getCorrespondingSrgEdge( edge );
					srgEdge->m_DependantSubgraphIDs.insert ( subgraph->m_ID );
				}
				else if ( edge->isOutput() )
				{

					// Output edges also have to be inserted into the srg

					// get the corresponding source and target nodes in the srg
					SRGraph::NodePtr source = matching.getCorrespondingSrgVertex( edge->m_Source.lock() );
					SRGraph::NodePtr target = matching.getCorrespondingSrgVertex( edge->m_Target.lock() );

					// XXXD: edge name convention should be encapsulated somewhere
					// create a new edge name as id + local edge name
					std::string edgeName =  subgraph->m_ID + ":" + edge->m_Name;

					// insert the edge into the srg and store the information sources
					// also store the name of the pattern in the output edges to identify
					// edges which should be deleted if a pattern is deleted.
					// Use attributes of instantiated pattern (AttributeExpressions...)

					SRGraph::EdgePtr e = m_GlobalSrg.addEdge( edgeName, source, target, 
						SRGEdgeAttributes( *subgraph->getEdge( edge->m_Name ), subgraph->m_ID, edge->m_Name ) );
					e->m_InformationSources = matching.m_informationSources;
					e->m_PatternName = pattern->m_Name;
				}
			}

			// store the instanciated pattern subgraph in the subgraph repository
			m_GraphRepository[ subgraph->m_ID ] = InstantiatedPattern( subgraph, pattern->m_clientId );
		}


		/**
		 * Try to apply all known patterns.
		 * 
		 * This method possible needs to be called multiple times to yield the desired result...
		 * @return number of applications
		 */
		unsigned applyAllPatterns()
		{
			#ifdef DO_SRGMANAGER_TIMING
			UBITRACK_TIME( g_timeApplyAllPatterns );
			#endif

			LOG4CPP_DEBUG( m_logger, "statistics: " << m_GraphRepository.size() << " SRG registrations, "
				<< m_Patterns.size() << " patterns, " << m_ActiveQueries.size() << " queries" );

			unsigned nApplications = 0;
			for ( PatternList::iterator it = m_Patterns.begin(); it != m_Patterns.end(); it++ )
				nApplications += applyPattern( *it );
				
			return nApplications;
		}


		/**
		 * create a response from a detected query pattern
		 *
		 * this method creates a response given a pattern derived from
		 * a query and an edge matching.
		 * first the detected query pattern is instanciated, then all the
		 * neccessary subgraphs on which the response depends are collected
		 * recusivly
		 */
		std::list< InstantiatedPattern > generateResponse( boost::shared_ptr< Pattern > pattern, 
			EdgeMatching< UTQLSubgraph, SRGraph >& matching )
		{
			// instatiate the detected pattern matching using the new subgraph id
			InstantiatedPattern subgraph = instanciatePattern( pattern, matching );

			// create a repeatable query ID by concatenating the names of all inputs
			std::string bigUglyRepeatableID;
			
			// collect all the required subgraphs recusively

			// stack to hold all still unresolved edge references
			std::stack< EdgeReference > referenceStack;
			// the collection of required subgraphs
			std::list< InstantiatedPattern > graphCollection;
			// the set of subgraph ids which already have been collected
			std::set< std::string > collectedSubgraphIDs;

			// initialize the reference stack with all referenced
			// edges in the instanciated subgraph

			// run over all edges in the subgraph
			for (std::map< std::string, UTQLSubgraph::EdgePtr >::iterator it = subgraph->m_Edges.begin();
				 it != subgraph->m_Edges.end(); ++it )
			{
				UTQLSubgraph::EdgePtr edge = it->second;

				// the pattern (and thus the instance subgraph) cannot contain
				// any output edges, since the pattern is derived from a query
				assert ( edge->isInput() );

				// get the edge reference
				EdgeReference& edgeRef = edge->m_EdgeReference;

				// if the edge reference is not empty push it onto the stack
				// and record the subgraph id
				if ( edgeRef.getSubgraphId().length() != 0 )
				{
					referenceStack.push( edgeRef );
					collectedSubgraphIDs.insert( edgeRef.getSubgraphId() );
				}
				
				// append to big ugly repeatable unique id
				bigUglyRepeatableID += edgeRef.getSubgraphId() + ":" + edgeRef.getEdgeName () + "%";
			}
			
			// hash the resulting string to make it shorter
			std::size_t numericID = boost::hash< std::string >()( bigUglyRepeatableID );
			std::ostringstream oss;
			oss << pattern->m_Name << std::hex << numericID;
			subgraph->m_ID = oss.str();

			// store the query instance subgraph in the collection
			graphCollection.push_back( subgraph );

			// work till there are no more unmet dependencies are left
			while ( !referenceStack.empty() )
			{
				// pop the top reference
				EdgeReference ref = referenceStack.top();
				referenceStack.pop();

				// get the corresponding subgraph from the subgraph repository
				subgraph = m_GraphRepository[ ref.getSubgraphId() ];

				// recurse and find all dependencies of this subgraph
				// run over all edges and check for additional requirements
				for (std::map< std::string, UTQLSubgraph::EdgePtr >::iterator it = subgraph->m_Edges.begin();
					 it != subgraph->m_Edges.end(); ++it )
				{
					// XXXD: skip output edged for ex?
					// get the referenced subgraph id from the edge
					UTQLSubgraph::EdgePtr edge = it->second;
					std::string refSubgraph = edge->m_EdgeReference.getSubgraphId();

					// if the edge reference is not empty and the subgraph is not
					// already collected, push it on the stack
					if ( ( refSubgraph.length() != 0 ) &&
						 ( collectedSubgraphIDs.find( refSubgraph ) == collectedSubgraphIDs.end() ) )
					{
						referenceStack.push( edge->m_EdgeReference );
						collectedSubgraphIDs.insert( refSubgraph );
					}
				}

				// store the subgraph in the collection
				graphCollection.push_back( subgraph );
			}

			// return the collection as the response
			return graphCollection;
		}


		/**
		 * register an base srg in the world srg
		 *
		 * this method adds a base srg to the world srg and stores the
		 * subgraph in the subgraph repository
		 * the nodes of the base srg are identifyed with the nodes of the
		 * world srg by the qualified names of the nodes (the IDs)
		 * not already present nodes are also created
		 */
		void registerSRG( boost::shared_ptr< UTQLSubgraph > subgraph, const std::string& clientID )
		{
			LOG4CPP_INFO( m_logger, "Registering SRG " << clientID << ":" << subgraph->m_Name << " (ID:" << subgraph->m_ID << ")" );
			
			// to register a base srg we have to find a matching
			// between base srg nodes and nodes in the world srg
			// which are potentially newly created
			std::map< UTQLSubgraph::NodePtr, SRGraph::NodePtr > matching;

			// the base srgs are the base building blocks for the foundation of
			// the world srg. store the subgraph in the subgraph repository
			std::string subgraphID = subgraph->m_ID;
			m_GraphRepository[ subgraphID ] = InstantiatedPattern( subgraph, clientID );

			// add subgraph output to global graph
			// identifying common nodes by equal id

			// run over all nodes of the base srg
			for ( std::map< std::string, UTQLSubgraph::NodePtr >::iterator it = subgraph->m_Nodes.begin();
				 it != subgraph->m_Nodes.end(); ++it )
			{
				UTQLSubgraph::NodePtr node = it->second;

				// ignore input nodes
				// XXXD: base srg cannot have input? also see above..
				if ( !node->isOutput() )
				{
					continue;
				}

				// identify already present nodes in the world srg by common
				// qualified name
				// if the node is already present join the information
				// create a new node otherwise
				if ( m_GlobalSrg.hasNode( node->m_QualifiedName ) )
				{
					SRGraph::NodePtr srgNode = m_GlobalSrg.getNode( node->m_QualifiedName );

					// merge the attributes of the node in the base srg
					// with the attributes of the already present node in
					// the world srg
					m_GlobalSrg.mergeNodeAttributes( srgNode, node, subgraphID );
					// match the already present node for the base srg node
					matching[ node ] = srgNode;
				}
				else
				{
					// if no such node is present, create a new one
					// with the node attributes of the base srg node attributes
					SRGraph::NodePtr srgNode = m_GlobalSrg.addNode( node, subgraphID );
					// store the node matching with the newly created node
					matching[ node ] = srgNode;
				}
			}

			// run over all edges in the base srg
			// as the nodes have all been matched now (either with exisiting
			// or with new nodes) the edges can just be inserted
			for (std::map< std::string, UTQLSubgraph::EdgePtr >::iterator it = subgraph->m_Edges.begin();
				 it != subgraph->m_Edges.end(); ++it )
			{

				// ignore input edges
				// XXXD: base srg cannot have input edges? see above?
				UTQLSubgraph::EdgePtr edge = it->second;
				if ( !edge->isOutput() )
				{
					continue;
				}

				// get the nodes from the node matching found above
				SRGraph::NodePtr src = matching[ edge->m_Source.lock() ];
				SRGraph::NodePtr dst = matching[ edge->m_Target.lock() ];

				// create a new edge name as subgraph id + local edge name in the base srg
				std::string edgeName =  subgraphID + ":" + edge->m_Name;

				// insert the edge into the world srg
				SRGraph::EdgePtr e = m_GlobalSrg.addEdge( edgeName, src, dst, SRGEdgeAttributes( *edge, subgraphID, edge->m_Name ) );

				// a base srg is also an information source atom
				// its edges have exactly the base srg as information source
				e->m_InformationSources.insert( edgeName );
			}
		}


		/**
		 * delete a query
		 *
		 * this method removes a query from the active query repository
		 * as queries have no output edges, no other subgraphs can depend
		 * on a query and also the manifest only in the active queries
		 * list which is only accessed when generating responses.
		 * thus it suffices to delete a query from this respository
		 */
		void deleteQuery( const std::string& queryName, const std::string& clientID )
		{
			LOG4CPP_INFO( m_logger, "Deleting Query " << clientID << ":" << queryName );

			// check if a query by that name is present in the repository
			// for the client id in question
			PatternList::iterator it;
			for ( it = m_ActiveQueries.begin(); it != m_ActiveQueries.end(); it++ )
				if ( (*it)->m_clientId == clientID && (*it)->m_Name == queryName )
				{
					// delete the query from the repository
					m_ActiveQueries.erase( it );
					return;
				}

			UBITRACK_THROW( "No such query " + queryName + " for client ID " + clientID );
		}

		/**
		 * delete a pattern and all dependent edges
		 *
		 * this method removes a pattern from the pattern repository
		 * and also removes any edge from the world srg which was derived
		 * by this pattern
		 * the removal of the derived edges is performed by removing the
		 * instaciated pattern subgraph in the same manner as a base srg
		 * removal
		 * note that this may remove additional edges which depend on the
		 * edges derived by the pattern
		 */
		void deletePattern( const std::string& patternName, const std::string& clientID )
		{
			LOG4CPP_INFO( m_logger, "Deleting pattern " << clientID << ":" << patternName );

			// check edges untill all edges are clean
			// this is necceassary since (recursive) edge deletion may
			// invalidate the iterators strange ways
			bool clean = false;
			while ( !clean )
			{
				// the srg is clean unless an edge to remove is found
				clean = true;

				// run over all edges in the world srg
				std::map< std::string, SRGraph::EdgePtr >::iterator k = m_GlobalSrg.m_Edges.begin();
				while ( k != m_GlobalSrg.m_Edges.end() )
				{
					SRGraph::EdgePtr e = k->second;
					k++;

					// if an edge is found which was derived by the pattern in
					// question, delete the corresponding instanciated pattern
					// subgraph, which can be treated like an base srg here
					const InstantiatedPattern& instance( m_GraphRepository[ e->m_SubgraphID ] );
					if ( instance->m_Name == patternName && instance.m_clientId == clientID )
					{
						// not clean..
						clean = false;
						deleteSRG( e->m_SubgraphID );

						// cannot guarentee that iterator is still valid..
						// better start from the beginning.
						break;
					}
				}
			}

			// also remove the pattern from the pattern repository
			for ( PatternList::iterator it = m_Patterns.begin(); it != m_Patterns.end(); it++ )
				if ( (*it)->m_clientId == clientID && (*it)->m_Name == patternName )
				{
					m_Patterns.erase( it );
					break;
				}
		}

		/**
		 * delete a base srg or instanciated pattern from the srg
		 *
		 * this method removes the edges of a base srg from the
		 * world srg and recursively deletes any edge and corresponding
		 * subgraph with depends on the subgraph.
		 */
		void deleteSRG( const std::string& primalSubgraphID )
		{
			#ifdef DO_SRGMANAGER_TIMING
			UBITRACK_TIME( g_timeDeleteSRG );
			#endif

			LOG4CPP_DEBUG( m_logger, "Deleting SRG " << primalSubgraphID );
			
			// delete subgraph and all which depend on it

			// keep a stack of subgraphs which are still to be deleted
			std::stack< std::string > deleteStack;
			// keep a set of subgraphs which are already deleted
			std::set< std::string > deletedSubgraphs;

			// ?
			std::set< std::string > removableNodes;

			// push the inital subgraph on the stack
			deleteStack.push( primalSubgraphID );

			// work until no more subgraphs are to be deleted
			while ( !deleteStack.empty() )
			{

				if ( m_logger.isTraceEnabled() )
				{
					// debug output
					LOG4CPP_TRACE( m_logger, "------------------------ srg state ------------------------" );
					for (std::map< std::string, SRGraph::EdgePtr >::iterator k = m_GlobalSrg.m_Edges.begin();
						 k != m_GlobalSrg.m_Edges.end(); ++k )
					{
						SRGraph::EdgePtr e = k->second;

						LOG4CPP_TRACE( m_logger, e->m_Name << ": " << e->m_SubgraphID << " pat: " << e->m_PatternName );
					}
					LOG4CPP_TRACE( m_logger, "------------------------ end srg state ------------------------" );
				}

				// get the next subgraph to be deleted from the stack
				std::string subgraphID = deleteStack.top();
				deleteStack.pop();

				LOG4CPP_TRACE( m_logger, "removing top stack element: " << subgraphID );

				// get the subgraph from the repository
				std::map< std::string, InstantiatedPattern >::iterator itSubgraph = m_GraphRepository.find( subgraphID );
				if ( itSubgraph == m_GraphRepository.end() )
				{
					// if the subgraph is no longer in the repository, it must have already been deleted
					// this can happen if different subgraphs at different levels of the stack have
					// common dependencies which were not detected at the time they
					// were put on the stack
					// UBITRACK_THROW( "Trying to remove unregistered subgraph:" + subgraphID );
					LOG4CPP_DEBUG( m_logger, "trying to remove already removed subgraph.." );
					continue;
				}
				boost::shared_ptr< UTQLSubgraph > subgraph = itSubgraph->second;

				// iterate over all edges of the subgraph
				for (std::map< std::string, UTQLSubgraph::EdgePtr >::iterator it = subgraph->m_Edges.begin();
					 it != subgraph->m_Edges.end(); ++it )
				{
					UTQLSubgraph::EdgePtr edge = it->second;


					if ( edge->isInput() )
					{
						// we have cross referenced dependencies of input edges on
						// the output edges of the referenced subgraphs
						// if we remove an input edge, we also have to update
						// the referenced edge and remove the dependency link

						// get the edge reference
						EdgeReference primalEdgeReference = edge->m_EdgeReference;

						// using the convention for edge names in the global srg
						// construct the name of the edge
						// XXXD: see above.. this convention should be encapsulated somewhere..
						std::string primalEdgeName = primalEdgeReference.getSubgraphId() + ":" + primalEdgeReference.getEdgeName();

						// find the edge in the global srg
						if ( !m_GlobalSrg.hasEdge( primalEdgeName ) )
						{
							LOG4CPP_TRACE( m_logger, "edge not present in global srg: " << primalEdgeName );
						}
						else
						{
							LOG4CPP_TRACE( m_logger, "edge present in global srg: " << primalEdgeName << " .. removing dependency" );

							// get the edge
							SRGraph::EdgePtr primalEdge = m_GlobalSrg.getEdge( primalEdgeName );
							// remove the dependency entry
							primalEdge->m_DependantSubgraphIDs.erase( subgraphID );
						}
					}
					if ( edge->isOutput() )
					{
						// if the edge is an output edge, we have to remove the
						// edge and mark all dependent edges to be deleted as well

						// construct the edge name and get the edge from the
						// global srg
						std::string edgeName =  subgraphID + ":" + edge->m_Name;
						SRGraph::EdgePtr srgEdge = m_GlobalSrg.getEdge( edgeName );

						// run over all dependent subgraph entries in the edge
						for ( std::set< std::string >::iterator depSubgraph = srgEdge->m_DependantSubgraphIDs.begin();
							  depSubgraph != srgEdge->m_DependantSubgraphIDs.end(); ++depSubgraph )
						{
							// if the subgraph is already marked as deleted we ignore it
							if ( deletedSubgraphs.find( *depSubgraph ) == deletedSubgraphs.end() )
							{
								// otherwise push the subgraphid on the stack and
								// insert the subgraphid into the set of deleted
								// subgrpahs
								deletedSubgraphs.insert( *depSubgraph );
								deleteStack.push( *depSubgraph );
							}
						}

						// actually remove the edge from the global srg
						LOG4CPP_TRACE( m_logger, "removing output edge: " << edgeName );
						m_GlobalSrg.removeEdge( edgeName );
					}
				}

				// iterate over all nodes of the current subgraph
				for (std::map< std::string, UTQLSubgraph::NodePtr >::iterator it = subgraph->m_Nodes.begin();
					 it != subgraph->m_Nodes.end(); ++it )
				{
					UTQLSubgraph::NodePtr node = it->second;

					if ( !node->isOutput() )
					{
						// we can ignore input nodes
						continue;
					}

					if ( !m_GlobalSrg.hasNode( node->m_QualifiedName ) )
					{
						// every registered node has to have a qualified name
						UBITRACK_THROW( "Trying to remove unregistered node:" + node->m_QualifiedName );
					}

					// as the node of the global srg may be the union of nodes
					// from different base srg, we keep a list of subgraphs
					// which spawn this node. when removing a subgraph
					// we remove the subgraph from this list and delete the node
					// if it becomes empty

					// note that we also should remove the additional
					// attributes the node gained from this subgraph, but
					// that's not trivial..

					// remove subgraph from node spawning list and node from reference list
					SRGraph::NodePtr srgNode = m_GlobalSrg.getNode( node->m_QualifiedName );
					srgNode->m_SubgraphIDs.erase( subgraphID );

					// also remove the subgraph node from the list of noderefs in
					// the global srg. the noderefs are used to update
					// pattern instance nodes if srg node attributes are updated
					srgNode->m_NodeRefs.erase( node );

					// if no more entries: mark node for deletion
					// we only makr for deletion and delete later to avoid
					// iterator invalidation here..
					if ( srgNode->m_SubgraphIDs.empty() )
					{
						LOG4CPP_TRACE( m_logger, "marking node for removal: " << node->m_QualifiedName );
						removableNodes.insert( node->m_QualifiedName );
					}
				}

				// delete subgraph from subgraoh repository
				m_GraphRepository.erase( subgraphID );
			}

			if ( m_logger.isTraceEnabled() )
			{
				// debug output
				LOG4CPP_TRACE( m_logger, "------------------------ srg state ------------------------" );
				for (std::map< std::string, SRGraph::EdgePtr >::iterator k = m_GlobalSrg.m_Edges.begin();
					 k != m_GlobalSrg.m_Edges.end(); ++k )
				{
					SRGraph::EdgePtr e = k->second;

					LOG4CPP_TRACE( m_logger, e->m_Name << ": " << e->m_SubgraphID << " pat: " << e->m_PatternName );
				}
				LOG4CPP_TRACE( m_logger, "------------------------ end srg state ------------------------" );
			}

			// remove all node which have been marked for deletion
			for ( std::set< std::string >::iterator pNodeName = removableNodes.begin();
				  pNodeName != removableNodes.end(); ++pNodeName )
			{
				LOG4CPP_DEBUG( m_logger, "Removing node: " << *pNodeName );
				m_GlobalSrg.removeNode( *pNodeName );
			}

			LOG4CPP_TRACE( m_logger, "Done in deleteSRG" );
		}


		/** returns true if a subgraph depends on another given subgraphId */
		bool subgraphDependsOnSubgraph( const std::string& edgeSubgraphId, const std::string& subgraphId )
		{
			if ( edgeSubgraphId == subgraphId )
				return true;

			// find the subgraph this edge belongs to
			boost::shared_ptr< UTQLSubgraph > pSubgraph = m_GraphRepository[ edgeSubgraphId ];

			// run over all edges in the subgraph
			for ( std::map< std::string, UTQLSubgraph::EdgePtr >::iterator it = pSubgraph->m_Edges.begin();
				 it != pSubgraph->m_Edges.end(); ++it )
			{
				UTQLSubgraph::EdgePtr edge = it->second;
				if ( edge->isInput() )
				{
					// get the edge reference
					EdgeReference& edgeRef = edge->m_EdgeReference;

					if ( subgraphDependsOnSubgraph( edgeRef.getSubgraphId(), subgraphId ) )
						return true;
				}
			}
			
			return false;
		}


		/** Debug output: logs the current SRG state */
		void logCurrentSRG() const
		{
			if ( !m_logger.isDebugEnabled() )
				return;

			// print SRG
			std::ostringstream os;
			for ( std::map< std::string, SRGraph::NodePtr >::const_iterator it = m_GlobalSrg.m_Nodes.begin();
				  it != m_GlobalSrg.m_Nodes.end(); ++it )
			{
				os << it->first << ":";

				// print node attributes
				os << *it->second << std::endl;

				// put all edges in vector
				std::vector< SRGraph::EdgePtr > sortedEdges;
				for ( SRGraph::Node::EdgeList::const_iterator j = it->second->m_OutEdges.begin();
					  j != it->second->m_OutEdges.end(); ++j )
					sortedEdges.push_back( j->lock() );

				// sort them
				std::sort( sortedEdges.begin(), sortedEdges.end(), edgeSortPrintHelper );

				// print them in sorted order
				for ( std::vector< SRGraph::EdgePtr >::iterator j = sortedEdges.begin(); j != sortedEdges.end(); j++ )
				{
					SRGraph::EdgePtr e = *j;
					os << "\t-> " << e->m_Target.lock()->m_Name << " [" << e->m_Name << "] ";

					// print edge attributes
					os << *e;

					// print information sources
					os << "< ";
					for ( std::set< std::string >::const_iterator k = e->m_InformationSources.begin();
						  k != e->m_InformationSources.end(); ++k )
						os << *k << " ";
					os << ">";

					os << std::endl;
				}
			}

			// print the DFN (all SRG edges and their inputs)
			for ( std::map< std::string, SRGraph::EdgePtr >::const_iterator it = m_GlobalSrg.m_Edges.begin();
				  it != m_GlobalSrg.m_Edges.end(); ++it )
			{
				const SRGraph::Edge& srgEdge( *(it->second) );
				os << srgEdge.m_Name << ": " 
					<< srgEdge.m_Source.lock()->getAttributeString( "id" ) << " -> "
					<< srgEdge.m_Target.lock()->getAttributeString( "id" );

				// iterate all input edges of the component
				os << ", inputs: { ";
				const UTQLSubgraph& component( *m_GraphRepository.find( srgEdge.m_SubgraphID )->second );
				for ( UTQLSubgraph::EdgeMap::const_iterator itEdge = component.m_Edges.begin(); 
					itEdge != component.m_Edges.end(); itEdge++ )
					if ( itEdge->second->isInput() )
						os << itEdge->second->m_EdgeReference.getSubgraphId() << ":" <<
							itEdge->second->m_EdgeReference.getEdgeName() << ", ";

				os << "}" << std::endl;
			}
			
			LOG4CPP_DEBUG( m_logger, "current SRG:" << std::endl << os.str() );
			LOG4CPP_DEBUG( m_logger, "Total: " << m_GlobalSrg.m_Nodes.size() << " nodes, " 
				<< m_GlobalSrg.m_Edges.size() << " edges, " << m_GraphRepository.size() << " instantiated patterns" );
		}

		/** helper function to sort edges to print more readable results */
		static bool edgeSortPrintHelper(  SRGraph::EdgePtr e1,  SRGraph::EdgePtr e2 )
		{
			// sort by target name
			if ( e1->m_Target.lock()->m_Name != e2->m_Target.lock()->m_Name )
				return e1->m_Target.lock()->m_Name < e2->m_Target.lock()->m_Name;
			// order by "type" attribute
			if ( e1->hasAttribute( "type" ) && e2->hasAttribute( "type" ) &&
				e1->getAttributeString( "type" ) != e2->getAttributeString( "type" ) )
				return e1->getAttributeString( "type" ) < e2->getAttributeString( "type" );
			// order by "mode" attribute
			if ( e1->hasAttribute( "mode" ) && e2->hasAttribute( "mode" ) &&
				e1->getAttributeString( "mode" ) != e2->getAttributeString( "mode" ) )
				return e1->getAttributeString( "mode" ) < e2->getAttributeString( "mode" );
			// order by number of information sources
			if ( e1->m_InformationSources.size() != e2->m_InformationSources.size() )
				return e1->m_InformationSources.size() < e2->m_InformationSources.size();
			// finally, order by name
			return e1->m_Name < e2->m_Name;
		}

		// the global srg
		SRGraph m_GlobalSrg;

		// the repository of subgraphs
		std::map< std::string, InstantiatedPattern > m_GraphRepository;

		// shortcut for list of pattern 
		typedef std::list< boost::shared_ptr< Pattern > > PatternList;

		// the pattern repository
		PatternList m_Patterns;

		// list of all query patterns
		PatternList m_ActiveQueries;

		// describes known attributes
		enum KnownAttributeType { biggerIsBetter, smallerIsBetter };
		typedef std::map< std::string, KnownAttributeType > KnownAttributeMap;

		/** map of known attributes */
		KnownAttributeMap m_knownAttributes;

		log4cpp::Category& m_logger;

	};
}}

#endif // __Ubitrack_Graph_UTQLSRGManager_INCLUDED__
