/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013,2014 Ivan Alonso (Kaian)
 ** Copyright (C) 2013,2014 Irontec SL. All rights reserved.
 **
 ** This program is free software: you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation, either version 3 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **
 ****************************************************************************/
/**
 * @file sip.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage SIP calls and messages
 *
 * This file contains the functions and structures to manage the SIP calls and
 * messages.
 */

#ifndef __SNGREP_SIP_H
#define __SNGREP_SIP_H

#include <pcap.h>
#include <sys/time.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "sip_attr.h"

//! Shorter declaration of sip_call structure
typedef struct sip_call sip_call_t;
//! Shorter declaration of sip_msg structure
typedef struct sip_msg sip_msg_t;
//! Shorter declaration of sip_call_list structure
typedef struct sip_call_list sip_call_list_t;

// Forward struct declaration for attributes
struct sip_attr_hdr;
struct sip_attr;

/**
 * @brief Information of a single message withing a dialog.
 *
 * Most of the data is just stored to be displayed in the UI so
 * the formats may be no the best, but the simplest for this
 * purpose. It also works as a linked lists of messages in a
 * call.
 */
struct sip_msg
{
    //! Message attribute list
    struct sip_attr *attrs;
    //! Timestamp
    struct timeval ts;
    //! Source address
    struct in_addr src;
    //! Source port
    u_short sport;
    //! Destiny address
    struct in_addr dst;
    //! Destiny port
    u_short dport;
    //! Temporal payload data before being parsed
    char *payload;
    //! Color for this message (in color.cseq mode)
    int color;

    //! PCAP Packet Header data
    struct pcap_pkthdr *pcap_header;
    //! PCAP Packet data
    u_char *pcap_packet;
    //! Message owner
    sip_call_t *call;

    //! Messages linked list
    sip_msg_t *next;
};

/**
 * @brief Contains all information of a call and its messages
 *
 * This structure acts as header of messages list of the same
 * callid (considered a dialog). It contains some replicated
 * data from its messages to speed up searches.
 */
struct sip_call
{
    //! Call attribute list
    struct sip_attr *attrs;
    //! List of messages of this call
    sip_msg_t *msgs;
    // Call Lock
    pthread_mutex_t lock;
    //! Calls double linked list
    sip_call_t *next, *prev;
};

/**
 * @brief call structures head list
 *
 * This structure acts as header of calls list
 */
struct sip_call_list
{
    // First and Last calls of the list
    sip_call_t *first;
    sip_call_t *last;
    // Call counter
    int count;
    // Warranty thread-safe access to the calls list
    pthread_mutex_t lock;
};

/**
 * @brief Create a new message from the readed header and payload
 *
 * Allocate required memory for a new SIP message. This function
 * will only store the given information, but wont parse it until
 * needed.
 *
 * @param payload Raw payload content
 * @return a new allocated message
 */
sip_msg_t *
sip_msg_create(const char *payload);

/**
 * @brief Destroy a SIP message and free its memory
 *
 * Deallocate memory of an existing SIP Message.
 * This function will remove the message from the call and the
 * passed pointer will be NULL.
 *
 * @param msg SIP message to be deleted
 */
void
sip_msg_destroy(sip_msg_t *msg);

/**
 * @brief Create a new call with the given callid (Minimum required data)
 *
 * Allocated required memory for a new SIP Call. The call acts as
 * header structure to all the messages with the same callid.
 *
 * @param callid Call-ID Header value
 * @return pointer to the sip_call created
 */
sip_call_t *
sip_call_create(char *callid);

/**
 * @brief Free all related memory from a call and remove from call list
 *
 * Deallocate memory of an existing SIP Call.
 * This will also remove all messages, calling sip_msg_destroy for each
 * one.
 *
 * @param call Call to be destroyed
 */
void
sip_call_destroy(sip_call_t *call);

/**
 * @brief Parses Call-ID header of a SIP message payload
 *
 * Mainly used to check if a payload contains a callid.
 *
 * @param payload SIP message payload
 * @return callid parsed from Call-ID header
 */
char *
sip_get_callid(const char* payload);

/**
 * @brief Loads a new message from raw header/payload
 *
 * Use this function to convert raw data into call and message
 * structures. This is mainly used to load data from a file or
 *
 * @todo This functions should stop using ngrep header format
 *
 * @param header Raw ngrep header
 * @param payload Raw ngrep payload
 * @return a SIP msg structure pointer
 */
sip_msg_t *
sip_load_message(struct timeval tv, struct in_addr src, u_short sport, struct in_addr dst,
                 u_short dport, u_char *payload);

/**
 * @brief Getter for calls linked list size
 *
 * @return how many calls are linked in the list
 */
int
sip_calls_count();

/**
 * @brief Append message to the call's message list
 *
 * Creates a relation between this call and the message, appending it
 * to the end of the message list and setting the message owner.
 *
 * @param call pointer to the call owner of the message
 * @param msg SIP message structure
 */
void
call_add_message(sip_call_t *call, sip_msg_t *msg);

/**
 * @brief Find a call structure in calls linked list given an callid
 *
 *
 *
 * @param callid Call-ID Header value
 * @return pointer to the sip_call structure found or NULL
 */
sip_call_t *
call_find_by_callid(const char *callid);

/**
 * @brief Find a call structure in calls linked list given an xcallid
 *
 * Find the call that have the xcallid attribute equal tot he given
 * value.
 *
 * @param xcallid X-Call-ID or X-CID Header value
 * @return pointer to the sip_call structure found or NULL
 */
sip_call_t *
call_find_by_xcallid(const char *xcallid);

/**
 * @brief Getter for call messages linked list size
 *
 * Return the number of messages stored in this call. All messages
 * share the same Call-ID
 *
 * @param call SIP call structure
 * @return how many messages are in the call
 */
int
call_msg_count(sip_call_t *call);

/**
 * @brief Finds the other leg of this call.
 *
 * If this call has a X-CID or X-Call-ID header, that call will be
 * find and returned. Otherwise, a call with X-CID or X-Call-ID header
 * matching the given call's Call-ID will be find or returned.
 *
 * @param call SIP call structure
 * @return The other call structure or NULL if none found
 */
sip_call_t *
call_get_xcall(sip_call_t *call);

/**
 * @brief Finds the next msg in a call.
 *
 * If the passed msg is NULL it returns the first message
 * in the call
 *
 * @param call SIP call structure
 * @param msg Actual SIP msg from the call (can be NULL)
 * @return Next chronological message in the call
 */
sip_msg_t *
call_get_next_msg(sip_call_t *call, sip_msg_t *msg);

/**
 * @brief Finds the prev msg in a call.
 *
 * If the passed msg is the first message in the call
 * this function will return NULL
 *
 * @param call SIP call structure
 * @param msg Actual SIP msg from the call
 * @return Previous chronological message in the call
 */
sip_msg_t *
call_get_prev_msg(sip_call_t *call, sip_msg_t *msg);

/**
 * @brief Get next call after applying filters and ignores
 *
 * General getter for call list. Never access calls list
 * directly, use this instead.
 *
 * @param cur Current call. Pass NULL to get the first call.
 * @return Next call in the list or NULL if there is no next call
 */
sip_call_t *
call_get_next(sip_call_t *cur);

/**
 * @brief Get previous call after applying filters and ignores
 *
 * General getter for call list. Never access calls list
 * directly, use this instead.
 *
 * @param cur Current call
 * @return Prev call in the list or NULL if there is no previous call
 */
sip_call_t *
call_get_prev(sip_call_t *cur);

/**
 * @brief Update Call State attribute with its last parsed message
 *
 * @param call Call structure to be updated
 */
void
call_update_state(sip_call_t *call);

/**
 * @brief Parse ngrep header line to get timestamps and ip addresses
 *
 * This function will convert the ngrep header line in format:
 *  U DD/MM/YY hh:mm:ss.uuuuuu fff.fff.fff.fff:pppp -> fff.fff.fff.fff:pppp
 *
 * to some attributes.
 *
 * @todo This MUST disappear someday.
 *
 * @param msg SIP message structure
 * @param header ngrep header generated by -qpt arguments
 * @return 0 on success, 1 on malformed header
 */
int
msg_parse_header(sip_msg_t *msg, const char *header);

/**
 * @brief Parse SIP Message payload to fill sip_msg structe
 *
 * Parse the payload content to set message attributes.
 *
 * @param msg SIP message structure
 * @param payload SIP message payload
 * @return 0 in all cases
 */
int
msg_parse_payload(sip_msg_t *msg, const char *payload);


/**
 * @brief Check if Message payload matches a given expression
 *
 * @param msg SIP message structure
 * @param match_expr expression to match
 * @return 1 if message matches, 0 otherwise
 */
int
msg_match_expression(sip_msg_t *msg, const char *match_expr);

/**
 * @brief Check if a package is a retransmission
 *
 * This function will compare its payload with the previous message
 * in the dialog, to check if it has the same content.
 *
 * @param msg SIP message that will be checked
 * @return 1 if the previous message is equal to msg, 0 otherwise
 */
int
msg_is_retrans(sip_msg_t *msg);

/**
 * @brief Get summary of message header data
 *
 * For raw prints, it's handy to have the ngrep header style message
 * data.
 *
 * @param msg SIP message
 * @param out pointer to allocated memory to contain the header output
 * @returns pointer to out
 */
char *
msg_get_header(sip_msg_t *msg, char *out);

/**
 * @brief Remove al calls
 *
 * This funtion will clear the call list invoking the destroy
 * function for each one.
 */
void
sip_calls_clear();

#endif
