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
 * Context for the evaluation of attributes and predicates
 *
 * @author Daniel Pustka <daniel.pustka@in.tum.de>
 */

#ifndef __Ubitrack_Graph_EvaluationContext_H_INCLUDED__
#define __Ubitrack_Graph_EvaluationContext_H_INCLUDED__

#include <string>
#include <map>
#include <assert.h>
#include "UTQLSubgraph.h"
#include "SRGraph.h"
#include "EdgeMatching.h"

namespace Ubitrack { namespace Graph {

/**
 * The evaluation context contains all relevent information to evaluate predicates and attribute
 * expressions, such as the attributes of the selected node/edge, the full matching etc.
 */
class UTDATAFLOW_EXPORT EvaluationContext
{
public:
	/** construct from SRGNodeAttributes of a node/edge */
	EvaluationContext( const SRGNodeAttributes& attrs )
		: m_pNodeEdgeAttributes( &attrs )
		, m_pPatternAttributes( &attrs )
		, m_pMatching( 0 )
	{}
	
	/** construct from SRGNodeAttributes of a node/edge */
	EvaluationContext( const SRGEdgeAttributes& attrs )
		: m_pNodeEdgeAttributes( &attrs )
		, m_pPatternAttributes( &attrs )
		, m_pMatching( 0 )
	{}
	
	/** construct from EdgeMatching */
	EvaluationContext( const EdgeMatching< UTQLSubgraph, SRGraph >& matching )
		: m_pNodeEdgeAttributes( 0 )
		, m_pPatternAttributes( 0 )
		, m_pMatching( &matching )
	{}
	
	/** returns true if pattern context, i.e. no node/edge */
	bool isGlobal() const
	{ return m_pNodeEdgeAttributes == 0; }
	
	/** returns the pointer to the NodeEdgeAttributes of the current object */
	const KeyValueAttributes* getNodeEdgeAttributes() const
	{ return m_pNodeEdgeAttributes; }

	/** returns the pointer to the PatternAttributes of the current object */
	const PatternAttributes* getNodeEdgePatternAttributes() const
	{ return m_pPatternAttributes; }

	/** returns the pointer to the NodeEdgeAttributes of a node/edge object */
	const KeyValueAttributes* getNodeEdgeAttributes( const std::string& sNodeEdge ) const
	{
		assert( isGlobal() && m_pMatching != 0 );
		 
		EdgeMatching< UTQLSubgraph, SRGraph >::AttributeObjectRefs::const_iterator it 
			= m_pMatching->m_allInputAttributes.find( sNodeEdge );
		if ( it == m_pMatching->m_allInputAttributes.end() )
			return 0;
		else
			return it->second;
	}

	/** returns the EdgeMatching object for special purpose applications */
	const EdgeMatching< UTQLSubgraph, SRGraph >* getMatching() const
	{ return m_pMatching; }
	
protected:
	/** 
	 * Pointer to KeyValueAttributes of a node or edge if predicate/expression is evaluated in 
	 * node/edge context. NULL otherwise.
	 */
	const KeyValueAttributes* m_pNodeEdgeAttributes;
	
	/** 
	 * Pointer to PatternAttributes of a node or edge if predicate/expression is evaluated in 
	 * node/edge context. NULL otherwise.
	 */
	const PatternAttributes* m_pPatternAttributes;
	
	/**
	 * Pointer to an EdgeMatching of a matched pattern. Used to evaluate global predicates and
	 * AttributeExpressions.
	 */
	const EdgeMatching< UTQLSubgraph, SRGraph >* m_pMatching;
};

} } // namespace Ubitrack::Graph

#endif
