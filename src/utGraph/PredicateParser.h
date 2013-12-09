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
 * UTQL predicate parser header file
 *
 * @author Daniel Pustka <daniel.pustka@in.tum.de>
 */
 
#ifndef __Ubitrack_Graph_PredicateParser_H_INCLUDED__
#define __Ubitrack_Graph_PredicateParser_H_INCLUDED__ 

#include <utDataflow.h>
#include "Predicate.h"
 
namespace Ubitrack { namespace Graph {

/**
 * Parses a predicate.
 * @param sPredicate predicate as string
 * @return parsed predicate object ready to be evaluated
 * @throws exception if wrong syntax
 */
UTDATAFLOW_EXPORT boost::shared_ptr< Predicate > parsePredicate( const std::string& sPredicate );

/**
 * Parses an attribute expression.
 * @param sExpression predicate as string
 * @return parsed AttributeExpression object ready to be evaluated
 * @throws exception if wrong syntax
 */
UTDATAFLOW_EXPORT boost::shared_ptr< AttributeExpression > parseAttributeExpression( const std::string& sExpression );

} } // namespace Ubitrack::Graph

#endif

