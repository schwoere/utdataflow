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
5~ * 02110-1301 USA, or see the FSF site: http://www.fsf.org.
 */


/**
 * @ingroup srg_algorithms
 *
 * @author Manuel Huber <huberma@in.tum.de>
 */

#ifndef __Ubitrack_Graph_Pattern_INCLUDED__
#define __Ubitrack_Graph_Pattern_INCLUDED__ __Ubitrack_Graph_Pattern_INCLUDED__

#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>

#include <utDataflow.h>
#include <utGraph/EdgeMatching.h>

#include <utGraph/SRGraph.h>
#include <utGraph/UTQLSubgraph.h>
#include <utGraph/EdgeMatching.h>
#include <utGraph/Predicate.h>
#include <utGraph/EvaluationContext.h>

#include <stack>

namespace Ubitrack { namespace Graph {

	/** internal information about a pattern */
	class Pattern
    {
    public:
		/** constructor */
		Pattern( boost::shared_ptr< UTQLSubgraph > pGraph, const std::string& clientId )
			: m_Name( pGraph->m_Name )
			, m_clientId( clientId )
			, m_Graph( pGraph )
		{
			// std::cout << "***** New search plan for " << m_Name << std::endl;
			if ( pGraph->m_Nodes.empty() )
				return;
				
			// construct simple search plan
			std::stack< UTQLSubgraph::NodePtr > nodeStack;
			std::set< std::string > matchedEdges;
			std::set< std::string > matchedNodes;
			
			// first find input node with id predicate
			UTQLSubgraph::NodePtr pFirstPredicateNode;
			BOOST_FOREACH( UTQLSubgraph::NodeMap::value_type& node, pGraph->m_Nodes )
				if ( node.second->m_Tag == InOutAttribute::Input && !node.second->m_predicateList.empty() )
				{
					Predicate::AttribList attribs = node.second->m_predicateList.front()->getConjunctiveEqualities();
					BOOST_FOREACH( Predicate::AttribList::value_type& a, attribs )
						if ( a.first == "id" )
						{
							m_searchPlan.push_back( SearchPlanElement( node.second, a.second ) );
							matchedNodes.insert( node.first );
							nodeStack.push( node.second );
							break;
						}
						
					if ( !pFirstPredicateNode )
						pFirstPredicateNode = node.second;
				}

			if ( nodeStack.empty() )
				// no id node
				if ( pFirstPredicateNode )
				{
					// if no id node was found prefer nodes with any predicate
					m_searchPlan.push_back( SearchPlanElement( pFirstPredicateNode ) );
					matchedNodes.insert( pFirstPredicateNode->m_Name );
					nodeStack.push( pFirstPredicateNode );
				}
				else
				{
					// no nodes with predicates -> start with first input edge
					BOOST_FOREACH( UTQLSubgraph::EdgeMap::value_type& edge, pGraph->m_Edges )
						if ( edge.second->m_Tag == InOutAttribute::Input )
						{
							m_searchPlan.push_back( SearchPlanElement( edge.second ) );
							matchedEdges.insert( edge.first );
							
							nodeStack.push( edge.second->m_Source.lock() );
							matchedNodes.insert( nodeStack.top()->m_Name );
							
							nodeStack.push( edge.second->m_Target.lock() );
							matchedNodes.insert( nodeStack.top()->m_Name );
							break;
						}
				}
				
			// graph search to find all edges in a connecting order
			do
			{
				while ( !nodeStack.empty() )
				{
					UTQLSubgraph::NodePtr pNode = nodeStack.top();
					nodeStack.pop();
					
					// follow out edges
					BOOST_FOREACH( boost::weak_ptr< UTQLSubgraph::Edge > pWeakEdge, pNode->m_OutEdges )
					{
						UTQLSubgraph::EdgePtr pEdge( pWeakEdge.lock() );
						if ( pEdge->m_Tag == InOutAttribute::Input && matchedEdges.find( pEdge->m_Name ) == matchedEdges.end() )
						{
							// not yet matched -> add to plan
							m_searchPlan.push_back( SearchPlanElement( pEdge ) );
							matchedEdges.insert( pEdge->m_Name );
							
							// add other node
							UTQLSubgraph::NodePtr pOtherNode = pEdge->m_Target.lock();
							if ( matchedNodes.find( pOtherNode->m_Name ) == matchedNodes.end() )
							{
								if ( !pOtherNode->m_predicateList.empty() )
									// only add nodes that need to be attribute checked to search plan
									m_searchPlan.push_back( SearchPlanElement( pOtherNode ) );
									
								matchedNodes.insert( pOtherNode->m_Name );
								nodeStack.push( pOtherNode );
							}
						}
					}
						
					// follow in edges
					BOOST_FOREACH( boost::weak_ptr< UTQLSubgraph::Edge > pWeakEdge, pNode->m_InEdges )
					{
						UTQLSubgraph::EdgePtr pEdge( pWeakEdge.lock() );
						if ( pEdge->m_Tag == InOutAttribute::Input && matchedEdges.find( pEdge->m_Name ) == matchedEdges.end() )
						{
							// not yet matched -> add to plan
							m_searchPlan.push_back( SearchPlanElement( pEdge ) );
							matchedEdges.insert( pEdge->m_Name );
							
							// add other node
							UTQLSubgraph::NodePtr pOtherNode = pEdge->m_Source.lock();
							if ( matchedNodes.find( pOtherNode->m_Name ) == matchedNodes.end() )
							{
								if ( !pOtherNode->m_predicateList.empty() )
									// only add nodes that need to be attribute checked to search plan
									m_searchPlan.push_back( SearchPlanElement( pOtherNode ) );
									
								matchedNodes.insert( pOtherNode->m_Name );
								nodeStack.push( pOtherNode );
							}
						}
					}
				}
				
				// any unmatched input nodes left? (prefer those with predicates)
				BOOST_FOREACH( UTQLSubgraph::NodeMap::value_type& node, pGraph->m_Nodes )
					if ( node.second->m_Tag == InOutAttribute::Input && matchedNodes.find( node.first ) == matchedNodes.end() )
					{
						m_searchPlan.push_back( SearchPlanElement( node.second ) );
						matchedNodes.insert( node.first );
						nodeStack.push( node.second );
						break;
					}
			}
			while ( !nodeStack.empty() );
		}

		/** name of the pattern */
		std::string m_Name;

		/** client id of the pattern */
		std::string m_clientId;

		/** the utql pattern description */
		boost::shared_ptr< UTQLSubgraph > m_Graph;

		struct SearchPlanElement
		{
			SearchPlanElement( UTQLSubgraph::NodePtr pNode, const std::string& sId = std::string() )
				: m_pNode( pNode )
				, m_sId( sId )
			{
				//std::cout << "Checking node " << pNode->m_Name << std::endl;
			}
			
			SearchPlanElement( UTQLSubgraph::EdgePtr pEdge )
				: m_pEdge( pEdge )
			{
				//std::cout << "Checking edge " << pEdge->m_Name << std::endl;
			}
			
			UTQLSubgraph::NodePtr m_pNode;
			UTQLSubgraph::EdgePtr m_pEdge;
			std::string m_sId;
		};
		
		std::vector< SearchPlanElement > m_searchPlan;
		
		// code the world is not yet ready for..
		// bool m_hasCorrespondence;
		// std::set< SrGraph::Node > m_FreeNodes;
		// std::string m_Expansion = "S";
		// Node u1, u2, v1, v2
		// Multi ?
		//
    };

	class PatternFunc
	{
	public:

		static bool isVertexCompatible( const UTQLSubgraph::NodePtr patternNode, const SRGraph::NodePtr srgNode )
		{
			for ( std::list< boost::shared_ptr< Predicate > >::const_iterator predicate = patternNode->m_predicateList.begin();
				  predicate != patternNode->m_predicateList.end(); ++predicate )
			{
				try 
				{ 
					if ( !(*predicate)->evaluate( EvaluationContext( *srgNode ) ) )
						return false;
				}
				catch( ... )
				{ return false; }

			}
			return true;
		}

		static bool isEdgeCompatible( UTQLSubgraph::EdgePtr patternEdge, SRGraph::EdgePtr srgEdge )
		{
			for ( std::list< boost::shared_ptr< Predicate > >::const_iterator predicate = patternEdge->m_predicateList.begin();
				  predicate != patternEdge->m_predicateList.end(); ++predicate )
			{
				try 
				{ 
					if ( !(*predicate)->evaluate( EvaluationContext( *srgEdge ) ) )
						return false;
				}
				catch( ... )
				{ return false; }

			}
			return true;
		}


		static boost::shared_ptr< std::vector< EdgeMatching< UTQLSubgraph, SRGraph > > > checkPattern ( boost::shared_ptr< Pattern > p, SRGraph& srg )
		{
			boost::shared_ptr< std::vector< EdgeMatching< UTQLSubgraph, SRGraph > > > detectedMatches ( new std::vector< EdgeMatching< UTQLSubgraph, SRGraph > > );

			std::stack < EdgeMatching< UTQLSubgraph, SRGraph > > stack;

			// start with a completely unmatched pattern
			stack.push( EdgeMatching< UTQLSubgraph, SRGraph >() );
			stack.top().m_iSearchPlanStep = 0;

			// while stack is not empty
			while ( !stack.empty() )
			{
				EdgeMatching< UTQLSubgraph, SRGraph > state = stack.top();
				stack.pop();
				
				unsigned iSearchPlanStep = state.m_iSearchPlanStep;
				state.m_iSearchPlanStep++;

				if ( iSearchPlanStep == p->m_searchPlan.size() )
				{
					// all edges and nodes matched
					detectedMatches->push_back( state );
					continue;
				}

				if ( p->m_searchPlan[ iSearchPlanStep ].m_pEdge )
				{
					// search plan says: match edge.
					UTQLSubgraph::EdgePtr pEdge = p->m_searchPlan[ iSearchPlanStep ].m_pEdge;
					bool bSourceMatched = state.isPatternVertexMatched( pEdge->m_Source.lock() );
					bool bTargetMatched = state.isPatternVertexMatched( pEdge->m_Target.lock() );
					
					if ( bSourceMatched )
					{
						// source is already matched
						SRGraph::NodePtr pStartNode = state.getCorrespondingSrgVertex( pEdge->m_Source.lock() );
						BOOST_FOREACH( boost::weak_ptr< SRGraph::Edge > pWeakEdge, pStartNode->m_OutEdges )
						{
							SRGraph::EdgePtr pSrgEdge( pWeakEdge.lock() );
							
							if ( state.isSrgEdgeMatched( pSrgEdge ) )
								continue;
								
							if ( bTargetMatched && state.getCorrespondingSrgVertex( pEdge->m_Target.lock() ) != pSrgEdge->m_Target.lock() )
								continue;
								
							if ( !isEdgeCompatible( pEdge, pSrgEdge ) )
								continue;
								
							// found match -> add refined EdgeMatching to stack
							stack.push( state );
							stack.top().addMatchedEdge( pEdge, pSrgEdge );
						}
					}
					else if ( bTargetMatched )
					{
						// target is already matched
						SRGraph::NodePtr pStartNode = state.getCorrespondingSrgVertex( pEdge->m_Target.lock() );
						BOOST_FOREACH( boost::weak_ptr< SRGraph::Edge > pWeakEdge, pStartNode->m_InEdges )
						{
							SRGraph::EdgePtr pSrgEdge( pWeakEdge.lock() );
							
							if ( state.isSrgEdgeMatched( pSrgEdge ) )
								continue;
								
							if ( bSourceMatched && state.getCorrespondingSrgVertex( pEdge->m_Source.lock() ) != pSrgEdge->m_Source.lock() )
								continue;
								
							if ( !isEdgeCompatible( pEdge, pSrgEdge ) )
								continue;
								
							// found match -> add refined EdgeMatching to stack
							stack.push( state );
							stack.top().addMatchedEdge( pEdge, pSrgEdge );
						}
					}
					else
					{
						// neither source nor target matched -> check all edges
						BOOST_FOREACH( SRGraph::EdgeMap::value_type& edge, srg.m_Edges )
						{
							if ( state.isSrgEdgeMatched( edge.second ) )
								continue;
								
							if ( state.isSrgVertexMatched( edge.second->m_Source.lock() ) )
								continue;

							if ( state.isSrgVertexMatched( edge.second->m_Target.lock() ) )
								continue;

							if ( !isEdgeCompatible( pEdge, edge.second ) )
								continue;
								
							// found match -> add refined EdgeMatching to stack
							stack.push( state );
							stack.top().addMatchedEdge( pEdge, edge.second );
						}
					}
				}
				else
				{
					// search plan says: match vertex
					UTQLSubgraph::NodePtr pVertex = p->m_searchPlan[ iSearchPlanStep ].m_pNode;
					
					if ( state.isPatternVertexMatched( pVertex ) )
					{
						// already matched -> only check attributes
						if ( !isVertexCompatible( pVertex, state.getCorrespondingSrgVertex( pVertex ) ) )
							continue;
							
						// push refined EdgeMatching onto stack
						stack.push( state );
					}
					else
					{
						const std::string& sId = p->m_searchPlan[ iSearchPlanStep ].m_sId;
						if ( sId.empty() )
						{
							// try all srg nodes
							BOOST_FOREACH( SRGraph::NodeMap::value_type& node, srg.m_Nodes )
							{
								if ( state.isSrgVertexMatched( node.second ) )
									continue;
									
								if ( !isVertexCompatible( pVertex, node.second ) )
									continue;

								// found new matching vertex -> push refined EdgeMatching onto stack
								stack.push( state );
								stack.top().addMatchedVertex( pVertex, node.second );
							}
						}
						else
						{
							// directly find node by id
							if ( srg.hasNode( sId ) )
							{
								stack.push( state );
								stack.top().addMatchedVertex( pVertex, srg.getNode( sId ) );
							}
						}
					}
				}
			}
			
			return detectedMatches;
		}

	};

}}

#endif // __Ubitrack_Graph_Pattern_INCLUDED__
