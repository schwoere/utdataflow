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

#ifndef __Ubitrack_Graph_EdgeMatching_INCLUDED__
#define __Ubitrack_Graph_EdgeMatching_INCLUDED__ __Ubitrack_Graph_EdgeMatching_INCLUDED__

#include <utDataflow.h>
#include <iostream>

namespace Ubitrack { namespace Graph {

	template< class G, class H > class EdgeMatching
	{
		// this is really just a collection of:
		// edge matching, node matching and back matching

	public:

		typedef typename G::EdgePtr PatternEdgePtr;
		typedef typename G::NodePtr PatternNodePtr;

		typedef typename H::EdgePtr SRGEdgePtr;
		typedef typename H::NodePtr SRGNodePtr;

		EdgeMatching()
		{}

		// ---------------- Add/Remove ----------------------------------------

		// add the matching of e (in the pattern) to f (in the SRG)
		// add forward and backward matching of edge and nodes
		void addMatchedEdge ( const PatternEdgePtr e, const SRGEdgePtr f )
		{
			// const SRGEdgeProperties& eProperties =
			// check if edge is alrady matched
			assert( !isPatternEdgeMatched ( e ) );
			assert( !isSrgEdgeMatched( f ) );

			PatternNodePtr ue, ve;
			SRGNodePtr uf, vf;

			ue = e->m_Source.lock();
			ve = e->m_Target.lock();
			uf = f->m_Source.lock();
			vf = f->m_Target.lock();

			assert( isEdgeMatchingCompatible ( ue, ve, uf, vf ) );

			m_edgeForwardMap[e] = f;
			m_edgeBackwardMap[f] = e;

			m_vertexForwardMap[ue].m_correspondence = uf;
			m_vertexForwardMap[ue].m_associationCount++;

			m_vertexForwardMap[ve].m_correspondence = vf;
			m_vertexForwardMap[ve].m_associationCount++;

			m_vertexBackwardMap[uf].m_correspondence = ue;
			m_vertexBackwardMap[uf].m_associationCount++;

			m_vertexBackwardMap[vf].m_correspondence = ve;
			m_vertexBackwardMap[vf].m_associationCount++;
		}

		void addMatchedVertex( const PatternNodePtr u, const SRGNodePtr v )
		{
			assert( isVertexMatchingCompatible( u, v ) );

			m_vertexForwardMap[u].m_correspondence = v;
			m_vertexForwardMap[u].m_associationCount++;

			m_vertexBackwardMap[v].m_correspondence = u;
			m_vertexBackwardMap[v].m_associationCount++;
		}

		void removeMatchedEdge ( const PatternEdgePtr e )
		{
			if ( !isPatternEdgeMatched ( e ) )
			{
				UBITRACK_THROW ( "Edge not matched in removeMatchedEdge" );
			}
			removeMatchedEdge( e, getCorrespondingSrgEdge ( e ) );
		}

		void removeMatchedEdge ( const SRGEdgePtr f )
		{
			if ( !isSrgEdgeMatched ( f ) )
			{
				UBITRACK_THROW ( "Edge not matched in removeMatchedEdge" );
			}
			removeMatchedEdge( getCorrespondingPatternEdge ( f ), f );
		}

		void removeMatchedEdge ( const PatternEdgePtr e, const SRGEdgePtr f )
		{
			if ( !isPatternEdgeMatched ( e ) || !isSrgEdgeMatched ( f ) )
			{
				UBITRACK_THROW ( "Edge not matched in removeMatchedEdge" );
			}

			PatternNodePtr ue, ve;
			SRGNodePtr uf, vf;

			ue = e->m_Source.lock();
			ve = e->m_Target.lock();
			uf = f->m_Source.lock();
			vf = f->m_Target.lock();

			m_vertexBackwardMap[vf].m_associationCount--;
			if ( m_vertexBackwardMap[vf].m_associationCount == 0 )
			{
				m_vertexBackwardMap.erase ( vf );
			}

			m_vertexBackwardMap[uf].m_associationCount--;
			if ( m_vertexBackwardMap[uf].m_associationCount == 0 )
			{
				m_vertexBackwardMap.erase ( uf );
			}

			m_vertexForwardMap[ve].m_associationCount--;
			if ( m_vertexForwardMap[ve].m_associationCount == 0 )
			{
				m_vertexForwardMap.erase ( ve );
			}

			m_vertexForwardMap[ue].m_associationCount--;
			if ( m_vertexForwardMap[ue].m_associationCount == 0 )
			{
				m_vertexForwardMap.erase ( ue );
			}

			m_edgeBackwardMap.erase ( f );
			m_edgeForwardMap.erase ( e );
		}

		// -----------------------------------------------------------

// 		bool isEdgeMatched ( const EdgePtr e )
// 		{
// 			return ( isPatternEdgeMatched ( e ) || isSrgEdgeMatched ( e ) );
// 		}

		bool isPatternEdgeMatched ( const PatternEdgePtr e ) const
		{
			return ( m_edgeForwardMap.find ( e ) != m_edgeForwardMap.end() );
		}

		bool isSrgEdgeMatched ( const SRGEdgePtr e ) const
		{
			return ( m_edgeBackwardMap.find ( e ) != m_edgeBackwardMap.end() );
		}

		SRGEdgePtr getCorrespondingSrgEdge ( const PatternEdgePtr e ) const
		{
			typename EdgeForwardMap::const_iterator it = m_edgeForwardMap.find( e );
			if ( it == m_edgeForwardMap.end() )
			{
				UBITRACK_THROW ( "Edge not matched in getCorrespondingSrgEdge: " );
			}
			return it->second;
		}

		PatternEdgePtr getCorrespondingPatternEdge ( const SRGEdgePtr e ) const
		{
			typename EdgeBackwardMap::const_iterator it( m_edgeBackwardMap.find( e ) );
			if ( it == m_edgeBackwardMap.end() )
			{
				UBITRACK_THROW ( "Edge not matched in getCorrespondingPatternEdge: " );
			}
			return it->second;
		}

		// -----------------------------------------------------------

// 		bool isVertexMatched ( const NodePtr u )
// 		{
// 			return ( isPatternVertexMatched ( u ) || isSrgVertexMatched ( u ) );
// 		}

		bool isPatternVertexMatched ( const PatternNodePtr u ) const
		{
			return ( m_vertexForwardMap.find ( u ) != m_vertexForwardMap.end() );
		}

		bool isSrgVertexMatched ( const SRGNodePtr u ) const
		{
			return ( m_vertexBackwardMap.find ( u ) != m_vertexBackwardMap.end() );
		}

		SRGNodePtr getCorrespondingSrgVertex ( const PatternNodePtr u ) const
		{
			typename VertexForwardMap::const_iterator it = m_vertexForwardMap.find( u );
			if ( it == m_vertexForwardMap.end() )
			{
				UBITRACK_THROW ( "Vertex " + u->m_Name + " not matched in getCorrespondingSrgVertex" );
			}
			return it->second.m_correspondence;
		}

		PatternNodePtr getCorrespondingPatternVertex ( const SRGNodePtr u ) const
		{
			typename VertexBackwardMap::const_iterator it = m_vertexBackwardMap.find( u );
			if ( it == m_vertexBackwardMap.end() )
			{
				UBITRACK_THROW ( "Vertex " + u->m_Name + " not matched in getCorrespondingPatternVertex" );
			}
			return it->second.m_correspondence;
		}

		// -----------------------------------------------------------

		bool isEdgeMatchingCompatible ( const PatternEdgePtr e, const SRGEdgePtr f)
		{
			PatternNodePtr ue, ve;
			SRGNodePtr uf, vf;

			ue = e->m_Source.lock();
			ve = e->m_Target.lock();
			uf = f->m_Source.lock();
			vf = f->m_Target.lock();

			return isEdgeMatchingCompatible ( ue, ve, uf, vf );
		}

		bool isVertexMatchingCompatible( const PatternNodePtr u, const SRGNodePtr v )
		{
			if ( isPatternVertexMatched ( u ) && getCorrespondingSrgVertex( u ) != v )
			{
				return false;
			}
			return true;
		}

		bool isEdgeMatchingCompatible ( const PatternNodePtr ue, const PatternNodePtr ve, const SRGNodePtr uf, const SRGNodePtr vf )
		{
			return ( isVertexMatchingCompatible( ue, uf ) && isVertexMatchingCompatible( ve, vf ) );
		}

		// private:

		template< class X > struct vertexMapData
		{
			vertexMapData()
				: m_associationCount (0)
			{}

			X m_correspondence;
			int m_associationCount;
		};

		typedef std::map< PatternEdgePtr, SRGEdgePtr > EdgeForwardMap;
		typedef std::map< SRGEdgePtr, PatternEdgePtr > EdgeBackwardMap;
		typedef std::map< PatternNodePtr, vertexMapData< SRGNodePtr > > VertexForwardMap;
		typedef std::map< SRGNodePtr, vertexMapData< PatternNodePtr > > VertexBackwardMap;

		EdgeForwardMap m_edgeForwardMap;
		EdgeBackwardMap m_edgeBackwardMap;
		VertexForwardMap m_vertexForwardMap;
		VertexBackwardMap m_vertexBackwardMap;

		typedef std::map< std::string, KeyValueAttributes > ExpandedAttributesMap;

		/** expanded attributes of the resulting output edges */
		ExpandedAttributesMap m_expandedEdgeAttributes;

		/** expanded attributes of the resulting output edges */
		ExpandedAttributesMap m_expandedNodeAttributes;

		/** map of attribute maps */
		typedef std::map< std::string, const KeyValueAttributes* > AttributeObjectRefs;

		/** 
		 * References to all input nodes and edges.
		 * Required for global predicates, AttributeExpressions and BestMatchExpressions.
		 */
		AttributeObjectRefs m_allInputAttributes;

		/** set of information sources of the resulting edges */
		std::set< std::string > m_informationSources;
		
		/** current step index in the search plan */
		int m_iSearchPlanStep;
	};


}}

#endif // __Ubitrack_Graph_EdgeMatching_INCLUDED__
