/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
*
* This file is part of Overpass_API.
*
* Overpass_API is free software: you can redistribute it and/or modify
* it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Overpass_API is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Overpass_API.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <algorithm>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "../../template_db/block_backend.h"
#include "../../template_db/random_file.h"
#include "../core/settings.h"
#include "../data/abstract_processing.h"
#include "../data/collect_members.h"
#include "recurse.h"

using namespace std;

const unsigned int RECURSE_RELATION_RELATION = 1;
const unsigned int RECURSE_RELATION_BACKWARDS = 2;
const unsigned int RECURSE_RELATION_WAY = 3;
const unsigned int RECURSE_RELATION_NODE = 4;
const unsigned int RECURSE_WAY_NODE = 5;
const unsigned int RECURSE_WAY_RELATION = 6;
const unsigned int RECURSE_NODE_RELATION = 7;
const unsigned int RECURSE_NODE_WAY = 8;
const unsigned int RECURSE_DOWN = 9;
const unsigned int RECURSE_DOWN_REL = 10;
const unsigned int RECURSE_UP = 11;
const unsigned int RECURSE_UP_REL = 12;

Generic_Statement_Maker< Recurse_Statement > Recurse_Statement::statement_maker("recurse");

//-----------------------------------------------------------------------------

template< class TIndex, class TObject, class Id_Type >
vector< Id_Type > extract_children_ids(const map< TIndex, vector< TObject > >& elems)
{
  vector< Id_Type > ids;
  
  {
    for (typename map< TIndex, vector< TObject > >::const_iterator
        it(elems.begin()); it != elems.end(); ++it)
    {
      for (typename vector< TObject >::const_iterator it2(it->second.begin());
          it2 != it->second.end(); ++it2)
        ids.push_back(Id_Type(it2->id.val()));
    }
  }
  
  sort(ids.begin(), ids.end());
  
  return ids;
}


template< class TIndex, class TObject >
set< Uint31_Index > extract_parent_indices(const map< TIndex, vector< TObject > >& elems)
{
  vector< uint32 > children;
  {
    for (typename map< TIndex, vector< TObject > >::const_iterator
        it(elems.begin()); it != elems.end(); ++it)
      children.push_back(it->first.val());
  }
  
  vector< uint32 > parents = calc_parents(children);
  
  set< Uint31_Index > req;
  for (vector< uint32 >::const_iterator it = parents.begin(); it != parents.end(); ++it)
    req.insert(Uint31_Index(*it));
  
  return req;
}


template< class TSourceIndex, class TSourceObject >
void collect_relations
    (const Statement& stmt, Resource_Manager& rman,
     const map< TSourceIndex, vector< TSourceObject > >& sources, uint32 source_type,
     map< Uint31_Index, vector< Relation_Skeleton > >& result)
{
  vector< Relation_Entry::Ref_Type > ids = extract_children_ids< TSourceIndex, TSourceObject, Relation_Entry::Ref_Type >(sources);    
  rman.health_check(stmt);
  set< Uint31_Index > req = extract_parent_indices(sources);
  rman.health_check(stmt);

  collect_items_discrete(&stmt, rman, *osm_base_settings().RELATIONS, req,
			 Get_Parent_Rels_Predicate(ids, source_type), result);
}


template< class TSourceIndex, class TSourceObject >
void collect_relations
    (const Statement& stmt, Resource_Manager& rman,
     const map< TSourceIndex, vector< TSourceObject > >& sources, uint32 source_type,
     map< Uint31_Index, vector< Relation_Skeleton > >& result, uint32 role_id)
{
  vector< Relation_Entry::Ref_Type > ids = extract_children_ids< TSourceIndex, TSourceObject, Relation_Entry::Ref_Type >(sources);    
  rman.health_check(stmt);
  set< Uint31_Index > req = extract_parent_indices(sources);
  rman.health_check(stmt);

  collect_items_discrete(&stmt, rman, *osm_base_settings().RELATIONS, req,
                         Get_Parent_Rels_Role_Predicate(ids, source_type, role_id), result);
}


template< class TSourceIndex, class TSourceObject >
void collect_relations
    (const Statement& stmt, Resource_Manager& rman,
     const map< TSourceIndex, vector< TSourceObject > >& sources, uint32 source_type,
     map< Uint31_Index, vector< Relation_Skeleton > >& result,
     const vector< Relation::Id_Type >& ids, bool invert_ids)
{
  vector< Relation_Entry::Ref_Type > children_ids = extract_children_ids< TSourceIndex, TSourceObject, Relation_Entry::Ref_Type >(sources);    
  rman.health_check(stmt);
  set< Uint31_Index > req = extract_parent_indices(sources);
  rman.health_check(stmt);

  if (!invert_ids)
    collect_items_discrete(&stmt, rman, *osm_base_settings().RELATIONS, req,
        And_Predicate< Relation_Skeleton,
	    Id_Predicate< Relation_Skeleton >, Get_Parent_Rels_Predicate >
	    (Id_Predicate< Relation_Skeleton >(ids),
            Get_Parent_Rels_Predicate(children_ids, source_type)), result);
  else
    collect_items_discrete(&stmt, rman, *osm_base_settings().RELATIONS, req,
        And_Predicate< Relation_Skeleton,
	    Not_Predicate< Relation_Skeleton, Id_Predicate< Relation_Skeleton > >,
	    Get_Parent_Rels_Predicate >
	    (Not_Predicate< Relation_Skeleton, Id_Predicate< Relation_Skeleton > >
	      (Id_Predicate< Relation_Skeleton >(ids)),
            Get_Parent_Rels_Predicate(children_ids, source_type)), result);
}


template< class TSourceIndex, class TSourceObject >
void collect_relations
    (const Statement& stmt, Resource_Manager& rman,
     const map< TSourceIndex, vector< TSourceObject > >& sources, uint32 source_type,
     map< Uint31_Index, vector< Relation_Skeleton > >& result,
     const vector< Relation::Id_Type >& ids, bool invert_ids, uint32 role_id)
{
  vector< Relation_Entry::Ref_Type > children_ids = extract_children_ids< TSourceIndex, TSourceObject, Relation_Entry::Ref_Type >(sources);    
  rman.health_check(stmt);
  set< Uint31_Index > req = extract_parent_indices(sources);
  rman.health_check(stmt);

  if (!invert_ids)
    collect_items_discrete(&stmt, rman, *osm_base_settings().RELATIONS, req,
        And_Predicate< Relation_Skeleton,
            Id_Predicate< Relation_Skeleton >, Get_Parent_Rels_Role_Predicate >
            (Id_Predicate< Relation_Skeleton >(ids),
            Get_Parent_Rels_Role_Predicate(children_ids, source_type, role_id)), result);
  else
    collect_items_discrete(&stmt, rman, *osm_base_settings().RELATIONS, req,
        And_Predicate< Relation_Skeleton,
            Not_Predicate< Relation_Skeleton, Id_Predicate< Relation_Skeleton > >,
            Get_Parent_Rels_Role_Predicate >
            (Not_Predicate< Relation_Skeleton, Id_Predicate< Relation_Skeleton > >
              (Id_Predicate< Relation_Skeleton >(ids)),
            Get_Parent_Rels_Role_Predicate(children_ids, source_type, role_id)), result);
}


void collect_relations
    (const Statement& stmt, Resource_Manager& rman,
     const map< Uint31_Index, vector< Relation_Skeleton > >& sources,
     map< Uint31_Index, vector< Relation_Skeleton > >& result)
{
  vector< Uint64 > ids = extract_children_ids< Uint31_Index, Relation_Skeleton, Uint64 >(sources);    
  rman.health_check(stmt);
  
  collect_items_flat(stmt, rman, *osm_base_settings().RELATIONS,
      Get_Parent_Rels_Predicate(ids, Relation_Entry::RELATION), result);
}


void collect_relations
    (const Statement& stmt, Resource_Manager& rman,
     const map< Uint31_Index, vector< Relation_Skeleton > >& sources,
     map< Uint31_Index, vector< Relation_Skeleton > >& result, uint32 role_id)
{
  vector< Uint64 > ids = extract_children_ids< Uint31_Index, Relation_Skeleton, Uint64 >(sources);    
  rman.health_check(stmt);
  
  collect_items_flat(stmt, rman, *osm_base_settings().RELATIONS,
      Get_Parent_Rels_Role_Predicate(ids, Relation_Entry::RELATION, role_id), result);
}


void collect_relations
    (const Statement& stmt, Resource_Manager& rman,
     const map< Uint31_Index, vector< Relation_Skeleton > >& sources,
     map< Uint31_Index, vector< Relation_Skeleton > >& result,
     const vector< Relation::Id_Type >& ids, bool invert_ids)
{
  vector< Uint64 > children_ids = extract_children_ids< Uint31_Index, Relation_Skeleton, Uint64 >(sources);    
  rman.health_check(stmt);
  
  if (!invert_ids)
    collect_items_flat(stmt, rman, *osm_base_settings().RELATIONS,
        And_Predicate< Relation_Skeleton,
	    Id_Predicate< Relation_Skeleton >, Get_Parent_Rels_Predicate >
	    (Id_Predicate< Relation_Skeleton >(ids),
            Get_Parent_Rels_Predicate(children_ids, Relation_Entry::RELATION)),
        result);
  else
    collect_items_flat(stmt, rman, *osm_base_settings().RELATIONS,
        And_Predicate< Relation_Skeleton,
	    Not_Predicate< Relation_Skeleton, Id_Predicate< Relation_Skeleton > >,
	    Get_Parent_Rels_Predicate >
	    (Not_Predicate< Relation_Skeleton, Id_Predicate< Relation_Skeleton > >
	      (Id_Predicate< Relation_Skeleton >(ids)),
            Get_Parent_Rels_Predicate(children_ids, Relation_Entry::RELATION)),
        result);
}


void collect_relations
    (const Statement& stmt, Resource_Manager& rman,
     const map< Uint31_Index, vector< Relation_Skeleton > >& sources,
     map< Uint31_Index, vector< Relation_Skeleton > >& result,
     const vector< Relation::Id_Type >& ids, bool invert_ids, uint32 role_id)
{
  vector< Uint64 > children_ids = extract_children_ids< Uint31_Index, Relation_Skeleton, Uint64 >(sources);    
  rman.health_check(stmt);
  
  if (!invert_ids)
    collect_items_flat(stmt, rman, *osm_base_settings().RELATIONS,
        And_Predicate< Relation_Skeleton,
            Id_Predicate< Relation_Skeleton >, Get_Parent_Rels_Role_Predicate >
            (Id_Predicate< Relation_Skeleton >(ids),
            Get_Parent_Rels_Role_Predicate(children_ids, Relation_Entry::RELATION, role_id)),
        result);
  else
    collect_items_flat(stmt, rman, *osm_base_settings().RELATIONS,
        And_Predicate< Relation_Skeleton,
            Not_Predicate< Relation_Skeleton, Id_Predicate< Relation_Skeleton > >,
            Get_Parent_Rels_Role_Predicate >
            (Not_Predicate< Relation_Skeleton, Id_Predicate< Relation_Skeleton > >
              (Id_Predicate< Relation_Skeleton >(ids)),
            Get_Parent_Rels_Role_Predicate(children_ids, Relation_Entry::RELATION, role_id)),
        result);
}


void collect_ways
    (const Statement& stmt, Resource_Manager& rman,
     const map< Uint32_Index, vector< Node_Skeleton > >& nodes,
     map< Uint31_Index, vector< Way_Skeleton > >& result)
{
  vector< Uint64 > ids = extract_children_ids< Uint32_Index, Node_Skeleton, Uint64 >(nodes);    
  rman.health_check(stmt);
  set< Uint31_Index > req = extract_parent_indices(nodes);
  rman.health_check(stmt);
  
  collect_items_discrete(&stmt, rman, *osm_base_settings().WAYS, req,
      Get_Parent_Ways_Predicate(ids), result);
}

void collect_ways
    (const Statement& stmt, Resource_Manager& rman,
     const map< Uint32_Index, vector< Node_Skeleton > >& nodes,
     map< Uint31_Index, vector< Way_Skeleton > >& result,
     const vector< Way::Id_Type >& ids, bool invert_ids)
{
  vector< Uint64 > children_ids = extract_children_ids< Uint32_Index, Node_Skeleton, Uint64 >(nodes);    
  rman.health_check(stmt);
  set< Uint31_Index > req = extract_parent_indices(nodes);
  rman.health_check(stmt);
  
  if (!invert_ids)
    collect_items_discrete(&stmt, rman, *osm_base_settings().WAYS, req,
        And_Predicate< Way_Skeleton,
	    Id_Predicate< Way_Skeleton >, Get_Parent_Ways_Predicate >
	    (Id_Predicate< Way_Skeleton >(ids), Get_Parent_Ways_Predicate(children_ids)), result);
  else
    collect_items_discrete(&stmt, rman, *osm_base_settings().WAYS, req,
        And_Predicate< Way_Skeleton,
	    Not_Predicate< Way_Skeleton, Id_Predicate< Way_Skeleton > >,
	    Get_Parent_Ways_Predicate >
	    (Not_Predicate< Way_Skeleton, Id_Predicate< Way_Skeleton > >
	      (Id_Predicate< Way_Skeleton >(ids)),
	     Get_Parent_Ways_Predicate(children_ids)), result);
}


void collect_nodes(const Statement& query, Resource_Manager& rman,
		   const map< Uint31_Index, vector< Relation_Skeleton > >& rels,
		   const set< pair< Uint32_Index, Uint32_Index > >& ranges,
		   const vector< Node::Id_Type >& ids, bool invert_ids,
		   map< Uint32_Index, vector< Node_Skeleton > >& nodes)
{
  if (ranges.empty())
  {
    if (ids.empty())
      nodes = relation_node_members(&query, rman, rels);
    else
      nodes = relation_node_members(&query, rman, rels, 0, &ids, invert_ids);
  }
  else
  {
    if (ids.empty())
      nodes = relation_node_members(&query, rman, rels, &ranges);
    else
      nodes = relation_node_members(&query, rman, rels, &ranges, &ids, invert_ids);
  }
}


void collect_nodes(const Statement& query, Resource_Manager& rman,
                   const map< Uint31_Index, vector< Relation_Skeleton > >& rels,
                   const set< pair< Uint32_Index, Uint32_Index > >& ranges,
                   const vector< Node::Id_Type >& ids, bool invert_ids,
                   map< Uint32_Index, vector< Node_Skeleton > >& nodes,
                   uint32 role_id)
{
  if (ranges.empty())
  {
    if (ids.empty())
      nodes = relation_node_members(&query, rman, rels, 0, 0, false, &role_id);
    else
      nodes = relation_node_members(&query, rman, rels, 0, &ids, invert_ids, &role_id);
  }
  else
  {
    if (ids.empty())
      nodes = relation_node_members(&query, rman, rels, &ranges, 0, false, &role_id);
    else
      nodes = relation_node_members(&query, rman, rels, &ranges, &ids, invert_ids, &role_id);
  }
}


void collect_nodes(const Statement& query, Resource_Manager& rman,
		   const map< Uint31_Index, vector< Way_Skeleton > >& rels,
		   const set< pair< Uint32_Index, Uint32_Index > >& ranges,
		   const vector< Node::Id_Type >& ids, bool invert_ids,
		   map< Uint32_Index, vector< Node_Skeleton > >& nodes)
{
  if (ranges.empty())
  {
    if (ids.empty())
      nodes = way_members(&query, rman, rels);
    else if (!invert_ids)
      nodes = way_members(&query, rman, rels, 0, &ids);
    else
      nodes = way_members(&query, rman, rels, 0, &ids, invert_ids);
  }
  else
  {
    if (ids.empty())
      nodes = way_members(&query, rman, rels, &ranges);
    else if (!invert_ids)
      nodes = way_members(&query, rman, rels, &ranges, &ids);
    else
      nodes = way_members(&query, rman, rels, 0, &ids, invert_ids);
  }
}


template< typename First, typename Second >
void swap_components(std::pair< First, Second > pair, First& first, Second& second)
{
  first.swap(pair.first);
  second.swap(pair.second);
}


void collect_nodes(const Statement& query, Resource_Manager& rman,
                   const map< Uint31_Index, vector< Way_Skeleton > >& rels,
                   const map< Uint31_Index, vector< Attic< Way_Skeleton > > >& attic_rels,
                   const set< pair< Uint32_Index, Uint32_Index > >& ranges,
                   const vector< Node::Id_Type >& ids, bool invert_ids,
                   uint64 timestamp,
                   map< Uint32_Index, vector< Node_Skeleton > >& nodes,
                   map< Uint32_Index, vector< Attic< Node_Skeleton > > >& attic_nodes)
{
  if (ranges.empty())
  {
    if (ids.empty())
      swap_components(way_members(&query, rman, rels, attic_rels, timestamp), nodes, attic_nodes);
    else if (!invert_ids)
      swap_components(way_members(&query, rman, rels, attic_rels, timestamp, 0, &ids), nodes, attic_nodes);
    else
      swap_components(way_members(&query, rman, rels, attic_rels, timestamp, 0, &ids, invert_ids),
                      nodes, attic_nodes);
  }
  else
  {
    if (ids.empty())
      swap_components(way_members(&query, rman, rels, attic_rels, timestamp, &ranges), nodes, attic_nodes);
    else if (!invert_ids)
      swap_components(way_members(&query, rman, rels, attic_rels, timestamp, &ranges, &ids),
                      nodes, attic_nodes);
    else
      swap_components(way_members(&query, rman, rels, attic_rels, timestamp, 0, &ids, invert_ids),
                      nodes, attic_nodes);
  }
}


void collect_ways(const Statement& query, Resource_Manager& rman,
		  const map< Uint31_Index, vector< Relation_Skeleton > >& rels,
		  const set< pair< Uint31_Index, Uint31_Index > >& ranges,
		  const vector< Way::Id_Type >& ids, bool invert_ids,
		  map< Uint31_Index, vector< Way_Skeleton > >& ways)
{
  if (ranges.empty())
  {
    if (ids.empty())
      ways = relation_way_members(&query, rman, rels);
    else
      ways = relation_way_members(&query, rman, rels, 0, &ids, invert_ids);
  }
  else
  {
    if (ids.empty())
      ways = relation_way_members(&query, rman, rels, &ranges);
    else
      ways = relation_way_members(&query, rman, rels, &ranges, &ids, invert_ids);
  }
}


void collect_ways(const Statement& query, Resource_Manager& rman,
                  const map< Uint31_Index, vector< Relation_Skeleton > >& rels,
                  const set< pair< Uint31_Index, Uint31_Index > >& ranges,
                  const vector< Way::Id_Type >& ids, bool invert_ids,
                  map< Uint31_Index, vector< Way_Skeleton > >& ways,
                  uint32 role_id)
{
  if (ranges.empty())
  {
    if (ids.empty())
      ways = relation_way_members(&query, rman, rels, 0, 0, false, &role_id);
    else
      ways = relation_way_members(&query, rman, rels, 0, &ids, invert_ids, &role_id);
  }
  else
  {
    if (ids.empty())
      ways = relation_way_members(&query, rman, rels, &ranges, 0, false, &role_id);
    else
      ways = relation_way_members(&query, rman, rels, &ranges, &ids, invert_ids, &role_id);
  }
}


void collect_relations(const Statement& query, Resource_Manager& rman,
		  const map< Uint31_Index, vector< Relation_Skeleton > >& rels,
		  const set< pair< Uint31_Index, Uint31_Index > >& ranges,
		  const vector< Relation::Id_Type >& ids, bool invert_ids,
		  map< Uint31_Index, vector< Relation_Skeleton > >& relations)
{
  if (ranges.empty())
  {
    if (ids.empty())
      relations = relation_relation_members(query, rman, rels);
    else
      relations = relation_relation_members(query, rman, rels, 0, &ids, invert_ids);
  }
  else
  {
    if (ids.empty())
      relations = relation_relation_members(query, rman, rels, &ranges);
    else
      relations = relation_relation_members(query, rman, rels, &ranges, &ids, invert_ids);
  }
}


void collect_relations(const Statement& query, Resource_Manager& rman,
                  const map< Uint31_Index, vector< Relation_Skeleton > >& rels,
                  const set< pair< Uint31_Index, Uint31_Index > >& ranges,
                  const vector< Relation::Id_Type >& ids, bool invert_ids,
                  map< Uint31_Index, vector< Relation_Skeleton > >& relations,
                  uint32 role_id)
{
  if (ranges.empty())
  {
    if (ids.empty())
      relations = relation_relation_members(query, rman, rels, 0, 0, false, &role_id);
    else
      relations = relation_relation_members(query, rman, rels, 0, &ids, invert_ids, &role_id);
  }
  else
  {
    if (ids.empty())
      relations = relation_relation_members(query, rman, rels, &ranges, 0, false, &role_id);
    else
      relations = relation_relation_members(query, rman, rels, &ranges, &ids, invert_ids, &role_id);
  }
}


uint count_relations(const map< Uint31_Index, vector< Relation_Skeleton > >& relations)
{
  uint result = 0;
  for (map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator it = relations.begin();
      it != relations.end(); ++it)
    result += it->second.size();
  return result;
}


void relations_loop(const Statement& query, Resource_Manager& rman,
		    map< Uint31_Index, vector< Relation_Skeleton > > source,
		    map< Uint31_Index, vector< Relation_Skeleton > >& result)
{
  uint old_rel_count = count_relations(source);
  while (true)
  {
    result = relation_relation_members(query, rman, source);
    indexed_set_union(result, source);
    uint new_rel_count = count_relations(result);
    if (new_rel_count == old_rel_count)
      return;
    old_rel_count = new_rel_count;
    source.swap(result);
  }
}

void relations_up_loop(const Statement& query, Resource_Manager& rman,
		    map< Uint31_Index, vector< Relation_Skeleton > > source,
		    map< Uint31_Index, vector< Relation_Skeleton > >& result)
{
  uint old_rel_count = count_relations(source);
  while (true)
  {
    result.clear();
    collect_relations(query, rman, source, result);
    indexed_set_union(result, source);
    uint new_rel_count = count_relations(result);
    if (new_rel_count == old_rel_count)
      return;
    old_rel_count = new_rel_count;
    source.swap(result);
  }
}

//-----------------------------------------------------------------------------

class Recurse_Constraint : public Query_Constraint
{
  public:
    Recurse_Constraint(Recurse_Statement& stmt_) : stmt(&stmt_) {}

    bool delivers_data() { return true; }
    
    virtual bool get_data(const Statement& query, Resource_Manager& rman, Set& into,
                          const set< pair< Uint32_Index, Uint32_Index > >& ranges,
                          const vector< Node::Id_Type >& ids,
                          bool invert_ids, uint64 timestamp);
    virtual bool get_data(const Statement& query, Resource_Manager& rman, Set& into,
                          const set< pair< Uint31_Index, Uint31_Index > >& ranges,
                          int type,
                          const vector< Uint32_Index >& ids,
                          bool invert_ids, uint64 timestamp);
    void filter(Resource_Manager& rman, Set& into, uint64 timestamp);
    void filter(const Statement& query, Resource_Manager& rman, Set& into, uint64 timestamp);
    virtual ~Recurse_Constraint() {}
    
  private:
    Recurse_Statement* stmt;
};

bool Recurse_Constraint::get_data
    (const Statement& query, Resource_Manager& rman, Set& into,
     const set< pair< Uint32_Index, Uint32_Index > >& ranges,
     const vector< Node::Id_Type >& ids,
     bool invert_ids, uint64 timestamp)
{
  map< string, Set >::const_iterator mit = rman.sets().find(stmt->get_input());
  if (mit == rman.sets().end())
    return true;
  
  if (timestamp == NOW)
  {
    if (stmt->get_role())
    {
      uint32 role_id = determine_role_id(*rman.get_transaction(), *stmt->get_role());
      if (role_id == numeric_limits< uint32 >::max())
        return true;
    
      if (stmt->get_type() == RECURSE_RELATION_NODE)
        ::collect_nodes(query, rman, mit->second.relations, ranges, ids, invert_ids, into.nodes, role_id);
      return true;
    }
  
    if (stmt->get_type() == RECURSE_RELATION_NODE)
      ::collect_nodes(query, rman, mit->second.relations, ranges, ids, invert_ids, into.nodes);
    else if (stmt->get_type() == RECURSE_WAY_NODE)
      ::collect_nodes(query, rman, mit->second.ways, ranges, ids, invert_ids, into.nodes);
    else if (stmt->get_type() == RECURSE_DOWN)
    {
      map< Uint32_Index, vector< Node_Skeleton > > rel_nodes;
      map< Uint31_Index, vector< Way_Skeleton > > rel_ways;
      ::collect_nodes(query, rman, mit->second.relations, ranges, ids, invert_ids, rel_nodes);
      rel_ways = relation_way_members(&query, rman, mit->second.relations);
      ::collect_nodes(query, rman, rel_ways, ranges, ids, invert_ids, into.nodes);
      indexed_set_union(into.nodes, rel_nodes);
    }
    else if (stmt->get_type() == RECURSE_DOWN_REL)
    {
      map< Uint31_Index, vector< Relation_Skeleton > > rel_rels;
      relations_loop(query, rman, mit->second.relations, rel_rels);
      map< Uint32_Index, vector< Node_Skeleton > > rel_nodes;
      map< Uint31_Index, vector< Way_Skeleton > > rel_ways;
      ::collect_nodes(query, rman, rel_rels, ranges, ids, invert_ids, rel_nodes);
      rel_ways = relation_way_members(&query, rman, rel_rels);
      ::collect_nodes(query, rman, rel_ways, ranges, ids, invert_ids, into.nodes);
      indexed_set_union(into.nodes, rel_nodes);
    }
  }
  else
  {
//     if (stmt->get_role())
//     {
//       uint32 role_id = determine_role_id(*rman.get_transaction(), *stmt->get_role());
//       if (role_id == numeric_limits< uint32 >::max())
//         return true;
//     
//       if (stmt->get_type() == RECURSE_RELATION_NODE)
//         ::collect_nodes(query, rman, mit->second.relations, ranges, ids, invert_ids, into.nodes, role_id);
//       return true;
//     }
  
//     if (stmt->get_type() == RECURSE_RELATION_NODE)
//       ::collect_nodes(query, rman, mit->second.relations, ranges, ids, invert_ids, into.nodes);
    /*else*/ if (stmt->get_type() == RECURSE_WAY_NODE)
      ::collect_nodes(query, rman, mit->second.ways, mit->second.attic_ways,
                      ranges, ids, invert_ids, timestamp, into.nodes, into.attic_nodes);
//     else if (stmt->get_type() == RECURSE_DOWN)
//     {
//       map< Uint32_Index, vector< Node_Skeleton > > rel_nodes;
//       map< Uint31_Index, vector< Way_Skeleton > > rel_ways;
//       ::collect_nodes(query, rman, mit->second.relations, ranges, ids, invert_ids, rel_nodes);
//       rel_ways = relation_way_members(&query, rman, mit->second.relations);
//       ::collect_nodes(query, rman, rel_ways, ranges, ids, invert_ids, into.nodes);
//       indexed_set_union(into.nodes, rel_nodes);
//     }
//     else if (stmt->get_type() == RECURSE_DOWN_REL)
//     {
//       map< Uint31_Index, vector< Relation_Skeleton > > rel_rels;
//       relations_loop(query, rman, mit->second.relations, rel_rels);
//       map< Uint32_Index, vector< Node_Skeleton > > rel_nodes;
//       map< Uint31_Index, vector< Way_Skeleton > > rel_ways;
//       ::collect_nodes(query, rman, rel_rels, ranges, ids, invert_ids, rel_nodes);
//       rel_ways = relation_way_members(&query, rman, rel_rels);
//       ::collect_nodes(query, rman, rel_ways, ranges, ids, invert_ids, into.nodes);
//       indexed_set_union(into.nodes, rel_nodes);
//     }
  }    
  return true;
}

bool Recurse_Constraint::get_data
    (const Statement& query, Resource_Manager& rman, Set& into,
     const set< pair< Uint31_Index, Uint31_Index > >& ranges,
     int type,
     const vector< Uint32_Index >& ids,
     bool invert_ids, uint64 timestamp)
{
  map< string, Set >::const_iterator mit = rman.sets().find(stmt->get_input());
  if (mit == rman.sets().end())
    return true;
  
  if (stmt->get_role())
  {
    uint32 role_id = determine_role_id(*rman.get_transaction(), *stmt->get_role());
    if (role_id == numeric_limits< uint32 >::max())
      return true;

    if (stmt->get_type() == RECURSE_RELATION_WAY)
    {
      collect_ways(query, rman, mit->second.relations, ranges, ids, invert_ids, into.ways, role_id);
      return true;
    }    
    else if (stmt->get_type() == RECURSE_RELATION_RELATION)
    {
      collect_relations(query, rman, mit->second.relations, ranges,
                        ids, invert_ids, into.relations, role_id);
      return true;
    }
    else if (stmt->get_type() == RECURSE_NODE_RELATION)
    {
      if (ids.empty())
        collect_relations(query, rman, mit->second.nodes, Relation_Entry::NODE, into.relations, role_id);
      else
        collect_relations(query, rman, mit->second.nodes, Relation_Entry::NODE, into.relations, ids, invert_ids, role_id);
      return true;
    }
    else if (stmt->get_type() == RECURSE_WAY_RELATION)
    {
      if (ids.empty())
        collect_relations(query, rman, mit->second.ways, Relation_Entry::WAY, into.relations, role_id);
      else
        collect_relations(query, rman, mit->second.ways, Relation_Entry::WAY,
                        into.relations, ids, invert_ids, role_id);
      return true;
    }
    else if (stmt->get_type() == RECURSE_RELATION_BACKWARDS)
    {
      if (ids.empty())
        collect_relations(query, rman, mit->second.relations, into.relations, role_id);
      else
        collect_relations(query, rman, mit->second.relations, into.relations,
                        ids, invert_ids, role_id);
      return true;
    }
    return false;
  }
  
  if (stmt->get_type() == RECURSE_RELATION_WAY)
  {
    collect_ways(query, rman, mit->second.relations, ranges, ids, invert_ids, into.ways);
    return true;
  }    
  else if (stmt->get_type() == RECURSE_RELATION_RELATION)
  {
    collect_relations(query, rman, mit->second.relations, ranges,
		      ids, invert_ids, into.relations);
    return true;
  }
  else if (stmt->get_type() == RECURSE_DOWN)
  {
    if (type != QUERY_WAY)
      return true;
    collect_ways(query, rman, mit->second.relations, ranges, ids, invert_ids, into.ways);
    return true;
  }
  else if (stmt->get_type() == RECURSE_DOWN_REL)
  {
    map< Uint31_Index, vector< Relation_Skeleton > > rel_rels;
    relations_loop(query, rman, mit->second.relations, rel_rels);
    if (type == QUERY_WAY)
      collect_ways(query, rman, rel_rels, ranges, ids, invert_ids, into.ways);
    else
    {
      if (!ids.empty())
      {
	if (!invert_ids)
	  filter_items
	      (Id_Predicate< Relation_Skeleton >(ids), rel_rels);
	else
	  filter_items
	      (Not_Predicate< Relation_Skeleton, Id_Predicate< Relation_Skeleton > >
	        (Id_Predicate< Relation_Skeleton >(ids)), rel_rels);
      }
      into.relations.swap(rel_rels);
    }
    return true;
  }
  else if (stmt->get_type() == RECURSE_NODE_WAY)
  {
    if (ids.empty())
      collect_ways(query, rman, mit->second.nodes, into.ways);
    else
      collect_ways(query, rman, mit->second.nodes, into.ways, ids, invert_ids);
    return true;
  }
  else if (stmt->get_type() == RECURSE_NODE_RELATION)
  {
    if (ids.empty())
      collect_relations(query, rman, mit->second.nodes, Relation_Entry::NODE, into.relations);
    else
      collect_relations(query, rman, mit->second.nodes, Relation_Entry::NODE, into.relations, ids, invert_ids);
    return true;
  }
  else if (stmt->get_type() == RECURSE_WAY_RELATION)
  {
    if (ids.empty())
      collect_relations(query, rman, mit->second.ways, Relation_Entry::WAY, into.relations);
    else
      collect_relations(query, rman, mit->second.ways, Relation_Entry::WAY,
			into.relations, ids, invert_ids);
    return true;
  }
  else if (stmt->get_type() == RECURSE_RELATION_BACKWARDS)
  {
    if (ids.empty())
      collect_relations(query, rman, mit->second.relations, into.relations);
    else
      collect_relations(query, rman, mit->second.relations, into.relations,
			ids, invert_ids);
    return true;
  }
  else if (stmt->get_type() == RECURSE_UP)
  {
    if (type == QUERY_WAY)
    {
      if (ids.empty())
	collect_ways(query, rman, mit->second.nodes, into.ways);
      else
	collect_ways(query, rman, mit->second.nodes, into.ways,
		     ids, invert_ids);
    }
    else
    {
      map< Uint31_Index, vector< Way_Skeleton > > rel_ways = mit->second.ways;
      map< Uint31_Index, vector< Way_Skeleton > > node_ways;
      collect_ways(query, rman, mit->second.nodes, node_ways);    
      indexed_set_union(rel_ways, node_ways);
      if (ids.empty())
	collect_relations(query, rman, rel_ways, Relation_Entry::WAY, into.relations);
      else
	collect_relations(query, rman, rel_ways, Relation_Entry::WAY, into.relations,
			  ids, invert_ids);
      
      map< Uint31_Index, vector< Relation_Skeleton > > node_rels;
      if (ids.empty())
	collect_relations(query, rman, mit->second.nodes, Relation_Entry::NODE, node_rels);
      else
	collect_relations(query, rman, mit->second.nodes, Relation_Entry::NODE, node_rels,
			  ids, invert_ids);
      indexed_set_union(into.relations, node_rels);
    }
    return true;
  }
  else if (stmt->get_type() == RECURSE_UP_REL)
  {
    if (type == QUERY_WAY)
    {
      if (ids.empty())
	collect_ways(query, rman, mit->second.nodes, into.ways);
      else
	collect_ways(query, rman, mit->second.nodes, into.ways, ids, invert_ids);
    }
    else
    {
      map< Uint31_Index, vector< Way_Skeleton > > rel_ways = mit->second.ways;
      map< Uint31_Index, vector< Way_Skeleton > > node_ways;
      collect_ways(query, rman, mit->second.nodes, node_ways);    
      indexed_set_union(rel_ways, node_ways);
      map< Uint31_Index, vector< Relation_Skeleton > > way_rels;
      collect_relations(query, rman, rel_ways, Relation_Entry::WAY, way_rels);
    
      map< Uint31_Index, vector< Relation_Skeleton > > rel_rels = mit->second.relations;
      indexed_set_union(rel_rels, way_rels);
    
      map< Uint31_Index, vector< Relation_Skeleton > > node_rels;
      collect_relations(query, rman, mit->second.nodes, Relation_Entry::NODE, node_rels);
      indexed_set_union(rel_rels, node_rels);
    
      relations_up_loop(query, rman, rel_rels, into.relations);

      if (!ids.empty())
      {
	if (!invert_ids)
	  filter_items(Id_Predicate< Relation_Skeleton >(ids), into.relations);
	else
	  filter_items
	      (Not_Predicate< Relation_Skeleton, Id_Predicate< Relation_Skeleton > >
	        (Id_Predicate< Relation_Skeleton >(ids)), into.relations);
      }
    }
    return true;
  }
  return false;
}

void Recurse_Constraint::filter(Resource_Manager& rman, Set& into, uint64 timestamp)
{
  map< string, Set >::const_iterator mit = rman.sets().find(stmt->get_input());
  if (mit == rman.sets().end())
  {
    into.nodes.clear();
    into.ways.clear();
    into.relations.clear();
    into.areas.clear();
    return;
  }
  
  if (stmt->get_type() == RECURSE_DOWN || stmt->get_type() == RECURSE_DOWN_REL)
    return;
  
  if (stmt->get_role())
  {
    uint32 role_id = determine_role_id(*rman.get_transaction(), *stmt->get_role());
    if (role_id == numeric_limits< uint32 >::max())
      return;

    vector< Node::Id_Type > ids;
    if (stmt->get_type() == RECURSE_RELATION_NODE)
      ids = relation_node_member_ids(rman, mit->second.relations, &role_id);
  
    filter_items(Id_Predicate< Node_Skeleton >(ids), into.nodes);
  
    if (stmt->get_type() == RECURSE_RELATION_WAY)
    {
      vector< Way::Id_Type > ids = relation_way_member_ids(rman, mit->second.relations, &role_id);
      filter_items(Id_Predicate< Way_Skeleton >(ids), into.ways);
    }
    else
      into.ways.clear();
    
    if (stmt->get_type() == RECURSE_UP || stmt->get_type() == RECURSE_UP_REL)
      return;
  
    ids.clear();
    if (stmt->get_type() == RECURSE_RELATION_RELATION)
    {
      vector< Relation::Id_Type > ids = relation_relation_member_ids(rman, mit->second.relations, &role_id);
      filter_items(Id_Predicate< Relation_Skeleton >(ids), into.relations);
    }
    else if (stmt->get_type() == RECURSE_NODE_RELATION
        || stmt->get_type() == RECURSE_WAY_RELATION
        || stmt->get_type() == RECURSE_RELATION_BACKWARDS)
    {
      uint32 source_type;    
      if (stmt->get_type() == RECURSE_NODE_RELATION)
        source_type = Relation_Entry::NODE;
      else if (stmt->get_type() == RECURSE_WAY_RELATION)
        source_type = Relation_Entry::WAY;
      else if (stmt->get_type() == RECURSE_RELATION_BACKWARDS)
        source_type = Relation_Entry::RELATION;
    
      vector< Relation_Entry::Ref_Type > ids;
      if (stmt->get_type() == RECURSE_NODE_RELATION)
        ids = extract_children_ids< Uint32_Index, Node_Skeleton, Relation_Entry::Ref_Type >(mit->second.nodes);
      else if (stmt->get_type() == RECURSE_WAY_RELATION)
        ids = extract_children_ids< Uint31_Index, Way_Skeleton, Relation_Entry::Ref_Type >(mit->second.ways);
      else if (stmt->get_type() == RECURSE_RELATION_BACKWARDS)
        ids = extract_children_ids< Uint31_Index, Relation_Skeleton, Relation_Entry::Ref_Type >(mit->second.relations);
    
      filter_items(Get_Parent_Rels_Role_Predicate(ids, source_type, role_id), into.relations);
    }
    else
      into.relations.clear();
  
    return;
  }
  
  vector< Node::Id_Type > ids;
  if (stmt->get_type() == RECURSE_WAY_NODE)
  {
    if (timestamp == NOW)
      ids = way_nd_ids(mit->second.ways);
    else
      ids = way_nd_ids(mit->second.ways, mit->second.attic_ways);
    rman.health_check(*stmt);
  }
  else if (stmt->get_type() == RECURSE_RELATION_NODE)
    ids = relation_node_member_ids(rman, mit->second.relations);
  
  filter_items(Id_Predicate< Node_Skeleton >(ids), into.nodes);
  filter_items(Id_Predicate< Node_Skeleton >(ids), into.attic_nodes);
  
  if (stmt->get_type() == RECURSE_RELATION_WAY)
  {
    vector< Way::Id_Type > ids = relation_way_member_ids(rman, mit->second.relations);
    filter_items(Id_Predicate< Way_Skeleton >(ids), into.ways);
  }
  else if (stmt->get_type() == RECURSE_NODE_WAY || stmt->get_type() == RECURSE_UP
      || stmt->get_type() == RECURSE_UP_REL)
  {
    vector< Node::Id_Type > ids = extract_children_ids< Uint32_Index, Node_Skeleton, Node::Id_Type >(mit->second.nodes);
    filter_items(Get_Parent_Ways_Predicate(ids), into.ways);
  }
  else
    into.ways.clear();
    
  if (stmt->get_type() == RECURSE_UP || stmt->get_type() == RECURSE_UP_REL)
    return;
  
  ids.clear();
  if (stmt->get_type() == RECURSE_RELATION_RELATION)
  {
    vector< Relation::Id_Type > ids = relation_relation_member_ids(rman, mit->second.relations);
    filter_items(Id_Predicate< Relation_Skeleton >(ids), into.relations);
  }
  else if (stmt->get_type() == RECURSE_NODE_RELATION
      || stmt->get_type() == RECURSE_WAY_RELATION
      || stmt->get_type() == RECURSE_RELATION_BACKWARDS)
  {
    uint32 source_type;    
    if (stmt->get_type() == RECURSE_NODE_RELATION)
      source_type = Relation_Entry::NODE;
    else if (stmt->get_type() == RECURSE_WAY_RELATION)
      source_type = Relation_Entry::WAY;
    else if (stmt->get_type() == RECURSE_RELATION_BACKWARDS)
      source_type = Relation_Entry::RELATION;
    
    vector< Relation_Entry::Ref_Type > ids;
    if (stmt->get_type() == RECURSE_NODE_RELATION)
      ids = extract_children_ids< Uint32_Index, Node_Skeleton, Relation_Entry::Ref_Type >(mit->second.nodes);
    else if (stmt->get_type() == RECURSE_WAY_RELATION)
      ids = extract_children_ids< Uint31_Index, Way_Skeleton, Relation_Entry::Ref_Type >(mit->second.ways);
    else if (stmt->get_type() == RECURSE_RELATION_BACKWARDS)
      ids = extract_children_ids< Uint31_Index, Relation_Skeleton, Relation_Entry::Ref_Type >(mit->second.relations);
    
    filter_items(Get_Parent_Rels_Predicate(ids, source_type), into.relations);
  }
  else
    into.relations.clear();
  
  //TODO: areas
}

void Recurse_Constraint::filter(const Statement& query, Resource_Manager& rman, Set& into, uint64 timestamp)
{
  map< string, Set >::const_iterator mit = rman.sets().find(stmt->get_input());
  
  if (stmt->get_type() != RECURSE_DOWN && stmt->get_type() != RECURSE_DOWN_REL)
    return;
  
  if (stmt->get_type() == RECURSE_DOWN)
  {
    vector< Node::Id_Type > rel_ids
        = relation_node_member_ids(rman, mit->second.relations);
    map< Uint31_Index, vector< Way_Skeleton > > intermediate_ways;
    collect_ways(query, rman, mit->second.relations, set< pair< Uint31_Index, Uint31_Index > >(),
		 vector< Way::Id_Type >(), false, intermediate_ways);
    vector< Node::Id_Type > way_ids = way_nd_ids(intermediate_ways);
    rman.health_check(*stmt);
    
    vector< Node::Id_Type > ids;
    set_union(way_ids.begin(), way_ids.end(), rel_ids.begin(), rel_ids.end(), back_inserter(ids));
  
    filter_items(Id_Predicate< Node_Skeleton >(ids), into.nodes);

    filter_items(Id_Predicate< Way_Skeleton >(relation_way_member_ids(rman, mit->second.relations)),
		 into.ways);
    
    into.relations.clear();
  }
  else if (stmt->get_type() == RECURSE_DOWN_REL)
  {
    map< Uint31_Index, vector< Relation_Skeleton > > rel_rels;
    relations_loop(query, rman, mit->second.relations, rel_rels);
    vector< Node::Id_Type > rel_ids
        = relation_node_member_ids(rman, rel_rels);
    map< Uint31_Index, vector< Way_Skeleton > > intermediate_ways;
    collect_ways(query, rman, rel_rels, set< pair< Uint31_Index, Uint31_Index > >(),
		 vector< Way::Id_Type >(), false, intermediate_ways);
    vector< Node::Id_Type > way_ids = way_nd_ids(intermediate_ways);
    rman.health_check(*stmt);
    
    vector< Node::Id_Type > ids;
    set_union(way_ids.begin(), way_ids.end(), rel_ids.begin(), rel_ids.end(), back_inserter(ids));
  
    filter_items(Id_Predicate< Node_Skeleton >(ids), into.nodes);

    filter_items(Id_Predicate< Way_Skeleton >(relation_way_member_ids(rman, rel_rels)), into.ways);

    filter_items(Id_Predicate< Relation_Skeleton >(filter_for_ids(rel_rels)), into.relations);
  }
  else if (stmt->get_type() == RECURSE_UP && !into.relations.empty())
  {
    map< Uint31_Index, vector< Way_Skeleton > > rel_ways = mit->second.ways;
    map< Uint31_Index, vector< Way_Skeleton > > node_ways;
    collect_ways(query, rman, mit->second.nodes, node_ways);    
    indexed_set_union(rel_ways, node_ways);
    
    vector< Relation_Entry::Ref_Type > node_ids = extract_children_ids< Uint32_Index, Node_Skeleton, Relation_Entry::Ref_Type >(mit->second.nodes);
    vector< Relation_Entry::Ref_Type > way_ids = extract_children_ids< Uint31_Index, Way_Skeleton, Relation_Entry::Ref_Type >(rel_ways);
    
    filter_items(
        Or_Predicate< Relation_Skeleton, Get_Parent_Rels_Predicate, Get_Parent_Rels_Predicate >
        (Get_Parent_Rels_Predicate(node_ids, Relation_Entry::NODE),
	 Get_Parent_Rels_Predicate(way_ids, Relation_Entry::WAY)), into.relations);
  }
  else if (stmt->get_type() == RECURSE_UP_REL && !into.relations.empty())
  {
    map< Uint31_Index, vector< Way_Skeleton > > rel_ways = mit->second.ways;
    map< Uint31_Index, vector< Way_Skeleton > > node_ways;
    collect_ways(query, rman, mit->second.nodes, node_ways);    
    indexed_set_union(rel_ways, node_ways);
    map< Uint31_Index, vector< Relation_Skeleton > > way_rels;
    collect_relations(query, rman, rel_ways, Relation_Entry::WAY, way_rels);
    
    map< Uint31_Index, vector< Relation_Skeleton > > rel_rels = mit->second.relations;
    indexed_set_union(rel_rels, way_rels);
    
    map< Uint31_Index, vector< Relation_Skeleton > > node_rels;
    collect_relations(query, rman, mit->second.nodes, Relation_Entry::NODE, node_rels);
    indexed_set_union(rel_rels, node_rels);
    
    relations_up_loop(query, rman, rel_rels, rel_rels);

    vector< Relation::Id_Type > ids = extract_children_ids< Uint31_Index, Relation_Skeleton, Relation::Id_Type >(rel_rels);
    filter_items(Id_Predicate< Relation_Skeleton >(ids), into.relations);
  }
  
  //TODO: areas
}

//-----------------------------------------------------------------------------

Recurse_Statement::Recurse_Statement
    (int line_number_, const map< string, string >& input_attributes, Query_Constraint* bbox_limitation)
    : Output_Statement(line_number_), restrict_to_role(false)
{
  map< string, string > attributes;
  
  attributes["from"] = "_";
  attributes["into"] = "_";
  attributes["type"] = "";
  attributes["role"] = "";
  attributes["role-restricted"] = "no";
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
  input = attributes["from"];
  set_output(attributes["into"]);
  
  if (attributes["type"] == "relation-relation")
    type = RECURSE_RELATION_RELATION;
  else if (attributes["type"] == "relation-backwards")
    type = RECURSE_RELATION_BACKWARDS;
  else if (attributes["type"] == "relation-way")
    type = RECURSE_RELATION_WAY;
  else if (attributes["type"] == "relation-node")
    type = RECURSE_RELATION_NODE;
  else if (attributes["type"] == "way-node")
    type = RECURSE_WAY_NODE;
  else if (attributes["type"] == "down")
    type = RECURSE_DOWN;
  else if (attributes["type"] == "down-rel")
    type = RECURSE_DOWN_REL;
  else if (attributes["type"] == "way-relation")
    type = RECURSE_WAY_RELATION;
  else if (attributes["type"] == "node-relation")
    type = RECURSE_NODE_RELATION;
  else if (attributes["type"] == "node-way")
    type = RECURSE_NODE_WAY;
  else if (attributes["type"] == "up")
    type = RECURSE_UP;
  else if (attributes["type"] == "up-rel")
    type = RECURSE_UP_REL;
  else
  {
    type = 0;
    ostringstream temp;
    temp<<"For the attribute \"type\" of the element \"recurse\""
	<<" the only allowed values are \"relation-relation\", \"relation-backwards\","
	<<"\"relation-way\", \"relation-node\", \"way-node\", \"way-relation\","
	<<"\"node-relation\", \"node-way\", \"down\", \"down-rel\", \"up\", or \"up-rel\".";
    add_static_error(temp.str());
  }
  if (attributes["role"] != "" || attributes["role-restricted"] == "yes")
  {
    if (type != RECURSE_RELATION_RELATION && type != RECURSE_RELATION_BACKWARDS
        && type != RECURSE_RELATION_WAY && type != RECURSE_WAY_RELATION
        && type != RECURSE_RELATION_NODE && type != RECURSE_NODE_RELATION)
    {
      ostringstream temp;
      temp<<"A role can only be specified for values \"relation-relation\", \"relation-backwards\","
          <<"\"relation-way\", \"relation-node\", \"way-relation\","
          <<"or \"node-relation\".";
      add_static_error(temp.str());
    }
    else
    {
      role = attributes["role"];
      restrict_to_role = true;
    }
  }
}


void Recurse_Statement::execute(Resource_Manager& rman)
{
  Set into;
  
  map< string, Set >::const_iterator mit(rman.sets().find(input));
  if (mit == rman.sets().end())
  {
    transfer_output(rman, into);
    return;
  }

  if (restrict_to_role)
  {
    uint32 role_id = determine_role_id(*rman.get_transaction(), role);
    if (role_id == numeric_limits< uint32 >::max())
    {
      transfer_output(rman, into);
      return;
    }
        
    if (type == RECURSE_RELATION_RELATION)
      into.relations = relation_relation_members(*this, rman, mit->second.relations,
                                                 0, 0, false, &role_id);
    else if (type == RECURSE_RELATION_WAY)
      into.ways = relation_way_members(this, rman, mit->second.relations,
                                       0, 0, false, &role_id);
    else if (type == RECURSE_RELATION_NODE)
      into.nodes = relation_node_members(this, rman, mit->second.relations,
                                         0, 0, false, &role_id);
    else if (type == RECURSE_RELATION_BACKWARDS)
      collect_relations(*this, rman, mit->second.relations, into.relations, role_id);
    else if (type == RECURSE_WAY_RELATION)
      collect_relations(*this, rman, mit->second.ways, Relation_Entry::WAY, into.relations, role_id);
    else if (type == RECURSE_NODE_RELATION)
      collect_relations(*this, rman, mit->second.nodes, Relation_Entry::NODE, into.relations, role_id);
  }
  else if (type == RECURSE_RELATION_RELATION)
    into.relations = relation_relation_members(*this, rman, mit->second.relations);
  else if (type == RECURSE_RELATION_BACKWARDS)
    collect_relations(*this, rman, mit->second.relations, into.relations);
  else if (type == RECURSE_RELATION_WAY)
    into.ways = relation_way_members(this, rman, mit->second.relations);
  else if (type == RECURSE_RELATION_NODE)
    into.nodes = relation_node_members(this, rman, mit->second.relations);
  else if (type == RECURSE_WAY_NODE)
  {
    if (rman.get_desired_timestamp() == NOW)
      into.nodes = way_members(this, rman, mit->second.ways);
    else
    {
      std::pair< std::map< Uint32_Index, std::vector< Node_Skeleton > >,
         std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > > > all_nodes
         = way_members(this, rman, mit->second.ways, mit->second.attic_ways, rman.get_desired_timestamp());
      into.nodes.swap(all_nodes.first);
      into.attic_nodes.swap(all_nodes.second);
    }
  }
  else if (type == RECURSE_DOWN)
  {
    map< Uint32_Index, vector< Node_Skeleton > > rel_nodes
        = relation_node_members(this, rman, mit->second.relations);
    into.ways = relation_way_members(this, rman, mit->second.relations);
    map< Uint31_Index, vector< Way_Skeleton > > source_ways = mit->second.ways;
    indexed_set_union(source_ways, into.ways);
    into.nodes = way_members(this, rman, source_ways);
    indexed_set_union(into.nodes, rel_nodes);
  }
  else if (type == RECURSE_DOWN_REL)
  {
    relations_loop(*this, rman, mit->second.relations, into.relations);    
    map< Uint32_Index, vector< Node_Skeleton > > rel_nodes
        = relation_node_members(this, rman, into.relations);
    into.ways = relation_way_members(this, rman, into.relations);
    map< Uint31_Index, vector< Way_Skeleton > > source_ways = mit->second.ways;
    indexed_set_union(source_ways, into.ways);
    into.nodes = way_members(this, rman, source_ways);
    indexed_set_union(into.nodes, rel_nodes);
  }
  else if (type == RECURSE_WAY_RELATION)
    collect_relations(*this, rman, mit->second.ways, Relation_Entry::WAY, into.relations);
  else if (type == RECURSE_NODE_WAY)
    collect_ways(*this, rman, mit->second.nodes, into.ways);
  else if (type == RECURSE_NODE_RELATION)
    collect_relations(*this, rman, mit->second.nodes, Relation_Entry::NODE, into.relations);
  else if (type == RECURSE_UP)
  {
    map< Uint31_Index, vector< Way_Skeleton > > rel_ways = mit->second.ways;
    collect_ways(*this, rman, mit->second.nodes, into.ways);
    
    indexed_set_union(rel_ways, into.ways);
    collect_relations(*this, rman, rel_ways, Relation_Entry::WAY, into.relations);
    
    map< Uint31_Index, vector< Relation_Skeleton > > node_rels;
    collect_relations(*this, rman, mit->second.nodes, Relation_Entry::NODE, node_rels);
    indexed_set_union(into.relations, node_rels);
  }
  else if (type == RECURSE_UP_REL)
  {
    map< Uint31_Index, vector< Way_Skeleton > > rel_ways = mit->second.ways;
    collect_ways(*this, rman, mit->second.nodes, into.ways);
    
    map< Uint31_Index, vector< Relation_Skeleton > > rel_rels = mit->second.relations;
    map< Uint31_Index, vector< Relation_Skeleton > > way_rels;
    indexed_set_union(rel_ways, into.ways);
    collect_relations(*this, rman, rel_ways, Relation_Entry::WAY, way_rels);
    indexed_set_union(rel_rels, way_rels);
    
    map< Uint31_Index, vector< Relation_Skeleton > > node_rels;
    collect_relations(*this, rman, mit->second.nodes, Relation_Entry::NODE, node_rels);
    indexed_set_union(rel_rels, node_rels);

    relations_up_loop(*this, rman, rel_rels, into.relations);    
  }

  transfer_output(rman, into);
  rman.health_check(*this);
}

Recurse_Statement::~Recurse_Statement()
{
  for (vector< Query_Constraint* >::const_iterator it = constraints.begin();
      it != constraints.end(); ++it)
    delete *it;
}

Query_Constraint* Recurse_Statement::get_query_constraint()
{
  constraints.push_back(new Recurse_Constraint(*this));
  return constraints.back();
}
