#include <algorithm>
#include <assert.h>
#include "Node.h"

namespace raft
{

Nodes::Nodes(NodeId id, bool isVoting) : _me(id)
{
    Node& r = add_node(id, true);
    r.set_voting(isVoting);
}

void Nodes::reset_all_votes()
{
    for (auto& i : _nodes)
        i.vote_for_me(false);
}

void Nodes::set_all_need_vote_req(bool need)
{
    for (Node& i: _nodes)
        i.set_need_vote_req(need);
}


void Nodes::set_all_need_pings(bool need)
{
    for (Node& i : _nodes)
        i.set_need_append_endtries_req(need);
}

bmcl::Option<const Node&> Nodes::get_node(NodeId id) const
{
    const auto& i = std::find_if(_nodes.begin(), _nodes.end(), [id](const Node& i) {return i.get_id() == id; });
    if (i == _nodes.end())
        return bmcl::None;
    return *i;
}

bmcl::Option<Node&> Nodes::get_node(NodeId id)
{
    const auto& i = std::find_if(_nodes.begin(), _nodes.end(), [id](const Node& i) {return i.get_id() == id; });
    if (i == _nodes.end())
        return bmcl::None;
    return *i;
}

const Node& Nodes::get_my_node() const
{
    const auto& n = get_node(_me);
    assert(n.isSome());
    return n.unwrap();
}

Node& Nodes::add_node(NodeId id, bool is_voting)
{   /* set to voting if node already exists */
    bmcl::Option<Node&> node = get_node(id);
    if (node.isSome())
    {
        if (is_voting)
            node->set_voting(true);
        return node.unwrap();
    }

    _nodes.emplace_back(Node(id));
    _nodes.back().set_voting(is_voting);
    std::sort(_nodes.begin(), _nodes.end(), [](const Node& l, const Node& r) { return l.get_id() < r.get_id(); });
    return *std::find_if(_nodes.begin(), _nodes.end(), [id](const Node& n) { return n.get_id() == id; });
}

void Nodes::remove_node(NodeId id)
{
    //assert(id != _me);
    const auto i = std::find_if(_nodes.begin(), _nodes.end(), [id](const Node& i) {return i.get_id() == id; });
    assert(i != _nodes.end());
    _nodes.erase(i);
}

std::size_t Nodes::get_nvotes_for_me(bmcl::Option<NodeId> voted_for, const Nodes& cfg) const
{
    //std::count_if(_nodes.begin(), _nodes.end(), [_me](const Node& i) { return _});
    std::size_t votes = 0;

    for (const Node& i : _nodes)
    {
        for (const Node& j : cfg._nodes)
        {
                if (_me != i.get_id() && i.is_voting() && i.has_vote_for_me() && j.get_id() == i.get_id())
                    votes += 1;
        }
    }
    if (voted_for == _me)
        votes += 1;

    return votes;
}

std::size_t Nodes::get_num_voting_nodes(const Nodes& cfg) const
{
    std::size_t num = 0;
    for (const Node& i : _nodes)
    {
        for (const Node& j : cfg._nodes)
        {
            if (j.get_id() == i.get_id() && i.is_voting())
                num++;
        }
    }
    return num;
}

bool Nodes::votes_has_majority(bmcl::Option<NodeId> voted_for, const Nodes& cfg) const
{
    return votes_has_majority(get_num_voting_nodes(cfg), get_nvotes_for_me(voted_for, cfg));
}

bool Nodes::votes_has_majority(std::size_t num_nodes, std::size_t nvotes)
{
    if (num_nodes < nvotes)
        return false;
    std::size_t half = num_nodes / 2;
    return half + 1 <= nvotes;
}

bool Nodes::is_committed(Index idx, const Nodes& cfg) const
{
    std::size_t votes = 1;
    for (const Node& i : _nodes)
    {
        if (!is_me(i.get_id()) && i.is_voting() && idx <= i.get_match_idx())
        {
            votes++;
        }
    }

    return (get_num_voting_nodes(cfg) / 2 < votes);
}

}
