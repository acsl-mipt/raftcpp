/**
 * Copyright (c) 2013, Willem-Hendrik Thiart
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * @file
 * @author Willem Thiart himself@willemthiart.com
 */

#pragma once
#include <bmcl/Option.h>
#include "raft/Ids.h"
#include "raft/Entry.h"
#include "raft/Error.h"
#include "raft/Storage.h"


namespace raft
{

enum class ReqVoteState : uint8_t
{
    UnknownNode,
    NotGranted,
    Granted,
};

const char* to_string(ReqVoteState vote);

/** Entry message response.
 * Indicates to client if entry was committed or not. */
struct MsgAddEntryRep
{
    MsgAddEntryRep(TermId term, EntryId id, Index idx) : term(term), id(id), idx(idx){}
    TermId  term;   /**< the entry's term */
    EntryId id;     /**< the entry's unique ID */
    Index   idx;    /**< the entry's index */
};

/** Vote request message.
 * Sent to nodes when a server wants to become leader.
 * This message could force a leader/candidate to become a follower. */
struct MsgVoteReq
{
    MsgVoteReq(TermId term, Index last_log_idx, TermId last_log_term, bool isPre)
        :term(term), last_log_idx(last_log_idx), last_log_term(last_log_term), isPre(isPre)
    {
    }
    TermId term;               /**< currentTerm, to force other leader/candidate to step down */
    Index  last_log_idx;       /**< index of candidate's last log entry */
    TermId last_log_term;      /**< term of candidate's last log entry */
    bool   isPre;              /**< true for prevote phase */
};

/** Vote request response message.
 * Indicates if node has accepted the server's vote request. */
struct MsgVoteRep
{
    MsgVoteRep(TermId term, ReqVoteState vote) : term(term), vote_granted(vote) {}
    TermId term;                   /**< currentTerm, for candidate to update itself */
    ReqVoteState vote_granted;     /**< true means candidate received vote */
};

/** Appendentries message.
 * This message is used to tell nodes if it's safe to apply entries to the FSM.
 * Can be sent without any entries as a keep alive message.
 * This message could force a leader/candidate to become a follower. */

struct MsgAppendEntriesReq
{
    MsgAppendEntriesReq(TermId term) : term(term), prev_log_term(TermId(0)), leader_commit(0), last_cfg_seen(0) {}
    MsgAppendEntriesReq(TermId term, TermId prev_log_term, Index leader_commit, Index last_cfg_seen, DataHandler data = DataHandler())
        : term(term), prev_log_term(prev_log_term), leader_commit(leader_commit), last_cfg_seen(last_cfg_seen), data(data) {}
    TermId  term;           /**< currentTerm, to force other leader/candidate to step down */
    TermId  prev_log_term;  /**< the term of the log just before the newest entry for the node who receives this message */
    Index   leader_commit;  /**< the index of the entry that has been appended to the majority of the cluster. Entries up to this index will be applied to the FSM */
    Index   last_cfg_seen;  /**< last cfg change met in log, which is need to reuse node ids. set to 0, to disable it*/

    DataHandler data;
};

/** Appendentries response message.
 * Can be sent without any entries as a keep alive message.
 * This message could force a leader/candidate to become a follower. */
struct MsgAppendEntriesRep
{
    MsgAppendEntriesRep(TermId term, bool success, Index current_idx)
        : term(term), success(success), current_idx(current_idx) {}
    TermId term;           /**< currentTerm, to force other leader/candidate to step down */
    bool success;               /**< true if follower contained entry matching prevLogidx and prevLogTerm */

    /* Non-Raft fields follow: */
    /* Having the following fields allows us to do less book keeping in regards to full fledged RPC */

    Index current_idx;    /**< This is the highest log IDX we've received and appended to our log */
} ;

class ISender
{
public:
    virtual ~ISender();

    /** Callback for sending request vote messages to all cluster's members */
    virtual bmcl::Option<Error> request_vote(const NodeId& node, const MsgVoteReq& msg) = 0;

    /** Callback for sending appendentries messages */
    virtual bmcl::Option<Error> append_entries(const NodeId& node, const MsgAppendEntriesReq& msg) = 0;
};

class IEventHandler
{
public:
    virtual ~IEventHandler();

    virtual void become_leader() {}
    virtual void become_follower() {}
    virtual void become_candidate() {}
    virtual void become_precandidate() {}
    virtual void radomize_timeouts() {}

    virtual void rcvd(NodeId from, const MsgAppendEntriesReq&) {}
    virtual void rcvd(NodeId from, const MsgAppendEntriesRep&) {}
    virtual void rcvd(NodeId from, const MsgVoteReq&) {}
    virtual void rcvd(NodeId from, const MsgVoteRep&) {}

    virtual void send(NodeId to, const MsgAppendEntriesReq&) {}
    virtual void send(NodeId to, const MsgAppendEntriesRep&) {}
    virtual void send(NodeId to, const MsgVoteReq&) {}
    virtual void send(NodeId to, const MsgVoteRep&) {}

    virtual void entry_rcvd(const Entry&) {}
    virtual void entry_stored(Index entry_idx, const Entry&) {}
    virtual void entry_poped(Index entry_idx, const Entry&) {}
    virtual void entry_applied(Index entry_idx, const Entry&) {}
};
}
