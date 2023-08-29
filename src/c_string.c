#include "server.h"
#include <math.h> /* isnan(), isinf() */

void incrTopCommand(client *c, long long incr, long long top) {
    long long value, oldvalue;
    robj *o, *new;

    o = lookupKeyWrite(c->db,c->argv[1]);
    if (checkType(c,o,OBJ_STRING)) return;
    if (getLongLongFromObjectOrReply(c,o,&value,NULL) != C_OK) return;

    oldvalue = value;
    if (oldvalue >= top) {
        addReplyLongLong(c, -1);
        return;
    }

    if ((incr < 0 && oldvalue < 0 && incr < (LLONG_MIN-oldvalue)) ||
        (incr > 0 && oldvalue > 0 && incr > (LLONG_MAX-oldvalue))) {
        addReplyError(c,"increment or decrement would overflow");
        return;
    }
    value += incr;
    if (value > top) {
        addReplyLongLong(c, -1);
        return;
    }

    if (o && o->refcount == 1 && o->encoding == OBJ_ENCODING_INT &&
        (value < 0 || value >= OBJ_SHARED_INTEGERS) &&
        value >= LONG_MIN && value <= LONG_MAX)
    {
        new = o;
        o->ptr = (void*)((long)value);
    } else {
        new = createStringObjectFromLongLongForValue(value);
        if (o) {
            dbReplaceValue(c->db,c->argv[1],new);
        } else {
            dbAdd(c->db,c->argv[1],new);
        }
    }
    signalModifiedKey(c,c->db,c->argv[1]);
    notifyKeyspaceEvent(NOTIFY_STRING,"incrby",c->argv[1],c->db->id);
    server.dirty++;
    addReplyLongLong(c, value);
}

void decrTopCommand(client *c, long long incr, long long bottom) {
    long long value, oldvalue;
    robj *o, *new;

    o = lookupKeyWrite(c->db,c->argv[1]);
    if (checkType(c,o,OBJ_STRING)) return;
    if (getLongLongFromObjectOrReply(c,o,&value,NULL) != C_OK) return;
    oldvalue = value;
    if (oldvalue <= bottom) {
        addReplyLongLong(c, -1);
        return;
    }
    
    if ((incr < 0 && oldvalue < 0 && incr < (LLONG_MIN-oldvalue)) ||
        (incr > 0 && oldvalue > 0 && incr > (LLONG_MAX-oldvalue))) {
        addReplyError(c,"increment or decrement would overflow");
        return;
    }
    value += incr;
    if (value < bottom) {
        addReplyLongLong(c, -1);
        return;
    }

    if (o && o->refcount == 1 && o->encoding == OBJ_ENCODING_INT &&
        (value < 0 || value >= OBJ_SHARED_INTEGERS) &&
        value >= LONG_MIN && value <= LONG_MAX)
    {
        new = o;
        o->ptr = (void*)((long)value);
    } else {
        new = createStringObjectFromLongLongForValue(value);
        if (o) {
            dbReplaceValue(c->db,c->argv[1],new);
        } else {
            dbAdd(c->db,c->argv[1],new);
        }
    }
    signalModifiedKey(c,c->db,c->argv[1]);
    notifyKeyspaceEvent(NOTIFY_STRING,"incrby",c->argv[1],c->db->id);
    server.dirty++;
    addReplyLongLong(c, value);
}

void incrtopCommand(client *c) {
    long long top;

    if (getLongLongFromObjectOrReply(c, c->argv[2], &top, NULL) != C_OK) return;

    incrTopCommand(c,1,top);
}

void decrtopCommand(client *c) {
    long long bottom;

    if (getLongLongFromObjectOrReply(c, c->argv[2], &bottom, NULL) != C_OK) return;

    decrTopCommand(c, -1, bottom);
}

void incrbytopCommand(client *c) {
    long long incr;

    long long top;

    if (getLongLongFromObjectOrReply(c, c->argv[2], &incr, NULL) != C_OK) return;

    if (getLongLongFromObjectOrReply(c, c->argv[3], &top, NULL) != C_OK) return;

    incrTopCommand(c, incr, top);
}

void decrbytopCommand(client *c) {
    long long incr;

    long long bottom;

    if (getLongLongFromObjectOrReply(c, c->argv[2], &incr, NULL) != C_OK) return;

    if (getLongLongFromObjectOrReply(c, c->argv[3], &bottom, NULL) != C_OK) return;
    /* Overflow check: negating LLONG_MIN will cause an overflow */
    if (incr == LLONG_MIN) {
        addReplyError(c, "decrement would overflow");
        return;
    }
    decrTopCommand(c, -incr, bottom);
}