/*
 * Maketool - GTK-based front end for gmake
 * Copyright (c) 1999-2003 Greg Banks
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _mqueue_h_
#define _mqueue_h_

#include "common.h"
#include "util.h"
#include <gdk/gdk.h>

typedef struct
{
    unsigned long msgid;
    GdkWindow *from;    	/* XID of window to reply to */
    estring body;
} Message;


/*
 * Both sender and receiver call mq_init() once at startup time
 * with their own window.
 *
 * To send a message, the sender constructs a Message
 * by initialising a Message struct with mq_msg_init(),
 * adding the message to msg.body using estring functions,
 * and sending it with mq_send().
 *
 * The receiver is waiting for a GDK_PROPERTY_NOTIFY
 * event, and when it is received calls mq_message_event()
 * with the event structure.  This function returns non-zero
 * if any new messages are waiting to be received.  If so,
 * the receiver calls mq_receive() and handles the newly
 * arrived messages, until mq_receive() returns FALSE.
 * Note that the receiver does not have to call mq_msg_free()
 * between calls, mq_receive() will handle that.
 *
 * For each message received, the receiver replies using
 * mq_reply().  Meanwhile, the sender is waiting for a
 * GDK_PROPERTY_NOTIFY event on its own window, and when
 * it arrives calls first mq_reply_event() and then
 * mq_get_reply().
 */

/* initialisation */
void mq_init(GdkWindow *self);
 
/* for sender */
void mq_send(GdkWindow *to, Message *msg);
gboolean mq_reply_event(GdkEvent *event);
gboolean mq_get_reply(const Message *msg, long *codep);

/* for receiver */
gboolean mq_message_event(GdkEvent *event);
Message *mq_receive(void);
void mq_reply(const Message *msg, long code);

/* utility */
Message *mq_msg_new_d(const char *data, int len);
Message *mq_msg_new_e(const estring *);
void mq_msg_delete(Message *msg);


#endif /* _mqueue_h_ */
