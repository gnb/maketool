/*
 * Maketool - GTK-based front end for gmake
 * Copyright (c) 1999-2001 Greg Banks
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

#include "mqueue.h"
#include <gdk/gdkx.h>

CVSID("$Id: mqueue.c,v 1.2 2002-09-24 14:13:52 gnb Exp $");

typedef struct
{
    unsigned long msgid;
    unsigned long from;    	/* XID of window to reply to */
    unsigned long length;
} MessageHeader;

typedef struct
{
    unsigned long msgid;
    long code;
} ReplyHeader;


static GdkAtom message_atom;
static GdkAtom reply_atom;
static GdkWindow *self_window;
static GList *receiveq;
static GList *replyq;
static unsigned long last_msgid = 0;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
mq_init(GdkWindow *self)
{
    message_atom = gdk_atom_intern("_MAKETOOL_MESSAGE", FALSE);
    reply_atom = gdk_atom_intern("_MAKETOOL_REPLY", FALSE);
    self_window = self;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static GdkWindow *
lookup_window(unsigned long xid)
{
    GdkWindow *win;
    
    if ((win = gdk_window_lookup(xid)) != 0)
    	gdk_window_ref(win);
    else
    	win = gdk_window_foreign_new(xid);
	
    return win;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


void
mq_send(GdkWindow *to, Message *msg)
{
    estring bits;
    MessageHeader hdr;
    
    msg->msgid = hdr.msgid = ++last_msgid;
    hdr.from = GDK_WINDOW_XWINDOW(self_window);
    hdr.length = msg->body.length;
	
    estring_init(&bits);
    estring_append_chars(&bits, (const char *)&hdr, sizeof(hdr));
    if (msg->body.length > 0)
	estring_append_chars(&bits, msg->body.data, msg->body.length);
    
    gdk_property_change(to,
    	    	    	message_atom,
			message_atom,
			/*format*/8,
			GDK_PROP_MODE_APPEND,
			(guchar *)bits.data, bits.length);
			
    estring_free(&bits);
}


Message *
mq_receive(void)
{
    Message *msg = 0;
    
    if (receiveq != 0)
    {
	msg = (Message *)receiveq->data;
	receiveq = g_list_remove_link(receiveq, receiveq);
    }
        
    return msg;    
}


gboolean
mq_message_event(GdkEvent *event)
{
    MessageHeader hdr;
    Message *msg;
    GdkAtom type = GDK_NONE;
    gint format = 0;
    gint length = 0;
    guchar *data = 0;
    int offset;

    /*
     * Check if this is the kind of event we want.
     */
    if (event->type != GDK_PROPERTY_NOTIFY ||
    	event->property.atom != message_atom ||
	event->property.state != GDK_PROPERTY_NEW_VALUE)
    	return FALSE;
	
    if (!gdk_property_get(
    	    self_window,
	    message_atom,   	    /* property */
	    message_atom,    	    /* type */
	    0,	    	    	    /* offset */
	    1024,   	    	    /* length */
	    TRUE,   	    	    /* pdelete */
	    &type,
	    &format,
	    &length,
	    &data))
	return TRUE;
	
    if (type != message_atom ||
    	format != 8 ||
	data == 0)
    {
    	if (data != 0)
	    g_free(data);
    	return TRUE;
    }

    /* Note that we always get back a handy extra trailing nul byte from XLib */
    length--;
    
    /* Parse the property data into a sequence of header/body pairs. */
    for (offset = 0 ; 
    	 offset+sizeof(hdr) < length ; 
	 )
    {
    	/* deal with alignment issue by copying */
    	memcpy(&hdr, data+offset, sizeof(hdr));
	offset += sizeof(hdr);
	
	msg = mq_msg_new_d((const char *)data+offset, hdr.length);
    	msg->msgid = hdr.msgid;
	msg->from = lookup_window(hdr.from);
		
	receiveq = g_list_append(receiveq, msg);
	
	offset += hdr.length;
    }
    
    g_free(data);
    
    return TRUE;
}



/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
mq_reply(const Message *msg, long code)
{
    ReplyHeader reply;
    
    reply.msgid = msg->msgid;
    reply.code = code;
	
    gdk_property_change(msg->from,
    	    	    	reply_atom,
			reply_atom,
			/*format*/8,
			GDK_PROP_MODE_APPEND,
			(guchar *)&reply, sizeof(reply));
}


gboolean
mq_get_reply(const Message *msg, long *codep)
{
    GList *list;

    for (list = replyq ; list != 0 ; list = list->next)
    {
	ReplyHeader *reply = (ReplyHeader *)list->data;
	
	if (reply->msgid == msg->msgid)
	{
	    if (codep != 0)
	    	*codep = reply->code;
	    replyq = g_list_remove_link(replyq, list);
	    g_free(reply);
	    return TRUE;
	}
    }
    
    return FALSE;
}


gboolean
mq_reply_event(GdkEvent *event)
{
    GdkAtom type = GDK_NONE;
    gint format = 0;
    gint length = 0;
    guchar *data = 0;
    int offset;
    ReplyHeader reply;
    
    if (event->type != GDK_PROPERTY_NOTIFY ||
	event->property.atom != reply_atom ||
	event->property.state != GDK_PROPERTY_NEW_VALUE)
    	return FALSE;

    if (!gdk_property_get(
    	    self_window,
	    reply_atom,   	    /* property */
	    reply_atom,    	    /* type */
	    0,	    	    	    /* offset */
	    1024,   	    	    /* length */
	    TRUE,   	    	    /* pdelete */
	    &type,
	    &format,
	    &length,
	    &data))
	return TRUE;

    if (type != reply_atom ||
    	format != 8 ||
	data == 0)
    {
    	if (data != 0)
	    g_free(data);
    	return TRUE;
    }
    
    /* Parse the property data into a sequence of header/body pairs. */
    for (offset = 0 ; 
    	 offset < length ; 
	 offset += sizeof(reply))
    {
	replyq = g_list_append(replyq, g_memdup(data+offset, sizeof(reply)));
    }
    
#if DEBUG
    {
    	GList *list;
	
	fprintf(stderr, "mq_reply_event: replyq =");
	for (list = replyq ; list != 0 ; list = list->next)
	{
	    ReplyHeader *r = (ReplyHeader *)list->data;

	    fprintf(stderr, " { msgid=%ld code=%ld }", r->msgid, r->code);
	}
	fprintf(stderr, "\n");
    }   
#endif

    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

Message *
mq_msg_new_d(const char *data, int len)
{
    Message *msg;
    
    msg = g_new(Message, 1);
    memset(msg, 0, sizeof(*msg));
    
    if (len != 0)
    	estring_append_chars(&msg->body, data, len);
    
    return msg;
}

Message *
mq_msg_new_e(const estring *e)
{
    return (e == 0 ? mq_msg_new_d(0, 0) : mq_msg_new_d(e->data, e->length));
}

void
mq_msg_delete(Message *msg)
{
    if (msg->from != 0)
    	gdk_window_unref(msg->from);
    estring_free(&msg->body);
    g_free(msg);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
